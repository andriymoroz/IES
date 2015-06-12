/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_events_int.h
 * Creation Date:   2005
 * Description:     Contains non-exposed functions for event handling threads
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

#ifndef __FM_FM_API_EVENTS_INT_H
#define __FM_FM_API_EVENTS_INT_H

typedef struct _fm_localDelivery
{
    /* each bit represents an event type-- set means deliver to this thread */
    fm_uint32  mask;

    /* thread that delivers events to its process */
    fm_thread *thread;

    /* the ID of the process that this thread is for */
    fm_int     processId;

} fm_localDelivery;


/* event handler for dispatching events to the stack */
void *fmGlobalEventHandler(void *args);


/* event handler for dispatching events to a particular process */
void *fmLocalEventHandler(void *args);

/* Receive Packet Thread */
void *fmReceivePacketTask(void *args);

void *fmFastMaintenanceTask(void *args);

const char * fmEventTypeToText(fm_int eventType);
const char * fmUpdateTypeToText(fm_int updateType);

/* Distribute Events to interested processes */
void fmDistributeEvent(fm_event *event);

/* Remove event handler */
fm_status fmRemoveEventHandler(fm_localDelivery ** delivery);

/* Switch-Specific Event Handler */
typedef void (*fm_switchEventHandler)(fm_event *event);


/* gets the upper layer event handler for a specific switch.  Returns
 * NULL if the switch is using the global event handler */
fm_status fmGetSwitchEventHandler(fm_int                 sw,
                                  fm_switchEventHandler *eventHandlerFuncPtr);


/* sets the upper layer event handler for a specific switch */
fm_status fmSetSwitchEventHandler(fm_int                sw,
                                  fm_switchEventHandler eventHandlerFunc);


extern fm_bool localDispatchThreadExit;

#endif /* __FM_FM_API_EVENTS_INT_H */
