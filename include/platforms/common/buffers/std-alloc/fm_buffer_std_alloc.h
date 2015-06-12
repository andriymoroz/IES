/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_buffer_std_alloc.h
 * Creation Date:   Sept. 19, 2007
 * Description:     Standard circular list buffer allocator.
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

#ifndef __FM_FM_BUFFER_STD_ALLOC_H
#define __FM_FM_BUFFER_STD_ALLOC_H

/* manages the circular list of buffers */
typedef struct
{
    /* Specifies whether separate RX and TX buffer pool is enabled or not */
    fm_bool    enableSeparatePool;

    /* Index of first free chunk in the chunk list. */
    fm_int     firstFree;

    /* Allocator: array of chunk indices indexed by chunk index */
    fm_int *   freeList;

    /* Total number of available buffers */
    fm_int     totalBufferCount;

    /* Number of available buffers */
    fm_int     availableBuffers[FM_NUM_BUFFER_TYPES];

    /* Table of buffers */
    fm_buffer *table;

    /* Pointer to the backing memory pool */
    fm_uint32 *pool;

    /* Buffer lock to protect against simultaneous access */
    fm_lock    bufferLock;

} fm_bufferAllocState;

#define TAKE_BUFFER_LOCK()                                           \
    fmCaptureLock( &fmRootPlatform->bufferAllocState.bufferLock,     \
                   FM_WAIT_FOREVER);
#define DROP_BUFFER_LOCK()                                           \
    fmReleaseLock( &fmRootPlatform->bufferAllocState.bufferLock);

fm_status fmPlatformInitBuffers(fm_uint32 *bufferMemoryPool);
fm_status fmPlatformInitBuffersV2(fm_uint32 *bufferMemoryPool, fm_int numBuffers);
fm_buffer *fmPlatformAllocateBufferV2(fm_bufferType type);
fm_status fmPlatformGetAvailableBuffersV2(fm_bufferType type, fm_int *count);


#endif /* __FM_FM_BUFFER_STD_ALLOC_H */
