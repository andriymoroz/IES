/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_rwlock.c
 * Creation Date:   2006
 * Description:     Provide reader/writer locks.   A reader/writer lock allows
 *                  simultaneous access to a resource by multiple readers and
 *                  ensures that all readers have released the lock before a
 *                  writer is given access.  All readers and writers will be
 *                  blocked when trying to get the lock while a writer has
 *                  it.
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

#if FM_LOCK_INVERSION_DEFENSE
#define VALIDATE_LOCK_PRECEDENCE(err, lck, index, take)    \
    err = ValidateLockPrecedence(lck, index, take);
#else
#define VALIDATE_LOCK_PRECEDENCE(err, lck, index, take)    \
    err = FM_OK;
#endif
    

/**************************************************
 * Debug Macros
 **************************************************/

#define INIT_RW_LOCK_DBG_LIST(attr)                                             \
{                                                                               \
    int x;                                                                      \
                                                                                \
    fmRootAlos->dbgRwLockListLock.handle = (pthread_mutex_t *)                  \
                                           fmAlloc( sizeof(pthread_mutex_t) );  \
    if (fmRootAlos->dbgRwLockListLock.handle == NULL)                           \
    {                                                                           \
        x = pthread_mutexattr_destroy(attr);                                    \
        if (x != 0)                                                             \
        {                                                                       \
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                \
                         "error %d from pthread_mutexattr_destroy\n",           \
                         x);                                                    \
        }                                                                       \
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);                     \
    }                                                                           \
    if (pthread_mutex_init(fmRootAlos->dbgRwLockListLock.handle, attr) != 0)    \
    {                                                                           \
        x = pthread_mutexattr_destroy(attr);                                    \
        if (x != 0)                                                             \
        {                                                                       \
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                \
                         "error %d from pthread_mutexattr_destroy\n",           \
                         x);                                                    \
        }                                                                       \
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);                  \
    }                                                                           \
    FM_CLEAR( fmRootAlos->dbgRwLockList);                                       \
}

#define DBG_LIST_ADD_RW_LOCK(lck)                                               \
{                                                                               \
    int x;                                                                      \
    x = pthread_mutex_lock(fmRootAlos->dbgRwLockListLock.handle);               \
    if (x != 0)                                                                 \
    {                                                                           \
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                    \
                     "error %d from pthread_mutex_lock\n",                      \
                     x);                                                        \
    }                                                                           \
                                                                                \
    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS ; i++)                   \
    {                                                                           \
        if (fmRootAlos->dbgRwLockList[i] == NULL)                               \
        {                                                                       \
            fmRootAlos->dbgRwLockList[i] = (lck);                               \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    x = pthread_mutex_unlock(fmRootAlos->dbgRwLockListLock.handle);             \
    if (x != 0)                                                                 \
    {                                                                           \
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                    \
                     "error %d from pthread_mutex_unlock\n",                    \
                     x);                                                        \
    }                                                                           \
}

#define DBG_LIST_DEL_RW_LOCK(lck)                                               \
{                                                                               \
    int x;                                                                      \
    x = pthread_mutex_lock(fmRootAlos->dbgRwLockListLock.handle);               \
    if (x != 0)                                                                 \
    {                                                                           \
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                    \
                     "error %d from pthread_mutex_lock\n",                      \
                     x);                                                        \
    }                                                                           \
    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS ; i++)                   \
    {                                                                           \
        if ( fmRootAlos->dbgRwLockList[i] == (lck) )                            \
        {                                                                       \
            fmRootAlos->dbgRwLockList[i] = NULL;                                \
            break;                                                              \
        }                                                                       \
    }                                                                           \
    x = pthread_mutex_unlock(fmRootAlos->dbgRwLockListLock.handle);             \
    if (x != 0)                                                                 \
    {                                                                           \
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                    \
                     "error %d from pthread_mutex_unlock\n",                    \
                     x);                                                        \
    }                                                                           \
}

#define INIT_RW_LOCK_DBG_STATS(attr)                                            \
{                                                                               \
    int x;                                                                      \
    fmRootAlos->rwLockDebugCounterLock.handle =                                 \
        (pthread_mutex_t *) fmAlloc(sizeof(pthread_mutex_t) );                  \
    if (fmRootAlos->rwLockDebugCounterLock.handle == NULL)                      \
    {                                                                           \
        x = pthread_mutexattr_destroy(attr);                                    \
        if (x != 0)                                                             \
        {                                                                       \
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                \
                         "error %d from pthread_mutexattr_destroy\n",           \
                         x);                                                    \
        }                                                                       \
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);                     \
    }                                                                           \
    if (pthread_mutex_init(fmRootAlos->rwLockDebugCounterLock.handle,           \
                           attr) != 0)                                          \
    {                                                                           \
        x = pthread_mutexattr_destroy(attr);                                    \
        if (x != 0)                                                             \
        {                                                                       \
            FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                \
                         "error %d from pthread_mutexattr_destroy\n",           \
                         x);                                                    \
        }                                                                       \
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);                  \
    }                                                                           \
    FM_CLEAR(fmRootAlos->rwLockDebugCounters);                                  \
}

#define TAKE_DBG_CTR()                                                          \
{                                                                               \
    int x;                                                                      \
    x = pthread_mutex_lock(fmRootAlos->rwLockDebugCounterLock.handle);          \
    if (x != 0)                                                                 \
    {                                                                           \
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                    \
                     "error %d from pthread_mutex_lock\n",                      \
                     x);                                                        \
    }                                                                           \
}

#define GIVE_DBG_CTR()                                                          \
{                                                                               \
    int x;                                                                      \
    x = pthread_mutex_unlock(fmRootAlos->rwLockDebugCounterLock.handle);        \
    if (x != 0)                                                                 \
    {                                                                           \
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,                                    \
                     "error %d from pthread_mutex_unlock\n",                    \
                     x);                                                        \
    }                                                                           \
}

#define PRINT_RWL_DBG_CTR(str, ctr)                      \
    FM_LOG_DEBUG(FM_LOG_CAT_ALOS_RWLOCK, "%-60s : %d\n", \
                 (str), fmRootAlos->rwLockDebugCounters[(ctr)])




typedef enum
{
    FM_RWL_LCK_CREATE = 0,
    FM_RWL_RD_LCK,
    FM_RWL_WR_LCK,
    FM_RWL_RD_REL,
    FM_RWL_WR_REL,
    FM_RWL_RD_LCK_NOBLOCK_SINGLE,
    FM_RWL_RD_LCK_NOBLOCK_MULTI,
    FM_RWL_RD_LCK_BLOCK_WR,
    FM_RWL_RD_LCK_REC_IN_RD,
    FM_RWL_RD_LCK_REC_IN_WR,
    FM_RWL_WR_LCK_NOBLOCK,
    FM_RWL_WR_LCK_BLOCK_RD,
    FM_RWL_WR_LCK_BLOCK_WR,
    FM_RWL_WR_LCK_REC_IN_WR,
    FM_RWL_RD_REL_NONE,
    FM_RWL_RD_REL_TO_RD,
    FM_RWL_RD_REL_TO_WR,
    FM_RWL_RD_REL_REC_IN_RD,
    FM_RWL_RD_REL_REC_IN_WR,
    FM_RWL_WR_REL_NONE,
    FM_RWL_WR_REL_TO_RD,
    FM_RWL_WR_REL_TO_WR,
    FM_RWL_WR_REL_REC_IN_RD,
    FM_RWL_WR_REL_REC_IN_WR,
    FM_RWL_CTR_MAX

} fm_rw_lock_dbg_counter;


/**************************************************
 * Helper macros for RW locks.
 **************************************************/

#define TAKE_ACCESS(lck)                                                      \
    if ( ( posixError = pthread_mutex_lock( (pthread_mutex_t *)               \
                                           (lck)->accessHandle ) ) != 0 )     \
                        {                                                     \
                            strErrNum =                                       \
                                FM_STRERROR_S(strErrBuf,                      \
                                              FM_STRERROR_BUF_SIZE,           \
                                              posixError);                    \
                            if (strErrNum == 0)                               \
                            {                                                 \
                                FM_LOG_ERROR( FM_LOG_CAT_ALOS_RWLOCK,         \
                                         "failed to take access lock - %s\n", \
                                         strErrBuf );                         \
                            }                                                 \
                            else                                              \
                            {                                                 \
                                FM_LOG_ERROR( FM_LOG_CAT_ALOS_RWLOCK,         \
                                         "failed to take access lock - %d\n", \
                                         posixError );                        \
                            }                                                 \
                            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);     \
                        }

#define GIVE_ACCESS(lck)                                                      \
    if ( ( posixError = pthread_mutex_unlock( (pthread_mutex_t *)             \
                                             (lck)->accessHandle ) ) != 0 )   \
                        {                                                     \
                            strErrNum =                                       \
                                FM_STRERROR_S(strErrBuf,                      \
                                              FM_STRERROR_BUF_SIZE,           \
                                              posixError);                    \
                            if (strErrNum == 0)                               \
                            {                                                 \
                                FM_LOG_ERROR( FM_LOG_CAT_ALOS_RWLOCK,         \
                                         "failed to give access lock - %s\n", \
                                         strErrBuf );                         \
                            }                                                 \
                            else                                              \
                            {                                                 \
                                FM_LOG_ERROR( FM_LOG_CAT_ALOS_RWLOCK,         \
                                         "failed to give access lock - %d\n", \
                                         posixError );                        \
                            }                                                 \
                            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);     \
                        }

#define ADD_THREAD(lck, index)                                  \
    (lck)->userList[index].used       = TRUE;                   \
    (lck)->userList[index].id         = fmGetCurrentThreadId(); \
    (lck)->userList[index].numReaders = 0;                      \
    (lck)->userList[index].numWriters = 0;                      \
    if (lck->maxThreads <= index) lck->maxThreads = index + 1;

#define DEL_THREAD(lck, index)                                  \
    (lck)->userList[index].used       = FALSE;                  \
    (lck)->maxThreads = FM_MAX_THREADS - 1;                     \
    while ((lck)->maxThreads >= 0 &&                                      \
           !(lck)->userList[(lck)->maxThreads].used) (lck)->maxThreads--; \
    (lck)->maxThreads++;



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
 * \note            Before this function is called on a capture, the thread
 *                  must first be added to the lock's user list and when
 *                  called on a release, it must be called before the
 *                  thread is removed from the lock's user list. Unlike
 *                  regular critical section locks, reader-writer locks
 *                  maintain state per-user thread, so we can safely
 *                  manipulate the takenCount inside this function.
 *
 * \note            The locks access lock must be taken before calling this
 *                  function.
 *
 * \param[in]       lck points to the lock state.
 *
 * \param[in]       index is the user list index for this thread.
 *
 * \param[in]       take should be TRUE when testing for a capture, FALSE when
 *                  testing for a release.
 *
 * \return          FM_OK if lock transaction is valid.
 * \return          FM_FAIL if lock transaction is out of order.
 *
 *****************************************************************************/
static fm_status ValidateLockPrecedence(fm_rwLock *lck, 
                                        fm_int    index, 
                                        fm_bool   take)
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
        /**************************************************
         * We don't really care about the takenCount for
         * non-precedenced locks, but since takenCount can
         * be manipulated outside this function without
         * regard to the lock being non-precedenced, let's
         * be tidy and maintain the counter properly.
         **************************************************/
        
        if (take)
        {
            ++lck->userList[index].takenCount;
        }
        else if (lck->userList[index].takenCount > 0)
        {
            --lck->userList[index].takenCount;
        }
        
        return FM_OK;
    }

    threadsLocks = fmGetCurrentThreadLockCollection();
    
    if (!threadsLocks)
    {
        /* Should never happen! */
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_RWLOCK,
                     "Could not get lock collection for thread %p!\n",
                     (void *) fmGetCurrentThreadId());
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
            ++lck->userList[index].takenCount;
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
    else if (lck->userList[index].takenCount > 1)
    {
        /**************************************************
         * This is not the last (un-nested) release, let
         * it pass since we may have done a nested take
         * after taking a higher precedence lock.
         **************************************************/
        
        --lck->userList[index].takenCount;
        return FM_OK;        
    }

    /**************************************************
     * Check for lock being taken out of order.
     **************************************************/
    
    if (precWithoutThisLock > thisLocksPrec)
    {
        /* Precedence violation! */
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "LOCK INVERSION: Attempted to %s lock '%s' (precedence = "
                     "0x%X) out of order on switch %d. Locks held: 0x%X (",
                     take ? "take" : "release",
                     lck->name,
                     lck->precedence,
                     lck->switchNumber,
                     *threadsLocks);
        fmGetLoggingVerbosity(&origVerbosity);
        fmSetLoggingVerbosity(FM_LOG_VERBOSITY_NONE);
        fmDbgDumpLockPrecMask(FM_LOG_CAT_ALOS_RWLOCK, 
                              FM_LOG_LEVEL_ERROR,
                              *threadsLocks);
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK, ")\n");
        fmSetLoggingVerbosity(origVerbosity);
        FM_LOG_CALL_STACK(FM_LOG_CAT_ALOS_RWLOCK, FM_LOG_LEVEL_ERROR);
    }
    
    if (take)
    {
        /* We're taking the lock, so add it to the collection. */
        *threadsLocks |= thisLocksPrec;
        ++lck->userList[index].takenCount;
    }
    else
    {
        if (lck->userList[index].takenCount == 1)
        {
            /* We're releasing the lock for the last time, so take it out 
             * of the thread's collection. */
            *threadsLocks = precWithoutThisLock;
        }
        
        if (lck->userList[index].takenCount > 0)
        {
            --lck->userList[index].takenCount;
        }
    }

    return FM_OK;

}   /* end ValidateLockPrecedence */

#endif  /* FM_LOCK_INVERSION_DEFENSE */


/*****************************************************************************/
/** FindThreadInUserList
 * \ingroup intAlosLock
 *
 * \desc            Search a lock's user list for the current thread's thread ID.
 *
 * \param[in]       lck is a pointer to the fm_rwLock object to be searched.
 *
 * \param[out]      index points to storage where this function should put
 *                  the user list index where this thread's ID appears.
 *
 * \param[out]      firstUnused points to storage where this function should put
 *                  the index to the first unused entry in the list.
 *
 * \return          TRUE if current thread is a user of this lock.
 * \return          FALSE if current thread is not a user of this lock.
 *
 *****************************************************************************/
static fm_bool FindThreadInUserList(fm_rwLock *lck,
                                    int *      index,
                                    int *      firstUnused)
{
    int   i;
    void *id;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, "handle=%p index=%p funused=%p\n",
                 (void *) lck, (void *) index, (void *) firstUnused);

    if (lck->maxThreads == 0)
    {
        *firstUnused = 0;
        return FALSE;
    }

    *firstUnused = -1;

    id = fmGetCurrentThreadId();

    for (i = 0 ; i < lck->maxThreads ; i++)
    {
        if ( lck->userList[i].used &&
            fmThreadIdsEqual(lck->userList[i].id, id) )
        {
            *index = i;

            FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ALOS_RWLOCK, TRUE,
                          "Exiting... (True)\n");
        }
        else if (!lck->userList[i].used)
        {
            if (*firstUnused == -1)
            {
                *firstUnused = i;
            }
        }
    }

    if (*firstUnused == -1 && lck->maxThreads < FM_MAX_THREADS)
    {
        *firstUnused = lck->maxThreads;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ALOS_RWLOCK, FALSE,
                  "Exiting... (False)\n");

}   /* end FindThreadInUserList */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/* This is a private function which should only be called from fmAlosRootInit */
fm_status fmAlosRwlockInit(void)
{
    pthread_mutexattr_t attr;
    int                 pterr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, "(no arguments)\n");

    /* This test cannot be made by pre-processor because FM_RWL_CTR_MAX
     * is a value in an enum, which won't be parsed until after the
     * pre-processor has completed. Thus, it has to be a run-time check. */
    if (FM_ALOS_INTERNAL_RWL_CTR_MAX < FM_RWL_CTR_MAX)
    {
        /* #define in header file doesn't match enum in source file */
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Set FM_ALOS_INTERNAL_RWL_CTR_MAX to be %d, and recompile.",
                     FM_RWL_CTR_MAX);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    if ( pthread_mutexattr_init(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);
    }

    INIT_RW_LOCK_DBG_LIST(&attr);
    INIT_RW_LOCK_DBG_STATS(&attr);

    if ( pthread_mutexattr_destroy(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);

}   /* end fmAlosRwlockInit */




/*****************************************************************************/
/** fmCreateRwLock
 * \ingroup alosLock
 *
 * \desc            Deprecated. Use ''fmCreateRwLockV2''.
 *                                                                      \lb\lb
 *                  Create a reader/writer lock. A reader/writer lock allows
 *                  simultaneous access to a resource by multiple readers and
 *                  ensures that all readers have released the lock before a
 *                  writer is given access.  All readers and writers will be
 *                  blocked when trying to get the lock while a writer has
 *                  it.
 *
 * \note            Locks created with this function may be taken in any order
 *                  with respect to all other locks. That is, there is no
 *                  defence against lock inversion as there is with locks
 *                  created with ''fmCreateRwLockV2''.
 *
 * \param[in]       lockName is an arbitrary text string used to identify the
 *                  lock for debugging purposes.
 *
 * \param[out]      lck points to a caller-allocated fm_rwLock object to be 
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation error.
 * \return          FM_ERR_UNINITIALIZED if ALOS not intialized.
 * \return          FM_ERR_INVALID_ARGUMENT if lck is NULL.
 * \return          FM_ERR_LOCK_INIT if unable to initialize lock.
 * \return          FM_FAIL if unable to create lock.
 *
 *****************************************************************************/
fm_status fmCreateRwLock(fm_text lockName, fm_rwLock *lck)
{
    fm_status err;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, 
                 "lockName=%s  lck=%p\n",
                 lockName, 
                 (void *) lck);

    err = fmCreateRwLockV2(lockName, 
                           FM_LOCK_SWITCH_NONE,
                           FM_LOCK_SUPER_PRECEDENCE, 
                           lck);
    
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, err);

}   /* end fmCreateRwLock */




/*****************************************************************************/
/** fmCreateRwLockV2
 * \ingroup alosLock
 *
 * \desc            Create a reader/writer lock. A reader/writer lock allows
 *                  simultaneous access to a resource by multiple readers and
 *                  ensures that all readers have released the lock before a
 *                  writer is given access.  All readers and writers will be
 *                  blocked when trying to get the lock while a writer has
 *                  it.
 *                                                                      \lb\lb
 *                  Each lock created is assigned a precedence value by the
 *                  caller, which constrains the order in which a thread may
 *                  capture the locks. The precedence mechanism is used to
 *                  defend against lock inversion between two different
 *                  threads, which could cause a deadlock. Precedence values 
 *                  are global across reader-writer locks and critical section
 *                  locks (see ''fmCreateLockV2'').
 *
 * \param[in]       lockName is an arbitrary text string used to identify the
 *                  lock for debugging purposes.
 *
 * \param[in]       sw identifies the switch with which this lock is associated.
 *                  This should be specified as FM_LOCK_SWITCH_NONE for locks 
 *                  that are not associated with any particular switch.
 *
 * \param[in]       precedence indicates this lock's precedence with respect
 *                  to all other reader-writer locks (and critical section 
 *                  locks) in the system. May be specified as 
 *                  FM_LOCK_SUPER_PRECEDENCE to transcend precedence (the 
 *                  lock may be taken in any order with respect to any other 
 *                  lock). Note that specifying this argument as
 *                  FM_LOCK_SUPER_PRECEDENCE is equivalent to calling 
 *                  ''fmCreateRwLock'' instead of this function.
 *
 * \param[out]      lck points to a caller-allocated fm_rwLock object to be 
 *                  filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation error.
 * \return          FM_ERR_UNINITIALIZED if ALOS not intialized.
 * \return          FM_ERR_INVALID_ARGUMENT if lck is NULL.
 * \return          FM_ERR_LOCK_INIT if unable to initialize lock.
 * \return          FM_FAIL if unable to create lock.
 *
 *****************************************************************************/
fm_status fmCreateRwLockV2(fm_text lockName, 
                           fm_int  sw,
                           fm_int precedence, 
                           fm_rwLock *lck)
{
    int                 i;
    pthread_mutexattr_t attr;
    pthread_mutex_t *   access;
    sem_t *             read;
    sem_t *             write;
    fm_int              size;
    int                 pterr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, 
                 "lockName=%s  precedence=%d  lck=%p\n",
                 lockName, 
                 precedence,
                 (void *) lck);

    if (fmRootAlos == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_UNINITIALIZED);
    }

    if (!lck)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_LOCK, FM_ERR_INVALID_ARGUMENT);
    }
    
    /* allocate thread specific list */
    size = sizeof(fm_rwLockThreadEntry) * FM_MAX_THREADS;
    lck->userList = (fm_rwLockThreadEntry *) fmAlloc(size);

    if (lck->userList == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
    }

    /* clear thread specific list */
    FM_MEMSET_S(lck->userList, size, 0, size);

    lck->numActiveReaders   = 0;
    lck->numActiveWriters   = 0;
    lck->numPendingReaders  = 0;
    lck->numPendingWriters  = 0;

    if (fmCreateBitArray(&(lck->readerToBePromoted), FM_MAX_THREADS) != FM_OK)
    {
        fmFree(lck->userList);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
    }

    lck->name = fmStringDuplicate(lockName);

    if (!lck->name)
    {
        fmFree(lck->userList);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
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
    
    if ( pthread_mutexattr_init(&attr) )
    {
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) ||
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);
    }

    access = (pthread_mutex_t *) fmAlloc( sizeof(pthread_mutex_t) );

    if (!access)
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
    }

    /* What attributes should we use? */
    if ( pthread_mutex_init(access, &attr) != 0 )
    {
        pterr = pthread_mutexattr_destroy(&attr);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex attr\n",
                         pterr);
        }

        fmFree(access);
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
    }

    if ( pthread_mutexattr_destroy(&attr) )
    {
        pterr = pthread_mutex_destroy(access);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex\n",
                         pterr);
        }
        fmFree(access);
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_INIT);
    }

    read = (sem_t *) fmAlloc( sizeof(sem_t) );

    if (read == NULL)
    {
        pterr = pthread_mutex_destroy(access);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex\n",
                         pterr);
        }
        fmFree(access);
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
    }

    if (sem_init(read, TRUE, 0) == -1)
    {
        pterr = pthread_mutex_destroy(access);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex\n",
                         pterr);
        }
        fmFree(access);
        fmFree(read);
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    write = (sem_t *) fmAlloc( sizeof(sem_t) );

    if (write == NULL)
    {
        pterr = pthread_mutex_destroy(access);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex\n",
                         pterr);
        }
        fmFree(access);
        sem_destroy(read);
        fmFree(read);
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_NO_MEM);
    }

    if (sem_init(write, TRUE, 0) == -1)
    {
        pterr = pthread_mutex_destroy(access);
        if (pterr != 0)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Error %d destroying mutex\n",
                         pterr);
        }
        fmFree(access);
        sem_destroy(read);
        fmFree(read);
        fmFree(write);
        fmFree(lck->userList);
        fmFree(lck->name);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    lck->accessHandle = (void *) access;
    lck->readHandle   = (void *) read;
    lck->writeHandle  = (void *) write;

    DBG_LIST_ADD_RW_LOCK(lck);

#ifdef FM_DBG_RWL_CTR
    TAKE_DBG_CTR();
    fmRootAlos->rwLockDebugCounters[FM_RWL_LCK_CREATE]++;
    GIVE_DBG_CTR();
#endif

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);

}   /* end fmCreateRwLockV2 */




/*****************************************************************************/
/** fmDeleteRwLock
 * \ingroup alosLock
 *
 * \desc            Deallocate a previously created reader/writer lock.
 *                  (Not implemented.)
 *
 * \param[in]       lck is a pointer to the fm_rwLock object to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if unable to delete lock.
 *
 *****************************************************************************/
fm_status fmDeleteRwLock(fm_rwLock *lck)
{
    fm_status        err;
    pthread_mutex_t *access;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, 
                 "lck=%p\n",
                 (void *) lck);

    err = FM_OK;

    access = (pthread_mutex_t *) lck->accessHandle;
    if ( pthread_mutex_destroy(access) )
    {
        err = FM_ERR_LOCK_DESTROY;
    }

    fmFree(lck->accessHandle);
    fmFree(lck->userList);
    fmFree(lck->name);
    fmDeleteBitArray(&(lck->readerToBePromoted));

    if ( sem_destroy( (sem_t *) lck->readHandle ) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_RWLOCK, "Unable to destroy read semaphore\n");

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }
    else
    {
        fmFree(lck->readHandle);
    }

    if ( sem_destroy( (sem_t *) lck->writeHandle ) )
    {
        FM_LOG_FATAL(FM_LOG_CAT_ALOS_RWLOCK, "Unable to destroy write semaphore\n");

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }
    else
    {
        fmFree(lck->writeHandle);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, err);

}   /* end fmDeleteRwLock */




/*****************************************************************************/
/** fmCaptureReadLock
 * \ingroup alosLock
 *
 * \desc            Take a reader/writer lock for read-only access.  This function
 *                  will block only if a writer currently has the lock. Must
 *                  be balanced by a call to ''fmReleaseReadLock''.
 *
 * \param[in]       lck is a pointer to the fm_rwLock object to be taken.
 *
 * \param[in]       timeout is the maximum amount of time in ticks to wait
 *                  blocked on the lock (currently not implemented).
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if lock's user list is full or other unexpected
 *                  error condition.
 *
 *****************************************************************************/
fm_status fmCaptureReadLock(fm_rwLock *lck, fm_timestamp *timeout)
{
    int       index;
    int       firstUnused;
    fm_status err;
    fm_bool   threadFound;
    int       posixError;
    char      strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t   strErrNum;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK,
                 "handle=%p timeout=%p\n",
                 (void *) lck, (void *) timeout);
#endif

    FM_NOT_USED(timeout);

    TAKE_ACCESS(lck);

    /***************************************************
     * Search the thread's user list to find an entry for
     * the calling thread, or the location of the first
     * available entry if the caller is unknown.
     **************************************************/

    threadFound = FindThreadInUserList(lck, &index, &firstUnused);

    /***************************************************
     * Recursive read thread case.  We assume if the
     * thread is in the list and has active readers then
     * this is a recursive call and we need to do nothing.
     * We do the same thing if the thread already has
     * writers because we allow writers to call readers
     * without issue.
     *
     * Note that this function will not exit without
     * eventually setting the thread's numReaders to at
     * least 1. Thus we should never deal with the case 
     * that the thread is found but numReaders for that 
     * thread is zero.
     **************************************************/

    if ( threadFound &&
        ( (lck->userList[index].numReaders > 0) ||
         (lck->userList[index].numWriters > 0) ) )
    {

        if (lck->userList[index].numReaders == 0)
        {
            /* If the thread was only a writer it is now also a 
               NEW active reader. */
            lck->numActiveReaders++;
        }

        lck->userList[index].numReaders++;
        lck->userList[index].takenCount++;

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK_REC_IN_WR]++;
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK]++;
        GIVE_DBG_CTR();
#endif

        GIVE_ACCESS(lck);

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
    }
    else if ( threadFound &&
             (lck->userList[index].numReaders == 0) &&
             (lck->userList[index].numWriters == 0) )
    {
        /* this case should never happen */
        FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                     FALSE,
                     "R-W lock %s has thread in user list, but thread is "
                     "not recorded as a reader or writer!\n",
                     lck->name);
        
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    if ( !threadFound && (firstUnused == -1) )
    {
        /**************************************************
         * No space for another thread on this lock.
         * This should never happen.
         **************************************************/
        
        FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                     FALSE,
                     "R-W lock %s could not add another thread!\n",
                     lck->name);
        
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }
    else if ( !threadFound && (firstUnused != -1) )
    {
        index = firstUnused;
    }
    else
    {
        /**************************************************
         * There shouldn't be any more possibilities, so
         * we should never get here, but catch it just
         * to be defensive.
         **************************************************/
        
        FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                     FALSE,
                     "Unexpected condition on R-W lock:\n"
                     "   lock              : %p\n"
                     "   name              : %s\n"
                     "   numActiveReaders  : %d\n"
                     "   numActiveWriters  : %d\n"
                     "   numPendingReaders : %d\n"
                     "   numPendingWriters : %d\n"
                     "   maxThreads        : %d\n"
                     "   FM_MAX_THREADS    : %d\n",
                     (void *) lck,
                     lck->name,
                     lck->numActiveReaders,
                     lck->numActiveWriters,
                     lck->numPendingReaders,
                     lck->numPendingWriters,
                     lck->maxThreads,
                     FM_MAX_THREADS);
        
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    /* add the thread to the list in the first available entry */
    ADD_THREAD(lck, index);

    /**************************************************
     * Now check the precedence. We have to do this
     * after adding the thread to the lock because
     * some of the data we need to look at in the
     * lock structure is per-thread.
     **************************************************/
    
    VALIDATE_LOCK_PRECEDENCE(err, lck, index, TRUE);
    
    if (err != FM_OK)
    {
        /**************************************************
         * Lock being taken out of order!
         **************************************************/
        
        /* We just added the thread to the lock, so delete it. */
        DEL_THREAD(lck, index);
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_PRECEDENCE);
    }
    
    if ( (lck->numPendingWriters == 0) && (lck->numActiveWriters == 0) )
    {
        /***************************************************
         * No writers pending or active, record ourselves and
         * return, updating counters as necessary. This is a
         * non-blocking case.
         **************************************************/

        lck->userList[index].numReaders++;
        lck->numActiveReaders++;

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();

        if (lck->numActiveReaders > 1)
        {
            fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK_NOBLOCK_MULTI]++;
        }
        else
        {
            fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK_NOBLOCK_SINGLE]++;
        }

        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK]++;
        GIVE_DBG_CTR();
#endif

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
    }
    else
    {
        /***************************************************
         * If there are writers pending then we become a
         * pending reader, blocked on the semaphore
         * which is given when the writers release.
         **************************************************/

        lck->numPendingReaders++;

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK_BLOCK_WR]++;
        GIVE_DBG_CTR();
#endif

        GIVE_ACCESS(lck);

        while (1)
        {
            int ret;
            ret = sem_wait( (sem_t *) lck->readHandle );
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
            }
            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
        }

        lck->userList[index].numReaders++;

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_LCK]++;
        GIVE_DBG_CTR();
#endif
    }

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmCaptureReadLock */




/*****************************************************************************/
/** fmCaptureWriteLock
 * \ingroup alosLock
 *
 * \desc            Take a reader/writer lock for write access.  This function
 *                  will block if a reader or another writer currently has the
 *                  lock.  Must be balanced by a call to ''fmReleaseWriteLock''.
 *
 * \note            If a thread calls this function to capture the lock as
 *                  a writer, it may safely call ''fmCaptureReadLock'' for the
 *                  same lock without causing a deadlock.
 *
 * \param[in]       lck is a pointer to the fm_rwLock object to be taken.
 *
 * \param[in]       timeout is the maximum amount of time in ticks to wait
 *                  blocked on the lock (currently not implemented).
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if lock's user list is full or other unexpected
 *                  error condition.
 *
 *****************************************************************************/
fm_status fmCaptureWriteLock(fm_rwLock *lck, fm_timestamp *timeout)
{
    int       index;
    int       firstUnused;
    fm_bool   threadFound;
    int       posixError;
    fm_status err;
    fm_bool   promoteReaderToWriter = FALSE;
    fm_bool   newThread             = FALSE;
    fm_int    numReaderToBePromoted = 0;
    char      strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t   strErrNum;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK,
                 "handle=%p timeout=%p\n",
                 (void *) lck, (void *) timeout);
#endif

    FM_NOT_USED(timeout);

    TAKE_ACCESS(lck);

    /***************************************************
     * Search the thread's user list to find an entry for
     * the calling thread, or the location of the first
     * available entry if the caller is unknown.
     **************************************************/

    threadFound = FindThreadInUserList(lck, &index, &firstUnused);

    if ( threadFound && (lck->userList[index].numWriters > 0) )
    {
        /***************************************************
         * Recursive write case.  The thread already owns the
         * write lock and so we needn't worry about anything
         * other than updating the thread's counters.
         **************************************************/
    
        lck->userList[index].numWriters++;
        lck->userList[index].takenCount++;

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK_REC_IN_WR]++;
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK]++;
        GIVE_DBG_CTR();
#endif

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
    }
    else if ( threadFound && (lck->userList[index].numWriters == 0) )
    {
        /**************************************************
         * Thread is already a reader and now wants to be
         * a writer.
         **************************************************/
        
        fmSetBitArrayBit(&lck->readerToBePromoted, index, TRUE);

        if ( (lck->numActiveWriters == 0 ) && (lck->numActiveReaders == 1) )
        {
            /**************************************************
             * Promote a reader to a writer. This is allowed 
             * if the current thread is the only reader and 
             * there is no other writer.  At this point 
             * lck->userList[index].numReaders is strictly 
             * positive.
             **************************************************/
             
            promoteReaderToWriter = TRUE;
        }
    }
    else if ( !threadFound && (firstUnused == -1) )
    {
        /**************************************************
         * No space for another thread on this lock.
         * This should never happen.
         **************************************************/
        
        FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                     FALSE,
                     "R-W lock %s could not add another thread!\n",
                     lck->name);
        
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }
    else if ( !threadFound && (firstUnused != -1) )
    {
        /**************************************************
         * Adding a new thread to this lock.
         **************************************************/
        
        index = firstUnused;
    }
    else
    {
        /**************************************************
         * There shouldn't be any more possibilities, so
         * we should never get here, but catch it just
         * to be defensive.
         **************************************************/
        
        FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                     FALSE,
                     "Unexpected condition on R-W lock:\n"
                     "   lock              : %p\n"
                     "   name              : %s\n"
                     "   numActiveReaders  : %d\n"
                     "   numActiveWriters  : %d\n"
                     "   numPendingReaders : %d\n"
                     "   numPendingWriters : %d\n"
                     "   maxThreads        : %d\n"
                     "   FM_MAX_THREADS    : %d\n",
                     (void *) lck,
                     lck->name,
                     lck->numActiveReaders,
                     lck->numActiveWriters,
                     lck->numPendingReaders,
                     lck->numPendingWriters,
                     lck->maxThreads,
                     FM_MAX_THREADS);
        
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    if (index == firstUnused)
    {
        /* Add the thread to the list in the first available entry */
        ADD_THREAD(lck, index);
        newThread = TRUE;
    }

    /**************************************************
     * Now check the precedence. We have to do this
     * after adding the thread to the lock because
     * some of the data we need to look at in the
     * lock structure is per-thread.
     **************************************************/
    
    VALIDATE_LOCK_PRECEDENCE(err, lck, index, TRUE);
    
    if (err != FM_OK)
    {
        /**************************************************
         * Lock being taken out of order!
         **************************************************/
        
        /* If we just added the thread to the lock, then delete it. */
        if (newThread)
        {
            DEL_THREAD(lck, index);
        }
        
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_PRECEDENCE);
    }
    
    if ( (lck->numActiveWriters == 0) && 
         ( (lck->numActiveReaders == 0) || promoteReaderToWriter ) )
    {
        /***************************************************
         * No one is pending, or we are promoting a reader 
         * to be a writer. Update the counters, acquire
         * the lock and return.
         **************************************************/

        fmSetBitArrayBit(&lck->readerToBePromoted, index, FALSE);

        lck->numActiveWriters++;

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK_NOBLOCK]++;
        GIVE_DBG_CTR();
#endif

        lck->userList[index].numWriters++;

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK]++;
        GIVE_DBG_CTR();
#endif
        
    }
    else
    {
        fmGetBitArrayNonZeroBitCount(&lck->readerToBePromoted, &numReaderToBePromoted);
 
        if ( (lck->numActiveWriters == 0) 
             && (lck->numActiveReaders == numReaderToBePromoted) )
        {
            /* There is no writer, and all active readers want to become a 
               reader. Promote the last one. */
            fmSetBitArrayBit(&lck->readerToBePromoted, index, FALSE);
            lck->numActiveWriters++;
            lck->userList[index].numWriters++;

            GIVE_ACCESS(lck);
        }
        else
        {
            lck->numPendingWriters++;

            GIVE_ACCESS(lck);

            /***************************************************
             * Either writers or readers are in flight.  In this
             * case, we block on the writer lock and update
             * counters once we get it.
             **************************************************/

#ifdef FM_DBG_RWL_CTR
            TAKE_DBG_CTR();

            if (lck->numActiveReaders > 0)
            {
                fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK_BLOCK_RD]++;
            }
            else
            {
                fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK_BLOCK_WR]++;
            }

            GIVE_DBG_CTR();
#endif

            while (1)
            {
                int ret;
                ret = sem_wait( (sem_t *) lck->writeHandle );
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
                }
                strErrNum =
                    FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
                if (strErrNum == 0)
                {
                    FM_LOG_FATAL( FM_LOG_CAT_ALOS_RWLOCK,
                                  "Failed to get writer semaphore - %s\n",
                                  strErrBuf );
                }
                else
                {
                    FM_LOG_FATAL( FM_LOG_CAT_ALOS_RWLOCK,
                                  "Failed to get writer semaphore - %d\n",
                                  errno );
                }
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
            }

            lck->userList[index].numWriters++;

            /* If a reader blocks waiting to be promoted, it is now 
               a writer. Reset the "waiting-to-be-promoted" flag. */
            fmSetBitArrayBit(&lck->readerToBePromoted, index, FALSE); 

#ifdef FM_DBG_RWL_CTR
            TAKE_DBG_CTR();
            fmRootAlos->rwLockDebugCounters[FM_RWL_WR_LCK]++;
            GIVE_DBG_CTR();
#endif
        }
    }

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmCaptureWriteLock */




/*****************************************************************************/
/** fmReleaseReadLock
 * \ingroup alosLock
 *
 * \desc            Release a reader's hold on a reader/writer lock. Must be
 *                  balanced by a call to ''fmCaptureReadLock''.
 *
 * \param[in]       lck is a pointer to the fm_rwLock object to be released.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if the lock was not previously captured by this
 *                  thread, or calls to ''fmCaptureReadLock'' and 
 *                  fmReleaseReadLock are not balanced.
 *
 *****************************************************************************/
fm_status fmReleaseReadLock(fm_rwLock *lck)
{
    int       index;
    int       firstUnused;
    fm_bool   threadFound;
    int       posixError;
    fm_status err;
    fm_int    numReaderToBePromoted = 0;
    char      strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t   strErrNum;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, "handle=%p\n", (void *) lck);
#endif

    TAKE_ACCESS(lck);

    /***************************************************
     * Search the thread's user list to find an entry for
     * the calling thread, or the location of the first
     * available entry if the caller is unknown.
     **************************************************/

    threadFound = FindThreadInUserList(lck, &index, &firstUnused);

    /***************************************************
     * At this point the thread needs to have already
     * done a capture, so we check that.
     **************************************************/
     
    if (!threadFound)
    {
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }
    else if (lck->userList[index].numReaders <= 0)
    {
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    /**************************************************
     * If this thread is releasing the lock for
     * the last time, remove it from its lock collection.
     **************************************************/
     
    VALIDATE_LOCK_PRECEDENCE(err, lck, index, FALSE);
    
    if (err != FM_OK)
    {
        /* Lock being released out of order! */
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_PRECEDENCE);
    }
    
    lck->userList[index].numReaders--;

    if ( lck->userList[index].numReaders == 0 ) 
    {

        lck->numActiveReaders--;

        FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                      lck->numActiveReaders >= 0,
                      "Releasing reader lock, but no active readers!\n"
                      "   lock              : %p\n"
                      "   name              : %s\n"
                      "   numActiveReaders  : %d\n"
                      "   numActiveWriters  : %d\n"
                      "   numPendingReaders : %d\n"
                      "   numPendingWriters : %d\n"
                      "   maxThreads        : %d\n",
                      (void *) lck,
                      lck->name,
                      lck->numActiveReaders,
                      lck->numActiveWriters,
                      lck->numPendingReaders,
                      lck->numPendingWriters,
                      lck->maxThreads);

        if (lck->userList[index].numWriters == 0)
        {
            /***************************************************
             * No more readers and writers, the thread is done,
             * make it non-active and delete the entry from our
             * list.
             **************************************************/
            DEL_THREAD(lck, index);
        }


        /***************************************************
         * We don't stop yet because we have to go check
         * on other active or pending threads.
         **************************************************/
    }
    else
    {
        /***************************************************
         * Recursive call case, do nothing if we still are
         * inside a read or a write block.
         **************************************************/

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL_REC_IN_RD]++;
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL]++;
        GIVE_DBG_CTR();
#endif

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
    }

    /* There is either active reader or active writer. */
    if ( (lck->numActiveReaders != 0) || (lck->numActiveWriters != 0) )
    {
        fmGetBitArrayNonZeroBitCount(&lck->readerToBePromoted, &numReaderToBePromoted);
 
        if ( (lck->numActiveWriters == 0) 
             && (lck->numActiveReaders == numReaderToBePromoted) )
        {
            /***************************************************************
             * If there is no active writer, and the current active readers 
             * are all waiting to be promoted to a writer, promote one of the
             * active reader now by giving it the write semaphore.
             **************************************************************/ 
            FM_LOG_ASSERT(FM_LOG_CAT_ALOS_RWLOCK,
                          lck->numPendingWriters > 0,
                          "Promoting a reader when releasing reader lock, "
                          "but there are no pending writers!\n"
                          "   lock              : %p\n"
                          "   name              : %s\n"
                          "   numActiveReaders  : %d\n"
                          "   numActiveWriters  : %d\n"
                          "   numPendingReaders : %d\n"
                          "   numPendingWriters : %d\n"
                          "   maxThreads        : %d\n",
                          (void *) lck,
                          lck->name,
                          lck->numActiveReaders,
                          lck->numActiveWriters,
                          lck->numPendingReaders,
                          lck->numPendingWriters,
                          lck->maxThreads);
    
            lck->numActiveWriters++;
            lck->numPendingWriters--;

            GIVE_ACCESS(lck);

            if (sem_post( (sem_t *) lck->writeHandle ) != 0)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
            }

            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
        }
        else
        { 
            /***************************************************
             * Other readers/writers are active.  Nothing more
             * to do.
             **************************************************/

            GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
            TAKE_DBG_CTR();
            fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL_REC_IN_RD]++;
            fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL]++;
            GIVE_DBG_CTR();
#endif

            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
        }
    }

    /* At this point there is no active readers and writers. */
    if (lck->numPendingWriters > 0)
    {
        /***************************************************
         * Second case, we have pending writers to unblock.
         * Note that we will let any pending readers wait.
         **************************************************/

        lck->numActiveWriters++;
        lck->numPendingWriters--;

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL_NONE]++;
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL]++;
        GIVE_DBG_CTR();
#endif

        if (sem_post( (sem_t *) lck->writeHandle ) != 0)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
        }
    }
    else
    {
        /* No one left, just give back access */
        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL_NONE]++;
        fmRootAlos->rwLockDebugCounters[FM_RWL_RD_REL]++;
        GIVE_DBG_CTR();
#endif
    }

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmReleaseReadLock */




/*****************************************************************************/
/** fmReleaseWriteLock
 * \ingroup alosLock
 *
 * \desc            Release a writer's hold on a reader/writer lock. Must be
 *                  balanced by a call to ''fmCaptureWriteLock''.
 *
 * \param[in]       lck is a pointer to the fm_rwLock object to be released.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if the lock was not previously captured by this
 *                  thread, or calls to ''fmCaptureWriteLock'' and 
 *                  fmReleaseWriteLock are not balanced.
 *
 *****************************************************************************/
fm_status fmReleaseWriteLock(fm_rwLock *lck)
{
    int       index;
    int       firstUnused;
    fm_bool   threadFound;
    int       posixError;
    fm_status err;
    char      strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t   strErrNum;

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_ENTRY(FM_LOG_CAT_ALOS_RWLOCK, "handle=%p\n", (void *) lck);
#endif

    TAKE_ACCESS(lck);

    /***************************************************
     * Search the thread's user list to find an entry for
     * the calling thread, or the location of the first
     * available entry if the caller is unknown.
     **************************************************/

    threadFound = FindThreadInUserList(lck, &index, &firstUnused);

    /***************************************************
     * At this point the thread needs to have already
     * done a capture, so we check that.
     **************************************************/
    if (!threadFound)
    {
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }
    else if (lck->userList[index].numWriters <= 0)
    {
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
    }

    /**************************************************
     * If this thread is releasing the lock for
     * the last time, remove it from its lock collection.
     **************************************************/
     
    VALIDATE_LOCK_PRECEDENCE(err, lck, index, FALSE);
    
    if (err != FM_OK)
    {
        /* Lock being released out of order! */
        GIVE_ACCESS(lck);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_ERR_LOCK_PRECEDENCE);
    }
    
    lck->userList[index].numWriters--;

    if ( lck->userList[index].numWriters == 0 ) 
    {
        lck->numActiveWriters--;

        /***************************************************
         * Only delete the thread when all writers and
         * readers have finished in this thread.
         **************************************************/
        if (lck->userList[index].numReaders == 0)
        {
            DEL_THREAD(lck, index);
        }
    }
    else
    {
        /***************************************************
         * If writers and readers still exist then we are
         * inside a recursive case, so simply continue.
         **************************************************/

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();

        if (lck->userList[index].numWriters >= lck->userList[index].numReaders)
        {
            fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL_REC_IN_WR]++;
        }
        else
        {
            fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL_REC_IN_RD]++;
        }

        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL]++;
        GIVE_DBG_CTR();
#endif

        FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
        
    }   /* end if ( lck->userList[index].numWriters == 0 ) */

    if (lck->numPendingWriters > 0)
    {
        lck->numPendingWriters--;
        lck->numActiveWriters++;

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL_TO_WR]++;
        GIVE_DBG_CTR();
#endif

        /* Let the next pending writer unblock */
        if (sem_post( (sem_t *) lck->writeHandle ) != 0)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
        }

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL]++;
        GIVE_DBG_CTR();
#endif
    }
    else
    {
        /***************************************************
         * Give the reader semaphore for each pending
         * reader.  We need to ensure each time that no
         * pending writers jump in during the loop.
         **************************************************/
#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL_TO_RD]++;
        GIVE_DBG_CTR();
#endif

        while (lck->numPendingReaders > 0)
        {
            lck->numPendingReaders--;
            lck->numActiveReaders++;

            if (sem_post( (sem_t *) lck->readHandle ) != 0)
            {
                GIVE_ACCESS(lck);
                FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_FAIL);
            }
        }

        GIVE_ACCESS(lck);

#ifdef FM_DBG_RWL_CTR
        TAKE_DBG_CTR();
        fmRootAlos->rwLockDebugCounters[FM_RWL_WR_REL]++;
        GIVE_DBG_CTR();
#endif
    }

#ifdef FM_ALOS_LOCK_FUNCTION_LOGGING   /* Performance-sensitive path */
    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);
#else
    return FM_OK;
#endif

}   /* end fmReleaseWriteLock */




/*****************************************************************************/
/* fmDbgDiagDumpRwLockState
 * \ingroup
 *
 * \desc            For debugging this module.
 *
 * \param[in]       sw is the switch number and is not used.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDiagDumpRwLockState(int sw)
{
    fm_rwLock *rwl;
    int        i, j;
    fm_text    threadName;
    fm_status  err;

    FM_NOT_USED(sw);

    i = pthread_mutex_lock(fmRootAlos->dbgRwLockListLock.handle);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_lock\n",
                     i);
    }

    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS ; i++)
    {
        rwl = fmRootAlos->dbgRwLockList[i];

        if (!rwl)
        {
            continue;
        }

        FM_LOG_PRINT("Reader Write Lock %s:\n", rwl->name);
        FM_LOG_PRINT("    Precedence      : ");
        
        if (rwl->precedence)
        {
            FM_LOG_PRINT("0x%08X\n",
                         rwl->precedence);
        }
        else
        {
            FM_LOG_PRINT("(none)\n");
        }
        
        FM_LOG_PRINT("    Active readers  : %d\n", rwl->numActiveReaders);
        FM_LOG_PRINT("    Active writers  : %d\n", rwl->numActiveWriters);
        FM_LOG_PRINT("    Pending readers : %d\n", rwl->numPendingReaders);
        FM_LOG_PRINT("    Pending writers : %d\n", rwl->numPendingWriters);
        FM_LOG_PRINT("\n");
        FM_LOG_PRINT("    %-8s %-5s %-5s\n", "ID", "R", "W");
        FM_LOG_PRINT("    ---------------------\n");

        for (j = 0 ; j < FM_MAX_THREADS ; j++)
        {
            if (rwl->userList[j].used)
            {
                err = fmGetThreadName(rwl->userList[j].id, &threadName);

                if (err != FM_OK)
                {
                    threadName = "** Name Unknown **";
                }

                FM_LOG_PRINT("    %8p %-5d %-5d\n    (%s)\n",
                             (void *) rwl->userList[j].id,
                             rwl->userList[j].numReaders,
                             rwl->userList[j].numWriters,
                             threadName);
            }
        }

        FM_LOG_PRINT("\n");
    }

    i = pthread_mutex_unlock(fmRootAlos->dbgRwLockListLock.handle);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
    }

}   /* end fmDbgDiagDumpRwLockState */




/*****************************************************************************/
/* fmDbgDiagDumpRwLockStats
 * \ingroup
 *
 * \desc            For debugging this module.
 *
 * \param[in]       sw is the switch number and is not used.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDiagDumpRwLockStats(int sw)
{
    FM_NOT_USED(sw);

#ifdef FM_DBG_RWL_CTR
    TAKE_DBG_CTR();

    PRINT_RWL_DBG_CTR("Read/Write Lock Created", FM_RWL_LCK_CREATE);
    PRINT_RWL_DBG_CTR("Read Lock Taken", FM_RWL_RD_LCK);
    PRINT_RWL_DBG_CTR("Write Lock Taken", FM_RWL_WR_LCK);
    PRINT_RWL_DBG_CTR("Read Lock Released", FM_RWL_RD_REL);
    PRINT_RWL_DBG_CTR("Write Lock Released", FM_RWL_WR_REL);
    PRINT_RWL_DBG_CTR("Read Lock Didn't Block (Single Reader)",
                      FM_RWL_RD_LCK_NOBLOCK_SINGLE);
    PRINT_RWL_DBG_CTR("Read Lock Didn't Block (Multiple Readers)",
                      FM_RWL_RD_LCK_NOBLOCK_MULTI);
    PRINT_RWL_DBG_CTR("Read Lock Blocked (Writer Active)",
                      FM_RWL_RD_LCK_BLOCK_WR);
    PRINT_RWL_DBG_CTR("Read Lock Taken Within Reader", FM_RWL_RD_LCK_REC_IN_RD);
    PRINT_RWL_DBG_CTR("Read Lock Taken Within Writer", FM_RWL_RD_LCK_REC_IN_WR);
    PRINT_RWL_DBG_CTR("Write Lock Didn't Block", FM_RWL_WR_LCK_NOBLOCK);
    PRINT_RWL_DBG_CTR("Write Lock Blocked (Reader Active)",
                      FM_RWL_WR_LCK_BLOCK_RD);
    PRINT_RWL_DBG_CTR("Write Lock Blocked (Writer Active)",
                      FM_RWL_WR_LCK_BLOCK_WR);
    PRINT_RWL_DBG_CTR("Write Lock Taken Within Writer", FM_RWL_WR_LCK_REC_IN_WR);
    PRINT_RWL_DBG_CTR("Read Lock Release (No Threads Left)", FM_RWL_RD_REL_NONE);
    PRINT_RWL_DBG_CTR("Read Lock Release (New Reader Active)",
                      FM_RWL_RD_REL_TO_RD);
    PRINT_RWL_DBG_CTR("Read Lock Release (New Writer Active)",
                      FM_RWL_RD_REL_TO_WR);
    PRINT_RWL_DBG_CTR("Read Lock Release Within Reader",
                      FM_RWL_RD_REL_REC_IN_RD);
    PRINT_RWL_DBG_CTR("Read Lock Release Within Writer",
                      FM_RWL_RD_REL_REC_IN_WR);
    PRINT_RWL_DBG_CTR("Write Lock Release (No Threads Left)",
                      FM_RWL_WR_REL_NONE);
    PRINT_RWL_DBG_CTR("Write Lock Release (New Reader Active)",
                      FM_RWL_WR_REL_TO_RD);
    PRINT_RWL_DBG_CTR("Write Lock Release (New Writer Active)",
                      FM_RWL_WR_REL_TO_WR);
    PRINT_RWL_DBG_CTR("Write Lock Release Within Reader",
                      FM_RWL_WR_REL_REC_IN_RD);
    PRINT_RWL_DBG_CTR("Write Lock Release Within Writer",
                      FM_RWL_WR_REL_REC_IN_WR);

    GIVE_DBG_CTR();
#endif

}   /* end fmDbgDiagDumpRwLockStats */




/*****************************************************************************/
/** fmDbgDiagCheckTaskState
 * \ingroup intAlos
 *
 * \desc            Debug function to scan all r/w locks and report if the
 *                  current thread holds any locks.
 *                  This function can be called by user code after return from
 *                  an API call to ensure that no r/w locks were left taken,
 *                  which could lead to a system deadlock.
 *
 * \param[in]       funcPtr contains the API function pointer that was called
 *                  which led to a lock not being released, if any unreleased
 *                  locks are found.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDiagCheckTaskState(unsigned int funcPtr)
{
    fm_rwLock *rwl;
    int        i;
    int        j;
    void *     id = fmGetCurrentThreadId();

    FM_NOT_USED(funcPtr);

    i = pthread_mutex_lock(fmRootAlos->dbgRwLockListLock.handle);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_lock\n",
                     i);
    }

    for (i = 0 ; i < FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS ; i++)
    {
        rwl = fmRootAlos->dbgRwLockList[i];

        if (!rwl)
        {
            continue;
        }

        for (j = 0 ; j <= rwl->maxThreads ; j++)
        {
            if ( rwl->userList[j].used && (rwl->userList[j].id == id) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_ALOS_RWLOCK,
                             "Thread %p (func ptr %08X) has lock %s:\n",
                             id, funcPtr, rwl->name);
                FM_LOG_DEBUG(FM_LOG_CAT_ALOS_RWLOCK,
                             "    numReaders = %d, numWriters = %d\n",
                             rwl->userList[j].numReaders,
                             rwl->userList[j].numWriters);
            }
        }
    }

    i = pthread_mutex_unlock(fmRootAlos->dbgRwLockListLock.handle);
    if (i != 0)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ALOS_RWLOCK,
                     "Error %d from pthread_mutex_unlock\n",
                     i);
    }

}   /* end fmDbgDiagCheckTaskState */




/*****************************************************************************/
/** fmGetThreadRwLockStatus
 * \ingroup alosLock
 *
 * \desc            Determine if the current thread holds any read or write 
 *                  locks on a specified lock. After execution, if reads and 
 *                  writes are both zero, this thread does not hold the 
 *                  specified lock in any way.
 *
 * \note            This function is not called by the API or by ALOS 
 *                  internally. It is a utulity function for use by the 
 *                  appilcation for debugging purposes. Implementation of this
 *                  function in the ALOS layer is optional.
 *
 * \param[in]       lck points to the read/write lock structure.
 *
 * \param[out]      reads points to caller-allocated storage which will
 *                  be modified by this function to contain the number of times
 *                  the current thread holds read-locks on this lock.
 *
 * \param[out]      writes points to caller-allocated storage which will be
 *                  modified by this function to contain the number of times
 *                  the current thread holds write-locks on this lock.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetThreadRwLockStatus(fm_rwLock *lck, fm_int *reads, fm_int *writes)
{
    fm_int  index;
    fm_int  firstUnused;
    fm_bool threadFound;
    int     posixError;
    char    strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t strErrNum;

    FM_LOG_ENTRY( FM_LOG_CAT_ALOS_RWLOCK,
                 "handle=%p, reads=%p, writes=%p\n",
                 (void *) lck,
                 (void *) reads,
                 (void *) writes );

    TAKE_ACCESS(lck);

    /***************************************************
     * Search the thread's user list to find an entry for
     * the calling thread, or the location of the first
     * available entry if the caller is unknown.
     **************************************************/

    threadFound = FindThreadInUserList(lck, &index, &firstUnused);

    if (threadFound)
    {
        *reads  = lck->userList[index].numReaders;
        *writes = lck->userList[index].numWriters;
    }
    else
    {
        *reads  = 0;
        *writes = 0;
    }

    GIVE_ACCESS(lck);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS_RWLOCK, FM_OK);

}   /* end fmGetThreadRwLockStatus */
