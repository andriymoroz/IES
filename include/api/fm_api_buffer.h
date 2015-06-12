/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_buffer.h
 * Creation Date:   May 15, 2007
 * Description:     Structures and functions for dealing with packet buffers
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

#ifndef __FM_FM_API_BUFFER_H
#define __FM_FM_API_BUFFER_H



/**********************************************************/
/** \ingroup typeEnum
 *  Specifies how a packet buffer is to be used. Used by
 *  ''fmAllocateBufferV2'' and ''fmPlatformAllocateBufferV2''.
 **********************************************************/
typedef enum
{
    /** Buffer used to receive a packet. */
    FM_BUFFER_RX = 0,

    /** Buffer used to send a packet. */
    FM_BUFFER_TX,

    /** Buffer used to send or receive a packet. Provided for
     *  backward compatibility. */
    FM_BUFFER_ANY,

} fm_bufferType;

/* Not part of the enumeration since it isn't a valid
 * fm_bufferType value. */
#define FM_NUM_BUFFER_TYPES     3



/**************************************************/
/** \ingroup typeStruct
 * Structure used for communicating Ethernet
 * packets between the API and the application. May
 * be chained together for packets that exceed the
 * basic data block (chunk) size.
 **************************************************/
typedef struct _fm_buffer
{
    /** Pointer to a "chunk" of packet data. The packet data is in network
     *  byte order and must begin at the first byte of the location pointed
     *  to by this structure member. Partially filled chunks are not permitted,
     *  except of course for the last in the chain. */
    fm_uint32 *data;

    /** Number of bytes pointed to by data (the number of bytes in this
     *  chunk). */
    int        len;

    /** Pointer to the next buffer in a chain, or NULL for the last. */
    struct _fm_buffer *next;

    /** Private data used by the API. The application should not touch this
     *  member. The index is a uniquely assigned value for this buffer
     *  instance, used for indexing and identifying buffer instances. */
    int        index;

    /** Private data used by the API. The application should not touch this
     *  member. Receive event corresponding to this buffer. */
    fm_event   *recvEvent;

    /** Private data used by the API. The application should not touch this
     *  member.Type of the buffer based on its usage. */
    fm_bufferType bufferType;

    /** Private data used by the API. The application should not touch this
     *  member. Node in the buffer queue in which this buffer is present.*/
    fm_dlist_node   *bufferQueueNode;

} fm_buffer;



/* returns a pointer to a newly allocated buffer */
fm_buffer *fmAllocateBuffer(int sw);


/* releases memory for an allocated buffer */
fm_status fmFreeBuffer(int sw, fm_buffer *buf);


/* releases memory for a chain of buffers */
fm_status fmFreeBufferChain(int sw, fm_buffer *bufChain);


/* wrappers to access internal members of the buffer */
fm_uint32 *fmGetBufferDataPtr(fm_buffer *buf);
fm_status fmSetBufferDataPtr(fm_buffer *buf, fm_uint32 *ptr);
fm_status fmGetBufferDataLength(fm_buffer *buf, fm_int *len);
fm_status fmSetBufferDataLength(fm_buffer *buf, fm_int len);
fm_buffer *fmGetNextBuffer(fm_buffer *buf);


/* walk the frame and add a buffer to the end */
fm_status fmAddBuffer(fm_buffer *frame, fm_buffer *buf);


/* helper for duplicating */
fm_buffer *fmDuplicateBufferChain(fm_int sw, fm_buffer *srcFrame);

/* Dequeue frame from the buffer queue corresponding to rcvPktEvent*/
fm_status fmFreeBufferQueueNode(int sw, fm_eventPktRecv *rcvPktEvent);

/* Enqueue frame in buffer queue corresponding to rcvPktEvent */
fm_status fmAddBufferChainInQueue(int sw,
                                  fm_buffer *buf,
                                  fm_uint32 pri,
                                  fm_bool isInsertBegin);

/* Allocates buffer from appropriate Rx or Tx pool if separate pool is enabled, 
 * else allocates from common pool. */
fm_buffer *fmAllocateBufferV2(int sw, fm_bufferType bufferType);

#endif /* __FM_FM_API_BUFFER_H */
