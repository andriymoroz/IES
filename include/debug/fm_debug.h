/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug.h
 * Creation Date:   April 27, 2006
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation.
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

#ifndef __FM_FM_DEBUG_H
#define __FM_FM_DEBUG_H


/* Flag value for port argument to fmDbgDumpPortMapV2 indicating that
 * the mapping for all ports should be displayed. */
#define FM_ALL_PORTS    -1

/**************************************************/
/** \ingroup constPortMapDumpTypes
 *  These Port Map Dump Types are the values used 
 *  for the portType argument to ''fmDbgDumpPortMapV2'' 
 *  and indicate how the port argument to that function 
 *  should be interpreted.
 **************************************************/
enum _fm_portMapDumpTypes
{
    /** The port argument specifies a logical port number. */
    FM_PORT_DUMP_TYPE_LOGICAL = 0,

    /** The port argument specifies a physical port number. */
    FM_PORT_DUMP_TYPE_PHYSICAL,

    /** The port argument specifies an EPL number (which will
     *  correspond to more than one logical/physical port). */
    FM_PORT_DUMP_TYPE_EPL,

    /** UNPUBLISHED: For internal use only. */
    FM_PORT_DUMP_TYPE_MAX

};  /* end enum _fm_portMapDumpTypes */




/******************************************************/
/** \ingroup intTypeEnum
 *  Specifies different kinds of logical port mappings
 *  to be used for the mappingType argument of
 *  fmDbgMapLogicalPort. 
 ******************************************************/
typedef enum
{
    /** Map logical port to physical port and physical switch. */
    FM_MAP_LOGICAL_PORT_TO_PHYSICAL_PORT = 0,

    /** Map logical port to fabric port. */
    FM_MAP_LOGICAL_PORT_TO_FABRIC_PORT,

    /** Map logical port to epl-lane. */
    FM_MAP_LOGICAL_PORT_TO_EPL_LANE,

    /** Map logical port to serdes id. */
    FM_MAP_LOGICAL_PORT_TO_SERDES_ID,

    /** Map logical port to serdes address - ring. */
    FM_MAP_LOGICAL_PORT_TO_SERDES_ADDRESS,

    /** Map logical port to epl-absolute lane. */
    FM_MAP_LOGICAL_PORT_TO_EPL_ABS_LANE,

    /** Map logical port to epl-channel. */
    FM_MAP_LOGICAL_PORT_TO_EPL_CHANNEL,

    /** UNPUBLISHED: For internal use only. */
    FM_MAP_LOGICAL_PORT_MAX

}    fm_logPortMappingType;




/*****************************************************************************/
/** \ingroup typeEnum
 *  Identifies individual diagnostic counters. Used as an argument to
 *  ''fmDbgDiagCountGet'' and ''fmDbgDiagCountClear''. Counters are kept for:
 *                                                                      \lb\lb
 *  * Tracking the flow of frames through the API.
 *                                                                      \lb
 *  * MA Table update events.
 *                                                                      \lb
 *  * Various error conditions.
 *****************************************************************************/
typedef enum
{
    /** Incremented when RX request message is handled. */
    FM_CTR_RX_REQ_MSG = 0,

    /** Incremented when API's RX packet array is full. THis condition implies
     * no more packets can be received. */
    FM_CTR_RX_PKT_ARRAY_FULL,

    /** Incremented when LCI RX bit timeout occurs. */
    FM_CTR_RX_LCI_TIMEOUT,

    /** Incremented when a RX packet is allocated from the RX packet array. */
    FM_CTR_RX_PKT_ALLOCS,

    /** Incremented when a RX packet is deallocated from the RX packet array.
     */
    FM_CTR_RX_PKT_FREES,

    /** Incremented when unable to allocate an event to propogate packet RX. */
    FM_CTR_RX_OUT_OF_EVENTS,

    /** Incremented when rx frame too large. */
    FM_CTR_RX_FRAME_TOO_LARGE,

    /** Incremented when whole frame rx'd into pkt. */
    FM_CTR_RX_PKT_COMPLETE,

    /** Incremented when an FIBM message is received. */
    FM_CTR_RX_PKT_FIBM,

    /** Incremented when the user part of driver is dispatched by the kernel. */
    FM_CTR_USER_DISPATCH,

    /** Incremented when the user part ioctl wakes up in error. */
    FM_CTR_USER_IOCTL_ERROR,

    /** Incremented when the user part ioctl wakes up okay. */
    FM_CTR_USER_IOCTL_RETURN,

    /** Incremented when the kernel driver drops a low-priority packet
     *  due to congestion. */
    FM_CTR_RX_KERNEL_PKT_DROPS,

    /** Incremented when the user part receives a pkt. */
    FM_CTR_RX_USER_PKT_SEEN,

    /** Incremented when the user part receives a pkt, but drops due to
     *  LACP. */
    FM_CTR_RX_PKT_DROPS_LACP,

    /** Incremented when the user part receives a pkt, but drops due to
     *  STP. */
    FM_CTR_RX_PKT_DROPS_STP,

    /** Incremented when the user part receives a pkt, but drops due to
     *  security violations. */
    FM_CTR_RX_PKT_DROPS_SECURITY,

    /** Incremented when the user part receives a packet, but drops it
     *  and sends a security violation event in its place.
     *  \chips  FM10000 */
    FM_CTR_RX_PKT_DROPS_SV_EVENT,

    /** Incremented when the user part receives a pkt, but unable to
     *  match to a logical port. */
    FM_CTR_RX_PKT_DROPS_NO_PORT,

    /** Incremented when the user part receives a pkt, but drops due to
     *  lack of event to send up. */
    FM_CTR_RX_PKT_DROPS_NO_EVENT,

    /** Incremented when the user part receives a pkt, but drops due to
     *  zero length. */
    FM_CTR_RX_PKT_DROPS_NO_DATA,

    /** Incremented when the user part receives a BPDU pkt, but the
     *  ingress port is discarding. */
    FM_CTR_RX_PKT_DROPS_BPDU_DISC,

    /** Incremented when the API receives a pkt, but does not forward it. */
    FM_CTR_RX_API_PKT_DROPS,

    /** Incremented when the API receives a pkt, and forwards it. */
    FM_CTR_RX_API_PKT_FWD,

    /** Incremented when the API receives a pkt but the status
     *  words says this packet in in error. */
    FM_CTR_RX_PKT_DROPS_FOR_ERROR,

    /** Incremented when the application receives a rx pkt. */
    FM_CTR_RX_APPL_PKT_SEEN,

    /** Incremented when the application forwards a rx frame. */
    FM_CTR_RX_APPL_FRM_FWD,

    /** Incremented when the application drops a rx frame. */
    FM_CTR_RX_APPL_FRM_DROPS,

    /** Incremented when the application forwards a tx pkt. */
    FM_CTR_TX_APPL_PKT_FWD,

    /** Incremented when tx frame too large. */
    FM_CTR_TX_FRAME_TOO_LARGE,

    /** Incremented when tx pkt array is full (cannot send another). */
    FM_CTR_TX_PKT_ARRAY_FULL,

    /** Incremented when LCI tx interrupt seen. */
    FM_CTR_TX_LCI_INT,

    /** Incremented when tx request msg seen. */
    FM_CTR_TX_REQ_MSG,

    /** Incremented when LCI tx bit timeout occurs. */
    FM_CTR_TX_LCI_TIMEOUT,

    /** Incremented when a whole packet has been sent to the switch for TX. */
    FM_CTR_TX_PKT_COMPLETE,

    /** Incremented when transferring a packet to the switch for TX fails. */
    FM_CTR_TX_PKT_DROP,

    /** Incremented when an EPL interrupt is seen. */
    FM_CTR_EPL_INT,

    /**************************************************
     * MA Table Learn Events
     **************************************************/

    /** Incremented when a LEARNED event causes an address to be learned. */
    FM_CTR_MAC_LEARN_LEARNED,

    /** Incremented when an API call generates an LEARNED event. */
    FM_CTR_MAC_API_LEARNED,

    /** Incremented when a learn event is reported. */
    FM_CTR_MAC_REPORT_LEARN,

    /** Incremented when a learn event is being sent to the ALPS. */
    FM_CTR_MAC_ALPS_LEARN,

    /** Incremented when a learn event is discarded by the event handler. */
    FM_CTR_MAC_LEARN_DISCARDED,

    /**************************************************
     * MA Table Age Events
     **************************************************/

    /** Incremented when a purge request causes an address to be aged.
     *  \chips  FM10000 */
    FM_CTR_MAC_PURGE_AGED,

    /** Incremented when a LEARNED event causes an address to be aged. */
    FM_CTR_MAC_LEARN_AGED,

    /** Incremented when an API call generates an AGED event. */
    FM_CTR_MAC_API_AGED,

    /** Incremented when an age event is reported. */
    FM_CTR_MAC_REPORT_AGE,

    /** Incremented when an age event is being sent to the ALPS. */
    FM_CTR_MAC_ALPS_AGE,

    /**************************************************
     * MA Table Other Events
     **************************************************/

    /** Incremented when a LEARNED event causes a destination port change.
     *  \chips  FM4000, FM10000 */
    FM_CTR_MAC_LEARN_PORT_CHANGED,

    /** Incremented when DeleteAllDynamicAddresses removes an entry from
     *  the software cache. */
    FM_CTR_MAC_CACHE_DELETED,

    /**************************************************
     * MA Table Errors
     **************************************************/

    /** Incremented when a duplicate address is removed from the software
     *  cache.
     *  \chips  FM4000, FM10000 */
    FM_CTR_MAC_CACHE_DUP,

    /** Incremented when a security violation is reported.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_CTR_MAC_SECURITY,

    /** Incremented when an error occurs during an MA table read.
     *  \chips  FM2000, FM4000 */
    FM_CTR_MAC_READ_ERR,

    /** Incremented when an error occurs during an MA table write.
     *  \chips  FM2000, FM4000, FM10000 */
    FM_CTR_MAC_WRITE_ERR,

    /** Incremented when an invalid port is detected.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_CTR_MAC_PORT_ERR,

    /** Incremented when a learn event is discarded due to a vlan boundary
     *  violation.
     *  \chips  FM10000 */
    FM_CTR_MAC_VLAN_ERR,

    /** Incremented when an event buffer cannot be allocated. */
    FM_CTR_MAC_EVENT_ALLOC_ERR,

    /** Incremented when an event buffer cannot be sent. */
    FM_CTR_MAC_EVENT_SEND_ERR,

    /**************************************************
     * MA Table Flush Requests
     **************************************************/

    /** Incremented when ''fmDeleteAllAddresses'' is called. */
    FM_CTR_DELETE_ALL,

    /** Incremented when ''fmDeleteAllDynamicAddresses'' is called. */
    FM_CTR_DELETE_ALL_DYNAMIC,

    /** Incremented when ''fmFlushAllDynamicAddresses'' is called or 
     *  ''fmFlushAddresses'' is called with the ''FM_FLUSH_MODE_ALL_DYNAMIC'' 
     *  mode. */
    FM_CTR_FLUSH_ALL_DYNAMIC,

    /** Incremented when ''fmFlushPortAddresses'' is called or 
     *  ''fmFlushAddresses'' is called with the ''FM_FLUSH_MODE_PORT'' mode. */
    FM_CTR_FLUSH_PORT,

    /** Incremented when ''fmFlushVlanAddresses'' is called or 
     *  ''fmFlushAddresses'' is called with the ''FM_FLUSH_MODE_VLAN'' mode. */
    FM_CTR_FLUSH_VLAN,

    /** Incremented when ''fmFlushPortVlanAddresses'' is called or 
     *  ''fmFlushAddresses'' is called with the ''FM_FLUSH_MODE_PORT_VLAN'' 
     *  mode. */
    FM_CTR_FLUSH_PORT_VLAN,

    /**************************************************
     * MA Table Purge Events
     **************************************************/

    /** Incremented when an MA Table hardware-based purge request is
     *  enqueued within the API. */
    FM_CTR_PURGE_REQ,

    /** Incremented when an MA Table hardware-based purge request is
     *  actually executed.
     *  \chips  FM4000, FM6000 */
    FM_CTR_PURGE_EX,

    /** Incremented when execution of an MA Table hardware-based purge has
     *  completed. */
    FM_CTR_PURGE_COMPLETE,

    /**************************************************
     * TCN FIFO Interrupts
     **************************************************/

    /** Incremented when a TCN interrupt is dispatched. */
    FM_CTR_TCN_INTERRUPT,

    /** Incremented when a TCN PendingEvents condition is detected. */
    FM_CTR_TCN_PENDING_EVENTS,

    /** Incremented when a TCN LearnedEvents overflow is detected.
     *  \chips  FM4000, FM10000 */
    FM_CTR_TCN_LEARNED_OVERFLOW,

    /**************************************************
     * TCN FIFO Events
     **************************************************/

    /** Incremented when a Learned event is removed from the TCN FIFO. */
    FM_CTR_TCN_LEARNED_EVENT,

    /** Incremented when a SecurityViolation_Moved event is removed from
     *  the TCN FIFO.
     *  \chips  FM4000, FM10000 */
    FM_CTR_TCN_SEC_VIOL_MOVED_EVENT,

    /**************************************************
     * TCN FIFO Errors
     **************************************************/

    /** Incremented when a TCN PTR read error is detected. */
    FM_CTR_TCN_PTR_READ_ERR,

    /** Incremented when a TCN PTR write error is detected.
     *  \chips  FM4000, FM6000 */
    FM_CTR_TCN_PTR_WRITE_ERR,

    /** Incremented when a TCN FIFO read error is detected. */
    FM_CTR_TCN_FIFO_READ_ERR,

    /** Incremented when a TCN FIFO parity error is detected.
     *  \chips  FM4000, FM10000 */
    FM_CTR_TCN_FIFO_PARITY_ERR,

    /** Incremented when a TCN FIFO conversion error is detected. */
    FM_CTR_TCN_FIFO_CONV_ERR,

    /**************************************************
     * MAC Maintenance Requests
     **************************************************/

    /** Incremented when the MAC table maintenance handler polls the pending
     *  work list. */
    FM_CTR_MAC_WORK_POLL_COUNT,

    /** The total number of service requests the MAC table maintenance
     *  handler has received. */
    FM_CTR_MAC_WORK_TOTAL_TASKS,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to perform a dynamic address flush. */
    FM_CTR_MAC_WORK_FLUSH_DYN_ADDR,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to service a TCN FIFO overflow. */
    FM_CTR_MAC_WORK_UPD_OVFLW,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to perform a periodic table scan. */
    FM_CTR_MAC_WORK_PERIODIC_SCAN,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to perform a port address flush. */
    FM_CTR_MAC_WORK_PORT_ADDR_FLUSH,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to perform an ACL update. */
    FM_CTR_MAC_WORK_PORT_ACL_UPDATE,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to perform a VLAN address flush. */
    FM_CTR_MAC_WORK_VLAN_ADDR_FLUSH,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to perform a VLAN port address flush. */
    FM_CTR_MAC_WORK_VLAN_PORT_ADDR_FLUSH,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to service the TCN FIFO. */
    FM_CTR_MAC_WORK_SERVICE_FIFO,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to service the purge request queue. */
    FM_CTR_MAC_WORK_HANDLE_PURGE,

    /** Incremented when the MAC table maintenance handler receives a request
     *  to refresh remote glort entries. */
    FM_CTR_MAC_WORK_REFRESH_REMOTE,

    /** Incremented when the MAC table maintenance handler removes an event
     *  from the TCN FIFO. */
    FM_CTR_MAC_WORK_FIFO_EVENTS,

    /**************************************************
     * MAC Security events.
     **************************************************/

    /** Frames with unknown SMACs received on ports whose security action
     *  is ''FM_PORT_SECURITY_ACTION_EVENT''.
     *  \chips  FM10000 */
    FM_CTR_SECURITY_UNKNOWN_SMAC_EVENTS,

    /** Frames with unknown SMACs received on ports whose security action
     *  is ''FM_PORT_SECURITY_ACTION_TRAP''.
     *  \chips  FM10000 */
    FM_CTR_SECURITY_UNKNOWN_SMAC_TRAPS,

    /** Frames with non-secure SMACs moving to a secure port whose
     *  security action is ''FM_PORT_SECURITY_ACTION_EVENT''.
     *  \chips  FM10000 */
    FM_CTR_SECURITY_NON_SECURE_SMAC_EVENTS,

    /** Frames with non-secure SMACs moving to a secure port whose
     *  security action is ''FM_PORT_SECURITY_ACTION_TRAP''.
     *  \chips  FM10000 */
    FM_CTR_SECURITY_NON_SECURE_SMAC_TRAPS,

    /** Frames with secure SMACs moving to any port when the security
     *  action is ''FM_MAC_SECURITY_ACTION_EVENT''.
     *  \chips  FM10000 */
    FM_CTR_SECURITY_SECURE_SMAC_EVENTS,

    /** Frames with secure SMACs moving to any port when the security
     *  action is ''FM_MAC_SECURITY_ACTION_TRAP''.
     *  \chips  FM10000 */
    FM_CTR_SECURITY_SECURE_SMAC_TRAPS,

    /**************************************************
     * Link change status
     **************************************************/

    /** Incremented when a link change event is signaled. */
    FM_CTR_LINK_CHANGE_EVENT,

    /** Incremented when an event cannot be allocated to report link
     *  change. */
    FM_CTR_LINK_CHANGE_OUT_OF_EVENTS,

    /**************************************************
     * Timestamp events
     **************************************************/

    /** Incremented when an egress timestamp event is signaled.
     *  \chips  FM6000, FM10000 */
    FM_CTR_EGRESS_TIMESTAMP_EVENT,

    /** Incremented when an egress timestamp event is lost.
     *  \chips  FM6000, FM10000 */
    FM_CTR_EGRESS_TIMESTAMP_LOST,

    /**************************************************
     * Parity Errors
     **************************************************/

    /** Incremented when an ARRAY memory error is detected.
     *  \chips  FM6000, FM10000 */
    FM_CTR_PARITY_AREA_ARRAY,

    /** Incremented when an INGRESS_XBAR_CTRL or EGRESS_XBAR_CTRL memory
     *  error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_CROSSBAR,

    /** Incremented when an EPL or SERDES memory error is detected..
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_EPL,

    /** Incremented when an FH_HEAD memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_FH_HEAD,

    /** Incremented when an FH_TAIL memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_FH_TAIL,

    /** Incremented when an uncorrectable memory error is detected in the
     *  scheduler FREELIST.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_FREELIST,

    /** Incremented when a MANAGEMENT memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_MGMT,

    /** Incremented when a MODIFY memory error is detected.
     *  \chips  FM6000, FM10000 */
    FM_CTR_PARITY_AREA_MODIFY,

    /** Incremented when a PARSER memory error is detected.
     *  \chips  FM6000 */
    FM_CTR_PARITY_AREA_PARSER,

    /** Incremented when a POLICER memory error is detected.
     *  \chips  FM6000 */
    FM_CTR_PARITY_AREA_POLICER,

    /** Incremented when an uncorrectable POLICER memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_POLICER_U_ERR,

    /** Incremented when a correctable POLICER memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_POLICER_C_ERR,

    /** Incremented when an uncorrectable STATS memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_STATS_U_ERR,

    /** Incremented when a correctable STATS memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_STATS_C_ERR,

    /** Incremented when an uncorrectable memory error is detected in the
     *  modify REFCOUNT.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_REFCOUNT,

    /** Incremented when a SCHEDULER memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_SCHEDULER,

    /** Incremented when a TCAM checksum error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_TCAM,

    /** Incremented when a Tunneling Engine memory error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_AREA_TUNNEL_ENGINE,

    /** Incremented when an UNDEFINED memory error is detected.
     *  \chips  FM6000 */
    FM_CTR_PARITY_AREA_UNDEFINED,

    /**************************************************
     * Parity Error severity
     **************************************************/

    /** Incremented when a TRANSIENT parity error is detected.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_CTR_PARITY_SEVERITY_TRANSIENT,

    /** Incremented when a REPAIRABLE parity error is detected.
     *  \chips  FM10000 */
    FM_CTR_PARITY_SEVERITY_REPAIRABLE,

    /** Incremented when a CUMULATIVE parity error is detected.
     *  \chips  FM4000, FM10000 */
    FM_CTR_PARITY_SEVERITY_CUMULATIVE,

    /** Incremented when a FATAL parity error is detected.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_CTR_PARITY_SEVERITY_FATAL,

    /**************************************************
     * Parity Error status
     **************************************************/

    /** Incremented when a parity error is fixed in software.
     *  \chips  FM6000, FM10000 */
    FM_CTR_PARITY_STATUS_FIXED,

    /** Incremented when the API attempts to fix a parity error, and the
     *  attempt fails.
     *  \chips  FM6000, FM10000 */
    FM_CTR_PARITY_STATUS_FIX_FAILED,

    /** Incremented when a parity error is corrected in hardware.
     *  \chips  FM6000 */
    FM_CTR_PARITY_STATUS_CORRECTED,

    /**************************************************
     * Parity Error events
     **************************************************/

    /** Incremented when a correctable SRAM error interrupt occurs.
     *  \chips  FM6000 */
    FM_CTR_SRAM_C_ERR_INTERRUPT,

    /** Incremented when an uncorrectable SRAM error interrupt occurs.
     *  \chips  FM6000 */
    FM_CTR_SRAM_U_ERR_INTERRUPT,

    /** Incremented when a parity error notification cannot be sent to the
     *  event handler.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_CTR_PARITY_EVENT_LOST,

    /**************************************************
     * Parity Repair events
     **************************************************/

    /** Incremented when the switch-specific parity repair task is
     *  dispatched.
     *  \chips  FM10000 */
    FM_CTR_PARITY_REPAIR_DISPATCH,

    /** Incremented when the repair type is invalid.
     *  \chips  FM10000 */
    FM_CTR_PARITY_REPAIR_INVALID,

    /**************************************************
     * Priority Buffer Queue events
     **************************************************/

    /** Incremented when buffer is added to buffer queue by allocating
     *  a node. */
    FM_CTR_BUFFERQ_NODE_ALLOCS,
    
    /** Incremented when a node is removed from buffer queue. */
    FM_CTR_BUFFERQ_NODE_FREES,

    /** Incremented when low priority packet buffer is flushed to provide space
     *  for higher priority packet. */
    FM_CTR_LOW_PRI_PKT_FLUSHED,

    /** Incremented when event corresponding to flushed low priority packet 
     *  buffer is released. */
    FM_CTR_LOW_PRI_PKT_EVENT_FLUSHED,

    /** Incremented when event corresponding to low priority packet found is 
     *  already removed from thread's queue. */
    FM_CTR_LOW_PRI_PKT_EVENT_NOT_IN_Q,

    /** Incremented when there is no low priority packet found for
     *  flushing. */
    FM_CTR_NO_LOW_PRI_PKTS_FOUND,

    /** Incremented when local event for low priority packet to be flushed
     *  is not delivered to local handler. */
    FM_CTR_LOW_PRI_PKT_EVENT_NOT_DELIVERED,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: Number of entries in the switch counter array. */
    FM_SWITCH_CTR_MAX

} fm_trackingCounterIndex;

/* Out-of-band value to indicate that the counter index is undefined. */
#define FM_CTR_INVALID  ((fm_trackingCounterIndex) -1)


/** \ingroup macroSynonym
 *  (Deprecated) Legacy synonym for ''FM_CTR_DELETE_ALL''. */
#define FM_CTR_ALL_DEL                  FM_CTR_DELETE_ALL

/** \ingroup macroSynonym
 *  (Deprecated) Legacy synonym for ''FM_CTR_DELETE_ALL_DYNAMIC''. */
#define FM_CTR_DYN_DEL                  FM_CTR_DELETE_ALL_DYNAMIC

/** \ingroup macroSynonym
 *  (Deprecated) Legacy synonym for ''FM_CTR_FLUSH_ALL_DYNAMIC''. */
#define FM_CTR_DYN_FLUSH                FM_CTR_FLUSH_ALL_DYNAMIC

/** \ingroup macroSynonym
 *  (Deprecated) Legacy synonym for ''FM_CTR_FLUSH_PORT''. */
#define FM_CTR_PORT_FLUSH               FM_CTR_FLUSH_PORT

/** \ingroup macroSynonym
 *  (Deprecated) Legacy synonym for ''FM_CTR_FLUSH_VLAN''. */
#define FM_CTR_VLAN_FLUSH               FM_CTR_FLUSH_VLAN

/** \ingroup macroSynonym
 *  (Deprecated) Legacy synonym for ''FM_CTR_FLUSH_PORT_VLAN''. */
#define FM_CTR_VLAN_PORT_FLUSH          FM_CTR_FLUSH_PORT_VLAN



/*****************************************************************************/
/** \ingroup typeEnum
 * A global set of diagnostic counters are kept for:
 *  * Tracking events
 *  * Buffer allocations and frees
 * The counters are stored in an array.  These enumerated values serve as the
 * indexes into the counter array for each type of event being counted.
 *****************************************************************************/
typedef enum
{
    /** Number of frame chunks/buffers allocated. */
    FM_GLOBAL_CTR_BUFFER_ALLOCS = 0,

    /** Number of frame chunks/buffers freed. */
    FM_GLOBAL_CTR_BUFFER_FREES,

    /** Incremented when a buffer is requested but
     * there are no free buffers remaining.
     */
    FM_GLOBAL_CTR_OUT_OF_BUFFERS,

    /** Incremented when a buffer is used for packet TX. */
    FM_GLOBAL_CTR_TX_BUFFER_ALLOCS,

    /** Incremented when a buffer used for TX is freed. */
    FM_GLOBAL_CTR_TX_BUFFER_FREES,

    /** Incremented when a buffer is used for packet RX. */
    FM_GLOBAL_CTR_RX_BUFFER_ALLOCS,

    /** Incremented when a buffer used for RX is freed. */
    FM_GLOBAL_CTR_RX_BUFFER_FREES,

    /** Incremented when a buffer is requested for packet RX but the
     * remaining buffers are reserved for TX.
     */
    FM_GLOBAL_CTR_NO_BUFFERS_FOR_RX,

    /** Incremented when a buffer is requested for packet RX but
     * there are no free buffers remaining.
     */
    FM_GLOBAL_CTR_RX_OUT_OF_BUFFERS,

    /** Incremented when a request for an event could not be satisfied. */
    FM_GLOBAL_CTR_NO_EVENTS_AVAILABLE,

    /** Incremented when a buffer queue node is freed. */
    FM_GLOBAL_CTR_BUFFER_QUEUE_NODE_FREES,

    /* ----  Add new entries above this line.  ---- */

    /** Used internally as the length of the global counter array. */
    FM_GLOBAL_CTR_MAX

} fm_globalDiagCounter;


/* Diagnostics */
typedef struct
{
    fm_uint64 counters[FM_SWITCH_CTR_MAX];

} fm_switchDiagnostics;


typedef struct
{
    fm_uint64 counters[FM_GLOBAL_CTR_MAX];

} fm_globalDiagnostics;


/**************************************************/
/** \ingroup typeStruct
 *  A single Ethernet port eye diagram sample, used
 *  as an argument in ''fmDbgGetEyeDiagramSampleFirst'',
 *  ''fmDbgGetEyeDiagramSampleNext'' and
 *  ''fmDbgGetEyeDiagramSampleList''.
 **************************************************/
typedef struct _fm_eyeDiagramSample
{
    /** Generic ID for this sample. */
    fm_int sampleId;

    /** Phase point. */
    fm_int   phase;

    /** Voltage offset. */
    fm_int   offset;

    /** Bit error rate. */
    fm_float ber;

    /** Absolute number of errors. */
    fm_uint64 errors;

} fm_eyeDiagramSample;



/***************************************************
 * All public debug functions.
 **************************************************/

/* Initializes the debug sub-system. */
fm_status fmDbgInitialize(void);

/* Switch self-test */
fm_status fmDbgSwitchSelfTest(fm_int sw, fm_bool ctrlState);
fm_status fmDbgPolicerTest(fm_int  sw,
                           fm_bool ctrlState,
                           fm_int *portList,
                           fm_int  portCnt,
                           fm_bool mrlLimiter);

/* Trace buffer management functions */
fm_status fmDbgTraceDump(fm_int start, fm_int end, fm_int stop);
fm_status fmDbgTracePost(fm_int eventCode,
                         fm_uint32 data1,
                         fm_uint32 data2,
                         fm_uint32 data3);
fm_status fmDbgTraceClear(void);
fm_status fmDbgTraceMode(fm_int mode, fm_int tail);
fm_status fmDbgTraceTrigger(fm_int eventCode, fm_int addOrDelete);
fm_status fmDbgTraceExclude(fm_int eventCode, fm_int addOrDelete);
void fmDbgTraceHelp(void);
void fmDbgTraceStatus(void);

/* Logical port management helpers */
void fmDbgDumpPortMap(int sw);
fm_status fmDbgDumpPortMapV2(fm_int sw, fm_int port, fm_int portType);
fm_status fmDbgMapLogicalPortToPhysical(fm_int  sw,
                                        fm_int  logPort,
                                        fm_int *physPort);

fm_status fmDbgMapLogicalPort(fm_int                 sw,
                              fm_int                 logPort,
                              fm_int                 lane,
                              fm_logPortMappingType  mappingType,
                              fm_int                *pMapped);

fm_int fmDbgMapPhysicalPortToLogical(fm_int switchNum, fm_int physPort);

/* Register monitoring facility */
fm_status fmDbgMonitorRegister(fm_int    sw,
                                      fm_uint32 regOffset,
                                      fm_bool   monitor);
void fmDbgRegisterUpdate(fm_int    sw,
                                fm_uint32 regOffset,
                                fm_uint32 regValue);

/* Memory and buffer management */
fm_status fmDbgBfrDump(fm_int sw);
fm_status fmDbgDumpDeviceMemoryStats(int sw);


/* Register dump functionality */
void fmDbgDumpRegister(fm_int sw, fm_int port, char *regname);
fm_status fmDbgDumpRegisterV2(fm_int  sw,
                              fm_int  indexA,
                              fm_int  indexB,
                              fm_text regname);
fm_status fmDbgDumpRegisterV3(fm_int  sw,
                              fm_int  indexA,
                              fm_int  indexB,
                              fm_int  indexC,
                              fm_text regname);
void fmDbgListRegisters(fm_int  sw,
                        fm_bool showGlobals,
                        fm_bool showPorts);
void fmDbgGetRegisterName(fm_int   sw,
                          fm_int   regId,
                          fm_uint  regAddress,
                          fm_text  regName,
                          fm_uint  regNameLength,
                          fm_bool *isPort,
                          fm_int * index0Ptr,
                          fm_int * index1Ptr,
                          fm_int * index2Ptr,
                          fm_bool  logicalPorts,
                          fm_bool  partialLongRegs);
void fmDbgReadRegister(fm_int  sw,
                       fm_int  firstIndex,
                       fm_int  secondIndex,
                       fm_text registerName,
                       void *  values);
void fmDbgWriteRegister(fm_int  sw,
                        fm_int  port,
                        fm_text regname,
                        fm_int  value);
fm_status fmDbgWriteRegisterV2(fm_int    sw,
                               fm_int    wordOffset,
                               fm_int    indexA,
                               fm_int    indexB,
                               fm_text   regName,
                               fm_uint32 value);
fm_status fmDbgWriteRegisterV3(fm_int    sw,
                               fm_int    wordOffset,
                               fm_int    indexA,
                               fm_int    indexB,
                               fm_int    indexC,
                               fm_text   regName,
                               fm_uint32 value);
fm_status fmDbgWriteRegisterField(fm_int    sw,
                                  fm_int    indexA,
                                  fm_int    indexB,
                                  fm_int    indexC,
                                  fm_text   regName,
                                  fm_text   fieldName,
                                  fm_uint64 value);
void fmDbgWriteRegisterBits(fm_int    sw,
                            fm_uint   reg,
                            fm_uint32 mask,
                            fm_uint32 value);
fm_status fmDbgVerifyRegisterCache(fm_int sw);

/* Packet helpers */
void fmPrettyPrintPacket(fm_buffer *pkt);
void fmDbgPacketSizeDistAdd(int ps);

/* Chip snapshot debugging functions */
void fmDbgTakeChipSnapshot(fm_int sw, fm_int snapshot);
void fmDbgDeleteChipSnapshot(fm_int snapshot);
void fmDbgPrintChipSnapshot(fm_int snapshot, fm_bool showZeroValues);
void fmDbgCompareChipSnapshots(fm_uint snapshotMask);

/* Timer management */
void fmDbgTimerDump(void);
void fmDbgTimerReset(int index);
void fmDbgTimerBeginSample(int index);
void fmDbgTimerEndSample(int index);

/***************************************************
 * Module specific debug functions
 **************************************************/

fm_status fmDbgDumpLinkDebounceInfo(fm_int sw);
fm_status fmDbgDumpPortMasks(int sw);
fm_status fmDbgDumpLag(int sw);
fm_status fmDbgDumpLinkUpMask(fm_int sw);
void fmDbgDumpThreads(void);
void fmDbgDumpPort(fm_int sw, fm_int port);
void fmDbgDumpMACTable(fm_int sw, fm_int numEntries);
void fmDbgDumpMACCache(fm_int sw, fm_int numEntries);
void fmDbgDumpMACTableEntry(fm_int sw, fm_uint16 vlan, char *addressStr);
void fmDbgTraceMACAddress(fm_macaddr macAddr);
fm_status fmDbgDumpVid(fm_int sw);
fm_status fmDbgDumpMirror(fm_int sw);
fm_status fmDbgDumpMirrorProfile(fm_int sw);
void fmDbgDumpMapper(fm_int sw);
void fmDbgDumpFFU(fm_int sw, fm_bool validSlicesOnly, fm_bool validRulesOnly);
void fmDbgDumpBstTable(fm_int sw);
void fmDbgDumpSBusRegister( fm_int  sw, 
                            fm_int  sbusDevID, 
                            fm_int  devRegID,
                            fm_bool writeReg );
fm_status fmDbgReadSBusRegister( fm_int     sw, 
                                 fm_int     sbusDevID, 
                                 fm_int     devRegID, 
                                 fm_bool    writeReg,
                                 fm_uint32 *value );
fm_status fmDbgWriteSBusRegister( fm_int     sw, 
                                  fm_int     sbusDevID, 
                                  fm_int     devRegID, 
                                  fm_uint32  value );
void fmDbgDumpEthSerDesRegister( fm_int  sw, 
                                 fm_int  port, 
                                 fm_int  devRegID,
                                 fm_bool writeReg );
fm_status fmDbgReadEthSerDesRegister( fm_int     sw, 
                                      fm_int     port, 
                                      fm_int     devRegID, 
                                      fm_bool    writeReg,
                                      fm_uint32 *value );
fm_status fmDbgWriteEthSerDesRegister( fm_int     sw, 
                                       fm_int     port, 
                                       fm_int     devRegID, 
                                       fm_uint32  value );
fm_status fmDbgInterruptSpico( fm_int      sw,
                               fm_int      cmd,
                               fm_int      arg,
                               fm_int      timeout,
                               fm_uint32 * result );
fm_status fmDbgInitSerDes(fm_int sw,
                          fm_int serDes,
                          fm_uint dataWidth,
                          fm_uint rateSel);
fm_status fmDbgRunSerDesDfeTuning(fm_int        sw,
                                  fm_int        serDes,
                                  fm_dfeMode    dfeMode,
                                  fm_int        dfeHf,
                                  fm_int        dfeLf,
                                  fm_int        dfeDc,
                                  fm_int        dfeBw);
fm_status fmDbgReadSerDesRegister(fm_int     sw, 
                                  fm_int     serDes,
                                  fm_int     reg, 
                                  fm_uint32 *value);
fm_status fmDbgWriteSerDesRegister(fm_int     sw, 
                                   fm_int     serDes, 
                                   fm_uint    reg, 
                                   fm_uint32  value);
fm_status fmDbgDumpSerDes(fm_int  sw, 
                          fm_int  serDes,
                          fm_text cmd);
fm_status fmDbgSetSerDesTxPattern(fm_int  sw,
                                  fm_int  serDes,
                                  fm_text pattern);
fm_status fmDbgSetSerDesRxPattern(fm_int  sw,
                                  fm_int  serDes,
                                  fm_text pattern);
fm_status fmDbgResetSerDesErrorCounter(fm_int sw, 
                                       fm_int serDes);
fm_status fmDbgGetSerDesErrorCounter(fm_int     sw,
                                     fm_int     serDes,
                                     fm_uint32 *pCounter);
fm_status fmDbgInjectSerDesErrors(fm_int sw,
                                  fm_int serDes,
                                  fm_int serDesDir, 
                                  fm_uint numErrors);
fm_status fmDbgSetSerDesCursor(fm_int     sw,
                               fm_int     serDes,
                               fm_int     cursor);
fm_status fmDbgSetSerDesPreCursor(fm_int    sw,
                                  fm_int    serDes,
                                  fm_int    preCursor);
fm_status fmDbgSetSerDesPostCursor(fm_int    sw,
                                   fm_int    serDes,
                                   fm_int    postCursor);
fm_status fmDbgSetSerDesPolarity(fm_int  sw, 
                                 fm_int  serDes,
                                 fm_text polarityStr);
fm_status fmDbgSetSerDesLoopback(fm_int  sw, 
                                fm_int  serDes,
                                fm_text loopbackStr);
fm_status fmDbgSetSerDesDfeParameter(fm_int     sw,
                                     fm_int     serDes,
                                     fm_uint32  paramSelector,
                                     fm_uint32  paramValue);
fm_status fmDbgGetSerDesDfeParameter(fm_int     sw,
                                     fm_int     serDes,
                                     fm_uint32  paramSelector,
                                     fm_uint32 *pParamValue);

fm_status fmDbgDumpSerDesDfeParameter(fm_int     sw,
                                      fm_int     serDes,
                                      fm_uint32  paramSelector);
void fmDbgDumpL2LSweepers(fm_int sw, fm_bool regs);
void fmDbgDumpSAFTable(fm_int sw);

void fmDbgInitSwitchRegisterTable(fm_int sw);

void fmDbgDumpVersion(void);
const char *fmDbgGetVersion(void);

fm_status fmDbgDumpTriggers(fm_int sw);

fm_status fmDbgDumpTriggerUsage(fm_int sw);

fm_status fmDbgDumpMulticastTables(fm_int sw);

fm_status fmDbgDumpGlortTable(fm_int sw);

fm_status fmDbgDumpGlortDestTable(fm_int sw, fm_bool raw);

void fmDbgConvertMacAddressToString(fm_macaddr macAddr, fm_char *textOut);
fm_bool fmDbgConvertStringToMacAddress(const char *addrStr, 
                                       fm_macaddr *addrValue); 

void fmDbgDumpACLsAsC(fm_int sw, const char *fileName);


/* Diagnostic counters API.
 * Switch counters:
 */
fm_status fmDbgDiagCountDump(fm_int sw);
fm_status fmDbgDiagCountClear(fm_int sw, fm_trackingCounterIndex counter);
fm_status fmDbgDiagCountClearAll(fm_int sw);
fm_status fmDbgDiagCountGet(fm_int sw, fm_trackingCounterIndex counter, fm_uint64 *outValue);
fm_status fmDbgDiagCountSet(fm_int sw, fm_trackingCounterIndex counter, fm_uint64 value);
fm_status fmDbgDiagCountIncr(fm_int                  sw,
                             fm_trackingCounterIndex counter,
                             fm_uint64               amount);

fm_status fmDbgDumpDriverCounts(fm_int sw);
fm_status fmDbgDumpMATableCounts(fm_int sw);
fm_status fmDbgDumpMacSecurityCounts(fm_int sw);
fm_status fmDbgDumpParityErrorCounts(fm_int sw);
fm_status fmDbgDumpParityRepairCounts(fm_int sw);
fm_status fmDbgDumpParitySweeperCounts(fm_int sw);


/* Global diagnostic counters
 */
fm_status fmDbgGlobalDiagCountDump(void);
fm_status fmDbgGlobalDiagCountClear(fm_globalDiagCounter counter);
fm_status fmDbgGlobalDiagCountClearAll(void);
fm_status fmDbgGlobalDiagCountGet(fm_globalDiagCounter counter, fm_uint64 *outValue);
fm_status fmDbgGlobalDiagCountSet(fm_globalDiagCounter counter, fm_uint64 value);
fm_status fmDbgGlobalDiagCountIncr(fm_globalDiagCounter counter, fm_uint64 amount);


/* Event queue debug API
 */
void fmDbgEventQueueCreated(fm_eventQueue *inQueue);
void fmDbgEventQueueDestroyed(fm_eventQueue *inQueue);
void fmDbgEventQueueEventPopped(fm_eventQueue *inQueue, fm_event *event);
void fmDbgEventQueueDump(void);


/* Switch insertion/removal debug API
 */
void fmDbgGenerateSwitchInsertionEvent(fm_int model, fm_int slot);
void fmDbgGenerateSwitchRemovalEvent(fm_int slot);


/* some helper macros */
#define FM_THREAD_ERR_CHECK(e)                       \
    if ( (e) != FM_OK )                              \
    {                                                \
        FM_LOG_ERROR( FM_LOG_CAT_ALOS_THREAD,        \
                     "%s: %s: line %d: error: %s\n", \
                     thread->name,                   \
                     __FILE__,                       \
                     __LINE__,                       \
                     fmErrorMsg( (e) ) );            \
    }

fm_status fmDbgDumpPortMax(fm_int sw, fm_int port);
fm_status fmDbgDumpQOS(fm_int sw, fm_int port);
fm_status fmDbgDumpMemoryUsage(fm_int sw);
fm_status fmDbgDumpMemoryUsageV2(fm_int  sw, 
                                 fm_int  rxPort, 
                                 fm_int  txPort,
                                 fm_int  rxmp, 
                                 fm_int  txmp, 
                                 fm_int  bsg,
                                 fm_bool useSegments);
fm_status fmDbgDumpMemoryUsageV3(fm_int  sw, 
                                 fm_int  rxPort, 
                                 fm_int  txPort,
                                 fm_int  smp, 
                                 fm_int  txTc, 
                                 fm_int  bsg,
                                 fm_bool useSegments);
fm_status fmDbgDumpWatermarks(fm_int sw);
fm_status fmDbgDumpWatermarksV2(fm_int sw, fm_int rxPort, fm_int txPort,
                                fm_int rxmp, fm_int txmp, fm_int islPri);
fm_status fmDbgDumpWatermarksV3(fm_int sw, fm_int rxPort, fm_int txPort,
                                fm_int smp, fm_int txTc, fm_int islPri);
fm_status fmDbgDumpSwpriMap(fm_int sw, fm_int attr);
fm_status fmDbgDumpPortIdxMap(fm_int sw, fm_int port, fm_int attr);
fm_status fmDbgDumpPolicers(fm_int sw);
void fmDbgDumpStatChanges(fm_int sw, fm_bool resetCopy);

/* this is defined outside the conditional below because it is called from the test 
   enviroment using an expert call. The function will be empty if FM_API_FUNCTION_TRACKING is 
   not define.
*/
void fmDbgDumpFuncExecTable(void);


#ifdef FM_API_FUNCTION_TRACKING
#define FM_DBG_TRACK_FUNC()  fmDbgFuncExec(__func__, __LINE__);
#define FM_DBG_NEED_TRACK_FUNC
#define FM_DBG_TRACK_FUNC_DUMP()  fmDbgDumpFuncExecTable();
/* function to track the number of times a function has executed */
void fmDbgFuncExec(const char *funcName, int lineNum);

#else
#define FM_DBG_TRACK_FUNC()
#define FM_DBG_TRACK_FUNC_DUMP()
#ifdef FM_DBG_NEED_TRACK_FUNC
#undef FM_DBG_NEED_TRACK_FUNC
#endif
#endif


/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmDbgDumpRegisterV2''. */
#define fmDbgDumpRegisterExt(sw, indexA, indexB, regname) \
    fmDbgDumpRegisterV2( (sw), (indexA), (indexB), (regname) )

/** A legacy synonym for ''fmDbgWriteRegisterV2''. */
#define fmDbgWriteRegisterExt(sw, wordOffset, indexA, indexB, regName, value) \
    fmDbgWriteRegisterV2( (sw), (wordOffset), (indexA), (indexB), (regName), (value) )

/** @} (end of Doxygen group) */


void fmDbgDrvEventClear(void);
void fmDbgDrvEventDump(void);
void fmDbgDrvEventCount(fm_eventID eventID, int alloc);

fm_status fmDbgPlotEyeDiagram( fm_int eyeDiagramId );
                                        
fm_status fmDbgTakeEyeDiagram( fm_int sw, 
                               fm_int port, 
                               fm_int mac, 
                               fm_int lane, 
                               fm_int eyeDiagramId );

fm_status fmDbgDeleteEyeDiagram( fm_int eyeDiagramId );

fm_status fmDbgGetEyeDiagramSampleFirst( fm_int eyeDiagramId, 
                                         fm_eyeDiagramSample *sample );

fm_status fmDbgGetEyeDiagramSampleList( fm_int  eyeDiagramId,
                                        fm_int *count,
                                        fm_eyeDiagramSample **sample );

fm_status fmDbgGetEyeDiagramSampleNext( fm_int eyeDiagramId, 
                                        fm_eyeDiagramSample *sample );

fm_status fmDbgGetEyeDiagramSampleCount( fm_int eyeDiagramId, 
                                         fm_int *count );

fm_status fmDbgSetPacketDefaultSourcePort(fm_int sw, fm_int sourcePort);

fm_status fmDbgDumpScheduler(fm_int sw);

/* BSM registers debug functions */ 
fm_status fmDbgDumpBsmScratch(fm_int sw, fm_uint32 regMask);
fm_status fmDbgPollBsmStatus(fm_int sw, fm_uint32 miliSec);
fm_status fmDbgPollLtssm(fm_int sw, fm_int pep, fm_uint32 miliSec);

#endif /* __FM_FM_DEBUG_H */
