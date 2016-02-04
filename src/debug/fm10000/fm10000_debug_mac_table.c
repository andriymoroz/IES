/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_debug_mac_table.c
 * Creation Date:   December 9, 2013
 * Description:     Debug functions for inspecting the MAC table.
 *
 * Copyright (c) 2006 - 2014, Intel Corporation
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

typedef struct _fm10000_maTableEntry
{
    fm_macaddr  macAddress;
    fm_uint16   fid;
    fm_uint16   glort;
    fm_int      port;
    fm_uint32   trigger;
    fm_bool     secure;
    fm_bool     valid;
} fm10000_maTableEntry;


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/* DecodeMacTableEntry
 * \ingroup intDiagMATable
 *
 * \desc            Decodes an FM10000 MA Table entry structure.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      entry points to the structure to receive the decoded
 *                  MA Table entry.
 *
 * \param[in]       words points to an array containing the hardware
 *                  MA Table entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeMacTableEntry(fm_int                 sw,
                                     fm10000_maTableEntry * entry,
                                     fm_uint32 *            words)
{
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_status           status;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    status = FM_OK;

    entry->macAddress = FM_ARRAY_GET_FIELD64(words,
                                             FM10000_MA_TABLE,
                                             MACAddress);
    entry->fid        = FM_ARRAY_GET_FIELD(words, FM10000_MA_TABLE, FID);
    entry->secure     = FM_ARRAY_GET_BIT(words, FM10000_MA_TABLE, secure);
    entry->trigger    = FM_ARRAY_GET_FIELD(words, FM10000_MA_TABLE, trigId);
    entry->glort      = FM_ARRAY_GET_FIELD(words, FM10000_MA_TABLE, glort);
    entry->valid      = FM_ARRAY_GET_BIT(words, FM10000_MA_TABLE, valid);
    entry->port       = -1;

    if (entry->valid)
    {
        status = fmGetGlortLogicalPort(sw, entry->glort, &entry->port);
        if (status == FM_ERR_INVALID_PORT)
        {
            /* The address may be associated with a glort on a remote switch
             * that is not yet known to us. See if we can create a remote
             * logical port for the glort, and use it if we are successful. */
            if (switchExt->createRemoteLogicalPorts)
            {
                status = fmCreateStackLogicalPort(sw, entry->glort, &entry->port);
                if (status != FM_OK)
                {
                    status = FM_ERR_INVALID_PORT;
                }
            }
        }

        if (status != FM_OK)
        {
            /* Check if it is a tunnel GloRT user, leave port as -1 if yes */
            status = fmCheckGlortRangeType(switchPtr, entry->glort, 1,
                                           FM_GLORT_TYPE_TUNNEL);
        }
    }

    return status;

}   /* end DecodeMacTableEntry */




/*****************************************************************************/
/** DumpMacTableEntry
 * \ingroup intDiagMATable
 *
 * \desc            Output an MA Table entry as it appears in hardware and
 *                  in the memory-based cache.
 *
 * \param[in]       index is the index into the MA Table.
 *
 * \param[in]       cacheEntry points to a copy of the memory-based cache
 *                  entry.
 *
 * \param[in]       dmacEntry points to a structure containing the decoded
 *                  destination MAC table entry.
 *
 * \param[in]       smacEntry points to a structure containing the decoded
 *                  source MAC table entry.
 *
 * \return          None.
 *
 *****************************************************************************/
static void DumpMacTableEntry(fm_int                    index,
                              fm_internalMacAddrEntry * cacheEntry,
                              fm10000_maTableEntry *    dmacEntry,
                              fm10000_maTableEntry *    smacEntry)
{
    char dmacVal[32];
    char smacVal[32];
    char cacheVal[32];
    
    FM_LOG_PRINT("MA_TABLE[%d]:\n\n", index);

#define HDR_FMT  "%-14s   %-18s %-18s %s\n"
#define STR_FMT  "%-14s : %-18s %-18s %s\n"
#define DEC_FMT  "%-14s : %-18d %-18d %d\n"
#define FILL_VAL "--"

    FM_LOG_PRINT(HDR_FMT,
                 "",
                 "CACHE ENTRY",
                 "DMAC ENTRY",
                 "SMAC ENTRY");

    FM_LOG_PRINT(STR_FMT,
                 "State",
                 fmEntryStateToText(cacheEntry->state),
                 (dmacEntry->valid) ? "Valid" : "Invalid",
                 (smacEntry->valid) ? "Valid" : "Invalid");

    FM_SNPRINTF_S(cacheVal, sizeof(cacheVal), "%012llx", cacheEntry->macAddress);
    FM_SNPRINTF_S(dmacVal,  sizeof(dmacVal),  "%012llx", dmacEntry->macAddress);
    FM_SNPRINTF_S(smacVal,  sizeof(smacVal),  "%012llx", smacEntry->macAddress);

    FM_LOG_PRINT(STR_FMT,
                 "MAC Address",
                 cacheVal,
                 dmacVal,
                 smacVal);

    FM_LOG_PRINT(DEC_FMT,
                 "FID",
                 cacheEntry->vlanID,
                 dmacEntry->fid,
                 smacEntry->fid);

    FM_LOG_PRINT(STR_FMT,
                 "Address Type",
                 fmAddressTypeToText(cacheEntry->addrType),
                 FILL_VAL,
                 FILL_VAL);

    FM_SNPRINTF_S(cacheVal, sizeof(cacheVal), "%s", FILL_VAL);
    FM_SNPRINTF_S(dmacVal,  sizeof(dmacVal),  "0x%04x", dmacEntry->glort);
    FM_SNPRINTF_S(smacVal,  sizeof(smacVal),  "0x%04x", smacEntry->glort);

    FM_LOG_PRINT(STR_FMT,
                 "Glort",
                 cacheVal,
                 dmacVal,
                 smacVal);

    FM_LOG_PRINT(DEC_FMT,
                 "Port",
                 cacheEntry->port,
                 dmacEntry->port,
                 smacEntry->port);

    FM_LOG_PRINT(DEC_FMT,
                 "Trigger",
                 cacheEntry->trigger,
                 dmacEntry->trigger,
                 smacEntry->trigger);

    FM_LOG_PRINT(STR_FMT,
                 "Secure",
                 FM_BOOLSTRING(FM_IS_ADDR_TYPE_SECURE(cacheEntry->addrType)),
                 FM_BOOLSTRING(dmacEntry->secure),
                 FM_BOOLSTRING(smacEntry->secure));

    FM_LOG_PRINT("\n");

}   /* end DumpMacTableEntry */



/*****************************************************************************/
/** ReadMacTableEntries
 * \ingroup intDiagMATable
 *
 * \desc            Output an MA Table entry as it appears in hardware and
 *                  in the memory-based cache.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index is the index into the MA Table.
 *
 * \param[out]      dmacEntry points to a structure to receive the decoded
 *                  destination MAC table entry.
 *
 * \param[out]      smacEntry points to a structure to receive the decoded
 *                  source MAC table entry.
 *
 * \return          None.
 *
 *****************************************************************************/
static fm_status ReadMacTableEntries(fm_int                 sw,
                                     fm_uint32              index,
                                     fm10000_maTableEntry * dmacEntry,
                                     fm10000_maTableEntry * smacEntry)
{
    fm_switch * switchPtr;
    fm_uint32   words[FM10000_MA_TABLE_WIDTH];
    fm_status   status;

    switchPtr = GET_SWITCH_PTR(sw);

    /* Retrieve DMAC table entry. */
    status = switchPtr->ReadUINT32Mult(sw,
                                       FM10000_MA_TABLE(0, index, 0),
                                       FM10000_MA_TABLE_WIDTH,
                                       words);
    if (status != FM_OK)
    {
        FM_LOG_PRINT("Error reading DMAC entry %u: %s\n",
                     index,
                     fmErrorMsg(status));
        return status;
    }

    status = DecodeMacTableEntry(sw, dmacEntry, words);
    if (status != FM_OK)
    {
        FM_LOG_PRINT("Error decoding DMAC entry %u: %s\n",
                     index,
                     fmErrorMsg(status));
        return status;
    }

    /* Retrieve SMAC table entry. */
    status = switchPtr->ReadUINT32Mult(sw,
                                       FM10000_MA_TABLE(1, index, 0),
                                       FM10000_MA_TABLE_WIDTH,
                                       words);
    if (status != FM_OK)
    {
        FM_LOG_PRINT("Error reading SMAC entry %u: %s\n",
                     index,
                     fmErrorMsg(status));
        return status;
    }

    status = DecodeMacTableEntry(sw, smacEntry, words);
    if (status != FM_OK)
    {
        FM_LOG_PRINT("Error decoding SMAC entry %u: %s\n",
                     index,
                     fmErrorMsg(status));
    }

    return status;

}   /* end ReadMacTableEntries */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000DbgDumpMACTable
 * \ingroup intDiagMATable
 *
 * \desc            Display a given number of entries or count the number of
 *                  entries in the switch's MAC Address Table and the
 *                  memory-based copy of the table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       numEntries is the number of entries to display.  Specify
 *                  0 for the first 10 entries only.  Specify -1 to just count
 *                  the number of entries in the table without displaying them.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpMACTable(fm_int sw, fm_int numEntries)
{
    fm_switch *             switchPtr;
    fm_internalMacAddrEntry cacheEntry;
    fm10000_maTableEntry    dmacEntry;
    fm10000_maTableEntry    smacEntry;
    fm_status               status;
    fm_int                  index;
    fm_int                  entriesReported;
    fm_bool                 countOnly;

    switchPtr = GET_SWITCH_PTR(sw);

    entriesReported = 0;
    countOnly = FALSE;

    if (numEntries < 0)
    {
        countOnly  = TRUE;
        numEntries = switchPtr->macTableSize;
    }
    else if (numEntries == 0)
    {
        numEntries = 10;
    }

    for (index = 0 ; index < switchPtr->macTableSize ; ++index)
    {
        /* Get cache entry. */
        FM_TAKE_L2_LOCK(sw);
        cacheEntry = switchPtr->maTable[index];
        FM_DROP_L2_LOCK(sw);

        /* Retrieve MAC table entries. */
        status = ReadMacTableEntries(sw, index, &dmacEntry, &smacEntry);
        if (status != FM_OK)
        {
            return;
        }

        if (dmacEntry.valid ||
            smacEntry.valid ||
            cacheEntry.state != FM_MAC_ENTRY_STATE_INVALID)
        {
            if (!countOnly)
            {
                DumpMacTableEntry(index, &cacheEntry, &dmacEntry, &smacEntry);
            }

            if (++entriesReported >= numEntries)
            {
                break;
            }

        }   /* end if (dmacEntry.valid) */

    }   /* end for (index = 0 ; index < switchPtr->macTableSize ; ++index) */

    FM_LOG_PRINT("%d %s %s\n",
                 entriesReported,
                 (entriesReported == 1) ? "entry" : "entries",
                 (countOnly) ? "in table" : "listed");

}   /* end fm10000DbgDumpMACTable */




/*****************************************************************************/
/** fm10000DbgDumpMACTableEntry
 * \ingroup intDiagMATable
 *
 * \desc            Display a specific entry in the switch's MAC Address Table
 *                  and the memory-based copy of the table entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       address is the MAC address of the entry to display.
 *
 * \param[in]       vlan is the VLAN ID of the entry to display.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpMACTableEntry(fm_int sw, fm_macaddr address, fm_uint16 vlan)
{
    fm_internalMacAddrEntry cacheEntry;
    fm10000_maTableEntry    dmacEntry;
    fm10000_maTableEntry    smacEntry;
    fm_switch *             switchPtr;
    fm_uint16               indexes[FM10000_MAC_ADDR_BANK_COUNT];
    fm_uint32               hashIndex;
    fm_status               status;
    fm_int                  bankId;
    fm_uint16               learningFID;
    fm_bool                 found;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_PRINT("\n");

    status = fm10000GetLearningFID(sw, vlan, &learningFID);
    if (status != FM_OK)
    {
        return;
    }

    status = fm10000ComputeAddressIndex(sw,
                                        address,
                                        learningFID,
                                        0,
                                        indexes);
    if (status != FM_OK)
    {
        return;
    }

    found = FALSE;

    for (bankId = switchPtr->macTableBankCount - 1 ; bankId >= 0 ; bankId--)
    {
        hashIndex = indexes[bankId];
        FM_TAKE_L2_LOCK(sw);
        cacheEntry = switchPtr->maTable[hashIndex];
        FM_DROP_L2_LOCK(sw);

        if ( (cacheEntry.state != FM_MAC_ENTRY_STATE_INVALID) &&
             (cacheEntry.macAddress == address) &&
             (cacheEntry.vlanID == learningFID) )
        {
            /* Found the requested entry. */
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        status = ReadMacTableEntries(sw, hashIndex, &dmacEntry, &smacEntry);
        if (status == FM_OK)
        {
            DumpMacTableEntry(hashIndex, &cacheEntry, &dmacEntry, &smacEntry);
        }
    }
    else
    {
        FM_LOG_PRINT("Entry not found for macAddress %012llx vlan %u (fid %u).\n",
                     address,
                     vlan,
                     learningFID);
    }
    

}   /* end fm10000DbgDumpMACTableEntry */


