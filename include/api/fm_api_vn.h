/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_api_vn.h
 * Creation Date:   August 30, 2012
 * Description:     Header file for virtual networking services.
 *
 * Copyright (c) 2012 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_VN_H
#define __FM_FM_API_VN_H


/** \ingroup constSystem
 * @{ */

/** The value used in virtual network tunneling to identify frames
 *  that need VXLAN decapsulation. The application does this by using
 *  this value as the updated VLAN2 value in an ACL rule action. */
#define FM_VN_DECAPSULATION_VLAN2_VXLAN     4095

/** The value used in virtual network tunneling to identify frames
 *  that need NVGRE decapsulation. The application does this by using
 *  this value as the updated VLAN2 value in an ACL rule action. */
#define FM_VN_DECAPSULATION_VLAN2_NVGRE     4094

/** The default UDP Destination port value for VXLAN protocol. May be
 *  overridden by setting the ''FM_SWITCH_TUNNEL_DEST_UDP_PORT'' switch
 *  attribute. */
#define FM_VN_VXLAN_UDP_DEST_PORT           4789

/** The default UDP Destination port value for Geneve protocol. May be
 *  overridden by setting the ''FM_SWITCH_GENEVE_TUNNEL_DEST_UDP_PORT''
 *  switch attribute. */
#define FM_VN_GENEVE_UDP_DEST_PORT          6081

/** @} (end of Doxygen group) */

/**
 * \ingroup macroSynonym 
 * A legacy synonym for ''FM_VN_DECAPSULATION_VLAN2_VXLAN''. 
 */
#define FM_VN_DECAPSULATION_VLAN2           FM_VN_DECAPSULATION_VLAN2_VXLAN


/**************************************************/
/** \ingroup typeEnum
 *  Identifies the protocol used by a tunnel.
 *  Referenced by ''fmCreateVNTunnel''.
 **************************************************/
typedef enum
{
    /** VXLAN Virtual Network tunnel for IPV4 traffic. */
    FM_VN_TUNNEL_TYPE_VXLAN_IPV4,

    /** VXLAN Virtual Network tunnel for IPV6 traffic. */
    FM_VN_TUNNEL_TYPE_VXLAN_IPV6,

    /** NVGRE Virtual Network. */
    FM_VN_TUNNEL_TYPE_NVGRE,

    /** Geneve (NGE) Virtual Network.
     *  \chips  FM10000 */
    FM_VN_TUNNEL_TYPE_GENEVE,

    /** UNPUBLISHED: For internal use only. */
    FM_VN_TUNNEL_TYPE_MAX       /* Must be last */

} fm_vnTunnelType;


/**
 * \ingroup macroSynonym 
 * A legacy synonym for ''FM_VN_TUNNEL_TYPE_VXLAN_IPV4''. 
 */
#define FM_VN_TUNNEL_TYPE_VXLAN           FM_VN_TUNNEL_TYPE_VXLAN_IPV4


/**************************************************/
/** \ingroup typeEnum
 *  Specifies the operating mode used for a virtual       .
 *  network. Referenced by ''fm_vnDescriptor''.
 **************************************************/
typedef enum
{
    /** Virtual Network is operating in VSwitch-Offload mode. The tunnel end
     *  point belongs to the VSwitch. */
    FM_VN_MODE_VSWITCH_OFFLOAD,

    /** Virtual Network is operating in Transparent mode. The tunnel end point
     *  belongs to the Switch Manager. */
    FM_VN_MODE_TRANSPARENT,

} fm_vnMode;


/**************************************************/
/** \ingroup typeEnum
 *  Specifies the type of addresses (MAC, IP)
 *  used for a virtual network.
 **************************************************/
typedef enum
{
    /** Virtual network uses MAC addresses. */
    FM_VN_ADDR_TYPE_MAC,

    /** Virtual network uses IP addresses. */
    FM_VN_ADDR_TYPE_IP,

} fm_vnAddressType;


/**************************************************/
/** \ingroup typeStruct
 *  Defines a union of address types.
 **************************************************/
typedef union _fm_vnAddress
{
    /** MAC address. */
    fm_macaddr macAddress;

    /** IP address. */
    fm_ipAddr  ipAddress;

} fm_vnAddress;


/**************************************************/
/** \ingroup typeStruct
 *  Describes a virtual network.
 *  Referenced by ''fmCreateVN'' and ''fmUpdateVN''.
 **************************************************/
typedef struct _fm_vnDescriptor
{
    /** Internal ID used for this virtual network. For FM6000 (at the
     *  present time) this value must match a VLAN reserved for use for
     *  internal tunneling purposes; i.e., no ports should be members of
     *  this VLAN. In the future, it is expected that this VLAN-internal ID
     *  linkage will be removed. For FM10000, this value is the VSI value
     *  (1-255) for this virtual (overlay) network.  */
    fm_uint          internalId;

    /** VLAN number for this virtual/overlay network. Used in FM10000. */
    fm_uint16        vlan;

    /** Tunneling mode. */
    fm_vnMode        mode;

    /** Address types used for this virtual network. */
    fm_vnAddressType addressType;

    /** Source IP address. Only used in vswitch-offload mode. If zero, the API
     *  will assume that the application will set the SIP for this overlay
     *  network's VSI using switch attribute ''FM_SWITCH_VSI_DATA''. */
    fm_ipAddr        sip;

    /** Whether broadcast & flooding support is required for this virtual
     *  network. TRUE to enable broadcast/flooding, FALSE to disable support. */
    fm_bool          bcastFlooding;

} fm_vnDescriptor;




/**************************************************/
/** \ingroup typeEnum
 *  Identifies an attribute of a virtual network
 *  tunnel. Referenced by ''fmSetVNTunnelAttribute''
 *  and ''fmGetVNTunnelAttribute''.
 **************************************************/
typedef enum
{
    /** Type fm_ipAddr: The IP Address for the local end of this tunnel.
     *
     *  \chips FM6000, FM10000 */
    FM_VNTUNNEL_ATTR_LOCAL_IP,

    /** Type fm_ipAddr: The IP Address for the remote end of this tunnel.
     *  Note: This attribute is considered as part of a tuple consisting
     *  of VRID and REMOTE_IP. REMOTE_IP may be changed without touching VRID,
     *  but if VRID is changed, REMOTE_IP must be updated after VRID is changed
     *  in order for the hardware to be updated.
     *
     *  \chips FM6000, FM10000 */
    FM_VNTUNNEL_ATTR_REMOTE_IP,

    /** Type fm_int: The virtual router associated with this tunnel.
     *  Note: This attribute is considered as part of a tuple consisting
     *  of VRID and REMOTE_IP. REMOTE_IP may be changed without touching VRID,
     *  but if VRID is changed, REMOTE_IP must be updated after VRID is changed
     *  in order for the hardware to be updated.
     *
     *  \chips FM6000, FM10000 */
    FM_VNTUNNEL_ATTR_VRID,

    /** Type fm_int: Tunnel Traffic Identifer. This identifier is used
     *  internally by the switch chip. It defaults to the tunnel ID + 1, or
     *  to the next available index in the case of FM6000 devices.
     *  Applications may change this if desired. Note that if an application
     *  specifies a traffic identifier for any tunnel, it may need to
     *  specify identifiers for all tunnels, or conflicts may arise between
     *  automatically-selected identifiers for some tunnels and
     *  application-specified identifiers for other tunnels.
     *
     *  \chips FM6000 */
    FM_VNTUNNEL_ATTR_TRAFFIC_IDENTIFIER,

    /** Type fm_int: Multicast Group used with this tunnel.
     *
     *  \chips FM6000 */
    FM_VNTUNNEL_ATTR_MCAST_GROUP,

    /** Type fm_macaddr: Destination MAC Address used with this multicast
     *  tunnel.
     *
     *  \chips FM6000 */
    FM_VNTUNNEL_ATTR_MCAST_DMAC,

    /** Type fm_vnTunnelType: The tunnel type for this tunnel. This is a
     *  read-only attribute.
     *
     *  \chips FM6000, FM10000 */
    FM_VNTUNNEL_ATTR_TUNNEL_TYPE,

    /** Type fm_uint: Specifies the TTL value to be used when building an outer
     *  frame during encapsulation. Valid values are 0 through 255. A value of
     *  zero means to use the TTL value from the ingress frame and decrement it.
     *  All other values represent the actual TTL value to be used. Default value
     *  is zero. A value of ~0 means to use the default TTL value provided in the
     *  ''outerTTL'' field of structure ''fm_vnConfiguration'' through a call to
     *  ''fmConfigureVN''.
     *
     *  \chips FM10000 */
    FM_VNTUNNEL_ATTR_ENCAP_TTL,

    /** UNPUBLISHED: For internal use only. */
    FM_VNTUNNEL_ATTR_MAX       /* Must be last */

} fm_vnTunnelAttrType;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_vnConfiguration'' to specify the
 * checksum options for different frame types.
 **************************************************/
typedef enum
{
    /** Trap those frames. */
    FM_VN_CHECKSUM_TRAP = 0,

    /** No need to compute checksum. VXLAN/NGE tunnel header will be set to
     *  a zero checksum at exit. */
    FM_VN_CHECKSUM_ZERO,

    /** Compute checksum from entire packet starting after FTAG and stopping
     *  at end of packet. */
    FM_VN_CHECKSUM_COMPUTE,

    /** Recover checksum from UDP/TCP header. This checksum will be used for
     *  computing VXLAN/NGE checksum. If the checksum is zero, then TE will
     *  compute the outer checksum over the entire IP packet. */
    FM_VN_CHECKSUM_HEADER,

    /** UNPUBLISHED: For internal use only. */
    FM_VN_CHECKSUM_MAX

} fm_vnChecksumAction;


/**************************************************/
/** \ingroup typeStruct
 *  Describes the configuration parameters for the Virtual Networking API.
 *  Referenced by ''fmConfigureVN''.
 *
 * \chips FM10000
 *
 **************************************************/
typedef struct _fm_vnConfiguration
{
    /** Parser Deep Inspection Configuration Index. Specifies which DI config
     *  register should be used for configuring VN deep inspection. Valid values
     *  are -1 through 5. -1 means do not configure deep inspection. 0 through 5
     *  specify the actual register index value to use. */
    fm_int              deepInspectionCfgIndex;

    /** Encapsulation Checksum Actions for non-IP frames. Default value is
     *  ''FM_VN_CHECKSUM_TRAP''. */
    fm_vnChecksumAction nonIP;

    /** Encapsulation Checksum Actions for non-TCP or non-UDP IP frames. Default
     *  value is ''FM_VN_CHECKSUM_TRAP''. */
    fm_vnChecksumAction nonTcpUdp;

    /** Encapsulation Checksum Actions for TCP or UDP frames. Default value is
     *  ''FM_VN_CHECKSUM_HEADER''. */
    fm_vnChecksumAction tcpOrUdp;

    /** Outer checksum Validation for VXLAN and Geneve decapsulation. Choices are:
     *  \lb\lb
     *  FM_DISABLED: The outer checksum is not validated in any way.    \lb
     *  FM_ENABLED:  The outer checksum is validated for VXLAN and NGE if and only
     *  if the checksum is not 0. If the checksum is invalid, then the packet is
     *  trapped. */
    fm_bool             outerChecksumValidation;

    /** Outer TTL value to use when encapsulating frames. \lb
     *  Valid values are 0255, with 0 being the default value. \lb\lb
     *  If zero is used, the TTL value from the frame being encapsulated will be
     *  used as the outer frame's TTL value. However, if L2 frames are being
     *  encapsulated, there is no original TTL value to use, and zero will then be
     *  written as the outer TTL. Therefore, for proper TTL operation, applications
     *  that expect to encapsulate L2 frames should set this field to a non-zero
     *  value. Note that this will then also override the L3 behavior, and the
     *  original TTL value will not be used. */
    fm_int              outerTTL;

} fm_vnConfiguration;


fm_status fmConfigureVN(fm_int sw, fm_vnConfiguration *config);

fm_status fmCreateVN(fm_int           sw,
                     fm_uint32        vsId,
                     fm_vnDescriptor *descriptor);

fm_status fmDeleteVN(fm_int sw, fm_uint32 vsId);

fm_status fmUpdateVN(fm_int           sw,
                     fm_uint32        vsId,
                     fm_vnDescriptor *descriptor);

fm_status fmCreateVNTunnel(fm_int          sw,
                           fm_vnTunnelType tunnelType,
                           fm_int *        tunnelId);

fm_status fmDeleteVNTunnel(fm_int sw, fm_int tunnelId);

fm_status fmSetVNTunnelAttribute(fm_int              sw,
                                 fm_int              tunnelId,
                                 fm_vnTunnelAttrType attr,
                                 void *              value);

fm_status fmGetVNTunnelAttribute(fm_int              sw,
                                 fm_int              tunnelId,
                                 fm_vnTunnelAttrType attr,
                                 void *              value);

fm_status fmGetVNList(fm_int           sw,
                      fm_int           maxVNs,
                      fm_int *         numVNs,
                      fm_uint32 *      vsidList,
                      fm_vnDescriptor *descriptorList);

fm_status fmGetVNFirst(fm_int           sw,
                       fm_int *         searchToken,
                       fm_uint32 *      vsId,
                       fm_vnDescriptor *descriptor);

fm_status fmGetVNNext(fm_int           sw,
                      fm_int *         searchToken,
                      fm_uint32 *      vsId,
                      fm_vnDescriptor *descriptor);

fm_status fmGetVNTunnelList(fm_int  sw,
                            fm_int  maxTunnels,
                            fm_int *numTunnels,
                            fm_int *tunnelIds);

fm_status fmGetVNTunnelFirst(fm_int  sw,
                             fm_int *searchToken,
                             fm_int *tunnelId);

fm_status fmGetVNTunnelNext(fm_int  sw,
                            fm_int *searchToken,
                            fm_int *tunnelId);

fm_status fmAddVNRemoteAddress(fm_int        sw,
                               fm_uint32     vni,
                               fm_int        tunnelId,
                               fm_vnAddress *addr);

fm_status fmDeleteVNRemoteAddress(fm_int        sw,
                                  fm_uint32     vni,
                                  fm_int        tunnelId,
                                  fm_vnAddress *addr);

fm_status fmAddVNRemoteAddressMask(fm_int        sw,
                                   fm_uint32     vni,
                                   fm_int        tunnelId,
                                   fm_vnAddress *baseAddr,
                                   fm_vnAddress *addrMask);

fm_status fmDeleteVNRemoteAddressMask(fm_int        sw,
                                      fm_uint32     vni,
                                      fm_vnAddress *baseAddr,
                                      fm_vnAddress *addrMask);

fm_status fmAddVNLocalPort(fm_int sw, fm_uint32 vni, fm_int port);

fm_status fmDeleteVNLocalPort(fm_int sw, fm_uint32 vni, fm_int port);

fm_status fmAddVNVsi(fm_int sw, fm_uint32 vni, fm_int vsi);

fm_status fmDeleteVNVsi(fm_int sw, fm_uint32 vni, fm_int vsi);


#endif

