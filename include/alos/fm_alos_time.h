/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_time.h
 * Creation Date:   2005
 * Description:     Timestamp functions
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#ifndef __FM_FM_ALOS_TIME_H
#define __FM_FM_ALOS_TIME_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/** A legacy synonym for ''fmDelayBy''.
 *  \ingroup macroSynonym */
#define fmDelay(secs,nsecs) fmDelayBy((secs), (nsecs))


/**************************************************/
/** \ingroup typeStruct
 * Structure used to hold a timestamp value.
 **************************************************/
typedef struct _fm_timestamp
{
    /** Number of whole seconds. */
    fm_uint64 sec;

    /** Number of microseconds in addition to sec. */
    fm_uint64 usec;

} fm_timestamp;


/**************************************************/
/** \ingroup intTypeEnum
 *  Timer task modes, used by fmCreateTimerTask.
 **************************************************/
typedef enum
{
    /** Periodic polling mode. */
    FM_TIMER_TASK_MODE_PERIODIC = 0,

    /** Timer task suspends until next timer event. */
    FM_TIMER_TASK_MODE_EVENT_DRIVEN,

    /** UNPUBLISHED: For internal use only. */
    FM_TIMER_TASK_MODE_MAX

} fm_timerTaskMode;


/* constants used by mutual exclusion function calls */

#define FM_WAIT_FOREVER  (NULL)
#define FM_NO_WAIT       &fmNoWaitTimeConstant;

/* timer type definitions */
typedef void *fm_timerHandle;
typedef void (*fm_timerCallback)( void *arg );

/* Constant representing an infinite number of timer occurrences */ 
#define FM_TIMER_REPEAT_FOREVER  -1


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

extern const fm_timestamp fmNoWaitTimeConstant;


/*****************************************************************************
 * Public Function Prototypes
 *****************************************************************************/

/* returns: -1 if t1 < t2
 *           0 if t1 = t2
 *           1 if t1 > t2
 */
fm_int fmCompareTimestamps(fm_timestamp *t1, fm_timestamp *t2);


/* performs t1 = t1 + t2 */
void fmAddTimestamps(fm_timestamp *t1, fm_timestamp *t2);


/* performs t3 = t1 - t2 */
void fmSubTimestamps(const fm_timestamp *t1,
                     const fm_timestamp *t2,
                     fm_timestamp *      t3);

fm_status fmGetTime(fm_timestamp *tvp);

fm_status fmGetTimeRes(fm_timestamp *tr);

/* delay for some time */
fm_status fmDelayBy( fm_int sec, fm_int nsec );
fm_status fmDelayUntil( fm_int sec, fm_int nsec );


/* get a formatted date string */
fm_status fmGetFormattedTime(char *dateStr);


/* Convert a timestamp into system ticks */
fm_uint64 fmConvertTimestampToTicks(fm_timestamp *tstamp);


/* Function to create a timer task */
fm_status fmCreateTimerTask( fm_text           taskName, 
                             fm_timerTaskMode  mode,
                             fm_timestamp     *period,
                             void             *thread );

/* function to delete a timer task */
fm_status fmDeleteTimerTask( void  *thread );

/* create a named timer */
fm_status fmCreateTimer( fm_text         timerName, 
                         void           *thread, 
                         fm_timerHandle *handlePtr );

/* delete a timer */
fm_status fmDeleteTimer( fm_timerHandle handle );

/* start a timer indicating timeout, number of repetitions and callback */
fm_status fmStartTimer( fm_timerHandle    handle, 
                        fm_timestamp     *timeout,
                        fm_int            nrRepetitions,
                        fm_timerCallback  timerCallback,
                        void             *callbackArg ); 

/* stop a running timer */
fm_status fmStopTimer( fm_timerHandle handle );

/* get the current timer parameters */
fm_status fmGetTimerParameters( fm_timerHandle   handle,
                                fm_text         *timerName,
                                void           **thread );

/* get the current timer state */
fm_status fmGetTimerState( fm_timerHandle  handle,
                           fm_bool        *isTimerRunning,
                           fm_timestamp   *elapsedTime,
                           fm_int         *nrRepetitionsSoFar,
                           fm_timestamp   *timeToGo,
                           fm_int         *nrRepetitionsToGo );

/* debug function to dump all existing timers */
fm_status fmDbgDumpTimers( fm_bool onlyActive );

/* debug function to dump all existing timers */
fm_status fmDbgDumpActiveTimerList( void );


#endif /* __FM_FM_ALOS_TIME_H */
