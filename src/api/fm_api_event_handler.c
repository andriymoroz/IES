/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_handler.c
 * Creation Date:   2005
 * Description:     Functions for initialization and switch status/information
 *                  retrieval functions for the API
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define MIN_WAIT_NANOS    FM_LITERAL_U64(1000)      /* start at 1 microsecond */
#define MAX_WAIT_NANOS    FM_LITERAL_U64(500000000) /* back off to 1/2 second */

#define NANOS_PER_SECOND  FM_LITERAL_U64(1000000000)
#define DELAY_NANOS(x)                            \
    fmDelay( (fm_int) ( (x) / NANOS_PER_SECOND ), \
            (fm_int) ( (x) % NANOS_PER_SECOND ) )

/*****************************************************************************
 * Global Variables
 *****************************************************************************/
/*
 * used to signal local dispatch thread to exit
 */
fm_bool localDispatchThreadExit = FALSE;


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_bool enableFramePriority = FALSE;

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
/** fmDistributeEvent
 * \ingroup intSwitch
 *
 * \desc            distributes events to those processes that have registered
 *                  an interest in the particular event.
 *
 * \param[in]       event points to the event structure.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmDistributeEvent(fm_event *event)
{
    fmCaptureLock(&fmRootApi->localDeliveryLock, FM_WAIT_FOREVER);

    {
        /**************************************************
         * We want to have a consistent snapshot of the
         * local delivery list, but we don't want to hold
         * the lock while we deliver all the events, so we
         * briefly grab the lock and copy the list into a
         * C99 variable-size array.
         **************************************************/

        fm_uint          count = fmRootApi->localDeliveryCount;
        fm_localDelivery delivery[count];
        fm_dlist_node *  node;
        fm_uint          i;
        fm_uint          pktDeliveryCount = 0;
        fm_eventPktRecv *rcvPktEvent = NULL;
        fm_status        status;
        fm_buffer        *buffer;

        node = FM_DLL_GET_FIRST( (&fmRootApi->localDeliveryThreads), head );

        for (i = 0 ; (node != NULL) && (i < count) ; i++)
        {
            delivery[i] = *(fm_localDelivery *) node->data;

            if ( (delivery[i].mask & 
                  (FM_EVENT_PKT_RECV | FM_EVENT_SFLOW_PKT_RECV)) &
                  event->type )
            {
                /* Found thread we need to deliver packet to. */
                pktDeliveryCount++;
            }

            node = FM_DLL_GET_NEXT(node, nextPtr);
        }

        /**************************************************
         * If the event is packet receive but no one has
         * registered for the event, free the associated
         * packet buffer and return.
         **************************************************/

        if ( ( (event->type == FM_EVENT_PKT_RECV) ||
               (event->type == FM_EVENT_SFLOW_PKT_RECV) ) 
             && (pktDeliveryCount == 0) )
        {
            rcvPktEvent = &event->info.fpPktEvent;
            if (enableFramePriority)
            {
                status = fmFreeBufferQueueNode(event->sw, rcvPktEvent);
                if (status != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                                 "Freeing Buffer queue node from the queue failed"
                                 "status = %d (%s) \n",
                                  status,
                                  fmErrorMsg(status));

                }
            }
            fmFreeBufferChain(event->sw, rcvPktEvent->pkt);
            fmDbgDiagCountIncr(event->sw, FM_CTR_RX_API_PKT_DROPS, 1);
            fmReleaseLock(&fmRootApi->localDeliveryLock);
            return;
        }

        /* valid actually found */
        count = i;
        fmReleaseLock(&fmRootApi->localDeliveryLock);

        /**************************************************
         * Now we do the actual delivery
         **************************************************/

        for (i = 0 ; i < count ; i++)
        {
            fm_event *localEvent = NULL;
            fm_uint64 nanos      = MIN_WAIT_NANOS;
            fm_status err        = FM_FAIL;
            fm_uint32 numUpdates;

            if ( (delivery[i].mask & event->type) == 0 )
            {
                continue;
            }

            /**************************************************
             * Always use high priority for the locally dispatched
             * events, because DistributeEvent is only called from
             * a single thread (the global event handler), and if
             * we allocated both low and high priority events here,
             * we could get priority inversion.
             **************************************************/

            while (localEvent == NULL)
            {
                localEvent = fmAllocateEvent(event->sw,
                                             event->eventID,
                                             event->type,
                                             FM_EVENT_PRIORITY_HIGH);

                if (localEvent == NULL)
                {
                    DELAY_NANOS(nanos);
                    nanos *= 2;

                    if (nanos > MAX_WAIT_NANOS)
                    {
                        nanos = MAX_WAIT_NANOS;
                        FM_LOG_WARNING(FM_LOG_CAT_EVENT,
                                       "Waiting to allocate event of type %d "
                                       "for switch %d\n",
                                       event->type,
                                       event->sw);
                    }
                }
            }

            if (event->type == FM_EVENT_TABLE_UPDATE)
            {
                /**************************************************
                 * Because the updates field is a pointer to memory
                 * that has been "secretly" allocated after the event,
                 * rather than just being part of the union, we have
                 * to handle it specially.
                 **************************************************/

                numUpdates = event->info.fpUpdateEvent.numUpdates;
                localEvent->info.fpUpdateEvent.numUpdates = numUpdates;
                FM_MEMCPY_S( localEvent->info.fpUpdateEvent.updates,
                             numUpdates * sizeof(fm_eventTableUpdate),
                             event->info.fpUpdateEvent.updates,
                             numUpdates * sizeof(fm_eventTableUpdate) );
            }
            else if (event->type == FM_EVENT_PURGE_SCAN_COMPLETE)
            {
                localEvent->info.purgeScanComplete = event->info.purgeScanComplete;
            } 
            else if ( (event->type == FM_EVENT_PKT_RECV) ||
                      (event->type == FM_EVENT_SFLOW_PKT_RECV) )
            {
                rcvPktEvent = &event->info.fpPktEvent;

                /**************************************************
                 * Copy the whole event, including the packet, to
                 * localEvent. If this is not the last registered
                 * client, we will overwrite the packet with a
                 * clone.
                 **************************************************/

                localEvent->info = event->info;

                /**************************************************
                 * If there is more than one remaining process that is
                 * interested in receive packet events, clone the
                 * receive buffer for delivery.
                 **************************************************/

                if (pktDeliveryCount-- > 1)
                {
                    if (enableFramePriority)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_EVENT,
                                     "Prioritization is supported only for the"
                                     "first registered process. Subsequent "
                                     "processes follow normal buffer allocation"
                                     " without prioritization.\n");
                    }
                    localEvent->info.fpPktEvent.pkt =
                        fmDuplicateBufferChain(event->sw, rcvPktEvent->pkt);

                    if (localEvent->info.fpPktEvent.pkt == NULL)
                    {
                        /**************************************************
                         * Couldn't copy the packet. Free the event so that
                         * it is not lost and continue the loop.
                         **************************************************/

                        fmReleaseEvent(localEvent);
                        fmDbgDiagCountIncr(event->sw, FM_CTR_RX_API_PKT_DROPS, 1);
                        continue;
                    }

                }

                if (enableFramePriority)
                {
                    buffer = ((fm_buffer *)(localEvent->info.fpPktEvent.pkt));
                    buffer->recvEvent = localEvent;
                }
            }
            else
            {
                /**************************************************
                 * Otherwise, we can just copy the whole union
                 * without worrying what type it is.
                 **************************************************/

                localEvent->info = event->info;
            }

            /**************************************************
             * Now try to send the event to the local dispatch
             * thread, using exponential backoff if the event
             * queue is full.
             **************************************************/

            nanos = MIN_WAIT_NANOS;

            while (err != FM_OK)
            {
                err = fmSendThreadEvent(delivery[i].thread, localEvent);

                if (err != FM_OK)
                {
                    DELAY_NANOS(nanos);
                    nanos *= 2;

                    if (nanos > MAX_WAIT_NANOS)
                    {
                        nanos = MAX_WAIT_NANOS;
                    }

                }   /* end if (err != FM_OK) */

            }   /* end while (err != FM_OK) */

        }   /* end for (i = 0 ; i < count ; i++) */

    }   /* end (local scope) */

}   /* end fmDistributeEvent */




/*****************************************************************************/
/** fmGlobalEventHandler
 * \ingroup intSwitch
 *
 * \desc            event handler for handling system events
 *
 * \param[in]       args points to the thread arguments
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void *fmGlobalEventHandler(void *args)
{
    fm_thread *               thread;
    fm_event *                event;
    fm_status                 err       = FM_OK;
    fm_eventPort *            portEvent = NULL;
    fm_eventTableUpdateBurst *updateEvent;
    fm_int                    physPort;
    fm_int                    logicalPort;
    fm_eventPktRecv *         rcvPktEvent;
    fm_eventSwitchInserted *  insertEvent;
    fm_eventSwitchRemoved *   removeEvent;
    fm_int                    sw = 0;
    fm_bool                   discardEvent;
    fm_port *                 portPtr = NULL;
    fm_bool                   switchIsProtected;
    fm_switch *               switchPtr;
    fm_int                    mode;
    fm_int                    info[8];
    fm_int                    state;
    fm_int                    numLanes;
    fm_uint32                 i;
    fm_bool                   isPhysicalSwitch;
    fm_switchEventHandler     eventHandler;
    fm_bool                   distributeEvent;
    fm_eventTableUpdate *     fpUpdateEvent;

    /* grab arguments */
    thread = FM_GET_THREAD_HANDLE(args);

    /* wait for initialization to finish before processing events */
    fmCaptureSemaphore(&fmRootApi->startGlobalEventHandler, FM_WAIT_FOREVER);

    enableFramePriority = GET_PROPERTY()->priorityBufQueues;

    while (1)
    {
        /* wait forever for an event */
        err = fmGetThreadEvent(thread, &event, FM_WAIT_FOREVER);

        if (err == FM_ERR_NO_EVENTS_AVAILABLE)
        {
            /* A timeout occurred - should never happen. */
            continue;
        }

        if (event == NULL)
        {
            /* NULL event should never happen. */
            continue;
        }

        sw                = event->sw;
        discardEvent      = FALSE;
        switchIsProtected = FALSE;
        switchPtr         = NULL;

        if (sw < 0 || sw >= fmRootPlatform->cfg.numSwitches)
        {
            discardEvent      = TRUE;
            switchIsProtected = FALSE;
        }
        else if ( SWITCH_LOCK_EXISTS(sw) )
        {
            if ( ( err = PROTECT_SWITCH(sw) ) != FM_OK )
            {
                discardEvent      = TRUE;
                switchIsProtected = FALSE;
            }
            else
            {
                switchIsProtected = TRUE;
                switchPtr         = fmRootApi->fmSwitchStateTable[sw];

                if (!fmRootApi->fmSwitchStateTable[sw])
                {
                    if ((event->type != FM_EVENT_SWITCH_REMOVED) &&
                        (event->type != FM_EVENT_SWITCH_INSERTED) )
                    {
                        discardEvent = TRUE;
                    }
                }
                else if (fmRootApi->fmSwitchStateTable[sw]->state != FM_SWITCH_STATE_UP)
                {
                    if ((event->type != FM_EVENT_SWITCH_REMOVED) &&
                        (event->type != FM_EVENT_SWITCH_INSERTED) )
                    {
                        discardEvent = TRUE;
                    }
                }
            }
        }
        else if (event->type != FM_EVENT_SWITCH_INSERTED)
        {
            discardEvent = TRUE;
        }


        if (discardEvent)
        {
            switch (event->type)
            {
                case FM_EVENT_PKT_RECV:
                case FM_EVENT_SFLOW_PKT_RECV:
                    /* Only dig into the event if the switch is valid */
                    if  ( (sw >= 0) && (sw < fmRootPlatform->cfg.numSwitches) )
                    {
                        rcvPktEvent = &event->info.fpPktEvent;
                        if (enableFramePriority)
                        {
                            err = fmFreeBufferQueueNode(sw, rcvPktEvent);
                            if (err != FM_OK)
                            {
                                FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                                             "Freeing Buffer queue node from the queue failed"
                                             "status = %d (%s) \n",
                                              err,
                                              fmErrorMsg(err));

                            }
                        }
                        fmFreeBufferChain(sw, rcvPktEvent->pkt);
                        fmDbgDiagCountIncr(sw, FM_CTR_RX_API_PKT_DROPS, 1);
                    }
                    break;

                default:
                    break;
            }

            goto FINISHED;
        }

        eventHandler     = NULL;

        if (switchPtr != NULL)
        {
            /* If the switch state table has an eventHandler pointer,
             * it overrides the global handler.  Call the switch-specific
             * function to handle the event.  This is intended to be used
             * for switches in a switch aggregate (and potentially
             * nested switch aggregates inside other switch aggregates?).
             */
            eventHandler = switchPtr->eventHandler;

            switch (switchPtr->switchModel)
            {
                case FM_SWITCH_MODEL_SWAG:
                    isPhysicalSwitch = FALSE;
                    break;

                default:
                    isPhysicalSwitch = TRUE;
                    break;
            }
        }
        else
        {
            /* Only physical switches should ever get here with a NULL pointer
             * because logical switches such as switch aggregates are always
             * created by application code before any events related to
             * the switch are possible.
             */
            isPhysicalSwitch = TRUE;
        }

        distributeEvent = FALSE;

        switch (event->type)
        {
            case FM_EVENT_SWITCH_INSERTED:
                insertEvent = &event->info.fpSwitchInsertedEvent;
                
                if (switchIsProtected)
                {
                    UNPROTECT_SWITCH(sw);
                    switchIsProtected = FALSE;
                }

                if (switchPtr == NULL)
                {
                    if (fmHandleSwitchInserted(sw, insertEvent) != FM_OK)
                    {
                        /* Don't generate an insert event if there error */
                        goto FINISHED;
                    }
                }

                distributeEvent = TRUE;

                break;

            case FM_EVENT_SWITCH_REMOVED:
                removeEvent = &event->info.fpSwitchRemovedEvent;

                if (switchIsProtected)
                {
                    UNPROTECT_SWITCH(sw);
                    switchIsProtected = FALSE;
                }

                if (switchPtr != NULL)
                {
                    fmHandleSwitchRemoved(sw, removeEvent);
                }

                distributeEvent = TRUE;
                break;

            case FM_EVENT_PORT:
                portEvent = &event->info.fpPortEvent;

                if (isPhysicalSwitch && portEvent->activeMac)
                {
                    logicalPort = portEvent->port;

                    if (switchPtr != NULL)
                    {
                        fmMapLogicalPortToPhysical(switchPtr,
                                                   logicalPort,
                                                   &physPort);

                        portPtr = switchPtr->portTable[logicalPort];
                    }
                    else
                    {
                        portPtr = NULL;
                    }

                    if (portPtr == NULL)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PORT,
                                     "Unexpected NULL port pointer for logical"
                                     " port %d\n",
                                     logicalPort);
                        break;
                    }

                    /* This attribute indicate whether the API should flush
                     * all the addresses on a port down event or not. */
                    if (GET_PROPERTY()->maFlushOnPortDown)
                    {
                        /* If a link goes down for a non-LAG port, remove any
                         * addresses associated with the port from the MA Table. */
                        if (portEvent->linkStatus == FM_PORT_STATUS_LINK_DOWN)
                        {
                            if (portPtr->portType != FM_PORT_TYPE_LAG)
                            {
                                err = fmFlushPortAddresses(sw, portEvent->port);

                                if (err != FM_OK)
                                {
                                    FM_LOG_WARNING(FM_LOG_CAT_EVENT_PORT,
                                                   "%s\n",
                                                   fmErrorMsg(err));
                                }
                            }
                        }
                    }

                    FM_LOG_DEBUG( FM_LOG_CAT_EVENT_PORT,
                                  "Port %s event reported on port %d.\n",
                                  (portPtr != NULL )
                                    ? ( (portPtr->linkUp) ? "UP  " : "DOWN" )
                                    : "UNKN",
                                  portEvent->port );

                    /* inform LAG module of port state changes */
                    if (portEvent->linkStatus == FM_PORT_STATUS_LINK_UP)
                    {
                        fmInformLAGPortUp(sw, portEvent->port);
                        
                        /* Inform LBGs of port link state change. */
                        FM_API_CALL_FAMILY_VOID(switchPtr->InformLBGLinkChange,
                                                sw, 
                                                portEvent->port, 
                                                FM_PORT_STATUS_LINK_UP);
                    }
                    else if (portEvent->linkStatus == FM_PORT_STATUS_LINK_DOWN)
                    {
                        fmInformLAGPortDown(sw, portEvent->port);
                        
                        /* Inform LBGs of port link state change. */
                        FM_API_CALL_FAMILY_VOID(switchPtr->InformLBGLinkChange,
                                                sw, 
                                                portEvent->port, 
                                                FM_PORT_STATUS_LINK_DOWN);
                    }

                    /* now update all the source masks */
                    fmUpdateSwitchPortMasks(sw);

                    if (switchPtr->UpdateRemoveDownPortsTrigger != NULL)
                    {
                        /**************************************************** 
                         * Update the switchExt->removeDownPortsTrigger
                         * used to drop routed/multicast/special delivery 
                         * frames which can not be handled by the PORT_CFG_2. 
                         * See Bugzilla 11387.
                         ***************************************************/
                        if (portEvent->linkStatus == FM_PORT_STATUS_LINK_UP)
                        {
                            switchPtr->UpdateRemoveDownPortsTrigger(sw, 
                                                                    physPort,
                                                                    FALSE);
                        }
                        else if (portEvent->linkStatus == FM_PORT_STATUS_LINK_DOWN)
                        {
                            if (!portPtr->isPortForceUp)
                            {
                                switchPtr->UpdateRemoveDownPortsTrigger(sw, 
                                                                        physPort,
                                                                        TRUE);
                            }
                        }
                    }

                    if (switchPtr->UpdateMirrorGroups != NULL)
                    {
                        /**************************************************** 
                         * Enable/Disable mirror groups based on the link
                         * status of the mirror port.
                         * See Bugzilla 11387.
                         ***************************************************/
                        if (portEvent->linkStatus == FM_PORT_STATUS_LINK_UP)
                        {
                            switchPtr->UpdateMirrorGroups(sw, 
                                                          logicalPort,
                                                          TRUE);
                        }
                        else if (portEvent->linkStatus == FM_PORT_STATUS_LINK_DOWN)
                        {
                            switchPtr->UpdateMirrorGroups(sw, 
                                                          logicalPort,
                                                          FALSE);
                        }
                    }

                    /* notify anyone else who needs to know */
                    if (portPtr && portPtr->NotifyLinkEvent)
                    {
                        err = portPtr->NotifyLinkEvent(sw, portEvent->port);

                        if (err != FM_OK)
                        {
                            FM_LOG_WARNING(FM_LOG_CAT_EVENT_PORT,
                                           "%s\n",
                                           fmErrorMsg(err));
                        }
                    }

                    /* Get port state and notify platform */

                    err = fmGetPortStateV3( sw,
                                            portEvent->port,
                                            portEvent->mac,
                                            8,
                                            &numLanes,
                                            &mode,
                                            &state,
                                            info );

                    if (err != FM_OK)
                    {
                        FM_LOG_WARNING(FM_LOG_CAT_EVENT_PORT,
                                       "fmGetPortState(%d,%u) failed: %s\n",
                                       sw,
                                       portEvent->port,
                                       fmErrorMsg(err));
                    }

                    fmPlatformNotifyPortState(sw,
                                              portEvent->port,
                                              portEvent->mac,
                                              FALSE,
                                              state);

                    if (switchIsProtected)
                    {
                        UNPROTECT_SWITCH(sw);
                        switchIsProtected = FALSE;
                    }

                }   /* end if (isPhysicalSwitch) */

                distributeEvent = TRUE;
                break;

            case FM_EVENT_PKT_RECV:
            case FM_EVENT_SFLOW_PKT_RECV:
                fmDbgDiagCountIncr(sw, FM_CTR_RX_API_PKT_FWD, 1);
                distributeEvent = TRUE;
                break;

            case FM_EVENT_PURGE_SCAN_COMPLETE:
                distributeEvent = TRUE;
                break; 

            case FM_EVENT_TABLE_UPDATE:
                if (switchPtr == NULL)
                {
                    break;
                }

                /* Update diagnostic counters. */
                updateEvent = &event->info.fpUpdateEvent;
                i = 0;

                while (i < updateEvent->numUpdates)
                {
                    fpUpdateEvent = &updateEvent->updates[i];

                    if (fpUpdateEvent->event == FM_EVENT_ENTRY_LEARNED)
                    {
                        /* Make sure the MA Table entry matches the entry
                         * in the update event. */
                        if (switchPtr->RemoveStaleLearnEvent != NULL &&
                            switchPtr->RemoveStaleLearnEvent(sw, updateEvent, i))
                        {
                            /* The learn event has been removed.
                             * Do not update 'i', since it now contains the 
                             * following event (if any). */
                            fmDbgDiagCountIncr(sw, FM_CTR_MAC_LEARN_DISCARDED, 1);
                        }
                        else
                        {
                            fmDbgDiagCountIncr(sw, FM_CTR_MAC_ALPS_LEARN, 1);
                            i++;
                        }
                    }
                    else
                    {
                        if (fpUpdateEvent->event == FM_EVENT_ENTRY_AGED)
                        {
                            fmDbgDiagCountIncr(sw, FM_CTR_MAC_ALPS_AGE, 1);
                        }
                        i++;
                    }
                }

                /* If all updates have been removed, don't distribute the
                   event */
                if (updateEvent->numUpdates > 0)
                {
                    distributeEvent = TRUE;
                }
                break;

            case FM_EVENT_SECURITY:
            case FM_EVENT_FRAME:
            case FM_EVENT_SOFTWARE:
            case FM_EVENT_PARITY_ERROR:
            case FM_EVENT_FIBM_THRESHOLD:
            case FM_EVENT_CRM:
            case FM_EVENT_ARP:
            case FM_EVENT_EGRESS_TIMESTAMP:
            case FM_EVENT_PLATFORM:
            case FM_EVENT_LOGICAL_PORT:
            case FM_EVENT_CABLE_MISMATCH:
                distributeEvent = TRUE;
                break;

            default:
                FM_LOG_WARNING(FM_LOG_CAT_EVENT_PORT,
                               "Received unknown event %d\n",
                               event->type);
                break;

        }   /* end switch (event->type) */

        if (distributeEvent)
        {
            if (switchIsProtected)
            {
                UNPROTECT_SWITCH(sw);
                switchIsProtected = FALSE;
            }

            if (eventHandler != NULL)
            {
                if (enableFramePriority && 
                    ( (event->type == FM_EVENT_PKT_RECV) ||
                      (event->type == FM_EVENT_SFLOW_PKT_RECV) ) )
                {
                    rcvPktEvent = &event->info.fpPktEvent;
                    err = fmFreeBufferQueueNode(sw, rcvPktEvent);
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Freeing Buffer queue node from the queue failed"
                                     "status = %d (%s) \n",
                                      err,
                                      fmErrorMsg(err));

                    }
                }

                eventHandler(event);
            }
            else
            {
                fmDistributeEvent(event);
            }
        }

FINISHED:

        fmReleaseEvent(event);

        /* release the switch lock if any is held */
        if (switchIsProtected)
        {
            UNPROTECT_SWITCH(sw);
        }

    }   /* end while (1) */

    fmExitThread(thread);

    return NULL;

}   /* end fmGlobalEventHandler */




/*****************************************************************************/
/** fmLocalEventHandler
 * \ingroup intSwitch
 *
 * \desc            event handler for local dispatch thread
 *
 * \param[in]       args points to the thread arguments
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void *fmLocalEventHandler(void *args)
{
    fm_thread *thread;
    fm_event * event;
    fm_status  status;

    /* grab arguments */
    thread = FM_GET_THREAD_HANDLE(args);

    while (1)
    {
        if (fmGetThreadEvent(thread, &event, FM_WAIT_FOREVER) != FM_OK)
        {
            if (localDispatchThreadExit == TRUE)
            {
                break;
            }
            else
            {
                continue;
            }
        }

        if (enableFramePriority &&
            ( (event->type == FM_EVENT_PKT_RECV) ||
              (event->type == FM_EVENT_SFLOW_PKT_RECV) ) )
        {
            status = fmFreeBufferQueueNode(FM_FIRST_FOCALPOINT, &event->info.fpPktEvent);
            if (status != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                             "Freeing Buffer queue node from the queue failed"
                             "status = %d (%s) \n",
                              status,
                              fmErrorMsg(status));

                fmReleaseEvent(event);
                continue;
            }
        }
        
        if (fmEventHandler != NULL)
        {
            fmEventHandler(event->type, event->sw, &event->info);
        }

        fmReleaseEvent(event);
    }

    fmExitThread(thread);

    return NULL;

}   /* end fmLocalEventHandler */




/*****************************************************************************/
/** fmSetProcessEventMask
 * \ingroup api
 *
 * \desc            Set which events are delivered to the current process's
 *                  event handler function (set by ''fmInitialize'' or
 *                  ''fmSetEventHandler'').
 *                                                                      \lb\lb
 *                  Multiple processes may subscribe to the same event. A
 *                  copy of each event will be sent to all processes that
 *                  subscribe to it.
 *
 * \param[in]       mask is a logical OR of ''Event Identifiers''.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetProcessEventMask(fm_uint32 mask)
{
    fm_status         err;
    fm_dlist_node *   node;
    fm_uint           count = 0;
    fm_localDelivery *delivery;
    fm_int            myProcessId;
    fm_uint           expectedCount;

    FM_LOG_ENTRY_API(FM_LOG_CAT_API, "mask=%u\n", mask);

    myProcessId = fmGetCurrentProcessId();

    err = fmCaptureLock(&fmRootApi->localDeliveryLock, FM_WAIT_FOREVER);

    if (err == FM_OK)
    {
        expectedCount = fmRootApi->localDeliveryCount;

        for ( node = FM_DLL_GET_FIRST( (&fmRootApi->localDeliveryThreads), head ) ;
             node != NULL ;
             node = FM_DLL_GET_NEXT(node, nextPtr) )
        {
            count++;
            delivery = (fm_localDelivery *) node->data;

            if (delivery->processId == myProcessId)
            {
                delivery->mask = mask;
            }
        }

        err = fmReleaseLock(&fmRootApi->localDeliveryLock);

        if (count != expectedCount)
        {
            FM_LOG_ERROR(FM_LOG_CAT_EVENT,
                         "Expected %d processes but found %d\n",
                         expectedCount,
                         count);
            err = FM_FAIL;
        }

    }   /* end if (err == FM_OK) */

    FM_LOG_EXIT_API(FM_LOG_CAT_API, err);

}   /* end fmSetProcessEventMask */




/*****************************************************************************/
/** fmRemoveEventHandler
 * \ingroup intApi
 *
 * \desc            Remove a local event handler from the list of handlers.
 *
 * \param[out]      delivery is a pointer to a pointer to a ''fm_localDelivery'' 
 *                  structure. If a delivery structure is found for the calling 
 *                  process then a pointer to it is returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if there is no delivery structure for
 *                  the calling process.
 *
 *****************************************************************************/
fm_status fmRemoveEventHandler(fm_localDelivery ** delivery)
{
    fm_status         err;
    fm_dlist_node *   node;
    fm_localDelivery *cur;
    fm_int            myProcessId;

    FM_LOG_ENTRY_API(FM_LOG_CAT_API, "delivery=%p\n", (void *) delivery);

    myProcessId = fmGetCurrentProcessId();

    err = fmCaptureLock(&fmRootApi->localDeliveryLock, FM_WAIT_FOREVER);

    if (err == FM_OK)
    {

        for ( node = FM_DLL_GET_FIRST( (&fmRootApi->localDeliveryThreads), head ) ;
             node != NULL ;
             node = FM_DLL_GET_NEXT(node, nextPtr) )
        {
            cur = (fm_localDelivery *) node->data;

            if (cur->processId == myProcessId)
            {
                break;
            }
        }

        if (node != NULL) 
        {
            fmDListRemove(&fmRootApi->localDeliveryThreads, node);
            *delivery = cur;
            fmRootApi->localDeliveryCount--;
        }
        else
        {
            err = FM_ERR_NOT_FOUND;
        }

        (void) fmReleaseLock(&fmRootApi->localDeliveryLock);


    }   /* end if (err == FM_OK) */

    FM_LOG_EXIT_API(FM_LOG_CAT_API, err);

}   /* end fmRemoveEventHandler */




/*****************************************************************************/
/** fmEventTypeToText
 * \ingroup intEvent
 *
 * \desc            Returns the textual representation of an event type.
 *
 * \param[in]       eventType is the event type.
 *
 * \return          Pointer to a string representing the event type.
 *
 *****************************************************************************/
const char * fmEventTypeToText(fm_int eventType)
{

    switch (eventType)
    {
        case FM_EVENT_SWITCH_INSERTED:
            return "SWITCH_INSERTED";

        case FM_EVENT_SWITCH_REMOVED:
            return "SWITCH_REMOVED";

        case FM_EVENT_SWITCH_UP:
            return "SWITCH_UP";

        case FM_EVENT_SWITCH_DOWN:
            return "SWITCH_DOWN";

        case FM_EVENT_TABLE_UPDATE:
            return "TABLE_UPDATE";

        case FM_EVENT_PKT_RECV:
            return "PKT_RECV";

        case FM_EVENT_PORT:
            return "PORT";

        case FM_EVENT_FRAME:
            return "FRAME";

        case FM_EVENT_SECURITY:
            return "SECURITY";

        case FM_EVENT_SOFTWARE:
            return "SOFTWARE";

        case FM_EVENT_SFLOW_PKT_RECV:
            return "SFLOW_PKT_RECV";

        case FM_EVENT_PARITY_ERROR:
            return "PARITY_ERROR";

        case FM_EVENT_FIBM_THRESHOLD:
            return "FIBM_THRESHOLD";

        case FM_EVENT_CRM:
            return "CRM";

        case FM_EVENT_ARP:
            return "ARP";

        case FM_EVENT_PURGE_SCAN_COMPLETE:
            return "PURGE_SCAN_COMPLETE";

        case FM_EVENT_EGRESS_TIMESTAMP:
            return "EGRESS_TIMESTAMP";

        case FM_EVENT_PLATFORM:
            return "PLATFORM";

        case FM_EVENT_LOGICAL_PORT:
            return "LOGICAL_PORT";

        default:
            return "UNKNOWN";

    }   /* end switch (eventType) */

}   /* end fmEventTypeToText */




/*****************************************************************************/
/** fmUpdateTypeToText
 * \ingroup intEvent
 *
 * \desc            Returns the textual representation of an MA table entry
 *                  update event type.
 *
 * \param[in]       updateType is the type of update.
 *
 * \return          Pointer to a string representing the update type.
 *
 *****************************************************************************/
const char * fmUpdateTypeToText(fm_int updateType)
{

    switch (updateType)
    {
        case FM_EVENT_ENTRY_EMPTY:
            return "EMPTY";

        case FM_EVENT_ENTRY_LEARNED:
            return "LEARNED";

        case FM_EVENT_ENTRY_AGED:
            return "AGED";

        case FM_EVENT_ENTRY_MEMORY_ERROR:
            return "MEMORY_ERROR";

        default:
            return "UNKNOWN";

    }   /* end switch (updateType) */

}   /* end fmUpdateTypeToText */

