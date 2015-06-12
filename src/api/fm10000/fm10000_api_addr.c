/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:          fm10000_api_addr.c            
 * Creation Date: April 10, 2013   
 * Description:   FM10000 functions for dealing with MA table configuration.  
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define L2L_HASH_TABLE_SIZE             256
#define MAX_UINT_VALUE                  4294967295


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
/** FindBestIndex
 * \ingroup intAddr
 *
 * \desc            Scans the eligible MA Table indexes and finds the
 *                  best candidate for writing a new entry. 
 *
 * \note            The MA Table lock should be taken prior to calling this
 *                  function if the caller intends to do something with the
 *                  best candidate entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the MAC table entry to be added.
 *
 * \param[in]       indexes is an array of MA Table absolute indexes to which 
 *                  the new entry hashes.
 *
 * \param[out]      bestIndex points to the location where this function
 *                  should store the index of the best candidate MA Table
 *                  entry in the indexes array.
 *
 * \param[out]      dupMask points to the location where this function
 *                  should store a bit mask indicating which entries in
 *                  the indexes array match the MAC Table entry to be added,
 *                  but were not the best index and so should be invalidated
 *                  by the caller.
 *
 * \param[out]      ageOld points to a location where this function should
 *                  indicate whether the entry returned in bestIndex needs
 *                  to be aged out before it is overwritten.
 *
 * \return          FM_OK if a best index was found.
 * \return          FM_ERR_STATIC_ADDR_EXISTS if a matching static entry already
 *                  exists when trying to write a dynamic entry.
 * \return          FM_ERR_ADDR_BANK_FULL if the MA Table bin is full.
 * \return          FM_ERR_INVALID_PORT if the glort could not be converted
 *                  into a logical port in an existing matching entry.
 *
 *****************************************************************************/
static fm_status FindBestIndex(fm_int              sw, 
                               fm_macAddressEntry *entry,
                               fm_uint16 *         indexes,
                               fm_int *            bestIndex,
                               fm_uint32 *         dupMask,
                               fm_bool *           ageOld)
{
    fm_internalMacAddrEntry *   cachePtr;
    fm_switch *                 switchPtr;
    fm_int                      bankId;
    fm_int                      unusedEntry;
    fm_int                      dynamicEntry;
    fm_int                      expiredEntry;
    fm_int                      matchingEntry;
    fm_status                   err;
    fm_bool                     isDynamic;
    fm_bool                     isStatic;
    fm_bool                     isMoved;
    
    FM_LOG_ENTRY_VERBOSE( FM_LOG_CAT_ADDR,
                          "sw=%d entry=%p<%012llx:%d => %d, type=%d>\n",
                          sw,
                          (void *) entry,
                          entry->macAddress,
                          entry->vlanID,
                          entry->port,
                          entry->type);
                         
    switchPtr = GET_SWITCH_PTR(sw);
    
    *bestIndex  = -1;
    *ageOld     = FALSE;
    *dupMask    = 0;
    err         = FM_OK;

    dynamicEntry    = -1;
    matchingEntry   = -1;
    unusedEntry     = -1;
    expiredEntry    = -1;

    isDynamic       = FALSE;
    isStatic        = FALSE;
    isMoved         = FALSE;
    
    /**************************************************
     * Search the bin for all possible candidates in
     * which to write the new entry.
     *
     * isDynamic is set if we find a matching dynamic
     * entry. isStatic is set if we find a matching
     * static entry. In theory, these flags are
     * mutually exclusive, but we keep both so we can
     * detect a pathological case appearing in the wild.
     *
     * In the search for a best entry, static entries 
     * take precedence over dynamic entries; so if
     * isStatic is already set, we won't ever set
     * isDynamic. If a matching dynamic entry is
     * found first, then a matching static, we will
     * clear isDynamic.
     **************************************************/
    
    for (bankId = 0 ; bankId < switchPtr->macTableBankCount ; bankId++)
    {
        cachePtr = &switchPtr->maTable[indexes[bankId]];
        
        /**************************************************
         * Process unused entry.
         **************************************************/

        if (cachePtr->state == FM_MAC_ENTRY_STATE_INVALID)
        {
            /* Found an unused entry. */
            unusedEntry = bankId;
        }         
        
        /**************************************************
         * Process matching entry.
         **************************************************/

        else if (cachePtr->macAddress == entry->macAddress &&
                 cachePtr->vlanID == entry->vlanID)
        {
            /* Found a matching entry. */
            *dupMask |= (1 << bankId);
            
            switch (cachePtr->addrType)
            {
                /**************************************************
                 * Matching dynamic address.
                 **************************************************/

                case FM_ADDRESS_DYNAMIC:
                case FM_ADDRESS_SECURE_DYNAMIC:
                    if (isStatic)
                    {
                        /* We found a matching static entry and a matching
                         * dynamic entry. That's bad. */
                        FM_LOG_ASSERT(FM_LOG_CAT_ADDR,
                                      FALSE,
                                      "Found both static and dynamic MA Table "
                                      "entries for %012llx:%u\n",
                                      entry->macAddress,
                                      entry->vlanID);
                    }
                    else
                    {
                        /* We found more than one matching dynamic entry.
                         * That's bad, too. */
                        FM_LOG_ASSERT(FM_LOG_CAT_ADDR,
                                      !isDynamic,
                                      "Found more than one dynamic MA Table "
                                      "entry for %012llx:%u\n",
                                      entry->macAddress,
                                      entry->vlanID);
                        
                        /* Record this matching dynamic entry */
                        matchingEntry = bankId;
                        isDynamic = TRUE;
                        
                        /* Note if it is on a different port. */
                        isMoved = (cachePtr->port != entry->port);
                        
                    }   /* end if (isStatic) */
                    break;

                /**************************************************
                 * Matching static address.
                 **************************************************/

                case FM_ADDRESS_STATIC:
                case FM_ADDRESS_SECURE_STATIC:
                    FM_LOG_ASSERT(FM_LOG_CAT_ADDR,
                                  !isDynamic && !isStatic,
                                  "Found more than one static MA Table entry "
                                  "for %012llx:%u\n",
                                  entry->macAddress,
                                  entry->vlanID);
                    
                    matchingEntry = bankId;
                    isStatic = TRUE;
                    
                    /* If we previously found a matching dynamic entry, we've
                     * now discarded it in favor of the static, so clear this
                     * flag. */
                    isDynamic = FALSE;
                    break;

                /**************************************************
                 * Unexpected address type.
                 **************************************************/

                default:
                    FM_LOG_ASSERT(FM_LOG_CAT_ADDR,
                                  FALSE,
                                  "Unexpected MA Table address type %s(%u) "
                                  "for %012llx:%u\n",
                                  fmAddressTypeToText(cachePtr->addrType),
                                  cachePtr->addrType,
                                  entry->macAddress,
                                  entry->vlanID);
                    break;
                    
            }   /* end switch (cachePtr->addrType) */

        }   /* end if (cachePtr->macAddress == entry->macAddress &&
                       cachePtr->vlanID == entry->vlanID) */
        
        /**************************************************
         * Process non-matching dynamic entry.
         **************************************************/

        else if (cachePtr->state == FM_MAC_ENTRY_STATE_YOUNG ||
                 cachePtr->state == FM_MAC_ENTRY_STATE_OLD)
        {
            /* Non-matching dynamic. */
            dynamicEntry = bankId;
        }
        else if (cachePtr->state == FM_MAC_ENTRY_STATE_EXPIRED)
        {
            /* Non-matching expired. */
            expiredEntry = bankId;
        }
            
    }   /* end for (bankId = 0 ; bankId < switchPtr->macTableBankCount ... */

#if 0
    /**************************************************
     * Dump inputs to decision-making stage.
     **************************************************/
    
    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "isDynamic=%s isStatic=%s isMoved=%s\n",
                 FM_BOOLSTRING(isDynamic),
                 FM_BOOLSTRING(isStatic),
                 FM_BOOLSTRING(isMoved));

    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "dynamicEntry=%d matchingEntry=%d expiredEntry=%d "
                 "dupMask=%02x\n",
                 dynamicEntry, matchingEntry, expiredEntry, *dupMask);
#endif
        
    /**************************************************
     * Don't report the best matching entry as a 
     * duplicate.
     **************************************************/
    
    if (matchingEntry != -1)
    {
        *dupMask &= ~(1 << matchingEntry);
    }

    /**************************************************
     * Now decide the best choice of all those found.
     **************************************************/

    if (FM_IS_ADDR_TYPE_STATIC(entry->type))
    {
        /**************************************************
         * For a new static entry, preference order is:
         *     1. Any existing entry with matching key
         *     2. Unused entry
         *     3. Dynamic entry that has aged out
         *     4. Dynamic entry with non-matching key
         **************************************************/
        
        if (matchingEntry != -1)
        {
            /* First choice is a matching entry. */
            *bestIndex = matchingEntry;
            
            if (isDynamic && isMoved)
            {
                /* We need to report an AGE event for old entry */
                *ageOld = TRUE;
            }
        }
        else if (unusedEntry != -1)
        {
            /* Next best is an unused entry. */
            *bestIndex = unusedEntry;
        }
        else if (expiredEntry != -1)
        {
            /* Next choice is a dynamic entry that has aged out. */
            *bestIndex = expiredEntry;
            *ageOld = TRUE;
        }
        else if (dynamicEntry != -1)
        {
            /* Last choice is a non-matching dynamic entry. */
            *bestIndex = dynamicEntry;
            *ageOld = TRUE;
        }
        else
        {
            /* No room for a new entry. */
            err = FM_ERR_ADDR_BANK_FULL;
            goto ABORT;
        }
    }
    else
    {
        /**************************************************
         * For a new dynamic entry, preference order is:
         *     1. Existing dynamic entry with matching key
         *     2. Unused entry
         *     3. Dynamic entry that has aged out
         **************************************************/
        
        if (matchingEntry != -1)
        {
            if (isStatic)
            {
                /* Matching static entry cannot be overwritten by dynamic */
                err = FM_ERR_STATIC_ADDR_EXISTS;
                goto ABORT;
            }

            /* Matching dynamic entry. */
            *bestIndex = matchingEntry;
            
            if (isMoved)
            {
                /* We need to report an AGE event for old entry */
                *ageOld = TRUE;
            }
        }
        else if (unusedEntry != -1)
        {
            /* Unused entry. */
            *bestIndex = unusedEntry;
        }
        else if (expiredEntry != -1)
        {
            /* Dynamic entry that has been aged out. */
            *bestIndex = expiredEntry;
            *ageOld = TRUE;
        }
        else
        {
            /* No room for a new entry. */
            err = FM_ERR_ADDR_BANK_FULL;
            goto ABORT;
        }
        
    }   /* end if (entry->type == FM_ADDRESS_STATIC) */

    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "bestIndex=%d dupMask=%02x ageOld=%s\n",
                 *bestIndex,
                 *dupMask,
                 FM_BOOLSTRING(*ageOld));
    
ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, err);
    
}   /* end FindBestIndex */




/*****************************************************************************/
/** ResetUsedEntry
 * \ingroup intAddr
 *
 * \desc            Resets the entry in the MA_USED_TABLE for the specified
 *                  MA_TABLE entry.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       index is the index of the MA Table entry whose USED table
 *                  entry is to be reset.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ResetUsedEntry(fm_int sw, fm_uint32 index)
{
    fm_switch * switchPtr;
    fm_status   status;
    fm_uint32   usedIndex;
    fm_uint32   usedValue;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR, "sw=%d index=%u\n", sw, index);

    switchPtr = GET_SWITCH_PTR(sw);

    usedIndex = index / 32;
    usedValue = 1 << (index % 32);

    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "usedIndex=%u usedValue=%08x\n",
                 usedIndex,
                 usedValue);

    /* Clear SMAC USED bit. */
    status = switchPtr->WriteUINT32(sw,
                                    FM10000_MA_USED_TABLE(1, usedIndex),
                                    usedValue);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, status);

}   /* end ResetUsedEntry */




/*****************************************************************************/
/** ValidateAddressFields
 * \ingroup intAddr
 *
 * \desc            Validates the fields in the address entry to be added.
 *
 * \param[in]       entry points to an ''fm_macAddressEntry'' structure that
 *                  describes the MA Table entry to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one or more of the fields
 *                  is invalid.
 *
 *****************************************************************************/
static fm_status ValidateAddressFields(fm_macAddressEntry * entry)
{
    fm_int  errors = 0;

    /* Require that destMask be UNUSED. */
    if (entry->destMask != FM_DESTMASK_UNUSED)
    {
        FM_LOG_WARNING(FM_LOG_CAT_ADDR,
                       "invalid field: destMask=0x%08x\n",
                       entry->destMask);
        ++errors;
    }

#if 0
    /* Require that unsupported parameters be zero. */
    if (entry->vlanID2 != 0 || entry->remoteID != 0 || entry->remoteMac != 0)
    {
        FM_LOG_WARNING(FM_LOG_CAT_ADDR,
                       "invalid field: vlanID2=%u remoteID=%u remoteMac=%u\n",
                       entry->vlanID2,
                       entry->remoteID,
                       entry->remoteMac);
        /*++errors;*/
    }
#endif

    /* Validate address type. */
    switch (entry->type)
    {
        case FM_ADDRESS_STATIC:
        case FM_ADDRESS_DYNAMIC:
        case FM_ADDRESS_SECURE_STATIC:
        case FM_ADDRESS_SECURE_DYNAMIC:
            break;

        default:
            FM_LOG_WARNING(FM_LOG_CAT_ADDR,
                           "invalid type: %s(%u)\n",
                           fmAddressTypeToText(entry->type),
                           entry->type);
            ++errors;
            break;

    }   /* end switch (entry->type) */

    return (errors != 0) ? FM_ERR_INVALID_ARGUMENT : FM_OK;

}   /* end ValidateAddressFields */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000AddAddress
 * \ingroup intAddr
 *
 * \desc            Adds an entry to the MA Table. See ''fmAddAddress'' for
 *                  other information. Called through the AddAddress function
 *                  pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   entry points to an ''fm_macAddressEntry'' structure that
 *                  describes the MA Table entry to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the entry type is unsupported.
 * \return          FM_ERR_ADDR_BANK_FULL if no room in MA Table for this
 *                  address.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if an attempt was made to
 *                  add a MAC address to a multicast group.
 *
 *****************************************************************************/
fm_status fm10000AddAddress(fm_int sw, fm_macAddressEntry *entry)
{
    fm_uint32       trigger;
    fm_status       err;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d "
                 "macAddress=" FM_FORMAT_ADDR " "
                 "vlanID=%d "
                 "port=%d\n",
                 sw,
                 entry->macAddress,
                 entry->vlanID,
                 entry->port);

    /* Validate the address. */
    err = ValidateAddressFields(entry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /* Validate port number. */
    err = fmValidateAddressPort(sw, entry->port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /* Select trigger identifier to use. */
    err = fm10000AssignMacTrigger(sw, entry, &trigger);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    /* Add address to MAC table. */
    err = fmAddAddressToTableInternal(sw, entry, trigger, TRUE, -1);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000AddAddress */




/*****************************************************************************/
/** fm10000AddMacTableEntry
 * \ingroup intAddr
 *
 * \desc            Adds an entry to the MAC Address table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the entry structure to add.
 * 
 * \param[in]       source is the source of the MA table entry.
 *
 * \param[in]       trigger is the trigger identifier to be stored in the
 *                  MA Table entry.
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
 * \return          FM_OK if successful.
 * \return          FM_ERR_ADDR_BANK_FULL if there is no room in the MA table
 *                  for the specified address.
 *
 *****************************************************************************/
fm_status fm10000AddMacTableEntry(fm_int               sw,
                                  fm_macAddressEntry * entry,
                                  fm_macSource         source,
                                  fm_uint32            trigger,
                                  fm_uint32 *          numUpdates,
                                  fm_event **          outEvent)
{
    fm_internalMacAddrEntry oldEntry;
    fm_internalMacAddrEntry newEntry;

    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint16       indexes[FM10000_MAC_ADDR_BANK_COUNT];
    fm_int          bestIndex;
    fm_uint32       dupMask;
    fm_bool         ageOld;
    fm_bool         reportNew;
    fm_bool         l2Locked = FALSE;
    fm_bool         isSecure;
    fm_bool         isTcnEvent;
    fm_int          hashIndex;
    fm_int          i;
    fm_int          reason;
    fm_status       err;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d macAddress=%012llx vlanID=%u type=%s "
                 "source=%s trigger=%d "
                 "numUpdates=%u outEvent=%p\n",
                 sw,
                 entry->macAddress,
                 entry->vlanID,
                 fmAddressTypeToText(entry->type),
                 fmMacSourceToText(source),
                 (fm_int) trigger,
                 *numUpdates,
                 (void *) *outEvent);
 
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;
    
    /**************************************************
     * Assign trigger if we don't have one.
     **************************************************/

    if (trigger == FM_DEFAULT_TRIGGER)
    {
        err = fm10000AssignMacTrigger(sw, entry, &trigger);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
    }
    
    /**************************************************
     * Find out whether this is a secure entry.
     **************************************************/

    isSecure = FM_IS_ADDR_TYPE_SECURE(entry->type);
    
    /**************************************************
     * Find out whether this is a TCN FIFO event.
     **************************************************/

    isTcnEvent = FM_IS_MAC_SOURCE_TCN_FIFO(source);
    
    /**************************************************
     * Get possible hash table indexes for entry.
     **************************************************/

    err = fm10000ComputeAddressIndex(sw, 
                                     entry->macAddress, 
                                     entry->vlanID, 
                                     0, 
                                     indexes);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "indexes[0]=%u indexes[1]=%u indexes[2]=%u indexes[3]=%u\n",
                 indexes[0], indexes[1], indexes[2], indexes[3]);
    
    /**************************************************
     * Get exclusive use of MAC table.
     **************************************************/

    FM_TAKE_L2_LOCK(sw);
    l2Locked = TRUE;
    
    /************************************************** 
     * Verify that the port is a member of the vlan. 
     * (Bug 28151) 
     **************************************************/

    if (isTcnEvent)
    {
        err = fm10000CheckVlanMembership(sw, entry->vlanID, entry->port);
        if (err != FM_OK)
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_VLAN_ERR, 1);
            goto ABORT;
        }
    }

    /**************************************************
     * Find the best entry for the address.
     **************************************************/

    err = FindBestIndex(sw, entry, indexes, &bestIndex, &dupMask, &ageOld);

    if (err == FM_ERR_STATIC_ADDR_EXISTS)
    {
        /**************************************************
         * If we are trying to write a dynamic entry and
         * there is already a matching static entry, do
         * nothing, and do not return an error. This is 
         * for consistency with existing code.
         **************************************************/
        err = FM_OK;
        goto ABORT;
    }
    else if (err != FM_OK)
    {
        goto ABORT;
    }

    /* Get the preferred hash table index. */
    hashIndex = indexes[bestIndex];

    /* Save the current entry so we can generate an AGE event. */
    oldEntry = switchPtr->maTable[hashIndex];
    
    /**************************************************
     * Initialize the new MA table entry.
     **************************************************/

    FM_CLEAR(newEntry);

    newEntry.destMask   = FM_DESTMASK_UNUSED;
    newEntry.port       = entry->port;
    newEntry.vlanID     = entry->vlanID;
    newEntry.macAddress = entry->macAddress;
    newEntry.addrType   = entry->type;
    newEntry.secure     = isSecure;

    if (FM_IS_ADDR_TYPE_STATIC(entry->type))
    {
        newEntry.state = FM_MAC_ENTRY_STATE_LOCKED;
    }
    else
    {
        newEntry.state = FM_MAC_ENTRY_STATE_YOUNG;
        newEntry.agingCounter = fm10000GetAgingTimer();
    }

    if (trigger != FM_DEFAULT_TRIGGER)
    {
        newEntry.trigger = trigger;
    }
    
    /************************************************** 
     * Ignore the transaction if the new entry comes
     * from the TCN FIO and it matches the entry in the 
     * cache. This may happen if frames with unknown 
     * SMACS arrive faster than software is able to 
     * process the TCN FIFO entries. (Bug 28232)
     **************************************************/

    if (isTcnEvent &&
        oldEntry.state != FM_MAC_ENTRY_STATE_INVALID &&
        /* Check the basic 3-tuple. */
        oldEntry.macAddress == newEntry.macAddress &&
        oldEntry.vlanID     == newEntry.vlanID &&
        oldEntry.port       == newEntry.port &&
        /* Check these two for insurance. */
        oldEntry.addrType   == newEntry.addrType &&
        oldEntry.trigger    == newEntry.trigger)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ADDR, "Duplicate entry, ignored\n");
        err = FM_OK;
        goto ABORT;
    }
    
    /**************************************************
     * Write new entry to cache.
     **************************************************/

    switchPtr->maTable[hashIndex] = newEntry;
    
    /**************************************************
     * Write new entry to hardware.
     **************************************************/

    err = fmWriteEntryAtIndex(sw, hashIndex, &newEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
    
    /**************************************************
     * If there are duplicate entries that need
     * invalidating, do it now.
     **************************************************/
     
    if (dupMask)
    {
        /* Invalidate all other entries for this MAC/VLAN. */
        for (i = 0 ; i < FM10000_MAC_ADDR_BANK_COUNT ; ++i)
        {
            if (dupMask & (1 << i))
            {
                fmDbgDiagCountIncr(sw, FM_CTR_MAC_CACHE_DUP, 1);

                err = fm10000InvalidateEntryAtIndex(sw, indexes[i]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);
            }
            
        }   /* end for (i = 0 ; i < FM10000_MAC_ADDR_BANK_COUNT ; ++i) */
        
    }   /* end if (dupMask) */
    
    /**************************************************
     * Drop lock before generating events.
     **************************************************/

    FM_DROP_L2_LOCK(sw);
    l2Locked = FALSE;
    
    /**************************************************
     * Now report the AGE event for the overwritten
     * entry, and report a LEARNED event if the 
     * application requested it. 
     **************************************************/
    
    /* See whether the API should send a LEARNED event to application. */
    if ( (newEntry.state == FM_MAC_ENTRY_STATE_LOCKED &&
          switchPtr->generateEventOnStaticAddr) ||
         (newEntry.state != FM_MAC_ENTRY_STATE_LOCKED &&
          switchPtr->generateEventOnDynamicAddr) ||
         (source == FM_MAC_SOURCE_TCN_LEARNED) ||
         (source == FM_MAC_SOURCE_TCN_MOVED) )
    {
        reportNew = TRUE;
    }
    else
    {
        reportNew = FALSE;
    }

    if (ageOld)
    {
        if (newEntry.vlanID     == oldEntry.vlanID &&
            newEntry.macAddress == oldEntry.macAddress) 
        {
            reason =
                (source == FM_MAC_SOURCE_API_ADDED) ?
                FM_MAC_REASON_API_LEARN_CHANGED :
                FM_MAC_REASON_LEARN_CHANGED;
        }
        else 
        {
            reason =
                (source == FM_MAC_SOURCE_API_ADDED) ?
                FM_MAC_REASON_API_LEARN_REPLACED :
                FM_MAC_REASON_LEARN_REPLACED;
        }

        fmGenerateUpdateForEvent(sw,
                                 &fmRootApi->eventThread,
                                 FM_EVENT_ENTRY_AGED,
                                 reason,
                                 hashIndex,
                                 &oldEntry,
                                 numUpdates,
                                 outEvent);

        if (source == FM_MAC_SOURCE_API_ADDED)
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_API_AGED, 1);
        }
        else
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_AGED, 1);
        }
    }
    else
    {
        reason =
            (source == FM_MAC_SOURCE_API_ADDED) ?
            FM_MAC_REASON_API_LEARNED :
            FM_MAC_REASON_LEARN_EVENT;
    }

    if (reportNew)
    {
        fmGenerateUpdateForEvent(sw,
                                 &fmRootApi->eventThread,
                                 FM_EVENT_ENTRY_LEARNED,
                                 reason,
                                 hashIndex,
                                 &newEntry,
                                 numUpdates,
                                 outEvent);

        if (source == FM_MAC_SOURCE_API_ADDED)
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_API_LEARNED, 1);
        }
        else
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_LEARNED, 1);
        }

        if (source == FM_MAC_SOURCE_TCN_MOVED)
        {
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_PORT_CHANGED, 1);
        }
    }

ABORT:
    if (l2Locked)
    {
        FM_DROP_L2_LOCK(sw);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000AddMacTableEntry */




/*****************************************************************************/
/** fm10000AssignTableEntry
 * \ingroup intAddr
 *
 * \desc            Assigns an address to an entry in the MA table and writes
 *                  it to the hardware. Called through the AssignTableEntry
 *                  function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to the entry structure to add.
 *
 * \param[in]       targetBank is not used. It must be -1.
 *
 * \param[in]       trigger is the trigger identifier to be stored in the
 *                  MA Table entry.
 *
 * \param[in]       updateHw is not used. It must be TRUE.
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
 * \return          FM_OK if successful.
 * \return          FM_ERR_ADDR_BANK_FULL if there is no room in the MA table
 *                  for the specified address.
 *
 *****************************************************************************/
fm_status fm10000AssignTableEntry(fm_int              sw,
                                  fm_macAddressEntry *entry,
                                  fm_int              targetBank,
                                  fm_uint32           trigger,
                                  fm_bool             updateHw,
                                  fm_uint32*          numUpdates,
                                  fm_event**          outEvent)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d, entry=%p, trigger=%d, "
                 "numUpdates=%p, outEvent=%p\n",
                 sw,
                 (void *) entry,
                 trigger,
                 (void *) numUpdates,
                 (void *) outEvent);

    if (targetBank != -1 || !updateHw)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ADDR,
                     "invalid arguments: targetBank=%d updateHw=%u\n",
                     targetBank,
                     updateHw);
        FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_ERR_UNSUPPORTED);
    }

    status = fm10000AddMacTableEntry(sw,
                                     entry,
                                     FM_MAC_SOURCE_API_ADDED,
                                     trigger,
                                     numUpdates,
                                     outEvent);

    FM_LOG_EXIT(FM_LOG_CAT_ADDR, status);

}   /* end fm10000AssignTableEntry */




/*****************************************************************************/
/** fm10000CalcAddrHash
 * \ingroup intAddr
 *
 * \desc            Computes the hash table indexes for a specified MAC
 *                  address and FID.
 * 
 * \note            This function is also used by the White Model code.
 *
 * \param[in]       macAddr is the MAC address to be hashed.
 * 
 * \param[in]       fid is the FID value to be hashed.
 * 
 * \param[in]       hashMode specifies whether to use standard hash mode (0)
 *                  or associative hash mode (1).
 *
 * \param[in]       hashes points to an array in which this function will
 *                  store the hash values.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CalcAddrHash(fm_macaddr  macAddr,
                              fm_uint16   fid,
                              fm_uint32   hashMode,
                              fm_uint16 * hashes)
{
    static const fm_uint64 magic[] =
    {
        FM_LITERAL_U64(0xA8682B5EC57B5532),
        FM_LITERAL_U64(0x54015762C2EDE43F),
        FM_LITERAL_U64(0xA802AEC585DBC87F),
        FM_LITERAL_U64(0x54D45C5443ACDEA4),
        FM_LITERAL_U64(0xA9A8B8A88759BD48),
        FM_LITERAL_U64(0x5780708E46A834CB),
        FM_LITERAL_U64(0xAF00E11C8D506997),
        FM_LITERAL_U64(0x5AD0C3E652BB9D74),
        FM_LITERAL_U64(0xC694D9EA73382DC0),
        FM_LITERAL_U64(0x89F8B20BAE6B15DB),
        FM_LITERAL_U64(0x172065C814CD65ED),
        FM_LITERAL_U64(0x2E40CB90299ACBDB)
    };

    fm_uint64 key;
    fm_int    i;
    fm_int    j;
    fm_int    setId;
    fm_bool   parity;
    fm_uint   hash;
    fm_uint64 temp;

    key =  ( ( (fm_uint64) fid & 0xfff) << 48 ) |
           ( ( (fm_uint64) macAddr & 0xffffffffffff ) );
 
    /* Compute hashes for 4 sets. */
    for (setId = 0 ; setId < FM10000_MAC_ADDR_BANK_COUNT ; ++setId)
    {
        hash = 0;
 
        for (i = 0 ; i < 12 ; i++)
        {
            /* Use magic to only use some bits. */
            temp = key & magic[i];
 
            /* Compute parity across 8 bytes. */
            parity = 0;
            for (j = 0 ; j < 8 ; j++) 
            {
                parity ^= fmRootApi->l2lHashTable[temp & 0xff];
                temp >>= 8;
            }
 
            /* Set hash bit if count is odd. */
            if (parity == 1)
            {
                hash |= (1 << i);
            }
 
        }   /* end for (i = 0 ; i < 12 ; i++) */
       
        /* Save this hash index. */
        hashes[setId] = (fm_uint16) hash;
        
        /* Rotate key for next set. */
        if (hashMode == 0)
        {    
            key = (key >> 8) | (key << 56);
        }

    }   /* end for (setId = 0 ; setId < FM10000_MAC_ADDR_BANK_COUNT ; ++setId) */
 
    return FM_OK;

}   /* end fm10000CalcAddrHash */




/*****************************************************************************/
/** fm10000CheckVlanMembership
 * \ingroup intAddr
 *
 * \desc            Verifies whether a port is a member of a vlan.
 * 
 * \note            This function assumes that the caller has taken the
 *                  L2_LOCK beforehand.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vlanID is the vlan identifier.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if the port is a member of the vlan.
 * \return          FM_ERR_INVALID_PORT if the port number is invalid.
 * \return          FM_ERR_INVALID_VLAN if the vlan identifier is invalid.
 * \return          FM_ERR_PORT_NOT_VLAN_MEMBER if the port is not a member
 *                  of the vlan.
 *
 *****************************************************************************/
fm_status fm10000CheckVlanMembership(fm_int sw, fm_uint16 vlanID, fm_int port)
{
    fm_status   err;
    fm_bool     isMember;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, "sw=%d vlanID=%u port=%d\n", sw, vlanID, port);

    if (vlanID >= FM_MAX_VLAN)
    {
        err = FM_ERR_INVALID_VLAN;
        goto ABORT;
    }

    err = fm10000GetVlanMembership(sw,
                                   GET_VLAN_PTR(sw, vlanID),
                                   port,
                                   &isMember);

    if (err == FM_OK && !isMember)
    {
        err = FM_ERR_PORT_NOT_VLAN_MEMBER;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000CheckVlanMembership */




/*****************************************************************************/
/* fm10000ComputeAddressIndex
 * \ingroup intAddr
 *
 * \desc            Computes the index(es) of an MA Table entry.
 *                  Called through the ComputeAddressIndex function pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       macAddr is the MAC address for the entry.
 *
 * \param[in]       vlanID is the VLAN number for the entry.
 * 
 * \param[in]       vlanID2 is ignored.
 *
 * \param[out]      indexes is an array into which this function should place
 *                  the indexes of each of the hash bin entries for the
 *                  specified MAC address and VLAN. Note that each entry
 *                  in the array is an absolute index into the MAC address
 *                  table. The length of the array must be equal to 
 *                  switchPtr->macTableBankCount and every element of the 
 *                  array will be given a valid index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ComputeAddressIndex(fm_int     sw,
                                     fm_macaddr macAddr,
                                     fm_uint16  vlanID,
                                     fm_uint16  vlanID2,
                                     fm_uint16 *indexes)
{
    fm_switch * switchPtr;
    fm_uint32   rv;
    fm_uint32   hashMode;
    fm_int      i;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, 
                 "sw=%d macAddr=" FM_FORMAT_ADDR " vlan=%d indexes=%p\n",
                 sw, 
                 macAddr, 
                 vlanID, 
                 (void *) indexes);

    FM_NOT_USED(vlanID2);

    if (indexes == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }
    
    switchPtr = GET_SWITCH_PTR(sw);
    
    /* Get hash mode from configuration register. */
    err = switchPtr->ReadUINT32(sw, 
                                FM10000_MA_TABLE_CFG_1(),
                                &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, err);

    hashMode = FM_GET_BIT(rv, FM10000_MA_TABLE_CFG_1, HashMode);

    /* Compute the L2 hash values. */
    fm10000CalcAddrHash(macAddr, vlanID, hashMode, indexes);

#if 0
    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "indexes[0]=%u indexes[1]=%u indexes[2]=%u indexes[3]=%u\n",
                 indexes[0], indexes[1], indexes[2], indexes[3]);
#endif

    /* Convert hash values to MAC table indexes. */
    for (i = 0 ; i < FM10000_MAC_ADDR_BANK_COUNT ; ++i)
    {
        indexes[i] += FM10000_MAC_ADDR_BANK_SIZE * i;
    }
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000ComputeAddressIndex */




/*****************************************************************************/
/* fm10000ConvertEntryToWords
 * \ingroup intAddr
 *
 * \desc            Converts an MA Table entry structure to an array of
 *                  four words.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       entry points to the MA Table entry structure.
 * 
 * \param[in]       port is the logical port number to be converted.
 *
 * \param[out]      words points to the buffer where the resulting 4 words
 *                  are to be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid arguments.
 * \return          FM_ERR_INVALID_PORT if the port could not be converted
 *                  to a valid glort.
 *
 *****************************************************************************/
fm_status fm10000ConvertEntryToWords(fm_int                   sw,
                                     fm_internalMacAddrEntry *entry,
                                     fm_int                   port,
                                     fm_uint32 *              words)
{
    fm_switch * switchPtr;
    fm_uint32   glort;
    fm_status   status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d entry=%p<%012llx:%d => %d> words=%p\n",
                         sw,
                         (void *) entry,
                         (entry ? entry->macAddress : 0),
                         (entry ? entry->vlanID : 0),
                         port,
                         (void *) words);

    switchPtr = GET_SWITCH_PTR(sw);
    status = FM_OK;

    if (entry == NULL || words == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /* Reset the buffer.  */
    memset((void *) words, 0, FM10000_MA_TABLE_WIDTH * sizeof(fm_uint32));

    if (entry->state != FM_MAC_ENTRY_STATE_INVALID)
    {
        FM_ARRAY_SET_FIELD64(words,
                             FM10000_MA_TABLE,
                             MACAddress,
                             entry->macAddress);

        FM_ARRAY_SET_FIELD(words, FM10000_MA_TABLE, FID, entry->vlanID);

        FM_ARRAY_SET_BIT(words, FM10000_MA_TABLE, valid, 1);

        if (entry->secure)
        {
            FM_ARRAY_SET_BIT(words, FM10000_MA_TABLE, secure, 1);
        }

        if (port != -1)
        {
            status = fmGetLogicalPortGlort(sw, port, &glort);
            if (status != FM_OK)
            {
                goto ABORT;
            }

            FM_ARRAY_SET_FIELD(words, FM10000_MA_TABLE, glort, glort);
        }

        if (entry->trigger != FM_DEFAULT_TRIGGER)
        {
            FM_ARRAY_SET_FIELD(words, FM10000_MA_TABLE, trigId, entry->trigger);
        }
    }

ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, status);

}   /* end fm10000ConvertEntryToWords */




#if 0

/*****************************************************************************/
/* fm10000ConvertWordsToEntry
 * \ingroup intAddr
 *
 * \desc            Converts an array of four words to an MA Table entry
 *                  structure.
 *
 * \note            The destination mask holds logical ports.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      entry points to the MA Table entry structure to be filled
 *                  in by this function.
 *
 * \param[in]       words points to the buffer where the 4 words
 *                  are located.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ConvertWordsToEntry(fm_int                   sw,
                                     fm_internalMacAddrEntry *entry,
                                     fm_uint32 *              words)
{

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d entry=%p words=%p<%08x%08x%08x%08x>\n",
                         sw,
                         (void *) entry,
                         (void *) words,
                         (words ? words[3] : 0),
                         (words ? words[2] : 0),
                         (words ? words[1] : 0),
                         (words ? words[0] : 0));

    FM_NOT_USED(sw);
    FM_NOT_USED(entry);
    FM_NOT_USED(words);

    /*************************************************** 
     * This function is not supported on the FM10000 
     * because the hardware registers do not contain
     * all the information necessary to recover the
     * MAC Table entry.
     **************************************************/

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, FM_ERR_UNSUPPORTED);

}   /* end fm10000ConvertWordsToEntry */

#endif  /* 0 */




/*****************************************************************************/
/** fm10000FillInUserEntryFromTable
 * \ingroup intAddr
 *
 * \desc            Fills in a user MA Table entry structure from an
 *                  internal MA Table entry structure. Called through
 *                  the FillInUserEntryFromTable function pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       tblentry is a pointer to the source internal structure.
 *
 * \param[out]      entry is a pointer to the destination user structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FillInUserEntryFromTable(fm_int                   sw,
                                          fm_internalMacAddrEntry *tblentry,
                                          fm_macAddressEntry *     entry)
{

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d tblentry=%p entry=%p\n",
                 sw,
                 (void *) tblentry,
                 (void *) entry);

    FM_CLEAR(*entry);
    
    entry->destMask   = FM_DESTMASK_UNUSED;
    entry->age        = (tblentry->state == FM_MAC_ENTRY_STATE_YOUNG) ? 1 : 0;
    entry->vlanID     = tblentry->vlanID;
    entry->macAddress = tblentry->macAddress;
    entry->port       = tblentry->port;
    entry->type       = tblentry->addrType;

    FM_LOG_DEBUG(FM_LOG_CAT_ADDR,
                 "macAddress=%012llx vlanID=%u port=%d type=%s(%u) age=%d\n",
                 entry->macAddress,
                 entry->vlanID,
                 entry->port,
                 fmAddressTypeToText(entry->type),
                 entry->type,
                 entry->age);

    FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_OK);

}   /* end fm10000FillInUserEntryFromTable */




/*****************************************************************************/
/** fm10000GetAddress
 * \ingroup intAddr
 *
 * \desc            Returns the MA Table entry that matches the specified
 *                  MAC address and VLAN ID. Called through the
 *                  GetAddressOverride function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       address is the MAC address to look up.
 *
 * \param[in]       vlanID is the VLAN number to look up.
 * 
 * \param[in]       vlanID2 is not used.
 *
 * \param[out]      entry points to an ''fm_macAddressEntry'' structure
 *                  to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ADDR_NOT_FOUND if address/vlan combination not
 *                  found in the MA Table.
 *
 *****************************************************************************/
fm_status fm10000GetAddress(fm_int              sw,
                            fm_macaddr          address,
                            fm_uint16           vlanID,
                            fm_uint16           vlanID2,
                            fm_macAddressEntry *entry)
{
    fm_internalMacAddrEntry cacheEntry;

    fm_switch * switchPtr;
    fm_uint16   indexes[FM10000_MAC_ADDR_BANK_COUNT];
    fm_status   status;
    fm_int      bankId;
    fm_uint16   learningFID;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d address=" FM_FORMAT_ADDR " vlanID=%d\n",
                 sw,
                 address,
                 vlanID);

    FM_NOT_USED(vlanID2);

    switchPtr = GET_SWITCH_PTR(sw);

    /***************************************************
     * The MAC address table entry is read from the
     * software cache, since on the FM10000 not all
     * of the information is stored in the hardware.
     **************************************************/

    status = fm10000GetLearningFID(sw, vlanID, &learningFID);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, status);

    status = fm10000ComputeAddressIndex(sw,
                                        address,
                                        learningFID,
                                        0,
                                        indexes);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ADDR, status);

    for (bankId = switchPtr->macTableBankCount - 1 ; bankId >= 0 ; bankId--)
    {
        FM_TAKE_L2_LOCK(sw);
        cacheEntry = switchPtr->maTable[indexes[bankId]];
        FM_DROP_L2_LOCK(sw);

        if ( (cacheEntry.state != FM_MAC_ENTRY_STATE_INVALID) &&
             (cacheEntry.macAddress == address) &&
             (cacheEntry.vlanID == learningFID) )
        {
            /* Found the requested entry. */
            status = fm10000FillInUserEntryFromTable(sw,
                                                     &cacheEntry,
                                                     entry);
            goto ABORT;
        }
    }

    status = FM_ERR_ADDR_NOT_FOUND;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ADDR, status);

}   /* end fm10000GetAddress */




/*****************************************************************************/
/** fm10000GetAddressTable
 * \ingroup intAddr
 *
 * \desc            Retrieves a copy of the entire MA Table. Called through
 *                  the GetAddressTable function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      nEntries points to a caller-supplied location where
 *                  this function is to store the number of MA Table entries
 *                  retrieved. May not be NULL.
 *
 * \param[out]      entries points to an array of fm_macAddressEntry
 *                  structures that will be filled in by this function with
 *                  MA Table entries.  The array must be large enough to
 *                  hold maxEntries number of MA Table entries.
 *
 * \param[in]       maxEntries is the maximum number of addresses that
 *                  the entries array can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetAddressTable(fm_int              sw,
                                 fm_int *            nEntries,
                                 fm_macAddressEntry *entries,
                                 fm_int              maxEntries)
{
    fm_switch *             switchPtr;
    fm_internalMacAddrEntry cacheEntry;
    fm_status               result = FM_OK;
    fm_status               status;
    fm_int                  i;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR,
                 "sw=%d nEntries=%p entries=%p maxEntries=%d\n",
                 sw,
                 (void *) nEntries,
                 (void *) entries,
                 maxEntries);

    switchPtr = GET_SWITCH_PTR(sw);

    *nEntries = 0;

    /***************************************************
     * The MAC address table is read from the software
     * cache, since on the FM10000 not all of the
     * information is stored in the hardware.
     *
     * Whenever an error is encountered, either because 
     * the entry could not be retrieved or due to a 
     * conversion error, the entry in question is skipped. 
     * If multiple errors occur, the user is only notified
     * of the reason for the first error.
     **************************************************/
    for (i = 0 ; i < switchPtr->macTableSize ; i++)
    {
        FM_TAKE_L2_LOCK(sw);
        cacheEntry = switchPtr->maTable[i];
        FM_DROP_L2_LOCK(sw);

        if (cacheEntry.state != FM_MAC_ENTRY_STATE_INVALID)
        {
            /* entries == NULL is used to count the number of MAC entry */
            if (entries != NULL)
            {
                if (*nEntries >= maxEntries)
                {
                    result = FM_ERR_BUFFER_FULL;
                    goto ABORT;
                }

                status = fm10000FillInUserEntryFromTable(sw,
                                                         &cacheEntry,
                                                         &entries[*nEntries]);
                if (status != FM_OK)
                {
                    FM_ERR_COMBINE(result, status);
                    continue;
                }
            }
            (*nEntries)++;
        }
    }
    
ABORT:        
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, result);

}   /* end fm10000GetAddressTable */




/*****************************************************************************/
/** fm10000GetAddressTableAttribute
 * \ingroup intAddr
 *
 * \desc            Retrieves the value of an MA table related attribute.
 *                  Called through the GetAddressTableAttribute function
 *                  pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the MA Table attribute to retrieve
 *                  (see 'MA Table Attributes').
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function should place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fm10000GetAddressTableAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_uint32 *      val32Ptr;
    fm_switch *      switchPtr;
    fm10000_switch * switchExt;
    fm_status        err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, 
                 "sw=%d attr=%d value=%p\n",
                 sw, 
                 attr, 
                 (void *) value);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    /* get a fm_uint32 pointer, since that is what most of the attributes use */
    val32Ptr = (fm_uint32 *) value;
    
    switch (attr)
    {
        case FM_MAC_TABLE_SIZE:
            *val32Ptr = sizeof(fm_macAddressEntry) * switchPtr->macTableSize;
            break;

        case FM_MAC_TABLE_ADDRESS_AGING_TIME:
        case FM_ADDRESS_AGING_TIME:
            /* Convert from msec to sec. */
            *val32Ptr = switchPtr->macAgingTicks / 1000;
            break;

        case FM_MAC_TABLE_SECURITY_ACTION:
            *val32Ptr = switchExt->macSecurityAction;
            break;
            
        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

        
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000GetAddressTableAttribute */




/*****************************************************************************/
/** fm10000GetLearningFID
 * \ingroup intAddr
 *
 * \desc            Gets the FID on which MAC addresses are learned.
 *                  Called through the GetLearningFID function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vlanId is the VLAN ID for the MAC address in question.
 *
 * \param[out]      learningFid is the FID on which MAC addresses in vlanID
 *                  are learned on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetLearningFID(fm_int      sw, 
                                fm_uint16   vlanId, 
                                fm_uint16 * learningFid)
{
    fm_switch * switchPtr;
    fm_status   status;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, 
                 "sw=%d vlanId=%d learningFid=%p\n",
                 sw, 
                 vlanId, 
                 (void *) learningFid);

    switchPtr = GET_SWITCH_PTR(sw);

    switch (switchPtr->vlanLearningMode)
    {
        case FM_VLAN_LEARNING_MODE_INDEPENDENT:
            *learningFid = vlanId;
            break;

        case FM_VLAN_LEARNING_MODE_SHARED:
            *learningFid = switchPtr->sharedLearningVlan;
            break;

        default:
            FM_LOG_ERROR(FM_LOG_CAT_ADDR, "Invalid vlan learning mode\n");
            status = FM_FAIL;
            goto ABORT;
    }

    status = FM_OK;

    FM_LOG_DEBUG(FM_LOG_CAT_ADDR, "learningFid=%u\n", *learningFid);
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, status);
    
}   /* end fm10000GetLearningFID */




/*****************************************************************************/
/** fm10000InitAddrHash
 * \ingroup intAddr
 *
 * \desc            Initialize data table needed to calculate MAC table
 *                  hash function.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if the l2lHashTable could not be allocated.
 *
 *****************************************************************************/
fm_status fm10000InitAddrHash(void)
{
    fm_int i;
    fm_int j;
    fm_int parity;

    if (fmRootApi->l2lHashTable != NULL)
    {
        /* Already initialized, just exit */
        FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_OK);
    }

    fmRootApi->l2lHashTable =
        (fm_int *) fmAlloc(sizeof(fm_int) * L2L_HASH_TABLE_SIZE);

    if (fmRootApi->l2lHashTable == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_ERR_NO_MEM);
    }
    
    for (i = 0 ; i < L2L_HASH_TABLE_SIZE ; i++)
    {
        parity = 0;
        
        for (j = 0 ; j < 8 ; j++) 
        {
            if ( i & (1 << j) )
            {
                parity ^= 1; 
            }
        }
        
        fmRootApi->l2lHashTable[i] = parity;
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, FM_OK);
        
}   /* end fm10000InitAddrHash */




/*****************************************************************************/
/** fm10000InitAddressTable
 * \ingroup intAddr
 *
 * \desc            Initializes the local copy of the MA Table. Called
 *                  through the InitAddressTable function pointer.
 *
 * \param[in]       switchPtr points to the switch's state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitAddressTable(fm_switch * switchPtr)
{
    fm_status err;
    fm_int    i;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH,
                 "switchPtr=%p, sw=%d\n",
                 (void *) switchPtr,
                 switchPtr->switchNumber);

    for (i = 0 ; i < switchPtr->macTableSize ; i++)
    {
        switchPtr->maTable[i].state = FM_MAC_ENTRY_STATE_INVALID;
    }

    err = fm10000InitAddrHash();
    
    FM_LOG_EXIT(FM_LOG_CAT_ADDR | FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitAddressTable */




/*****************************************************************************/
/** fm10000InvalidateEntryAtIndex
 * \ingroup intAddr
 *
 * \desc            Invalidates the MA Table entry at the specified index.
 *                  Updates both cache and hardware values.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       index is the MA Table index whose entry is to be
 *                  invalidated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InvalidateEntryAtIndex(fm_int sw, fm_uint32 index)
{
    fm_switch *              switchPtr;
    fm_internalMacAddrEntry *cachePtr;
    fm_status                status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR, "sw=%d index=%u\n", sw, index);

    switchPtr = GET_SWITCH_PTR(sw);

    cachePtr = &switchPtr->maTable[index];

    /* Invalidate software cache entry. */
    FM_CLEAR(*cachePtr);

    /* Invalidate hardware entries. */
    status = fm10000WriteEntryAtIndex(sw, index, cachePtr);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, status);

}   /* end fm10000InvalidateEntryAtIndex */




#if 0

/*****************************************************************************/
/** fm10000ReadEntryAtIndex
 * \ingroup intAddr
 *
 * \desc            Utility function to read an MA Table entry in from the
 *                  hardware registers and convert it into an MA Table entry
 *                  structure.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       index is the MA Table index from which the entry is to be
 *                  read. 
 *
 * \param[out]      entry points to the MA Table entry structure to hold
 *                  the information retrieved from the hardware.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ReadEntryAtIndex(fm_int                   sw,
                                  fm_uint32                index,
                                  fm_internalMacAddrEntry *entry)
{

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d index=%u entry=%p\n",
                         sw,
                         index,
                         (void *) entry);

    FM_NOT_USED(sw);
    FM_NOT_USED(index);
    FM_NOT_USED(entry);

    /*************************************************** 
     * This function is not supported on the FM10000 
     * because the hardware registers do not contain
     * all the information necessary to recover the
     * MAC Table entry.
     **************************************************/

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, FM_ERR_UNSUPPORTED);

}   /* end fm10000ReadEntryAtIndex */

#endif  /* 0 */




/*****************************************************************************/
/** fm10000SetAddressTableAttribute
 * \ingroup intAddr
 *
 * \desc            Sets the value of an MA table related attribute. Called
 *                  through the SetAddressTableAttribute function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the MA Table attribute to retrieve
 *                  (see 'MA Table Attributes').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT attr is read-only or value is
 *                  invalid.
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fm10000SetAddressTableAttribute(fm_int sw, fm_int attr, void *value)
{
    fm_uint32       val32;
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_status       err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_ADDR, 
                 "sw=%d attr=%d value=%p\n",
                 sw, 
                 attr, 
                 (void *) value);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    switch (attr)
    {
        case FM_MAC_TABLE_SIZE:
            /* read-only attribute */
            err = FM_ERR_UNSUPPORTED;
            break;

        case FM_MAC_TABLE_ADDRESS_AGING_TIME:
        case FM_ADDRESS_AGING_TIME:
            /* Get specified number of seconds */
            val32 = *( (fm_uint32 *) value );

            /* fm_uint32 overflow validation */
            if (val32 > MAX_UINT_VALUE / FM_TICKS_PER_SECOND)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                break;
            }

            /* Convert to milliseconds and store */
            switchPtr->macAgingTicks = (fm_uint) val32 * FM_TICKS_PER_SECOND;
            break;

        case FM_MAC_TABLE_SECURITY_ACTION:
            val32 = *( (fm_uint32 *) value );

            switch (val32)
            {
                case FM_MAC_SECURITY_ACTION_DROP:
                case FM_MAC_SECURITY_ACTION_EVENT:
                case FM_MAC_SECURITY_ACTION_TRAP:
                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    goto ABORT;
            }

            switchExt->macSecurityAction = val32;
            err = fm10000UpdateMacSecurity(sw);
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */
    
ABORT:        
    FM_LOG_EXIT(FM_LOG_CAT_ADDR, err);

}   /* end fm10000SetAddressTableAttribute */




/*****************************************************************************/
/** fm10000WriteEntryAtIndex
 * \ingroup intAddr
 *
 * \desc            Converts an MA Table entry structure to register values
 *                  and writes it to the hardware registers. Called through
 *                  the WriteEntryAtIndex function pointer.
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
fm_status fm10000WriteEntryAtIndex(fm_int                   sw,
                                   fm_uint32                index,
                                   fm_internalMacAddrEntry *entry)
{
    fm_switch * switchPtr;
    fm_status   status;
    fm_status   retVal;
    fm_uint32   words[FM10000_MA_TABLE_WIDTH];

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d index=%d entry=%p\n",
                         sw,
                         index,
                         (void *) entry);

    switchPtr = GET_SWITCH_PTR(sw);

    retVal = fm10000ConvertEntryToWords(sw, entry, entry->port, words);

    if (retVal  == FM_OK)
    {
        /* write DMAC table entry */
        status = switchPtr->WriteUINT32Mult(sw,
                                            FM10000_MA_TABLE(0, index, 0),
                                            FM10000_MA_TABLE_WIDTH,
                                            words);
        FM_ERR_COMBINE(retVal, status);

        /* write SMAC table entry */
        status = switchPtr->WriteUINT32Mult(sw,
                                            FM10000_MA_TABLE(1, index, 0),
                                            FM10000_MA_TABLE_WIDTH,
                                            words);
        FM_ERR_COMBINE(retVal, status);

        /* reset USED table entry */
        status = ResetUsedEntry(sw, index);
        FM_ERR_COMBINE(retVal, status);

        if (retVal != FM_OK)
        {
            /* Error writing MAC table entry. */
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_WRITE_ERR, 1);
        }

    }   /* end if (status == FM_OK) */

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, retVal );

}   /* end fm10000WriteEntryAtIndex */




/*****************************************************************************/
/** fm10000WriteSmacEntry
 * \ingroup intAddr
 *
 * \desc            Converts an MA Table entry structure to register values
 *                  and writes it to the hardware SMAC table.
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
fm_status fm10000WriteSmacEntry(fm_int                    sw,
                                fm_uint32                 index,
                                fm_internalMacAddrEntry * entry)
{
    fm_switch * switchPtr;
    fm_uint32   words[FM10000_MA_TABLE_WIDTH];
    fm_status   retVal;
    fm_status   status;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ADDR,
                         "sw=%d index=%d entry=%p\n",
                         sw,
                         index,
                         (void *) entry);

    switchPtr = GET_SWITCH_PTR(sw);

    retVal = fm10000ConvertEntryToWords(sw, entry, entry->port, words);

    if (retVal == FM_OK)
    {
        /* write SMAC table entry */
        status = switchPtr->WriteUINT32Mult(sw,
                                            FM10000_MA_TABLE(1, index, 0),
                                            FM10000_MA_TABLE_WIDTH,
                                            words);
        FM_ERR_COMBINE(retVal, status);

        /* reset USED table entry */
        status = ResetUsedEntry(sw, index);
        FM_ERR_COMBINE(retVal, status);

        if (retVal != FM_OK)
        {
            /* Error writing MAC table entry. */
            fmDbgDiagCountIncr(sw, FM_CTR_MAC_WRITE_ERR, 1);
        }

    }   /* end if (status == FM_OK) */

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ADDR, retVal);

}   /* end fm10000WriteSmacEntry */

