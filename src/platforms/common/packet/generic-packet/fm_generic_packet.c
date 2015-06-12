/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_generic_packet.c
 * Creation Date:   August, 2011
 * Description:     Generic packet sending code.
 *
 * Copyright (c) 2011 - 2015, Intel Corporation
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

/* Some platforms might not define this */
#ifndef FM_PACKET_OFFSET_ETHERTYPE
#define FM_PACKET_OFFSET_ETHERTYPE 3
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


/*****************************************************************************/
/** fmGetPortMaxFrameSizeInt
 * \ingroup intPlatformCommon
 *
 * \desc            Returns the port max frame size
 *
 * \note            This is an optimized version for packet processing without
 *                  any locking. The input parameters should already been
 *                  validated before calling this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port to get the max frame size.
 *
 * \param[out]      maxSize points to caller allocated storage where max size
 *                  is stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status fmGetPortMaxFrameSizeInt(fm_int  sw,
                                          fm_int  port,
                                          fm_int *maxSize)
{
    fm_portAttr *portAttr;

    portAttr  = GET_PORT_ATTR(sw, port);
    *maxSize  = portAttr->maxFrameSize;

    return FM_OK;

}   /* end fmGetPortMaxFrameSizeInt */




/*****************************************************************************/
/** ValidatePortList
 * \ingroup intPlatformCommon
 *
 * \desc            Validates that the ports in a port list are
 *                  valid for transmission.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       portList points to an array of logical port numbers the
 *                  switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if a port is invalid.
 *
 *****************************************************************************/
static fm_status ValidatePortList(fm_int sw, fm_int* portList, fm_int numPorts)
{
    fm_status err = FM_OK;
    fm_int    listIndex;
    fm_int    port;

    /* Verify the port numbers. */
    for (listIndex = 0 ; listIndex < numPorts ; listIndex++)
    {
        port = portList[listIndex];

        /* must be a valid cardinal, LAG, or remote port */
        if ( !fmIsValidPort(sw, port, ALLOW_CPU | ALLOW_LAG | ALLOW_REMOTE) )
        {
            err = FM_ERR_INVALID_PORT;
            break;
        }
    }

    return err;
} /* end ValidatePortList */




/*****************************************************************************/
/** ValidateFrameLength
 * \ingroup intPlatformCommon
 *
 * \desc            Validates that the frame length is valid for the CPU
 *                  port and returns the packet lengh.
 *
 * \param[in]       sw is the switch on which to send the packet.
 * 
 * \param[in]       cpuPort contains the logical port number for the CPU port. 
 *
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 * 
 * \param[in]       packetLength points to the caller allocated storage where
 *                  the packet length should be stored.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long
 * \return          FM_ERR_INVALID_ARGUMENT if buffer is not valid
 *
 *****************************************************************************/
static fm_status ValidateFrameLength(fm_int    sw, 
                                     fm_int     cpuPort, 
                                     fm_buffer *packet, 
                                     fm_int *   packetLength)
{
    fm_status err = FM_OK;
    fm_int    cpuMaxFrameSize;
    
    *packetLength = fmComputeTotalPacketLength(packet);

    if (*packetLength <= 0)
    {
        /* The buffer was malformed */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    err = fmGetPortMaxFrameSizeInt(sw,
                                   cpuPort,
                                   &cpuMaxFrameSize);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    if (*packetLength > cpuMaxFrameSize - 4)
    {
        err = FM_ERR_FRAME_TOO_LARGE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

ABORT:
    return err;

} /* end ValidateFrameLength */




/*****************************************************************************/
/** IsValidLagPort
 * \ingroup intPlatformCommon
 *
 * \desc            Validates that it is OK to send on this lag port.
 *
 * \param[in]       sw is the switch on which to send the packet.
 * 
 * \param[in]       port is the port to validate.
 * 
 * \param[out]      isValid is the caller allocated storage to store
 *                  the validity of the lag port
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status IsValidLagPort(fm_int   sw, 
                                fm_int   port,
                                fm_bool* isValid)
{
    fm_status   err;
    fm_int      firstLAGPort;

    *isValid = FALSE;

    /******************************************************
     * Check if the port is a LAG. If so
     * we proceed if the LAG has at least one member.
     * Note we do not check the state of the member ports.
     * In case a frame is hashed to a down member of the lag
     * the frame will be dropped by the switch.
     *****************************************************/

    err = fmGetLAGPortFirstExt(sw, port, &firstLAGPort);
    if (err != FM_OK)
    {
        /* The LAG is empty */
        err = FM_ERR_INVALID_PORT_STATE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    *isValid = TRUE;

ABORT:
    return err;

} /* end IsValidLagPort */




/*****************************************************************************/
/** FilterPortList
 * \ingroup intPlatformCommon
 *
 * \desc            Strip out ports that are down.
 *
 * \param[in]       sw is the switch on which to send the packet.
 * 
 * \param[in]       portList points to an array of logical port numbers the
 *                  switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 * 
 * \param[out]      newPortList points to an array to be filled with the
 *                  filtered logical port numbers on which the packets should
 *                  be sent. Ports that get filtered out will have
 *                  a value of -1.
 * 
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 * 
 * \param[in]       cpuPort contains the logical port number for the CPU port.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status FilterPortList(fm_int     sw, 
                                fm_int*    portList, 
                                fm_int     numPorts, 
                                fm_int*    newPortList, 
                                fm_buffer *packet,
                                fm_int     cpuPort)
{
    fm_status       err = FM_OK;
    fm_int          listIndex;
    fm_int          port;
    fm_port *       dPort;
    fm_int          portSkipCount;
    fm_bool         isLagPortUp;
    fm_bool         allowDirectSendToCpu;
    
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw = %d\n", sw);

    portSkipCount = 0;

    /* Iterate through port list and eliminate ports that are down */
    for (listIndex = 0 ; listIndex < numPorts ; listIndex++)
    {
        port  = portList[listIndex];
        dPort = GET_PORT_PTR(sw, port);

        /* Apply special filtering for LAG ports */
        if (dPort->portType == FM_PORT_TYPE_LAG)
        {
            err = IsValidLagPort(sw, port, &isLagPortUp);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

            if (!isLagPortUp)
            {
                /* Mark this port as SKIP */
                newPortList[listIndex] = -1;
                portSkipCount++;
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                             "Invalid port state: skipping port %d\n",
                              port);
                continue;
            }
        }
        else
        {
            /* Filter out down ports */
            if (!fmIsPortLinkUp(sw, port))
            {
                /* Mark this port as SKIP */
                newPortList[listIndex] = -1;
                portSkipCount++;
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                             "Invalid port state: skipping port %d\n",
                              port);
                continue;
            }

            /* Filter out CPU port */
            if (port == cpuPort)
            {
                /* Special handling of sending to the CPU port in the directed mode */
                allowDirectSendToCpu =
                    fmGetBoolApiProperty(FM_AAK_API_DIRECT_SEND_TO_CPU,
                                         FM_AAD_API_DIRECT_SEND_TO_CPU);

                if (!allowDirectSendToCpu)
                {
                    newPortList[listIndex] = -1;
                    portSkipCount++;
                    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                             "Invalid port state: skipping port %d\n",
                              port);
                    continue;
                }
            }
        }

        /* Port is good, add it to the new list */
        newPortList[listIndex] = port;
    }

    if (portSkipCount == numPorts)
    {
        /********************************************************
         * If none of the ports in the portList is in the
         * proper state, return an error to the user application.
         ********************************************************/
        err = FM_ERR_INVALID_PORT_STATE;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

} /* end FilterPortList */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmPacketQueueLock
 * \ingroup intPlatformCommon
 *
 * \desc            Lock packet queue.
 *
 * \param[in]       queue is the pointer to the packet queue.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPacketQueueLock(fm_packetQueue *queue)
{
#if 1
    if (pthread_mutex_lock(&queue->mutex))
    {
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_PKT_TX, 
                      FALSE,
                      "Failed to lock queue's mutex!\n");
    }
#else
    TAKE_PLAT_LOCK(queue->switchNum, FM_PLAT_INFO);
#endif

}   /* end fmPacketQueueLock */




/*****************************************************************************/
/** fmPacketQueueUnlock
 * \ingroup intPlatformCommon
 *
 * \desc            Unlock packet queue.
 *
 * \param[in]       queue is the pointer to the packet queue.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPacketQueueUnlock(fm_packetQueue *queue)
{
#if 1
    if (pthread_mutex_unlock(&queue->mutex))
    {
        FM_LOG_ASSERT(FM_LOG_CAT_EVENT_PKT_TX, 
                      FALSE,
                      "Failed to unlock queue's mutex!\n");
    }
#else
    DROP_PLAT_LOCK(queue->switchNum, FM_PLAT_INFO);
#endif

}   /* end fmPacketQueueUnlock */




/*****************************************************************************/
/** fmPacketQueueInit
 * \ingroup intPlatformCommon
 *
 * \desc            Initialize packet queue.
 *
 * \param[in]       queue is the pointer to the packet queue.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPacketQueueInit(fm_packetQueue *queue, fm_int sw)
{
    /* Initialize mutex if it is enabled */
    pthread_mutexattr_t attr;

    if (!queue) 
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    memset(queue, 0, sizeof(*queue));
    queue->switchNum = sw;

    if ( pthread_mutexattr_init(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) ||
         pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) )
    {
        pthread_mutexattr_destroy(&attr);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutex_init(&queue->mutex, &attr) )
    {
        pthread_mutexattr_destroy(&attr);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_LOCK_INIT);
    }

    if ( pthread_mutexattr_destroy(&attr) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_LOCK_INIT);
    }

    return FM_OK;

}   /* end fmPacketQueueInit */




/*****************************************************************************/
/** fmPacketQueueFree
 * \ingroup intPlatformCommon
 *
 * \desc            Free all buffers in packet queue.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPacketQueueFree(fm_int sw)
{
    fm_status       err       = FM_OK;
    fm_packetQueue *txQueue;
    fm_packetEntry *packet;

    txQueue = &GET_PLAT_PKT_STATE(sw)->txQueue;
    fmPacketQueueLock(txQueue);

    for ( ;
          txQueue->pullIndex != txQueue->pushIndex ;
          txQueue->pullIndex = (txQueue->pullIndex + 1) % FM_PACKET_QUEUE_SIZE)
    {
        packet = &txQueue->packetQueueList[txQueue->pullIndex];

        if (packet && packet->freePacketBuffer)
        {
            fmFreeBuffer(sw, packet->packet);
        }

        fmDbgGlobalDiagCountIncr(FM_GLOBAL_CTR_TX_BUFFER_FREES, 1);

    }

    fmPacketQueueUnlock(txQueue);

    return err;

}   /* end fmPacketQueueFree */




/*****************************************************************************/
/** fmPacketQueueUpdate
 * \ingroup intPlatformCommon
 *
 * \desc            Update packet queue by advancing one entry
 *
 * \param[in]       queue is the pointer to the packet queue.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if queue if full.
 *
 *****************************************************************************/
fm_status fmPacketQueueUpdate(fm_packetQueue *queue)
{

    /* check if the Tx queue is full */
    if ( (queue->pushIndex + 1) % FM_PACKET_QUEUE_SIZE == queue->pullIndex )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmPacketQueueUpdate:"
                     "TX queue is full?: pushIndex = %d, pullIndex = %d\n",
                     queue->pushIndex, queue->pullIndex);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_TX_PACKET_QUEUE_FULL);
    }
    else
    {
        /* updated indices */
        ++queue->pushIndex;
        queue->pushIndex = queue->pushIndex % FM_PACKET_QUEUE_SIZE;
    }

    return FM_OK;

}   /* end fmPacketQueueUpdate */




/*****************************************************************************/
/** fmPacketQueueEnqueue
 * \ingroup intPlatformCommon
 *
 * \desc            Queue packet to the tx packet queue
 *
 * \param[in]       queue is the pointer to the packet queue.
 *
 * \param[in]       packet is the buffer containing the packet.
 *
 * \param[in]       packetLength is the length of the packet.
 *
 * \param[in]       islTag is the pointer containing ISL tag words.
 * 
 * \param[in]       islTagFormat is the isl tag format.
 *
 * \param[in]       suppressVlanTag is the flag to indicate whether
 *                  to suppress the vlan tag in the data.
 *
 * \param[in]       freeBuffer is the flag to indicate whether
 *                  to free the packet buffer or not.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPacketQueueEnqueue(fm_packetQueue * queue,
                               fm_buffer *      packet,
                               fm_int           packetLength,
                               fm_islTag *      islTag,
                               fm_islTagFormat  islTagFormat,
                               fm_bool          suppressVlanTag,
                               fm_bool          freeBuffer)
{
    fm_packetEntry *entry;

    entry = &queue->packetQueueList[queue->pushIndex];

    entry->packet = packet;
    entry->length = packetLength;
    entry->fcsVal = FM_USE_DEFAULT_FCS;
    FM_MEMCPY_S( &entry->islTag,
                 sizeof(entry->islTag),
                 islTag,
                 sizeof(fm_islTag) );
    entry->islTagFormat = islTagFormat;
    entry->suppressVlanTag  = suppressVlanTag;
    entry->freePacketBuffer = freeBuffer;

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                 "fm_packet_queue_enqueue: packet queued "
                 "in slot %d, length %d bytes\n",
                 queue->pushIndex,
                 entry->length);

    /* check if the Tx queue is full */
    if ( (queue->pushIndex + 1) % FM_PACKET_QUEUE_SIZE == queue->pullIndex )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fm_packet_queue_enqueue:"
                     "TX queue is full?: pushIndex = %d, pullIndex = %d\n",
                     queue->pushIndex, queue->pullIndex);
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_TX_PACKET_QUEUE_FULL);
    }
    else
    {
        /* updated indices */
        ++queue->pushIndex;
        queue->pushIndex = queue->pushIndex % FM_PACKET_QUEUE_SIZE;
    }

    return FM_OK;

}   /* end fmPacketQueueEnqueue */




/*****************************************************************************/
/** fmPacketReceiveEnqueue
 * \ingroup intPlatformCommon
 *
 * \desc            Processes a complete received packet event and
 *                  tries to send it upward.
 *
 * \note            This is an optimized version for packet processing without
 *                  any locking. The input parameters should already been
 *                  validated before calling this function.
 *
 * \param[in]       sw is the switch to send the event for.
 *
 * \param[in]       event is a fully filled event structure.
 *
 * \param[in]       selfTestEventHandler is a pointer to the event handler.
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPacketReceiveEnqueue(fm_int     sw,
                                 fm_event * event,
                                 fm_switchEventHandler selfTestEventHandler)
{
    fm_eventPktRecv *pktEvent = &event->info.fpPktEvent;
    fm_status               err      = FM_OK;
    fm_bool                 isLacpToBeDropped;
    fm_bool                 isPktSFlowLogged;
    fm_switchEventHandler   switchEventHandler;
    fm_switch              *switchPtr;
    fm_bool                 enableFramePriority;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_RX,
                 "sw = %d, event->type = %d, event->sw = %d, "
                 "event->eventID = %d\n",
                 sw,
                 event->type,
                 event->sw,
                 event->eventID);

    switchPtr = GET_SWITCH_PTR(sw);

    switchEventHandler = switchPtr->eventHandler;

#ifdef __FM_FM_PRIORITY_BUFFER_QUEUES_H
    enableFramePriority = fmRootPlatform->enablePriorityScheduling; 
#else
    enableFramePriority = FALSE;
#endif

    /***************************************************
     * Check the LACP filter configuration based on the
     * switch the event is associated with, not the
     * switch the frame was received on.  These are
     * potentially different in an FIBM environment.
     **************************************************/
    fmCheckLACPFilter(event->sw, pktEvent, &isLacpToBeDropped);

    if (!isLacpToBeDropped)
    {
        /* Check if the packet is logged by a sFlow instance */
        if (switchPtr->CheckSFlowLogging)
        {
            (void)switchPtr->CheckSFlowLogging(sw, pktEvent, &isPktSFlowLogged);

            if (isPktSFlowLogged)
            {
                /* Change the type to FM_EVENT_SFLOW_PKT_RECV */
                event->type = FM_EVENT_SFLOW_PKT_RECV;
            }
        }
        
        /* Don't do a direct enqueue if in self-test */
        if (GET_PLAT_PKT_STATE(sw)->rxDirectEnqueueing &&
            switchEventHandler != selfTestEventHandler)
        {
            /* Pass directly up to the application */
            if (fmEventHandler != NULL)
            {
                if (enableFramePriority)
                {
                    err = fmFreeBufferQueueNode(sw, pktEvent);
                    if (err == FM_ERR_NOT_FOUND)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Node with buffer of priority: %d "
                                     "already removed. Should have been"
                                     " flushed due to prioritized allocation",
                                     pktEvent->priority);
                        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, FM_OK);
                    }
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Freeing Buffer queue node from the queue failed"
                                     "status = %d (%s) \n",
                                      err,
                                      fmErrorMsg(err));
                        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);
                    }
                }

                fmEventHandler(event->type, event->sw, &event->info);
                /* Don't need to release event, since event structure
                 * is on the stack for direct enqueueing method
                 */
            }
            else
            {
                if (enableFramePriority)
                {
                    err = fmFreeBufferQueueNode(sw, pktEvent);
                    if (err == FM_ERR_NOT_FOUND)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Node already removed. Should have been"
                                     " flushed due to prioritized allocation");
                        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, FM_OK);
                    }
                    else if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Freeing Buffer queue node from the queue failed"
                                     "status = %d (%s) \n",
                                      err,
                                      fmErrorMsg(err));
                        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);

                    }
                }
                /* Drop the packet without forwarding it. */
                fmFreeBufferChain(sw, (fm_buffer *) pktEvent->pkt);
                fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_PORT, 1);
            }
            FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, FM_OK);
        }
        else
        {
            err = fmSendThreadEvent(&fmRootApi->eventThread, event);

            if (err != FM_OK)
            {
                if (enableFramePriority)
                {
                    err = fmFreeBufferQueueNode(sw, pktEvent);
                    if (err == FM_ERR_NOT_FOUND)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Node already removed. Should have been"
                                     " flushed due to prioritized allocation");

                        /* Buffer and Event already released while flushing 
                         * low priority packet. */
                        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);
                    }
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                                     "Freeing Buffer queue node from the queue failed"
                                     "status = %d (%s) \n",
                                      err,
                                      fmErrorMsg(err));
                        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);
                    }
                }
                /* Free the event since we could not send it to thread */
                fmFreeBufferChain(sw, (fm_buffer *) pktEvent->pkt);
                fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_NO_EVENT, 1);
                fmReleaseEvent(event);
            }
        }

    }
    else
    {
        if (enableFramePriority)
        {
            err = fmFreeBufferQueueNode(sw, pktEvent);
            if (err == FM_ERR_NOT_FOUND)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_RX,
                             "Node already removed. Should have been"
                             " flushed due to prioritized allocation");

                /* Buffer and Event already released while flushing 
                 * low priority packet. */
                FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);
            }

            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_EVENT_PKT_RX,
                             "Freeing Buffer queue node from the queue failed"
                             "status = %d (%s) \n",
                              err,
                              fmErrorMsg(err));

            }
        }
        /* Drop the packet without forwarding it. */
        fmFreeBufferChain(sw, (fm_buffer *) pktEvent->pkt);
        fmDbgDiagCountIncr(sw, FM_CTR_RX_PKT_DROPS_LACP, 1);
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_RX, err);

}   /* end fmPacketReceiveEnqueue */




/*****************************************************************************/
/** fmPacketClearCRC
 * \ingroup intPlatformCommon
 *
 * \desc            Clear the last 4 bytes in the buffer chain. 
 *
 * \param[in]       buffer points to the receive packet buffer.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPacketClearCRC(fm_buffer *buffer)
{
    fm_buffer *lastBuf = buffer;
    fm_buffer *nextToLastBuf = buffer;
    fm_byte   *pByte;
    fm_int    rem;

    if (buffer->next == NULL)
    {
        /* Quick check for small packets */
        pByte = (fm_byte*)buffer->data;
        memset(pByte+buffer->len-4, 0, 4);
        return;
    }

    while (buffer)
    {
        nextToLastBuf = lastBuf;
        lastBuf = buffer;
        buffer = buffer->next;
    }


    if (lastBuf->len >=4)
    {
        pByte = (fm_byte*)lastBuf->data;
        memset(pByte+lastBuf->len-4, 0, 4);
    }
    else
    {
        pByte = (fm_byte*)lastBuf->data;
        memset(pByte, 0, lastBuf->len);

        rem = 4 - lastBuf->len;
        pByte = (fm_byte*)nextToLastBuf->data;
        memset(pByte+nextToLastBuf->len-rem, 0, rem);
    }

}   /* end fmPacketClearCRC */




/*****************************************************************************/
/** fmPacketGetCRC
 * \ingroup intPlatformCommon
 *
 * \desc            Returns the last 4 bytes in the buffer chain.
 *
 * \param[in]       buffer points to the receive packet buffer.
 *
 * \return          FCS value for the buffer.
 *
 *****************************************************************************/
fm_uint32 fmPacketGetCRC(fm_buffer *buffer)
{
    fm_buffer * lastBuf = NULL;
    fm_buffer * prevBuf = NULL;
    fm_byte   * pByte;
    fm_int      rem;

    union {
        fm_uint32 w;
        fm_byte   b[4];
    } fcsVal;

    if (buffer == NULL)
    {
        return 0;
    }

    while (buffer)
    {
        prevBuf = lastBuf;
        lastBuf = buffer;
        buffer = buffer->next;
    }

    if (lastBuf->len >= 4)
    {
        pByte = ((fm_byte *) lastBuf->data) + lastBuf->len - 4;
        FM_MEMCPY_S(&fcsVal.b[0], sizeof(fcsVal.b), pByte, 4);
    }
    else if (prevBuf == NULL)
    {
        return 0;
    }
    else
    {
        rem = 4 - lastBuf->len;
        pByte = ((fm_byte *) prevBuf->data) + prevBuf->len - rem;
        FM_MEMCPY_S(fcsVal.b, sizeof(fcsVal.b), pByte, rem);
        FM_MEMCPY_S(&fcsVal.b[rem], 4 - rem, lastBuf->data, lastBuf->len);
    }

    return ntohl(fcsVal.w);

}   /* end fmPacketGetCRC */




/*****************************************************************************/
/** fmFindSlaveSwitchPortByGlort
 * \ingroup intPlatformCommon
 *
 * \desc            Find out which switch and port this glort belong to
 *
 * \param[in]       glort is the glort number.
 *
 * \param[out]      switchNum is the pointer to hold the switch number.
 *
 * \param[out]      port is the pointer to hold the port number.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if not found.
 *
 *****************************************************************************/
fm_status fmFindSlaveSwitchPortByGlort(fm_uint32 glort, 
                                       fm_int *switchNum, 
                                       fm_int *port)
{
    fm_int    sw;
    fm_status err = FM_FAIL;

    for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
    {
        if (!SWITCH_LOCK_EXISTS(sw))
        {
            continue;
        }

        fm_switch *switchPtr;

        switchPtr = GET_SWITCH_PTR(sw);

        if (!switchPtr)
        {
            continue;
        }
        /* Need to take a switch lock here ?? */
        if ( (glort & ~switchPtr->glortRange.glortMask) ==
            switchPtr->glortRange.glortBase )
        {
            *switchNum = sw;
            err = fmGetGlortLogicalPort(sw, glort, port);
            break;
        }
    }

    return err;

}   /* end fmFindSlaveSwitchPortByGlort */




/*****************************************************************************/
/** fmComputeTotalPacketLength
 * \ingroup intPlatformCommon
 *
 * \desc            Computes the packet length by iterating through buffers.
 *
 * \param[in]       packet is a pointer to an fm_buffer representing the first
 *                  buffer in the packet.
 *
 * \return          The packet length in bytes.
 *
 *****************************************************************************/
fm_int fmComputeTotalPacketLength(fm_buffer *packet)
{
    fm_int length = 0;

    while (packet)
    {
        if ( (packet->len < 0) || (packet->len > FM_BUFFER_SIZE_BYTES) )
        {
            length = -1;
            return length;
        }
        length += packet->len;
        packet  = packet->next;
    }

    return length;

}   /* end fmComputeTotalPacketLength */




/*****************************************************************************/
/** fmGetPortDefVlanInt
 * \ingroup intPlatformCommon
 *
 * \desc            Returns the port default vlan.
 *
 * \note            This is an optimized version for packet processing without
 *                  any locking. The input parameters should already been
 *                  validated before calling this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port to get the default vlan.
 *
 * \param[out]      vlan points to caller allocated storage where default vlan
 *                  is stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetPortDefVlanInt(fm_int     sw,
                              fm_int     port,
                              fm_uint16 *vlan)
{
    fm_portAttr *portAttr;

    portAttr = GET_PORT_ATTR(sw, port);

    *vlan = portAttr->defVlan;

    return FM_OK;

}   /* end fmGetPortDefVlanInt */




/*****************************************************************************/
/** fmGetPortDefVlanDefPriorityInt
 * \ingroup intPlatformCommon
 *
 * \desc            Returns the port default vlan and priority.
 *
 * \note            This is an optimized version for packet processing without
 *                  any locking. The input parameters should already been
 *                  validated before calling this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port to get the default vlan and priority.
 *
 * \param[out]      vlan points to caller allocated storage where default vlan
 *                  is stored.
 *
 * \param[out]      priority points to caller allocated storage where default priority
 *                  is stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetPortDefVlanDefPriorityInt(fm_int     sw,
                                         fm_int     port,
                                         fm_uint16 *vlan,
                                         fm_byte *  priority)
{
    fm_portAttr *portAttr;

    portAttr = GET_PORT_ATTR(sw, port);

    *vlan = portAttr->defVlan;
    *priority = portAttr->defVlanPri;

    return FM_OK;

}   /* end fmGetPortDefVlanDefPriorityInt */




/*****************************************************************************/
/** fmGenericPacketDestroy
 * \ingroup intPlatformCommon
 *
 * \desc            Performs generic packet transfer cleanup.
 *
 * \param[in]       sw is the switch number to clean.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGenericPacketDestroy(fm_int sw)
{
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX, "sw = %d\n", sw);

    fmPacketQueueFree(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmGenericPacketDestroy */




/*****************************************************************************/
/** fmGenericPacketHandlingInitialize
 * \ingroup intPlatformCommon
 *
 * \desc            Performs generic packet transfer initialization.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGenericPacketHandlingInitialize(fm_int sw)
{
    return fmGenericPacketHandlingInitializeV2(sw, FALSE);
}




/*****************************************************************************/
/** fmGenericPacketHandlingInitializeV2
 * \ingroup intPlatformCommon
 *
 * \desc            Performs generic packet transfer initialization.
 *
 * \param[in]       sw is the switch number to initialize.
 * 
 * \param[in]       hasFcs is TRUE if the packet includes the FCS field.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGenericPacketHandlingInitializeV2(fm_int sw, fm_bool hasFcs)
{
    fm_packetHandlingState *ps = GET_PLAT_PKT_STATE(sw);
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d hasFcs = %s\n",
                 sw,
                 FM_BOOLSTRING(hasFcs));

    /* clear out all state */
    memset( ps, 0, sizeof(fm_packetHandlingState) );

    fmPacketQueueInit(&ps->txQueue, sw);
    
    /* reset state here */
    ps->recvInProgress       = FALSE;
    ps->recvBufferOffset     = 0;
    ps->currentWordsReceived = 0;
    ps->cachedEndianness     = -1; /* -1 indicates unspecified endianness */
    ps->sendUserFcs          = hasFcs;

    ps->rxDirectEnqueueing   =
        fmGetBoolApiProperty(FM_AAK_API_PACKET_RX_DIRECT_ENQUEUEING,
                             FALSE);

    /* initialize the signal sem for event availability */
    err = fmCreateSemaphore("netdevEventsAvailable",
                            FM_SEM_BINARY,
                            &ps->eventsAvailableSignal, 
                            0);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmGenericPacketHandlingInitializeV2 */




/*****************************************************************************/
/** fmGenericSendPacketISL
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add a packet to the TX packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       islTagList points to an array of islTag to send out
 *                  along with packet
 * 
 * \param[in]       islTagFormat is the isl tag format. 
 *
 * \param[in]       numPorts is the number of elements in islTagList.
 *
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if transmit packet queue is full.
 * \return          FM_FAIL if network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmGenericSendPacketISL(fm_int          sw,
                                 fm_islTag *     islTagList,
                                 fm_islTagFormat islTagFormat,
                                 fm_int          numPorts,
                                 fm_buffer *     packet)
{
    fm_packetQueue         *txQueue;
    fm_int                  packetLength;
    fm_int                  port;
    fm_int                  cpuMaxFrameSize;
    fm_int                  oldPushIndex;
    fm_int                  cpuPort;
    fm_int                  masterSw; /* For support FIBM slave switch */
    fm_status               err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "islTag = %p, "
                 "numPorts = %d, "
                 "packet->index = 0x%x\n",
                 sw,
                 (void *) islTagList,
                 numPorts,
                 packet->index);

    masterSw  = fmFibmSlaveGetMasterSwitch(sw);

    if (masterSw >= 0)
    {
        VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, masterSw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
        
        err = fmGetCpuPortInt(masterSw, &cpuPort);
        
        if (err == FM_OK)
        {
            err = fmGetPortMaxFrameSizeInt(masterSw,
                                           cpuPort,
                                           &cpuMaxFrameSize);
        }
        
        UNPROTECT_SWITCH(masterSw);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }
    else
    {
        /* This is for FIBM using the NIC. */
        cpuMaxFrameSize = 10000; /* Just allow to pass */
        
        /* Standalone use itself as master switch */
        masterSw = sw;
    }

    if (!fmIsRawPacketSocketDeviceOperational(masterSw))
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_FAIL);

    }

    packetLength = fmComputeTotalPacketLength(packet);

    if (packetLength <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    if (packetLength > cpuMaxFrameSize - 4)
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_FRAME_TOO_LARGE);
    }

    txQueue = &GET_PLAT_PKT_STATE(masterSw)->txQueue;
     
    fmPacketQueueLock(txQueue);

    oldPushIndex = txQueue->pushIndex;

    for (port = 0 ; port < numPorts ; port++)
    {
        fm_bool freePacketBuffer;

        if (port < numPorts - 1)
        {
            freePacketBuffer = FALSE;
        }
        else
        {
            /* only free the packet buffer if we have sent to all target
             *  ports in the vlan */
            freePacketBuffer = TRUE;
        }
        err = fmPacketQueueEnqueue(txQueue,
                                   packet,
                                   packetLength,
                                   &islTagList[port],
                                   islTagFormat,
                                   FALSE,
                                   freePacketBuffer);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    fmPacketQueueUnlock(txQueue);

    if (err == FM_OK)
    {
        if (!fmRootPlatform->dmaEnabled)
        {
            fm_switch *switchPtr;
            switchPtr = GET_SWITCH_PTR(sw);

            /**************************************************
             * We must take a lock before writing
             * intrSendPackets because the lock is used
             * by the API to ensure an atomic read-modify-write
             * to intrSendPackets.
             *
             * The platform lock is used instead of state lock
             * because on FIBM platforms, there is an access
             * to intrSendPackets that must be protected before
             * the switch's locks are even created. 
             **************************************************/
            
            FM_TAKE_PKT_INT_LOCK(sw);
            switchPtr->intrSendPackets = TRUE;
            FM_DROP_PKT_INT_LOCK(sw);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmGenericSendPacketISL: triggering interrupt handler\n");

        /* Handle the following error case cleanly! */
        err = fmPlatformTriggerInterrupt(masterSw, FM_INTERRUPT_SOURCE_API);

        if (err != FM_OK)
        {
            /***************************************************************
             * We are here because sem_post() fails. In this case we can not
             * cleanly unwind the TX queue to recover, so we simply log a
             * fatal error, and return FM_OK.
             **************************************************************/
            FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX,
                         "fmGenericSendPacketISL: "
                         "fmPlatformTriggerInterrupt returned error");

            err = FM_OK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

ABORT:
    txQueue->pushIndex = oldPushIndex;
    fmPacketQueueUnlock(txQueue);
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmGenericSendPacketISL */




/*****************************************************************************/
/** fmGenericSendPacketDirected
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add a packet to the TX packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       portList points to an array of logical port numbers the
 *                  switch is to send the packet.
 *
 * \param[in]       numPorts is the number of elements in portList.
 *
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 * 
 * \param[in]       fcsValue is the value to be sent in the FCS field.
 *
 * \param[in]       cpuPort contains the logical port number for the CPU port. 
 *
 * \param[in]       switchPriority is the switch priority.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if transmit packet queue is full.
 * \return          FM_FAIL if network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmGenericSendPacketDirected(fm_int     sw,
                                      fm_int *   portList,
                                      fm_int     numPorts,
                                      fm_buffer *packet,
                                      fm_uint32  fcsValue,
                                      fm_int     cpuPort,
                                      fm_uint32  switchPriority)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm_packetQueue         *txQueue;
    fm_packetEntry *        entry;
    fm_int                  packetLength;
    fm_int                  listIndex;
    fm_int                  port;
    fm_packetInfo           tempInfo;
    fm_int                  oldPushIndex;
    fm_int                  masterSw; /* For support FIBM slave switch */
    fm_int                  newPortList[numPorts];
    fm_bool                 packetQueueLockFlag;
    
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "portList = %p, "
                 "numPorts = %d, "
                 "packet->index = 0x%x\n",
                 sw,
                 (void *) portList,
                 numPorts,
                 packet->index);

    switchPtr           = GET_SWITCH_PTR(sw);
    txQueue             = &GET_PLAT_PKT_STATE(sw)->txQueue;
    oldPushIndex        = -1;
    packetQueueLockFlag = FALSE;
    
    /* Validate all ports are valid */
    err = ValidatePortList(sw, portList, numPorts);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    /* Validate that the frame respects the CPU frame length */
    err = ValidateFrameLength(sw, cpuPort, packet, &packetLength);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err); 
    
    /* Strip out ports that are down */
    err = FilterPortList(sw, portList, numPorts, newPortList, packet, cpuPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    if (fmRootApi->isSwitchFibmSlave[sw])
    {
        masterSw  = fmFibmSlaveGetMasterSwitch(sw);
        if (masterSw < 0)
        {
            /* In standalone NIC mode, the 
             * master is also the same as the slave.
             */
            masterSw = sw;
        }
        else
        {
            /* We are sending packet via master switch
             * These is no platform lock on slave switch in 
             * master slave mode.
             */
            switchPtr = GET_SWITCH_PTR(masterSw);
            txQueue   = &GET_PLAT_PKT_STATE(masterSw)->txQueue;
        }
    }
    else
    {
        masterSw = sw;
    }

    if (!fmIsRawPacketSocketDeviceOperational(masterSw))
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_FAIL);

    }

    fmPacketQueueLock(txQueue);
    packetQueueLockFlag = TRUE;

    /***********************************************************
     * oldSendPushIndex records the current push index in the TX
     * queue. We use it to keep tab on where we started upon
     * entering this function, in case of the need for roll back
     * on the push index when (1) the tx queue is full; or (2)
     * the function calls returns an error which we return to the
     * user application, after having enqued some entries in the
     * tx queue.
     **********************************************************/
    oldPushIndex = txQueue->pushIndex;

    for (listIndex = 0 ; listIndex < numPorts ; listIndex++)
    {
        port  = newPortList[listIndex];

        /* If port has been filtered out, skip it */
        if (port == -1)
        {
            continue;
        }

        /* tempInfo will be used to generate ISL tag */
        memset( &tempInfo, 0, sizeof(tempInfo) ); 
        entry = &txQueue->packetQueueList[txQueue->pushIndex];

        /* Build the packetEntry */
        entry->packet = packet;
        entry->length = packetLength;
        entry->fcsVal = fcsValue;

        /***********************************************************
         * Update some of the parameters in tempInfo which will
         * be used to generate ISL tag
         **********************************************************/

        /* The CPU port did not get filtered out */
        if (port == cpuPort)
        {
            tempInfo.directSendToCpu = TRUE;
        }

        tempInfo.logicalPort = port;

        if (switchPtr->defaultSourcePort < 0)
        {
            tempInfo.zeroSourceGlort = TRUE;
        }
        else
        {
            tempInfo.sourcePort = switchPtr->defaultSourcePort;
        }
        
        /***********************************************************
         * Generate ISL tag header
         **********************************************************/
        err = switchPtr->GeneratePacketISL(sw,
                                           packet,
                                           &tempInfo,
                                           cpuPort,
                                           switchPriority,
                                           &entry->islTagFormat,
                                           &entry->islTag,
                                           &entry->suppressVlanTag);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmGenericSendPacketDirected: packet queued "
                     "in slot %d, length %d bytes, port %d\n",
                     txQueue->pushIndex,
                     entry->length,
                     port);

        err = fmPacketQueueUpdate(txQueue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        if (listIndex < numPorts - 1)
        {
            entry->freePacketBuffer = FALSE;
        }
        else
        {
            /* only free the packet buffer if we have sent to all target
            *  ports in the vlan */
            entry->freePacketBuffer = TRUE;
        }
    }

    fmPacketQueueUnlock(txQueue);
    packetQueueLockFlag = FALSE;

    if (err == FM_OK)
    {
        if (!fmRootPlatform->dmaEnabled)
        {
            /**************************************************
             * We must take a lock before writing
             * intrSendPackets because the lock is used
             * by the API to ensure an atomic read-modify-write
             * to intrSendPackets.
             *
             * The platform lock is used instead of state lock
             * because on FIBM platforms, there is an access
             * to intrSendPackets that must be protected before
             * the switch's locks are even created. 
             **************************************************/
            
            FM_TAKE_PKT_INT_LOCK(sw);
            switchPtr->intrSendPackets = TRUE;
            FM_DROP_PKT_INT_LOCK(sw);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmGenericSendPacketDirected: "
                     "triggering interrupt handler\n");

        err = fmPlatformTriggerInterrupt(masterSw, FM_INTERRUPT_SOURCE_API);

        /* Handle the following error case cleanly! */
        if (err != FM_OK)
        {
            /***************************************************************
             * We are here because sem_post() fails. In this case we can not
             * cleanly unwind the TX queue to recover, so we simply log a
             * fatal error, and return FM_OK.
             **************************************************************/
            FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX, 
                         "fmGenericSendPacket: "
                         "fmPlatformTriggerInterrupt returned error");

            err = FM_OK;
        }
    }

ABORT:
    if (packetQueueLockFlag)
    {
        /* There was an error, lets restore the txQueue's push index */
        if (oldPushIndex != -1)
        {
            txQueue->pushIndex = oldPushIndex;
        }

        fmPacketQueueUnlock(txQueue);
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmGenericSendPacketDirected */




/*****************************************************************************/
/** fmGenericSendPacketSwitched
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add a packet to the TX packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       packet points to the packet buffer's first ''fm_buffer''
 *                  structure in a chain of one or more buffers.
 *
 * \param[in]       cpuPort contains the logical port number for the CPU port. 
 *
 * \param[in]       switchPriority is the switch priority.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if packet is not valid packet buffer
 * \return          FM_ERR_INVALID_PORT if port is not valid
 * \return          FM_ERR_INVALID_SWITCH if sw is not valid
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if the transmit packet queue is full.
 * \return          FM_ERR_FRAME_TOO_LARGE if the packet is too long.
 * \return          FM_FAIL if network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmGenericSendPacketSwitched(fm_int     sw,
                                      fm_buffer *packet,
                                      fm_int     cpuPort,
                                      fm_uint32  switchPriority)
{
    fm_switch *             switchPtr = GET_SWITCH_PTR(sw);
    fm_packetQueue         *txQueue;
    fm_packetEntry *        entry;
    fm_int                  packetLength;
    fm_status               err = FM_OK;
    fm_packetInfo           tempInfo;
    fm_int                  masterSw;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "packet->index = 0x%x\n",
                 sw,
                 packet->index);

    txQueue   = &GET_PLAT_PKT_STATE(sw)->txQueue;

    err = ValidateFrameLength(sw, cpuPort, packet, &packetLength);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    if (fmRootApi->isSwitchFibmSlave[sw])
    {
        masterSw  = fmFibmSlaveGetMasterSwitch(sw);
        if (masterSw < 0)
        {
            /* In standalone NIC mode, the 
             * master is also the same as the slave.
             */
            masterSw = sw;
        }
        else
        {
            /* We are sending packet via master switch
             * These is no platform lock on slave switch in 
             * master slave mode.
             */
            switchPtr = GET_SWITCH_PTR(masterSw);
            txQueue   = &GET_PLAT_PKT_STATE(masterSw)->txQueue;
        }
    }
    else
    {
        masterSw = sw;
    }

    if (!fmIsRawPacketSocketDeviceOperational(masterSw))
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_FAIL);

    }

    fmPacketQueueLock(txQueue); 

    entry = &txQueue->packetQueueList[txQueue->pushIndex];

    /* Build the packetEntry */
    entry->packet           = packet;
    entry->length           = packetLength;
    entry->fcsVal           = FM_USE_DEFAULT_FCS;
    entry->freePacketBuffer = TRUE;

    memset(&tempInfo,0,sizeof(fm_packetInfo));
    tempInfo.logicalPort     = FM_LOG_PORT_USE_FTYPE_NORMAL;
    tempInfo.sourcePort      = switchPtr->defaultSourcePort;
    tempInfo.switchPriority  = FM_USE_VLAN_PRIORITY;
    tempInfo.directSendToCpu = FALSE;

    err = switchPtr->GeneratePacketISL(sw,
                                       packet,
                                       &tempInfo,
                                       cpuPort,
                                       switchPriority,
                                       &entry->islTagFormat,
                                       &entry->islTag,
                                       &entry->suppressVlanTag);
    if (err != FM_OK)
    {
        fmPacketQueueUnlock(txQueue); 
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    err = fmPacketQueueUpdate(txQueue);

    if (err != FM_OK)
    {
        /* Check for queue FULL */
        fmPacketQueueUnlock(txQueue);
    
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    fmPacketQueueUnlock(txQueue);

    if (!fmRootPlatform->dmaEnabled)
    {
        /**************************************************
         * We must take a lock before writing
         * intrSendPackets because the lock is used
         * by the API to ensure an atomic read-modify-write
         * to intrSendPackets.
         *
         * The platform lock is used instead of state lock
         * because on FIBM platforms, there is an access
         * to intrSendPackets that must be protected before
         * the switch's locks are even created. 
         **************************************************/
        
        FM_TAKE_PKT_INT_LOCK(sw);
        switchPtr->intrSendPackets = TRUE;
        FM_DROP_PKT_INT_LOCK(sw);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                 "fmGenericSendPacketSwitched: "
                 "triggering interrupt handler\n");

    err = fmPlatformTriggerInterrupt(masterSw, FM_INTERRUPT_SOURCE_API);

    /* Handle the following error case cleanly! */
    if (err != FM_OK)
    {
        /***************************************************************
         * We are here because sem_post() fails. In this case we can not
         * cleanly unwind the TX queue to recover, so we simply log a
         * fatal error, and return FM_OK.
         **************************************************************/
        FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmGenericSendPacket: "
                     "fmPlatformTriggerInterrupt returned error");

        err = FM_OK;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmGenericSendPacketSwitched */




/*****************************************************************************/
/** fmGenericSendPacket
 * \ingroup intPlatformCommon
 *
 * \desc            Called to add a packet to the TX packet queue.
 *
 * \param[in]       sw is the switch on which to send the packet.
 *
 * \param[in]       info is a pointer to associated information about
 *                  the packet including where it is going.
 *
 * \param[in]       packet is a pointer to a chain of fm_buffer structures
 *                  containing the payload.
 *
 * \param[in]       cpuPort contains the logical port number for the CPU port. 
 *
 * \param[in]       stagTypeA is the SVLAN tag type A.
 *
 * \param[in]       stagTypeB is the SVLAN tag type B.
 *
 * \param[in]       switchPriority is the switch priority.
 *
 * \param[in]       trapGlort is the source glort to use if not specified in
 *                  info.
 *
 * \param[in]       suppressVlanTagAllowed is the flag to indicate whether
 *                  to suppress the vlan tag in the data is allowed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TX_PACKET_QUEUE_FULL if transmit packet queue is full.
 * \return          FM_FAIL if network device is not operational.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmGenericSendPacket(fm_int         sw,
                              fm_packetInfo *info,
                              fm_buffer *    packet,
                              fm_int         cpuPort,
                              fm_uint32      stagTypeA,
                              fm_uint32      stagTypeB,
                              fm_uint32      switchPriority,
                              fm_uint32      trapGlort,
                              fm_bool        suppressVlanTagAllowed)
{
    fm_switch *             switchPtr;
    fm_packetQueue *        txQueue;
    fm_int                  masterSw;
    fm_packetEntry *        entry;
    fm_int                  firstPort;
    fm_int                  nextPort;
    fm_int                  state;
    fm_bool                 packetSent = FALSE;
    fm_port *               dPort;
    fm_int                  firstLAGPort;
    fm_bool                 allowDirectSendToCpu = TRUE;
    fm_int                  oldPushIndex;
    fm_packetInfo           tempInfo;
    fm_int                  packetLength;
    fm_int                  cpuMaxFrameSize;
    fm_status               err;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_PKT_TX,
                 "sw = %d, "
                 "info->destMask = 0x%08x, "
                 "info->logicalPort = %d, "
                 "info->sourcePort = %d, "
                 "info->vlanId = %d, "
                 "info->vlanPriority = %d, "
                 "info->switchPriority = %d, "
                 "info->useEgressRules = %s, "
                 "packet->index = 0x%x\n",
                 sw,
                 info->destMask,
                 info->logicalPort,
                 info->sourcePort,
                 info->vlanId,
                 info->vlanPriority,
                 info->switchPriority,
                 FM_BOOLSTRING(info->useEgressRules),
                 packet->index);

    err          = FM_OK;
    packetLength = fmComputeTotalPacketLength(packet);
    switchPtr    = GET_SWITCH_PTR(sw);

    if (packetLength <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmGetPortMaxFrameSizeInt(sw,
                                   cpuPort,
                                   &cpuMaxFrameSize);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);
    }

    if (packetLength > cpuMaxFrameSize - 4)
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_ERR_FRAME_TOO_LARGE);
    }

    if (fmRootApi->isSwitchFibmSlave[sw])
    {
        masterSw  = fmFibmSlaveGetMasterSwitch(sw);
        if (masterSw < 0)
        {
            /* In standalone NIC mode, the 
             * master is also the same as the slave.
             */
            masterSw = sw;
        }
    }
    else
    {
        masterSw = sw;
    }

    if (!fmIsRawPacketSocketDeviceOperational(masterSw))
    {
        FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, FM_FAIL);

    }

    txQueue = &GET_PLAT_PKT_STATE(masterSw)->txQueue;

    fmPacketQueueLock(txQueue);

    /***********************************************************
     * oldSendPushIndex records the current push index in the TX
     * queue. We use it to keep tab on where we started upon
     * entering this function, in case of the need for roll back
     * on the push index when the tx queue is full or if we
     * encounter an error condition after having enqued some
     * entries in the tx queue;
     **********************************************************/
    oldPushIndex = txQueue->pushIndex;

    if (info->logicalPort != FM_DIRECT_VLAN_SEND)
    {
        /* direct send to a single destination port, or normal send */
        if ( (info->logicalPort != FM_LOG_PORT_USE_FTYPE_NORMAL)
            || info->directSendToCpu )
        {
            /*********************************************************
             * In the case of direct sending to a physical port,
             * check the port state, only proceed if the port is up.
             * If the info->logicalPort is a LAG we proceed if the
             * LAG has at least one member.
             * Note we do not check the state of the member ports.
             * In case the frame is hashed to a down member of the lag
             * the frame will be dropped by the switch.
             *********************************************************/
            dPort = GET_PORT_PTR(sw, info->logicalPort);

            if (dPort->portType == FM_PORT_TYPE_LAG)
            {
                err = fmGetLAGPortFirstExt(sw, info->logicalPort, &firstLAGPort);

                if (err != FM_OK)
                {
                    /* The LAG is empty */
                    err = FM_ERR_INVALID_PORT_STATE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
                }
            }
            else
            {
                /* Solution for a Klocwork issue. It gets confused by our
                 * repeated setting of dPort and assumes that the values
                 * might be different, thus leading to different portTypes.
                 * Setting firstLAGPort will at least keep it from complaining
                 * about an uninitialized variable. */
                firstLAGPort = info->logicalPort;

                /* check the destination port is a physical port */
                if (!info->directSendToCpu)
                {
                    if (! (fmIsCardinalPort(sw, info->logicalPort) ||
                           fmIsRemotePort(sw, info->logicalPort)) )
                    {
                        err = FM_ERR_INVALID_PORT;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
                    }

                    if (!fmIsPortLinkUp(sw, info->logicalPort))
                    {
                        err = FM_ERR_INVALID_PORT_STATE;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
                    }
                }
                else
                {
                    /* direct send to the CPU */
                    allowDirectSendToCpu =
                        fmGetBoolApiProperty(FM_AAK_API_DIRECT_SEND_TO_CPU,
                                             FM_AAD_API_DIRECT_SEND_TO_CPU);

                    if (!allowDirectSendToCpu)
                    {
                        err = FM_ERR_INVALID_PORT;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
                    }
                }
            }
        }

        entry = &txQueue->packetQueueList[txQueue->pushIndex];

        /* copy over packet info */
        FM_MEMCPY_S( &tempInfo, sizeof(tempInfo), info, sizeof(*info) );

        if (tempInfo.sourcePort == 0)
        {
            if (switchPtr->defaultSourcePort < 0)
            {
                tempInfo.zeroSourceGlort = TRUE;
            }
            else
            {
                tempInfo.sourcePort = switchPtr->defaultSourcePort;
            }
        }

        entry->packet = packet;
        entry->length = packetLength;
        entry->fcsVal = FM_USE_DEFAULT_FCS;
        entry->freePacketBuffer = TRUE;

        /* The rest of entry fields are generated here */
        err = switchPtr->GeneratePacketISL(sw,
                                           packet,
                                           &tempInfo,
                                           cpuPort,
                                           switchPriority,
                                           &entry->islTagFormat,
                                           &entry->islTag,
                                           &entry->suppressVlanTag);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmGenericSendPacket: packet queued "
                     "in slot %d, length %d bytes\n",
                     txQueue->pushIndex,
                     entry->length);

        err = fmPacketQueueUpdate(txQueue);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

    }
    else
    {
        /* direct send to an entire vlan */
        err = fmGetVlanPortFirst(sw, info->directSendVlanId, &firstPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

        while (firstPort != -1)
        {
            entry = &txQueue->packetQueueList[txQueue->pushIndex];

            /* copy over packet info to tempInfo so later can build isl */
            FM_MEMCPY_S( &tempInfo, sizeof(tempInfo), info, sizeof(*info) );
            tempInfo.logicalPort = firstPort;

            if (tempInfo.sourcePort == 0)
            {
                if (switchPtr->defaultSourcePort < 0)
                {
                    tempInfo.zeroSourceGlort = TRUE;
                }
                else
                {
                    tempInfo.sourcePort = switchPtr->defaultSourcePort;
                }
            }

            entry->packet = packet;
            entry->length = packetLength;
            entry->fcsVal = FM_USE_DEFAULT_FCS;

            /**********************************************************
             * If any of ports in the vlan info->directSendVlanId
             * is UP and not in Blocking or Disabled state in outVlanId
             * we will flag packetSent as TRUE, so that the packet
             * buffer will not be freed
             *********************************************************/
            if (firstPort != cpuPort)
            {
                if (fmIsPortLinkUp(sw, firstPort))
                {
                    state = FM_PORT_STATE_UP;
                }
                else
                {
                    state = FM_PORT_STATE_DOWN;
                }
                
            }
            else
            {
                /* Special handling of the CPU port as the member of the vlan. */
                allowDirectSendToCpu =
                    fmGetBoolApiProperty(FM_AAK_API_DIRECT_SEND_TO_CPU,
                                         FM_AAD_API_DIRECT_SEND_TO_CPU);

                if (allowDirectSendToCpu)
                {
                    tempInfo.directSendToCpu = TRUE;
                    state                    = FM_PORT_STATE_UP;
                }
                else
                {
                    /***********************************************************
                     * We set the state to FM_PORT_STATE_DOWN so packet will not
                     * be sent to the CPU port.
                     **********************************************************/
                    state = FM_PORT_STATE_DOWN;
                }
            }

            if (state == FM_PORT_STATE_UP)
            {
                /***********************************************
                 *  We only send to ports that are up
                 ***********************************************/
                 FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                             "fmGenericSendPacket: packet queued "
                             "in slot %d, length %d bytes, port %d\n",
                             txQueue->pushIndex,
                             entry->length, firstPort);

                err = fmPacketQueueUpdate(txQueue);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);

                packetSent = TRUE;
            }

            err = fmGetVlanPortNext(sw,
                                    info->directSendVlanId,
                                    firstPort,
                                    &nextPort);

            if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
            }

            err = switchPtr->GeneratePacketISL(sw,
                                               packet,
                                               &tempInfo,
                                               cpuPort,
                                               switchPriority,
                                               &entry->islTagFormat,
                                               &entry->islTag,
                                               &entry->suppressVlanTag);
            if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
            }

            if (nextPort != -1)
            {
                entry->freePacketBuffer = FALSE;
            }
            else
            {
                /*******************************************************
                 * Free the packet buffer if we have sent to all target
                 * ports in the vlan.
                 ******************************************************/
                entry->freePacketBuffer = TRUE;
            }

            firstPort = nextPort;
        }

        if (!packetSent)
        {
            /***************************************************************
             * If none of the ports in the info->directSendVlanId is in the
             * proper state, return an error to the user application.
             * Note at this point no entry has been enqued in the tx queue.
             ***************************************************************/
            err = FM_ERR_INVALID_PORT_STATE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_EVENT_PKT_TX, err);
        }
    }

    fmPacketQueueUnlock(txQueue);

    if (err == FM_OK)
    {
        if (!fmRootPlatform->dmaEnabled)
        {
            /**************************************************
             * We must take a lock before writing
             * intrSendPackets because the lock is used
             * by the API to ensure an atomic read-modify-write
             * to intrSendPackets.
             *
             * The platform lock is used instead of state lock
             * because on FIBM platforms, there is an access
             * to intrSendPackets that must be protected before
             * the switch's locks are even created. 
             **************************************************/
            
            FM_TAKE_PKT_INT_LOCK(sw);
            GET_SWITCH_PTR(masterSw)->intrSendPackets = TRUE;
            FM_DROP_PKT_INT_LOCK(sw);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_PKT_TX,
                     "fmGenericSendPackets: triggering interrupt handler\n");

        /* Handle the following error case cleanly! */
        err = fmPlatformTriggerInterrupt(masterSw, FM_INTERRUPT_SOURCE_API);

        if (err != FM_OK)
        {
            /***************************************************************
             * We are here because sem_post() fails. In this case we can not
             * cleanly unwind the TX queue to recover, so we simply log a
             * fatal error, and return FM_OK.
             **************************************************************/
            FM_LOG_FATAL(FM_LOG_CAT_EVENT_PKT_TX,
                         "fmGenericSendPacket: "
                         "fmPlatformTriggerInterrupt returned error");

            err = FM_OK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

ABORT:
    txQueue->pushIndex = oldPushIndex;
    fmPacketQueueUnlock(txQueue);

    FM_LOG_EXIT(FM_LOG_CAT_EVENT_PKT_TX, err);

}   /* end fmGenericSendPacket */


