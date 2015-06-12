/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_alos_time_int.h
 * Creation Date:   November 26, 2013
 * Description:     Internal declarations related to the ALOS time module
 *
 * Copyright (c) 2007 - 2013, Intel Corporation
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

#ifndef __FM_FM_ALOS_TIME_INT_H
#define __FM_FM_ALOS_TIME_INT_H

/* forward declarations of the timer and timer task data strucure types */
typedef struct _fm_timer fm_timer;
typedef struct _fm_timerTask fm_timerTask;


/* definition of the internal timer structure */
struct _fm_timer
{
    /* magic number */
    fm_uint32      magicNumber;

    /* timer name */
    fm_text        name;

    /* pointer to the associated task */
    fm_timerTask   *task;

    /* boolean indicating if this timer is currently running */
    fm_bool        running;

    /* duration of a single repetition */
    fm_timestamp   timeout;

    /* when the current repetition started (absolute time) */
    fm_timestamp   start;

    /* when this timer is expected to expire next (absolute time) */
    fm_timestamp   end;

    /* total number of repetitions */
    fm_int         nrRepetitions;

    /* number of repetitions so far */
    fm_int         nrRepetitionsSoFar;

    /* callback funciton */
    fm_timerCallback callback;

    /* pointer to the callback argument */
    void             *arg;

    /* existing timers linked list node */
    FM_DLL_DEFINE_NODE( _fm_timer, nextTimer, prevTimer );

    /* active timers linked list node */
    FM_DLL_DEFINE_NODE( _fm_timer, nextActiveTimer, prevActiveTimer );

};



/* definition of the internal timer task control structure */
struct _fm_timerTask
{
    /* wheter or not this entry is being used */
    fm_bool          used;

    /* the timer task is initialized */
    fm_bool          initialized;

    /* timer task mode */
    fm_timerTaskMode mode;

    /* pointer to the associated thread */
    fm_thread        *thread;

    /* resolution for this timer thread (if mode is periodic) */
    fm_timestamp     period;

    /* lock for access resolution */
    fm_lock          lock;

    /* condition suspend/wakeup when the timer task is event-driven */
    void             *cond;

    /* sequencing semaphore */
    fm_semaphore     sem;

    fm_timestamp     *nextTimeout;

    /* linked list of existing timers */
    FM_DLL_DEFINE_LIST( _fm_timer, firstTimer, lastTimer );

    /* linked list of active timers */
    FM_DLL_DEFINE_LIST( _fm_timer, firstActiveTimer, lastActiveTimer );

};


#endif /* __FM_FM_ALOS_TIME_INT_H */
