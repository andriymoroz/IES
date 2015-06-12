/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_packet.h
 * Creation Date:   May 15, 2007
 * Description:     Constants and structures related to packet sending
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

#ifndef __FM_FM_API_PACKET_H
#define __FM_FM_API_PACKET_H

/* The user sets info.logicalPort to 0xFFFF to indicate the intention
 * for direct vlan send */
#define FM_DIRECT_VLAN_SEND     0xFFFF

/* the user specified info.switchPriority value indicating using
 * info.vlanPriority to construct the SWPRI field of the ISL tag */
#define FM_USE_VLAN_PRIORITY    0xFFFF

/* The caller sets fcsValue to 0 to indicate that default processing
 * is to occur. */
#define FM_USE_DEFAULT_FCS      0

/* Extracts a 31-bit ingress timestamp from the packet's FCS value. */
#define FM_FCS_VALUE_TO_TIMESTAMP(fcsVal)          \
    ((((fcsVal) >> 1) & 0x7fffff80) | ((fcsVal) & 0x7f))

/* Converts a 31-bit ingress timestamp and a 1-bit egress timestamp request
 * flag to the value to be stored in the packet's FCS field. */
#define FM_TIMESTAMP_TO_FCS_VALUE(eplTime, lastBit) \
    ((((eplTime) << 1) & 0xffffff00) | ((eplTime) & 0x7f) | ((lastBit) << 7))


/**************************************************/
/** \ingroup typeEnum 
 * Specifies how the FCS field should be generated. 
 * Used in the fcsMode field of ''fm_packetInfoV2''.
 **************************************************/
typedef enum
{
    /** Use the default FCS mode for this platform. */
    FM_FCS_MODE_DEFAULT = 0,

    /** Automatically generate the FCS for the frame.
     *  Sets the AttachCRC bit in the LCI control word.
     *  \chips  FM2000, FM4000 */
    FM_FCS_MODE_AUTO,

    /** Set the FCS to zero.
     *  \chips  FM6000 */
    FM_FCS_MODE_ZERO,

    /** Set the FCS to request an egress timestamp.
     *  \chips  FM6000 */
    FM_FCS_MODE_TIMESTAMP,

    /** Set the FCS to the value specified in the fcsValue field.
     *  \chips  FM6000 */
    FM_FCS_MODE_VALUE

} fm_fcsMode;


/**************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmSendPacketExt'' to
 * indicate how the packet should be handled.
 *                                              \lb\lb
 * Note: the structure must have all fields
 * initialized when ''fmSendPacketExt'' is called.
 * Any unused field should be set to zero. Failure
 * to initialize all fields can result in the packet
 * being mishandled.
 **************************************************/
typedef struct _fm_packetInfo
{
    /** Mask of destination logical ports to which the packet should be
     *  sent. Set to zero to have the switch choose the destination based on
     *  the MA Table. This field is ignored if logicalPort is not zero.
     *  
     *  \chips  FM2000 */
    fm_uint   destMask;

    /** Specifies the port on the switch through which the
     *  packet should egress. Set to zero to have the switch choose the
     *  destination (for FM2000, destMask must also be zero). For FM2000,
     *  FM4000, and FM6000, if set to FM_DIRECT_VLAN_SEND, then the packet
     *  will be sent on all ports that are members of the VLAN identified by
     *  directSendVlanId. */
    fm_uint16 logicalPort;

    /** Identifies a VLAN on which to broadcast the packet. The packet will be
     *  transmitted on all ports that are members of the specified VLAN.
     *  logicalPort must be set to FM_DIRECT_VLAN_SEND. */
    fm_int    directSendVlanId;

    /** Specifies whether to transmit the packet using the egress ports'
     *  tagging rules. If TRUE, and an egress port in the VLAN specified by
     *  vlanId is configured to transmit tagged frames, the packet will be
     *  tagged with the VLAN ID specified by vlanId. */
    fm_bool   useEgressRules;

    /** If useEgressRules is TRUE and logicalPort (or destMask on FM2000) is
     *  not zero, this field specifies the VLAN ID that will appear in the
     *  packet as it egresses a port in the specified VLAN and for which
     *  tagging is enabled. */
    fm_int    vlanId;

    /** If useEgressRules is TRUE and logicalPort (or destMask on FM2000) is
     *  not zero, this field identifies the VLAN priority that will appear in
     *  the packet as it egresses a port in the specified VLAN and for which
     *  tagging is enabled. */
    fm_int    vlanPriority;

    /** Specifies the switch priority with which the packet should be
     *  identified when logicalPort is not zero.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    fm_uint16 switchPriority;

    /** Specifies whether the packet is sent to the CPU port.
     *  
     *  \chips  FM3000, FM4000 */
    fm_bool   directSendToCpu;

    /** The logical port number to use as the source port of the packet. A
     *  value of 0 indicates using the (default) cpu port.
     *  
     *  \chips  FM3000, FM4000, FM6000 */
    fm_int    sourcePort;

    /** Forces the source glort delivered to the CPU to be zero.
     *  FALSE specifies normal operation, TRUE forces the source glort field
     *  to zero. A source glort value of zero will cause the destination
     *  switch to use the local ingress port's glort as the source glort
     *  when processing the frame.
     *
     *  \chips  FM3000, FM4000, FM6000 */
    fm_bool   zeroSourceGlort;

} fm_packetInfo;


/**************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmSendPacketDirectedV2'' 
 * to indicate how the packet should be handled.
 *                                              \lb\lb
 * Note: the structure must have all fields
 * initialized when ''fmSendPacketDirectedV2'' is 
 * called. Any unused fields should be set to zero. 
 * Failure to initialize all fields can result in 
 * the packet being mishandled.
 **************************************************/
typedef struct _fm_packetInfoV2
{
    /** Specifies the switch priority with which the packet should
     *  be identified. */
    fm_uint16   switchPriority;

    /** Specifies how the FCS field should be generated. */
    fm_fcsMode  fcsMode;

    /** Specifies the FCS value to be appended to the packet when
     *  fcsMode is FM_FCS_MODE_VALUE.
     *  \chips  FM6000  */
    fm_uint32   fcsValue;

} fm_packetInfoV2;



/****************************************************************************/
/** Rx Packet Driver Destination Masks
 *  \ingroup constRxPacketDrvDest
 *  \page RxPacketDrvDestMasks
 *
 *  The following bit masks may be ORed together to produce a value for API
 *  property ''api.packet.rxDriverDestinations''. Each bit mask selects a
 *  destination to which received packets should be forwarded by the driver.
 ****************************************************************************/

/** \ingroup constRxPacketDrvDest
 * @{ */

/** Received packets are forwarded to netdev. */
#define FM_PACKET_RX_DRV_DEST_NETDEV      (1U << 0)

/** Received packets are forwarded to netlink sockets that are read and
 *  processed by the API. The API passes frames to the application. */
#define FM_PACKET_RX_DRV_DEST_NETLINK_API (1U << 1)

/** Received packets are forwarded to netlink sockets that are read by
 *  the application directly, without the API processing them. Packets
 *  will have the extended netlink header (see ''fm_nlExtHdr''). Netlink
 *  sockets should be created with protocol ''FM_NL_SOCK'' and bound to
 *  group type ''FM_NL_GROUP_ALL''.
 *                                                                      \lb\lb
 *  For this destination, the driver does not perform the following
 *  processing, which is normally done by the API for received packets:
 *  dropping trapped BPDU packets based on switch configuration,
 *  dropping LACP packets based on switch configuration, timestamp
 *  generation, and matching against sFlow. */
#define FM_PACKET_RX_DRV_DEST_NETLINK_APP (1U << 2)

/** @} (end of Doxygen group) */

#endif /* __FM_FM_API_PACKET_H */
