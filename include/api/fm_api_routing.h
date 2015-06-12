/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm_api_routing.h
 * Creation Date:   February 7, 2007
 * Description:     Header file for routing services.
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

#ifndef __FM_FM_API_ROUTING_H
#define __FM_FM_API_ROUTING_H


/****************************************************************************/
/** Router Debug Dump Flags
 * \ingroup constRouterDbgDump
 * \page routerDbgDump
 *
 * Flags used to identify which routing tables to dump during a call to
 * ''fmDbgDumpRouteTables''.
 ****************************************************************************/
/** \ingroup constRouterDbgDump
 * @{ */

/** Dump Internal ARP tables. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_INTERNAL_ARP      0x00000001

/** Dump Hardware ARP tables. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_HARDWARE_ARP      0x00000002

/** Dump Unicast Route Information. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_UNICAST_ROUTES    0x00000004

/** Dump Multicast Route Information. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_MULTICAST_ROUTES  0x00000008

/** Dump All Route Information. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_ALL_ROUTES        0x0000000C

/** Dump TCAM Slice Table Information. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_TCAM_TABLES       0x00000010

/** Dump Hardware FFU Tables. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_FFU_HW_TABLES     0x00000020

/** Dump Route Prefix List Tables. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_RT_PREFIX_LISTS   0x00000040

/** Dump Routes in TCAM Slice and Row Order. 
 *
 *  \chips  FM4000, FM600, FM100000 */
#define FM_ROUTER_DUMP_BY_SLICE_AND_ROW  0x00000080

/** Reset ARP Used bits when dumping Hardware ARP tables. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_RESET_ARP_USED    0x00000100

/** Dump ACL Hardware FFU Tables. 
*
*  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_ACL_FFU_HW_TABLES 0x00000200

/** Dump ECMP Group Tables.
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_ECMP_TABLES       0x00000400

/** Dump Routes in Prefix Tables.
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ROUTER_DUMP_BY_PREFIX         0x00000800

/** Dump All Routing Tables. */
#define FM_ROUTER_DUMP_ALL               -1

/** @} (end of Doxygen group) */

#define FM_DEFAULT_NEXTHOP_TRAPCODE      0


/****************************************************************************/
/** \ingroup constRouterAttr
 *
 *  Router Attributes, used as an argument to ''fmSetRouterAttribute'' and 
 *  ''fmGetRouterAttribute''.
 *                                                                          \lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_routerAttr
{
    /** Type fm_int: Handling of routed frames whose TTL field is 1 or 0.
     *  See ''Router TTL 1 Trapping Options'' for possible values. Default
     *  value is FM_ROUTER_TTL1_DROP_ALL. 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_TRAP_TTL1 = 0,

    /** Type fm_bool: Indicates if a frame routed back on the same VLAN it
     *  is received on should generate an event to the application giving 
     *  the SIP and the DIP of the frame: FM_ENABLED or FM_DISABLED (default). 
     *  When set to FM_ENABLED, after an event is generated, the
     *  attribute will automatically return to FM_DISABLED. To get
     *  another event, the application must set this attribute to
     *  FM_ENABLED again. This mechanism allows the application to
     *  control the rate at which multiple such events will be
     *  reported.
     *                                                                  \lb\lb
     *  For FM10000, when set to FM_ENABLED, instead of an event the aplication
     *  will receive a logged unicast frame routed back on the same VLAN with
     *  trap type FM_TRAPCODE_LOG_ARP_REDIRECT. When set to FM_ENABLED, after
     *  a logged unicast frame routed back on the same VLAN is received, the
     *  attribute will automatically return to FM_DISABLED. To get
     *  another such logged frame, the application must set this attribute to
     *  FM_ENABLED again. This mechanism allows the application to
     *  control the rate at which multiple such frames will be
     *  reported.
     * 
     *  \chips  FM4000, FM6000, FM10000 */    
    FM_ROUTER_TRAP_REDIRECT_EVENT,

    /** Type fm_macaddr: The MAC address of the physical router. Default is
     *  0x000000000000. This MAC address is used for virtual router 0. 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_PHYSICAL_MAC_ADDRESS,

    /** Type fm_macaddr: The Mac address of the virtual router. Default is
     *  0x000000000000.  This MAC address is used for virtual routers 1 -255.
     *  The least significant 8 bits are ignored and are replaced by the
     *  virtual router number, as used in routes, ARPs, etc. Note that the
     *  hardware supports 15 (FM4000), 7 (FM6000) or up to 13 (FM10000) virtual
     *  routers (in addition to the physical router). This means that
     *  the application may number its virtual routers with any numbers
     *  between 1 and 255, but may not have more than 15 (FM4000), 7 (FM6000)
     *  or 13 (FM10000) virtual routers at any one time. Note also that total
     *  number of routers (physical and virtual) supported by FM10000 is
     *  limited by the value of ''api.FM10000.ffu.mapMAC.reservedForRouting''.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_VIRTUAL_MAC_ADDRESS,

    /** Type fm_macaddr: The second Mac address of the virtual router.
     *  Default is 0x000000000000. This MAC address may be used for virtual
     *  routers 1 -255. In order to configure a virtual router to use
     *  the second MAC address its MAC mode needs to be updated by
     *  ''fmSetRouterMacMode'' (see ''fm_routerMacMode'' for possible MAC mode
     *  values). The least significant 8 bits of the MAC address are ignored
     *  and are replaced by the virtual router number, as used in routes, ARPs,
     *  etc. Note that the hardware supports 15 (FM4000), 7 (FM6000) or up to
     *  13 (FM10000) virtual routers (in addition to the physical router).
     *  This means that the application may number its virtual routers with
     *  any numbers between 1 and 255, but may not have more than 15 (FM4000),
     *  7 (FM6000) or 13 (FM10000) virtual routers at any one time. Note also
     *  that total number of routers (physical and virtual) supported by
     *  FM10000 is limited by the value of
     *  ''api.FM10000.ffu.mapMAC.reservedForRouting''. 
     *
     *  \chips  FM6000, FM10000 */
    FM_ROUTER_VIRTUAL_MAC_ADDRESS_2,

    /** Type fm_bool: Trap frames with IP options: FM_ENABLED or
     *  FM_DISABLED (default).
     *                                                                  \lb\lb
     *  Use of this attribute is deprecated in favor of using the
     *  ''FM_IP_OPTIONS_DISPOSITION'' switch attribute, which produces
     *  the same effect.
     *                                                                  \lb\lb
     *  Note that to use this attribute, one also needs to configure the
     *  ''FM_PORT_PARSER_FLAG_OPTIONS'' port attribute on the
     *  relevant ports to appropriately flag the relevant options. 
     *
     *  \chips  FM4000, FM10000 */
    FM_ROUTER_TRAP_IP_OPTIONS,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTER_ATTRIBUTE_MAX

};


/****************************************************************************/
/** \ingroup constRouterTTL1
 *
 *  Set of possible enumerated values for the ''FM_ROUTER_TRAP_TTL1'' router
 *  attribute. Indicates what to do with routed frames that have a TTL value 
 *  of zero or one.
 ****************************************************************************/

enum _fm_routerTTL1
{
    /** Silently drop all frames (default). 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_TTL1_DROP_ALL = 0,

    /** Trap unicast frames and log multicast frames that are ICMP, but drop
     *  all others. 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_TTL1_TRAP_ICMP,

    /** Trap all unicast frames and log all multicast frames. 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_TTL1_TRAP_ALL,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTER_TTL1_MAX

};


/****************************************************************************/
/** \ingroup typeEnum
 *
 * Router state.
 ****************************************************************************/
typedef enum
{
    /** Router is active and all routes attached to this router are
     *  activated. */
    FM_ROUTER_STATE_ADMIN_UP = 0,

    /** Router is not active and all routes attached to this router are
     *  deactivated. */
    FM_ROUTER_STATE_ADMIN_DOWN

} fm_routerState;


/****************************************************************************/
/** \ingroup typeEnum
 *
 * Router MAC mode.
 ****************************************************************************/
typedef enum
{
    /** Router uses the MAC address of the physical router.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_MAC_MODE_PHYSICAL_MAC_ADDRESS = 0,

    /** Router uses the first MAC address of the virtual router
     *  (default for virtual routers).
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1,

    /** Router uses the second MAC address of the virtual router.
     *
     *  \chips  FM6000, FM10000 */
    FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_2,

    /** Router uses both the first and the second MAC addresses of the virtual
     *  router.
     *
     *  \chips  FM6000 */
    FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1_AND_2,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTER_MAC_MODE_MAX

} fm_routerMacMode;


/****************************************************************************/
/** \ingroup typeEnum
 *
 * Route table entry state.
 ****************************************************************************/
typedef enum
{
    /** The route is active. */
    FM_ROUTE_STATE_UP = 0,

    /** The route is administratively set inactive. Note that
     *  this will remove the route from the H/W even if the route
     *  has multiple NextHops (ECMP) */
    FM_ROUTE_STATE_ADMIN_DOWN,

    /** The route is inactivate because the interface is down. */
    FM_ROUTE_STATE_INTERFACE_ADMIN_DOWN,

    /** The route is inactivate because the VLAN is down. */
    FM_ROUTE_STATE_INTERFACE_VLAN_DOWN,

    /** The NextHop of the route is set to active. Can be used
     *  instead of FM_ROUTE_STATE_UP to activate a route when the
     *  route has several NextHops (ECMP) */
    FM_ROUTE_STATE_NEXT_HOP_UP,

    /** The NextHop of the route is set to inactive. Can be used
     *  instead of FM_ROUTE_STATE_DOWN to deactivate a route when the
     *  route has several NextHops (ECMP) */
    FM_ROUTE_STATE_NEXT_HOP_DOWN,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTE_STATE_MAX

} fm_routeState;


/****************************************************************************/
/** \ingroup constRouteAttr
 *  Route Attributes, used as an argument to ''fmSetRouteAttribute'' and 
 *  ''fmGetRouteAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_routeAttr
{
    /** Type fm_int: Whether to log or trap frames to the CPU.  See
     *  ''Route Forward To CPU Options'' for possible values.  Default
     *  value is FM_ROUTE_DONT_FORWARD_TO_CPU. */
    FM_ROUTE_FWD_TO_CPU = 0,

    /** Type fm_int: The ECMP group used for a unicast-ecmp route.  Default
     *  value is passed in as part of the route definition when the route
     *  is added using ''fmAddRoute''. */
    FM_ROUTE_ECMP_GROUP,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTE_ATTRIBUTE_MAX

};


/****************************************************************************/
/** \ingroup constRouteFwdCpu
 *  Route Forward To CPU Options are the set of possible enumerated values 
 *  for the ''FM_ROUTE_FWD_TO_CPU'' route attribute. Indicates whether to 
 *  forward frames to the CPU using either TRAP action or LOG action.
 ****************************************************************************/

enum _fm_routeFwdToCpu
{
    /** Do not forward frames to the CPU (default). */
    FM_ROUTE_FWD_CPU_DONT_FWD = 0,

    /** Forward frames to the CPU using the TRAP action. */
    FM_ROUTE_FWD_CPU_TRAP,

    /** Forward frames to the CPU using the LOG action. */
    FM_ROUTE_FWD_CPU_LOG,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTE_FWD_CPU_MAX

};


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines an IP address.
 ****************************************************************************/

typedef struct _fm_ipAddr
{
    /** IP address in network byte order. For IPv4 use addr[0] only. For
     *  IPv6 addr[0] is the the least significant 32 bits. */
    fm_uint32 addr[4];

    /** Flag indicating if the address is IPv4 (false) or IPv6 (true) */
    fm_bool   isIPv6;

} fm_ipAddr;


/****************************************************************************/
/** \ingroup typeEnum
 *
 * Route Type.
 ****************************************************************************/
typedef enum
{
    /** Unknown Route */
    FM_ROUTE_TYPE_UNKNOWN = 0,

    /** Unicast Route */
    FM_ROUTE_TYPE_UNICAST,

    /** Unicast ECMP Route */
    FM_ROUTE_TYPE_UNICAST_ECMP,

    /** Multicast Address */
    FM_ROUTE_TYPE_MULTICAST,

} fm_routeType;


/****************************************************************************/
/** \ingroup typeEnum
 *
 * Multicast Address Type. Indicates whether the multicast address
 * entry is of type (*,G), (S,G), (*,G,V) or (S,G,V).
 ****************************************************************************/
typedef enum
{
    /** Unknown Multicast Address Type. */
    FM_MCAST_ADDR_TYPE_UNKNOWN,

    /** Multicast Address consisting of a MAC address/VLAN pair. */
    FM_MCAST_ADDR_TYPE_L2MAC_VLAN,

    /** Multicast Address consisting of an IP Destination Address/Mask only:
     *   (*,G). */
    FM_MCAST_ADDR_TYPE_DSTIP,

    /** Multicast Address consisting of an IP Destination
     *  Address/Mask/VLAN pair: (*,G,V). */
    FM_MCAST_ADDR_TYPE_DSTIP_VLAN,

    /** Multicast Address consisting of an IP Destination Address/Mask and
     *  Source IP Destination Address/Mask: (S,G). */
    FM_MCAST_ADDR_TYPE_DSTIP_SRCIP,

    /** Multicast Address consisting of an IP Destination Address/Mask,
     *  Source IP Destination Address/Mask, and VLAN: (S,G,V). */
    FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN

} fm_multicastAddressType;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a multicast L2 MAC address entry used for
 * ''FM_MCAST_ADDR_TYPE_L2MAC_VLAN'' address type.
 ****************************************************************************/

typedef struct _fm_multicastMacAddress
{
    /** Layer 2 Destination MAC Address. */
    fm_macaddr destMacAddress;

    /** VLAN ID (VLAN1). */
    fm_uint16  vlan;

    /** VLAN ID2 (VLAN2). Set this field to zero and VLAN attribute
     *  ''FM_VLAN_FID2_IVL'' to FM_DISABLED (the default) to prevent FID2
     *  from being considered during the MAC address table lookup.
     *  
     *  \chips  FM6000 */
    fm_uint16  vlan2;

} fm_multicastMacAddress;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines multicast route information used for
 * ''FM_MCAST_ADDR_TYPE_DSTIP'' address type.
 ****************************************************************************/

typedef struct _fm_multicastDstIpRoute
{
    /** Destination IP Address. */
    fm_ipAddr dstAddr;

    /** Destination Address Prefix Mask Length. */
    fm_int    dstPrefixLength;

} fm_multicastDstIpRoute;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines multicast route information used for
 * ''FM_MCAST_ADDR_TYPE_DSTIP_VLAN'' address type.
 ****************************************************************************/

typedef struct _fm_multicastDstIpVlanRoute
{
    /** Destination IP Address. */
    fm_ipAddr dstAddr;

    /** Destination Address Prefix Mask Length. */
    fm_int    dstPrefixLength;

    /** VLAN ID. */
    fm_uint16 vlan;

    /** VLAN Prefix Mask Length. */
    fm_uint16 vlanPrefixLength;

} fm_multicastDstIpVlanRoute;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines multicast route information used for
 * ''FM_MCAST_ADDR_TYPE_DSTIP_SRCIP'' address type.
 ****************************************************************************/

typedef struct _fm_multicastDstSrcIpRoute
{
    /** Destination IP Address. */
    fm_ipAddr dstAddr;

    /** Destination Address Prefix Mask Length. */
    fm_int    dstPrefixLength;

    /** Source IP Address. */
    fm_ipAddr srcAddr;

    /** Source Address Prefix Mask Length. */
    fm_int    srcPrefixLength;

} fm_multicastDstSrcIpRoute;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines multicast route information used for
 * ''FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN'' address type.
 ****************************************************************************/

typedef struct _fm_multicastDstSrcIpVlanRoute
{
    /** Destination IP Address. */
    fm_ipAddr dstAddr;

    /** Destination Address Prefix Mask Length. */
    fm_int    dstPrefixLength;

    /** Source IP Address. */
    fm_ipAddr srcAddr;

    /** Source Address Prefix Mask Length */
    fm_int    srcPrefixLength;

    /** VLAN ID. */
    fm_uint16 vlan;

    /** VLAN Mask Length. */
    fm_uint16 vlanPrefixLength;

} fm_multicastDstSrcIpVlanRoute;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a union of multicast addresses.
 ****************************************************************************/

typedef union _fm_multicastAddressInfo
{
    /** Layer 2 Destination MAC Address. Used for address type
     *  ''FM_MCAST_ADDR_TYPE_L2MAC_VLAN''. */
    fm_multicastMacAddress        mac;

    /** Destination IP Address/Mask only. Used for address type
     *  ''FM_MCAST_ADDR_TYPE_DSTIP''. */
    fm_multicastDstIpRoute        dstIpRoute;

    /**  Destination IP Address/Mask and Vlan Number/Mask. Used for address
     *  type ''FM_MCAST_ADDR_TYPE_DSTIP_VLAN''. */
    fm_multicastDstIpVlanRoute    dstIpVlanRoute;

    /** Destination IP Address/Mask and Source IP Address/Mask. Used for
     *  address type ''FM_MCAST_ADDR_TYPE_DSTIP_SRCIP''. */
    fm_multicastDstSrcIpRoute     dstSrcIpRoute;

    /** Destination IP Address/Mask, Source IP Address/Mask, and VLAN 
     *  number/Mask. Used for address type 
     *  ''FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN''. */
    fm_multicastDstSrcIpVlanRoute dstSrcIpVlanRoute;

} fm_multicastAddressInfo;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a multicast address entry.
 ****************************************************************************/

typedef struct _fm_multicastAddress
{
    /** Multicast Address Type. Indicates whether the multicast address
     *  entry is of type (*,G), (S,G), (*,G,V) or (S,G,V). */
    fm_multicastAddressType addressType;

    /** Multicast group number. */
    fm_int                  mcastGroup;

    /** Multicast Address Information. */
    fm_multicastAddressInfo info;

} fm_multicastAddress;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a unicast route table entry.
 ****************************************************************************/

typedef struct _fm_unicastRouteEntry
{
    /** Destination IP address */
    fm_ipAddr dstAddr;

    /** Destination IP subnet mask prefix length. */
    fm_int    prefixLength;

    /** Next hop IP address. */
    fm_ipAddr nextHop;

    /** Next hop Interface. */
    fm_ipAddr interfaceAddr;

    /** Next hop Vlan.  Only used if interfaceAddr == 0 */
    fm_uint16 vlan;

    /** Virtual router identifier. 0 indicates the real router. */
    fm_int    vrid;

} fm_unicastRouteEntry;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a unicast ECMP route table entry.
 ****************************************************************************/
typedef struct _fm_unicastECMPRouteEntry
{
    /** Destination IP address */
    fm_ipAddr dstAddr;

    /** Destination IP subnet mask prefix length. */
    fm_int    prefixLength;

    /** Next hop ECMP Group ID. */
    fm_int    ecmpGroup;

    /** Virtual router identifier. 0 indicates the real router. */
    fm_int    vrid;

} fm_unicastECMPRouteEntry;


/****************************************************************************/
/** \ingroup typeEnum
 * Vrid associated with a Route.
 ****************************************************************************/
typedef enum
{
    /** Indicates that all the virtual routers including the
     *  physical router can share this route entry. */
    FM_ROUTER_ANY = -1,

    /** Indicates a route that will only be a part of the
     *  physical router. */
    FM_PHYSICAL_ROUTER = 0,

} fm_vrid;


/**************************************************/
/** \ingroup typeEnum
 *  Referenced by ''fm_routeAction'', these enumerated 
 *  values specify an action to be taken when a 
 *  routing rule hits. Any data required by the action
 *  will be specified by ''fm_routeAction'' in
 *  a ''fm_routeActionData'' union.
 **************************************************/
typedef enum
{
    /** Route this frame normally.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTE_ACTION_ROUTE,

    /** Drop this frame.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTE_ACTION_DROP,
 
    /** RPF failure route.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTE_ACTION_RPF_FAILURE,

    /** Route with a NOP FFU action. Used by multicast fastdrop routes
     *  but available for use by other routes as needed.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_ROUTE_ACTION_NOP,

    /** Execute a trigger for this frame.
     *
     *  \chips  FM4000, FM10000 */
    FM_ROUTE_ACTION_TRIGGER,

    /** UNPUBLISHED: For internal use only. */
    FM_ROUTE_ACTION_MAX

} fm_routeActionType;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_routeActionData'', this structure 
 *  specifies the data for a trigger route action,  
 *  specific to the action type (''fm_routeActionType'').
 **************************************************/
typedef union _fm_routeActionTrigger
{
    /** Trigger value to be used for this route. */
    fm_byte triggerValue;

    /** Trigger mask to be used for this route. */
    fm_byte triggerMask;

} fm_routeActionTrigger;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_routeAction'', this union 
 *  specifies the data for a route action,  
 *  specific to the action type (''fm_routeActionType'').
 **************************************************/
typedef union _fm_routeActionData
{
    fm_routeActionTrigger trigger;

} fm_routeActionData;


/**************************************************/
/** \ingroup typeStruct
 *  Used by ''fmAddRouteExt'' and ''fmSetRouteAction''
 *  and for the ''FM_MCASTGROUP_ACTION'' multicast
 *  group attribute, this structure specifies a 
 *  complete routing action, both the action type, 
 *  and the type-specific data.
 **************************************************/
typedef struct _fm_RouteAction
{
    /** Which action to perform. */
    fm_routeActionType action;

    /** Data related to the action, if needed. */
    fm_routeActionData data;

} fm_routeAction;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a union containing unicast and multicast routing information.
 ****************************************************************************/

typedef union _fm_routeEntryData
{
    /** Unicast entry information. */
    fm_unicastRouteEntry unicast;

    /** Unicast ECMP entry information. */
    fm_unicastECMPRouteEntry unicastECMP;

    /** Multicast entry information. */
    fm_multicastAddress  multicast;

} fm_routeEntryData;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a route table entry. The API checks the destination
 * address to automatically determine if this is a multicast unicast entry.
 ****************************************************************************/

typedef struct _fm_routeEntry
{
    /** Route Type. */
    fm_routeType      routeType;

    /** Route Entry Information */
    fm_routeEntryData data;

} fm_routeEntry;


/****************************************************************************
 * Used to specify an IPv6 stateless autoconfig ARP entry. This could be used
 * as the macAddr field of the ''fm_arpEntry'' structure.
 ****************************************************************************/
#define FM_MAC_STATELESS_AUTOCONFIG  FM_LITERAL_64(0x0015ed000000)

/** Used to indicate that an interface has no valid vlan.
 *  \ingroup constSystem */
#define FM_INVALID_INTERFACE_VLAN  ( (fm_uint16) 0xffff )

/** A legacy synonym for ''FM_INVALID_INTERFACE_VLAN''.
 *  \ingroup macroSynonym */
#define FM_INVALID_VLAN FM_INVALID_INTERFACE_VLAN


fm_status fmSetRouterAttribute(fm_int sw,
                               fm_int attr,
                               void * value);
fm_status fmGetRouterAttribute(fm_int sw,
                               fm_int attr,
                               void * value);
fm_status fmCreateVirtualRouter(fm_int sw,
                                fm_int vrid);
fm_status fmDeleteVirtualRouter(fm_int sw,
                                fm_int vrid);
fm_status fmGetVirtualRouterList(fm_int  sw,
                                 fm_int *numVrids,
                                 fm_int *vridList,
                                 fm_int  max);
fm_status fmGetVirtualRouterFirst(fm_int  sw,
                                  fm_int *firstVrid);
fm_status fmGetVirtualRouterNext(fm_int  sw,
                                 fm_int  currentVrid,
                                 fm_int *nextVrid);
fm_status fmSetRouterState(fm_int         sw,
                           fm_int         vrid,
                           fm_routerState state);
fm_status fmGetRouterState(fm_int          sw,
                           fm_int          vrid,
                           fm_routerState *state);
fm_status fmSetRouterMacMode(fm_int           sw,
                             fm_int           vrid,
                             fm_routerMacMode mode);
fm_status fmGetRouterMacMode(fm_int            sw,
                             fm_int            vrid,
                             fm_routerMacMode *mode);
fm_status fmAddRoute(fm_int         sw,
                     fm_routeEntry *route,
                     fm_routeState  state);
fm_status fmAddRouteExt(fm_int          sw,
                        fm_routeEntry * route,
                        fm_routeState   state,
                        fm_routeAction *action);
fm_status fmDeleteRoute(fm_int         sw,
                        fm_routeEntry *route);
fm_status fmReplaceRouteECMP(fm_int         sw,
                             fm_routeEntry *oldRoute,
                             fm_routeEntry *newRoute);
fm_status fmSetRouteState(fm_int         sw,
                          fm_routeEntry *route,
                          fm_routeState  state);
fm_status fmGetRouteState(fm_int         sw,
                          fm_routeEntry *route,
                          fm_routeState *state);
fm_status fmSetRouteAction(fm_int          sw,
                           fm_routeEntry * route,
                           fm_routeAction *action);
fm_status fmGetRouteAction(fm_int         sw,
                          fm_routeEntry * route,
                          fm_routeAction *action);
fm_status fmGetRouteList(fm_int         sw,
                         fm_int *       numRoutes,
                         fm_routeEntry *routeList,
                         fm_int         max);
fm_status fmGetRouteFirst(fm_int         sw,
                          fm_voidptr *   searchToken,
                          fm_routeEntry *firstRoute);
fm_status fmGetRouteNext(fm_int         sw,
                         fm_voidptr *   searchToken,
                         fm_routeEntry *nextRoute);

fm_bool fmIsRouteEntryUnicast(fm_routeEntry *route);
fm_bool fmIsRouteEntryMulticast(fm_routeEntry *route);
fm_bool fmIsRouteEntrySGMulticast(fm_routeEntry *route);
fm_int fmCompareIPAddresses(fm_ipAddr *first, fm_ipAddr *second);
fm_int fmCompareRoutes(fm_routeEntry *first, fm_routeEntry *second);
fm_int fmCompareEcmpRoutes(fm_routeEntry *first, fm_routeEntry *second);
void fmDbgConvertIPAddressToString(const fm_ipAddr *ipAddr, char *textOut);
void fmDbgBuildMulticastDescription(fm_multicastAddress *mcast, fm_text textOut);
void fmDbgBuildRouteDescription(fm_routeEntry *route, char *textOut);
fm_status fmDbgDumpRouteStats(fm_int sw);
fm_status fmDbgDumpRouteTables(fm_int sw, fm_int flags);
fm_bool fmDbgConvertStringToIPAddress(const fm_char *ipString,
                                      fm_bool        forceIPv6,
                                      fm_ipAddr *    ipAddr);
fm_status fmDbgDumpRouteLookupTrees(fm_int sw, fm_int vrid);
fm_status fmDbgDumpRouteForIP(fm_int sw, fm_int vrid, fm_text ipAddr);
fm_status fmDbgValidateRouteTables(fm_int sw);
void fmDbgTestRouteMask(fm_char *               route,
                        fm_int                  prefix,
                        fm_routeType            routeType,
                        fm_multicastAddressType mcastType);
void fmGetRouteDestAddress(fm_routeEntry *route, fm_ipAddr *destAddr);
void fmGetRouteMcastSourceAddress(fm_routeEntry *route, fm_ipAddr *srcAddr);
fm_status fmDbgGetRouteCount(fm_int sw, fm_int *countPtr);
fm_status fmSetRouteAttribute(fm_int         sw,
                              fm_routeEntry *route,
                              fm_int         attr,
                              void *         value);
fm_status fmGetRouteAttribute(fm_int         sw,
                              fm_routeEntry *route,
                              fm_int         attr,
                              void *         value);

#endif /* __FM_FM_API_ROUTING_H */
