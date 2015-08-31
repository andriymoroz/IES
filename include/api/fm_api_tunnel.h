/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_tunnel.h
 * Creation Date:   January 14, 2014
 * Description:     Tunnel API
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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

#ifndef __FM_FM_API_TUNNEL_H
#define __FM_FM_API_TUNNEL_H

/* Size of the NGE Data array. */
#define FM_TUNNEL_NGE_DATA_SIZE                 16

/* Size of GPE header in 4-byte words */
#define FM_TUNNEL_GPE_HDR_SIZE                  2

/* Size of NSH header in 4-byte words */
#define FM_TUNNEL_NSH_HDR_SIZE                  2

/* Size available for NSH data in 4-byte words */
#define FM_TUNNEL_NSH_DATA_SIZE                 (FM_TUNNEL_NGE_DATA_SIZE -\
                                                 FM_TUNNEL_GPE_HDR_SIZE  -\
                                                 FM_TUNNEL_NSH_HDR_SIZE)

/* Number of Tunnel Group that can be created */
#define FM_MAX_TUNNEL_GROUP                     16

/**************************************************/
/** \ingroup typeEnum 
 *  Tunnel Condition Masks.
 *  
 *  The following set of bit masks may be ORed
 *  together to produce an ''fm_tunnelCondition'' value.
 *  Each bit mask selects a field within the received
 *  frame that should be matched when a tunnel rule is
 *  tested.
 *                                              \lb\lb
 *  The field in the frame being processed will be
 *  compared against values specified in the
 *  ''fm_tunnelConditionParam'' structure. See the
 *  definition of that structure for the association
 *  between its various members and these condition bits.
 *                                              \lb\lb
 *  Note that tunnel condition is only used on hash
 *  tunnel group and the condition mask must be equal
 *  between flows and the superseed group.
 **************************************************/
typedef enum
{
    /** Whether outer VSI (encap) or TEP-ID (decap) is used for hash or search. */
    FM_TUNNEL_MATCH_VSI_TEP = (1 << 0),

    /** Whether outer VNI (decap) is used for hash or search. */
    FM_TUNNEL_MATCH_VNI = (1 << 1),

    /** Whether inner Destination MAC is used for hash or search. */
    FM_TUNNEL_MATCH_DMAC = (1 << 2),

    /** Whether inner Source MAC is used for hash or search. */
    FM_TUNNEL_MATCH_SMAC = (1 << 3),

    /** Whether inner Vlan ID is used for hash or search. */
    FM_TUNNEL_MATCH_VLAN = (1 << 4),

    /** Whether inner Destination IP is used for hash or search. */
    FM_TUNNEL_MATCH_DIP = (1 << 5),

    /** Whether inner Source IP is used for hash or search. */
    FM_TUNNEL_MATCH_SIP = (1 << 6),

    /** Whether inner L4 Source Port is used for hash or search. */
    FM_TUNNEL_MATCH_L4SRC = (1 << 7),

    /** Whether inner L4 Destination Port is used for hash or search. */
    FM_TUNNEL_MATCH_L4DST = (1 << 8),

    /** Whether inner Protocol is used for hash or search. */
    FM_TUNNEL_MATCH_PROT = (1 << 9),

    /** Whether inner Protocol == UDP is a match. Only used on search key and
     *  must not be set in parallel with ''FM_TUNNEL_MATCH_PROT''. */
    FM_TUNNEL_MATCH_UDP = (1 << 10),

    /** Whether inner Protocol == TCP is a match. Only used on search key  and
     *  must not be set in parallel with ''FM_TUNNEL_MATCH_PROT''. */
    FM_TUNNEL_MATCH_TCP = (1 << 11),

} fm_tunnelConditionMask;


/**************************************************/
/** \ingroup typeScalar
 * Used as an argument to ''fmAddTunnelRule'' 
 * and ''fmUpdateTunnelRule''. It comprises a 
 * bit mask representing a set of tests. See 
 * ''fm_tunnelConditionMask'' for definitions of 
 * each bit in the condition mask.
 **************************************************/
typedef fm_uint32    fm_tunnelCondition;


/**************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmCreateTunnel''.
 * This structure is used to carry various tunnel group. 
 * parameters.
 **************************************************/
typedef struct _fm_tunnelParam
{
    /** Defines the tunneling engine to uses for that group */
    fm_int  te;

    /** Defines the group size. This refer to the maximum of rule to be
     *  defined on a direct lookup group or the number of bins for a group
     *  that uses hash lookup. */
    fm_int  size;

    /** Use USER bit to extend the indexing. Default usage model should not
     *  define that flag. */
    fm_bool useUser;

    /** Defines if the change is Encap/NAT (TRUE) or Decap (FALSE) */
    fm_bool encap;

    /** Defines the keys included in the search or set it to 0 for
     *  direct lookup:
     *  ''FM_TUNNEL_MATCH_VSI_TEP''             \lb
     *  ''FM_TUNNEL_MATCH_VNI''                 \lb
     *  ''FM_TUNNEL_MATCH_DMAC''                \lb
     *  ''FM_TUNNEL_MATCH_SMAC''                \lb
     *  ''FM_TUNNEL_MATCH_VLAN''                \lb
     *  ''FM_TUNNEL_MATCH_DIP''                 \lb
     *  ''FM_TUNNEL_MATCH_SIP''                 \lb
     *  ''FM_TUNNEL_MATCH_L4SRC''               \lb
     *  ''FM_TUNNEL_MATCH_L4DST''               \lb
     *  ''FM_TUNNEL_MATCH_PROT''                 */
    fm_tunnelCondition hashKeyConfig;

    /** Defines the number of TEP-ID supported. Only for hash lookup on decap
     *  when hash key ''FM_TUNNEL_MATCH_VSI_TEP'' is selected. */
    fm_int  tepSize;

} fm_tunnelParam;


/**************************************************/
/** \ingroup typeScalar
 * Used as an argument to ''fmAddTunnelEncapFlow'' 
 * and ''fmUpdateTunnelEncapFlow''. It comprises
 * a bit mask representing a set of actions. See
 * ''fm_tunnelEncapActionMask'' for definitions of
 * each bit in the tunnel encap action mask.
 **************************************************/
typedef fm_uint32    fm_tunnelEncapFlow;


/**************************************************/
/** \ingroup typeEnum 
 *  Tunnel Encap Action Masks.
 *
 *  These bit masks are used to define the action
 *  argument of type ''fm_tunnelEncapFlow'' in
 *  ''fmAddTunnelEncapFlow'' and
 *  ''fmUpdateTunnelEncapFlow''. When an encap
 *  tunnel flow "hits," one or more associated actions
 *  may be taken as indicated in the action bit mask.
 *  Note that some actions require an associated param
 *  argument as specified in the ''fm_tunnelEncapFlowParam''
 *  structure. See the definition of that structure for
 *  the association between its various members and these
 *  action bits.
 **************************************************/
typedef enum
{
    /** Defines the outer Source IP for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_SIP = (1 << 0),

    /** Defines the outer TOS for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_TOS = (1 << 1),

    /** Defines the outer TTL for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_TTL = (1 << 2),

    /** Defines the outer L4 Destination Port for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_L4DST = (1 << 3),

    /** Defines the outer L4 Source Port for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_L4SRC = (1 << 4),

    /** Defines if a counter structure is specified. */
    FM_TUNNEL_ENCAP_FLOW_COUNTER = (1 << 5),

    /** Defines the outer NGE Data for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_NGE = (1 << 6),

    /** Whether the time tag should be loaded into NGE in words[14:15]. */
    FM_TUNNEL_ENCAP_FLOW_NGE_TIME = (1 << 7),

    /** Defines the GPE header's VNI field for this tunnel.  */
    FM_TUNNEL_ENCAP_FLOW_GPE_VNI = (1 << 8),

    /** Defines the NSH Base Header fields (Critical Flag, Length
     *  and MD Type) for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_NSH_BASE_HDR = (1 << 9),

    /** Defines the NSH Service Header fields for this tunnel. */
    FM_TUNNEL_ENCAP_FLOW_NSH_SERVICE_HDR = (1 << 10),

    /** Defines the NSH header's data bytes field for this
     *  tunnel. */
    FM_TUNNEL_ENCAP_FLOW_NSH_DATA = (1 << 11),

    /** Defines whether all the NSH fields are provided (Base
     *  Header, Service Header and data.) */
    FM_TUNNEL_ENCAP_FLOW_GPE_NSH_ALL = FM_TUNNEL_ENCAP_FLOW_GPE_VNI         |
                                       FM_TUNNEL_ENCAP_FLOW_NSH_BASE_HDR    |
                                       FM_TUNNEL_ENCAP_FLOW_NSH_SERVICE_HDR |
                                       FM_TUNNEL_ENCAP_FLOW_NSH_DATA,

} fm_tunnelEncapActionMask;


/**************************************************/
/** \ingroup typeEnum
 * Used to define which encapsulation type is selected.
 **************************************************/
typedef enum
{
    /** Encapsulate using VXLAN type of tunnel */
    FM_TUNNEL_TYPE_VXLAN,

    /** Encapsulate using NGE type of tunnel */
    FM_TUNNEL_TYPE_NGE,

    /** Encapsulate using NVGRE type of tunnel */
    FM_TUNNEL_TYPE_NVGRE,

    /** Encapsulate using VXLAN-GPE type of tunnel */
    FM_TUNNEL_TYPE_GPE,

    /** Encapsulate using VXLAN-GPE/NSH type of tunnel */
    FM_TUNNEL_TYPE_GPE_NSH,

    /** UNPUBLISHED: For internal use only. */
    FM_TUNNEL_TYPE_MAX

} fm_tunnelType;


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmAddTunnelEncapFlow''
 *  and ''fmUpdateTunnelEncapFlow''. Some tunnel encap
 *  actions require an argument upon which the action
 *  will be performed or filtered against.
 *                                                   \lb\lb
 *  Not all members of this structure need contain
 *  valid values, only those that are relevant to the
 *  tunnel encap flow's action, as noted in the descriptions
 *  of each member, below.
 **************************************************/
typedef struct _fm_tunnelEncapFlowParam
{
    /** Specify the encapsulation tunnel type */
    fm_tunnelType type;

    /** Specify if this tunnel flow is shareable for different rules (TRUE) or
     *  not (FALSE). Sharing tunnel flow reduce the resources usage when
     *  multiple rules refer to the same tunnel flow but doing this also
     *  increase latency. */
    fm_bool    shared;

    /** Must always be set (REQUIRED). This specify the outer Destination IP
     *  for this tunnel. */
    fm_ipAddr  dip;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_SIP'' is
     *  set. */
    fm_ipAddr  sip;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_TOS'' is
     *  set. */
    fm_byte    tos;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_TTL'' is
     *  set. */
    fm_byte    ttl;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_L4DST'' is
     *  set. */
    fm_uint16  l4Dst;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_L4SRC'' is
     *  set. */
    fm_uint16  l4Src;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_NGE'' is
     *  set. This is a mask indicating which of 16 words is present.  */
    fm_uint16  ngeMask;

    /** Must be set to the proper value if ''FM_TUNNEL_ENCAP_FLOW_NGE'' is
     *  set. This is the value to set based on each word set in the mask.  */
    fm_uint32  ngeData[FM_TUNNEL_NGE_DATA_SIZE];

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_GPE_VNI'' is set. This is the VNI
     *  that will be stored in the VXLAN-GPE header. */
    fm_uint32  gpeVni;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_BASE_HDR'' is set. This is length
     *  of the NSH header in 4-byte words including the Base Header,
     *  Service Path Header and Context Data.  */
    fm_byte    nshLength;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_BASE_HDR'' is set. This bit
     *  should be set if there are critical TLV that are included in
     *  the NSH data. */
    fm_bool    nshCritical;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_BASE_HDR'' is set. This field
     *  should contain the MD Type. */
    fm_byte    nshMdType;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_SERVICE_HDR'' is set. This field
     *  should contain the Service Path ID. */
    fm_uint32  nshSvcPathId;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_SERVICE_HDR'' is set. This field
     *  should contain the Service Index. */
    fm_byte    nshSvcIndex;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_DATA'' is set. This field should
     *  contain the NSH context data that follows the service
     *  header. */
    fm_uint32  nshData[FM_TUNNEL_NSH_DATA_SIZE];

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_ENCAP_FLOW_NSH_DATA'' is set. This field is a
     *  bitmask of the valid 32-bit words in nshData. */
    fm_uint16  nshDataMask;

} fm_tunnelEncapFlowParam;


/**************************************************/
/** \ingroup typeStruct
 * An Tunnel condition param is an argument to 
 * ''fmAddTunnelRule'' and ''fmUpdateTunnelRule''. It 
 * consists of a collection of packet field values that 
 * must be matched to satisfy the tunnel rule.
 *                                              \lb\lb
 * Not all members of this structure need contain
 * valid values, only those that are relevant to the
 * Tunnel rule's condition (see 'fm_tunnelCondition').
 **************************************************/
typedef struct _fm_tunnelConditionParam
{
    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_VSI_TEP'' is set. */
    fm_uint16  vsiTep;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_VNI'' is set. */
    fm_uint32  vni;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_DMAC'' is set. */
    fm_macaddr dmac;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_SMAC'' is set. */
    fm_macaddr smac;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_VLAN'' is set. */
    fm_uint16  vlan;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_DIP'' is set. */
    fm_ipAddr  dip;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_SIP'' is set. */
    fm_ipAddr  sip;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_L4SRC'' is set. */
    fm_uint16  l4Src;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_L4DST'' is set. */
    fm_uint16  l4Dst;

    /** Must be set to the proper matching value if the search key
     *  ''FM_TUNNEL_MATCH_PROT'' is set. */
    fm_byte    protocol;

} fm_tunnelConditionParam;


/**************************************************/
/** \ingroup typeEnum 
 *  Tunnel Action Masks.
 *
 *  These bit masks are used to define the action
 *  argument of type ''fm_tunnelAction'' in
 *  ''fmAddTunnelRule'' and ''fmUpdateTunnelRule''.
 *  When a tunnel rule "hits," one or more associated actions
 *  may be taken as indicated in the action bit mask.
 *  Note that some actions require an associated param
 *  argument as specified in the ''fm_tunnelActionParam''
 *  structure. See the definition of that structure for
 *  the association between its various members and these
 *  action bits.
 **************************************************/
typedef enum
{
    /** Defines the Destination GLORT for this flow, applicable to decap groups only. */
    FM_TUNNEL_SET_DGLORT = (1 << 0),

    /** Defines the outer VNI for this flow, applicable to encap groups only. */
    FM_TUNNEL_SET_VNI = (1 << 1),

    /** Defines the inner Destination MAC for this flow. */
    FM_TUNNEL_SET_DMAC = (1 << 2),

    /** Defines the inner Source MAC for this flow. */
    FM_TUNNEL_SET_SMAC = (1 << 3),

    /** Defines the inner Vlan ID for this flow. */
    FM_TUNNEL_SET_VLAN = (1 << 4),

    /** Defines the inner Destination IP for this flow. */
    FM_TUNNEL_SET_DIP = (1 << 5),

    /** Defines the inner Source IP for this flow. */
    FM_TUNNEL_SET_SIP = (1 << 6),

    /** Defines the inner L4 Source Port for this flow. */
    FM_TUNNEL_SET_L4SRC = (1 << 7),

    /** Defines the inner L4 Destination Port for this flow. */
    FM_TUNNEL_SET_L4DST = (1 << 8),

    /** Defines the inner TTL for this flow. */
    FM_TUNNEL_SET_TTL = (1 << 9),

    /** Defines the outer NGE Data for this flow. */
    FM_TUNNEL_SET_NGE = (1 << 10),

    /** Whether time tag should be loaded into NGE in words[14:15]. */
    FM_TUNNEL_SET_NGE_TIME = (1 << 11),

    /** Whether tunneling information is supplied. */
    FM_TUNNEL_ENCAP_FLOW = (1 << 12),

    /** Whether a counter structure is specified. */
    FM_TUNNEL_COUNT = (1 << 13),

    /** Specifies whether this rule should keep the outer header in place
     *  even if the decap flag is set. Update of the L4 checksum and inner
     *  field are still permitted and applied. This is only applicable to
     *  decap groups and can't be performed in parallel with
     *  ''FM_TUNNEL_DECAP_MOVE_OUTER_HDR''. */
    FM_TUNNEL_DECAP_KEEP_OUTER_HDR = (1 << 14),

    /** Specifies whether this rule should move the outer header at the end
     *  of the packet and append the Outer Header length and Flow Pointer.
     *  This is only applicable to decap groups and can't be performed in
     *  parallel with ''FM_TUNNEL_DECAP_KEEP_OUTER_HDR''. */
    FM_TUNNEL_DECAP_MOVE_OUTER_HDR = (1 << 15),

    /** Defines the GPE header's VNI field */
    FM_TUNNEL_SET_GPE_VNI = (1 << 16),

    /** Defines the NSH Base Header fields (Critical Flag, Length
     *  and MD Type) */
    FM_TUNNEL_SET_NSH_BASE_HDR = (1 << 17),

    /** Defines the NSH Service Header fields. */
    FM_TUNNEL_SET_NSH_SERVICE_HDR = (1 << 18),

    /** Defines the NSH header's data bytes field */
    FM_TUNNEL_SET_NSH_DATA = (1 << 19),

    /** Defines whether all the NSH fields are provided (Base
     *  Header, Service Header and data.) */
    FM_TUNNEL_SET_GPE_NSH_ALL = FM_TUNNEL_SET_GPE_VNI         | 
                                FM_TUNNEL_SET_NSH_BASE_HDR    |
                                FM_TUNNEL_SET_NSH_SERVICE_HDR |
                                FM_TUNNEL_SET_NSH_DATA,

} fm_tunnelActionMask;


/**************************************************/
/** \ingroup typeScalar
 * Used as an argument to ''fmAddTunnelRule'' 
 * and ''fmUpdateTunnelRule''. It comprises a 
 * bit mask representing a set of actions. See 
 * ''fm_tunnelActionMask'' for definitions of 
 * each bit in the tunnel action mask.
 **************************************************/
typedef fm_uint32    fm_tunnelAction;


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmAddTunnelRule''
 *  and ''fmUpdateTunnelRule''. Some tunnel actions
 *  require an argument upon which the action will be
 *  performed or filtered against.
 *                                                   \lb\lb
 *  Not all members of this structure need contain
 *  valid values, only those that are relevant to the
 *  tunnel rule's action, as noted in the descriptions
 *  of each member, below.
 **************************************************/
typedef struct _fm_tunnelActionParam
{
    /** Must be set to the proper field value if ''FM_TUNNEL_SET_DGLORT''
     *  is set. */
    fm_uint16  dglort;

    /** Must be set to the proper field value if ''FM_TUNNEL_SET_VNI''
     *  is set. */
    fm_uint32  vni;

    /** Must be set to the proper field value if ''FM_TUNNEL_SET_DMAC''
     *  is set. */
    fm_macaddr dmac;

    /** Must be set to the proper field value if ''FM_TUNNEL_SET_SMAC''
     *  is set. */
    fm_macaddr smac;

    /** Must be set to the proper field value if ''FM_TUNNEL_SET_VLAN''
     *  is set. */
    fm_uint16  vlan;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_DIP'' is
     *  set. */
    fm_ipAddr  dip;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_SIP'' is
     *  set. */
    fm_ipAddr  sip;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_L4SRC'' is
     *  set. */
    fm_uint16  l4Src;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_L4DST'' is
     *  set. */
    fm_uint16  l4Dst;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_TTL'' is
     *  set. */
    fm_byte    ttl;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_NGE'' is
     *  set. This is a mask indicating which of 16 words is present.  */
    fm_uint16  ngeMask;

    /** Must be set to the proper value if ''FM_TUNNEL_SET_NGE'' is
     *  set. This is the value to set based on each word set in the mask.  */
    fm_uint32  ngeData[FM_TUNNEL_NGE_DATA_SIZE];

    /** Must be set to the proper tunnel encap flow if
     *  ''FM_TUNNEL_ENCAP_FLOW'' is set. */
    fm_int     encapFlow;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_GPE_VNI'' is set. This is the VNI
     *  that will be stored in the VXLAN-GPE header. */
    fm_uint32  gpeVni;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_BASE_HDR'' is set. This is length
     *  of the NSH header in 4-byte words including the Base Header,
     *  Service Path Header and Context Data.  */
    fm_byte    nshLength;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_BASE_HDR'' is set. This bit
     *  should be set if there are critical TLV that are included in
     *  the NSH data. */
    fm_bool    nshCritical;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_BASE_HDR'' is set. This field
     *  should contain the MD Type. */
    fm_byte    nshMdType;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_SERVICE_HDR'' is set. This field should
     *  contain the Service Path ID. */
    fm_uint32  nshSvcPathId;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_SERVICE_HDR'' is set. This field should
     *  contain the Service Index. */
    fm_byte    nshSvcIndex;

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_DATA'' is set. This field should
     *  contain the NSH context data that follows the service
     *  header. */
    fm_uint32  nshData[FM_TUNNEL_NSH_DATA_SIZE];

    /** Must be set to the proper value if
     *  ''FM_TUNNEL_SET_NSH_DATA'' is set. This field is a
     *  bitmask of the valid 32-bit words in nshData. */
    fm_uint16  nshDataMask;

} fm_tunnelActionParam;


/**************************************************/
/** \ingroup typeStruct
 * Tunnel Counters
 * Used as an argument to ''fmGetTunnelRuleCount'' 
 * and ''fmGetTunnelEncapFlowCount''.
 **************************************************/
typedef struct _fm_tunnelCounters
{
    /** Number of packets counted by the rule. */
    fm_uint64 cntPkts;

    /** Number of bytes counted by the rule as seen on egress including
     *  any packet modification. */
    fm_uint64 cntOctets;

} fm_tunnelCounters;


/**************************************************/
/** \ingroup typeStruct
 * Tunnel Group statistics
 * Used as an argument to ''FM_TUNNEL_USAGE'' Tunnel 
 * Group attribute.
 **************************************************/
typedef struct _fm_tunnelUsage
{
    /** Number of rules(direct) or bins(hash) reserved for that group. */
    fm_int lookupReserved;

    /** Number of rules(direct) or bins(hash) currently used for that group. */
    fm_int lookupUsed;

    /** Number of rules(direct) or bins(hash) not currently assigned by any groups. */
    fm_int lookupAvailable;

    /** Number of rules(direct) or bins(hash) assigned or not by any groups. */
    fm_int lookupTotal;

    /** Number of flow entries currently used by that group. Flow entries refer
     *  to the hardware resources used by rules and encapFlows. Multiple flow
     *  entries can be used to cover a single rule depending on the condition
     *  and action size. */
    fm_int flowUsed;

    /** Number of flow entries not currently assigned by any groups. */
    fm_int flowAvailable;

    /** Number of flow entries assigned or not by any groups. */
    fm_int flowTotal;

    /** Number of counter entries currently used by that group. */
    fm_int countUsed;

    /** Number of counter entries not currently assigned by any groups. */
    fm_int countAvailable;

    /** Number of counter entries assigned or not by any groups. */
    fm_int countTotal;

} fm_tunnelUsage;


/**************************************************/
/** \ingroup typeStruct
 * Tunnel Group GLORT/USER allocation
 * Used as an argument to ''FM_TUNNEL_GLORT_USER'' Tunnel 
 * Group/Rule attribute.
 **************************************************/
typedef struct _fm_tunnelGlortUser
{
    /** GLORT value for that group or rule */
    fm_uint16 glort;

    /** GLORT mask for that group or rule */
    fm_uint16 glortMask;

    /** USER value for that group or rule */
    fm_byte   user;

    /** USER mask for that group or rule */
    fm_byte   userMask;

} fm_tunnelGlortUser;


/****************************************************************************/
/** \ingroup constTunnelAttr
 *
 *  Tunnel Attributes, used as an argument to ''fmSetTunnelAttribute'' and 
 *  ''fmGetTunnelAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_tunnelAttr
{
    /** Type fm_bool: Defines if SGLORT is set to TE_DEFAULT_SGLORT (TRUE) or
     *  passthrough (FALSE). Rule argument is not used since this is a group
     *  attribute only. */
    FM_TUNNEL_SET_DEFAULT_SGLORT = 0,

    /** Type fm_bool: Defines if DGLORT is set to TE_DEFAULT_DGLORT (TRUE) or
     *  passthrough (FALSE). Only applicable if the ''FM_TUNNEL_SET_DGLORT''
     *  action is not specified in the rule. Rule argument is not used since
     *  this is a group attribute only */
    FM_TUNNEL_SET_DEFAULT_DGLORT,

    /** Type fm_tunnelUsage (read-only): Retrieve the current resource usage
     *  of that tunnel group. Rule argument is not used since this is a group
     *  attribute only */
    FM_TUNNEL_USAGE,

    /** Type fm_tunnelGlortUser (read-only): Retrieve the current GLORT and USER
     *  allocation for that specific entry. For entries that are part of a
     *  direct lookup group, specifying both group and rule argument get the
     *  exact rule location while hash lookup group entry must specify the
     *  TEP to be retrieved using the rule argument. Both groups type can also
     *  specify a rule argument equal to -1 to retrieve the whole group GLORT
     *  and USER reserved range. */
    FM_TUNNEL_GLORT_USER,

    /** UNPUBLISHED: For internal use only. */
    FM_TUNNEL_ATTRIBUTE_MAX

};  /* end enum _fm_tunnelAttr */


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Mode. Specifies which protocols
 *  are supported for a given tunnel engine.  
 **************************************************/
typedef enum
{
    /** Support the VXLAN, NVGRE and NGE protocols. */
    FM_TUNNEL_API_MODE_VXLAN_NVGRE_NGE = 0,

    /** Support the VXLAN, GPE and NSH protocols. */
    FM_TUNNEL_API_MODE_VXLAN_GPE_NSH,

    /** UNPUBLISHED: For internal use only. */
    FM_TUNNEL_API_MODE_MAX,

} fm_teMode;


/**************************************************/
/** \ingroup typeStruct
 * Tunnel API Mode. Used as an argument to the 
 * ''FM_TUNNEL_API_MODE'' Tunnel API Attribute.
 **************************************************/
typedef struct _fm_tunnelModeAttr
{
    /** Te on which the mode should get set/get. */
    fm_byte   te;

    /** Tunnel Engine Mode. */
    fm_teMode mode;

} fm_tunnelModeAttr;


/**************************************************/
/** \ingroup typeEnum 
 *  Tunnel API Attributes. 
 *
 *  Used as an argument to ''fmSetTunnelApiAttribute''
 *  and ''fmGetTunnelApiAttribute''.
 *
 *  For each attribute, the data type of the
 *  corresponding attribute value is specified.
 **************************************************/
typedef enum
{
    /** Type fm_int: Defines which set of protocols the tunnel
     *  engine should operate with. See ''fm_teMode''. This
     *  attribute may only be changed while there are no tunnel
     *  groups/rules in the tunnel engine. */
    FM_TUNNEL_API_MODE = 0,

    /** UNPUBLISHED: For internal use only. */
    FM_TUNNEL_API_ATTRIBUTE_MAX

} fm_tunnelApiAttr;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

fm_status fmCreateTunnel(fm_int sw, fm_int *group, fm_tunnelParam *tunnelParam);
fm_status fmDeleteTunnel(fm_int sw, fm_int group);

fm_status fmGetTunnel(fm_int sw, fm_int group, fm_tunnelParam *tunnelParam);
fm_status fmGetTunnelFirst(fm_int sw, fm_int *firstGroup);
fm_status fmGetTunnelNext(fm_int sw, fm_int currentGroup, fm_int *nextGroup);

fm_status fmAddTunnelEncapFlow(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   encapFlow,
                               fm_tunnelEncapFlow       field,
                               fm_tunnelEncapFlowParam *param);
fm_status fmDeleteTunnelEncapFlow(fm_int sw, fm_int group, fm_int encapFlow);
fm_status fmUpdateTunnelEncapFlow(fm_int                   sw,
                                  fm_int                   group,
                                  fm_int                   encapFlow,
                                  fm_tunnelEncapFlow       field,
                                  fm_tunnelEncapFlowParam *param);

fm_status fmGetTunnelEncapFlow(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   encapFlow,
                               fm_tunnelEncapFlow *     field,
                               fm_tunnelEncapFlowParam *param);
fm_status fmGetTunnelEncapFlowFirst(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int *                 firstEncapFlow,
                                    fm_tunnelEncapFlow *     field,
                                    fm_tunnelEncapFlowParam *param);
fm_status fmGetTunnelEncapFlowNext(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int                   currentEncapFlow,
                                   fm_int *                 nextEncapFlow,
                                   fm_tunnelEncapFlow *     field,
                                   fm_tunnelEncapFlowParam *param);

fm_status fmAddTunnelRule(fm_int                   sw,
                          fm_int                   group,
                          fm_int                   rule,
                          fm_tunnelCondition       cond,
                          fm_tunnelConditionParam *condParam,
                          fm_tunnelAction          action,
                          fm_tunnelActionParam *   actParam);
fm_status fmDeleteTunnelRule(fm_int sw, fm_int group, fm_int rule);
fm_status fmUpdateTunnelRule(fm_int                   sw,
                             fm_int                   group,
                             fm_int                   rule,
                             fm_tunnelCondition       cond,
                             fm_tunnelConditionParam *condParam,
                             fm_tunnelAction          action,
                             fm_tunnelActionParam *   actParam);

fm_status fmGetTunnelRule(fm_int                   sw,
                          fm_int                   group,
                          fm_int                   rule,
                          fm_tunnelCondition *     cond,
                          fm_tunnelConditionParam *condParam,
                          fm_tunnelAction *        action,
                          fm_tunnelActionParam *   actParam);
fm_status fmGetTunnelRuleFirst(fm_int                   sw,
                               fm_int                   group,
                               fm_int *                 firstRule,
                               fm_tunnelCondition *     cond,
                               fm_tunnelConditionParam *condParam,
                               fm_tunnelAction *        action,
                               fm_tunnelActionParam *   actParam);
fm_status fmGetTunnelRuleNext(fm_int                   sw,
                              fm_int                   group,
                              fm_int                   currentRule,
                              fm_int *                 nextRule,
                              fm_tunnelCondition *     cond,
                              fm_tunnelConditionParam *condParam,
                              fm_tunnelAction *        action,
                              fm_tunnelActionParam *   actParam);

fm_status fmGetTunnelRuleCount(fm_int             sw,
                               fm_int             group,
                               fm_int             rule,
                               fm_tunnelCounters *counters);
fm_status fmGetTunnelEncapFlowCount(fm_int             sw,
                                    fm_int             group,
                                    fm_int             encapFlow,
                                    fm_tunnelCounters *counters);
fm_status fmGetTunnelRuleUsed(fm_int   sw,
                              fm_int   group,
                              fm_int   rule,
                              fm_bool *used);

fm_status fmResetTunnelRuleCount(fm_int sw, fm_int group, fm_int rule);
fm_status fmResetTunnelEncapFlowCount(fm_int sw,
                                      fm_int group,
                                      fm_int encapFlow);
fm_status fmResetTunnelRuleUsed(fm_int sw, fm_int group, fm_int rule);

fm_status fmSetTunnelAttribute(fm_int sw,
                               fm_int group,
                               fm_int rule,
                               fm_int attr,
                               void * value);
fm_status fmGetTunnelAttribute(fm_int sw,
                               fm_int group,
                               fm_int rule,
                               fm_int attr,
                               void * value);
fm_status fmSetTunnelApiAttribute(fm_int sw,
                                  fm_int attr,
                                  void * value);
fm_status fmGetTunnelApiAttribute(fm_int sw,
                                  fm_int attr,
                                  void * value);

fm_status fmDbgDumpTunnel(fm_int sw);

#endif /* __FM_FM_API_TUNNEL_H */
