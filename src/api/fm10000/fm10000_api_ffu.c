/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_ffu.c
 * Creation Date:  April 29, 2013
 * Description:    Low-level API for manipulating the Filtering &
 *                 Forwarding Unit
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Intel Corporation nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* FFU Action Command definition */
#define FM_ACTION_COMMAND_ROUTE_ARP   0
#define FM_ACTION_COMMAND_ROUTE_GLORT 1
#define FM_ACTION_COMMAND_BIT_SET     2
#define FM_ACTION_COMMAND_FIELD_SET   3

/* FFU Action SubCommand definition for BIT_SET */
#define FM_ACTION_SUBCMD_FLAG         0
#define FM_ACTION_SUBCMD_TRIG         1
#define FM_ACTION_SUBCMD_USR          2

/* FFU Action SubCommand definition for FIELD_SET */
#define FM_ACTION_SUBCMD_SET_VLAN     0
#define FM_ACTION_SUBCMD_SET_DSCP     1

/* FFU Flag definition for SubCommand FLAG */
#define FM_ACTION_FLAG_DROP           (1 << 0)
#define FM_ACTION_FLAG_TRAP           (1 << 1)
#define FM_ACTION_FLAG_LOG            (1 << 2)
#define FM_ACTION_FLAG_NOROUTE        (1 << 3)
#define FM_ACTION_FLAG_RXMIRROR       (1 << 4)
#define FM_ACTION_FLAG_CAPTURE_TIME   (1 << 5)

/* Arp Count limitation */
#define FM_ACTION_ARP_CNT_MINRANGE_MINVALUE  1
#define FM_ACTION_ARP_CNT_MINRANGE_MAXVALUE  16
#define FM_ACTION_ARP_CNT_MAXRANGE_MINVALUE  0
#define FM_ACTION_ARP_CNT_MAXRANGE_MAXVALUE  12


#define FM_API_REQUIRE(expr, failCode)       \
    if ( !(expr) )                           \
    {                                        \
        err = failCode;                      \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU,  \
                            failCode);       \
    }


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* Generic selection values are one byte in length, which limits them to 256
 * unique values.  Hardware selection mux is 6-bits wide, limiting hardware
 * to 64 unique values. */
static fm_int  GenericToMuxMap[256][FM_FFU_SELECTS_PER_MINSLICE];
static fm_int  MuxToGenericMap[FM_FFU_SELECTS_PER_MINSLICE][64];
static fm_bool MuxMapsLoaded = FALSE;

typedef fm_status (*fm_writeReg32Seq)(fm_int     sw,
                                      fm_uint32 *addr,
                                      fm_uint32 *value,
                                      fm_int     n);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmSupportsFfu
 * \ingroup intLowlevFFU10k
 *
 * \desc            Determines whether the specified switch has an FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          TRUE if the switch has an FFU.
 * \return          FALSE if the switch does not have an FFU.
 *
 *****************************************************************************/
static inline fm_bool fmSupportsFfu(fm_int sw)
{
    return ( (fmRootApi->fmSwitchStateTable[sw]->switchFamily ==
              FM_SWITCH_FAMILY_FM10000) );

}   /* end fmSupportsFfu */




/*****************************************************************************/
/** SetActionFlag
 * \ingroup intLowlevFFU10k
 *
 * \desc            Update the fm_ffuActionFlags structure.
 *
 * \param[out]      flags refer to the structure to update.
 * 
 * \param[in]       flag refer to the flag value.
 * 
 * \param[in]       bits refer to the structure field to update.
 *
 * \return          None.
 *
 *****************************************************************************/
static void SetActionFlag(fm_ffuActionFlags *flags,
                          fm_ffuFlag         flag,
                          fm_byte            bits)
{
    if (bits & FM_ACTION_FLAG_DROP)
    {
        flags->drop = flag;
    }

    if (bits & FM_ACTION_FLAG_TRAP)
    {
        flags->trap = flag;
    }

    if (bits & FM_ACTION_FLAG_LOG)
    {
        flags->log = flag;
    }

    if (bits & FM_ACTION_FLAG_NOROUTE)
    {
        flags->noRoute = flag;
    }

    if (bits & FM_ACTION_FLAG_RXMIRROR)
    {
        flags->rxMirror = flag;
    }

    if (bits & FM_ACTION_FLAG_CAPTURE_TIME)
    {
        flags->captureTime = flag;
    }

}   /* end SetActionFlag */




/*****************************************************************************/
/** SetActionFlagMask
 * \ingroup intLowlevFFU10k
 *
 * \desc            Update the mask and value action flag.
 *
 * \param[out]      mask refer to the mask to update.
 * 
 * \param[out]      value refer to the value to update.
 * 
 * \param[in]       bit refer to the flag value.
 * 
 * \param[in]       flag refer to the action.
 *
 * \return          None.
 *
 *****************************************************************************/
static void SetActionFlagMask(fm_byte *  mask,
                              fm_byte *  value,
                              fm_byte    bit,
                              fm_ffuFlag flag)
{
    switch (flag)
    {
        case FM_FFU_FLAG_SET:
            *value |= bit;       /* fall-through */

        case FM_FFU_FLAG_CLEAR:
            *mask |= bit;

        default:
            break;
    }

}   /* end SetActionFlagMask */




/*****************************************************************************/
/** InitScenarioSelectMaps
 * \ingroup intLowlevFFU10k
 *
 * \desc            Initializes the scenario select map tables.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void InitScenarioSelectMaps(void)
{
    static const fm_int rawMapTable[][FM_FFU_SELECTS_PER_MINSLICE + 1] =
    {
        /* selectValue                           mux0  mux1  mux2  mux3  top */
        {  FM_FFU_MUX_SELECT_DIP_31_0,             34,   34,   34,   34,  -1  }, 
        {  FM_FFU_MUX_SELECT_DIP_63_32,            35,   35,   35,   35,  -1  },
        {  FM_FFU_MUX_SELECT_DIP_95_64,            36,   36,   36,   36,  -1  },
        {  FM_FFU_MUX_SELECT_DIP_127_96,           37,   37,   37,   37,  -1  },
        {  FM_FFU_MUX_SELECT_SIP_31_0,             38,   38,   38,   38,  -1  },
        {  FM_FFU_MUX_SELECT_SIP_63_32,            39,   39,   39,   39,  -1  },
        {  FM_FFU_MUX_SELECT_SIP_95_64,            40,   40,   40,   40,  -1  },
        {  FM_FFU_MUX_SELECT_SIP_127_96,           41,   41,   41,   41,  -1  },
        {  FM_FFU_MUX_SELECT_DMAC_15_0,            14,   14,   14,   14,  -1  },
        {  FM_FFU_MUX_SELECT_DMAC_31_16,           15,   15,   15,   15,  -1  },
        {  FM_FFU_MUX_SELECT_DMAC_47_32,           16,   16,   16,   16,  -1  },
        {  FM_FFU_MUX_SELECT_SMAC_15_0,            17,   17,   17,   17,  -1  },
        {  FM_FFU_MUX_SELECT_SMAC_31_16,           18,   18,   18,   18,  -1  },
        {  FM_FFU_MUX_SELECT_SMAC_47_32,           19,   19,   19,   19,  -1  },
        {  FM_FFU_MUX_SELECT_DGLORT,               20,   20,   20,   20,  -1  },
        {  FM_FFU_MUX_SELECT_SGLORT,               21,   21,   21,   21,  -1  },
        {  FM_FFU_MUX_SELECT_L2_VPRI1_L2_VID1,     22,   22,   22,   22,  -1  },
        {  FM_FFU_MUX_SELECT_L2_VPRI2_L2_VID2,     23,   23,   23,   23,  -1  },
        {  FM_FFU_MUX_SELECT_ETHER_TYPE,           24,   24,   24,   24,  -1  },
        {  FM_FFU_MUX_SELECT_L4DST,                25,   25,   25,   25,  -1  },
        {  FM_FFU_MUX_SELECT_L4SRC,                26,   26,   26,   26,  -1  },
        {  FM_FFU_MUX_SELECT_MAP_L4DST,            27,   27,   27,   27,  -1  },
        {  FM_FFU_MUX_SELECT_MAP_L4SRC,            28,   28,   28,   28,  -1  },
        {  FM_FFU_MUX_SELECT_L4A,                  29,   29,   29,   29,  -1  },
        {  FM_FFU_MUX_SELECT_L4B,                  30,   30,   30,   30,  -1  },
        {  FM_FFU_MUX_SELECT_L4C,                  31,   31,   31,   31,  -1  },
        {  FM_FFU_MUX_SELECT_L4D,                  32,   32,   32,   32,  -1  },
        {  FM_FFU_MUX_SELECT_MAP_VLAN_VPRI,        33,   33,   33,   33,  -1  },
        {  FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP,       0,    0,    0,    0,   0  },
        {  FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,     1,    1,    1,    1,   1  },
        {  FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH,   2,    2,    2,    2,   2  },
        {  FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE,      3,    3,    3,    3,   3  },
        {  FM_FFU_MUX_SELECT_USER,                  4,    4,    4,    4,   4  },
        {  FM_FFU_MUX_SELECT_ISLCMD_SYSPRI,         5,    5,    5,    5,   5  },
        {  FM_FFU_MUX_SELECT_MISC,                  6,    6,    6,    6,   6  },
        {  FM_FFU_MUX_SELECT_TOS,                   7,    7,    7,    7,   7  },
        {  FM_FFU_MUX_SELECT_PROT,                  8,    8,    8,    8,   8  },
        {  FM_FFU_MUX_SELECT_TTL,                   9,    9,    9,    9,   9  },
        {  FM_FFU_MUX_SELECT_SRC_PORT,             10,   10,   10,   10,  10  },
        {  FM_FFU_MUX_SELECT_VPRI_VID_11_8,        11,   11,   11,   11,  11  },
        {  FM_FFU_MUX_SELECT_VID_7_0,              12,   12,   12,   12,  12  },
        {  FM_FFU_MUX_SELECT_RXTAG,                13,   13,   13,   13,  13  },

        /* Must be last */
        {  -1,                                     -1,   -1,   -1,   -1,  -1  }
    };
    fm_int index;
    fm_int muxIndex;
    fm_int muxVal;
    fm_int selectVal;

    if (MuxMapsLoaded)
    {
        return;
    }

    FM_MEMSET_S(GenericToMuxMap, sizeof(GenericToMuxMap), -1, sizeof(GenericToMuxMap));
    FM_MEMSET_S(MuxToGenericMap, sizeof(MuxToGenericMap), -1, sizeof(MuxToGenericMap));

    for ( index = 0 ; (selectVal = rawMapTable[index][0]) != -1 ; index++ )
    {
        for (muxIndex = 0 ; muxIndex < FM_FFU_SELECTS_PER_MINSLICE ; muxIndex++)
        {
            muxVal = rawMapTable[index][muxIndex + 1];

            if (muxVal != -1)
            {
                GenericToMuxMap[selectVal][muxIndex] = muxVal;

                if (MuxToGenericMap[muxIndex][muxVal] == -1)
                {
                    MuxToGenericMap[muxIndex][muxVal] = selectVal;
                }
                else
                {
                    FM_LOG_FATAL(FM_LOG_CAT_FFU,
                                 "FFU Mux Select Map has duplicate entries "
                                 "for mux index %d, muxVal %d, "
                                 "raw table index %d\n",
                                 muxIndex,
                                 muxVal,
                                 index);
                }
            }
        }
    }

    MuxMapsLoaded = TRUE;

}   /* end InitScenarioSelectMaps */




/*****************************************************************************/
/** GetHardwareScenarioMuxSelect
 * \ingroup intLowlevFFU10k
 *
 * \desc            Given a generic scenario selection value and the mux
 *                  select position, returns the appropriate hardware
 *                  6-bit select value.  Returns 255 if the generic value
 *                  provided is not supported by the hardware.
 *
 * \param[in]       genericValue is the generic scenario selection value.
 *
 * \param[in]       muxSelect is the mux position (0-4, where 4 is the top
 *                  mux position).
 *
 * \return          0-63 if the selection value was successfully mapped into
 *                  the hardware 6-bit value.
 * \return          -1 if the value could not be mapped.
 *
 *****************************************************************************/
static fm_int GetHardwareScenarioMuxSelect(fm_byte genericValue,
                                           fm_int  muxSelect)
{

    if (!MuxMapsLoaded)
    {
        InitScenarioSelectMaps();
    }

    return GenericToMuxMap[genericValue][muxSelect];

}   /* end GetHardwareScenarioMuxSelect */




/*****************************************************************************/
/** SetFFURuleValid
 * \ingroup intLowlevFFU10k
 *
 * \desc            Marks a rule as valid or invalid, leaving the rest of
 *                  the rule alone.
 * 
 * \note            The caller has already protected the switch.
 *
 * \note            A rule can also be rendered invalid by specifying a
 *                  0 value for the precedence in the ''fm_ffuAction''
 *                  argument to ''fm10000SetFFURule'' and ''fm10000SetFFURules''.
 *                  This function does not alter the precedence, so a rule
 *                  that is invalid by virtue of a zero precedence cannot
 *                  be made valid by calling this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice points to a user-supplied data structure
 *                  identifying the slice or chain of slices on which to 
 *                  operate.
 *
 * \param[in]       ruleIndex is the index of the rule to set.
 *
 * \param[in]       valid indicates whether the rule is valid.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if ruleIndex is invalid.
 *
 *****************************************************************************/
static fm_status SetFFURuleValid(fm_int                 sw,
                                 const fm_ffuSliceInfo *slice,
                                 fm_uint16              ruleIndex,
                                 fm_bool                valid,
                                 fm_bool                useCache)
{
    fm_regsCacheKeyValid    ruleValidArray[FM10000_FFU_SLICE_TCAM_ENTRIES_1];
    fm_regsCacheKeyValid   *ruleValid = ruleValidArray;
    fm_uint                 nKeySlices;
    fm_int                  i;
    fm_uint32               ruleId[FM_REGS_CACHE_MAX_INDICES];
    fm_registerSGListEntry  sgList[FM10000_FFU_SLICE_TCAM_ENTRIES_1];
    fm_registerSGListEntry *sgListPtr = sgList;
    fm_uint32               tempData[FM10000_FFU_SLICE_TCAM_ENTRIES_1 *
                                     FM10000_FFU_SLICE_TCAM_WIDTH];
    fm_uint32              *tempDataPtr = tempData;       
    fm_uint32               key;
    fm_uint32               keyInvert;
    fm_status               err = FM_OK;
    fm_bool                 regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "ruleIndex = %u, "
                  "valid = %s, "
                  "useCache = %s\n",
                  sw,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  ruleIndex,
                  FM_BOOLSTRING(valid),
                  FM_BOOLSTRING(useCache) );

    FM_API_REQUIRE(ruleIndex <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);

    FM_CLEAR(ruleValidArray);
    FM_CLEAR(sgList);
    FM_CLEAR(tempData);

    /* determine the number of slices used by this rule */
    nKeySlices = slice->keyEnd - slice->keyStart + 1;

    /* scan the KeyValid cache for all slices used by this rule */
    ruleId[0] = ruleIndex;
    for (i = 0 ; i < (fm_int)nKeySlices ; i++)
    {
        ruleId[1] = slice->keyStart + i;
        err = fmRegCacheReadKeyValid(sw,
                                     &fm10000CacheFfuSliceTcam,
                                     ruleId,
                                     &ruleValid[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

        FM_API_REQUIRE( (ruleValid[i] != FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1) ||
                        (valid == FALSE),
                        FM_ERR_INVALID_ARGUMENT );
    }

    /* if we got here, it's a legitimate request */
    for (i = 0 ; i < (fm_int)nKeySlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(sgListPtr,
                                  &fm10000CacheFfuSliceTcam,
                                  1,
                                  ruleIndex,
                                  slice->keyStart + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  tempDataPtr,
                                  FALSE);

        sgListPtr++;
        tempDataPtr += FM10000_FFU_SLICE_TCAM_WIDTH;
    }

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    /* get the rule register(s) from the hardware or from the local cache */
    err = fmRegCacheRead(sw, nKeySlices, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    /* Reset the DataPtr */
    tempDataPtr = tempData;
    for (i = 0 ; i < (fm_int)nKeySlices ; i++)
    {
        /* get the key */
        key = FM_ARRAY_GET_FIELD(tempDataPtr,
                                 FM10000_FFU_SLICE_TCAM,
                                 Key);

        /* get its inverted value */
        keyInvert = FM_ARRAY_GET_FIELD(tempDataPtr,
                                       FM10000_FFU_SLICE_TCAM,
                                       KeyInvert);

        if (valid == TRUE)
        {
            /**************************************************
             * we want to make the rule valid, restore the 
             * values of Bit0 for both key and keyInvert from 
             * the keyValid local cache for this register set, 
             * i.e. 'ruleValid'.
             **************************************************/
            /* clear bit 0 */
            key &= ~0x1;
            keyInvert &= ~0x1;

            ruleId[1] = slice->keyStart + i;
            err = fmRegCacheRestoreKeyValid(sw,
                                            &fm10000CacheFfuSliceTcam,
                                            ruleId,
                                            &key,
                                            &keyInvert);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
        }
        else
        {
            /* set bit 0 to invalidate the rule */
            key |= 0x1;
            keyInvert |= 0x1;
        }

        /* update the scratch area with the new values of key, keyInvert */
        FM_ARRAY_SET_FIELD(tempDataPtr,
                           FM10000_FFU_SLICE_TCAM,          
                           Key,
                           key);
        FM_ARRAY_SET_FIELD(tempDataPtr,
                           FM10000_FFU_SLICE_TCAM,          
                           KeyInvert,
                           keyInvert);

        tempDataPtr += FM10000_FFU_SLICE_TCAM_WIDTH;
    }

    /* write back to the hardware and its cache to make it effective */
    err = fmRegCacheWrite(sw, nKeySlices, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end SetFFURuleValid */




/*****************************************************************************/
/** TranslateFFUAction
 * \ingroup intLowlevFFU10k
 *
 * \desc            Translate the fm_ffuAction structure into register data.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       action refer to the structure to translate.
 * 
 * \param[out]      data refer to the register data pointer to fill.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TranslateFFUAction(fm_int              sw,
                                    const fm_ffuAction *action,
                                    fm_uint32 *         data)
{
    fm_byte    byteMask;
    fm_byte    byteData;
    fm_uint32  glort;
    fm_status  err = FM_OK;

    FM_API_REQUIRE(action->precedence <=
                   FM_FIELD_UNSIGNED_MAX(FM10000_FFU_SLICE_SRAM,
                                         Precedence),
                   FM_ERR_INVALID_ACL_PRECEDENCE);
    FM_API_REQUIRE(action->bank <=
                   FM_FIELD_UNSIGNED_MAX(FM10000_FFU_SLICE_SRAM,
                                         CounterBank),
                   FM_ERR_INVALID_COUNTER_BANK);
    FM_API_REQUIRE(action->counter <=
                   FM_FIELD_UNSIGNED_MAX(FM10000_FFU_SLICE_SRAM,
                                         CounterIndex),
                   FM_ERR_INVALID_COUNTER_INDEX);
    FM_ARRAY_SET_FIELD(data,
                       FM10000_FFU_SLICE_SRAM,
                       Precedence,
                       action->precedence);
    FM_ARRAY_SET_FIELD(data,
                       FM10000_FFU_SLICE_SRAM,
                       CounterBank,
                       action->bank);
    FM_ARRAY_SET_FIELD(data,
                       FM10000_FFU_SLICE_SRAM,
                       CounterIndex,
                       action->counter);

    switch (action->action)
    {
        case FM_FFU_ACTION_NOP:
            /* BIT_SET command with a mask of 0 equal NOP */
            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_BIT_SET);
            break;

        case FM_FFU_ACTION_ROUTE_ARP:
            FM_API_REQUIRE(action->data.arp.arpType < FM_FFU_ARP_TYPE_MAX,
                           FM_ERR_INVALID_ARGUMENT);

            if (action->data.arp.arpType == FM_FFU_ARP_TYPE_MIN_RANGE)
            {
                FM_API_REQUIRE(action->data.arp.count >=
                               FM_ACTION_ARP_CNT_MINRANGE_MINVALUE,
                               FM_ERR_INVALID_ARGUMENT);

                /* Value 16 will be truncated to 0 */
                FM_API_REQUIRE(action->data.arp.count <=
                               FM_ACTION_ARP_CNT_MINRANGE_MAXVALUE,
                               FM_ERR_INVALID_ARGUMENT);
            }
            else
            {
                FM_API_REQUIRE(action->data.arp.count <=
                               FM_ACTION_ARP_CNT_MAXRANGE_MAXVALUE,
                               FM_ERR_INVALID_ARGUMENT);
            }

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_ROUTE_ARP);

            FM_ARRAY_SET_BIT(data,
                             FM10000_FFU_SLICE_SRAM_ROUTE_ARP,
                             ArpType,
                             action->data.arp.arpType);
            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_ROUTE_ARP,
                               ArpCount,
                               action->data.arp.count);
            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_ROUTE_ARP,
                               ArpIndex,
                               action->data.arp.arpIndex);
            break;

        case FM_FFU_ACTION_ROUTE_LOGICAL_PORT:
        case FM_FFU_ACTION_ROUTE_FLOOD_DEST:

            err = fmGetLogicalPortGlort(sw,
                                        action->data.logicalPort,
                                        &glort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_ROUTE_GLORT);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_ROUTE_GLORT,
                               DGlort,
                               glort);

            if (action->action == FM_FFU_ACTION_ROUTE_FLOOD_DEST)
            {
                FM_ARRAY_SET_BIT(data,
                                 FM10000_FFU_SLICE_SRAM_ROUTE_GLORT,
                                 FloodSet,
                                 1);
            }
            break;

        case FM_FFU_ACTION_ROUTE_GLORT:

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_ROUTE_GLORT);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_ROUTE_GLORT,
                               DGlort,
                               action->data.glort);

            break;

        case FM_FFU_ACTION_SET_FLAGS:

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_BIT_SET);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               SubCommand,
                               FM_ACTION_SUBCMD_FLAG);

            byteMask = 0;
            byteData = 0;
            SetActionFlagMask(&byteMask,
                              &byteData,
                              FM_ACTION_FLAG_DROP,
                              action->data.flags.drop);
            SetActionFlagMask(&byteMask,
                              &byteData,
                              FM_ACTION_FLAG_TRAP,
                              action->data.flags.trap);
            SetActionFlagMask(&byteMask,
                              &byteData,
                              FM_ACTION_FLAG_LOG,
                              action->data.flags.log);
            SetActionFlagMask(&byteMask,
                              &byteData,
                              FM_ACTION_FLAG_NOROUTE,
                              action->data.flags.noRoute);
            SetActionFlagMask(&byteMask,
                              &byteData,
                              FM_ACTION_FLAG_RXMIRROR,
                              action->data.flags.rxMirror);
            SetActionFlagMask(&byteMask,
                              &byteData,
                              FM_ACTION_FLAG_CAPTURE_TIME,
                              action->data.flags.captureTime);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               ByteMask,
                               byteMask);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               ByteData,
                               byteData);
            break;

        case FM_FFU_ACTION_SET_TRIGGER:
            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_BIT_SET);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               SubCommand,
                               FM_ACTION_SUBCMD_TRIG);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               ByteMask,
                               action->data.trigger.mask);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               ByteData,
                               action->data.trigger.value);
            break;

        case FM_FFU_ACTION_SET_USER:
            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_BIT_SET);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               SubCommand,
                               FM_ACTION_SUBCMD_USR);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               ByteMask,
                               action->data.user.mask);

            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM_SET_BITS,
                               ByteData,
                               action->data.user.value);
            break;

        case FM_FFU_ACTION_SET_FIELDS:
            FM_ARRAY_SET_FIELD(data,
                               FM10000_FFU_SLICE_SRAM,
                               Command,
                               FM_ACTION_COMMAND_FIELD_SET);

            switch (action->data.fields.fieldType)
            {
                case FM_FFU_FIELD_NEITHER:
                case FM_FFU_FIELD_DSCP:
                    FM_ARRAY_SET_BIT(data,
                                     FM10000_FFU_SLICE_SRAM_SET_PRI,
                                     SubCommand,
                                     FM_ACTION_SUBCMD_SET_DSCP);

                    if (action->data.fields.fieldType == FM_FFU_FIELD_DSCP)
                    {
                        FM_API_REQUIRE(action->data.fields.fieldValue < FM_MAX_DSCP_PRIORITIES,
                                       FM_ERR_INVALID_ARGUMENT);
                        FM_ARRAY_SET_BIT(data,
                                         FM10000_FFU_SLICE_SRAM_SET_PRI,
                                         SetDSCP,
                                         1);
                        FM_ARRAY_SET_FIELD(data,
                                           FM10000_FFU_SLICE_SRAM_SET_PRI,
                                           DSCP,
                                           action->data.fields.fieldValue);
                    }

                    if (action->data.fields.setPri == TRUE)
                    {
                        FM_API_REQUIRE(action->data.fields.priority < FM_MAX_SWITCH_PRIORITIES,
                                       FM_ERR_INVALID_ARGUMENT);
                        FM_ARRAY_SET_BIT(data,
                                         FM10000_FFU_SLICE_SRAM_SET_PRI,
                                         SetPRI,
                                         1);
                        FM_ARRAY_SET_FIELD(data,
                                           FM10000_FFU_SLICE_SRAM_SET_PRI,
                                           PRI,
                                           action->data.fields.priority);
                    }

                    if (action->data.fields.setVpri == TRUE)
                    {
                        FM_API_REQUIRE(action->data.fields.priority < FM_MAX_VLAN_PRIORITIES,
                                       FM_ERR_INVALID_ARGUMENT);
                        FM_ARRAY_SET_BIT(data,
                                         FM10000_FFU_SLICE_SRAM_SET_PRI,
                                         SetVPRI,
                                         1);
                        FM_ARRAY_SET_FIELD(data,
                                           FM10000_FFU_SLICE_SRAM_SET_PRI,
                                           PRI,
                                           action->data.fields.priority);
                    }

                    break;

                case FM_FFU_FIELD_VLAN:
                    FM_API_REQUIRE(action->data.fields.fieldValue < FM_MAX_VLAN,
                                   FM_ERR_INVALID_ARGUMENT);
                    FM_API_REQUIRE(action->data.fields.txTag < FM_FFU_TXTAG_MAX,
                                   FM_ERR_INVALID_ARGUMENT);

                    FM_ARRAY_SET_BIT(data,
                                     FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                     SubCommand,
                                     FM_ACTION_SUBCMD_SET_VLAN);
                    FM_ARRAY_SET_FIELD(data,
                                       FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                       VLAN,
                                       action->data.fields.fieldValue);
                    FM_ARRAY_SET_FIELD(data,
                                       FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                       TxTag,
                                       action->data.fields.txTag);

                    if (action->data.fields.setPri == TRUE)
                    {
                        FM_API_REQUIRE(action->data.fields.priority < FM_MAX_SWITCH_PRIORITIES,
                                       FM_ERR_INVALID_ARGUMENT);
                        FM_ARRAY_SET_BIT(data,
                                         FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                         SetPRI,
                                         1);
                        FM_ARRAY_SET_FIELD(data,
                                           FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                           PRI,
                                           action->data.fields.priority);
                    }

                    if (action->data.fields.setVpri == TRUE)
                    {
                        FM_API_REQUIRE(action->data.fields.priority < FM_MAX_VLAN_PRIORITIES,
                                       FM_ERR_INVALID_ARGUMENT);
                        FM_ARRAY_SET_BIT(data,
                                         FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                         SetVPRI,
                                         1);
                        FM_ARRAY_SET_FIELD(data,
                                           FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                           PRI,
                                           action->data.fields.priority);
                    }

                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

            }   /* end switch (action->data.fields.fieldType) */

            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    }   /* end switch (action->action) */

ABORT:

    return err;
    

}   /* end TranslateFFUAction */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000SetFFUSliceOwnership
 * \ingroup intLowlevFFU10k
 *
 * \desc            Sets the slice range ownership within the FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       owner represents a valid software component.
 *
 * \param[in]       firstSlice is the first slice number in the range.
 *
 * \param[in]       lastSlice is the last slice number in the range.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_FFU_RES_OWNED if this range is already owned.
 *
 *****************************************************************************/
fm_status fm10000SetFFUSliceOwnership(fm_int          sw,
                                      fm_ffuOwnerType owner,
                                      fm_int          firstSlice,
                                      fm_int          lastSlice)
{
    fm10000_switch *            switchExt = NULL;
    fm_fm10000FfuOwnershipInfo *info = NULL;
    fm_int                      slice;
    fm_status                   err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "owner = %d, "
                  "firstSlice = %d, "
                  "lastSlice = %d\n",
                  sw,
                  owner,
                  firstSlice,
                  lastSlice);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    switchExt = GET_SWITCH_EXT(sw);
    info      = &switchExt->ffuOwnershipInfo;

    FM_API_REQUIRE(firstSlice >= 0,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(lastSlice < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);

    for (slice = firstSlice ; slice <= lastSlice ; slice++)
    {
        if ( (info->sliceOwner[slice] != FM_FFU_OWNER_NONE) &&
             (info->sliceOwner[slice] != owner) &&
             (owner != FM_FFU_OWNER_NONE) )
        {
            err = FM_ERR_FFU_RES_OWNED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
        }
    }

    for (slice = firstSlice ; slice <= lastSlice ; slice++)
    {
        info->sliceOwner[slice] = owner;
    }


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFUSliceOwnership */




/*****************************************************************************/
/** fm10000GetFFUSliceOwnership
 * \ingroup intLowlevFFU10k
 *
 * \desc            Gets the slice range ownership within the FFU.
 *
 * \note            This assumes there is a single, contiguous slice range.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       owner represents the ownership type to get the slice range
 *                  for.
 *
 * \param[out]      firstSlice points to caller allocated storage where the
 *                  first slice in the assigned range is written to.
 *
 * \param[out]      lastSlice points to caller allocated storage where the
 *                  last slice in the assigned range is written to.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetFFUSliceOwnership(fm_int          sw,
                                      fm_ffuOwnerType owner,
                                      fm_int *        firstSlice,
                                      fm_int *        lastSlice)
{
    fm10000_switch *            switchExt = NULL;
    fm_fm10000FfuOwnershipInfo *info = NULL;
    fm_int                      slice;
    fm_status                   err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "owner = %d, "
                  "firstSlice = %p, "
                  "lastSlice = %p\n",
                  sw,
                  owner,
                  (void *) firstSlice,
                  (void *) lastSlice);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    switchExt = GET_SWITCH_EXT(sw);
    info      = &switchExt->ffuOwnershipInfo;

    FM_API_REQUIRE(firstSlice, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(lastSlice, FM_ERR_INVALID_ARGUMENT);

    *firstSlice = -1;
    *lastSlice  = -1;

    for (slice = 0 ; slice < FM10000_FFU_SLICE_VALID_ENTRIES ; slice++)
    {
        if (info->sliceOwner[slice] == owner)
        {
            if (*firstSlice == -1)
            {
                *firstSlice = slice;
            }

            *lastSlice = slice;
        }
    }

    if ( (*firstSlice == -1) || (*lastSlice == -1) )
    {
        err = FM_ERR_NO_FFU_RES_FOUND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetFFUSliceOwnership */




/*****************************************************************************/
/** fm10000GetFFUSliceOwner
 * \ingroup intLowlevFFU10k
 *
 * \desc            Gets the owner for a specific slice within the FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice is the slice number.
 *
 * \param[out]      owner contains the owner for the specified slice.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetFFUSliceOwner(fm_int           sw,
                                  fm_int           slice,
                                  fm_ffuOwnerType *owner)
{
    fm10000_switch *switchExt = NULL;
    fm_status       err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice = %d, "
                  "owner = %p\n",
                  sw,
                  slice,
                  (void *) owner);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(slice >= 0,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(owner, FM_ERR_INVALID_ARGUMENT);

    switchExt = GET_SWITCH_EXT(sw);

    *owner = switchExt->ffuOwnershipInfo.sliceOwner[slice];


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetFFUSliceOwner */



 
/*****************************************************************************/
/** fm10000FFUInit
 * \ingroup intLowlevFFU10k
 *
 * \desc            Private initialization function called from 
 *                  ''fm10000PostBootSwitch''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FFUInit(fm_int sw)
{
    fm_status             err = FM_OK;
    fm_uint32             slice;
    fm_uint32             rule;
    fm_ffuSliceInfo       sliceInfo;
    fm_fm10000FfuSliceKey ruleKey;
    fm_ffuCaseLocation    caseLocation = FM_FFU_CASE_NOT_MAPPED;
    fm_ffuAction          action;
    fm_uint32             value[FM10000_FFU_SLICE_VALID_WIDTH];

    /* Disable all the FFU slices and chunk at init time */
    err = fm10000SetFFUMasterValid(sw, 0, 0, FALSE);
    if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Default value for all rules */
    ruleKey.key = 0;
    ruleKey.keyMask = 0;
    ruleKey.kase.value = 0;
    ruleKey.kase.mask = 0;

    action.action = FM_FFU_ACTION_NOP;
    action.precedence = 0;
    action.bank = 0;
    action.counter = 0;

    value[0] = 0xffffffff;

#if (!defined(FV_CODE) && !defined(FAST_API_BOOT))
    /* Invalidate all the rules of every slice first. */
    for (slice = 0 ; slice < FM10000_FFU_SLICE_VALID_ENTRIES; slice++)
    {
        sliceInfo.keyStart = slice;
        sliceInfo.keyEnd = slice;
        sliceInfo.actionEnd = slice;
        sliceInfo.caseLocation = &caseLocation;

        for (rule = 0 ; rule < FM10000_FFU_SLICE_TCAM_ENTRIES_0 ; rule++)
        {
            err = fm10000SetFFURule(sw,
                                    &sliceInfo,
                                    rule,
                                    FALSE,
                                    &ruleKey,
                                    &action,
                                    FALSE,
                                    FALSE);
            if (err != FM_OK)
            {
                goto ABORT;
            }
        }

        /* Force FFU_SLICE_VALID to always match for all scenarios */
        err = fmRegCacheWriteSingle1D(sw, 
                                      &fm10000CacheFfuSliceValid,
                                      value,
                                      slice,
                                      FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }
#endif

ABORT:

    return err;

}   /* end fm10000FFUInit */




/*****************************************************************************/
/** fm10000MoveFFURules                                 
 * \ingroup intLowlevFFU10k
 *
 * \desc            Move a range of rules to a different index within
 *                  the same slice or chain of cascaded slices in a non
 *                  disturbing way. This function must be as optimize as
 *                  possible to achieve good non disruptive acl performance.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice points to a user-supplied data structure
 *                  identifying the slice or chain of slices on which to 
 *                  operate.
 *
 * \param[in]       fromIndex is the index of the first rule to move 
 *
 * \param[in]       nRules is the number of rules to move.
 *
 * \param[in]       toIndex is the destination index of the first rule 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if fromIndex, toIndex,
 *                  or nRules are invalid.
 *
 *****************************************************************************/
fm_status fm10000MoveFFURules(fm_int                 sw,
                              const fm_ffuSliceInfo *slice,
                              fm_uint16              fromIndex,
                              fm_uint16              nRules,
                              fm_uint16              toIndex)
{
    fm_int        camOffset;
    fm_int        ramOffset;
    fm_int        startIndex;
    fm_int        i;
    fm_int        j;
    fm_int        numSlice;
    fm_int        numActSlice;
    fm_int        nextCamIndex;
    fm_int        nextRamIndex;
    fm_int        nextBitIndex;
    fm_uint32 *   camCachePtr;
    fm_uint32 *   camCachePtrTmp;
    fm_uint32     camCacheRegOffset;
    fm_uint32     camSwitchRegOffset;
    fm_uint32     camSwitchRegPos;
    fm_uint32     camRuleRegOffset;
    fm_uint32 *   ramCachePtr;
    fm_uint32 *   ramCachePtrTmp;
    fm_uint32     ramCacheRegOffset;
    fm_uint32     ramSwitchRegOffset;
    fm_uint32     ramSwitchRegPos;
    fm_uint32     ramRuleRegOffset;
    fm_switch *   switchPtr;
    fm_uint32     bitOffset;
    fm_uint32     bitSliceOffset;
    fm_status     err = FM_OK;
    fm_uint32 *   bitArrayDataPtr;
    fm_uint32 *   bitArrayDataPtrSrc;
    fm_uint32     bitMaskSrc;
    fm_uint32     bitArrayBitSrc;
    fm_uint32 *   bitArrayDataPtrDst;
    fm_uint32     bitMaskDst;
    fm_uint32     bitArrayBitDst;
    fm_uint32     addrArray[(FM10000_FFU_SLICE_TCAM_ENTRIES_1 + 1) * FM10000_FFU_SLICE_TCAM_WIDTH * 2];
    fm_uint32     valueArray[(FM10000_FFU_SLICE_TCAM_ENTRIES_1 + 1) * FM10000_FFU_SLICE_TCAM_WIDTH * 2];
    fm_int        addrCount;
    fm_writeReg32Seq WriteReg32Seq;

    /* declare all what you need and log the arguments */
    FM_LOG_ENTRY(FM_LOG_CAT_FFU,
                 "sw = %d, slice = %p, fromIndex = %d, "
                 "nRules = %d, toIndex=%d\n",
                 sw,
                 (void*) slice,
                 fromIndex,
                 nRules,
                 toIndex);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);   /* make access atomic */
    TAKE_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    /* slice pointer must be valid */
    FM_API_REQUIRE(slice != NULL, FM_ERR_INVALID_ARGUMENT);

    /* from index range must be valid */
    FM_API_REQUIRE((fromIndex + nRules) <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);

    /* to index range must be valid */
    FM_API_REQUIRE((toIndex + nRules) <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);

    /* Retrieve array that holds cached value */
    camCachePtr = fm10000CacheFfuSliceTcam.getCache.data(sw);
    ramCachePtr = fm10000CacheFfuSliceSram.getCache.data(sw);
    bitArrayDataPtr = fm10000CacheFfuSliceTcam.getCache.valid(sw)->bitArrayData;

    /* Compute cam and ram offset from the current to the desired location */
    camOffset = (toIndex - fromIndex) * FM10000_FFU_SLICE_TCAM_WIDTH;
    ramOffset = (toIndex - fromIndex) * FM10000_FFU_SLICE_SRAM_WIDTH;

    /* Row move would be done in reverse direction */
    if (toIndex > fromIndex)
    {
        startIndex = fromIndex + nRules - 1;
        nextCamIndex = FM10000_FFU_SLICE_TCAM_WIDTH;
        nextRamIndex = FM10000_FFU_SLICE_SRAM_WIDTH;
        nextBitIndex = 2;
    }
    else
    {
        startIndex = fromIndex;
        nextCamIndex = -FM10000_FFU_SLICE_TCAM_WIDTH;
        nextRamIndex = -FM10000_FFU_SLICE_SRAM_WIDTH;
        nextBitIndex = -2;
    }

    /* Use the raw register write function if the platform supports it. */
    if (switchPtr->WriteRawUINT32Seq)
    {
        WriteReg32Seq = switchPtr->WriteRawUINT32Seq;
    }
    else
    {
        /* Emulates it using register write loops */
        WriteReg32Seq = fmEmulateWriteRawUINT32Seq;
    }

    /* Compute different offset that would be used to properly update switch
     * and cache registers. */
    camCacheRegOffset = (slice->keyStart *
                         FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                         FM10000_FFU_SLICE_TCAM_WIDTH);
    camSwitchRegOffset = camCacheRegOffset * 2;
    camRuleRegOffset = (startIndex * FM10000_FFU_SLICE_TCAM_WIDTH);
    camCacheRegOffset += camRuleRegOffset;
    camSwitchRegOffset += camRuleRegOffset;

    camCachePtrTmp = camCachePtr + camCacheRegOffset;
    camSwitchRegPos = FM10000_FFU_SLICE_TCAM(0,0,0) + camSwitchRegOffset;

    ramCacheRegOffset = (slice->keyEnd *
                         FM10000_FFU_SLICE_SRAM_ENTRIES_0 *
                         FM10000_FFU_SLICE_SRAM_WIDTH);
    ramSwitchRegOffset = ramCacheRegOffset * 4;
    ramRuleRegOffset = (startIndex * FM10000_FFU_SLICE_SRAM_WIDTH);
    ramCacheRegOffset += ramRuleRegOffset;
    ramSwitchRegOffset += ramRuleRegOffset;

    ramCachePtrTmp = ramCachePtr + ramCacheRegOffset;
    ramSwitchRegPos = FM10000_FFU_SLICE_SRAM(0,0,0) + ramSwitchRegOffset;

    numSlice = slice->keyEnd - slice->keyStart + 1;
    numActSlice = slice->actionEnd - slice->keyEnd + 1;

    bitOffset = camCacheRegOffset / 2;
    bitSliceOffset = numSlice * FM10000_FFU_SLICE_TCAM_ENTRIES_0 * 2;

    camCacheRegOffset = numSlice *
                        FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                        FM10000_FFU_SLICE_TCAM_WIDTH;
    camSwitchRegOffset = camCacheRegOffset * 2;

    ramCacheRegOffset = numActSlice *
                        FM10000_FFU_SLICE_SRAM_ENTRIES_0 *
                        FM10000_FFU_SLICE_SRAM_WIDTH;
    ramSwitchRegOffset = ramCacheRegOffset * 4;

    /* Process each rule and update all slice (condition + action) */
    for (i = 0 ; i < nRules ; i++)
    {
        /* Reset the register write count */
        addrCount = 0;

        /* Move action slice first since the destination row must be disabled. */
        for (j = 0 ; j < numActSlice ; j++)
        {
            /* Read Data in cache and update switch and cache registers. */
            addrArray[addrCount] = ramSwitchRegPos + ramOffset;
            valueArray[addrCount] = *(ramCachePtrTmp);
            *(ramCachePtrTmp + ramOffset) = *(ramCachePtrTmp);
            ++addrCount;

            addrArray[addrCount] = ramSwitchRegPos + ramOffset + 1;
            valueArray[addrCount] = *(ramCachePtrTmp + 1);
            *(ramCachePtrTmp + ramOffset + 1) = *(ramCachePtrTmp + 1);
            ++addrCount;

            /* Process Next action slice. */
            ramCachePtrTmp += FM10000_FFU_SLICE_SRAM_ENTRIES_0 *
                              FM10000_FFU_SLICE_SRAM_WIDTH;
            ramSwitchRegPos += FM10000_FFU_SLICE_SRAM_ENTRIES_0 *
                               FM10000_FFU_SLICE_SRAM_WIDTH * 4;

        }   /* end for (j = 0 ; j < numActSlice ; j++) */

        /* Updates pointer to the next row */
        ramCachePtrTmp -= ramCacheRegOffset + nextRamIndex;
        ramSwitchRegPos -= ramSwitchRegOffset + nextRamIndex;

        /* Move condition slices where the last condition write would enable
         * the row entirely. */
        for (j = 0 ; j < numSlice ; j++)
        {
            /* Keep the valid bit of the processed row */
            bitArrayDataPtrSrc = bitArrayDataPtr + (bitOffset / 32);
            bitMaskSrc = 3 << (bitOffset % 32);
            bitArrayBitSrc = *bitArrayDataPtrSrc & bitMaskSrc;

            /* Read Data in cache and update switch and cache registers. */
            addrArray[addrCount] = camSwitchRegPos + camOffset;
            valueArray[addrCount++] = *(camCachePtrTmp);
            *(camCachePtrTmp + camOffset) = *(camCachePtrTmp);
            addrArray[addrCount] = camSwitchRegPos + camOffset + 1;
            valueArray[addrCount++] = *(camCachePtrTmp + 1);
            *(camCachePtrTmp + camOffset + 1) = *(camCachePtrTmp + 1);
            addrArray[addrCount] = camSwitchRegPos + camOffset + 2;
            valueArray[addrCount++] = *(camCachePtrTmp + 2);
            *(camCachePtrTmp + camOffset + 2) = *(camCachePtrTmp + 2);
            addrArray[addrCount] = camSwitchRegPos + camOffset + 3;
            valueArray[addrCount++] = *(camCachePtrTmp + 3);
            *(camCachePtrTmp + camOffset + 3) = *(camCachePtrTmp + 3);

            /* Update valid bits related to the moved rule. */
            bitArrayDataPtrDst = bitArrayDataPtr + ((bitOffset + (camOffset / 2)) / 32);
            bitMaskDst = 3 << ((bitOffset + (camOffset / 2)) % 32);

            /* Swap the BitArray value of source and destination to keep the
             * total number of bitArray bit in sync. */
            bitArrayBitDst = *bitArrayDataPtrDst & bitMaskDst;
            bitArrayBitDst >>= ((bitOffset + (camOffset / 2)) % 32);
            bitArrayBitDst <<= (bitOffset % 32);
            *bitArrayDataPtrSrc &= ~bitMaskSrc;
            *bitArrayDataPtrSrc |= bitArrayBitDst;

            /* Now set the BitArray dest value to the one captured from the
             * source */
            bitArrayBitDst = bitArrayBitSrc >> (bitOffset % 32);
            bitArrayBitDst <<= ((bitOffset + (camOffset / 2)) % 32);

            *bitArrayDataPtrDst &= ~bitMaskDst;
            *bitArrayDataPtrDst |= bitArrayBitDst;

            /* Process Next condition slice. */
            camCachePtrTmp += FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                              FM10000_FFU_SLICE_TCAM_WIDTH;
            camSwitchRegPos += FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                               FM10000_FFU_SLICE_TCAM_WIDTH * 2;
            bitOffset += FM10000_FFU_SLICE_TCAM_ENTRIES_0 * 2;

        }   /* end for (j = 0 ; j < numSlice ; j++) */

        /* Revert pointer modification to the original state. */
        camCachePtrTmp -= camCacheRegOffset;
        camSwitchRegPos -= camSwitchRegOffset;
        bitOffset -= bitSliceOffset + nextBitIndex;

        /* Invalidate source row condition and valid bit. */
        for (j = 0 ; j < numSlice ; j++)
        {
            /* Invalidate switch register and update cache.
             * Invalidating entries with (Key == KeyInvert == 1) */
            addrArray[addrCount] = camSwitchRegPos;
            valueArray[addrCount++] = 0x1;               //Key
            addrArray[addrCount] = camSwitchRegPos + 1;
            valueArray[addrCount++] = 0x0;               //KeyTop
            addrArray[addrCount] = camSwitchRegPos + 2;
            valueArray[addrCount++] = 0x1;               //KeyInvert
            addrArray[addrCount] = camSwitchRegPos + 3;
            valueArray[addrCount++] = 0x0;               //KeyTopInvert

            *(camCachePtrTmp)     = 0x1;         //Key
            *(camCachePtrTmp + 1) = 0x0;         //KeyTop
            *(camCachePtrTmp + 2) = 0x1;         //KeyInvert
            *(camCachePtrTmp + 3) = 0x0;         //KeyTopInvert

            /* Next condition slice */
            camCachePtrTmp += FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                              FM10000_FFU_SLICE_TCAM_WIDTH;
            camSwitchRegPos += FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                               FM10000_FFU_SLICE_TCAM_WIDTH * 2;

        }   /* end for (j = 0 ; j < numSlice ; j++) */

        /* Updates pointer to the next row */
        camCachePtrTmp -= camCacheRegOffset + nextCamIndex;
        camSwitchRegPos -= camSwitchRegOffset + nextCamIndex;

        /* Apply the register write sequence entered for the current row */
        err = WriteReg32Seq(sw, addrArray, valueArray, addrCount);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    }   /* end for (i = 0 ; i < nRules ; i++) */

ABORT:

    DROP_PLAT_LOCK(sw, FM_MEM_TYPE_CSR);
    DROP_REG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000MoveFFURules */




/*****************************************************************************/
/** fm10000SetFFUMasterValid
 * \ingroup lowlevFfu10k
 *
 * \desc            Atomically set the master valid bit for all 32
 *                  slices.  The slice will only hit if its master valid
 *                  bit is true, regardless of the setting of any other
 *                  valid bits.  The same thing is also provided for egress
 *                  chunks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       validIngress contains one bit for each slice.
 *
 * \param[in]       validEgress contains one bit for each chunk.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000SetFFUMasterValid(fm_int    sw,
                                   fm_uint32 validIngress,
                                   fm_uint32 validEgress,
                                   fm_bool   useCache)
{
    fm_uint32  value[FM10000_FFU_MASTER_VALID_WIDTH] = {0};
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "validIngress = %x, "
                  "validEgress = %x, "
                  "useCache = %s\n",
                  sw,
                  validIngress,
                  validEgress,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_ARRAY_SET_FIELD(value, FM10000_FFU_MASTER_VALID, SliceValid, validIngress);
    FM_ARRAY_SET_FIELD(value, FM10000_FFU_MASTER_VALID, ChunkValid, validEgress);

    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuMasterValid,
                                  value,
                                  0,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFUMasterValid */



/*****************************************************************************/
/** fm10000GetFFUMasterValid
 * \ingroup lowlevFfu10k
 *
 * \desc            Read the value of the master valid bits for all
 *                  32 ingress slices and all 32 egress chunks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      validIngress contains one bit for each slice.
 *
 * \param[out]      validEgress contains one bit for each chunk.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will be returned from the cache
 *                  without querying the hardware.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000GetFFUMasterValid(fm_int     sw,
                                   fm_uint32 *validIngress,
                                   fm_uint32 *validEgress,
                                   fm_bool    useCache)
{
    fm_uint32  value[FM10000_FFU_MASTER_VALID_WIDTH] = {0};
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "validIngress = %p, "
                  "validEgress = %p, "
                  "useCache = %s\n",
                  sw,
                  (void *) validIngress,
                  (void *) validEgress,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuMasterValid,
                                 value,
                                 0,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    *validIngress = FM_ARRAY_GET_FIELD(value,
                                       FM10000_FFU_MASTER_VALID,
                                       SliceValid);
    *validEgress = FM_ARRAY_GET_FIELD(value,
                                      FM10000_FFU_MASTER_VALID,
                                      ChunkValid);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetFFUMasterValid */



/*****************************************************************************/
/** fm10000ConfigureFFUSlice
 * \ingroup lowlevFfu10k
 *
 * \desc            Configures the multiplexer selection on one or more
 *                  scenarios for a slice or chain of cascaded slices.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice points to a data structure describing the slice
 *                  or chain of slices on which to operate.
 *
 * \param[in]       useCache specifies whether the cache may be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if kase is out of range.
 * \return          FM_ERR_INVALID_MUX_SELECT if any parameter in selects
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fm10000ConfigureFFUSlice(fm_int                 sw,
                                   const fm_ffuSliceInfo *slice,
                                   fm_bool                useCache)
{
    fm_uint                 i;
    fm_uint                 j;
    fm_uint                 k;
    fm_uint                 nKeySlices;
    fm_uint                 nActionSlices;
    fm_uint                 nEntries;
    fm_uint                 dataSize;
    fm_uint                 nReadEntries;
    fm_uint                 nWriteEntries;
    fm_registerSGListEntry *sgList;
    fm_uint                 sgIndex = 0;
    fm_uint32 *             data;
    fm_uint32 *             dataPtr;
    fm_int                  muxValue;
    const fm_uint           selectWidth = FM_FIELD_WIDTH(FM10000_FFU_SLICE_CFG,
                                                         Select0);
    fm_cleanupListEntry *   cleanupList = NULL;
    fm_status               err = FM_OK;
    fm_bool                 regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->validScenarios = %x, "
                  "slice->kase = %u, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "slice->validLow = %u, "
                  "slice->validHigh = %u, "
                  "useCache = %s\n",
                  sw,
                  slice->validScenarios,
                  slice->kase,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  slice->validLow,
                  slice->validHigh,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    /* This special case enables the possibility to update only the Key part
     * of the scenario by setting actionEnd == keyEnd - 1 */
    FM_API_REQUIRE((slice->actionEnd + 1) >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->kase <= FM_FIELD_UNSIGNED_MAX(FM10000_FFU_SLICE_CFG, Case_XXX),
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->validLow <= 1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->validHigh <= 1, FM_ERR_INVALID_ARGUMENT);

    nKeySlices    = 1 + slice->keyEnd - slice->keyStart;
    nActionSlices = 1 + slice->actionEnd - slice->keyEnd;

    /* Cover the worst case (validScenarios == 0xffffffff) */
    nEntries = ((FM10000_FFU_SLICE_CFG_ENTRIES_0 * nKeySlices) + nActionSlices);
    FM_ALLOC_TEMP_ARRAY(sgList, fm_registerSGListEntry, nEntries);

    dataSize = ( ((FM10000_FFU_SLICE_CFG_WIDTH * FM10000_FFU_SLICE_CFG_ENTRIES_0) * nKeySlices) +
                 (FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH * nActionSlices) );
    FM_ALLOC_TEMP_ARRAY(data, fm_uint32, dataSize);
    dataPtr = data;

    for (i = 0 ; i < nActionSlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex + i],
                                  &fm10000CacheFfuSliceCascadeAction,
                                  1,
                                  i + slice->keyEnd,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr+= FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH;
    }
    sgIndex += nActionSlices;

    nReadEntries = sgIndex;

    /**************************************************
     * We only need to do a write (but not read) on
     * FFU_SLICE_CFG, because it is not shared
     * between the scenario, so we can completely
     * overwrite it.
     **************************************************/
    for (i = 0 ; i < nKeySlices ; i++)
    {
        for (j = 0 ; j < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; j++)
        {
            if (slice->validScenarios & (1 << j))
            {
                FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                          &fm10000CacheFfuSliceCfg,
                                          1,
                                          j,
                                          i + slice->keyStart,
                                          FM_REGS_CACHE_INDEX_UNUSED,
                                          dataPtr,
                                          FALSE);
                sgIndex++;
                dataPtr += FM10000_FFU_SLICE_CFG_WIDTH;
            }
        }
    }

    nWriteEntries = sgIndex;

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, nReadEntries, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    sgIndex = 0;

    for (i = 0 ; i < nActionSlices ; i++)
    {
        dataPtr   = sgList[sgIndex++].data; /* FFU_SLICE_CASCADE_ACTION */
        *dataPtr |= slice->validScenarios;
    }

    for (i = 0 ; i < nKeySlices ; i++)
    {
        for (j = 0 ; j < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; j++)
        {
            if (slice->validScenarios & (1 << j))
            {
                dataPtr = sgList[sgIndex++].data; /* FFU_SLICE_CFG */

                for (k = 0 ; k < FM_FFU_SELECTS_PER_MINSLICE ; k++)
                {
                    /* Translate the high level ffu select to the proper ffu
                     * key. */
                    muxValue = slice->selects[(i * FM_FFU_SELECTS_PER_MINSLICE) + k];
                    muxValue = GetHardwareScenarioMuxSelect(muxValue, k);

                    /* Return error if select can't be translated */
                    FM_API_REQUIRE(muxValue != -1, FM_ERR_INVALID_MUX_SELECT);

                    *dataPtr |= muxValue << (k * selectWidth);
                }

                FM_ARRAY_SET_BIT(dataPtr,
                                 FM10000_FFU_SLICE_CFG,
                                 StartCompare,
                                 (i == 0));

                FM_ARRAY_SET_BIT(dataPtr,
                                 FM10000_FFU_SLICE_CFG,
                                 StartAction,
                                 ((i == nKeySlices - 1) && nActionSlices));

                FM_ARRAY_SET_BIT(dataPtr,
                                 FM10000_FFU_SLICE_CFG,
                                 ValidLow,
                                 slice->validLow);

                FM_ARRAY_SET_BIT(dataPtr,
                                 FM10000_FFU_SLICE_CFG,
                                 ValidHigh,
                                 slice->validHigh);

                FM_ARRAY_SET_FIELD(dataPtr,
                                   FM10000_FFU_SLICE_CFG,
                                   Case_XXX,
                                   slice->kase);

                FM_API_REQUIRE(slice->caseLocation[i] < FM_FFU_CASE_MAX,
                               FM_ERR_INVALID_ARGUMENT);

                FM_ARRAY_SET_FIELD(dataPtr,
                                   FM10000_FFU_SLICE_CFG,
                                   SetCaseLocation,
                                   slice->caseLocation[i]);
            }
        }
    }

    err = fmRegCacheWrite(sw, nWriteEntries, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    FM_FREE_TEMP_ARRAYS();
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000ConfigureFFUSlice */




/*****************************************************************************/
/** fm10000UnconfigureFFUSlice
 * \ingroup lowlevFfu10k
 *
 * \desc            Tears down the configuration created by
 *                  ''fm10000ConfigureFFUSlice''.  You must unconfigure
 *                  each slice in the chain before you can configure a
 *                  new slice or chain of slices for the same scenario.
 *
 * \note            It is very important that calls to fm10000ConfigureFFUSlice
 *                  and fm10000UnconfigureFFUSlice be paired, with the exact
 *                  same slice argument.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice points to a data structure describing the slice
 *                  on which to operate.
 *
 * \param[in]       useCache specifies whether the cache may be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 *
 *****************************************************************************/
fm_status fm10000UnconfigureFFUSlice(fm_int                 sw,
                                     const fm_ffuSliceInfo *slice,
                                     fm_bool                useCache)
{
    fm_uint                 i;
    fm_uint                 j;
    fm_uint                 nKeySlices;
    fm_uint                 nActionSlices;
    fm_uint                 nEntries;
    fm_uint                 dataSize;
    fm_uint                 nReadEntries;
    fm_uint                 nWriteEntries;
    fm_registerSGListEntry *sgList;
    fm_uint                 sgIndex = 0;
    fm_uint32 *             data;
    fm_uint32 *             dataPtr;
    fm_cleanupListEntry *   cleanupList = NULL;
    fm_status               err = FM_OK;
    fm_bool                 regLockTaken = FALSE;
    
    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->validScenarios = %x, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "useCache = %s\n",
                  sw,
                  slice->validScenarios,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);

    nKeySlices    = 1 + slice->keyEnd - slice->keyStart;
    nActionSlices = 1 + slice->actionEnd - slice->keyEnd;

    /* Cover the worst case (validScenarios == 0xffffffff) */
    nEntries = ((FM10000_FFU_SLICE_CFG_ENTRIES_0 * nKeySlices) + nActionSlices);
    FM_ALLOC_TEMP_ARRAY(sgList, fm_registerSGListEntry, nEntries);

    dataSize = ( ((FM10000_FFU_SLICE_CFG_WIDTH * FM10000_FFU_SLICE_CFG_ENTRIES_0) * nKeySlices) +
                 (FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH * nActionSlices) );
    FM_ALLOC_TEMP_ARRAY(data, fm_uint32, dataSize);
    dataPtr = data;

    for (i = 0 ; i < nActionSlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex + i],
                                  &fm10000CacheFfuSliceCascadeAction,
                                  1,
                                  i + slice->keyEnd,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr+= FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH;
    }
    sgIndex += nActionSlices;

    nReadEntries = sgIndex;

    /**************************************************
     * We only need to do a write (but not read) on
     * FFU_SLICE_CFG, because it is not shared
     * between the scenario, so we can completely
     * overwrite it.
     **************************************************/
    for (i = 0 ; i < nKeySlices ; i++)
    {
        for (j = 0 ; j < FM10000_FFU_SLICE_CFG_ENTRIES_0 ; j++)
        {
            if (slice->validScenarios & (1 << j))
            {
                FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                          &fm10000CacheFfuSliceCfg,
                                          1,
                                          j,
                                          i + slice->keyStart,
                                          FM_REGS_CACHE_INDEX_UNUSED,
                                          dataPtr,
                                          FALSE);
                sgIndex++;
                dataPtr += FM10000_FFU_SLICE_CFG_WIDTH;
            }
        }
    }

    nWriteEntries = sgIndex;

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, nReadEntries, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    sgIndex = 0;

    for (i = 0 ; i < nActionSlices ; i++)
    {
        dataPtr   = sgList[sgIndex++].data; /* FFU_SLICE_CASCADE_ACTION */
        *dataPtr &= ~slice->validScenarios;
    }

    /* FFU_SLICE_CFG will be cleared for all scenarios that are part of
     * validScenarios. */
    err = fmRegCacheWrite(sw, nWriteEntries, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    FM_FREE_TEMP_ARRAYS();
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000UnconfigureFFUSlice */




/*****************************************************************************/
/** fm10000SetFFURule
 * \ingroup lowlevFfu10k
 *
 * \desc            Writes a rule into the Filtering & Forwarding unit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice is a user-supplied data structure describing the
 *                  slice and the case on which to operate.
 *
 * \param[in]       ruleIndex is the index of the rule to set.
 *
 * \param[in]       valid indicates whether the rule is valid.
 *
 * \param[in]       ruleKey is an array of ''fm_fm10000FfuSliceKey'' structures,
 *                  one element for each slice in the chain containing the key 
 *                  (which is slice->keyEnd - slice->keyStart + 1 elements).
 *
 * \param[in]       actionList is an array of type ''fm_ffuAction'', one
 *                  element for each slice in the chain containing the actions
 *                  to be executed if the rule hits.
 *                                                                      \lb\lb
 *                  The number of elements in the actionList array should
 *                  be (slice->actionEnd - slice->keyEnd + 1).
 *
 * \param[in]       live indicates whether the FFU is currently running.
 *                  Since an FFU rule exists in multiple registers
 *                  which cannot be updated atomically, overwriting one
 *                  valid rule with another while the FFU is running
 *                  could result in an undesirable "hybrid" state existing
 *                  temporarily.  To avoid this, fmFFUSetRule can set
 *                  the old rule to invalid before writing the new rule.
 *                  (However, this will still result in the rule
 *                  temporarily "blinking out.")  If the FFU is not
 *                  currently running, set live to false to avoid this
 *                  special handling.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ACL_PRECEDENCE if precedence is
 *                  not in the range 0 - FM_MAX_ACL_PRECEDENCE.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any other parameter is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetFFURule(fm_int                       sw,
                            const fm_ffuSliceInfo *      slice,
                            fm_uint16                    ruleIndex,
                            fm_bool                      valid,
                            const fm_fm10000FfuSliceKey *ruleKey,
                            const fm_ffuAction *         actionList,
                            fm_bool                      live,
                            fm_bool                      useCache)
{
    fm_registerSGListEntry  sgList[FM10000_FFU_SLICE_TCAM_ENTRIES_1 +
                                   FM10000_FFU_SLICE_SRAM_ENTRIES_1];
    fm_uint                 nKeySlices;
    fm_uint                 nActionSlices;
    fm_uint32               data[(FM10000_FFU_SLICE_TCAM_ENTRIES_1 + 1) *
                                  FM10000_FFU_SLICE_TCAM_WIDTH];
    fm_uint32 *             dataPtr;
    fm_uint                 sgIndex;
    fm_int                  i;
    fm_uint32               mask[2];
    fm_uint32               value[2];
    fm_uint32               key32[2];
    fm_uint32               keyInvert32[2];
    fm_byte                 bitPair;
    fm_regsCacheKeyValid    keyValid;
    fm_uint32               idx[FM_REGS_CACHE_MAX_INDICES];
    fm_status               err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "ruleIndex = %u, "
                  "live = %s, "
                  "useCache = %s\n",
                  sw,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  ruleIndex,
                  FM_BOOLSTRING(live),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(ruleIndex < FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);

    nKeySlices    = 1 + slice->keyEnd - slice->keyStart;
    nActionSlices = 1 + slice->actionEnd - slice->keyEnd;

    /* Translate the key part of the rule for all the condition slices */
    FM_CLEAR(data);

    dataPtr = data;
    idx[0] = ruleIndex;
    for (i = 0 ; (fm_uint) i < nKeySlices ; i++)
    {
        idx[1]  = slice->keyStart + i;

        FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                                  &fm10000CacheFfuSliceTcam,
                                  1,
                                  ruleIndex,
                                  slice->keyStart + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        /* translate the 64 bit key and mask to a 32 bit array of key and
         * mask. The lower 32 bit part is the same as the 64 bit one while
         * the top part mainly refer to the case location. */
        value[0] = ruleKey[i].key & ruleKey[i].keyMask & 0xffffffff;
        mask[0]  = ruleKey[i].keyMask & 0xffffffff;

        switch (slice->caseLocation[i])
        {
            case FM_FFU_CASE_NOT_MAPPED:
                value[1] = (ruleKey[i].key >> 32) & 0xff;
                mask[1]  = (ruleKey[i].keyMask >> 32) & 0xff;

                break;

            case FM_FFU_CASE_TOP_LOW_NIBBLE:
                value[1] = (ruleKey[i].kase.value & 0xf) |
                           ((ruleKey[i].key >> 32) & 0xf0);
                mask[1]  = (ruleKey[i].kase.mask & 0xf) |
                           ((ruleKey[i].keyMask >> 32) & 0xf0);

                break;

            case FM_FFU_CASE_TOP_HIGH_NIBBLE:
                value[1] = ((ruleKey[i].kase.value & 0xf) << 4) |
                           ((ruleKey[i].key >> 32) & 0xf);
                mask[1]  = ((ruleKey[i].kase.mask & 0xf) << 4) |
                           ((ruleKey[i].keyMask >> 32) & 0xf);

                break;

            default:
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
        }

        /* FFU key matching works with Key and KeyInvert. */
        fmGenerateCAMKey2(value,
                          mask,
                          key32,
                          keyInvert32,
                          2);

        /* save the Bit0 pair from key and keyInvert in the local cache */
        bitPair = (keyInvert32[0] & 0x1) << 1;
        bitPair |= (key32[0] & 0x1);
        switch ( bitPair )
        {
            case 0:
                /* Bit0 unused for lookups, key and keyInvert set to '1' */ 
                keyValid = FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_0;
                break;

            case 1:
                /* Bit0 of key is '1' */
                keyValid = FM_REGS_CACHE_KEY_IS_1;
                break;
        
            case 2:
                /* Bit 0 of keyInvert is '1' */
                keyValid = FM_REGS_CACHE_KEYINVERT_IS_1;
                break;

            case 3:
            default:
                /* this shouldn't happen, assert if it does*/
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_FFU, 
                                       FALSE,
                                       err = FM_FAIL,
                                       "fm10000SetFFURule: unexpected CAM key "
                                       "generated: slice %d, rule %d, bitPair %u",
                                       i,
                                       ruleIndex,
                                       bitPair);

                /* the following assignment is to silence the compiler */
                keyValid = FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1;
                break;

        }  /* end switch (bitPair) */
        
        /* save Bit0 of key and keyInvert in the local cache */
        err = fmRegCacheWriteKeyValid(sw,
                                      &fm10000CacheFfuSliceTcam,
                                      idx,
                                      keyValid);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

        /* if the rule is invalid set Bit0 of both key and keyInvert.
         * The rule is also invalidated if applied in live mode. */
        if ( (valid == FALSE) || (live == TRUE) )
        {
            key32[0] |= 0x1;
            keyInvert32[0] |= 0x1;
        }

        /* Apply the translated Key and KeyInvert. */
        FM_ARRAY_SET_FIELD(dataPtr,
                           FM10000_FFU_SLICE_TCAM,
                           Key,
                           key32[0]);
        FM_ARRAY_SET_FIELD(dataPtr,
                           FM10000_FFU_SLICE_TCAM,
                           KeyTop,
                           key32[1]);

        FM_ARRAY_SET_FIELD(dataPtr,
                           FM10000_FFU_SLICE_TCAM,
                           KeyInvert,
                           keyInvert32[0]);
        FM_ARRAY_SET_FIELD(dataPtr,
                           FM10000_FFU_SLICE_TCAM,
                           KeyTopInvert,
                           keyInvert32[1]);

        dataPtr += FM10000_FFU_SLICE_TCAM_WIDTH;
    }

    sgIndex = nKeySlices;

    /* Translate the action part of the rule for all the action slices */
    for (i = 0 ; (fm_uint) i < nActionSlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex + i],
                                  &fm10000CacheFfuSliceSram,
                                  1,
                                  ruleIndex,
                                  slice->keyEnd + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        err = TranslateFFUAction(sw, &(actionList[i]), dataPtr);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

        dataPtr += FM10000_FFU_SLICE_SRAM_WIDTH;
    }

    sgIndex += nActionSlices;

    err = fmRegCacheWrite(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    /* If we are in live mode, validate all target rules */
    if ( live == TRUE )
    {
        err = SetFFURuleValid(sw, slice, ruleIndex, valid, useCache); 
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    }   /* end if ( live == TRUE ) */


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFURule */




/*****************************************************************************/
/** fm10000SetFFURules
 * \ingroup lowlevFfu10k
 *
 * \desc            Writes a range of rules into the Filtering &
 *                  Forwarding unit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice is a user-supplied data structure
 *                  describing the slice on which to operate.
 *
 * \param[in]       ruleIndex is the index of the first rule to set.
 *
 * \param[in]       nRules is the number of rules to set.
 *
 * \param[in]       valid is an array indicating whether each rule is valid.
 * 
 * \param[in]       ruleKeys is a two dimensional array of 
 *                  ''fm_fm10000FfuSliceKey'' structures. The first index is per 
 *                  rule, as discussed in the note above.
 *                                                                      \lb\lb
 *                  The second index is per each slice in the chain 
 *                  containing the key (which is                            \lb
 *                  slice->keyEnd - slice->keyStart + 1 elements).
 *
 * \param[in]       actionLists is a two dimensional array of 
 *                  ''fm_ffuAction'' structures. The first index is per
 *                  rule, as discussed in the note above.
 *                                                                      \lb\lb
 *                  The second index is per slice for each slice in the chain 
 *                  containing the actions to be executed if the rule hits 
 *                  (which is slice->actionEnd - slice->keyEnd + 1 elements
 *                  per rule).
 * 
 * \param[in]       live indicates whether the FFU is currently running.
 *                  Since an FFU rule exists in multiple registers
 *                  which cannot be updated atomically, overwriting one
 *                  valid rule with another while the FFU is running
 *                  could result in an undesirable "hybrid" state existing
 *                  temporarily.  To avoid this, fm10000FFUSetRules can set
 *                  the old rule to invalid before writing the new rule.
 *                  (However, this will still result in the rule
 *                  temporarily "blinking out.")  If the FFU is not
 *                  currently running, set live to false to avoid this
 *                  special handling.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ACL_PRECEDENCE if precedence is
 *                  not in the range 0 - FM_MAX_ACL_PRECEDENCE.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if any other parameter is
 *                  invalid.
 *
 *****************************************************************************/
fm_status fm10000SetFFURules(fm_int                        sw,
                             const fm_ffuSliceInfo *       slice,
                             fm_uint16                     ruleIndex,
                             fm_uint16                     nRules,
                             const fm_bool *               valid,
                             const fm_fm10000FfuSliceKey **ruleKeys,
                             const fm_ffuAction **         actionLists,
                             fm_bool                       live,
                             fm_bool                       useCache)
{
    fm_registerSGListEntry *sgList;
    fm_uint                 nKeySlices;
    fm_uint                 nActionSlices;
    fm_uint32 *             data;
    fm_uint32 *             dataPtr;
    fm_uint                 sgIndex;
    fm_int                  i;
    fm_uint                 j;
    fm_uint32               mask[2];
    fm_uint32               value[2];
    fm_uint32               key32[2];
    fm_uint32               keyInvert32[2];
    fm_byte                 bitPair;
    fm_regsCacheKeyValid    keyValid;
    fm_uint32               idx[FM_REGS_CACHE_MAX_INDICES];
    fm_cleanupListEntry *   cleanupList = NULL;
    fm_status               err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "ruleIndex = %u, "
                  "nRules = %u, "
                  "live = %s, "
                  "useCache = %s\n",
                  sw,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  ruleIndex,
                  nRules,
                  FM_BOOLSTRING(live),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(nRules > 0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(ruleIndex + nRules <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);

    nKeySlices    = 1 + slice->keyEnd - slice->keyStart;
    nActionSlices = 1 + slice->actionEnd - slice->keyEnd;

    FM_ALLOC_TEMP_ARRAY(sgList,
                        fm_registerSGListEntry,
                        nKeySlices + nActionSlices);

    FM_ALLOC_TEMP_ARRAY(data,
                        fm_uint32,
                        nRules *
                        (nKeySlices * FM10000_FFU_SLICE_TCAM_WIDTH +
                         nActionSlices * FM10000_FFU_SLICE_SRAM_WIDTH));

    /* Translate the key part of the rule for all the condition slices */
    dataPtr = data;
    for (i = 0 ; (fm_uint) i < nKeySlices ; i++)
    {
        idx[1]  = slice->keyStart + i;

        FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                                  &fm10000CacheFfuSliceTcam,
                                  nRules,
                                  ruleIndex,
                                  slice->keyStart + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        for (j = 0 ; j < nRules ; j++)
        {
            idx[0] = ruleIndex + j;

            /* translate the 64 bit key and mask to a 32 bit array of key and
             * mask. The lower 32 bit part is the same as the 64 bit one while
             * the top part mainly refer to the case location. */
            value[0] = ruleKeys[j][i].key & ruleKeys[j][i].keyMask & 0xffffffff;
            mask[0]  = ruleKeys[j][i].keyMask & 0xffffffff;

            switch (slice->caseLocation[i])
            {
                case FM_FFU_CASE_NOT_MAPPED:
                    value[1] = (ruleKeys[j][i].key >> 32) & 0xff;
                    mask[1]  = (ruleKeys[j][i].keyMask >> 32) & 0xff;

                    break;

                case FM_FFU_CASE_TOP_LOW_NIBBLE:
                    value[1] = (ruleKeys[j][i].kase.value & 0xf) |
                               ((ruleKeys[j][i].key >> 32) & 0xf0);
                    mask[1]  = (ruleKeys[j][i].kase.mask & 0xf) |
                               ((ruleKeys[j][i].keyMask >> 32) & 0xf0);

                    break;

                case FM_FFU_CASE_TOP_HIGH_NIBBLE:
                    value[1] = ((ruleKeys[j][i].kase.value & 0xf) << 4) |
                               ((ruleKeys[j][i].key >> 32) & 0xf);
                    mask[1]  = ((ruleKeys[j][i].kase.mask & 0xf) << 4) |
                               ((ruleKeys[j][i].keyMask >> 32) & 0xf);

                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
            }

            /* FFU key matching works with Key and KeyInvert. */
            fmGenerateCAMKey2(value,
                              mask,
                              key32,
                              keyInvert32,
                              2);

            /* save the Bit0 pair from key and keyInvert in the local cache */
            bitPair = (keyInvert32[0] & 0x1) << 1;
            bitPair |= (key32[0] & 0x1);
            switch ( bitPair )
            {
                case 0:
                    /* Bit0 unused for lookups, key and keyInvert set to '1' */ 
                    keyValid = FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_0;
                    break;

                case 1:
                    /* Bit0 of key is '1' */
                    keyValid = FM_REGS_CACHE_KEY_IS_1;
                    break;
            
                case 2:
                    /* Bit 0 of keyInvert is '1' */
                    keyValid = FM_REGS_CACHE_KEYINVERT_IS_1;
                    break;

                case 3:
                default:
                    /* this shouldn't happen, assert if it does*/
                    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_FFU, 
                                           FALSE,
                                           err = FM_FAIL,
                                           "fm10000SetFFURules: unexpected CAM key "
                                           "generated: slice %d, rule %d, bitPair %u",
                                           i,
                                           j,
                                           bitPair);

                    /* the following assignment is to silence the compiler */
                    keyValid = FM_REGS_CACHE_KEY_AND_KEYINVERT_BOTH_1;
                    break;

            }  /* end switch (bitPair) */
            
            /* save Bit0 of key and keyInvert in the local cache */
            err = fmRegCacheWriteKeyValid(sw,
                                          &fm10000CacheFfuSliceTcam,
                                          idx,
                                          keyValid);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

            /* if the rule is invalid set Bit0 of both key and keyInvert.
             * The rule is also invalidated if applied in live mode. */
            if ( (valid[j] == FALSE) || (live == TRUE) )
            {
                key32[0] |= 0x1;
                keyInvert32[0] |= 0x1;
            }

            /* Apply the translated Key and KeyInvert. */
            FM_ARRAY_SET_FIELD(dataPtr,
                               FM10000_FFU_SLICE_TCAM,
                               Key,
                               key32[0]);
            FM_ARRAY_SET_FIELD(dataPtr,
                               FM10000_FFU_SLICE_TCAM,
                               KeyTop,
                               key32[1]);

            FM_ARRAY_SET_FIELD(dataPtr,
                               FM10000_FFU_SLICE_TCAM,
                               KeyInvert,
                               keyInvert32[0]);
            FM_ARRAY_SET_FIELD(dataPtr,
                               FM10000_FFU_SLICE_TCAM,
                               KeyTopInvert,
                               keyInvert32[1]);

            dataPtr += FM10000_FFU_SLICE_TCAM_WIDTH;
        }
    }

    sgIndex = nKeySlices;

    /* Translate the action part of the rule for all the action slices */
    for (i = 0 ; (fm_uint) i < nActionSlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex + i],
                                  &fm10000CacheFfuSliceSram,
                                  nRules,
                                  ruleIndex,
                                  slice->keyEnd + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        for (j = 0 ; j < nRules ; j++)
        {
            err = TranslateFFUAction(sw, &(actionLists[j][i]), dataPtr);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

            dataPtr += FM10000_FFU_SLICE_SRAM_WIDTH;
        }
    }

    sgIndex += nActionSlices;

    err = fmRegCacheWrite(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    /* If we are in live mode, validate all target rules */
    if ( live == TRUE )
    {
        for (i = 0 ; i < nRules  ; i++ )
        {
            err = SetFFURuleValid(sw, slice, ruleIndex + i, valid[i], useCache);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
        }

    }   /* end if ( live == TRUE ) */


ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFURules */




/*****************************************************************************/
/** fm10000SetFFURuleValid
 * \ingroup lowlevFfu10k
 *
 * \desc            Marks a rule as valid or invalid, leaving the rest of
 *                  the rule alone.
 *
 * \note            A rule can also be rendered invalid by specifying a
 *                  0 value for the precedence in the ''fm_ffuAction''
 *                  argument to ''fm10000SetFFURule'' and ''fm10000SetFFURules''.
 *                  This function does not alter the precedence, so a rule
 *                  that is invalid by virtue of a zero precedence cannot
 *                  be made valid by calling this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice points to a user-supplied data structure
 *                  identifying the slice or chain of slices on which to 
 *                  operate.
 *
 * \param[in]       ruleIndex is the index of the rule to set.
 *
 * \param[in]       valid indicates whether the rule is valid.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if ruleIndex is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetFFURuleValid(fm_int                 sw,
                                 const fm_ffuSliceInfo *slice,
                                 fm_uint16              ruleIndex,
                                 fm_bool                valid,
                                 fm_bool                useCache)
{
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_FFU, "sw = %d\n", sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    err = SetFFURuleValid(sw, slice, ruleIndex, valid, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFURuleValid */




/*****************************************************************************/
/** fm10000GetFFURule
 * \ingroup lowlevFfu10k
 *
 * \desc            Reads a rule into the Filtering & Forwarding unit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice is a user-supplied data structure describing the
 *                  slice on which to operate.
 *
 * \param[in]       ruleIndex is the index of the rule to get.
 *
 * \param[out]      valid indicates whether the rule is valid.
 *                  (The valid bit of every slices in the rule must
 *                  be valid for the rule to be considered valid.)
 *
 * \param[out]      ruleKey is an array of ''fm_fm10000FfuSliceKey'' structures,
 *                  one element for each slice in the chain containing the key 
 *                  (which is slice->keyEnd - slice->keyStart + 1 elements),
 *                  into which the rule's key configuration will be placed by
 *                  this function.
 *
 * \param[out]      actionList is an array of type ''fm_ffuAction'', one
 *                  element for each slice in the chain containing the actions
 *                  that will be executed if the rule hits, into which the
 *                  rule's action configuration will be placed by this function.
 *                                                                      \lb\lb
 *                  The number of elements in the actionList array should
 *                  be (slice->actionEnd - slice->keyEnd + 1).
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will be returned from the cache
 *                  without querying the hardware.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_PARITY if the action SRAM has bad parity.
 * \return          FM_ERR_INVALID_REGISTER_CONTENTS if the action SRAM
 *                  could not be converted to an fm_ffuAction.
 *
 *****************************************************************************/
fm_status fm10000GetFFURule(fm_int                 sw,
                            const fm_ffuSliceInfo *slice,
                            fm_uint16              ruleIndex,
                            fm_bool *              valid,
                            fm_fm10000FfuSliceKey *ruleKey,
                            fm_ffuAction *         actionList,
                            fm_bool                useCache)
{
    return fm10000GetFFURules(sw, 
                              slice, 
                              ruleIndex, 
                              1, 
                              valid, 
                              &ruleKey,
                              &actionList, 
                              useCache);

}   /* end fm10000GetFFURule */




/*****************************************************************************/
/** fm10000GetFFURules
 * \ingroup lowlevFfu10k
 *
 * \desc            Reads a range of rules from the Filtering & Forwarding unit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice is a user-supplied data structure describing the
 *                  slice on which to operate.
 *
 * \param[in]       ruleIndex is the index of the rule to get.
 * 
 * \param[in]       nRules is the number of rules to get.
 *
 * \param[out]      valid is an array indicating whether the rule is valid.
 *                  (The valid bit of every slices in the rule must
 *                  be valid for the rule to be considered valid.)
 * 
 * \param[out]      ruleKeys is a two dimensional array of 
 *                  ''fm_fm10000FfuSliceKey'' structures. The first index is per 
 *                  rule, as discussed in the note above.
 *                                                                      \lb\lb
 *                  The second index is per each slice in the chain 
 *                  containing the key (which is                            \lb
 *                  slice->keyEnd - slice->keyStart + 1 elements).
 *
 * \param[out]      actionLists is a two dimensional array of 
 *                  ''fm_ffuAction'' structures. The first index is per
 *                  rule, as discussed in the note above.
 *                                                                      \lb\lb
 *                  The second index is per slice for each slice in the chain 
 *                  containing the actions that will be executed if the rule
 *                  hits (which is slice->actionEnd - slice->keyEnd + 1 elements
 *                  per rule).
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will be returned from the cache
 *                  without querying the hardware.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_REGISTER_CONTENTS if the action SRAM
 *                  could not be converted to an fm_ffuAction.
 *
 *****************************************************************************/
fm_status fm10000GetFFURules(fm_int                  sw,
                             const fm_ffuSliceInfo * slice,
                             fm_uint16               ruleIndex,
                             fm_uint16               nRules,
                             fm_bool *               valid,
                             fm_fm10000FfuSliceKey **ruleKeys,
                             fm_ffuAction **         actionLists,
                             fm_bool                 useCache)
{
    fm_registerSGListEntry *sgList;
    fm_uint                 nKeySlices;
    fm_uint                 nActionSlices;
    fm_uint32 *             data;
    fm_uint32 *             dataPtr;
    fm_uint                 sgIndex;
    fm_uint                 i;
    fm_uint                 j;
    fm_uint32               keyMask32[2];
    fm_uint32               keyValue32[2];
    fm_uint32               key32[2];
    fm_uint32               keyInvert32[2];
    fm_uint                 actionCommand;
    fm_uint                 actionSubCmd;
    fm_byte                 byteMask;
    fm_byte                 byteData;
    fm_byte                 arpCount;
    fm_bool                 setCmd;
    fm_bool                 floodSet;
    fm_ffuAction *          action;
    fm_uint32               glort;
    fm_int                  logicalPort;
    fm_uint32               ruleId[FM_REGS_CACHE_MAX_INDICES];
    fm_cleanupListEntry *   cleanupList = NULL;
    fm_status               err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "ruleIndex = %u, "
                  "nRules = %u, "
                  "useCache = %s\n",
                  sw,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  ruleIndex,
                  nRules,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(nRules > 0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(ruleIndex + nRules <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);

    nKeySlices    = 1 + slice->keyEnd - slice->keyStart;
    nActionSlices = 1 + slice->actionEnd - slice->keyEnd;

    FM_ALLOC_TEMP_ARRAY(sgList,
                        fm_registerSGListEntry,
                        nKeySlices + nActionSlices);

    FM_ALLOC_TEMP_ARRAY(data,
                        fm_uint32,
                        nRules *
                        (nKeySlices * FM10000_FFU_SLICE_TCAM_WIDTH +
                         nActionSlices * FM10000_FFU_SLICE_SRAM_WIDTH));

    dataPtr = data;

    for (i = 0 ; i < nKeySlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                                  &fm10000CacheFfuSliceTcam,
                                  nRules,
                                  ruleIndex,
                                  slice->keyStart + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr += (FM10000_FFU_SLICE_TCAM_WIDTH * nRules);
    }

    sgIndex = nKeySlices;

    for (i = 0 ; i < nActionSlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex + i],
                                  &fm10000CacheFfuSliceSram,
                                  nRules,
                                  ruleIndex,
                                  slice->keyEnd + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr += (FM10000_FFU_SLICE_SRAM_WIDTH * nRules);
    }

    sgIndex += nActionSlices;

    err = fmRegCacheRead(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    /**************************************************
     * translate keys
     **************************************************/
    sgIndex = 0;
    for (i = 0 ; i < nKeySlices ; i++)
    {
        dataPtr = sgList[sgIndex++].data;

        for (j = 0 ; j < nRules ; j++)
        {
            /* get the key */
            key32[0] = FM_ARRAY_GET_FIELD(dataPtr,
                                          FM10000_FFU_SLICE_TCAM,
                                          Key);
            key32[1] = FM_ARRAY_GET_FIELD(dataPtr,
                                          FM10000_FFU_SLICE_TCAM,
                                          KeyTop);

            /* get its inverted value */
            keyInvert32[0] = FM_ARRAY_GET_FIELD(dataPtr,
                                                FM10000_FFU_SLICE_TCAM,
                                                KeyInvert);
            keyInvert32[1] = FM_ARRAY_GET_FIELD(dataPtr,
                                                FM10000_FFU_SLICE_TCAM,
                                                KeyTopInvert);

            /* is this CAM key valid? */
            if (IsCamKeyValid2(key32, keyInvert32, 2) == FALSE) 
            {
                /* rule isn't valid, mark it as such */
                valid[j] = FALSE;

                /* clear bit 0 */
                key32[0] &= ~0x1;
                keyInvert32[0] &= ~0x1;

                /* If this rule was ever set, restore its real key/keyInvert */
                ruleId[0] = ruleIndex + j;
                ruleId[1] = slice->keyStart + i;
                err = fmRegCacheRestoreKeyValid(sw,
                                                &fm10000CacheFfuSliceTcam,
                                                ruleId,
                                                key32,
                                                keyInvert32);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
            }
            else
            {
                valid[j] = TRUE;
            }

            /**************************************************
             * Now process it and return the key, mask and case 
             * If the key is not valid value and mask will be 
             * zeroed 
             **************************************************/
            
            fmGenerateCAMValueMask2(key32,
                                    keyInvert32,
                                    keyValue32,
                                    keyMask32,
                                    2);

            ruleKeys[j][i].key = keyValue32[0];
            ruleKeys[j][i].keyMask = keyMask32[0];

            switch (slice->caseLocation[i])
            {
                case FM_FFU_CASE_NOT_MAPPED:
                    ruleKeys[j][i].key |= (fm_uint64)(keyValue32[1] & 0xff) << 32;
                    ruleKeys[j][i].keyMask |= (fm_uint64)(keyMask32[1] & 0xff) << 32;

                    ruleKeys[j][i].kase.value = 0;
                    ruleKeys[j][i].kase.mask = 0;

                    break;

                case FM_FFU_CASE_TOP_LOW_NIBBLE:
                    ruleKeys[j][i].key |= (fm_uint64)(keyValue32[1] & 0xf0) << 32;
                    ruleKeys[j][i].keyMask |= (fm_uint64)(keyMask32[1] & 0xf0) << 32;

                    ruleKeys[j][i].kase.value = keyValue32[1] & 0xf;
                    ruleKeys[j][i].kase.mask = keyMask32[1] & 0xf;

                    break;

                case FM_FFU_CASE_TOP_HIGH_NIBBLE:
                    ruleKeys[j][i].key |= (fm_uint64)(keyValue32[1] & 0xf) << 32;
                    ruleKeys[j][i].keyMask |= (fm_uint64)(keyMask32[1] & 0xf) << 32;

                    ruleKeys[j][i].kase.value = (keyValue32[1] & 0xf0) >> 4;
                    ruleKeys[j][i].kase.mask = (keyMask32[1] & 0xf0) >> 4;

                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
            }

            dataPtr += FM10000_FFU_SLICE_TCAM_WIDTH;
        }
    }

    /**************************************************
     * translate actions
     **************************************************/
    for (i = 0 ; i < nActionSlices ; i++)
    {
        dataPtr = sgList[sgIndex++].data;

        for (j = 0 ; j < nRules ; j++)
        {
            action = &(actionLists[j][i]);

            action->precedence = FM_ARRAY_GET_FIELD(dataPtr,
                                                    FM10000_FFU_SLICE_SRAM,
                                                    Precedence);
            action->bank = FM_ARRAY_GET_FIELD(dataPtr,
                                              FM10000_FFU_SLICE_SRAM,
                                              CounterBank);
            action->counter = FM_ARRAY_GET_FIELD(dataPtr,
                                                 FM10000_FFU_SLICE_SRAM,
                                                 CounterIndex);

            actionCommand = FM_ARRAY_GET_FIELD(dataPtr,
                                               FM10000_FFU_SLICE_SRAM,
                                               Command);

            switch (actionCommand)
            {
                case FM_ACTION_COMMAND_ROUTE_ARP:
                    action->action = FM_FFU_ACTION_ROUTE_ARP;

                    action->data.arp.arpType = FM_ARRAY_GET_BIT(dataPtr,
                                                                FM10000_FFU_SLICE_SRAM_ROUTE_ARP,
                                                                ArpType);
                    arpCount = FM_ARRAY_GET_FIELD(dataPtr,
                                                  FM10000_FFU_SLICE_SRAM_ROUTE_ARP,
                                                  ArpCount);
                    if (action->data.arp.arpType == FM_FFU_ARP_TYPE_MIN_RANGE)
                    {
                        /* Remap count value 0 --> 16 */
                        action->data.arp.count = (arpCount < FM_ACTION_ARP_CNT_MINRANGE_MINVALUE ?
                                                  FM_ACTION_ARP_CNT_MINRANGE_MAXVALUE :
                                                  arpCount);
                    }
                    else
                    {
                        action->data.arp.count = arpCount;
                    }

                    action->data.arp.arpIndex = FM_ARRAY_GET_FIELD(dataPtr,
                                                                   FM10000_FFU_SLICE_SRAM_ROUTE_ARP,
                                                                   ArpIndex);
                    break;

                case FM_ACTION_COMMAND_ROUTE_GLORT:
                    floodSet = FM_ARRAY_GET_BIT(dataPtr,
                                                FM10000_FFU_SLICE_SRAM_ROUTE_GLORT,
                                                FloodSet);
                    if (floodSet)
                    {
                        action->action = FM_FFU_ACTION_ROUTE_FLOOD_DEST;
                    }
                    else
                    {
                        action->action = FM_FFU_ACTION_ROUTE_LOGICAL_PORT;
                    }

                    glort = FM_ARRAY_GET_FIELD(dataPtr,
                                               FM10000_FFU_SLICE_SRAM_ROUTE_GLORT,
                                               DGlort);
                    err = fmGetGlortLogicalPort(sw,
                                                glort,
                                                &logicalPort);
                    if (err == FM_ERR_INVALID_PORT)
                    {
                        action->action = FM_FFU_ACTION_ROUTE_GLORT;
                        action->data.glort = glort;
                        err = FM_OK;
                    }
                    else
                    {
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
                        action->data.logicalPort = logicalPort;
                    }
                    break;

                case FM_ACTION_COMMAND_BIT_SET:
                    actionSubCmd = FM_ARRAY_GET_FIELD(dataPtr,
                                                      FM10000_FFU_SLICE_SRAM_SET_BITS,
                                                      SubCommand);
                    byteMask = FM_ARRAY_GET_FIELD(dataPtr,
                                                  FM10000_FFU_SLICE_SRAM_SET_BITS,
                                                  ByteMask);
                    byteData = FM_ARRAY_GET_FIELD(dataPtr,
                                                  FM10000_FFU_SLICE_SRAM_SET_BITS,
                                                  ByteData);

                    switch (actionSubCmd)
                    {
                        case FM_ACTION_SUBCMD_FLAG:
                            if (byteMask == 0)
                            {
                                action->action = FM_FFU_ACTION_NOP;
                            }
                            else
                            {
                                action->action = FM_FFU_ACTION_SET_FLAGS;
                                SetActionFlag(&(action->data.flags),
                                              FM_FFU_FLAG_NOP, 0xff);
                                SetActionFlag(&(action->data.flags),
                                              FM_FFU_FLAG_CLEAR, byteMask);
                                SetActionFlag(&(action->data.flags),
                                              FM_FFU_FLAG_SET, byteData);
                            }
                            break;

                        case FM_ACTION_SUBCMD_TRIG:
                            action->action = FM_FFU_ACTION_SET_TRIGGER;
                            action->data.trigger.value = byteData;
                            action->data.trigger.mask  = byteMask;
                            break;

                        case FM_ACTION_SUBCMD_USR:
                            action->action = FM_FFU_ACTION_SET_USER;
                            action->data.user.value = byteData;
                            action->data.user.mask  = byteMask;
                            break;

                        default:
                            err = FM_ERR_INVALID_REGISTER_CONTENTS;
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

                    }   /* end switch (actionSubCmd) */

                    break;

                case FM_ACTION_COMMAND_FIELD_SET:
                    action->action = FM_FFU_ACTION_SET_FIELDS;

                    actionSubCmd = FM_ARRAY_GET_BIT(dataPtr,
                                                    FM10000_FFU_SLICE_SRAM_SET_PRI,
                                                    SubCommand);
                    if (actionSubCmd == FM_ACTION_SUBCMD_SET_DSCP)
                    {
                        setCmd = FM_ARRAY_GET_BIT(dataPtr,
                                                  FM10000_FFU_SLICE_SRAM_SET_PRI,
                                                  SetDSCP);
                        if (setCmd)
                        {
                            action->data.fields.fieldType = FM_FFU_FIELD_DSCP;

                            action->data.fields.fieldValue = FM_ARRAY_GET_FIELD(dataPtr,
                                                                                FM10000_FFU_SLICE_SRAM_SET_PRI,
                                                                                DSCP);
                        }
                        else
                        {
                            action->data.fields.fieldType = FM_FFU_FIELD_NEITHER;
                        }

                        action->data.fields.setVpri = FM_ARRAY_GET_BIT(dataPtr,
                                                                       FM10000_FFU_SLICE_SRAM_SET_PRI,
                                                                       SetVPRI);
                        action->data.fields.setPri = FM_ARRAY_GET_BIT(dataPtr,
                                                                      FM10000_FFU_SLICE_SRAM_SET_PRI,
                                                                      SetPRI);
                        action->data.fields.priority = FM_ARRAY_GET_FIELD(dataPtr,
                                                                          FM10000_FFU_SLICE_SRAM_SET_PRI,
                                                                          PRI);
                    }
                    else
                    {
                        action->data.fields.fieldType = FM_FFU_FIELD_VLAN;

                        action->data.fields.fieldValue = FM_ARRAY_GET_FIELD(dataPtr,
                                                                            FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                                                            VLAN);
                        action->data.fields.txTag = FM_ARRAY_GET_FIELD(dataPtr,
                                                                       FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                                                       TxTag);

                        action->data.fields.setVpri = FM_ARRAY_GET_BIT(dataPtr,
                                                                       FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                                                       SetVPRI);
                        action->data.fields.setPri = FM_ARRAY_GET_BIT(dataPtr,
                                                                      FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                                                      SetPRI);
                        action->data.fields.priority = FM_ARRAY_GET_FIELD(dataPtr,
                                                                          FM10000_FFU_SLICE_SRAM_SET_VLAN,
                                                                          PRI);
                    }

                    break;

            }

            dataPtr += FM10000_FFU_SLICE_SRAM_WIDTH;
        }
    }


ABORT:
    FM_FREE_TEMP_ARRAYS();
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetFFURules */




/*****************************************************************************/
/** fm10000CopyFFURules
 * \ingroup lowlevFfu10k
 *
 * \desc            Copy a range of rules to a different index within
 *                  the same slice or chain of cascaded slices.
 *
 * \note            In live mode, copy is done "forwards" unless the
 *                  toIndex is greater than fromIndex,
 *                  in which case the copy is done "backwards."
 *                  In non-live mode, the rules are copied in the most
 *                  efficient manner without regard to order. Also, the
 *                  user needs to be aware that this function:
 *                                                                      \lb\lb
 *                  Does not prevent the copy operation if the source
 *                  and destination ranges of rules do overlap.
 *                                                                      \lb\lb
 *                  Does not check if the destination range of rules
 *                  overwrites one or more valid rules.
 *                                                                      \lb\lb
 *                  Does not mark the rules in the source range as
 *                  invalid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice points to a user-supplied data structure
 *                  identifying the slice or chain of slices on which to 
 *                  operate.
 *
 * \param[in]       fromIndex is the index of the first rule to copy.
 *
 * \param[in]       nRules is the number of rules to copy.
 *
 * \param[in]       toIndex is the destination index of the first rule.
 *
 * \param[in]       live indicates whether the FFU is currently running.
 *                  Since an FFU rule exists in multiple registers
 *                  which cannot be updated atomically, overwriting one
 *                  valid rule with another while the FFU is running
 *                  could result in an undesirable "hybrid" state existing
 *                  temporarily.  To avoid this, this function can set
 *                  the old rule to invalid before writing the new rule.
 *                  (However, this will still result in the rule
 *                  temporarily "blinking out.")  If the FFU is not
 *                  currently running, set live to false to avoid this
 *                  special handling.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If TRUE, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_SLICE if any parameter in slice
 *                  is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if fromIndex, toIndex,
 *                  or nRules are invalid.
 *
 *****************************************************************************/
fm_status fm10000CopyFFURules(fm_int                 sw,
                              const fm_ffuSliceInfo *slice,
                              fm_uint16              fromIndex,
                              fm_uint16              nRules,
                              fm_uint16              toIndex,
                              fm_bool                live,
                              fm_bool                useCache)
{
    fm_bool                 backwards;
    fm_registerSGListEntry *sgList;
    fm_uint                 nKeySlices;
    fm_uint                 nActionSlices;
    fm_uint32 *             data;
    fm_uint32 *             dataPtr;
    fm_uint                 sgIndex;
    fm_int                  i;
    fm_int                  j;
    fm_uint                 nSG;
    fm_registerSGListEntry *oldSG;
    fm_uint32               zero[FM10000_FFU_SLICE_TCAM_WIDTH];
    fm_uint32               fromIdx[FM_REGS_CACHE_MAX_INDICES];
    fm_uint32               toIdx[FM_REGS_CACHE_MAX_INDICES];
    fm_cleanupListEntry *   cleanupList = NULL;
    fm_status               err = FM_OK;
    fm_bool                 regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "slice->keyStart = %u, "
                  "slice->keyEnd = %u, "
                  "slice->actionEnd = %u, "
                  "fromIndex = %u, "
                  "nRules = %u, "
                  "toIndex = %u, "
                  "live = %s, "
                  "useCache = %s\n",
                  sw,
                  slice->keyStart,
                  slice->keyEnd,
                  slice->actionEnd,
                  fromIndex,
                  nRules,
                  toIndex,
                  FM_BOOLSTRING(live),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(nRules > 0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(fromIndex + nRules <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(toIndex + nRules <= FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(slice->keyStart < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd < FM10000_FFU_SLICE_VALID_ENTRIES,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->keyEnd >= slice->keyStart,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(slice->actionEnd >= slice->keyEnd,
                   FM_ERR_INVALID_SLICE);
    FM_API_REQUIRE(toIndex != fromIndex, FM_OK); /* nothing to do! */

    backwards     = (toIndex > fromIndex);
    nKeySlices    = 1 + slice->keyEnd - slice->keyStart;
    nActionSlices = 1 + slice->actionEnd - slice->keyEnd;

    FM_ALLOC_TEMP_ARRAY(sgList,
                        fm_registerSGListEntry,
                        nKeySlices + nActionSlices);

    FM_ALLOC_TEMP_ARRAY(data,
                        fm_uint32,
                        nRules *
                        (nKeySlices * FM10000_FFU_SLICE_TCAM_WIDTH +
                         nActionSlices * FM10000_FFU_SLICE_SRAM_WIDTH));

    dataPtr = data;

    for (i = 0 ; (fm_uint) i < nKeySlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[i],
                                  &fm10000CacheFfuSliceTcam,
                                  nRules,
                                  fromIndex,
                                  slice->keyStart + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr += (FM10000_FFU_SLICE_TCAM_WIDTH * nRules);
    }

    sgIndex = nKeySlices;

    for (i = 0 ; (fm_uint) i < nActionSlices ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex + i],
                                  &fm10000CacheFfuSliceSram,
                                  nRules,
                                  fromIndex,
                                  slice->keyEnd + i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  dataPtr,
                                  FALSE);

        dataPtr += (FM10000_FFU_SLICE_SRAM_WIDTH * nRules);
    }

    sgIndex += nActionSlices;

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    for (i = 0 ; (fm_uint) i < sgIndex ; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST_IDX0(&sgList[i], toIndex);
    }

    nSG = sgIndex;

    if (live)
    {
        oldSG = sgList;
        nSG++;
        nSG *= nRules;
        FM_ALLOC_TEMP_ARRAY(sgList, fm_registerSGListEntry, nSG);
        sgIndex = 0;

        FM_CLEAR(zero);

        for ( j = (backwards ? nRules - 1 : 0) ;
             (backwards ? j >= 0 : j < nRules) ;
             (backwards ? j-- : j++) )
        {
            FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                      oldSG[0].registerSet,
                                      1,
                                      oldSG[0].idx[0] + j,
                                      oldSG[0].idx[1],
                                      FM_REGS_CACHE_INDEX_UNUSED,
                                      zero,
                                      FALSE);
            sgIndex++;

            for (i = nKeySlices + nActionSlices - 1 ; i >= 0 ; i--)
            {
                FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                          oldSG[i].registerSet,
                                          1,
                                          oldSG[i].idx[0] + j,
                                          oldSG[i].idx[1],
                                          FM_REGS_CACHE_INDEX_UNUSED,
                                          (oldSG[i].data +
                                          (j * oldSG[i].registerSet->nWords)),
                                          (i == 0));
                sgIndex++;
            }
        }

        FM_API_REQUIRE(nSG == sgIndex, FM_FAIL); /* assertion */
    }

    err = fmRegCacheWrite(sw, nSG, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    /**************************************************
     * whether we are in live mode or not we must copy 
     * the cached Bit0 values of key and keyInvert 
     * from the source range to the destination range. 
     **************************************************/
    for (i = 0 ; (fm_uint) i < nRules ; i++)
    {
        fromIdx[0] = fromIndex + i;
        toIdx  [0] = toIndex + i;
        for (j = 0 ; (fm_uint) j < nKeySlices ; j++)
        {
            fromIdx[1] = slice->keyStart + j;
            toIdx  [1] = slice->keyStart + j;
            err = fmRegCacheCopyKeyValid(sw, 
                                         &fm10000CacheFfuSliceTcam,
                                         fromIdx,
                                         toIdx);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
        }
    }


ABORT:
    FM_FREE_TEMP_ARRAYS();
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000CopyFFURules */




/*****************************************************************************/
/** fm10000ConfigureFFUEaclChunk
 * \ingroup lowlevFfu10k
 *
 * \desc            Configure an Egress ACL Chunk.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       chunk is the one to configure.
 *
 * \param[in]       validScenarios bitmask of scenarios for which
 *                  that chunk is valid.
 *
 * \param[in]       dstPhysicalPortMask is a bitmask of destination ports
 *                  the chunk applies to.
 *
 * \param[in]       cascade indicate whether to start a priority hit cascade
 *                  across chunks.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ARGUMENT if chunk is not in the
 *                  range 0 - 31, or nChunks is invalid.
 * \return          FM_ERR_INVALID_DSTMASK if dstPortMask is invalid.
 *
 *****************************************************************************/
fm_status fm10000ConfigureFFUEaclChunk(fm_int     sw,
                                       fm_byte    chunk,
                                       fm_uint32  validScenarios,
                                       fm_uint64  dstPhysicalPortMask,
                                       fm_bool    cascade,
                                       fm_bool    useCache)
{
    fm_registerSGListEntry sgList[3];
    fm_uint32              portCfg[FM10000_FFU_EGRESS_PORT_CFG_ENTRIES *
                                   FM10000_FFU_EGRESS_PORT_CFG_WIDTH];
    fm_uint                i;
    fm_bool                regLockTaken = FALSE;
    fm_status              err = FM_OK;
    fm_uint32              startCascade[FM10000_FFU_EGRESS_CHUNK_CFG_WIDTH];
    fm_uint32              valid[FM10000_FFU_EGRESS_CHUNK_VALID_WIDTH];

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "chunk = %d, "
                  "validScenarios = 0x%x, "
                  "dstPhysicalPortMask = 0x%llx, "
                  "cascade = %s, "
                  "useCache = %s\n",
                  sw,
                  chunk,
                  validScenarios,
                  dstPhysicalPortMask,
                  FM_BOOLSTRING(cascade),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(chunk < FM10000_FFU_EGRESS_CHUNK_VALID_ENTRIES,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(dstPhysicalPortMask <= 0xffffffffffffLL,
                   FM_ERR_INVALID_DSTMASK);

    /**************************************************
     * We need to do a read-modify-write on 
     * FFU_EGRESS_PORT_CFG.
     **************************************************/
    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheFfuEgressPortCfg,
                              FM10000_FFU_EGRESS_PORT_CFG_ENTRIES,
                              0,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              portCfg,
                              FALSE);

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, 1, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    for (i = 0 ; i < FM10000_FFU_EGRESS_PORT_CFG_ENTRIES ; i++)
    {
        if (dstPhysicalPortMask & (fm_uint64)(1LL << i))
        {
            portCfg[i * FM10000_FFU_EGRESS_PORT_CFG_WIDTH] |= (1 << chunk);
        }
        else
        {
            portCfg[i * FM10000_FFU_EGRESS_PORT_CFG_WIDTH] &= ~(1 << chunk);
        }
    }

    FM_ARRAY_SET_BIT(startCascade,
                     FM10000_FFU_EGRESS_CHUNK_CFG,
                     StartCascade,
                     cascade);

    FM_ARRAY_SET_FIELD(valid,
                       FM10000_FFU_EGRESS_CHUNK_VALID,
                       Valid,
                       validScenarios);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheFfuEgressChunkCfg,
                              1,
                              chunk,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              startCascade,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[2],
                              &fm10000CacheFfuEgressChunkValid,
                              1,
                              chunk,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              valid,
                              FALSE);

    err = fmRegCacheWrite(sw, 3, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000ConfigureFFUEaclChunk */




/*****************************************************************************/
/** fm10000SetFFUEaclAction
 * \ingroup lowlevFfu10k
 *
 * \desc            Sets the action to apply when this specific Egress ACL
 *                  rule hit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ruleIndex is the index of the rule on which to operate.
 *
 * \param[in]       drop indicates the frame should be dropped.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ARGUMENT if ruleIndex is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetFFUEaclAction(fm_int    sw,
                                  fm_uint16 ruleIndex,
                                  fm_bool   drop,
                                  fm_bool   useCache)
{
    fm_status err = FM_OK;
    fm_uint32 data[FM10000_FFU_EGRESS_CHUNK_ACTIONS_WIDTH];
    fm_bool regLockTaken = FALSE;
    fm_uint32 chunk;
    fm_uint32 index;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "ruleIndex = %d, "
                  "drop = %s, "
                  "useCache = %s\n",
                  sw,
                  ruleIndex,
                  FM_BOOLSTRING(drop),
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(ruleIndex < FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(drop <= 1, FM_ERR_INVALID_ARGUMENT);

    chunk = ruleIndex / FM10000_FFU_EGRESS_CHUNK_ACTIONS_ENTRIES;
    index = ruleIndex % FM10000_FFU_EGRESS_CHUNK_ACTIONS_ENTRIES;

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheReadSingle1D(sw,
                                 &fm10000CacheFfuEgressChunkActions,
                                 data,
                                 chunk,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    data[0] &= ~(1 << index);
    data[0] |= (drop << index);

    err = fmRegCacheWriteSingle1D(sw, 
                                  &fm10000CacheFfuEgressChunkActions,
                                  data,
                                  chunk,
                                  useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFUEaclAction */




/*****************************************************************************/
/** fm10000GetFFUEaclAction
 * \ingroup lowlevFfu10k
 *
 * \desc            Get the configured action applied when this specific
 *                  Egress ACL rule hit.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ruleIndex is the index of the rule on which to operate.
 *
 * \param[out]      drop indicates the frame should be dropped.
 *
 * \param[in]       useCache indicates whether using the cache is allowed.
 *                  If true, the value will only be written to the
 *                  hardware if it differs from the cached value.
 *                  The cache is updated regardless of the value of useCache.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_ARGUMENT if ruleIndex is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFFUEaclAction(fm_int     sw,
                                  fm_uint16  ruleIndex,
                                  fm_bool   *drop,
                                  fm_bool    useCache)
{
    fm_status err = FM_OK;
    fm_uint32 data[FM10000_FFU_EGRESS_CHUNK_ACTIONS_WIDTH];
    fm_uint32 chunk;
    fm_uint32 index;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "ruleIndex = %d, "
                  "useCache = %s\n",
                  sw,
                  ruleIndex,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    FM_API_REQUIRE(ruleIndex < FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);

    chunk = ruleIndex / FM10000_FFU_EGRESS_CHUNK_ACTIONS_ENTRIES;
    index = ruleIndex % FM10000_FFU_EGRESS_CHUNK_ACTIONS_ENTRIES;

    err = fmRegCacheReadSingle1D(sw, 
                                 &fm10000CacheFfuEgressChunkActions,
                                 data,
                                 chunk,
                                 useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);

    *drop = (data[0] >> index) & 0x1;


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetFFUEaclAction */




/*****************************************************************************/
/** fm10000SetFFUEaclCounter
 * \ingroup lowlevFfu10k
 *
 * \desc            Sets an egress ACL counter. Can be effectively used to
 *                  reset the value.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port whose counter is to be set.
 *
 * \param[in]       dropCount is the number of frames counted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetFFUEaclCounter(fm_int    sw,
                                   fm_byte   port,
                                   fm_uint64 dropCount)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "port = %d, "
                  "dropCount = %lld\n",
                  sw,
                  port,
                  dropCount );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_REQUIRE(port < FM10000_FFU_EGRESS_DROP_COUNT_ENTRIES,
                   FM_ERR_INVALID_PORT);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_FFU_EGRESS_DROP_COUNT(port, 0),
                                 dropCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000SetFFUEaclCounter */




/*****************************************************************************/
/** fm10000GetFFUEaclCounter
 * \ingroup lowlevFfu10k
 *
 * \desc            Get the number of Egress ACL Drop on a perport basis.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port whose counter is to be retrieved.
 *
 * \param[out]      dropCount is the number of frames dropped.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 * \return          FM_ERR_INVALID_PORT if port is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetFFUEaclCounter(fm_int     sw,
                                   fm_byte    port,
                                   fm_uint64 *dropCount)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_FFU,
                  "sw = %d, "
                  "port = %d\n",
                  sw,
                  port );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsFfu(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_REQUIRE(port < FM10000_FFU_EGRESS_DROP_COUNT_ENTRIES,
                   FM_ERR_INVALID_PORT);

    err = switchPtr->ReadUINT64(sw,
                                FM10000_FFU_EGRESS_DROP_COUNT(port, 0),
                                dropCount);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_FFU, err);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_FFU, err);

}   /* end fm10000GetFFUEaclCounter */

