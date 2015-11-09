/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_alos_init_int.h
 * Creation Date:  June 18, 2007
 * Description:    Structures internal to ALOS.
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

#ifndef __FM_FM_ALOS_INIT_INT_H
#define __FM_FM_ALOS_INIT_INT_H


#define FM_ALOS_INTERNAL_RWL_CTR_MAX  24

#ifndef FM_ALOS_INTERNAL_MAX_TIMER_TASKS
#define FM_ALOS_INTERNAL_MAX_TIMER_TASKS 1
#endif 


/* "global" ALOS variables that are shared between processes */
typedef struct _fm_rootAlos
{
    /* fm_alos_lock.c */
    fm_lock             LockLock;
    fm_lock *           LockList[FM_ALOS_INTERNAL_MAX_LOCKS];

    /* fm_alos_rwlock.c */
    fm_lock             dbgRwLockListLock;
    fm_rwLock *         dbgRwLockList[FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS];
    fm_lock             rwLockDebugCounterLock;
    int                 rwLockDebugCounters[FM_ALOS_INTERNAL_RWL_CTR_MAX];

    /* fm_alos_sem.c */
    pthread_mutex_t     dbgAccessLock;
    fm_semaphore *      dbgSemaphoreList[FM_ALOS_INTERNAL_MAX_SEMAPHORES];

    /* fm_alos_logging.c */
    fm_loggingState     fmLoggingState;

    /* common/fm_property.c */
    fm_lock             propertyLock;
    fm_property         property;
#if defined(FM_SUPPORT_FM10000)
    fm10000_property    fm10000_property;
#endif

    /* Mask of non-switch-specific locks that have a precedence. */
    fm_lockPrecedence   nonSwitchLockPrecs;

    /* Table of Dynamically-Loaded Libraries. (fm_alos_dynamic_load.c). */
    fm_dynLoadLib *     dlLibs[FM_ALOS_INTERNAL_DYN_LOAD_LIBS];

    /* Lock to control access to the dlLibs table. */
    fm_lock             dlAccessLock;

    /* common/fm_tree.c */
    fm_tree             treeTree;
    pthread_mutex_t     treeTreeLock;

    /* fm_alos_time.c */
    fm_lock             timerTasksLock;
    fm_int              nrTimerTasks;
    fm_timerTask        timerTasks[FM_ALOS_INTERNAL_MAX_TIMER_TASKS];

} fm_rootAlos;

extern fm_rootAlos *      fmRootAlos;

/* Information about threads, which is private to each process */
typedef struct _fm_alosThreadState
{
    /** Tree mapping pthread_t to fm_thread* for the threads in this process */
    fm_tree         dbgThreadTree;

    /** Mutex for access to dbgThreadTree */
    pthread_mutex_t threadTreeLock;

    /** thread to hold main process info */
    fm_thread       mainThread;

    /** Indicates that this data structure has been initialized */
    fm_bool         initialized;
    
    fm_int          foreignThreadCount;

} fm_alosThreadState;

extern fm_alosThreadState fmAlosThreadState;

/* internal initialization functions */
fm_status fmAlosLockInit(void);
fm_status fmAlosRwlockInit(void);
fm_status fmAlosSemInit(void);
fm_status fmAlosThreadInit(void);
fm_status fmAlosLoggingInit(void);
fm_status fmAlosTimeInit(void);

#define GET_PROPERTY()  (&fmRootAlos->property)
#define GET_FM10000_PROPERTY()  (&fmRootAlos->fm10000_property)

#endif /* __FM_FM_ALOS_INIT_INT_H */
