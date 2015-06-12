/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_priority_buffer_queues.h
 * Creation Date:   Sept. 23, 2013
 * Description:     Priority based packet scheduling to application from switch
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

#ifndef __FM_FM_PRIORITY_BUFFER_QUEUES_H
#define __FM_FM_PRIORITY_BUFFER_QUEUES_H

#define FM_NUM_SWPRI 16

typedef enum
{
    FM_PKT_SCHEDULE_STRICT_PRI = 0,

    FM_PKT_SCHEDULE_WRR

} fm_pktScheduleType; 

/*****************************************************************************
 * Stores the mapping from SWPRI to internal Receive Buffer Priority and each 
 * queues weight for Weighted Round Robin(WRR) 
 *****************************************************************************/
typedef struct
{
    /* Mapping of SwPri to corresponding Buffer Queue indexed by SwPri */
    fm_int bufferQueueNum[FM_NUM_SWPRI];

    /* Weightage for WRR scheduling in the range 0.0 - 1.0. Indexed by SwPri */
    fm_float weight[FM_NUM_SWPRI];

} fm_swpriToRecvBufferQueueMap;
 
/* Array of buffer queues. Array contains 'numPriorityLevels' elements. */
typedef struct
{
    /* Array of fm_buffer queues.*/
    fm_dlist *bufferQueues;

    /* Number of receive buffer queues */
    fm_int numPriorityLevels;

    /* Table for mapping SWPRI to Buffer Queue */
    fm_swpriToRecvBufferQueueMap swpriToRecvBufferQueueMap;
 
    /* Next buffer to be processed for each queue */
    fm_dlist_node **nextBuffers;
 
    /* Number of pkts to be served in each round of scheduling.
     * It is calculated based on weight */
    fm_int   *weightInNumPkts;
 
    /* Type of scheduling whether Strict priority or Weighted 
     * Round Robin */
    fm_pktScheduleType  pktScheduleType;

    /* Common lock for access to this data structure */
    fm_lock   accessLock;

} fm_bufferQueues;

fm_status fmPlatformInitBufferQueues(fm_swpriToRecvBufferQueueMap *map);
fm_status fmPlatformAddBufferChain(fm_buffer *bufferChainHead,
                                   fm_uint32 swpri,
                                   fm_bool    isDuplicate);
fm_buffer *fmPlatformGetBufferChain();
fm_status fmPlatformFreeLeastPriorityFrame(fm_uint32 swpri);
fm_status fmPlatformFreeBufferQueueNode(fm_eventPktRecv *rcvPktEvent);
fm_status fmPlatformInitScratchBuffer(fm_int numBuffers);
fm_status fmPlatformLoadPriorityMap(fm_text fileName);

#endif /* __FM_FM_PRIORITY_BUFFER_QUEUES_H */

