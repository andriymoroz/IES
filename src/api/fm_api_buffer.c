/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_buffer.c
 * Creation Date:   May 15, 2007
 * Description:     Functions dealing with packet buffers
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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmAllocateBuffer
 * \ingroup buffer
 *
 * \desc            Allocate a packet buffer.  The buffer is accessed through
 *                  an ''fm_buffer'' structure, which keeps housekeeping
 *                  information for chaining multiple data blocks together.
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \return          A pointer to an ''fm_buffer'' structure.
 * \return          NULL is returned if there are no buffers available.
 *
 *****************************************************************************/
fm_buffer *fmAllocateBuffer(int sw)
{
    fm_buffer *outBuffer = NULL;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "sw=%d\n", sw);

    outBuffer = fmPlatformAllocateBuffer();

    if (outBuffer == NULL)
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_OUT_OF_BUFFERS, 1);
    }
    else
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_BUFFER_ALLOCS, 1);
    }

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_BUFFER,
                           outBuffer,
                           "outBuffer=%p\n",
                           (void *) outBuffer);

}   /* end fmAllocateBuffer */




/*****************************************************************************/
/** fmAllocateBufferV2
 * \ingroup buffer
 *
 * \desc            Allocate a packet buffer for send/receive.  The buffer is 
 *                  accessed through an ''fm_buffer'' structure, which keeps 
 *                  housekeeping information for chaining multiple data blocks 
 *                  together.
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \param[in]       bufferType specifies whether the allocated buffer is for
 *                  Send or Receive. Used to select the pool from which the
 *                  buffer will be allocated.
 *
 * \return          A pointer to an ''fm_buffer'' structure.
 * \return          NULL if there are no buffers available.
 *
 *****************************************************************************/
fm_buffer *fmAllocateBufferV2(int sw, fm_bufferType bufferType)
{
    fm_buffer *outBuffer = NULL;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "sw=%d\n", sw);

    outBuffer = fmPlatformAllocateBufferV2(bufferType);

    if (outBuffer == NULL)
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_OUT_OF_BUFFERS, 1);
    }
    else
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_BUFFER_ALLOCS, 1);
    }

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_BUFFER,
                           outBuffer,
                           "outBuffer=%p\n",
                           (void *) outBuffer);

}   /* end fmAllocateBufferV2 */




/*****************************************************************************/
/** fmFreeBuffer
 * \ingroup buffer
 *
 * \desc            Return a packet buffer, previously allocated with a
 *                  call to ''fmAllocateBuffer'', to the free buffer pool.
 *
 * \note            If the buffer is part of a chain of buffers, this function
 *                  will not dispose of the other buffers in the chain.  See
 *                  fmFreeBufferChain for disposing of an entire chain.
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \param[in]       buf points to the buffer's ''fm_buffer'' structure.
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fmFreeBuffer(int sw, fm_buffer *buf)
{
    fm_status err;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "sw=%d\n", sw);

    err = fmPlatformFreeBuffer(buf);

    if (err == FM_OK)
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_BUFFER_FREES, 1);
        err = fmSignalSemaphore(&fmRootApi->waitForBufferSemaphore);
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, err);

}   /* end fmFreeBuffer */




/*****************************************************************************/
/** fmFreeBufferChain
 * \ingroup buffer
 *
 * \desc            Return an entire chain of packet buffers, previously
 *                  allocated with calls to 'fmAllocateBuffer'', to the free
 *                  buffer pool.
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \param[in]       bufChain points to the first buffer in the chain.
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fmFreeBufferChain(int sw, fm_buffer *bufChain)
{
    fm_buffer *nextBuffer;
    fm_buffer *curBuffer;
    fm_status  status = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "sw=%d\n", sw);

    curBuffer = bufChain;

    while (curBuffer != NULL)
    {
        nextBuffer = curBuffer->next;
        status     = fmFreeBuffer(sw, curBuffer);

        if (status != FM_OK)
        {
            break;
        }

        curBuffer = nextBuffer;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, status);

}   /* end fmFreeBufferChain */




/*****************************************************************************/
/** fmGetBufferDataPtr
 * \ingroup buffer
 *
 * \desc            Return a pointer to a buffer's data section.
 *
 * \param[in]       buf is a pointer to the ''fm_buffer'' structure.
 *
 * \return          A pointer to the data payload of the buffer.
 * \return          Returns NULL if buf is invalid.
 *
 *****************************************************************************/
fm_uint32 *fmGetBufferDataPtr(fm_buffer *buf)
{
    fm_uint32 *data;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "buf=%p\n", (void *) buf);

    if (buf)
    {
        data = buf->data;
    }
    else
    {
        data = NULL;
    }

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_BUFFER, data, "data=%p\n", (void *) data);

}   /* end fmGetBufferDataPtr */




fm_status fmSetBufferDataPtr(fm_buffer *buf, fm_uint32 *ptr)
{
    if (!buf)
    {
        return FM_ERR_BAD_BUFFER;
    }

    buf->data = ptr;

    return FM_OK;

}   /* end fmSetBufferDataPtr */




/*****************************************************************************/
/** fmGetBufferDataLength
 * \ingroup buffer
 *
 * \desc            Return the length of a buffer's data section actually used.
 *
 * \param[in]       buf is a pointer to the ''fm_buffer'' structure.
 *
 * \param[out]      len points to caller-allocated storage where this
 *                  function should place the length of the buffer.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BAD_BUFFER if buf is invalid.
 *
 *****************************************************************************/
fm_status fmGetBufferDataLength(fm_buffer *buf, fm_int *len)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "buf=%p\n", (void *) buf);

    if (!buf)
    {
        err = FM_ERR_BAD_BUFFER;
    }
    else
    {
        *len = buf->len;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, err);

}   /* end fmGetBufferDataLength */




/*****************************************************************************/
/** fmSetBufferDataLength
 * \ingroup buffer
 *
 * \desc            Set a buffer's length.
 *
 * \param[in]       buf is a pointer to the ''fm_buffer'' structure.
 *
 * \param[in]       len contains the length in bytes of the data in the buffer
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BAD_BUFFER if buf is invalid.
 *
 *****************************************************************************/
fm_status fmSetBufferDataLength(fm_buffer *buf, fm_int len)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "buf=%p len=%d\n", (void *) buf, len);

    if (!buf)
    {
        err = FM_ERR_BAD_BUFFER;
    }
    else
    {
        buf->len = len;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, err);

}   /* end fmSetBufferDataLength */




/*****************************************************************************/
/** fmGetNextBuffer
 * \ingroup buffer
 *
 * \desc            Get the next buffer in a chain.
 *
 * \param[in]       buf is a pointer to the ''fm_buffer'' structure.
 *
 * \return          A pointer to the next buffer in the chain.
 * \return          NULL if the end of the chain was reached.
 *
 *****************************************************************************/
fm_buffer *fmGetNextBuffer(fm_buffer *buf)
{
    fm_buffer *nextBuf;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "buf=%p\n", (void *) buf);

    if (buf)
    {
        nextBuf = buf->next;
    }
    else
    {
        nextBuf = NULL;
    }

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_BUFFER,
                           nextBuf,
                           "nextBuf=%p\n",
                           (void *) nextBuf);

}   /* end fmGetNextBuffer */




/*****************************************************************************/
/** fmAddBuffer
 * \ingroup buffer
 *
 * \desc            Add a buffer to the end of a chain of buffers.
 *
 * \param[in]       frame is a pointer to the first ''fm_buffer'' structure in
 *                  a chain of buffers to which the new buffer should be added.
 *
 * \param[in]       buf is a pointer to the new ''fm_buffer'' structure to be
 *                  added to the chain.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_BAD_BUFFER if buf is invalid.
 *
 *****************************************************************************/
fm_status fmAddBuffer(fm_buffer *frame, fm_buffer *buf)
{
    fm_status  err = FM_OK;
    fm_buffer *ptr = frame;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER,
                     "frame=%p buf=%p\n",
                     (void *) frame,
                     (void *) buf);

    if (!buf)
    {
        err = FM_ERR_BAD_BUFFER;
    }
    else
    {
        while (ptr->next)
        {
            ptr = ptr->next;
        }

        ptr->next = buf;
        buf->next = NULL;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, err);

}   /* end fmAddBuffer */




/*****************************************************************************/
/** fmDuplicateBufferChain
 * \ingroup buffer
 *
 * \desc            Duplicate a buffer chain by allocating buffers and copying
 *                  the contents from the source buffer(s) into the new
 *                  buffer(s).
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \param[in]       srcFrame is a pointer to the first ''fm_buffer'' structure
 *                  in a chain of buffers which are to be cloned into a new
 *                  chain.
 *
 * \return          Pointer to the first buffer in the new chain, or NULL if
 *                  the new chain could not be allocated.
 *
 *****************************************************************************/
fm_buffer *fmDuplicateBufferChain(fm_int sw, fm_buffer *srcFrame)
{
    fm_buffer *newChain;
    fm_buffer *currentSrc;
    fm_buffer *currentDest;

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER,
                     "sw=%d srcFrame=%p\n",
                     sw,
                     (void *) srcFrame);

    newChain = NULL;

    currentSrc = srcFrame;

    while (currentSrc != NULL)
    {
        if (currentSrc->len > 0)
        {
            currentDest = fmAllocateBufferV2(sw, srcFrame->bufferType);

            if (currentDest == NULL)
            {
                FM_LOG_WARNING(FM_LOG_CAT_BUFFER,
                               "fmDuplicateBufferChain unable to allocate "
                               "buffer - cancelling duplication\n");

                if (newChain != NULL)
                {
                    fmFreeBufferChain(sw, newChain);
                    newChain = NULL;
                }

                break;
            }

            FM_MEMCPY_S(currentDest->data,
                        currentSrc->len,
                        currentSrc->data,
                        currentSrc->len);
            currentDest->len  = currentSrc->len;
            currentDest->next = NULL;

            if (newChain == NULL)
            {
                newChain = currentDest;
            }
            else
            {
                fmAddBuffer(newChain, currentDest);
            }
        }

        currentSrc = currentSrc->next;
    }

    FM_LOG_EXIT_API_CUSTOM(FM_LOG_CAT_BUFFER,
                           newChain,
                           "newChain=%p\n",
                           (void *) newChain);

}   /* end fmDuplicateBufferChain */




/*****************************************************************************/
/** fmFreeBufferQueueNode
 * \ingroup intBuffer
 *
 * \desc            Dequeue frame from the buffer queue corresponding to   
 *                  rcvPktEvent. This function is used internally in API when 
 *                  api.platform.priorityBufferQueues is enabled.
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \param[in]       rcvPktEvent points to the receive event of frame whose buffer
 *                  has to be dequeued from appropriate buffer queue. 
 *                  fm_eventPktRecv structure is used as arg in order to get swpri
 *                  ISL tag member so that corresponding buffer queue can be 
 *                  identified.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmFreeBufferQueueNode(int sw, fm_eventPktRecv *rcvPktEvent)
{
    fm_status err;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "sw=%d\n", sw);

#ifdef __FM_FM_PRIORITY_BUFFER_QUEUES_H
    err = fmPlatformFreeBufferQueueNode(rcvPktEvent);

    if (err == FM_OK)
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_BUFFER_QUEUE_NODE_FREES, 1);
    }
#else
    err = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, err);

}   /* end fmFreeBufferQueueNode */



/*****************************************************************************/
/** fmAddBufferChainInQueue
 * \ingroup intBuffer
 *
 * \desc            Enqueue frame into the buffer queue corresponding to
 *                  rcvPktEvent. This function is used internally when
 *                  api.platform.priorityBufferQueues is enabled.
 *
 * \param[in]       sw is not used. This is a legacy argument for backward
 *                  compatibility with existing applications.
 *
 * \param[in]       buf points to the buffer chain that contains the received
 *                  packet.
 *
 * \param[in]       pri is the switch priority of the frame in the buffer.
 *
 * \param[in]       isInsertBegin is TRUE if the packet is to be inserted
 *                  at the beginning of the queue, or FALSE if it goes
 *                  at the end of the queue.
 * 
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmAddBufferChainInQueue(int        sw, 
                                  fm_buffer *buf,
                                  fm_uint32  pri,
                                  fm_bool    isInsertBegin)
{
    fm_status err;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY_API(FM_LOG_CAT_BUFFER, "sw=%d\n", sw);

#ifdef __FM_FM_PRIORITY_BUFFER_QUEUES_H
    err = fmPlatformAddBufferChain(buf, pri, isInsertBegin);

    if (err == FM_OK)
    {
        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_BUFFER_QUEUE_NODE_FREES, 1);
    }
#else
    err = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT_API(FM_LOG_CAT_BUFFER, err);

}   /* end fmAddBufferChainInQueue */

