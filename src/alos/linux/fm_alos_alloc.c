/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm_alos_alloc.c
 * Creation Date:  June 04, 2007
 * Description:    Platform-independent memory allocation.
 *
 * Copyright (c) 2007 - 2013, Intel Corporation
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
#include <common/fm_version.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define MEMORY_DEBUG_CALLER  FALSE
#define FM_SHM_TIMEOUT       60

#if MEMORY_DEBUG_CALLER

#define DBG_FULL_CALLER_DEPTH FALSE

/* Use 0 to track all buckets, non-zero to only track callers for a range of
 * buckets */

#define DBG_TRACK_MIN_BUCKET  0
#define DBG_TRACK_MAX_BUCKET  0

#if DBG_FULL_CALLER_DEPTH
#define DBG_CALLER_DEPTH      10
#endif

#endif  /* MEMORY_DEBUG_CALLER */

#ifdef FM_HAVE_VALGRIND

/* See http://www.valgrind.org/docs/manual/mc-manual.html#mc-manual.mempools. */
#include <valgrind/memcheck.h>

#define FM_VALGRIND_MAKE_MEMBER_MEM_DEFINED(offset, type, member)              \
    VALGRIND_MAKE_MEM_DEFINED( (offset) + offsetof(type, member),              \
                               sizeof( ( (type *) 0 )->member ) )

#define FM_VALGRIND_MAKE_BUCKET_MEM_DEFINED(hdr, ptr)                          \
    (ptr) = (hdr)->buckets;                                                    \
    while ((ptr) != NULL)                                                      \
    {                                                                          \
        VALGRIND_MAKE_MEM_DEFINED((ptr), sizeof(fm_memoryBucket));             \
        (ptr) = (ptr)->next;                                                   \
    }

#define FM_VALGRIND_MAKE_BUCKET_MEM_NOACCESS(hdr, ptr, next)                   \
    (ptr) = (hdr)->buckets;                                                    \
    while ((ptr) != NULL)                                                      \
    {                                                                          \
        (next) = (ptr)->next;                                                  \
        VALGRIND_MAKE_MEM_NOACCESS((ptr), sizeof(fm_memoryBucket));            \
        (ptr) = (next);                                                        \
    }

#endif  /* FM_HAVE_VALGRIND */

/* Holds free objects of a particular size */
typedef struct _fm_memoryBucket
{
    /* Must equal BUCKET_SIGNATURE; used to detect corruption */
    fm_uint signature;

    /* Size, in bytes, of the objects kept in this bucket */
    fm_uint size;

    /* Linked list of free objects (first word contains next pointer) */
    void *  freeList;

    /* Next-larger-sized memory bucket (must be in sorted order) */
    struct _fm_memoryBucket *next;

    /* This is purely for statistics.  Since bucket sizes are a multiple
     * of 8 bytes, there may be 0-7 bytes of padding at the end.
     * This byte is a bitmask of which of these sizes have ever been
     * allocated from this bucket.  bit 0 = have we ever done an allocation
     * from this bucket that had 0 bytes of padding ... bit 7 = have we
     * ever done an allocation from this bucket that had 7 bytes of padding. */
    fm_byte allocationRemainderBitmask;

} fm_memoryBucket;


/* Registers a "root" (essentially the global variables for a particular
 * named subsystem) */
typedef struct _fm_rootInfo
{
    /* Name of this root */
    const char *name;

    /* Was there an error when initializing the root? */
    fm_status   err;

    /* Pointer to the root data structure itself */
    void *      root;

    /* These are chained together in a linked list */
    struct _fm_rootInfo *next;

} fm_rootInfo;


/* Structure which appears at the beginning of the shared memory */
typedef struct _fm_sharedHeader
{
    /* Check that we are all using the same version of the API */
    fm_uint64        versionIdentifier;

    /* Indicates whether this header has been initialized */
    volatile fm_bool initialized;

    /* Sanity check that everyone is mapping this at the same address */
    void *           self;

    /* SHARED_MEMORY_ADDR + SHARED_MEMORY SIZE; another sanity check */
    void *           end;

    /* Sanity check that the shared library is loaded at the right address
     * by making sure everyone agrees on the address of an arbitrary
     * function in it. */
    void (*funcInSharedLibrary)(void);

    /* Linked list of memory buckets */
    fm_memoryBucket *buckets;

    /* First byte in the shared memory that has never been allocated */
    void *           freeSpace;

    /* Mutex used to lock "buckets" and "freeSpace" during alloc/free */
    pthread_mutex_t  mutex;

    /* Mutex used to lock root list during fmGetRoot */
    pthread_mutex_t  rootMutex;

    /* Memory bucket for holding fm_memoryBucket-sized memory.
     * (All other buckets are allocated like normal objects, but this
     * one is needed to solve the chicken-and-egg problem.) */
    fm_memoryBucket  bucketBucket;

    /* Linked list of roots, essentially forming a map of strings to
     * arbitrary pointers. */
    fm_rootInfo *    roots;

} fm_sharedHeader;


/* Additional information kept below the allocated object */
typedef struct _fm_objectHeader
{
    /* Memory bucket this object was allocated from */
    fm_memoryBucket *myBucket;

#if MEMORY_DEBUG_CALLER
    void *           caller;
#if DBG_FULL_CALLER_DEPTH
    void *           callerArray[DBG_CALLER_DEPTH];
    size_t           callerDepth;
#endif
#endif

} fm_objectHeader;


#if MEMORY_DEBUG_CALLER
typedef struct _fm_callerInfo
{
    /* number of objects allocated by this caller */
    fm_uint32 count;

    /* return value of backtrace_symbols */
    char **   caller;

} fm_callerInfo;

#include <execinfo.h>         /* prototypes for backtrace, backtrace_symbols */
#endif


#define BUCKET_SIGNATURE     0x4ffe874

#define IN_SHARED_MEMORY(x)                                         \
    ( (void *) (x) >= (void *) FM_SHARED_MEMORY_ADDR &&             \
      (void *) (x) < (void *) ((fm_uintptr)FM_SHARED_MEMORY_ADDR +  \
                               (fm_uintptr)FM_SHARED_MEMORY_SIZE) )

/* Round up to a multiple of 8 */
#define ROUND_UP(x)  ( ( (x - 1) | 7 ) + 1 )

/* FNV hash constants from http://isthe.com/chongo/tech/comp/fnv/ */
#define FNV_OFFSET_BASIS_64  FM_LITERAL_U64(14695981039346656037)
#define FNV_PRIME_64         FM_LITERAL_U64(1099511628211)

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* Process local variable indicating whether the process created the SHM */
fm_bool processCreatedSHM = FALSE;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** VersionIdentifier
 * \ingroup intAlosAlloc
 *
 * \desc            Return an integer which is unique to this version of
 *                  the API.
 *
 * \param           None.
 *
 * \return          The unique integer.
 *
 *****************************************************************************/
static fm_uint64 VersionIdentifier(void)
{
    const char *const versionString = FM_BUILD_IDENTIFIER
                                      " $Id: //depot/sw/sdk/focalpoint4.0/release-4.3/src/alos/linux/fm_alos_alloc.c#1 $";

    /**************************************************
     * Compute the 64-bit FNV hash of the above string,
     * to try to get a number which is unique to the
     * current version of the API.
     **************************************************/

    fm_uint64         result = FNV_OFFSET_BASIS_64;
    fm_uint           i;
    fm_uint           len = strlen(versionString);

    for (i = 0 ; i < len ; i++)
    {
        result = (result * FNV_PRIME_64) ^ versionString[i];
    }

    return result;

}   /* end VersionIdentifier */




/*****************************************************************************/
/** MemoryCorruptionWarning
 * \ingroup intAlosAlloc
 *
 * \desc            Log a fatal error indicating memory corruption has
 *                  occurred.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
static void MemoryCorruptionWarning(void)
{
    FM_LOG_FATAL(FM_LOG_CAT_ALOS, "Memory has been corrupted!\n");

}   /* end MemoryCorruptionWarning */




/*****************************************************************************/
/** GetBucket
 * \ingroup intAlosAlloc
 *
 * \desc            Return a pointer to a memory bucket of the requested
 *                  size.
 *
 * \param[in]       hdr points to the shared memory header.
 *
 * \param[in]       size is the number of bytes to allocate.
 *
 * \return          Pointer to a memory bucket for the requested size or NULL
 *                  if none available.
 *
 *****************************************************************************/
static fm_memoryBucket *GetBucket(fm_sharedHeader *hdr, fm_uint size)
{
    fm_memoryBucket **ptr = &hdr->buckets;
    fm_memoryBucket * bucket;

#ifdef FM_HAVE_VALGRIND
    fm_memoryBucket * it;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "hdr=%p size=%d\n",
                 (void *) hdr, size);

    while ( ( *ptr != NULL ) && ( (*ptr)->size < size ) )
    {
        if ( (*ptr)->signature != BUCKET_SIGNATURE )
        {
            MemoryCorruptionWarning();
            bucket = NULL;
            goto ABORT;
        }

        ptr = &( (*ptr)->next );
    }

    bucket = *ptr;

    if ( ( *ptr == NULL ) || ( (*ptr)->size != size ) )
    {
        bucket = fmAlloc( sizeof(fm_memoryBucket) );

        if (bucket == NULL)
        {
            bucket = NULL;
            goto ABORT;
        }

#ifdef FM_HAVE_VALGRIND
        /* Temporarily re-whitelist access to the memory bucket linked list. */
        FM_VALGRIND_MAKE_BUCKET_MEM_DEFINED(hdr, it);
#endif

        bucket->signature                  = BUCKET_SIGNATURE;
        bucket->size                       = size;
        bucket->freeList                   = NULL;
        bucket->next                       = *ptr;
        bucket->allocationRemainderBitmask = 0;

        /* Insert the new bucket prior to the bucket pointed to by ptr. */
        *ptr = bucket;
    }

ABORT:
    FM_LOG_DEBUG(FM_LOG_CAT_ALOS,
                 "Exiting... (bucket=%p)\n", (void *) bucket);

    return bucket;

}   /* end GetBucket */




/*****************************************************************************/
/** LockMutex
 * \ingroup intAlosAlloc
 *
 * \desc            Lock the shared memory.
 *
 * \param[in]       hdr points to the shared memory header.
 *
 * \return          None.
 *
 *****************************************************************************/
static void LockMutex(fm_sharedHeader *hdr)
{
    int     posixError;
    char    buf[FM_STRERROR_BUF_SIZE];
    errno_t errnum;

    if ( ( posixError = pthread_mutex_lock( &(hdr->mutex) ) ) != 0 )
    {
        errnum = FM_STRERROR_S(buf, FM_STRERROR_BUF_SIZE, posixError);
        if (errnum == 0)
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't lock mutex: %s\n", buf );
        }
        else
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't lock mutex: %d\n",
                          posixError );
        }
    }

}   /* end LockMutex */




/*****************************************************************************/
/** UnlockMutex
 * \ingroup intAlosAlloc
 *
 * \desc            Unlock the shared memory.
 *
 * \param[in]       hdr points to the shared memory header.
 *
 * \return          None.
 *
 *****************************************************************************/
static void UnlockMutex(fm_sharedHeader *hdr)
{
    int     posixError;
    char    buf[FM_STRERROR_BUF_SIZE];
    errno_t errnum;

    if ( ( posixError = pthread_mutex_unlock( &(hdr->mutex) ) ) != 0 )
    {
        errnum = FM_STRERROR_S(buf, FM_STRERROR_BUF_SIZE, posixError);
        if (errnum == 0)
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't unlock mutex: %s\n", buf );
        }
        else
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't unlock mutex: %d\n",
                          posixError );
        }
    }

}   /* end UnlockMutex */




/*****************************************************************************/
/** LockRootMutex
 * \ingroup intAlosAlloc
 *
 * \desc            Lock the root list.
 *
 * \param[in]       hdr points to the shared memory header.
 *
 * \return          None.
 *
 *****************************************************************************/
static void LockRootMutex(fm_sharedHeader *hdr)
{
    int     posixError;
    char    buf[FM_STRERROR_BUF_SIZE];
    errno_t errnum;

    if ( ( posixError = pthread_mutex_lock( &(hdr->rootMutex) ) ) != 0 )
    {
        errnum = FM_STRERROR_S(buf, FM_STRERROR_BUF_SIZE, posixError);
        if (errnum == 0)
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't lock root mutex: %s\n",
                          buf );
        }
        else
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't lock root mutex: %d\n",
                          posixError );
        }
    }

}   /* end LockRootMutex */




/*****************************************************************************/
/** UnlockRootMutex
 * \ingroup intAlosAlloc
 *
 * \desc            Unlock the root list.
 *
 * \param[in]       hdr points to the shared memory header.
 *
 * \return          None.
 *
 *****************************************************************************/
static void UnlockRootMutex(fm_sharedHeader *hdr)
{
    int     posixError;
    char    buf[FM_STRERROR_BUF_SIZE];
    errno_t errnum;

    if ( ( posixError = pthread_mutex_unlock( &(hdr->rootMutex) ) ) != 0 )
    {
        errnum = FM_STRERROR_S(buf, FM_STRERROR_BUF_SIZE, posixError);
        if (errnum == 0)
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't unlock root mutex: %s\n",
                          buf );
        }
        else
        {
            FM_LOG_FATAL( FM_LOG_CAT_ALOS, "Couldn't unlock root mutex: %d\n",
                          posixError );
        }
    }

}   /* end UnlockRootMutex */




#if MEMORY_DEBUG_CALLER
static void DeleteCallerInfo(void *p)
{
    fm_callerInfo *info = (fm_callerInfo *) p;

    free(info->caller);
    free(p);

}   /* end DeleteCallerInfo */




#endif

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAlloc
 * \ingroup alosAlloc
 *
 * \desc            Allocates memory.
 *
 * \param[in]       size is the number of bytes to allocate
 *
 * \return          pointer to allocated memory if successful.
 *                  NULL if not successful.
 *
 *****************************************************************************/
void *fmAlloc(fm_uint size)
{
    fm_memoryBucket *bucket;
    fm_objectHeader *objHdr;
    fm_sharedHeader *hdr = (fm_sharedHeader *) FM_SHARED_MEMORY_ADDR;
    fm_uint          originalSize;
    fm_uint          unroundedSize;
    unsigned char *  ptr;
    void *           newObject = NULL;

#ifdef FM_HAVE_VALGRIND
    fm_memoryBucket *it;
    fm_memoryBucket *next;
#endif

#if MEMORY_DEBUG_CALLER
#if !DBG_FULL_CALLER_DEPTH
    void *           btBuffer[2];
#endif
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "size=%d\n", size);

    if (size == 0)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ALOS, NULL, "size==0, returning NULL\n");
    }

    if (size >= FM_SHARED_MEMORY_SIZE)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ALOS,
                           NULL,
                           "size >= FM_SHARED_MEMORY_SIZE (%u), "
                           "returning NULL\n",
                           FM_SHARED_MEMORY_SIZE);
    }

    LockMutex(hdr);

#ifdef FM_HAVE_VALGRIND
    /* Temporarily whitelist access to the memory bucket linked list. */
    FM_VALGRIND_MAKE_BUCKET_MEM_DEFINED(hdr, it);
#endif

    originalSize  = size;
    size         += sizeof(fm_objectHeader);
    unroundedSize = size;
    size          = ROUND_UP(size);
    bucket        = GetBucket(hdr, size);

    if (bucket != NULL)
    {
        newObject = bucket->freeList;

        if (newObject == NULL)
        {
            ptr = hdr->freeSpace;

            objHdr = (fm_objectHeader *) ptr;
#ifdef FM_HAVE_VALGRIND
            VALGRIND_MEMPOOL_ALLOC((void *) FM_SHARED_MEMORY_ADDR,
                                   objHdr,
                                   sizeof(fm_objectHeader));
#endif

            if ( (void *) (ptr + (fm_uintptr)size) <=
                 (void *) ((fm_uintptr)FM_SHARED_MEMORY_ADDR +
                           (fm_uintptr)FM_SHARED_MEMORY_SIZE) )
            {
                newObject = ptr + sizeof(fm_objectHeader);
#ifdef FM_HAVE_VALGRIND
                VALGRIND_MEMPOOL_ALLOC((void *) FM_SHARED_MEMORY_ADDR,
                                       newObject,
                                       size - sizeof(fm_objectHeader));
#endif

                objHdr->myBucket = bucket;

                hdr->freeSpace = ptr + size;

            } /* else newObject stays NULL; we are out of space */
        }
        else
        {
#ifdef FM_HAVE_VALGRIND
            VALGRIND_MEMPOOL_ALLOC((void *) FM_SHARED_MEMORY_ADDR,
                                   newObject,
                                   size - sizeof(fm_objectHeader));
#endif

            bucket->freeList = *(void **) newObject;

            ptr = newObject;

            objHdr = (fm_objectHeader *) ( ptr - sizeof(fm_objectHeader) );
#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_DEFINED(objHdr, sizeof(fm_objectHeader));
#endif
        }

        if (newObject)
        {
            /* set info only when newObject allocated succesfully */
#if MEMORY_DEBUG_CALLER
#if DBG_FULL_CALLER_DEPTH
#if DBG_TRACK_MAX_BUCKET != 0
            if ( ( originalSize >= DBG_TRACK_MIN_BUCKET ) &&
                 ( originalSize <= DBG_TRACK_MAX_BUCKET ) )
            {
#endif  /* DBG_TRACK_MAX_BUCKET != 0 */
                objHdr->callerDepth = backtrace(objHdr->callerArray,
                                                DBG_CALLER_DEPTH);
                objHdr->caller = objHdr->callerArray[1];
#if DBG_TRACK_MAX_BUCKET != 0
            }
            else
            {
                /* first function in backtrace is fmAlloc itself, so we need
                 * the second function */
                objHdr->callerDepth = backtrace(objHdr->callerArray, 2);
                objHdr->caller      = objHdr->callerArray[1];
            }
#endif  /* DBG_TRACK_MAX_BUCKET != 0 */
#else
            /* first function in backtrace is fmAlloc itself, so we need the
             * second function */
            backtrace(btBuffer, 2);
            objHdr->caller = btBuffer[1];
#endif  /* DBG_FULL_CALLER_DEPTH */
#endif  /* MEMORY_DEBUG_CALLER */

#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(objHdr, sizeof(fm_objectHeader));
#endif

            bucket->allocationRemainderBitmask |= 1 << (size - unroundedSize);
        }
    }

#ifdef FM_HAVE_VALGRIND
    /* Blacklist access to the memory bucket linked list. */
    FM_VALGRIND_MAKE_BUCKET_MEM_NOACCESS(hdr, it, next);
#endif

    UnlockMutex(hdr);

    FM_LOG_DEBUG(FM_LOG_CAT_ALOS,
                  "Exiting... (object=%p)\n", newObject);

    return newObject;

}   /* end fmAlloc */




/*****************************************************************************/
/** fmFree
 * \ingroup alosAlloc
 *
 * \desc            Deallocates memory which was allocated by fmAlloc.
 *
 * \param[in]       obj is the memory to deallocate
 *
 * \return          None
 *
 *****************************************************************************/
void fmFree(void *obj)
{
    fm_memoryBucket *bucket;
    fm_objectHeader *objHdr;
    fm_sharedHeader *hdr = (fm_sharedHeader *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  ptr;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "object=%p\n", obj);

    if ( !IN_SHARED_MEMORY(obj) )
    {
        void *traceArray[100];
        size_t traceSize;

        FM_LOG_FATAL( FM_LOG_CAT_ALOS,
                     "Tried to fmFree object at %p, not in shared memory "
                     "range %p-%p\n", obj, (void *) FM_SHARED_MEMORY_ADDR,
                     (void *) ((fm_uintptr)FM_SHARED_MEMORY_ADDR +
                               (fm_uintptr)FM_SHARED_MEMORY_SIZE -
                               (fm_uintptr)1) );

        traceSize = backtrace(traceArray, 100);
        backtrace_symbols_fd( traceArray, traceSize, 1 );
    }
    else
    {
        LockMutex(hdr);

        ptr = obj;
        ptr -= sizeof(fm_objectHeader);

        objHdr = (fm_objectHeader *) ptr;
#ifdef FM_HAVE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED(objHdr, sizeof(fm_objectHeader));
#endif

        bucket = objHdr->myBucket;
#ifdef FM_HAVE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED(bucket, sizeof(fm_memoryBucket));
#endif

        if (!IN_SHARED_MEMORY(bucket) || bucket->signature != BUCKET_SIGNATURE)
        {
            MemoryCorruptionWarning();
        }
        else
        {
            *(void **) obj   = bucket->freeList;
            bucket->freeList = obj;
#if MEMORY_DEBUG_CALLER
            objHdr->caller = NULL;
#if DBG_FULL_CALLER_DEPTH
            FM_CLEAR(objHdr->callerArray);
            objHdr->callerDepth = 0;
#endif
#endif  /* MEMORY_DEBUG_CALLER */
        }

#ifdef FM_HAVE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(bucket, sizeof(fm_memoryBucket));
        VALGRIND_MAKE_MEM_NOACCESS(objHdr, sizeof(fm_objectHeader));
#endif

        UnlockMutex(hdr);
    }

#ifdef FM_HAVE_VALGRIND
    /* Note: The object header is never deallocated. */
    VALGRIND_MEMPOOL_FREE((void *) FM_SHARED_MEMORY_ADDR, obj);
#endif

}   /* end fmFree */




/*****************************************************************************/
/** fmDbgDumpAllocStats
 * \ingroup diagMisc
 *
 * \desc            Dump memory allocator statistics.
 *
 * \param           None
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpAllocStats(void)
{
    fm_sharedHeader *hdr   = (fm_sharedHeader *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  start = (unsigned char *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  ptr;
    fm_uint          offset;
    fm_memoryBucket *bucket;
    char             requested[256];
    fm_uint          size;
    fm_uint          used;
    fm_uint          unused;
    fm_uint          total;
    void *           freeList;
    fm_objectHeader *objHdr;
    fm_int           i;
    char *           buf;
    fm_int           bufSize;
    fm_int           retVal;
    fm_uint          overhead;
    fm_uint          freed = 0;
    fm_uint          managed;
    fm_uint          bucketSpace;
    fm_rootInfo *    rootInfo;
    FILE *           f;

#ifdef FM_HAVE_VALGRIND
    fm_memoryBucket *it;
    fm_memoryBucket *next;
#endif

#if MEMORY_DEBUG_CALLER
    fm_tree          treeOfCallers;
    fm_uint64        key;
    void *           value;
    fm_callerInfo *  callerInfo;
    fm_treeIterator  ti;
    char *           first;
    char *           last;
    char *           shortName;
#if DBG_TRACK_MAX_BUCKET != 0
#define MAX_TRACKED_BUCKETS 1000
    fm_uint          trackedBuckets[MAX_TRACKED_BUCKETS];
    fm_int           nextTrackedBucket = 0;
#endif
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "(no arguments)\n");

    LockMutex(hdr);

    FM_LOG_PRINT("\n");

    bucketSpace = 0;

    managed = ( (unsigned char *) (hdr->freeSpace) ) - start;

    offset = sizeof(fm_sharedHeader);

    while ( ( ( offset + sizeof(fm_objectHeader) ) & 7 ) != 0 )
    {
        offset++;
    }

#ifdef FM_HAVE_VALGRIND
    /* Temporarily whitelist access to the memory bucket linked list. */
    FM_VALGRIND_MAKE_BUCKET_MEM_DEFINED(hdr, it);
#endif

    start   += offset;
    bucket   = hdr->buckets;
    overhead = offset;

    /**************************************************
     * Loop through all the buckets
     **************************************************/
    while (bucket != NULL)
    {
        size     = bucket->size - sizeof(fm_objectHeader);
        unused   = 0;
        total    = 0;
        freeList = bucket->freeList;

        /**************************************************
         * Count the amount of free space on this bucket's freelist
         **************************************************/
        while (freeList != NULL)
        {
            unused++;
            freeList = *(void **) freeList;
            freed   += size;
        }

        /**************************************************
         * Walk through all the object headers, and count the
         * amount of space that points to this bucket (whether
         * allocated or unallocated)
         **************************************************/
#if MEMORY_DEBUG_CALLER
        fmTreeInitWithAllocator(&treeOfCallers, (fmAllocFunc) malloc, free);
#endif
        ptr = start;

        while ( (void *) ptr < hdr->freeSpace )
        {
            objHdr = (fm_objectHeader *) ptr;
#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_DEFINED(objHdr, sizeof(fm_objectHeader));
#endif

            if (objHdr->myBucket == bucket)
            {
                total++;
                overhead += sizeof(fm_objectHeader);
#if MEMORY_DEBUG_CALLER

                if (objHdr->caller != NULL)
                {
                    /**************************************************
                     * For each allocated object of the current
                     * bucket size, add the address of its caller
                     * into a tree.  The tree's key is the caller address
                     * and the value is a structure which contains
                     * a count of the number of objects with this
                     * caller, and the string name of the caller.
                     **************************************************/
                    key = (fm_uint64) (unsigned long) objHdr->caller;

                    if (fmTreeFind(&treeOfCallers, key, &value) == FM_OK)
                    {
                        callerInfo = (fm_callerInfo *) value;
                        callerInfo->count++;
                    }
                    else
                    {
                        value = malloc( sizeof(fm_callerInfo) );

                        if (value != NULL)
                        {
                            callerInfo         = (fm_callerInfo *) value;
                            callerInfo->count  = 1;
                            callerInfo->caller =
                                backtrace_symbols(&objHdr->caller, 1);
                            fmTreeInsert(&treeOfCallers, key, value);
                        }
                    }

#if DBG_TRACK_MAX_BUCKET != 0
                    if ( (size >= DBG_TRACK_MIN_BUCKET)
                        && (size <= DBG_TRACK_MAX_BUCKET) )
                    {
                        for (i = 0 ; i < nextTrackedBucket ; i++)
                        {
                            if (size == trackedBuckets[i])
                            {
                                break;
                            }
                        }

                        if ( (i >= nextTrackedBucket)
                            && (nextTrackedBucket < MAX_TRACKED_BUCKETS) )
                        {
                            trackedBuckets[nextTrackedBucket++] = size;
                        }
                    }
#endif
                }

#endif
            }

            ptr += objHdr->myBucket->size;

#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(objHdr, sizeof(fm_objectHeader));
#endif
        }

        used = total - unused;

        buf     = requested;
        bufSize = sizeof(requested);
        *buf    = 0;

        /**************************************************
         * Since the bucket sizes are multiples of 8 bytes,
         * that means there are eight possible requested sizes
         * that could be lumped into one bucket.
         * allocationRemainderBitmask keeps track of which of
         * those eight possibilities have been requested, and
         * we print that out here.
         **************************************************/
        for (i = 7 ; i >= 0 ; i--)
        {
            if ( ( bucket->allocationRemainderBitmask & (1 << i) ) != 0 )
            {
                retVal = FM_SNPRINTF_S(buf, bufSize, "%s%u",
                                       (buf != requested ? ", " : ""), size - i);

                if (retVal >= 0 && retVal < bufSize)
                {
                    buf     += retVal;
                    bufSize -= retVal;
                }
            }
        }

        FM_LOG_PRINT("Bucket size %u: %u used/%u total (Requested sizes: %s)\n",
                     size, used, total, requested);

#if MEMORY_DEBUG_CALLER
        fmTreeIterInit(&ti, &treeOfCallers);

        while (fmTreeIterNext(&ti, &key, &value) == FM_OK)
        {
            callerInfo = (fm_callerInfo *) value;
            shortName  = callerInfo->caller[0];
            first      = strchr(shortName, '(');
            last       = strchr(shortName, ')');

            if (first == NULL || last == NULL)
            {
                first = strchr(shortName, '[');
                last  = strchr(shortName, ']');
            }

            if (first != NULL && last != NULL)
            {
                *last     = 0;
                shortName = first + 1;
            }

            FM_LOG_PRINT("    %u object%s allocated by %s\n",
                         callerInfo->count,
                         callerInfo->count == 1 ? "" : "s",
                         shortName);
        }

        fmTreeDestroy(&treeOfCallers, DeleteCallerInfo);
#endif
        /**************************************************
         * Add the sum of all memory consumed by memory 
         * buckets. This memory is never freed. 
         **************************************************/
        bucketSpace += ROUND_UP(sizeof(fm_memoryBucket));

        bucket = bucket->next;
    }

#ifdef FM_HAVE_VALGRIND
    /* Blacklist access to the memory bucket linked list. */
    FM_VALGRIND_MAKE_BUCKET_MEM_NOACCESS(hdr, it, next);
#endif

#if DBG_TRACK_MAX_BUCKET != 0
    FM_LOG_PRINT("\nDump of Call Stacks for %d Tracked Buckets\n", nextTrackedBucket);
    for (i = 0 ; i < nextTrackedBucket ; i++)
    {
        fmDbgDumpAllocCallStacks(trackedBuckets[i]);
    }
#endif

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Currently allocated: %u bytes\n",
                 managed - freed - overhead - bucketSpace);
    FM_LOG_PRINT("Free but previously allocated: %u bytes\n", freed);
    FM_LOG_PRINT("Overhead: %u bytes\n", overhead);
    FM_LOG_PRINT("BucketSpace: %u bytes\n", bucketSpace);
    FM_LOG_PRINT("Never allocated: %u bytes\n", FM_SHARED_MEMORY_SIZE - managed);
    FM_LOG_PRINT("\n");

    buf     = requested;
    bufSize = sizeof(requested);
    *buf    = 0;

    for (rootInfo = hdr->roots ; rootInfo != NULL ; rootInfo = rootInfo->next)
    {
        retVal = FM_SNPRINTF_S(buf, bufSize, "%s%s",
                               (buf != requested ? ", " : ""), rootInfo->name);

        if (retVal >= 0 && retVal < bufSize)
        {
            buf     += retVal;
            bufSize -= retVal;
        }
    }

    FM_LOG_PRINT("Roots: %s\n", requested);
    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("Virtual memory map:\n");

    buf     = requested;
    bufSize = sizeof(requested);
    *buf    = 0;

    f = fopen("/proc/self/maps", "r");

    if (f != NULL)
    {
        while (fgets(buf, bufSize, f) != NULL)
        {
            FM_LOG_PRINT("%s", buf);
        }

        fclose(f);
    }

    FM_LOG_PRINT("\n");

    UnlockMutex(hdr);

}   /* end fmDbgDumpAllocStats */



/*****************************************************************************/
/** fmDbgDumpAllocCallStacks
 * \ingroup intDiagMisc
 *
 * \desc            Dump full call stacks for all allocated buffers of a
 *                  specified size.
 *
 * \param[in]       bufSize is the buffer size for which to dump allocation
 *                  call stacks.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpAllocCallStacks(fm_uint bufSize)
{
#if DBG_FULL_CALLER_DEPTH
    fm_sharedHeader *hdr   = (fm_sharedHeader *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  start = (unsigned char *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  ptr;
    fm_uint          offset;
    fm_memoryBucket *bucket;
    fm_uint          size;
    fm_objectHeader *objHdr;
    fm_int           bufNum;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "bufSize = %u\n", bufSize);

    LockMutex(hdr);

    FM_LOG_PRINT("\n");

    offset = sizeof(fm_sharedHeader);

    while ( ( ( offset + sizeof(fm_objectHeader) ) & 7 ) != 0 )
    {
        offset++;
    }

    start += offset;
    size  = bufSize + sizeof(fm_objectHeader);

    /**************************************************
     * Find the bucket.
     **************************************************/
    bucket = GetBucket(hdr, size);

    if (bucket != NULL)
    {
        size = bucket->size - sizeof(fm_objectHeader);

        /**************************************************
         * Walk through all the object headers, and write
         * out the caller stacks for all objects belonging
         * to the specified bucket.
         **************************************************/
        ptr    = start;
        bufNum = 0;

        while ( (void *) ptr < hdr->freeSpace )
        {
            objHdr = (fm_objectHeader *) ptr;
#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_DEFINED(objHdr, sizeof(fm_objectHeader));
#endif

            if (objHdr->myBucket == bucket)
            {
                if (objHdr->callerDepth > 0)
                {
                    FM_LOG_PRINT("\nBuffer %d: Allocated by:\n", bufNum);
                    backtrace_symbols_fd(objHdr->callerArray,
                                         objHdr->callerDepth,
                                         1);
                }

                bufNum++;
            }

            ptr += objHdr->myBucket->size;

#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(objHdr, sizeof(fm_objectHeader));
#endif
        }

    }   /* end if (bucket != NULL) */
    else
    {
        FM_LOG_PRINT("Bucket size %u not found\n", bufSize);
    }

    FM_LOG_PRINT("\n");

    UnlockMutex(hdr);

#else
    FM_NOT_USED(bufSize);

    FM_LOG_PRINT("fmDbgDumpAllocCallStacks requires DBG_FULL_CALLER_DEPTH "
                 "to be set to TRUE in fm_alos_alloc.c\n");
#endif

}   /* end fmDbgDumpAllocCallStacks */




/*****************************************************************************/
/** fmGetAllocatedMemorySize
 * \ingroup intAlos
 *
 * \desc            Returns the amount of allocated memory in the allocator.
 *
 * \param[out]      allocMemory points to caller allocated storage where
 *                  the amount of allocated memory in bytes is written.
 * 
 * \return          None.
 *
 *****************************************************************************/
void fmGetAllocatedMemorySize(fm_uint32 *allocMemory)
{
    fm_sharedHeader *hdr   = (fm_sharedHeader *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  start = (unsigned char *) FM_SHARED_MEMORY_ADDR;
    unsigned char *  ptr;
    fm_uint          offset;
    fm_memoryBucket *bucket;
    fm_uint          size;
    void *           freeList;
    fm_objectHeader *objHdr;
    fm_uint          overhead;
    fm_uint          freed = 0;
    fm_uint          managed;
    fm_uint          bucketSpace;

#ifdef FM_HAVE_VALGRIND
    fm_memoryBucket *it;
    fm_memoryBucket *next;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "(no arguments)\n");

    LockMutex(hdr);

    bucketSpace = 0;
    managed = ( (unsigned char *) (hdr->freeSpace) ) - start;

    offset = sizeof(fm_sharedHeader);

    while ( ( ( offset + sizeof(fm_objectHeader) ) & 7 ) != 0 )
    {
        offset++;
    }

#ifdef FM_HAVE_VALGRIND
    /* Temporarily whitelist access to the memory bucket linked list. */
    FM_VALGRIND_MAKE_BUCKET_MEM_DEFINED(hdr, it);
#endif

    start   += offset;
    bucket   = hdr->buckets;
    overhead = offset;

    /**************************************************
     * Loop through all the buckets
     **************************************************/
    while (bucket != NULL)
    {
        size     = bucket->size - sizeof(fm_objectHeader);
        freeList = bucket->freeList;

        /**************************************************
         * Count the amount of free space on this bucket's freelist
         **************************************************/
        while (freeList != NULL)
        {
            freeList = *(void **) freeList;
            freed   += size;
        }

        /**************************************************
         * Walk through all the object headers, and count the
         * amount of space that points to this bucket (whether
         * allocated or unallocated)
         **************************************************/
        ptr = start;

        while ( (void *) ptr < hdr->freeSpace )
        {
            objHdr = (fm_objectHeader *) ptr;
#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_DEFINED(objHdr, sizeof(fm_objectHeader));
#endif

            if (objHdr->myBucket == bucket)
            {
                overhead += sizeof(fm_objectHeader);
            }

            ptr += objHdr->myBucket->size;

#ifdef FM_HAVE_VALGRIND
            VALGRIND_MAKE_MEM_NOACCESS(objHdr, sizeof(fm_objectHeader));
#endif
        }

        /**************************************************
         * Add the sum of all memory consumed by memory 
         * buckets. This memory is never freed. 
         **************************************************/
        bucketSpace += ROUND_UP(sizeof(fm_memoryBucket));

        bucket = bucket->next;
    }

#ifdef FM_HAVE_VALGRIND
    /* Blacklist access to the memory bucket linked list. */
    FM_VALGRIND_MAKE_BUCKET_MEM_NOACCESS(hdr, it, next);
#endif

    *allocMemory = managed - freed - overhead - bucketSpace;

    UnlockMutex(hdr);

}   /* end fmGetAllocatedMemorySize */




/*****************************************************************************/
/** fmGetRoot
 * \ingroup alosAlloc
 *
 * \desc            Find/create a "root" for the shared memory system.
 *                  Since global variables are local to a specific
 *                  process, all global state needs to be located in
 *                  structures instantiated in shared memory.
 *                  This function allows one process to discover the
 *                  global structures created by another process.
 *                  If the named root already exists, then its address
 *                  is stored in the location pointed to by rootPtr.
 *                  If the named root does not already exist, then
 *                  rootFunc is called to create it.  rootFunc is
 *                  expected to store the address of the root in the
 *                  location pointed to by rootPtr, which fmGetRoot
 *                  will associate with the name when rootFunc returns.
 *                  A lock is held during the execution of rootFunc, so
 *                  that multiple processes will not try to create the
 *                  same root.
 *
 * \param[in]       rootName is a unique string identifying this root.
 *
 * \param[out]      rootPtr is a pointer to the per-process global
 *                  pointer where this root is cached.
 *
 * \param[in]       rootFunc is the function which will create the root
 *                  if it does not already exist.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          other errors propagated from rootFunc.
 *
 *****************************************************************************/
fm_status fmGetRoot(const char *          rootName,
                    void **               rootPtr,
                    fm_getDataRootHandler rootFunc)
{
    fm_sharedHeader *hdr = (fm_sharedHeader *) FM_SHARED_MEMORY_ADDR;
    fm_rootInfo *    rootInfo;
    char *           nameCopy;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "root=%s ptr=%p\n",
                 rootName, (void *) rootPtr);

    LockRootMutex(hdr);

    for (rootInfo = hdr->roots ; rootInfo != NULL ; rootInfo = rootInfo->next)
    {
        if (strcmp(rootName, rootInfo->name) == 0)
        {
            /* found an existing root */
            *rootPtr = rootInfo->root;

            UnlockRootMutex(hdr);

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, rootInfo->err);
        }
    }

    /* didn't find it; have to create the root */
    rootInfo = fmAlloc( sizeof(fm_rootInfo) );
    nameCopy = fmStringDuplicate(rootName);

    if (rootInfo == NULL || nameCopy == NULL)
    {
        UnlockRootMutex(hdr);
        if (rootInfo != NULL)
        {
            fmFree(rootInfo);
        }
        if (nameCopy != NULL)
        {
            fmFree(nameCopy);
        }
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
    }

    rootInfo->name = nameCopy;
    rootInfo->err  = rootFunc();
    rootInfo->root = *rootPtr;  /* This should have been set by rootFunc */
    rootInfo->next = hdr->roots;
    hdr->roots     = rootInfo;

    UnlockRootMutex(hdr);

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, rootInfo->err);

}   /* end fmGetRoot */




/*****************************************************************************/
/** fmGetAvailableSharedVirtualBaseAddress
 * \ingroup alosAlloc
 *
 * \desc            Get a pointer to an unmapped region of virtual memory.
 *                  The pointer value will be the same for every process that
 *                  calls this function.
 *
 * \note            The rationale for this function is that the platform
 *                  may need to memory map some memory from the device
 *                  (e.g., /dev/focalpoint0). If this memory is used
 *                  for packet buffers (as the FM85XXEP platform does),
 *                  the address needs to be consistent across all processes,
 *                  because pointers to the packet buffers are kept in shared
 *                  memory and may be accessed from multiple processes.
 *                  It is unacceptable to let mmap choose the virtual address
 *                  on its own (since it may differ between processes), so the
 *                  platform code must provide a specific virtual address.
 *
 * \param[out]      ptr points to a void pointer in which the desired
 *                  virtual address is written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetAvailableSharedVirtualBaseAddress(void **ptr)
{
    *ptr = (void *) ((fm_uintptr)FM_SHARED_MEMORY_ADDR +
                     (fm_uintptr)FM_SHARED_MEMORY_SIZE);

    return FM_OK;

}   /* end fmGetAvailableSharedVirtualBaseAddress */



/*****************************************************************************/
/** fmMemInitialize
 * \ingroup intAlosAlloc
 *
 * \desc            Map the API's data memory as shared into the client
 *                  process memory space.
 *
 * \param[in]       None.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not successful.
 *
 *****************************************************************************/
fm_status fmMemInitialize(void)
{
    void *              addr;
    fm_sharedHeader *   hdr;
    unsigned char *     ptr;
    fm_uint             offset;
    pthread_mutexattr_t attr;
    fm_uint             i;
    fm_int              shmId;
    fm_text             shmKeyStr = NULL;
    fm_text             shmKeyExtra = NULL;
    fm_text             shmKeyParseErr = NULL;
    fm_int              shmKey = -1;
    fm_int              flags;
    fm_bool             cleanStart = FALSE;
    struct shmid_ds     shmInfo;
    fm_bool             mutexInitOK;
    int                 pterr;
    char *              context;
    fm_uint             s1max;
    char                strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t             strErrNum;

    FM_LOG_ENTRY(FM_LOG_CAT_ALOS, "(no arguments)\n");

    /***************************************************
     * Default flags used for creating/attaching the
     * shared memory region.
     **************************************************/
    flags = S_IRUSR | /* user read */
            S_IWUSR | /* user write */
            S_IRGRP | /* group read */
            S_IWGRP | /* group write */
            S_IROTH | /* other read */
            S_IWOTH;  /* other write */

    /***************************************************
     * Check for shared memory mode.
     **************************************************/
    if (getenv("FM_API_SHM_KEY"))
    {
        s1max     = RSIZE_MAX - 1;
        context   = NULL;
        shmKeyStr = FM_STRTOK_S( getenv("FM_API_SHM_KEY"),
                                 &s1max,
                                 ",",
                                 &context);

        if (shmKeyStr == NULL)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "FM_API_SHM_KEY should be of the form "
                         "\"id,restart\" or \"id\"\n");

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }

        shmKeyExtra = FM_STRTOK_S(NULL, &s1max, ",", &context);

        /***************************************************
         * Valid input is of the form "id" or "id,restart".
         * Anything else is an error.
         **************************************************/
        if ( shmKeyExtra && (strcasecmp(shmKeyExtra, "restart") == 0) )
        {
            cleanStart = TRUE;
        }
        else if ( shmKeyExtra )
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "FM_API_SHM_KEY should be of the form "
                         "\"id,restart\" or \"id\"\n");

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }

        /***************************************************
         * Properly convert the id into an integer.
         **************************************************/

        errno = 0;
        shmKey = (fm_int) strtol(shmKeyStr,
                                 &shmKeyParseErr,
                                 10);

        if (errno != 0)
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Unable to convert key value %s - %s\n",
                             shmKeyStr,
                             strErrBuf);
            }
            else
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Unable to convert key value %s - %d\n",
                             shmKeyStr,
                             errno);
            }

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }

        /***************************************************
         * Now see if this ID exists.
         **************************************************/
        shmId = shmget(shmKey, FM_SHARED_MEMORY_SIZE, 0);

        /***************************************************
         * Handle the clean start case.
         **************************************************/
        if ( (shmId != -1) && cleanStart )
        {
            if ( shmctl(shmId, IPC_RMID, &shmInfo) == -1 )
            {
                strErrNum =
                    FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
                if (strErrNum == 0)
                {
                    FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                                 "Unable to delete SHM region for ID %d - %s\n",
                                 shmId,
                                 strErrBuf);
                }
                else
                {
                    FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                                 "Unable to delete SHM region for ID %d - %d\n",
                                 shmId,
                                 errno);
                }

                FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
            }

            /* Force the creation of a new SHM now */
            shmId = -1;
            errno = ENOENT;
        }

        /***************************************************
         * Create a new SHM
         **************************************************/
        if ( (shmId == -1) && (errno == ENOENT) )
        {
            shmId = shmget(shmKey, FM_SHARED_MEMORY_SIZE, IPC_CREAT | IPC_EXCL | flags);

            if (shmId == -1)
            {
                strErrNum =
                    FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
                if (strErrNum == 0)
                {
                    FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                                 "Unable to create shared memory object - %s\n",
                                 strErrBuf);
                }
                else
                {
                    FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                                 "Unable to create shared memory object - %d\n",
                                 errno);
                }

                FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
            }
        }
        /***************************************************
         * Fallthrough if we can't get an ID because of
         * some other error
         **************************************************/
        else if ( shmId == -1 )
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Unable to get shared memory ID - %s\n",
                             strErrBuf);
            }
            else
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Unable to get shared memory ID - %d\n",
                             errno);
            }

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
        }

        /***************************************************
         * Attach to the shared memory region
         **************************************************/
        if (shmId != -1)
        {
            addr = shmat(shmId,
                    (void *) FM_SHARED_MEMORY_ADDR,
                    flags);

            if (addr == ((void *) -1))
            {
                strErrNum =
                    FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
                if (strErrNum == 0)
                {
                    FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                                 "Unable to attach shared memory object - %s\n",
                                 strErrBuf);
                }
                else
                {
                    FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                                 "Unable to attach shared memory object - %d\n",
                                 errno);
                }

                FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
            }
        }
        else
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Unable to get shared memory object - %s\n",
                             strErrBuf);
            }
            else
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Unable to get shared memory object - %d\n",
                             errno);
            }

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
        }

        if (addr != ((void *) FM_SHARED_MEMORY_ADDR))
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                    "Shared memory not mapped at requested address\n");

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_NO_MEM);
        }
    }
    /***************************************************
     * Otherwise annonymous mmap.
     **************************************************/
    else
    {
        /* Map file into memory. */
        addr = mmap( (void*) FM_SHARED_MEMORY_ADDR,
                     FM_SHARED_MEMORY_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1,
                     0);

        if (addr == MAP_FAILED)
        {
            strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
            if (strErrNum == 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Can't map shared memory - %s\n",
                             strErrBuf);
            }
            else
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Can't map shared memory - %d\n",
                             errno);
            }
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }
    }

#ifdef FM_HAVE_VALGRIND
    VALGRIND_CREATE_MEMPOOL((void *) FM_SHARED_MEMORY_ADDR,
                            0,
                            getenv("FM_API_SHM_KEY") ? 0 : 1);
#endif

    /***************************************************
     * Now proceed to setup the memory block.
     **************************************************/

    hdr = (fm_sharedHeader*) addr;

    if (!hdr->initialized)
    {
        processCreatedSHM = TRUE;

        /**************************************************
         * Set up sanity check information in the header
         **************************************************/
        hdr->versionIdentifier   = VersionIdentifier();
        hdr->self                = addr;
        hdr->end                 = (void *) ((fm_uintptr)FM_SHARED_MEMORY_ADDR
                                    + (fm_uintptr)FM_SHARED_MEMORY_SIZE);
        hdr->funcInSharedLibrary = MemoryCorruptionWarning;

        /**************************************************
         * Bootstrap the first memory bucket
         **************************************************/
        hdr->bucketBucket.signature = BUCKET_SIGNATURE;
        hdr->bucketBucket.size      = ROUND_UP( sizeof(fm_memoryBucket) +
                                               sizeof(fm_objectHeader) );
        hdr->bucketBucket.freeList = NULL;
        hdr->bucketBucket.next     = NULL;
        hdr->buckets               = &(hdr->bucketBucket);

        offset = sizeof(fm_sharedHeader);

        while ( ( ( offset + sizeof(fm_objectHeader) ) & 7 ) != 0 )
        {
            offset++;
        }

        ptr            = addr;
        ptr           += offset;
        hdr->freeSpace = ptr;

        hdr->roots = NULL;

        /**************************************************
         * We use pthread mutex functions directly, because
         * fm_lock facility has not yet been initialized.
         **************************************************/
        if ( pthread_mutexattr_init(&attr) )
        {
            mutexInitOK = FALSE;
        }
        else if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) )
        {
            mutexInitOK = FALSE;
            pterr = pthread_mutexattr_destroy(&attr);
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex attr\n",
                             pterr);
            }
        }
        else if ( pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
        {
            mutexInitOK = FALSE;
            pterr = pthread_mutexattr_destroy(&attr);
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex attr\n",
                             pterr);
            }
        }
        else if ( pthread_mutex_init(&(hdr->mutex), &attr) )
        {
            mutexInitOK = FALSE;
            pterr = pthread_mutexattr_destroy(&attr);
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex attr\n",
                             pterr);
            }
        }
        else if ( pthread_mutex_init(&(hdr->rootMutex), &attr) )
        {
            mutexInitOK = FALSE;
            pterr = pthread_mutex_destroy( &(hdr->mutex) );
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex\n",
                             pterr);
            }
            pterr = pthread_mutexattr_destroy(&attr);
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex attr\n",
                             pterr);
            }
        }
        else if ( pthread_mutexattr_destroy(&attr) )
        {
            mutexInitOK = FALSE;
            pterr = pthread_mutex_destroy( &(hdr->rootMutex) );
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex\n",
                             pterr);
            }

            pterr = pthread_mutex_destroy( &(hdr->mutex) );
            if (pterr != 0)
            {
                FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                             "Error %d destroying mutex\n",
                             pterr);
            }
        }
        else
        {
            mutexInitOK = TRUE;
        }

        if (!mutexInitOK)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Failed to initialize mutexes\n");

            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_ERR_LOCK_INIT);
        }

        /**************************************************
         * Now mark the header as initialized
         **************************************************/
        hdr->initialized = TRUE;

#ifdef FM_HAVE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(addr, sizeof(fm_sharedHeader));
        /* White list the members of the fm_sharedHeader data structure that
         * change during normal operation. */
        FM_VALGRIND_MAKE_MEMBER_MEM_DEFINED(addr, fm_sharedHeader, buckets);
        FM_VALGRIND_MAKE_MEMBER_MEM_DEFINED(addr, fm_sharedHeader, freeSpace);
        FM_VALGRIND_MAKE_MEMBER_MEM_DEFINED(addr, fm_sharedHeader, mutex);
        FM_VALGRIND_MAKE_MEMBER_MEM_DEFINED(addr, fm_sharedHeader, rootMutex);
        FM_VALGRIND_MAKE_MEMBER_MEM_DEFINED(addr, fm_sharedHeader, roots);
#endif
    }
    else
    {
        /**************************************************
         * Wait for header to be initialized
         **************************************************/
        for (i = 0 ; i < FM_SHM_TIMEOUT ; i++)
        {
            if (hdr->initialized)
            {
                goto okay2;
            }

            sleep(1);
        }

        FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                     "Timed out waiting for shared memory after %d seconds\n",
                     FM_SHM_TIMEOUT);
        FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);

okay2:

        /**************************************************
         * Check sanity check information
         **************************************************/
        if ( hdr->versionIdentifier != VersionIdentifier() )
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Version signature mismatch in shared memory\n");
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }

        if (hdr->self != addr)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Start address mismatch in shared memory\n");
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }

        if ( hdr->end != (void *) ((fm_uintptr)FM_SHARED_MEMORY_ADDR +
                                   (fm_uintptr)FM_SHARED_MEMORY_SIZE) )
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "End address mismatch in shared memory\n");
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }

        if (hdr->funcInSharedLibrary != MemoryCorruptionWarning)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ALOS,
                         "Shared library seems to be loaded at a "
                         "different address\n");
            FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_FAIL);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ALOS, FM_OK);

}   /* end fmMemInitialize */




/*****************************************************************************/
/** fmIsMasterProcess
 * \ingroup alosAlloc
 *
 * \desc            Returns TRUE if the caller was the process that created the
 *                  API shared memory region (the first process to call
 *                  ''fmOSInitialize'', which is generally called when the
 *                  application calls ''fmInitialize''.
 *
 * \param[out]      isMaster points to caller-allocated storage where this
 *                  function is to place the result.
 *
 * \return          FM_OK unconditionally
 *
 *****************************************************************************/
fm_status fmIsMasterProcess(fm_bool *isMaster)
{
    *isMaster = processCreatedSHM;

    return FM_OK;

} /* end fmIsMasterProcess */

