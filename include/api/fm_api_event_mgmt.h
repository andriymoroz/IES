/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_mgmt.h
 * Creation Date:   May 15, 2007
 * Description:     Functions for dealing with events
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

#ifndef __FM_FM_API_EVENT_MGMT_H
#define __FM_FM_API_EVENT_MGMT_H

typedef enum
{
    /* Notification is done in strict order with this first */
    EVENT_FREE_NOTIFY_PKT_INTR,
    EVENT_FREE_NOTIFY_LINK_TRANSITION,

    /* Add additional notification above here */
    MAX_EVENT_FREE_NOTIFY_HANDLER,
} fm_eventFreeNotify;

typedef void (*fm_eventFreeNotifyHndlr)(fm_int sw);

/* allocate an event buffer */
fm_event *fmAllocateEvent(fm_int           sw,
                          fm_eventID       eventID,
                          fm_int           eventType,
                          fm_eventPriority priority);


/* release an event buffer */
fm_status fmReleaseEvent(fm_event *event);

fm_status fmAddEventFreeNotify(fm_int sw,
                               fm_eventFreeNotify type,
                               fm_eventFreeNotifyHndlr handler);

/* initialize event handling subsystem */
fm_status fmEventHandlingInitialize(void);


/* prototypes for the generic event tasks */
void *fmDebounceLinkStateTask(void *args);
void *fmInterruptHandler(void *args);
fm_status fmSendSoftwareEvent(fm_int sw, fm_uint32 events);


#endif /* __FM_FM_API_EVENT_MGMT_H */
