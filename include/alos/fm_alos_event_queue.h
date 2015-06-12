/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_event_queue.h
 * Creation Date:   2005
 * Description:     Defines an event structure and a wrapper to hold a
 *                  queue of events
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_ALOS_EVENT_QUEUE_H
#define __FM_FM_ALOS_EVENT_QUEUE_H


/**************************************************/
/** \ingroup intTypeStruct
 *
 *  Encapsulates a thread-safe event queue.
 **************************************************/
typedef struct _fm_eventQueue
{
    /** The heart of the queue is a doubly-linked list */
    fm_dlist eventQueue;

    /** lock for both queue read and write access */
    fm_lock  accessLock;

    /** size for convenience */
    fm_int   size;

    /** total number of allowed events */
    fm_int   max;

    /** event queue name, for debugging */
    fm_text  name;

    /** Other debug variables. */
    fm_uint  totalEventsPosted;
    fm_uint  totalEventsPopped;
    fm_int   maxSize; 
    
    /** Event propogation timings. */
    fm_float avgTime;
    fm_float minTime;
    fm_float maxTime;

} fm_eventQueue;


/* (non-blocking) initializes the queue, should be only done once */
fm_status fmEventQueueInitialize(fm_eventQueue *q, int maxSize, fm_text qName);


/* (blocking) enqueue an event at current + timeDelta time */
fm_status fmEventQueueAdd(fm_eventQueue *q, fm_event *event);


/* (blocking) get the next event whose timestamp has expired */
fm_status fmEventQueueGet(fm_eventQueue *q, fm_event **eventPtr);


/* (non-blocking) peek the next event */
fm_status fmEventQueuePeek(fm_eventQueue *q, fm_event **eventPtr);


/* (non-blocking) cleans up the queue */
fm_status fmEventQueueDestroy(fm_eventQueue *q);


/* (non-blocking) returns the number of entries currently in the queue */
fm_status fmEventQueueCount(fm_eventQueue *q, fm_int *eventCount);

/* (blocking) remove event node from the queue */
fm_status fmEventQueueRemove(fm_eventQueue *q,
                             fm_event *eventPtr);


#endif /* __FM_FM_ALOS_EVENT_QUEUE_H */
