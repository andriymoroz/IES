/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_sem.h
 * Creation Date:   2005
 * Description:     ALOS routines for dealing with semaphores abstractly
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

#ifndef __FM_FM_ALOS_SEM_H
#define __FM_FM_ALOS_SEM_H


/* Sempahore types */
/**************************************************/
/** \ingroup typeEnum
 *  When a semaphore is created, the type of
 *  semaphore must be indicated.
 **************************************************/
typedef enum
{
    /** A binary semaphore is either "full" or "empty." */
    FM_SEM_BINARY = 0,

    /** A counting semaphore holds a numeric value. */
    FM_SEM_COUNTING

} fm_semType;


/**************************************************/
/** \ingroup typeStruct
 *  Semaphore type used to abstract operating system
 *  sempahore implementations.
 **************************************************/
typedef struct _fm_semaphore
{
    /** Used internally by ALOS to hold the operating system's sempahore
     *  handle. */
    void *     handle;

    /** The type of sempahore (see 'fm_semType'). */
    fm_semType semType;

    /** Name by which to identify the semaphore. */
    fm_text    name;

} fm_semaphore;

fm_status fmCreateSemaphore(fm_text       semName,
                            fm_semType    semType,
                            fm_semaphore *semHandle,
                            fm_int        initial);
fm_status fmFindSemaphore(fm_text semName, fm_semaphore *semHandle);
fm_status fmDeleteSemaphore(fm_semaphore *semHandle);
fm_status fmCaptureSemaphore(fm_semaphore *semHandle, fm_timestamp *timeout);
fm_status fmReleaseSemaphore(fm_semaphore *semHandle);


/** \ingroup macroSynonym
 * @{ */

/** A synonym for ''fmCaptureSemaphore''. */
#define fmWaitSemaphore(h, t)  fmCaptureSemaphore( (h), (t) )

/** A synonym for ''fmReleaseSemaphore''. */
#define fmSignalSemaphore(h)   fmReleaseSemaphore( (h) )

/** @} (end of Doxygen group) */

void fmDbgDumpAllSemaphores(void);


#endif /* __FM_FM_ALOS_SEM_H */
