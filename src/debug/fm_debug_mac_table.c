/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_mac_table.c
 * Creation Date:   April 27, 2006
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fmDbgDumpMACTable
 * \ingroup diagMATable 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display a given number of entries or count the number of
 *                  entries in the switch's MAC Address Table.
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
void fmDbgDumpMACTable(fm_int sw, fm_int numEntries)
{
    fm_switch *switchPtr;
    fm_status  err;
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n",
                     sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMACTable, sw, numEntries);

    UNPROTECT_SWITCH(sw);
        
}   /* end fmDbgDumpMACTable */




/*****************************************************************************/
/** fmDbgDumpMACCache
 * \ingroup diagMATable
 *
 * \chips           FM2000, FM3000, FM4000
 *
 * \desc            Display a given number of entries or count the number of
 *                  entries in the memory-based copy of the switch's MAC
 *                  Address Table.
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
void fmDbgDumpMACCache(fm_int sw, fm_int numEntries)
{
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMACCache, sw, numEntries);

}   /* end fmDbgDumpMACCache */




/*****************************************************************************/
/** fmDbgDumpMACTableEntry
 * \ingroup diagMATable 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Display a specific entry in the switch's MAC Address Table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlan is the VLAN ID of the entry to display.
 *
 * \param[in]       addressStr is the MAC address of the entry to display
 *                  formatted as "XX:XX:XX:XX:XX:XX" or "XXXXXXXXXXXX".
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpMACTableEntry(fm_int sw, fm_uint16 vlan, fm_text addressStr)
{
    fm_switch * switchPtr;
    fm_uint32   words[6];
    fm_macaddr  address;
    fm_int      scanCount;
    fm_status   err;
    fm_int      i;

    /**************************************************
     * Parse the specified MAC address.
     **************************************************/

    scanCount = FM_SSCANF_S(addressStr,
                            "%2x:%2x:%2x:%2x:%2x:%2x",
                            &words[0],
                            &words[1],
                            &words[2],
                            &words[3],
                            &words[4],
                            &words[5]);

    if (scanCount != 6)
    {
        /**************************************************
         * Try again without colons.
         **************************************************/
        scanCount = FM_SSCANF_S(addressStr,
                                "%2x%2x%2x%2x%2x%2x",
                                &words[0],
                                &words[1],
                                &words[2],
                                &words[3],
                                &words[4],
                                &words[5]);

        if (scanCount != 6)
        {
            FM_LOG_PRINT("%s is not a valid MAC address.\n", addressStr);
            return;
        }
    }

    address = FM_LITERAL_64(0);

    /* Pack bytes into a single address variable */
    for (i = 0 ; i < 6 ; i++)
    {
        address <<= 8;
        address  |= (fm_macaddr)words[i];
    }
    
    VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw);
    
    if (err != FM_OK)
    {
        FM_LOG_PRINT("Switch %d does not exist or is down.\n", sw);
        return;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpMACTableEntry,
                            sw,
                            address,
                            vlan);

    UNPROTECT_SWITCH(sw);

}   /* end fmDbgDumpMACTableEntry */




/*****************************************************************************/
/** fmDbgTraceMACAddress
 * \ingroup diagMATable 
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a MAC address that should be traced. Diagnostic
 *                  messages will be output for all MA Table operations on
 *                  the specified MAC address.
 *
 * \param[in]       macAddr is the MAC address to trace. Set to zero to
 *                  turn off tracing.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgTraceMACAddress(fm_macaddr macAddr)
{
    fmRootApi->testTraceMacAddress = macAddr;

}   /* end fmDbgTraceMACAddress */
