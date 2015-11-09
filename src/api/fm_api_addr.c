/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_addr.c
 * Creation Date:   2005
 * Description:     Structures and functions for dealing with MA table
 *                  configuration
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#define IS_VALID_PORT(sw, port) \
    fmIsValidPort(sw, (fm_int)port, ALLOW_CPU | ALLOW_LAG | ALLOW_REMOTE)

#define IS_VALID_VLAN(sw, vid) \
    ( (vid) > 0 && (vid) < FM_MAX_VLAN && \
      GET_SWITCH_PTR(sw)->vidTable[vid].valid && \
      GET_SWITCH_PTR(sw)->reservedVlan != (fm_uint16)(vid) )


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
/** fmAddressTypeToText
 * \ingroup intAddr
 *
 * \desc            Returns a textual representation of an MA Table
 *                  address type.
 *
 * \param[in]       addrType is the MA Table address type.
 *
 * \return          Pointer to a string representation of addrType.
 *
 *****************************************************************************/
fm_text fmAddressTypeToText(fm_int addrType)
{

    switch (addrType)
    {
        case FM_ADDRESS_STATIC:
            return "STATIC";

        case FM_ADDRESS_DYNAMIC:
            return "DYNAMIC";

        case FM_ADDRESS_SECURE_STATIC:
            return "SECURE_STATIC";
 
        case FM_ADDRESS_SECURE_DYNAMIC:
            return "SECURE_DYNAMIC";

        case FM_ADDRESS_PROVISIONAL:
            return "PROVISIONAL";

        default:
            return "UNKNOWN";

    }   /* end switch (addrType) */

}   /* end fmAddressTypeToText */




/*****************************************************************************/
/** fmEntryStateToText
 * \ingroup intAddr
 *
 * \desc            Returns a textual representation of an MA Table state.
 *
 * \param[in]       entryState is the MA Table state.
 *
 * \return          Pointer to a string representation of entryState.
 *
 *****************************************************************************/
fm_text fmEntryStateToText(fm_int entryState)
{

    switch (entryState)
    {
        case FM_MAC_ENTRY_STATE_INVALID:
            return "Invalid";

        case FM_MAC_ENTRY_STATE_YOUNG:
            return "Young";

        case FM_MAC_ENTRY_STATE_OLD:
            return "Old";

        case FM_MAC_ENTRY_STATE_MOVED:
            return "Moved";

        case FM_MAC_ENTRY_STATE_LOCKED:
            return "Locked";

        case FM_MAC_ENTRY_STATE_EXPIRED:
            return "Expired";

        default:
            return "Unknown";

    }   /* end switch (entryState) */

}   /* end fmEntryStateToText */




/*****************************************************************************/
/** fmFlushModeToText
 * \ingroup intAddr
 *
 * \desc            Returns a textual representation of an MA Table
 *                  flush mode.
 *
 * \param[in]       flushMode is the MA Table flush mode.
 *
 * \return          Pointer to a string representation of entryState.
 *
 *****************************************************************************/
fm_text fmFlushModeToText(fm_int flushMode)
{

    switch (flushMode)
    {
        case FM_FLUSH_MODE_ALL_DYNAMIC:
            return "DYNAMIC";

        case FM_FLUSH_MODE_PORT:
            return "PORT";

        case FM_FLUSH_MODE_PORT_VLAN:
            return "PORT_VLAN";

        case FM_FLUSH_MODE_VLAN:
            return "VLAN";

        case FM_FLUSH_MODE_VID1_VID2:
            return "VID1_VID2";

        case FM_FLUSH_MODE_PORT_VID1_VID2:
            return "PORT_VID1_VID2";

        case FM_FLUSH_MODE_PORT_VID2:
            return "PORT_VID2";

        case FM_FLUSH_MODE_VID2:
            return "VID2";

        case FM_FLUSH_MODE_PORT_VID1_REMOTEID:
            return "PORT_VID1_REMOTEID";

        case FM_FLUSH_MODE_VID1_REMOTEID:
            return "VID1_REMOTEID";

        case FM_FLUSH_MODE_REMOTEID:
            return "REMOTEID";

        default:
            return "UNKNOWN";

    }   /* end switch (flushMode) */

}   /* end fmFlushModeToText */




/*****************************************************************************/
/** fmFlushModeToTextV2
 * \ingroup intAddr
 *
 * \desc            Returns a textual representation of the MA Table
 *                  flush mode, or "NONE" if the update is not a flush.
 *
 * \param[in]       workType is the type of update to perform.
 *
 * \param[in]       flushMode is the MA Table flush mode.
 *
 * \return          Pointer to a string representation of entryState.
 *
 *****************************************************************************/
fm_text fmFlushModeToTextV2(fm_int workType, fm_int flushMode)
{

    return
        (workType == FM_UPD_FLUSH_ADDRESSES) ?
        fmFlushModeToText(flushMode) :
        "NONE";

}   /* end fmFlushModeToTextV2 */




/*****************************************************************************/
/** fmMacSourceToText
 * \ingroup intAddr
 *
 * \desc            Returns a textual representation of a MAC address
 *                  source type.
 *
 * \param[in]       srcType is the MAC address source type.
 *
 * \return          Pointer to a string representation of srcType.
 *
 *****************************************************************************/
fm_text fmMacSourceToText(fm_int srcType)
{

    switch (srcType)
    {
        case FM_MAC_SOURCE_UNKNOWN:
            return "UNKNOWN";

        case FM_MAC_SOURCE_TCN_LEARNED:
            return "LEARNED";

        case FM_MAC_SOURCE_TCN_MOVED:
            return "MOVED";

        case FM_MAC_SOURCE_API_ADDED:
            return "ADDED";

        default:
            return "INVALID";

    }   /* end switch (srcType) */

}   /* end fmMacSourceToText */




/*****************************************************************************/
/** fmReadEntryAtIndex
 * \ingroup intAddr
 *
 * \desc            Reads a MAC address table entry from the hardware registers
 *                  and converts it to an internal MAC address data structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index is the MAC address table index of the entry that is
 *                  to be read.
 *
 * \param[out]      entry points to an internal MAC address data structure in
 *                  which this function will store the information retrieved
 *                  from the hardware registers.
 *
 * \return          FM_OK is successful
 *
 *****************************************************************************/
fm_status fmReadEntryAtIndex(fm_int                   sw,
                             fm_uint32                index,
                             fm_internalMacAddrEntry *entry)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d index=%u entry=%p\n",
                         sw,
                         index,
                         (void *) entry);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(status, switchPtr->ReadEntryAtIndex, sw, index, entry);

    if (status == FM_OK)
    {
        if (FM_IS_TEST_TRACE_ADDRESS(entry->macAddress))
        {
            FM_LOG_PRINT("Read: sw=%d "
                         "index=%u "
                         "MAC=" FM_FORMAT_ADDR " "
                         "VLAN=%hu/%hu "
                         "state=%d (%s) "
                         "dMask=%08x "
                         "port=%d "
                         "trig=%u\n",
                         sw,
                         index,
                         entry->macAddress,
                         entry->vlanID,
                         entry->vlanID2,
                         entry->state,
                         fmEntryStateToText(entry->state),
                         entry->destMask,
                         entry->port,
                         entry->trigger);
        }

    }   /* end if (status == FM_OK) */

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, status);

}   /* end fmReadEntryAtIndex */




/*****************************************************************************/
/** fmWriteEntryAtIndex
 * \ingroup intAddr
 *
 * \desc            Converts an MA Table entry structure to register values
 *                  and writes it to the hardware registers.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       index is the MA Table index at which the entry is to be
 *                  written.
 *
 * \param[in]       entry points to the MA Table entry structure to be
 *                  converted and written to the hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmWriteEntryAtIndex(fm_int                   sw,
                              fm_uint32                index,
                              fm_internalMacAddrEntry *entry)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d index=%u entry=%p\n",
                         sw,
                         index,
                         (void *) entry);

    switchPtr = GET_SWITCH_PTR(sw);

    if (FM_IS_TEST_TRACE_ADDRESS(entry->macAddress))
    {
        FM_LOG_PRINT("Write: sw=%d "
                     "index=%u "
                     "MAC=" FM_FORMAT_ADDR " "
                     "VLAN=%hu/%hu "
                     "state=%d (%s) "
                     "dMask=%08x "
                     "port=%d "
                     "trig=%u "
                     "secure=%d\n",
                     sw,
                     index,
                     entry->macAddress,
                     entry->vlanID,
                     entry->vlanID2,
                     entry->state,
                     fmEntryStateToText(entry->state),
                     entry->destMask,
                     entry->port,
                     entry->trigger,
                     entry->secure);
    }

    FM_API_CALL_FAMILY(status, switchPtr->WriteEntryAtIndex, sw, index, entry);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, status);

}   /* end fmWriteEntryAtIndex */




/*****************************************************************************/
/** fmAllocateAddressTableDataStructures
 * \ingroup intAddr
 *
 * \desc            Allocates the internal storage used for managing the MAC 
 *                  address table.
 *
 * \param[in]       switchPtr points to the switch's state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmAllocateAddressTableDataStructures(fm_switch *switchPtr)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "switchPtr=%p<sw=%d>\n",
                 (void *) switchPtr,
                 switchPtr->switchNumber);

    FM_API_CALL_FAMILY(status, switchPtr->AllocAddrTableData, switchPtr);
    
    if (status == FM_ERR_UNSUPPORTED)
    {
        status = FM_OK;
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmAllocateAddressTableDataStructures */




/*****************************************************************************/
/** fmFreeAddressTableDataStructures
 * \ingroup intAddr
 *
 * \desc            Frees the internal storage used for managing the MAC 
 *                  address table.
 *
 * \note            This function assumes that the relevant pointer is 
 *                  already protected by some mechanism.
 *
 * \param[in]       switchPtr points to the switch's state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeAddressTableDataStructures(fm_switch *switchPtr)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "switchPtr=%p<sw=%d>\n",
                 (void *) switchPtr,
                 switchPtr ? switchPtr->switchNumber : -1);

    if (switchPtr == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }
    
    FM_API_CALL_FAMILY(status, switchPtr->FreeAddrTableData, switchPtr);
    
    if (status == FM_ERR_UNSUPPORTED)
    {
        status = FM_OK;
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmFreeAddressTableDataStructures */




/*****************************************************************************/
/** fmInitAddressTable
 * \ingroup intAddr
 *
 * \desc            Initializes the local copy of the MA Table.
 *
 * \param[in]       switchPtr points to the switch's state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitAddressTable(fm_switch *switchPtr)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "switchPtr=%p<sw=%d>\n",
                 (void *) switchPtr,
                 switchPtr->switchNumber);

    FM_API_CALL_FAMILY(status, switchPtr->InitAddressTable, switchPtr);
    
    if (status == FM_ERR_UNSUPPORTED)
    {
        status = FM_OK;
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmInitAddressTable */




/*****************************************************************************/
/** fmAddAddress
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Adds an entry to the MA Table.  Note that the VLAN field
 *                  of the entry structure is overridden if the spanning tree
 *                  mode (FM2000, FM4000) or vlan learning mode (FM6000,
 *                  FM10000) is shared.
 *                                                                      \lb\lb
 *                  Addition of MAC addresses to multicast group logical ports
 *                  is not permitted using this function.  The multicast
 *                  group functions must be used to add a multicast address
 *                  to a multicast group.
 *
 * \note            If the address to be added already exists in the MA Table,
 *                  it will be overwritten by the new entry, unless the new
 *                  entry is dynamic and the existing entry is static. To
 *                  overwrite a static entry with a dynamic entry, first
 *                  delete the existing static entry with a call to
 *                  ''fmDeleteAddress''.
 *
 * \note            On FM3000 and FM4000 devices only, if the entry type is
 *                  ''FM_ADDRESS_DYNAMIC'', the destMask member of the
 *                  ''fm_macAddressEntry'' structure may specify only a single
 *                  port, otherwise it must be set to FM_DESTMASK_UNUSED and
 *                  the destination specified with the port structure member.
 *
 * \note            On FM6000 and FM10000 devices, the destMask member of 
 *                  ''fm_macAddressEntry'' must be set to FM_DESTMASK_UNUSED.
 *                  Only the "port" structure member may be used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   entry points to an ''fm_macAddressEntry'' structure that
 *                  describes the MA Table entry to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an attempt was made to use
 *                  a multi-port destMask for a dynamic entry on a non-FM2000
 *                  device, or if the entry type is incorrect, or if entry
 *                  is not valid.
 * \return          FM_ERR_INVALID_VLAN if VLANID is out of range
 * \return          FM_ERR_ADDR_BANK_FULL if no room in MA Table for this
 *                  address.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if an attempt was made to
 *                  add a MAC address to a multicast group.
 *
 *****************************************************************************/
fm_status fmAddAddress(fm_int sw, fm_macAddressEntry *entry)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ADDR,
                      "sw=%d "
                      "macAddress=" FM_FORMAT_ADDR " "
                      "vlanID=%d/%d "
                      "destMask=0x%08x "
                      "port=%d "
                      "type=%d "
                      "remoteID=%d "
                      "remoteMac=%d\n",
                      sw,
                      (entry != NULL) ? entry->macAddress : 0L,
                      (entry != NULL) ? entry->vlanID : 0,
                      (entry != NULL) ? entry->vlanID2 : 0,
                      (entry != NULL) ? entry->destMask : 0,
                      (entry != NULL) ? entry->port : -1, 
                      (entry != NULL) ? entry->type : -1,
                      (entry != NULL) ? entry->remoteID : 0,
                      (entry != NULL) ? entry->remoteMac : 0  );


    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    if (entry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    if ( VLAN_OUT_OF_BOUNDS(entry->vlanID) )
    {
        err = FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VLAN, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    FM_API_CALL_FAMILY(err, switchPtr->AddAddress, sw, entry);

ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmAddAddress */




/*****************************************************************************/
/** fmAddAddressInternal
 * \ingroup intAddr
 *
 * \desc            Adds an entry to the MA table. Note that the VLAN field
 *                  of the entry structure is overridden to zero if the
 *                  current vlan mode is shared. This override is reflected
 *                  in the argument structure as well as the internal cache.
 *                  Also, the 'valid' member of the specified entry structure 
 *                  is ignored. The entry written is marked as valid.
 *
 *                  Will return an FM_ERR_ADDR_BANK_FULL error status and will
 *                  not add the address to the table if the request specifies
 *                  a static address and the bin is full of static addresses.
 *                  If the bin is full but does contain some dynamic
 *                  addresses, then the API will remove a dynamic address
 *                  and add the new static address. The API will return the
 *                  status FM_OK in this case.
 *
 *                  Will return an FM_ERR_ADDR_BANK_FULL error status and will
 *                  not add the address to the table if the request specifies
 *                  a dynamic address and the bin is full, regardless of
 *                  whether the bin is full of static addresses, dynamic 
 *                  addresses, or a mixture of both.
 *
 *                  Note that a new static address whose MAC and VLAN match
 *                  an existing static address will override the existing
 *                  static address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   entry points to the entry structure to add.
 *
 * \param[in]       trigger is the trigger to write to the entry, or -1 to use
 *                  the default. Ignored on FM6000.
 *
 * \param[in]       updateHw should be TRUE to update the hardware table 
 *                  registers. Ignored on FM6000 as the hardware is always
 *                  updated.
 *
 * \param[in]       bank should be set to -1 to find any available bank, 
 *                  otherwise explicitly set bank. Ignored on FM6000 as all 
 *                  callers always specify -1 anyway.
 *
 * \param[in,out]   numUpdates points to a variable containing the number of
 *                  updates stored in the event buffer. Will be updated if
 *                  an event is added to the buffer.
 *
 * \param[in,out]   outEvent points to a variable containing a pointer to
 *                  the buffer to which the learning and aging events
 *                  should be added. May be NULL, in which case an event
 *                  buffer will be allocated if one is needed. Will be
 *                  updated to point to the new event buffer.
 *
 * \return          FM_OK if successful, or error code (see fm_errno.h)
 * \return          FM_ERR_ADDR_BANK_FULL if there is no room in the MA table
 *                  for the specified address.
 *
 *****************************************************************************/
fm_status fmAddAddressInternal(fm_int                      sw,
                               fm_macAddressEntry*         entry,
                               fm_uint32                   trigger,
                               fm_bool                     updateHw,
                               fm_int                      bank,
                               fm_uint32*                  numUpdates,
                               fm_event**                  outEvent)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_bool    complete;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d entry=%p trigger=0x%x updateHw=%d bank=%d, "
                 "macAddress=" FM_FORMAT_ADDR ", vlan=%u/%u, type=%u, "
                 "destMask=0x%x, port=%d, type=%d, remoteID=%d remoteMac=%d\n",
                 sw,
                 (void *) entry,
                 trigger,
                 updateHw,
                 bank,
                 (entry != NULL) ? entry->macAddress : 0,
                 (entry != NULL) ? entry->vlanID : 0,
                 (entry != NULL) ? entry->vlanID2 : 0,
                 (entry != NULL) ? entry->type : 0,
                 (entry != NULL) ? entry->destMask : 0,
                 (entry != NULL) ? entry->port : -1,
                 (entry != NULL) ? entry->type : -1,
                 (entry != NULL) ? entry->remoteID : -1,
                 (entry != NULL) ? entry->remoteMac : -1);

    if (entry == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Do pre-processing. For SWAG, this will be the
     * only processing. The complete flag indicates
     * to do no further processing.
     **************************************************/
    
    FM_API_CALL_FAMILY(err,
                       switchPtr->AddAddressToTablePre,
                       sw, 
                       entry, 
                       trigger, 
                       updateHw, 
                       bank, 
                       &complete);
    
    if (err == FM_OK && complete)
    {
        goto EXIT;
    }
    
    if ( (err != FM_OK) && (err != FM_ERR_UNSUPPORTED) )
    {
        goto EXIT;
    }

    switch (entry->type)
    {
        case FM_ADDRESS_STATIC:
        case FM_ADDRESS_DYNAMIC:
        case FM_ADDRESS_SECURE_STATIC:
        case FM_ADDRESS_SECURE_DYNAMIC:
        case FM_ADDRESS_PROVISIONAL:
            break;

        default:
            err = FM_ERR_INVALID_ENTRYTYPE;
            goto EXIT;

    }   /* end switch (entry->type) */

    if (FM_IS_TEST_TRACE_ADDRESS(entry->macAddress))
    {
        FM_LOG_PRINT("fmAddAddressInternal(%d): " 
                     "adding mac address " FM_FORMAT_ADDR ":\n"
                     "    vlan=%d/%d, type=%s, MAC secure=%s, "
                     "destMask=%08X, port=%d, age=%d, "
                     "trig=%d, updateHw=%d, bank=%d\n",
                     sw, entry->macAddress,
                     entry->vlanID,
                     entry->vlanID2,
                     FM_IS_ADDR_TYPE_STATIC(entry->type) ? "static" : "dynamic",
                     FM_IS_ADDR_TYPE_SECURE(entry->type) ? "secured" : "unsecured",
                     entry->destMask,
                     entry->port,
                     entry->age,
                     trigger,
                     updateHw,
                     bank);
    }
    

    /**************************************************
     * Get the learning FID. This will either be the
     * VLAN's unique FID or the shared learning FID.
     **************************************************/
        
    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetLearningFID, 
                       sw, 
                       entry->vlanID, 
                       &entry->vlanID);
    
    if (err != FM_OK)
    {
        goto EXIT;
    }
    
    FM_TAKE_L2_LOCK(sw);
        
    /* Figure out where to store the address and store it. */
    FM_API_CALL_FAMILY(err,
                       switchPtr->AssignTableEntry,
                       sw, 
                       entry, 
                       bank,
                       trigger,
                       updateHw,
                       numUpdates,
                       outEvent);

    FM_DROP_L2_LOCK(sw);
    
EXIT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fmAddAddressInternal */




/*****************************************************************************/
/** fmAddAddressToTable
 * \ingroup intAddr
 *
 * \desc            Adds an entry to the MA table. Note that the VLAN field
 *                  of the entry structure is overridden to zero if the
 *                  current vlan mode is shared. This override is reflected
 *                  in the argument structure as well as the internal cache.
 *                  Also, the 'valid' member of the specified entry structure 
 *                  is ignored. The entry written is marked as valid.
 *
 *                  Will return an FM_ERR_ADDR_BANK_FULL error status and will
 *                  not add the address to the table if the request specifies
 *                  a static address and the bin is full of static addresses.
 *                  If the bin is full but does contain some dynamic
 *                  addresses, then the API will remove a dynamic address
 *                  and add the new static address. The API will return the
 *                  status FM_OK in this case.
 *
 *                  Will return an FM_ERR_ADDR_BANK_FULL error status and will
 *                  not add the address to the table if the request specifies
 *                  a dynamic address and the bin is full, regardless of
 *                  whether the bin is full of static addresses, dynamic 
 *                  addresses, or a mixture of both.
 *
 *                  Note that a new static address whose MAC and VLAN match
 *                  an existing static address will override the existing
 *                  static address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the entry structure to add (will be cloned)
 *
 * \param[in]       trigger is the trigger to write to the entry, or -1 to use
 *                  the default.
 *
 * \param[in]       updateHw should be TRUE to update the hardware table 
 *                  registers.
 *
 * \param[in]       bank should be set to -1 to find any available bank, 
 *                  otherwise explicitly set bank.
 *
 * \return          FM_OK if successful, or error code (see fm_errno.h)
 * \return          FM_ERR_ADDR_BANK_FULL if there is no room in the MA table
 *                  for the specified address.
 *
 *****************************************************************************/
fm_status fmAddAddressToTable(fm_int                sw,
                              fm_macAddressEntry*   entry,
                              fm_uint32             trigger,
                              fm_bool               updateHw,
                              fm_int                bank)
{
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmAddAddressToTableInternal(sw, entry, trigger, updateHw, bank);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmAddAddressToTable */




/*****************************************************************************/
/** fmAddAddressToTableInternal
 * \ingroup intAddr
 *
 * \desc            Adds an entry to the MA table. Note that the VLAN field
 *                  of the entry structure is overridden to zero if the
 *                  current vlan mode is shared. This override is reflected
 *                  in the argument structure as well as the internal cache.
 *                  Also, the 'valid' member of the specified entry structure 
 *                  is ignored. The entry written is marked as valid.
 *
 *                  Will return an FM_ERR_ADDR_BANK_FULL error status and will
 *                  not add the address to the table if the request specifies
 *                  a static address and the bin is full of static addresses.
 *                  If the bin is full but does contain some dynamic
 *                  addresses, then the API will remove a dynamic address
 *                  and add the new static address. The API will return the
 *                  status FM_OK in this case.
 *
 *                  Will return an FM_ERR_ADDR_BANK_FULL error status and will
 *                  not add the address to the table if the request specifies
 *                  a dynamic address and the bin is full, regardless of
 *                  whether the bin is full of static addresses, dynamic 
 *                  addresses, or a mixture of both.
 *
 *                  Note that a new static address whose MAC and VLAN match
 *                  an existing static address will override the existing
 *                  static address.
 *
 *                  This function can be called only internally by API.
 *                  It does not have switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the entry structure to add (will be cloned)
 *
 * \param[in]       trigger is the trigger to write to the entry, or -1 to use
 *                  the default.
 *
 * \param[in]       updateHw should be TRUE to update the hardware table 
 *                  registers.
 *
 * \param[in]       bank should be set to -1 to find any available bank, 
 *                  otherwise explicitly set bank.
 *
 * \return          FM_OK if successful, or error code (see fm_errno.h)
 * \return          FM_ERR_ADDR_BANK_FULL if there is no room in the MA table
 *                  for the specified address.
 *
 *****************************************************************************/
fm_status fmAddAddressToTableInternal(fm_int                sw,
                                      fm_macAddressEntry*   entry,
                                      fm_uint32             trigger,
                                      fm_bool               updateHw,
                                      fm_int                bank)
{
    fm_event*   event = NULL;
    fm_uint32   numUpdates = 0;
    fm_status   err;

    err = fmAddAddressInternal(sw,
                               entry,
                               trigger,
                               updateHw,
                               bank,
                               &numUpdates,
                               &event);

    if (numUpdates != 0)
    {
        fmSendMacUpdateEvent(sw,
                             &fmRootApi->eventThread,
                             &numUpdates,
                             &event,
                             FALSE);
    }

    if (event != NULL)
    {
        fmReleaseEvent(event);
    }

    return err;

}   /* end fmAddAddressToTableInternal */





/*****************************************************************************/
/** fmGetAddress
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Returns an ''fm_macAddressEntry'' structure for the MA 
 *                  Table entry that matches the specified MAC address and 
 *                  VLAN ID. In shared spanning tree mode (FM2000, FM4000)
 *                  or shared vlan learning mode (FM6000, FM10000), the
 *                  supplied VLAN ID is ignored.
 *                                                                      \lb\lb
 *                  See also ''fmGetAddressV2''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       address is the MAC address to look up.
 *
 * \param[in]       vlanID is the VLAN number to look up.
 *
 * \param[out]      entry points to a caller-allocated ''fm_macAddressEntry'' 
 *                  structure to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory available.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 *
 *****************************************************************************/
fm_status fmGetAddress(fm_int              sw,
                       fm_macaddr          address,
                       fm_int              vlanID,
                       fm_macAddressEntry *entry)
{
    fm_status err;

    err = fmGetAddressV2(sw, address, vlanID, 0, entry);

    return err;

}   /* end fmGetAddress */




/*****************************************************************************/
/** fmGetAddressV2
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Returns an ''fm_macAddressEntry'' structure for the MA 
 *                  Table entry that matches the specified MAC address and 
 *                  VLAN IDs. In shared spanning tree mode (FM2000, FM4000)
 *                  or shared vlan learning mode (FM6000, FM10000), the
 *                  supplied VLAN IDs are ignored.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       address is the MAC address to look up.
 *
 * \param[in]       vlanID is the VLAN number to look up.
 * 
 * \param[in]       vlanID2 is the second VLAN number to look up (ignored on
 *                  non-FM6000 devices).
 *
 * \param[out]      entry points to a caller-allocated ''fm_macAddressEntry'' 
 *                  structure to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory available.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 *
 *****************************************************************************/
fm_status fmGetAddressV2(fm_int              sw,
                         fm_macaddr          address,
                         fm_int              vlanID,
                         fm_int              vlanID2,
                         fm_macAddressEntry *entry)
{
    fm_status               err;
    fm_status               retCode;
    fm_uint32               addr;
    fm_uint16 *             indexes = NULL;
    fm_int                  bank;
    fm_internalMacAddrEntry tblentry;
    fm_switch *             switchPtr;
    fm_uint16               learningFID;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR,
                     "sw=%d address=" FM_FORMAT_ADDR " vlanID=%d\n",
                     sw,
                     address,
                     vlanID);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    retCode = FM_ERR_ADDR_NOT_FOUND;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->GetAddressOverride != NULL)
    {
        retCode = switchPtr->GetAddressOverride(sw, address, vlanID, vlanID2, entry);
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err, switchPtr->GetLearningFID, sw, vlanID, &learningFID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
    vlanID = learningFID;
    
    indexes = (fm_uint16 *) fmAlloc( sizeof(fm_uint16) * switchPtr->macTableBankCount );
    
    if (indexes == NULL)
    {
        retCode = FM_ERR_NO_MEM;
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err, 
                       switchPtr->ComputeAddressIndex, 
                       sw, 
                       address, 
                       vlanID, 
                       vlanID2,
                       indexes);

    if (err != FM_OK)
    {
        retCode = err;
        goto ABORT;
    }

    for (bank = switchPtr->macTableBankCount - 1 ; bank >= 0 ; bank--)
    {
        addr = indexes[bank];

        err = fmReadEntryAtIndex(sw, addr, &tblentry);
    
        if (err != FM_OK)
        {
            retCode = err;
            break;
        }

        if ( (tblentry.state != FM_MAC_ENTRY_STATE_INVALID) &&
             (tblentry.macAddress == address) &&
             (tblentry.vlanID == vlanID) )
        {
            /* Found the requested entry. */
            FM_API_CALL_FAMILY(err,
                               switchPtr->FillInUserEntryFromTable,
                               sw,
                               &tblentry,
                               entry);
            retCode = FM_OK;
            break;
        }
    }

ABORT:
    if (indexes != NULL)
    {
        fmFree(indexes);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, retCode);

}   /* end fmGetAddressV2 */




/*****************************************************************************/
/** fmGetAddressInternal
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM10000
 *
 * \desc            Returns a pointer to the internal address table entry
 *                  that matches the MAC address and VLAN ID supplied in the
 *                  arguments.  In shared spanning tree mode, the supplied
 *                  VLAN ID is ignored (0 is used instead).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       address is the MAC address to look up.
 *
 * \param[in]       vlanID is the VLAN number to look up.
 *
 * \param[out]      entry points to storage where the pointer to the internal
 *                  MA Table entry structure is to be filled in by this
 *                  function.
 *
 * \param[out]      addrIndex points to storage where the maTable index will
 *                  be placed.  A NULL pointer will cause the index to not
 *                  be returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 *
 *****************************************************************************/
fm_status fmGetAddressInternal(fm_int                    sw,
                               fm_macaddr                address,
                               fm_int                    vlanID,
                               fm_internalMacAddrEntry **entry,
                               fm_uint32 *               addrIndex)
{
    fm_status                err = FM_OK;
    fm_uint32                addr;
    fm_uint16 *              indexes = NULL;
    fm_int                   bank;
    fm_internalMacAddrEntry *tblentry;
    fm_switch *              switchPtr;
    fm_uint16                learningFID;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_ADDR,
                           switchPtr->switchFamily != FM_SWITCH_FAMILY_FM6000,
                           err = FM_ERR_ASSERTION_FAILED,
                           "Function not supported for FM6000\n");

    FM_API_CALL_FAMILY(err, switchPtr->GetLearningFID, sw, vlanID, &learningFID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
    vlanID = learningFID;

    indexes = (fm_uint16 *) fmAlloc( sizeof(fm_uint16) * switchPtr->macTableBankCount );
    
    if (indexes == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err, 
                       switchPtr->ComputeAddressIndex, 
                       sw, 
                       address, 
                       vlanID, 
                       0,
                       indexes);


    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = FM_ERR_ADDR_NOT_FOUND;

    FM_TAKE_L2_LOCK(sw);

    for (bank = switchPtr->macTableBankCount - 1 ; bank >= 0 ; bank--)
    {
        addr     = indexes[bank];
        tblentry = &switchPtr->maTable[addr];

        if ((tblentry->state != FM_MAC_ENTRY_STATE_INVALID) &&
            (tblentry->macAddress == address) &&
            (tblentry->vlanID == vlanID))
        {
            tblentry->entryType = FM_MAC_ENTRY_TYPE_CACHE;
            *entry = tblentry;

            if (addrIndex != NULL)
            {
                *addrIndex = addr;
            }

            err = FM_OK;
            break;
        }
    }

    FM_DROP_L2_LOCK(sw);

ABORT:
    if (indexes != NULL)
    {
        fmFree(indexes);
    }

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmGetAddressInternal */




/*****************************************************************************/
/** fmDeleteAddress
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes an address from the MA Table.
 * 
 * \note            Deletion of MAC addresses from multicast group logical
 *                  ports is not permitted using this function.
 *                  To delete an address from a multicast group use the
 *                  multicast group subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the MA Table entry user structure
 *                  containing the address and vlan combination to be
 *                  deleted from the MA Table (other members of the structure
 *                  are ignored).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if entry is not valid.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if an attempt is made to
 *                  delete a MAC address from a multicast group.
 *
 *****************************************************************************/
fm_status fmDeleteAddress(fm_int sw, fm_macAddressEntry *entry)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ADDR,
                      "sw=%d "
                      "entry->macAddress=" FM_FORMAT_ADDR " "
                      "entry->vlanID=%d/%d\n",
                      sw,
                      (entry != NULL) ? entry->macAddress : 0L,
                      (entry != NULL) ? entry->vlanID : 0,
                      (entry != NULL) ? entry->vlanID2 : 0 );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (entry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    FM_API_CALL_FAMILY(err, switchPtr->DeleteAddressPre, sw, entry);
    
    if ( (err == FM_OK) || (err == FM_ERR_UNSUPPORTED) )
    {
        err = fmDeleteAddressFromTable(sw, entry, FALSE, TRUE, -1);
    }
    
ABORT:
    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmDeleteAddress */




/*****************************************************************************/
/** fmDeleteAddressFromTable
 * \ingroup intAddr
 *
 * \desc            Resets the valid bit for the given entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the user structure entry to delete (fields
 *                  used for calculating hash into the cache of the MA table).
 *
 * \param[in]       overrideMspt indicates whether to ignore shared VLAN mode
 *                  and unconditionally use the specified vlanID.
 *
 * \param[in]       updateHw should be TRUE to update the hardware registers.
 *
 * \param[in]       bank should be set to -1 to find any available bank,
 *                  otherwise explicitly set bank.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 *
 *****************************************************************************/
fm_status fmDeleteAddressFromTable(fm_int              sw,
                                   fm_macAddressEntry *entry,
                                   fm_bool             overrideMspt,
                                   fm_bool             updateHw,
                                   fm_int              bank)
{
    fm_status  err;
    fm_uint16 *indexes = NULL;
    fm_uint16  vlanID;
    fm_switch *switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (FM_IS_TEST_TRACE_ADDRESS(entry->macAddress))
    {
        FM_LOG_PRINT("fmDeleteAddressFromTable(%d): " 
                     "deleting mac address " FM_FORMAT_ADDR ":\n"
                     "    vlan=%d/%d, type=%s, "
                     "destMask=%08X, port=%d, age=%d, overrideMspt=%d\n"
                     "    updateHw=%d, bank=%d\n",
                     sw,
                     entry->macAddress,
                     entry->vlanID,
                     entry->vlanID2,
                     FM_IS_ADDR_TYPE_STATIC(entry->type) ? "static" : "dynamic",
                     entry->destMask,
                     entry->port,
                     entry->age,
                     overrideMspt,
                     updateHw,
                     bank);
    }

    if (switchPtr->DeleteAddressOverride != NULL)
    {
        err = switchPtr->DeleteAddressOverride(sw,
                                               entry,
                                               overrideMspt,
                                               updateHw,
                                               bank);
        goto ABORT;
    }

    /* It's possible that in shared VLAN learning mode, we will want
     * to search for an entry with vlanID equal to 0 regardless of what the
     * user specified in the entry.  However, if the user just switched
     * to shared VLAN learning mode, maybe they actually want to delete
     * entries with non-zero VLAN IDs.  Of course, in switching to shared
     * VLAN mode, we should probably be re-writing the MA Table and VLAN
     * table, so this may be irrelevant, but the caller can specify
     * the intended behavior with the overrideMspt argument.
     */
    vlanID = entry->vlanID;
    
    if (!overrideMspt)
    {
        FM_API_CALL_FAMILY(err, switchPtr->GetLearningFID, sw, vlanID, &vlanID);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
    }

    indexes = (fm_uint16 *) fmAlloc( sizeof(fm_uint16) * switchPtr->macTableBankCount );
    
    if (indexes == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err, 
                       switchPtr->ComputeAddressIndex, 
                       sw, 
                       entry->macAddress, 
                       vlanID, 
                       entry->vlanID2,
                       indexes);


    if (err != FM_OK)
    {
        goto ABORT;
    }

    FM_API_CALL_FAMILY(err, 
                       switchPtr->FindAndInvalidateAddress, 
                       sw, 
                       entry->macAddress, 
                       vlanID, 
                       entry->vlanID2,
                       bank,
                       indexes,
                       updateHw);

    if (err != FM_OK)
    {
        FM_LOG_DEBUG2(FM_LOG_CAT_ADDR,
                      "ERROR: could not find address " FM_FORMAT_ADDR
                      ", index=0%x\n",
                      entry->macAddress, 
                      indexes[0]);
    }

ABORT:
    if (indexes != NULL)
    {
        fmFree(indexes);
    }

    return err;

}   /* end fmDeleteAddressFromTable */




/*****************************************************************************/
/** fmGetAddressTable
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves a copy of the entire MA Table.
 * 
 * \note            Deprecated in favor of ''fmGetAddressTableExt''.
 *
 * \note            On FM4000 devices, may not be used when TCAM MAC Table 
 *                  offload is enabled with the api.FM4000.MATable.offloadEnable 
 *                  API property.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      nEntries points to caller-allocated storage where this
 *                  function is to store the number of MA Table entries
 *                  retrieved. May not be NULL.
 *
 * \param[out]      entries points to an array of ''fm_macAddressEntry''
 *                  structures that will be filled in by this function with
 *                  MA Table entries.  The array must be large enough to
 *                  hold the maximum number of possible MA Table entries
 *                  (FM2000_MAX_ADDR, FM4000_MAX_ADDR, FM6000_MAX_ADDR or 
 *                  ''FM10000_MAX_ADDR'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if an attempt is made to use this
 *                  function when TCAM MAC Table offload is enabled. In that
 *                  case, use ''fmGetAddressTableExt'' instead.
 *
 *****************************************************************************/
fm_status fmGetAddressTable(fm_int              sw,
                            fm_int *            nEntries,
                            fm_macAddressEntry *entries)
{
    fm_switch *    switchPtr;
    fm_status      err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR,
                     "sw=%d nEntries=%p entries=%p\n",
                     sw,
                     (void *) nEntries,
                     (void *) entries);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);
    
    err = fmGetAddressTableExt(sw, nEntries, entries, switchPtr->macTableSize);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmGetAddressTable */




/*****************************************************************************/
/** fmGetAddressTableExt
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves a copy of the entire MA Table.
 * 
 *                  Note that calling this function with entries equal to NULL
 *                  will only return the number of MA Table entries used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      nEntries points to caller-allocated storage where this
 *                  function is to store the number of MA Table entries
 *                  retrieved. May not be NULL.
 *
 * \param[out]      entries points to an array of ''fm_macAddressEntry''
 *                  structures that will be filled in by this function with
 *                  MA Table entries.  The array must be large enough to
 *                  hold maxEntries number of MA Table entries. entries may
 *                  be set to NULL, in which case only nEntries will be
 *                  filled in by this function.
 *
 * \param[in]       maxEntries is the maximum number of addresses that the
 *                  entries array can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * 
 *****************************************************************************/
fm_status fmGetAddressTableExt(fm_int              sw,
                               fm_int *            nEntries,
                               fm_macAddressEntry *entries,
                               fm_int              maxEntries)
{
    fm_switch *switchPtr;
    fm_status  err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR,
                     "sw=%d nEntries=%p entries=%p\n",
                     sw,
                     (void *) nEntries,
                     (void *) entries);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (nEntries != NULL)
    {
        FM_API_CALL_FAMILY(err,
                           switchPtr->GetAddressTable,
                           sw,
                           nEntries,
                           entries,
                           maxEntries);
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmGetAddressTableExt */




/*****************************************************************************/
/** fmDeleteAllAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes all addresses from the MA Table before
 *                  returning to the caller, whether they are dynamic or
 *                  static. Age events are not reported.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteAllAddresses(fm_int sw)
{
    fm_status err; 

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    err = fmDeleteAllAddressesInternal(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmDeleteAllAddresses */




/*****************************************************************************/
/** fmDeleteAllAddressesInternal
 * \ingroup intAddr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes all addresses from the MA Table before
 *                  returning to the caller, whether they are dynamic or
 *                  static. Age events are not reported.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteAllAddressesInternal(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    switchPtr = GET_SWITCH_PTR(sw);

    fmDbgDiagCountIncr(sw, FM_CTR_DELETE_ALL, 1);

    /* Clear MA Table */
    FM_API_CALL_FAMILY(err, switchPtr->DeleteAllAddresses, sw, FALSE);
    
    return err;

}   /* end fmDeleteAllAddressesInternal */




/*****************************************************************************/
/** fmDeleteAllDynamicAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes all dynamic addresses from the MA Table before
 *                  returning to the caller. Static entries are left untouched. 
 *                  Age events are not reported. (See also 
 *                  ''fmFlushAllDynamicAddresses''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteAllDynamicAddresses(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    fmDbgDiagCountIncr(sw, FM_CTR_DELETE_ALL_DYNAMIC, 1);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DeleteAllAddresses, sw, TRUE);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmDeleteAllDynamicAddresses */




/*****************************************************************************/
/** fmFlushAllDynamicAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dispatch an independent thread to delete all dynamic
 *                  addresses from the MA Table (this function returns
 *                  immediately, before the MA Table entries have actually been
 *                  deleted). Static entries are left untouched. An age event
 *                  is reported to the application for each MA Table entry
 *                  deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if function not supported.
 * \return          FM_ERR_EVENT_QUEUE_FULL if the independent thread's event
 *                  queue is full.
 *
 *****************************************************************************/
fm_status fmFlushAllDynamicAddresses(fm_int sw)
{
    fm_status      err;
    fm_flushParams params;

    FM_CLEAR(params);

    err = fmFlushAddresses(sw, FM_FLUSH_MODE_ALL_DYNAMIC, params);

    return err;

}   /* end fmFlushAllDynamicAddresses */




/*****************************************************************************/
/** fmFlushPortAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dispatch an independent thread to delete all dynamic MA 
 *                  Table entries associated with the specified port (this 
 *                  function returns immediately, before the MA Table entries 
 *                  have actually been deleted). An age event is reported to 
 *                  the application for each MA Table entry deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number to filter on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if function not supported.
 * \return          FM_ERR_EVENT_QUEUE_FULL if the independent thread's event
 *                  queue is full.
 *
 *****************************************************************************/
fm_status fmFlushPortAddresses(fm_int sw, fm_uint port)
{
    fm_status      err;
    fm_flushParams params;

    memset(&params, 0, sizeof(params));
    params.port = port;

    err = fmFlushAddresses(sw, FM_FLUSH_MODE_PORT, params);

    return err;

}   /* end fmFlushPortAddresses */




/*****************************************************************************/
/** fmFlushVlanAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dispatch an independent thread to delete all dynamic MA 
 *                  Table entries associated with the specified VLAN (this 
 *                  function returns immediately, before the MA Table entries 
 *                  have actually been deleted). An age event is reported to 
 *                  the application for each MA Table entry deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlan is the VLAN number to filter on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if function not supported.
 * \return          FM_ERR_EVENT_QUEUE_FULL if the independent thread's event
 *                  queue is full.
 *
 *****************************************************************************/
fm_status fmFlushVlanAddresses(fm_int sw, fm_uint vlan)
{
    fm_status      err;
    fm_flushParams params;

    memset(&params, 0, sizeof(params));
    params.vid1 = vlan;

    err = fmFlushAddresses(sw, FM_FLUSH_MODE_VLAN, params);

    return err;

}   /* end fmFlushVlanAddresses */




/*****************************************************************************/
/** fmFlushPortVlanAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dispatch an independent thread to delete all dynamic MA 
 *                  Table entries associated with the specified port and VLAN 
 *                  (this function returns immediately, before the MA Table 
 *                  entries have actually been deleted). An age event is 
 *                  reported to the application for each MA Table entry deleted. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number to filter on.
 *
 * \param[in]       vlan is the VLAN number to filter on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if function not supported.
 * \return          FM_ERR_EVENT_QUEUE_FULL if the independent thread's event
 *                  queue is full.
 *
 *****************************************************************************/
fm_status fmFlushPortVlanAddresses(fm_int sw, fm_uint port, fm_int vlan)
{
    fm_status      err;
    fm_flushParams params;

    memset(&params, 0, sizeof(params));
    params.port = port;
    params.vid1 = vlan;

    err = fmFlushAddresses(sw, FM_FLUSH_MODE_PORT_VLAN, params);

    return err;

}   /* end fmFlushPortVlanAddresses */




/*****************************************************************************/
/** fmFlushAddresses
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dispatch an independent thread to delete all dynamic MA 
 *                  Table entries associated with the specified mode (this 
 *                  function returns immediately, before the MA Table entries 
 *                  have actually been deleted). An age event is reported to 
 *                  the application for each MA Table entry deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mode indicates the set of parameters used to filter
 *                  the MA Table entries to be flushed. See ''fm_flushMode'' 
 *                  for possible values.
 * 
 * \param[in]       params contains the set of parameters indicated by mode.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if mode is not supported by the switch.
 * \return          FM_ERR_INVALID_ARGUMENT if mode is not recognized.
 * \return          FM_ERR_EVENT_QUEUE_FULL if the independent thread's event
 *                  queue is full.
 *
 *****************************************************************************/
fm_status fmFlushAddresses(fm_int sw, fm_flushMode mode, fm_flushParams params)
{
    fm_switch *     switchPtr;
    fm_status       err;
    fm_bool         autoFlush;
    fm_bool         checkPort;
    fm_bool         checkVid1;
    fm_bool         checkVid2;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR,
                     "sw=%d, mode=%d, port=%d, "
                     "vid1=%d, vid2=%d, remoteId=%d\n", 
                     sw, 
                     mode, 
                     params.port,
                     params.vid1,
                     params.vid2,
                     params.remoteId);

    switchPtr = GET_SWITCH_PTR(sw);

    autoFlush = GET_PROPERTY()->maFlushOnVlanChange;

    checkPort = FALSE;
    checkVid1 = FALSE;
    checkVid2 = FALSE;

    if (switchPtr->CheckFlushRequest)
    {
        err = switchPtr->CheckFlushRequest(sw, mode, &params);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
    }

    switch (mode)
    {
        case FM_FLUSH_MODE_ALL_DYNAMIC:
            break;

        case FM_FLUSH_MODE_PORT:
            checkPort = TRUE;
            break;

        case FM_FLUSH_MODE_PORT_VLAN:
            checkPort = TRUE;
            checkVid1 = autoFlush;
            break;

        case FM_FLUSH_MODE_VLAN:
            checkVid1 = autoFlush;
            break;
            
        case FM_FLUSH_MODE_VID1_VID2:
            checkVid1 = autoFlush;
            checkVid2 = autoFlush;
            break;

        case FM_FLUSH_MODE_PORT_VID1_VID2:
            checkPort = TRUE;
            checkVid1 = autoFlush;
            checkVid2 = autoFlush;
            break;

        case FM_FLUSH_MODE_PORT_VID2:
            checkPort = TRUE;
            checkVid2 = autoFlush;
            break;

        case FM_FLUSH_MODE_VID2:
            checkVid2 = autoFlush;
            break;

        case FM_FLUSH_MODE_PORT_VID1_REMOTEID:
            checkPort = TRUE;
            checkVid1 = autoFlush;
            break;

        case FM_FLUSH_MODE_VID1_REMOTEID:
            checkVid1 = autoFlush;
            break;

        case FM_FLUSH_MODE_REMOTEID:
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
            break;

    }   /* end switch (mode) */

    if (checkPort && !IS_VALID_PORT(sw, params.port))
    {
        err = FM_ERR_INVALID_PORT;
        goto ABORT;
    }

    if (checkVid1 && !IS_VALID_VLAN(sw, params.vid1))
    {
        err = FM_ERR_INVALID_VLAN;
        goto ABORT;
    }

    if (checkVid2 && !IS_VALID_VLAN(sw, params.vid2))
    {
        err = FM_ERR_INVALID_VLAN;
        goto ABORT;
    }

    err = fmFlushMATable(sw, mode, params, NULL, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmFlushAddresses */




/*****************************************************************************/
/** fmGetAddressTableAttribute
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the value of an MA table related attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the MA Table attribute to retrieve
 *                  (see 'MA Table Attributes').
 *
 * \param[out]      value points to a caller-supplied location where this
 *                  function should store the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fmGetAddressTableAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR,
                     "sw=%d, attr=%d, value=%p\n",
                     sw,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetAddressTableAttribute,
                       sw,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmGetAddressTableAttribute */




/*****************************************************************************/
/** fmSetAddressTableAttribute
 * \ingroup addr
 *
 * \chips           FM2000, FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Sets the value of an MA table related attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the MA Table attribute to update
 *                  (see 'MA Table Attributes').
 *
 * \param[in]       value points to a location containing the value
 *                  to be assigned to the attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT attr is read-only or value is
 *                  invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fmSetAddressTableAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR,
                     "sw=%d, attr=%d, value=%p\n",
                     sw,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetAddressTableAttribute,
                       sw,
                       attr,
                       value);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmSetAddressTableAttribute */




/*****************************************************************************/
/** fmCompareInternalMacAddressEntries
 * \ingroup intAddr
 *
 * \desc            Compares two MAC address entries for sorting purposes.
 *
 * \param[in]       key1 points to the first record's key.
 *
 * \param[in]       key2 points to the second record's key.
 *
 * \return          -1 if key1 comes before key2.
 * \return           0 if the keys are identical.
 * \return           1 if key1 comes after key2.
 *
 *****************************************************************************/
fm_int fmCompareInternalMacAddressEntries(const void *key1, const void *key2)
{
    fm_internalMacAddrEntry *entry1;
    fm_internalMacAddrEntry *entry2;

    entry1 = (fm_internalMacAddrEntry *) key1;
    entry2 = (fm_internalMacAddrEntry *) key2;

    if (entry1->macAddress < entry2->macAddress)
    {
        return -1;
    }

    if (entry1->macAddress > entry2->macAddress)
    {
        return 1;
    }

    if (entry1->vlanID < entry2->vlanID)
    {
        return -1;
    }

    if (entry1->vlanID > entry2->vlanID)
    {
        return 1;
    }

    if (entry1->vlanID2 < entry2->vlanID2)
    {
        return -1;
    }

    if (entry1->vlanID2 > entry2->vlanID2)
    {
        return 1;
    }

    return 0;

}   /* end fmCompareInternalMacAddressEntries */




/*****************************************************************************/
/** fmValidateAddressPort
 * \ingroup intAddr
 *
 * \desc            Called by chip-specific MAC address management services,
 *                  this function validates the logical port number associated
 *                  with a MAC address and ensures that these management
 *                  services are not used to for multicast groups.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the logical port associated with the MAC
 *                  address.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if logicalPort is not valid.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if logicalPort identifies a
 *                  multicast group.
 *
 *****************************************************************************/
fm_status fmValidateAddressPort(fm_int sw, fm_int logicalPort)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, "sw=%d logicalPort=%d\n", sw, logicalPort);

    /* Validate the logical port number */
    if (!fmIsValidPort(sw, logicalPort, ALLOW_ALL))
    {
        FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_ERR_INVALID_PORT);
    }

    /* Does the logical port belong to a multicast group?
     * If so, reject the request.
     * Note that multicast groups are assigned a separate, unique
     * logical port. */
    if (fmFindMcastGroupByPort(sw, logicalPort) != NULL)
    {
        /* Addresses may not be attached to Multicast Groups using this
         * interface function - the multicast group functions should
         * be used instead. */
        err = FM_ERR_USE_MCAST_FUNCTIONS;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fmValidateAddressPort */




/*****************************************************************************/
/** fmCommonCheckFlushRequest
 * \ingroup intAddr
 *
 * \chips           FM2000, FM3000, FM4000, FM10000
 *
 * \desc            Pre-processes an MA table flush request. Called through
 *                  the CheckFlushRequest function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mode specifies the type of flush to perform.
 * 
 * \param[in,out]   params points to the set of parameters indicated by mode.
 *                  This function may modify the parameters.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if mode is not supported by the switch.
 * \return          FM_ERR_INVALID_ARGUMENT if mode is not recognized.
 *
 *****************************************************************************/
fm_status fmCommonCheckFlushRequest(fm_int          sw,
                                    fm_flushMode    mode,
                                    fm_flushParams *params)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(params);

    switch (mode)
    {
        case FM_FLUSH_MODE_ALL_DYNAMIC:
        case FM_FLUSH_MODE_PORT:
        case FM_FLUSH_MODE_PORT_VLAN:
        case FM_FLUSH_MODE_VLAN:
            return FM_OK;

        case FM_FLUSH_MODE_VID1_VID2:
        case FM_FLUSH_MODE_PORT_VID1_VID2:
        case FM_FLUSH_MODE_PORT_VID2:
        case FM_FLUSH_MODE_VID2:
        case FM_FLUSH_MODE_PORT_VID1_REMOTEID:
        case FM_FLUSH_MODE_VID1_REMOTEID:
        case FM_FLUSH_MODE_REMOTEID:
            return FM_ERR_UNSUPPORTED;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

}   /* end fmCommonCheckFlushRequest */




/*****************************************************************************/
/** fmCommonFindAndInvalidateAddr
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM10000
 *
 * \desc            Scans the MA Table bin for a specified address/VLAN
 *                  and invalidates the entry. Called through the
 *                  FindAndInvalidateAddress function pointer.
 *
 * \note            The switch lock is assumed to be taken on entry. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       macAddress is the MAC address to search for.
 *
 * \param[in]       vlanID is the VLAN ID to search for.
 * 
 * \param[in]       vlanID2 is the second VLAN ID to search for.
 *
 * \param[in]       bank is the bank within the bin to operate on, or -1
 *                  if the bin should be searched.
 *
 * \param[in]       indexes is an array of the MA Table indexes comprising
 *                  the hash bin for the macAddress and vlanID. The length 
 *                  of the array is equal to switchPtr->macTableBankSize.
 *
 * \param[in]       updateHw should be TRUE to update the hardware registers.
 *
 * \return          FM_OK if entry successfully deleted.
 * \return          FM_ERR_ADDR_NOT_FOUND if macAddress/vlanID could not be
 *                  found at any of the MA Table indexes in indexes.
 *
 *****************************************************************************/
fm_status fmCommonFindAndInvalidateAddr(fm_int     sw, 
                                        fm_macaddr macAddress, 
                                        fm_uint16  vlanID,
                                        fm_uint16  vlanID2,
                                        fm_int     bank,
                                        fm_uint16 *indexes,
                                        fm_bool    updateHw)
{
    fm_uint32                addr;
    fm_internalMacAddrEntry *tblentry;
    fm_internalMacAddrEntry  tmpEntry;
    fm_switch *              switchPtr;
    fm_status                err       = FM_OK;
    fm_event *               event = NULL;
    fm_uint32                numUpdates = 0;
    fm_bool                  l2Locked=FALSE;
    
    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR, 
                         "sw=%d, macAddress=" FM_FORMAT_ADDR " "
                         "entry->vlanID=%d, bank=%d, indexes=%p, updateHw=%d\n",
                         sw,
                         macAddress,
                         vlanID,
                         bank,
                         (void *) indexes,
                         updateHw);

    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_ADDR,
                           switchPtr->switchFamily != FM_SWITCH_FAMILY_FM6000,
                           err = FM_ERR_ASSERTION_FAILED,
                           "Function not supported for FM6000\n");

    FM_TAKE_L2_LOCK(sw);
    l2Locked = TRUE;

    if (bank == -1)
    {
        for (bank = switchPtr->macTableBankCount - 1 ; bank >= 0 ; bank--)
        {
            addr     = indexes[bank];
            tblentry = &switchPtr->maTable[addr];

            if ((tblentry->state != FM_MAC_ENTRY_STATE_INVALID) &&
                (tblentry->macAddress == macAddress) &&
                (tblentry->vlanID == vlanID) &&
                (tblentry->vlanID2 == vlanID2) )
            {
                break;
            }
        }
    }

    if (bank != -1)
    {
        addr     = indexes[bank];
        tblentry = &switchPtr->maTable[addr];

        if ((tblentry->state != FM_MAC_ENTRY_STATE_INVALID) &&
            (tblentry->macAddress == macAddress) &&
            (tblentry->vlanID == vlanID) &&
            (tblentry->vlanID2 == vlanID2) )
        {
            tmpEntry = *tblentry;
            tmpEntry.state = FM_MAC_ENTRY_STATE_INVALID;

            if (updateHw)
            {
                err = fmWriteEntryAtIndex(sw, addr, &tmpEntry);

                if ( (tblentry->state == FM_MAC_ENTRY_STATE_LOCKED &&
                      switchPtr->generateEventOnStaticAddr) ||
                     (tblentry->state != FM_MAC_ENTRY_STATE_LOCKED &&
                      switchPtr->generateEventOnDynamicAddr) )
                {
                    /* Relinquish lock before generating updates. */
                    FM_DROP_L2_LOCK(sw);
                    l2Locked = FALSE;

                    fmGenerateUpdateForEvent(sw,
                                             &fmRootApi->eventThread,
                                             FM_EVENT_ENTRY_AGED,
                                             FM_MAC_REASON_API_AGED,
                                             addr,
                                             tblentry,
                                             &numUpdates,
                                             &event);
                    
                    fmDbgDiagCountIncr(sw, FM_CTR_MAC_API_AGED, 1);
                }
            }

            tblentry->state = FM_MAC_ENTRY_STATE_INVALID;

            FM_LOG_DEBUG2(FM_LOG_CAT_ADDR,
                          "index=0x%x/%d mac=" FM_FORMAT_ADDR 
                          "/0x%x/0x%x aged (api)\n",
                          addr, 
                          bank, 
                          tblentry->macAddress, 
                          tblentry->vlanID,
                          tblentry->vlanID2);
        }
    }
    else
    {
        err = FM_ERR_ADDR_NOT_FOUND;
    }
    
    if (l2Locked)
    {
        FM_DROP_L2_LOCK(sw);
    }

    if (numUpdates != 0)
    {
        fmSendMacUpdateEvent(sw,
                             &fmRootApi->eventThread,
                             &numUpdates,
                             &event,
                             FALSE);
    }

    if (event != NULL)
    {
        fmReleaseEvent(event);
    }
    
ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, err);

}   /* end fmCommonFindAndInvalidateAddr */




/*****************************************************************************/
/** fmCommonAllocAddrTableCache
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM10000
 *
 * \desc            Allocates the internal storage that contains the address
 *                  table. Called through the AllocAddrTableData function
 *                  pointer.
 *
 * \param[in]       switchPtr points to the switch's state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCommonAllocAddrTableCache(fm_switch *switchPtr)
{
    fm_status err = FM_OK;
    fm_uint   size;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH,
                 "switchPtr=%p, sw=%d\n",
                 (void *) switchPtr,
                 switchPtr->switchNumber);

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH,
                           switchPtr->switchFamily != FM_SWITCH_FAMILY_FM6000,
                           err = FM_ERR_ASSERTION_FAILED,
                           "Function not supported for FM6000\n");

    size  = (fm_uint) sizeof(fm_internalMacAddrEntry);
    size *= (fm_uint) switchPtr->macTableSize;

    switchPtr->maTable = (fm_internalMacAddrEntry *) fmAlloc(size);

    if (switchPtr->maTable == NULL)
    {
        err = FM_ERR_NO_MEM;
    }
    else
    {
        memset((void *) switchPtr->maTable, 0, (size_t) size);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH, err);

}   /* end fmCommonAllocAddrTableCache */




/*****************************************************************************/
/** fmCommonFreeAddrTableCache
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM10000
 *
 * \desc            Frees the internal storage that contains the address table.
 *                  Called through the FreeAddrTableData function pointer.
 *
 * \param[in]       switchPtr points to the switch's state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCommonFreeAddrTableCache(fm_switch *switchPtr)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH,
                 "switchPtr=%p, sw=%d\n",
                 (void *) switchPtr,
                 switchPtr->switchNumber);

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH,
                           switchPtr->switchFamily != FM_SWITCH_FAMILY_FM6000,
                           err = FM_ERR_ASSERTION_FAILED,
                           "Function not supported for FM6000\n");

    if (switchPtr->maTable)
    {
        fmFree(switchPtr->maTable);
        switchPtr->maTable = NULL;
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH, err);

}   /* end fmCommonFreeAddrTableCache */




/*****************************************************************************/
/** fmCommonGetAddressTable
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM6000
 *
 * \desc            Retrieves a copy of the entire MA Table. Called through
 *                  the GetAddressTable function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      nEntries points to caller-allocated storage where this
 *                  function is to store the number of MA Table entries
 *                  retrieved. May not be NULL.
 *
 * \param[out]      entries points to an array of fm_macAddressEntry
 *                  structures that will be filled in by this function with
 *                  MA Table entries.  The array must be large enough to
 *                  hold maxEntries number of MA Table entries.
 *
 * \param[in]       maxEntries is the size of entries, being the maximum
 *                  number of addresses that entries can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmCommonGetAddressTable(fm_int              sw,
                                  fm_int *            nEntries,
                                  fm_macAddressEntry *entries,
                                  fm_int              maxEntries)
{
    fm_switch *             switchPtr;
    fm_status               result = FM_OK;
    fm_status               status;
    fm_int                  i;
    fm_internalMacAddrEntry hwEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d nEntries=%p entries=%p maxEntries=%d\n",
                 sw,
                 (void *) nEntries,
                 (void *) entries,
                 maxEntries);

    switchPtr = GET_SWITCH_PTR(sw);

    *nEntries = 0;

    /***************************************************
     * The MAC address table is read from hardware rather
     * than the cache. On the FM2000 and FM4000 this is
     * for debugging purposes. On the FM6000 this is a
     * necessity since there is no cache.
     *
     * Whenever an error is encountered, either because 
     * the entry could not be retrieved or due to a 
     * conversion error, the entry in question is skipped. 
     * If multiple errors occur, the user is only notified
     * of the reason for the first error.
     **************************************************/
    for (i = 0 ; i < switchPtr->macTableSize ; i++)
    {
        /* Skip any non applicable entries */
        if (switchPtr->IsIndexValid != NULL && !switchPtr->IsIndexValid(sw, i))
        {
            continue;
        }

        status = fmReadEntryAtIndex(sw, (fm_uint32) i, &hwEntry);
        FM_ERR_COMBINE(result, status);

        if (hwEntry.state != FM_MAC_ENTRY_STATE_INVALID)
        {
            /* entries == NULL is used to count the number of MAC entry */
            if (entries != NULL)
            {
                if (*nEntries >= maxEntries)
                {
                    result = FM_ERR_BUFFER_FULL;
                    goto ABORT;
                }

                FM_API_CALL_FAMILY(status,
                                   switchPtr->FillInUserEntryFromTable,
                                   sw,
                                   &hwEntry,
                                   &entries[*nEntries]);
                FM_ERR_COMBINE(result, status);
            }

            (*nEntries)++;
        }
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, result);

}   /* end fmCommonGetAddressTable */




/*****************************************************************************/
/** fmCommonDeleteAddressPre
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM10000
 *
 * \desc            Verifies that we are not trying to delete a MAC address
 *                  for a multicast group using the address management API.
 *                  Called through the DeleteAddressPre function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the MAC address table entry to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if logicalPort identifies a
 *                  multicast group.
 *
 *****************************************************************************/
fm_status fmCommonDeleteAddressPre(fm_int sw, fm_macAddressEntry *entry)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d "
                 "macAddress=" FM_FORMAT_ADDR " "
                 "vlanID=%d/%d "
                 "destMask=0x%8x "
                 "port=%d\n",
                 sw,
                 entry->macAddress,
                 entry->vlanID,
                 entry->vlanID2,
                 entry->destMask,
                 entry->port);

    
    if (entry->destMask == FM_DESTMASK_UNUSED)
    {
        /* Does the logical port belong to a multicast group?
         * If so, reject the request.
         * Note that multicast groups are assigned a separate, unique
         * logical port. */
        if (fmFindMcastGroupByPort(sw, entry->port) != NULL)
        {
            /* Addresses may not be detached from Multicast Groups using
             * this interface function - the multicast group functions
             * should be used instead.
             */
             err = FM_ERR_USE_MCAST_FUNCTIONS;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fmCommonDeleteAddressPre */




/*****************************************************************************/
/** fmCommonDeleteAllAddresses
 * \ingroup intAddr
 * 
 * \chips           FM2000, FM4000, FM10000
 *
 * \desc            Deletes addresses from the MA Table without generating
 *                  AGE events. Called through the DeleteAllAddresses function
 *                  pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       dynamicOnly should be set to TRUE to leave static entries
 *                  in the table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmCommonDeleteAllAddresses(fm_int sw, fm_bool dynamicOnly)
{
    fm_switch *              switchPtr;
    fm_status                status;
    fm_int                   i;
    fm_internalMacAddrEntry *cacheEntry;
    fm_status                err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, 
                 "sw=%d, dynamicOnly=%s\n", 
                 sw,
                 dynamicOnly ? "TRUE" : "FALSE");

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_ADDR,
                           (switchPtr->switchFamily != FM_SWITCH_FAMILY_FM6000) &&
                           (switchPtr->switchFamily != FM_SWITCH_FAMILY_REMOTE_FM6000),
                           err = FM_ERR_ASSERTION_FAILED,
                           "Function not supported for FM6000\n");

    FM_BEGIN_FIBM_BATCH(sw, err, FM_LOG_CAT_ADDR);
    
    for (i = 0 ; i < switchPtr->macTableSize ; i++)
    {
        /***************************************************
         * Take and release the MAC address table lock for
         * each individual entry, since deleting all entries
         * could be a relatively long operation.
         **************************************************/
         
        FM_TAKE_L2_LOCK(sw);

        cacheEntry = &switchPtr->maTable[i];

        if ( !dynamicOnly ||
             ( (cacheEntry->state != FM_MAC_ENTRY_STATE_INVALID) &&
               (cacheEntry->state != FM_MAC_ENTRY_STATE_LOCKED) ) )
        {
            cacheEntry->state = FM_MAC_ENTRY_STATE_INVALID;

            fmDbgDiagCountIncr(sw, FM_CTR_MAC_CACHE_DELETED, 1);

            status = fmWriteEntryAtIndex(sw, (fm_uint32) i, cacheEntry); 
            if (status != FM_OK)
            {
                fmDbgDiagCountIncr(sw, FM_CTR_MAC_WRITE_ERR, 1);
                err = (err == FM_OK) ? status : err;
            }
            
        }

        FM_DROP_L2_LOCK(sw);
        
    }   /* end for (i = 0 ; i < switchPtr->macTableSize ; i++) */

    FM_END_FIBM_BATCH(sw, status);

    if (err == FM_OK)
    {
        err = status;
    }
    
ABORT:    
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end fmCommonDeleteAllAddresses */




/*****************************************************************************/
/** fmGetAddressIndex
 * \ingroup intAddr
 *
 * \chips           FM6000
 *
 * \desc            Provided for the benefit of TestPoint, returns the
 *                  index and bank of the MAC Table entry matching the
 *                  specified MAC address and VLAN ID. In shared spanning 
 *                  tree mode, the supplied VLAN ID is ignored and the
 *                  default is used instead.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       macAddress is the MAC address to look up.
 *
 * \param[in]       vlanID is the VLAN number to look up.
 * 
 * \param[in]       vlanID2 is the second VLAN number to look up.
 *
 * \param[out]      index points to caller-allocated storage where the 
 *                  this function should place the located address's table
 *                  index.
 *
 * \param[out]      bank points to caller-allocated storage where the 
 *                  this function should place the located address's table
 *                  bank.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 *
 *****************************************************************************/
fm_status fmGetAddressIndex(fm_int     sw,
                            fm_macaddr macAddress,
                            fm_int     vlanID,
                            fm_int     vlanID2,
                            fm_int *   index,
                            fm_int *   bank)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_uint16  learningFID;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d "
                 "macAddress=" FM_FORMAT_ADDR " "
                 "vlanID=%d "
                 "index=%p "
                 "bank=%p\n",
                 sw,
                 macAddress,
                 vlanID,
                 (void *) index,
                 (void *) bank);
    
    VALIDATE_AND_PROTECT_SWITCH(sw);
    switchPtr = GET_SWITCH_PTR(sw);
    
    FM_API_CALL_FAMILY(err, switchPtr->GetLearningFID, sw, vlanID, &learningFID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    vlanID = learningFID;

    FM_API_CALL_FAMILY(err, 
                       switchPtr->GetAddressIndex, 
                       sw,
                       macAddress,
                       vlanID,
                       vlanID2,
                       index,
                       bank);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fmGetAddressIndex */




/*****************************************************************************/
/** fmGetSecurityStats
 * \ingroup stats
 *
 * \chips           FM10000
 *
 * \desc            Retrieves MAC security statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      stats points to an ''fm_securityStats'' structure to be
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmGetSecurityStats(fm_int sw, fm_securityStats *stats)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR, 
                     "sw=%d stats=%p\n", 
                     sw, 
                     (void *)stats);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (stats == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetSecurityStats, sw, stats);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmGetSecurityStats */




/*****************************************************************************/
/** fmResetSecurityStats
 * \ingroup stats
 *
 * \chips           FM10000
 *
 * \desc            Resets MAC security statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fmResetSecurityStats(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ADDR, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->ResetSecurityStats, sw);

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_ADDR, err);

}   /* end fmResetSecurityStats */

