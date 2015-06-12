/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_lock.c
 * Creation Date:   September 16, 2005
 * Description:     Provide lock (mutex) functionality.  A lock provides
 *                  mutually exclusive access to a resource by multiple
 *                  threads.  If a particular thread already has the lock
 *                  and attempts to get it again, it will succeed. If another
 *                  thread attempts to get the lock, it will be blocked until
 *                  the first thread releases it.
 *
 * Copyright (c) 2006 - 2014, Intel Corporation
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

#define MAX_THREAD_NAME_LENGTH                  128

#if FM_LOCK_INVERSION_DEFENSE
#define VALIDATE_LOCK_PRECEDENCE(err, lck, take)    \
                    err = ValidateLockPrecedence(lck, take);
#else
#define VALIDATE_LOCK_PRECEDENCE(err, lck, take)    \
                    err = FM_OK;
#endif
    

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

#if FM_LOCK_INVERSION_DEFENSE 
/*****************************************************************************/
/** ValidateLockPrecedence
 * \ingroup intAlosLock
 *
 * \desc            Validate a lock transaction (capture or release) by
 *                  comparing the precedence of the lock being taken or
 *                  released with other locks held by the thread. Used to 
 *                  defend against lock inversion between threads.
 *
 * \note            When this function is called on a capture, the lock is 
 *                  not yet owned by the thread (unless this is a nested 
 *                  capture), so the lock's takenCount cannot be accessed.
 *                  However, when called on a release, the lock is still 
 *                  owned by the thread, so the takenCount can be accessed.
 *                  The takenCount is not changed in this function.
 *
 * \param[in]       lck points to the lock state.
 *
 * \param[in]       take should be TRUE when testing for a capture, FALSE when
 *                  testing for a release.
 *
 * \return          FM_OK if lock transaction is valid.
 * \return          FM_FAIL if lock transaction is out of order.
 *
 *****************************************************************************/
static fm_status ValidateLockPrecedence(fm_lock *lck, fm_bool take)
{
    fm_lockPrecedence *threadsLocks;
    fm_lockPrecedence  precWithoutThisLock;
    fm_lockPrecedence  thisLocksPrec;
    fm_uint32          origVerbosity;
    
    /**************************************************
     * Not all locks have a precedence. For those that
     * don't, let lock be captured/released regardless
     * of any other locks being held by this thread.
     **************************************************/
    
    if (lck->precedence == 0)
    {
        return FM_OK;
    }

    threadsLocks = fmGetCurrentThreadLockCollection();
    
    if (!threadsLocks)
    {
        /* Should never happen! */
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_LOCK,
                     "Could not get lock collection for thread %p!\n",
                     (void *)fmGetCurrentThreadId());
        return FM_FAIL;
    }
    
    threadsLocks = &threadsLocks[lck->switchNumber];
    
    thisLocksPrec       = lck->precedence;
    precWithoutThisLock = *threadsLocks & ~thisLocksPrec;
    
    /**************************************************
     * Handle nested capture or release.
     **************************************************/
    
    if (take)
    {
        /**************************************************
         * If this thread already holds this lock and
         * is taking it again, let it pass regardless of
         * any higher precedence locks that have been
         * taken in the meantime. This is not an
         * inversion since this lock had to have already 
         * been taken by this thread in the proper order.
         **************************************************/
        
        if (*threadsLocks & thisLocksPrec)
        {
            return FM_OK;
        }
        
        /**************************************************
         * If this is a switch-specific lock and it is not
         * the switch lock, make sure we have taken the 
         * switch lock first!
         **************************************************/
         
        if ( ( (thisLocksPrec & fmRootAlos->nonSwitchLockPrecs) == 0) &&
             ( thisLocksPrec ^ (1 << FM_LOCK_PREC_SWITCH) ) )
        {
            /* We're taking a switch-specific lock that is not the switch lock.
             * Make sure we've already taken the switch lock. */
            if ( ( *threadsLocks & (1 << FM_LOCK_PREC_SWITCH) ) == 0)
            {
                /**************************************************
                 * Switch lock hasn't been taken!
                 **************************************************/
                
                FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                             "SWITCH LOCK NOT TAKEN: Attempted to take lock "
                             "'%s' (precedence = 0x%X) without first taking "
                             "the switch lock on switch %d.\n",
                             lck->name,
                             lck->precedence,
                             lck->switchNumber);
                FM_LOG_CALL_STACK(FM_LOG_CAT_ALOS_RWLOCK, FM_LOG_LEVEL_ERROR);
                
            }   /* end if ( ( threadsLocks & (1 << FM_LOCK_PREC_SWITCH) )... */
            
        }   /* end if ( ( (thisLocksPrec & fmRootAlos->nonSwitchLockPrecs) ... */
    }
    else if (lck->takenCount > 1)
    {
        /**************************************************
         * This is not the last (un-nested) release, let
         * it pass since we may have done a nested take
         * after taking a higher precedence lock.
         **************************************************/
        
        return FM_OK;        
    }

    /**************************************************
     * Check for lock being taken out of order.
     **************************************************/

    if (precWithoutThisLock > thisLocksPrec)
    {
        /* Precedence violation! */
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "LOCK INVERSION: Attempted to %s lock '%s' (precedence = "
                     "0x%X) out of order on switch %d. Locks held: 0x%X (",
                     take ? "take" : "release",
                     lck->name,
                     lck->precedence,
                     lck->switchNumber,
                     *threadsLocks);
        fmGetLoggingVerbosity(&origVerbosity);
        fmSetLoggingVerbosity(FM_LOG_VERBOSITY_NONE);
        fmDbgDumpLockPrecMask(FM_LOG_CAT_ALOS_LOCK, 
                              FM_LOG_LEVEL_ERROR,
                              *threadsLocks);
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, ")\n");
        fmSetLoggingVerbosity(origVerbosity);
        FM_LOG_CALL_STACK(FM_LOG_CAT_ALOS_RWLOCK, FM_LOG_LEVEL_ERROR);
    }

    if (take)
    {
        /* We're taking the lock, so add it to the collection. */
        *threadsLocks |= thisLocksPrec;
    }
    else
    {
        if (lck->takenCount == 1)
        {
            /* We're releasing the lock for the last time, so take it out 
             * of the thread's collection. */
            *threadsLocks = precWithoutThisLock;
        }
    }

    return FM_OK;

}   /* end ValidateLockPrecedence */

#endif  /* FM_LOCK_INVERSION_DEFENSE */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/* This is a private function which should only be called from fmAlosRootInit */
fm_status fmAlosLockInit(void)
{
    pthread_mutexattr_t attr;
    int                 pterr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_LOCK, "(no arguments)\n");

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNINITIALIZED);
    }

    fmRootAlos->LockLock.handle =
        (pthread_mutex_t *) fmAlloc( sizeof(pthread_mutex_t) );

    if (fmRootAlos->LockLock.handle == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_NO_MEM);
    }

    if ( pthread_mutexattr_init(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_INIT);
    }
    else if ( pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_INIT);
    }
    else if ( pthread_mutex_init( (pthread_mutex_t *) fmRootAlos->LockLock.handle,
                                  &attr ) )
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_INIT);
    }
    else if ( pthread_mutexattr_destroy(&attr) )
    {
        pterr = pthread_mutex_destroy( (pthread_mutex_t *) fmRootAlos->LockLock.handle );
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_INIT);
    }

    fmRootAlos->LockLock.name = fmStringDuplicate("LockLock");

    if (fmRootAlos->LockLock.name == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_NO_MEM);
    }

    FM_CLEAR(fmRootAlos->LockList);
    fmRootAlos->LockList[0] = &(fmRootAlos->LockLock);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);

}   /* end fmAlosLockInit */




/*****************************************************************************/
/** fmCreateLock
 * \ingroup alosLock
 *
 * \desc            Deprecated. Use ''fmCreateLockV2''.
 *                                                                      \lb\lb
 *                  Create a critical section lock (mutex). A lock provides 
 *                  mutually exclusive access to a resource by multiple threads.  
 *                  If a particular thread already has the lock and attempts to 
 *                  get it again, it will succeed. If another thread attempts 
 *                  to get the lock, it will be blocked until the first thread 
 *                  releases it.
 *
 * \note            Locks created with this function may be taken in any order
 *                  with respect to all other locks. That is, there is no
 *                  defence against lock inversion as there is with locks
 *                  created with ''fmCreateLockV2''.
 *
 * \param[in]       lockName is an arbitrary text string used to identify the
 *                  lock for debugging purposes.
 *
 * \param[out]      lck points to a caller-allocated fm_lock object to be 
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation error.
 * \return          FM_ERR_UNINITIALIZED if ALOS not intialized.
 * \return          FM_ERR_INVALID_ARGUMENT if lck is NULL.
 * \return          FM_ERR_LOCK_INIT if unable to initialize lock.
 *
 *****************************************************************************/
fm_status fmCreateLock(fm_text lockName, fm_lock *lck)
{
    fm_status err;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_LOCK, 
                 "lockName=%s lck=%p\n",
                 lockName, 
                 (void *) lck);

    err = fmCreateLockV2(lockName, 
                         FM_LOCK_SWITCH_NONE,
                         FM_LOCK_SUPER_PRECEDENCE, 
                         lck);
    
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, err);

}   /* end fmCreateLock */


/*****************************************************************************/
/** fmCreateLockV2
 * \ingroup alosLock
 *
 * \desc            Create a critical section lock (mutex) with support for 
 *                  lock precedence. A lock provides mutually exclusive access 
 *                  to a resource by multiple threads.  If a particular thread 
 *                  already has the lock and attempts to get it again, it will 
 *                  succeed. If another thread attempts to get the lock, it 
 *                  will be blocked until the first thread releases it.
 *                                                                      \lb\lb
 *                  Each lock created is assigned a precedence value by the
 *                  caller, which constrains the order in which a thread may
 *                  capture the locks. The precedence mechanism is used to
 *                  defend against lock inversion between two different
 *                  threads, which could cause a deadlock. Precedence values 
 *                  are global across locks and reader-writer locks 
 *                  (see ''fmCreateRwLockV2'').
 *
 * \param[in]       lockName is an arbitrary text string used to identify the
 *                  lock for debugging purposes.
 *
 * \param[in]       sw identifies the switch with which this lock is associated.
 *                  This should be specified as FM_LOCK_SWITCH_NONE for locks 
 *                  that are not associated with any particular switch.
 *
 * \param[in]       precedence indicates this lock's precedence with respect
 *                  to all other locks (and reader-writer locks) in the system.
 *                  May be specified as FM_LOCK_SUPER_PRECEDENCE to transcend
 *                  precedence (lock may be taken in any order with respect to
 *                  any other lock). Note that specifying this argument as
 *                  FM_LOCK_SUPER_PRECEDENCE is equivalent to calling 
 *                  ''fmCreateLock'' instead of this function.
 *
 * \param[out]      lck points to a caller-allocated fm_lock object to be 
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation error.
 * \return          FM_ERR_UNINITIALIZED if ALOS not intialized.
 * \return          FM_ERR_INVALID_ARGUMENT if lck is NULL.
 * \return          FM_ERR_LOCK_INIT if unable to initialize lock.
 *
 *****************************************************************************/
fm_status fmCreateLockV2(fm_text  lockName, 
                         fm_int   sw,
                         fm_int   precedence,
                         fm_lock *lck)
{
    pthread_mutexattr_t attr;
    int                 i;
    int                 pterr;
    fm_status           err;
    fm_bool             attrInit;
    fm_bool             mutexInit;
    fm_bool             listLocked;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_LOCK, 
                 "lockName=%s sw=%d, precedence=%d lck=%p\n",
                 lockName, 
                 sw,
                 precedence,
                 (void *) lck);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNINITIALIZED);
    }

    if (!lck)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }
    
    if ( pthread_mutexattr_init(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_INIT);
    }

    FM_CLEAR(*lck);

    attrInit   = TRUE;
    mutexInit  = FALSE;
    listLocked = FALSE;

    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) ||
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Error setting mutex attributes\n");
        err = FM_ERR_LOCK_INIT;
        goto ABORT;
    }

    lck->handle     = (pthread_mutex_t *) fmAlloc( sizeof(pthread_mutex_t) );
    lck->name       = fmStringDuplicate(lockName);
    
    if (lck->name == NULL || lck->handle == NULL)
    {
        err = FM_ERR_NO_MEM;
        goto ABORT;
    }

    if (sw != FM_LOCK_SWITCH_NONE)
    {
        lck->switchNumber = sw;
    }
    else
    {
        lck->switchNumber = 0;
    }
    
    if (precedence != FM_LOCK_SUPER_PRECEDENCE)
    {
        lck->precedence = (1 << precedence);
        
        if (sw == FM_LOCK_SWITCH_NONE)
        {
            /**************************************************
             * Keep track of locks that are not per-switch,
             * so we don't require the switch lock to be taken
             * prior to taking these locks.
             **************************************************/
            
            fmRootAlos->nonSwitchLockPrecs |= lck->precedence;
        }
    }
    else
    {
        lck->precedence = 0;
    }

    pterr = pthread_mutex_init( (pthread_mutex_t *) lck->handle, &attr );
    if (pterr != 0)
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Error %d initializing mutex\n", pterr);
        err = FM_ERR_LOCK_INIT;
        goto ABORT;
    }

    mutexInit = TRUE;

    if ( pthread_mutexattr_destroy(&attr) )
    {
        err = FM_ERR_LOCK_INIT;
        goto ABORT;
    }

    attrInit = FALSE;

    FM_LOG_DEBUG(FM_LOG_CAT_ALOS_LOCK,
                 "Lock with handle %p created (%s)\n",
                 (void *) lck->handle,
                 lockName);

    if ( pthread_mutex_lock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Error locking LockList\n");
        err = FM_ERR_LOCK_INIT;
        goto ABORT;
    }

    listLocked = TRUE;

    for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_LOCKS ; i++)
    {
        if (fmRootAlos->LockList[i] == NULL)
        {
            fmRootAlos->LockList[i] = lck;
            break;
        }
    }
    
    if (i >= FM_ALOS_INTERNAL_MAX_LOCKS)
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_LOCK,
                     "FM_ALOS_INTERNAL_MAX_LOCKS needs to be increased!\n");
        err = FM_ERR_LOCK_INIT;
        goto ABORT;
    }

    if ( pthread_mutex_unlock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Error unlocking LockList\n");
        err = FM_ERR_LOCK_INIT;
        goto ABORT;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);

ABORT:

    if (listLocked)
    {
        if (i > 0 && i < FM_ALOS_INTERNAL_MAX_LOCKS)
        {
            fmRootAlos->LockList[i] = NULL;
        }

        pterr = pthread_mutex_unlock( (pthread_mutex_t *) fmRootAlos->LockLock.handle );
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Error %d dropping LockList lock\n", pterr);
        }
    }

    if (mutexInit)
    {
        pterr = pthread_mutex_destroy( (pthread_mutex_t *) lck->handle );
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex\n",
                         pterr);
        }
    }

    if (attrInit)
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }
    }

    if (lck->name)
    {
        fmFree(lck->name);
    }

    if (lck->handle)
    {
        fmFree(lck->handle);
    }

    FM_CLEAR(*lck);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, err);

}   /* end fmCreateLockV2 */




/*****************************************************************************/
/** fmDeleteLock
 * \ingroup alosLock
 *
 * \desc            Deallocate a previously created lock.
 *
 * \param[in]       lck is a pointer to the fm_lock object to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT lck is not valid.
 *
 *****************************************************************************/
fm_status fmDeleteLock(fm_lock *lck)
{
    int i;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_LOCK, "lck=%p\n", (void *) lck);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNINITIALIZED);
    }

    if (!lck)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if (pthread_mutex_destroy( (pthread_mutex_t *) lck->handle ) != 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_DESTROY);
    }

    if ( pthread_mutex_lock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNABLE_TO_LOCK);
    }

    for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_LOCKS ; i++)
    {
        if (fmRootAlos->LockList[i] == lck)
        {
            fmRootAlos->LockList[i] = NULL;
            break;
        }
    }

    if ( pthread_mutex_unlock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNABLE_TO_UNLOCK);
    }

    fmFree(lck->name);
    fmFree(lck->handle);
    lck->name   = NULL;
    lck->handle = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);

}   /* end fmDeleteLock */




/*****************************************************************************/
/** fmCaptureLock
 * \ingroup alosLock
 *
 * \desc            Take a lock (mutex).  This function will block if another
 *                  thread currently has the lock. Must be balanced by a call
 *                  to fmReleaseLock.
 *
 * \param[in]       lck is a pointer to the fm_lock object to be taken.
 *
 * \param[in]       timeout is the maximum amount of time to wait
 *                  blocked on the lock.  A value of FM_WAIT_FOREVER will
 *                  result in blocking indefinitely.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNABLE_TO_LOCK if an error occurred trying to
 *                  acquire the lock.
 * \return          FM_ERR_LOCK_TIMEOUT if a timeout occurred waiting for
 *                  the lock.
 * \return          FM_ERR_INVALID_ARGUMENT if lck does not point to a valid
 *                  fm_lock object.
 * \return          FM_ERR_LOCK_UNINITIALIZED if lck points to an fm_lock
 *                  which has not yet been initialized with a call to
 *                  ''fmCreateLock'' or ''fmCreateLockV2''.
 *
 *****************************************************************************/
fm_status fmCaptureLock(fm_lock *lck, fm_timestamp *timeout)
{
    int             posixError;
    struct timespec ts;
    struct timeval  ct;
    fm_status       err = FM_OK;
    char            strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t         strErrNum;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_LOCK,
                 "handle=%p timeout=%p\n",
                 (void *) lck, (void *) timeout);
#endif

    if (!lck)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "NULL argument to fmCaptureLock\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if (!lck->handle)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "Attempted to lock an uninitialized lock\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_UNINITIALIZED);
    }
    
    /**************************************************
     * Validate the lock precedence. If this thread
     * has already taken a lock with higher precedence,
     * then this is an error.
     **************************************************/
     
    VALIDATE_LOCK_PRECEDENCE(err, lck, TRUE);
    
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_PRECEDENCE);
    }
    
    if (timeout == FM_WAIT_FOREVER)
    {
        if ( ( posixError = pthread_mutex_lock( (pthread_mutex_t *)
                                                lck->handle ) ) != 0 )
        {
            strErrNum =
                FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, posixError);
            if (strErrNum == 0)
            {
                FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                              "pthread_mutex_lock failed - %s\n",
                              strErrBuf );
            }
            else
            {
                FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                              "pthread_mutex_lock failed - %d\n",
                              posixError );
            }
            VALIDATE_LOCK_PRECEDENCE(err, lck, FALSE);
            FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNABLE_TO_LOCK);
        }
    }
    else
    {
        posixError = gettimeofday(&ct, NULL);

        if (posixError != 0)
        {
            strErrNum =
                FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, posixError);
            if (strErrNum == 0)
            {
                FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                         "gettimeofday failed - %s\n", strErrBuf );
            }
            else
            {
                FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                              "gettimeofday failed - %d\n", posixError );
            }
            VALIDATE_LOCK_PRECEDENCE(err, lck, FALSE);
            FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNABLE_TO_LOCK);
        }

        ts.tv_sec  = ct.tv_sec + timeout->sec;
        ts.tv_nsec = (ct.tv_usec + timeout->usec) * 1000;

        posixError = pthread_mutex_timedlock( (pthread_mutex_t *) lck->handle,
                                              &ts );
        if (posixError != 0)
        {
            if (posixError == ETIMEDOUT)
            {
                VALIDATE_LOCK_PRECEDENCE(err, lck, FALSE);
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_TIMEOUT);
            }
            else
            {
                strErrNum =
                    FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, posixError);
                if (strErrNum == 0)
                {
                    FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                                  "pthread_mutex_lock failed - %s\n",
                                  strErrBuf );
                }
                else
                {
                    FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                                  "pthread_mutex_lock failed - %d\n",
                                  posixError );
                }
                VALIDATE_LOCK_PRECEDENCE(err, lck, FALSE);
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNABLE_TO_LOCK);
            }
        }
    }

    ++lck->takenCount;
    lck->owner = fmGetCurrentThreadId();

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmCaptureLock */




/*****************************************************************************/
/** fmReleaseLock
 * \ingroup alosLock
 *
 * \desc            Release the current thread's hold on a lock. Must be
 *                  balanced by a call to fmCaptureLock.
 *
 * \param[in]       lck is a pointer to the fm_lock object to be released.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNABLE_TO_UNLOCK if an error occurred trying to
 *                  release the lock.
 * \return          FM_ERR_INVALID_ARGUMENT if lck does not point to a valid
 *                  fm_lock object.
 * \return          FM_ERR_LOCK_UNINITIALIZED if lck points to an fm_lock
 *                  which has not yet been initialized with a call to
 *                  fmCreateLock.
 *
 *****************************************************************************/
fm_status fmReleaseLock(fm_lock *lck)
{
    int       posixError;
    fm_status err;
    char      strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t   strErrNum;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_LOCK, "handle=%p\n", (void *) lck);
#endif

    if (!lck)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "NULL argument to fmReleaseLock\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if (!lck->handle)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "Attempted to unlock an uninitialized lock\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_UNINITIALIZED);
    }

    /**************************************************
     * If this thread is releasing the lock for
     * the last time, remove it from its lock collection.
     **************************************************/
     
    VALIDATE_LOCK_PRECEDENCE(err, lck, FALSE);
    
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_PRECEDENCE);
    }
    
    if (lck->takenCount)
    {
        if(--lck->takenCount == 0)
        {
            lck->owner = NULL;
        }
    }
    
    if ( ( posixError = pthread_mutex_unlock( (pthread_mutex_t *)
                                              lck->handle ) ) != 0 )
    {
        ++lck->takenCount;
        lck->owner = fmGetCurrentThreadId();

        VALIDATE_LOCK_PRECEDENCE(err, lck, TRUE);
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, posixError);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                          "pthread_mutex_unlock failed on lock %s - %s\n",
                          lck->name, strErrBuf );
        }
        else
        {
            FM_LOG_ERROR( FM_LOG_CAT_ALOS_LOCK,
                          "pthread_mutex_unlock failed on lock %s - %d\n",
                          lck->name, posixError );
        }
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_UNABLE_TO_UNLOCK);
    }
    
#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmReleaseLock */




/*****************************************************************************/
/** fmIsLockTaken
 * \ingroup alosLock
 *
 * \desc            Determines whether the calling thread currently holds a
 *                  specified lock.
 *
 * \param[in]       lck is a pointer to the fm_lock object to be checked.
 *
 * \param[out]      isTaken points to caller-allocated storage into which the
 *                  results of the check will be written: TRUE means that the
 *                  calling thread is holding the specified lock. FALSE means
 *                  that the calling thread is not holding the specified lock.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if lck does not point to a valid
 *                  fm_lock object.
 * \return          FM_ERR_LOCK_UNINITIALIZED if lck points to an fm_lock
 *                  which has not yet been initialized with a call to
 *                  ''fmCreateLock''.
 *
 *****************************************************************************/
fm_status fmIsLockTaken(fm_lock *lck, fm_bool *isTaken)
{

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY( FM_LOG_CAT_ALOS_LOCK,
                  "handle=%p, isTaken=%p\n",
                  (void *) lck,
                  (void *) isTaken );
#endif

    if (lck == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "NULL argument to fmIsLockTaken\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if (isTaken == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "NULL argument to fmIsLockTaken\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if (!lck->handle)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "Attempted to access an uninitialized lock\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_UNINITIALIZED);
    }

    if ( lck->takenCount && ( lck->owner == fmGetCurrentThreadId() ) )
    {
        *isTaken = TRUE;
    }
    else
    {
        *isTaken = FALSE;
    }
    
#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmIsLockTaken */




/*****************************************************************************/
/** fmDbgDumpLocks
 * \ingroup diagAlos
 *
 * \desc            Print out the state of all locks.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpLocks(void)
{
    fm_lock *  lck;
    fm_int     lockCount = 0;
    fm_int     i;
    fm_status  err;
    fm_char    threadName[MAX_THREAD_NAME_LENGTH];
    void *     thisThreadId;
    fm_char    bfr[32];
    fm_thread *threadPtr;
    
    thisThreadId = fmGetCurrentThreadId();
    err = fmGetThreadState(thisThreadId,
                           threadName,
                           MAX_THREAD_NAME_LENGTH);
    
    FM_LOG_PRINT("\nCalled from thread %p", (void *)thisThreadId);
    
    if (err == FM_OK)
    {
        FM_LOG_PRINT(": %s \n", threadName);
    }
    else
    {
        FM_LOG_PRINT("\n");
    }
    
    FM_LOG_PRINT("Critical Section Locks:\n");
    FM_LOG_PRINT("Name                           Sw    Prec   Taken Owner\n");
    FM_LOG_PRINT("------------------------------ --- -------- ----- --------");
    FM_LOG_PRINT("----------------------\n");
    
    if ( pthread_mutex_lock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, "\nUnable to take lock lock!\n");
        goto ABORT;
    }

    for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_LOCKS ; i++)
    {
        if (fmRootAlos->LockList[i])
        {
            ++lockCount;
            lck = fmRootAlos->LockList[i];

            /* Get owner info */
            err = fmGetThreadState(lck->owner,
                                   threadName,
                                   MAX_THREAD_NAME_LENGTH);
            
            if (err != FM_OK)
            {
                /* Unknown thread. Add it to the list of threads. */
                FM_SPRINTF_S(bfr,
                             sizeof(bfr),
                             "foreign.%d",
                             fmAlosThreadState.foreignThreadCount++);
                err = fmAddForeignThreadToList(bfr, &threadPtr);
                
                if (err == FM_OK)
                {
                    /* Now try to get the name again. */
                    err = fmGetThreadState(lck->owner,
                                           threadName,
                                           MAX_THREAD_NAME_LENGTH);
                }
            }

            /* Lock name. */
            FM_LOG_PRINT("%-30s ",
                         lck->name);
            
            /* Lock's associated switch number */
            FM_LOG_PRINT("%3d ", lck->switchNumber);
            
            /* Lock's precedence mask */
            if (lck->precedence)
            {
                FM_LOG_PRINT("%08X ",
                             lck->precedence);
            }
            else
            {
                FM_LOG_PRINT("(none)   ");
            }
            
            /* Number of times lock is being held */
            FM_LOG_PRINT("%4d  ",
                         lck->takenCount);
            
            /* Thread info */
            if (err == FM_OK)
            {
                FM_LOG_PRINT("%p %s\n",
                             (void *) lck->owner,
                             threadName);
            }
            else
            {
                FM_LOG_PRINT("(none)\n");
            }
            
            
        }   /* end if (fmRootAlos->LockList[i]) */
        
    }   /* end for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_LOCKS ; i++) */

    if ( pthread_mutex_unlock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, "\nUnable to release lock lock!\n");
        goto ABORT;
    }

    FM_LOG_PRINT("----------------------------\n");
    FM_LOG_PRINT("%d locks\n\n", lockCount);
    
    /**************************************************
     * Now do reader-writer locks.
     **************************************************/
    
    /* The switch argument is not used, so we can just pass zero. */
    fmDbgDiagDumpRwLockState(0);
    fmDbgDiagDumpRwLockStats(0);
    
ABORT:
    return;
    
}   /* end fmDbgDumpLocks */



/*****************************************************************************/
/** fmDbgDumpLockPrecMask
 * \ingroup intAlos
 *
 * \desc            Dump the names of all locks identified in a lock
 *                  precedence bit mask.
 *
 * \note            The output is on a single line with no trailing
 *                  newline.
 *
 * \param[in]       logCat is the log category to generate output at.
 *
 * \param[in]       logLevel is the log level to generate otuput at.
 *
 * \param[in]       precMask is a bit mask of all the locks to be identified
 *                  by precedence.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpLockPrecMask(fm_uint64 logCat, 
                           fm_uint64 logLevel, 
                           fm_lockPrecedence precMask)
{
    fm_int    bit;
    fm_int    count;
    fm_int    i;
    fm_uint32 origVerbosity;
    fm_text   lockName;
    fm_bool   first = TRUE;
    
    
    /* Total number of bits in the mask. */
    count = sizeof(fm_lockPrecedence) * 8;
    fmGetLoggingVerbosity(&origVerbosity);
    fmSetLoggingVerbosity(FM_LOG_VERBOSITY_NONE);
    
    /**************************************************
     * We need the name of each lock being held, which
     * we will take from the lock structures. We need
     * to search the list of locks and rw locks, so
     * take the lock locks.
     *
     * We won't worry about performance as this
     * function should only be called for diagnostic
     * purposes during dire situations.
     **************************************************/
    
    if ( pthread_mutex_lock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, "\nUnable to take lock lock!\n");
        goto ABORT;
    }
    
    if ( pthread_mutex_lock(fmRootAlos->dbgRwLockListLock.handle) ) 
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, 
                     "\nUnable to take reader-writer lock lock!\n");
        goto ABORT;
    }
                
    for (bit = 0 ; count > 0 ; bit++, count--)
    {
        if (precMask & 0x01)
        {
            if (first)
            {
                first = FALSE;
            }
            else
            {
                FM_LOG_PRINTF(logCat,
                              logLevel,
                              ", "); 
            }
            
            /**************************************************
             * First scan critical section locks for the name
             * of this lock.
             **************************************************/
            
            lockName = NULL;
            
            for (i = 1 ; i < FM_ALOS_INTERNAL_MAX_LOCKS ; i++)
            {
                if (fmRootAlos->LockList[i] != NULL)
                {
                    if ( fmRootAlos->LockList[i]->precedence == 
                                            (fm_lockPrecedence )(1 << bit) )
                    {
                        lockName = fmRootAlos->LockList[i]->name;
                        break;
                    }
                }
            }
        
            /**************************************************
             * If we didn't find a critical section lock with 
             * this precedence, then search the reader-writer
             * locks.
             **************************************************/
            
            if (lockName == NULL)
            {
                for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS ; i++) 
                {                                                         
                    if (fmRootAlos->dbgRwLockList[i] != NULL)             
                    {                                                     
                        if ( fmRootAlos->dbgRwLockList[i]->precedence == 
                                            (fm_lockPrecedence) (1 << bit) )
                        {
                            lockName = fmRootAlos->dbgRwLockList[i]->name;
                            break;
                        }
                    }                                                     
                }                                                         
            }
        
            FM_LOG_PRINTF(logCat,
                          logLevel,
                          "%s", 
                          lockName ? lockName : "Unknown");
            
        }   /* end if (precMask & 0x01) */
        
        precMask >>= 1;
        
    }   /* end for (bit = 0 ; count > 0 ; bit++, count--) */
    
    /**************************************************
     * Unlock the lock locks.
     **************************************************/

    if ( pthread_mutex_unlock(fmRootAlos->dbgRwLockListLock.handle) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, 
                     "\nUnable to release reader-writer lock lock!\n");
        goto ABORT;
    }

ABORT:

    if ( pthread_mutex_unlock( (pthread_mutex_t *) fmRootAlos->LockLock.handle ) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK, "\nUnable to release lock lock!\n");
    }

    fmSetLoggingVerbosity(origVerbosity);

}   /* end fmDbgDumpLockPrecMask */




/*****************************************************************************/
/* fmDbgTakeLock
 * \ingroup intAlos
 *
 * \desc            For debugging purposes: capture a lock with repeated 
 *                  retries. The default is 4 attempts of 15 seconds each,
 *                  for a total of 60 seconds.
 *
 * \param[in]       sw is the switch number on which to operate. Used in the
 *                  log message.
 *
 * \param[in,out]   lockPtr is a pointer to the lock to be captured.
 *
 * \param[in]       tryTime is the number of seconds to wait on each attempt.
 *                  Specify -1 for the default of 15 seconds.
 *
 * \param[in]       numTries is the maximum number of attempts to make.
 *                  Specify -1 for the default of 4 attempts.
 *
 * \param[in]       funcName points to a string containing the name of the
 *                  function that is requesting the lock.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgTakeLock(fm_int      sw,
                        fm_lock*    lockPtr,
                        fm_int      tryTime,
                        fm_int      numTries,
                        const char* funcName)
{
    fm_timestamp timeout;
    fm_int err;

    FM_NOT_USED(sw);

    if (tryTime <= 0)
    {
        tryTime = 15;
    }

    if (numTries <= 0)
    {
        numTries = 4;
    }

    if (funcName == NULL)
    {
        funcName = __func__;
    }

    timeout.sec = (fm_uint)tryTime;
    timeout.usec = 0;

    for ( ; ; )
    {
        err = fmCaptureLock(lockPtr, &timeout);

        if (err == FM_OK)
        {
            break;
        }

        --numTries;

        FM_LOG_WARNING(FM_LOG_CAT_ALOS_LOCK,
                       "%s[%d]: error taking %s lock (%d %s left)\n", 
                       funcName,
                       sw,
                       (lockPtr->name) ? lockPtr->name : "unnamed",
                       numTries,
                       (numTries != 1) ? "tries" : "try");

        if (numTries <= 0)
        {
            break;
        }
    }

    return err;

}   /* end fmDbgTakeLock */


/*****************************************************************************/
/** fmGetLockPrecedence
 * \ingroup alosLock
 *
 * \desc            Returns an ALOS lock's precedence 
 *
 * \param[in]       lck is a pointer to the fm_lock object to be checked.
 *
 * \param[out]      precedence points to caller-allocated storage into which
 *                  this function will return the lock's precedence value
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if lck does not point to a valid
 *                  fm_lock object.
 * \return          FM_ERR_LOCK_UNINITIALIZED if lck points to an fm_lock
 *                  which has not yet been initialized with a call to
 *                  ''fmCreateLock''.
 * 
 *****************************************************************************/
fm_status fmGetLockPrecedence(fm_lock *lck, fm_int *precedence)
{
    fm_int i;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY( FM_LOG_CAT_ALOS_LOCK,
                  "handle=%p, precedence=%p\n",
                  (void *) lck,
                  (void *) precedence );
#endif

    if (lck == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "NULL argument to fmGetLockPrecedence\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if ( precedence == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "NULL argument to fmGetLockPrecedence\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }

    if (!lck->handle)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_LOCK,
                     "Attempted to access an uninitialized lock\n");
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_LOCK_UNINITIALIZED);
    }

    /* translate the precedence mask into */
    if ( lck->precedence == 0 )
    {
        *precedence = FM_LOCK_SUPER_PRECEDENCE;
    }
    else
    {
        for (i = 0 ; i < 32  ; i++)
        {
            if ( ( lck->precedence & (1 << i) ) != 0 )
            {
                break;
            }
        }
        *precedence = i;
    }
    
#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmGetLockPrecedence */

