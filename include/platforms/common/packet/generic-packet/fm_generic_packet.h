/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_generic_packet.h
 * Creation Date:   Jan 5, 2009
 * Description:     Header file for generic packet transfer
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

#ifndef __FM_FM_GENERIC_PACKET_H
#define __FM_FM_GENERIC_PACKET_H

/* Packet Transfer Definnitions */
#define FM_MAX_FDS_NUM                            1024
#define FM_FDS_POLL_TIMEOUT_USEC                  1000

/* holds a packet entry */
typedef struct _fm_packetEntry
{
    /* holds information about where the packet is going */
    fm_packetInfo   info;

    /* holds the data for the packet */
    fm_buffer *     packet;

    /* the total length of the packet */
    fm_uint         length;

    /* FM6000 only. The FCS value to be sent with the packet. */
    fm_uint32       fcsVal;

    /* ISL Ftag format */
    fm_islTagFormat islTagFormat;

    /* ISL Ftag */
    fm_islTag       islTag;

    /* if need to suppress vlantag */
    fm_bool         suppressVlanTag;

    /* This is filled in by the API when useEgressRules is set to true. */
    fm_uint32       egressVlanTag;

    /*  This is used to indicate whether or not to free
     *  the packet buffer. For direct sending to a single port this will be
     *  set to TRUE. For direct sending to an entire vlan or to a group
     *  of ports, the last packet entry will have this field set to TRUE. */
    fm_bool         freePacketBuffer;

} fm_packetEntry;


typedef struct _fm_packetQueue
{
    /* holds the current send queue of packets in a circular buffer */
    fm_packetEntry packetQueueList[FM_PACKET_QUEUE_SIZE];

    /**************************************************
     * The packetQueue is a rotary queue of packets
     * intended to be transmitted.  We maintain an
     * index to the next position to push into,
     * and an index to the next position to pull
     * from.
     *
     * The queue is empty when the push index is equal
     * to the pull index (push == pull => empty).
     *
     * The queue is full when incrementing the push
     * index would make it equal to the pull index
     * (push + 1 % QueueSize == pull => full).
     **************************************************/
    fm_uint        pushIndex;
    fm_uint        pullIndex;

    fm_int         switchNum;

    pthread_mutex_t mutex;

} fm_packetQueue;


/* manages packet sending state */
typedef struct
{
    /**************************************************
     * Packet sending state.
     **************************************************/
    fm_packetQueue txQueue;

    /* To signal receive thread to continue when events
     * are available again
     */
    fm_semaphore   eventsAvailableSignal;

    /**************************************************
     * This control whether packets are directly
     * enqueued to the application, bypassing
     * sending to event thread.
     * Note: no support for FM2000 yet
     **************************************************/
    fm_bool        rxDirectEnqueueing;

    /**************************************************
     * This maintains an offset into current packet,
     * with -2 and -1 referring to the first and second
     * pre-data header words.
     * Note: only used for FM2000
     **************************************************/
    fm_int         sendOffset;

    /* This maintains an offset into current buffer. */
    fm_int         sendBufferOffset;

    /**************************************************
     * This maintains a pointer to the current active
     * buffer for the current packet. NULL if none.
     **************************************************/
    fm_buffer *    currentSendBuffer;

    /**************************************************
     * Whether we should include the user-supplied FCS 
     * when sending a packet.
     **************************************************/
    fm_bool        sendUserFcs;

    /**************************************************
     * Packet receive state.
     **************************************************/

    /* This maintains an offset into current buffer. */
    fm_int         recvBufferOffset;

    /**************************************************
     * This maintains a pointer to the current active
     * buffer for the current packet.
     **************************************************/
    fm_buffer *    currentRecvBuffer;

    /* Pointer to first buffer in receive buffer chain. */
    fm_buffer *    recvChainHead;

    /* Pointer to the last buffer in receive buffer chain. */
    fm_buffer *    recvChainTail;

    /* Whether a recv is in progress right now */
    fm_bool        recvInProgress;

    /**************************************************
     * This maintains a pointer to the current active
     * event pointer for the current packet.
     * Note: only for FM2000
     **************************************************/
    fm_event *     currentRecvEvent;

    /* Keeps track of the number of words read out of the fifo */
    fm_int         currentWordsReceived;

    /*********************************************************
     * the software cache of the LCI_CFG.endinaness so we don't
     * have to read LCI_CFG more than once after its written to
     **********************************************************/
    fm_int         cachedEndianness;

} fm_packetHandlingState;


/* Defines any side band data that needs to be passed along with the
 * packet transfer functions */
typedef struct _fm_pktSideBandData
{
    /* The raw ingress timestamp */
    fm_uint64            rawTimeStamp;

    /* Wallclock ingress timestamp. */
    fm_timespec          ingressTimestamp;

} fm_pktSideBandData;



fm_status fmPacketQueueInit(fm_packetQueue *queue, fm_int sw);
fm_status fmPacketQueueFree(fm_int sw);
void      fmPacketQueueLock(fm_packetQueue *queue);
void      fmPacketQueueUnlock(fm_packetQueue *queue);
fm_status fmPacketQueueUpdate(fm_packetQueue *queue);

fm_status fmPacketQueueEnqueue(fm_packetQueue * queue,
                               fm_buffer *      packet,
                               fm_int           packetLength,
                               fm_islTag *      islTag,
                               fm_islTagFormat  islTagFormat,
                               fm_bool          suppressVlanTag,
                               fm_bool          freeBuffer);

fm_status fmPacketReceiveEnqueue(fm_int sw, fm_event *event,
                                 fm_switchEventHandler selfTestEventHandler);

fm_int    fmComputeTotalPacketLength(fm_buffer *packet);
fm_uint32 fmPacketGetCRC(fm_buffer *buffer);
void fmPacketClearCRC(fm_buffer *buffer);
fm_status fmFindSlaveSwitchPortByGlort(fm_uint32 glort, 
                                       fm_int *switchNum, 
                                       fm_int *port);
fm_status fmGetPortDefVlanInt(fm_int     sw,
                              fm_int     port,
                              fm_uint16 *vlan);
fm_status fmGetPortDefVlanDefPriorityInt(fm_int     sw,
                                         fm_int     port,
                                         fm_uint16 *vlan,
                                         fm_byte *  priority);

fm_status fmGenericPacketHandlingInitialize(fm_int sw);
fm_status fmGenericPacketHandlingInitializeV2(fm_int sw, fm_bool hasFcs);
fm_status fmGenericPacketDestroy(fm_int sw);

fm_status fmGenericSendPacketISL(fm_int          sw,
                                 fm_islTag *     islTagList,
                                 fm_islTagFormat islTagFormat,
                                 fm_int          numPorts,
                                 fm_buffer *     packet);

fm_status fmGenericSendPacketDirected(fm_int     sw,
                                      fm_int *   portList,
                                      fm_int     numPorts,
                                      fm_buffer *packet,
                                      fm_uint32  fcsValue,
                                      fm_int     cpuPort,
                                      fm_uint32  switchPriority);

fm_status fmGenericSendPacketSwitched(fm_int     sw,
                                      fm_buffer *packet,
                                      fm_int     cpuPort,
                                      fm_uint32  switchPriority);

fm_status fmGenericSendPacket(fm_int         sw,
                              fm_packetInfo *info,
                              fm_buffer *    packet,
                              fm_int         cpuPort,
                              fm_uint32      stagTypeA,
                              fm_uint32      stagTypeB,
                              fm_uint32      switchPriority,
                              fm_uint32      trapGlort,
                              fm_bool        suppressVlanTagAllowed);

#endif /* __FM_FM_GENERIC_PACKET_H */
