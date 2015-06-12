/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_event_queue.c
 * Creation Date:   September 16, 2005
 * Description:     Linux-native specific functions for dealing with event queues
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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
/** fmEventQueueInitialize
 * \ingroup intAlosEvent
 *
 * \desc            (non-blocking) initializes the queue
 *
 * \note            should be only done once
 *
 * \param[in]       q is the pointer to the event queue to initialize
 *
 * \param[in]       maxSize is the maximum number of events in the queue
 *
 * \param[in]       qName is the name of the queue, for debugging purposes
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueueInitialize(fm_eventQueue *q, int maxSize, fm_text qName)
{
    fm_status err;
    fm_bool   lockInit;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS,
                 "queue=%p maxSize=%d name=%s\n",
                 (void *) q, maxSize, qName);

    if (q == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(*q);

    lockInit = FALSE;

    fmDListInit(&q->eventQueue);

    err = fmCreateLock(qName, &q->accessLock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);
    lockInit = TRUE;

    q->max  = maxSize;
    q->name = fmStringDuplicate(qName);

    if (q->name == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    /* Notify debug system. */
    fmDbgEventQueueCreated(q);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);

ABORT:

    if (q->name)
    {
        fmFree(q->name);
    }

    if (lockInit)
    {
        fmDeleteLock(&q->accessLock);
    }

    FM_CLEAR(*q);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);

}   /* end fmEventQueueInitialize */




/*****************************************************************************/
/** fmEventQueueAdd
 * \ingroup intAlosEvent
 *
 * \desc            (blocking) enqueue an event
 *
 * \param[in]       q is the pointer to the event queue
 *
 * \param[in]       event is the pointer to the event to be added to the queue
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueueAdd(fm_eventQueue *q, fm_event *event)
{
    fm_status      err, rerr = FM_OK;
    fm_dlist_node *eventNode; 

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "queue=%p event=%p\n",
                 (void *) q, (void *) event);

    if (q->size == q->max)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_EVENT_QUEUE_FULL);
    }

    /* Timeout not implememented, so we block forever if we deadlock */
    if ( ( err = fmCaptureLock(&q->accessLock, FM_WAIT_FOREVER) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

#ifdef ENABLE_EVENTQ_TIMESTAMP
    /* Don't enable by default, slow down packet delivery */
    if (fmGetTime(&event->postedTimestamp) != 0)
    {
        rerr = FM_ERR_BAD_GETTIME;
    }
    else
#endif
    {
        if ( ( err = fmDListInsertEndV2(&q->eventQueue, 
                                        event, 
                                        &eventNode) ) == FM_OK )
        {
            q->totalEventsPosted++;
            q->size++;
            q->maxSize = q->size > q->maxSize ? q->size : q->maxSize; 

            event->q = q;
            event->node = eventNode; 
        }
        else
        {
            rerr = err;
        }
    }

    if ( ( err = fmReleaseLock(&q->accessLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, rerr);

}   /* end fmEventQueueAdd */




/*****************************************************************************/
/** fmEventQueueGet
 * \ingroup intAlosEvent
 *
 * \desc            (blocking) get the next event
 *
 * \param[in]       q is the pointer to the event queue
 *
 * \param[out]      eventPtr is a pointer to storage for the event pointer
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueueGet(fm_eventQueue *q, fm_event **eventPtr)
{
    fm_status err;
    fm_event *ev;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "queue=%p event=%p\n",
                 (void *) q, (void *) eventPtr);

    if (eventPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_INVALID_ARGUMENT);
    }

    /* Timeout not implememented, so we block forever if we deadlock */
    if ( ( err = fmCaptureLock(&q->accessLock, FM_WAIT_FOREVER) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

    ev = (fm_event *) fmDListRemove(&q->eventQueue, q->eventQueue.head);

    if (ev)
    {
        /* record the time of its removal before notifying debug. */
#ifdef ENABLE_EVENTQ_TIMESTAMP
        fmGetTime(&ev->poppedTimestamp);
#endif

        *eventPtr = ev;
        q->size--;
#ifdef ENABLE_EVENTQ_TIMESTAMP
        fmDbgEventQueueEventPopped(q, ev);
#endif

        ev->q    = NULL;
        ev->node = NULL;
    }
    else
    {
        *eventPtr = NULL;

        /**************************************************
         * The event queue is empty, this an error condiion 
         * Release the event queue access lock then increase 
         * the appropriate diagnostic counter, which requires 
         * the debug lock to be taken 
         ***************************************************/

        err = fmReleaseLock(&q->accessLock);

        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_NO_EVENTS_AVAILABLE, 1);

        if ( err != FM_OK )
        {
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
        }

        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    if ( ( err = fmReleaseLock(&q->accessLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);

}   /* end fmEventQueueGet */




/*****************************************************************************/
/** fmEventQueuePeek
 * \ingroup intAlosEvent
 *
 * \desc            peek the next event
 *
 * \param[in]       q is the pointer to the event queue
 *
 * \param[out]      eventPtr is a pointer to storage for the event pointer
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueuePeek(fm_eventQueue *q, fm_event **eventPtr)
{
    fm_status err;
    fm_event *ev;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "queue=%p event=%p\n",
                 (void *) q, (void *) eventPtr);

    if (eventPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_INVALID_ARGUMENT);
    }

    /* Timeout not implememented, so we block forever if we deadlock */
    if ( ( err = fmCaptureLock(&q->accessLock, FM_WAIT_FOREVER) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

    if (q->eventQueue.head)
    {
        ev        = (fm_event *) q->eventQueue.head->data;
        *eventPtr = ev;
    }
    else
    {
        *eventPtr = NULL;

        if ( ( err = fmReleaseLock(&q->accessLock) ) != FM_OK )
        {
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
        }

        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    if ( ( err = fmReleaseLock(&q->accessLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);

}   /* end fmEventQueuePeek */




/*****************************************************************************/
/** fmEventQueueDestroy
 * \ingroup intAlosEvent
 *
 * \desc            (non-blocking) cleans up the queue
 *
 * \param[in]       q is the pointer to the event queue
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueueDestroy(fm_eventQueue *q)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "queue=%p\n", (void *) q);

    /* Notify debug system first */
    fmDbgEventQueueDestroyed(q);

    if ( ( err = fmDeleteLock(&q->accessLock) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }

    fmFree(q->name);
    q->name = NULL;

    fmDListFree(&q->eventQueue);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);

}   /* end fmEventQueueDestroy */




/*****************************************************************************/
/** fmEventQueueCount
 * \ingroup intAlosEvent
 *
 * \desc            (non-blocking) returns the number of entries currently
 *                  in the queue
 *
 * \param[in]       q is the pointer to the event queue
 *
 * \param[in]       eventCount points to where the count should be stored
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueueCount(fm_eventQueue *q, fm_int *eventCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "queue=%p count=%p\n",
                 (void *) q, (void *) eventCount);

    *eventCount = q->size;

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);

}   /* end fmEventQueueCount */




/*****************************************************************************/
/** fmEventQueueRemove
 * \ingroup intAlosEvent
 *
 * \desc            (blocking) Remove the event from the queue
 *
 * \param[in]       q is the pointer to the event queue
 *
 * \param[in]       eventPtr is the pointer to event to be removed.
 *
 * \return          Status code
 *
 *****************************************************************************/
fm_status fmEventQueueRemove(fm_eventQueue *q, 
                             fm_event *eventPtr)
{
    fm_status      err;
    fm_event      *ev;
    fm_dlist_node *eventNode;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "queue=%p eventPtr=%p\n",
                 (void *) q, (void *)eventPtr);

    if (eventPtr == NULL || q == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_INVALID_ARGUMENT);
    }


    if ( ( err = fmCaptureLock(&q->accessLock, FM_WAIT_FOREVER) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);
    }
    
    eventNode = eventPtr->node;
    
    if (eventNode == NULL || eventPtr->q != q)
    {
        err = FM_ERR_NO_MORE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS, err);
    } 

    ev = (fm_event *) fmDListRemove(&q->eventQueue, eventNode);
    if (ev)
    {
        q->size--;
        ev->node = NULL;
        ev->q    = NULL;
    }
    else
    {
        err = FM_ERR_NO_EVENTS_AVAILABLE; 
    }

ABORT:
    if ( fmReleaseLock(&q->accessLock) != FM_OK )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Releasing event queue lock failed\n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, err);

}   /* end fmEventQueueRemove */
