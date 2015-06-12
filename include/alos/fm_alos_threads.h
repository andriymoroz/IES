/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_threads.h
 * Creation Date:   2005
 * Description:     Implementation of ALOS thread handling
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

#ifndef __FM_FM_ALOS_THREADS_H
#define __FM_FM_ALOS_THREADS_H


/* macros for dealing with thread arguments */
#define FM_GET_THREAD_HANDLE(arg)       ( (fm_thread *) ( ( (void **) arg )[0] ) )
#define FM_GET_THREAD_PARAM(type, arg)  ( (type *) ( ( (void **) arg )[1] ) )


/**************************************************/
/** \ingroup typeScalar
 * A function that serves as the entry point of
 * a thread created by fmCreateThread. This function
 * takes as an argument a pointer to an array of
 * two pointers. The first is an fm_thread object and
 * the second is an arbitrary void pointer which can
 * be set to anything.
 **************************************************/
typedef void *(*fm_threadBody)(void *);


/**************************************************/
/** \ingroup intTypeStruct
 * Thread type used to abstract operating system
 * thread implementations.
 **************************************************/
typedef struct _fm_thread
{
    /** Used internally by ALOS to hold the operating system's thread handle. */
    void *              handle;

    /** Event queue from which this thread will receive event messages. */
    fm_eventQueue       events;

    /** Used internally by ALOS to hold the operating system's condition
     *  variable handle (Linux only). */
    void *              cond;

    /** Used internally by ALOS to hold a lock on which the thread will block
     *  waiting for events to appear in its receive queue. */
    /*  lock for waiting on events. */
    fm_lock             waiter;

    /** Name of the thread for identification. */
    fm_text             name;

    /** Used internally by ALOS, a 2-element array used to hold arguments
     *  that will be passed to the thread upon invocation. The operating
     *  system will pass the address of this array to the thread as an
     *  argv.  The first element will contain the address of this fm_thread
     *  structure so that the thread can find its own management structure.
     *  The second element will contain a thread-specific argument that
     *  is specified in the call to fmCreateThread. */
    void *              arguments[2];

    /** Pointer to the thread's entry point function (not used). */
    fm_threadBody       threadFunc;

} fm_thread;


extern fm_status fmCreateThread(fm_text       threadName,
                                fm_int        eventQueueSize,
                                fm_threadBody threadFunc,
                                void *        threadArg,
                                fm_thread *   thread);


/* called from within a thread to cleanup */
extern fm_status fmExitThread(fm_thread *thread);


/* (blocks) sends an event */
extern fm_status fmSendThreadEvent(fm_thread *thread,
                                   fm_event * event);


/* (blocks) grabs an event off the queue */
extern fm_status fmGetThreadEvent(fm_thread *   thread,
                                  fm_event **   eventPtr,
                                  fm_timestamp *timeout);


/* (unblocking) looks at the next event in queue */
extern fm_status fmPeekThreadEvent(fm_thread *thread,
                                   fm_event **eventPtr);


/* signals a thread event handler */
extern fm_status fmSignalThreadEventHandler(fm_thread *thread);


/* suspends execution of the calling thread until the target thread terminates */
extern fm_status fmWaitThreadExit(fm_thread *thread);


/* yields control to other threads */
extern fm_status fmYield(void);


/* for dealing with process ids */
extern fm_int fmGetCurrentProcessId(void);


/* for dealing with thread ids */
extern void *fmGetCurrentThreadId(void);
extern fm_bool fmThreadIdsEqual(void *a, void *b);


/* for getting the current thread's name */
extern char *fmGetCurrentThreadName(void);

/* for getting a thread's name given a thread ID */
extern fm_status fmGetThreadName(void *threadId, fm_text *threadName);

/* Get a copy of thread's state */
extern fm_status fmGetThreadState(void *             threadHandle,
                                  fm_text            threadNameBfr,
                                  fm_int             nameBfrLength);

extern fm_lockPrecedence *fmGetCurrentThreadLockCollection(void);

/* Diagnostics */
extern fm_status fmDbgRegisterThread(fm_text threadName);
extern fm_status fmDbgUnregisterThread(void);



#endif /* __FM_FM_ALOS_THREADS_H */
