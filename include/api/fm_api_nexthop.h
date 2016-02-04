/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_nexthop.h
 * Creation Date:   August 5, 2013
 * Description:     Contains constants and functions used to support trigger.
 *
 * Copyright (c) 2007 - 2016, Intel Corporation
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

#ifndef __FM_FM10000_API_NEXTHOP_H
#define __FM_FM10000_API_NEXTHOP_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines an ARP table entry.
 ****************************************************************************/

typedef struct _fm_arpEntry
{
    /** The destination IP address */
    fm_ipAddr  ipAddr;

    /** The interface on which the destination IP address may be found. */
    fm_int     interface;

    /** Vlan to use for this ARP entry.  Only used if interface == -1 */
    fm_uint16  vlan;

    /** The MAC address of the next hop. Set this member to
     *  FM_MAC_STATELESS_AUTOCONFIG to use IPv6 Stateless Autoconfiguration
     *  functionality. */
    fm_macaddr macAddr;

} fm_arpEntry;




/****************************************************************************/
/** \ingroup typeStruct
 *
 *  Used as an argument to ''fmGetARPEntryInfo'' for returning information 
 *  about an ARP table entry.
 ****************************************************************************/

typedef struct _fm_arpEntryInfo
{
    /** The ARP table entry. */
    fm_arpEntry arp;

    /** Indicates whether the ARP entry has been accessed. */
    fm_bool     used;

} fm_arpEntryInfo;


/****************************************************************************/
/** \ingroup typeEnum
 *  Referenced by ''fm_ecmpNextHop'', these are the types of ECMP Group 
 *  next-hops.
 ****************************************************************************/
typedef enum 
{
    /** ARP-style next-hop entry.
     *
     * \chips FM4000, FM6000, FM10000 */
    FM_NEXTHOP_TYPE_ARP,

    /** Raw narrow next-hop entry.
     *
     *  \chips  FM6000 */
    FM_NEXTHOP_TYPE_RAW_NARROW,

    /** Raw wide next-hop entry.
     *
     *  \chips  FM6000 */
    FM_NEXTHOP_TYPE_RAW_WIDE,

    /** Drop next-hop entry.
     *
     *  \chips FM4000, FM6000, FM10000 */
    FM_NEXTHOP_TYPE_DROP,

    /** DMAC next-hop entry.
     *
     *  \chips FM6000, FM10000 */
    FM_NEXTHOP_TYPE_DMAC,

    /** MPLS ARP next-hop entry. Only valid when used with an MPLS ECMP Group
     *  (see the ''fm_ecmpGroupInfo'' structure).
     *
     *  \chips FM6000 */
    FM_NEXTHOP_TYPE_MPLS_ARP,

    /** Tunnel next-hop entry.
     *
     *  \chips FM10000 */
    FM_NEXTHOP_TYPE_TUNNEL,

    /** Virtual-Network (VXLAN, NVGRE, Geneve) Tunnel next-hop entry.
     *
     *  \chips FM10000 */
    FM_NEXTHOP_TYPE_VN_TUNNEL,

    /** Logical Port (GLORT) next-hop entry.
     *
     *  \chips FM10000 */
    FM_NEXTHOP_TYPE_LOGICAL_PORT,

    /** UNPUBLISHED: For internal use only. */
    FM_NEXTHOP_TYPE_MAX

} fm_ecmpNextHopType;


/****************************************************************************/
/** \ingroup typeEnum
 *
 * \chips FM6000
 *
 * MPLS Actions that can be taken via a next-hop.
 * Referenced by ''fm_mplsArpNextHop''.
 ****************************************************************************/
typedef enum
{
    /** Push 1 MPLS Label */
    FM_MPLS_NEXT_HOP_ACTION_PUSH_1_LABEL,

    /** Push 2 MPLS Labels */
    FM_MPLS_NEXT_HOP_ACTION_PUSH_2_LABELS,

    /** Pop 1 MPLS Label */
    FM_MPLS_NEXT_HOP_ACTION_POP_1_LABEL,

    /** Pop 2 MPLS Labels */
    FM_MPLS_NEXT_HOP_ACTION_POP_2_LABELS,

    /** Push 1 Pop 1 MPLS Label */
    FM_MPLS_NEXT_HOP_ACTION_PUSH_1_POP_1_LABEL,

    /** Push 1 Pop 2 MPLS Labels */
    FM_MPLS_NEXT_HOP_ACTION_PUSH_1_POP_2_LABEL,

    /** Push 2 Pop 1 MPLS Labels */
    FM_MPLS_NEXT_HOP_ACTION_PUSH_2_POP_1_LABEL,

    /** Push 2 Pop 2 MPLS Labels */
    FM_MPLS_NEXT_HOP_ACTION_PUSH_2_POP_2_LABEL,

    /** UNPUBLISHED: For internal use only. */
    FM_MPLS_NEXTHOP_ACTION_MAX

} fm_mplsNextHopAction;

/**************************************************/
/** \ingroup typeStruct
 * Next hop specification, used as an argument to 
 * many of the ''ECMP Group Management'' functions.
 *
 * Use of this structure is required when the
 * next-hop type is ''FM_NEXTHOP_TYPE_ARP'' or
 * ''FM_NEXTHOP_TYPE_DROP''.
 **************************************************/
typedef struct _fm_nextHop
{
    /** Next hop Address
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_ipAddr addr;

    /** Next hop Interface. 
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_ipAddr interfaceAddr;

    /** Next hop Vlan.  Only used if interfaceAddr is all zeros. 
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    fm_uint16 vlan;

    /** The trap code to use if the nexthop entry is unresolved,
     *  FM_TRAPCODE_L3_ROUTED_NO_ARP_0 (default) or
     *  FM_TRAPCODE_L3_ROUTED_NO_ARP_1. Specify 
     *  FM_DEFAULT_NEXTHOP_TRAPCODE to use the default value.
     *  
     *  \chips  FM6000, FM10000 */
    fm_uint32 trapCode;

} fm_nextHop;


/**************************************************/
/** \ingroup typeStruct
 *  DMAC next-hop data structure. Referenced 
 *  by ''fm_ecmpNextHopData'' when the next-hop type 
 *  is ''FM_NEXTHOP_TYPE_DMAC''.
**************************************************/
typedef struct _fm_dmacNextHop
{
     /** DMAC address. */
     fm_macaddr dmac;

} fm_dmacNextHop;


/**************************************************/
/** \ingroup typeStruct 
 * MPLS ARP next-hop specification, used as an argument
 * to many of the ''ECMP Group Management'' functions.
 *
 * Use of this structure is required when the
 * next-hop type is ''FM_NEXTHOP_TYPE_MPLS_ARP''.
 **************************************************/
typedef struct _fm_mplsArpNextHop
{
    /** Next-hop Address. */
    fm_ipAddr            addr;

    /** Next-hop Interface. */
    fm_ipAddr            interfaceAddr;

    /** Next-hop Vlan.  Only used if interfaceAddr is all zeroes.  */
    fm_uint16            vlan;

    /** The trap code to use if the nexthop entry is unresolved,
     *  FM_TRAPCODE_L3_ROUTED_NO_ARP_0 (default), or
     *  FM_TRAPCODE_L3_ROUTED_NO_ARP_1. Specify
     *  FM_DEFAULT_NEXTHOP_TRAPCODE to use the default value. */
    fm_uint32            trapCode;

    /** MPLS Action to be taken. */
    fm_mplsNextHopAction mplsAction;

    /** First MPLS Label, if mplsAction is add 1 or 2 MPLS labels. 24-bits,
     *  containing the MPLS Label, the EXP field, and the Bottom-of-Stack bit:\lb
     *  Bits 23-4: MPLS Label (20 bits)                                     \lb
     *  Bits  3-1: EXP Field                                                \lb
     *  Bit     0: Bottom-Of-Stack                                          \lb
     *                                                                      \lb
     *  Note that the EXP field shall be set to 0 when used in
     *  conjuncture with the ''FM_ACL_ACTIONEXT_SET_MPLS_TC''
     *  action. The latter can be used to set the egress MPLS
     *  Traffic Class, i.e. EXP */
    fm_uint32            mplsLabel1;

    /** Second MPLS Label, if mplsAction is add 2 MPLS labels. 24-bits,
     *  containing the MPLS Label, the EXP field, and the Bottom-of-Stack bit:\lb
     *  Bits 23-4: MPLS Label (20 bits)                                     \lb
     *  Bits  3-1: EXP Field                                                \lb
     *  Bit     0: Bottom-Of-Stack                                          \lb
     *                                                                      \lb
     *  Note that the EXP field shall be set to 0 when used in
     *  conjuncture with the ''FM_ACL_ACTIONEXT_SET_MPLS_TC''
     *  action. The latter can be used to set the egress MPLS
     *  Traffic Class, i.e. EXP */
     fm_uint32            mplsLabel2;

     /** Unlabeled Ether-type. This ether-type will be inserted into the egress
      *  frame in place of the MPLS ether-type and MPLS labels if the last
      *  MPLS labels are being removed from the frame. If zero, 0x0800 (the IPv4
      *  ether-type) will be inserted. */
     fm_uint32           unlabeledEtherType;

} fm_mplsArpNextHop;


/**************************************************/
/** \ingroup typeStruct
 *  Tunnel next-hop data structure. Referenced 
 *  by ''fm_ecmpNextHopData'' when the next-hop type 
 *  is ''FM_NEXTHOP_TYPE_TUNNEL''.
**************************************************/
typedef struct _fm_tunnelNextHop
{
     /** Tunnel Group. */
     fm_int tunnelGrp;

     /** Tunnel Rule. */
     fm_int tunnelRule;

} fm_tunnelNextHop;


/**************************************************/
/** \ingroup typeStruct
 *  Virtual-Network Tunnel next-hop data structure.
 *  Referenced by ''fm_ecmpNextHopData'' when the
 *  next-hop type is ''FM_NEXTHOP_TYPE_VN_TUNNEL''.
**************************************************/
typedef struct _fm_vnTunnelNextHop
{
     /** Tunnel ID. */
     fm_int    tunnel;

     /** TRUE for ENCAP, FALSE for DECAP */
     fm_bool   encap;

     /** Virtual-Network ID. */
     fm_uint32 vni;

} fm_vnTunnelNextHop;


/**************************************************/
/** \ingroup typeStruct
 *  Raw narrow next-hop data structure. Referenced 
 *  by ''fm_ecmpNextHopData'' when the next-hop type 
 *  is ''FM_NEXTHOP_TYPE_RAW_NARROW''.
**************************************************/
typedef struct _fm_rawNarrowNextHop
{
     /** 64-bit raw value for a narrow next-hop record. */
     fm_uint64 value;

} fm_rawNarrowNextHop;


/** The number of 64-bit values in a wide next-hop record.
 *  \ingroup constSystem */
#define FM_RAW_WIDE_NEXTHOP_SIZE    2


/**************************************************/
/** \ingroup typeStruct
 *  Raw wide next-hop data structure. Referenced 
 *  by ''fm_ecmpNextHopData'' when the
 *  next-hop type is ''FM_NEXTHOP_TYPE_RAW_WIDE''.
**************************************************/
typedef struct _fm_rawWideNextHop
{
     /** Array of 64-bit raw values for a wide next-hop record. */
     fm_uint64 values[FM_RAW_WIDE_NEXTHOP_SIZE];

} fm_rawWideNextHop;


/**************************************************/
/** \ingroup typeStruct
 *  Logical port next-hop data structure.
 *  Referenced by ''fm_ecmpNextHopData'' when the
 *  next-hop type is ''FM_NEXTHOP_TYPE_LOGICAL_PORT''.
**************************************************/
typedef struct _fm_portNextHop
{
     /** Logical port number. */
     fm_int    logicalPort;

     /** TRUE for the frame to be treated as routed, FALSE otherwise.
      *
      * Note that setting this field to TRUE is only relevant for IP frames.
      * In that case, the ingress IP frame that hits the ACL (or Flow) is
      * always 'routed' to the targeted logical port, irrespective of its
      * DMAC (router MAC or not), vlan (routable or not), and port (routable
      * or not). Modified routed fields are: SMAC (to router MAC) and TTL
      * (decremented). DMAC is left as in original frame. Non-IP frames are
      * simply switched to the logical port. */
     fm_bool   routed;

     /** MTU index to use when routed flag is set to TRUE. */
     fm_int    mtuIndex;

     /** Router Id to use when routed flag is set to TRUE. 
      *
      *  ''FM_ROUTER_ID_NO_REPLACEMENT'' is used to specify that the
      *  pipeline's Router ID derived from mapper should be retained. If
      *  RouterId is between 1 and 14, then this new RouterID means virtual
      *  router ID and replaces the one derived from the mapper. Value
      *  ''FM_PHYSICAL_ROUTER'' means to replace to physical router. */
     fm_int    routerId;

     /** Egress VLAN ID to use when routed flag is set to TRUE.*/
     fm_uint16 vlan;

} fm_portNextHop;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_ecmpNextHop'', provides the
 *  specific information about a next hop entry.
**************************************************/
typedef union _fm_ecmpNextHopData
{
     /** ARP-style next-hop.
      *
      *  \chips FM4000, FM6000, FM10000 */
     fm_nextHop          arp;

     /** Raw narrow next-hop. The data will be written directly to the
      * hardware without interpretation by the API.
      *
      *  \chips FM6000 */
     fm_rawNarrowNextHop rawNarrow;

     /** Raw wide next-hop. The data will be written directly to the
      * hardware without interpretation by the API.
      *
      *  \chips FM6000 */
     fm_rawWideNextHop   rawWide;

     /** DMAC next-hop.
      *
      *  \chips FM6000, FM10000 */
     fm_dmacNextHop      dmac;

     /** MPLS ARP next-hop. Only valid when used with ECMP groups that have
      *  been created as MPLS ECMP groups.
      *
      *  \chips FM6000 */
     fm_mplsArpNextHop   mplsArp;

     /** Tunnel next-hop.
      *
      *  \chips FM10000 */
     fm_tunnelNextHop    tunnel;

     /** Virtual-Network Tunnel next-hop.
      *
      *  \chips FM10000 */
     fm_vnTunnelNextHop  vnTunnel;


     /** Logical Port next-hop.
      *
      *  \chips FM10000 */
     fm_portNextHop  port;

} fm_ecmpNextHopData;


/**************************************************/
/** \ingroup typeStruct
 *  Generic next-hop specification, used as an argument 
 *  to ''fmSetECMPGroupNextHops'',
 *  ''fmDeleteECMPGroupNextHopsV2'',
 *  ''fmGetECMPGroupNextHopUsedV2'' and
 *  ''fmReplaceECMPGroupNextHopV2''.
 **************************************************/
typedef struct _fm_ecmpNextHop
{
    /** Type of next-hop */
    fm_ecmpNextHopType type;

    /** Raw Data */
    fm_ecmpNextHopData data;

} fm_ecmpNextHop;


/**************************************************/
/** \ingroup typeStruct
 *  ECMP Group characteristics information, used when
 *  creating an ECMP group with ''fmCreateECMPGroupV2''.
 *                                                                      \lb\lb
 *  Note that this structure should be memset to
 *  zero prior to setting each structure member, to
 *  facilitate backward compatibility if new members 
 *  are added to this structure in a future version
 *  of the API.
 **************************************************/
typedef struct _fm_ecmpGroupInfo
{
    /** Whether the ECMP group uses narrow or wide next-hops.
     *  TRUE means that the ECMP group uses wide next-hops.
     *
     *  \chips  FM6000 */
    fm_bool wideNextHops;

    /** The number of next-hop entries in a fixed-size ECMP group.
     *  0 indicates that the group's size is adjustable.
     *  Greater than zero specifies the fixed number of next-hops
     *  used by this ECMP group.
     *
     *  \chips  FM4000, FM6000, FM10000 */
    fm_int  numFixedEntries;

    /** The loopback suppression VLAN ID to store in the next-hop
     *  entry of the ECMP group attached to a multicast group.
     *
     *  \chips  FM6000 */
    fm_int  lbsVlan;

    /** Whether the ECMP group is an MPLS ECMP group. If TRUE, the value of
     *  wideNextHops will be overridden, since all MPLS next-hops are wide
     *  next-hops. Only MPLS Next Hops may be used with an MPLS ECMP group.
     *  Conversely, MPLS Next Hops may not be used with a non-MPLS ECMP group.
     *  
     *  \chips  FM6000 */
    fm_bool isMpls;

} fm_ecmpGroupInfo;


/****************************************************************************/
/** \ingroup constInterfaceAttr
 * Router Interface Attributes, used as an argument to
 * ''fmSetInterfaceAttribute'' and ''fmGetInterfaceAttribute''.
 *                                                                          \lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 ****************************************************************************/
enum _fm_interfaceAttr
{
    /** Type ''fm_interfaceState'': Interface state. See ''fm_interfaceState''
     *  for possible values. Default value is 
     *  ''FM_INTERFACE_STATE_ADMIN_DOWN''. 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_INTERFACE_STATE,

    /** Type fm_uint16: Interface VLAN ID. Default value is "no vlan"
     *  (''FM_INVALID_VLAN'' value). 
     *                                                                  \lb\lb
     *  Note: Normally you will also want to set the VLAN's ''FM_VLAN_ROUTABLE'' 
     *  attribute to FM_ENABLED (by calling ''fmSetVlanAttribute''). 
     *
     *  \chips  FM4000, FM6000, FM10000 */
    FM_INTERFACE_VLAN,

    /** UNPUBLISHED: For internal use only. */
    FM_INTERFACE_ATTRIBUTE_MAX

};

/****************************************************************************/
/** \ingroup typeEnum
 *
 * Router interface states. Set of possible values for the ''FM_INTERFACE_STATE''
 * router interface attribute.
 ****************************************************************************/
typedef enum
{
    /** Interface is active and may be used to route */
    FM_INTERFACE_STATE_ADMIN_UP = 0,

    /** Interface is not active and all routes using this interface are
     *  blocked (default). */
    FM_INTERFACE_STATE_ADMIN_DOWN,

    /** UNPUBLISHED: For internal use only. */
    FM_INTERFACE_STATE_MAX

} fm_interfaceState;


/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/
fm_status fmNextHopAlloc(fm_int sw);
fm_status fmNextHopFree(fm_int sw);
fm_status fmNextHopInit(fm_int sw);
fm_status fmNextHopCleanup(fm_int sw);
fm_status fmCreateECMPGroup(fm_int      sw,
                            fm_int     *pGroupId,
                            fm_int      numNextHops,
                            fm_nextHop *pNextHopList);

fm_status fmCreateECMPGroupV2(fm_int            sw,
                              fm_int *          pGroupId,
                              fm_ecmpGroupInfo *pInfo);
fm_status fmDeleteECMPGroup(fm_int sw, fm_int groupId);
fm_status fmAddECMPGroupNextHops(fm_int      sw,
                                 fm_int      groupId,
                                 fm_int      numNextHops,
                                 fm_nextHop *nextHopList);
fm_status fmAddECMPGroupNextHopsV2(fm_int          sw,
                                   fm_int          groupId,
                                   fm_int          numNextHops,
                                   fm_ecmpNextHop *nextHopList);
fm_status fmAddECMPGroupRawNextHop(fm_int             sw,
                                   fm_int             groupId,
                                   fm_ecmpNextHopType nextHopType,
                                   fm_uint64          value0,
                                   fm_uint64          value1);

fm_status fmDeleteECMPGroupNextHops(fm_int      sw,
                                    fm_int      groupId,
                                    fm_int      numNextHops,
                                    fm_nextHop *nextHopList);
fm_status fmDeleteECMPGroupNextHopsV2(fm_int          sw,
                                      fm_int          groupId,
                                      fm_int          numNextHops,
                                      fm_ecmpNextHop *nextHopList);
fm_status fmReplaceECMPGroupNextHop(fm_int      sw,
                                    fm_int      groupId,
                                    fm_nextHop *oldNextHop,
                                    fm_nextHop *newNextHop);
fm_status fmReplaceECMPGroupNextHopV2(fm_int          sw,
                                      fm_int          groupId,
                                      fm_ecmpNextHop *oldNextHop,
                                      fm_ecmpNextHop *newNextHop);
fm_status fmDeleteECMPGroupRawNextHop(fm_int                sw,
                                      fm_int                groupId,
                                      fm_ecmpNextHopType    nextHopType,
                                      fm_uint64             value0,
                                      fm_uint64             value1);

fm_status fmSetECMPGroupRawNextHop(fm_int               sw,
                                   fm_int               groupId,
                                   fm_int               index,
                                   fm_ecmpNextHopType   nextHopType,
                                   fm_uint64            value0,
                                   fm_uint64            value1);
fm_status fmSetECMPGroupNextHops(fm_int          sw,
                                 fm_int          groupId,
                                 fm_int          firstIndex,
                                 fm_int          numNextHops,
                                 fm_ecmpNextHop *nextHopList);
fm_status fmGetECMPGroupFirst(fm_int sw, fm_int *firstGroupId);
fm_status fmGetECMPGroupNext(fm_int  sw,
                             fm_int  prevGroupId,
                             fm_int *nextGroupId);
fm_status fmGetECMPGroupList(fm_int  sw,
                             fm_int *numGroups,
                             fm_int *groupList,
                             fm_int  max);
fm_status fmGetECMPGroupNextHopFirst(fm_int      sw,
                                     fm_int      groupId,
                                     fm_int *    searchToken,
                                     fm_nextHop *firstNextHop);
fm_status fmGetECMPGroupNextHopNext(fm_int      sw,
                                    fm_int      groupId,
                                    fm_int *    searchToken,
                                    fm_nextHop *nextArp);
fm_status fmGetECMPGroupNextHopList(fm_int      sw,
                                    fm_int      groupId,
                                    fm_int *    numNextHops,
                                    fm_nextHop *nextHopList,
                                    fm_int      max);
fm_status fmGetECMPGroupNextHopUsed(fm_int      sw,
                                    fm_int      groupId,
                                    fm_nextHop *nextHop,
                                    fm_bool*    used,
                                    fm_bool     resetFlag);
fm_status fmGetECMPGroupNextHopUsedV2(fm_int          sw,
                                      fm_int          groupId,
                                      fm_ecmpNextHop *nextHop,
                                      fm_bool*        used,
                                      fm_bool         resetFlag);
fm_status fmGetECMPGroupRouteCount(fm_int      sw,
                                   fm_int      groupId,
                                   fm_int *    routeCountPtr);
fm_status fmGetECMPGroupNextHopIndexRange(fm_int  sw,
                                          fm_int  groupId,
                                          fm_int *firstIndex,
                                          fm_int *lastIndex);
fm_status fmGetNextHopIndexUsed(fm_int   sw,
                                fm_int   index,
                                fm_bool *used,
                                fm_bool  resetFlag);
fm_status fmAddARPEntry(fm_int       sw,
                        fm_arpEntry *arp);
fm_status fmDeleteARPEntry(fm_int       sw,
                           fm_arpEntry *arp);
fm_status fmUpdateARPEntryDMAC(fm_int       sw,
                               fm_arpEntry *arp);
fm_status fmUpdateARPEntryVrid(fm_int       sw,
                               fm_arpEntry *arp,
                               fm_int       vrid);
fm_status fmGetARPEntryList(fm_int       sw,
                            fm_int *     numArps,
                            fm_arpEntry *arpList,
                            fm_int       max);
fm_status fmGetARPEntryFirst(fm_int       sw,
                             fm_voidptr * searchToken,
                             fm_arpEntry *firstArp);
fm_status fmGetARPEntryNext(fm_int       sw,
                            fm_voidptr * searchToken,
                            fm_arpEntry *nextArp);
fm_status fmGetARPEntryInfo(fm_int              sw,
                            fm_arpEntry*        arpEntry,
                            fm_arpEntryInfo*    arpInfo);
fm_status fmDbgDumpArpTable(fm_int sw, fm_bool verbose);
fm_status fmDbgDumpArpHandleTable(fm_int sw,
                                  fm_bool verbose);
fm_status fmDbgPlotArpUsedDiagram(fm_int sw);
fm_status fmGetARPEntryUsed(fm_int          sw,
                            fm_arpEntry*    arp,
                            fm_bool*        used,
                            fm_bool         resetFlag);
fm_status fmRefreshARPUsedCache(fm_int  sw,
                                fm_bool invalidateCache,
                                fm_bool resetFlag);


fm_status fmCreateInterface(fm_int  sw,
                            fm_int *interface);
fm_status fmDeleteInterface(fm_int sw,
                            fm_int interface);
fm_status fmGetInterfaceList(fm_int  sw,
                             fm_int *numInterfaces,
                             fm_int *interfaceList,
                             fm_int  max);
fm_status fmGetInterfaceFirst(fm_int sw, fm_int *firstInterface);
fm_status fmGetInterfaceNext(fm_int  sw,
                             fm_int  currentInterface,
                             fm_int *nextInterface);
fm_status fmSetInterfaceAttribute(fm_int sw,
                                  fm_int interface,
                                  fm_int attr,
                                  void * value);
fm_status fmGetInterfaceAttribute(fm_int sw,
                                  fm_int interface,
                                  fm_int attr,
                                  void * value);
fm_status fmAddInterfaceAddr(fm_int     sw,
                             fm_int     interface,
                             fm_ipAddr *addr);
fm_status fmDeleteInterfaceAddr(fm_int     sw,
                                fm_int     interface,
                                fm_ipAddr *addr);
fm_status fmGetInterfaceAddrList(fm_int     sw,
                                 fm_int     interface,
                                 fm_int *   numAddresses,
                                 fm_ipAddr *addressList,
                                 fm_int     max);
fm_status fmGetInterfaceAddrFirst(fm_int      sw,
                                  fm_int      interface,
                                  fm_voidptr *searchToken,
                                  fm_ipAddr * addr);
fm_status fmGetInterfaceAddrNext(fm_int      sw,
                                 fm_voidptr *searchToken,
                                 fm_ipAddr * addrNext);






#endif /* __FM_FM10000_API_NEXTHOP_H */

