/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_lock.h
 * Creation Date:   2005
 * Description:     ALOS routines for dealing with locks abstractly
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

#ifndef __FM_FM_ALOS_LOCK_H
#define __FM_FM_ALOS_LOCK_H


/* May be used as an argument to fmCreateLockV2. */
#define FM_LOCK_SWITCH_NONE                 -1

typedef fm_uint32 fm_lockPrecedence;


/**************************************************/
/** \ingroup intTypeStruct
 *  Lock type that abstracts operating system
 *  implementations for ensuring mutually exclusive
 *  access to system resources (mutexes).
 **************************************************/
typedef struct _fm_lock
{
    /** Used internally by ALOS to hold the operating system's mutex handle. */
    void *              handle;

    /** Name by which to identify the lock. */
    fm_text             name;
    
    /** Switch with which the lock is associated. Will be zero for locks
     *  not associated with any switch (this field is used to
     *  track lock inversions, not for unambiguously identifying the
     *  associated switch). */
    fm_int              switchNumber;
    
    /** A singleton bit mask that represents the precedence for this lock.
     *  ''fmCaptureLock'' will enforce the order in which locks may be
     *  captured by a thread. Locks with lower precedence may not be
     *  captured after locks with higher precedence. This mechanism is
     *  used to prevent lock inversion between two threads. */
    fm_lockPrecedence   precedence;
    
    /** Used internally by ALOS to hold the number of times the lock has been 
     *  taken by a thread. Incremented on capture, decremented on release. 
     *  Used to know when it is time to remove the lock's precedence from the 
     *  thread's lock collection. */
    fm_uint             takenCount;
    
    /** Used internally by ALOS to hold the thread ID of owner. */
    void *              owner;

} fm_lock;


extern fm_status fmCreateLock(fm_text lockName, fm_lock *lck);
extern fm_status fmCreateLockV2(fm_text lockName, 
                                fm_int  sw,
                                fm_int  precedence,
                                fm_lock *lck);
extern fm_status fmDeleteLock(fm_lock *lck);

extern fm_status fmCaptureLock(fm_lock *lck, fm_timestamp *timeout);
extern fm_status fmReleaseLock(fm_lock *lck);
extern fm_status fmIsLockTaken(fm_lock *lck, fm_bool *isTaken);

extern void fmDbgDumpLocks(void);

extern fm_status fmDbgTakeLock(fm_int       sw,
                               fm_lock *    lockPtr,
                               fm_int       tryTime,
                               fm_int       numTries,
                               const char * function);

void fmDbgDumpLockPrecMask(fm_uint64         logCat, 
                           fm_uint64         logLevel, 
                           fm_lockPrecedence precMask);

fm_status fmGetLockPrecedence(fm_lock *lck, fm_int *precedence);

#endif /* __FM_FM_ALOS_LOCK_H */
