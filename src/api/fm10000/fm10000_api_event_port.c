/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_event_port.c
 * Creation Date:   May 13, 2013
 * Description:     Port related event handling
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


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
fm_uint64 portStateEventNotifications = 0;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static void EventFreeHandler(fm_int sw)
{
    fm_port *  portPtr;
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     cpi;
    fm_int     physPort;
    fm_int     port;
    fm_int     mac;
    fm_bool    pushedFreeEventNotify;

    switchPtr = GET_SWITCH_PTR(sw);

    for ( cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++ )
    {
        fmMapCardinalPortInternal(switchPtr, cpi, &port, &physPort);

        portPtr = GET_PORT_PTR(sw, port);

        /* On FM10000 port mac is not used (not configurable), set to 0. */
        mac = 0;

        /* try to send event for ports with pending notifications */
        if ( portPtr->waitForFreeEventBuffer ) 
        {
            /* Clear wait for free event's buffer indication flag */
            portPtr->waitForFreeEventBuffer = FALSE;

            /* Do not send event notifications when no link state change 
               is reported - the last event sent was for the same link state 
               as now */
            if( portPtr->linkUp != 
                   (fm_bool)( (portStateEventNotifications >> physPort) & 1 ))
            {
                status = switchPtr->SendLinkUpDownEventV2(sw,
                                                          physPort,
                                                          mac,
                                                          portPtr->linkUp,
                                                          FM_EVENT_PRIORITY_LOW,
                                                          &pushedFreeEventNotify);

                if (status != FM_OK)
                {
                    FM_LOG_FATAL(FM_LOG_CAT_EVENT_PORT,
                                 "Can't send link event for port %d\n",
                                 port);
                }

                if (pushedFreeEventNotify)
                {
                    /* There are not free buffers */
                    break;
                }
            }
        }
    }

}   /* end EventFreeHandler */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000SendLinkUpDownEvent
 * \ingroup intDriver
 *
 * \desc            Function to inject a link up or link down event into the
 *                  event queue.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       physPort is the physical port number for the port.
 *
 * \param[in]       mac is the MAC number for the port (Not used).
 *
 * \param[in]       linkUp is TRUE if a link-up event is to be injected,
 *                  FALSE to inject a link-down event.
 *
 * \param[in]       priority is the priority to send the event at
 *
 * \return          None.
 *
 *****************************************************************************/
fm_status fm10000SendLinkUpDownEvent(fm_int           sw,
                                     fm_int           physPort,
                                     fm_int           mac,
                                     fm_bool          linkUp,
                                     fm_eventPriority priority)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PORT,
                 "sw=%d physPort=%d mac=%d linkUp=%d priority=%d\n",
                 sw, physPort, mac, linkUp, priority);

    err = fm10000SendLinkUpDownEventV2(sw,physPort,mac,linkUp,priority,NULL);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PORT, err);

}   /* end fm10000SendLinkUpDownEvent */




/*****************************************************************************/
/** fm10000SendLinkUpDownEventV2
 * \ingroup intDriver
 *
 * \desc            Function to inject a link up or link down event into the
 *                  event queue.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       physPort is the physical port number for the port.
 *
 * \param[in]       mac is the MAC number for the port (Not used).
 *
 * \param[in]       linkUp is TRUE if a link-up event is to be injected,
 *                  FALSE to inject a link-down event.
 *
 * \param[in]       priority is the priority to send the event at
 * 
 * \param[in]       pAddedFreeEvent points to the caller-allocated storage that
 *                  this function will set to TRUE if there was no available
 *                  and an event free notify has been pushed into the event
 *                  queue.
 *
 * \return          None.
 *
 *****************************************************************************/
fm_status fm10000SendLinkUpDownEventV2(fm_int           sw,
                                       fm_int           physPort,
                                       fm_int           mac,
                                       fm_bool          linkUp,
                                       fm_eventPriority priority,
                                       fm_bool *        pAddedFreeEvent)
{
    fm_status     err;
    fm_event *    event;
    fm_eventPort *portEvent;
    fm_int        logPort;
    fm_switch *   switchPtr;
    fm_port *       portEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PORT,
                 "sw=%d physPort=%d mac=%d linkUp=%d priority=%d pAddedFreeEvent=%p\n",
                 sw, physPort, mac, linkUp, priority, (void*)pAddedFreeEvent);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    if (pAddedFreeEvent)
    {
        *pAddedFreeEvent = FALSE;
    }

    fmMapPhysicalPortToLogical(switchPtr, physPort, &logPort);

    if (!linkUp && fmFibmSlaveIsLogicalPortMgmt(sw, logPort))
    {
        /* If we get here, then the connection to the fibm
         * switch is dead. Just don't report the link down
         * in case it is intermittent, and let the user
         * know to debug what the problem is.
         */
        FM_LOG_FATAL(FM_LOG_CAT_EVENT_PORT,
                "Switch %d fibm mgmt port %d is going down\n", sw, logPort);
        return FM_FAIL;
    }

    event = fmAllocateEvent(sw,
                            FM_EVID_HIGH_PORT,
                            FM_EVENT_PORT,
                            priority);

    /* Allocate event can be blocked for low priority or return NULL.
     * Handle NULL properly
     */
    if (event == NULL)
    {
        portEntry = GET_PORT_PTR(sw, logPort);

        /* Make sure that no free notification was not added */
        if(portEntry->waitForFreeEventBuffer != TRUE)
        {
            portEntry->waitForFreeEventBuffer = TRUE;
            /* Will get notify when a free event is available, skip returning  
               error when trying to add redundant event handler */
            fmAddEventFreeNotify(sw, 
                                 EVENT_FREE_NOTIFY_LINK_TRANSITION, 
                                 EventFreeHandler);

            fmDbgDiagCountIncr(sw, FM_CTR_LINK_CHANGE_OUT_OF_EVENTS, 1);

            if (pAddedFreeEvent)
            {
                *pAddedFreeEvent = TRUE;
            }
        }
        /* Return error - sending link state change event failed */
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PORT, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    fmDbgDiagCountIncr(sw, FM_CTR_LINK_CHANGE_EVENT, 1);

    portEvent = &event->info.fpPortEvent;
    FM_CLEAR(*portEvent);

    portEvent->port      = logPort;
    portEvent->mac       = mac;
    portEvent->activeMac = TRUE;

    if (linkUp)
    {
        portEvent->linkStatus = FM_PORT_STATUS_LINK_UP;
    }
    else
    {
        portEvent->linkStatus = FM_PORT_STATUS_LINK_DOWN;
    }

    err = fmSendThreadEvent(&fmRootApi->eventThread, event);

    if (err != FM_OK)
    {
        /* Free the event since we could not send it to thread */
        fmReleaseEvent(event);

        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PORT, err);
    }
    else
    {
        /* Set portStateEventNotifications */
        portStateEventNotifications = 
           ( portStateEventNotifications & ~(FM_LITERAL_U64(1) << physPort) ) | 
           ( ( linkUp & FM_LITERAL_U64(1) ) << physPort );
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PORT, FM_OK);

}   /* end fm10000SendLinkUpDownEventV2 */




/*****************************************************************************
 * fm10000LinkStateDebounceThread
 *
 * Description: Thread for debouncing link state changes. Does not return.
 *
 * Arguments:   args pointer to thread argument array
 *
 * Returns:     None.
 *
 *****************************************************************************/
void fm10000DebounceLinkStates(fm_int     sw,
                              fm_thread *thread,
                              fm_thread *handlerThread)
{
#if 0
    fm_status       err;
    fm_int          cpi;
    fm_int          port;
    fm_port *       portEntry;
    fm_timestamp    currentTime;
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_portAttr *   portAttr;
#endif

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_EVENT_PORT,
                         "sw=%d thread=%p:<%s> handlerThread=%p:<%s>\n",
                         sw,
                         (void *) thread,
                         thread->name,
                         (void *) handlerThread,
                         handlerThread->name);

    FM_NOT_USED(sw);
    FM_NOT_USED(thread);
    FM_NOT_USED(handlerThread);

#if 0
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* get current timestamp, don't need really accurate time per port
     * so move this out of the for loop
     */
    err = fmGetTime(&currentTime);
    if (err != FM_OK)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PORT, "fmGetTime failed\n");
        FM_LOG_EXIT_VOID_VERBOSE(FM_LOG_CAT_EVENT_PORT);
    }

    for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port      = GET_LOGICAL_PORT(sw, cpi);
        portEntry = GET_PORT_PTR(sw, port);
        portAttr  = GET_PORT_ATTR(sw, port);

    }   /* end for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++) */
#endif

    FM_LOG_EXIT_VOID_VERBOSE(FM_LOG_CAT_EVENT_PORT);

}   /* end fm10000DebounceLinkStates */

