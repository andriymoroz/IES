/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_flow.h
 * Creation Date:   July 20, 2010
 * Description:     Constants for attributes and attribute values
 *
 * Copyright (c) 2005 - 2016, Intel Corporation
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

#ifndef __FM_FM_API_FLOW_H
#define __FM_FM_API_FLOW_H

/** Maximum number of flow table types.               \ingroup constSystem */
#define FM_FLOW_MAX_TABLE_TYPE      32

/** The static port set for all ports.                \ingroup constSystem */
#define FM_FLOW_PORT_SET_ALL        FM_PORT_SET_ALL


/****************************************************************************/
/** Flow Condition Masks
 *  \ingroup constFlowCond
 *  \page flowCondMasks
 *
 *  The following set of bit masks may be ORed together to produce an
 *  ''fm_flowCondition'' value. Each bit mask selects a field within the
 *  received frame that should be matched when a flow key is tested.
 *                                                                      \lb\lb
 *  For each bit mask specified in ''fm_flowCondition'', a corresponding
 *  value and mask must be specified in the ''fm_flowValue'' argument to
 *  ''fmAddFlow'' or ''fmModifyFlow''. The required value and mask
 *  fields of ''fm_flowValue'' are indicated for each bit mask.
 * 
 *  See also ''Flow Condition Mask Examples''.
 ****************************************************************************/
/** \ingroup constFlowCond
 * @{ */

/** Match the source MAC address. Specify src and srcMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_SRC_MAC           (FM_LITERAL_U64(1) << 0)

/** Match the destination MAC address. Specify dst and dstMask in 
 *  ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_DST_MAC           (FM_LITERAL_U64(1) << 1)

/** Match the Ethernet type. Specify ethType and ethTypeMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM10000 */
#define FM_FLOW_MATCH_ETHERTYPE         (FM_LITERAL_U64(1) << 2)

/** Match the VLAN ID. Specify vlanId and vlanIdMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_VLAN              (FM_LITERAL_U64(1) << 3)

/** Match the VLAN priority. Specify vlanPri and vlanPriMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_VLAN_PRIORITY     (FM_LITERAL_U64(1) << 4)

/** Match the IP source address. Specify srcIp and srcIpMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_SRC_IP            (FM_LITERAL_U64(1) << 5)

/** Match the IP destination address. Specify dstIp and dstIpMask in  
 *  ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_DST_IP            (FM_LITERAL_U64(1) << 6)

/** Match the L4 protocol (IPv4) or Next Header (IPv6). Specify protocol and 
 *  protocolMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM6000 devices, this condition uses the mapped value to reduce the 
 *  required number of slices needed. Up to 15 different protocols can be 
 *  mapped.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_PROTOCOL          (FM_LITERAL_U64(1) << 7)

/** Match the TCP/UDP source port.
 *                                                                      \lb\lb
 *  For FM4000 and FM10000 devices, specify L4SrcStart in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM6000 devices, this condition uses the mapped value to reduce the 
 *  required number of slices needed. Up to 32 differents L4 Src Port range 
 *  can be defined and this condition must always be paired with
 *  ''FM_FLOW_MATCH_PROTOCOL''. The entered range should not overlap with
 *  any other L4 source port range already configured for the same protocol.
 *  Specify L4SrcStart and L4SrcEnd in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_L4_SRC_PORT       (FM_LITERAL_U64(1) << 8)

/** Match the TCP/UDP destination port.
 *                                                                      \lb\lb
 *  For FM4000 and FM10000 devices, specify L4DstStart in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM6000 devices, this condition uses the mapped value to reduce the 
 *  required number of slices needed. Up to 32 differents L4 Dst Port range 
 *  can be defined and this condition must always be paired with
 *  ''FM_FLOW_MATCH_PROTOCOL''. The entered range should not overlap with
 *  any other L4 destination port range already configured for the same 
 *  protocol. Specify L4DstStart and L4DstEnd in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_L4_DST_PORT       (FM_LITERAL_U64(1) << 9)

/** Match on ingress logical port mask. Specify ingressPortMask in 
 *  ''fm_flowValue''.
 *  
 *  \chips  FM3000, FM4000 */
#define FM_FLOW_MATCH_INGRESS_PORT_MASK (FM_LITERAL_U64(1) << 10)

/** Match on ingress logical port set. Specify portSet in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM6000 devices, this condition uses the mapped value to reduce the 
 *  required number of slices needed. Up to 8 different port sets can be 
 *  defined. Matching on the port set ''FM_FLOW_PORT_SET_ALL''  does not 
 *  consume any mapped resources. For BST tables ''FM_FLOW_PORT_SET_ALL'' is not
 *  supported.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition uses the mapped value to reduce the 
 *  required number of slices needed. Up to 4 different port sets can be 
 *  defined. Matching on the port set ''FM_FLOW_PORT_SET_ALL''  does not 
 *  consume any mapped resources.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_INGRESS_PORT_SET  (FM_LITERAL_U64(1) << 11)

/** Match the Type of Service octet (IPv4) or Traffic Class octet (IPv6).
 *  Specify tos and tosMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_TOS               (FM_LITERAL_U64(1) << 12)

/** Match the Frame Type. Specify frameType in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_MATCH_FRAME_TYPE        (FM_LITERAL_U64(1) << 13)

/** Match on Multi-Stage flow action ''FM_FLOW_ACTION_SET_CONDITION'' value.
 *  Specify table1Condition and table1ConditionMask in ''fm_flowValue''.
 *  This condition should only be used on either flow TCAM tables located in FFU
 *  TCAM table 0 or flow BST table to be effective. Flow attribute
 *  ''FM_FLOW_TCAM_TABLE_SELECTION'' indicates which FFU TCAM table to use for
 *  any specific flow TCAM table.
 *  
 *  \chips  FM6000 */
#define FM_FLOW_MATCH_TABLE1_CONDITION  (FM_LITERAL_U64(1) << 14)

/** Match on a single ingress logical port. This logical port must refer to a
 *  single physical port. Specify logicalPort in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_MATCH_SRC_PORT          (FM_LITERAL_U64(1) << 15)

/** Match on the TCP flags.
 *                                                                      \lb\lb
 *  For FM6000 devices, the ''FM_PORT_CAPTURE_L4_ENTRY'' port attribute 
 *  must be set.
 *                                                                      \lb\lb
 *  For FM6000 and FM10000 devices, this condition must be paired with
 *  ''FM_FLOW_MATCH_PROTOCOL''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_MATCH_TCP_FLAGS         (FM_LITERAL_U64(1) << 16)

/** Match on the L4 deep inspection bit field.
 *                                                                      \lb\lb
 *  For FM3000/FM4000 and FM10000 devices, this implementation supports
 *  up to 32 bytes of L4 deep inspection.
 *                                                                      \lb\lb
 *  For FM6000 devices this implementation supports up to 40 bytes
 *  of L4 deep inspection.
 *                                                                      \lb\lb
 *  Note that the first two 16-bit words of the L4 payload will be compared 
 *  against the L4 source port and L4 destination port, respectively, as 
 *  specified in ''fm_flowValue''.
 *
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_L4_DEEP_INSPECTION (FM_LITERAL_U64(1) << 17)

/** Match on bytes within the payload of a non-IP frame. This condition
 *  can be used for L2 deep inspection, but it can also be used to match 
 *  bytes within the payload of an IP frame for data patterns that are
 *  not covered by the other Flow conditions.
 *                                                                      \lb\lb
 *  For FM3000/FM4000 and FM10000 devices, this implementation supports
 *  up to 32 bytes of L2 deep inspection.
 *                                                                      \lb\lb
 *  For FM6000 devices this implementation supports up to 52 bytes
 *  of L2 deep inspection.
 *
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_L2_DEEP_INSPECTION (FM_LITERAL_U64(1) << 18)

/** Match the switch priority.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_SWITCH_PRIORITY    (FM_LITERAL_U64(1) << 19)

/** Match on the VLAN tag type configured.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_MATCH_VLAN_TAG_TYPE      (FM_LITERAL_U64(1) << 20)

/** Match the VLAN2 ID.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_MATCH_VLAN2              (FM_LITERAL_U64(1) << 21)

/** Match the VLAN2 priority.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_MATCH_PRIORITY2          (FM_LITERAL_U64(1) << 22)

/** Match the IP Fragment Type.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_MATCH_FRAG               (FM_LITERAL_U64(1) << 23)

/** Match on the VNI.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_MATCH_VNI                (FM_LITERAL_U64(1) << 24)

/** Match on the VSI (encapsulation) or TEP-ID (decapsulation).
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_MATCH_VSI_TEP            (FM_LITERAL_U64(1) << 25)

/** Match on the ingress logical port. The logical port can be a physical
 *  port, a LAG, or a virtual port. This condition is similar to the
 *  ''FM_FLOW_MATCH_SRC_PORT'' condition except that in this case it
 *  matches on the GLORT that belongs to this logical port instead of
 *  the underlying physical port. Specify logicalPort in
 *  ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_MATCH_LOGICAL_PORT       (FM_LITERAL_U64(1) << 26)

/** Match the TTL field in an IPv4 header or Hop Limit in an IPv6 header.
 *  Specify ttl and ttlMask in ''fm_flowValue''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this condition is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_MATCH_TTL                (FM_LITERAL_U64(1) << 27)

/** @} (end of Doxygen group) */


/****************************************************************************/
/** Flow Condition Mask Examples
 *  \ingroup constFlowCondExample
 *  \page flowCondMasksExample
 *
 *  The following bit masks are examples of an ''fm_flowCondition''
 *  value for flow tables. Each bit mask selects fields within the
 *  received frame that should be matched when a flow key is tested.
 *                                                                      \lb\lb
 *  Selecting smaller condition sets allows a table to use fewer
 *  resources. In effect, more smaller flow tables can be configured in
 *  comparison with full 12-tuple tables.
 ****************************************************************************/
/** \ingroup constFlowCondExample
 * @{ */

/** Match all 12 possible keys (except of FM_FLOW_MATCH_TABLE1_CONDITION).
 *  This table requires 6 FFU slices when full matching masks are configured.
 *   
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_ALL_12_TUPLE \
    (FM_FLOW_MATCH_SRC_MAC           | \
     FM_FLOW_MATCH_DST_MAC           | \
     FM_FLOW_MATCH_VLAN              | \
     FM_FLOW_MATCH_VLAN_PRIORITY     | \
     FM_FLOW_MATCH_SRC_IP            | \
     FM_FLOW_MATCH_DST_IP            | \
     FM_FLOW_MATCH_TOS               | \
     FM_FLOW_MATCH_FRAME_TYPE        | \
     FM_FLOW_MATCH_L4_SRC_PORT       | \
     FM_FLOW_MATCH_L4_DST_PORT       | \
     FM_FLOW_MATCH_PROTOCOL          | \
     FM_FLOW_MATCH_INGRESS_PORT_SET)

/** Match IPv4 8-tuple: DestIP, SourceIP, L4DST, L4SRC, VLAN/VPRI/CFI,
 *  PROTOCOL and Ingress Port. This table requires 3 FFU slices when
 *  full matching masks are configured.
 * 
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_IPV4_8_TUPLE \
    (FM_FLOW_MATCH_VLAN               | \
     FM_FLOW_MATCH_VLAN_PRIORITY      | \
     FM_FLOW_MATCH_SRC_IP             | \
     FM_FLOW_MATCH_DST_IP             | \
     FM_FLOW_MATCH_L4_SRC_PORT        | \
     FM_FLOW_MATCH_L4_DST_PORT        | \
     FM_FLOW_MATCH_PROTOCOL           | \
     FM_FLOW_MATCH_INGRESS_PORT_SET)

/** Match IPv4 5-tuple: DestIP, SourceIP, L4DST, L4SRC and PROTOCOL.
 *  This table requires 3 FFU slices when full matching masks are
 *  configured.
 * 
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_IPV4_5_TUPLE \
    (FM_FLOW_MATCH_SRC_IP             | \
     FM_FLOW_MATCH_DST_IP             | \
     FM_FLOW_MATCH_L4_SRC_PORT        | \
     FM_FLOW_MATCH_L4_DST_PORT        | \
     FM_FLOW_MATCH_PROTOCOL)

/** Match L2 6-tuple: DMAC, SMAC, VLAN/VPRI/CFI, Ingress Port and Ethertype.
 *                                                                      \lb\lb
 *  For FM6000 devices, this table requires 4 FFU slices when full matching 
 *  masks are configured.
 *                                                                      \lb\lb
 *  For FM10000 devices, this table requires 3 FFU slices when full matching 
 *  masks are configured.
 * 
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_L2_6_TUPLE   \
    (FM_FLOW_MATCH_SRC_MAC            | \
     FM_FLOW_MATCH_DST_MAC            | \
     FM_FLOW_MATCH_VLAN               | \
     FM_FLOW_MATCH_VLAN_PRIORITY      | \
     FM_FLOW_MATCH_FRAME_TYPE         | \
     FM_FLOW_MATCH_INGRESS_PORT_SET)

/** Match L2 3-tuple: DMAC and VLAN/VPRI/CFI.
 *  This table requires 2 FFU slices when full matching masks are configured.
 * 
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_L2_3_TUPLE   \
    (FM_FLOW_MATCH_DST_MAC            | \
     FM_FLOW_MATCH_VLAN               | \
     FM_FLOW_MATCH_VLAN_PRIORITY)

/** Match IPv4 Destination: DestIP.
 *  This table requires 1 FFU slice when full matching masks are configured.
 * 
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_IPV4_DEST FM_FLOW_MATCH_DST_IP

/** Match Priority Vector: VPRI/CFI, IP TOS, Ethertype and Protocol.
 *  This table requires 1 FFU slice when full matching masks are configured.
 * 
 *  \chips FM6000, FM10000 */
#define FM_FLOW_TABLE_COND_VLAN_PRIORITY \
    (FM_FLOW_MATCH_VLAN_PRIORITY       | \
     FM_FLOW_MATCH_TOS                 | \
     FM_FLOW_MATCH_FRAME_TYPE          | \
     FM_FLOW_MATCH_PROTOCOL)

/** Match IPv4 8-tuple: DestIP, SourceIP, L4DST, L4SRC, VLAN/VPRI/CFI,
 *  PROTOCOL and Source Port. This table requires 3 FFU slices when
 *  full matching masks are configured.
 * 
 *  \chips FM10000 */
#define FM_FLOW_TABLE_COND_IPV4_SRC_PORT_8_TUPLE \
    (FM_FLOW_MATCH_VLAN               | \
     FM_FLOW_MATCH_VLAN_PRIORITY      | \
     FM_FLOW_MATCH_SRC_IP             | \
     FM_FLOW_MATCH_DST_IP             | \
     FM_FLOW_MATCH_L4_SRC_PORT        | \
     FM_FLOW_MATCH_L4_DST_PORT        | \
     FM_FLOW_MATCH_PROTOCOL           | \
     FM_FLOW_MATCH_SRC_PORT)

/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeEnum
 *  Flow table types, used as an argument to
 *  ''fmGetFlowTableType''
 **************************************************/
typedef enum
{
    /** FFU TCAM flow table.
     *  \chips  FM4000, FM6000, FM10000 */
    FM_FLOW_TCAM_TABLE,

    /** BST flow table.
     *  \chips  FM6000 */
    FM_FLOW_BST_TABLE,

    /** Tunnel Engine flow table.
     *  \chips  FM10000 */
    FM_FLOW_TE_TABLE,

    /** UNPUBLISHED: For internal use only. */
    FM_FLOW_TABLE_MAX

} fm_flowTableType;


/**************************************************/
/** \ingroup typeEnum
 *  Flow states, used as an argument to ''fmAddFlow''
 *  and ''fmSetFlowState''.
 **************************************************/
typedef enum
{
    /** Flow is added to the hardware in a standby (disabled) mode. */
    FM_FLOW_STATE_STANDBY,
    
    /** Flow is added to the hardware in an enabled mode. */
    FM_FLOW_STATE_ENABLED

} fm_flowState;


/**************************************************/
/** \ingroup typeEnum
 *  Possible values for the ''FM_FLOW_TCAM_TABLE_SELECTION''
 *  Flow API attribute.
 **************************************************/
typedef enum
{
    /** Use the next table resource available (default). */
    FM_FLOW_TCAM_TABLE_BEST_FIT = 0,

    /** Flows in this table can specify condition
     *  ''FM_FLOW_MATCH_TABLE1_CONDITION'' that can match
     *  ''FM_FLOW_ACTION_SET_CONDITION'' action value set in a flow TCAM table
     *  with the attribute value ''FM_FLOW_TCAM_TABLE_0''.
     *                                                                  \lb\lb
     *  Note that all the TCAM flow tables with higher tableIndex must use
     *  either the same attribute value or the value
     *  ''FM_FLOW_TCAM_TABLE_BEST_FIT''. */
    FM_FLOW_TCAM_TABLE_0,

    /** Flows in this table can specify action ''FM_FLOW_ACTION_SET_CONDITION'' 
     *  that can be matched in either BST
     *  flow table or TCAM flow table with the attribute value
     *  ''FM_FLOW_TCAM_TABLE_0''.
     *                                                                  \lb\lb
     *  Note that all the TCAM flow tables with lower tableIndex must use
     *  either the same attribute value or the value
     *  ''FM_FLOW_TCAM_TABLE_BEST_FIT''. */
    FM_FLOW_TCAM_TABLE_1,

} fm_flowTCAMTable;


/****************************************************************************/
/** \ingroup constFlowAttr
 *
 *  Flow API attributes, used as an argument to ''fmSetFlowAttribute'' and 
 *  ''fmGetFlowAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_flowGroupAttr
{
    /** Type fm_int: A read-only attribute used to get an identifier for a
     *  Default Flow Action triggered with ''FM_FLOW_ACTION_DEFAULT''. The
     *  identifier is the frame destination GLORT.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_FLOW_DEF_ACTION_CODE, 

    /** Type fm_int: A read-only attribute used to get an identifier for a
     *  Trap Action triggered with ''FM_FLOW_ACTION_TRAP''. The
     *  identifier is the frame destination GLORT.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_FLOW_TRAP_ACTION_CODE, 

    /** Type fm_int: A read-only attribute used to get an identifier for a
     *  Forward to CPU Action triggered with ''FM_FLOW_ACTION_FORWARD''
     *  coupled with the CPU logical port. The identifier is the frame
     *  destination GLORT.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_FLOW_FWD_TO_CPU_ACTION_CODE, 

    /** Type fm_bool: Specifies if this tableIndex supports priority
     *  between flows: FM_ENABLED or FM_DISABLED (default). Addition and
     *  removal of flows that are part of a table that supports priority
     *  takes more CPU resources. This attribute must be configured prior
     *  to calling ''fmCreateFlowTCAMTable''.
     *  This attribute is not supported for BST flow tables.
     *
     *  \chips  FM6000, FM10000 */
    FM_FLOW_TABLE_WITH_PRIORITY, 

    /** Type fm_bool: Specifies if a low priority catch-all flow that redirects
     *  traffic to the CPU port will be automatically added when the tableIndex
     *  is created. This attribute is only valid for tableIndex without priority
     *  support (see ''FM_FLOW_TABLE_WITH_PRIORITY''): FM_ENABLED or 
     *  FM_DISABLED (default). This attribute must be configured prior to 
     *  calling ''fmCreateFlowTCAMTable''. This attribute is supported only by
     *  a TCAM table. If this attribute is set to enabled for a BST table, the
     *  table creation will fail.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_FLOW_TABLE_WITH_DEFAULT_ACTION,

    /** Type fm_flowTCAMTable: Selects between FFU TCAM table 0 and FFU TCAM
     *  table 1 for this particular flow TCAM table. By default, the next
     *  available  table resource available will be used
     *  (''FM_FLOW_TCAM_TABLE_BEST_FIT''). This attribute must be configured
     *  prior to calling ''fmCreateFlowTCAMTable''.
     *
     *  \chips FM6000 */
    FM_FLOW_TCAM_TABLE_SELECTION,
    
    /** Type fm_flowValue: Specifies the choice of condition fields matching
     *  masks for a table. By default, condition matching masks are set to
     *  full masks for each condition. A ''fm_flowValue'' structure contains
     *  the values and masks to match against for conditions. For the purpose
     *  of this attribute, the values are ignored and only the masks for
     *  conditions are set for a table. For each flow in the table, the
     *  flow's condition matching mask must be a subset of the mask defined
     *  for the table. For a BST table, for all conditions selected for a
     *  table, flow's matching masks must be equal to table's matching
     *  masks. This attribute must be configured prior to calling
     *  ''fmCreateFlowTCAMTable'' or ''fmCreateFlowBSTTable''.
     *
     *  \chips FM6000, FM10000 */
    FM_FLOW_TABLE_COND_MATCH_MASKS,   
 
    /** Type fm_bool: Specifies if a flow table supports action
     *  ''FM_FLOW_ACTION_COUNT'' : FM_ENABLED(default) or FM_DISABLED.
     *  This attribute must be configured prior to calling
     *  ''fmCreateFlowTCAMTable'' or ''fmCreateFlowBSTTable''.
     *
     *  \chips FM6000, FM10000 */
    FM_FLOW_TABLE_WITH_COUNT,  

    /** Type fm_int: Specifies which Tunnel Engine the flow table will be 
     *  created in. Default is Tunnel Engine 0. This attribute must be 
     *  configured prior to calling ''fmCreateFlowTETable''. This attribute 
     *  is supported for TE flow tables.
     *
     *  \chips  FM10000 */
    FM_FLOW_TABLE_TUNNEL_ENGINE,

    /** Type fm_bool: Specifies whether a TE flow table is Encap (TRUE) or 
     *  Decap (FALSE). Encap flow tables can be used to push tunnel headers.
     *  Decap flow tables can be used to pop tunnel headers (if present).
     *  Default is Encap (TRUE). If TE flow table is configured as Decap,
     *  flow condition matching will be performed against inner fields of
     *  encapsulated frame. Tunnel type is specified by the tunnelType
     *  field of ''fm_flowParam''. This attribute must be configured prior to 
     *  calling ''fmCreateFlowTETable''. This attribute is supported for 
     *  TE flow tables.
     *
     *  \chips FM10000 */
    FM_FLOW_TABLE_TUNNEL_ENCAP,
    
    /** Type fm_int: A read-only attribute used to get Tunnel Group of the flow
     *  table. This attribute is supported for TE flow tables.
     *
     *  \chips  FM10000 */
    FM_FLOW_TABLE_TUNNEL_GROUP,

    /** Type fm_uint32: Specifies scenarios bitmask indicating when ACL is
     *  valid (see ''ACL Scenario Masks''). Default value:
     *  (FM_ACL_SCENARIO_ANY_FRAME_TYPE | FM_ACL_SCENARIO_ANY_ROUTING_TYPE).
     *  This attribute must be configured prior to calling
     *  ''fmCreateFlowTCAMTable''. This attribute is supported only by a TCAM
     *  table.
     *
     *  \chips  FM10000 */
    FM_FLOW_TABLE_SCENARIO,

    /** Type fm_flowCondition: A read-only attribute used to get a bit mask of
     *  matching conditions supported by a flow table.
     *
     *  \chips FM10000 */
    FM_FLOW_TABLE_CONDITION,

    /** Type fm_uint32: A read-only attribute used to get the maximum number of
     *  actions supported by rules in flow table.
     *
     *  \chips FM10000 */
    FM_FLOW_TABLE_MAX_ACTIONS,

    /** Type fm_flowAction: A read-only attribute used to get a bitmask of
     *  actions supported by rules in flow table.
     *
     *  \chips FM10000 */
    FM_FLOW_TABLE_SUPPORTED_ACTIONS,

    /** Type fm_int: A read-only attribute used to get the size of the flow
     *  table.
     *
     *  \chips FM10000 */
    FM_FLOW_TABLE_MAX_ENTRIES,

    /** Type fm_int: A read-only attribute used to get the number of empty
     *  flow entries left in the flow table.
     *
     *  \chips FM10000 */
    FM_FLOW_TABLE_EMPTY_ENTRIES,

   /** UNPUBLISHED: For internal use only. */
    FM_FLOW_ATTR_MAX
};


/**************************************************/
/** Flow Action Masks
 * \ingroup constFlowAction
 * \page flowActionMasks
 *
 *  These bit masks are used to define the action
 *  argument of type ''fm_flowAction'' in
 *  ''fmAddFlow'' and ''fmModifyFlow''. When a flow
 *  condition match occurs on a received packet,
 *  one or more associated actions may be taken as
 *  indicated in the action bit mask. Note that some
 *  actions require an associated param argument to
 * ''fmAddFlow'' or ''fmModifyFlow'' of type 
 *  ''fm_flowParam''. Actions requiring a param 
 *  argument are noted below.
 **************************************************/
/** \ingroup constFlowAction
 * @{ */

/** Forward the packet to the logical port specified in the logicalPort 
 * field of ''fm_flowParam'', which may be a port, LAG or a flood
 * logical port.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM and TE tables. Note
 *  that TE tables only supports it if the ''FM_FLOW_TABLE_TUNNEL_ENCAP''
 *  attribute is set to Decap (FALSE).
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_FORWARD           (FM_LITERAL_U64(1) << 0)

/** Bypass the flow forwarding rules and forward the packet using normal
 *  L2/L3 switch processing.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_FORWARD_NORMAL    (FM_LITERAL_U64(1) << 1)

/** Drop the packet (forward to a null destination).
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_DROP              (FM_LITERAL_U64(1) << 2) 

/** Trap the packet to the CPU.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_TRAP              (FM_LITERAL_U64(1) << 3) 

/** Redirect the packet to the CPU.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_DEFAULT           (FM_LITERAL_U64(1) << 4) 

/** Count the packet.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM and TE tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_COUNT             (FM_LITERAL_U64(1) << 5)

/** Define a multi-stage flow condition. The condition field of ''fm_flowParam''
 *  in the param argument to the ''fmAddFlow'' function specifies the value to
 *  match on using the flow condition ''FM_FLOW_MATCH_TABLE1_CONDITION''. This 
 *  action should only be used on flow TCAM tables located in FFU TCAM table 1
 *  to be effective. Flow attribute ''FM_FLOW_TCAM_TABLE_SELECTION'' indicates 
 *  which FFU TCAM table to use for any specific flow TCAM table. The assigned 
 *  value must not exceed the size of 15 bits.
 *
 *  \chips FM6000 */
#define FM_FLOW_ACTION_SET_CONDITION     (FM_LITERAL_U64(1) << 6)

/** Push a new outermost VLAN header onto the frame.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  This action is not supported in the SWAG environment due to the
 *  possibility of undesired double tagging when passing a frame from
 *  one switch to another.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_ACTION_PUSH_VLAN         (FM_LITERAL_U64(1) << 7)

/** Pop the outermost VLAN header from the frame.
 *                                                                      \lb\lb
 *  For FM10000 devices this action must always be paired with
 *  ''FM_FLOW_ACTION_SET_VLAN'' or
 *  ''FM_FLOW_ACTION_UPD_OR_ADD_VLAN'' to specify the new vlan
 *  that would be used for filtering. This action pops all
 *  parsed VLAN headers for FM10000 devices. The VLAN headers
 *  parsing configuration is specified by
 *  ''FM_PORT_PARSER_VLAN1_TAG'', ''FM_PORT_PARSER_VLAN2_TAG''
 *  and ''FM_SWITCH_PARSER_VLAN_ETYPES'' attributes.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_ACTION_POP_VLAN          (FM_LITERAL_U64(1) << 8)

/** Set VLAN ID of the frame.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_SET_VLAN          (FM_LITERAL_U64(1) << 9)

/** Set the destination MAC address.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_ACTION_SET_DMAC          (FM_LITERAL_U64(1) << 10)

/** Set the source MAC address.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM6000, FM10000 */
#define FM_FLOW_ACTION_SET_SMAC          (FM_LITERAL_U64(1) << 11)

/** Set the destination IP address.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_SET_DIP           (FM_LITERAL_U64(1) << 12)

/** Set the source IP address.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_SET_SIP           (FM_LITERAL_U64(1) << 13)

/** Set the L4 destination port.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_SET_L4DST         (FM_LITERAL_U64(1) << 14)

/** Set the L4 source port.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_SET_L4SRC         (FM_LITERAL_U64(1) << 15)

/** Set the TTL value.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_SET_TTL           (FM_LITERAL_U64(1) << 16)

/** Set the VNI value.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ENCAP_VNI         (FM_LITERAL_U64(1) << 17)

/** Set the outer Source IP.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ENCAP_SIP         (FM_LITERAL_U64(1) << 18)

/** Set the outer TTL.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ENCAP_TTL         (FM_LITERAL_U64(1) << 19)

/** Set the outer L4 Destination Port.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ENCAP_L4DST       (FM_LITERAL_U64(1) << 20)

/** Set the outer L4 Source Port.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ENCAP_L4SRC       (FM_LITERAL_U64(1) << 21)

/** Set the outer NGE Data.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ENCAP_NGE         (FM_LITERAL_U64(1) << 22)

/** Redirect the frame to one of the tunnel engine tables as specified by
 *  tableIndex and flowId fields of ''fm_flowParam''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_REDIRECT_TUNNEL   (FM_LITERAL_U64(1) << 23)

/** Route the frame to a balance group specified by the balanceGroup field of 
 *  ''fm_flowParam''. Balance group should be created by ''fmCreateFlowBalanceGrp''
 *  function.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_BALANCE           (FM_LITERAL_U64(1) << 24)

/** Route the frame to an ECMP group specified by the ecmpGroup field of 
 *  ''fm_flowParam''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_ROUTE             (FM_LITERAL_U64(1) << 25)

/** Permit frame to transit the switch.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_PERMIT            (FM_LITERAL_U64(1) << 26)

/** Deny frame from transiting the switch.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_DENY              (FM_LITERAL_U64(1) << 27)

/** Change the frame's VLAN1 priority.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_SET_VLAN_PRIORITY (FM_LITERAL_U64(1) << 28)

/** Change the frame's switch priority.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_SET_SWITCH_PRIORITY (FM_LITERAL_U64(1) << 29)

/** Change the frame's differentiated services code point.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_SET_DSCP          (FM_LITERAL_U64(1) << 30)

/** Send the frame to a load balancing group that was created with 
 *  ''fmCreateLBG''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_LOAD_BALANCE      (FM_LITERAL_U64(1) << 31)

/** If there is a miss in the destination MAC address lookup, redirect 
 *  the frame to a logical port specified in the logicalPort field of 
 *  ''fm_flowParam''.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_FLOW_ACTION_SET_FLOOD_DEST    (FM_LITERAL_U64(1) << 32)

/** Keep the outer header in place even if the decap flag is set.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_DECAP_KEEP        (FM_LITERAL_U64(1) << 33)

/** Move the outer header at the end of the packet and append the Outer
 *  Header length.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TE tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_DECAP_MOVE        (FM_LITERAL_U64(1) << 34)

/** Mirror the frame to a mirror group specified in the mirrorGrp field
 *  of ''fm_flowParam''. The selected mirror group must have the mirror
 *  attribute FM_MIRROR_ACL enabled.
 *                                                                      \lb\lb
 *  For FM10000 devices, this action is available for TCAM tables.
 *
 *  \chips  FM10000 */
#define FM_FLOW_ACTION_MIRROR_GRP            (FM_LITERAL_U64(1) << 35)

/** Either update the existing VLAN ID if a VLAN tag is present
  * in the frame or add a VLAN tag if the frame was untagged.
  *                                                                     \lb\lb
  * For FM10000 devices, this action is available for TCAM tables.
  *
  * \chips FM10000 */  
#define FM_FLOW_ACTION_UPD_OR_ADD_VLAN       (FM_LITERAL_U64(1) << 36)


/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeStruct
 * A Flow API matching condition value, used as an 
 *  argument to ''fmAddFlow'', ''fmModifyFlow'' and 
 * ''fmGetFlow''.
 *                                              \lb\lb
 * Not all members of this structure need contain
 * valid values, only those that are relevant to the
 * flow's condition (see 'fm_flowCondition').
 **************************************************/
typedef struct _fm_flowValue
{
    /** Source MAC address for the ''FM_FLOW_MATCH_SRC_MAC'' condition. */
    fm_macaddr      src;

    /** Source MAC address mask for the ''FM_FLOW_MATCH_SRC_MAC'' condition. */
    fm_macaddr      srcMask;

    /** Destination MAC address for the ''FM_FLOW_MATCH_DST_MAC'' condition. */
    fm_macaddr      dst;

    /** Destination MAC address mask for the ''FM_FLOW_MATCH_DST_MAC'' condition. */
    fm_macaddr      dstMask;

    /** Ethernet type for the ''FM_FLOW_MATCH_ETHERTYPE'' condition. */
    fm_uint16       ethType;

    /** Ethernet type mask for the ''FM_FLOW_MATCH_ETHERTYPE'' condition. */
    fm_uint16       ethTypeMask;

    /** VLAN ID for the ''FM_FLOW_MATCH_VLAN'' condition. */
    fm_uint16       vlanId;

    /** VLAN ID mask for the ''FM_FLOW_MATCH_VLAN'' condition. */
    fm_uint16       vlanIdMask;

    /** VLAN priority for the ''FM_FLOW_MATCH_VLAN_PRIORITY'' condition.
     *                                                                  \lb\lb
     *  The VLAN priority is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte         vlanPri;

    /** VLAN priority mask for the ''FM_FLOW_MATCH_VLAN_PRIORITY''
     *  condition.
     *                                                                  \lb\lb
     *  The VLAN priority mask is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte         vlanPriMask;

    /** Source IP address for the ''FM_FLOW_MATCH_SRC_IP'' condition. */
    fm_ipAddr       srcIp;

    /** Source IP address mask for the ''FM_FLOW_MATCH_SRC_IP'' condition. */
    fm_ipAddr       srcIpMask;

    /** Destination IP address for the ''FM_FLOW_MATCH_DST_IP'' condition. */
    fm_ipAddr       dstIp;

    /** Destination IP address mask for the ''FM_FLOW_MATCH_DST_IP'' condition. */
    fm_ipAddr       dstIpMask;

    /** L4 Protocol (IPv4) or Next Header (IPv6) for the 
     *  ''FM_FLOW_MATCH_PROTOCOL'' condition. */
    fm_byte         protocol;

    /** L4 Protocol (IPv4) or Next Header (IPv6) mask for the 
     *  ''FM_FLOW_MATCH_PROTOCOL'' condition. */
    fm_byte         protocolMask;

    /** TCP or UDP source port for the ''FM_FLOW_MATCH_L4_SRC_PORT'' condition. */
    fm_uint16       L4SrcStart;

    /** The last TCP or UDP source port in a range of port numbers for the 
     *  ''FM_FLOW_MATCH_L4_SRC_PORT'' condition (FM6000 only). */
    fm_uint16       L4SrcEnd;

    /** TCP or UDP source port mask for the ''FM_FLOW_MATCH_L4_SRC_PORT'' 
     *  condition. The field data in the packet will be ANDed with this 
     *  mask prior to being compared to the L4SrcStart value. */
    fm_uint16       L4SrcMask;

    /** TCP or UDP destination port for the ''FM_FLOW_MATCH_L4_DST_PORT'' 
     *  condition. */
    fm_uint16       L4DstStart;

    /** The last TCP or UDP destination port in a range of port numbers for the 
     *  ''FM_FLOW_MATCH_L4_DST_PORT'' condition (FM6000 only). */
    fm_uint16       L4DstEnd;

    /** TCP or UDP destination port mask for the ''FM_FLOW_MATCH_L4_DST_PORT''
     *  condition. The field data in the packet will be ANDed with this 
     *  mask prior to being compared to the L4DstStart value. */
    fm_uint16       L4DstMask;

    /** Mask specifying the ingress logical ports to which this rule applies
     *  for the ''FM_FLOW_MATCH_INGRESS_PORT_MASK'' condition. */
    fm_uint32       ingressPortMask;

    /** The port set number to which an flow should apply for the 
     *  ''FM_FLOW_MATCH_INGRESS_PORT_SET'' condition. The value can be a port 
     *  set number returned by a call to ''fmCreatePortSet'' or one of 
     *  the following predefined port sets:
     *                                                                  \lb\lb
     *  FM_PORT_SET_ALL - Includes all ports.
     *                                                                  \lb\lb
     *  FM_PORT_SET_ALL_BUT_CPU - Includes all ports except the
     *  CPU port.
     *                                                                  \lb\lb
     *  FM_PORT_SET_ALL_EXTERNAL - Includes all ports in a SWAG that
     *  are of type FM_SWAG_LINK_EXTERNAL. Outside of a SWAG, this is the
     *  same as FM_PORT_SET_ALL_BUT_CPU. 
     *                                                                  \lb\lb
     *  Note that this port set will override the ACL's port association as set
     *  with ''fmAddACLPort'' or ''fmAddACLPortExt'' for ingress ACLs and
     *  ingress port mask as set with the ''FM_FLOW_MATCH_INGRESS_PORT_MASK''
     *  condition. */
    fm_int32        portSet;

    /** The logical port number to which a flow should apply for the
     *  ''FM_FLOW_MATCH_SRC_PORT'' or ''FM_FLOW_MATCH_LOGICAL_PORT''
     *  condition. */
    fm_int          logicalPort;

    /** Type of Service octet (IPv4) or Traffic Class octet (IPv6) for the 
     *  ''FM_FLOW_MATCH_TOS'' condition. */
    fm_byte         tos;

    /** Mask for Type of Service octet (IPv4) or Traffic Class octet (IPv6) for 
     *  the ''FM_FLOW_MATCH_TOS'' condition. */
    fm_byte         tosMask;

    /** Frame Type for the ''FM_FLOW_MATCH_FRAME_TYPE'' condition. */
    fm_aclFrameType frameType;

    /** Multi-Stage flow value for the ''FM_FLOW_MATCH_TABLE1_CONDITION''
     *  condition. */
    fm_uint16       table1Condition;

    /** Multi-Stage flow mask for the ''FM_FLOW_MATCH_TABLE1_CONDITION''
     *  condition. For a BST, this mask must include the following bits:
     *  0x7000 (due to API internally setting one additional control bit and
     *  a 4-bit granularity requirement for BST matching masks). */
    fm_uint16       table1ConditionMask;

    /** TCP flags for the ''FM_FLOW_MATCH_TCP_FLAGS'' condition. The following 
     *  bit masks can be ORed to produce this value: FM_TCP_FLAG_ACK, 
     *  FM_TCP_FLAG_FIN, FM_TCP_FLAG_PSH, FM_TCP_FLAG_RST, FM_TCP_FLAG_SYN
     *  and FM_TCP_FLAG_URG. */
    fm_byte         tcpFlags;

    /** TCP flags mask for the ''FM_FLOW_MATCH_TCP_FLAGS'' condition. */
    fm_byte         tcpFlagsMask;

    /** A set of bytes, extracted from the L4 area of the frame for the
     *  ''FM_FLOW_MATCH_L4_DEEP_INSPECTION'' condition.
     *  The first two 16-bit words of L4 payload (following the IP 
     *  header) will always be matched against the values that appear in the 
     *  L4 source port and destination port members of this structure, even 
     *  if the frame does not have a recognized L4 protocol. This member is 
     *  used to match bytes following the first two 16-bit words of L4 payload.  
     *  Which bytes from the frame are compared to this value may be different 
     *  depending on a number of factors. See ''Deep Inspection'' in the
     *  API User Guide. */
    fm_byte         L4DeepInspection[FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES];

    /** L4 deep inspection extended bit field mask for the
     *  ''FM_FLOW_MATCH_L4_DEEP_INSPECTION'' condition. */
    fm_byte         L4DeepInspectionMask[FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES];

    /** An arbitrary contiguous field taken from the L3 portion of the
     *  frame for the ''FM_FLOW_MATCH_L2_DEEP_INSPECTION'' condition.
     *  The maximum size of this field is 50 bytes, but the actual matched
     *  portion is  determined by the ''FM_PORT_PARSER_NOT_IP_PAYLOAD'' port
     *  attribute. The bytes are interpreted in network order. */
    fm_byte         L2DeepInspection[FM_MAX_ACL_NON_IP_PAYLOAD_BYTES];

    /** Up to 52-bytes may be matched for non-IP frames for the 
     *  ''FM_FLOW_MATCH_L2_DEEP_INSPECTION'' condition. */
    fm_byte         L2DeepInspectionMask[FM_MAX_ACL_NON_IP_PAYLOAD_BYTES];

    /** Switch priority for the ''FM_FLOW_MATCH_SWITCH_PRIORITY'' condition. */
    fm_byte         switchPri;

    /** Switch priority mask for the ''FM_FLOW_MATCH_SWITCH_PRIORITY''
     *  condition. */
    fm_byte         switchPriMask;

    /** The VLAN tag type for the ''FM_FLOW_MATCH_VLAN_TAG_TYPE'' condition. */
    fm_aclVlanTagType vlanTag;

    /** VLAN2 ID for the ''FM_FLOW_MATCH_VLAN2'' condition. */
    fm_uint16       vlanId2;

    /** VLAN2 ID mask for the ''FM_FLOW_MATCH_VLAN2'' condition. */
    fm_uint16       vlanId2Mask;

    /** VLAN2 priority for the ''FM_FLOW_MATCH_PRIORITY2'' condition.
     *                                                                  \lb\lb
     *  The VLAN2 priority is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte         vlanPri2;

    /** VLAN2 priority mask for the ''FM_FLOW_MATCH_PRIORITY2'' condition.
     *                                                                  \lb\lb
     *  The VLAN2 priority mask is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte         vlanPri2Mask;

    /** Fragmentation Frame Type for the ''FM_FLOW_MATCH_FRAG'' condition. */
    fm_aclFragType  fragType;

    /** VNI for the ''FM_FLOW_MATCH_VNI'' condition */
    fm_uint32       vni;

    /** VSI (encapsulation) or TEP-ID (decapsulation) for the
     *  ''FM_FLOW_MATCH_VSI_TEP'' condition */
    fm_uint16       vsiTep;

    /** IP Time To Live (IPv4) or Hop Limit (IPv6) for the
     *  ''FM_FLOW_MATCH_TTL'' condition. */
    fm_byte         ttl;

    /** IP Time To Live (IPv4) or Hop Limit (IPv6) mask for the
     *  ''FM_FLOW_MATCH_TTL'' condition. */
    fm_byte         ttlMask;

} fm_flowValue;


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmAddFlow'', ''fmModifyFlow'',
 *  and ''fmGetFlow''. Some flow actions require an 
 *  argument upon which the action will be performed 
 *  or filtered against. The argument value is
 *  carried in this data type. 
 *                                                   \lb\lb
 *  Not all members of this structure need contain
 *  valid values, only those that are relevant to the
 *  flow's action, as noted in the descriptions
 *  of each member, below.
 **************************************************/
typedef struct _fm_flowParam
{
    /** New VLAN ID, for use with ''FM_FLOW_ACTION_SET_VLAN'' and 
     *  ''FM_FLOW_ACTION_PUSH_VLAN'' actions. */
    fm_uint16     vlan;

    /** Logical port number, for use with ''FM_FLOW_ACTION_FORWARD'' and
     *  ''FM_FLOW_ACTION_SET_FLOOD_DEST'' actions. */
    fm_int        logicalPort;

    /** Condition value, for use with ''FM_FLOW_ACTION_SET_CONDITION''. */
    fm_uint16     condition;

    /** Destination MAC address, for use with ''FM_FLOW_ACTION_SET_DMAC''. */
    fm_macaddr    dmac;

    /** Source MAC address, for use with ''FM_FLOW_ACTION_SET_SMAC''. */
    fm_macaddr    smac;

    /** TE table index, for use with ''FM_FLOW_ACTION_REDIRECT_TUNNEL''. */
    fm_int        tableIndex;

    /** Flow ID, for use with ''FM_FLOW_ACTION_REDIRECT_TUNNEL''. */
    fm_int        flowId;

    /** Balance group id, for use with ''FM_FLOW_ACTION_BALANCE''. */
    fm_int        balanceGroup;

    /** Destination IP address, for use with ''FM_FLOW_ACTION_SET_DIP''.  */
    fm_ipAddr     dip;

    /** Source IP address, for use with ''FM_FLOW_ACTION_SET_SIP''.  */
    fm_ipAddr     sip;

    /** L4 Source Port, for use with ''FM_FLOW_ACTION_SET_L4SRC''.  */
    fm_uint16     l4Src;

    /** L4 Destination Port, for use with ''FM_FLOW_ACTION_SET_L4DST''.  */
    fm_uint16     l4Dst;

    /** TTL, for use with ''FM_FLOW_ACTION_SET_TTL''.  */
    fm_byte       ttl;

    /** VNI, for use with ''FM_FLOW_ACTION_ENCAP_VNI''.  */
    fm_uint32     outerVni;

    /** Encapsulation tunnel type. Must be set for ''FM_FLOW_ACTION_ENCAP_SIP'',
     *  ''FM_FLOW_ACTION_ENCAP_TTL'', ''FM_FLOW_ACTION_ENCAP_L4SRC'',
     *  ''FM_FLOW_ACTION_ENCAP_L4DST'', ''FM_FLOW_ACTION_ENCAP_VNI'' and 
     *  ''FM_FLOW_ACTION_ENCAP_NGE''.
     *  
     *  To use FM_TUNNEL_TYPE_GPE_NSH, make sure the tunnel
     *  engine is configured in the GPE_NSH mode using the attribute
     *  ''FM_TUNNEL_API_MODE'' with the ''fmSetTunnelApiAttribute''
     *  function. This must be done BEFORE adding any flow that uses
     *  the tunnel engine.  */
    fm_tunnelType tunnelType;

    /** Destination IP address. Must be set for ''FM_FLOW_ACTION_ENCAP_SIP'',
     *  ''FM_FLOW_ACTION_ENCAP_TTL'', ''FM_FLOW_ACTION_ENCAP_L4SRC'',
     *  ''FM_FLOW_ACTION_ENCAP_L4DST'', ''FM_FLOW_ACTION_ENCAP_VNI'' and 
     *  ''FM_FLOW_ACTION_ENCAP_NGE''.  */
    fm_ipAddr     outerDip;

    /** Source IP address, for use with ''FM_FLOW_ACTION_ENCAP_SIP''.  */
    fm_ipAddr     outerSip;

    /** TTL, for use with ''FM_FLOW_ACTION_ENCAP_TTL''.  */
    fm_byte       outerTtl;

    /** L4 Source Port, for use with ''FM_FLOW_ACTION_ENCAP_L4SRC''.  */
    fm_uint16     outerL4Src;

    /** L4 Destination Port, for use with ''FM_FLOW_ACTION_ENCAP_L4DST''.  */
    fm_uint16     outerL4Dst;

    /** NGE Mask, for use with ''FM_FLOW_ACTION_ENCAP_NGE''.
     *  This is a mask indicating which of 16 words is present.  */
    fm_uint16     outerNgeMask;

    /** NGE Data, for use with ''FM_FLOW_ACTION_ENCAP_NGE''.
     *  This is the value to set based on each word set in the mask.  */
    fm_uint32     outerNgeData[FM_TUNNEL_NGE_DATA_SIZE];

    /** Must be set to the proper value if fm_flowParam.tunnelType
     *  == FM_TUNNEL_TYPE_GPE_NSH.
     *  
     *  This is length of the NSH header in 4-byte words including
     *  the Base Header, Service Path Header and Context Data. */
    fm_byte       outerNshLength;

    /** Must be set to the proper value if fm_flowParam.tunnelType
     *  == FM_TUNNEL_TYPE_GPE_NSH.
     *  
     *  This bit should be set if there are critical TLV that are included in
     *  the NSH data. */
    fm_bool       outerNshCritical;  
      
    /** Must be set to the proper  
     *  value if fm_flowParam.tunnelType == FM_TUNNEL_TYPE_GPE_NSH.
     *
     *  This field should contain the MD Type. */
    fm_byte       outerNshMdType;

    /** Must be set to the proper value if fm_flowParam.tunnelType
     *  == FM_TUNNEL_TYPE_GPE_NSH.
     *  
     *  This field should contain the Service Path ID. */
    fm_uint32     outerNshSvcPathId;

    /** Must be set to the proper value if fm_flowParam.tunnelType
     *  == FM_TUNNEL_TYPE_GPE_NSH.
     *
     *  This field should contain the Service Index. */
    fm_byte       outerNshSvcIndex;

    /** Must be set to the proper value if fm_flowParam.tunnelType
     *  == FM_TUNNEL_TYPE_GPE_NSH.
     *  
     *  This field should contain the NSH context data that follows
     *  the service header. */
    fm_uint32     outerNshData[FM_TUNNEL_NSH_DATA_SIZE];

    /** Must be set to the proper value if fm_flowParam.tunnelType
     *  == FM_TUNNEL_TYPE_GPE_NSH.
     *  
     *  This field is a bitmask of the valid 32-bit words in nshData. */
    fm_uint16     outerNshDataMask;

    /** ECMP group id, for use with ''FM_FLOW_ACTION_ROUTE''. */
    fm_int        ecmpGroup;

    /** New VLAN1 priority, for use with ''FM_FLOW_ACTION_SET_VLAN_PRIORITY''.
     *  The VLAN priority is 4 bits and will be remapped by
     *  the ''FM_QOS_TX_PRIORITY_MAP'' port QoS attribute on egress.
     *  (Also see the ''FM_PORT_TXCFI'' port attribute.) */
    fm_byte       vlanPriority;

    /** New switch priority, for use with
     *  ''FM_FLOW_ACTION_SET_SWITCH_PRIORITY''. */
    fm_byte       switchPriority;

    /** New DSCP value, for use with ''FM_FLOW_ACTION_SET_DSCP''. */
    fm_byte       dscp;

    /** Load balancer number, for use with ''FM_FLOW_ACTION_LOAD_BALANCE''. */
    fm_int        lbgNumber;

    /** Mirror group number, for use with ''FM_FLOW_ACTION_MIRROR_GRP''. */
    fm_byte       mirrorGrp;

} fm_flowParam;


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmGetFlowCount''.
 **************************************************/
typedef struct _fm_flowCounters
{
    /** Number of packets counted. */
    fm_uint64 cntPkts;

    /** Number of bytes counted. */
    fm_uint64 cntOctets;

} fm_flowCounters;


/**************************************************/
/** \ingroup typeScalar
 *  A Flow API matching condition, used as an argument 
 *  to ''fmAddFlow'', ''fmCreateFlowTCAMTable'',
 *  and ''fmModifyFlow'' . It comprises a bit mask
 *  representing a set of matching conditions. 
 *  See ''Flow Condition Masks'' for definitions
 *  of each bit in the mask.
 **************************************************/
typedef fm_uint64       fm_flowCondition;


/**************************************************/
/** \ingroup typeScalar
 *  A Flow API action to be taken on a matching 
 *  condition, used as an argument 
 *  to ''fmAddFlow'' and ''fmModifyFlow'' . It
 *  comprises a bit mask representing a set of 
 *  actions. See ''Flow Action Masks'' for 
 *  definitions of each bit in the mask.
 **************************************************/
typedef fm_uint64       fm_flowAction;


fm_status fmCreateFlowTCAMTable(fm_int           sw, 
                                fm_int           tableIndex, 
                                fm_flowCondition condition,
                                fm_uint32        maxEntries,
                                fm_uint32        maxAction);

fm_status fmDeleteFlowTCAMTable(fm_int sw, 
                                fm_int tableIndex);

fm_status fmCreateFlowBSTTable(fm_int           sw, 
                               fm_int           tableIndex, 
                               fm_flowCondition condition,
                               fm_uint32        maxEntries,
                               fm_uint32        maxAction);

fm_status fmDeleteFlowBSTTable(fm_int sw, 
                               fm_int tableIndex);

fm_status fmCreateFlowTETable(fm_int           sw, 
                              fm_int           tableIndex, 
                              fm_flowCondition condition,
                              fm_uint32        maxEntries,
                              fm_uint32        maxAction);

fm_status fmDeleteFlowTETable(fm_int sw, 
                              fm_int tableIndex);

fm_status fmAddFlow(fm_int           sw, 
                    fm_int           tableIndex,
                    fm_uint16        priority,
                    fm_uint32        precedence, 
                    fm_flowCondition condition,
                    fm_flowValue    *condVal,
                    fm_flowAction    action,
                    fm_flowParam    *param,
                    fm_flowState     flowState,
                    fm_int          *flowId);

fm_status fmGetFlow(fm_int             sw, 
                    fm_int             tableIndex,
                    fm_int             flowId,
                    fm_flowCondition * flowCond,
                    fm_flowValue *     flowValue,
                    fm_flowAction *    flowAction,
                    fm_flowParam *     flowParam,
                    fm_int *           priority,
                    fm_int *           precedence);

fm_status fmGetFlowTableType(fm_int             sw,
                             fm_int             tableIndex,
                             fm_flowTableType * flowTableType);

fm_status fmGetFlowFirst(fm_int   sw,
                         fm_int * tableIndex);

fm_status fmGetFlowNext(fm_int   sw,
                        fm_int   currentTable,
                        fm_int * tableIndex);

fm_status fmGetFlowRuleFirst(fm_int   sw,
                             fm_int   tableIndex,
                             fm_int * firstRule);

fm_status fmGetFlowRuleNext(fm_int   sw,
                            fm_int   tableIndex,
                            fm_int   currentRule,
                            fm_int * nextRule);

fm_status fmModifyFlow(fm_int           sw, 
                       fm_int           tableIndex,
                       fm_int           flowId,
                       fm_uint16        priority,
                       fm_uint32        precedence, 
                       fm_flowCondition condition,
                       fm_flowValue    *condVal,
                       fm_flowAction    action,
                       fm_flowParam    *param);

fm_status fmDeleteFlow(fm_int sw, fm_int tableIndex, fm_int flowId);

fm_status fmSetFlowState(fm_int       sw, 
                         fm_int       tableIndex, 
                         fm_int       flowId, 
                         fm_flowState flowState);

fm_status fmGetFlowCount(fm_int           sw, 
                         fm_int           tableIndex, 
                         fm_int           flowId,
                         fm_flowCounters *counters);

fm_status fmResetFlowCount(fm_int sw, 
                           fm_int tableIndex, 
                           fm_int flowId);

fm_status fmGetFlowUsed(fm_int   sw, 
                        fm_int   tableIndex, 
                        fm_int   flowId,
                        fm_bool  clear,
                        fm_bool *used);

fm_status fmSetFlowAttribute(fm_int sw,
                             fm_int tableIndex,
                             fm_int attr,
                             void  *value);

fm_status fmGetFlowAttribute(fm_int sw,
                             fm_int tableIndex,
                             fm_int attr,
                             void  *value);

fm_status fmCreateFlowBalanceGrp(fm_int  sw,
                                 fm_int *groupId);

fm_status fmDeleteFlowBalanceGrp(fm_int sw,
                                 fm_int groupId);

fm_status fmAddFlowBalanceGrpEntry(fm_int sw,
                                   fm_int groupId,
                                   fm_int tableIndex,
                                   fm_int flowId);

fm_status fmDeleteFlowBalanceGrpEntry(fm_int sw,
                                      fm_int groupId,
                                      fm_int tableIndex,
                                      fm_int flowId);

#endif /* __FM_FM_API_FLOW_H */
