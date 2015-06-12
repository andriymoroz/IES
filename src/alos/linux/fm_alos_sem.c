/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_sem.c
 * Creation Date:   2005
 * Description:     Linux-native specific functions for dealing with semaphores
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

/* type wrapper for a binary semaphore */
typedef struct
{
    fm_bool value;
    sem_t   sem;

} fm_binary_semaphore;

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static void fmDbgAddSemaphore(fm_semaphore *sem);
static void fmDbgDelSemaphore(fm_semaphore *sem);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

static fm_status fmDbgSemaphoreInitialize(pthread_mutexattr_t *attr)
{
    int ret;

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG, "attr=%p\n", (void *) attr);

    ret = pthread_mutex_init(&fmRootAlos->dbgAccessLock, attr);

    if (ret != 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_ERR_LOCK_INIT);
    }

    FM_CLEAR(fmRootAlos->dbgSemaphoreList);

    FM_LOG_EXIT(FM_LOG_CAT_DEBUG, FM_OK);

}   /* end fmDbgSemaphoreInitialize */




static void fmDbgAddSemaphore(fm_semaphore *sem)
{
    int i;

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG, "sem=%p\n", (void *) sem);

    i = pthread_mutex_lock(&fmRootAlos->dbgAccessLock);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_lock\n",
                     i);
    }

    for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_SEMAPHORES ; i++)
    {
        if (fmRootAlos->dbgSemaphoreList[i] == NULL)
        {
            fmRootAlos->dbgSemaphoreList[i] = sem;
            break;
        }
    }

    i = pthread_mutex_unlock(&fmRootAlos->dbgAccessLock);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
    }

    FM_LOG_PRINTF(FM_LOG_CAT_DEBUG, FM_LOG_LEVEL_FUNC_EXIT, "Exiting...\n");

}   /* end fmDbgAddSemaphore */




static void fmDbgDelSemaphore(fm_semaphore *sem)
{
    int i;

    FM_LOG_ENTRY(FM_LOG_CAT_DEBUG, "sem=%p\n", (void *) sem);

    i = pthread_mutex_lock(&fmRootAlos->dbgAccessLock);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_lock\n",
                     i);
    }

    for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_SEMAPHORES ; i++)
    {
        if (fmRootAlos->dbgSemaphoreList[i] == sem)
        {
            fmRootAlos->dbgSemaphoreList[i] = NULL;
            break;
        }
    }

    i = pthread_mutex_unlock(&fmRootAlos->dbgAccessLock);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
    }

    FM_LOG_PRINTF(FM_LOG_CAT_DEBUG, FM_LOG_LEVEL_FUNC_EXIT, "Exiting...\n");

}   /* end fmDbgDelSemaphore */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAlosSemInit
 * \ingroup intAlosSem
 *
 * \desc            Initializes the ALOS semaphore facility.
 *
 * \return          FM_OK if successful
 *
 **********************************************************************/
fm_status fmAlosSemInit(void)
{
    fm_status           err;
    pthread_mutexattr_t attr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_SEM, "(no arguments)\n");

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_UNINITIALIZED);
    }

    if ( pthread_mutexattr_init(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        pthread_mutexattr_destroy(&attr);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_LOCK_INIT);
    }

    err = fmDbgSemaphoreInitialize(&attr);

    if ( pthread_mutexattr_destroy(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_LOCK_INIT);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, err);

}   /* end fmAlosSemInit */




/*****************************************************************************/
/** fmCreateSemaphore
 * \ingroup alosSem
 *
 * \desc            Create a binary or counting semaphore. Generally used for
 *                  for resource protection or inter-thread signaling.
 *                  A binary semaphore has two states, captured and released.
 *                  A counting semaphore is initialized to some count value. 
 *                  As the semaphore is released, the count is incremented.  
 *                  As the semaphore is captured, it is decremented. If it 
 *                  reaches zero, it cannot be captured again until released 
 *                  at least once.
 *                                                                      \lb\lb
 *                  A semaphore is captured by calling ''fmCaptureSemaphore'' 
 *                  and released by calling ''fmReleaseSemaphore''.
 *
 * \param[in]       semName is a string by which the sempahore can be
 *                  identified.
 *
 * \param[in]       semType identifies the type of semaphore to be
 *                  created (see 'fm_semType').
 *
 * \param[out]      semHandle is a pointer to caller-allocated memory where
 *                  this function should place the created semaphore's
 *                  control structure.
 *
 * \param[in]       initial is the initial state of the semaphore.  If
 *                  semType is ''FM_SEM_BINARY'', initial may be 0 to signify
 *                  "empty" or 1 to signify "full."  If semType is
 *                  ''FM_SEM_COUNTING'', initial should be the starting count
 *                  value. Negative values are not required by the API and
 *                  may not be supported by ALOS.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if invalid argument
 * \return          FM_ERR_NO_MEM if memory allocation error
 * \return          FM_FAIL if general failure
 *
 **********************************************************************/
fm_status fmCreateSemaphore(fm_text       semName,
                            fm_semType    semType,
                            fm_semaphore *semHandle,
                            fm_int        initial)
{
    fm_binary_semaphore *bsem;
    sem_t *              csem;
    int                  ret;
    char                 strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t              strErrNum;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_SEM, "name=%s type=%d handle=%p initial=%d\n",
                 semName, semType, (void *) semHandle, initial);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_UNINITIALIZED);
    }

    if (!semHandle)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR(*semHandle);

    switch (semType)
    {
        /**************************************************
         * Create a binary sempahore.
         **************************************************/

        case FM_SEM_BINARY:

            /* Make sure initial is in valid range for binary sem. */
            if (initial != 0 && initial != 1)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);
            }


            bsem = (fm_binary_semaphore *) fmAlloc(
                sizeof(fm_binary_semaphore) );

            if (!bsem)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_NO_MEM);
            }

            bsem->value = (initial == 1);

            ret = sem_init(&bsem->sem, 1, 0);

            if (ret)
            {
                fmFree(bsem);
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);
            }

            semHandle->handle = bsem;
            break;

            /**************************************************
             * Create a counting sempahore.
             **************************************************/

        case FM_SEM_COUNTING:
            csem = (sem_t *) fmAlloc( sizeof(sem_t) );

            if (!csem)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_NO_MEM);
            }

            ret   = sem_init(csem, 1, initial);

            if (ret)
            {
                fmFree(csem);
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);
            }

            semHandle->handle = csem;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);

    }   /* end switch(semType) */

    /**************************************************
     * Check if successful creation and export to
     * O/S-agnostic structure.
     **************************************************/

    if (ret < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_SEM, "sem_init failed with '%s' for %s\n",
                         strErrBuf, semName);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_SEM, "sem_init failed with '%d' for %s\n",
                         errno, semName);
        }
        if (semHandle->handle != NULL)
        {
            sem_destroy(semHandle->handle);
            fmFree(semHandle->handle);
            semHandle->handle = NULL;
        }
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
    }

    semHandle->semType = semType;
    semHandle->name    = fmStringDuplicate(semName);

    if (semHandle->name == NULL)
    {
        sem_destroy(semHandle->handle);
        fmFree(semHandle->handle);
        semHandle->handle = NULL;
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_NO_MEM);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_ALOS_SEM, "Semaphore %s created\n", semName);

    fmDbgAddSemaphore(semHandle);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_OK);

}   /* end fmCreateSemaphore */




/*****************************************************************************/
/** fmFindSemaphore
 * \ingroup intAlosSem
 *
 * \desc            Locate a semaphore by name (not implemented).
 *
 *                  This function is may be used for diagnostic purposes.
 *
 * \param[in]       semName name to lookup.
 *
 * \param[out]      semHandle is a pointer to where this function should
 *                  place the address of the located semaphore structure.
 *
 * \return          FM_OK if successful
 * \return          FM_FAIL if sempahore not found
 *
 **********************************************************************/
fm_status fmFindSemaphore(fm_text semName, fm_semaphore *semHandle)
{
    FM_NOT_USED(semName);
    FM_NOT_USED(semHandle);
    return FM_ERR_UNSUPPORTED;

}   /* end fmFindSemaphore */




/*****************************************************************************/
/** fmDeleteSemaphore
 * \ingroup alosSem
 *
 * \desc            Delete a semphore previously allocated by a call to
 *                  fmCreateSemaphore.
 *
 *\param[in]        semHandle is a pointer to the semaphore to be operated
 *                  on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid argument.
 * \return          FM_FAIL if general failure.
 *
 **********************************************************************/
fm_status fmDeleteSemaphore(fm_semaphore *semHandle)
{
    fm_binary_semaphore *bsem;
    int                  ret;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_SEM, "handle=%p\n", (void *) semHandle);

    if (!semHandle)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);
    }

    /* we must hold the semaphore before we can destroy it */
    if (fmCaptureSemaphore(semHandle, FM_WAIT_FOREVER) != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
    }

    fmDbgDelSemaphore(semHandle);

    switch (semHandle->semType)
    {
        case FM_SEM_BINARY:
            bsem = (fm_binary_semaphore *) semHandle->handle;

            ret = sem_destroy(&bsem->sem);

            if (ret != 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
            }

            break;

        case FM_SEM_COUNTING:

            ret = sem_destroy( (sem_t *) semHandle->handle );

            if (ret != 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
            }

            break;

    }   /* end switch (semHandle->semType) */

    /* Delete the sempahore. */
    fmFree(semHandle->name);
    fmFree(semHandle->handle);
    semHandle->name   = NULL;
    semHandle->handle = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_OK);

}   /* end fmDeleteSemaphore */




/*****************************************************************************/
/** fmCaptureSemaphore
 * \ingroup alosSem
 *
 * \desc            Capture a semaphore. 
 *                                                                      \lb\lb
 *                  If the semphore is a binary semaphore, this function will
 *                  block unless or until the semaphore is released with a
 *                  call to ''fmReleaseSemaphore'' (typically from another
 *                  thread).
 *                                                                      \lb\lb
 *                  If the semaphore is a counting semaphore, this function
 *                  will decrement the semaphore's count, unless the count
 *                  is zero, in which case it will block. The fuction will
 *                  return when the count is incremented with a call to 
 *                  ''fmReleaseSemaphore'' (typically from another thread).
 *
 * \note            ''fmWaitSemaphore'' is a synonym for this function that
 *                  may be more intuitive for binary semaphores used for
 *                  inter-task signaling.
 *
 * \param[in]       semHandle is a pointer to the semaphore to be operated
 *                  on.
 *
 * \param[in]       timeout is the period of time to wait for the semaphore
 *                  as a number of seconds and microseconds from the present
 *                  time.  May also be specified as FM_WAIT_FOREVER or
 *                  FM_NO_WAIT.
 *
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if invalid argument
 * \return          FM_ERR_SEM_TIMEOUT timeout waiting for sempahore
 * \return          FM_FAIL if general failure
 *
 **********************************************************************/
fm_status fmCaptureSemaphore(fm_semaphore *semHandle, fm_timestamp *timeout)
{
    fm_binary_semaphore *bsem;
    sem_t *              csem;
    int                  count = 0;
    int                  ret;
    struct timespec      ts;
    struct timeval       ct;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_SEM,
                 "handle=%p timeout=%p\n",
                 (void *) semHandle, (void *) timeout);

    if (timeout != FM_WAIT_FOREVER)
    {
        gettimeofday(&ct, NULL);
        ts.tv_sec  = ct.tv_sec + timeout->sec;
        ts.tv_nsec = (ct.tv_usec + timeout->usec) * 1000;
        /* nsec overflow */
        while (ts.tv_nsec >= 1000000000)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        } 
    }

    if (!semHandle)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_INVALID_ARGUMENT);
    }

    switch (semHandle->semType)
    {
        case FM_SEM_BINARY:
            bsem = (fm_binary_semaphore *) semHandle->handle;

            /**************************************************
             * The binary semaphore implementation has to take
             * extra care to provide the semantics we require,
             * namely that multiple signals result in only one
             * wait.  In order to do so our approach is to use
             * sem_trywait to flush out the posting queue.
             **************************************************/

            while (1)
            {
                /**************************************************
                 * sem_trywait will return -1 if it would block, and
                 * EAGAIN if the semaphore is already held.  Any
                 * other situation resulting in a non-zero return
                 * code is a fatal error in which case we just
                 * return FM_FAIL.
                 **************************************************/
                ret = sem_trywait(&bsem->sem);

                if (ret == -1)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    break;
                }
                else if (ret != 0)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
                }

                /**************************************************
                 * The counter is used to keep track of how many
                 * times we succesfully captured the semaphore.
                 * Consider the following cases:
                 *
                 * Case I:
                 *   The semaphore has not previously been
                 *   posted to.  In this case, the first call to
                 *   sem_trywait returns -1 indicating that it
                 *   would block, which breaks out of the loop
                 *   before the counter is incremented.  In
                 *   this case, count is 0.
                 *
                 * Case II:
                 *   The semaphore had 1 previous post to it.  In
                 *   this case, the first call to sem_trywait
                 *   succeeds, and count is incremented.  The
                 *   second call to sem_trywait returns -1
                 *   and we break out of the loop.
                 *
                 * Case III:
                 *   The semaphore had multiple posts to it.  This
                 *   is actually a general case that extends II.
                 *   The only difference is that count will be
                 *   incremented for each time a post occurred.
                 *   So when the final call to sem_trywait returns
                 *   -1 and we break out, count contains the
                 *   number of posts to the semaphore.
                 **************************************************/
                count++;
            }

            if (count)
            {
                /**************************************************
                 * A positive value of count indicates that
                 * sem_trywait succeeded at least once and we
                 * can return.
                 **************************************************/
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_OK);
            }
            else
            {
                /**************************************************
                 * Otherwise we must call the blocking form sem_wait
                 * (or its timed variant).
                 **************************************************/
                while (1)
                {
                    if (timeout == FM_WAIT_FOREVER)
                    {
                        ret = sem_wait(&bsem->sem);
                    }
                    else
                    {
                        ret = sem_timedwait(&bsem->sem, &ts);
                    }

                    if (ret == 0)
                    {
                        break;
                    }

                    if (ret == -1)
                    {
                        if (errno == EINTR)
                        {
                            continue;
                        }

                        if (errno == ETIMEDOUT)
                        {
                            FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM,
                                        FM_ERR_SEM_TIMEOUT);
                        }
                    }

                    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
                }
            }

            break;

        case FM_SEM_COUNTING:
            csem = (sem_t *) semHandle->handle;

            while (1)
            {
                if (timeout == FM_WAIT_FOREVER)
                {
                    ret = sem_wait(csem);
                }
                else
                {
                    ret = sem_timedwait(csem, &ts);
                }

                if (ret == 0)
                {
                    break;
                }

                if (ret == -1)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    if (errno == ETIMEDOUT)
                    {
                        FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_ERR_SEM_TIMEOUT);
                    }
                }

                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
            }

            break;

    }   /* end switch (semHandle->semType) */

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_OK);

}   /* end fmCaptureSemaphore */




/*****************************************************************************/
/** fmReleaseSemaphore
 * \ingroup alosSem
 *
 * \desc            Release a semaphore.
 *                                                                      \lb\lb
 *                  If a call to ''fmCaptureSemaphore'' is blocked on the
 *                  semaphore (in another thread), that function will return.
 *                  
 *                  If there is not a blocked call to ''fmCaptureSemaphore'',
 *                  the semaphore will be primed to not block on the next
 *                  call to ''fmCaptureSemaphore''.
 *                                                                      \lb\lb
 *                  If the semaphore is a counting semaphore, this function
 *                  will increment the semaphore's count.
 *
 * \note            ''fmSignalSemaphore'' is a synonym for this function that
 *                  may be more intuitive for binary semaphores used for
 *                  inter-task signaling.
 *
 * \param[in]       semHandle is a pointer to the semaphore to be operated
 *                  on.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if invalid argument.
 * \return          FM_FAIL if general failure.
 *
 **********************************************************************/
fm_status fmReleaseSemaphore(fm_semaphore *semHandle)
{
    fm_binary_semaphore *bsem;
    sem_t *              csem;
    int                  ret;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_SEM, "handle=%p\n", (void *) semHandle);

    if (!semHandle)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (semHandle->semType)
    {
        case FM_SEM_BINARY:
            bsem = (fm_binary_semaphore *) semHandle->handle;

            ret = sem_post(&bsem->sem);

            if (ret != 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
            }

            break;

        case FM_SEM_COUNTING:
            csem = (sem_t *) semHandle->handle;

            ret = sem_post(csem);

            if (ret != 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_FAIL);
            }

            break;

    }   /* end switch (semHandle->semType) */

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_SEM, FM_OK);

}   /* end fmReleaseSemaphore */




/*****************************************************************************/
/** fmDbgDumpAllSemaphores
 * \ingroup diagMisc
 *
 * \desc            Display the status of all currently allocated semaphores.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpAllSemaphores(void)
{
    int    i;

    /*
     *  Do NOT take the dbgAccessLock before doing this!  If somehow that lock
     *  has deadlocked, it would prevent us from displaying the status!
     *  It is possible we will have to not take the printf lock as well,
     *  if that turns out to be a deadlock problem.
     */
    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_SEMAPHORES ; i++)
    {
        if (fmRootAlos->dbgSemaphoreList[i] != NULL)
        {
            FM_LOG_PRINT("Semaphore Name      : %s\n",
                         fmRootAlos->dbgSemaphoreList[i]->name);

            FM_LOG_PRINT("Semaphore Type      : %s\n",
                         (fmRootAlos->dbgSemaphoreList[i]->semType == FM_SEM_BINARY)
                            ? "FM_SEM_BINARY" : "FM_SEM_COUNTING");
        }
    }

}   /* end fmDbgDumpAllSemaphores */
