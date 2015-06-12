/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_mgmt.c
 * Creation Date:   May 4, 2007
 * Description:     Functions to manage event handling.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

static fm_timestamp eventTimeout;
static fm_int       blockThreshold;
static fm_int       unblockThreshold;


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************
 * fmEventHandlingInitialize
 *
 * Description: Initializes the event handling system
 *
 * Returns:     FM_OK if successful
 *              FM_FAIL if general failure to initialize driver.
 *              FM_ERR_NO_MEM if unable to allocate memory for device.
 *              FM_ERR_OPENING_INFO_DEVICE_NODE if unable to count switches
 *              FM_ERR_BAD_IOCTL if ioctl operation failed
 *              FM_ERR_OPENING_DEVICE_NODE if unable to initialize device
 *              Other error code (see fm_errno.h)
 *
 *****************************************************************************/
fm_status fmEventHandlingInitialize(void)
{
    fm_event *ptr;
    fm_status err;
    int       i;
    fm_int    timeout;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_EVENT);

    /* Initialize semaphore used for low priority events */
    err = fmCreateSemaphore("low priority event sem",
                            FM_SEM_BINARY,
                            &fmRootApi->fmLowPriorityEventSem,
                            1);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT, err);

    err = fmEventQueueInitialize(&fmRootApi->fmEventFreeQueue, FM_MAX_EVENTS,
                                 "fmEventFreeQueue");
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT, err);

    /* put all event buffers into the free queue */
    for (i = 0 ; i < FM_MAX_EVENTS ; i++)
    {
        /***************************************************
         * We allocate the space for the maximum table update
         * burst at the end of each event.
         **************************************************/
        ptr = (fm_event *) fmAlloc( sizeof(fm_event) +
                                   (sizeof(fm_eventTableUpdate) *
                                    FM_TABLE_UPDATE_BURST_SIZE) );
        if (ptr == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_ERR_NO_MEM);
        }

        err = fmEventQueueAdd(&fmRootApi->fmEventFreeQueue, ptr);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT, err);
    }

    /* Initialize variables */
    timeout           = fmGetIntApiProperty(FM_AAK_API_EVENT_SEM_TIMEOUT,
                                            FM_AAD_API_EVENT_SEM_TIMEOUT);
    eventTimeout.sec  = timeout/1000;
    eventTimeout.usec = (timeout%1000)*1000;
     
    blockThreshold    = fmGetIntApiProperty(FM_AAK_API_FREE_EVENT_BLOCK_THRESHOLD,
                                            FM_AAD_API_FREE_EVENT_BLOCK_THRESHOLD);
    unblockThreshold  = fmGetIntApiProperty(FM_AAK_API_FREE_EVENT_UNBLOCK_THRESHOLD,
                                            FM_AAD_API_FREE_EVENT_UNBLOCK_THRESHOLD);

    if (blockThreshold > unblockThreshold || 
        blockThreshold >= FM_MAX_EVENTS ||
        unblockThreshold >= FM_MAX_EVENTS)
    {
        /* Should not be able get this */
        FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_ERR_INVALID_ATTRIB);
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_OK);

}   /* end fmEventHandlingInitialize */




/*****************************************************************************
 * fmAllocateEvent
 *
 * Description: Gets an event from the event free queue.
 *
 * Arguments:   sw is the switch number.
 *
 *              eventID is the event ID to be stored to the allocated
 *              event.
 *
 *              eventType is the event type to be stored to the allocated
 *              event.
 *
 * Returns:     Pointer to allocated event or NULL if allocation failed
 *              or waiting on semaphore timeouts.
 *
 *****************************************************************************/
fm_event *fmAllocateEvent(fm_int           sw,
                          fm_eventID       eventID,
                          fm_int           eventType,
                          fm_eventPriority priority)
{
    fm_event *event;
    fm_status err;
    fm_int    eventCount;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT,
                 "sw = %d, eventID = %d, eventType = %d, priority = %d\n",
                 sw,
                 eventID,
                 eventType,
                 priority);

    if (priority == FM_EVENT_PRIORITY_LOW)
    {
        err = fmEventQueueCount(&fmRootApi->fmEventFreeQueue, &eventCount);
        if (err != FM_OK)
        {
            FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_EVENT, NULL, "NULL\n");
        }

        if (eventCount < blockThreshold)
        {
            /* block until the number of events is reasonable */
            err = fmCaptureSemaphore(&fmRootApi->fmLowPriorityEventSem, &eventTimeout);
            if (err != FM_OK)
            {
                /* Timeout, then return NULL */
                return NULL;
            }
        }
    }

    err = fmEventQueueGet(&fmRootApi->fmEventFreeQueue, &event);

    if (err != FM_OK)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_EVENT, NULL, "NULL\n");
    }
    else
    {
        event->sw       = sw;
        event->eventID  = eventID;
        event->type     = eventType;
        event->priority = priority;

        if (eventType == FM_EVENT_TABLE_UPDATE)
        {
            /***************************************************
             * Remember that the updates field is a pointer,
             * but we have allocated each event block to have
             * enough free space after the event structure to
             * hold the maximum number of events.
             **************************************************/

            event->info.fpUpdateEvent.updates =
                (fm_eventTableUpdate *) (event + 1);
        }

        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_EVENT, event, "%p\n", (void *) event);
    }

}   /* end fmAllocateEvent */




/*****************************************************************************
 * fmReleaseEvent
 *
 * Description: Puts an event back onto the event free queue.
 *
 * Arguments:   event points to the event to be freed.
 *
 * Returns:     None.
 *
 *****************************************************************************/
fm_status fmReleaseEvent(fm_event *event)
{
    fm_switch *      switchState;
    fm_int           switchNum;
    fm_int           type;
    fm_eventFreeNotifyHndlr handler;
    fm_int           eventCount;


    FM_LOG_ENTRY(FM_LOG_CAT_EVENT, "%p\n", (void *) event);

    fmEventQueueAdd(&fmRootApi->fmEventFreeQueue, event);

    /* Need to unblock regardless of priority */
    eventCount = 0; /* incase call return error */
    fmEventQueueCount(&fmRootApi->fmEventFreeQueue, &eventCount);

    /* Only release when free event above a threshold */
    if (eventCount > unblockThreshold)
    {
        fmReleaseSemaphore(&fmRootApi->fmLowPriorityEventSem);
    }

    /**************************************************
     * If any switch's frame receiver could previously
     * not get an event, then signal it to try again
     * now that there is one available.
     **************************************************/

    for (switchNum = FM_FIRST_FOCALPOINT ;
         switchNum <= FM_LAST_FOCALPOINT ;
         switchNum++)
    {
        switchState = fmRootApi->fmSwitchStateTable[switchNum];

        if (switchState && switchState->state == FM_SWITCH_STATE_UP)
        {
            for (type = 0 ; type < MAX_EVENT_FREE_NOTIFY_HANDLER ; type ++)
            {
                handler = switchState->eventFreeNotifyHndlr[type];
                if (switchState->eventFreeNotifyHndlr[type])
                {
                    switchState->eventFreeNotifyHndlr[type] = 0;
                    handler(switchNum);
                    /* only notify one at a time */
                    FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_OK);
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_OK);

}   /* end fmReleaseEvent */



/*****************************************************************************
 * fmAddEventFreeNotify
 *
 * Description: Add a notify handler when an event is free.
 *
 * Arguments:   sw is the switch number.
 *
 *              type is the type of different notifications.
 *
 *              handler is the function pointer to call when a event is freed.
 *
 * Returns:     FM_OK.
 *
 *****************************************************************************/
fm_status fmAddEventFreeNotify(fm_int sw,
                               fm_eventFreeNotify type,
                               fm_eventFreeNotifyHndlr handler)
{
    fm_switch *  switchPtr;

    /* Don't print out handler since can't cast properly on 32- and 64-bit systems */
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT, "sw: %d type: %d\n", sw, type);

    switchPtr = fmRootApi->fmSwitchStateTable[sw];

    if (switchPtr)
    {
        if (switchPtr->eventFreeNotifyHndlr[type])
        {
            /* The application should not add a redundant event handler */
            FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_ERR_ALREADY_EXISTS);
        }
        switchPtr->eventFreeNotifyHndlr[type] = handler;
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT, FM_OK);
}

