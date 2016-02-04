/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_nat.h
 * Creation Date:   March 13, 2014
 * Description:     NAT API
 *
 * Copyright (c) 2014 - 2016, Intel Corporation
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

#ifndef __FM_FM_API_NAT_H
#define __FM_FM_API_NAT_H


/*****************************************************************************/
/** \ingroup typeScalar
 * Used as an argument to ''fmAddNatRule'' and ''fmAddNatPrefilter''. It 
 * comprises a bit mask representing a set of matching conditions. See 
 * ''fm_natConditionMask'' for definitions of each bit in the mask.
 *****************************************************************************/
typedef fm_uint32    fm_natCondition;


/*****************************************************************************/
/** \ingroup typeEnum 
 *  NAT Condition Masks.
 *
 *  The following set of bit masks may be ORed together to produce an
 *  ''fm_natCondition'' value. Each bit mask selects a field within the
 *  received frame that should be matched when a NAT entry is tested.
 *                                                                    \lb\lb
 *  Note that if the mode selected is ''FM_NAT_MODE_RESOURCE'' and the table
 *  is configured to decapsulate tunneled frames, rule matching will be
 *  performed against inner fields for all supported conditions except 
 *  ''FM_NAT_MATCH_VNI''. However, prefilter entries always match on outer
 *  fields regardless of the mode selected.
 *****************************************************************************/
typedef enum
{
    /** Match the source MAC address. */
    FM_NAT_MATCH_SRC_MAC = (1 << 0),

    /** Match the destination MAC address. */
    FM_NAT_MATCH_DST_MAC = (1 << 1),

    /** Match the VLAN ID. This condition is only available on rules if the mode
     *  selected is ''FM_NAT_MODE_PERFORMANCE''. Otherwise it can be used as a
     *  matching condition for prefilter entries if the mode selected is
     *  ''FM_NAT_MODE_RESOURCE''. */
    FM_NAT_MATCH_VLAN = (1 << 2),

    /** Match the VLAN2 ID. This condition is only available on rules if the mode
     *  selected is ''FM_NAT_MODE_PERFORMANCE''. Otherwise it can be used as a
     *  matching condition for prefilter entries if the mode selected is
     *  ''FM_NAT_MODE_RESOURCE''. */
    FM_NAT_MATCH_VLAN2 = (1 << 3),

    /** Match the IP source address. */
    FM_NAT_MATCH_SRC_IP = (1 << 4),

    /** Match the IP destination address. */
    FM_NAT_MATCH_DST_IP = (1 << 5),

    /** Match the L4 protocol (IPv4) or Next Header (IPv6). */
    FM_NAT_MATCH_PROTOCOL = (1 << 6),

    /** Match on a TCP/UDP source port. */
    FM_NAT_MATCH_L4_SRC_PORT = (1 << 7),

    /** Match on a TCP/UDP destination port. */
    FM_NAT_MATCH_L4_DST_PORT = (1 << 8),

    /** Match on a single ingress logical port. This logical port must refer to a
     *  single physical port. This condition is only available on rules if the mode
     *  selected is ''FM_NAT_MODE_PERFORMANCE''. Otherwise it can be used as a
     *  matching condition for prefilter entries if the mode selected is
     *  ''FM_NAT_MODE_RESOURCE''. */
    FM_NAT_MATCH_SRC_PORT = (1 << 9),

    /** Match on outer VNI field. This condition is only available on rules if the
     *  mode selected is ''FM_NAT_MODE_RESOURCE''. */
    FM_NAT_MATCH_VNI = (1 << 10),

    /** Match on UDP protocol. This condition is ored with FM_NAT_MATCH_TCP.
     *  This condition is only available on rules if the mode selected is
     *  ''FM_NAT_MODE_RESOURCE''. */
    FM_NAT_MATCH_UDP = (1 << 11),

    /** Match on TCP protocol. This condition is ored with FM_NAT_MATCH_UDP.
     *  This condition is only available on rules if the mode selected is
     *  ''FM_NAT_MODE_RESOURCE''. */
    FM_NAT_MATCH_TCP = (1 << 12),

} fm_natConditionMask;


/*****************************************************************************/
/** \ingroup typeEnum
 * Used to define which NAT mode is selected.
 *****************************************************************************/
typedef enum
{
    /** In that mode the matching is processed by the FFU and the action is
     *  executed by the TE. This mode supports ECMP to hash over multiple
     *  entries for a single hit. This is the most flexible mode but also the
     *  one that consume the most FFU resources. */
    FM_NAT_MODE_PERFORMANCE,

    /** In that mode the matching is processed partially by the FFU as a
     *  prefiltering step then a second matching is done at the TE level. This
     *  mode does not supports ECMP to hash over multiple entries for a single
     *  hit. This is the best mode to uses to pack as much entries as possible.
     *  This mode works best if most of the conditions are configured with full
     *  mask. */
    FM_NAT_MODE_RESOURCE,

    /** UNPUBLISHED: For internal use only. */
    FM_NAT_MODE_MAX

} fm_natMode;


/*****************************************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmCreateNatTable''. This structure is used to 
 * carry various NAT table parameters.
 *****************************************************************************/
typedef struct _fm_natParam
{
    /** Defines the NAT mode selected for this table. */
    fm_natMode mode;

    /** Defines the table size. This refer to the maximum of entries to be
     *  defined on this table. */
    fm_int  size;

    /** Defines if this table encapuslate and decapsulate tunnelled frame.
     *  Supported tunnel mode are VxLan, NVGRE and NGE. */
    fm_bool tunnel;

    /** Defines the table direction. This could be either TRUE to
     *  encapsulate/NAT or FALSE to decapsulate/DENAT frames. Most of the time
     *  two table would be required to cover both direction. */
    fm_bool direction;

    /** Defines prefilter matching condition. Those conditions will then be
     *  reused by the function ''fmAddNatPrefilter'' to specify flows that
     *  needs to be translated. This is only available if the mode selected is
     *  ''FM_NAT_MODE_RESOURCE''. Note that prefilter entries are ORed
     *  together so that any hit in the prefilter table would initiate a NAT
     *  tentative. */
    fm_natCondition preFilterCond;

    /** Defines rule matching condition. Those conditions will be reused by the
     *  function ''fmAddNatRule''. If the mode selected is
     *  ''FM_NAT_MODE_RESOURCE'' then every rule must defines every condition
     *  specified. */
    fm_natCondition ruleCond;

} fm_natParam;


/*****************************************************************************/
/** \ingroup typeStruct
 * Used to set default NGE configuration on NAT entries.
 *****************************************************************************/
typedef struct _fm_natNgeDefault
{
    /** Defines the L4 port used for NGE. */
    fm_uint16  l4Dst;

    /** Defines the protocol to uses in the NGE headers when encapsulating. */
    fm_uint16  protocol;

    /** Defines default NGE Data words */
    fm_uint32  ngeData[FM_TUNNEL_NGE_DATA_SIZE];

    /** Defines which words are valid in ngeData[] */
    fm_uint16  ngeMask;

    /** Defines if the timetag should be loaded into words[14..15] (TRUE)
     *  by default. */
    fm_bool    ngeTime;

} fm_natNgeDefault;


/*****************************************************************************/
/** \ingroup typeStruct
 * Used to set default GPE configuration on NAT entries.
 *****************************************************************************/
typedef struct _fm_natGpeHdr
{
    /** GPE header's Next Protocol field. */
    fm_uint32  nextProt;

    /** The VNI that will be stored in the VXLAN-GPE header. */
    fm_uint32  vni;

} fm_natGpeHdr;


/*****************************************************************************/
/** \ingroup typeStruct
 * Used to set default GPE NSH configuration on NAT entries.
 *****************************************************************************/
typedef struct _fm_natNshHdr
{
    /** The length of the NSH header in 4-byte words including the Base Header,
     *  Service Path Header and Context Data. */
    fm_byte    length;

    /** The bit that should be set if there are critical TLVs that are
     * included in the NSH data. */ 
    fm_bool    critical;

    /** MD Type. */
    fm_byte    mdType;

    /** Service Path ID. */
    fm_uint32  svcPathId;

    /** Service Index. */
    fm_byte    svcIndex;

    /** The NSH context data that follows the service header. */
    fm_uint32  data[FM_TUNNEL_NSH_DATA_SIZE];

    /** The bitmask of the valid 32-bit words in nshData. */
    fm_uint16  dataMask;

} fm_natNshHdr;


/*****************************************************************************/
/** \ingroup typeEnum
 *  NAT Action Masks.
 *
 *  These bit masks are used to define which fields of the NSH header are set
 *  using ''fm_natNshHdr'' in ''fmCreateNatTunnel''
 *  (see ''fm_natTunnel'' structure).
 *****************************************************************************/
typedef enum
{
    /** Specify the NSH Base Header fields (Critical Flag, Length,
     *  and MD Type). */
    FM_NAT_TUNNEL_NSH_BASE_HDR    = (1 << 0),

    /** Specify the NSH Service Header fields. */
    FM_NAT_TUNNEL_NSH_SERVICE_HDR = (1 << 1),

    /** Specify the NSH header's data bytes field. */
    FM_NAT_TUNNEL_NSH_DATA        = (1 << 2),

} fm_natNshHdrMask;


/*****************************************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmSetNatTunnelDefault''. This structure is used 
 * to configure the type of tunnelling used and provide default value for some
 * of the outer header field.
 *****************************************************************************/
typedef struct _fm_fm_natTunnelDefault
{
    /** Specify the encapsulation tunnel type. Selecting VxLan or NVGRE tunnel
     *  types automatically configure proper L4 Destination port. */
    fm_tunnelType    type;

    /** Defines default TTL for tunnels during encapsulation if none specified
     *  with tunnel parameters. If this field is set to 0, then the outer IP
     *  header will use the inner TTL received. */
    fm_byte          ttl;

    /** Defines default TOS for tunnels during encapsulation if none specified
     *  with tunnel parameters. */
    fm_byte          tos;

    /** Defines how the TOS is set during encapsulation if none specified with
     *  tunnel parameters. Choices are:                               \lb\lb
     *  FALSE: use the default tos field                              \lb
     *  TRUE:  copy over the InnerTOS from packet */
    fm_bool          deriveOuterTOS;

    /** Defines default outer destination MAC. */
    fm_macaddr       dmac;

    /** Defines default outer source MAC. */
    fm_macaddr       smac;

    /** Defines NGE specific configuration. This should only be set if the tunnel
     *  type selected is FM_TUNNEL_TYPE_NGE. */
    fm_natNgeDefault ngeCfg;

    /** Defines GPE specific configuration. This should only be set if the
     *  tunnel type selected is FM_TUNNEL_TYPE_GPE or FM_TUNNEL_TYPE_GPE_NSH. */
    fm_natGpeHdr     gpeCfg;

    /** Defines GPE NSH specific configuration. This should only be set if the
     *  tunnel type selected is FM_TUNNEL_TYPE_GPE_NSH. */
    fm_natNshHdr     nshCfg;

} fm_natTunnelDefault;


/** Use default configuration as specified by the ''fmSetNatTunnelDefault''
 *  function.
 *  \ingroup constSystem */
#define FM_NAT_TUNNEL_DEFAULT                    -1


/*****************************************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmCreateNatTunnel''.
 *****************************************************************************/
typedef struct _fm_natTunnel
{
    /** Specifies the outer Destination IP for this tunnel. */
    fm_ipAddr            dip;

    /** Specifies the outer Source IP for this tunnel. */
    fm_ipAddr            sip;

    /** Specifies the outer L4 Source Port for this tunnel. This is only
     *  configurable for tunnel type ''FM_TUNNEL_TYPE_VXLAN'' or
     *  ''FM_TUNNEL_TYPE_NGE''. ''FM_NAT_TUNNEL_DEFAULT'' value can be set
     *  to use a hash based value built using inner dip, sip, l4Src, l4Dst,
     *  and Protocol. */
    fm_int               l4Src;

    /** Specifies the outer TOS for this tunnel. ''FM_NAT_TUNNEL_DEFAULT''
     *  value can be set to use the default configuration as specified by
     *  the ''fmSetNatTunnelDefault'' function. */
    fm_int               tos;

    /** Specifies the outer TTL for this tunnel. ''FM_NAT_TUNNEL_DEFAULT''
     *  value can be set to use the default configuration as specified by
     *  the ''fmSetNatTunnelDefault'' function. */
    fm_int               ttl;

    /** Specifies the outer L4 Destination port for this tunnel.
     *  ''FM_NAT_TUNNEL_DEFAULT'' value can be set to use the default
     *  configuration as specified by the ''fmSetNatTunnelDefault''
     *  function. */
    fm_int               l4Dst;

    /** Specifies the outer NGE information. Only valid for
     *  ''FM_TUNNEL_TYPE_NGE'' tunnel type. This is a mask indicating which
     *  of 16 words are present in ngeData[]. ''FM_NAT_TUNNEL_DEFAULT''
     *  value can be set to use the default configuration as specified by
     *  the ''fmSetNatTunnelDefault'' function. */
    fm_int               ngeMask;

    /** Specifies the outer NGE information. Only valid for
     *  ''FM_TUNNEL_TYPE_NGE'' tunnel type. This is the value to set based on
     *  each bit set in the mask.  */
    fm_uint32            ngeData[FM_TUNNEL_NGE_DATA_SIZE];

    /** Specifies the VNI that will be stored in the VXLAN-GPE header. */
    fm_int               gpeVni;

    /** Defines GPE NSH specific configuration. This should only be set if the
     *  tunnel type selected is FM_TUNNEL_TYPE_GPE_NSH. To make use of this
     *  configuration, nshMask (''fm_natNshHdrMask'') must be set */
    fm_natNshHdr         nsh;

    /** Defines which fields of the NSH header are set using ''fm_natNshHdr''
     *  structure. This should only be set if the tunnel type selected is
     *  FM_TUNNEL_TYPE_GPE_NSH. */
    fm_natNshHdrMask     nshMask;

} fm_natTunnel;


/*****************************************************************************/
/** \ingroup typeStruct
 * This is an argument to ''fmAddNatRule''. It consists of a collection of 
 * packet field values that must be matched to satisfy the NAT rule.
 *****************************************************************************/
typedef struct _fm_natConditionParam
{
    /** Source MAC address for the ''FM_NAT_MATCH_SRC_MAC'' condition. */
    fm_macaddr smac;

    /** Destination MAC address for the ''FM_NAT_MATCH_DST_MAC'' condition. */
    fm_macaddr dmac;

    /** VLAN ID for the ''FM_NAT_MATCH_VLAN'' condition. */
    fm_uint16  vlan;

    /** VLAN2 ID for the ''FM_NAT_MATCH_VLAN2'' condition. */
    fm_uint16  vlan2;

    /** Source IP address for the ''FM_NAT_MATCH_SRC_IP'' condition. */
    fm_ipAddr  srcIp;

    /** Source IP address mask for the ''FM_NAT_MATCH_SRC_IP'' condition.
     *  Partial masking is only supported on rules if the mode selected is
     *  ''FM_NAT_MODE_PERFORMANCE''. Otherwise partial masking can be used
     *  for prefilter entries if the mode selected is ''FM_NAT_MODE_RESOURCE''. */
    fm_ipAddr  srcIpMask;

    /** Destination IP address for the ''FM_NAT_MATCH_DST_IP'' condition. */
    fm_ipAddr  dstIp;

    /** Destination IP address mask for the ''FM_NAT_MATCH_DST_IP'' condition.
     *  Partial masking is only supported on rules if the mode selected is
     *  ''FM_NAT_MODE_PERFORMANCE''. Otherwise partial masking can be used
     *  for prefilter entries if the mode selected is ''FM_NAT_MODE_RESOURCE''. */
    fm_ipAddr  dstIpMask;

    /** L4 Protocol (IPv4) or Next Header (IPv6) for the
     *  ''FM_NAT_MATCH_PROTOCOL'' condition. */
    fm_byte    protocol;

    /** TCP or UDP source port for the ''FM_NAT_MATCH_L4_SRC_PORT'' condition. */
    fm_uint16  l4Src;

    /** TCP or UDP destination port for the ''FM_NAT_MATCH_L4_DST_PORT''
     *  condition. */
    fm_uint16  l4Dst;

    /** The logical port number to which a NAT rule should apply for the
     *  ''FM_NAT_MATCH_SRC_PORT'' condition. */
    fm_int     logicalPort;

    /** VNI field for the ''FM_NAT_MATCH_VNI'' condition. */
    fm_uint32  vni;

} fm_natConditionParam;


/*****************************************************************************/
/** \ingroup typeScalar
 * Used as an argument to ''fmAddNatRule''. It is comprised of a bit mask 
 * representing a set of actions. See ''fm_natActionMask'' for definitions 
 * of each bit in the mask.
 *****************************************************************************/
typedef fm_uint32    fm_natAction;


/*****************************************************************************/
/** \ingroup typeEnum 
 *  NAT Action Masks.
 *
 *  These bit masks are used to define the action argument of type
 *  ''fm_natAction'' in ''fmAddNatRule''. When a NAT rule "hits", one or
 *  more associated actions may be taken as indicated in the action bit
 *  mask. Note that some actions require an associated actParam argument to
 * ''fmAddNatRule'' of type ''fm_natActionParam''.
 *****************************************************************************/
typedef enum
{
    /** Set the Destination MAC address. */
    FM_NAT_ACTION_SET_DMAC = (1 << 0),

    /** Set the Source MAC address. */
    FM_NAT_ACTION_SET_SMAC = (1 << 1),

    /** Set the Destination IP. */
    FM_NAT_ACTION_SET_DIP = (1 << 2),

    /** Set the Source IP. */
    FM_NAT_ACTION_SET_SIP = (1 << 3),

    /** Set the L4 Source port. */
    FM_NAT_ACTION_SET_L4SRC = (1 << 4),

    /** Set the L4 Destination port. */
    FM_NAT_ACTION_SET_L4DST = (1 << 5),

    /** Set the TTL field. */
    FM_NAT_ACTION_SET_TTL = (1 << 6),

    /** Count the number of frames and bytes that hit this entry. */
    FM_NAT_ACTION_COUNT = (1 << 7),

    /** Add tunnel header. */
    FM_NAT_ACTION_TUNNEL = (1 << 8),

    /** Specify the VNI field of the added tunnel header. */
    FM_NAT_ACTION_SET_TUNNEL_VNI = (1 << 9),

    /** Specify the NGE data of the added tunnel header. */
    FM_NAT_ACTION_SET_TUNNEL_NGE = (1 << 10),

    /** Specify the receiving host to uses on tunnel decap. */
    FM_NAT_ACTION_SET_TUNNEL_VSI = (1 << 11),

    /** Specify the GPE header's VNI field. */
    FM_NAT_ACTION_SET_TUNNEL_GPE_VNI = (1 << 12),

    /** Specify the NSH Base Header fields (Critical Flag, Length,
     *  and MD Type). */
    FM_NAT_ACTION_SET_TUNNEL_NSH_BASE_HDR = (1 << 13),

    /** Specify the NSH Service Header fields. */
    FM_NAT_ACTION_SET_TUNNEL_NSH_SERVICE_HDR = (1 << 14),

    /** Specify the NSH header's data bytes field. */
    FM_NAT_ACTION_SET_TUNNEL_NSH_DATA = (1 << 15),

    /** Trap the frame. */
    FM_NAT_ACTION_TRAP = (1 << 16),

} fm_natActionMask;


/*****************************************************************************/
/** \ingroup typeStruct
 * This is an argument to ''fmAddNatRule''. Some NAT actions require an 
 * argument upon which the action will be performed or filtered against.
 *****************************************************************************/
typedef struct _fm_natActionParam
{
    /** Destination MAC address, for use with ''FM_NAT_ACTION_SET_DMAC''. */
    fm_macaddr   dmac;

    /** Source MAC address, for use with ''FM_NAT_ACTION_SET_SMAC''. */
    fm_macaddr   smac;

    /** Destination IP address, for use with ''FM_NAT_ACTION_SET_DIP''. */
    fm_ipAddr    dstIp;

    /** Source IP address, for use with ''FM_NAT_ACTION_SET_SIP''. */
    fm_ipAddr    srcIp;

    /** TCP or UDP source port, for use with ''FM_NAT_ACTION_SET_L4SRC''. */
    fm_uint16    l4Src;

    /** TCP or UDP destination port, for use with ''FM_NAT_ACTION_SET_L4DST''. */
    fm_uint16    l4Dst;

    /** TTL value, for use with ''FM_NAT_ACTION_SET_TTL''. */
    fm_byte      ttl;

    /** Tunnel entry created using ''fmCreateNatTunnel'', for use with
     *  ''FM_NAT_ACTION_TUNNEL''. */
    fm_int       tunnel;

    /** VNI field, for use with ''FM_NAT_ACTION_SET_TUNNEL_VNI''. */
    fm_uint32    vni;

    /** Must be set to the proper value if ''FM_NAT_ACTION_SET_TUNNEL_NGE'' is
     *  set. This is a mask indicating which of 16 words is present.  */
    fm_uint16    ngeMask;

    /** Must be set to the proper value if ''FM_NAT_ACTION_SET_TUNNEL_NGE'' is
     *  set. This is the value to set based on each word set in the mask.  */
    fm_uint32    ngeData[FM_TUNNEL_NGE_DATA_SIZE];

    /** Logical port number, for use with ''FM_NAT_ACTION_SET_TUNNEL_VSI''. */
    fm_int       logicalPort;

    /** The VNI that will be stored in the VXLAN-GPE header. */
    fm_uint32    gpeVni;

    /** Defines GPE NSH specific configuration. This should only be set if the
     *  tunnel type selected is FM_TUNNEL_TYPE_GPE_NSH. */
    fm_natNshHdr nsh;

} fm_natActionParam;




/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

fm_status fmCreateNatTable(fm_int sw, fm_int table, fm_natParam *natParam);
fm_status fmDeleteNatTable(fm_int sw, fm_int table);
fm_status fmGetNatTable(fm_int sw, fm_int table, fm_natParam *natParam);
fm_status fmGetNatTableFirst(fm_int sw, fm_int *firstTable);
fm_status fmGetNatTableNext(fm_int sw, fm_int currentTable, fm_int *nextTable);

fm_status fmSetNatTunnelDefault(fm_int sw, fm_natTunnelDefault *tunnelDefault);
fm_status fmGetNatTunnelDefault(fm_int sw, fm_natTunnelDefault *tunnelDefault);
fm_status fmCreateNatTunnel(fm_int        sw,
                            fm_int        table,
                            fm_int        tunnel,
                            fm_natTunnel *param);
fm_status fmDeleteNatTunnel(fm_int sw, fm_int table, fm_int tunnel);
fm_status fmGetNatTunnel(fm_int        sw,
                         fm_int        table,
                         fm_int        tunnel,
                         fm_natTunnel *param);
fm_status fmGetNatTunnelFirst(fm_int        sw,
                              fm_int        table,
                              fm_int *      firstNatTunnel,
                              fm_natTunnel *param);
fm_status fmGetNatTunnelNext(fm_int        sw,
                             fm_int        table,
                             fm_int        currentNatTunnel,
                             fm_int *      nextNatTunnel,
                             fm_natTunnel *param);

fm_status fmAddNatRule(fm_int                sw,
                       fm_int                table,
                       fm_int                rule,
                       fm_natCondition       condition,
                       fm_natConditionParam *cndParam,
                       fm_natAction          action,
                       fm_natActionParam *   actParam);
fm_status fmDeleteNatRule(fm_int sw, fm_int table, fm_int rule);
fm_status fmGetNatRule(fm_int                sw,
                       fm_int                table,
                       fm_int                rule,
                       fm_natCondition *     condition,
                       fm_natConditionParam *cndParam,
                       fm_natAction *        action,
                       fm_natActionParam *   actParam);
fm_status fmGetNatRuleFirst(fm_int                sw,
                            fm_int                table,
                            fm_int *              firstNatRule,
                            fm_natCondition *     condition,
                            fm_natConditionParam *cndParam,
                            fm_natAction *        action,
                            fm_natActionParam *   actParam);
fm_status fmGetNatRuleNext(fm_int                sw,
                           fm_int                table,
                           fm_int                currentNatRule,
                           fm_int *              nextNatRule,
                           fm_natCondition *     condition,
                           fm_natConditionParam *cndParam,
                           fm_natAction *        action,
                           fm_natActionParam *   actParam);

fm_status fmAddNatPrefilter(fm_int                sw,
                            fm_int                table,
                            fm_int                entry,
                            fm_natCondition       condition,
                            fm_natConditionParam *cndParam);
fm_status fmDeleteNatPrefilter(fm_int sw, fm_int table, fm_int entry);
fm_status fmGetNatPrefilter(fm_int                sw,
                            fm_int                table,
                            fm_int                entry,
                            fm_natCondition *     condition,
                            fm_natConditionParam *cndParam);
fm_status fmGetNatPrefilterFirst(fm_int                sw,
                                 fm_int                table,
                                 fm_int *              firstEntry,
                                 fm_natCondition *     condition,
                                 fm_natConditionParam *cndParam);
fm_status fmGetNatPrefilterNext(fm_int                sw,
                                fm_int                table,
                                fm_int                currentEntry,
                                fm_int *              nextEntry,
                                fm_natCondition *     condition,
                                fm_natConditionParam *cndParam);

fm_status fmGetNatRuleCount(fm_int             sw,
                            fm_int             table,
                            fm_int             rule,
                            fm_tunnelCounters *counters);
fm_status fmResetNatRuleCount(fm_int sw, fm_int table, fm_int rule);

fm_status fmDbgDumpNat(fm_int sw);

#endif /* __FM_FM_API_NAT_H */
