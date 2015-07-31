/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_time.c
 * Creation Date:   2005
 * Description:     Timestamp wrapping and comparison functions
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define NANOSECS_PER_SECOND     1000000000L
#define TIMER_MAGIC_NUMBER      0xA87FCA3B

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

const fm_timestamp fmNoWaitTimeConstant =
{
    0, 0
};

/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_status SearchExistingTimerTask( fm_thread *thread, fm_timerTask **task );
static fm_status SearchAvailableTimerTask( fm_timerTask **task );
static void      *PeriodicTimerLoop( void *args );
static void      *EventDrivenTimerLoop( void *args );
static fm_status CreateTimerCondition( void **cond );
static fm_status DeleteTimerCondition( void *cond );
static fm_status WakeupTimerTask( fm_timerTask *task );
static fm_status SuspendTimerTask( fm_timerTask *task, fm_timestamp *timeout );
static fm_status AddActiveTimerToTask( fm_timerTask *task, fm_timer *timer );
static fm_status StopTimer( fm_timer *timer );
static void      PrintDbgRuler( void );

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** SearchExistingTimerTask
 * \ingroup intTimer
 *
 * \desc            Search the list of timer task, for the one matching a given
 *                  thread handle.
 *
 * \param[in]       thread points to the timer thread.
 * 
 * \param[out]      task points to a caller-allocated area where this function
 *                  will return the pointer to the matching task.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if not found.
 *
 *****************************************************************************/
static fm_status SearchExistingTimerTask( fm_thread *thread, fm_timerTask **task )
{
    fm_int i;
    fm_status status;

    *task  = NULL;
    status = FM_ERR_NOT_FOUND;

    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_TIMER_TASKS  ; i++)
    {
        if ( fmRootAlos->timerTasks[i].used == TRUE )
        {
            if ( fmThreadIdsEqual( thread, fmRootAlos->timerTasks[i].thread ) )
            {
                *task = &fmRootAlos->timerTasks[i];
                status = FM_OK;
                break;
            }
        }
    }
    return status;

}   /* end SearchExistingTimerTask */




/*****************************************************************************/
/** SearchAvailableTimerTask
 * \ingroup intTimer
 *
 * \desc            Search the list of timer task control blocks looking for
 *                  an available entry.
 * 
 * \param[out]      task is a pointer to a caller-allocated area where this
 *                  function will return the pointer to the timer task entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if no entry was available.
 * 
 *****************************************************************************/
static fm_status SearchAvailableTimerTask( fm_timerTask **task )
{
    fm_int i;
    fm_status status;

    *task  = NULL;
    status = FM_ERR_NOT_FOUND;

    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_TIMER_TASKS  ; i++)
    {
        if ( fmRootAlos->timerTasks[i].used == FALSE )
        {
            *task = &fmRootAlos->timerTasks[i];
            status = FM_OK;
            break;
        }
    }
    return status;

}   /* end SearchAvailableTimerTask */




/*****************************************************************************/
/** PeriodicTimerLoop
 * \ingroup intTimer
 *
 * \desc            Main loop for an instance of a periodic timer task.
 *
 * \param[in]       args is the thread argument pointer.
 *
 * \return          None.
 *
 *****************************************************************************/
static void *PeriodicTimerLoop( void *args )
{
    fm_thread     *thisThread;
    fm_timerTask  *task;
    fm_timestamp  next;
    fm_status     status;
    fm_bool       timerLockTaken = FALSE;
    fm_timer      *timer;

    /* grab arguments */
    thisThread =  FM_GET_THREAD_HANDLE( args );
    task       =  FM_GET_THREAD_PARAM( fm_timerTask, args );

    /* schedule the first check one period from now */
    fmGetTime( &next );
    fmAddTimestamps( &next, &task->period );

    /* signal that the initialization is complete */
    status = fmSignalSemaphore( &task->sem );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );

    task->initialized = TRUE;

    while ( task->initialized )
    {
        fmDelayUntil( next.sec, next.usec * 1000 );

        /**************************************************
         * Process those active timers whose expiration 
         * time has passed already 
         **************************************************/

        status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        timerLockTaken = TRUE;

        timer = FM_DLL_GET_FIRST( task, firstActiveTimer );
        while ( timer && fmCompareTimestamps( &next, &timer->end ) >= 0 )
        {
            /* remove the timer only if no longer active, i.e.
               if this was the last or only repetition, otherwise
               put it back in the list of active timers but in
               the right position */

            FM_DLL_REMOVE_NODE( task, 
                                firstActiveTimer,
                                lastActiveTimer,
                                timer,
                                nextActiveTimer,
                                prevActiveTimer );

            /* is this the last repetition? */
            timer->nrRepetitionsSoFar++;
            if ( ( timer->nrRepetitions != FM_TIMER_REPEAT_FOREVER   ) && 
                 ( timer->nrRepetitionsSoFar >= timer->nrRepetitions ) )
            {
                /* yes, this timer is no longer active */
                timer->running = FALSE;
            }
            else
            {
                /* no, compute the next expiraration date and
                   add the timer back in the active timer list
                   for this task */
                timer->start = timer->end;
                fmAddTimestamps( &timer->end, &timer->timeout );
                AddActiveTimerToTask( task, timer );
            }

            /* execute the callback, but release the lock temporarily
               to prevent lock inversion problems */
            status = fmReleaseLock( &task->lock );
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
            timerLockTaken = FALSE;

            /* ignore the return code, we can't give up for a caller error */
            timer->callback( timer->arg );

            /* grab the lock again and move on to the next active timer */
            status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
            timerLockTaken = TRUE;

            timer = FM_DLL_GET_FIRST( task, firstActiveTimer );
        }

        status = fmReleaseLock( &task->lock );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        timerLockTaken = FALSE;
         
        /* next cycle */
        fmAddTimestamps( &next, &task->period );

    }   /* end while (TRUE) i.e. timer thread main loop */


ABORT:
    if ( timerLockTaken )
    {
        fmReleaseLock( &task->lock );
    }

    FM_LOG_ERROR(FM_LOG_CAT_ALOS_TIME,
                 "ERROR: PeriodicTimerLoop: exiting inadvertently!\n");

    fmExitThread( thisThread );

    return NULL;

}   /* end PeriodicTimerLoop */




/*****************************************************************************/
/** EventDrivenTimerLoop
 * \ingroup intTimer
 *
 * \desc            Main loop for an instance of an event-driven timer task.
 *
 * \param[in]       args is the thread argument pointer.
 *
 * \return          None.
 *
 *****************************************************************************/
static void *EventDrivenTimerLoop( void *args )
{
    fm_thread      *thisThread;
    fm_timestamp   *timeout;
    fm_timestamp   curTime;
    fm_status      status;
    fm_timer       *timer;
    fm_bool        timerLockTaken = FALSE;
    fm_timerTask   *task;

    /* grab arguments */
    thisThread =  FM_GET_THREAD_HANDLE( args );
    task       =  FM_GET_THREAD_PARAM( fm_timerTask, args );

    /* we need to enter the main loop holding the timer task lock */
    status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );
    timerLockTaken = TRUE;

    /* signal that the initialization is complete */
    status = fmSignalSemaphore( &task->sem );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );

    task->initialized = TRUE;

    timeout = NULL;
    while ( task->initialized )
    {
        status = SuspendTimerTask( task, timeout );
        if ( status != FM_OK )
        {
            /* ignore the return code, we can't give up */
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_TIME,
                         "ERROR: SuspendTimerTask: status = %d\n", status);
        }

        /******************************************************
         * Here's where we need to process the list of 
         * timers handled by this thread. There are two cases: 
         *  
         * Case 1:
         *  The timeout was NULL (infinite wait) or hasn't 
         *  expired yet. That means we've been woken up by one
         *  of these functions: fmStartTimer(), fmStopTimer()
         *  or fmDeleteTimer(). In those cases we simply need
         *  to check if there is a new timeout (indicating
         *  the head of the list has changed) and suspend again
         *  
         * Case 2: 
         *  The timeout has expired. In that case we need to 
         *  process all the expired timers before suspending 
         *  again. 
         ******************************************************/

        status = fmGetTime( &curTime );
        if ( status != FM_OK )
        {
            /* ignore the return code, we can't give up */
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_TIME,
                         "ERROR: fmGetTime: status = %d\n", status);
        }

        timer = FM_DLL_GET_FIRST( task, firstActiveTimer );
        if ( timeout == NULL || fmCompareTimestamps( &curTime, timeout ) < 0 )
        {
            /* Case 1 */
            timeout = ( (timer != NULL) ? &timer->end : NULL );
            continue;
        }
        else
        {
            /* Case 2  */
            while ( timer && fmCompareTimestamps( &curTime, &timer->end ) >= 0 ) 
            {
                /* remove the timer only if no longer active, i.e.
                   if this was the last or only repetition, otherwise
                   put it back in the list of active timers but in
                   the right position */

                FM_DLL_REMOVE_NODE( task, 
                                    firstActiveTimer,
                                    lastActiveTimer,
                                    timer,
                                    nextActiveTimer,
                                    prevActiveTimer );

                /* is this the last repetition? */
                timer->nrRepetitionsSoFar++;
                if ( ( timer->nrRepetitions != FM_TIMER_REPEAT_FOREVER   ) && 
                     ( timer->nrRepetitionsSoFar >= timer->nrRepetitions ) )
                {
                    /* yes, this timer is no longer active */
                    timer->running = FALSE;
                }
                else
                {
                    /* no, compute the next expiraration date and
                       add the timer back in the active timer list
                       for this task */
                    timer->start = timer->end;
                    fmAddTimestamps( &timer->end, &timer->timeout );
                    AddActiveTimerToTask( task, timer );
                }

                /* execute the callback, but release the lock temporarily
                   to prevent lock inversion problems */
                status = fmReleaseLock( &task->lock );
                FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
                timerLockTaken = FALSE;

                /* ignore the return code, we can't give up for a caller error */
                timer->callback( timer->arg );

                /* grab the lock again and move on to the next active timer */
                status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
                FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
                timerLockTaken = TRUE;

                /* check next timer now */
                timer = FM_DLL_GET_FIRST( task, firstActiveTimer );

            }   /* end while ( timer && ... ) */

            /* next cycle: next timeout belongs to the  head of the active
               timer list associated with this timer task. if the list is
               empty then we'll block until signalled otherwise */
            timeout = ( (timer != NULL) ? &timer->end : NULL );

        }  /* end else  (i.e. one or more timers have expired ) */

    }   /* end while (TRUE) i.e. timer thread main loop */


ABORT:
    if ( timerLockTaken == TRUE )
    {
        fmReleaseLock( &task->lock );
    }

    FM_LOG_ERROR(FM_LOG_CAT_ALOS_TIME,
                 "ERROR: EventDrivenTimerLoop: exiting inadvertently!\n");

    fmExitThread( thisThread );

    return NULL;

}   /* end EventDrivenTimerLoop */




/*****************************************************************************/
/** CreateTimerCondition
 * \ingroup intTimer
 *
 * \desc            Create and initialize a condition variable used to suspend
 *                  and wake up the timer task.
 *
 * \param[in]       cond is a pointer to a caller-allocated area where this
 *                  function will return the pointer to the condition variable.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory available to allocate for
 *                  the new thread.
 * \return          FM_ERR_UNABLE_TO_CREATE_COND is an error that caused the
 *                  condition variable not to be created
 * 
 *****************************************************************************/
static fm_status CreateTimerCondition( void **cond )
{
    pthread_condattr_t attr;
    fm_status          status;
    char               strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t            strErrNum;
    int                posixError;

    *(pthread_cond_t **)cond = fmAlloc( sizeof(pthread_cond_t) );
    if ( *(pthread_cond_t **)cond == NULL )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, FM_ERR_NO_MEM);
    }

    /* Make the conditional variable process shared. */
    if ((pthread_condattr_init(&attr) != 0) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_TIME,
                     "Failed to initialize condition variable\n");
        fmFree(*cond);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, FM_ERR_UNABLE_TO_CREATE_COND);
    }

    /*
     * At this point, cond has has been allocated and attr has been
     * initialized, so we may ABORT.
     */ 

    if ( pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0 )
    {
        pthread_condattr_destroy(&attr);
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_TIME,
                     "Failed to initialize condition variable\n");
        status = FM_ERR_UNABLE_TO_CREATE_COND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status);
    }

    /* associate this condition variable to the monotonic clock */
    if ( pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) != 0 )
    {
        pthread_condattr_destroy(&attr);
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_TIME,
                     "Failed to initialize condition variable\n");
        status = FM_ERR_UNABLE_TO_CREATE_COND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status);
    }

    /* initialize condition var */
    if ( ( posixError = pthread_cond_init( *cond, &attr ) ) != 0 )
    {
        pthread_condattr_destroy(&attr);
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, posixError);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR( FM_LOG_CAT_ALOS_TIME,
                          "Failed to create condition %s\n",
                          strErrBuf );
        }
        else
        {
            FM_LOG_ERROR( FM_LOG_CAT_ALOS_TIME,
                          "Failed to create condition %d\n",
                          posixError );
        }

        status = FM_ERR_UNABLE_TO_CREATE_COND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status);
    }

    status = FM_OK;

ABORT:
    if ( status != FM_OK )
    {
        fmFree(*(pthread_cond_t **)cond);
    }
    pthread_condattr_destroy(&attr);
    return status;

}   /* end CreateTimerCondition */




/*****************************************************************************/
/** DeleteTimerCondition
 * \ingroup intTimer
 *
 * \desc            Delete a condition variable previously created using
 *                  ''CreateTimerCondition''.
 *
 * \param[in]       cond is a pointer to a caller-allocated area containing
 *                  the condition variable to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if there was a posix-related error.
 * 
 *****************************************************************************/
static fm_status DeleteTimerCondition( void *cond )
{
    fm_status status = FM_OK;

    if ( pthread_cond_broadcast( cond ) != 0 )
    {
        status = FM_FAIL;
    }
    if ( pthread_cond_destroy( cond ) != 0 )
    {
        status = FM_FAIL;
    }
    
    fmFree( cond );

    return status;

}   /* DeleteTimerCondition() */



/*****************************************************************************/
/** WakeupTimerTask
 * \ingroup intTimer
 *
 * \desc            Function to wake up a suspended timer task
 *                  blocked on a given condition variable.
 * 
 * \note            This function must be called with the timer task's lock
 *                  already taken. Failure to do so may lead to unpredictable
 *                  results.
 * 
 * \param[in]       task is the pointer to the timer task that needs to be
 *                  woken up.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNABLE_TO_SIGNAL_COND if unable to send signal to
 *                  the condition variable.
 * 
 *****************************************************************************/
static fm_status WakeupTimerTask( fm_timerTask *task )
{
    int posixError;

    posixError = pthread_cond_signal( task->cond );

    return (posixError == 0 ? FM_OK : FM_ERR_UNABLE_TO_SIGNAL_COND);

}   /* end WakeupTimerTask */


/*****************************************************************************/
/** SuspendTimerTask
 * \ingroup intTimer
 *
 * \desc            Suspend a timer task until a given time.
 * 
 * \note            This function must be called with the timer task's lock
 *                  already taken. Failure to do so may lead to unpredictable
 *                  results.
 * 
 * \param[in]       task is the pointer to the timer task that needs to be
 *                  woken up.
 * 
 * \param[in]       timeout is the pointer to a caller-allocated area
 *                  containing the absolute time when the task will unblock
 *                  itself if it's not explicitly woken up with
 *                  ''WakeupTimerTask''. If NULL the task will block until
 *                  woken up.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_CONDWAIT_FAILED if there was an error while blocking
 *                  on the condition variable.
 * 
 *****************************************************************************/
static fm_status SuspendTimerTask( fm_timerTask *task, fm_timestamp *timeout )
{
    fm_int          posixRc;
    fm_status       status;
    struct timespec endtime;

    if ( timeout == NULL )
    {
        FM_LOG_ENTRY( FM_LOG_CAT_ALOS_TIME, "timeout is NULL\n" );

        posixRc = pthread_cond_wait( task->cond, task->lock.handle );
        status = ( !posixRc ? FM_OK : FM_ERR_CONDWAIT_FAILED );
    }
    else
    {
        FM_LOG_ENTRY( FM_LOG_CAT_ALOS_TIME,
                      "seconds=%lld nanoseconds=%lld\n",
                      timeout->sec,
                      timeout->usec*1000 );

        endtime.tv_sec  = timeout->sec;
        endtime.tv_nsec = timeout->usec*1000;

        while ( TRUE )
        {
            posixRc = pthread_cond_timedwait ( task->cond, 
                                               task->lock.handle, 
                                               &endtime );
            status = ( (!posixRc || posixRc == ETIMEDOUT) ? 
                       FM_OK : FM_ERR_CONDWAIT_FAILED );
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, status );

}   /* end SuspendTimerTask */




/*****************************************************************************/
/** AddActiveTimerToTask
 * \ingroup intTimer
 *
 * \desc            Adds a timer to the sorted list of active timers
 *                  associated with a given timer task. The list is sorted
 *                  based on the timer's expiration time, with the earliest
 *                  being the first in the list.
 * 
 * \param[in]       task is the pointer to the timer task data structure.
 * 
 * \param[in]       timer is the pointer to the timer data structure.
 *
 * \return          FM_OK if successful.
 * 
 *****************************************************************************/
static fm_status AddActiveTimerToTask( fm_timerTask *task, fm_timer *timer )
{
    fm_timer  *activeTimer;
    fm_timer  *prevActiveTimer;

    /* scan the list of active timers */
    prevActiveTimer = NULL;
    activeTimer = FM_DLL_GET_FIRST( task, firstActiveTimer );
    while ( ( activeTimer != NULL ) &&
            ( fmCompareTimestamps( &timer->end, &activeTimer->end ) > 0 ) ) 
    {
        prevActiveTimer = activeTimer;
        activeTimer = FM_DLL_GET_NEXT( activeTimer, nextActiveTimer );
    }

    /* check list consistency */
    if ( activeTimer == NULL)
    {
        if ( task->lastActiveTimer != prevActiveTimer )
        {
            FM_LOG_DEBUG( FM_LOG_CAT_ALOS_TIME, 
                          "Invalid list of active timers (fixed)\n");
            task->lastActiveTimer = prevActiveTimer;
        }
    }

    FM_DLL_INSERT_BEFORE( task, 
                          firstActiveTimer, 
                          lastActiveTimer, 
                          activeTimer,
                          nextActiveTimer,
                          prevActiveTimer,
                          timer );

    return FM_OK;

}   /* end AddActiveTimerToTask */




/*****************************************************************************/
/** StopTimer
 * \ingroup intTimer
 *
 * \desc            Internal version of ''fmStopTimer''. Stops a timer but
 *                  skips the initial sanity checks on the argument.
 * 
 * \param[in]       timer is the pointer to the timer data structure.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
static fm_status StopTimer( fm_timer *timer )
{
    fm_timerTask *task;
    fm_bool       timerLockTaken = FALSE;
    fm_status     status;
    fm_timer     *oldFirstActiveTimer;
    fm_timer     *pTimer;

    task = timer->task;
    oldFirstActiveTimer = NULL;

    status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    timerLockTaken = TRUE;


    /* verify that the timer is in the active timers list, then remove it */
    pTimer = FM_DLL_GET_FIRST( task, firstActiveTimer );

    while ( pTimer != NULL ) 
    {
        if ( pTimer == timer )
        {
            /* Peek at the head of the list, we may need to know if this was the
               first active timer */
            oldFirstActiveTimer = FM_DLL_GET_FIRST( task, firstActiveTimer );

            /* remove it from the list of active timers */
            FM_DLL_REMOVE_NODE( task,
                                firstActiveTimer,
                                lastActiveTimer,
                                timer,
                                nextActiveTimer,
                                prevActiveTimer );
            break;
        }

        pTimer = FM_DLL_GET_NEXT( pTimer, nextActiveTimer );
    }

    timer->running = FALSE;

    /* For an event-driven timer task, signal the condition to the main loop */
    if ( task->mode == FM_TIMER_TASK_MODE_EVENT_DRIVEN )
    {
        /* we do it only if it's the first active timer in the list */
        if ( oldFirstActiveTimer == timer )
        {
            /* wakeup the timer task, so that it can process the new timer event */
            status = WakeupTimerTask( task );
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        }
    }

    status = FM_OK;

ABORT:
    if ( timerLockTaken == TRUE )
    {
        fmReleaseLock( &task->lock );
    }
    return status;

}   /* end StopTimer */




/*****************************************************************************/
/** PrintDbgRuler
 * \ingroup intTimer
 *
 * \desc            Displays a ruler for the formatted output of
 *                  ''fmDbgDumpTimers''
 *
 * \return          None.
 * 
 *****************************************************************************/
static void PrintDbgRuler( void )
{
    FM_LOG_PRINT("+----------------+-------+-----------+------------"
                 "+------------------+------------------+\n");

}   /* end PrintDbgRuler */




/*****************************************************************************/
/** CleanupAllTimers
 * \ingroup intTimer
 *
 * \desc            Cleans up all timers associated with a given timer task.
 *                  Useful when the timer task is about to be deleted.
 * 
 * \note            This is a local function to be invoked only with the timer
 *                  task lock taken.
 * 
 * \param[in]       task is the pointer to the timer task data structure.
 *
 * \return          FM_OK.
 * 
 *****************************************************************************/
static fm_status CleanupAllTimers( fm_timerTask *task )
{
    fm_timer *timer;
    fm_timer *nextTimer;

    /* scan the list of instantiated timers */
    timer = FM_DLL_GET_FIRST( task, firstTimer );
    while ( timer != NULL )
    {
        /* save the next pointer */
        nextTimer = FM_DLL_GET_NEXT( timer, nextTimer );
        
        /* if active, remove it from the list of active timers */
        if ( timer->running )
        {
            FM_DLL_REMOVE_NODE( task,
                                firstActiveTimer,
                                lastActiveTimer,
                                timer,
                                nextActiveTimer,
                                prevActiveTimer );
        }

        /* remove it from the list of instantiated timers */
        FM_DLL_REMOVE_NODE( task,
                            firstTimer,
                            lastTimer,
                            timer,
                            nextTimer,
                            prevTimer );

        /* now try next */
        timer = nextTimer;
    }

    return FM_OK;

}   /* end CleanupAllTimers */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCompareTimestamps
 * \ingroup alosTime
 *
 * \desc            Compare two timestamp objects.
 *
 * \param[in]       t1 is a pointer to one fm_timestamp object.
 *
 * \param[in]       t2 is a pointer to the other fm_timestamp object.
 *
 * \return          -1 if t1 < t2
 * \return          0 if t1 = t2
 * \return          1 if t1 > t2
 *
 *****************************************************************************/
fm_int fmCompareTimestamps(fm_timestamp *t1, fm_timestamp *t2)
{
    if ((t1->sec < t2->sec) || ((t1->sec == t2->sec) && (t1->usec < t2->usec)))
    {
        return -1;
    }
    else if (t1->sec == t2->sec && t1->usec == t2->usec)
    {
        return 0;
    }
    else
    {
        return 1;
    }

}   /* end fmCompareTimestamps */




/*****************************************************************************/
/** fmAddTimestamps
 * \ingroup intAlosTime
 *
 * \desc            Add two timestamp objects together: t1 = t1 + t2.
 *
 * \param[in,out]   t1 is a pointer to one fm_timestamp object.
 *
 * \param[in]       t2 is a pointer to the other fm_timestamp object.
 *
 * \return          None
 *
 *****************************************************************************/
void fmAddTimestamps(fm_timestamp *t1, fm_timestamp *t2)
{
    t1->sec  += t2->sec;
    t1->usec += t2->usec;

    if (t1->usec >= 1000000)
    {
        t1->sec += (t1->usec / 1000000);
        t1->usec = (t1->usec % 1000000);
    }

}   /* end fmAddTimestamps */




/*****************************************************************************/
/** fmSubTimestamps
 * \ingroup alosTime
 *
 * \desc            Subtract one timestamp object from another: t3 = t1 - t2.
 *
 * \param[in]       t1 is a pointer to the minuend fm_timestamp object.
 *
 * \param[in]       t2 is a pointer to the subtrahend fm_timestamp object.
 *
 * \param[out]      t3 is a pointer to the difference fm_timestamp object.
 *
 * \return          None
 *
 *****************************************************************************/
void fmSubTimestamps(const fm_timestamp *t1,
                     const fm_timestamp *t2,
                     fm_timestamp *      t3)
{
    fm_int64 usecDif;

    t3->sec  = t1->sec - t2->sec;
    t3->usec = t1->usec - t2->usec;
    usecDif  = (fm_int64) t3->usec;

    if (usecDif < 0)
    {
        t3->sec  -= 1;
        t3->usec += 1000000;
    }

}   /* end fmSubTimestamps */




/*****************************************************************************/
/** fmGetTime
 * \ingroup alosTime
 *
 * \desc            Get the number of seconds and microseconds since
 *                  the Epoch.
 *
 * \param[out]      ts points to a caller-allocated fm_timestamp structure
 *                  where this function should place the current time.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmGetTime(fm_timestamp *ts)
{
    struct timespec tv;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_TIME, "ts=%p\n", (void *) ts);

    /* use CLOCK_MONOTONIC to be not sensitive to sysTime change */
    if (clock_gettime(CLOCK_MONOTONIC, &tv) != 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, FM_FAIL);
    }

    ts->sec  = tv.tv_sec;
    ts->usec = tv.tv_nsec/1000;

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, FM_OK);

}   /* end fmGetTime */




/*****************************************************************************/
/** fmGetFormattedTime
 * \ingroup intAlosTime
 *
 * \desc            Fills the provided buffer with a string containing the
 *                  formatted current time.
 *
 * \param[out]      dateStr will be filled with a string representing the
 *                  current time, in the same format as the standard library
 *                  function ctime, except without a newline on the end.
 *                  The string is 24 characters long, but the provided
 *                  buffer must be 26 characters long.  (One for the
 *                  terminating NUL character, and one for internal
 *                  scratch purposes.)
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmGetFormattedTime(char *dateStr)
{
    time_t timeSinceEpoch = time(NULL);

    /***************************************************
     * Specifically do *NOT* log this function since it
     * is called by the logging facility itself.
     **************************************************/

    if ( !ctime_r(&timeSinceEpoch, dateStr) )
    {
        return FM_FAIL;
    }

    /* chop off newline */
    *(dateStr + strlen(dateStr) - 1) = 0;

    return FM_OK;

}   /* end fmGetFormattedTime */




/*****************************************************************************/
/** fmDelayBy
 * \ingroup alosTime
 *
 * \desc            Blocks for a given amount of time.  The delay is given as
 *                  two arguments, seconds (s) and nanoseconds (n), where the
 *                  total delay would be s + (1.0e-9 * n) seconds.  Note that
 *                  most systems will only be able to block for a minimum
 *                  duration of 1us or sometimes even higher.
 *
 * \param[in]       seconds is the number of seconds to wait
 *
 * \param[in]       nanoseconds is the additional amount of nanoseconds to
 *                  wait.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmDelayBy(fm_int seconds, fm_int nanoseconds)
{
    struct timespec delaytime;
    struct timespec remaintime;
    int rc=0; 

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_TIME,
                 "seconds=%d nanoseconds=%d\n",
                 seconds,
                 nanoseconds);

    delaytime.tv_sec  = seconds;
    delaytime.tv_nsec = nanoseconds;

    do 
    {
        remaintime.tv_sec  = 0;
        remaintime.tv_nsec = 0;
        rc = nanosleep(&delaytime, &remaintime);
        delaytime = remaintime;
    } while ((rc == EINTR) && (remaintime.tv_sec != 0 || remaintime.tv_nsec != 0)); 

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, (rc == 0) ? FM_OK : FM_FAIL);

}   /* end fmDelayBy */




/*****************************************************************************/
/** fmDelayUntil
 * \ingroup alosTime
 *
 * \desc            Blocks until a given time
 *
 * \param[in]       seconds 
 *
 * \param[in]       nanoseconds
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmDelayUntil(fm_int seconds, fm_int nanoseconds)
{
    struct timespec endtime;
    struct timespec remaintime;
    int rc = 0; 

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_TIME,
                 "seconds=%d nanoseconds=%d\n",
                 seconds,
                 nanoseconds);


    endtime.tv_sec   = seconds;
    endtime.tv_nsec  = nanoseconds;

    do 
    {
        remaintime.tv_sec  = 0;
        remaintime.tv_nsec = 0;
        rc = clock_nanosleep(CLOCK_MONOTONIC,
                             TIMER_ABSTIME,
                             &endtime, 
                             &remaintime);
    } while ((rc == EINTR) && (remaintime.tv_sec != 0 || remaintime.tv_nsec != 0)); 


    FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, (rc == 0) ? FM_OK : FM_FAIL);

}   /* end fmDelayUntil */




/*****************************************************************************/
/** fmConvertTimestampToTicks
 * \ingroup intAlosTime
 *
 * \desc            Converts a timestamp into system ticks.
 *
 * \param[in]       tstamp contains the timestamp.
 *
 * \return          Time value in ticks.
 *
 *****************************************************************************/
fm_uint64 fmConvertTimestampToTicks(fm_timestamp *tstamp)
{
    fm_uint64 ticks;

    /* convert seconds into ticks */
    ticks = (fm_uint64) tstamp->sec * FM_TICKS_PER_SECOND;

    /* compute number of ticks within the current second,
     * (1 million microseconds per second)
     */
    ticks += (fm_uint64) ( tstamp->usec / (1000000 / FM_TICKS_PER_SECOND) );

    return ticks;

}   /* end fmConvertTimestampToTicks */




/*****************************************************************************/
/** fmGetTimeRes
 * \ingroup intAlosTime
 *
 * \desc            Get timer resolution.
 *
 * \param[out]      tr points to a caller-allocated fm_timestamp structure
 *                  where this function should place the timer resolution.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not.
 *
 *****************************************************************************/
fm_status fmGetTimeRes( fm_timestamp *tr )
{
    struct timespec res;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_TIME, "tr=%p\n", (void *) tr);

    /* use CLOCK_MONOTONIC to be insensitive to sysTime change */
    if (clock_getres(CLOCK_MONOTONIC, &res) != 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, FM_FAIL);
    }

    tr->sec  = res.tv_sec;
    tr->usec = res.tv_nsec/1000;

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, FM_OK);

}   /* end fmGetTimeRes */




/*****************************************************************************/
/** fmAlosTimeInit
 * \ingroup intAlosTime
 *
 * \desc            Initializes the data structures used by the time utility
 *                  module.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the ALOS root object hasn't been
 *                  initialized yet.
 * \return          FM_ERR_NO_MEM if there was a memory allocation problem.
 * \return          FM_ERR_LOCK_INIT if there was a problem initializing the
 *                  lock associated with the timer utility.
 * 
 *****************************************************************************/
fm_status fmAlosTimeInit( void )
{
    fm_status status;
    int       posixRc;
    fm_int    i;
    pthread_mutexattr_t attr;

    /* quit right away, if the ALOS root data structure wasn't initialized */
    if ( fmRootAlos == NULL )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* clear all the timer task data structures */
    fmRootAlos->nrTimerTasks = 0;
    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_TIMER_TASKS  ; i++)
    {
        FM_MEMSET_S( &fmRootAlos->timerTasks[i],
                     sizeof(fm_timerTask),
                     0,
                     sizeof(fm_timerTask) );
    }

    /* now initialize the root timer lock */
    fmRootAlos->timerTasksLock.handle =
        (pthread_mutex_t *) fmAlloc( sizeof(pthread_mutex_t) );

    if (fmRootAlos->timerTasksLock.handle == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status)
    }

    if ( pthread_mutexattr_init(&attr) )
    {
        status = FM_ERR_LOCK_INIT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status)
    }
    else if ( pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        posixRc = pthread_mutexattr_destroy(&attr);
        if (posixRc != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS_TIME,
                         "Error %d destroying mutex attr\n",
                         posixRc);
        }
        status = FM_ERR_LOCK_INIT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );
    }
    else if ( pthread_mutex_init( (pthread_mutex_t *) 
                                  fmRootAlos->timerTasksLock.handle,
                                  &attr ) )
    {
        posixRc = pthread_mutexattr_destroy(&attr);
        if (posixRc != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS_TIME,
                         "Error %d destroying mutex attr\n",
                         posixRc);
        }
        status = FM_ERR_LOCK_INIT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );
    }
    else if ( pthread_mutexattr_destroy(&attr) )
    {
        posixRc = pthread_mutex_destroy( (pthread_mutex_t *) 
                                         fmRootAlos->timerTasksLock.handle );
        if (posixRc != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS_TIME,
                         "Error %d destroying mutex attr\n",
                         posixRc);
        }
        status = FM_ERR_LOCK_INIT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );
    }

    fmRootAlos->timerTasksLock.name = fmStringDuplicate("TimerLock");
    if (fmRootAlos->timerTasksLock.name == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_TIME, status );
    }

    status = FM_OK;

ABORT:
    /* clean up if something went wrong */
    if ( fmRootAlos != 0 && status != FM_OK )
    {
        if ( fmRootAlos->timerTasksLock.handle != NULL )
        {
            fmFree( fmRootAlos->timerTasksLock.handle );
        }
        if ( fmRootAlos->timerTasksLock.name != NULL )
        {
            fmFree( fmRootAlos->timerTasksLock.name );
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_TIME, status);

}   /* end fmAlosTimeInit */




/*****************************************************************************/
/** fmCreateTimerTask
 * \ingroup intTimer
 *
 * \desc            Creates a timer thread.
 *
 * \note            For the sake of accuracy, if the timer task mode is
 *                  periodic, it's recommended that the period is an integer
 *                  multiple of the underlying OS's timer resolution. This
 *                  function will allow any scheduling period, but will warn
 *                  the caller with an appropriate log message if the tick
 *                  isn't a multiple of the timer resolution. The caller can
 *                  verify the system's timer resolution using ''fmGetTimeRes''.
 * 
 * \param[in]       taskName is a string representing this task.
 * 
 * \param[in]       mode is the timer task mode, see ''fm_timerTaskMode'' for a
 *                  list of supported values.
 * 
 * \param[in]       period is the the pointer to a caller-allocated area
 *                  containing the timer thread period, valid only when mode
 *                  is ''FM_TIMER_TASK_MODE_PERIODIC''. If NULL, this function
 *                  will use the underlying OS's timer resolution as period.
 *
 * \param[out]      thread points to the caller-allocated fm_thread structure.
 *                  The structure will be filled in by this function. The
 *                  API uses this structure to maintain housekeeping
 *                  information for the thread; the application need never be
 *                  concerned with its contents.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if there was a memory allocation failure.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * \return          FM_ERR_NO_FREE_RESOURCES if we exceeded the number of timer
 *                  task entries.
 * 
 *****************************************************************************/
fm_status fmCreateTimerTask( fm_text           taskName, 
                             fm_timerTaskMode  mode,
                             fm_timestamp     *period,
                             void             *thread )
{
    fm_status     status;
    fm_char       auxName[32];
    fm_timerTask *task                = NULL;
    fm_bool       taskSemInit         = FALSE;
    fm_bool       taskLockInit        = FALSE;
    fm_bool       taskCondInit        = FALSE;
    fm_bool       timerTasksLockTaken = FALSE;
    fm_timestamp  timerRes;

    /**************************************************
     * Check timer resolution here
     **************************************************/

    /* check the arguments */
    if ( taskName     == NULL || 
         thread       == NULL ||
         (fm_int)mode < 0     ||
         (fm_int)mode >= FM_TIMER_TASK_MODE_MAX )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* for periodic tasks, a period must be specified */
    /* if it's not, we'll use the underlying OS tick  */
    if ( mode == FM_TIMER_TASK_MODE_PERIODIC && period == NULL )
    {
        status = fmGetTimeRes( &timerRes );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        period = &timerRes;
    }

    /* make sure the initialization sequence has been completed */
    if ( fmRootAlos == NULL                        || 
         fmRootAlos->timerTasksLock.handle == NULL ||
         fmRootAlos->timerTasksLock.name   == NULL )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* have we exceeded the max number of timer tasks */
    if ( fmRootAlos->nrTimerTasks >= FM_ALOS_INTERNAL_MAX_TIMER_TASKS )
    {
        status = FM_ERR_NO_FREE_RESOURCES;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* now capture the timer task lock so that we can add this new task */
    status = fmCaptureLock( &fmRootAlos->timerTasksLock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    timerTasksLockTaken = TRUE;

    status = SearchAvailableTimerTask( &task );
    if ( status != FM_OK || task == NULL )
    {
        /* this shouldn't happen */
        status = FM_ERR_NO_FREE_RESOURCES;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* now, initialize the timer task control block */
    FM_MEMSET_S( task, sizeof(fm_timerTask), 0, sizeof(fm_timerTask) );

    /* fill out the timer task information */
    task->thread      =  thread;
    task->initialized = FALSE;
    task->mode        =  mode;
    if ( mode == FM_TIMER_TASK_MODE_PERIODIC )
    {
        task->period   = *period;
    }

    /* create the lock for this task */
    FM_SPRINTF_S( auxName, 32, "%sLock", taskName );
    status = fmCreateLock( auxName, &task->lock );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    taskLockInit = TRUE;

    /* create the semaphore for this task */
    FM_SPRINTF_S( auxName, 32, "%sSem", taskName );
    status = fmCreateSemaphore( auxName, FM_SEM_BINARY, &task->sem, 0 );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    taskSemInit = TRUE;

    /* if event-driven timer task, initialize the condition variable*/
    if ( mode == FM_TIMER_TASK_MODE_EVENT_DRIVEN )
    {
        status = CreateTimerCondition( &task->cond );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        taskCondInit = TRUE;
    }

    /* create the actual thread. The entry point will depend on the mode */
    if ( mode == FM_TIMER_TASK_MODE_PERIODIC )
    {
        /* actually create the associate thread */
        status  = fmCreateThread( taskName,
                                  FM_EVENT_QUEUE_SIZE_NONE, 
                                  PeriodicTimerLoop,
                                  task,
                                  thread );
    }
    else
    {
        /* actually create the associate thread */
        status  = fmCreateThread( taskName,
                                  FM_EVENT_QUEUE_SIZE_NONE,
                                  EventDrivenTimerLoop,
                                  task,
                                  thread );
    }
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );

    /* finally we can claim success */
    task->used   =  TRUE;
    fmRootAlos->nrTimerTasks++;


ABORT:        
    /* release the lock if it was taken */
    if ( timerTasksLockTaken == TRUE )
    {
        fmReleaseLock( &fmRootAlos->timerTasksLock );
    }

    /* cleanup if something went wrong */
    if ( status != FM_OK && task != NULL )
    {
        /* free memory allocated for the timer task control block */
        if ( taskLockInit == TRUE )
        {
            fmDeleteLock( &task->lock );
        }
        if ( taskSemInit == TRUE )
        {
            fmDeleteSemaphore( &task->sem );
        }
        if ( taskCondInit == TRUE )
        {
            DeleteTimerCondition( task->cond );
        }
    }

    return status;

}   /* end fmCreateTimerTask */




/*****************************************************************************/
/** fmDeleteTimerTask
 * \ingroup intTimer
 *
 * \desc            Deletes a timer task.
 *
 * \note            Before deleting this timer thread, this function will first
 *                  stop (if active) then delete any timer associated with this
 *                  timer thread.
 * 
 * \param[in]       thread points to the caller-allocated fm_thread structure
 *                  associated with the task to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the ALOS root object hasn't been
 *                  initialized yet.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmDeleteTimerTask( void *thread )
{
    fm_status      status;
    fm_timerTask  *task;
    fm_bool        timerTasksLockTaken = FALSE;
    fm_bool        timerLockTaken      = FALSE;

    /* check the arguments */
    if ( thread == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }


    /* make sure the initialization sequence has been completed */
    if ( fmRootAlos == NULL                        || 
         fmRootAlos->timerTasksLock.handle == NULL ||
         fmRootAlos->timerTasksLock.name   == NULL )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    status = fmCaptureLock( &fmRootAlos->timerTasksLock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    timerTasksLockTaken = TRUE;

    /* search the task in the task list using the thread as search key */
    status = SearchExistingTimerTask( thread, &task );
    fmReleaseLock( &fmRootAlos->timerTasksLock );
    timerTasksLockTaken = FALSE;

    if ( status == FM_OK && task != NULL )
    {
        
        /* we cleanup all timers associated with this task
           while holding the task's lock */
        status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        timerLockTaken = TRUE;

        CleanupAllTimers( task );
        task->initialized = FALSE;

        /* releasing the lock is enough for periodic timer tasks to exit */
        status = fmReleaseLock( &task->lock );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        timerLockTaken = FALSE;

        /* for event-driven timer tasks, we have to wake it up to exit */
        if ( task->mode == FM_TIMER_TASK_MODE_EVENT_DRIVEN )
        {
            WakeupTimerTask( task );
        }

        /* wait until the timer thread exits before proceeding*/
        fmWaitThreadExit( task->thread );

        /* delete semaphore, condition variable and lock */
        fmDeleteSemaphore( &task->sem );
        if ( task->mode == FM_TIMER_TASK_MODE_EVENT_DRIVEN )
        {
            DeleteTimerCondition( task->cond );
        }
        task->thread = NULL;
        fmDeleteLock( &task->lock );

        /* update the list of timer tasks */
        status = fmCaptureLock( &fmRootAlos->timerTasksLock, FM_WAIT_FOREVER );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        timerTasksLockTaken = TRUE;

        task->used = FALSE;
        fmRootAlos->nrTimerTasks--;

        fmReleaseLock( &fmRootAlos->timerTasksLock );
        timerTasksLockTaken = FALSE;

    }

    /* if we get here, exit gracefully even in case of error */
    status = FM_OK;


ABORT:
    /* release the locks if they are still taken */
    if ( timerTasksLockTaken == TRUE )
    {
        fmReleaseLock( &fmRootAlos->timerTasksLock );
    }
    if ( timerLockTaken == TRUE )
    {
        fmReleaseLock( &task->lock );
    }
    return status;

}   /* end fmDeleteTimerTask */




/*****************************************************************************/
/** fmCreateTimer
 * \ingroup intTimer
 *
 * \desc            Creates a timer.
 * 
 * \param[in]       timerName is a string representing this timer.
 * 
 * \param[in]       thread is the timer thread this timer is supposed to be
 *                  associated with.
 *
 * \param[out]      handlePtr is a pointer to caller allocated aread where this
 *                  function will return a handle representing this timer. The
 *                  caller is expected to use this handle for subsequent
 *                  operations on the same timer.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if there was a memory allocation failure.
 * \return          FM_ERR_UNINITIALIZED if the ALOS root object hasn't been
 *                  initialized yet.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmCreateTimer( fm_text         timerName, 
                         void           *thread, 
                         fm_timerHandle *handlePtr )
{
    fm_status     status;
    fm_timerTask *task;
    fm_timer     *timer = NULL;
    fm_bool       timerTasksLockTaken = FALSE;

    /* check the arguments */
    if ( timerName == NULL || thread == NULL || handlePtr == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* make sure the initialization sequence has been completed */
    if ( fmRootAlos == NULL                        || 
         fmRootAlos->timerTasksLock.handle == NULL ||
         fmRootAlos->timerTasksLock.name   == NULL )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    status = fmCaptureLock( &fmRootAlos->timerTasksLock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    timerTasksLockTaken = TRUE;

    /* search the task in the task list using the thread as search key */
    status = SearchExistingTimerTask( thread, &task );
    if ( status == FM_OK && task != NULL )
    {
        /* Timer task found, now create the timer data structure */
        /* Allocate the timer data object                        */
        timer = fmAlloc( sizeof(fm_timer) );
        if ( timer == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        }

        /* now initialize it */
        FM_MEMSET_S( timer, sizeof(fm_timer), 0, sizeof(fm_timer) );
        timer->magicNumber = TIMER_MAGIC_NUMBER;
        timer->running     = FALSE;
        timer->task        = task;
        timer->name = fmStringDuplicate( timerName );
        if ( timer->name == NULL )
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        }
        *(fm_timer **)handlePtr = timer;
        
        /* now add the timer to the list of existing timers for this task */
        FM_DLL_INSERT_LAST( task, 
                            firstTimer, 
                            lastTimer, 
                            timer, 
                            nextTimer,
                            prevTimer );

    }   /* end if ( status == FM_OK .... ) */

ABORT:
    if ( timerTasksLockTaken )
    {
        fmReleaseLock( &fmRootAlos->timerTasksLock );
    }
    if ( status != FM_OK && timer != NULL )
    {
        if ( timer->name != NULL )
        {
            fmFree( timer->name );
        }
        fmFree( timer );
    }
    return status;

}   /* end fmCreateTimer */




/*****************************************************************************/
/** fmDeleteTimer
 * \ingroup intTimer
 *
 * \desc            Deletes a timer.
 * 
 * \param[in]       handle is the handle associated with this timer.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmDeleteTimer( fm_timerHandle handle )
{
    fm_status     status;
    fm_timerTask *task;
    fm_timer     *timer;

    /* check the input args */
    if ( handle == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* double-check that the handle represents a valid timer object */
    timer = (fm_timer *)handle;
    if ( timer->magicNumber != TIMER_MAGIC_NUMBER )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* stop the timer if it's running */
    if ( timer->running == TRUE )
    {
        StopTimer( timer );
    }

    task = timer->task;

    /* now remove the timer from the list of existing timers for this task */
    status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );

    FM_DLL_REMOVE_NODE( task, 
                        firstTimer, 
                        lastTimer, 
                        timer,
                        nextTimer,
                        prevTimer );

    fmReleaseLock( &task->lock );

    /* free the data objects */
    fmFree( timer->name );
    fmFree( timer );

    status = FM_OK;

ABORT:
    return status;

}   /* end fmDeleteTimer */



 
/*****************************************************************************/
/** fmStartTimer
 * \ingroup intTimer
 *
 * \desc            Starts a timer.
 * 
 * \note            If the timer is already running, it is initially stopped
 *                  then restarted with the new parameters.
 * 
 * \param[in]       handle is the handle associated with this timer.
 * 
 * \param[in]       timeout is a pointer to a caller allocated object
 *                  containing the timeout expressed in terms of relative time
 *                  from the current time.
 * 
 * \param[in]       nrRepetitions is the number of repetitions. Each
 *                  repetition is intended to last ''timeout''.
 * 
 * \param[in]       timerCallback is the callback function invoked at the end
 *                  of each repetition.
 * 
 * \param[in]       callbackArg is the argument for the callback function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmStartTimer( fm_timerHandle    handle, 
                        fm_timestamp     *timeout,
                        fm_int            nrRepetitions,
                        fm_timerCallback  timerCallback,
                        void             *callbackArg )
{
    fm_status      status;
    fm_timer      *timer;
    fm_timer      *firstActiveTimer;
    fm_timerTask  *task;
    fm_timestamp   curTime;
    fm_bool        timerLockTaken = FALSE;

    /* check the input args */
    if ( handle == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* double-check that the handle represents a valid timer object */
    timer = (fm_timer *)handle;
    if ( timer->magicNumber != TIMER_MAGIC_NUMBER )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* check the arguments here */
    if ( ( timeout       == NULL                                          ) || 
         ( timerCallback == NULL                                          ) ||
         ( nrRepetitions <= 0 && nrRepetitions != FM_TIMER_REPEAT_FOREVER ) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }
                                
    /* if the timer is already running, stop it then proceed */
    if ( timer->running == TRUE )
    {
        StopTimer( timer );
    }

    task = timer->task;

    /* if the timer task hasn't completed initialization, wait */
    if ( task->initialized == FALSE )
    {
        status = fmWaitSemaphore( &task->sem, FM_WAIT_FOREVER );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* grab this timer task's lock to preoceed */
    status = fmCaptureLock( &task->lock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    timerLockTaken = TRUE;

    /* the timeout is relative, so get the current time */
    status = fmGetTime( &curTime );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );

    /* populate the timer data structure */
    timer->running            = TRUE;
    timer->nrRepetitionsSoFar = 0;
    timer->nrRepetitions      = nrRepetitions;
    timer->callback           = timerCallback;
    timer->arg                = callbackArg;
    timer->start              = curTime;
    timer->timeout            = *timeout;
    timer->end                = curTime;
    fmAddTimestamps( &timer->end, &timer->timeout );

    /* add it to the list of active timers for this timer task */
    status = AddActiveTimerToTask( task, timer );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );

    /* For an event-driven timer task, signal the condition to the main loop */
    if ( task->mode == FM_TIMER_TASK_MODE_EVENT_DRIVEN )
    {
        /* we do it only if it's the first active timer in hte list */
        firstActiveTimer = FM_DLL_GET_FIRST( task, firstActiveTimer );
        if ( firstActiveTimer == timer )
        {
            /* wakeup the timer task, so that it can process the new timer event */
            status = WakeupTimerTask( task );
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
        }
    }

    status = FM_OK;

ABORT:
    if ( timerLockTaken == TRUE )
    {
        fmReleaseLock( &task->lock );
    }
    return status;

}   /* end fmStartTimer */




/*****************************************************************************/
/** fmStopTimer
 * \ingroup intTimer
 *
 * \desc            Stops a timer.
 * 
 * \param[in]       handle is the handle associated with this timer.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmStopTimer( fm_timerHandle handle )
{
    fm_timer *timer;
    fm_status status;

    /* check the input args */
    if ( handle == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* double-check that the handle represents a valid timer object */
    timer = (fm_timer *)handle;
    if ( timer->magicNumber != TIMER_MAGIC_NUMBER )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* stop the timer if it's running */
    /* exit gracefully otherwise      */
    if ( timer->running )
    {
        status = StopTimer( timer );
    }
    else
    {
        status = FM_OK;
    }

ABORT:
    return status;

}   /* end fmStopTimer */




/*****************************************************************************/
/** fmGetTimerParameters
 * \ingroup intTimer
 *
 * \desc            Returns parameters associated with an existing timer.
 * 
 * \param[in]       handle is the handle associated with this timer.
 * 
 * \param[out]      timerName is a pointer to a caller-allocated area where 
 *                  this function will return the pointer to the timer name
 *                  string.
 * 
 * \param[out]      thread is a pointer to a caller-allocated area where this
 *                  function will return the pointer to thread of the timer
 *                  task this timer is associated with.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmGetTimerParameters( fm_timerHandle   handle,
                                fm_text         *timerName,
                                void           **thread )
{
    fm_status  status;
    fm_timer  *timer;

    /* check the input args */
    if ( handle == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* double-check that the handle represents a valid timer object */
    timer = (fm_timer *)handle;
    if ( timer->magicNumber != TIMER_MAGIC_NUMBER )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    status = fmCaptureLock( &timer->task->lock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );

    *timerName = timer->name;
    *thread    = timer->task->thread;

    fmReleaseLock( &timer->task->lock );
    status = FM_OK;

ABORT:
    return status;

}   /* end fmGetTimerParameters */




/*****************************************************************************/
/** fmGetTimerState
 * \ingroup intTimer
 *
 * \desc            Returns the current state of an existing timer.
 * 
 * \param[in]       handle is the handle associated with this timer.
 * 
 * \param[out]      isTimerRunning is a pointer to a caller-allocated area
 *                  where this function will return a boolean variable
 *                  indicating whether the timer is running.
 * 
 * \param[out]      elapsedTime is a pointer to a caller-allocated area where
 *                  this function will return the time elapsed since the
 *                  beginning of the current repetition. The returned value
 *                  is undefined if the timer is currently not running.
 * 
 * \param[out]      nrRepetitionsSoFar is a pointer to a caller-allocated
 *                  area where this function will return the number of
 *                  completed repetitions. The returned value is undefined if
 *                  the timer is currently not running.
 * 
 * \param[out]      timeToGo is a pointer to a caller-allocated area where this
 *                  function will return the time remaining to the end of the
 *                  current repetition. The returned value is undefined if the
 *                  timer is currently not running.
 * 
 * \param[out]      nrRepetitionsToGo is a pointer to a caller-allocated area
 *                  where this function will return the number of repetitions
 *                  remaining including the current one. The returned value
 *                  is undefined if the timer is currently not running.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 * 
 *****************************************************************************/
fm_status fmGetTimerState( fm_timerHandle  handle,
                           fm_bool        *isTimerRunning,
                           fm_timestamp   *elapsedTime,
                           fm_int         *nrRepetitionsSoFar,
                           fm_timestamp   *timeToGo,
                           fm_int         *nrRepetitionsToGo )
{
    fm_status    status;
    fm_timer    *timer;
    fm_timestamp curTime;
    fm_bool      timerLockTaken = FALSE;

    /* check the input args */
    if ( handle == NULL )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* double-check that the handle represents a valid timer object */
    timer = (fm_timer *)handle;
    if ( timer->magicNumber != TIMER_MAGIC_NUMBER )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    status = fmCaptureLock( &timer->task->lock, FM_WAIT_FOREVER );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    timerLockTaken = TRUE;

    *isTimerRunning = timer->running;
    if ( timer->running )
    {
        status = fmGetTime( &curTime );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );

        fmSubTimestamps( &curTime, &timer->start, elapsedTime );

        if ( fmCompareTimestamps(&curTime,&timer->end) < 0 )
        {
            fmSubTimestamps( &timer->end, &curTime, timeToGo );
        }
        else
        {
            /* timer has already expired */
            timeToGo->sec  = 0;
            timeToGo->usec = 0;
        }
        
        *nrRepetitionsSoFar = timer->nrRepetitionsSoFar;
        *nrRepetitionsToGo  = timer->nrRepetitionsSoFar - timer->nrRepetitions;
    }

    status = FM_OK;

ABORT:
    if ( timerLockTaken == TRUE )
    {
        fmReleaseLock( &timer->task->lock );
    }
    return status;

}   /* end fmGetTimerState */




/*****************************************************************************/
/** fmDbgDumpTimers
 * \ingroup intTimer
 *
 * \desc            Dumps debug information about all existing timers.
 * 
 * \param[in]       onlyActive: when set to TRUE, this function will display
 *                  only active timers, otherwise it will display all timers
 *                  instantiated with ''fmCreateTimer''.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the ALOS root object hasn't been
 *                  initialized yet.
 * 
 *****************************************************************************/
fm_status fmDbgDumpTimers( fm_bool onlyActive )
{
    fm_status     status;
    fm_timerTask *task;
    fm_timer     *timer;
    fm_int        i;
    fm_int        timerCount;
    fm_char       auxStr[70];

    /* make sure the initialization sequence has been completed */
    if ( ( fmRootAlos                        == NULL ) || 
         ( fmRootAlos->timerTasksLock.handle == NULL ) ||
         ( fmRootAlos->timerTasksLock.name   == NULL ) )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* go down the list of timer tasks */
    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_TIMER_TASKS  ; i++)
    {
        task = &fmRootAlos->timerTasks[i];
        if ( task->used == TRUE )
        {
            FM_LOG_PRINT("\n+====================================================================+");
            FM_SPRINTF_S( auxStr, 
                          sizeof(auxStr), 
                          " %sTimers for Timer Task '%s'",
                          (onlyActive ? "Active " : ""),
                          task->thread->name );
            FM_LOG_PRINT( "\n|%-68s|\n", auxStr );
            PrintDbgRuler();
            FM_LOG_PRINT("|TIMER           ");
            FM_LOG_PRINT("|TIMER  ");
            FM_LOG_PRINT("|# TIMES    ");
            FM_LOG_PRINT("|# SCHEDULED ");
            FM_LOG_PRINT("|LAST              ");
            FM_LOG_PRINT("|NEXT              |\n");
            FM_LOG_PRINT("|NAME            ");
            FM_LOG_PRINT("|STATE  ");
            FM_LOG_PRINT("|EXPIRED    ");
            FM_LOG_PRINT("|EXPIRATIONS ");
            FM_LOG_PRINT("|START TIME        ");
            FM_LOG_PRINT("|EXPIRATION TIME   |\n");
            PrintDbgRuler();

            timerCount = 0;
            timer = FM_DLL_GET_FIRST( task, firstTimer );
            while ( timer != NULL )
            {

                if ( !onlyActive || timer->running )
                {
                    FM_LOG_PRINT( "|%-16s", timer->name );
                    FM_LOG_PRINT( "|%-7s", 
                                  (timer->running ? 
                                   "Active" : 
                                   "Idle") );
                    if ( timer->running )
                    {
                        FM_LOG_PRINT("|%-11d", timer->nrRepetitionsSoFar );
                        if ( timer->nrRepetitions == FM_TIMER_REPEAT_FOREVER )
                        {
                            FM_LOG_PRINT("|%-12s", "INF");
                        }
                        else
                        {
                            FM_LOG_PRINT("|%-12d", timer->nrRepetitions );
                        }
                        FM_SPRINTF_S( auxStr,
                                      sizeof(auxStr),
                                      "|%llu.%03llus",
                                      timer->start.sec, 
                                      timer->start.usec/1000 );
                        FM_LOG_PRINT("%-19s", auxStr );
                        FM_SPRINTF_S( auxStr,
                                      sizeof(auxStr),
                                      "|%llu.%03llus",
                                      timer->end.sec, 
                                      timer->end.usec/1000 );
                        FM_LOG_PRINT("%-19s|\n", auxStr);
                    }
                    else
                    {
                        FM_LOG_PRINT("|%-11s", "N/A");
                        FM_LOG_PRINT("|%-12s", "N/A");
                        FM_LOG_PRINT("|%-18s", "N/A");
                        FM_LOG_PRINT("|%-18s|\n", "N/A");
                    }
                    timerCount++;

                }   /* end if ( !onlyActive | ... ) */

                timer = FM_DLL_GET_NEXT( timer, nextTimer );

            }   /* end while ( timer != NULL ) */

            if ( timerCount == 0 )
            {
                FM_LOG_PRINT("|%-87s|\n", 
                             ( onlyActive ? 
                               "No timers currently active" : 
                               "No timers currently instantiated" ) );
            }
            PrintDbgRuler();

            FM_LOG_PRINT("\n");

        }   /* end if ( task->used ) */

    }   /* end for ( i = 0; ... ) */

    status = FM_OK;
    
ABORT:

    return status;

}   /* end fmDbgDumpTimers */




/*****************************************************************************/
/** fmDbgDumpActiveTimerList
 * \ingroup intTimer
 *
 * \desc            Dumps debug information about all existing timers in the
 *                  active list.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if the ALOS root object hasn't been
 *                  initialized yet.
 * 
 *****************************************************************************/
fm_status fmDbgDumpActiveTimerList(void)
{
    fm_status     status;
    fm_timerTask *task;
    fm_timer     *timer;
    fm_int        i;
    fm_int        timerCount;
    fm_timestamp  curTime;
    fm_char       auxStr[70];

    /* make sure the initialization sequence has been completed */
    if ( ( fmRootAlos                        == NULL ) || 
         ( fmRootAlos->timerTasksLock.handle == NULL ) ||
         ( fmRootAlos->timerTasksLock.name   == NULL ) )
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_ALOS_TIME, status );
    }

    /* go down the list of timer tasks */
    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_TIMER_TASKS  ; i++)
    {
        task = &fmRootAlos->timerTasks[i];
        if ( task->used == TRUE )
        {
            FM_LOG_PRINT("\n+====================================================================+");
            FM_SPRINTF_S( auxStr, 
                          sizeof(auxStr), 
                          " %sTimers for Timer Task '%s'Active ",
                          task->thread->name );
            FM_LOG_PRINT( "\n|%-68s|\n", auxStr );
            status = fmGetTime( &curTime );
            FM_SPRINTF_S( auxStr, 
                          sizeof(auxStr), 
                          " Current Time: %llu.%03llus",
                          curTime.sec, 
                          curTime.usec/1000 );
            FM_LOG_PRINT( "\n|%-68s|\n", auxStr );
            PrintDbgRuler();
            FM_LOG_PRINT("|TIMER           ");
            FM_LOG_PRINT("|TIMER  ");
            FM_LOG_PRINT("|# TIMES    ");
            FM_LOG_PRINT("|# SCHEDULED ");
            FM_LOG_PRINT("|LAST              ");
            FM_LOG_PRINT("|NEXT              |\n");
            FM_LOG_PRINT("|NAME            ");
            FM_LOG_PRINT("|STATE  ");
            FM_LOG_PRINT("|EXPIRED    ");
            FM_LOG_PRINT("|EXPIRATIONS ");
            FM_LOG_PRINT("|START TIME        ");
            FM_LOG_PRINT("|EXPIRATION TIME   |\n");
            PrintDbgRuler();

            timerCount = 0;
            timer = FM_DLL_GET_FIRST( task, firstActiveTimer );
            while ( timer != NULL )
            {

                /* if ( 1 ) */
                {
                    FM_LOG_PRINT( "|%-16s", timer->name );
                    FM_LOG_PRINT( "|%-7s", 
                                  (timer->running ? 
                                   "Active" : 
                                   "Idle") );
                    if ( timer->running )
                    {
                        FM_LOG_PRINT("|%-11d", timer->nrRepetitionsSoFar );
                        if ( timer->nrRepetitions == FM_TIMER_REPEAT_FOREVER )
                        {
                            FM_LOG_PRINT("|%-12s", "INF");
                        }
                        else
                        {
                            FM_LOG_PRINT("|%-12d", timer->nrRepetitions );
                        }
                        FM_SPRINTF_S( auxStr,
                                      sizeof(auxStr),
                                      "|%llu.%03llus",
                                      timer->start.sec, 
                                      timer->start.usec/1000 );
                        FM_LOG_PRINT("%-19s", auxStr );
                        FM_SPRINTF_S( auxStr,
                                      sizeof(auxStr),
                                      "|%llu.%03llus",
                                      timer->end.sec, 
                                      timer->end.usec/1000 );
                        FM_LOG_PRINT("%-19s|\n", auxStr);
                    }
                    else
                    {
                        FM_LOG_PRINT("|%-11s", "N/A");
                        FM_LOG_PRINT("|%-12s", "N/A");
                        FM_LOG_PRINT("|%-18s", "N/A");
                        FM_LOG_PRINT("|%-18s|\n", "N/A");
                    }
                    timerCount++;

                }

                timer = FM_DLL_GET_NEXT( timer, nextActiveTimer );

            }   /* end while ( timer != NULL ) */

            if ( timerCount == 0 )
            {
                FM_LOG_PRINT("|%-87s|\n", "No timers currently active");
            }
            PrintDbgRuler();

            FM_LOG_PRINT("\n");

        }   /* end if ( task->used ) */

    }   /* end for ( i = 0; ... ) */

    status = FM_OK;


ABORT:

    return status;

}   /* end fmDbgDumpActiveTimerList */

