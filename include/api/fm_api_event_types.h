/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_types.h
 * Creation Date:   September 9, 2005
 * Description:     Constants and structures for API level events
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#ifndef __FM_FM_API_EVENT_TYPES_H
#define __FM_FM_API_EVENT_TYPES_H


/**************************************************/
/** \ingroup intTypeStruct
 * Structure carried in an fm_interrupt structure
 * to report EPL (port) interrupt conditions.
 **************************************************/
typedef struct _fm_eplInt
{
    /** Bit mask indicating one or more interrupt sources on a port
     *  are active. This member shadows the EPL_INT_DETECT register
     *  (see FM2000 datasheet) for the port. */
    fm_uint32 interruptDetect;

    /** Bit mask indicating one or more SERDES interrupt sources on a port
     *  are active. This member shadows the SERDES_IP register (see FM2000
     *  datasheet) for the port. */
    fm_uint32 serdes;

    /** Bit mask indicating one or more PCS interrupt sources on a port
     *  are active. This member shadows the PCS_IP register (see FM2000
     *  datasheet) for the port. */
    fm_uint32 pcs;

    /** Bit mask indicating one or more MAC interrupt sources on a port
     *  are active. This member shadows the MAC_IP register (see FM2000
     *  datasheet) for the port. */
    fm_uint32 mac;

} fm_eplInt;


/**************************************************/
/* FM10000-specific EPL interrupt information.
 **************************************************/
typedef struct _fm10000_eplIntr
{
    /** Contents of the AnPortInterrupt field. */
    fm_uint32   an[4];

    /** Contents of the LinkPortInterrupt field. */
    fm_uint32   link[4];

    /** Contents of the SerdesInterrupt field. */
    fm_uint32   serdes[4];

} fm10000_eplIntr;

/**************************************************/
/** \ingroup intTypeStruct
 * Structure used to report FM10000-specific
 * interrupt information.
 **************************************************/
typedef struct _fm10000_interrupt
{

    /** EPL interrupt sources.  */
    fm10000_eplIntr epl[9];

    /** PCIE interrupt sources. Shadows the PCIE_IP registers. */
    fm_uint32   pcie[9];

    /** MA Table interrupt sources. Shadows the MA_TCN_IP register. */
    fm_uint32   ma_tcn;

} fm10000_interrupt;


/**************************************************/
/* Wraps the device-specific interrupt structures
 * together.
 **************************************************/
typedef union _fm_deviceSpecificInterrupts
{
    fm10000_interrupt fm10000;

} fm_deviceSpecificInterrupts;


/**************************************************/
/** \ingroup intTypeStruct
 * Structure used to report interrupt information.
 **************************************************/
typedef struct _fm_interrupt
{
    /** Indicates that a VLAN egress boundary violation has occurred. */
    fm_uint32                   vlan1;

    /** Indicates that a VLAN ingress boundary violation has occurred. */
    fm_uint32                   vlan2;

    /** Indicates that a MAC address security violation has occurred.
     *  Note that this member is not used since security vioalations
     *  are detected when the violating frame is trapped to the CPU. */
    fm_uint32                   macSecurity;

    /** Indicates that an interrupt is being reported by the EPL on one
     *  of the ports. The EPL interrupt register must be examined for
     *  each port to locate the interrupting port and the nature of the
     *  interrupt. */
    fm_uint32                   globalEPL;

    /** An array of structures, indexed by the zero-based port number.
     *  Each element holds port-specific interrupt information. */
    fm_eplInt                   epl[25];

    /** Bit mask indicating one or more chip management interrupt sources
     *  are active. This member shadows the MGR_IP register (see FM2000
     *  datasheet). */
    fm_uint32                   mgr;

    /** Bit mask indicating one or more Logical CPU interface interrupt sources
     *  are active. This member shadows the LCI_IP register (see FM2000
     *  datasheet). */
    fm_uint32                   lci;

    /** Shadow of the LCI_IM register. */
    fm_uint32                   lciMask;

    /** Contains device-specific interrupt information. */
    fm_deviceSpecificInterrupts deviceSpecific;

} fm_interrupt;


/**************************************************/
/** Event Identifiers
 *  \ingroup constEvents
 *  \page eventIds
 *
 *  Each identifier indicates the type of event
 *  being reported by the platform layer to the API
 *  (and then to the application) or by the API
 *  directly to the application. These identifiers
 *  are used by the API global event handler,
 *  fmGlobalEventHandler and the application's event
 *  handler call-back function of type ''fm_eventHandler''
 *  (which is specified with a call to ''fmInitialize''
 *  or ''fmSetEventHandler'').
 *                                               \lb\lb
 *  Each event identifier has associated with it an
 *  event structure that carries detailed information
 *  about the event.
 **************************************************/
/** \ingroup constEvents
 * @{ */

/** Reported by the platform layer to indicate that a switch
 *  device has been inserted into the system. The associated event structure
 *  is ''fm_eventSwitchInserted''. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_SWITCH_INSERTED            (1 << 1)

/** Reported by the platform layer to indicate that a switch
 *  device has been removed from the system. The associated event structure
 *  is ''fm_eventSwitchInserted''. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_SWITCH_REMOVED				(1 << 2)

/** Reported by the API directly to the application. Indicates that a switch
 *  is now operational and ready for configuration. This event is currently 
 *  not supported. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_SWITCH_UP				    (1 << 3)

/** Reported by the API directly to the application. Indicates that a switch
 *  is now no longer operational. This event is currently not supported. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_SWITCH_DOWN				(1 << 4)

/** Reported by the API directly to the application. Indicates that the
 *  MAC Address Table has been updated with addresses learned and/or
 *  aged. The associated event structure is ''fm_eventTableUpdateBurst''. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_TABLE_UPDATE				(1 << 5)

/** Reported by the API directly to the application. Indicates that a
 *  packet has been received from the switch and is being forwarded to the
 *  CPU. The associated event structure is ''fm_eventPktRecv''. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_PKT_RECV				    (1 << 6)

/** Reported by the API directly to the application. Indicates a change in
 *  a port's state. The associated event structure is ''fm_eventPort''. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_PORT				        (1 << 7)

/** Reported by the API directly to the application. Indicates a frame
 *  handler event other than an address table update: parity error,
 *  learning skipped, SA lookup skipped, etc. This event is currently 
 *  not supported. 
 *
 *  \chips  FM2000, FM3000, FM4000 */
#define FM_EVENT_FRAME				        (1 << 8)

/** Reported by the API directly to the application. Indicates a security
 *  violation has occurred - a frame was seen to ingress for a source
 *  address not in the MAC Address Table with security enabled. The
 *  associated event structure is ''fm_eventSecurity''. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_SECURITY				    (1 << 9)

/** FM4000 only. Reported by the API directly to the application. Indicates
 *  a software event has occurred - some other software components
 *  wrote the relevant software interrupt bit in the hardware and
 *  generated an event via interrupts. The associated event structure is
 *  ''fm_eventSoftware''. 
 *
 *  \chips  FM3000, FM4000, FM6000 */
#define FM_EVENT_SOFTWARE				    (1 << 10)

/** Reported by the API directly to the application. Indicates
 *  that a packet has been logged to the CPU due to a configured sFlow instance.
 *  The associated event structure is ''fm_eventPktRecv''. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_SFLOW_PKT_RECV             (1 << 11)

/** Reported by the API directly to the application. Indicates 
 *  that a parity error had occurred in one of the hardware tables. The 
 *  associated event structure is ''fm_eventParityError''.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_EVENT_PARITY_ERROR               (1 << 12)

/** Reported by the API directly to the application. Indicates
 *  that the FIBM retries threshold has been reached.
 *  The associated event structure is ''fm_eventFibm''.
 *
 *  \chips FM3000, FM4000, FM6000 */
#define FM_EVENT_FIBM_THRESHOLD             (1 << 13)

/** Reported by the API directly to the application. Indicates that an
 *  interrupt occurred for a particular CRM. The associated event
 *  structure is ''fm_eventCrm''.
 *
 *  \chips FM3000, FM4000 */
#define FM_EVENT_CRM                        (1 << 14)

/** Reported by the API directly to the application. Indicates that
 *  an ARP interrupt has occurred. The associated event structure is
 *  ''fm_eventArp''.
 *
 *  \chips FM4000, FM6000 */
#define FM_EVENT_ARP                        (1 << 15)

/** Reported by the API directly to the application. Indicates that the
 *  MAC table purge is completed. The associated event structure
 *  is ''fm_eventPayload''.
 *
 *  \chips FM3000, FM4000 */
#define FM_EVENT_PURGE_SCAN_COMPLETE        (1 << 16) 

/**
 * Indicates that an EgressTimeStamp event has occurred on a port.
 * Reported by the API directly to the application. 
 * The associated event structure is ''fm_eventTimestamp''. 
 *  
 * \chips FM6000 
 */
#define FM_EVENT_EGRESS_TIMESTAMP           (1 << 17)

/** Reported by the platform directly to the application.
 *  The associated event structure is ''fm_eventPlatform''.
 *  
 * \chips FM6000 
 */
#define FM_EVENT_PLATFORM                   (1 << 18)

/** Signals the packet scheduler when a buffer is added to the queue for
 *  prioritized delivery to the application. This event is used when
 *  api.platform.priorityBufferQueues is enabled.
 *                                                                      \lb\lb
 *  For internal use by the API. This event is never delivered to the
 *  application.
 * 
 *  \chips FM6000
 */
#define FM_EVENT_PACKET_ENQUEUED            (1 << 19)

/** Reported by the platform directly to the application.
 *  Indicates that new logical ports for PEP usage were created or deleted.
 *  The associated event structure is ''fm_eventLogicalPort''.
 *  
 * \chips FM10000
 */
#define FM_EVENT_LOGICAL_PORT               (1 << 20)

/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values identify a port's link
 * status.
 **************************************************/
typedef enum
{
    /** Link is down. 
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_STATUS_LINK_DOWN,

    /** Link is up. 
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_STATUS_LINK_UP,

    /** Auto-negotiation just started.
     *  \chips  FM10000 */
    FM_PORT_STATUS_AUTONEG_STARTED,

    /** Auto-negotiation is complete.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_STATUS_AUTONEG_COMPLETE,

    /** Auto-negotiation failed. 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_STATUS_AUTONEG_FAILED,

    /** Auto-negotiation failed.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PORT_STATUS_AUTONEG_REMOTE_FAULT,

    /** The DFE tuning process is complete on one of the lanes belonging to
     *  this port. The outcome can be determined by fetching the
     *  ''FM_PORT_COARSE_DFE_STATE'' and ''FM_PORT_FINE_DFE_STATE'' attributes
     *  for the same port, mac, lane combination.
     *  \chips  FM6000, FM10000 */
    FM_PORT_STATUS_DFE_COMPLETE,

    /** The DFE tuning process timed out on one of the lanes belonging to this
     *  port. The API is retrying.
     *  \chips  FM6000, FM10000 */
    FM_PORT_STATUS_DFE_TIMEOUT,

    /** There was an error in the DFE tuning process on one of the lanes
     *  belonging to this port.
     *  \chips  FM6000, FM10000 */
    FM_PORT_STATUS_DFE_ERROR

} fm_portLinkStatus;


/**************************************************/
/** \ingroup constTblUpdateEvent
 * These MA Table Update Events identify the type 
 *  of MA Table update being reported in the event 
 *  member of the ''fm_eventTableUpdate'' structure 
 *  for an ''FM_EVENT_TABLE_UPDATE'' event.
 **************************************************/
enum _fm_tblUpdateEvent
{
    /** MA Table entry not in use (not supported). 
     *  \chips  (None) */
    FM_EVENT_ENTRY_EMPTY = 0,

    /** An address has been learned. 
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_EVENT_ENTRY_LEARNED,

    /** An address has been aged. 
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_EVENT_ENTRY_AGED,

    /** A memory error occurred. 
     *  \chips  FM6000 */
    FM_EVENT_ENTRY_MEMORY_ERROR,

    /** UNPUBLISHED: For internal use only. */
    FM_EVENT_ENTRY_MAX

};  /* end enum _fm_tblUpdateEvent */


/** \ingroup macroSynonym
 *  Legacy synonym for ''FM_EVENT_ENTRY_MEMORY_ERROR''. */
#define FM_EVENT_ENTRY_PERR FM_EVENT_ENTRY_MEMORY_ERROR


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a single MA Table
 * update (learn or age). Such updates are reported
 * several at a time as an ''FM_EVENT_TABLE_UPDATE''
 * event.
 **************************************************/
typedef struct _fm_eventTableUpdate
{
    /** Used internally by the API to hold the index into the MA Table
     *  where the update occurred. */
    fm_int     index;

    /** Type of update event. See 'MA Table Update Events'. */
    fm_int     event;

    /** Copied from the MA Table entry, a mask of logical port numbers.
     *  \chips  FM2000, FM3000, FM4000 */
    fm_uint32  destMask;

    /** Destination logical port. Only used when destMask is 
     *  ''FM_DESTMASK_UNUSED''. For the FM6000, may be -1 if the entry
     *  has been aged out as the result of a station move. */
    fm_int     port;

    /** Copied from the MA Table entry, a new entry will have age = 1. */
    fm_bool    age;

    /** Copied from the MA Table entry, a static entry will have locked = 1. */
    fm_bool    locked;

    /** Copied from the MA Table entry, indicates if the entry is valid or
     *  aged out. */
    fm_bool    valid;

    /** Used internally by the API to indicate the reason this update was
     *  generated. */
    fm_byte    reason;

    /** Copied from the MA Table entry, the trigger number activated on an
     *  address match.
     *  \chips  FM2000, FM3000, FM4000, FM10000 */
    fm_uint32  trigger;

    /** Copied from the MA Table entry, the VLAN number associated with the
     *  entry. */
    fm_int     vlanID;

    /** Copied from the MA Table entry, the second VLAN number associated
     *  with the entry.
     *  \chips  FM6000 */
    fm_int     vlanID2;

    /** Copied from the MA Table entry, the MAC address associated with the
     *  entry. */
    fm_macaddr macAddress;

    /** Copied from the MA Table entry, the remote ID associated with the
     *  entry.
     *  \chips  FM6000 */
    fm_int     remoteID;

    /** Copied from the MA Table entry, a remote MAC is associated with the
     *  entry.
     *  \chips  FM6000 */
    fm_bool    remoteMac;

    /** Address type specified when the entry was created. Copied from the
     *  MA Table entry.
     *  \chips  FM10000 */
    fm_uint16  addrType;

} fm_eventTableUpdate;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report multiple simultaneous
 * MA Table updates for an ''FM_EVENT_TABLE_UPDATE''
 * event.
 **************************************************/
typedef struct _fm_eventTableUpdateBurst
{
    /** The number of updates being reported. */
    fm_uint32            numUpdates;

    /** An array of individual MA Table updates. The array has
     *  ''FM_TABLE_UPDATE_BURST_SIZE'' entries. */
    fm_eventTableUpdate *updates;

} fm_eventTableUpdateBurst;


/** The number of 32-bit words that appear in the Inter-Switch Link (ISL) tag.
 *  \ingroup constSystem
 */
#define FM_MAX_ISL_TAG_SIZE  2


/**************************************************/
/** \ingroup typeStruct
 * Precise wallclock time, in a format equivalent
 * to the IEEE 1588 TimeInterval structure.
 **************************************************/
typedef struct _fm_preciseTimestamp
{
    /** Integer portion of the timestamp, in whole seconds since the
     *  beginning of the epoch. */
    fm_uint64   seconds;

    /** Fractional portion of the timestamp expressed in units of 
     *  nanoseconds multiplied by 2^16. */
    fm_uint64   scaledNanoseconds;

} fm_preciseTimestamp;


/**************************************************/
/** \ingroup typeStruct
 * Precise wallclock time, in a format equivalent
 * to the POSIX timespec structure.
 **************************************************/
typedef struct _fm_timespec
{
    /** Seconds since beginning of the epoch. */
    fm_int64   seconds;

    /** Fractional portion of the timestamp expressed in
     *  nanoseconds. */
    fm_int64   nanoseconds;

} fm_timespec;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a received packet for
 * an ''FM_EVENT_PKT_RECV'' event.
 **************************************************/
typedef struct _fm_eventPktRecv
{
    /** Pointer to an ''fm_buffer'' containing the received packet. Note that
     *  the receiving application should not write to the ''fm_buffer''
     *  structure pointed to by pkt. It should not be reused, for example,
     *  as a transmit buffer. Instead, the received buffer should be freed
     *  and a new buffer allocated for transmission.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000  */
    void *    pkt;

    /** The number of the switch from which the packet was received.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    fm_int    switchNum;

    /** The logical port number from which the packet was received.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    fm_int    srcPort;

    /** The VLAN from which the packet was received.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    fm_int    vlan;

    /** Indicates how the vlan member should be interpreted:            \lb
     *  0 - The vlan member is the same as the VLAN ID in the packet.   \lb
     *  1 - The vlan member is the new VLAN association for this packet
     *      and the VLAN tag present in packet has been removed.        \lb
     *  2 - The vlan member is the new VLAN association for this
     *      packet and has been added to the packet.                    \lb
     *  3 - The vlan member is the new VLAN association for this packet
     *      and replaced the one present in the packet.
     *  
     * \chips  FM2000 */
    fm_int    action;

    /** Indicates the reason that the frame was delivered by the switch to 
     *  the CPU. Refer to the FM_SWITCH_TRAP_CODE switch attribute
     *  for the meaning of each trapAction.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int    trapAction;

    /** Indicates the internal switch priority associated with this packet.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    fm_int    priority;

    /** The ISL tag, which is an array of ''FM_MAX_ISL_TAG_SIZE'' 32-bit 
     *  words.
     *  
     *  \chips  FM4000, FM6000, FM10000  */
    fm_uint32 ISLTag[FM_MAX_ISL_TAG_SIZE];

    /** The vlan priority associated with this packet. 
     *  
     *  \chips FM4000, FM6000, FM10000  */
    fm_int    vlanPriority;

    /** The vlan EtherType associated with this packet.
     *  
     *  \chips  FM4000, FM6000 */
    fm_int    vlanEtherType;

    /** The ingress timestamp, in EPL timer ticks.
     *  
     *  \chips  FM6000 */
    fm_uint32            rawIngressTime;

    /** The wallclock time at which the packet was received, in 1588
     *  TimeInterval format.
     *  
     *  \chips  FM6000 */
    fm_preciseTimestamp  ingressTime;

    /** The 64-bit ingress timestamp in Hardware clock ticks. 
     * 
     *  \chips FM10000 */
    fm_uint64            rawIngressTimeV2;

    /** The wallclock time at which the packet was received, in POSIX
     *  timespec format.
     *
     *   \chips FM10000 */
    fm_timespec          ingressTimeV2;

} fm_eventPktRecv;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report port events (link
 * UP/DOWN) for the ''FM_EVENT_PORT'' event. Also used
 * internally when requesting an MA Table flush for
 * a specific port or ACL.
 **************************************************/
typedef struct _fm_eventPort
{
    /** The logical port for which the event is being reported or MA Table
     *  flush is being requested. Note that when reporting events, the
     *  driver will initially record the physical port number, but it will
     *  be converted to a logical port number by the API before being
     *  passed on to the application. */
    fm_uint           port;

    /** Port link status when used to report a port event (see
     *  ''fm_portLinkStatus''). */
    fm_portLinkStatus linkStatus;

    /** Bit mask indicating one or more active PCS interrupt sources on
     *  the port. This member shadows the PCS_IP register for the port
     *  when reporting port events. (Not used.) */
    fm_uint32         pcsStatus;

    /** Value of received AN code word for SGMII, Clause 37, or
     *  Clause 73 events. SGMII and Clause 37 uses only the low-order 32 bits. */
    fm_uint64         autonegCode;

    /** The MAC for the logical port on which the event occurred. */
    fm_int            mac;

    /** The port's zero-based lane number for which a per-lane link event
     *  being reported. Should be set to FM_PORT_LANE_NA for events that
     *  don't have a per-lane scope */
    fm_int            lane; 

    /** Indicates if the MAC on which the event occurred is the active
     *  MAC for this logical port. */
    fm_bool           activeMac;

} fm_eventPort;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to request an MA Table flush
 * for a specific VLAN.
 **************************************************/
typedef struct _fm_eventVlan
{
    /** The VLAN on which to perform the MA Table flush. */
    fm_uint vlan;

} fm_eventVlan;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a source MAC address
 * security violation for event ''FM_EVENT_SECURITY''.
 **************************************************/
typedef struct _fm_eventSecurity
{
    /** The logical port number on which the violating source MAC address
     *  was received. */
    fm_uint    port;

    /** The VLAN on which the violating source MAC address was received. */
    fm_int     vlan;

    /** Offending source MAC address. */
    fm_macaddr address;

} fm_eventSecurity;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a software event for
 * event ''FM_EVENT_SOFTWARE''.
 **************************************************/
typedef struct _fm_eventSoftware
{
    /** The bitmask of currently active events */
    fm_uint32 activeEvents;

} fm_eventSoftware;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report an ''FM_EVENT_LOGICAL_PORT'' 
 * software event.
 **************************************************/
typedef struct _fm_eventLogicalPort
{
    /** Number of first port created. */
    fm_int firstPort;

    /** Number of consecutive ports created. */
    fm_int numberOfPorts;

    /** PEP ID number. */
    fm_int pepId;

    /** PEP logical port. */
    fm_int pepLogicalPort;

    /** Value to indicate whether ports were created or deleted. */
    fm_bool portCreated;

} fm_eventLogicalPort;


/**************************************************/
/* Internal event type identifier.
 **************************************************/
typedef enum
{
    FM_EVID_UNIDENTIFIED = 0,
    FM_EVID_LOW_SOFTWARE,
    FM_EVID_LOW_FRAME_CTRL,
    FM_EVID_LOW_LCI,
    FM_EVID_LOW_MAC_SECURITY,
    FM_EVID_HIGH_MAC_SECURITY,
    FM_EVID_HIGH_TABLE_UPDATE,
    FM_EVID_HIGH_PURGE_SCAN_COMPLETE,
    FM_EVID_HIGH_EVENT_FRAME,
    FM_EVID_HIGH_PKT_RECV,
    FM_EVID_HIGH_PKT_RECV_NEW_BUFS,
    FM_EVID_HIGH_PKT_SEND,
    FM_EVID_HIGH_PORT,
    FM_EVID_HIGH_EGRESS_TIMESTAMP,
    FM_EVID_SYSTEM,
    FM_EVID_ARP,
    FM_EVID_PLATFORM,

    /* Add new types above this line */
    FM_EVID_OUT_OF_EVENTS,
    FM_EVID_MAX

} fm_eventID;


/**************************************************/
/** \ingroup typeStruct
 * This structure is used by the platform layer to
 * send an ''FM_EVENT_SWITCH_INSERTED'' to the global
 * event handler, fmGlobalEventHandler (see
 * ''Event Identifiers''). It identifies the model
 * and slot number for a switch device that has
 * been inserted into the system and that must now
 * be managed by the API.
 **************************************************/
typedef struct _fm_eventSwitchInserted
{
    /** Zero-based physical slot into which the switch was inserted. Note that
     *  the slot number is not necessarily the switch number, and that the
     *  platform is responsible for translating the slot number to a switch
     *  number. */
    fm_int                  slot;

    /* The platform layer code should pass the switch ID info below so the API
     * won't have to do identify the switch again and perform an extra 
     * switch reset to get the same information */

    /** Identifies the family of the switch that was inserted, as determined
     *  by the platform layer code. Will be zero (FM_SWITCH_FAMILY_UNKNOWN)
     *  if the platform layer does not support this feature. */
    fm_switchFamily         family;

    /** Identifies the model of the switch that was inserted, as determined
     *  by the platform layer code. Will be zero or -1 if the platform layer
     *  does not support this feature. */
    fm_switchModel          model;

    /** Identifies the version of the switch that was inserted, as determined
     *  by the platform layer code. Will be zero if the platform layer does
     *  not support this feature. */
    fm_switchVersion        version;

} fm_eventSwitchInserted;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is used when an 
 *  ''FM_EVENT_SWITCH_REMOVED'' event is being 
 *  reported. It identifies the model and slot number 
 *  for a switch device that has been inserted 
 *  into the system and that must now be managed by 
 *  the API.
 **************************************************/
typedef struct _fm_eventSwitchRemoved
{
    /** Zero-based physical slot into which the switch was inserted. Note that
     *  the slot number is not necessarily the switch number and that the
     *  platform is responsible for translating the slot number to a switch
     *  number. */
    fm_int slot;

} fm_eventSwitchRemoved;


/** The maximum size of platform event structures.
 *  \ingroup constSystem
 */
#define FM_EVENT_PLATFORM_MAX_SIZE  16*4

/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a platform event for
 * event ''FM_EVENT_PLATFORM''. This is a generic 
 * structure for the API to pass the event to the
 * application. The content is platform specific.
 **************************************************/
typedef struct _fm_eventPlatform
{
    /** Platform event type. */
    fm_uint32 type;

    /** Platform event data. */
    fm_byte   eventData[FM_EVENT_PLATFORM_MAX_SIZE];

} fm_eventPlatform;


/**************************************************/
/** \ingroup typeEnum
 *  A component of an ''fm_eventParityError'' event
 *  report. Identifies the severity of a reported 
 *  parity error.
 **************************************************/
typedef enum
{
    /** The parity error severity is not defined. */
    FM_PARITY_SEVERITY_UNDEFINED,

    /** A software correctable parity error was detected. The API will have
     *  attempted to correct the error. No further action is required.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_PARITY_SEVERITY_USER_FIXABLE,

    /** A parity error occurred on a single packet. The error is not
     *  persistent, has no consequence and no further action is required.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    FM_PARITY_SEVERITY_TRANSIENT,

    /** A cumulative parity error occurred, causing some memory to be lost,
     *  but has no effect otherwise.  The switch must be reset (with
     *  ''fmSetSwitchState'') if too much memory is lost to cumulative
     *  errors.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    FM_PARITY_SEVERITY_CUMULATIVE,

    /** A fatal parity error has occurred. The switch must be reset (with
     *  ''fmSetSwitchState'') to restore proper operation.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_PARITY_SEVERITY_FATAL,

    /** A parity error was automatically corrected by the hardware.
     *  No further action is required.
     *  
     *  \chips  FM6000 */
    FM_PARITY_SEVERITY_CORRECTED,

    /** UNPUBLISHED: For internal use only. */
    FM_PARITY_SEVERITY_MAX

} fm_paritySeverity;


/**************************************************/
/** \ingroup typeEnum
 *  A component of an ''fm_eventParityError'' event
 *  report. Identifies the memory area in which the
 *  the parity error occurred.
 **************************************************/
typedef enum
{
    /** The memory area is not defined. */
    FM_PARITY_AREA_UNDEFINED,

    /** ARRAY memory error.
     *  \chips  FM6000, FM10000 */
    FM_PARITY_AREA_ARRAY,

    /** CROSSBAR memory error.
     *  \chips  FM10000 */
    FM_PARITY_AREA_CROSSBAR,

    /** EPL or SERDES memory error.
     *  \chips  FM10000 */
    FM_PARITY_AREA_EPL,

    /** MODIFY memory error.
     *  \chips  FM6000, FM10000 */
    FM_PARITY_AREA_MODIFY,

    /** PARSER memory error.
     *  \chips  FM6000 */
    FM_PARITY_AREA_PARSER,

    /** POLICER memory error.
     *  \chips  FM6000 */
    FM_PARITY_AREA_POLICER,

    /** SCHEDULER memory error.
     *  \chips  FM10000 */
    FM_PARITY_AREA_SCHEDULER,

    /** TCAM memory error.
     *  \chips  FM10000 */
    FM_PARITY_AREA_TCAM,

    /** Tunneling Engine memory error.
     *  \chips  FM10000 */
    FM_PARITY_AREA_TUNNEL_ENGINE,

    /** UNPUBLISHED: For internal use only. */
    FM_PARITY_AREA_MAX

} fm_parityMemArea;


/**************************************************/
/** \ingroup typeEnum
 *  A component of an ''fm_eventParityError'' event
 *  report. Indicates the status of the parity 
 *  error.
 **************************************************/
typedef enum
{
    /** No parity error was detected.  */
    FM_PARITY_STATUS_NO_ERROR_DETECTED,

    /** Parity error was detected and repaired. */
    FM_PARITY_STATUS_ERROR_FIXED,

    /** Parity error was detected and an attempt to repair it failed. */
    FM_PARITY_STATUS_FIX_FAILED,

    /** No action is required for this parity error.  */
    FM_PARITY_STATUS_NO_ACTION_REQUIRED,

    /** Fatal parity error; switch must be reset. */
    FM_PARITY_STATUS_FATAL_ERROR,

    /** Parity error was automatically corrected in hardware.
     *  \chips  FM6000 */
    FM_PARITY_STATUS_ECC_CORRECTED,

    /** Parity error was detected, but no attempt was made to repair it.
     *  \chips  FM6000 */
    FM_PARITY_STATUS_NO_FIX_ATTEMPTED,

    /** The application will eventually need to reset the switch in order to
     *  recover lost resources, but this action may be deferred to an
     *  appropriate time as the loss of memory is small.
     *  \chips  FM10000 */
    FM_PARITY_STATUS_DEFERRED_RESET,

    /** Parity error status is undefined. */
    FM_PARITY_STATUS_UNDEFINED,

    /** UNPUBLISHED: For internal use only. */
    FM_PARITY_STATUS_MAX

} fm_parityStatus;


/**************************************************/
/** \ingroup typeEnum
 *  A component of an ''fm_eventParityError'' event
 *  report. Indicates the type of parity error being
 *  reported.
 **************************************************/
typedef enum
{
    /** No parity error type.
     *  \chips  FM3000, FM4000, FM6000 */
    FM_PARITY_ERRTYPE_NONE,

    /** The sweeper found a disparity between the cache
     *  and the register contents.
     *  \chips  FM6000 */
    FM_PARITY_ERRTYPE_CACHE_MISMATCH,

    /** A correctable ECC error was detected.
     *  \chips  FM6000 */
    FM_PARITY_ERRTYPE_SRAM_CORRECTED,

    /** An uncorrectable ECC or parity error was detected.
     *  \chips  FM6000, FM10000 */
    FM_PARITY_ERRTYPE_SRAM_UNCORRECTABLE,

    /** UNPUBLISHED: For internal use only. */
    FM_PARITY_ERRTYPE_MAX

} fm_parityErrType;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a memory parity error for
 * an ''FM_EVENT_PARITY_ERROR'' event.
 **************************************************/
typedef struct _fm_eventParityError
{
    /** Type of parity error being reported.
     *  See ''fm_parityErrType'' for a complete description. */
    fm_parityErrType    errType;

    /** Severity of the parity error. See ''fm_paritySeverity'' for
     *  a complete description. */
    fm_paritySeverity   paritySeverity;

    /** Memory area reporting the parity error. See ''fm_parityMemArea''
     *  for a complete description. */
    fm_parityMemArea    memoryArea;

    /** Status of the parity error, indicating whether the parity error
     *  has been fixed (see ''fm_parityStatus''). */
    fm_parityStatus     parityStatus;

    /** Parity error register value, used when memoryArea is set to 
     *  FM_PARITY_AREA_PERR_IP.
     *  
     *  \chips  FM3000, FM4000 */
    fm_uint32           ipRegValue;

    /** Another parity error register value, used for other values 
     * of memoryArea. 
     *  
     * \chips   FM3000, FM4000 */ 
    fm_uint32           parityRegValue;

    /** Value that failed parity check. When a 64-bit or larger value is saved 
     * here, the array contains the individual 32-bit words in big-endian order,
     * regardless of the ordering of the individual words. 
     *  
     * \chips   FM3000, FM4000, FM6000 */
    fm_uint32           badData[4];

    /** The cached value that was written to the target to fix the parity error.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    fm_uint32           cachedData[4];

    /** The number of values in the above two members that are valid.
     *  Will be zero for FM10000.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    fm_uint32           numValidData;
    
    /** Indices into a table that was fixed.
     *  
     * \chips   FM3000, FM4000, FM6000 */ 
    fm_uint32           tableIndices[2];

    /** The number of indices that are valid. Will be zero for FM10000.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    fm_uint32           numIndices;

    /** Base address of the register table in which the error occurred.
     *  Used when errType is ''FM_PARITY_ERRTYPE_CACHE_MISMATCH''.
     *  
     *  \chips  FM6000 */
    fm_uint32           baseAddr;

    /** The number of invalid entries in the register table.
     *  Used when errType is ''FM_PARITY_ERRTYPE_CACHE_MISMATCH''.
     *  
     *  \chips  FM6000 */
    fm_int              numErrors;

    /** The SRAM number in which the error was detected.
     *  Used when errType is ''FM_PARITY_ERRTYPE_SRAM_CORRECTED'' or
     *  ''FM_PARITY_ERRTYPE_SRAM_UNCORRECTABLE''.
     *  
     *  \chips  FM6000, FM10000 */
    fm_int              sramNo;

} fm_eventParityError;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is used when an 
 *  ''FM_EVENT_FIBM_THRESHOLD'' event is being 
 *  reported. It identifies the number of FIBM retries.
 **************************************************/
typedef struct _fm_eventFibm
{
    /** Identifies the number of FIBM retries. */
    fm_int retries;

} fm_eventFibm;
               

/**************************************************/
/** \ingroup typeStruct
 * Structure used to report a CRM interrupt for
 * an ''FM_EVENT_CRM'' event.
 **************************************************/
typedef struct _fm_eventCrm
{
    /** The CRM for which the event is being reported. */
    fm_int  crmID;

} fm_eventCrm;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report an ARP interrupt for
 * an ''FM_EVENT_ARP'' event.
 **************************************************/
typedef struct _fm_eventArp
{
    /** The SIP for which the event is being reported */
    fm_ipAddr  arpRedirectSip;
    
    /** The DIP for which the event is being reported */
    fm_ipAddr  arpRedirectDip;

} fm_eventArp;


/**************************************************/
/** \ingroup typeStruct
 * Structure used to report an Egress Timestamp 
 * interrupt for an ''FM_EVENT_EGRESS_TIMESTAMP'' 
 * event.
 **************************************************/
typedef struct _fmEventTimestamp
{
    /** The logical port on which the frame was transmitted. */
    fm_int       egressPort;

    /** 32-bit ingress timestamp, in EPL timer ticks. */
    fm_uint32    rawIngressTime;

    /** 32-bit egress timestamp, in EPL timer ticks. */
    fm_uint32    rawEgressTime;

    /** The wallclock time at which the frame was transmitted. */
    fm_preciseTimestamp egressTime;

} fm_eventTimestamp;


/**************************************************/
/** \ingroup typeStruct
 * A union of event buffer payloads that are
 * carried in an ''fm_event'' structure.
 **************************************************/
typedef union _fm_eventPayload
{
    /** Used for an interrupt event. Used internally by the API. 
     *  Application code should never receive this event. */
    fm_interrupt             fpIntr;

    /** Used for a MA Table learn or age event. */
    fm_eventTableUpdateBurst fpUpdateEvent;

    /** Used for a received packet. */
    fm_eventPktRecv          fpPktEvent;

    /** Used to report port up/down, to request an MA Table port flush
     *  and to request an MA Table ACL update. */
    fm_eventPort             fpPortEvent;

    /** Used to request an MA Table VLAN flush. */
    fm_eventVlan             fpVlanEvent;

    /** Used for a security violation event. */
    fm_eventSecurity         fpSecEvent;

    /** Used for a software event. Used internally by the API. 
     *  Application code should never receive this event. */
    fm_eventSoftware         fpSoftwareEvent;

    /** Switch Inserted Event */
    fm_eventSwitchInserted   fpSwitchInsertedEvent;

    /** Switch Removed Event */
    fm_eventSwitchRemoved    fpSwitchRemovedEvent;

    /** Parity error Event */
    fm_eventParityError      fpParityErrorEvent;

    /** Fibm retries threshold event */
    fm_eventFibm             fibmRetries;

    /** CRM interrupt event */
    fm_eventCrm              fpCrmEvent;

    /** ARP interrupt event */
    fm_eventArp              fpArpEvent;

    /** MAC table purge scan complete event */
    fm_bool                  purgeScanComplete; 

    /** Egress timestamp event. */
    fm_eventTimestamp        egressTimestamp;

    /** Platform-specific event. Used by the platform layer to signal events
     *  to the application. The API should not care about the content of
     *  this event. */
    fm_eventPlatform         fpPlatformEvent;

    /** Logical port creation/deletion event. */
    fm_eventLogicalPort      fpLogicalPortEvent;

} fm_eventPayload;


/**************************************************/
/** \ingroup typeEnum
 * Identifies the priority of the event.  Higher
 * priority events may be sent unthrottled over
 * lower priority events
 **************************************************/
typedef enum
{
    /* The calling thread cannot be blocked */
    FM_EVENT_PRIORITY_HIGH = 0,

    /* The calling thread can be blocked */
    FM_EVENT_PRIORITY_LOW

} fm_eventPriority;


/**************************************************/
/** \ingroup typeStruct
 * An event is a buffer that is used to communicate
 * information between tasks in the driver. This
 * structure is used to carry information specific
 * to the event being communicated.
 * Due to dependence of the alos/fm_alos_event_queue.h
 * and alos/fm_alos_threads.h files on pre-definition
 * of the fm_event typedef, this typedef was
 * forward-defined in fm_sdk.h.  Thus, all that
 * is needed here is the structure definition.
 **************************************************/
struct _fm_event
{
    /** See 'Event Identifiers'. */
    fm_int           type;

    /** Identifies the switch for which this event is being reported. */
    fm_int           sw;

    /** An identifier used for diagnostics. */
    fm_eventID       eventID;

    /** Records when the event was posted to an event queue. */
    fm_timestamp     postedTimestamp;

    /** Records when the event was popped from the queue. */
    fm_timestamp     poppedTimestamp;

    /** Priority of the event. */
    fm_eventPriority priority;

    /** Queue in which this event is present. Null if not in any queue. */
    fm_eventQueue    *q;

    /** Node in the event queue containing this event. */
    fm_dlist_node    *node;

    /** Union of event information payloads for different event types. */
    fm_eventPayload  info;

};

#endif /* __FM_FM_API_EVENT_TYPES_H */
