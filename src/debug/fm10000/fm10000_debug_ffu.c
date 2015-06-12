/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_debug_ffu.c
 * Creation Date:  May 8, 2013
 * Description:    Provide debugging functions for inspecting the FFU.
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#define READ_WORDS(addr, dest)                                          \
    if ( switchPtr->ReadUINT32Mult( sw, (addr),                         \
                                    sizeof(dest) / sizeof( (dest)[0] ), \
                                    (dest) ) != FM_OK )                 \
    {                                                                   \
        return;                                                         \
    }

#define READ_WORD(addr, dest)                                    \
    if ( FM_OK != switchPtr->ReadUINT32( sw, (addr), &(dest) ) ) \
    {                                                            \
        return;                                                  \
    }

#define BUFFER_SIZE  1024

typedef struct _fm_ffuDescription
{
    fm_uint     n;
    const char *s;
    fm_uint     size;

} fm_ffuDescription;

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

static const fm_ffuDescription fmFfuTable[] =
{
    { 0,   "FM_FFU_MUX_SELECT_MAP_DIP_MAP_SIP",     8                 },
    { 1,   "FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC",   8                 },
    { 2,   "FM_FFU_MUX_SELECT_MAP_PROT_MAP_LENGTH", 8                 },
    { 3,   "FM_FFU_MUX_SELECT_MAP_SRC_MAP_TYPE",    8                 },
    { 4,   "FM_FFU_MUX_SELECT_USER",                8                 },
    { 5,   "FM_FFU_MUX_SELECT_ISLCMD_SYSPRI",       8                 },
    { 6,   "FM_FFU_MUX_SELECT_MISC",                8                 },
    { 7,   "FM_FFU_MUX_SELECT_TOS",                 8                 },
    { 8,   "FM_FFU_MUX_SELECT_PROT",                8                 },
    { 9,   "FM_FFU_MUX_SELECT_TTL",                 8                 },
    {10,   "FM_FFU_MUX_SELECT_SRC_PORT",            8                 },
    {11,   "FM_FFU_MUX_SELECT_VPRI_VID_11_8",       8                 },
    {12,   "FM_FFU_MUX_SELECT_VID_7_0",             8                 },
    {13,   "FM_FFU_MUX_SELECT_RXTAG",               8                 },
    {14,   "FM_FFU_MUX_SELECT_DMAC_15_0",          16                 },
    {15,   "FM_FFU_MUX_SELECT_DMAC_31_16",         16                 },
    {16,   "FM_FFU_MUX_SELECT_DMAC_47_32",         16                 },
    {17,   "FM_FFU_MUX_SELECT_SMAC_15_0",          16                 },
    {18,   "FM_FFU_MUX_SELECT_SMAC_31_16",         16                 },
    {19,   "FM_FFU_MUX_SELECT_SMAC_47_32",         16                 },
    {20,   "FM_FFU_MUX_SELECT_DGLORT",             16                 },
    {21,   "FM_FFU_MUX_SELECT_SGLORT",             16                 },
    {22,   "FM_FFU_MUX_SELECT_L2_VPRI1_L2_VID1",   16                 },
    {23,   "FM_FFU_MUX_SELECT_L2_VPRI2_L2_VID2",   16                 },
    {24,   "FM_FFU_MUX_SELECT_ETHER_TYPE",         16                 },
    {25,   "FM_FFU_MUX_SELECT_L4DST",              16                 },
    {26,   "FM_FFU_MUX_SELECT_L4SRC",              16                 },
    {27,   "FM_FFU_MUX_SELECT_MAP_L4DST",          16                 },
    {28,   "FM_FFU_MUX_SELECT_MAP_L4SRC",          16                 },
    {29,   "FM_FFU_MUX_SELECT_L4A",                16                 },
    {30,   "FM_FFU_MUX_SELECT_L4B",                16                 },
    {31,   "FM_FFU_MUX_SELECT_L4C",                16                 },
    {32,   "FM_FFU_MUX_SELECT_L4D",                16                 },
    {33,   "FM_FFU_MUX_SELECT_MAP_VLAN_VPRI",      16                 },
    {34,   "FM_FFU_MUX_SELECT_DIP_31_0",           32                 },
    {35,   "FM_FFU_MUX_SELECT_DIP_63_32",          32                 },
    {36,   "FM_FFU_MUX_SELECT_DIP_95_64",          32                 },
    {37,   "FM_FFU_MUX_SELECT_DIP_127_96",         32                 },
    {38,   "FM_FFU_MUX_SELECT_SIP_31_0",           32                 },
    {39,   "FM_FFU_MUX_SELECT_SIP_63_32",          32                 },
    {40,   "FM_FFU_MUX_SELECT_SIP_95_64",          32                 },
    {41,   "FM_FFU_MUX_SELECT_SIP_127_96",         32                 },
};


static const char *const fmSelectName[] =
{
    "Select0:  ",
    "Select1:  ",
    "Select2:  ",
    "Select3:  ",
    "SelectTop:",
};


static const char *const fmCommandName[] =
{
    "(RouteArp)  ",
    "(RouteGlort)",
    "(BitSet)    ",
    "(FieldSet)  ",
};

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static int Compare(const void *aPtr, const void *bPtr)
{
    const fm_ffuDescription *a = (const fm_ffuDescription *) aPtr;
    const fm_ffuDescription *b = (const fm_ffuDescription *) bPtr;

    if (a->n < b->n)
    {
        return -1;
    }
    else if (a->n > b->n)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}   /* end Compare */




static void PrintSelect(fm_uint sel, fm_uint value)
{
    const fm_ffuDescription *desc;
    fm_ffuDescription        key;
    const char *             s;

    key.n = value;
    key.s = NULL;               /* not actually used but makes tool happier */
    key.size = 0;

    desc = (const fm_ffuDescription *)
           bsearch(&key, fmFfuTable,
                   sizeof(fmFfuTable) / sizeof(fmFfuTable[0]),
                   sizeof(fmFfuTable[0]), Compare);

    if (desc != NULL)
    {
        if ( (sel == 4) && (desc->size != 8))
        {
            s = "FM_FFU_MUX_SELECT_INVALID_KEY_TOP";
        }
        else
        {
            s = desc->s;
        }
    }
    else
    {
        s = "FM_FFU_MUX_SELECT_INVALID";
    }

    FM_LOG_PRINT("%s (%u) %s\n", fmSelectName[sel], value, s);

}   /* end PrintSelect */




static void PrintSliceHeading(void)
{
    char buf[BUFFER_SIZE];

    FM_SNPRINTF_S(buf, BUFFER_SIZE, "%s",
                  "   Line   Valid Mask            Value          Prec Bank Index Action\n"
                  "--------- ----- --------------  --------------  --- ---- ----  -----------");

    FM_LOG_PRINT("%s\n", buf);

}   /* end PrintSliceHeading */




static void FormatSliceLine(char *dest,
                            fm_bool validRule,
                            fm_fm10000FfuSliceKey *ruleKey,
                            fm_ffuAction *action,
                            fm_byte caseLocation)
{
    fm_int offset;

    offset = FM_SNPRINTF_S(dest, BUFFER_SIZE,
                           "  %d   "                              /* Case */
                           "%02llx %02llx %02llx %02llx %02llx  " /* Mask */
                           "%02llx %02llx %02llx %02llx %02llx "  /* Value */
                           " %2d  "                               /* Prec */
                           " %2d "                                /* Bank */
                           " %4d  ",                              /* Index */
                           validRule,
                           (ruleKey->keyMask >> 32 & 0xffLL),
                           (ruleKey->keyMask >> 24 & 0xffLL),
                           (ruleKey->keyMask >> 16 & 0xffLL),
                           (ruleKey->keyMask >>  8 & 0xffLL),
                           (ruleKey->keyMask >>  0 & 0xffLL),
                           (ruleKey->key >> 32 & 0xffLL),
                           (ruleKey->key >> 24 & 0xffLL),
                           (ruleKey->key >> 16 & 0xffLL),
                           (ruleKey->key >>  8 & 0xffLL),
                           (ruleKey->key >>  0 & 0xffLL),
                           action->precedence,
                           action->bank,
                           action->counter
                           );

    switch (action->action)
    {
        case FM_FFU_ACTION_NOP:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "%s",
                                    "NOP"
                                    );
            break;

        case FM_FFU_ACTION_ROUTE_ARP:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "RouteArp Type=%d Index=%d Cnt=%d",
                                    action->data.arp.arpType,
                                    action->data.arp.arpIndex,
                                    action->data.arp.count
                                    );
            break;

        case FM_FFU_ACTION_ROUTE_LOGICAL_PORT:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "RoutePort Port=%d",
                                    action->data.logicalPort
                                    );
            break;

        case FM_FFU_ACTION_ROUTE_FLOOD_DEST:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "FloodPort Port=%d",
                                    action->data.logicalPort
                                    );
            break;

        case FM_FFU_ACTION_ROUTE_GLORT:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "RouteGlort glort=0x%x",
                                    action->data.glort
                                    );
            break;

        case FM_FFU_ACTION_SET_FLAGS:
            if ( (action->data.flags.drop        == FM_FFU_FLAG_SET) ||
                 (action->data.flags.trap        == FM_FFU_FLAG_SET) ||
                 (action->data.flags.log         == FM_FFU_FLAG_SET) ||
                 (action->data.flags.noRoute     == FM_FFU_FLAG_SET) ||
                 (action->data.flags.rxMirror    == FM_FFU_FLAG_SET) ||
                 (action->data.flags.captureTime == FM_FFU_FLAG_SET) )
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        "%s",
                                        "SetFlags "
                                        );
                if (action->data.flags.drop == FM_FFU_FLAG_SET)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            "DROP "
                                            );
                }
                if (action->data.flags.trap == FM_FFU_FLAG_SET)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            "TRAP "
                                            );
                }
                if (action->data.flags.log == FM_FFU_FLAG_SET)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            "LOG "
                                            );
                }
                if (action->data.flags.noRoute == FM_FFU_FLAG_SET)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            "NO_ROUTE "
                                            );
                }
                if (action->data.flags.rxMirror == FM_FFU_FLAG_SET)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            "RX_MIRROR "
                                            );
                }
                if (action->data.flags.captureTime == FM_FFU_FLAG_SET)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            "CAP_TIME "
                                            );
                }

            }

            if ( (action->data.flags.drop        == FM_FFU_FLAG_CLEAR) ||
                 (action->data.flags.trap        == FM_FFU_FLAG_CLEAR) ||
                 (action->data.flags.log         == FM_FFU_FLAG_CLEAR) ||
                 (action->data.flags.noRoute     == FM_FFU_FLAG_CLEAR) ||
                 (action->data.flags.rxMirror    == FM_FFU_FLAG_CLEAR) ||
                 (action->data.flags.captureTime == FM_FFU_FLAG_CLEAR) )
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        "%s",
                                        "ClearFlags"
                                        );
                if (action->data.flags.drop == FM_FFU_FLAG_CLEAR)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " DROP"
                                            );
                }
                if (action->data.flags.trap == FM_FFU_FLAG_CLEAR)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " TRAP"
                                            );
                }
                if (action->data.flags.log == FM_FFU_FLAG_CLEAR)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " LOG"
                                            );
                }
                if (action->data.flags.noRoute == FM_FFU_FLAG_CLEAR)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " NO_ROUTE"
                                            );
                }
                if (action->data.flags.rxMirror == FM_FFU_FLAG_CLEAR)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " RX_MIRROR"
                                            );
                }
                if (action->data.flags.captureTime == FM_FFU_FLAG_CLEAR)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " CAP_TIME"
                                            );
                }

            }
            break;

        case FM_FFU_ACTION_SET_TRIGGER:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "SetTrig Data=0x%x/0x%x",
                                    action->data.trigger.value,
                                    action->data.trigger.mask
                                    );
            break;

        case FM_FFU_ACTION_SET_USER:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "SetUser Data=0x%x/0x%x",
                                    action->data.user.value,
                                    action->data.user.mask
                                    );
            break;

        case FM_FFU_ACTION_SET_FIELDS:
            if (action->data.fields.fieldType == FM_FFU_FIELD_NEITHER)
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        "%s",
                                        "SetField None"
                                        );
            }
            else if (action->data.fields.fieldType == FM_FFU_FIELD_DSCP)
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        "SetDSCP %d",
                                        action->data.fields.fieldValue
                                        );
            }
            else if (action->data.fields.fieldType == FM_FFU_FIELD_VLAN)
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        "SetVlan %d",
                                        action->data.fields.fieldValue
                                        );

                if (action->data.fields.txTag == FM_FFU_TXTAG_NORMAL)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " With TxTag Normal"
                                            );
                }
                else if (action->data.fields.txTag == FM_FFU_TXTAG_ADD_TAG)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " With TxTag Add"
                                            );
                }
                else if (action->data.fields.txTag == FM_FFU_TXTAG_DEL_TAG)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " With TxTag Del"
                                            );
                }
                else if (action->data.fields.txTag == FM_FFU_TXTAG_UPD_OR_ADD_TAG)
                {
                    offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                            "%s",
                                            " With TxTag Upd"
                                            );
                }
            }
            if (action->data.fields.setPri)
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        " And SetISLPRI %d",
                                        action->data.fields.priority
                                        );
            }

            if (action->data.fields.setVpri)
            {
                offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                        " And SetVPRI %d",
                                        action->data.fields.priority
                                        );
            }

            break;

        default:
            offset += FM_SNPRINTF_S(dest + offset, BUFFER_SIZE - offset,
                                    "%s",
                                    "Invalid"
                                    );
            break;
        }

}   /* end FormatSliceLine */




static void FormatPolicerLine(char *dest,
                              fm_ffuPolicerState *committed,
                              fm_ffuPolicerState *excess)
{
    fm_uint64 sweeperFreq = 2500; // 2.5KHz
    fm_uint64 committedRate;
    fm_uint32 committedCap;
    fm_uint64 excessRate;
    fm_uint32 excessCap;

    committedRate = ((fm_uint64)(committed->rateMantissa << committed->rateExponent) * (sweeperFreq * 8LL)) / 1000LL;
    committedCap  = (committed->capacityMantissa << committed->capacityExponent);

    excessRate = ((fm_uint64)(excess->rateMantissa << excess->rateExponent) * (sweeperFreq * 8LL)) / 1000LL;
    excessCap  = (excess->capacityMantissa << excess->capacityExponent);

    FM_SNPRINTF_S(dest, BUFFER_SIZE,
                  "|%010llu kbps + %08u B + %s |%010llu kbps + %08u B + %s |",
                  committedRate,
                  committedCap,
                  (committed->action == FM_FFU_POLICER_ACTION_DROP)? "Drop":"MkDn",
                  excessRate,
                  excessCap,
                  (excess->action == FM_FFU_POLICER_ACTION_DROP)? "Drop":"MkDn");

}   /* end FormatPolicerLine */




static void FormatCounterLine(char *    dest,
                              fm_uint64 frameCount,
                              fm_uint64 byteCount)
{
    FM_SNPRINTF_S(dest, BUFFER_SIZE,
                  "| %020llu   |   %020llu |",
                  frameCount,
                  byteCount);

}   /* end FormatCounterLine */




static void PrintSliceLine(fm_uint start, fm_uint end, const char *line)
{
    FM_NOT_USED(line);

    if (start == end)
    {
        FM_LOG_PRINT("     %04u %s\n", start, line);
    }
    else
    {
        FM_LOG_PRINT("%04u-%04u %s\n", start, end, line);
    }

}   /* end PrintSliceLine */




static void PrintShortLine(fm_uint start, fm_uint end, const char *line)
{
    FM_NOT_USED(line);

    if (start == end)
    {
        FM_LOG_PRINT("   %02u %s\n", start, line);
    }
    else
    {
        FM_LOG_PRINT("%02u-%02u %s\n", start, end, line);
    }

}   /* end PrintSliceLine */




static void PrintIpLine(fm_uint start, fm_uint end, const char *line)
{
    FM_NOT_USED(line);

    if (start == end)
    {
        FM_LOG_PRINT("   %2u: %s\n", start, line);
    }
    else
    {
        FM_LOG_PRINT("%2u-%2u: %s\n", start, end, line);
    }

}   /* end PrintIpLine */




/*****************************************************************************/
/** FormatL4PortLine
 * \ingroup intDiagFFU
 *
 * \desc            Formats an entry from the L4 source or destination port
 *                  mapper.
 *
 * \param[out]      line points to a buffer of size BUFFER_SIZE in which
 *                  the formatted text will be stored.
 *
 * \param[in]       entry points to an array of 32-bit words containing
 *                  the entry to be formatted.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void FormatL4PortLine(char* line, const fm_uint32* entry)
{

    FM_SNPRINTF_S(line, BUFFER_SIZE,
                  "portKey: %5u  protocol: %d  valid: %d  mapResult: %5u",
                  FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_L4_SRC, L4SRC),
                  FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_L4_SRC, MAP_PROT),
                  FM_ARRAY_GET_BIT(  entry, FM10000_FFU_MAP_L4_SRC, VALID),
                  FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_L4_SRC, MAP_L4SRC));

}   /* end FormatL4PortLine*/




/*****************************************************************************/
/** PrintL4PortLine
 * \ingroup intDiagFFU
 *
 * \desc            Prints a line containing a formatted entry from the
 *                  L4 source or destination port mapper.
 *
 * \param[in]       start is the number of the first line with the specified
 *                  contents.
 *
 * \param[in]       end is the number of the last line with the specified
 *                  contents. May be the same as start.
 *
 * \param[in]       line points to the text of the line to print.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void PrintL4PortLine(fm_int start, fm_int end, const char* line)
{

    FM_NOT_USED(line);

    if (start == end)
    {
        FM_LOG_PRINT("   %2u: %s\n", start, line);
    }
    else
    {
        FM_LOG_PRINT("%2u-%2u: %s\n", start, end, line);
    }

}   /* end PrintL4PortLine */




/*****************************************************************************/
/** PrintEntryLine
 * \ingroup intDiagFFU
 *
 * \desc            Prints a line containing a formatted entry from the
 *                  mapper.
 *
 * \param[in]       start is the number of the first line with the specified
 *                  contents.
 *
 * \param[in]       end is the number of the last line with the specified
 *                  contents. May be the same as start.
 *
 * \param[in]       line points to the text of the line to print.
 * 
 * \param[in]       wide is TRUE if the line should be in wide format,
 *                  FALSE otherwise.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void PrintEntryLine(fm_int       start,
                           fm_int       end,
                           const char * line,
                           fm_bool      wide)
{

    FM_NOT_USED(line);

    if (start == end)
    {
        if (wide)
        {
            FM_LOG_PRINT("     %4u: %s\n", start, line);
        }
        else
        {
            FM_LOG_PRINT("   %2u: %s\n", start, line);
        }
    }
    else
    {
        if (wide)
        {
            FM_LOG_PRINT("%4u-%4u: %s\n", start, end, line);
        }
        else
        {
            FM_LOG_PRINT("%2u-%2u: %s\n", start, end, line);
        }
    }

}   /* end PrintEntryLine */




/*****************************************************************************/
/** DumpL4SrcMapper
 * \ingroup intDiagFFU
 *
 * \desc            Display the contents of the L4 source port mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpL4SrcMapper(fm_int sw)
{
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;
    char        newLine[BUFFER_SIZE];
    fm_uint32   entry[FM10000_FFU_MAP_L4_SRC_WIDTH];

    FM_LOG_PRINT("L4_SRC Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_L4_SRC_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapL4Src,
                               entry,
                               i,
                               FALSE);

        FormatL4PortLine(newLine, entry);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintL4PortLine(start, i - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintL4PortLine(start, i - 1, oldLine);
    FM_LOG_PRINT("\n");

}   /* end DumpL4SrcMapper */




/*****************************************************************************/
/** DumpL4DstMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the L4 destination port mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpL4DstMapper(fm_int sw)
{
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;
    char        newLine[BUFFER_SIZE];
    fm_uint32   entry[FM10000_FFU_MAP_L4_DST_WIDTH];

    FM_LOG_PRINT("L4_DST Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_L4_DST_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapL4Dst,
                               entry,
                               i,
                               FALSE);

        FormatL4PortLine(newLine, entry);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintL4PortLine(start, i - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintL4PortLine(start, i - 1, oldLine);
    FM_LOG_PRINT("\n");

}   /* end DumpL4DstMapper */




/*****************************************************************************/
/** FormatIpEntry
 * \ingroup intDiagFFU
 *
 * \desc            Formats an entry from the IP address mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slotId is the number of the slot to be formatted.
 *
 * \param[out]      line points to a buffer of size BUFFER_SIZE in which
 *                  the formatted text will be stored.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void FormatIpEntry(fm_int sw, fm_int slotId, char* line)
{
    char        addrBuffer[48];
    fm_fm10000MapIpCfg mapIpCfg;
    fm_status   err;

    err = fm10000GetMapIp(sw,
                          slotId,
                          &mapIpCfg,
                          FALSE);

    if (err != FM_OK)
    {
        FM_SNPRINTF_S(line, BUFFER_SIZE, "ERROR: %s", fmErrorMsg(err));
        return;
    }

    mapIpCfg.ipAddr.isIPv6 = TRUE;
    fmDbgConvertIPAddressToString(&(mapIpCfg.ipAddr), addrBuffer);

    FM_SNPRINTF_S(line, BUFFER_SIZE,
                  "%-39s %4d   %2d  %2d  %3d",
                  addrBuffer,
                  mapIpCfg.ignoreLength,
                  mapIpCfg.validSIP,
                  mapIpCfg.validDIP,
                  mapIpCfg.mapIp);

}   /* end FormatIpEntry*/




/*****************************************************************************/
/** DumpIpMapper
 * \ingroup intDiagFFU
 *
 * \desc            Display the contents of the IP address mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpIpMapper(fm_int sw)
{
    char        oldLine[BUFFER_SIZE];
    char        newLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;

    FM_LOG_PRINT("IP Mapper\n");
    FM_LOG_PRINT("=========\n");
    FM_LOG_PRINT("\n");

    FM_LOG_PRINT(" Slot  IP Address                              Ignore SIP DIP Value\n"
                 "-----  --------------------------------------- ------ --- --- -----\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_IP_CFG_ENTRIES ; ++i)
    {
        FormatIpEntry(sw, i, newLine);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintIpLine(start, i - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintIpLine(start, i - 1, oldLine);
    FM_LOG_PRINT("\n");

}   /* end DumpIpMapper */




/*****************************************************************************/
/** DumpSrcPortMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the source port mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpSrcPortMapper(fm_int sw)
{
    fm_int      i;
    fm_uint32   entry[FM10000_FFU_MAP_SRC_WIDTH];
    char        newLine[BUFFER_SIZE];
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;

    FM_LOG_PRINT("Src_Port Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_SRC_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapSrc,
                               entry,
                               i,
                               FALSE);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "mapResult: %2u routable: %u\n",
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_SRC, MAP_SRC),
                      FM_ARRAY_GET_BIT(entry, FM10000_FFU_MAP_SRC, Routable) );

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintEntryLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintEntryLine(start, i - 1, oldLine, FALSE);
    FM_LOG_PRINT("\n");

}   /* end DumpSrcPortMapper */




/*****************************************************************************/
/** DumpVidMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the vlan ID mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpVidMapper(fm_int sw)
{
    fm_int      i;
    fm_uint32   entry[FM10000_FFU_MAP_VLAN_WIDTH];
    char        newLine[BUFFER_SIZE];
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;

    FM_LOG_PRINT("Vlan_Id Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_VLAN_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapVlan,
                               entry,
                               i,
                               FALSE);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "mapResult: %2u routable: %u\n",
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_VLAN, MAP_VLAN),
                      FM_ARRAY_GET_BIT(entry, FM10000_FFU_MAP_VLAN, Routable) );

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintEntryLine(start, i - 1, oldLine, TRUE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintEntryLine(start, i - 1, oldLine, TRUE);
    FM_LOG_PRINT("\n");

}   /* end DumpVidMapper */




/*****************************************************************************/
/** DumpProtMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the protocol mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpProtMapper(fm_int sw)
{
    char        newLine[BUFFER_SIZE];
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;
    fm_uint32   entry[FM10000_FFU_MAP_PROT_WIDTH];

    FM_LOG_PRINT("Protocol Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_PROT_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapProt,
                               entry,
                               i,
                               FALSE);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "protocol: %3u mapResult: %1u\n",
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_PROT, PROT),
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_PROT, MAP_PROT) );

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintEntryLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintEntryLine(start, i - 1, oldLine, FALSE);
    FM_LOG_PRINT("\n");

}   /* end DumpProtMapper */




/*****************************************************************************/
/** DumpMacMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the MAC mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpMacMapper(fm_int sw)
{
    char        macStr[BUFFER_SIZE];
    char        newLine[BUFFER_SIZE];
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;
    fm_uint32   entry[FM10000_FFU_MAP_MAC_WIDTH];

    FM_LOG_PRINT("MAC Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_MAC_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapMac,
                               entry,
                               i,
                               FALSE);

        fmDbgConvertMacAddressToString(FM_ARRAY_GET_FIELD64(entry,
                                                            FM10000_FFU_MAP_MAC,
                                                            MAC),
                                       macStr);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "MAC: %s Ignore: %d bits SMAC: %u "
                      "DMAC: %u Router: %u mapResult: %2u\n",
                      macStr,
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_MAC, IgnoreLength),
                      FM_ARRAY_GET_BIT(entry, FM10000_FFU_MAP_MAC, validSMAC),
                      FM_ARRAY_GET_BIT(entry, FM10000_FFU_MAP_MAC, validDMAC),
                      FM_ARRAY_GET_BIT(entry, FM10000_FFU_MAP_MAC, Router),
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_MAC, MAP_MAC));

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintEntryLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintEntryLine(start, i - 1, oldLine, FALSE);
    FM_LOG_PRINT("\n");

}   /* end DumpMacMapper */




/*****************************************************************************/
/** DumpEthTypeMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the Ethernet type mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpEthTypeMapper(fm_int sw)
{
    char        newLine[BUFFER_SIZE];
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;
    fm_uint32   entry[FM10000_FFU_MAP_TYPE_WIDTH];

    FM_LOG_PRINT("Ethernet Type Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_TYPE_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapType,
                               entry,
                               i,
                               FALSE);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "EtherType: 0x%.4x mapResult: %2u\n",
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_TYPE, TYPE_XXX),
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_TYPE, MAP_TYPE));

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintEntryLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintEntryLine(start, i - 1, oldLine, FALSE);
    FM_LOG_PRINT("\n");

}   /* end DumpEthTypeMapper */




/*****************************************************************************/
/** DumpIpLengthMapper
 * \ingroup intDiagFFU
 *
 * \desc            Displays the contents of the IP Length mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DumpIpLengthMapper(fm_int sw)
{
    char        newLine[BUFFER_SIZE];
    char        oldLine[BUFFER_SIZE];
    fm_int      start = 0;
    fm_int      i;
    fm_uint32   entry[FM10000_FFU_MAP_LENGTH_WIDTH];

    FM_LOG_PRINT("IP Length Mapper\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;

    for (i = 0 ; i < FM10000_FFU_MAP_LENGTH_ENTRIES ; ++i)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuMapLength,
                               entry,
                               i,
                               FALSE);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "Length: %.5d mapResult: %2u\n",
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_LENGTH, LENGTH),
                      FM_ARRAY_GET_FIELD(entry, FM10000_FFU_MAP_LENGTH, MAP_LENGTH));

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0])
            {
                PrintEntryLine(start, i - 1, oldLine, FALSE);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintEntryLine(start, i - 1, oldLine, FALSE);
    FM_LOG_PRINT("\n");

}   /* end DumpIpLengthMapper */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000DbgDumpFFUSlice
 * \ingroup intDiagFFU
 *
 * \desc            Display the contents of the single FFU slice.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       slice is the one to print
 * 
 * \param[in]       onlyValidRules means skip any invalid CAM rules in
 *                  this slice
 *
 * \return          None
 *
 *****************************************************************************/
void fm10000DbgDumpFFUSlice(fm_int sw, fm_uint slice, fm_bool onlyValidRules)
{
    fm_switch *switchPtr = GET_SWITCH_PTR(sw);
    fm_uint    i;
    fm_int     j;
    fm_uint32  validSlice[FM10000_FFU_SLICE_VALID_WIDTH];
    fm_uint32  valid;
    fm_uint32  validRemain;
    fm_uint32  scenarioMask;
    fm_uint32  cascadeActionSlice[FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH];
    fm_uint32  cascadeAction;
    fm_byte    isCascading = 0;
    char       oldLine[BUFFER_SIZE];
    char       newLine[BUFFER_SIZE];
    fm_uint    start = 0;
    fm_registerSGListEntry sgList;
    fm_uint32 data[FM10000_FFU_SLICE_CFG_ENTRIES_0 * FM10000_FFU_SLICE_CFG_WIDTH];
    fm_uint32 dataCmp[FM10000_FFU_SLICE_CFG_WIDTH] = {0};
    fm_uint32 masterValid[2];
    fm_ffuSliceInfo sliceInfo;
    fm_bool validRule;
    fm_fm10000FfuSliceKey ruleKey;
    fm_ffuAction action;
    fm_ffuCaseLocation caseLocation = FM_FFU_CASE_NOT_MAPPED;

    READ_WORDS(FM10000_FFU_MASTER_VALID(0), masterValid);

    fmRegCacheReadSingle1D(sw, &fm10000CacheFfuSliceValid, validSlice, slice, FALSE);

    valid = FM_ARRAY_GET_FIELD(validSlice, FM10000_FFU_SLICE_VALID, Valid);

    FM_REGS_CACHE_FILL_SGLIST(&sgList,
                              &fm10000CacheFfuSliceCfg,
                              FM10000_FFU_SLICE_CFG_ENTRIES_0,
                              0,
                              slice,
                              FM_REGS_CACHE_INDEX_UNUSED,
                              data,
                              FALSE);
    fmRegCacheRead(sw, 1, &sgList, FALSE);

    fmRegCacheReadSingle1D(sw,
                           &fm10000CacheFfuSliceCascadeAction,
                           cascadeActionSlice,
                           slice,
                           FALSE);

    cascadeAction = FM_ARRAY_GET_FIELD(cascadeActionSlice,
                                       FM10000_FFU_SLICE_CASCADE_ACTION,
                                       CascadeAction);

    FM_LOG_PRINT("====================================================\n");
    FM_LOG_PRINT("FFU Slice %02d - MASTER_VALID %d SLICE_VALID 0x%08x CASCADE_ACTION 0x%08x\n",
                 slice, (masterValid[0] >> slice) & 0x1, valid, cascadeAction);
    FM_LOG_PRINT("====================================================\n");

    validRemain = valid;

    while (validRemain != 0)
    {
        scenarioMask = 0;
        for (i = 0 ; i < FM10000_FFU_SLICE_CFG_ENTRIES_0; i++)
        {
            if (validRemain & (1 << i))
            {
                if (scenarioMask == 0)
                {
                    dataCmp[0] = data[i * FM10000_FFU_SLICE_CFG_WIDTH];
                    dataCmp[1] = data[(i * FM10000_FFU_SLICE_CFG_WIDTH) + 1];

                    scenarioMask = (1 << i);
                    validRemain &= ~(1 << i);
                    isCascading = (cascadeAction >> i) & 0x1;
                }
                else if ( (dataCmp[0] == data[i * FM10000_FFU_SLICE_CFG_WIDTH]) &&
                          (dataCmp[1] == data[(i * FM10000_FFU_SLICE_CFG_WIDTH) + 1]) &&
                          (isCascading == ((cascadeAction >> i) & 0x1)) )
                {
                    scenarioMask |= (1 << i);
                    validRemain &= ~(1 << i);
                }
            }
        }
        FM_LOG_PRINT("SLICE_CFG: 0x%08x%08x\n", dataCmp[1], dataCmp[0]);
        FM_LOG_PRINT("ScenarioMask: 0x%08x  ActionCascade: %d\n", scenarioMask, isCascading);
        FM_LOG_PRINT("----------------------------------------\n");
        FM_LOG_PRINT("StartCompare: %d StartAction: %d ValidLow: %d ValidHigh: %d\n",
                     FM_ARRAY_GET_BIT(dataCmp, FM10000_FFU_SLICE_CFG, StartCompare),
                     FM_ARRAY_GET_BIT(dataCmp, FM10000_FFU_SLICE_CFG, StartAction),
                     FM_ARRAY_GET_BIT(dataCmp, FM10000_FFU_SLICE_CFG, ValidLow),
                     FM_ARRAY_GET_BIT(dataCmp, FM10000_FFU_SLICE_CFG, ValidHigh));

        FM_LOG_PRINT("Case_XXX:   0x%x Position: ",
                     FM_ARRAY_GET_FIELD(dataCmp, FM10000_FFU_SLICE_CFG, Case_XXX));

        caseLocation = FM_ARRAY_GET_FIELD(dataCmp, FM10000_FFU_SLICE_CFG, SetCaseLocation);
        switch (caseLocation)
        {
            case FM_FFU_CASE_NOT_MAPPED:
                FM_LOG_PRINT("FM_FFU_CASE_NOT_MAPPED\n");
                break;

            case FM_FFU_CASE_TOP_LOW_NIBBLE:
                FM_LOG_PRINT("FM_FFU_CASE_TOP_LOW_NIBBLE\n");
                break;

            case FM_FFU_CASE_TOP_HIGH_NIBBLE:
                FM_LOG_PRINT("FM_FFU_CASE_TOP_HIGH_NIBBLE\n");
                break;

            default:
                FM_LOG_PRINT("UNDEFINED POSITION\n");
                break;
        }
        for (j = 4 ; j >= 0 ; j--)
        {
            PrintSelect(j, ( 0x3f & ( dataCmp[0] >> (j * 6) ) ) );
        }

        FM_LOG_PRINT("\n");

    }

    PrintSliceHeading();

    oldLine[0] = 0;
    caseLocation = FM_FFU_CASE_NOT_MAPPED;
    for (j = 0 ; j < FM10000_FFU_SLICE_TCAM_ENTRIES_0 ; j++)
    {
        sliceInfo.keyStart = slice;
        sliceInfo.keyEnd = slice;
        sliceInfo.actionEnd = slice;
        sliceInfo.caseLocation = &caseLocation;

        fm10000GetFFURule(sw, &sliceInfo, j, &validRule, &ruleKey, &action, FALSE);

        FormatSliceLine(newLine, validRule, &ruleKey, &action, caseLocation);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0] != 0)
            {
                PrintSliceLine(start, j - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = j;
        }
    }

    PrintSliceLine(start, j - 1, oldLine);
    FM_LOG_PRINT("\n");

}



/*****************************************************************************/
/** fm10000DbgDumpMapper
 * \ingroup intDiagFFU
 *
 * \desc            Display the contents of the Mapper.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None
 *
 *****************************************************************************/
void fm10000DbgDumpMapper(fm_int sw)
{
    DumpIpMapper(sw);
    DumpL4SrcMapper(sw);
    DumpL4DstMapper(sw);
    DumpProtMapper(sw);
    DumpMacMapper(sw);
    DumpEthTypeMapper(sw);
    DumpIpLengthMapper(sw);
    DumpSrcPortMapper(sw);
    DumpVidMapper(sw);

}   /* end fm10000DbgDumpMapper */



/*****************************************************************************/
/** fm10000DbgDumpFFU
 * \ingroup intDiagFFU
 *
 * \desc            Display the contents of the FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       onlyValidSlices means skip any minslices
 *                  whose master valid bit is zero or whose valid register
 *                  is all zeros.
 *
 * \param[in]       onlyValidRules means skip any invalid CAM rules in
 *                  this slice
 *
 * \return          None
 *
 *****************************************************************************/
void fm10000DbgDumpFFU(fm_int sw,
                       fm_bool onlyValidSlices,
                       fm_bool onlyValidRules)
{
    fm_switch *switchPtr;
    fm_uint32  masterValid[2];
    fm_uint    i;
    fm_uint    j;
    char       oldLine[BUFFER_SIZE];
    char       newLine[BUFFER_SIZE];
    fm_uint    start = 0;
    fm_uint32  validSlice[FM10000_FFU_SLICE_VALID_WIDTH];
    fm_uint32  valid;
    fm_uint32  cascadeActionSlice[FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH];
    fm_uint32  cascadeAction;
    fm_uint32  cfg[FM10000_FFU_EGRESS_CHUNK_CFG_WIDTH];
    fm_uint32  validChunk[FM10000_FFU_EGRESS_CHUNK_VALID_WIDTH];
    fm_uint32  portCfg[FM10000_FFU_EGRESS_PORT_CFG_WIDTH];
    fm_uint64  portMask;
    fm_bool    drop;
    fm_uint64  dropCount;

    switchPtr = GET_SWITCH_PTR(sw);

    READ_WORDS(FM10000_FFU_MASTER_VALID(0), masterValid);

    for (i = 0 ; i < FM10000_FFU_SLICE_VALID_ENTRIES ; i++)
    {
        fmRegCacheReadSingle1D(sw, &fm10000CacheFfuSliceValid, validSlice, i, FALSE);

        valid = FM_ARRAY_GET_FIELD(validSlice, FM10000_FFU_SLICE_VALID, Valid);

        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuSliceCascadeAction,
                               cascadeActionSlice,
                               i,
                               FALSE);

        cascadeAction = FM_ARRAY_GET_FIELD(cascadeActionSlice,
                                           FM10000_FFU_SLICE_CASCADE_ACTION,
                                           CascadeAction);

        if ( onlyValidSlices && ( ( (valid == 0) && (cascadeAction == 0) ) ||
                                  ( ( 1 & (masterValid[0] >> i) ) == 0 ) ) )
        {
            continue;
        }

        fm10000DbgDumpFFUSlice(sw, i, onlyValidRules);
    }

    /**************************************************
     * Egress ACLs
     **************************************************/

    oldLine[0] = 0;
    start = 0;

    for (i = 0 ; i < FM10000_FFU_EGRESS_CHUNK_CFG_ENTRIES ; i++)
    {
        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuEgressChunkCfg,
                               cfg,
                               i,
                               FALSE);

        fmRegCacheReadSingle1D(sw,
                               &fm10000CacheFfuEgressChunkValid,
                               validChunk,
                               i,
                               FALSE);

        valid = FM_ARRAY_GET_FIELD(validChunk, FM10000_FFU_EGRESS_CHUNK_VALID, Valid);

        if ( onlyValidSlices && ( ( valid == 0 ) ||
                                  ( ( 1 & (masterValid[1] >> i) ) == 0 ) ) )
        {
            continue;
        }

        FM_LOG_PRINT("====================================================\n");
        FM_LOG_PRINT("FFU Chunk %02d - MASTER_VALID %d CHUNK_VALID 0x%08x NEW_PRI %d\n",
                     i, (masterValid[1] >> i) & 0x1, valid,
                     FM_ARRAY_GET_BIT(cfg, FM10000_FFU_EGRESS_CHUNK_CFG, StartCascade));

        portMask = 0;
        for (j = 0 ; j < FM10000_FFU_EGRESS_PORT_CFG_ENTRIES ; j++)
        {
            fmRegCacheReadSingle1D(sw,
                                   &fm10000CacheFfuEgressPortCfg,
                                   portCfg,
                                   j,
                                   FALSE);
            portMask |= ((fm_uint64)((portCfg[0] >> i) & 1LL) << j);
        }
        FM_LOG_PRINT("PORT MASK 0x%012llx\n", portMask);
        FM_LOG_PRINT("====================================================\n");

        oldLine[0] = 0;
        start = i * 32;
        for (j = (i * 32) ; j < ((i + 1) * 32); j++)
        {
            fm10000GetFFUEaclAction(sw, j, &drop, FALSE);

            if (drop)
            {
                FM_STRCPY_S(newLine, sizeof(newLine), "Action Drop");
            }
            else
            {
                FM_STRCPY_S(newLine, sizeof(newLine), "Action None");
            }

            if (strcmp(oldLine, newLine) != 0)
            {
                if (oldLine[0] != 0)
                {
                    PrintSliceLine(start, j - 1, oldLine);
                }

                FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
                start = j;
            }
        }

        PrintSliceLine(start, j - 1, oldLine);
    }

    FM_LOG_PRINT("\nEgress Drop Count\n");
    FM_LOG_PRINT("=============\n");
    FM_LOG_PRINT("\n");

    oldLine[0] = 0;
    start = 0;

    for (i = 0 ; i < FM10000_FFU_EGRESS_DROP_COUNT_ENTRIES ; i++)
    {
        fm10000GetFFUEaclCounter(sw, i, &dropCount);

        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "Drop Count = %llu", dropCount);

        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0] != 0)
            {
                PrintSliceLine(start, i - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }

    PrintSliceLine(start, i - 1, oldLine);


}   /* end fm10000DbgDumpFFU */



/*****************************************************************************/
/** fm10000DbgDumpPolicers
 * \ingroup intDiagFFU
 *
 * \desc            Display the contents of the Policers bank.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          None
 *
 *****************************************************************************/
void fm10000DbgDumpPolicers(fm_int sw)
{
    fm_uint32 i;
    fm_byte   table[FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES];
    fm_uint32 bank;
    fm_uint16 indexLastPolicer;
    fm_ffuColorSource ingressColorSource;
    fm_bool   markDSCP;
    fm_bool   markSwitchPri;
    fm_uint32 maxPolicerIndex;
    fm_ffuPolicerState committed;
    fm_ffuPolicerState excess;
    char      oldLine[BUFFER_SIZE];
    char      newLine[BUFFER_SIZE];
    fm_uint   start = 0;
    fm_uint64 frameCount;
    fm_uint64 byteCount;

    fm10000GetPolicerDSCPDownMap(sw, table, FALSE);
    FM_LOG_PRINT("\n==================================================================");
    FM_LOG_PRINT("\nPolicer DSCP Down Map");
    FM_LOG_PRINT("\n==================================================================\n");
    FM_LOG_PRINT("---------------+-----------------+\n");
    FM_LOG_PRINT(" Original DSCP | Downgraded DSCP |\n");
    oldLine[0] = 0;
    for (i = 0 ; i < FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES; i++)
    {
        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "         |       %02d        |", table[i]);
        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0] != 0)
            {
                PrintShortLine(start, i - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintShortLine(start, i - 1, oldLine);
    FM_LOG_PRINT("---------------+-----------------+\n");

    fm10000GetPolicerSwPriDownMap(sw, table, FALSE);
    FM_LOG_PRINT("\n==================================================================");
    FM_LOG_PRINT("\nPolicer Switch Priority Down Map");
    FM_LOG_PRINT("\n==================================================================\n");
    FM_LOG_PRINT("----------------+------------------+\n");
    FM_LOG_PRINT(" Original SwPri | Downgraded SwPri |\n");
    oldLine[0] = 0;
    for (i = 0 ; i < FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES; i++)
    {
        FM_SNPRINTF_S(newLine, BUFFER_SIZE, "          |        %02d        |", table[i]);
        if (strcmp(oldLine, newLine) != 0)
        {
            if (oldLine[0] != 0)
            {
                PrintShortLine(start, i - 1, oldLine);
            }

            FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
            start = i;
        }
    }
    PrintShortLine(start, i - 1, oldLine);
    FM_LOG_PRINT("----------------+------------------+\n");

    for (bank = 0 ; bank < FM_FM10000_POLICER_BANK_MAX ; bank++)
    {
        fm10000GetPolicerConfig(sw,
                                bank,
                                &indexLastPolicer,
                                &ingressColorSource,
                                &markDSCP,
                                &markSwitchPri,
                                FALSE);

        if (bank <= FM_FM10000_POLICER_BANK_4K_2)
        {
            maxPolicerIndex = FM_FM10000_MAX_POLICER_4K_INDEX;
        }
        else
        {
            maxPolicerIndex = FM_FM10000_MAX_POLICER_512_INDEX;
        }

        FM_LOG_PRINT("\n==================================================================");
        FM_LOG_PRINT("\nPolicer %s Bank %d Config",
                     (maxPolicerIndex == FM_FM10000_MAX_POLICER_4K_INDEX)? "4K" : "512",
                     bank);
        FM_LOG_PRINT("\n==================================================================\n");
        FM_LOG_PRINT("IndexLastPolicer = %04d   ingressColorSrc = ", indexLastPolicer);
        switch (ingressColorSource)
        {
            case FM_COLOR_SOURCE_DSCP:
                FM_LOG_PRINT("DSCP\n");
                break;

            case FM_COLOR_SOURCE_SWITCH_PRI:
                FM_LOG_PRINT("Swith Pri\n");
                break;

            case FM_COLOR_SOURCE_ASSUME_GREEN:
                FM_LOG_PRINT("Assume Green\n");
                break;

            default:
                FM_LOG_PRINT("Error\n");
                break;
        }
        FM_LOG_PRINT("MarkDown DSCP = %d         MarkDown SwPri = %d\n", markDSCP, markSwitchPri);
        FM_LOG_PRINT("----------+----------------+------------+------+----------------+------------+------+\n");
        FM_LOG_PRINT("   Index  | Committed Rate +  Capacity  + Act  |   Excess Rate  +  Capacity  + Act  |\n");
        oldLine[0] = 0;
        for (i = 0 ; i <= maxPolicerIndex ; i++)
        {
            fm10000GetPolicer(sw, bank, i, &committed, &excess);

            FormatPolicerLine(newLine, &committed, &excess);

            if (strcmp(oldLine, newLine) != 0)
            {
                if (oldLine[0] != 0)
                {
                    PrintSliceLine(start, i - 1, oldLine);
                }

                FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
                start = i;
            }
        }

        PrintSliceLine(start, i - 1, oldLine);
        FM_LOG_PRINT("----------+----------------+------------+------+----------------+------------+------+\n");
        FM_LOG_PRINT("\n==================================================================");
        FM_LOG_PRINT("\nPolicer %s Bank %d State",
                     (maxPolicerIndex == FM_FM10000_MAX_POLICER_4K_INDEX)? "4K" : "512",
                     bank);
        FM_LOG_PRINT("\n==================================================================\n");
        FM_LOG_PRINT("----------+------------------------+------------------------+\n");
        FM_LOG_PRINT("   Index  |         Bytes          |         Frames         |\n");
        oldLine[0] = 0;
        for (i = 0 ; i <= maxPolicerIndex ; i++)
        {
            fm10000GetPolicerCounter(sw, bank, i, &frameCount, &byteCount);

            FormatCounterLine(newLine, frameCount, byteCount);

            if (strcmp(oldLine, newLine) != 0)
            {
                if (oldLine[0] != 0)
                {
                    PrintSliceLine(start, i - 1, oldLine);
                }

                FM_MEMCPY_S(oldLine, sizeof(oldLine), newLine, BUFFER_SIZE);
                start = i;
            }
        }

        PrintSliceLine(start, i - 1, oldLine);
        FM_LOG_PRINT("----------+------------------------+------------------------+\n");
            
    }

}   /* end fm10000DbgDumpPolicers */

