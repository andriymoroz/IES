/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_mac_purge_table.c
 * Creation Date:   July 30, 2010
 * Description:     MAC address table purge handling.
 *
 * Copyright (c) 2010 - 2015, Intel Corporation
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

#define PURGE_CAP_PORT      (1 << 0)
#define PURGE_CAP_VID1      (1 << 1)
#define PURGE_CAP_VID2      (1 << 2)
#define PURGE_CAP_REMID     (1 << 3)
#define PURGE_CAP_HW        (1 << 4)
#define PURGE_CAP_NOP       (1 << 5)
#define PURGE_CAP_EXP       (1 << 6)

#define PURGE_CAP_STD       (PURGE_CAP_PORT | PURGE_CAP_VID1)
#define PURGE_CAP_ADV       (PURGE_CAP_VID2 | PURGE_CAP_REMID)

#define PURGE_CAP_FM4000    (PURGE_CAP_STD | PURGE_CAP_NOP | PURGE_CAP_HW)
#define PURGE_CAP_FM6000    (PURGE_CAP_STD | PURGE_CAP_ADV | PURGE_CAP_HW)
#define PURGE_CAP_FM10000   (PURGE_CAP_STD | PURGE_CAP_EXP)


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
/** AllocatePurgeListEntry
 * \ingroup intAddr
 *
 * \desc            Allocate and initialize a purge list entry
 *
 * \param[in]       entry is the pointer to the pointer that will point to the
 *                  newly allocated purge list entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status AllocatePurgeListEntry(fm_maPurgeListEntry **entry)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "entry = %p\n",
                 (void *) entry);

    /* Allocate a new purge list entry. */
    *entry = fmAlloc( sizeof(fm_maPurgeListEntry) );
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           *entry != NULL,
                           err = FM_ERR_NO_MEM,
                           "Unable to allocate MA Table purge list entry\n");

    memset(*entry, 0, sizeof(fm_maPurgeListEntry));
    err = fmCreateBitArray(&((*entry)->pendingVlans), FM_MAX_VLAN);
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           err == FM_OK,
                           err = FM_ERR_NO_MEM,
                           "Unable to allocate pending VLAN purge bit array\n");

    /* Initialize vid2Tree */
    fmTreeInit( &( (*entry)->vid2Tree ) );

    /* Initialize remoteIdTree */
    fmTreeInit( &( (*entry)->remoteIdTree ) );

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}    /* end AllocatePurgeListEntry */




/*****************************************************************************/
/** GetPurgeCapabilities
 * \ingroup intAddr
 *
 * \desc            Returns the purge capabilities of the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          Bit mask indicating what purge capabilities the specified
 *                  switch supports.
 *
 *****************************************************************************/
static fm_int GetPurgeCapabilities(fm_int sw)
{
    fm_switch * switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    switch (switchPtr->switchFamily)
    {
        case FM_SWITCH_FAMILY_FM4000:
        case FM_SWITCH_FAMILY_REMOTE_FM4000:
            return PURGE_CAP_FM4000;

        case FM_SWITCH_FAMILY_FM6000:
        case FM_SWITCH_FAMILY_REMOTE_FM6000:
            return PURGE_CAP_FM6000;

        case FM_SWITCH_FAMILY_FM10000:
            return PURGE_CAP_FM10000;

        default:
            return 0;
    }

}   /* end GetPurgeCapabilities */




/*****************************************************************************/
/** GetPurgeEntry
 * \ingroup intAddr
 *
 * \desc            Retrieves (if exists) or creates a purge entry based on
 *                  a purge request.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       allPorts indicates if the purge applies to all ports.
 * 
 * \param[in]       port is the port that the purge applies to. Note that
 *                  port is only valid if allPorts=FALSE.
 * 
 * \param[in]       entry is a pointer to the pointer that will point to the
 *                  purge list entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status GetPurgeEntry(fm_int                sw,
                               fm_bool               allPorts, 
                               fm_int                port, 
                               fm_maPurgeListEntry **entry)
{
    fm_status   err = FM_OK;
    fm_switch * switchPtr;
    fm_maPurge *purgePtr;
    fm_port *   portPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw = %d, allPorts = %d, port = %d, entry = %p\n",
                 sw,
                 allPorts,
                 port,
                 (void *) entry);

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;

    if (!allPorts)
    {
        portPtr = GET_PORT_PTR(sw, port);

        if (portPtr == NULL)
        {
            ++purgePtr->stats.numRequestedInvalidPort;
            err = FM_ERR_INVALID_PORT;
            goto ABORT;
        }
    }

    /* Find or allocate the maPurgeListEntry object for the port. */
    if (allPorts)
    {
        *entry = purgePtr->globalListEntry;
    }
    else
    {    
        /**************************************************
         * If we have not yet allocated a purge list
         * entry for this port, then do so now.
         **************************************************/
        
        if (!portPtr->maPurgeListEntry)
        {
            /* Allocate a new purge list entry. */
            ++purgePtr->stats.numEntriesAllocated;

            err = AllocatePurgeListEntry(entry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, err);
            
            /* Add the entry to the tail of the purge list. */
            FM_DLL_INSERT_LAST(purgePtr, listHead, listTail, *entry, next, prev);
            
            /* Store the port and glort of the entry. */
            (*entry)->port = port;
            (*entry)->glort = portPtr->glort;
            (*entry)->portExists = TRUE;

            /* Store the entry in the logical port structure. */
            portPtr->maPurgeListEntry = *entry;
            
        }   /* end if (!portPtr->maPurgeListEntry) */
        
        *entry = portPtr->maPurgeListEntry;
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      (*entry)->glort == portPtr->glort,
                      "Purge glort (0x%04x) and port glort (0x%04x) don't "
                      "match!\n",
                      (*entry)->glort,
                      portPtr->glort);

    }   /* end if (allPorts) */

    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           (*entry) != NULL,
                           err = FM_FAIL,
                           "Couldn't obtain purge list entry!\n");
    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                  (*entry)->port == (allPorts ? -1 : port),
                  "Purge port (%d) and specified port (%d) don't match!\n",
                  (*entry)->port,
                  (allPorts ? -1 : port));

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}    /* end GetPurgeEntry */




/*****************************************************************************/
/** GetBitArray
 * \ingroup intAddr
 *
 * \desc            This function tries to find a bitArray in a tree
 *                  of a purge entry.
 * 
 * \param[in]       tree is a pointer to the tree
 * 
 * \param[in]       key is the vid1 which is used as the tree key. The
 *                  VID2_ONLY_FLUSH macro should be used for vid2 only flush.
 *                  
 * \param[in]       bitArray is a pointer to the pointer where the bitArray
 *                  should be saved.
 *
 * \return          TRUE if bitArray is found
 *                  FALSE if bitArray not found
 *
 *****************************************************************************/
static fm_bool GetBitArray(fm_tree *     tree, 
                           fm_int        key, 
                           fm_bitArray **bitArray)
{
    fm_status err;
    fm_bool   retval;    

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_MAC_MAINT,
                         "tree=%p, key=%d, bitArray=%p\n",
                         (void *) tree,
                         key,
                         (void *) bitArray);  

    err = fmTreeFind(tree, key, (void **) bitArray);
    retval = (err == FM_OK);

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_EVENT_MAC_MAINT,
                               retval,
                               "%s\n",
                               retval ? "TRUE" : "FALSE")

}   /* end GetBitArray */




/*****************************************************************************/
/** InsertTreeKey
 * \ingroup intAddr
 *
 * \desc            Inserts an entry for a tree
 * 
 * \param[in]       tree is a pointer to the tree
 * 
 * \param[in]       key is the key that will be inserted into the tree
 * 
 * \param[in]       bitArraySize is the size of the bit array to create
 * 
 * \param[out]      bitArray is a pointer to the newly allocated bitArray
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
static fm_status InsertTreeKey(fm_tree *     tree, 
                               fm_int        key,
                               fm_int        bitArraySize, 
                               fm_bitArray **bitArray)
{
    fm_status    err;
    fm_status    err2;
    fm_bitArray *bitArrayPtr;
    fm_bool      bitArrayAllocated;
    fm_bool      bitArrayCreated;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "tree=%p, key,=%d, bitArray=%p\n",
                 (void *) tree,
                 key,
                 (void *) bitArray);

    bitArrayAllocated = FALSE;
    bitArrayCreated   = FALSE;

    bitArrayPtr = fmAlloc( sizeof(fm_bitArray) );
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           bitArrayPtr != NULL,
                           err = FM_ERR_NO_MEM,
                           "Couldn't allocate bitArray!\n");

    bitArrayAllocated = TRUE;

    err = fmCreateBitArray(bitArrayPtr, bitArraySize);
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           err == FM_OK,
                           err = err,
                           "Unable to create bit array\n");

    bitArrayCreated = TRUE;

    err = fmTreeInsert(tree, 
                       key, 
                       bitArrayPtr);
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           err == FM_OK,
                           err = err,
                           "Unable to insert bit array in tree\n");

    *bitArray = bitArrayPtr;

ABORT:

    if (err != FM_OK)
    {
        if (bitArrayCreated)
        {
            err2 = fmDeleteBitArray(bitArrayPtr);
            if (err2 != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_EVENT_MAC_MAINT,
                              "Error deleting bit array: %s\n",
                              fmErrorMsg(err2) );
            }
        }

        if (bitArrayAllocated)
        {
            fmFree(bitArrayPtr);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}    /* end InsertTreeKey */




/*****************************************************************************/
/** RemoveVid2TreeKey
 * \ingroup intAddr
 *
 * \desc            Remove an entry for a vid2Tree
 * 
 * \param[in]       purgeEntry is entry on which keys should be removed from.
 * 
 * \param[in]       key is the vid2 key that will be removed from the tree
 * 
 * \param[in]       bitArray is the address of the bitArray that is in the tree.
 *                  This parameter is provided for faster removal. Set to NULL
 *                  if unknown.
 * 
 * \param[in]       removePendingVid1 is to remove the vid1 from pendingVlans
 * 
 * \return          none
 *
 *****************************************************************************/
static void RemoveVid2TreeKey(fm_maPurgeListEntry *purgeEntry, 
                              fm_int               key, 
                              fm_bitArray *        bitArray, 
                              fm_bool              removePendingVid1)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "purgeEntry=%p, key=%d, bitArray=%p, removePendingVid1=%s\n",
                 (void *) purgeEntry,
                 key,
                 (void *) bitArray,
                 removePendingVid1 ? "TRUE" : "FALSE");

    if (bitArray == NULL)
    {
        err = fmTreeFind(&purgeEntry->vid2Tree, key, (void **) &bitArray);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err == FM_OK,
                      "fmTreeFind returned error!\n");
    }

    err = fmDeleteBitArray(bitArray);
    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                  err == FM_OK,
                  "fmDeleteBitArray returned error!\n");

    fmFree(bitArray);

    err = fmTreeRemove(&purgeEntry->vid2Tree, key, NULL);
    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                  err == FM_OK,
                  "fmTreeRemove returned error!\n");

    if (removePendingVid1)
    {
        err = fmSetBitArrayBit(&purgeEntry->pendingVlans, key, FALSE);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err == FM_OK,
                      "fmSetBitArrayBit returned error!\n");
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_EVENT_MAC_MAINT);

}    /* end RemoveVid2TreeKey */




/*****************************************************************************/
/** RemoveRemIdTreeKey
 * \ingroup intAddr
 *
 * \desc            Remove an entry for a remoteIdTree
 * 
 * \param[in]       purgeEntry is entry on which keys should be removed from.
 * 
 * \param[in]       key is the key that will be removed from the tree
 * 
 * \param[in]       bitArray is the address of the bitArray that is in the tree.
 *                  This parameter is provided for faster removal. Set to NULL
 *                  if unknown.
 * 
 * \param[in]       removePendingRemId is to remove the remId from pendingVlans
 * 
 * \return          none
 *
 *****************************************************************************/
static void RemoveRemIdTreeKey(fm_maPurgeListEntry *purgeEntry, 
                               fm_int               key, 
                               fm_bitArray *        bitArray, 
                               fm_bool              removePendingRemId)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "purgeEntry=%p, key=%d, bitArray=%p, removePendingRemId=%s\n",
                 (void *) purgeEntry,
                 key,
                 (void *) bitArray,
                 removePendingRemId ? "TRUE" : "FALSE");

    if (bitArray == NULL)
    {
        err = fmTreeFind(&purgeEntry->remoteIdTree, key, (void **) &bitArray);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err == FM_OK,
                      "fmTreeFind returned error!\n");
    }

    err = fmDeleteBitArray(bitArray);
    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                  err == FM_OK,
                  "fmDeleteBitArray returned error!\n");

    fmFree(bitArray);

    err = fmTreeRemove(&purgeEntry->remoteIdTree, key, NULL);
    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                  err == FM_OK,
                  "fmTreeRemove returned error!\n");

    if (removePendingRemId)
    {
        err = fmSetBitArrayBit(&purgeEntry->pendingVlans, key, FALSE);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err == FM_OK,
                      "fmSetBitArrayBit returned error!\n");
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_EVENT_MAC_MAINT);

}    /* end RemoveRemIdTreeKey */




/*****************************************************************************/
/** RemoveMultipleVid2TreeKey
 * \ingroup intAddr
 *
 * \desc            Remove multiple entries for a vid2Tree, can also remove the
 *                  associated vid1 from pendingVlans
 * 
 * \param[in]       purgeEntry is entry on which keys should be removed from.
 * 
 * \param[in]       bitArray contains the indexes of the the vid1 indexes to
 *                  removed from the tree.
 * 
 * \param[in]       removePendingVid1 is to remove the vid1 from pendingVlans
 * 
 * \return          none
 *
 *****************************************************************************/
static void RemoveMultipleVid2TreeKey(fm_maPurgeListEntry *purgeEntry, 
                                      fm_bitArray *        bitArray,
                                      fm_bool              removePendingVid1)
{
    fm_status err;
    fm_int    iterVid1;
    fm_int    bitIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "purgeEntry=%p, bitArray=%p, removePendingVid1=%s\n",
                 (void *) purgeEntry,
                 (void *) bitArray,
                 removePendingVid1 ? "TRUE" : "FALSE");

    iterVid1 = 0;

    do
    {
        fmFindBitInBitArray(bitArray, 
                            (fm_int) iterVid1, 
                            TRUE, 
                            &bitIndex);

        if (bitIndex != -1)
        {
            RemoveVid2TreeKey(purgeEntry, 
                              (fm_int) bitIndex,
                              NULL,
                              removePendingVid1);

            err = fmSetBitArrayBit(bitArray, bitIndex, FALSE);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err == FM_OK,
                          "fmSetBitArrayBit returned error!\n");
        }

        iterVid1 = bitIndex;

    } while (bitIndex != -1);

    FM_LOG_EXIT_VOID(FM_LOG_CAT_EVENT_MAC_MAINT);
    
}    /* end RemoveMultipleVid2TreeKey */




/*****************************************************************************/
/** RemoveMultipleRemIdTreeKey
 * \ingroup intAddr
 *
 * \desc            Remove multiple entries for a remoteIdTree, can also remove
 *                  the associated vid1 from pendingVlans
 * 
 * \param[in]       purgeEntry is entry on which keys should be removed from.
 * 
 * \param[in]       bitArray contains the indexes of the the vid1 indexes to
 *                  removed from the tree.
 * 
 * \param[in]       removePendingVid1 is to remove the vid1 from pendingVlans
 * 
 * \return          none
 *
 *****************************************************************************/
static void RemoveMultipleRemIdTreeKey(fm_maPurgeListEntry *purgeEntry, 
                                       fm_bitArray *        bitArray,
                                       fm_bool              removePendingVid1)
{
    fm_status err;
    fm_int    iterRemId;
    fm_int    bitIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "purgeEntry=%p, bitArray=%p, removePendingVid1=%s\n",
                 (void *) purgeEntry,
                 (void *) bitArray,
                 removePendingVid1 ? "TRUE" : "FALSE");

    iterRemId = 0;

    do
    {
        fmFindBitInBitArray(bitArray, 
                            (fm_int) iterRemId, 
                            TRUE, 
                            &bitIndex);

        if (bitIndex != -1)
        {
            RemoveRemIdTreeKey(purgeEntry, 
                               (fm_int) bitIndex,
                               NULL,
                               removePendingVid1);

            err = fmSetBitArrayBit(bitArray, bitIndex, FALSE);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err == FM_OK,
                          "fmSetBitArrayBit returned error!\n");
        }

        iterRemId = bitIndex;

    } while (bitIndex != -1);

    FM_LOG_EXIT_VOID(FM_LOG_CAT_EVENT_MAC_MAINT);
    
}    /* end RemoveMultipleRemIdTreeKey */




/*****************************************************************************/
/** CancelAllPurgeEntries
 * \ingroup intAddr
 *
 * \desc            This function will cancel any pending purge that are
 *                  less specific than purge all.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CancelAllPurgeEntries(fm_int sw)
{
    fm_status            err = FM_OK;
    fm_status            err2;
    fm_switch *          switchPtr;
    fm_maPurge *         purgePtr;
    fm_maPurgeListEntry *tmpEntry;
    fm_maPurgeListEntry *tmp2Entry;
    fm_bitArray *        bitArrayPtr;
    fm_treeIterator      iter;
    fm_uint64            iterVid1;
    fm_bitArray          vid1DelArray;
    fm_bool              removeVid2OnlyFlush;
    fm_bool              removeRemIdOnlyFlush;
    
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw = %d\n",
                 sw);

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;

    err2 = fmCreateBitArray(&vid1DelArray, FM_MAX_VLAN);
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           err2 == FM_OK,
                           err = err2,
                           "Unable to create bit array\n");
    
    /* Cancel any other pending purges. */
    tmpEntry = purgePtr->listHead;
    
    while (tmpEntry != NULL)
    {
        /* All ports */
        if (tmpEntry->port == -1)
        {
            removeVid2OnlyFlush = FALSE;

            /* Iterate in the vid2tree to remove entries */
            for ( fmTreeIterInit(&iter, &tmpEntry->vid2Tree) ;
                  (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                )
            {
                if (iterVid1 == VID2_ONLY_FLUSH)
                {
                    /*  caseId=7 This will cancel vid2 specific purges */
                    purgePtr->stats.numCancelledVid2 += 
                        bitArrayPtr->nonZeroBitCount;

                    removeVid2OnlyFlush = TRUE;
                }
                else
                {
                    /* caseId=6 Cancel any pending vid1/vid2 specific
                     * purges for this port. */
                    purgePtr->stats.numCancelledVid1Vid2 += 
                        bitArrayPtr->nonZeroBitCount;

                    /* Tag tree item for removal (we cannot alter the tree
                     * while iterating) */
                    err2 = fmSetBitArrayBit(&vid1DelArray, 
                                            iterVid1, 
                                            TRUE);
                    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                  err2 == FM_OK,
                                  "fmSetBitArrayBit returned error!\n");
                }
            }

            /* Remove vid2Tree entries tagged for removal
             * and the pendingVlans associated*/
            if (removeVid2OnlyFlush)
            {
                RemoveVid2TreeKey(tmpEntry, VID2_ONLY_FLUSH, NULL, FALSE);
            }

            RemoveMultipleVid2TreeKey(tmpEntry, &vid1DelArray, TRUE);


            removeRemIdOnlyFlush = FALSE;

            /* Iterate in the remoteIdTree to remove entries */
            for ( fmTreeIterInit(&iter, &tmpEntry->remoteIdTree) ;
                  (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                )
            {
                if (iterVid1 == REMID_ONLY_FLUSH)
                {
                    /*  caseId=50 This will cancel remId specific purges */
                    purgePtr->stats.numCancelledRemoteId += 
                        bitArrayPtr->nonZeroBitCount;

                    removeRemIdOnlyFlush = TRUE;
                }
                else
                {
                    /* caseId=49 Cancel any pending vid1/remId specific
                     * purges for this port. */
                    purgePtr->stats.numCancelledVid1RemoteId += 
                        bitArrayPtr->nonZeroBitCount;

                    /* Tag tree item for removal (we cannot alter the tree
                     * while iterating) */
                    err2 = fmSetBitArrayBit(&vid1DelArray, 
                                            iterVid1, 
                                            TRUE);
                    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                  err2 == FM_OK,
                                  "fmSetBitArrayBit returned error!\n");
                }
            }

            /* Remove remoteIdTree entries tagged for removal
             * and the pendingVlans associated*/
            if (removeRemIdOnlyFlush)
            {
                RemoveRemIdTreeKey(tmpEntry, REMID_ONLY_FLUSH, NULL, FALSE);
            }

            RemoveMultipleRemIdTreeKey(tmpEntry, &vid1DelArray, TRUE);


            /* caseId=2 Cancel any vlan specific purges */
            purgePtr->stats.numCancelledVlan += 
                            tmpEntry->pendingVlans.nonZeroBitCount;
            err2 = fmClearBitArray(&tmpEntry->pendingVlans);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err2 == FM_OK,
                          "fmClearBitArray returned error!\n");
        }
        else
        {
            if (tmpEntry->allVlansPending)
            {
                /* caseId=1 Cancel any pending port specific purge */
                ++purgePtr->stats.numCancelledPort;
                tmpEntry->allVlansPending = FALSE;
            }
            else
            {
                removeVid2OnlyFlush = FALSE;
    
                /* Iterate in the vid2tree to remove entries */
                for ( fmTreeIterInit(&iter, &tmpEntry->vid2Tree) ;
                      (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                    )
                {
                    if (iterVid1 == VID2_ONLY_FLUSH)
                    {
                        /*  caseId=5 This will cancel vid2 specific purges */
                        purgePtr->stats.numCancelledPortVid2 += 
                            bitArrayPtr->nonZeroBitCount;
    
                        removeVid2OnlyFlush = TRUE;
                    }
                    else
                    {
                        /* caseId=4 Cancel any pending vid1/vid2 specific
                         * purges for this port. */
                        purgePtr->stats.numCancelledPortVid1Vid2 += 
                            bitArrayPtr->nonZeroBitCount;
    
                        /* Tag tree item for removal (we cannot alter the tree
                         * while iterating) */
                        err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                iterVid1, 
                                                TRUE);
                        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                      err2 == FM_OK,
                                      "fmSetBitArrayBit returned error!\n");
                    }
                }
    
                /* Remove vid2Tree entries tagged for removal
                 * and the pendingVlans associated*/
                if (removeVid2OnlyFlush)
                {
                    RemoveVid2TreeKey(tmpEntry, VID2_ONLY_FLUSH, NULL, FALSE);
                }
    
                RemoveMultipleVid2TreeKey(tmpEntry, &vid1DelArray, TRUE);


                removeRemIdOnlyFlush = FALSE;

                /* Iterate in the remoteIdTree to remove entries */
                for ( fmTreeIterInit(&iter, &tmpEntry->remoteIdTree) ;
                      (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                    )
                {
                    if (iterVid1 == REMID_ONLY_FLUSH)
                    {
                        removeRemIdOnlyFlush = TRUE;
                    }
                    else
                    {
                        /* caseId=40 Cancel any pending port/vid1/remoteId specific
                         * purges for this port. */
                        purgePtr->stats.numCancelledPortVid1RemoteId += 
                            bitArrayPtr->nonZeroBitCount;

                        /* Tag tree item for removal (we cannot alter the tree
                         * while iterating) */
                        err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                iterVid1, 
                                                TRUE);
                        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                      err2 == FM_OK,
                                      "fmSetBitArrayBit returned error!\n");
                    }
                }

                /* Remove remoteIdTree entries tagged for removal
                 * and the pendingVlans associated*/
                if (removeRemIdOnlyFlush)
                {
                    RemoveRemIdTreeKey(tmpEntry, REMID_ONLY_FLUSH, NULL, FALSE);
                }

                RemoveMultipleRemIdTreeKey(tmpEntry, &vid1DelArray, TRUE);


                /* caseId=3 Cancel any pending port/vlan specific purges*/
                purgePtr->stats.numCancelledPortVlan += 
                    tmpEntry->pendingVlans.nonZeroBitCount;
                err2 = fmClearBitArray(&tmpEntry->pendingVlans);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmClearBitArray returned error!\n");
            }

        }   /* end if (tmpEntry->port == -1) */

        /* This entry may now need to be destroyed. */
        tmp2Entry = tmpEntry;
        tmpEntry = tmpEntry->next;
        
        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
        {
            ++purgePtr->stats.numEntriesFreedLater;
        }
        
    }   /* end while (tmpEntry != NULL) */
 

ABORT:
    fmDeleteBitArray(&vid1DelArray);
    
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}    /* end CancelAllPurgeEntries */




/*****************************************************************************/
/** RemovePendingPurgeEntryData
 * \ingroup intAddr
 *
 * \desc            Remove pending data from purge entry
 * 
 * \param[in]       purgeEntry is the entry in which we should remove pending
 *                  pending data
 * 
 * \param[in]       vid1 is the vid1 bit to be removed, set to -1 if
 *                  it does not apply or to remove allVlansPending bit
 * 
 * \param[in]       vid2 is the vid2 bit to be removed, set to -1 if
 *                  it does not apply or to remove allVlansPending bit
 * 
 * \param[in]       remoteId is the remoteId bit to be removed, set to -1 if
 *                  it does not apply or to remove allVlansPending bit
 * 
 * \return          none
 *
 *****************************************************************************/
static void RemovePendingPurgeEntryData(fm_maPurgeListEntry *purgeEntry,
                                        fm_int               vid1,
                                        fm_int               vid2,
                                        fm_int               remoteId)
{
    fm_status    err;
    fm_bool      foundVid2BitArray;
    fm_bool      foundRemIdBitArray;
    fm_bitArray *bitArrayPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "purgeEntry=%p, vid1=%d, vid2=%d\n",
                 (void *) purgeEntry,
                 vid1,
                 vid2);
    
    if (vid1 == -1 && vid2 == -1 && remoteId == -1)
    {
        purgeEntry->allVlansPending = FALSE;
    }
    else if (vid2 == -1 && remoteId == -1)
    {
        err = fmSetBitArrayBit(&purgeEntry->pendingVlans, 
                               vid1, 
                               FALSE);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err == FM_OK,
                      "fmSetBitArrayBit returned an error!\n");
    }
    else if (remoteId == -1)
    {
        if (vid1 == -1)
        {
            vid1 = VID2_ONLY_FLUSH;
        }

        foundVid2BitArray = GetBitArray(&purgeEntry->vid2Tree, 
                                        vid1, 
                                        &bitArrayPtr);

        if (foundVid2BitArray)
        {
            err = fmSetBitArrayBit(bitArrayPtr,
                                   vid2,
                                   FALSE);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err == FM_OK,
                          "fmSetBitArrayBit returned an error!\n");

            if (bitArrayPtr->nonZeroBitCount == 0)
            {
                /* Remove tree item */
                RemoveVid2TreeKey(purgeEntry,
                                  vid1,
                                  bitArrayPtr,
                                  (vid1 == VID2_ONLY_FLUSH) ? FALSE : TRUE);
            }
        }
    }
    else
    {
        if (vid1 == -1)
        {
            vid1 = REMID_ONLY_FLUSH;
        }

        foundRemIdBitArray = GetBitArray(&purgeEntry->remoteIdTree, 
                                         vid1, 
                                         &bitArrayPtr);

        if (foundRemIdBitArray)
        {
            err = fmSetBitArrayBit(bitArrayPtr, 
                                   remoteId, 
                                   FALSE);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err == FM_OK,
                          "fmSetBitArrayBit returned an error!\n");

            if (bitArrayPtr->nonZeroBitCount == 0)
            {
                /* Remove tree item */
                RemoveRemIdTreeKey(purgeEntry, 
                                   vid1, 
                                   bitArrayPtr,
                                   (vid1 == REMID_ONLY_FLUSH) ? FALSE : TRUE);
            }
        }
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_EVENT_MAC_MAINT);

}    /* end RemovePendingPurgeEntryData */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAllocateMacTablePurgeList
 * \ingroup intAddr
 *
 * \desc            Allocate the MA Table purge list.
 *
 * \param[in]       switchPtr points to the fm_switch structure for the
 *                  swith on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if switchPtr is NULL.
 * \return          FM_ERR_NO_MEM if no memory for data structures.
 *
 *****************************************************************************/
fm_status fmAllocateMacTablePurgeList(fm_switch* switchPtr)
{
    fm_status            err;
    fm_maPurge *         purgePtr;
    fm_maPurgeListEntry *purgeEntry;

    if (switchPtr == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT, "switchPtr = NULL\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "switchNumber = %d\n",
                 switchPtr->switchNumber);

    purgePtr               = &switchPtr->maPurge;
    purgePtr->purgeState   = FM_PURGE_STATE_IDLE;
    purgePtr->callbackList = NULL;
    purgePtr->nextSeq      = 1;

    err = AllocatePurgeListEntry(&purgeEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, err);

    purgeEntry->prev       = NULL;
    purgeEntry->next       = NULL;
    purgeEntry->port       = -1;
    purgeEntry->portExists = FALSE;
    purgeEntry->glort      = 0;

    purgePtr->globalListEntry = purgeEntry;
    purgePtr->listHead        = purgeEntry;
    purgePtr->listTail        = purgeEntry;
    purgePtr->scanEntry       = purgeEntry;
    
ABORT:    
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end fmAllocateMacTablePurgeList */




/*****************************************************************************/
/** fmFreeMacTablePurgeList
 * \ingroup intAddr
 *
 * \desc            Deallocate the MAC address table purge list.
 *
 * \param[in]       switchPtr points to the fm_switch structure for the
 *                  swith on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if switchPtr is NULL.
 *
 *****************************************************************************/
fm_status fmFreeMacTablePurgeList(fm_switch* switchPtr)
{
    fm_status            status;
    fm_status            err2;
    fm_maPurge *         purgePtr;
    fm_maPurgeCallBack * callback;
    fm_maPurgeListEntry *tmpEntry;
    fm_maPurgeListEntry *tmp2Entry;
    fm_bitArray *        bitArrayPtr;
    fm_treeIterator      iter;
    fm_uint64            iterVid1;
    fm_bitArray          vid1DelArray;
    fm_bool              removeVid2OnlyFlush;
    fm_bool              removeRemIdOnlyFlush;

    if (switchPtr == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT, "switchPtr = NULL\n");
        return FM_ERR_INVALID_ARGUMENT;
    }

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw = %d\n",
                 switchPtr->switchNumber);

    purgePtr             = &switchPtr->maPurge;
    tmpEntry             = purgePtr->listHead;
    removeVid2OnlyFlush  = FALSE;
    removeRemIdOnlyFlush = FALSE;

    FM_TAKE_MA_PURGE_LOCK(switchPtr->switchNumber);
    
    err2 = fmCreateBitArray(&vid1DelArray, FM_MAX_VLAN);
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           err2 == FM_OK,
                           (void)err2,
                           "Unable to create bit array\n");

    while (tmpEntry != NULL)
    {
        fmDeleteBitArray(&tmpEntry->pendingVlans);
        
        status = fmClearBitArray(&vid1DelArray);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      status == FM_OK,
                      "fmClearBitArray returned an error!\n");

        /* Iterate in the vid2tree to remove entries */
        for ( fmTreeIterInit(&iter, &tmpEntry->vid2Tree) ;
              (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
            )
        {
            if (iterVid1 == VID2_ONLY_FLUSH)
            {
                removeVid2OnlyFlush = TRUE;
            }
            else
            {
                /* Tag entries for removal */
                err2 = fmSetBitArrayBit(&vid1DelArray, 
                                        iterVid1, 
                                        TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err2 == FM_OK,
                      "fmSetBitArrayBit returned error!\n");
            }
        }

        /* Remove vid2Tree entries tagged for removal
         * and the pendingVlans associated */
        if (removeVid2OnlyFlush)
        {
            RemoveVid2TreeKey(tmpEntry, VID2_ONLY_FLUSH, NULL, FALSE);
        }

        RemoveMultipleVid2TreeKey(tmpEntry, &vid1DelArray, TRUE);

        fmTreeDestroy(&tmpEntry->vid2Tree, NULL);

        status = fmClearBitArray(&vid1DelArray);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      status == FM_OK,
                      "fmClearBitArray returned an error!\n");

        /* Iterate in the remoteIdTree to remove entries */
        for ( fmTreeIterInit(&iter, &tmpEntry->remoteIdTree) ;
              (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
            )
        {
            if (iterVid1 == REMID_ONLY_FLUSH)
            {
                removeRemIdOnlyFlush = TRUE;
            }
            else
            {
                /* Tag entries for removal */
                err2 = fmSetBitArrayBit(&vid1DelArray, 
                                        iterVid1, 
                                        TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err2 == FM_OK,
                      "fmSetBitArrayBit returned error!\n");
            }
        }

        /* Remove remoteIdTree entries tagged for removal
         * and the pendingVlans associated */
        if (removeRemIdOnlyFlush)
        {
            RemoveRemIdTreeKey(tmpEntry, REMID_ONLY_FLUSH, NULL, FALSE);
        }

        RemoveMultipleRemIdTreeKey(tmpEntry, &vid1DelArray, TRUE);

        fmTreeDestroy(&tmpEntry->remoteIdTree, NULL);

        /* This entry may now need to be destroyed. */
        tmp2Entry = tmpEntry;
        tmpEntry = tmpEntry->next;
        
        fmFree(tmp2Entry);
    }

    purgePtr->globalListEntry = NULL;

    while (purgePtr->callbackList)
    {
        callback = purgePtr->callbackList;
        purgePtr->callbackList = callback->next;
        fmFree(callback);
    }

    FM_DROP_MA_PURGE_LOCK(switchPtr->switchNumber);

ABORT:
    fmDeleteBitArray(&vid1DelArray);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, FM_OK);

}   /* end fmFreeMacTablePurgeList */




/******************************************************************************
 * 
 *  Additional information for fmEnqueueMAPurge.
 * 
 *  If multiple requests with the same parameters are enqueued, they will be
 *  coalesced into a single request. The following table lists the operations
 *  that will occur based on the request and existing entries:
 * 
 *                                                new entry is    new entry is
 *                                                less specific   more specific
 *                                                than:           than:
 *                                                (must cancel    
 *                                                existing        (must suppress 
 * caseId  flushMode                              entries)        new entry)
 * ------  -------------------------------------  --------------  --------------
 * 0       FM_FLUSH_MODE_ALL_DYNAMIC              -               Impossible
 * 1       FM_FLUSH_MODE_ALL_DYNAMIC              port            -
 * 2       FM_FLUSH_MODE_ALL_DYNAMIC              vlan            -
 * 3       FM_FLUSH_MODE_ALL_DYNAMIC              port/vlan       -
 * 4       FM_FLUSH_MODE_ALL_DYNAMIC              port/vid1/vid2  -
 * 5       FM_FLUSH_MODE_ALL_DYNAMIC              port/vid2       -
 * 6       FM_FLUSH_MODE_ALL_DYNAMIC              vid1/vid2       -
 * 7       FM_FLUSH_MODE_ALL_DYNAMIC              vid2            -
 * 40      FM_FLUSH_MODE_ALL_DYNAMIC              port/vid1/remId -
 * 49      FM_FLUSH_MODE_ALL_DYNAMIC              vid1/remId      -
 * 50      FM_FLUSH_MODE_ALL_DYNAMIC              remId           -
 * ------  -------------------------------------  --------------  --------------
 * 8       FM_FLUSH_MODE_PORT                     -               dynamic
 * 9       FM_FLUSH_MODE_PORT                     port/vlan       -
 * 10      FM_FLUSH_MODE_PORT                     port/vid1/vid2  -
 * 11      FM_FLUSH_MODE_PORT                     port/vid2       -
 * 41      FM_FLUSH_MODE_PORT                     port/vid1/remId -
 * ------  -------------------------------------  --------------  --------------
 * 12      FM_FLUSH_MODE_VLAN                     -               dynamic
 * 13      FM_FLUSH_MODE_VLAN                     port/vlan       -
 * 14      FM_FLUSH_MODE_VLAN                     port/vid1/vid2  -
 * 15      FM_FLUSH_MODE_VLAN                     vid1/vid2       -
 * 42      FM_FLUSH_MODE_VLAN                     port/vid1/remId -
 * 51      FM_FLUSH_MODE_VLAN                     vid1/remId      -
 * ------  -------------------------------------  --------------  --------------
 * 16      FM_FLUSH_MODE_PORT_VLAN                -               dynamic
 * 17      FM_FLUSH_MODE_PORT_VLAN                -               port
 * 18      FM_FLUSH_MODE_PORT_VLAN                -               vlan
 * 19      FM_FLUSH_MODE_PORT_VLAN                port/vid1/vid2  -
 * 43      FM_FLUSH_MODE_PORT_VLAN                port/vid1/remId -
 * ------  -------------------------------------  --------------  --------------
 * 20      FM_FLUSH_MODE_VID1_VID2                -               dynamic
 * 21      FM_FLUSH_MODE_VID1_VID2                -               vlan
 * 22      FM_FLUSH_MODE_VID1_VID2                -               vid2
 * 23      FM_FLUSH_MODE_VID1_VID2                port/vid1/vid2  -
 * ------  -------------------------------------  --------------  --------------
 * 24      FM_FLUSH_MODE_PORT_VID1_VID2           -               dynamic
 * 25      FM_FLUSH_MODE_PORT_VID1_VID2           -               port
 * 26      FM_FLUSH_MODE_PORT_VID1_VID2           -               vlan
 * 27      FM_FLUSH_MODE_PORT_VID1_VID2           -               vid2
 * 28      FM_FLUSH_MODE_PORT_VID1_VID2           -               port/vlan
 * 29      FM_FLUSH_MODE_PORT_VID1_VID2           -               port/vid2
 * 30      FM_FLUSH_MODE_PORT_VID1_VID2           -               vid1/vid2
 * 31      FM_FLUSH_MODE_PORT_VID1_VID2           Impossible      -
 * ------  -------------------------------------  --------------  --------------
 * 32      FM_FLUSH_MODE_PORT_VID2                -               dynamic
 * 33      FM_FLUSH_MODE_PORT_VID2                -               port
 * 34      FM_FLUSH_MODE_PORT_VID2                -               vid2
 * 35      FM_FLUSH_MODE_PORT_VID2                port/vid1/vid2  -
 * ------  -------------------------------------  --------------  --------------
 * 36      FM_FLUSH_MODE_VID2                     -               dynamic
 * 37      FM_FLUSH_MODE_VID2                     port/vid2       -
 * 38      FM_FLUSH_MODE_VID2                     port/vid1/vid2  -
 * 39      FM_FLUSH_MODE_VID2                     vid1/vid2       -
 * ------  -------------------------------------  --------------  --------------
 * 44      FM_FLUSH_MODE_PORT_VID1_REMOTEID        -               dynamic
 * 45      FM_FLUSH_MODE_PORT_VID1_REMOTEID        -               port
 * 46      FM_FLUSH_MODE_PORT_VID1_REMOTEID        -               vlan
 * 47      FM_FLUSH_MODE_PORT_VID1_REMOTEID        -               port/vlan
 * 59      FM_FLUSH_MODE_PORT_VID1_REMOTEID        -               vid1/remId
 * 60      FM_FLUSH_MODE_PORT_VID1_REMOTEID        -               remId
 * 48      FM_FLUSH_MODE_PORT_VID1_REMOTEID        Impossible      -
 * ------  -------------------------------------  --------------  --------------
 * 52      FM_FLUSH_MODE_VID1_REMOTEID            -               dynamic
 * 53      FM_FLUSH_MODE_VID1_REMOTEID            -               vlan
 * 54      FM_FLUSH_MODE_VID1_REMOTEID            -               remoteId
 * 55      FM_FLUSH_MODE_VID1_REMOTEID            port/vid1/remId -
 * ------  -------------------------------------  --------------  --------------
 * 56      FM_FLUSH_MODE_REMOTEID                 -               dynamic
 * 57      FM_FLUSH_MODE_REMOTEID                 port/vid1/remId -
 * 58      FM_FLUSH_MODE_REMOTEID                 vid1/remId -
 *
 *****************************************************************************/




/*****************************************************************************/
/** fmEnqueueMAPurge
 * \ingroup intAddr
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Enqueues a hardware-based purge request and, if no purge
 *                  request is being executed, executes the purge request
 *                  immediately.
 * 
 *                  If multiple requests with the same parameters are enqueued,
 *                  they will be coalesced into a single request.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       workType is the type of update to perform.
 *
 * \param[in]       data contains a parameter whose meaning depends upon
 *                  the value of type.
 *
 * \param[in]       handler points to an address maintenance callback function
 *                  that is to be called on completion of the hardware-based 
 *                  purge request.
 *
 * \param[in]       context points to the context in which the address
 *                  maintenance callback function is to be executed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if port is not valid.
 * \return          FM_ERR_NO_EVENTS_AVAILABLE if the purge request could not
 *                  be added to the purge request queue.
 * \return          FM_ERR_INVALID_ARGUMENT if both fid and vlan arguments
 *                  are specified.
 * \return          FM_ERR_NO_MEM if memory allocation error.
 * \return          FM_FAIL if unable to get purge list entry.
 *
 *****************************************************************************/
fm_status fmEnqueueMAPurge(fm_int              sw,
                           fm_maWorkType       workType,
                           fm_maWorkTypeData   data,
                           fm_addrMaintHandler handler,
                           void *              context)
{
    fm_switch *          switchPtr;
    fm_maPurge *         purgePtr;
    fm_status            status;
    fm_status            err2;
    fm_bool              allPorts;
    fm_bool              allVlans;
    fm_bool              vid1Defined;
    fm_bool              vid2Defined;
    fm_bool              remoteIdDefined;
    fm_maPurgeListEntry *purgeEntry;
    fm_maPurgeListEntry *tmpEntry;
    fm_maPurgeListEntry *tmp2Entry;
    fm_maPurgeCallBack * callbackEntry;
    fm_maPurgeCallBack * callbackListTail;
    fm_bitArray *        bitArrayPtr;
    fm_bitArray *        bitArrayPtr2;
    fm_bitArray *        bitArrayPtr3;
    fm_bitArray *        bitArrayPtr4;
    fm_bitArray *        bitArrayPtr1RemId;
    fm_bitArray *        bitArrayPtr2RemId;
    fm_bitArray *        bitArrayPtr3RemId;
    fm_bitArray *        bitArrayPtr4RemId;
    fm_bool              foundVid2BitArray;
    fm_bool              foundVid2BitArray2;
    fm_bool              foundVid2BitArray3;
    fm_bool              foundVid2BitArray4;
    fm_bool              foundRemIdBitArray;
    fm_bool              foundRemIdBitArray2;
    fm_bool              foundRemIdBitArray3;
    fm_bool              foundRemIdBitArray4;
    fm_treeIterator      iter;
    fm_uint64            iterVid1;
    fm_bitArray          vid1DelArray;
    fm_bool              vid1DelArrayCreated;
    fm_bool              removeVid2OnlyFlush;
    fm_bool              removeRemIdOnlyFlush;

    FM_NOT_USED(workType);
    
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw=%d, workType=%s, flushMode=%s, port=%d, vid1=%d, vid2=%d, context=%p\n",
                 sw,
                 fmMATableWorkTypeToText(workType),
                 fmFlushModeToTextV2(workType, data.flushMode),
                 data.port,
                 data.vid1,
                 data.vid2,
                 context);

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;
    
    allPorts        = FALSE;
    allVlans        = FALSE;
    vid1Defined     = FALSE;
    vid2Defined     = FALSE;
    remoteIdDefined = FALSE;

    vid1DelArrayCreated = FALSE;

    if (workType == FM_UPD_FLUSH_ADDRESSES)
    {
        switch (data.flushMode)
        {
            case FM_FLUSH_MODE_PORT:
                ++purgePtr->stats.numRequestedPort;
                allVlans = TRUE;
                break;

            case FM_FLUSH_MODE_ALL_DYNAMIC:
                ++purgePtr->stats.numRequestedGlobal;
                allPorts = TRUE;
                allVlans = TRUE;
                break;

            case FM_FLUSH_MODE_VLAN:
                ++purgePtr->stats.numRequestedVlan;
                allPorts = TRUE;
                vid1Defined = TRUE;
                break;

            case FM_FLUSH_MODE_PORT_VLAN:
                ++purgePtr->stats.numRequestedPortVlan;
                vid1Defined = TRUE;
                break;

            case FM_FLUSH_MODE_VID1_VID2:
                ++purgePtr->stats.numRequestedVid1Vid2;
                allPorts = TRUE;
                vid1Defined = TRUE;
                vid2Defined = TRUE;
                break;

            case FM_FLUSH_MODE_PORT_VID1_VID2:
                ++purgePtr->stats.numRequestedPortVid1Vid2;
                vid1Defined = TRUE;
                vid2Defined = TRUE;
                break;

            case FM_FLUSH_MODE_PORT_VID2:
                ++purgePtr->stats.numRequestedPortVid2;
                vid2Defined = TRUE;
                break;

            case FM_FLUSH_MODE_VID2:
                ++purgePtr->stats.numRequestedVid2;
                allPorts = TRUE;
                vid2Defined = TRUE;
                break;

            case FM_FLUSH_MODE_PORT_VID1_REMOTEID:
                ++purgePtr->stats.numRequestedPortVid1RemoteId;
                vid1Defined = TRUE;
                remoteIdDefined = TRUE;
                break;

            case FM_FLUSH_MODE_VID1_REMOTEID:
                ++purgePtr->stats.numRequestedVid1RemoteId;
                allPorts = TRUE;
                vid1Defined = TRUE;
                remoteIdDefined = TRUE;
                break;

            case FM_FLUSH_MODE_REMOTEID:
                ++purgePtr->stats.numRequestedRemoteId;
                allPorts = TRUE;
                remoteIdDefined = TRUE;
                break;

            default:
                break;

        }   /* end switch (data.flushMode) */
    }
    else if (workType == FM_UPD_FLUSH_EXPIRED)
    {
        ++purgePtr->stats.numRequestedExpired;
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                     "Invalid workType %s(%d)\n",
                     fmMATableWorkTypeToText(workType),
                     workType);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, FM_ERR_INVALID_ARGUMENT);
    }
    
    FM_TAKE_MA_PURGE_LOCK(sw);

    FM_VERIFY_MA_PURGE(sw);

    /* There is one global purge entry, and there can be one purge entry
     * per port. Retrieve that entry, if it has not been created
     * for the flush port, it will be created. */
    status = GetPurgeEntry(sw, allPorts, data.port, &purgeEntry);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);

    /* Set the bit for the VLAN in the maPurgeListEntry object, checking for
     * duplicate (or less-specific) requests, and cancelling more-specific
     * requests. */
    if (workType == FM_UPD_FLUSH_EXPIRED)
    {
        purgePtr->flushExpired = TRUE;
    }
    else if (allVlans)
    {
        if (purgeEntry->allVlansPending)
        {
            if (allPorts)
            {
                ++purgePtr->stats.numDuplicateGlobal;
            }
            else
            {
                ++purgePtr->stats.numDuplicatePort;
            }
        }
        else if ((!allPorts) && (purgePtr->globalListEntry->allVlansPending)) 
        {
            /* caseId=8 */
            ++purgePtr->stats.numSuppressedPort;
        }
        else
        {
            purgeEntry->allVlansPending = TRUE;

            if (allPorts)
            {
                /**************************************************
                 * This is the most general purge request (all
                 * ports on all VLANs) so we can cancel all other
                 * purge requests as they are eclipsed by this one.
                 **************************************************/
                
                ++purgePtr->stats.numEnqueuedGlobal;

                /* casedId=1,2,3,4,5,6,7,40,49,50 */
                status = CancelAllPurgeEntries(sw);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);
            }
            else
            {
                ++purgePtr->stats.numEnqueuedPort;

                err2 = fmCreateBitArray(&vid1DelArray, FM_MAX_VLAN);
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                       err2 == FM_OK,
                                       (void)err2,
                                       "Unable to create bit array\n");
                vid1DelArrayCreated = TRUE;
                removeVid2OnlyFlush = FALSE;
                removeRemIdOnlyFlush = FALSE;

                /* Iterate in the vid2tree to remove entries */
                for ( fmTreeIterInit(&iter, &purgeEntry->vid2Tree) ;
                      (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                    )
                {
                    if (iterVid1 == VID2_ONLY_FLUSH)
                    {
                        /*  caseId=11 This will cancel port/vid2 specific
                         *  purges */
                        purgePtr->stats.numCancelledPortVid2 += 
                            bitArrayPtr->nonZeroBitCount;

                        removeVid2OnlyFlush = TRUE;
                    }
                    else
                    {
                        /* caseId=10 Cancel any pending port/vid1/vid2 specific
                         * purges for this port. */
                        purgePtr->stats.numCancelledPortVid1Vid2 += 
                            bitArrayPtr->nonZeroBitCount;

                        /* Tag tree item for removal (we cannot alter the tree
                         * while iterating) */
                        err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                iterVid1, 
                                                TRUE);
                        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                      err2 == FM_OK,
                                      "fmSetBitArrayBit returned error!\n");
                    }
                }

                /* Remove vid2Tree entries tagged for removal
                 * and the pendingVlans associated*/
                if (removeVid2OnlyFlush)
                {
                    RemoveVid2TreeKey(purgeEntry, VID2_ONLY_FLUSH, NULL, FALSE);
                }

                RemoveMultipleVid2TreeKey(purgeEntry, &vid1DelArray, TRUE);
                

                err2 = fmClearBitArray(&vid1DelArray);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmClearBitArray returned an error!\n");

                /* Iterate in the remoteIdTree to remove entries */
                for ( fmTreeIterInit(&iter, &purgeEntry->remoteIdTree) ;
                      (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                    )
                {
                    if (iterVid1 == REMID_ONLY_FLUSH)
                    {
                        removeRemIdOnlyFlush = TRUE;
                    }
                    else
                    {
                        /* caseId=41 Cancel any pending port/vid1/remoteId specific
                         * purges for this port. */
                        purgePtr->stats.numCancelledPortVid1RemoteId += 
                            bitArrayPtr->nonZeroBitCount;

                        /* Tag tree item for removal (we cannot alter the tree
                         * while iterating) */
                        err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                iterVid1, 
                                                TRUE);
                        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                      err2 == FM_OK,
                                      "fmSetBitArrayBit returned error!\n");
                    }
                }

                /* Remove remoteIdTree entries tagged for removal
                 * and the pendingVlans associated*/
                if (removeRemIdOnlyFlush)
                {
                    RemoveRemIdTreeKey(purgeEntry, REMID_ONLY_FLUSH, NULL, FALSE);
                }
    
                RemoveMultipleRemIdTreeKey(purgeEntry, &vid1DelArray, TRUE);
                
                /* caseId=9 Cancel any pending port/VLAN specific purges
                 * for this port. */
                purgePtr->stats.numCancelledPortVlan += 
                    purgeEntry->pendingVlans.nonZeroBitCount;
                err2 = fmClearBitArray(&purgeEntry->pendingVlans);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmClearBitArray returned error!\n");

            }   /* end if (allPorts) */
            
        }   /* end if (purgeEntry->allVlansPending) */
    }
    else
    {
        if (vid1Defined && !vid2Defined && !remoteIdDefined)
        {
            if (fmIsBitInBitArray(&purgeEntry->pendingVlans, data.vid1))
            {
                foundVid2BitArray = GetBitArray(&purgeEntry->vid2Tree, 
                                                data.vid1, 
                                                &bitArrayPtr);

                foundRemIdBitArray = GetBitArray(&purgeEntry->remoteIdTree,
                                                 data.vid1,
                                                 &bitArrayPtr1RemId);
                if (foundVid2BitArray)
                {
                    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                  bitArrayPtr->nonZeroBitCount > 0,
                                  "Expected bitArray to contain at least one "
                                  "non zero bit!");

                    /* This entry is less specific, cancel any pending vid1/vid2
                    *  and port/vid1/vid2 combination related to vid1*/
                    if (allPorts)
                    {
                        ++purgePtr->stats.numEnqueuedVlan;

                        /* caseId=15 */
                        purgePtr->stats.numCancelledVid1Vid2 += 
                            bitArrayPtr->nonZeroBitCount;
                    }
                    else
                    {
                        ++purgePtr->stats.numEnqueuedPortVlan;

                        /* caseId=14 and caseId=19*/
                        purgePtr->stats.numCancelledPortVid1Vid2 += 
                            bitArrayPtr->nonZeroBitCount;
                    }

                    RemoveVid2TreeKey(purgeEntry, data.vid1, bitArrayPtr, FALSE);
                }
                else if (foundRemIdBitArray)
                {
                    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                  bitArrayPtr1RemId->nonZeroBitCount > 0,
                                  "Expected bitArrayPtr1RemId to contain at least one "
                                  "non zero bit!");

                    /* This entry is less specific, cancel any pending vid1/remId
                    *  and port/vid1/remId combination related to vid1*/
                    if (allPorts)
                    {
                        ++purgePtr->stats.numEnqueuedVlan;

                        /* caseId=51 */
                        purgePtr->stats.numCancelledVid1RemoteId += 
                            bitArrayPtr1RemId->nonZeroBitCount;
                    }
                    else
                    {
                        ++purgePtr->stats.numEnqueuedPortVlan;

                        /* caseId=42 and caseId=43*/
                        purgePtr->stats.numCancelledPortVid1RemoteId += 
                            bitArrayPtr1RemId->nonZeroBitCount;
                    }

                    RemoveRemIdTreeKey(purgeEntry, data.vid1, bitArrayPtr1RemId, FALSE);
                }
                else
                {
                    if (allPorts)
                    {
                        ++purgePtr->stats.numDuplicateVlan;
                    }
                    else
                    {
                        ++purgePtr->stats.numDuplicatePortVlan;
                    }
                }
            }
            else if (purgeEntry->allVlansPending)
            {
                if (allPorts)
                {
                    /* caseId=12 A global request already exists */
                    ++purgePtr->stats.numSuppressedVlan;
                }
                else
                {
                    /* caseId=17 */
                    ++purgePtr->stats.numSuppressedPortVlan;
                }
            }
            else if ( (!allPorts) &&
                      ( (purgePtr->globalListEntry->allVlansPending) ||
                        (fmIsBitInBitArray(&purgePtr->globalListEntry->pendingVlans, 
                                           data.vid1) && 
                         !GetBitArray(&purgePtr->globalListEntry->vid2Tree, 
                                      data.vid1, 
                                      &bitArrayPtr) &&
                         !GetBitArray(&purgePtr->globalListEntry->remoteIdTree,
                                      data.vid1,
                                      &bitArrayPtr1RemId) ) ) )
            {
                /* caseId=16 and caseId=18 */
                ++purgePtr->stats.numSuppressedPortVlan;
            }
            else
            {
                status = fmSetBitArrayBit(&purgeEntry->pendingVlans, data.vid1, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              status == FM_OK,
                              "fmSetBitArrayBit returned error!\n");
    
                if (allPorts)
                {
                    ++purgePtr->stats.numEnqueuedVlan;
    
                    /* Cancel any other pending port/vlan,
                     * port/vid1/vid2-specific or port/vid1/remoteId purges
                     * for this VLAN. */
                    tmpEntry = purgePtr->listHead;
                    
                    while (tmpEntry != NULL)
                    {
                        if ( (tmpEntry->port != -1) && 
                             fmIsBitInBitArray(&tmpEntry->pendingVlans, data.vid1) )
                        {
                            foundVid2BitArray = GetBitArray(&tmpEntry->vid2Tree, 
                                                            data.vid1, 
                                                            &bitArrayPtr);

                            foundRemIdBitArray = GetBitArray(
                                                      &tmpEntry->remoteIdTree,
                                                      data.vid1,
                                                      &bitArrayPtr1RemId);

                            if (foundVid2BitArray)
                            {
                                /* caseId=14 */
                                purgePtr->stats.numCancelledPortVid1Vid2 += 
                                    bitArrayPtr->nonZeroBitCount;
                                
                                RemoveVid2TreeKey(tmpEntry, 
                                                  data.vid1, 
                                                  bitArrayPtr,
                                                  FALSE);
                            }
                            else if (foundRemIdBitArray)
                            {
                                /* caseId=42 */
                                purgePtr->stats.numCancelledPortVid1RemoteId += 
                                    bitArrayPtr1RemId->nonZeroBitCount;

                                RemoveRemIdTreeKey(tmpEntry, 
                                                   data.vid1, 
                                                   bitArrayPtr1RemId,
                                                   FALSE);
                            }
                            else
                            {
                                /* caseId=13 */
                                ++purgePtr->stats.numCancelledPortVlan;
                            }

                            err2 = fmSetBitArrayBit(&tmpEntry->pendingVlans, 
                                                    data.vid1, 
                                                    FALSE);
                            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                          err2 == FM_OK,
                                          "fmSetBitArrayBit returned error!\n");

                        }
    
                        /* This entry may now need to be destroyed. */
                        tmp2Entry = tmpEntry;
                        tmpEntry = tmpEntry->next;
                        
                        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
                        {
                            ++purgePtr->stats.numEntriesFreedLater;
                        }
                        
                    }   /* end while (tmpEntry != NULL) */
                }
                else
                {
                    ++purgePtr->stats.numEnqueuedPortVlan;
                    
                }   /* end if (allPorts) */
                
            }   /* end if (fmIsBitInBitArray(&purgeEntry->pendingVlans, data.vid1)) */
            
        }
        else if (!vid1Defined && vid2Defined)
        {
            /* Look if there is an existing vid2 requests only */
            foundVid2BitArray = GetBitArray(&purgeEntry->vid2Tree, 
                                            VID2_ONLY_FLUSH, 
                                            &bitArrayPtr);

            foundVid2BitArray2 = GetBitArray(&purgePtr->globalListEntry->vid2Tree, 
                                             VID2_ONLY_FLUSH, 
                                             &bitArrayPtr2);

            if (purgeEntry->allVlansPending)
            {
                if (allPorts)
                {
                    /* caseId=36 */
                    ++purgePtr->stats.numSuppressedVid2;
                }
                else
                {
                    /* caseId=33 */
                    ++purgePtr->stats.numSuppressedPortVid2;
                }
            }
            else if (foundVid2BitArray &&
                     fmIsBitInBitArray(bitArrayPtr, data.vid2))
            {
                if (allPorts)
                {
                    ++purgePtr->stats.numDuplicateVid2;
                }
                else
                {
                    ++purgePtr->stats.numDuplicatePortVid2;
                }
            }
            else if ( (!allPorts) &&
                      ( (purgePtr->globalListEntry->allVlansPending) ||
                        (foundVid2BitArray2 &&
                         fmIsBitInBitArray(bitArrayPtr2, data.vid2) ) ) )
            {
                /* caseId=32 or caseId=34 */
                ++purgePtr->stats.numSuppressedPortVid2;
            }
            else
            {
                if (!foundVid2BitArray)
                {
                    status = InsertTreeKey(&purgeEntry->vid2Tree,
                                           VID2_ONLY_FLUSH,
                                           VID2_ONLY_FLUSH,
                                           &bitArrayPtr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);
                }
                
                err2 = fmSetBitArrayBit(bitArrayPtr, data.vid2, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmSetBitArrayBit returned error!\n");

                err2 = fmCreateBitArray(&vid1DelArray, FM_MAX_VLAN);
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                       err2 == FM_OK,
                                       (void)err2,
                                       "Unable to create bit array\n");
                vid1DelArrayCreated = TRUE;

                if (allPorts)
                {
                    ++purgePtr->stats.numEnqueuedVid2;

                    /* Cancel any other pending port/vid2 specific purges
                    *  for this vid2.
                    * 
                    *  This code will also cancel port/vid1/vid2
                    *  specific purges. The code needs to iterate in the
                    *  vid2Tree and look for vid2 in the vid2Tree. */ 
                    tmpEntry = purgePtr->listHead;
                    
                    while (tmpEntry != NULL)
                    {
                        foundVid2BitArray = GetBitArray(&tmpEntry->vid2Tree, 
                                                        VID2_ONLY_FLUSH, 
                                                        &bitArrayPtr);

                        if ( (tmpEntry->port != -1) && 
                             (foundVid2BitArray) )
                        {
                            if ( fmIsBitInBitArray(bitArrayPtr, data.vid2) )
                            {
                                /* caseId=37 */
                                ++purgePtr->stats.numCancelledPortVid2;

                                err2 = fmSetBitArrayBit(bitArrayPtr, 
                                                        data.vid2, 
                                                        FALSE);
                                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                              err2 == FM_OK,
                                              "fmSetBitArrayBit returned error!\n");

                                if (bitArrayPtr->nonZeroBitCount == 0)
                                {
                                    /* Remove tree item */
                                    RemoveVid2TreeKey(tmpEntry, 
                                                      VID2_ONLY_FLUSH, 
                                                      bitArrayPtr,
                                                      FALSE);
                                }
                            }
                        }
                        else
                        {
                            err2 = fmClearBitArray(&vid1DelArray);
                            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                          err2 == FM_OK,
                                          "fmClearBitArray returned error!\n");

                            /* Iterate in the vid2tree to find vid2 for any
                             * vid1 */
                            for ( fmTreeIterInit(&iter, &tmpEntry->vid2Tree) ;
                                  (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                                )
                            {
                                if (iterVid1 == VID2_ONLY_FLUSH)
                                {
                                    continue;
                                }

                                if ( fmIsBitInBitArray(bitArrayPtr, data.vid2) )
                                {
                                    if (tmpEntry->port != -1)
                                    {
                                        /* caseId=38 */
                                        ++purgePtr->stats.numCancelledPortVid1Vid2;
                                    }
                                    else
                                    {
                                        /* caseId=39 */
                                        ++purgePtr->stats.numCancelledVid1Vid2;
                                    }
    
                                    err2 = fmSetBitArrayBit(bitArrayPtr, 
                                                            data.vid2, 
                                                            FALSE);
                                    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                                  err2 == FM_OK,
                                                  "fmSetBitArrayBit returned error!\n");
    
                                    if (bitArrayPtr->nonZeroBitCount == 0)
                                    {
                                        /* Tag tree item for removal (we cannot
                                         * alter the tree while iterating) */
                                        err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                                iterVid1, 
                                                                TRUE);
                                        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                                      err2 == FM_OK,
                                                      "fmSetBitArrayBit returned error!\n");
                                    }
                                }
                            }

                            /* Remove vid2Tree entries tagged for removal
                             * and the pendingVlans associated*/
                            RemoveMultipleVid2TreeKey(tmpEntry, 
                                                      &vid1DelArray, 
                                                      TRUE);
                        }
    
                        /* This entry may now need to be destroyed. */
                        tmp2Entry = tmpEntry;
                        tmpEntry = tmpEntry->next;
                        
                        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
                        {
                            ++purgePtr->stats.numEntriesFreedLater;
                        }
                        
                    }   /* end while (tmpEntry != NULL) */
                }
                else
                {
                    ++purgePtr->stats.numEnqueuedPortVid2;

                    /* Iterate in the vid2tree to find vid2 for any
                     * vid1 */
                    for ( fmTreeIterInit(&iter, &purgeEntry->vid2Tree) ;
                          (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
                        )
                    {
                        if (iterVid1 == VID2_ONLY_FLUSH)
                        {
                            continue;
                        }

                        if ( fmIsBitInBitArray(bitArrayPtr, data.vid2) )
                        {
                            /* caseId=35 */
                            ++purgePtr->stats.numCancelledPortVid1Vid2;
                            
                            err2 = fmSetBitArrayBit(bitArrayPtr, 
                                                    data.vid2, 
                                                    FALSE);
                            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                          err2 == FM_OK,
                                          "fmSetBitArrayBit returned error!\n");

                            if (bitArrayPtr->nonZeroBitCount == 0)
                            {
                                /* Tag tree item for removal (we cannot
                                 * alter the tree while iterating) */
                                err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                        iterVid1, 
                                                        TRUE);
                                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                              err2 == FM_OK,
                                              "fmSetBitArrayBit returned error!\n");
                            }
                        }
                    }

                    /* Remove vid2Tree entries tagged for removal
                     * and the pendingVlans associated*/
                    RemoveMultipleVid2TreeKey(purgeEntry, &vid1DelArray, TRUE);

                }   /* end if (allPorts) */

            }   /* end if (purgeEntry->allVlansPending) */

        }
        else if (!vid1Defined && remoteIdDefined)
        {
            /* Look if there is an existing vid2 requests only */
            foundRemIdBitArray = GetBitArray(&purgeEntry->remoteIdTree, 
                                             REMID_ONLY_FLUSH, 
                                             &bitArrayPtr1RemId);

            foundRemIdBitArray2 = GetBitArray(&purgePtr->globalListEntry->remoteIdTree, 
                                              REMID_ONLY_FLUSH, 
                                              &bitArrayPtr2RemId);

            if (purgeEntry->allVlansPending)
            {
                if (allPorts)
                {
                    /* caseId=56 */
                    ++purgePtr->stats.numSuppressedRemoteId;
                }
            }
            else if (foundRemIdBitArray &&
                     fmIsBitInBitArray(bitArrayPtr1RemId, data.remoteId))
            {
                if (allPorts)
                {
                    ++purgePtr->stats.numDuplicateVid2;
                }
            }
            else
            {
                if (!foundRemIdBitArray)
                {
                    status = InsertTreeKey(&purgeEntry->remoteIdTree,
                                           REMID_ONLY_FLUSH,
                                           REMID_ONLY_FLUSH,
                                           &bitArrayPtr1RemId);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);
                }

                err2 = fmSetBitArrayBit(bitArrayPtr1RemId, data.remoteId, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmSetBitArrayBit returned error!\n");

                err2 = fmCreateBitArray(&vid1DelArray, FM_MAX_VLAN);
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                       err2 == FM_OK,
                                       (void)err2,
                                       "Unable to create bit array\n");
                vid1DelArrayCreated = TRUE;

                if (allPorts)
                {
                    ++purgePtr->stats.numEnqueuedRemoteId;

                    /* Cancel any other pending port/remId specific purges
                    *  for this remId.
                    * 
                    *  This code will also cancel port/vid1/remId
                    *  specific purges. The code needs to iterate in the
                    *  remoteIdTree and look for remId in the remoteIdTree. */ 
                    tmpEntry = purgePtr->listHead;

                    while (tmpEntry != NULL)
                    {
                        foundRemIdBitArray = GetBitArray(&tmpEntry->remoteIdTree, 
                                                         REMID_ONLY_FLUSH, 
                                                         &bitArrayPtr1RemId);

                        if ( (tmpEntry->port != -1) && 
                             (foundRemIdBitArray) )
                        {
                            if ( fmIsBitInBitArray(bitArrayPtr1RemId, data.remoteId) )
                            {
                                err2 = fmSetBitArrayBit(bitArrayPtr1RemId, 
                                                        data.remoteId, 
                                                        FALSE);
                                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                              err2 == FM_OK,
                                              "fmSetBitArrayBit returned error!\n");

                                if (bitArrayPtr1RemId->nonZeroBitCount == 0)
                                {
                                    /* Remove tree item */
                                    RemoveRemIdTreeKey(tmpEntry, 
                                                       REMID_ONLY_FLUSH, 
                                                       bitArrayPtr1RemId,
                                                       FALSE);
                                }
                            }
                        }
                        else
                        {
                            err2 = fmClearBitArray(&vid1DelArray);
                            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                          err2 == FM_OK,
                                          "fmClearBitArray returned error!\n");

                            /* Iterate in the remoteIdTree to find remId for any
                             * vid1 */
                            for ( fmTreeIterInit(&iter, &tmpEntry->remoteIdTree) ;
                                  (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr1RemId)) == FM_OK ;
                                )
                            {
                                if (iterVid1 == REMID_ONLY_FLUSH)
                                {
                                    continue;
                                }

                                if ( fmIsBitInBitArray(bitArrayPtr1RemId, data.remoteId) )
                                {
                                    if (tmpEntry->port != -1)
                                    {
                                        /* caseId=57 */
                                        ++purgePtr->stats.numCancelledPortVid1RemoteId;
                                    }
                                    else
                                    {
                                        /* caseId=58 */
                                        ++purgePtr->stats.numCancelledVid1RemoteId;
                                    }

                                    err2 = fmSetBitArrayBit(bitArrayPtr1RemId, 
                                                            data.remoteId, 
                                                            FALSE);
                                    FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                                  err2 == FM_OK,
                                                  "fmSetBitArrayBit returned error!\n");

                                    if (bitArrayPtr1RemId->nonZeroBitCount == 0)
                                    {
                                        /* Tag tree item for removal (we cannot
                                         * alter the tree while iterating) */
                                        err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                                iterVid1, 
                                                                TRUE);
                                        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                                      err2 == FM_OK,
                                                      "fmSetBitArrayBit returned error!\n");
                                    }
                                }
                            }

                            /* Remove remoteIdTree entries tagged for removal
                             * and the pendingVlans associated*/
                            RemoveMultipleRemIdTreeKey(tmpEntry, 
                                                       &vid1DelArray, 
                                                       TRUE);
                        }

                        /* This entry may now need to be destroyed. */
                        tmp2Entry = tmpEntry;
                        tmpEntry = tmpEntry->next;

                        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
                        {
                            ++purgePtr->stats.numEntriesFreedLater;
                        }

                    }   /* end while (tmpEntry != NULL) */
                }
                else
                {
                    /* Iterate in the remoteIdTree to find remId for any
                     * vid1 */
                    for ( fmTreeIterInit(&iter, &purgeEntry->remoteIdTree) ;
                          (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr1RemId)) == FM_OK ;
                        )
                    {
                        if (iterVid1 == REMID_ONLY_FLUSH)
                        {
                            continue;
                        }

                        if ( fmIsBitInBitArray(bitArrayPtr1RemId, data.remoteId) )
                        {
                            err2 = fmSetBitArrayBit(bitArrayPtr1RemId, 
                                                    data.remoteId, 
                                                    FALSE);
                            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                          err2 == FM_OK,
                                          "fmSetBitArrayBit returned error!\n");

                            if (bitArrayPtr1RemId->nonZeroBitCount == 0)
                            {
                                /* Tag tree item for removal (we cannot
                                 * alter the tree while iterating) */
                                err2 = fmSetBitArrayBit(&vid1DelArray, 
                                                        iterVid1, 
                                                        TRUE);
                                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                              err2 == FM_OK,
                                              "fmSetBitArrayBit returned error!\n");
                            }
                        }
                    }

                    /* Remove remoteIdTree entries tagged for removal
                     * and the pendingVlans associated*/
                    RemoveMultipleRemIdTreeKey(purgeEntry, &vid1DelArray, TRUE);

                }   /* end if (allPorts) */

            }   /* end if (purgeEntry->allVlansPending) */

        }
        else if (vid1Defined && vid2Defined)
        {
            foundVid2BitArray = GetBitArray(&purgeEntry->vid2Tree, 
                                            data.vid1, 
                                            &bitArrayPtr);

            foundVid2BitArray2 = GetBitArray(&purgePtr->globalListEntry->vid2Tree, 
                                             data.vid1, 
                                             &bitArrayPtr2);

            foundVid2BitArray3 = GetBitArray(&purgeEntry->vid2Tree, 
                                             VID2_ONLY_FLUSH, 
                                             &bitArrayPtr3);

            foundVid2BitArray4 = GetBitArray(&purgePtr->globalListEntry->vid2Tree, 
                                             VID2_ONLY_FLUSH, 
                                             &bitArrayPtr4);

            foundRemIdBitArray = GetBitArray(&purgeEntry->remoteIdTree,
                                             data.vid1,
                                             &bitArrayPtr1RemId);

            foundRemIdBitArray2 = GetBitArray(&purgePtr->globalListEntry->remoteIdTree,
                                              data.vid1,
                                              &bitArrayPtr2RemId);
            if (purgeEntry->allVlansPending)
            {
                if (allPorts)
                {
                    /* caseId=20 */
                    ++purgePtr->stats.numSuppressedVid1Vid2;
                }
                else
                {
                    /* caseId=25 */
                    ++purgePtr->stats.numSuppressedPortVid1Vid2;
                }
            }
            else if (foundVid2BitArray &&
                     fmIsBitInBitArray(bitArrayPtr, data.vid2))
            {
                if (allPorts)
                {
                    ++purgePtr->stats.numDuplicateVid1Vid2;
                }
                else
                {
                    ++purgePtr->stats.numDuplicatePortVid1Vid2;
                }
            }
            else if (fmIsBitInBitArray(&purgeEntry->pendingVlans, data.vid1) && 
                    (!foundVid2BitArray) && (!foundRemIdBitArray))
            {
                /* Purge entry existing for vid1 only */
                if (allPorts)
                {
                    /* caseId=21 */
                    ++purgePtr->stats.numSuppressedVid1Vid2;
                }
                else
                {
                    /* caseId=26 */
                    ++purgePtr->stats.numSuppressedPortVid1Vid2;
                }
            }
            else if (foundVid2BitArray3 &&
                     fmIsBitInBitArray(bitArrayPtr3, data.vid2) )
            {
                /* Purge entry existing for vid2 only */
                if (allPorts)
                {
                    /* caseId=22 */
                    ++purgePtr->stats.numSuppressedVid1Vid2;
                }
                else
                {
                    /* caseId=27 */
                    ++purgePtr->stats.numSuppressedPortVid1Vid2;
                }
            }
            else if ( (!allPorts) &&
                      ( (purgePtr->globalListEntry->allVlansPending) ||
                        (foundVid2BitArray2 &&
                         fmIsBitInBitArray(bitArrayPtr2, data.vid2) ) ) )
            {
                /* caseId=24 or caseId=30 port/vid/vid2 flush requested but
                *  vid1/vid2 request already exists or there is a global
                *  request to flush all */
                ++purgePtr->stats.numSuppressedPortVid1Vid2;
            }
            else if ( (!allPorts ) &&
                      fmIsBitInBitArray(&purgePtr->globalListEntry->pendingVlans, 
                                        data.vid1) &&
                      (!foundVid2BitArray2) && (!foundRemIdBitArray2) )
            {
                /* caseid=28 Global purge entry existing for vid1 only */
                ++purgePtr->stats.numSuppressedPortVid1Vid2;
            }
            else if ( (!allPorts ) &&
                      foundVid2BitArray4 &&
                      fmIsBitInBitArray(bitArrayPtr4, data.vid2) )
            {
                /* caseId=29 Global purge entry existing for vid2 only */
                ++purgePtr->stats.numSuppressedPortVid1Vid2;
            }
            else
            {
                /* add the vid1/vid2 entry */
                status = fmSetBitArrayBit(&purgeEntry->pendingVlans, data.vid1, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              status == FM_OK,
                              "fmSetBitArrayBit returned error!\n");

                if (!foundVid2BitArray)
                {
                    status = InsertTreeKey(&purgeEntry->vid2Tree,
                                           data.vid1,
                                           VID2_ONLY_FLUSH,
                                           &bitArrayPtr);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);
                }
                
                err2 = fmSetBitArrayBit(bitArrayPtr, data.vid2, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmSetBitArrayBit returned error!\n");

                if (allPorts)
                {
                    ++purgePtr->stats.numEnqueuedVid1Vid2;

                    /* Cancel any pending port/vid1/vid2 specific purges
                     * for this vid1/vid2 request */
                    tmpEntry = purgePtr->listHead;
                    
                    while (tmpEntry != NULL)
                    {
                        foundVid2BitArray = GetBitArray(&tmpEntry->vid2Tree, 
                                                        data.vid1, 
                                                        &bitArrayPtr);

                        if ( (tmpEntry->port != -1) && 
                             (foundVid2BitArray) )
                        {
                            if ( fmIsBitInBitArray(bitArrayPtr, data.vid2) )
                            {
                                /* caseId=23 */
                                ++purgePtr->stats.numCancelledPortVid1Vid2;

                                err2 = fmSetBitArrayBit(bitArrayPtr, 
                                                        data.vid2, 
                                                        FALSE);
                                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                              err2 == FM_OK,
                                              "fmSetBitArrayBit returned error!\n");

                                if (bitArrayPtr->nonZeroBitCount == 0)
                                {
                                    /* Remove tree item */
                                    RemoveVid2TreeKey(tmpEntry, 
                                                      data.vid1, 
                                                      bitArrayPtr,
                                                      TRUE);
                                }
                            }
                        }
    
                        /* This entry may now need to be destroyed. */
                        tmp2Entry = tmpEntry;
                        tmpEntry = tmpEntry->next;
                        
                        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
                        {
                            ++purgePtr->stats.numEntriesFreedLater;
                        }
                        
                    }   /* end while (tmpEntry != NULL) */

                }
                else
                {
                     ++purgePtr->stats.numEnqueuedPortVid1Vid2;

                }   /* end if (allPorts) */

            }   /* end if (purgeEntry->allVlansPending) */

        }   /* end if (vid1Defined && !vid2Defined) */
        else if (vid1Defined && remoteIdDefined)
        {
            /* add the vid1/remoteId entry */
            foundRemIdBitArray = GetBitArray(&purgeEntry->remoteIdTree,
                                             data.vid1,
                                             &bitArrayPtr1RemId);

            foundRemIdBitArray2 = GetBitArray(&purgePtr->globalListEntry->remoteIdTree,
                                              data.vid1,
                                              &bitArrayPtr2RemId);

            foundRemIdBitArray3 = GetBitArray(&purgeEntry->remoteIdTree,
                                              REMID_ONLY_FLUSH,
                                              &bitArrayPtr3RemId);

            foundRemIdBitArray4 = GetBitArray(&purgePtr->globalListEntry->remoteIdTree,
                                              REMID_ONLY_FLUSH,
                                              &bitArrayPtr4RemId);

            foundVid2BitArray = GetBitArray(&purgeEntry->vid2Tree, 
                                            data.vid1, 
                                            &bitArrayPtr);

            foundVid2BitArray2 = GetBitArray(&purgePtr->globalListEntry->vid2Tree, 
                                             data.vid1, 
                                             &bitArrayPtr2);

            if (purgeEntry->allVlansPending)
            {
                if (allPorts)
                {
                    /* caseId=52 */
                    ++purgePtr->stats.numSuppressedVid1RemoteId;
                }
                else
                {
                    /* caseId=45 */
                    ++purgePtr->stats.numSuppressedPortVid1RemoteId;
                }
            }
            else if (foundRemIdBitArray &&
                     fmIsBitInBitArray(bitArrayPtr1RemId, data.remoteId))
            {
                if (allPorts)
                {
                    ++purgePtr->stats.numDuplicateVid1RemoteId;
                }
                else
                {
                    ++purgePtr->stats.numDuplicatePortVid1RemoteId;
                }
            }
            else if (fmIsBitInBitArray(&purgeEntry->pendingVlans, data.vid1) &&
                    (!foundRemIdBitArray) && (!foundVid2BitArray) )
            {
                /* Purge entry existing for vid1 only */
                if (allPorts)
                {
                    /* caseId=53 */
                    ++purgePtr->stats.numSuppressedVid1RemoteId;
                }
                else
                {
                    /* caseId=46 */
                    ++purgePtr->stats.numSuppressedPortVid1RemoteId;
                }
            }
            else if (foundRemIdBitArray3 &&
                     fmIsBitInBitArray(bitArrayPtr3RemId, data.remoteId) )
            {
                /* Purge entry existing for remoteId only */
                if (allPorts)
                {
                    /* caseId=54 */
                    ++purgePtr->stats.numSuppressedVid1RemoteId;
                }
                else
                {
                    /* caseId=60 */
                    ++purgePtr->stats.numSuppressedPortVid1RemoteId;
                }
            }
            else if ( (!allPorts) &&
                      ( (purgePtr->globalListEntry->allVlansPending) ||
                        (foundRemIdBitArray2 &&
                         fmIsBitInBitArray(bitArrayPtr2RemId, data.remoteId) ) ) )
            {
                /* caseId=44 or caseId=59 port/vid/remoteId flush requested but
                 *  vid1/remoId request already exists or there is a global 
                 *  request to flush all */ 
                ++purgePtr->stats.numSuppressedPortVid1RemoteId;
            }
            else if ( (!allPorts ) &&
                      fmIsBitInBitArray(&purgePtr->globalListEntry->pendingVlans,
                                        data.vid1) &&
                      (!foundRemIdBitArray2) && (!foundVid2BitArray2) )
            {
                /* caseid=47 Global purge entry existing for vid1 only */
                ++purgePtr->stats.numSuppressedPortVid1RemoteId;
            }
            else if ( (!allPorts ) &&
                      foundRemIdBitArray4 &&
                      fmIsBitInBitArray(bitArrayPtr4RemId, data.remoteId) )
            {
                /* caseId=60 Global purge entry existing for remoteId only */
                ++purgePtr->stats.numSuppressedPortVid1RemoteId;
            }
            else
            {
                /* add the vid1/remoteID entry */
                status = fmSetBitArrayBit(&purgeEntry->pendingVlans, data.vid1, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              status == FM_OK,
                              "fmSetBitArrayBit returned error!\n");

                if (!foundRemIdBitArray)
                {
                    status = InsertTreeKey(&purgeEntry->remoteIdTree,
                                           data.vid1,
                                           REMID_ONLY_FLUSH,
                                           &bitArrayPtr1RemId);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_MAC_MAINT, status);
                }

                err2 = fmSetBitArrayBit(bitArrayPtr1RemId, data.remoteId, TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmSetBitArrayBit returned error!\n");

                if (allPorts)
                {
                    ++purgePtr->stats.numEnqueuedVid1RemoteId;

                    /* Cancel any pending port/vid1/remId specific purges
                     * for this vid1/remId request */
                    tmpEntry = purgePtr->listHead;
                    
                    while (tmpEntry != NULL)
                    {
                        foundRemIdBitArray = GetBitArray(&tmpEntry->remoteIdTree, 
                                                         data.vid1, 
                                                         &bitArrayPtr1RemId);

                        if ( (tmpEntry->port != -1) && 
                             (foundRemIdBitArray) )
                        {
                            if ( fmIsBitInBitArray(bitArrayPtr1RemId, data.remoteId) )
                            {
                                /* caseId=55 */
                                ++purgePtr->stats.numCancelledPortVid1RemoteId;

                                err2 = fmSetBitArrayBit(bitArrayPtr1RemId, 
                                                        data.remoteId, 
                                                        FALSE);
                                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                              err2 == FM_OK,
                                              "fmSetBitArrayBit returned error!\n");

                                if (bitArrayPtr1RemId->nonZeroBitCount == 0)
                                {
                                    /* Remove tree item */
                                    RemoveRemIdTreeKey(tmpEntry, 
                                                       data.vid1, 
                                                       bitArrayPtr1RemId,
                                                       TRUE);
                                }
                            }
                        }
    
                        /* This entry may now need to be destroyed. */
                        tmp2Entry = tmpEntry;
                        tmpEntry = tmpEntry->next;
                        
                        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
                        {
                            ++purgePtr->stats.numEntriesFreedLater;
                        }
                        
                    }   /* end while (tmpEntry != NULL) */

                }
                else
                {
                     ++purgePtr->stats.numEnqueuedPortVid1RemoteId;

                }   /* end if (allPorts) */
            }   /* end if (purgeEntry->allVlansPending) */

        }   /* end if (vid1Defined && !vid2Defined) */
    }   /* end if (allVlans) */

    /**************************************************
     * If there is a callback handler, put it in the
     * list of callback handlers.
     *
     * First scan for a duplicate
     **************************************************/
     
    if (handler != NULL)
    {
        callbackEntry    = purgePtr->callbackList;
        callbackListTail = NULL;

        while (callbackEntry)
        {
            if ( callbackEntry->handler == handler &&
                 callbackEntry->context == context &&
                 callbackEntry->vlan == data.vid1  &&
                 callbackEntry->port == data.port )
            {
                /* Found a duplicate. */
                break;
            }
            
            callbackListTail = callbackEntry;
            callbackEntry = callbackEntry->next;
            
        }   /* end while (callbackEntry) */
        
        if (callbackEntry == NULL)
        {
            /**************************************************
             * We need to allocate a new callback record.
             **************************************************/
             
            callbackEntry = fmAlloc( sizeof(fm_maPurgeCallBack) );
            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                   callbackEntry != NULL,
                                   status = FM_ERR_NO_MEM,
                                   "Couldn't allocate purge callback list entry!\n");
            memset(callbackEntry, 0, sizeof(fm_maPurgeCallBack));
            
            callbackEntry->handler = handler;
            callbackEntry->context = context;
            callbackEntry->port    = data.port;
            callbackEntry->vlan    = data.vid1;
            
            if (callbackListTail)
            {
                /* List not empty - chain to end of list. */
                callbackListTail->next = callbackEntry;
            }
            else
            {
                /* List is empty - chain to head of list. */
                purgePtr->callbackList = callbackEntry;
            }
            
        }   /* end if (callbackEntry == NULL) */
        
        /* Set the sequence number. */
        callbackEntry->seq = purgePtr->nextSeq;
        
    }   /* end if (handler != NULL) */
     
    FM_VERIFY_MA_PURGE(sw);

    status = fmTriggerMAPurge(sw);

ABORT:
    if (vid1DelArrayCreated)
    {
        fmDeleteBitArray(&vid1DelArray);
    }

    FM_DROP_MA_PURGE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, status);

}   /* end fmEnqueueMAPurge */




/*****************************************************************************/
/** fmMaybeDestroyMAPurgeListEntry
 * \ingroup intAddr
 *
 * \desc            Delete a port's purge list entry, but only if the port
 *                  no longer exists and there are no purges still pending 
 *                  on the entry.
 *
 * \note            The caller is assumed to have taken the purge lock 
 *                  (FM_TAKE_MA_PURGE_LOCK).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       purgeEntry points to the port's purge list entry.
 *
 * \return          TRUE if entry was destroyed.
 * \return          FALSE if entry was not destroyed.
 *
 *****************************************************************************/
fm_bool fmMaybeDestroyMAPurgeListEntry(fm_int sw, 
                                       fm_maPurgeListEntry *purgeEntry)
{
    fm_switch * switchPtr;
    fm_maPurge *purgePtr;
    fm_bool     destroyed = FALSE;
    
    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;

    /* If the maPurgeListEntry is no longer needed, destroy it.  The entry is no
     * longer needed if it
     *  -- belongs to a logical port that has been destroyed
     *  -- records no pending purges.
     */
    if ((purgeEntry->port != -1) && !purgeEntry->portExists )
    {
        /**************************************************
         * The current entry belongs to a port that has been
         * destroyed. If there are no more pending purges
         * in that entry, then we can free it now.
         **************************************************/

        if (!purgeEntry->allVlansPending &&
            (purgeEntry->pendingVlans.nonZeroBitCount == 0) &&
            fmTreeSize(&purgeEntry->vid2Tree) == 0 &&
            fmTreeSize(&purgeEntry->remoteIdTree) == 0)
        {
            /**************************************************
             * There are no pending purges for this port.
             * The entry can be destroyed.  Remove it from the 
             * purge list.
             *
             * The entry is never the first on the list, because 
             * we always add to the tail of the list and
             * maPurge.globalListEntry is always the first on
             * the list. 
             **************************************************/
            
            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                   purgeEntry->prev != NULL,
                                   FM_NOT_USED(switchPtr),
                                   "Purge list corrupted!\n");

            /* Remove from the forward link. */
            purgeEntry->prev->next = purgeEntry->next;

            /* Remove from the backward link differently depending on
             * if this was the last entry or not. */
            if (purgeEntry->next)
            {
                /* It was not the last entry. */
                purgeEntry->next->prev = purgeEntry->prev;
            }
            else
            {
                /* It was the last entry. */
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                                       purgePtr->listTail == purgeEntry,
                                       FM_NOT_USED(switchPtr),
                                       "Purge list corrupted!\n");
                purgePtr->listTail = purgeEntry->prev;
            }

            /* Reset the scan position to be the head of the list. */
            purgePtr->scanEntry = purgePtr->listHead;
            
            fmTreeDestroy(&purgeEntry->vid2Tree, NULL);
            fmTreeDestroy(&purgeEntry->remoteIdTree, NULL);

            /* Free the entry. */
            fmDeleteBitArray(&purgeEntry->pendingVlans);
            fmFree(purgeEntry);
            destroyed = TRUE;
            
        }   /* end if (!purgeEntry->allVlansPending &&... */
        
    }   /* end if ((purgeEntry->port != -1) && !purgeEntry->portExists ) */
    
ABORT:
    return destroyed;
    
}   /* end fmMaybeDestroyMAPurgeListEntry */




/*****************************************************************************/
/** fmGetNextPurgeRequest
 * \ingroup intAddr
 *
 * \desc            Scan the purge request list for the next pending purge
 *                  request. If one is found, prep the request structure
 *                  in fm_switch.maPurge.
 *
 * \note            The caller is assumed to have taken the purge lock 
 *                  (FM_TAKE_MA_PURGE_LOCK).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if a purge request was found and prepared.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if no more purge requests were found.
 *
 *****************************************************************************/
fm_status fmGetNextPurgeRequest(fm_int sw)
{
    fm_switch *          switchPtr;
    fm_maPurge *         purgePtr;
    fm_status            err;
    fm_status            err2;
    fm_int               port;
    fm_uint32            glort;
    fm_int               vid1;
    fm_int               vid2;
    fm_int               remoteId;
    fm_int               tmpVid1;
    fm_int               tmpVid2;
    fm_int               tmpRemoteId;
    fm_maPurgeListEntry *startEntry;
    fm_maPurgeListEntry *purgeEntry;
    fm_bool              startEntryScanned;
    fm_bool              foundVid2BitArray;
    fm_bool              foundRemIdBitArray;
    fm_bitArray *        bitArrayPtr;
    fm_bitArray *        bitArrayPtrRemId;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, "sw=%d\n",  sw);

    switchPtr  = GET_SWITCH_PTR(sw);
    purgePtr   = &switchPtr->maPurge;
    
    FM_VERIFY_MA_PURGE(sw);
    
    /**************************************************
     * Store the current scan position so that we know 
     * when to give up looking for a pending event.
     **************************************************/
    
    startEntry          = purgePtr->scanEntry;
    startEntryScanned   = FALSE;
    purgeEntry          = NULL;
    err                 = FM_ERR_NO_MORE;
    vid1                = 0;
    vid2                = 0;
    port                = 0;
    glort               = 0;
    remoteId            = 0;

    while ( !( (purgePtr->scanEntry == startEntry) && (startEntryScanned) ) )
    {
        if (purgePtr->scanEntry->allVlansPending)
        {
            /* One of the following requests:
             * -FM_FLUSH_MODE_ALL_DYNAMIC 
             * -FM_FLUSH_MODE_PORT */
        
            purgeEntry = purgePtr->scanEntry;
            vid1       = -1;
            vid2       = -1;
            remoteId   = -1;
            port       = purgeEntry->port;
            glort      = purgeEntry->glort;
            break;
        }
        else if (purgePtr->scanEntry->pendingVlans.nonZeroBitCount)
        {
            /* One of the following requests:
             * -FM_FLUSH_MODE_VLAN 
             * -FM_FLUSH_MODE_PORT_VLAN 
             * -FM_FLUSH_MODE_VID1_VID2 
             * -FM_FLUSH_MODE_PORT_VID1_VID2 
             * -FM_FLUSH_MODE_PORT_VID1_REMOTEID 
             * -FM_FLUSH_MODE_VID1_REMOTEID */ 

            /* Find next VLAN in bit array marked for purging. */
            err2 = fmFindBitInBitArray(&purgePtr->scanEntry->pendingVlans,
                                       0, 
                                       TRUE, 
                                       &tmpVid1);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err2 == FM_OK,
                          "fmFindBitInBitArray returned an error!\n");

            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          tmpVid1 != -1,
                          "bitArray is corrupt!\n");

            foundVid2BitArray = GetBitArray(&purgePtr->scanEntry->vid2Tree, 
                                            tmpVid1, 
                                            &bitArrayPtr);

            foundRemIdBitArray = GetBitArray(&purgePtr->scanEntry->remoteIdTree, 
                                             tmpVid1, 
                                             &bitArrayPtrRemId);

            if (foundVid2BitArray)
            {
                err2 = fmFindBitInBitArray(bitArrayPtr,
                                           0, 
                                           TRUE, 
                                           &tmpVid2);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmFindBitInBitArray returned an error!\n");

                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              tmpVid2 != -1,
                              "bitArray should not be empty!\n");

                purgeEntry = purgePtr->scanEntry;
                vid1       = tmpVid1;
                vid2       = tmpVid2;
                remoteId   = -1;
                port       = purgeEntry->port;
                glort      = purgeEntry->glort;
                break;
            }
            else if (foundRemIdBitArray)
            {
                err2 = fmFindBitInBitArray(bitArrayPtrRemId,
                                           0, 
                                           TRUE, 
                                           &tmpRemoteId);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              err2 == FM_OK,
                              "fmFindBitInBitArray returned an error!\n");

                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                              tmpRemoteId != -1,
                              "bitArrayPtrRemoteId should not be empty!\n");

                purgeEntry = purgePtr->scanEntry;
                vid1       = tmpVid1;
                vid2       = -1;
                remoteId   = tmpRemoteId;
                port       = purgeEntry->port;
                glort      = purgeEntry->glort;
                break;
            }
            else
            {
                purgeEntry = purgePtr->scanEntry;
                vid1       = tmpVid1;
                vid2       = -1;
                remoteId   = -1;
                port       = purgeEntry->port;
                glort      = purgeEntry->glort;
                break;
            }
        }
        else if (fmTreeSize(&purgePtr->scanEntry->vid2Tree) != 0)
        {
            /* One of the following requests:
             * -FM_FLUSH_MODE_VID2 
             * -FM_FLUSH_MODE_PORT_VID2 */ 

            foundVid2BitArray = GetBitArray(&purgePtr->scanEntry->vid2Tree, 
                                            VID2_ONLY_FLUSH, 
                                            &bitArrayPtr);

            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          foundVid2BitArray == TRUE,
                          "Expected VID2 only entry to exist!\n");

            err2 = fmFindBitInBitArray(bitArrayPtr,
                                       0, 
                                       TRUE, 
                                       &tmpVid2);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err2 == FM_OK,
                          "fmFindBitInBitArray returned an error!\n");

            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          tmpVid2 != -1,
                          "bitArray should not be empty!\n");

            purgeEntry = purgePtr->scanEntry;
            vid1       = -1;
            vid2       = tmpVid2;
            port       = purgeEntry->port;
            remoteId   = -1;
            glort      = purgeEntry->glort;
            break;
        }
        else if (fmTreeSize(&purgePtr->scanEntry->remoteIdTree) != 0)
        {
            /* One of the following requests:
             * -FM_FLUSH_MODE_REMOTEID */ 

            foundRemIdBitArray = GetBitArray(&purgePtr->scanEntry->remoteIdTree, 
                                             REMID_ONLY_FLUSH, 
                                             &bitArrayPtrRemId);

            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          foundRemIdBitArray == TRUE,
                          "Expected RemId only entry to exist!\n");

            err2 = fmFindBitInBitArray(bitArrayPtrRemId,
                                       0, 
                                       TRUE, 
                                       &tmpRemoteId);
            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          err2 == FM_OK,
                          "fmFindBitInBitArray returned an error!\n");

            FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                          tmpRemoteId != -1,
                          "bitArray should not be empty!\n");

            purgeEntry = purgePtr->scanEntry;
            vid1       = -1;
            vid2       = -1;
            port       = purgeEntry->port;
            remoteId   = tmpRemoteId;
            glort      = purgeEntry->glort;
            break;
        }
        else
        {
            /* There are no pending requests in this entry, grab the
             * next purgeEntry */

            if (purgePtr->scanEntry == startEntry)
            {
                /* This prevents the do..while loop to go on forever */
                startEntryScanned = TRUE;
            }

            purgePtr->scanEntry = purgePtr->scanEntry->next;
            
            if (!purgePtr->scanEntry)
            {
                /* Done last request. Go back to first request */
                purgePtr->scanEntry = purgePtr->listHead;
            }
        }

    }   /* end while ( !( (purgePtr->scanEntry == startEntry) && ...) ) */

    if (purgeEntry != NULL)
    {
        RemovePendingPurgeEntryData(purgeEntry, vid1, vid2, remoteId);
        err = FM_OK;
    }
    
    FM_VERIFY_MA_PURGE(sw);
    
    if (err == FM_OK)
    {
        /***************************************************
         * Set up purge request.
         **************************************************/
         
        FM_CLEAR(purgePtr->request);
        purgePtr->request.port     = port;
        purgePtr->request.glort    = glort;
        purgePtr->request.vid1     = vid1;
        purgePtr->request.vid2     = vid2;
        purgePtr->request.remoteId = remoteId;
        purgePtr->request.deleting = FALSE;
        purgePtr->request.seq      = purgePtr->nextSeq++;
        purgePtr->request.expired  = FALSE;

        if ( fmMaybeDestroyMAPurgeListEntry(sw, purgeEntry) )
        {
            ++purgePtr->stats.numEntriesFreedLater;
        }

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_MAC_MAINT,
                    "Next purge request: sw=%d, port=%d, vid1=%d, vid2=%d, "
                    "deleting=%s\n",
                     sw,
                     purgePtr->request.port,
                     purgePtr->request.vid1,
                     purgePtr->request.vid2,
                     FM_BOOLSTRING(purgePtr->request.deleting));
    }
    else if (purgePtr->flushExpired)
    {
        FM_CLEAR(purgePtr->request);
        purgePtr->request.expired = TRUE;
        err = FM_OK;

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_MAC_MAINT,
                    "Next purge request: sw=%d, expired=TRUE\n",
                     sw);
    }

    purgePtr->flushExpired = FALSE;

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end fmGetNextPurgeRequest */




/*****************************************************************************/
/** fmIsAnotherPurgePending
 * \ingroup intAddr
 *
 * \desc            Determine if another MA Table purge request is pending.
 *
 * \note            The caller must first take the purge lock 
 *                  (FM_TAKE_MA_PURGE_LOCK).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          TRUE if another purge is pending.
 * \return          FALSE if another purge is not pending.
 *
 *****************************************************************************/
fm_bool fmIsAnotherPurgePending(fm_int sw)
{
    fm_switch *          switchPtr;
    fm_maPurge *         purgePtr;
    fm_maPurgeListEntry *purgeEntry;

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;
    
    for (purgeEntry = purgePtr->listHead ; 
         purgeEntry != NULL ; 
         purgeEntry = purgeEntry->next) 
    {
        if ( purgeEntry->allVlansPending || 
             (purgeEntry->pendingVlans.nonZeroBitCount != 0) ||
             fmTreeSize(&purgeEntry->vid2Tree) != 0 ||
             fmTreeSize(&purgeEntry->remoteIdTree) != 0)
        {
            ++purgePtr->stats.numCompletedWithMorePending;
            return TRUE;
        }
        
    }   /* end for (purgeEntry = purgePtr->listHead;... */
    
    return FALSE;
        
}   /* end fmIsAnotherPurgePending */




/*****************************************************************************/
/** fmCancelMacTableFlushRequests
 * \ingroup intAddr
 *
 * \desc            Called before the switch is brought down, to flush the
 *                  MA table purge request list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK unconditionally.
 *
 *****************************************************************************/
fm_status fmCancelMacTableFlushRequests(fm_int sw)
{
    fm_switch *          switchPtr;
    fm_status            status;
    fm_status            err2;
    fm_maPurge *         purgePtr;
    fm_maPurgeListEntry *tmpEntry;
    fm_maPurgeListEntry *tmp2Entry;
    fm_bitArray *        bitArrayPtr;
    fm_treeIterator      iter;
    fm_uint64            iterVid1;
    fm_bitArray          vid1DelArray;
    fm_bool              removeVid2OnlyFlush;
    fm_bool              removeRemIdOnlyFlush;
    fm_maPurgeCallBack * callback;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;

    FM_TAKE_MA_PURGE_LOCK(sw);

    removeVid2OnlyFlush = FALSE;
    removeRemIdOnlyFlush = FALSE;
    purgePtr->purgeState = FM_PURGE_STATE_IDLE;
    tmpEntry = purgePtr->listHead;

    err2 = fmCreateBitArray(&vid1DelArray, FM_MAX_VLAN);
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           err2 == FM_OK,
                           (void)err2,
                           "Unable to create bit array\n");
                    
    while (tmpEntry != NULL)
    {
        tmpEntry->allVlansPending = FALSE;
        status = fmClearBitArray(&tmpEntry->pendingVlans);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      status == FM_OK,
                      "fmClearBitArray returned an error!\n");

        status = fmClearBitArray(&vid1DelArray);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      status == FM_OK,
                      "fmClearBitArray returned an error!\n");

        /* Iterate in the vid2tree to remove entries */
        removeVid2OnlyFlush = FALSE;
        for ( fmTreeIterInit(&iter, &tmpEntry->vid2Tree) ;
              (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ; )
        {
            if (iterVid1 == VID2_ONLY_FLUSH)
            {
                removeVid2OnlyFlush = TRUE;
            }
            else
            {
                /* Tag entries for removal */
                err2 = fmSetBitArrayBit(&vid1DelArray, 
                                        iterVid1, 
                                        TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err2 == FM_OK,
                      "fmSetBitArrayBit returned error!\n");
            }
        }

        /* Remove vid2Tree entries tagged for removal
         * and the pendingVlans associated*/
        if (removeVid2OnlyFlush)
        {
            RemoveVid2TreeKey(tmpEntry, VID2_ONLY_FLUSH, NULL, FALSE);
        }

        RemoveMultipleVid2TreeKey(tmpEntry, &vid1DelArray, TRUE);

        status = fmClearBitArray(&vid1DelArray);
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      status == FM_OK,
                      "fmClearBitArray returned an error!\n");

        /* Iterate in the remoteIdTree to remove entries */
        removeRemIdOnlyFlush = FALSE;
        for ( fmTreeIterInit(&iter, &tmpEntry->remoteIdTree) ;
              (err2 = fmTreeIterNext(&iter, &iterVid1, (void **) &bitArrayPtr)) == FM_OK ;
            )
        {
            if (iterVid1 == REMID_ONLY_FLUSH)
            {
                removeRemIdOnlyFlush = TRUE;
            }
            else
            {
                /* Tag entries for removal */
                err2 = fmSetBitArrayBit(&vid1DelArray, 
                                        iterVid1, 
                                        TRUE);
                FM_LOG_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                      err2 == FM_OK,
                      "fmSetBitArrayBit returned error!\n");
            }
        }

        /* Remove remoteIdTree entries tagged for removal
         * and the pendingVlans associated*/
        if (removeRemIdOnlyFlush)
        {
            RemoveRemIdTreeKey(tmpEntry, REMID_ONLY_FLUSH, NULL, FALSE);
        }

        RemoveMultipleRemIdTreeKey(tmpEntry, &vid1DelArray, TRUE);

        /* This entry may now need to be destroyed. */
        tmp2Entry = tmpEntry;
        tmpEntry = tmpEntry->next;
        
        if ( fmMaybeDestroyMAPurgeListEntry(sw, tmp2Entry) )
        {
            ++purgePtr->stats.numEntriesFreedLater;
        }
    }

    fmDeleteBitArray(&vid1DelArray);
    
    purgePtr->scanEntry = purgePtr->globalListEntry;
    memset(&purgePtr->stats, 0, sizeof(purgePtr->stats));

    /* Free the callback list */
    while (purgePtr->callbackList)
    {
        callback = purgePtr->callbackList;
        purgePtr->callbackList = callback->next;
        fmFree(callback);
    }

ABORT:
    FM_DROP_MA_PURGE_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, FM_OK);

}   /* end fmCancelMacTableFlushRequests */




/*****************************************************************************/
/** fmProcessPurgeCallbacks
 * \ingroup intAddr
 *
 * \desc            Upon completion of a MA Table purge, scan the list of
 *                  pending callback records to see if this purge satisfied
 *                  any of them. If so, call the callback functions and
 *                  remove them from the callback list.
 *
 * \note            The caller must have taken the purge lock 
 *                  (FM_TAKE_MA_PURGE_LOCK).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmProcessPurgeCallbacks(fm_int sw)
{
    fm_switch *          switchPtr;
    fm_maPurge *         purgePtr;
    fm_maPurgeCallBack * callbackEntry;
    fm_maPurgeCallBack **callbackListPrev;
    
    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;
    
    callbackEntry    = purgePtr->callbackList;
    callbackListPrev = &purgePtr->callbackList;

    while (callbackEntry)
    {
        /* If purge started after the callback was registered... */
        if (purgePtr->request.seq >= callbackEntry->seq)
        {
            /* ...and the purge covered the callback's VLAN... */
            if (purgePtr->request.vid1 == -1 ||
                purgePtr->request.vid1 == callbackEntry->vlan)
            {
                /* ...and the purge covered the callback's port... */
                if (purgePtr->request.port == -1 || 
                    purgePtr->request.port == callbackEntry->port)
                {
                    /* ...then call the callback function. */
                    callbackEntry->handler(sw, callbackEntry->context);
                    
                    /* Unchain and free the callback record. */
                    *callbackListPrev = callbackEntry->next;
                    fmFree(callbackEntry);
                    callbackEntry = *callbackListPrev;
                    continue;
                }
                
            }   /* end purgePtr->request.vlan == callbackEntry->vlan) */
            
        }   /* end if (purgePtr->request.seq >= callbackEntry->seq) */
        
        /* Chain to next callback record. */
        callbackListPrev = &callbackEntry->next;
        callbackEntry = callbackEntry->next;
        
    }   /* end while (callbackEntry) */
    
    /* If the callback list is now empty, reset the sequence number
     * so we don't have to worry about it wrapping. */
    if (purgePtr->callbackList == NULL)
    {
        purgePtr->nextSeq = 1;
    }
    
}   /* end fmProcessPurgeCallbacks */




/*****************************************************************************/
/** fmDbgDumpMACTablePurgeStats
 * \ingroup diagMATable 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dump the MA Table purge diagnostic statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if sw is not up.
 * \return          FM_ERR_UNSUPPORTED if switch type does not support purges.
 *
 *****************************************************************************/
fm_status fmDbgDumpMACTablePurgeStats(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_status       err;
    
    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);
    FM_API_CALL_FAMILY(err, switchPtr->DumpPurgeStats, sw);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgDumpMACTablePurgeStats */




/*****************************************************************************/
/** fmCommonDumpPurgeStats
 * \ingroup intDiagMATable 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Dumps the MA Table purge diagnostic statistics.
 *                  Called through the DumpPurgeStats function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK unconditionally
 *
 *****************************************************************************/
fm_status fmCommonDumpPurgeStats(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_maPurge *    purgePtr;
    fm_maPurgeStats stats;
    fm_int          purgeCaps;
    fm_text         execText;
    fm_status       err = FM_OK;

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr = &switchPtr->maPurge;
    
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           (switchPtr->switchFamily != FM_SWITCH_FAMILY_FM2000) &&
                           (switchPtr->switchFamily != FM_SWITCH_FAMILY_SWAG),
                           err = FM_ERR_ASSERTION_FAILED,
                           "Switch type not supported by this function.");
    
    /**************************************************
     * Take a snapshot of the stats.
     **************************************************/
    
    FM_TAKE_MA_PURGE_LOCK(sw);
    stats = purgePtr->stats;
    FM_DROP_MA_PURGE_LOCK(sw);
    
    /**************************************************
     * Get purge capabilities of this switch.
     **************************************************/

    purgeCaps = GetPurgeCapabilities(sw);

    if (purgeCaps & PURGE_CAP_HW)
    {
        execText = "written to hardware";
    }
    else
    {
        execText = "initiated";
    }
    
    FM_LOG_PRINT("\nMA Table Purge Statistics\n");
    FM_LOG_PRINT("   Purge Requests made:\n");
    FM_LOG_PRINT("      With invalid port    : %u\n", stats.numRequestedInvalidPort);
    FM_LOG_PRINT("      All ports and VLANs  : %u\n", stats.numRequestedGlobal);
    FM_LOG_PRINT("      Per vlan             : %u\n", stats.numRequestedVlan);
    FM_LOG_PRINT("      Per port             : %u\n", stats.numRequestedPort);
    FM_LOG_PRINT("      Per port/vlan        : %u\n", stats.numRequestedPortVlan);
    if (purgeCaps & PURGE_CAP_ADV)
    {
        FM_LOG_PRINT("      Per port/vid1/vid2   : %u\n", stats.numRequestedPortVid1Vid2);
        FM_LOG_PRINT("      Per port/vid2        : %u\n", stats.numRequestedPortVid2);
        FM_LOG_PRINT("      Per port/vid1/remId  : %u\n", stats.numRequestedPortVid1RemoteId);
        FM_LOG_PRINT("      Per vid1/vid2        : %u\n", stats.numRequestedVid1Vid2);
        FM_LOG_PRINT("      Per vid2             : %u\n", stats.numRequestedVid2);
        FM_LOG_PRINT("      Per vid1/remId       : %u\n", stats.numRequestedVid1RemoteId);
        FM_LOG_PRINT("      Per remId            : %u\n", stats.numRequestedRemoteId);
    }
    if (purgeCaps & PURGE_CAP_EXP)
    {
        FM_LOG_PRINT("      Expired addresses    : %u\n", stats.numRequestedExpired);
    }

    FM_LOG_PRINT("   Duplicate Requests ignored:\n");
    FM_LOG_PRINT("      All ports and VLANs  : %u\n", stats.numDuplicateGlobal);
    FM_LOG_PRINT("      Per vlan             : %u\n", stats.numDuplicateVlan);
    FM_LOG_PRINT("      Per port             : %u\n", stats.numDuplicatePort);
    FM_LOG_PRINT("      Per port/vlan        : %u\n", stats.numDuplicatePortVlan);
    if (purgeCaps & PURGE_CAP_ADV)
    {
        FM_LOG_PRINT("      Per port/vid1/vid2   : %u\n", stats.numDuplicatePortVid1Vid2);
        FM_LOG_PRINT("      Per port/vid2        : %u\n", stats.numDuplicatePortVid2);
        FM_LOG_PRINT("      Per port/vid1/remId  : %u\n", stats.numDuplicatePortVid1RemoteId);
        FM_LOG_PRINT("      Per vid1/vid2        : %u\n", stats.numDuplicateVid1Vid2);
        FM_LOG_PRINT("      Per vid2             : %u\n", stats.numDuplicateVid2);
        FM_LOG_PRINT("      Per vid1/remId       : %u\n", stats.numDuplicateVid1RemoteId);
        FM_LOG_PRINT("      Per remId            : %u\n", stats.numDuplicateRemoteId);
    }

    FM_LOG_PRINT("   Requests suppressed because less specific one pending:\n");
    FM_LOG_PRINT("      Per vlan             : %u\n", stats.numSuppressedVlan);
    FM_LOG_PRINT("      Per port             : %u\n", stats.numSuppressedPort);
    FM_LOG_PRINT("      Per port/vlan        : %u\n", stats.numSuppressedPortVlan);
    if (purgeCaps & PURGE_CAP_ADV)
    {
        FM_LOG_PRINT("      Per port/vid1/vid2   : %u\n", stats.numSuppressedPortVid1Vid2);
        FM_LOG_PRINT("      Per port/vid2        : %u\n", stats.numSuppressedPortVid2);
        FM_LOG_PRINT("      Per port/vid1/remId  : %u\n", stats.numSuppressedPortVid1RemoteId);
        FM_LOG_PRINT("      Per vid1/vid2        : %u\n", stats.numSuppressedVid1Vid2);
        FM_LOG_PRINT("      Per vid2             : %u\n", stats.numSuppressedVid2);
        FM_LOG_PRINT("      Per vid1/remId       : %u\n", stats.numSuppressedVid1RemoteId);
        FM_LOG_PRINT("      Per remId            : %u\n", stats.numSuppressedRemoteId);
    }

    FM_LOG_PRINT("   Requests added to pending list:\n");
    FM_LOG_PRINT("      All ports and VLANs  : %u\n", stats.numEnqueuedGlobal);
    FM_LOG_PRINT("      Per vlan             : %u\n", stats.numEnqueuedVlan);
    FM_LOG_PRINT("      Per port             : %u\n", stats.numEnqueuedPort);
    FM_LOG_PRINT("      Per port/vlan        : %u\n", stats.numEnqueuedPortVlan);
    if (purgeCaps & PURGE_CAP_ADV)
    {
        FM_LOG_PRINT("      Per port/vid1/vid2   : %u\n", stats.numEnqueuedPortVid1Vid2);
        FM_LOG_PRINT("      Per port/vid2        : %u\n", stats.numEnqueuedPortVid2);
        FM_LOG_PRINT("      Per port/vid1/remId  : %u\n", stats.numEnqueuedPortVid1RemoteId);
        FM_LOG_PRINT("      Per vid1/vid2        : %u\n", stats.numEnqueuedVid1Vid2);
        FM_LOG_PRINT("      Per vid2             : %u\n", stats.numEnqueuedVid2);
        FM_LOG_PRINT("      Per vid1/remId       : %u\n", stats.numEnqueuedVid1RemoteId);
        FM_LOG_PRINT("      Per remId            : %u\n", stats.numEnqueuedRemoteId);
    }

    FM_LOG_PRINT("   Pending requests cancelled when less specific one made:\n");
    FM_LOG_PRINT("      Per vlan             : %u\n", stats.numCancelledVlan);
    FM_LOG_PRINT("      Per port             : %u\n", stats.numCancelledPort);
    FM_LOG_PRINT("      Per port/vlan        : %u\n", stats.numCancelledPortVlan);
    if (purgeCaps & PURGE_CAP_ADV)
    {
        FM_LOG_PRINT("      Per port/vid1/vid2   : %u\n", stats.numCancelledPortVid1Vid2);
        FM_LOG_PRINT("      Per port/vid2        : %u\n", stats.numCancelledPortVid2);
        FM_LOG_PRINT("      Per port/vid1/remId  : %u\n", stats.numCancelledPortVid1RemoteId);
        FM_LOG_PRINT("      Per vid1/vid2        : %u\n", stats.numCancelledVid1Vid2);
        FM_LOG_PRINT("      Per vid2             : %u\n", stats.numCancelledVid2);
        FM_LOG_PRINT("      Per vid1/remId       : %u\n", stats.numCancelledVid1RemoteId);
        FM_LOG_PRINT("      Per remId            : %u\n", stats.numCancelledRemoteId);
    }

    FM_LOG_PRINT("   Purge requests %s:\n", execText);
    
    if (purgeCaps & PURGE_CAP_NOP)
    {
        FM_LOG_PRINT("      FM4000 NOP purge     : %u\n", stats.numExecutedNop);
    }
    
    FM_LOG_PRINT("      All ports and VLANs  : %u\n", stats.numExecutedGlobal);
    FM_LOG_PRINT("      Per vlan             : %u\n", stats.numExecutedVlan);
    FM_LOG_PRINT("      Per port             : %u\n", stats.numExecutedPort);
    FM_LOG_PRINT("      Per port/vlan        : %u\n", stats.numExecutedPortVlan);

    if (purgeCaps & PURGE_CAP_ADV)
    {
        FM_LOG_PRINT("      Per port/vid1/vid2   : %u\n", stats.numExecutedPortVid1Vid2);
        FM_LOG_PRINT("      Per port/vid2        : %u\n", stats.numExecutedPortVid2);
        FM_LOG_PRINT("      Per port/vid1/remId  : %u\n", stats.numExecutedPortVid1RemoteId);
        FM_LOG_PRINT("      Per vid1/vid2        : %u\n", stats.numExecutedVid1Vid2);
        FM_LOG_PRINT("      Per vid2             : %u\n", stats.numExecutedVid2);
        FM_LOG_PRINT("      Per vid1/remId       : %u\n", stats.numExecutedVid1RemoteId);
        FM_LOG_PRINT("      Per remId            : %u\n", stats.numExecutedRemoteId);
    }

    FM_LOG_PRINT("   Purge requests %s that completed:\n", execText);

    if (purgeCaps & PURGE_CAP_NOP)
    {
        FM_LOG_PRINT("      FM4000 NOP purge     : %u\n", stats.numCompletedNop);
    }

    FM_LOG_PRINT("      Total purges         : %u\n", stats.numCompletedOther);
    FM_LOG_PRINT("      With more pending    : %u\n", stats.numCompletedWithMorePending);

    FM_LOG_PRINT("   Per-port purge list entries:\n");
    FM_LOG_PRINT("      Currently allocated  : %u\n", stats.numEntriesAllocated);
    FM_LOG_PRINT("      Freed immediately    : %u\n", stats.numEntriesFreedWithLogicalPort);
    FM_LOG_PRINT("      Freed after purge    : %u\n", stats.numEntriesFreedLater);
    FM_LOG_PRINT("\n");
    
ABORT:
    return err;
    
}   /* end fmCommonDumpPurgeStats */




/*****************************************************************************/
/** fmDbgResetMACTablePurgeStats
 * \ingroup diagMATable 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Reset the MA Table purge diagnostic statistics.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_SWITCH_NOT_UP if sw is not up.
 * \return          FM_ERR_UNSUPPORTED if switch type does not support purges.
 *
 *****************************************************************************/
fm_status fmDbgResetMACTablePurgeStats(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_status       err;
    
    VALIDATE_AND_PROTECT_SWITCH(sw);
    
    switchPtr = GET_SWITCH_PTR(sw);
    FM_API_CALL_FAMILY(err, switchPtr->ResetPurgeStats, sw);
    
    UNPROTECT_SWITCH(sw);
    return err;
    
}   /* end fmDbgResetMACTablePurgeStats */




/*****************************************************************************/
/** fmCommonResetPurgeStats
 * \ingroup intDiagMATable 
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Resets MA Table purge diagnostic statistics.
 *                  Called through the ResetPurgeStats function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK unconditionally
 *
 *****************************************************************************/
fm_status fmCommonResetPurgeStats(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_maPurge *    purgePtr;
    fm_status       err = FM_OK;
    
    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr = &switchPtr->maPurge;
    
    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_EVENT_MAC_MAINT,
                           (switchPtr->switchFamily != FM_SWITCH_FAMILY_FM2000) &&
                           (switchPtr->switchFamily != FM_SWITCH_FAMILY_SWAG),
                           err = FM_ERR_ASSERTION_FAILED,
                           "Switch type not supported by this function.");
    
    /**************************************************
     * Take a snapshot of the stats.
     **************************************************/
    
    FM_TAKE_MA_PURGE_LOCK(sw);
    memset( &purgePtr->stats, 0, sizeof(purgePtr->stats) );
    FM_DROP_MA_PURGE_LOCK(sw);
    
ABORT:
    return err;
    
}   /* end fmCommonResetPurgeStats */



