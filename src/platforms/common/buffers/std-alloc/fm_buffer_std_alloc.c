/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_buffer_std_alloc.c
 * Creation Date:   September 19, 2007
 * Description:     Common module to manage the free buffers as a circular
 *                  list.  Relies on the following to be used:
 *
 *                  - the state structure fm_bufferAllocState must be defined
 *                    in the platform root as the field bufferAllocState
 *                  - the platform must define fmPlatformGetBufferCount to
 *                    return the number of buffers to allocate
 *                  - the platform must define the following constants:
 *                      FM_BUFFER_SIZE_WORDS
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

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

/*****************************************************************************/
/** GetBufferMemory
 * \ingroup intPlatform
 *
 * \desc            Returns the memory for a given chunk.  Assumes that the
 *                  pool field has already been setup.
 *
 * \param[in]       index is the chunk number from 0..N-1, where N is the value
 *                  of FM_NUM_BUFFERS
 *
 * \return          the pointer to the chunk memory.
 *
 *****************************************************************************/
static fm_uint32 *GetBufferMemory(fm_int index)
{
    return fmRootPlatform->bufferAllocState.pool + (index * FM_BUFFER_SIZE_WORDS);

}   /* end GetBufferMemory */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmPlatformInitBuffersV2
 * \ingroup intPlatform
 *
 * \desc            Initializes the buffer allocator subsystem
 *
 * \param[in]       bufferMemoryPool points to the caller-allocated memory
 *                  used to back the buffer data.
 * 
 * \param[in]       numScratchBuffers is the number of buffers used as scratch
 *                  to receive the maximum size frame on cpu.
 *
 * \return          fm_status code
 *
 *****************************************************************************/
fm_status fmPlatformInitBuffersV2(fm_uint32 *bufferMemoryPool, 
                                  fm_int numScratchBuffers)
{
    int                  i;
    fm_bufferAllocState *info;
    fm_int               numRxBuffers;
    fm_int               numTxBuffers;
    fm_status            err;

    FM_LOG_ENTRY(FM_LOG_CAT_BUFFER, "pool=%p\n", (void *) bufferMemoryPool);
    
    info = &fmRootPlatform->bufferAllocState;

    memset( info, 0, sizeof(fm_bufferAllocState) );

    info->pool             = bufferMemoryPool;

    info->enableSeparatePool =
        fmGetBoolApiProperty(FM_AAK_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE,
                             FM_AAD_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE);   

    if (info->enableSeparatePool)
    {
        numRxBuffers =
            fmGetIntApiProperty(FM_AAK_API_PLATFORM_NUM_BUFFERS_RX,
                                FM_AAD_API_PLATFORM_NUM_BUFFERS_RX);

        numTxBuffers =
            fmGetIntApiProperty(FM_AAK_API_PLATFORM_NUM_BUFFERS_TX,
                                FM_AAD_API_PLATFORM_NUM_BUFFERS_TX); 

        if (numRxBuffers < 0 || numTxBuffers < 0)
        {
           FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_ERR_INVALID_VALUE); 
        }
        info->totalBufferCount = numRxBuffers + numTxBuffers + numScratchBuffers; 

        info->availableBuffers[FM_BUFFER_RX]  = numRxBuffers + numScratchBuffers;
        info->availableBuffers[FM_BUFFER_TX]  = numTxBuffers;

        /* When separate buffer pool is enabled, FM_BUFFER_ANY in 
         * availableBuffers should not be used. */
        info->availableBuffers[FM_BUFFER_ANY] = -1;
        
    }
    else
    {
#ifdef FM_NUM_BUFFERS
        info->totalBufferCount = FM_NUM_BUFFERS + numScratchBuffers;

        info->availableBuffers[FM_BUFFER_RX]  = -1;
        info->availableBuffers[FM_BUFFER_TX]  = -1;
        info->availableBuffers[FM_BUFFER_ANY] = info->totalBufferCount;
#else
        /* FM_NUM_BUFFERS must be defined */
        FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_ERR_INVALID_VALUE);
#endif        

    }   

    info->table = (fm_buffer *) fmAlloc(sizeof(fm_buffer) *
                                        info->totalBufferCount);

    if (info->table == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_ERR_NO_MEM);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_BUFFER,
                 "Number of  buffers initialized = %d\n",
                 info->totalBufferCount);

    /**************************************************
     * Initialize allocator.  Each buffer in the pool
     * is represented by its index into the pool memory
     * block.  The allocator is an array of buffer
     * indices, indexed by a buffer's index.  The cell
     * content is the index of the next buffer in a
     * virtual list of chunks.
     *
     * The list is terminated with a -1 entry.
     *
     * The for-loop below initializes the array as:
     *
     *          0    1    2    3         fd   fe   ff
     *       +----+----+----+----+-----+----+----+----+
     *       |  1 |  2 |  3 |  4 | ... | fe | ff | -1 |
     *       +----+----+----+----+-----+----+----+----+
     *
     * for the case where the pool contains 256 buffers.
     **************************************************/

    /* Allocate the allocator. */
    info->freeList = (fm_int *) fmAlloc(sizeof(fm_int) *
                                        info->totalBufferCount);

    if (info->freeList == NULL)
    {
        fmFree(info->table);
        info->table = NULL;
        FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_ERR_NO_MEM);
    }

    /* Index of first buffer in list - used for allocating. */
    info->firstFree = 0;

    /* Initialize the allocator array. */
    for (i = 0 ; i < info->totalBufferCount ; i++)
    {
        info->freeList[i] = i + 1;
    }

    /* Terminate the list. */
    info->freeList[info->totalBufferCount - 1] = -1;

    /* Initialize all buffers */
    for (i = 0 ; i < info->totalBufferCount ; i++)
    {
        info->table[i].data = GetBufferMemory(i);

        info->table[i].next  = NULL;
        info->table[i].len   = 0;
        info->table[i].index = i;
        info->table[i].bufferQueueNode = NULL;
        info->table[i].recvEvent = NULL;

        /**
         * Also initialize all the data areas.  This causes the page
         * faults needed to map the data areas into user memory to all
         * occur now, saving time during frame transmission/reception
         * later.
         */
        memset(info->table[i].data, 'z', FM_BUFFER_SIZE_BYTES);
    }

    err = fmCreateLock("Buffer Lock", &info->bufferLock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_BUFFER, err);

    FM_LOG_DEBUG(FM_LOG_CAT_BUFFER,
                 "Initialized buffers left: RX: %d TX: %d Total: %d\n",
                 info->availableBuffers[FM_BUFFER_RX],
                 info->availableBuffers[FM_BUFFER_TX],
                 info->availableBuffers[FM_BUFFER_ANY]);

    FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_OK);

}   /* end fmPlatformInitBuffersV2 */




/*****************************************************************************/
/** fmPlatformInitBuffers
 * \ingroup intPlatform
 *
 * \desc            Initializes the buffer allocator subsystem
 *
 * \param[in]       bufferMemoryPool points to the caller allocated memory used
 *                  to back the buffer data.
 *
 * \return          fm_status code
 *
 *****************************************************************************/
fm_status fmPlatformInitBuffers(fm_uint32 *bufferMemoryPool)
{

    return fmPlatformInitBuffersV2(bufferMemoryPool, 0);

}   /* end fmPlatformInitBuffers */




/*****************************************************************************/
/** fmPlatformAllocateBuffer
 * \ingroup platform
 *
 * \desc            Allocate a packet buffer.  The buffer is accessed through
 *                  an ''fm_buffer'' structure, which keeps housekeeping
 *                  information for chaining multiple data blocks together.
 *
 * \return          A pointer to an ''fm_buffer'' structure.
 * \return          NULL is returned if there are no buffers available.
 *
 *****************************************************************************/
fm_buffer *fmPlatformAllocateBuffer(void)
{
    fm_bufferAllocState *info;
    fm_buffer *          ret;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_BUFFER);
    
    info = &fmRootPlatform->bufferAllocState;

    if (info->enableSeparatePool)
    {
        ret = fmPlatformAllocateBufferV2(FM_BUFFER_TX); 
    }
    else
    {
        ret = fmPlatformAllocateBufferV2(FM_BUFFER_ANY);
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_BUFFER, ret, "%p\n", (void *) ret);

}   /* end fmPlatformAllocateBuffer */




/*****************************************************************************/
/** fmPlatformFreeBuffer
 * \ingroup platform
 *
 * \desc            Return a packet buffer, previously allocated with a
 *                  call to ''fmAllocateBuffer'', to the free buffer pool.
 *
 * \note            If the buffer is part of a chain of buffers, this function
 *                  will not dispose of the other buffers in the chain.  See
 *                  fmFreeBufferChain for disposing of an entire chain.
 *
 * \param[in]       buf points to the buffer's ''fm_buffer'' structure.
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fmPlatformFreeBuffer(fm_buffer *buf)
{
    fm_bufferAllocState *info;
    fm_switch *          switchState;
    fm_int               switchNum;
    fm_int               index;

    index = buf->index;

    FM_LOG_ENTRY(FM_LOG_CAT_BUFFER,
                 "buf = %p, buf->index = %d\n",
                 (void *) buf, index);

    info = &fmRootPlatform->bufferAllocState;

    TAKE_BUFFER_LOCK();

    /**************************************************
     * Validate chunk.
     **************************************************/

    /* Valid chunk index? */
    if ( (index >= info->totalBufferCount) ||
        (index == -1) ||
        (info->freeList[index] != -2) )
    {
        /* Invalid chunk index. */
        DROP_BUFFER_LOCK();

        FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_ERR_INVALID_ARGUMENT);
    }

    /**************************************************
     * Put the chunk back in the free list.
     **************************************************/

    info->freeList[index] = info->firstFree;
    info->firstFree       = index;

    /**************************************************
     * Reset its pointer to the base of the chunk
     **************************************************/

    info->table[index].data = GetBufferMemory(index);

    /* Clear existing values */
    info->table[index].bufferQueueNode = NULL;
    info->table[index].recvEvent       = NULL;

    /* The below statements were not there before. Any reason
     * not to do the following? */
    info->table[index].len = 0;
    info->table[index].next = NULL;

    /**************************************************
     * Count the free event.
     **************************************************/

    info->availableBuffers[buf->bufferType]++;

    FM_LOG_DEBUG(FM_LOG_CAT_BUFFER,
                 "Freed buffer #%d, %d RX buf left, %d TX buf left," 
                 "%d ANY buf left\n", 
                 index,
                 info->availableBuffers[FM_BUFFER_RX],
                 info->availableBuffers[FM_BUFFER_TX],
                 info->availableBuffers[FM_BUFFER_ANY]);

    DROP_BUFFER_LOCK();

    if ( info->enableSeparatePool && buf->bufferType == FM_BUFFER_TX)
    {

        FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_OK);
    }

    /*************************************************************************
     * The following should be done only when the separate pool is not enabled
     * or separate pool is enabled and the buffer type is RX.
     * 
     * If any switch's frame receiver could previously  not get a chunk, then 
     * signal it to try again now that there is one available.
     *************************************************************************/

    for (switchNum = FM_FIRST_FOCALPOINT ;
         switchNum <= FM_LAST_FOCALPOINT ;
         switchNum++)
    {
        switchState = fmRootApi->fmSwitchStateTable[switchNum];

        if (switchState && switchState->state == FM_SWITCH_STATE_UP)
        {
            if (switchState->buffersNeeded)
            {
                /* Clear out the flag since we know we have a free buffer */
                switchState->buffersNeeded      = FALSE;
                
                /**************************************************
                 * We must take a lock before writing
                 * intrReceivePackets because the lock is used
                 * by the API to ensure an atomic read-modify-write
                 * to intrReceivePackets.
                 *
                 * The platform lock is used instead of state lock
                 * because on FIBM platforms, there is an access
                 * to intrSendPackets that must be protected before
                 * the switch's locks are even created. 
                 **************************************************/
                
                FM_TAKE_PKT_INT_LOCK(switchNum);
                switchState->intrReceivePackets = TRUE;
                FM_DROP_PKT_INT_LOCK(switchNum);

                /* Wake up the interrupt handler so it will see the message. */
                fmPlatformTriggerInterrupt(switchNum, FM_INTERRUPT_SOURCE_API);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_OK);

}   /* end fmPlatformFreeBuffer */




/*****************************************************************************/
/** fmPlatformGetAvailableBuffers
 * \ingroup intPlatform
 *
 * \desc            Returns the number of available buffers.
 *
 * \param[out]      count points to caller allocated storage where the number
 *                  of available buffers is written.
 *
 * \return          fm_status code
 *
 *****************************************************************************/
fm_status fmPlatformGetAvailableBuffers(fm_int *count)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_BUFFER, "count=%p\n", (void *) count);

    if (fmRootPlatform->bufferAllocState.enableSeparatePool)
    {
        status = fmPlatformGetAvailableBuffersV2(FM_BUFFER_TX, count);
    }
    else
    {
        status = fmPlatformGetAvailableBuffersV2(FM_BUFFER_ANY, count);
    }

    FM_LOG_EXIT(FM_LOG_CAT_BUFFER, status);

}   /* end fmPlatformGetAvailableBuffers */




/*****************************************************************************/
/** fmPlatformGetAvailableBuffersV2
 * \ingroup intPlatform
 *
 * \desc            Returns the number of available buffers in appropriate pool.
 *
 * \param[in]       type specifies the type of buffer pool. 
 *
 * \param[out]      count points to caller allocated storage where the number
 *                  of available buffers is written.
 *
 * \return          fm_status code
 *
 *****************************************************************************/
fm_status fmPlatformGetAvailableBuffersV2(fm_bufferType type, fm_int *count)
{
    FM_LOG_ENTRY(FM_LOG_CAT_BUFFER, "count=%p\n", (void *) count);

    if (!count)
    {
        FM_LOG_FATAL(FM_LOG_CAT_BUFFER, "Count pointer is null\n");

        FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_ERR_INVALID_ARGUMENT);
    }

    *count = fmRootPlatform->bufferAllocState.availableBuffers[type];

    FM_LOG_EXIT(FM_LOG_CAT_BUFFER, FM_OK);

}   /* end fmPlatformGetAvailableBuffersV2 */




/*****************************************************************************/
/** fmPlatformAllocateBufferV2
 * \ingroup platform
 *
 * \desc            Allocate a packet buffer.  The buffer is accessed through
 *                  an ''fm_buffer'' structure, which keeps housekeeping
 *                  information for chaining multiple data blocks together.
 *
 * \param[in]       type specifies the type of buffer. When the buffer
 *                  is freed, the type is used to handle freeing of the
 *                  buffer to the appropriate pool.
 *
 * \return          A pointer to an ''fm_buffer'' structure.
 * \return          NULL if there are no buffers available.
 *
 *****************************************************************************/
fm_buffer *fmPlatformAllocateBufferV2(fm_bufferType type)
{
    fm_bufferAllocState *info;
    fm_int               chunk;
    fm_buffer *          ret;

    FM_LOG_ENTRY_NOARGS(FM_LOG_CAT_BUFFER);
    
    info = &fmRootPlatform->bufferAllocState;

    if ( (!info->enableSeparatePool && type != FM_BUFFER_ANY) ||
         (info->enableSeparatePool && type == FM_BUFFER_ANY) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_BUFFER,
                     "Conflicting buffer type: %d\n", type);
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_BUFFER, NULL, 
                           "Conflicting buffer type: %d\n", type);
        
    }
    TAKE_BUFFER_LOCK();

    if (info->availableBuffers[type] == 0)
    {
        DROP_BUFFER_LOCK();
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_BUFFER, NULL, 
                           "No free buffer available in pool: %d\n", type);
    }

    /**************************************************
     * Get a chunk from the head of the free list.
     **************************************************/

    chunk = info->firstFree;

    if (chunk != -1)
    {
        info->firstFree       = info->freeList[chunk];
        info->freeList[chunk] = -2;

        info->availableBuffers[type]--;

        FM_LOG_DEBUG(FM_LOG_CAT_BUFFER,
                     "Allocated buffer #%d, %d left\n", chunk,
                     info->availableBuffers[type]);

    }

    DROP_BUFFER_LOCK();

    ret = (chunk == -1) ? NULL : &info->table[chunk];

    /* set the next pointer of the allocated buffer to NULL */
    if (ret)
    {
        ret->next       = NULL;
        ret->bufferType = type;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_BUFFER, ret, "%p\n", (void *) ret);

}   /* end fmPlatformAllocateBufferV2 */


