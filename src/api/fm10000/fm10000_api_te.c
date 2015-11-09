/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_te.c
 * Creation Date:  November 28, 2013
 * Description:    Low-level API for manipulating the Tunneling Engine.
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

#define FM_API_REQUIRE(expr, failCode)       \
    if ( !(expr) )                           \
    {                                        \
        err = failCode;                      \
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE,   \
                            failCode);       \
    }

/* Define TE_DATA maximum block size in different representation */
#define FM10000_TE_DATA_LENGTH_SIZE     FM_FIELD_UNSIGNED_MAX(FM10000_TE_LOOKUP, DataLength)
#define FM10000_TE_DATA_LENGTH_SIZE_32 (FM10000_TE_DATA_LENGTH_SIZE * FM10000_TE_DATA_WIDTH)
#define FM10000_TE_DATA_LENGTH_SIZE_16 (FM10000_TE_DATA_LENGTH_SIZE_32 * 2)

/* TE_LOOKUP Last position on 16-bit boundary */
#define FM10000_TE_LOOKUP_LAST_POS     (FM10000_TE_LOOKUP_b_Last - 16)

#define BUFFER_SIZE                     1024

#define FM10000_TE_MAX_SYNC_RETRY       1000

#define FM10000_TE_ENCAP_VERSION_MAX    3
#define FM10000_TE_MODE_MAX             1

#define FM_IND_PRINT(...)                                      \
    {                                                          \
        fm_int space;                                          \
        for (space = (deep * 4) ; space > 0 ; space-- )        \
        {                                                      \
            FM_LOG_PRINT(" ");                                 \
        }                                                      \
        FM_LOG_PRINT(__VA_ARGS__);                             \
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

/* Variable used to keep track of the sync statistic */
fm_uint64 syncRetry[FM10000_TE_FRAMES_IN_ENTRIES] = {0};
fm_uint64 syncCall[FM10000_TE_FRAMES_IN_ENTRIES] = {0};
fm_uint64 maxSyncRetry[FM10000_TE_FRAMES_IN_ENTRIES] = {0};

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmSupportsTe
 * \ingroup intLowlevTe10k
 *
 * \desc            Determines whether the specified switch has an Tunneling
 *                  Engine.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          TRUE if the switch has an TE.
 * \return          FALSE if the switch does not have an TE.
 *
 *****************************************************************************/
static inline fm_bool fmSupportsTe(fm_int sw)
{
    return ( (fmRootApi->fmSwitchStateTable[sw]->switchFamily ==
              FM_SWITCH_FAMILY_FM10000) );

}   /* end fmSupportsTe */




/*****************************************************************************/
/** PrintChecksum
 * \ingroup intlowlevTe10k
 *
 * \desc            Dump the checksum action for debugging purpose.
 * 
 * \param[in]       checksumAct is the checksum action to print.
 *
 * \return          None
 *
 *****************************************************************************/
static void PrintChecksum(fm_fm10000TeChecksumAction checksumAct)
{
    switch (checksumAct)
    {
        case FM_FM10000_TE_CHECKSUM_TRAP:
            FM_LOG_PRINT("FM_FM10000_TE_CHECKSUM_TRAP\n");
            break;

        case FM_FM10000_TE_CHECKSUM_ZERO:
            FM_LOG_PRINT("FM_FM10000_TE_CHECKSUM_ZERO\n");
            break;

        case FM_FM10000_TE_CHECKSUM_COMPUTE:
            FM_LOG_PRINT("FM_FM10000_TE_CHECKSUM_COMPUTE\n");
            break;

        case FM_FM10000_TE_CHECKSUM_HEADER:
            FM_LOG_PRINT("FM_FM10000_TE_CHECKSUM_HEADER\n");
            break;

        default:
            FM_LOG_PRINT("Invalid Value\n");
            break;
    }
}




/*****************************************************************************/
/** PrintTrap
 * \ingroup intlowlevTe10k
 *
 * \desc            Dump the trap action for debugging purpose.
 * 
 * \param[in]       trapAct is the trap action to print.
 *
 * \return          None
 *
 *****************************************************************************/
static void PrintTrap(fm_fm10000TeTrapAction trapAct)
{
    switch (trapAct)
    {
        case FM_FM10000_TE_TRAP_DGLORT0:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_DGLORT0\n");
            break;

        case FM_FM10000_TE_TRAP_DGLORT1:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_DGLORT1\n");
            break;

        case FM_FM10000_TE_TRAP_DGLORT2:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_DGLORT2\n");
            break;

        case FM_FM10000_TE_TRAP_DGLORT3:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_DGLORT3\n");
            break;

        case FM_FM10000_TE_TRAP_PASS:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_PASS\n");
            break;

        case FM_FM10000_TE_TRAP_PASS_WITH_ERROR:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_PASS_WITH_ERROR\n");
            break;

        case FM_FM10000_TE_TRAP_DROP:
            FM_LOG_PRINT("FM_FM10000_TE_TRAP_DROP\n");
            break;

        default:
            FM_LOG_PRINT("Invalid Value\n");
            break;
    }
}




/*****************************************************************************/
/** PrintLine
 * \ingroup intlowlevTe10k
 *
 * \desc            Print range style debug line
 * 
 * \param[in]       start is the first limit of the range.
 * 
 * \param[in]       end is the last limit of the range.
 * 
 * \param[in]       line refer to the buffer to append.
 * 
 * \param[in]       wide refer to the start and end length.
 *
 * \return          None
 *
 *****************************************************************************/
static void PrintLine(fm_uint start, fm_uint end, const char *line, fm_bool wide)
{
    if (wide)
    {
        if (start == end)
        {
            FM_LOG_PRINT("      %05u: %s\n", start, line);
        }
        else
        {
            FM_LOG_PRINT("%05u-%05u: %s\n", start, end, line);
        }
    }
    else
    {
        if (start == end)
        {
            FM_LOG_PRINT("    %03u: %s\n", start, line);
        }
        else
        {
            FM_LOG_PRINT("%03u-%03u: %s\n", start, end, line);
        }
    }
}




/*****************************************************************************/
/** PrintTeDataBlock
 * \ingroup intlowlevTe10k
 *
 * \desc            Print Tunneling engine data block
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       hash specify the type of the decoded block. TRUE for
 *                  hash block, FALSE for direct one.
 * 
 * \param[in]       encap specify the direction of the decoded block. TRUE for
 *                  encapsulation, FALSE for decapsulation.
 * 
 * \param[in]       teData is an array of user-supplied data structure of type
 *                  ''fm_fm10000TeLookup'' that would be decoded.
 * 
 * \param[in]       teDataLength is the number of element referenced by teData.
 * 
 * \param[in]       deep is the current layer of recursivity. This is used to
 *                  catch infinite loop in the configuration.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PrintTeDataBlock(fm_int            sw,
                                  fm_int            te,
                                  fm_bool           hash,
                                  fm_bool           encap,
                                  fm_fm10000TeData *teData,
                                  fm_int            teDataLength,
                                  fm_int            deep)
{
    fm_int i;
    fm_int j;
    fm_int teDataReturnLength;
    fm_fm10000TeData teDataLocal[FM10000_TE_MAX_DATA_BIN_SIZE];
    char addrBuffer[48];
    fm_status err = FM_OK;

    /* Do not dereference TeDataBlock that are deeper than 10 layer */
    if (deep > 10)
    {
        FM_LOG_PRINT("\n!!!Error: Next Pointer Loop detected!!!\n\n");
        return err;
    }

    for (i = 0 ; i < teDataLength ; i++)
    {
        switch (teData[i].blockType)
        {
            case FM_FM10000_TE_DATA_BLOCK_POINTER:
                FM_IND_PRINT("FM_FM10000_TE_DATA_BLOCK_POINTER Type which refer to %d of length %d %s last frag\n",
                             teData[i].blockVal.nextLookup.dataPtr,
                             teData[i].blockVal.nextLookup.dataLength,
                             teData[i].blockVal.nextLookup.last ? "with" : "without");
                FM_IND_PRINT("Decoding this referenced block:\n");

                j = 0;
                if (teData[i].blockVal.nextLookup.last == 0)
                {
                    teDataLocal[j++].blockType = FM_FM10000_TE_DATA_BLOCK_POINTER;
                }

                if (hash)
                {
                    teDataLocal[j].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;
                }
                else if (encap)
                {
                    teDataLocal[j].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP;
                }
                else
                {
                    teDataLocal[j].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP;
                }

                err = fm10000GetTeData(sw,
                                       te,
                                       teData[i].blockVal.nextLookup.dataPtr,
                                       teData[i].blockVal.nextLookup.dataLength,
                                       encap,
                                       teDataLocal,
                                       FM10000_TE_MAX_DATA_BIN_SIZE,
                                       &teDataReturnLength,
                                       FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                err = PrintTeDataBlock(sw,
                                       te,
                                       hash,
                                       encap,
                                       teDataLocal,
                                       teDataReturnLength,
                                       deep + 1);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                break;

            case FM_FM10000_TE_DATA_BLOCK_FLOW_KEY:
                FM_IND_PRINT("FM_FM10000_TE_DATA_BLOCK_FLOW_KEY Type with keys 0x%04x:\n",
                             teData[i].blockVal.flowKeyVal.searchKeyConfig);
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_VSI_TEP)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_VSI_TEP:%d\n",
                                 teData[i].blockVal.flowKeyVal.vsiTep);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_VNI)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_VNI:%d\n",
                                 teData[i].blockVal.flowKeyVal.vni);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_DMAC)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_DMAC:0x%012llx\n",
                                 teData[i].blockVal.flowKeyVal.dmac);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_SMAC)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_SMAC:0x%012llx\n",
                                 teData[i].blockVal.flowKeyVal.smac);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_VLAN)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_VLAN:%d\n",
                                 teData[i].blockVal.flowKeyVal.vlan);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_DIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.flowKeyVal.dip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_KEY_DIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_SIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.flowKeyVal.sip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_KEY_SIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_L4SRC)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_L4SRC:%d\n",
                                 teData[i].blockVal.flowKeyVal.l4Src);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_L4DST)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_L4DST:%d\n",
                                 teData[i].blockVal.flowKeyVal.l4Dst);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_PROT)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_PROT:%d\n",
                                 teData[i].blockVal.flowKeyVal.protocol);
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_UDP)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_UDP:Match\n");
                }
                if (teData[i].blockVal.flowKeyVal.searchKeyConfig & FM10000_TE_KEY_TCP)
                {
                    FM_IND_PRINT("    FM10000_TE_KEY_TCP:Match\n");
                }
                break;

            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP:
                FM_IND_PRINT("FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP Type with keys 0x%04x:\n",
                             teData[i].blockVal.flowEncapVal.encapConfig);
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_VNI)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_VNI:%d\n",
                                 teData[i].blockVal.flowEncapVal.vni);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_COUNTER)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_COUNTER:%d\n",
                                 teData[i].blockVal.flowEncapVal.counterIdx);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_DMAC)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_DMAC:0x%012llx\n",
                                 teData[i].blockVal.flowEncapVal.dmac);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_SMAC)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_SMAC:0x%012llx\n",
                                 teData[i].blockVal.flowEncapVal.smac);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_VLAN)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_VLAN:%d\n",
                                 teData[i].blockVal.flowEncapVal.vlan);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_DIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.flowEncapVal.dip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_DIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_SIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.flowEncapVal.sip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_SIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_L4SRC)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_L4SRC:%d\n",
                                 teData[i].blockVal.flowEncapVal.l4Src);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_L4DST)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_L4DST:%d\n",
                                 teData[i].blockVal.flowEncapVal.l4Dst);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_TTL)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_TTL:%d\n",
                                 teData[i].blockVal.flowEncapVal.ttl);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_NGE)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_NGE: Mask = 0x%04x\n",
                                 teData[i].blockVal.flowEncapVal.ngeMask);

                    FM_IND_PRINT("        Data[0..3]   = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.flowEncapVal.ngeData[0], 
                                 teData[i].blockVal.flowEncapVal.ngeData[1], 
                                 teData[i].blockVal.flowEncapVal.ngeData[2], 
                                 teData[i].blockVal.flowEncapVal.ngeData[3]);
                    FM_IND_PRINT("        Data[4..7]   = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.flowEncapVal.ngeData[4], 
                                 teData[i].blockVal.flowEncapVal.ngeData[5], 
                                 teData[i].blockVal.flowEncapVal.ngeData[6], 
                                 teData[i].blockVal.flowEncapVal.ngeData[7]);
                    FM_IND_PRINT("        Data[8..11]  = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.flowEncapVal.ngeData[8], 
                                 teData[i].blockVal.flowEncapVal.ngeData[9], 
                                 teData[i].blockVal.flowEncapVal.ngeData[10], 
                                 teData[i].blockVal.flowEncapVal.ngeData[11]);
                    FM_IND_PRINT("        Data[12..15] = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.flowEncapVal.ngeData[12], 
                                 teData[i].blockVal.flowEncapVal.ngeData[13], 
                                 teData[i].blockVal.flowEncapVal.ngeData[14], 
                                 teData[i].blockVal.flowEncapVal.ngeData[15]);
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_TUNNEL:Present\n");
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_NGE_TIME)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_NGE_TIME:Present\n");
                }
                if (teData[i].blockVal.flowEncapVal.encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL_PTR)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_ENCAP_TUNNEL_PTR Type which refer to %d of length %d\n",
                             teData[i].blockVal.flowEncapVal.tunnelIdx.dataPtr,
                             teData[i].blockVal.flowEncapVal.tunnelIdx.dataLength);
                    FM_IND_PRINT("    Decoding this referenced block:\n");

                    teDataLocal[0].blockType = FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA;

                    err = fm10000GetTeData(sw,
                                           te,
                                           teData[i].blockVal.flowEncapVal.tunnelIdx.dataPtr,
                                           teData[i].blockVal.flowEncapVal.tunnelIdx.dataLength,
                                           encap,
                                           teDataLocal,
                                           FM10000_TE_MAX_DATA_BIN_SIZE,
                                           &teDataReturnLength,
                                           FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                    err = PrintTeDataBlock(sw,
                                           te,
                                           hash,
                                           encap,
                                           teDataLocal,
                                           teDataReturnLength,
                                           deep + 1);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                break;

            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP:
                FM_IND_PRINT("FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP Type with keys 0x%04x:\n",
                             teData[i].blockVal.flowDecapVal.decapConfig);

                if (teData[i].blockVal.flowDecapVal.outerHeader == FM_FM10000_TE_OUTER_HEADER_DELETE)
                {
                    FM_IND_PRINT("    FM_FM10000_TE_OUTER_HEADER_DELETE\n");
                }
                else if (teData[i].blockVal.flowDecapVal.outerHeader == FM_FM10000_TE_OUTER_HEADER_LEAVE_AS_IS)
                {
                    FM_IND_PRINT("    FM_FM10000_TE_OUTER_HEADER_LEAVE_AS_IS\n");
                }
                else if (teData[i].blockVal.flowDecapVal.outerHeader == FM_FM10000_TE_OUTER_HEADER_MOVE_TO_END)
                {
                    FM_IND_PRINT("    FM_FM10000_TE_OUTER_HEADER_MOVE_TO_END\n");
                }

                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_DGLORT)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_DGLORT:0x%04x\n",
                                 teData[i].blockVal.flowDecapVal.dglort);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_DMAC)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_DMAC:0x%012llx\n",
                                 teData[i].blockVal.flowDecapVal.dmac);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_SMAC)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_SMAC:0x%012llx\n",
                                 teData[i].blockVal.flowDecapVal.smac);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_VLAN)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_VLAN:%d\n",
                                 teData[i].blockVal.flowDecapVal.vlan);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_DIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.flowDecapVal.dip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_DIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_SIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.flowDecapVal.sip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_SIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_TTL)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_TTL:%d\n",
                                 teData[i].blockVal.flowDecapVal.ttl);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_L4SRC)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_L4SRC:%d\n",
                                 teData[i].blockVal.flowDecapVal.l4Src);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_L4DST)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_L4DST:%d\n",
                                 teData[i].blockVal.flowDecapVal.l4Dst);
                }
                if (teData[i].blockVal.flowDecapVal.decapConfig & FM10000_TE_FLOW_DECAP_COUNTER)
                {
                    FM_IND_PRINT("    FM10000_TE_FLOW_DECAP_COUNTER:%d\n",
                                 teData[i].blockVal.flowDecapVal.counterIdx);
                }
                break;

            case FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA:
                FM_IND_PRINT("FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA Type with keys 0x%04x:\n",
                             teData[i].blockVal.tunnelVal.tunnelConfig);

                if (teData[i].blockVal.tunnelVal.tunnelType == FM_FM10000_TE_TUNNEL_TYPE_VXLAN)
                {
                    FM_IND_PRINT("    FM_FM10000_TE_TUNNEL_TYPE_VXLAN\n");
                }
                else if (teData[i].blockVal.tunnelVal.tunnelType == FM_FM10000_TE_TUNNEL_TYPE_NGE)
                {
                    FM_IND_PRINT("    FM_FM10000_TE_TUNNEL_TYPE_NGE\n");
                }
                else if (teData[i].blockVal.tunnelVal.tunnelType == FM_FM10000_TE_TUNNEL_TYPE_NVGRE)
                {
                    FM_IND_PRINT("    FM_FM10000_TE_TUNNEL_TYPE_NVGRE\n");
                }

                fmDbgConvertIPAddressToString(&(teData[i].blockVal.tunnelVal.dip), addrBuffer);
                FM_IND_PRINT("    Required Encap DIP:%s\n",
                             addrBuffer);
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_SIP)
                {
                    fmDbgConvertIPAddressToString(&(teData[i].blockVal.tunnelVal.sip), addrBuffer);
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_SIP:%s\n",
                                 addrBuffer);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TOS)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_TOS:%d\n",
                                 teData[i].blockVal.tunnelVal.tos);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TTL)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_TTL:%d\n",
                                 teData[i].blockVal.tunnelVal.ttl);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4DST)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_L4DST:%d\n",
                                 teData[i].blockVal.tunnelVal.l4Dst);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4SRC)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_L4SRC:%d\n",
                                 teData[i].blockVal.tunnelVal.l4Src);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_COUNTER:%d\n",
                                 teData[i].blockVal.tunnelVal.counterIdx);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_NGE)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_NGE: Mask = 0x%04x\n",
                                 teData[i].blockVal.tunnelVal.ngeMask);
                    FM_IND_PRINT("        Data[0..3]   = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.tunnelVal.ngeData[0],
                                 teData[i].blockVal.tunnelVal.ngeData[1],
                                 teData[i].blockVal.tunnelVal.ngeData[2],
                                 teData[i].blockVal.tunnelVal.ngeData[3]);
                    FM_IND_PRINT("        Data[4..7]   = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.tunnelVal.ngeData[4], 
                                 teData[i].blockVal.tunnelVal.ngeData[5], 
                                 teData[i].blockVal.tunnelVal.ngeData[6], 
                                 teData[i].blockVal.tunnelVal.ngeData[7]);
                    FM_IND_PRINT("        Data[8..11]  = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.tunnelVal.ngeData[8], 
                                 teData[i].blockVal.tunnelVal.ngeData[9], 
                                 teData[i].blockVal.tunnelVal.ngeData[10], 
                                 teData[i].blockVal.tunnelVal.ngeData[11]);
                    FM_IND_PRINT("        Data[12..15] = 0x%08x 0x%08x 0x%08x 0x%08x\n", 
                                 teData[i].blockVal.tunnelVal.ngeData[12], 
                                 teData[i].blockVal.tunnelVal.ngeData[13], 
                                 teData[i].blockVal.tunnelVal.ngeData[14], 
                                 teData[i].blockVal.tunnelVal.ngeData[15]);
                }
                if (teData[i].blockVal.tunnelVal.tunnelConfig & FM10000_TE_TUNNEL_ENCAP_NGE_TIME)
                {
                    FM_IND_PRINT("    FM10000_TE_TUNNEL_ENCAP_NGE_TIME:Present\n");
                }
                break;

            default:
                FM_IND_PRINT("Undefined Type %d\n", teData[i].blockType);
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                break;
        }
    }

ABORT:

    return err;
}




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000TeInit
 * \ingroup intLowlevTe10k
 *
 * \desc            Private initialization function called from 
 *                  ''fm10000PostBootSwitch''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000TeInit(fm_int sw)
{
    fm_status              err;
    fm_uint32              teCfg[FM10000_TE_CFG_WIDTH];
    fm_registerSGListEntry sgList;
    fm_int                 i;

    FM_LOG_ENTRY(FM_LOG_CAT_TE, "sw = %d\n", sw);

    FM_CLEAR(teCfg);

    for (i = 0; i < FM10000_TE_CFG_ENTRIES; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CacheTeCfg,
                                  1,
                                  i,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teCfg,
                                  FALSE);

        err = fmRegCacheRead(sw, 1, &sgList, TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_ARRAY_SET_BIT(teCfg, FM10000_TE_CFG, SwitchLoopbackDisable, 1);

        err = fmRegCacheWrite(sw, 1, &sgList, TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_TE, FM_OK);

}   /* end fm10000TeInit */




/*****************************************************************************/
/** fm10000SyncTeDataLookup
 * \ingroup intlowlevTe10k
 *
 * \desc            Sync TeData with TeLookup by making sure the tunneling
 *                  engine pipeline is empty or processed at least one frame.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SyncTeDataLookup(fm_int sw,
                                  fm_int te)
{
    fm_uint64  frameIn;
    fm_uint64  frameDone;
    fm_uint    retry;
    fm_switch *switchPtr;
    fm_status  err = FM_OK;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Cache API is not used for those registers */
    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_FRAMES_IN(te, 0),
                                &frameIn);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    for (retry = 0 ; retry < FM10000_TE_MAX_SYNC_RETRY ; retry++)
    {
        err = switchPtr->ReadUINT64(sw,
                                    FM10000_TE_FRAMES_DONE(te, 0),
                                    &frameDone);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        if (((frameDone - frameIn) & 0x80000000) == 0)
        {
            syncRetry[te] += retry;
            syncCall[te]++;
            if (retry > maxSyncRetry[te])
            {
                maxSyncRetry[te] = retry;
            }
            goto ABORT;
        }
    }

    /* Not able to sync after FM10000_TE_MAX_SYNC_RETRY retry */
    err = FM_FAIL;

ABORT:

    return err;

}   /* end fm10000SyncTeDataLookup */




/*****************************************************************************/
/** fm10000SetTeDGlort
 * \ingroup intlowlevTe10k
 *
 * \desc            Configure a destination GLORT tunneling entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the glort index to configure.
 *
 * \param[in]       teDGlort points to a structure of type
 *                  ''fm_fm10000TeDGlort'' containing the destination
 *                  glort configuration to set.
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
fm_status fm10000SetTeDGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeDGlort *teDGlort,
                             fm_bool             useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_uint16              tmpValue;
    fm_byte                bit;
    fm_status              err = FM_OK;
    fm_uint32              teDglortMap[FM10000_TE_DGLORT_MAP_WIDTH] = {0};
    fm_uint32              teDglortDec[FM10000_TE_DGLORT_DEC_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "teDGlort = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  index,
                  (void*) teDGlort,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DGLORT_MAP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_DGLORT_MAP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort->baseLookup < FM10000_TE_LOOKUP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort->lookupType < FM_FM10000_TE_LOOKUP_MAX, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort->encap <= 1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort->setSGlort <= 1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort->setDGlort <= 1, FM_ERR_INVALID_ARGUMENT);

    FM_ARRAY_SET_FIELD(teDglortMap,
                       FM10000_TE_DGLORT_MAP,
                       DGLORT_Value,
                       teDGlort->glortValue);

    FM_ARRAY_SET_FIELD(teDglortMap,
                       FM10000_TE_DGLORT_MAP,
                       DGLORT_Mask,
                       teDGlort->glortMask);

    FM_ARRAY_SET_FIELD(teDglortMap,
                       FM10000_TE_DGLORT_MAP,
                       UserValue,
                       teDGlort->userValue);

    FM_ARRAY_SET_FIELD(teDglortMap,
                       FM10000_TE_DGLORT_MAP,
                       UserMask,
                       teDGlort->userMask);

    /* Entry disabled, update DGLORT_MAP prior to DGLORT_DEC */
    if ( (teDGlort->glortMask == 0) && (teDGlort->glortValue != 0) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                                  &fm10000CacheTeDglortMap,
                                  1,
                                  index,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDglortMap,
                                  FALSE);

        err = fmRegCacheWrite(sw, 1, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Make sure no frames are currently in process that would
         * potentially uses this entry of DGLORT_MAP */
        err = fm10000SyncTeDataLookup(sw, te);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    /* Enabling entry, update DGLORT_DEC first to do it atomically */
    else
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                                  &fm10000CacheTeDglortMap,
                                  1,
                                  index,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDglortMap,
                                  FALSE);
    }

    FM_ARRAY_SET_FIELD(teDglortDec,
                       FM10000_TE_DGLORT_DEC,
                       Base,
                       teDGlort->baseLookup);

    FM_ARRAY_SET_BIT(teDglortDec,
                     FM10000_TE_DGLORT_DEC,
                     Encap,
                     teDGlort->encap);

    FM_ARRAY_SET_BIT(teDglortDec,
                     FM10000_TE_DGLORT_DEC,
                     SetSGLORT,
                     teDGlort->setSGlort);

    FM_ARRAY_SET_BIT(teDglortDec,
                     FM10000_TE_DGLORT_DEC,
                     SetDGLORT,
                     teDGlort->setDGlort);

    if (teDGlort->lookupType == FM_FM10000_TE_LOOKUP_DIRECT)
    {
        FM_API_REQUIRE(teDGlort->lookupData.directLookup.indexStart <=
                           FM_FIELD_UNSIGNED_MAX(FM10000_TE_DGLORT_DEC, IndexStart),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teDglortDec,
                           FM10000_TE_DGLORT_DEC,
                           IndexStart,
                           teDGlort->lookupData.directLookup.indexStart);

        FM_API_REQUIRE(teDGlort->lookupData.directLookup.indexWidth <=
                           FM_FIELD_UNSIGNED_MAX(FM10000_TE_DGLORT_DEC, IndexLength),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teDglortDec,
                           FM10000_TE_DGLORT_DEC,
                           IndexLength,
                           teDGlort->lookupData.directLookup.indexWidth);
    }
    /* FM_FM10000_TE_LOOKUP_HASH */
    else
    {
        FM_ARRAY_SET_BIT(teDglortDec,
                         FM10000_TE_DGLORT_DEC,
                         Hash,
                         TRUE);

        FM_ARRAY_SET_FIELD(teDglortDec,
                           FM10000_TE_DGLORT_DEC,
                           HashKeyConfig,
                           teDGlort->lookupData.hashLookup.hashKeyConfig);

        FM_API_REQUIRE(teDGlort->lookupData.hashLookup.tepStart <=
                           FM_FIELD_UNSIGNED_MAX(FM10000_TE_DGLORT_DEC, IndexStart),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teDglortDec,
                           FM10000_TE_DGLORT_DEC,
                           IndexStart,
                           teDGlort->lookupData.hashLookup.tepStart);

        FM_API_REQUIRE(teDGlort->lookupData.hashLookup.tepWidth <=
                           FM_FIELD_UNSIGNED_MAX(FM10000_TE_DGLORT_DEC, IndexLength),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teDglortDec,
                           FM10000_TE_DGLORT_DEC,
                           IndexLength,
                           teDGlort->lookupData.hashLookup.tepWidth);

        FM_API_REQUIRE((teDGlort->baseLookup +
                        teDGlort->lookupData.hashLookup.hashSize) <= FM10000_TE_LOOKUP_ENTRIES_0,
                       FM_ERR_INVALID_ARGUMENT);
        /* Translate Hash Size */
        tmpValue = teDGlort->lookupData.hashLookup.hashSize;
        for (bit = 0 ; bit < 16 ; bit++)
        {
            if (tmpValue & 0x1)
            {
                /* Clear the only bit */
                tmpValue >>= 1;
                break;
            }
            else
            {
                tmpValue >>= 1;
            }
        }

        /* Validates Boundary and Incorrect value */
        if ( (bit == 0) ||
             (bit == 16) ||
             (tmpValue != 0) )
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }
        else
        {
            /* Value 0 refer to a size of 2 and so on... */
            bit--;
            FM_API_REQUIRE(bit <= FM_FIELD_UNSIGNED_MAX(FM10000_TE_DGLORT_DEC,
                                                        HashSize),
                           FM_ERR_INVALID_ARGUMENT);
            FM_ARRAY_SET_FIELD(teDglortDec,
                               FM10000_TE_DGLORT_DEC,
                               HashSize,
                               bit);
        }
    }

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeDglortDec,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDglortDec,
                              FALSE);

    if ( (teDGlort->glortMask == 0) && (teDGlort->glortValue != 0) )
    {
        /* write only DGLORT_DEC */
        err = fmRegCacheWrite(sw, 1, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }
    else
    {
        /* write both DGLORT_DEC and DGLORT_MAP */
        err = fmRegCacheWrite(sw, 2, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeDGlort */




/*****************************************************************************/
/** fm10000GetTeDGlort
 * \ingroup intlowlevTe10k
 *
 * \desc            Retrieve a destination GLORT tunneling entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the glort index to retrieve.
 * 
 * \param[out]      teDGlort is a user-supplied data structure of type
 *                  ''fm_fm10000TeDGlort'' used to retrieve the destination
 *                  glort configuration.
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
fm_status fm10000GetTeDGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeDGlort *teDGlort,
                             fm_bool             useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_byte                bit;
    fm_status              err = FM_OK;
    fm_uint32              teDglortMap[FM10000_TE_DGLORT_MAP_WIDTH] = {0};
    fm_uint32              teDglortDec[FM10000_TE_DGLORT_DEC_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "teDGlort = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  index,
                  (void*) teDGlort,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DGLORT_MAP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_DGLORT_MAP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDGlort != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeDglortMap,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDglortMap,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeDglortDec,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDglortDec,
                              FALSE);

    err = fmRegCacheRead(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    teDGlort->glortValue = FM_ARRAY_GET_FIELD(teDglortMap,
                                              FM10000_TE_DGLORT_MAP,
                                              DGLORT_Value);

    teDGlort->glortMask = FM_ARRAY_GET_FIELD(teDglortMap,
                                             FM10000_TE_DGLORT_MAP,
                                             DGLORT_Mask);

    teDGlort->userValue = FM_ARRAY_GET_FIELD(teDglortMap,
                                             FM10000_TE_DGLORT_MAP,
                                             UserValue);

    teDGlort->userMask = FM_ARRAY_GET_FIELD(teDglortMap,
                                            FM10000_TE_DGLORT_MAP,
                                            UserMask);

    teDGlort->baseLookup = FM_ARRAY_GET_FIELD(teDglortDec,
                                              FM10000_TE_DGLORT_DEC,
                                              Base);

    teDGlort->encap = FM_ARRAY_GET_BIT(teDglortDec,
                                       FM10000_TE_DGLORT_DEC,
                                       Encap);

    teDGlort->setSGlort = FM_ARRAY_GET_BIT(teDglortDec,
                                           FM10000_TE_DGLORT_DEC,
                                           SetSGLORT);

    teDGlort->setDGlort = FM_ARRAY_GET_BIT(teDglortDec,
                                           FM10000_TE_DGLORT_DEC,
                                           SetDGLORT);

    /* FM_FM10000_TE_LOOKUP_HASH */
    if (FM_ARRAY_GET_BIT(teDglortDec, FM10000_TE_DGLORT_DEC, Hash))
    {
        teDGlort->lookupType = FM_FM10000_TE_LOOKUP_HASH;

        teDGlort->lookupData.hashLookup.hashKeyConfig = FM_ARRAY_GET_FIELD(teDglortDec,
                                                                           FM10000_TE_DGLORT_DEC,
                                                                           HashKeyConfig);

        teDGlort->lookupData.hashLookup.tepStart = FM_ARRAY_GET_FIELD(teDglortDec,
                                                                      FM10000_TE_DGLORT_DEC,
                                                                      IndexStart);

        teDGlort->lookupData.hashLookup.tepWidth = FM_ARRAY_GET_FIELD(teDglortDec,
                                                                      FM10000_TE_DGLORT_DEC,
                                                                      IndexLength);

        bit = FM_ARRAY_GET_FIELD(teDglortDec,
                                 FM10000_TE_DGLORT_DEC,
                                 HashSize);

        teDGlort->lookupData.hashLookup.hashSize = 1 << (bit + 1);
    }
    /* FM_FM10000_TE_LOOKUP_DIRECT */
    else
    {
        teDGlort->lookupType = FM_FM10000_TE_LOOKUP_DIRECT;

        teDGlort->lookupData.directLookup.indexStart = FM_ARRAY_GET_FIELD(teDglortDec,
                                                                          FM10000_TE_DGLORT_DEC,
                                                                          IndexStart);

        teDGlort->lookupData.directLookup.indexWidth = FM_ARRAY_GET_FIELD(teDglortDec,
                                                                          FM10000_TE_DGLORT_DEC,
                                                                          IndexLength);
    }


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeDGlort */




/*****************************************************************************/
/** fm10000SetTeSGlort
 * \ingroup lowlevTe10k
 *
 * \desc            Configure a source GLORT tunneling entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the glort index to configure.
 *
 * \param[in]       teSGlort points to a structure of type
 *                  ''fm_fm10000TeSGlort'' containing the source
 *                  glort configuration to set.
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
fm_status fm10000SetTeSGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeSGlort *teSGlort,
                             fm_bool             useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_status              err = FM_OK;
    fm_uint32              teSglortMap[FM10000_TE_SGLORT_MAP_WIDTH] = {0};
    fm_uint32              teSglortDec[FM10000_TE_SGLORT_DEC_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "teSGlort = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  index,
                  (void*) teSGlort,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_SGLORT_MAP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_SGLORT_MAP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teSGlort != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teSGlort->vsiStart <= FM_FIELD_UNSIGNED_MAX(FM10000_TE_SGLORT_DEC,
                                                               VSI_Start),
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teSGlort->vsiLength <= FM_FIELD_UNSIGNED_MAX(FM10000_TE_SGLORT_DEC,
                                                                VSI_Length),
                   FM_ERR_INVALID_ARGUMENT);

    FM_ARRAY_SET_FIELD(teSglortMap,
                       FM10000_TE_SGLORT_MAP,
                       SGLORT_Value,
                       teSGlort->glortValue);

    FM_ARRAY_SET_FIELD(teSglortMap,
                       FM10000_TE_SGLORT_MAP,
                       SGLORT_Mask,
                       teSGlort->glortMask);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeSglortMap,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSglortMap,
                              FALSE);

    FM_ARRAY_SET_FIELD(teSglortDec,
                       FM10000_TE_SGLORT_DEC,
                       VSI_Start,
                       teSGlort->vsiStart);

    FM_ARRAY_SET_FIELD(teSglortDec,
                       FM10000_TE_SGLORT_DEC,
                       VSI_Length,
                       teSGlort->vsiLength);

    FM_ARRAY_SET_FIELD(teSglortDec,
                       FM10000_TE_SGLORT_DEC,
                       VSI_Offset,
                       teSGlort->vsiOffset);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeSglortDec,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSglortDec,
                              FALSE);

    /* write to the register */
    err = fmRegCacheWrite(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeSGlort */




/*****************************************************************************/
/** fm10000GetTeSGlort
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve a source GLORT tunneling entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the glort index to retrieve.
 * 
 * \param[out]      teSGlort is a user-supplied data structure of type
 *                  ''fm_fm10000TeSGlort'' used to retrieve the source
 *                  glort configuration.
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
fm_status fm10000GetTeSGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeSGlort *teSGlort,
                             fm_bool             useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_status              err = FM_OK;
    fm_uint32              teSglortMap[FM10000_TE_SGLORT_MAP_WIDTH] = {0};
    fm_uint32              teSglortDec[FM10000_TE_SGLORT_DEC_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "teSGlort = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  index,
                  (void*) teSGlort,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_SGLORT_MAP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_SGLORT_MAP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teSGlort != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeSglortMap,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSglortMap,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeSglortDec,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSglortDec,
                              FALSE);

    err = fmRegCacheRead(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    teSGlort->glortValue = FM_ARRAY_GET_FIELD(teSglortMap,
                                              FM10000_TE_SGLORT_MAP,
                                              SGLORT_Value);

    teSGlort->glortMask = FM_ARRAY_GET_FIELD(teSglortMap,
                                             FM10000_TE_SGLORT_MAP,
                                             SGLORT_Mask);

    teSGlort->vsiStart = FM_ARRAY_GET_FIELD(teSglortDec,
                                            FM10000_TE_SGLORT_DEC,
                                            VSI_Start);

    teSGlort->vsiLength = FM_ARRAY_GET_FIELD(teSglortDec,
                                             FM10000_TE_SGLORT_DEC,
                                             VSI_Length);

    teSGlort->vsiOffset = FM_ARRAY_GET_FIELD(teSglortDec,
                                             FM10000_TE_SGLORT_DEC,
                                             VSI_Offset);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeSGlort */




/*****************************************************************************/
/** fm10000SetTeDefaultGlort
 * \ingroup lowlevTe10k
 *
 * \desc            Configure the default value selected for all GLORT field
 *                  when applied to tunneling engine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       teGlortCfg points to a structure of type
 *                  ''fm_fm10000TeGlortCfg'' containing the default
 *                  glort configuration to set.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teGlortCfg should be written to the tunneling engine. See
 *                  ''fm_fm10000TeDefGlortSel'' for bit definitions.
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
fm_status fm10000SetTeDefaultGlort(fm_int                sw,
                                   fm_int                te,
                                   fm_fm10000TeGlortCfg *teGlortCfg,
                                   fm_uint32             fieldSelectMask,
                                   fm_bool               useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_status              err = FM_OK;
    fm_uint32              teDefaultDglort[FM10000_TE_DEFAULT_DGLORT_WIDTH] = {0};
    fm_uint32              teDefaultSglort[FM10000_TE_DEFAULT_SGLORT_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teGlortCfg = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teGlortCfg,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DEFAULT_DGLORT_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teGlortCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeDefaultDglort,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultDglort,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeDefaultSglort,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultSglort,
                              FALSE);

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM10000_TE_DEFAULT_GLORT_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        err = fmRegCacheRead(sw, 2, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GLORT_ENCAP_DGLORT)
    {
        FM_ARRAY_SET_FIELD(teDefaultDglort,
                           FM10000_TE_DEFAULT_DGLORT,
                           DefaultEncapDGLORT,
                           teGlortCfg->encapDglort);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GLORT_DECAP_DGLORT)
    {
        FM_ARRAY_SET_FIELD(teDefaultDglort,
                           FM10000_TE_DEFAULT_DGLORT,
                           DefaultDecapDGLORT,
                           teGlortCfg->decapDglort);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GLORT_ENCAP_SGLORT)
    {
        FM_ARRAY_SET_FIELD(teDefaultSglort,
                           FM10000_TE_DEFAULT_SGLORT,
                           DefaultEncapSGLORT,
                           teGlortCfg->encapSglort);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GLORT_DECAP_SGLORT)
    {
        FM_ARRAY_SET_FIELD(teDefaultSglort,
                           FM10000_TE_DEFAULT_SGLORT,
                           DefaultDecapSGLORT,
                           teGlortCfg->decapSglort);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeDefaultGlort */




/*****************************************************************************/
/** fm10000GetTeDefaultGlort
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the default value selected for all GLORT field
 *                  applied to tunneling engine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[out]      teGlortCfg is a user-supplied data structure of type
 *                  ''fm_fm10000TeGlortCfg'' used to retrieve the default
 *                  glort configuration.
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
fm_status fm10000GetTeDefaultGlort(fm_int                sw,
                                   fm_int                te,
                                   fm_fm10000TeGlortCfg *teGlortCfg,
                                   fm_bool               useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_status              err = FM_OK;
    fm_uint32              teDefaultDglort[FM10000_TE_DEFAULT_DGLORT_WIDTH] = {0};
    fm_uint32              teDefaultSglort[FM10000_TE_DEFAULT_SGLORT_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teGlortCfg = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teGlortCfg,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DEFAULT_DGLORT_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teGlortCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeDefaultDglort,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultDglort,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeDefaultSglort,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultSglort,
                              FALSE);

    err = fmRegCacheRead(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    teGlortCfg->encapDglort = FM_ARRAY_GET_FIELD(teDefaultDglort,
                                                 FM10000_TE_DEFAULT_DGLORT,
                                                 DefaultEncapDGLORT);

    teGlortCfg->decapDglort = FM_ARRAY_GET_FIELD(teDefaultDglort,
                                                 FM10000_TE_DEFAULT_DGLORT,
                                                 DefaultDecapDGLORT);

    teGlortCfg->encapSglort = FM_ARRAY_GET_FIELD(teDefaultSglort,
                                                 FM10000_TE_DEFAULT_SGLORT,
                                                 DefaultEncapSGLORT);

    teGlortCfg->decapSglort = FM_ARRAY_GET_FIELD(teDefaultSglort,
                                                 FM10000_TE_DEFAULT_SGLORT,
                                                 DefaultDecapSGLORT);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeDefaultGlort */




/*****************************************************************************/
/** fm10000SetTeDefaultTep
 * \ingroup lowlevTe10k
 *
 * \desc            Configure the default value selected for all tep field
 *                  when applied to tunneling engine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       tep is the tunneling end point index to set.
 * 
 * \param[in]       teTepCfg points to a structure of type
 *                  ''fm_fm10000TeTepCfg'' containing the default
 *                  tep configuration to set.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teTepCfg should be written to the tunneling engine. See
 *                  ''fm_fm10000TeDefTepSel'' for bit definitions.
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
fm_status fm10000SetTeDefaultTep(fm_int              sw,
                                 fm_int              te,
                                 fm_int              tep,
                                 fm_fm10000TeTepCfg *teTepCfg,
                                 fm_uint32           fieldSelectMask,
                                 fm_bool             useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_int                 i;
    fm_status              err = FM_OK;
    fm_uint32              teSip[FM10000_TE_SIP_WIDTH] = {0};
    fm_uint32              teVni[FM10000_TE_VNI_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "tep = %d, "
                  "teTepCfg = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  tep,
                  (void*) teTepCfg,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_SIP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(tep < FM10000_TE_SIP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teTepCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeSip,
                              1,
                              tep,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSip,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeVni,
                              1,
                              tep,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teVni,
                              FALSE);

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM10000_TE_DEFAULT_TEP_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        err = fmRegCacheRead(sw, 2, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TEP_SIP)
    {
        for (i = 0 ; i < (teTepCfg->srcIpAddr.isIPv6 ? 4 : 1) ; i++)
        {
            teSip[i] = ntohl(teTepCfg->srcIpAddr.addr[i]);
        }
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TEP_VNI)
    {
        FM_API_REQUIRE(teTepCfg->vni <= FM_FIELD_UNSIGNED_MAX(FM10000_TE_VNI, VNI),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teVni,
                           FM10000_TE_VNI,
                           VNI,
                           teTepCfg->vni);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeDefaultTep */




/*****************************************************************************/
/** fm10000GetTeDefaultTep
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the default value selected for all tep field
 *                  applied to tunneling engine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       tep is the tunneling end point index to retrieve.
 * 
 * \param[out]      teTepCfg is a user-supplied data structure of type
 *                  ''fm_fm10000TeTepCfg'' used to retrieve the default
 *                  tep configuration.
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
fm_status fm10000GetTeDefaultTep(fm_int              sw,
                                 fm_int              te,
                                 fm_int              tep,
                                 fm_fm10000TeTepCfg *teTepCfg,
                                 fm_bool             useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_int                 i;
    fm_status              err = FM_OK;
    fm_uint32              teSip[FM10000_TE_SIP_WIDTH] = {0};
    fm_uint32              teVni[FM10000_TE_VNI_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "tep = %d, "
                  "teTepCfg = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  tep,
                  (void*) teTepCfg,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_SIP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(tep < FM10000_TE_SIP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teTepCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeSip,
                              1,
                              tep,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSip,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeVni,
                              1,
                              tep,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teVni,
                              FALSE);

    err = fmRegCacheRead(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    /**************************************************
     * Assume it's an IPv4 address if words 1..3 of 
     * the value are zeros 
     **************************************************/
    teTepCfg->srcIpAddr.isIPv6 = FALSE;
    teTepCfg->srcIpAddr.addr[0] = htonl(teSip[0]);

    for (i = 1 ; i < 4 ; i++)
    {
        teTepCfg->srcIpAddr.addr[i] = htonl(teSip[i]);

        if (teTepCfg->srcIpAddr.addr[i] != 0)
        {
            teTepCfg->srcIpAddr.isIPv6 = TRUE;
        }
    }

    teTepCfg->vni = FM_ARRAY_GET_FIELD(teVni,
                                       FM10000_TE_VNI,
                                       VNI);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeDefaultTep */




/*****************************************************************************/
/** fm10000SetTeDefaultTunnel
 * \ingroup lowlevTe10k
 *
 * \desc            Configure the default value selected for all tunnel
 *                  specific field when applied to tunneling engine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       teTunnelCfg points to a structure of type
 *                  ''fm_fm10000TeTunnelCfg'' containing the default
 *                  tunnel configuration to set.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teTunnelCfg should be written to the tunneling engine. See
 *                  ''fm_fm10000TeDefTunnelSel'' for bit definitions.
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
fm_status fm10000SetTeDefaultTunnel(fm_int                 sw,
                                    fm_int                 te,
                                    fm_fm10000TeTunnelCfg *teTunnelCfg,
                                    fm_uint32              fieldSelectMask,
                                    fm_bool                useCache)
{
    fm_registerSGListEntry sgList[7];
    fm_int                 i;
    fm_int                 sgIndex = 0;
    fm_status              err = FM_OK;
    fm_uint32              teDefaultL4Dst[FM10000_TE_DEFAULT_L4DST_WIDTH] = {0};
    fm_uint32              teCfg[FM10000_TE_CFG_WIDTH] = {0};
    fm_uint32              teTunHeaderCfg[FM10000_TE_TUN_HEADER_CFG_WIDTH] = {0};
    fm_uint32              teDefaultNgeData[FM10000_TE_DEFAULT_NGE_DATA_WIDTH *
                                            FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0] = {0};
    fm_uint32              teDefaultNgeMask[FM10000_TE_DEFAULT_NGE_MASK_WIDTH] = {0};
    fm_uint32              teDmac[FM10000_TE_DMAC_WIDTH] = {0};
    fm_uint32              teSmac[FM10000_TE_SMAC_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;
    fm_uint32              chipVersion;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teTunnelCfg = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teTunnelCfg,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DEFAULT_L4DST_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teTunnelCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    /* NGE and GPE/NSH are mutually exclusive */
    if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_ALL) &&
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_GPE_NSH_ALL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* Only add the required register to the scatter-gather list */
    if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_L4DST_VXLAN) ||
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeDefaultL4Dst,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDefaultL4Dst,
                                  FALSE);
        sgIndex++;
    }

    if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_TTL) ||
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_TOS) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeCfg,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teCfg,
                                  FALSE);
        sgIndex++;
    }

    if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_PROTOCOL) ||
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_VERSION) || 
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_MODE ) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeTunHeaderCfg,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teTunHeaderCfg,
                                  FALSE);
        sgIndex++;
    }

    if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_DATA) ||
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_GPE_NSH_ALL) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeDefaultNgeData,
                                  FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0,
                                  0,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDefaultNgeData,
                                  FALSE);
        sgIndex++;
    }

    if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_MASK) ||
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_TIME) ||
         (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_GPE_NSH_ALL) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeDefaultNgeMask,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDefaultNgeMask,
                                  FALSE);
        sgIndex++;
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_DMAC)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeDmac,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDmac,
                                  FALSE);
        sgIndex++;
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_SMAC)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeSmac,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teSmac,
                                  FALSE);
        sgIndex++;
    }

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_L4DST_VXLAN)
    {
        FM_ARRAY_SET_FIELD(teDefaultL4Dst,
                           FM10000_TE_DEFAULT_L4DST,
                           VXLAN,
                           teTunnelCfg->l4DstVxLan);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE)
    {
        FM_ARRAY_SET_FIELD(teDefaultL4Dst,
                           FM10000_TE_DEFAULT_L4DST,
                           NGE,
                           teTunnelCfg->l4DstNge);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_TTL)
    {
        FM_ARRAY_SET_FIELD(teCfg,
                           FM10000_TE_CFG,
                           OuterTTL,
                           teTunnelCfg->ttl);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_TOS)
    {
        FM_ARRAY_SET_FIELD(teCfg,
                           FM10000_TE_CFG,
                           OuterTOS,
                           teTunnelCfg->tos);

        FM_API_REQUIRE(teTunnelCfg->deriveOuterTOS <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teCfg,
                         FM10000_TE_CFG,
                         DeriveOuterTOS,
                         teTunnelCfg->deriveOuterTOS);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_PROTOCOL)
    {
        FM_ARRAY_SET_FIELD(teTunHeaderCfg,
                           FM10000_TE_TUN_HEADER_CFG,
                           EncapProtocol,
                           teTunnelCfg->encapProtocol);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_VERSION)
    {
        /* Even if encapVersion is 3 bits, limit to a value of 3 because
         * encapVersion[2] is used to enable GPE/NSH in B0. */
        FM_API_REQUIRE(teTunnelCfg->encapVersion <= FM10000_TE_ENCAP_VERSION_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTunHeaderCfg,
                           FM10000_TE_TUN_HEADER_CFG,
                           EncapVersion,
                           teTunnelCfg->encapVersion);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_MODE)
    {
        err = fmReadUINT32(sw, FM10000_CHIP_VERSION(), &chipVersion);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        if (chipVersion == FM10000_CHIP_VERSION_A0)
        {
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        if ( (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_PROTOCOL) == 0)
        {
            FM_LOG_WARNING(FM_LOG_CAT_TE, 
                           "Tunnel Protocol should be specified along with "
                           "Tunnel Mode.");
        }

        /* Tunnel Mode users encapVersion[2] to enable GPE/NSH in B0. */
        FM_API_REQUIRE(teTunnelCfg->mode <= FM10000_TE_MODE_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_UNNAMED_BIT(teTunHeaderCfg, 
                                 FM10000_TE_TUN_HEADER_CFG_h_EncapVersion,
                                 teTunnelCfg->mode);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GPE_NSH_CLEAR)
    {
        FM_ARRAY_SET_FIELD(teDefaultNgeMask,
                           FM10000_TE_DEFAULT_NGE_MASK,
                           Mask,
                           0);
        FM_ARRAY_SET_BIT(teDefaultNgeMask,
                         FM10000_TE_DEFAULT_NGE_MASK,
                         LoadTimetag,
                         0);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GPE_NEXT_PROT)
    {
        teDefaultNgeMask[0] |= FM10000_NGE_MASK_GPE_FLAGS_NEXT_PROT;
        teDefaultNgeData[FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT * 
                         FM10000_TE_DEFAULT_NGE_DATA_WIDTH] = 
            (0x0C000000 | (teTunnelCfg->gpeNextProt & 0x0F));
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_GPE_VNI)
    {
        teDefaultNgeMask[0] |= FM10000_NGE_MASK_GPE_VNI;
        teDefaultNgeData[FM10000_NGE_POS_GPE_VNI * 
                         FM10000_TE_DEFAULT_NGE_DATA_WIDTH] = 
            ( (teTunnelCfg->gpeVni & 0xFFFFFF) << 8);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_NSH_BASE_HDR)
    {
        if (teTunnelCfg->nshLength > (FM10000_TE_NGE_DATA_SIZE - 
                                      FM10000_TE_GPE_HDR_SIZE) )
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
        }

        teDefaultNgeMask[0] |= FM10000_NGE_MASK_NSH_BASE_HDR;

        /* NSH: Flags=0x0, NextProt=3 */
        teDefaultNgeData[FM10000_NGE_POS_NSH_BASE_HDR * 
                         FM10000_TE_DEFAULT_NGE_DATA_WIDTH] = 
            ( (teTunnelCfg->nshCritical & 1) << 28 )  |
            ( (teTunnelCfg->nshLength & 0x3F) << 16 ) |
            ( (teTunnelCfg->nshMdType & 0xFF) << 8 ) |
            0x3;
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_NSH_SERVICE_HDR)
    {
        teDefaultNgeMask[0] |= FM10000_NGE_MASK_NSH_SERVICE_HDR;
        teDefaultNgeData[FM10000_NGE_POS_NSH_SERVICE_HDR * 
                         FM10000_TE_DEFAULT_NGE_DATA_WIDTH] = 
            ( ( (teTunnelCfg->nshSvcPathId & 0xFFFFFF) << 8) |
              (teTunnelCfg->nshSvcIndex & 0xFF) ) ;
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_NSH_DATA)
    {
        teDefaultNgeMask[0] |= (teTunnelCfg->nshDataMask & 
                                FM10000_TE_NSH_DATA_MASK) << 
                                FM10000_NGE_POS_NSH_DATA;

        for (i = FM10000_NGE_POS_NSH_DATA ; 
             i < FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0 ; 
             i++)
        {
            FM_ARRAY_SET_FIELD(&teDefaultNgeData[i * FM10000_TE_DEFAULT_NGE_DATA_WIDTH],
                               FM10000_TE_DEFAULT_NGE_DATA,
                               Data,
                               teTunnelCfg->nshData[i-FM10000_NGE_POS_NSH_DATA]);
        }
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_DATA)
    {
        for (i = 0 ; i < FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0 ; i++)
        {
            FM_ARRAY_SET_FIELD(&teDefaultNgeData[i * FM10000_TE_DEFAULT_NGE_DATA_WIDTH],
                               FM10000_TE_DEFAULT_NGE_DATA,
                               Data,
                               teTunnelCfg->ngeData[i]);
        }
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_MASK)
    {
        FM_ARRAY_SET_FIELD(teDefaultNgeMask,
                           FM10000_TE_DEFAULT_NGE_MASK,
                           Mask,
                           teTunnelCfg->ngeMask);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_NGE_TIME)
    {
        FM_API_REQUIRE(teTunnelCfg->ngeTime <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teDefaultNgeMask,
                         FM10000_TE_DEFAULT_NGE_MASK,
                         LoadTimetag,
                         teTunnelCfg->ngeTime);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_DMAC)
    {
        FM_API_REQUIRE(teTunnelCfg->dmac <= FM_FIELD_UNSIGNED_MAX64(FM10000_TE_DMAC, DMAC),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD64(teDmac,
                             FM10000_TE_DMAC,
                             DMAC,
                             teTunnelCfg->dmac);
    }

    if (fieldSelectMask & FM10000_TE_DEFAULT_TUNNEL_SMAC)
    {
        FM_API_REQUIRE(teTunnelCfg->smac <= FM_FIELD_UNSIGNED_MAX64(FM10000_TE_SMAC, SMAC),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD64(teSmac,
                             FM10000_TE_SMAC,
                             SMAC,
                             teTunnelCfg->smac);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeDefaultTunnel */




/*****************************************************************************/
/** fm10000GetTeDefaultTunnel
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the default value selected for all tunnel
 *                  specific field applied to tunneling engine.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[out]      teTunnelCfg is a user-supplied data structure of type
 *                  ''fm_fm10000TeTunnelCfg'' used to retrieve the default
 *                  tunnel configuration.
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
fm_status fm10000GetTeDefaultTunnel(fm_int                 sw,
                                    fm_int                 te,
                                    fm_fm10000TeTunnelCfg *teTunnelCfg,
                                    fm_bool                useCache)
{
    fm_registerSGListEntry sgList[7];
    fm_int                 i;
    fm_status              err = FM_OK;
    fm_uint32              teDefaultL4Dst[FM10000_TE_DEFAULT_L4DST_WIDTH] = {0};
    fm_uint32              teCfg[FM10000_TE_CFG_WIDTH] = {0};
    fm_uint32              teTunHeaderCfg[FM10000_TE_TUN_HEADER_CFG_WIDTH] = {0};
    fm_uint32              teDefaultNgeData[FM10000_TE_DEFAULT_NGE_DATA_WIDTH *
                                            FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0] = {0};
    fm_uint32              teDefaultNgeMask[FM10000_TE_DEFAULT_NGE_MASK_WIDTH] = {0};
    fm_uint32              teDmac[FM10000_TE_DMAC_WIDTH] = {0};
    fm_uint32              teSmac[FM10000_TE_SMAC_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teTunnelCfg = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teTunnelCfg,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DEFAULT_L4DST_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teTunnelCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeDefaultL4Dst,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultL4Dst,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeCfg,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teCfg,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[2],
                              &fm10000CacheTeTunHeaderCfg,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teTunHeaderCfg,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[3],
                              &fm10000CacheTeDefaultNgeData,
                              FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0,
                              0,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultNgeData,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[4],
                              &fm10000CacheTeDefaultNgeMask,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDefaultNgeMask,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[5],
                              &fm10000CacheTeDmac,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDmac,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[6],
                              &fm10000CacheTeSmac,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teSmac,
                              FALSE);

    err = fmRegCacheRead(sw, 7, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    teTunnelCfg->l4DstVxLan = FM_ARRAY_GET_FIELD(teDefaultL4Dst,
                                                 FM10000_TE_DEFAULT_L4DST,
                                                 VXLAN);

    teTunnelCfg->l4DstNge = FM_ARRAY_GET_FIELD(teDefaultL4Dst,
                                               FM10000_TE_DEFAULT_L4DST,
                                               NGE);

    teTunnelCfg->ttl = FM_ARRAY_GET_FIELD(teCfg,
                                          FM10000_TE_CFG,
                                          OuterTTL);

    teTunnelCfg->tos = FM_ARRAY_GET_FIELD(teCfg,
                                          FM10000_TE_CFG,
                                          OuterTOS);

    teTunnelCfg->deriveOuterTOS = FM_ARRAY_GET_BIT(teCfg,
                                                   FM10000_TE_CFG,
                                                   DeriveOuterTOS);

    teTunnelCfg->encapProtocol = FM_ARRAY_GET_FIELD(teTunHeaderCfg,
                                                    FM10000_TE_TUN_HEADER_CFG,
                                                    EncapProtocol);

    teTunnelCfg->encapVersion = FM_ARRAY_GET_FIELD(teTunHeaderCfg,
                                                   FM10000_TE_TUN_HEADER_CFG,
                                                   EncapVersion);

    for (i = 0 ; i < FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0 ; i++)
    {
        teTunnelCfg->ngeData[i] = FM_ARRAY_GET_FIELD(&teDefaultNgeData[i * FM10000_TE_DEFAULT_NGE_DATA_WIDTH],
                                                     FM10000_TE_DEFAULT_NGE_DATA,
                                                     Data);
    }

    teTunnelCfg->ngeMask = FM_ARRAY_GET_FIELD(teDefaultNgeMask,
                                              FM10000_TE_DEFAULT_NGE_MASK,
                                              Mask);

    teTunnelCfg->ngeTime = FM_ARRAY_GET_BIT(teDefaultNgeMask,
                                            FM10000_TE_DEFAULT_NGE_MASK,
                                            LoadTimetag);

    teTunnelCfg->dmac = FM_ARRAY_GET_FIELD64(teDmac,
                                             FM10000_TE_DMAC,
                                             DMAC);

    teTunnelCfg->smac = FM_ARRAY_GET_FIELD64(teSmac,
                                             FM10000_TE_SMAC,
                                             SMAC);

    teTunnelCfg->mode = FM_ARRAY_GET_UNNAMED_BIT(teTunHeaderCfg, 
                                                 FM10000_TE_TUN_HEADER_CFG_h_EncapVersion);

    teTunnelCfg->gpeNextProt = 0;
    teTunnelCfg->gpeVni = 0;
    teTunnelCfg->nshLength = 0;
    teTunnelCfg->nshCritical = 0;
    teTunnelCfg->nshMdType = 0;
    teTunnelCfg->nshSvcPathId = 0;
    teTunnelCfg->nshSvcIndex = 0;
    teTunnelCfg->nshDataMask = 0;

    for (i = 0; i < FM10000_TE_NSH_DATA_SIZE; i++)
    {
        teTunnelCfg->nshData[i] = 0; 
    }

    if (teTunnelCfg->mode == FM10000_TE_MODE_VXLAN_GPE_NSH)
    {
        if (teTunnelCfg->ngeMask & FM10000_NGE_MASK_GPE_FLAGS_NEXT_PROT)
        {
            teTunnelCfg->gpeNextProt = teTunnelCfg->ngeData[FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT] & 0xFF;
        }
        
        if (teTunnelCfg->ngeMask & FM10000_NGE_MASK_GPE_VNI)
        {
            teTunnelCfg->gpeVni = (teTunnelCfg->ngeData[FM10000_NGE_POS_GPE_VNI] >> 8) & 0xFFFFFF;
        }

        if (teTunnelCfg->ngeMask & FM10000_NGE_MASK_NSH_BASE_HDR)
        {
            teTunnelCfg->nshLength   = (teTunnelCfg->ngeData[FM10000_NGE_POS_NSH_BASE_HDR] >> 16) & 0x3F;
            teTunnelCfg->nshCritical = (teTunnelCfg->ngeData[FM10000_NGE_POS_NSH_BASE_HDR] >> 28) & 0x1;
            teTunnelCfg->nshMdType   = (teTunnelCfg->ngeData[FM10000_NGE_POS_NSH_BASE_HDR] >> 8) & 0xFF;
        }

        if (teTunnelCfg->ngeMask & FM10000_NGE_MASK_NSH_SERVICE_HDR)
        {
            teTunnelCfg->nshSvcPathId = (teTunnelCfg->ngeData[FM10000_NGE_POS_NSH_SERVICE_HDR] >> 8) & 0xFFFFFF;
            teTunnelCfg->nshSvcIndex = teTunnelCfg->ngeData[FM10000_NGE_POS_NSH_SERVICE_HDR] & 0xFF;
        }

        teTunnelCfg->nshDataMask = teTunnelCfg->ngeMask >> FM10000_NGE_POS_NSH_DATA;

        for (i = 0; i < FM10000_TE_NSH_DATA_SIZE; i++)
        {
            teTunnelCfg->nshData[i] = teTunnelCfg->ngeData[i+FM10000_NGE_POS_NSH_DATA];
        }
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeDefaultTunnel */




/*****************************************************************************/
/** fm10000SetTeChecksum
 * \ingroup lowlevTe10k
 *
 * \desc            Configure the tunneling engine checksum handling.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       teChecksumCfg points to a structure of type
 *                  ''fm_fm10000TeChecksumCfg'' containing the checksum
 *                  configuration to set.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teChecksumCfg should be written to the tunneling engine. See
 *                  ''fm_fm10000TeChecksumSel'' for bit definitions.
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
fm_status fm10000SetTeChecksum(fm_int                   sw,
                               fm_int                   te,
                               fm_fm10000TeChecksumCfg *teChecksumCfg,
                               fm_uint32                fieldSelectMask,
                               fm_bool                  useCache)
{
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              teCfg[FM10000_TE_CFG_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teChecksumCfg = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teChecksumCfg,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DEFAULT_L4DST_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teChecksumCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheTeCfg,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teCfg,
                              FALSE);

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    if (fieldSelectMask & FM10000_TE_CHECKSUM_NOT_IP)
    {
        FM_API_REQUIRE(teChecksumCfg->notIp < FM_FM10000_TE_CHECKSUM_HEADER,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teCfg,
                           FM10000_TE_CFG,
                           NotIP,
                           teChecksumCfg->notIp);
    }

    if (fieldSelectMask & FM10000_TE_CHECKSUM_NOT_TCP_OR_UDP)
    {
        FM_API_REQUIRE(teChecksumCfg->notTcpOrUdp < FM_FM10000_TE_CHECKSUM_HEADER,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teCfg,
                           FM10000_TE_CFG,
                           IPnotTCPnotUDP,
                           teChecksumCfg->notTcpOrUdp);
    }

    if (fieldSelectMask & FM10000_TE_CHECKSUM_TCP_OR_UDP)
    {
        FM_API_REQUIRE(teChecksumCfg->tcpOrUdp < FM_FM10000_TE_CHECKSUM_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teCfg,
                           FM10000_TE_CFG,
                           IPisTCPorUDP,
                           teChecksumCfg->tcpOrUdp);
    }

    if (fieldSelectMask & FM10000_TE_CHECKSUM_DECAP_VALID)
    {
        FM_API_REQUIRE(teChecksumCfg->verifDecapChecksum <= 1,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teCfg,
                         FM10000_TE_CFG,
                         VerifyDecapCSUM,
                         teChecksumCfg->verifDecapChecksum);
    }

    if (fieldSelectMask & FM10000_TE_CHECKSUM_DECAP_UPDATE)
    {
        FM_API_REQUIRE(teChecksumCfg->updateDecapChecksum <= 1,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teCfg,
                         FM10000_TE_CFG,
                         UpdateOldHeaderInPlaceCSUM,
                         teChecksumCfg->updateDecapChecksum);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeChecksum */




/*****************************************************************************/
/** fm10000GetTeChecksum
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the tunneling engine checksum configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[out]      teChecksumCfg is a user-supplied data structure of type
 *                  ''fm_fm10000TeChecksumCfg'' used to retrieve the checksum
 *                  configuration.
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
fm_status fm10000GetTeChecksum(fm_int                   sw,
                               fm_int                   te,
                               fm_fm10000TeChecksumCfg *teChecksumCfg,
                               fm_bool                  useCache)
{
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              teCfg[FM10000_TE_CFG_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teChecksumCfg = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teChecksumCfg,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DEFAULT_L4DST_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teChecksumCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheTeCfg,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teCfg,
                              FALSE);

    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    teChecksumCfg->notIp = FM_ARRAY_GET_FIELD(teCfg,
                                              FM10000_TE_CFG,
                                              NotIP);

    teChecksumCfg->notTcpOrUdp = FM_ARRAY_GET_FIELD(teCfg,
                                                    FM10000_TE_CFG,
                                                    IPnotTCPnotUDP);

    teChecksumCfg->tcpOrUdp = FM_ARRAY_GET_FIELD(teCfg,
                                                 FM10000_TE_CFG,
                                                 IPisTCPorUDP);

    teChecksumCfg->verifDecapChecksum = FM_ARRAY_GET_BIT(teCfg,
                                                         FM10000_TE_CFG,
                                                         VerifyDecapCSUM);

    teChecksumCfg->updateDecapChecksum = FM_ARRAY_GET_BIT(teCfg,
                                                          FM10000_TE_CFG,
                                                          UpdateOldHeaderInPlaceCSUM);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeChecksum */




/*****************************************************************************/
/** fm10000SetTeParser
 * \ingroup lowlevTe10k
 *
 * \desc            Configure the tunneling engine parser.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       teParserCfg points to a structure of type
 *                  ''fm_fm10000TeParserCfg'' containing the parser
 *                  configuration to set.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teParserCfg should be written to the tunneling engine. See
 *                  ''fm_fm10000TeParserSel'' for bit definitions.
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
fm_status fm10000SetTeParser(fm_int                 sw,
                             fm_int                 te,
                             fm_fm10000TeParserCfg *teParserCfg,
                             fm_uint32              fieldSelectMask,
                             fm_bool                useCache)
{
    fm_registerSGListEntry sgList[3];
    fm_int                 sgIndex = 0;
    fm_status              err = FM_OK;
    fm_uint32              tePorts[FM10000_TE_PORTS_WIDTH] = {0};
    fm_uint32              teTunHeaderCfg[FM10000_TE_TUN_HEADER_CFG_WIDTH] = {0};
    fm_uint32              teExvet[FM10000_TE_EXVET_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teParserCfg = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teParserCfg,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_PORTS_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teParserCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    /* Only add the required register to the scatter-gather list */
    if ( (fieldSelectMask & FM10000_TE_PARSER_VXLAN_PORT) ||
         (fieldSelectMask & FM10000_TE_PARSER_NGE_PORT) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTePorts,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  tePorts,
                                  FALSE);
        sgIndex++;
    }

    if ( (fieldSelectMask & FM10000_TE_PARSER_CHECK_PROTOCOL) ||
         (fieldSelectMask & FM10000_TE_PARSER_CHECK_VERSION) ||
         (fieldSelectMask & FM10000_TE_PARSER_CHECK_NGE_OAM) ||
         (fieldSelectMask & FM10000_TE_PARSER_CHECK_NGE_C))
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeTunHeaderCfg,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teTunHeaderCfg,
                                  FALSE);
        sgIndex++;
    }

    if ( (fieldSelectMask & FM10000_TE_PARSER_ETH_TYPE) ||
         (fieldSelectMask & FM10000_TE_PARSER_ETH_TYPE_VLAN) ||
         (fieldSelectMask & FM10000_TE_PARSER_TAG_SIZE) )
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeExvet,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teExvet,
                                  FALSE);
        sgIndex++;
    }

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = fmRegCacheRead(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    if (fieldSelectMask & FM10000_TE_PARSER_VXLAN_PORT)
    {
        FM_ARRAY_SET_FIELD(tePorts,
                           FM10000_TE_PORTS,
                           VXLAN,
                           teParserCfg->vxLanPort);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_NGE_PORT)
    {
        FM_ARRAY_SET_FIELD(tePorts,
                           FM10000_TE_PORTS,
                           NGE,
                           teParserCfg->ngePort);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_CHECK_PROTOCOL)
    {
        FM_API_REQUIRE(teParserCfg->checkProtocol <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teTunHeaderCfg,
                         FM10000_TE_TUN_HEADER_CFG,
                         CheckProtocol,
                         teParserCfg->checkProtocol);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_CHECK_VERSION)
    {
        FM_API_REQUIRE(teParserCfg->checkVersion <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teTunHeaderCfg,
                         FM10000_TE_TUN_HEADER_CFG,
                         CheckVersion,
                         teParserCfg->checkVersion);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_CHECK_NGE_OAM)
    {
        FM_API_REQUIRE(teParserCfg->checkNgeOam <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teTunHeaderCfg,
                         FM10000_TE_TUN_HEADER_CFG,
                         CheckOam,
                         teParserCfg->checkNgeOam);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_CHECK_NGE_C)
    {
        FM_API_REQUIRE(teParserCfg->checkNgeC <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teTunHeaderCfg,
                         FM10000_TE_TUN_HEADER_CFG,
                         CheckC,
                         teParserCfg->checkNgeC);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_ETH_TYPE)
    {
        FM_ARRAY_SET_FIELD(teExvet,
                           FM10000_TE_EXVET,
                           EthernetType,
                           teParserCfg->etherType);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_ETH_TYPE_VLAN)
    {
        FM_API_REQUIRE(teParserCfg->afterVlan <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teExvet,
                         FM10000_TE_EXVET,
                         AfterVLAN,
                         teParserCfg->afterVlan);
    }

    if (fieldSelectMask & FM10000_TE_PARSER_TAG_SIZE)
    {
        FM_API_REQUIRE(teParserCfg->tagSize <=
                           FM_FIELD_UNSIGNED_MAX(FM10000_TE_EXVET, TagSize),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teExvet,
                           FM10000_TE_EXVET,
                           TagSize,
                           teParserCfg->tagSize);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeParser */




/*****************************************************************************/
/** fm10000GetTeParser
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the tunneling engine parser configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[out]      teParserCfg is a user-supplied data structure of type
 *                  ''fm_fm10000TeParserCfg'' used to retrieve the parser
 *                  configuration.
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
fm_status fm10000GetTeParser(fm_int                 sw,
                             fm_int                 te,
                             fm_fm10000TeParserCfg *teParserCfg,
                             fm_bool                useCache)
{
    fm_registerSGListEntry sgList[3];
    fm_status              err = FM_OK;
    fm_uint32              tePorts[FM10000_TE_PORTS_WIDTH] = {0};
    fm_uint32              teTunHeaderCfg[FM10000_TE_TUN_HEADER_CFG_WIDTH] = {0};
    fm_uint32              teExvet[FM10000_TE_EXVET_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teParserCfg = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teParserCfg,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_PORTS_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teParserCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTePorts,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              tePorts,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeTunHeaderCfg,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teTunHeaderCfg,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[2],
                              &fm10000CacheTeExvet,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teExvet,
                              FALSE);

    err = fmRegCacheRead(sw, 3, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    teParserCfg->vxLanPort = FM_ARRAY_GET_FIELD(tePorts,
                                                FM10000_TE_PORTS,
                                                VXLAN);

    teParserCfg->ngePort = FM_ARRAY_GET_FIELD(tePorts,
                                              FM10000_TE_PORTS,
                                              NGE);

    teParserCfg->checkProtocol = FM_ARRAY_GET_BIT(teTunHeaderCfg,
                                                  FM10000_TE_TUN_HEADER_CFG,
                                                  CheckProtocol);

    teParserCfg->checkVersion = FM_ARRAY_GET_BIT(teTunHeaderCfg,
                                                 FM10000_TE_TUN_HEADER_CFG,
                                                 CheckVersion);

    teParserCfg->checkNgeOam = FM_ARRAY_GET_BIT(teTunHeaderCfg,
                                              FM10000_TE_TUN_HEADER_CFG,
                                              CheckOam);

    teParserCfg->checkNgeC = FM_ARRAY_GET_BIT(teTunHeaderCfg,
                                              FM10000_TE_TUN_HEADER_CFG,
                                              CheckC);

    teParserCfg->etherType = FM_ARRAY_GET_FIELD(teExvet,
                                                FM10000_TE_EXVET,
                                                EthernetType);

    teParserCfg->afterVlan = FM_ARRAY_GET_BIT(teExvet,
                                              FM10000_TE_EXVET,
                                              AfterVLAN);

    teParserCfg->tagSize = FM_ARRAY_GET_FIELD(teExvet,
                                              FM10000_TE_EXVET,
                                              TagSize);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeParser */




/*****************************************************************************/
/** fm10000SetTeTrap
 * \ingroup lowlevTe10k
 *
 * \desc            Configure the tunneling engine trap handling.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       teTrapCfg points to a structure of type
 *                  ''fm_fm10000TeTrapCfg'' containing the trap
 *                  configuration to set.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teTrapCfg should be written to the tunneling engine. See
 *                  ''fm_fm10000TeTrapSel'' for bit definitions.
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
fm_status fm10000SetTeTrap(fm_int               sw,
                           fm_int               te,
                           fm_fm10000TeTrapCfg *teTrapCfg,
                           fm_uint32            fieldSelectMask,
                           fm_bool              useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_int                 sgIndex = 0;
    fm_status              err = FM_OK;
    fm_uint32              teTrapGlort[FM10000_TE_TRAP_DGLORT_WIDTH] = {0};
    fm_uint32              teTrapConfig[FM10000_TE_TRAP_CONFIG_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teTrapCfg = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teTrapCfg,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_TRAP_DGLORT_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teTrapCfg != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE((fieldSelectMask & FM10000_TE_TRAP_ALL) != 0, FM_ERR_INVALID_ARGUMENT);

    /* Only add the required register to the scatter-gather list */
    if ((fieldSelectMask & FM10000_TE_TRAP_ALL) != FM10000_TE_TRAP_BASE_DGLORT)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeTrapCfg,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teTrapConfig,
                                  FALSE);
        sgIndex++;
    }

    if (fieldSelectMask & FM10000_TE_TRAP_BASE_DGLORT)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList[sgIndex],
                                  &fm10000CacheTeTrapDglort,
                                  1,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teTrapGlort,
                                  FALSE);
        sgIndex++;
    }

    /* Trap DGLORT register is entirely reconfigured so no need to read it */
    if ((fieldSelectMask & FM10000_TE_TRAP_ALL) != FM10000_TE_TRAP_BASE_DGLORT)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        /* Only read-modify-write the trap cfg */
        err = fmRegCacheRead(sw, 1, sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


    if (fieldSelectMask & FM10000_TE_TRAP_BASE_DGLORT)
    {
        FM_ARRAY_SET_FIELD(teTrapGlort,
                           FM10000_TE_TRAP_DGLORT,
                           TrapDGLORT,
                           teTrapCfg->trapGlort);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_NORMAL)
    {
        FM_API_REQUIRE(teTrapCfg->normal < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           NORMAL,
                           teTrapCfg->normal);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_NO_DGLORT_MATCH)
    {
        FM_API_REQUIRE(teTrapCfg->noDglortMatch < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           NO_DGLORT_MATCH,
                           teTrapCfg->noDglortMatch);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_NO_SGLORT_MATCH)
    {
        FM_API_REQUIRE(teTrapCfg->noSglortMatch < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           NO_SGLORT_MATCH,
                           teTrapCfg->noSglortMatch);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_DECAP_NO_OUTER_L3)
    {
        FM_API_REQUIRE(teTrapCfg->decapNoOuterL3 < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           DECAP_NO_OUTER_L3,
                           teTrapCfg->decapNoOuterL3);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_DECAP_NO_OUTER_L4)
    {
        FM_API_REQUIRE(teTrapCfg->decapNoOuterL4 < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           DECAP_NO_OUTER_L4,
                           teTrapCfg->decapNoOuterL4);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_DECAP_NO_OUTER_TUN)
    {
        FM_API_REQUIRE(teTrapCfg->decapNoOuterTun < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           DECAP_NO_OUTER_TUN,
                           teTrapCfg->decapNoOuterTun);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_DECAP_BAD_CHECKSUM)
    {
        FM_API_REQUIRE(teTrapCfg->decapBadChecksum < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           DECAP_BAD_CSUM,
                           teTrapCfg->decapBadChecksum);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_DECAP_BAD_TUNNEL)
    {
        FM_API_REQUIRE(teTrapCfg->decapBadTunnel < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           DECAP_BAD_TUN,
                           teTrapCfg->decapBadTunnel);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_ENCAP_NO_L3)
    {
        FM_API_REQUIRE(teTrapCfg->encapNoL3 < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           ENCAP_NO_L3,
                           teTrapCfg->encapNoL3);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_ENCAP_NO_L4)
    {
        FM_API_REQUIRE(teTrapCfg->encapNoL4 < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           ENCAP_NO_L4,
                           teTrapCfg->encapNoL4);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_ENCAP_ANY_L4)
    {
        FM_API_REQUIRE(teTrapCfg->encapAnyL4 < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           ENCAP_ANY_L4,
                           teTrapCfg->encapAnyL4);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_TRUNCATED_HEADER)
    {
        FM_API_REQUIRE(teTrapCfg->truncatedHeader < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           TRUNCATED_HEADER,
                           teTrapCfg->truncatedHeader);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_TRUNC_IP_PAYLOAD)
    {
        FM_API_REQUIRE(teTrapCfg->truncIpPayload < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           TRUNC_IP_PAYLOAD,
                           teTrapCfg->truncIpPayload);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_NO_FLOW_MATCH)
    {
        FM_API_REQUIRE(teTrapCfg->noFlowMatch < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           NO_FLOW_MATCH,
                           teTrapCfg->noFlowMatch);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_MISSING_RECORD)
    {
        FM_API_REQUIRE(teTrapCfg->missingRecord < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           MISSING_RECORD,
                           teTrapCfg->missingRecord);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_UC_ERR)
    {
        FM_API_REQUIRE(teTrapCfg->ucErr < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           UC_ERR,
                           teTrapCfg->ucErr);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_LOOKUP_BOUNDS)
    {
        FM_API_REQUIRE(teTrapCfg->lookupBounds < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           TE_LOOKUP_BOUNDS,
                           teTrapCfg->lookupBounds);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_DATA_BOUNDS)
    {
        FM_API_REQUIRE(teTrapCfg->dataBounds < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           TE_DATA_BOUNDS,
                           teTrapCfg->dataBounds);
    }

    if (fieldSelectMask & FM10000_TE_TRAP_HEADER_LIMIT)
    {
        FM_API_REQUIRE(teTrapCfg->headerLimit < FM_FM10000_TE_TRAP_MAX,
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teTrapConfig,
                           FM10000_TE_TRAP_CONFIG,
                           TOTAL_HEADER_LIMIT,
                           teTrapCfg->headerLimit);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, sgIndex, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeTrap */




/*****************************************************************************/
/** fm10000GetTeTrap
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the tunneling engine trap configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[out]      teTrapCfg is a user-supplied data structure of type
 *                  ''fm_fm10000TeTrapCfg'' used to retrieve the trap
 *                  configuration.
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
fm_status fm10000GetTeTrap(fm_int               sw,
                           fm_int               te,
                           fm_fm10000TeTrapCfg *teTrapCfg,
                           fm_bool              useCache)
{
    fm_registerSGListEntry sgList[2];
    fm_status              err = FM_OK;
    fm_uint32              teTrapGlort[FM10000_TE_TRAP_DGLORT_WIDTH] = {0};
    fm_uint32              teTrapConfig[FM10000_TE_TRAP_CONFIG_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "teTrapCfg = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  (void*) teTrapCfg,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_TRAP_DGLORT_ENTRIES, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teTrapCfg != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[0],
                              &fm10000CacheTeTrapCfg,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teTrapConfig,
                              FALSE);

    FM_REGS_CACHE_FILL_SGLIST(&sgList[1],
                              &fm10000CacheTeTrapDglort,
                              1,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teTrapGlort,
                              FALSE);

    err = fmRegCacheRead(sw, 2, sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


    teTrapCfg->trapGlort = FM_ARRAY_GET_FIELD(teTrapGlort,
                                              FM10000_TE_TRAP_DGLORT,
                                              TrapDGLORT);

    teTrapCfg->normal = FM_ARRAY_GET_FIELD(teTrapConfig,
                                           FM10000_TE_TRAP_CONFIG,
                                           NORMAL);

    teTrapCfg->noDglortMatch = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                  FM10000_TE_TRAP_CONFIG,
                                                  NO_DGLORT_MATCH);

    teTrapCfg->noSglortMatch = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                  FM10000_TE_TRAP_CONFIG,
                                                  NO_SGLORT_MATCH);

    teTrapCfg->decapNoOuterL3 = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                   FM10000_TE_TRAP_CONFIG,
                                                   DECAP_NO_OUTER_L3);

    teTrapCfg->decapNoOuterL4 = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                   FM10000_TE_TRAP_CONFIG,
                                                   DECAP_NO_OUTER_L4);

    teTrapCfg->decapNoOuterTun = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                    FM10000_TE_TRAP_CONFIG,
                                                    DECAP_NO_OUTER_TUN);

    teTrapCfg->decapBadChecksum = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                     FM10000_TE_TRAP_CONFIG,
                                                     DECAP_BAD_CSUM);

    teTrapCfg->decapBadTunnel = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                   FM10000_TE_TRAP_CONFIG,
                                                   DECAP_BAD_TUN);

    teTrapCfg->encapNoL3 = FM_ARRAY_GET_FIELD(teTrapConfig,
                                              FM10000_TE_TRAP_CONFIG,
                                              ENCAP_NO_L3);

    teTrapCfg->encapNoL4 = FM_ARRAY_GET_FIELD(teTrapConfig,
                                              FM10000_TE_TRAP_CONFIG,
                                              ENCAP_NO_L4);

    teTrapCfg->encapAnyL4 = FM_ARRAY_GET_FIELD(teTrapConfig,
                                               FM10000_TE_TRAP_CONFIG,
                                               ENCAP_ANY_L4);

    teTrapCfg->truncatedHeader = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                    FM10000_TE_TRAP_CONFIG,
                                                    TRUNCATED_HEADER);

    teTrapCfg->truncIpPayload = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                   FM10000_TE_TRAP_CONFIG,
                                                   TRUNC_IP_PAYLOAD);

    teTrapCfg->noFlowMatch = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                FM10000_TE_TRAP_CONFIG,
                                                NO_FLOW_MATCH);

    teTrapCfg->missingRecord = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                  FM10000_TE_TRAP_CONFIG,
                                                  MISSING_RECORD);

    teTrapCfg->ucErr = FM_ARRAY_GET_FIELD(teTrapConfig,
                                          FM10000_TE_TRAP_CONFIG,
                                          UC_ERR);

    teTrapCfg->lookupBounds = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                 FM10000_TE_TRAP_CONFIG,
                                                 TE_LOOKUP_BOUNDS);

    teTrapCfg->dataBounds = FM_ARRAY_GET_FIELD(teTrapConfig,
                                               FM10000_TE_TRAP_CONFIG,
                                               TE_DATA_BOUNDS);

    teTrapCfg->headerLimit = FM_ARRAY_GET_FIELD(teTrapConfig,
                                                FM10000_TE_TRAP_CONFIG,
                                                TOTAL_HEADER_LIMIT);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeTrap */




/*****************************************************************************/
/** fm10000SetTeCnt
 * \ingroup lowlevTe10k
 *
 * \desc            Set the tunneling engine global counter.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       dropCnt is the value to set as the global number of frames
 *                  dropped.
 * 
 * \param[in]       frameInCnt is the value to set as the global number of
 *                  frames that entered the tunneling engine.
 * 
 * \param[in]       frameDoneCnt is the value to set as the global number of
 *                  frames processed by the tunneling engine.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000SetTeCnt(fm_int    sw,
                          fm_int    te,
                          fm_uint16 dropCnt,
                          fm_uint32 frameInCnt,
                          fm_uint32 frameDoneCnt)
{
    fm_switch *switchPtr;
    fm_uint64  value64;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "dropCnt = %d, "
                  "frameInCnt = %d, "
                  "frameDoneCnt = %d\n",
                  sw,
                  te,
                  dropCnt,
                  frameInCnt,
                  frameDoneCnt );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_FRAMES_IN_ENTRIES, FM_ERR_INVALID_ARGUMENT);

    /* Cache API is not used for those registers */
    err = switchPtr->WriteUINT64(sw,
                                 FM10000_TE_FRAMES_IN(te, 0),
                                 frameInCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_TE_FRAMES_DONE(te, 0),
                                 frameDoneCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_TE_DROP_COUNT(te, 0),
                                 dropCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    /* This is to unblock the TE registers */
    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_FRAMES_IN(te, 0),
                                &value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeCnt */




/*****************************************************************************/
/** fm10000GetTeCnt
 * \ingroup lowlevTe10k
 *
 * \desc            Retrieve the tunneling engine global counter.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[out]      dropCnt refer to the global number of frames dropped.
 * 
 * \param[out]      frameInCnt refer to the global number of frames that
 *                  entered the tunneling engine.
 * 
 * \param[out]      frameDoneCnt refer to the global number of frames
 *                  processed by the tunneling engine.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000GetTeCnt(fm_int     sw,
                          fm_int     te,
                          fm_uint16 *dropCnt,
                          fm_uint32 *frameInCnt,
                          fm_uint32 *frameDoneCnt)
{
    fm_switch *switchPtr;
    fm_uint64  value64;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "dropCnt = %p, "
                  "frameInCnt = %p, "
                  "frameDoneCnt = %p\n",
                  sw,
                  te,
                  (void*) dropCnt,
                  (void*) frameInCnt,
                  (void*) frameDoneCnt );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_FRAMES_IN_ENTRIES, FM_ERR_INVALID_ARGUMENT);

    /* Cache API is not used for those registers */
    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_FRAMES_IN(te, 0),
                                &value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    *frameInCnt = value64;

    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_FRAMES_DONE(te, 0),
                                &value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    *frameDoneCnt = value64;

    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_DROP_COUNT(te, 0),
                                &value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    *dropCnt = value64;


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeCnt */




/*****************************************************************************/
/** fm10000SetTeFlowCnt
 * \ingroup intlowlevTe10k
 *
 * \desc            Set the tunneling engine flow counter.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the counter index.
 * 
 * \param[in]       frameCnt is the value to set as the frame count.
 * 
 * \param[in]       byteCnt is the value to set as the byte count.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000SetTeFlowCnt(fm_int    sw,
                              fm_int    te,
                              fm_int    index,
                              fm_uint64 frameCnt,
                              fm_uint64 byteCnt)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;
    fm_uint32  teStats[FM10000_TE_STATS_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "frameCnt = %lld, "
                  "byteCnt = %lld\n",
                  sw,
                  te,
                  index,
                  frameCnt,
                  byteCnt );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_STATS_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_STATS_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(frameCnt <= FM_FIELD_UNSIGNED_MAX64(FM10000_TE_STATS, Frames),
                   FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(byteCnt <= FM_FIELD_UNSIGNED_MAX64(FM10000_TE_STATS, Bytes),
                   FM_ERR_INVALID_ARGUMENT);

    /* Cache API is not used for this register */
    FM_ARRAY_SET_FIELD64(teStats,
                         FM10000_TE_STATS,
                         Frames,
                         frameCnt);

    FM_ARRAY_SET_FIELD64(teStats,
                         FM10000_TE_STATS,
                         Bytes,
                         byteCnt);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_TE_STATS(te, index, 0),
                                     FM10000_TE_STATS_WIDTH,
                                     teStats);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeFlowCnt */




/*****************************************************************************/
/** fm10000GetTeFlowCnt
 * \ingroup intlowlevTe10k
 *
 * \desc            Retrieve the tunneling engine flow counter.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the counter index.
 * 
 * \param[out]      frameCnt is the value to get as the frame count.
 * 
 * \param[out]      byteCnt is the value to get as the byte count.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000GetTeFlowCnt(fm_int     sw,
                              fm_int     te,
                              fm_int     index,
                              fm_uint64 *frameCnt,
                              fm_uint64 *byteCnt)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;
    fm_uint32  teStats[FM10000_TE_STATS_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "frameCnt = %p, "
                  "byteCnt = %p\n",
                  sw,
                  te,
                  index,
                  (void*) frameCnt,
                  (void*) byteCnt );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_STATS_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_STATS_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);

    /* Cache API is not used for this register */
    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_TE_STATS(te, index, 0),
                                    FM10000_TE_STATS_WIDTH,
                                    teStats);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    *frameCnt = FM_ARRAY_GET_FIELD64(teStats,
                                     FM10000_TE_STATS,
                                     Frames);

    *byteCnt = FM_ARRAY_GET_FIELD64(teStats,
                                    FM10000_TE_STATS,
                                    Bytes);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeFlowCnt */




/*****************************************************************************/
/** fm10000SetTeFlowUsed
 * \ingroup intlowlevTe10k
 *
 * \desc            Set the tunneling engine usage bit for a specific flow
 *                  pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the flow pointer index.
 * 
 * \param[in]       used is the value to set.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000SetTeFlowUsed(fm_int  sw,
                               fm_int  te,
                               fm_int  index,
                               fm_bool used)
{
    fm_switch *switchPtr;
    fm_int     divIndex;
    fm_int     modIndex;
    fm_uint64  value64;
    fm_status  err = FM_OK;
    fm_bool    regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "used = %d\n",
                  sw,
                  te,
                  index,
                  used );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_USED_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(used <= 1, FM_ERR_INVALID_ARGUMENT);

    /* The hardware set the bit FM10000_TE_USED[index/64].Used[index%64]
     * each time a flow matches a packet received */
    divIndex = index >> 6;    //Div 64
    modIndex = index & 0x3f;  //Mod 64

    FM_API_REQUIRE(divIndex < FM10000_TE_USED_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);

    /**************************************************
     * Acquire the regLock, so that the read-modify-write
     * is atomic.
     **************************************************/
    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_USED(te, divIndex, 0),
                                &value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    /* Only update the proper bit as specified by index */
    value64 &= ~((fm_uint64)(1LL << modIndex));
    value64 |= ((fm_uint64)((fm_uint64)used << modIndex));

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_TE_USED(te, divIndex, 0),
                                 value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);


ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeFlowUsed */




/*****************************************************************************/
/** fm10000GetTeFlowUsed
 * \ingroup intlowlevTe10k
 *
 * \desc            Retrieve the tunneling engine usage bit for a specific flow
 *                  pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the flow pointer index.
 * 
 * \param[out]      used is the value to get.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000GetTeFlowUsed(fm_int   sw,
                               fm_int   te,
                               fm_int   index,
                               fm_bool *used)
{
    fm_switch *switchPtr;
    fm_int     divIndex;
    fm_int     modIndex;
    fm_uint64  value64;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "used = %p\n",
                  sw,
                  te,
                  index,
                  (void*) used );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_USED_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);

    /* The hardware set the bit FM10000_TE_USED[index/64].Used[index%64]
     * each time a flow matches a packet received */
    divIndex = index >> 6;    //Div 64
    modIndex = index & 0x3f;  //Mod 64

    FM_API_REQUIRE(divIndex < FM10000_TE_USED_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);

    err = switchPtr->ReadUINT64(sw,
                                FM10000_TE_USED(te, divIndex, 0),
                                &value64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    /* Only return the proper bit as specified by index */
    *used = ((value64 >> modIndex) & 0x1LL);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeFlowUsed */




/*****************************************************************************/
/** fm10000ResetTeFlowUsed
 * \ingroup intlowlevTe10k
 *
 * \desc            Reset the whole tunneling engine usage bit table.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
fm_status fm10000ResetTeFlowUsed(fm_int  sw,
                                 fm_int  te)
{
    fm_switch *switchPtr;
    fm_int     i;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d\n",
                  sw,
                  te );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_USED_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);

    /* Clear the whole table */
    for (i = 0 ; i < FM10000_TE_USED_ENTRIES_0; i++)
    {
        err = switchPtr->WriteUINT64(sw,
                                     FM10000_TE_USED(te, i, 0),
                                     0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000ResetTeFlowUsed */




/*****************************************************************************/
/** fm10000SetTeLookup
 * \ingroup intlowlevTe10k
 *
 * \desc            Configure a tunneling engine lookup entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the lookup entry to set.
 * 
 * \param[in]       teLookup points to a structure of type
 *                  ''fm_fm10000TeLookup'' containing the lookup field value.
 * 
 * \param[in]       fieldSelectMask is a bit mask identifying which fields in
 *                  teLookup should be written to the tunneling engine. See
 *                  ''fm_fm10000TeLookupSel'' for bit definitions.
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
fm_status fm10000SetTeLookup(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeLookup *teLookup,
                             fm_uint32           fieldSelectMask,
                             fm_bool             useCache)
{
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              teLookupReg[FM10000_TE_LOOKUP_WIDTH] = {0};
    fm_bool                regLockTaken = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "teLookup = %p, "
                  "fieldSelectMask = %x, "
                  "useCache = %s\n",
                  sw,
                  te,
                  index,
                  (void*) teLookup,
                  fieldSelectMask,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_LOOKUP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_LOOKUP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teLookup != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheTeLookup,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teLookupReg,
                              FALSE);

    /**************************************************
     * If we want to update only one of its fields,
     * we need to get the current value to start 
     * with 
     **************************************************/
    if (fieldSelectMask != FM10000_TE_LOOKUP_ALL)
    {
        /**************************************************
         * Acquire the regLock, so that the read-modify-write
         * is atomic.
         **************************************************/
        TAKE_REG_LOCK(sw);
        regLockTaken = TRUE;

        err = fmRegCacheRead(sw, 1, &sgList, useCache);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    if (fieldSelectMask & FM10000_TE_LOOKUP_DATA_PTR)
    {
        FM_ARRAY_SET_FIELD(teLookupReg,
                           FM10000_TE_LOOKUP,
                           DataPointer,
                           teLookup->dataPtr);
    }

    if (fieldSelectMask & FM10000_TE_LOOKUP_DATA_LENGTH)
    {
        FM_API_REQUIRE(teLookup->dataLength <=
                           FM_FIELD_UNSIGNED_MAX(FM10000_TE_LOOKUP, DataLength),
                       FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_FIELD(teLookupReg,
                           FM10000_TE_LOOKUP,
                           DataLength,
                           teLookup->dataLength);
    }

    if (fieldSelectMask & FM10000_TE_LOOKUP_LAST)
    {
        FM_API_REQUIRE(teLookup->last <= 1, FM_ERR_INVALID_ARGUMENT);
        FM_ARRAY_SET_BIT(teLookupReg,
                         FM10000_TE_LOOKUP,
                         Last,
                         teLookup->last);
    }

    /* write to the register */
    err = fmRegCacheWrite(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    err = fm10000SyncTeDataLookup(sw, te);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeLookup */




/*****************************************************************************/
/** fm10000GetTeLookup
 * \ingroup intlowlevTe10k
 *
 * \desc            Retrieve a tunneling engine lookup entry.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       index is the lookup entry to get.
 * 
 * \param[out]      teLookup is a user-supplied data structure of type
 *                  ''fm_fm10000TeLookup'' used to retrieve the lookup
 *                  configuration.
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
fm_status fm10000GetTeLookup(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeLookup *teLookup,
                             fm_bool             useCache)
{
    fm_registerSGListEntry sgList;
    fm_status              err = FM_OK;
    fm_uint32              teLookupReg[FM10000_TE_LOOKUP_WIDTH] = {0};

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "index = %d, "
                  "teLookup = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  index,
                  (void*) teLookup,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_LOOKUP_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(index < FM10000_TE_LOOKUP_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teLookup != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheTeLookup,
                              1,
                              index,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teLookupReg,
                              FALSE);

    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    teLookup->dataPtr = FM_ARRAY_GET_FIELD(teLookupReg,
                                           FM10000_TE_LOOKUP,
                                           DataPointer);

    teLookup->dataLength = FM_ARRAY_GET_FIELD(teLookupReg,
                                              FM10000_TE_LOOKUP,
                                              DataLength);

    teLookup->last = FM_ARRAY_GET_BIT(teLookupReg,
                                      FM10000_TE_LOOKUP,
                                      Last);


ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeLookup */




/*****************************************************************************/
/** fm10000SetTeData
 * \ingroup intlowlevTe10k
 *
 * \desc            Configure a tunneling engine data block.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       baseIndex is the first entry to set.
 * 
 * \param[in]       teData points to one or multiple structure of type
 *                  ''fm_fm10000TeData'' containing the data information.
 * 
 * \param[in]       teDataLength is the number of element referenced by teData.
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
fm_status fm10000SetTeData(fm_int            sw,
                           fm_int            te,
                           fm_int            baseIndex,
                           fm_fm10000TeData *teData,
                           fm_int            teDataLength,
                           fm_bool           useCache)
{
    fm_registerSGListEntry        sgList;
    fm_int                        teDataIndex;
    fm_uint                       i;
    fm_status                     err = FM_OK;
    fm_uint                       teDataReg16Length = 0;
    fm_uint                       teDataReg16PadLength;
    fm_uint16                     teDataReg16[FM10000_TE_DATA_LENGTH_SIZE_16];
    fm_uint                       teDataRegLength;
    fm_uint32                     teDataReg[FM10000_TE_DATA_LENGTH_SIZE_32];
    fm_uint32                     ipTmp;
    fm_fm10000TeLookup *          nextLookup;
    fm_fm10000TeDataFlowKeyVal *  flowKeyVal;
    fm_fm10000TeDataFlowEncapVal *flowEncapVal;
    fm_fm10000TeDataFlowDecapVal *flowDecapVal;
    fm_fm10000TeDataTunnelVal    *tunnelVal;


    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "baseIndex = %d, "
                  "teData = %p, "
                  "teDataLength = %d, "
                  "useCache = %s\n",
                  sw,
                  te,
                  baseIndex,
                  (void*) teData,
                  teDataLength,
                  FM_BOOLSTRING(useCache) );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DATA_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(baseIndex < FM10000_TE_DATA_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teData != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDataLength > 0, FM_ERR_INVALID_ARGUMENT);

    /* Go over all the teData structure and pack them all together using 16-bit
     * boundary. The size of the whole block can't exceed 128 entries of 128-bit
     * or (128 * 8) 16-bit entries. as defined with 
     * FM10000_TE_DATA_LENGTH_SIZE_16. */
    for (teDataIndex = 0 ; teDataIndex < teDataLength ; teDataIndex++)
    {
        switch (teData[teDataIndex].blockType)
        {
            /* 32 bit wide */
            case FM_FM10000_TE_DATA_BLOCK_POINTER:
                FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeLookup structure */
                nextLookup = &teData[teDataIndex].blockVal.nextLookup;

                /* First 16 bit is the pointer location in FM10000_TE_DATA */
                teDataReg16[teDataReg16Length++] = nextLookup->dataPtr;

                FM_API_REQUIRE(nextLookup->dataLength <=
                                   FM_FIELD_UNSIGNED_MAX(FM10000_TE_LOOKUP, DataLength),
                               FM_ERR_INVALID_ARGUMENT);

                /* Next 7 bit is the length */
                teDataReg16[teDataReg16Length] = nextLookup->dataLength;

                /* Bit position */
                teDataReg16[teDataReg16Length++] |= ((nextLookup->last & 0x1) << FM10000_TE_LOOKUP_LAST_POS);
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_KEY:
                FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataFlowKeyVal structure */
                flowKeyVal = &teData[teDataIndex].blockVal.flowKeyVal;

                /* First 16 bit refer to the key mask */
                teDataReg16[teDataReg16Length++] = flowKeyVal->searchKeyConfig;

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VSI_TEP)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->vsiTep;
                }

                /* 32 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VNI)
                {
                    FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->vni & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowKeyVal->vni >> 16) & 0xff;
                }

                /* 48 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_DMAC)
                {
                    FM_API_REQUIRE((teDataReg16Length + 2) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->dmac & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowKeyVal->dmac >> 16) & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowKeyVal->dmac >> 32) & 0xffff;
                }

                /* 48 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_SMAC)
                {
                    FM_API_REQUIRE((teDataReg16Length + 2) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->smac & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowKeyVal->smac >> 16) & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowKeyVal->smac >> 32) & 0xffff;
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VLAN)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = (flowKeyVal->vlan & 0xfff);
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_DIP)
                {
                    /* 128 bit */
                    if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(flowKeyVal->dip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(flowKeyVal->dip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_SIP)
                {
                    /* 128 bit */
                    if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(flowKeyVal->sip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(flowKeyVal->sip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->l4Src;
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->l4Dst;
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_PROT)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowKeyVal->protocol;
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP:
                FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataFlowEncapVal structure */
                flowEncapVal = &teData[teDataIndex].blockVal.flowEncapVal;

                /* First 16 bit is the encap key mask */
                teDataReg16[teDataReg16Length++] = flowEncapVal->encapConfig;

                /* 32 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_VNI)
                {
                    FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->vni & 0xffff);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->vni >> 16 & 0xff);
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_COUNTER)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    FM_API_REQUIRE(flowEncapVal->counterIdx < FM10000_TE_STATS_ENTRIES_0,
                                   FM_ERR_INVALID_ARGUMENT);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->counterIdx;
                }

                /* 48 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_DMAC)
                {
                    FM_API_REQUIRE((teDataReg16Length + 2) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->dmac & 0xffff);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->dmac >> 16 & 0xffff);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->dmac >> 32 & 0xffff);
                }

                /* 48 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_SMAC)
                {
                    FM_API_REQUIRE((teDataReg16Length + 2) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->smac & 0xffff);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->smac >> 16 & 0xffff);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->smac >> 32 & 0xffff);
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_VLAN)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = (flowEncapVal->vlan & 0xfff);
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_DIP)
                {
                    /* 128 bit */
                    if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(flowEncapVal->dip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(flowEncapVal->dip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_SIP)
                {
                    /* 128 bit */
                    if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(flowEncapVal->sip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(flowEncapVal->sip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->l4Src;
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->l4Dst;
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TTL)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->ttl;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_NGE)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->ngeMask;

                    /* 32 bit times the number of data word defines */
                    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE; i++)
                    {
                        if (flowEncapVal->ngeMask & (1 << i))
                        {
                            FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                           FM_ERR_BUFFER_FULL);
                            teDataReg16[teDataReg16Length++] = flowEncapVal->ngeData[i] & 0xffff;
                            teDataReg16[teDataReg16Length++] = (flowEncapVal->ngeData[i] >> 16) & 0xffff;
                        }
                    }
                }

                /* 32 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL_PTR)
                {
                    /* Uses the same definition as TE_LOOKUP */
                    FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->tunnelIdx.dataPtr;

                    FM_API_REQUIRE(flowEncapVal->tunnelIdx.dataLength <=
                                       FM_FIELD_UNSIGNED_MAX(FM10000_TE_LOOKUP, DataLength),
                                   FM_ERR_INVALID_ARGUMENT);
                    teDataReg16[teDataReg16Length++] = flowEncapVal->tunnelIdx.dataLength;
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP:
                FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataFlowDecapVal structure */
                flowDecapVal = &teData[teDataIndex].blockVal.flowDecapVal;

                /* First 16 bit is the decap key mask */
                teDataReg16[teDataReg16Length] = flowDecapVal->decapConfig;

                FM_API_REQUIRE(flowDecapVal->outerHeader < FM_FM10000_TE_OUTER_HEADER_MAX,
                               FM_ERR_INVALID_ARGUMENT);
                teDataReg16[teDataReg16Length++] |= flowDecapVal->outerHeader;

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DGLORT)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->dglort;
                }

                /* 48 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DMAC)
                {
                    FM_API_REQUIRE((teDataReg16Length + 2) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->dmac & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowDecapVal->dmac >> 16) & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowDecapVal->dmac >> 32) & 0xffff;
                }

                /* 48 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_SMAC)
                {
                    FM_API_REQUIRE((teDataReg16Length + 2) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->smac & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowDecapVal->smac >> 16) & 0xffff;
                    teDataReg16[teDataReg16Length++] = (flowDecapVal->smac >> 32) & 0xffff;
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_VLAN)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->vlan & 0xfff;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DIP)
                {
                    /* 128 bit */
                    if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(flowDecapVal->dip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(flowDecapVal->dip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_SIP)
                {
                    /* 128 bit */
                    if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(flowDecapVal->sip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(flowDecapVal->sip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_TTL)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->ttl;
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->l4Src;
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->l4Dst;
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_COUNTER)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    FM_API_REQUIRE(flowDecapVal->counterIdx < FM10000_TE_STATS_ENTRIES_0,
                                   FM_ERR_INVALID_ARGUMENT);
                    teDataReg16[teDataReg16Length++] = flowDecapVal->counterIdx;
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA:
                FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataTunnelVal structure */
                tunnelVal = &teData[teDataIndex].blockVal.tunnelVal;

                /* First 16 bit is the tunnel key mask */
                teDataReg16[teDataReg16Length] = tunnelVal->tunnelConfig;

                FM_API_REQUIRE(tunnelVal->tunnelType < FM_FM10000_TE_TUNNEL_TYPE_MAX,
                               FM_ERR_INVALID_ARGUMENT);
                teDataReg16[teDataReg16Length++] |= tunnelVal->tunnelType;

                /* Destination IP is always present */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_IPV6)
                {
                    /* 128 bit */
                    FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    for (i = 0 ; i < 4 ; i++)
                    {
                        ipTmp = ntohl(tunnelVal->dip.addr[i]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }
                else
                {
                    /* 32 bit */
                    FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    ipTmp = ntohl(tunnelVal->dip.addr[0]);
                    teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                    teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_SIP)
                {
                    /* 128 bit */
                    if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Length + 7) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = ntohl(tunnelVal->sip.addr[i]);
                            teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                            teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                        }
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = ntohl(tunnelVal->sip.addr[0]);
                        teDataReg16[teDataReg16Length++] = ipTmp & 0xffff;
                        teDataReg16[teDataReg16Length++] = ipTmp >> 16 & 0xffff;
                    }
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TOS)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = tunnelVal->tos;
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TTL)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = tunnelVal->ttl;
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = tunnelVal->l4Dst;
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = tunnelVal->l4Src;
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
                {
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    FM_API_REQUIRE(tunnelVal->counterIdx < FM10000_TE_STATS_ENTRIES_0,
                                   FM_ERR_INVALID_ARGUMENT);
                    teDataReg16[teDataReg16Length++] = tunnelVal->counterIdx;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_NGE)
                {
                    /* 16 bit for the mask */
                    FM_API_REQUIRE(teDataReg16Length < FM10000_TE_DATA_LENGTH_SIZE_16,
                                   FM_ERR_BUFFER_FULL);
                    teDataReg16[teDataReg16Length++] = tunnelVal->ngeMask;

                    /* 32 bit for every NGE word defines */
                    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE; i++)
                    {
                        if (tunnelVal->ngeMask & (1 << i))
                        {
                            FM_API_REQUIRE((teDataReg16Length + 1) < FM10000_TE_DATA_LENGTH_SIZE_16,
                                           FM_ERR_BUFFER_FULL);
                            teDataReg16[teDataReg16Length++] = tunnelVal->ngeData[i] & 0xffff;
                            teDataReg16[teDataReg16Length++] = (tunnelVal->ngeData[i] >> 16) & 0xffff;
                        }
                    }
                }
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                break;
        }
    }

    /* Pad using 128-bit boundary and fill it with zero */
    teDataReg16PadLength = ((~teDataReg16Length & 0x7) + 1) & 0x7;

    for (i = 0 ; i < teDataReg16PadLength ; i++)
    {
        teDataReg16[teDataReg16Length++] = 0;
    }

    /* Convert 16-bit array to 32-bit for the cache API */
    for (i = 0 ; i < teDataReg16Length ; i+= 2)
    {
        teDataReg[i >> 1] = teDataReg16[i];
        teDataReg[i >> 1] |= (teDataReg16[i + 1] << 16);
    }

    /* Register length is defined on a 128-bit width */
    teDataRegLength = (teDataReg16Length / (2 * FM10000_TE_DATA_WIDTH));

    FM_API_REQUIRE((baseIndex + teDataRegLength) <= FM10000_TE_DATA_ENTRIES_0,
                   FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheTeData,
                              teDataRegLength,
                              baseIndex,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDataReg,
                              FALSE);

    /* write to the register */
    err = fmRegCacheWrite(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000SetTeData */




/*****************************************************************************/
/** fm10000GetTeData
 * \ingroup intlowlevTe10k
 *
 * \desc            Retrieve a tunneling engine data block.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 * 
 * \param[in]       baseIndex is the first entry to get.
 * 
 * \param[in]       blockLength represent the number of TE_DATA register to
 *                  extract.
 * 
 * \param[in]       encap specify the direction of the decoded block. TRUE for
 *                  encapsulation, FALSE for decapsulation.
 * 
 * \param[in,out]   teData is an array of user-supplied data structure of type
 *                  ''fm_fm10000TeLookup'' used to retrieve the data
 *                  configuration. The first element must be set to the type
 *                  expected at that position. If the first element is of type
 *                  ''FM_FM10000_TE_DATA_BLOCK_POINTER'' than the second element
 *                  type must also be defined.
 * 
 * \param[in]       teDataLength is the number of element referenced by teData.
 * 
 * \param[out]      teDataReturnLength is used to return the number of element
 *                  retrieved.
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
fm_status fm10000GetTeData(fm_int            sw,
                           fm_int            te,
                           fm_int            baseIndex,
                           fm_int            blockLength,
                           fm_bool           encap,
                           fm_fm10000TeData *teData,
                           fm_int            teDataLength,
                           fm_int           *teDataReturnLength,
                           fm_bool           useCache)
{
    fm_registerSGListEntry        sgList;
    fm_int                        teDataIndex;
    fm_uint                       i;
    fm_status                     err = FM_OK;
    fm_uint                       teDataReg16Length;
    fm_uint                       teDataReg16Index = 0;
    fm_uint16                     teDataReg16[FM10000_TE_DATA_LENGTH_SIZE_16];
    fm_uint32                     teDataReg[FM10000_TE_DATA_LENGTH_SIZE_32];
    fm_uint32                     ipTmp;
    fm_fm10000TeLookup *          nextLookup;
    fm_fm10000TeDataFlowKeyVal *  flowKeyVal;
    fm_fm10000TeDataFlowEncapVal *flowEncapVal;
    fm_fm10000TeDataFlowDecapVal *flowDecapVal;
    fm_fm10000TeDataTunnelVal    *tunnelVal;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d, "
                  "baseIndex = %d, "
                  "blockLength = %d, "
                  "encap = %d, "
                  "teData = %p, "
                  "teDataLength = %d, "
                  "teDataReturnLength = %p, "
                  "useCache = %s\n",
                  sw,
                  te,
                  baseIndex,
                  blockLength,
                  encap,
                  (void*) teData,
                  teDataLength,
                  (void*) teDataReturnLength,
                  FM_BOOLSTRING(useCache) );


    teDataIndex = 0;
    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    /* sanity check on the arguments */
    FM_API_REQUIRE(te < FM10000_TE_DATA_ENTRIES_1, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(baseIndex + blockLength <= FM10000_TE_DATA_ENTRIES_0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teData != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDataLength > 0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDataReturnLength != NULL, FM_ERR_INVALID_ARGUMENT);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheTeData,
                              blockLength,
                              baseIndex,
                              te,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              teDataReg,
                              FALSE);

    err = fmRegCacheRead(sw, 1, &sgList, useCache);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    /* Convert 128-bit register to 16-bit array for processing */
    teDataReg16Length = blockLength * 2 * FM10000_TE_DATA_WIDTH;
    for (i = 0 ; i < teDataReg16Length ; i+= 2)
    {
        teDataReg16[i] = teDataReg[i >> 1] & 0xffff;
        teDataReg16[i + 1] = (teDataReg[i >> 1] >> 16) & 0xffff;
    }

    /* Start with the block type defined and extract data based on that type.
     * if this type is a Next Ptr, the next block also needs to be defines. */
    for (teDataIndex = 0 ; teDataIndex < teDataLength ; teDataIndex++)
    {
        switch (teData[teDataIndex].blockType)
        {
            /* 32 bit wide */
            case FM_FM10000_TE_DATA_BLOCK_POINTER:
                FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeLookup structure */
                nextLookup = &teData[teDataIndex].blockVal.nextLookup;

                /* First 16 bit is the pointer location in FM10000_TE_DATA */
                nextLookup->dataPtr = teDataReg16[teDataReg16Index++];

                /* Next 7 bit is the length */
                nextLookup->dataLength = teDataReg16[teDataReg16Index] & 0x7f;

                /* Bit position */
                nextLookup->last = (teDataReg16[teDataReg16Index++] >> FM10000_TE_LOOKUP_LAST_POS) & 0x1;
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_KEY:
                if (teDataReg16Index == teDataReg16Length)
                {
                    /* Previous entry was the last one */
                    teDataIndex--;
                    goto ABORT;
                }

                FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataFlowKeyVal structure */
                flowKeyVal = &teData[teDataIndex].blockVal.flowKeyVal;

                /* First 16 bit refer to the key mask */
                flowKeyVal->searchKeyConfig = teDataReg16[teDataReg16Index++];

                /* Key equal to zero refer to the end of the block */
                if (flowKeyVal->searchKeyConfig == 0)
                {
                    /* This was not a valid entry */
                    teDataIndex--;
                    goto ABORT;
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VSI_TEP)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->vsiTep = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VNI)
                {
                    FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->vni = teDataReg16[teDataReg16Index++];
                    flowKeyVal->vni |= (fm_uint32)(teDataReg16[teDataReg16Index++] & 0xff) << 16;
                }

                /* 48 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_DMAC)
                {
                    FM_API_REQUIRE((teDataReg16Index + 2) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->dmac = teDataReg16[teDataReg16Index++];
                    flowKeyVal->dmac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 16;
                    flowKeyVal->dmac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 32;
                }

                /* 48 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_SMAC)
                {
                    FM_API_REQUIRE((teDataReg16Index + 2) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->smac = teDataReg16[teDataReg16Index++];
                    flowKeyVal->smac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 16;
                    flowKeyVal->smac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 32;
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VLAN)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->vlan = teDataReg16[teDataReg16Index++] & 0xfff;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_DIP)
                {
                    /* 128 bit */
                    if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            flowKeyVal->dip.addr[i] = htonl(ipTmp);
                        }
                        flowKeyVal->dip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        flowKeyVal->dip.addr[0] = htonl(ipTmp);
                        flowKeyVal->dip.isIPv6 = FALSE;
                    }
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_SIP)
                {
                    /* 128 bit */
                    if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            flowKeyVal->sip.addr[i] = htonl(ipTmp);
                        }
                        flowKeyVal->sip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        flowKeyVal->sip.addr[0] = htonl(ipTmp);
                        flowKeyVal->sip.isIPv6 = FALSE;
                    }
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->l4Src = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->l4Dst = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_PROT)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowKeyVal->protocol = teDataReg16[teDataReg16Index++];
                }

                /* Is it the last block to process? */
                if ((teDataIndex + 1) < teDataLength)
                {
                    /* Next Block type should be a DATA_ENCAP */
                    if (encap)
                    {
                        teData[teDataIndex + 1].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP;
                    }
                    /* Next Block type should be a DATA_DECAP */
                    else
                    {
                        teData[teDataIndex + 1].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP;
                    }
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP:
                FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataFlowEncapVal structure */
                flowEncapVal = &teData[teDataIndex].blockVal.flowEncapVal;

                /* First 16 bit is the encap key mask */
                flowEncapVal->encapConfig = teDataReg16[teDataReg16Index++];

                /* 32 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_VNI)
                {
                    FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->vni = teDataReg16[teDataReg16Index++];
                    flowEncapVal->vni |= (fm_uint32)(teDataReg16[teDataReg16Index++] & 0xff) << 16;
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_COUNTER)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->counterIdx = teDataReg16[teDataReg16Index++];
                }

                /* 48 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_DMAC)
                {
                    FM_API_REQUIRE((teDataReg16Index + 2) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->dmac = teDataReg16[teDataReg16Index++];
                    flowEncapVal->dmac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 16;
                    flowEncapVal->dmac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 32;
                }

                /* 48 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_SMAC)
                {
                    FM_API_REQUIRE((teDataReg16Index + 2) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->smac = teDataReg16[teDataReg16Index++];
                    flowEncapVal->smac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 16;
                    flowEncapVal->smac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 32;
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_VLAN)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->vlan  = teDataReg16[teDataReg16Index++] & 0xfff;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_DIP)
                {
                    /* 128 bit */
                    if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            flowEncapVal->dip.addr[i] = htonl(ipTmp);
                        }
                        flowEncapVal->dip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        flowEncapVal->dip.addr[0] = htonl(ipTmp);
                        flowEncapVal->dip.isIPv6 = FALSE;
                    }
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_SIP)
                {
                    /* 128 bit */
                    if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            flowEncapVal->sip.addr[i] = htonl(ipTmp);
                        }
                        flowEncapVal->sip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        flowEncapVal->sip.addr[0] = htonl(ipTmp);
                        flowEncapVal->sip.isIPv6 = FALSE;
                    }
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->l4Src = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->l4Dst = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TTL)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->ttl = teDataReg16[teDataReg16Index++];
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_NGE)
                {
                    /* 16 bit for the mask */
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->ngeMask = teDataReg16[teDataReg16Index++];

                    /* 32 bit times the number of NGE word present */
                    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE; i++)
                    {
                        if (flowEncapVal->ngeMask & (1 << i))
                        {
                            FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                           FM_ERR_BUFFER_FULL);
                            flowEncapVal->ngeData[i] = teDataReg16[teDataReg16Index++];
                            flowEncapVal->ngeData[i] |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        }
                    }
                }

                /* 32 bit */
                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL_PTR)
                {
                    FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowEncapVal->tunnelIdx.dataPtr = teDataReg16[teDataReg16Index++];
                    flowEncapVal->tunnelIdx.dataLength = teDataReg16[teDataReg16Index++] & 0x7f;
                }

                /* Is it the last block to process? */
                if ((teDataIndex + 1) < teDataLength)
                {
                    /* Tunnel block is the next one */
                    if ( (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL) &&
                         ((flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL_PTR) == 0) )
                    {
                        teData[teDataIndex + 1].blockType = FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA;
                    }
                    /* The first block was not a flow-key one which indicates
                     * that we are not dealing with a list but a single entry. */
                    else if (teData[0].blockType == FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP)
                    {
                        goto ABORT;
                    }
                    /* Next key in that hashing block */
                    else
                    {
                        teData[teDataIndex + 1].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;
                    }
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP:
                FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataFlowDecapVal structure */
                flowDecapVal = &teData[teDataIndex].blockVal.flowDecapVal;

                /* First 16 bit is the decap key mask */
                flowDecapVal->decapConfig = teDataReg16[teDataReg16Index] & ~FM_FM10000_TE_OUTER_HEADER_MAX;
                flowDecapVal->outerHeader = teDataReg16[teDataReg16Index++] & FM_FM10000_TE_OUTER_HEADER_MAX;

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DGLORT)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->dglort = teDataReg16[teDataReg16Index++];
                }

                /* 48 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DMAC)
                {
                    FM_API_REQUIRE((teDataReg16Index + 2) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->dmac = teDataReg16[teDataReg16Index++];
                    flowDecapVal->dmac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 16;
                    flowDecapVal->dmac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 32;
                }

                /* 48 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_SMAC)
                {
                    FM_API_REQUIRE((teDataReg16Index + 2) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->smac = teDataReg16[teDataReg16Index++];
                    flowDecapVal->smac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 16;
                    flowDecapVal->smac |= (fm_macaddr)(teDataReg16[teDataReg16Index++]) << 32;
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_VLAN)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->vlan = teDataReg16[teDataReg16Index++] & 0xfff;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DIP)
                {
                    /* 128 bit */
                    if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            flowDecapVal->dip.addr[i] = htonl(ipTmp);
                        }
                        flowDecapVal->dip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        flowDecapVal->dip.addr[0] = htonl(ipTmp);
                        flowDecapVal->dip.isIPv6 = FALSE;
                    }
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_SIP)
                {
                    /* 128 bit */
                    if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            flowDecapVal->sip.addr[i] = htonl(ipTmp);
                        }
                        flowDecapVal->sip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        flowDecapVal->sip.addr[0] = htonl(ipTmp);
                        flowDecapVal->sip.isIPv6 = FALSE;
                    }
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_TTL)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->ttl = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->l4Src = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->l4Dst = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_COUNTER)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    flowDecapVal->counterIdx = teDataReg16[teDataReg16Index++];
                }

                /* Is it the last block to process? */
                if ((teDataIndex + 1) < teDataLength)
                {
                    /* The first block was not a flow-key one which indicates
                     * that we are not dealing with a list but a single entry. */
                    if (teData[0].blockType == FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP)
                    {
                        goto ABORT;
                    }
                    /* Next key in that hashing block */
                    else
                    {
                        teData[teDataIndex + 1].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;
                    }
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA:
                FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                               FM_ERR_BUFFER_FULL);

                /* Value is a fm_fm10000TeDataTunnelVal structure */
                tunnelVal = &teData[teDataIndex].blockVal.tunnelVal;

                /* First 16 bit is the tunnel key mask */
                tunnelVal->tunnelConfig = teDataReg16[teDataReg16Index] & ~0x3;
                tunnelVal->tunnelType = teDataReg16[teDataReg16Index++] & 0x3;

                /* Destination IP is always present */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_IPV6)
                {
                    /* 128 bit */
                    FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    for (i = 0 ; i < 4 ; i++)
                    {
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        tunnelVal->dip.addr[i] = htonl(ipTmp);
                    }
                    tunnelVal->dip.isIPv6 = TRUE;
                }
                else
                {
                    /* 32 bit */
                    FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    ipTmp = teDataReg16[teDataReg16Index++];
                    ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                    tunnelVal->dip.addr[0] = htonl(ipTmp);
                    tunnelVal->dip.isIPv6 = FALSE;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_SIP)
                {
                    /* 128 bit */
                    if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_IPV6)
                    {
                        FM_API_REQUIRE((teDataReg16Index + 7) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        for (i = 0 ; i < 4 ; i++)
                        {
                            ipTmp = teDataReg16[teDataReg16Index++];
                            ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                            tunnelVal->sip.addr[i] = htonl(ipTmp);
                        }
                        tunnelVal->sip.isIPv6 = TRUE;
                    }
                    /* 32 bit */
                    else
                    {
                        FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                       FM_ERR_BUFFER_FULL);
                        ipTmp = teDataReg16[teDataReg16Index++];
                        ipTmp |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        tunnelVal->sip.addr[0] = htonl(ipTmp);
                        tunnelVal->sip.isIPv6 = FALSE;
                    }
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TOS)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    tunnelVal->tos = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TTL)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    tunnelVal->ttl = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4DST)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    tunnelVal->l4Dst = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4SRC)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    tunnelVal->l4Src = teDataReg16[teDataReg16Index++];
                }

                /* 16 bit */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
                {
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    tunnelVal->counterIdx = teDataReg16[teDataReg16Index++];
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_NGE)
                {
                    /* 16 bit for the mask */
                    FM_API_REQUIRE(teDataReg16Index < teDataReg16Length,
                                   FM_ERR_BUFFER_FULL);
                    tunnelVal->ngeMask = teDataReg16[teDataReg16Index++];

                    /* 32 bit times the number of NGE words defined */
                    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE; i++)
                    {
                        if (tunnelVal->ngeMask & (1 << i))
                        {
                            FM_API_REQUIRE((teDataReg16Index + 1) < teDataReg16Length,
                                           FM_ERR_BUFFER_FULL);
                            tunnelVal->ngeData[i] = teDataReg16[teDataReg16Index++];
                            tunnelVal->ngeData[i] |= (fm_uint32)(teDataReg16[teDataReg16Index++]) << 16;
                        }
                    }
                }

                /* Is it the last block to process? */
                if ((teDataIndex + 1) < teDataLength)
                {
                    /* The first block was not a flow-key one which indicates
                     * that we are not dealing with a list but a single entry. */
                    if ( (teData[0].blockType == FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP) ||
                         (teData[0].blockType == FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP) )
                    {
                        goto ABORT;
                    }
                    /* Next key in that hashing block */
                    else
                    {
                        teData[teDataIndex + 1].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;
                    }
                }
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                break;
        }
    }


ABORT:

    if (teDataIndex == teDataLength)
    {
        *teDataReturnLength = teDataIndex;
    }
    else
    {
        *teDataReturnLength = teDataIndex + 1;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeData */




/*****************************************************************************/
/** fm10000GetTeDataBlockLength
 * \ingroup intlowlevTe10k
 *
 * \desc            Retrieve the number of entries needed for the specified
 *                  block.
 *
 * \param[in]       teData points to one or multiple structure of type
 *                  ''fm_fm10000TeData'' containing the data information.
 * 
 * \param[in]       teDataLength is the number of element referenced by teData.
 *
 * \param[out]      blockLength is used to retrieve the number of entries
 *                  needed for that block.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetTeDataBlockLength(fm_fm10000TeData *teData,
                                      fm_int            teDataLength,
                                      fm_int           *blockLength)
{
    fm_int                        teDataIndex;
    fm_int                        i;
    fm_status                     err = FM_OK;
    fm_int                        teDataReg16Length = 0;
    fm_fm10000TeDataFlowKeyVal *  flowKeyVal;
    fm_fm10000TeDataFlowEncapVal *flowEncapVal;
    fm_fm10000TeDataFlowDecapVal *flowDecapVal;
    fm_fm10000TeDataTunnelVal    *tunnelVal;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "teData = %p, "
                  "teDataLength = %d, "
                  "blockLength = %p\n",
                  (void*)teData,
                  teDataLength,
                  (void*)blockLength);

    /* sanity check on the arguments */
    FM_API_REQUIRE(teData != NULL, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(teDataLength > 0, FM_ERR_INVALID_ARGUMENT);
    FM_API_REQUIRE(blockLength != NULL, FM_ERR_INVALID_ARGUMENT);

    for (teDataIndex = 0 ; teDataIndex < teDataLength ; teDataIndex++)
    {
        switch (teData[teDataIndex].blockType)
        {
            /* 32 bit wide */
            case FM_FM10000_TE_DATA_BLOCK_POINTER:
                teDataReg16Length += 2;
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_KEY:
                teDataReg16Length += 1;

                /* Value is a fm_fm10000TeDataFlowKeyVal structure */
                flowKeyVal = &teData[teDataIndex].blockVal.flowKeyVal;

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VSI_TEP)
                {
                    teDataReg16Length += 1;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VNI)
                {
                    teDataReg16Length += 2;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_DMAC)
                {
                    teDataReg16Length += 3;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_SMAC)
                {
                    teDataReg16Length += 3;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_VLAN)
                {
                    teDataReg16Length += 1;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_DIP)
                {
                    if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_SIP)
                {
                    if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_L4SRC)
                {
                    teDataReg16Length += 1;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_L4DST)
                {
                    teDataReg16Length += 1;
                }

                if (flowKeyVal->searchKeyConfig & FM10000_TE_KEY_PROT)
                {
                    teDataReg16Length += 1;
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP:
                teDataReg16Length += 1;

                /* Value is a fm_fm10000TeDataFlowEncapVal structure */
                flowEncapVal = &teData[teDataIndex].blockVal.flowEncapVal;

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_VNI)
                {
                    teDataReg16Length += 2;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_COUNTER)
                {
                    teDataReg16Length += 1;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_DMAC)
                {
                    teDataReg16Length += 3;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_SMAC)
                {
                    teDataReg16Length += 3;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_VLAN)
                {
                    teDataReg16Length += 1;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_DIP)
                {
                    if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_SIP)
                {
                    if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_L4SRC)
                {
                    teDataReg16Length += 1;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_L4DST)
                {
                    teDataReg16Length += 1;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TTL)
                {
                    teDataReg16Length += 1;
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_NGE)
                {
                    teDataReg16Length += 1;

                    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE; i++)
                    {
                        if (flowEncapVal->ngeMask & (1 << i))
                        {
                            teDataReg16Length += 2;
                        }
                    }
                }

                if (flowEncapVal->encapConfig & FM10000_TE_FLOW_ENCAP_TUNNEL_PTR)
                {
                    teDataReg16Length += 2;
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP:
                teDataReg16Length += 1;

                /* Value is a fm_fm10000TeDataFlowDecapVal structure */
                flowDecapVal = &teData[teDataIndex].blockVal.flowDecapVal;

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DGLORT)
                {
                    teDataReg16Length += 1;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DMAC)
                {
                    teDataReg16Length += 3;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_SMAC)
                {
                    teDataReg16Length += 3;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_VLAN)
                {
                    teDataReg16Length += 1;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_DIP)
                {
                    if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_SIP)
                {
                    if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_TTL)
                {
                    teDataReg16Length += 1;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_L4SRC)
                {
                    teDataReg16Length += 1;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_L4DST)
                {
                    teDataReg16Length += 1;
                }

                if (flowDecapVal->decapConfig & FM10000_TE_FLOW_DECAP_COUNTER)
                {
                    teDataReg16Length += 1;
                }
                break;

            /* Variable length */
            case FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA:
                teDataReg16Length += 1;

                /* Value is a fm_fm10000TeDataTunnelVal structure */
                tunnelVal = &teData[teDataIndex].blockVal.tunnelVal;

                /* Destination IP is always present */
                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_IPV6)
                {
                    teDataReg16Length += 8;
                }
                else
                {
                    teDataReg16Length += 2;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_SIP)
                {
                    if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_IPV6)
                    {
                        teDataReg16Length += 8;
                    }
                    else
                    {
                        teDataReg16Length += 2;
                    }
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TOS)
                {
                    teDataReg16Length += 1;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_TTL)
                {
                    teDataReg16Length += 1;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4DST)
                {
                    teDataReg16Length += 1;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_L4SRC)
                {
                    teDataReg16Length += 1;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_COUNTER)
                {
                    teDataReg16Length += 1;
                }

                if (tunnelVal->tunnelConfig & FM10000_TE_TUNNEL_ENCAP_NGE)
                {
                    teDataReg16Length += 1;

                    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE; i++)
                    {
                        if (tunnelVal->ngeMask & (1 << i))
                        {
                            teDataReg16Length += 2;
                        }
                    }
                }
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                break;
        }
    }

    /* Always Round Up to 128 bit boundary */
    teDataReg16Length += 7;

    *blockLength = (teDataReg16Length / (2 * FM10000_TE_DATA_WIDTH));


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_TE, err);

}   /* end fm10000GetTeDataBlockLength */




/*****************************************************************************/
/** fm10000DbgDumpTe
 * \ingroup intlowlevTe10k
 *
 * \desc            Dump the tunneling engine table.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       te is the tunneling engine on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_SWITCH_TYPE if sw does not support this API.
 *
 *****************************************************************************/
void fm10000DbgDumpTe(fm_int sw, fm_int te)
{
    fm_int     i;
    fm_int     j;
    fm_int     start;
    fm_int     length;
    char       addrBuffer[48];
    char       oldLine[BUFFER_SIZE];
    char       newLine[BUFFER_SIZE];
    fm_status  err = FM_OK;
    fm_fm10000TeParserCfg teParserCfg;
    fm_fm10000TeChecksumCfg teChecksumCfg;
    fm_fm10000TeTrapCfg teTrapCfg;
    fm_fm10000TeGlortCfg teGlortCfg;
    fm_fm10000TeTunnelCfg teTunnelCfg;
    fm_fm10000TeTepCfg teTepCfg;
    fm_fm10000TeSGlort teSGlort;
    fm_fm10000TeDGlort teDGlort;
    fm_fm10000TeLookup teLookup;
    fm_fm10000TeData   teData[FM10000_TE_MAX_DATA_BIN_SIZE];
    fm_uint32          teDataReg[FM10000_TE_DATA_WIDTH];
    fm_registerSGListEntry sgList;
    fm_uint16 dropCnt;
    fm_uint32 frameInCnt;
    fm_uint32 frameDoneCnt;
    fm_uint64 frameCnt;
    fm_uint64 byteCnt;
    fm_bool   used;
    fm_uint32 lastDglortUser;
    fm_uint16 rangePos;
    fm_int    teDataReturnLength;

    FM_LOG_ENTRY( FM_LOG_CAT_TE,
                  "sw = %d, "
                  "te = %d\n",
                  sw,
                  te);

    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    if (!fmSupportsTe(sw))
    {
        err = FM_ERR_INVALID_SWITCH_TYPE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
    }

    FM_LOG_PRINT("===============================================================================\n");
    FM_LOG_PRINT("Tunneling Engine %d\n", te);
    FM_LOG_PRINT("===============================================================================\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> Parser Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    err = fm10000GetTeParser(sw, te, &teParserCfg, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    FM_LOG_PRINT("VxLanPort:0x%04x  NgePort:0x%04x\n",
                 teParserCfg.vxLanPort, teParserCfg.ngePort);
    FM_LOG_PRINT("Extra EtherType:0x%04x  Size:%d  AfterVlan?:%d\n",
                 teParserCfg.etherType, teParserCfg.tagSize, teParserCfg.afterVlan);
    FM_LOG_PRINT("Validate Protocol:%d  Version:%d  NgeOam:%d  NgeC:%d\n",
                 teParserCfg.checkProtocol, teParserCfg.checkVersion, 
                 teParserCfg.checkNgeOam, teParserCfg.checkNgeC);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> Checksum Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    err = fm10000GetTeChecksum(sw, te, &teChecksumCfg, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    FM_LOG_PRINT("Non IP Frame: ");
    PrintChecksum(teChecksumCfg.notIp);

    FM_LOG_PRINT("IP Frame that are not TCP or UDP: ");
    PrintChecksum(teChecksumCfg.notTcpOrUdp);

    FM_LOG_PRINT("IP Frame that are TCP or UDP: ");
    PrintChecksum(teChecksumCfg.tcpOrUdp);

    FM_LOG_PRINT("Validation of outer checksum for VXLAN and NGE:%d\n",
                 teChecksumCfg.verifDecapChecksum);

    FM_LOG_PRINT("Update of outer checksum for VXLAN and NGE:%d\n",
                 teChecksumCfg.updateDecapChecksum);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> Trap Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    err = fm10000GetTeTrap(sw, te, &teTrapCfg, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    FM_LOG_PRINT("Trap DGLORT:0x%04x\n", teTrapCfg.trapGlort);

    FM_LOG_PRINT("Normal: ");
    PrintTrap(teTrapCfg.normal);

    FM_LOG_PRINT("No DGLORT Match: ");
    PrintTrap(teTrapCfg.noDglortMatch);

    FM_LOG_PRINT("No SGLORT Match: ");
    PrintTrap(teTrapCfg.noSglortMatch);

    FM_LOG_PRINT("Decap No Outer L3: ");
    PrintTrap(teTrapCfg.decapNoOuterL3);

    FM_LOG_PRINT("Decap No Outer L4: ");
    PrintTrap(teTrapCfg.decapNoOuterL4);

    FM_LOG_PRINT("Decap No Outer Tunnel: ");
    PrintTrap(teTrapCfg.decapNoOuterTun);

    FM_LOG_PRINT("Decap Bad Checksum: ");
    PrintTrap(teTrapCfg.decapBadChecksum);

    FM_LOG_PRINT("Decap Bad Tunnel: ");
    PrintTrap(teTrapCfg.decapBadTunnel);

    FM_LOG_PRINT("Encap No L3: ");
    PrintTrap(teTrapCfg.encapNoL3);

    FM_LOG_PRINT("Encap No L4: ");
    PrintTrap(teTrapCfg.encapNoL4);

    FM_LOG_PRINT("Encap Any L4: ");
    PrintTrap(teTrapCfg.encapAnyL4);

    FM_LOG_PRINT("Truncated Header: ");
    PrintTrap(teTrapCfg.truncatedHeader);

    FM_LOG_PRINT("Truncated IP Payload: ");
    PrintTrap(teTrapCfg.truncIpPayload);

    FM_LOG_PRINT("No Flow Match: ");
    PrintTrap(teTrapCfg.noFlowMatch);

    FM_LOG_PRINT("Missing Record: ");
    PrintTrap(teTrapCfg.missingRecord);

    FM_LOG_PRINT("ECC Error: ");
    PrintTrap(teTrapCfg.ucErr);

    FM_LOG_PRINT("Lookup out of Bounds: ");
    PrintTrap(teTrapCfg.lookupBounds);

    FM_LOG_PRINT("Flow Table out of Bounds: ");
    PrintTrap(teTrapCfg.dataBounds);

    FM_LOG_PRINT("Header Limit Reached: ");
    PrintTrap(teTrapCfg.headerLimit);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> Default GLORT Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    err = fm10000GetTeDefaultGlort(sw, te, &teGlortCfg, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    FM_LOG_PRINT("Encap DGLORT:0x%04x  Decap DGLORT:0x%04x\n",
                 teGlortCfg.encapDglort, teGlortCfg.decapDglort);

    FM_LOG_PRINT("Encap SGLORT:0x%04x  Decap SGLORT:0x%04x\n",
                 teGlortCfg.encapSglort, teGlortCfg.decapSglort);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> Default Tunnel Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    err = fm10000GetTeDefaultTunnel(sw, te, &teTunnelCfg, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    FM_LOG_PRINT("L4 Destination Port:%d(VxLan) %d(NGE)  TTL:%d  TOS:%d  DeriveOuterTOS?:%d\n",
                 teTunnelCfg.l4DstVxLan, teTunnelCfg.l4DstNge, teTunnelCfg.ttl,
                 teTunnelCfg.tos, teTunnelCfg.deriveOuterTOS);

    FM_LOG_PRINT("DMAC:0x%012llx  SMAC:0x%012llx\n",
                 teTunnelCfg.dmac, teTunnelCfg.smac);

    FM_LOG_PRINT("NVGRE Protocol:0x%04x  NVGRE/NGE Version:0x%x\n",
                 teTunnelCfg.encapProtocol, teTunnelCfg.encapVersion);

    FM_LOG_PRINT("Tunnel Mode: %d (%s)\n",
                 teTunnelCfg.mode,
                 teTunnelCfg.mode == FM10000_TE_MODE_VXLAN_NVGRE_NGE ? 
                 "VXLAN_NVGRE_NGE" : "VXLAN_GPE_NSH");

    FM_LOG_PRINT("NGE Time?:%d  NGE Mask:0x%04x\n",
                 teTunnelCfg.ngeTime, teTunnelCfg.ngeMask);

    FM_LOG_PRINT("NGE Data:");

    for (i = 0 ; i < FM10000_TE_NGE_DATA_SIZE ; i++)
    {
        FM_LOG_PRINT(" [%d]=0x%08x", i, teTunnelCfg.ngeData[i]);

        if ((i % 5) == 0)
        {
            FM_LOG_PRINT("\n");
        }
    }

    FM_LOG_PRINT("NSH Length: %d  NSH MD Type: %d\n", 
                 teTunnelCfg.nshLength, teTunnelCfg.nshMdType);

    FM_LOG_PRINT("NSH Service Index: %d  NSH Service Path ID: %d\n",
                 teTunnelCfg.nshSvcIndex, teTunnelCfg.nshSvcPathId);

    FM_LOG_PRINT("NSH Critical TLVs?: %d (%s)\n",
                 teTunnelCfg.nshCritical, teTunnelCfg.nshCritical ? "True" : "False");

    FM_LOG_PRINT("NSH Data:");

    for (i = 0 ; i < FM10000_TE_NSH_DATA_SIZE ; i++)
    {
        FM_LOG_PRINT(" [%d]=0x%08x", i, teTunnelCfg.nshData[i]);

        if ((i % 5) == 0)
        {
            FM_LOG_PRINT("\n");
        }
    }

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> Default TEP Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT(" Tep     IP Address                                 VNI\n"
                 "-------  --------------------------------------- --------\n");
    for (i = 0 ; i < FM10000_TE_SIP_ENTRIES_0; i++)
    {
        err = fm10000GetTeDefaultTep(sw, te, i, &teTepCfg, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        teTepCfg.srcIpAddr.isIPv6 = TRUE;
        fmDbgConvertIPAddressToString(&(teTepCfg.srcIpAddr), addrBuffer);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                      "%-39s 0x%06x",
                      addrBuffer,
                      teTepCfg.vni);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, FALSE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> SGLORT Decode Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT(" Slot    GLORT/Mask   VSI Start   VSI Length   VSI Offset\n"
                 "-------  -----------      --          --            --\n");
    for (i = 0 ; i < FM10000_TE_SGLORT_MAP_ENTRIES_0; i++)
    {
        err = fm10000GetTeSGlort(sw, te, i, &teSGlort, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                      "0x%04x/%04x      %2u          %2u           %3u",
                      teSGlort.glortValue,
                      teSGlort.glortMask,
                      teSGlort.vsiStart,
                      teSGlort.vsiLength,
                      teSGlort.vsiOffset);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, FALSE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> DGLORT Decode Settings >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT(" Slot    GLORT/Mask  USER/Mask Base  Encap SetSGLORT SetDGLORT Lookup\n"
                 "-------  -----------  -------  -----   -       -          -    ------\n");
    for (i = 0 ; i < FM10000_TE_DGLORT_DEC_ENTRIES_0; i++)
    {
        err = fm10000GetTeDGlort(sw, te, i, &teDGlort, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_SNPRINTF_S(newLine, sizeof(newLine),
                      "0x%04x/%04x  0x%02x/%02x  %5d   %d       %d          %d    %s\n",
                      teDGlort.glortValue,
                      teDGlort.glortMask,
                      teDGlort.userValue,
                      teDGlort.userMask,
                      teDGlort.baseLookup,
                      teDGlort.encap,
                      teDGlort.setSGlort,
                      teDGlort.setDGlort,
                      (teDGlort.lookupType == FM_FM10000_TE_LOOKUP_DIRECT) ? "Direct" : "Hash");

        length = FM_STRNLEN_S(newLine, sizeof(newLine));

        if (teDGlort.lookupType == FM_FM10000_TE_LOOKUP_DIRECT)
        {
            FM_SNPRINTF_S(newLine + length, sizeof(newLine) - length,
                          "-------  -----------  -------  -----   -       -          -    ------\n"
                          "         Index Start   Index Width\n"
                          "             --            --\n"
                          "             %2d            %2d\n"
                          "             --            --\n",
                          teDGlort.lookupData.directLookup.indexStart,
                          teDGlort.lookupData.directLookup.indexWidth);
        }
        else
        {
            FM_SNPRINTF_S(newLine + length, sizeof(newLine) - length,
                          "-------  -----------  -------  -----   -       -          -    ------\n"
                          "         Hash Key   Hash Size   TEP Start   TEP Width\n"
                          "          ------      -----        --           --\n"
                          "          0x%04x      %5d        %2d           %2d\n"
                          "          ------      -----        --           --\n",
                          teDGlort.lookupData.hashLookup.hashKeyConfig,
                          teDGlort.lookupData.hashLookup.hashSize,
                          teDGlort.lookupData.hashLookup.tepStart,
                          teDGlort.lookupData.hashLookup.tepWidth);
        }

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, FALSE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Lookup Table >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT("   Slot      TE Data Index   Length (x 128b)   Last\n"
                 "-----------      ------         ---              - \n");
    for (i = 0 ; i < FM10000_TE_LOOKUP_ENTRIES_0; i++)
    {
        err = fm10000GetTeLookup(sw, te, i, &teLookup, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                      "    0x%04x         %3d              %d",
                      teLookup.dataPtr,
                      teLookup.dataLength,
                      teLookup.last);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, TRUE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, TRUE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Data Raw Table >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT("   Slot         Data[3]   Data[2]   Data[1]   Data[0]\n"
                 "-----------    --------  --------  --------  --------\n");
    for (i = 0 ; i < FM10000_TE_DATA_ENTRIES_0; i++)
    {
        FM_REGS_CACHE_FILL_SGLIST(&sgList,
                                  &fm10000CacheTeData,
                                  1,
                                  i,
                                  te,
                                  FM_REGS_CACHE_INDEX_UNUSED,
                                  teDataReg,
                                  FALSE);

        err = fmRegCacheRead(sw, 1, &sgList, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                      "0x%08x  %08x  %08x  %08x",
                      teDataReg[3],
                      teDataReg[2],
                      teDataReg[1],
                      teDataReg[0]);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, TRUE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, TRUE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Sync Statistic >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    FM_LOG_PRINT("syncRetry:%lld syncCall:%lld maxSyncRetry:%lld averageRetry:%.5f\n",
                 syncRetry[te], syncCall[te], maxSyncRetry[te],
                 syncCall[te] ? (float)syncRetry[te]/(float)syncCall[te] : 0);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Global Statistic >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    err = fm10000GetTeCnt(sw, te, &dropCnt, &frameInCnt, &frameDoneCnt);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

    FM_LOG_PRINT("DropCount:%d   InCount:%d   DoneCount:%d\n",
                 dropCnt, frameInCnt, frameDoneCnt);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Flow Statistic >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT("   Slot        Frame Count         Byte Count\n"
                 "-----------  ----------------  ------------------\n");
    for (i = 0 ; i < FM10000_TE_STATS_ENTRIES_0; i++)
    {
        err = fm10000GetTeFlowCnt(sw, te, i, &frameCnt, &byteCnt);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                      "%16lld  %18lld",
                      frameCnt,
                      byteCnt);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, TRUE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, TRUE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Flow Used Statistic >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    start = 0;
    oldLine[0] = 0;

    FM_LOG_PRINT("   Slot      Used\n"
                 "-----------   --\n");
    for (i = 0 ; i < FM10000_TE_DATA_ENTRIES_0; i++)
    {
        err = fm10000GetTeFlowUsed(sw, te, i, &used);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE,
                      " %d",
                      used);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintLine(start, i - 1, oldLine, TRUE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintLine(start, i - 1, oldLine, TRUE);

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

    FM_LOG_PRINT(">>>>>>>>>>>>>>>>>>>>>>>>>>> TE Table Decoding >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n");

    for (i = 0 ; i < FM10000_TE_DGLORT_DEC_ENTRIES_0 ; i++)
    {
        err = fm10000GetTeDGlort(sw, te, i, &teDGlort, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

        /* Invalid entry */
        if ( (teDGlort.glortValue & teDGlort.glortMask) != teDGlort.glortValue)
        {
            continue;
        }

        FM_LOG_PRINT("===============================================================================\n");
        FM_LOG_PRINT("Decoding TeDGlort slot %d that matches on DGLORT 0x%04x/%04x USER 0x%02x/%02x\n",
                     i, teDGlort.glortValue, teDGlort.glortMask,
                     teDGlort.userValue, teDGlort.userMask);
        if (teDGlort.lookupType == FM_FM10000_TE_LOOKUP_DIRECT)
        {
            FM_LOG_PRINT("FM_FM10000_TE_LOOKUP_DIRECT Type With BaseLookup %d and size %d\n",
                         teDGlort.baseLookup, (1 << teDGlort.lookupData.directLookup.indexWidth));
            FM_LOG_PRINT("Base DGLORT/USER: 0x%04x/0x%02x\n",
                         teDGlort.glortValue, teDGlort.userValue);

            lastDglortUser = ((teDGlort.glortValue << 8) | teDGlort.userValue);
            rangePos = 0;
            for (j = teDGlort.lookupData.directLookup.indexStart ; j < 24 ; j++)
            {
                if (rangePos < teDGlort.lookupData.directLookup.indexWidth)
                {
                    lastDglortUser |= (1 << j);
                    rangePos++;
                }
            }
            FM_LOG_PRINT("Last DGLORT/USER: 0x%04x/0x%02x\n", lastDglortUser >> 8, (lastDglortUser & 0xff));
            FM_LOG_PRINT("%s Flow With Outgoing SGLORT set to %s and Outgoing DGLORT set to %s\n",
                         (teDGlort.encap) ? "Encoding" : "Decoding",
                         (teDGlort.setSGlort) ? "Default" : "Passthrough",
                         (teDGlort.setDGlort) ? "Default" : "Passthrough");
            FM_LOG_PRINT("===============================================================================\n\n");

            lastDglortUser = ((teDGlort.glortValue << 8) | teDGlort.userValue);
            for (j = teDGlort.baseLookup ; 
                 j < (teDGlort.baseLookup + (1 << teDGlort.lookupData.directLookup.indexWidth)) ; 
                 j++)
            {
                err = fm10000GetTeLookup(sw, te, j, &teLookup, FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                if (teLookup.dataLength)
                {
                    if (teLookup.last == 0)
                    {
                        teData[0].blockType = FM_FM10000_TE_DATA_BLOCK_POINTER;
                    }
                    else if (teDGlort.encap)
                    {
                        teData[0].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP;
                    }
                    else
                    {
                        teData[0].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP;
                    }
                    
                    err = fm10000GetTeData(sw,
                                           te,
                                           teLookup.dataPtr,
                                           teLookup.dataLength,
                                           teDGlort.encap,
                                           teData,
                                           FM10000_TE_MAX_DATA_BIN_SIZE,
                                           &teDataReturnLength,
                                           FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                    FM_LOG_PRINT("Index %d, DGLORT/USER: 0x%04x/0x%02x --> TE_Data Index %d\n",
                                 j, lastDglortUser >> 8, (lastDglortUser & 0xff),
                                 teLookup.dataPtr);

                    err = PrintTeDataBlock(sw,
                                           te,
                                           FALSE,
                                           teDGlort.encap,
                                           teData,
                                           teDataReturnLength,
                                           0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);
                }
                lastDglortUser += (1 << teDGlort.lookupData.directLookup.indexStart);
            }
        }
        else
        {
            FM_LOG_PRINT("FM_FM10000_TE_LOOKUP_HASH Type With BaseLookup %d and size %d\n",
                         teDGlort.baseLookup, teDGlort.lookupData.hashLookup.hashSize);
            FM_LOG_PRINT("Base DGLORT/USER: 0x%04x/0x%02x\n",
                         teDGlort.glortValue, teDGlort.userValue);

            lastDglortUser = 0;
            rangePos = 0;
            for (j = teDGlort.lookupData.hashLookup.tepStart ; j < 24 ; j++)
            {
                if (rangePos < teDGlort.lookupData.hashLookup.tepWidth)
                {
                    lastDglortUser |= (1 << j);
                    rangePos++;
                }
            }
            FM_LOG_PRINT("TEP Position in DGLORT/USER: 0x%04x/0x%02x\n", lastDglortUser >> 8, (lastDglortUser & 0xff));
            FM_LOG_PRINT("%s Flow With Outgoing SGLORT set to %s and Outgoing DGLORT set to %s\n",
                         (teDGlort.encap) ? "Encoding" : "Decoding",
                         (teDGlort.setSGlort) ? "Default" : "Passthrough",
                         (teDGlort.setDGlort) ? "Default" : "Passthrough");
            FM_LOG_PRINT("===============================================================================\n\n");

            for (j = teDGlort.baseLookup; 
                 j < (teDGlort.baseLookup + teDGlort.lookupData.hashLookup.hashSize); 
                 j++)
            {
                err = fm10000GetTeLookup(sw, te, j, &teLookup, FALSE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                if (teLookup.dataLength)
                {
                    if (teLookup.last == 0)
                    {
                        teData[0].blockType = FM_FM10000_TE_DATA_BLOCK_POINTER;
                        teData[1].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;
                    }
                    else
                    {
                        teData[0].blockType = FM_FM10000_TE_DATA_BLOCK_FLOW_KEY;
                    }

                    err = fm10000GetTeData(sw,
                                           te,
                                           teLookup.dataPtr,
                                           teLookup.dataLength,
                                           teDGlort.encap,
                                           teData,
                                           FM10000_TE_MAX_DATA_BIN_SIZE,
                                           &teDataReturnLength,
                                           FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                    FM_LOG_PRINT("Bin: %05d --> TE_Data Index %d\n",
                                 j - teDGlort.baseLookup, teLookup.dataPtr);
                    FM_LOG_PRINT("-------------------------------------------------------------------------------\n");

                    err = PrintTeDataBlock(sw,
                                           te,
                                           TRUE,
                                           teDGlort.encap,
                                           teData,
                                           teDataReturnLength,
                                           0);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_TE, err);

                    FM_LOG_PRINT("-------------------------------------------------------------------------------\n");
                }
            }
        }
        
    }

    FM_LOG_PRINT("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_VOID(FM_LOG_CAT_TE);

}   /* end fm10000DbgDumpTe */
