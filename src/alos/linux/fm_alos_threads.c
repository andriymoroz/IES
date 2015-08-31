/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_threads.c
 * Creation Date:   2005
 * Description:     OS abstraction for creating and managing threads.
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


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/* Process-local information about the threads in this process. */
fm_alosThreadState fmAlosThreadState;

/**************************************************
 * Thread local storage key used by the lock
 * inversion defense.
 *
 * Note that this variable cannot be in fm_rootAlos
 * because each process needs its own.
 **************************************************/

pthread_key_t fmTLSKeyLock;


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmGetCurrentThreadState
 * \ingroup intAlosTask
 *
 * \desc            Get the current thread's state.
 *
 * \param[in]       None.
 *
 * \return          Pointer to current thread's state, or NULL if error.
 *
 *****************************************************************************/
static fm_thread *fmGetCurrentThreadState(void)
{
    fm_status   err;
    pthread_t   handle;
    fm_thread * thread;

    if (!fmAlosThreadState.initialized)
    {
        return NULL;
    }

    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) != 0)
    {
        return NULL;
    }

    handle = (pthread_t) fmGetCurrentThreadId();

    err = fmTreeFind(&fmAlosThreadState.dbgThreadTree,
                     (fm_uint64) handle,
                     (void *) &thread);

    if (err != FM_OK)
    {
        thread = NULL;
    }

    if (pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock) != 0)
    {
        thread = NULL;
    }

    return thread;

}   /* end fmGetCurrentThreadState */




/*****************************************************************************/
/** fmGetThreadStateCopy
 * \ingroup intAlosTask
 *
 * \desc            Get a thread's state.
 *
 * \param[in]       handle is the thread ID.
 *
 * \param[in]       thread points to a caller-allocated fm_thread structure
 *                  into which this function should copy the thread state.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if thread not found.
 * \return          FM_ERR_UNINITIALIZED if thread subsystem not initialized.
 *
 *****************************************************************************/
static fm_status fmGetThreadStateCopy(pthread_t handle, fm_thread *thread)
{
    fm_status  err;
    fm_thread *foundThread;

    if (!fmAlosThreadState.initialized)
    {
        return FM_ERR_UNINITIALIZED;
    }

    err = FM_FAIL;

    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0)
    {
        err = fmTreeFind(&fmAlosThreadState.dbgThreadTree,
                         (fm_uint64) handle,
                         (void *) &foundThread);
        
        if (err == FM_OK)
        {
            /* struct copy */
            *thread = *foundThread;
        }
        else
        {
            err = FM_FAIL;
        }

        if (pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock) != 0)
        {
            err = FM_FAIL;
        }
    }

    return err;

}   /* end fmGetThreadStateCopy */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmAlosThreadInit
 * \ingroup intAlosTask
 *
 * \desc            Called by ''fmOSInitialize'' to register the calling 
 *                  process. The process's name is recorded so that it can be 
 *                  included in log messages that are generated later.
 *
 * \param[in]       None.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if unable to register process.
 *
 *****************************************************************************/
fm_status fmAlosThreadInit(void)
{
    pthread_t handle = (pthread_t) fmGetCurrentThreadId();
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "(no arguments)\n");

    /* Initialize a tree for the calling process. */
    fmTreeInitWithAllocator(&fmAlosThreadState.dbgThreadTree,
                            (fmAllocFunc) malloc,
                            free);

    /* Default attributes are fine-- doesn't need to be process-shared,
     * doesn't need to be recursive. */
    if ( pthread_mutex_init(&fmAlosThreadState.threadTreeLock, NULL) )
    {
        return FM_FAIL;
    }

    /***************************************************
     * Register the main process name
     *
     * Note: program_invocation_short_name is a GNU
     * extension and may not be available on all
     * operating systems.
     **************************************************/

    fmAlosThreadState.mainThread.name = program_invocation_short_name;

    err = fmTreeInsert(&fmAlosThreadState.dbgThreadTree,
                       (fm_uint64) handle,
                       &fmAlosThreadState.mainThread);

    if (err == FM_OK)
    {
        fmAlosThreadState.initialized = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmAlosThreadInit */




/*****************************************************************************/
/** fmCreateThread
 * \ingroup alosTask
 *
 * \desc            Create a thread.
 *
 * \note            Threads are never completely cleaned up.  (Even if you
 *                  call fmExitThread.)  Therefore, threads should be
 *                  created on startup and live forever.  Constantly
 *                  creating new threads will result in a memory leak.
 *
 * \param[in]       threadName is a text string used to identify the thread.
 *                  This string is used for diagnostic purposes only.
 *
 * \param[in]       eventQueueSize is the number of event messages that can
 *                  be held by the thread's event queue.
 *
 * \param[in]       threadFunc is a pointer to the thread's entry point
 *                  function.
 *
 * \param[in]       threadArg is an argument that will be passed to the
 *                  thread's entry point function.
 *
 * \param[out]      thread points to the caller-allocated fm_thread structure.
 *                  The structure will be filled in by this function. The
 *                  API uses this structure to maintain housekeeping information
 *                  for the thread; the application need never be concerned
 *                  with its contents.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if thread does not point to a
 *                  fm_thread structure.
 * \return          FM_ERR_NO_MEM if unable to allocate memory for the thread.
 * \return          FM_FAIL if unable to create a lock for the thread.
 * \return          FM_ERR_UNABLE_TO_CREATE_THREAD if operating system thread
 *                  creation failed.
 *
 *****************************************************************************/
fm_status fmCreateThread(fm_text       threadName,
                         fm_int        eventQueueSize,
                         fm_threadBody threadFunc,
                         void *        threadArg,
                         fm_thread *   thread)
{
    fm_status           err;
    pthread_attr_t      threadAttributes;
    struct sched_param  schedulingAttributes;
    pthread_condattr_t  attr;
    int                 posixError;
    fm_bool             threadAttrInit;
    char                lockNameBuf[30+1];
    fm_text             lockNamePtr;
    char                strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t             strErrNum;
    fm_bool             lockInit;
    fm_bool             queueInit;
    fm_bool             condInit;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD,
                 "name=%s qs=%d argument=%p handle=%p\n",
                 threadName, eventQueueSize,
                 (void *) threadArg, (void *) thread);

    if (!fmAlosThreadState.initialized)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_UNINITIALIZED);
    }

    if (!thread)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(*thread);

    posixError  = 0;
    lockInit    = FALSE;
    queueInit   = FALSE;
    condInit    = FALSE;

    /* allocate a new pthread_t object */
    thread->handle  = (pthread_t *) fmAlloc( sizeof(pthread_t) );
    thread->cond    = (pthread_cond_t *) fmAlloc( sizeof(pthread_cond_t) );
    thread->name    = fmStringDuplicate(threadName);

    if (!thread->handle || !thread->name || !thread->cond)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    FM_SPRINTF_S(lockNameBuf, sizeof(lockNameBuf), "%s.wait", threadName);
    lockNamePtr = (lockNameBuf[0]) ? lockNameBuf : threadName;

    /* create waiting lock */
    err = fmCreateLock(lockNamePtr, &thread->waiter);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_THREAD, err);
    lockInit = TRUE;

    FM_SPRINTF_S(lockNameBuf, sizeof(lockNameBuf), "%s.event", threadName);
    lockNamePtr = (lockNameBuf[0]) ? lockNameBuf : threadName;

    /* set up the event queue */
    err = fmEventQueueInitialize(&thread->events, eventQueueSize, lockNamePtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ALOS_THREAD, err);
    queueInit = TRUE;

    /* setup arguments */
    thread->arguments[0] = thread;
    thread->arguments[1] = threadArg;

    /* Make the conditional variable process shared. */
    if ( pthread_condattr_init(&attr) != 0 )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_THREAD,
                     "Failed to initialize condition variable\n");
        err = FM_ERR_UNABLE_TO_CREATE_COND;
        goto ABORT;
    }

    if ( pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0 )
    {
        pthread_condattr_destroy(&attr);
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_THREAD,
                     "Failed to initialize condition variable\n");
        err = FM_ERR_UNABLE_TO_CREATE_COND;
        goto ABORT;
    }

    /* initialize condition var */
    if ( ( posixError = pthread_cond_init(thread->cond, &attr) ) != 0 )
    {
        pthread_condattr_destroy(&attr);
        err = FM_ERR_UNABLE_TO_CREATE_COND;
        goto ABORT;
    }
    condInit = TRUE;

    pthread_condattr_destroy(&attr);

    /***************************************************
     * Schedule this thread under SCHED_RR and at min 
     * priority to insure that application threads do 
     * not preempt this thread.
     **************************************************/
     
    schedulingAttributes.sched_priority = sched_get_priority_min(SCHED_RR); 

    posixError = pthread_attr_init(&threadAttributes);
    if (posixError != 0)
    {
        threadAttrInit = FALSE;
    }
    else
    {
        threadAttrInit = TRUE;
    }

#ifdef FM_ALOS_PTHREAD_STACK_SIZE
    if (posixError == 0)
    {
        posixError = pthread_attr_setstacksize(&threadAttributes,
                                               FM_ALOS_PTHREAD_STACK_SIZE);
    }
#endif

    if (posixError == 0)
    {
        posixError = pthread_attr_setschedpolicy(&threadAttributes,
                                                 SCHED_RR);
    }

    if (posixError == 0)
    {
        posixError = pthread_attr_setschedparam(&threadAttributes,
                                                &schedulingAttributes);
    }

    if (posixError == 0)
    {
        posixError = pthread_create( (pthread_t *) thread->handle,
                                     &threadAttributes,
                                     threadFunc,
                                     thread->arguments );
    }

    if (threadAttrInit)
    {
        if (posixError == 0)
        {
            posixError = pthread_attr_destroy(&threadAttributes);
        }
        else
        {
            pthread_attr_destroy(&threadAttributes);
        }
    }

    if (posixError != 0)
    {
        err = FM_ERR_UNABLE_TO_CREATE_THREAD;
        goto ABORT;
    }

    /* store it in the debug list */
    err = FM_FAIL;

    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0)
    {
        err = fmTreeInsert(&fmAlosThreadState.dbgThreadTree,
                           (fm_uint64) * ( (pthread_t *) thread->handle ),
                           thread);

        if (pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock))
        {
            FM_ERR_COMBINE(err, FM_FAIL);
        }
    }

    if (err == FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_OK);
    }

ABORT:

    if (posixError != 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, posixError);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR( FM_LOG_CAT_ALOS_THREAD,
                          "Failed to create thread '%s' - %s\n",
                          threadName,
                          strErrBuf );
        }
        else
        {
            FM_LOG_ERROR( FM_LOG_CAT_ALOS_THREAD,
                          "Failed to create thread '%s' - %d\n",
                          threadName,
                          posixError );
        }
    }

    if (condInit)
    {
        posixError = pthread_cond_destroy(thread->cond);
        if (posixError != 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                         "Error %d calling pthread_cond_destroy\n",
                         posixError);
        }
    }

    if (queueInit)
    {
        fmEventQueueDestroy(&thread->events);
    }

    if (lockInit)
    {
        fmDeleteLock(&thread->waiter);
    }

    if (thread->name)
    {
        fmFree(thread->name);
    }

    if (thread->cond)
    {
        fmFree(thread->cond);
    }

    if (thread->handle)
    {
        fmFree(thread->handle);
    }

    FM_CLEAR(*thread);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmCreateThread */




/*****************************************************************************/
/** fmExitThread
 * \ingroup alosTask
 *
 * \desc            Called by a thread that is exiting to deallocate itself.
 *
 * \note            This function terminates the current thread and
 *                  deallocates some of the resources used by it, but
 *                  it does not deallocate all the resources used by
 *                  the thread.  Threads are expected to live for the
 *                  lifetime of the application.
 *
 * \param[in]       thread points to the thread's associated fm_thread
 *                  structure that was filled in by ''fmCreateThread''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if thread does not point to an
 *                  fm_thread structure.
 *
 *****************************************************************************/
fm_status fmExitThread(fm_thread *thread)
{
    int posixErr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "thread=%p\n", (void *) thread);

    if (!thread)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    /* remove this thread from ALOS's thread tree */
    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0)
    {

        fmTreeRemove( &fmAlosThreadState.dbgThreadTree,
                      (fm_uint64) * ( (pthread_t *) thread->handle ),
                      NULL );

        posixErr = pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock) ;
        if (posixErr != 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                         "error %d from pthread_mutex_unlock\n",
                         posixErr);
        }
        
    }

    /* cleanup all the events */
    fmEventQueueDestroy(&thread->events);

    /* get rid of locks, conditional vars, etc. */
    fmDeleteLock(&thread->waiter);
    posixErr = pthread_cond_destroy(thread->cond);
    if (posixErr != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "error %d from pthread_cond_destroy\n",
                     posixErr);
    }

    /* Free memory that was allocated when the thread was created */
    fmFree(thread->handle);
    fmFree(thread->cond);
    fmFree(thread->name);

    thread->handle = NULL;

    pthread_exit(NULL);

    /* unreachable */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_OK);

}   /* end fmExitThread */




/*****************************************************************************/
/** fmDbgRegisterThread
 * \ingroup diagAlos
 *
 * \desc            Register a "foreign" thread with ALOS for diagnostic
 *                  purposes. A foreign thread is one that is not created
 *                  by calling ''fmCreateThread'', but from which calls to the
 *                  API will be made. Use of this function is optional and
 *                  and only serves to provide a method for identifying the
 *                  thread with a meaningful name when displayed as part of
 *                  diagnostic messages.
 *
 * \note            Threads registered with this function should not call
 *                  ''fmExitThread''.
 *
 * \note            All foreign threads are expected to be using the same
 *                  underlying threading library that ''fmCreateThread'' uses,
 *                  which on Linux, for example, is pthreads.
 *
 * \param[in]       threadName is a text string used to identify the thread.
 *                  This string is used for diagnostic purposes only.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if an error occurs while trying to register
 *                  the thread.
 * \return          FM_ERR_ALREADY_EXISTS if the thread is registered.
 * \return          FM_ERR_NO_MEM if no memory is available for allocating
 *                  the new thread object.
 * \return          FM_ERR_UNINITIALIZED if the threading subsystem is not
 *                  yet initialized (''fmOSInitialize'' has not been called).
 *
 *****************************************************************************/
fm_status fmDbgRegisterThread(fm_text threadName) 
{
    fm_status err;
    fm_thread *thread;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "threadName=%s\n", threadName); 
    
    err = fmAddForeignThreadToList(threadName, &thread);
    
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmDbgRegisterThread */




/*****************************************************************************/
/** fmDbgUnregisterThread
 * \ingroup diagAlos
 *
 * \desc            Unregister a "foreign" thread previously registered with
 *                  ''fmDbgRegisterThread''.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if calling thread not recognized.
 * \return          FM_FAIL if an error occurs while trying to unregister
 *                  the thread.
 * \return          FM_ERR_UNINITIALIZED if the threading subsystem is not
 *                  yet initialized (''fmOSInitialize'' has not been called).
 *
 *****************************************************************************/
fm_status fmDbgUnregisterThread(void)
{
    fm_status err;
    fm_thread *thread;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "\n"); 
    
    /* Confirm subsystem initialized (fmGetCurrentThreadState does
     * not disambiguate this condition). */
    if (!fmAlosThreadState.initialized)
    {
        return FM_ERR_UNINITIALIZED;
    }
    
    /* Get this thread's record. */
    thread = fmGetCurrentThreadState();
    
    if (!thread)
    {
        /* Thread is not in the thread list. */
        return FM_ERR_NOT_FOUND;
    }
    
    /* Remove the thread from the thread list */
    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0)
    {
        err = fmTreeRemoveCertain(&fmAlosThreadState.dbgThreadTree, 
                                  (fm_uint64) * ( (pthread_t *) thread->handle ), 
                                  NULL);
        
        if (err != FM_OK)
        {
            /* Couldn't remove thread from list. */
            return FM_FAIL;
        }
        
        if (pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock) != 0)
        {
            /* Couldn't unlock list. */
            err = FM_FAIL;
        }
    }
    else
    {
        /* Couldn't lock list. */
        return FM_FAIL;
        
    }   /* end if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0) */
    

    /* Deallocate the thread's record. */
    fmFree(thread->name);
    fmFree(thread->handle);
    fmFree(thread);
    
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmDbgUnregisterThread */




/*****************************************************************************/
/** fmSendThreadEvent
 * \ingroup alosTask
 *
 * \desc            Send an event to a thread, optionally after a specified
 *                  delay.
 *
 * \param[in]       thread points to the target thread's associated fm_thread
 *                  structure that was filled in by ''fmCreateThread''.
 *
 * \param[in]       event points to the caller-allocated event to be sent to
 *                  the thread.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either thread or event do not
 *                  point to valid objects.
 * \return          FM_ERR_EVENT_QUEUE_FULL if the thread's event queue is
 *                  full.
 * \return          FM_ERR_UNABLE_TO_LOCK if unable to access the thread's
 *                  event queue.
 * \return          FM_ERR_BAD_GETTIME if unable to get the current time for
 *                  providing a delay.
 *
 *****************************************************************************/
fm_status fmSendThreadEvent(fm_thread *thread,
                            fm_event * event)
{
    int       i;
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "thread=%p event=%p\n",
                 (void *) thread, (void *) event);

    if (!thread || !event)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    /* Put event on thread's event queue. */
    if ( ( err = fmEventQueueAdd(&thread->events, event) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);
    }

    /* Lock before calling pthread_cond_signal */
    i = pthread_mutex_lock( (pthread_mutex_t *) thread->waiter.handle );
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "Error %d from pthread_mutex_lock\n",
                     i);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_UNABLE_TO_LOCK);
    }

    if ( pthread_cond_signal(thread->cond) )
    {
        err = FM_ERR_UNABLE_TO_SIGNAL_COND;
    }
    else
    {
        err = FM_OK;
    }

    i = pthread_mutex_unlock( (pthread_mutex_t *) thread->waiter.handle );
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
        if (err == FM_OK)
        {
            err = FM_ERR_UNABLE_TO_UNLOCK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmSendThreadEvent */




/*****************************************************************************/
/** fmGetThreadEvent
 * \ingroup alosTask
 *
 * \desc            Called by a thread to pull the event at the head of its
 *                  event queue.  If the queue is empty, this function will
 *                  block until an event becomes available.
 *
 * \param[in]       thread points to the thread's associated fm_thread
 *                  structure that was filled in by ''fmCreateThread''.
 *
 * \param[out]      eventPtr points to caller-allocated storage where this
 *                  function should place the address of the event pulled
 *                  from the queue. The caller must deallocate a successfully
 *                  retrieved event when it is done processing the event.
 *
 * \param[in]       timeout points to an ''fm_timestamp'' structure that
 *                  contains the time (in seconds/microseconds) to wait for
 *                  an event before giving up. If timeout is FM_WAIT_FOREVER
 *                  (NULL), this function will block until an event is
 *                  available without timing out. If timeout points to a
 *                  structure that indicates zero seconds/microseconds, this
 *                  function will return without waiting for an event if none
 *                  are currently available.
 *
 * \return          FM_OK if successful (an event was received).
 * \return          FM_ERR_INVALID_ARGUMENT if thread or eventPtr are invalid.
 * \return          FM_ERR_NO_EVENTS_AVAILABLE if timeout expired.
 * \return          FM_ERR_UNABLE_TO_LOCK if unable to access thread's queue.
 *
 *****************************************************************************/
fm_status fmGetThreadEvent(fm_thread *   thread,
                           fm_event **   eventPtr,
                           fm_timestamp *timeout)
{
    fm_status       err;
    fm_timestamp    ct;
    struct timespec ts;
    int             i;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "thread=%p event=%p timeout=%p\n",
                 (void *) thread, (void *) eventPtr, (void *) timeout);

    if (!thread || !eventPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    /***************************************************
     * We use this mutex to protect the condition variable.
     * Since we can't guarantee that signal state caches
     * on condition variables, we will lock right here
     * before we check whether the queue has events.
     **************************************************/
    i = pthread_mutex_lock( (pthread_mutex_t *) thread->waiter.handle );
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "Error %d from pthread_mutex_lock\n",
                     i);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_UNABLE_TO_LOCK);
    }

    err = fmEventQueueGet(&thread->events, eventPtr);

    /* If there was an event then just return.
     * Otherwise return the error, if there was one, and
     * if not wait 'timeout' for a new event.
     */
    if (err == FM_OK)
    {
        /* Release mutex */
        i = pthread_mutex_unlock( (pthread_mutex_t *) thread->waiter.handle );
        if (i != 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                         "Error %d from pthread_mutex_unlock\n",
                         i);
        }

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_OK);
    }

    if (err != FM_ERR_NO_EVENTS_AVAILABLE)
    {
        /* Release mutex */
        i = pthread_mutex_unlock( (pthread_mutex_t *) thread->waiter.handle );
        if (i != 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                         "Error %d from pthread_mutex_unlock\n",
                         i);
        }

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);
    }

    if (timeout == FM_WAIT_FOREVER)
    {
        /* NULL timeout value indicates wait forever. */
        i = pthread_cond_wait( (pthread_cond_t *) thread->cond,
                               (pthread_mutex_t *) thread->waiter.handle );
        if (i != 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                         "Error %d calling pthread_cond_wait\n",
                         i);
        }
    }
    else
    {
        /* If timeout is specified. Is it "no wait?" */
        if (timeout->sec == 0 && timeout->usec == 0)
        {
            /* Don't wait. Just release mutex and return. */
            i = pthread_mutex_unlock( (pthread_mutex_t *) thread->waiter.handle );
            if (i != 0)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                             "Error %d from pthread_mutex_unlock\n",
                             i);
            }

            FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_NO_EVENTS_AVAILABLE);
        }
        else
        {
            /* Wait time specified. pthread_cond_timedwait requires an
             * absolute time to handle the timeout properly, so we need
             * to get the current time first and then append the relative
             * timeout value passed in. */
            fmGetTime(&ct);

            /* convert to timespec */
            ts.tv_sec  = ct.sec + timeout->sec;
            ts.tv_nsec = (ct.usec + timeout->usec) * 1000;

            /* handle overflow of nsec */
            while (ts.tv_nsec >= 1000000000)
            {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }

            /* timed wait until either timeout or signal */
            i = pthread_cond_timedwait( (pthread_cond_t *) thread->cond,
                                        (pthread_mutex_t *) thread->waiter.handle,
                                        &ts );
            if (i != 0)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                             "Error %d from pthread_cond_timedwait\n",
                             i);
            }

        }   /* end if (timeout->sec == 0 && timeout->usec == 0) */

    }       /* end if (timeout == FM_WAIT_FOREVER) */

    /* Release mutex */
    i = pthread_mutex_unlock( (pthread_mutex_t *) thread->waiter.handle );
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
    }

    /* Now try getting the next event. Return error regardless of type. */
    return fmEventQueueGet(&thread->events, eventPtr);

}   /* end fmGetThreadEvent */




/*****************************************************************************/
/** fmPeekThreadEvent
 * \ingroup intAlosTask
 *
 * \desc            Called by a thread to determine if there is an event at the
 *                  head of its event queue.  Does not block.
 *
 * \param[in]       thread points to the thread's associated fm_thread
 *                  structure that was filled in by ''fmCreateThread''.
 *
 * \param[out]      eventPtr points to caller-allocated storage where this
 *                  function should place the address of the event at the head
 *                  of the queue. Note that the event will remain in the
 *                  thread's queue until ''fmGetThreadEvent'' is called, so the
 *                  caller to this function should not attempt to deallocate
 *                  the returned event.
 *
 * \return          FM_OK if successful (an event was received).
 * \return          FM_ERR_INVALID_ARGUMENT if thread or eventPtr are invalid.
 * \return          FM_ERR_NO_EVENTS_AVAILABLE if event queue is empty.
 * \return          FM_ERR_UNABLE_TO_LOCK if unable to access thread's queue.
 *
 *****************************************************************************/
fm_status fmPeekThreadEvent(fm_thread *thread,
                            fm_event **eventPtr)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "thread=%p event=%p\n",
                 (void *) thread, (void *) eventPtr);

    if (!thread || !eventPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    if (thread->events.size)
    {
        return fmEventQueuePeek(&thread->events, eventPtr);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_NO_EVENTS_AVAILABLE);

}   /* end fmPeekThreadEvent */





/*****************************************************************************/
/** fmSignalThreadEventHandler
 * \ingroup intAlosTask
 *
 * \desc            Signal an event handler but do not queue an event.
 *
 * \param[in]       thread points to the target thread's associated fm_thread
 *                  structure that was filled in by ''fmCreateThread''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if thread does not point to
 *                  a valid object.
 * \return          FM_ERR_UNABLE_TO_SIGNAL_COND if the thread's condition
 *                  variable could not be signalled.
 *
 *****************************************************************************/
fm_status fmSignalThreadEventHandler(fm_thread *thread)
{
    fm_status err;
    int       i;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "thread=%p\n",
                 (void *) thread);

    if (!thread)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    /* Lock before calling pthread_cond_signal */
    i = pthread_mutex_lock( (pthread_mutex_t *) thread->waiter.handle );
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "Error %d from pthread_mutex_lock\n",
                     i);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_UNABLE_TO_LOCK);
    }

    if ( pthread_cond_signal(thread->cond) )
    {
        err = FM_ERR_UNABLE_TO_SIGNAL_COND;
    }
    else
    {
        err = FM_OK;
    }

    i = pthread_mutex_unlock( (pthread_mutex_t *) thread->waiter.handle );
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_THREAD,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
        if (err == FM_OK)
        {
            err = FM_ERR_UNABLE_TO_UNLOCK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmSignalThreadEventHandler */



/*****************************************************************************/
/** fmWaitThreadExit
 * \ingroup intAlosTask
 *
 * \desc            Suspends execution of the calling thread until the target
 *                  thread terminates, unless the target thread has already
 *                  terminated.
 *
 * \param[in]       thread points to the target thread's associated fm_thread
 *                  structure that was filled in by ''fmCreateThread''.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmWaitThreadExit(fm_thread *thread)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "thread=%p\n",
                 (void *) thread);

    if (!thread)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_ERR_INVALID_ARGUMENT);
    }

    if (pthread_join(*((pthread_t *) thread->handle), NULL) != 0)
    {
        err = FM_FAIL;
    }
    else
    {
        err = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmWaitThreadExit */



/*****************************************************************************/
/** fmGetCurrentProcessId
 * \ingroup alosTask
 *
 * \desc            Get the current process's process ID.
 *
 * \param           None
 *
 * \return          The process ID.
 *
 *****************************************************************************/
fm_int fmGetCurrentProcessId(void)
{
    return (fm_int) getpid();

}   /* end fmGetCurrentProcessId */




/*****************************************************************************/
/** fmGetCurrentThreadId
 * \ingroup intAlosTask
 *
 * \desc            Get the current thread's thread ID.
 *
 * \param           None.
 *
 * \return          The caller's thread ID.
 *
 *****************************************************************************/
void *fmGetCurrentThreadId()
{
    return (void *) pthread_self();

}   /* end fmGetCurrentThreadId */




/*****************************************************************************/
/** fmThreadIdsEqual
 * \ingroup intAlosTask
 *
 * \desc            Determine if two thread IDs are equal.
 *
 * \param[in]       a is one thread ID.
 *
 * \param[in]       b is the other thread ID.
 *
 * \return          TRUE if the thread IDs are equal.
 * \return          FALSE if the thread IDs are not equal.
 *
 *****************************************************************************/
fm_bool fmThreadIdsEqual(void *a, void *b)
{
    return pthread_equal( (pthread_t) a, (pthread_t) b ) != 0;

}   /* end fmThreadIdsEqual */




/*****************************************************************************/
/** fmGetCurrentThreadName
 * \ingroup intAlosTask
 *
 * \desc            Get the current thread's name.  Note that the name may be
 *                  NULL if called from a thread who was not created via ALOS.
 *
 * \param           None
 *
 * \return          The thread name or NULL if not found.
 *
 *****************************************************************************/
char *fmGetCurrentThreadName()
{
    fm_thread *thread;

    thread = fmGetCurrentThreadState();
    
    if (thread)
    {
        return thread->name;
    }

    return NULL;

}   /* end fmGetCurrentThreadName */




/*****************************************************************************/
/** fmYield
 * \ingroup alosTask
 *
 * \desc            Yields control to other threads.
 *
 * \param[in]       None.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
fm_status fmYield(void)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "(no arguments)\n");

    sched_yield();

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_OK);

}   /* end fmYield */


/*****************************************************************************/
/** fmGetThreadState
 * \ingroup intAlosTask
 *
 * \desc            Retrieve a copy of certain state variables for the
 *                  specified thread. A copy is returned instead of a pointer
 *                  to the actual state for two reasons:                    \lb
 *                  1. So we don't need to put a lock around access to the
 *                  state in case the thread is destroyed while the caller is
 *                  trying to access its state.                             \lb
 *                  2. So the caller need not be cognizant of the fm_thread
 *                  structure.
 *
 * \param[in]       threadHandle is the thread ID whose state is to be
 *                  retrieved.
 *
 * \param[out]      threadNameBfr points to a caller-allocated buffer where
 *                  this function should place the name of the thread.
 *
 * \param[in]       nameBfrLength is the length of threadNameBfr (including
 *                  space for a NULL terminator.
 *
 * \return          
 *
 *****************************************************************************/
fm_status fmGetThreadState(void *             threadHandle,
                           fm_text            threadNameBfr,
                           fm_int             nameBfrLength)
{
    fm_status err;
    fm_int    copySize;
    fm_thread thread = 
    {
        /* Initialization required to prevent compiler warning. */
        .name           = NULL,
    };
    
    err = fmGetThreadStateCopy( (pthread_t) threadHandle, &thread);
    
    if (err == FM_OK)
    {
        /* Copy thread name. */
        copySize = strlen(thread.name) + 1;
        
        if (copySize >= nameBfrLength)
        {
            copySize = nameBfrLength - 1;
        }
        
        FM_MEMCPY_S(threadNameBfr, nameBfrLength, thread.name, copySize);
        
        threadNameBfr[copySize] = '\0';
        
    }
    
    return err;
    
}   /* end fmGetThreadState */




/*****************************************************************************/
/** fmGetThreadName
 * \ingroup alosTask
 *
 * \desc            Given a thread ID, returns the thread's name.
 *
 * \param[in]       threadId is the thread ID (also known as the thread handle).
 *
 * \param[out]      threadName points to caller-allocated storage into which
 *                  the pointer to the thread name will be copied.  Note that
 *                  the caller MUST not modify the text of the name.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if threadId is not a valid thread
 *                  handle.
 * \return          FM_ERR_UNABLE_TO_LOCK if the needed lock could not be taken.
 * \return          FM_ERR_UNABLE_TO_UNLOCK if the lock couldn't be released.
 *
 *****************************************************************************/
fm_status fmGetThreadName(void *threadId, fm_text *threadName)
{
    fm_status  err;
    fm_thread *thread;
    pthread_t  handle;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "threadId=%p\n", threadId);

    handle = (pthread_t) threadId;

    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0)
    {
        err = fmTreeFind( &fmAlosThreadState.dbgThreadTree,
                         (fm_uint64) handle,
                         (void **) &thread );

        if (err == FM_OK)
        {
            *threadName = thread->name;
        }

        if (pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock) != 0)
        {
            FM_ERR_COMBINE(err, FM_ERR_UNABLE_TO_UNLOCK);
        }
    }
    else
    {
        err = FM_ERR_UNABLE_TO_LOCK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, err);

}   /* end fmGetThreadName */




/*****************************************************************************/
/** fmDestructLockCollection
 * \ingroup intAlosTask
 *
 * \desc            Will be called upon thread exit as a result of having
 *                  registered this function with pthread_key_create in 
 *                  ''fmInitThreadLockCollection''.
 *
 * \param[in]       collection is the thread local storage value, which will
 *                  be the address of the array of lock collections per-switch.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDestructLockCollection(void *collection)
{
    if (collection)
    {
        fmFree(collection);
    }
    
}   /* end fmDestructLockCollection */




/*****************************************************************************/
/** fmInitThreadLockCollection
 * \ingroup intAlosInit
 *
 * \desc            Initialize thread local storage for holding each thread's
 *                  held lock collection mask.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmInitThreadLockCollection(void)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_THREAD, "(no arguments)\n");
    
    if (pthread_key_create(&fmTLSKeyLock, fmDestructLockCollection) != 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_THREAD, FM_OK);
    
}   /* end fmInitThreadLockCollection */




/*****************************************************************************/
/** fmGetCurrentThreadLockCollection
 * \ingroup intAlosTask
 *
 * \desc            Returns a copy of the thread's lock collection.
 *
 * \param           None.
 *
 * \return          The lock collection
 *
 *****************************************************************************/
fm_lockPrecedence *fmGetCurrentThreadLockCollection(void)
{
    void *collection;
    
    /** A bit mask of all locks held by this thread. Used by ''fmCaptureLock''
     *  to enforce the order in which locks are taken as a mechanism for
     *  preventing lock inversion (and a subsequent deadlock) between
     *  threads. */
    collection = pthread_getspecific(fmTLSKeyLock);
    
    if (!collection)
    {
        /**************************************************
         * We haven't yet allocated the collection, for this
         * thread, so do it now.
         **************************************************/
        collection = fmAlloc( sizeof(fm_lockPrecedence) * FM_MAX_NUM_SWITCHES ); 
        
        if (collection)
        {
            FM_MEMSET_S( collection,
                         sizeof(fm_lockPrecedence) * FM_MAX_NUM_SWITCHES,
                         0,
                         sizeof(fm_lockPrecedence) * FM_MAX_NUM_SWITCHES );
            
            if (pthread_setspecific(fmTLSKeyLock, collection) != 0)
            {
                fmFree(collection);
                collection = NULL;
                FM_LOG_FATAL(FM_LOG_CAT_ALOS_THREAD, 
                             "Unable to write lock collection to thread-local "
                             "storage!\n");
                FM_LOG_CALL_STACK(FM_LOG_CAT_ALOS_THREAD, FM_LOG_LEVEL_FATAL);
            }
        }
        else
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS_THREAD,
                         "Unable to allocate memory for thread's "
                         "lock collection!\n");
            FM_LOG_CALL_STACK(FM_LOG_CAT_ALOS_THREAD, FM_LOG_LEVEL_FATAL);
        }
        
    }
    
    return ( (fm_lockPrecedence *) collection );
    
}   /* end fmGetCurrentThreadLockCollection */




/*****************************************************************************/
/** fmAddForeignThreadToList
 * \ingroup intAlosTask
 *
 * \desc            Add the current thread to the list of threads for
 *                  identification.
 *
 * \note            This function must not be called if the thread is
 *                  already in the thread list.
 *
 * \param[in]       threadName is a text string that will be used to
 *                  identify the thread for diagnostic purposes only.
 *
 * \param[out]      threadPtr is a pointer to a pointer to an ''fm_thread''
 *                  structure that will be filled in by this function with the 
 *                  address of a freshly allocated thread. The caller may 
 *                  access this structure at will but should not free it.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if error while inserting thread into list.
 * \return          FM_ERR_ALREADY_EXISTS if the thread is already in the list.
 * \return          FM_ERR_NO_MEM if no memory available to allocate for
 *                  the new thread.
 * \return          FM_ERR_UNINITIALIZED if the threading subsystem is not
 *                  yet initialized.
 *
 *****************************************************************************/
fm_status fmAddForeignThreadToList(fm_text threadName, 
                                   fm_thread **threadPtr)
{
    fm_status  err;
    fm_thread *thread;

    thread = fmGetCurrentThreadState();
    
    if (thread)
    {
        return FM_ERR_ALREADY_EXISTS;
    }
    
    /**************************************************
     * This is a "foreign" thread - one that was not
     * registered via a call to fmInitialize or created
     * with fmCreateThread.
     *
     * Build a thread structure for this thread and
     * register it in the tree of threads.
     *
     * Note that once this thread structure is allocated,
     * it is never freed because there currently is no
     * exit mechanism in the API.
     **************************************************/
     
    if (!fmAlosThreadState.initialized)
    {
        return FM_ERR_UNINITIALIZED;
    }

    thread = fmAlloc( sizeof(fm_thread) );
    
    if (!thread)
    {
        return FM_ERR_NO_MEM;
    }
    
    FM_CLEAR(*thread);
    
    thread->handle = (pthread_t *) fmAlloc( sizeof(pthread_t) );

    /* Make sure we got a handle. */
    if (!thread->handle)
    {
        fmFree(thread);
        return FM_ERR_NO_MEM;
    }
    
    *(pthread_t *) thread->handle = (pthread_t) fmGetCurrentThreadId();
    thread->name = fmStringDuplicate(threadName);
    
    /* Make sure we copied name. */
    if (!thread->name)
    {
        fmFree(thread->handle);
        fmFree(thread);
        return FM_ERR_NO_MEM;
    }
    
    /* Store it in the thread list */
    if (pthread_mutex_lock(&fmAlosThreadState.threadTreeLock) == 0)
    {
        err = fmTreeInsert(&fmAlosThreadState.dbgThreadTree,
                           (fm_uint64) * ( (pthread_t *) thread->handle ),
                           thread);

        if (pthread_mutex_unlock(&fmAlosThreadState.threadTreeLock) != 0 ||
            err != FM_OK)
        {
            fmFree(thread->name);
            fmFree(thread->handle);
            fmFree(thread);
            return FM_FAIL;
        }
    }
    else
    {
        fmFree(thread->name);
        fmFree(thread->handle);
        fmFree(thread);
        return FM_FAIL;
        
    }   /* end if (fmAlosThreadState.initialized && ... */
    
    *threadPtr = thread;
    return FM_OK;
    
}   /* end fmAddForeignThreadToList */





