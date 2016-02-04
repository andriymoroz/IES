/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mac_purge_table.c
 * Creation Date:   August 23, 2013
 * Description:     MAC table purge for the FM10000.
 *
 * Copyright (c) 2005 - 2016, Intel Corporation
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

/* The maximum number of entries to skip before relinquishing
 * the L2 resource lock. */
#define SKIP_THRESHOLD      16


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
/** FinishPurge
 * \ingroup intMacMaint
 *
 * \desc            Finishes processing a purge request.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
static fm_status FinishPurge(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_maPurge *    purgePtr;
    fm_status       err = FM_OK;
#if FM_SUPPORT_SWAG
    fm_int          swagSw;
    fm_bool         swagLagLockTaken;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;

#if FM_SUPPORT_SWAG
    /* SWAG LAG lock must be taken first. */
    swagLagLockTaken = FALSE;

    err = fmIsSwitchInASWAG(sw, &swagSw);
    if (err == FM_OK)
    {
        TAKE_LAG_LOCK(swagSw);
        swagLagLockTaken = TRUE;
    }
#endif  

    /**************************************************
     * The LAG lock must be taken first in case we 
     * make a callback to the LAG deletion logic. 
     * The LAG lock is lower precedence than the L2 
     * and purge locks.
     **************************************************/

    TAKE_LAG_LOCK(sw);
    FM_TAKE_L2_LOCK(sw);
    FM_TAKE_MA_PURGE_LOCK(sw);
    
    ++purgePtr->stats.numCompletedOther;
    
    /* Process any callbacks satisfied by the last purge. */
    fmProcessPurgeCallbacks(sw);
    
    fmDbgDiagCountIncr(sw, FM_CTR_PURGE_COMPLETE, 1);

    /* Set purge state back to idle. */
    purgePtr->purgeState = FM_PURGE_STATE_IDLE;
    
    /**************************************************
     * Check the purge request queue for more work to
     * do.
     **************************************************/
     
    if ( fmIsAnotherPurgePending(sw) )
    {
        err = fmTriggerMAPurge(sw);
    }
    
    FM_DROP_MA_PURGE_LOCK(sw);
    FM_DROP_L2_LOCK(sw);
    DROP_LAG_LOCK(sw);

#if FM_SUPPORT_SWAG
    if (swagLagLockTaken == TRUE)
    {
        DROP_LAG_LOCK(swagSw);
    }
#endif

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end FinishPurge */




/*****************************************************************************/
/** InitiatePurge
 * \ingroup intAddr
 *
 * \desc            Initiates an MA table purge operation.
 *
 * \note            The caller is assumed to have taken the purge lock 
 *                  (FM_TAKE_MA_PURGE_LOCK).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitiatePurge(fm_int sw)
{
    fm_switch *         switchPtr;
    fm_maPurge *        purgePtr;
    fm_maPurgeRequest * request;
    fm_status           err;

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;
    request   = &purgePtr->request;

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_MAC_MAINT,
                 "sw=%d, port=%d, vlan=%d\n",
                 sw,
                 request->port,
                 request->vid1);

    purgePtr->restoreLocked = FALSE;
    purgePtr->purgeTimeout = 0;

    /* Get purge start time. */
    err = fmGetTime( &purgePtr->startTime );
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                     "Error getting purge start time: %s\n",
                     fmErrorMsg(err));
    }

    purgePtr->purgeState = FM_PURGE_STATE_ACTIVE;

    return FM_OK;

}   /* end InitiatePurge */




/*****************************************************************************/
/** MeetsPurgeCriteria
 * \ingroup intMacMaint
 *
 * \desc            Determines whether an MA Table entry meets the criteria
 *                  for the current purge request.
 *
 * \note            The caller is assumed to have taken the L2 lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       entry points to an internal mac address structure
 *                  representing the address table entry.
 *
 * \return          TRUE if the entry meets the purge criteria, FALSE
 *                  otherwise.
 *
 *****************************************************************************/
static fm_bool MeetsPurgeCriteria(fm_int                   sw,
                                  fm_internalMacAddrEntry* entry)
{
    fm_switch *         switchPtr;
    fm_maPurge *        purgePtr;
    fm_maPurgeRequest * request;

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;
    request   = &purgePtr->request;

    if ( entry->state == FM_MAC_ENTRY_STATE_INVALID ||
        (entry->state == FM_MAC_ENTRY_STATE_LOCKED && 
         request->statics == FALSE) )
    {
        /* We are only purging dynamic entries. */
        return FALSE;
    }

    if (entry->state == FM_MAC_ENTRY_STATE_EXPIRED)
    {
        /* Entry has aged out. */
        return TRUE;
    }
    else if (request->expired)
    {
        /* We are only purging expired entries. */
        return FALSE;
    }

    if (request->port < 0 && request->vid1 < 0)
    {
        /* Matches all dynamic entries. */
        return TRUE;
    }

    if (request->vid1 >= 0 && request->vid1 != entry->vlanID)
    {
        /* VLAN does not match. */
        return FALSE;
    }

    if ( (request->port < 0) ||
         ( (entry->port == request->port) &&
           (entry->isTunnelEntry == FM_DISABLED) ) )
    {
        /* Port matches. */
        return TRUE;
    }

    if (request->port < 0)
    {
        if (request->vid1 < 0)
        {
            /* All dynamic entries. */
            return TRUE;
        }
        else if (request->vid1 == entry->vlanID)
        {
            /* All ports for vlan. */
            return TRUE;
        }
    }
    else if (request->port == entry->port &&
             entry->isTunnelEntry == FM_DISABLED)
    {
        if (request->vid1 < 0)
        {
            /* All vlans for port. */
            return TRUE;
        }
        else if (request->vid1 == entry->vlanID)
        {
            /* Vlan and port. */
            return TRUE;
        }
    }

    return FALSE;

}   /* end MeetsPurgeCriteria */




/*****************************************************************************/
/** PerformPurge
 * \ingroup intMacMaint
 *
 * \desc            Initiates an MA table purge operation.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PerformPurge(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_bool         l2Locked;
    fm_int          numSkipped;
    fm_event *      eventPtr;
    fm_uint32       numUpdates;
    fm_int          entryIndex;
    fm_status       err;

    fm_internalMacAddrEntry *   cachePtr;
    fm_internalMacAddrEntry     oldEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    l2Locked    = FALSE;
    numSkipped  = 0;
    numUpdates  = 0;

    /***************************************************
     * Preallocate an event buffer.
     **************************************************/

    /* NOTE: This call will block if the number of free event 
     * buffers drops below the acceptable threshold. */
    eventPtr = fmAllocateEvent(sw,
                               FM_EVID_HIGH_TABLE_UPDATE,
                               FM_EVENT_TABLE_UPDATE,
                               FM_EVENT_PRIORITY_LOW);

    if (eventPtr == NULL)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_MAC_EVENT_ALLOC_ERR, 1);

        FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                     "out of event buffers\n");
    }

    /***************************************************
     * Iterate over MAC table cache.
     **************************************************/

    for ( entryIndex = 0 ;
          entryIndex < switchPtr->macTableSize ;
          ++entryIndex )
    {
        if (!l2Locked)
        {
            FM_TAKE_L2_LOCK(sw);
            l2Locked   = TRUE;
            numSkipped = 0;
        }

        cachePtr = &switchPtr->maTable[entryIndex];

        /***************************************************
         * Determine whether to purge this entry.
         **************************************************/

        if (!MeetsPurgeCriteria(sw, cachePtr))
        {
            ++numSkipped;
            if (numSkipped >= SKIP_THRESHOLD)
            {
                FM_DROP_L2_LOCK(sw);
                l2Locked = FALSE;
            }
            continue;
        }

        if (FM_IS_TEST_TRACE_ADDRESS(cachePtr->macAddress))
        {
            FM_LOG_PRINT("fm10000HandlePurgeRequest(%d): "
                         "purging mac address " FM_FORMAT_ADDR "/%u\n",
                         sw,
                         cachePtr->macAddress,
                         cachePtr->vlanID);
        }

        /***************************************************
         * Remove entry from MAC table.
         **************************************************/

        /* Make a copy of the cache entry before we invalidate it.
         * We'll need it to generate the AGED event. */ 
        oldEntry = *cachePtr;

        /* Invalidate cache and hardware entry. */
        err = fm10000InvalidateEntryAtIndex(sw, entryIndex);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT_MAC_MAINT,
                         "Error purging MA table entry: sw=%d index=%d\n",
                         sw,
                         entryIndex);
        }

        /***************************************************
         * Generate an AGED event.
         **************************************************/

        /* Relinquish lock before generating event. */
        FM_DROP_L2_LOCK(sw);
        l2Locked = FALSE;

        fmGenerateUpdateForEvent(sw,
                                 &fmRootApi->eventThread,
                                 FM_EVENT_ENTRY_AGED,
                                 FM_MAC_REASON_PURGE_AGED,
                                 entryIndex,
                                 &oldEntry,
                                 &numUpdates,
                                 &eventPtr);

        fmDbgDiagCountIncr(sw, FM_CTR_MAC_PURGE_AGED, 1);

    }   /* end for ( entryIndex = 0 ; ... ) */

    if (l2Locked)
    {
        FM_DROP_L2_LOCK(sw);
    }

    if (numUpdates != 0)
    {
        fmSendMacUpdateEvent(sw,
                             &fmRootApi->eventThread,
                             &numUpdates,
                             &eventPtr,
                             FALSE);
    }

    if (eventPtr != NULL)
    {
        fmReleaseEvent(eventPtr);
    }

    err = FinishPurge(sw);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end PerformPurge */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000HandlePurgeRequest
 * \ingroup intMacMaint
 *
 * \desc            Services the purge request queue. Called through the
 *                  HandlePurgeRequest function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000HandlePurgeRequest(fm_int sw)
{
    fm_switch *         switchPtr;
    fm_maPurge *        purgePtr;
    fm_status           err;
    fm_bool             purgeLocked;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_MAC_MAINT, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    purgePtr  = &switchPtr->maPurge;

    FM_TAKE_MA_PURGE_LOCK(sw);
    purgeLocked = TRUE;
    
    if (purgePtr->purgeState == FM_PURGE_STATE_ACTIVE)
    {
        err = FM_OK;
        goto ABORT;
    }

    /***************************************************
     * Get purge request.
     **************************************************/

    err = fmGetNextPurgeRequest(sw);
    if (err != FM_OK)
    {
        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
        }
        goto ABORT;
    }

    /***************************************************
     * Initiate software purge.
     **************************************************/

    err = InitiatePurge(sw);

    FM_DROP_MA_PURGE_LOCK(sw);
    purgeLocked = FALSE;

    /***************************************************
     * Perform the purge.
     **************************************************/

    if (err == FM_OK)
    {
        err = PerformPurge(sw);
    }

ABORT:
    if (purgeLocked)
    {
        FM_DROP_MA_PURGE_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_MAC_MAINT, err);

}   /* end fm10000HandlePurgeRequest */

