/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_acl.h
 * Creation Date:   March 27, 2006
 * Description:     Structures and functions for dealing with ACLs
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

#ifndef __FM_FM_API_ACL_H
#define __FM_FM_API_ACL_H


/* Since ACLs are stored in trees now, the only constraint on FM_MAX_ACLS
 * and FM_MAX_ACL_RULES is that they must fit in a 32-bit signed integer. */

/* The maximum number of Access Control Lists. */
#define FM_MAX_ACLS                           0x7fffffff

/* The maximum number of ACL rules. */
#define FM_MAX_ACL_RULES                      0x7fffffff

/** The default ACL precedence.                         \ingroup constSystem */
#define FM_ACL_DEFAULT_PRECEDENCE             4

/** The maximum ACL precedence.                         \ingroup constSystem */
#define FM_MAX_ACL_PRECEDENCE                 7

/** The default ACL instance.                           \ingroup constSystem */
#define FM_ACL_NO_INSTANCE                   -1

/** The maximum number of ACLs that belongs to the same instance.
 *  \ingroup constSystem */
#define FM_MAX_ACL_PER_INSTANCE               16

/** The maximum number of bytes to match for a non-IP paylod. This value
 *  differ for each switch family.
 *  FM4000, FM10000 : 32 bytes
 *  FM6000 : 50 bytes */
#define FM_MAX_ACL_NON_IP_PAYLOAD_BYTES       50

/** The maximum number of bytes to match for a L4 deep inspection. This value
 *  differs for each switch family.
 *  FM4000, FM10000 : 32 bytes
 *  FM6000 : 38 bytes */
#define FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES   38

/** The maximum number of bytes that can be matched for a L4 deep inspection
 *  using IPv6 scenarios. This value differs for each switch family.
 *  FM4000, FM10000 : 8 bytes
 *  FM6000 : 12 bytes */
#define FM_MAX_ACL_IPV6_L4_DEEP_INSPECTION_BYTES 12

/** The maximum number of policer banks associated with an ACL rule.
 *  \ingroup    constSystem
 *  \chips      FM6000, FM10000 */
#define FM_MAX_ACL_RULE_POLICER_BANKS           4

/**************************************************/
/** \ingroup typeEnum
 * An ACL rule specifies one of these actions be
 * taken if the frame evaluates true against the
 * rule and the rule "hits" (the rule is the
 * winning priority rule in the ACL's rule set).
 * Used as an argument to ''fmAddACLRule''.
 **************************************************/
typedef enum
{
    /** Permit frame to transit the switch if the matching condition is
     *  true. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ACTION_PERMIT,

    /** Permit all frames unconditionally. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ACTION_PERMIT_ALL,

    /** Deny frame from transiting the switch if the matching condition is
     *  true. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ACTION_DENY,

    /** Deny all frames unconditionally. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ACTION_DENY_ALL,

    /** Intercept the frame and send it to the CPU instead of its intended
     *  destination if the matching condition is true. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ACTION_TRAP,

    /** Count frame if the matching condition is true. Note: this action is
     *  deprecated. Use the ''FM_ACL_ACTIONEXT_COUNT'' extended action instead. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ACTION_COUNT,

    /** Mirror frame if the source or destination address matching condition
     *  is true.  Note: this action is deprecated. Use the
     *  ''FM_ACL_ACTIONEXT_MIRROR'' extended action instead. 
     *
     *  \chips  FM2000, FM3000, FM4000 */
    FM_ACL_ACTION_MONITOR,

    /* ---- Add new actions above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_ACL_ACTION_MAX

} fm_aclAction;


/**************************************************/
/** ACL Extended Action Masks
 * \ingroup constAclExtAction
 * \page aclOptionMasks
 *
 *  These bit masks are used to define the action
 *  argument of type ''fm_aclActionExt'' in
 *  ''fmAddACLRuleExt''. When an ACL rule "hits,"
 *  one or more associated actions may be taken as
 *  indicated in the action bit mask. Note that some
 *  actions require an associated param argument to
 * ''fmAddACLRuleExt'' of type ''fm_aclParamExt''.
 *  Actions requiring a param argument are noted.
 **************************************************/
/** \ingroup constAclExtAction
 * @{ */

/** Permit frame to transit the switch if the matching condition is
 *  true. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_PERMIT               (FM_LITERAL_U64(1) << 0)

/** Deny frame from transiting the switch if the matching condition is
 *  true.
 *                                                                      \lb\lb
 *  For FM6000 devices this action supersedes any ACL action except for 
 *  ''FM_ACL_ACTIONEXT_PERMIT'' when resolving multiple parallel 
 *  ACL actions. This behavior can be bypassed by using the 
 *  ''FM_ACL_ACTIONEXT_REDIRECT'' action instead along with a destination
 *  logical port with no physical ports in its destination mask. In that case,
 *  ACL precedence is respected among parallel matching ACLs with the
 *  following actions: ''FM_ACL_ACTIONEXT_LOAD_BALANCE'',
 *  ''FM_ACL_ACTIONEXT_REDIRECT'', ''FM_ACL_ACTIONEXT_ROUTE'' and
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''.
 *  
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_DENY                 (FM_LITERAL_U64(1) << 1)

/** Intercept the frame and send it to the CPU instead of its intended
 *  destination if the matching condition is true. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_TRAP                 (FM_LITERAL_U64(1) << 2)

/** Send a copy of the frame to an additional port, specified by the
 *  mirrorPort field of ''fm_aclParamExt''. 
 *
 *  \chips  FM2000, FM3000, FM4000 */
#define FM_ACL_ACTIONEXT_MIRROR               (FM_LITERAL_U64(1) << 3)

/** Copy the frame to the CPU. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_LOG                  (FM_LITERAL_U64(1) << 4)

/** Count frame and the number of octets in the frame. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_COUNT                (FM_LITERAL_U64(1) << 5)

/** Not Supported. Count frame and send a notification to the CPU. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_ACTIONEXT_COUNT_NOTIFY         (FM_LITERAL_U64(1) << 6)

/** Police the flow indicated by the ACL rule in a manner described by the
 *  associated policer. The ''fm_aclParamExt'' param argument to the
 *  ''fmAddACLRuleExt'' function indicates the policer number associated
 *  with the rule. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_POLICE               (FM_LITERAL_U64(1) << 7)

/** Change the frame's VLAN1.  The ''fm_aclParamExt'' param argument
 *  to the ''fmAddACLRuleExt'' function indicates the new VLAN value to
 *  set. 
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually exclusive
 *  actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN''                                           \lb
 *  ''FM_ACL_ACTIONEXT_PUSH_VLAN''                                          \lb
 *  ''FM_ACL_ACTIONEXT_POP_VLAN''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_VLAN             (FM_LITERAL_U64(1) << 8)

/** Change the frame's VLAN1 priority.  The param argument
 *  to the ''fmAddACLRuleExt'' function indicates the new VLAN priority value
 *  to set.  The interpretation of the VLAN priority depends on the
 *  ''FM_QOS_TX_PRIORITY_MAP'' QoS attribute and the ''FM_PORT_TXCFI'' port 
 *  attribute of the egress port.
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY''                                  \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY''                                 \lb
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY''                                \lb
 *  ''FM_ACL_ACTIONEXT_SET_DSCP''                                           \lb
 *  ''FM_ACL_ACTIONEXT_MIRROR_GRP''                                         \lb
 *                                                                          \lb
 *  With the exception of ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY'', 
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY'' and ''FM_ACL_ACTIONEXT_SET_DSCP''
 *  which can be grouped together under the same rule. Any permutation based
 *  on these 3 actions (group of 2 or 3 actions) can also be achieved.
 *  Grouping of these actions is only possible on system that doesn't
 *  support TRILL.
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY    (FM_LITERAL_U64(1) << 9)

/** Change the frame's switch priority.  The param argument
 *  to the ''fmAddACLRuleExt'' function indicates the new switch priority value
 *  to set.
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY''                                  \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY''                                 \lb
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY''                                \lb
 *  ''FM_ACL_ACTIONEXT_SET_DSCP''                                           \lb
 *  ''FM_ACL_ACTIONEXT_MIRROR_GRP''                                         \lb
 *                                                                          \lb
 *  With the exception of ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY'', 
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY'' and ''FM_ACL_ACTIONEXT_SET_DSCP''
 *  which can be grouped together under the same rule. Any permutation based
 *  on these 3 actions (group of 2 or 3 actions) can also be achieved.
 *  Grouping of these actions is only possible on system that doesn't
 *  support TRILL.
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY  (FM_LITERAL_U64(1) << 10)

/** Change the frame's differentiated services code point.
 *  The param argument to the ''fmAddACLRuleExt'' function indicates the new
 *  DSCP value to set. 
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY''                                  \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY''                                 \lb
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY''                                \lb
 *  ''FM_ACL_ACTIONEXT_SET_DSCP''                                           \lb
 *  ''FM_ACL_ACTIONEXT_MIRROR_GRP''                                         \lb
 *                                                                          \lb
 *  With the exception of ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY'', 
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY'' and ''FM_ACL_ACTIONEXT_SET_DSCP''
 *  which can be grouped together under the same rule. Any permutation based
 *  on these 3 actions (group of 2 or 3 actions) can also be achieved.
 *  Grouping of these actions is only possible on system that doesn't
 *  support TRILL.
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_DSCP             (FM_LITERAL_U64(1) << 11)

/** Change the frame's user bits.
 *  The param argument to the ''fmAddACLRuleExt'' function indicates the new
 *  user bits value to set. 
 *
 *  \chips  FM3000, FM4000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_USER             (FM_LITERAL_U64(1) << 12)

/** Send the frame to a load balancing group that was created with 
 *  ''fmCreateLBG''. The ''fm_aclParamExt'' param argument to the 
 *  ''fmAddACLRuleExt'' function indicates the target load balancing group 
 *  logical port.
 *                                                                     \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_LOAD_BALANCE''                                       \lb
 *  ''FM_ACL_ACTIONEXT_REDIRECT''                                           \lb
 *  ''FM_ACL_ACTIONEXT_ROUTE''                                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''                                     \lb
 *  ''FM_ACL_ACTIONEXT_VN_TUNNEL''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_DMAC''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_LOAD_BALANCE         (FM_LITERAL_U64(1) << 13)

/** Trap the frame to the CPU regardless of STP state. This also
 *  includes specially handled frame as well if the ACL scenario
 *  is set to hit on those frames.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_TRAP_ALWAYS          (FM_LITERAL_U64(1) << 14)

/** Redirect the frame to a logical port specified in the
 *  logicalPort field of ''fm_aclParamExt'', ignoring the
 *  result of the the destination MAC address lookup. 
 *                                                                     \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_LOAD_BALANCE''                                       \lb
 *  ''FM_ACL_ACTIONEXT_REDIRECT''                                           \lb
 *  ''FM_ACL_ACTIONEXT_ROUTE''                                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''                                     \lb
 *  ''FM_ACL_ACTIONEXT_VN_TUNNEL''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_DMAC''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_REDIRECT             (FM_LITERAL_U64(1) << 15)

/** Ignore the route action if the matching condition is true. 
 *
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_NOROUTE              (FM_LITERAL_U64(1) << 16)

/** Route the frame to an ECMP group.
 *                                                                     \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_LOAD_BALANCE''                                       \lb
 *  ''FM_ACL_ACTIONEXT_REDIRECT''                                           \lb
 *  ''FM_ACL_ACTIONEXT_ROUTE''                                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''                                     \lb
 *  ''FM_ACL_ACTIONEXT_VN_TUNNEL''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_DMAC''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  \chips  FM4000, FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_ROUTE                (FM_LITERAL_U64(1) << 17)

/** If there is a miss in the destination MAC address lookup, redirect 
 *  the frame to a logical port specified in the logicalPort field of 
 *  ''fm_aclParamExt''.
 *                                                                     \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually exclusive
 *  actions:                                                                \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_LOAD_BALANCE''                                       \lb
 *  ''FM_ACL_ACTIONEXT_REDIRECT''                                           \lb
 *  ''FM_ACL_ACTIONEXT_ROUTE''                                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''                                     \lb
 *  ''FM_ACL_ACTIONEXT_VN_TUNNEL''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_DMAC''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  \chips  FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_FLOOD_DEST       (FM_LITERAL_U64(1) << 18)

/** Set the L3 hashing profile with the index specified in the
 *  l3HashProfile field of ''fm_aclParamExt''. This action
 *  should only be used on ACLs located prior to the remap stage
 *  to be effective. ACL attribute ''FM_ACL_TABLE_SELECTION''
 *  indicates which FFU table to use for any specific ACL.
 *  
 *  Note that the configured profile will be used for the selection
 *  of the L2 hashing profile if it is not reset.
 *  
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_L3_HASH_PROFILE  (FM_LITERAL_U64(1) << 19)

/** Send a copy of the frame to one or more previously defined mirror
 *  groups, as specified by the mirrorGrp field of ''fm_aclParamExt''. 
 *  The selected mirror group must have the mirror attribute ''FM_MIRROR_ACL'' 
 *  enabled.
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY''                                  \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY''                                 \lb
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY''                                \lb
 *  ''FM_ACL_ACTIONEXT_SET_DSCP''                                           \lb
 *  ''FM_ACL_ACTIONEXT_MIRROR_GRP''                                         \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_MIRROR_GRP           (FM_LITERAL_U64(1) << 20)

/** Change the frame's VLAN2.  The ''fm_aclParamExt'' param argument
 *  to the ''fmAddACLRuleExt'' function indicates the new VLAN value to
 *  set.
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''                               \lb
 *  ''FM_ACL_ACTIONEXT_SET_CONDITION''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''                                        \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_VLAN2            (FM_LITERAL_U64(1) << 21)

/** Change the frame's VLAN2 priority.  The param argument
 *  to the ''fmAddACLRuleExt'' function indicates the new VLAN priority value
 *  to set.  The interpretation of the VLAN priority depends on the
 *  ''FM_QOS_TX_PRIORITY_MAP'' QoS attribute and the ''FM_PORT_TXCFI'' port 
 *  attribute of the egress port.
 *                                                                      \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY''                                  \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY''                                 \lb
 *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY''                                \lb
 *  ''FM_ACL_ACTIONEXT_SET_DSCP''                                           \lb
 *  ''FM_ACL_ACTIONEXT_MIRROR_GRP''                                         \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY   (FM_LITERAL_U64(1) << 22)

/** Change the frame's source GLORT.  The param argument to the
 *  ''fmAddACLRuleExt'' function specifies the new source GLORT value to
 *  set. The assigned GLORT must not exceed a size of 12 bits.
 *                                                                      \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''                               \lb
 *  ''FM_ACL_ACTIONEXT_SET_CONDITION''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''                                        \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_SRC_GLORT        (FM_LITERAL_U64(1) << 23)

/** Assign a TRILL Distribution Tree if there is a miss in the
 *  destination MAC address lookup. The param argument to the
 *  ''fmAddACLRuleExt'' function specifies the distribution tree
 *  ID to set.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_TRILL_DIST_TREE  (FM_LITERAL_U64(1) << 24)

/** Set the frame's source Tunnel ID.  The param argument to the
 *  ''fmAddACLRuleExt'' function specifies the new Tunnel ID
 *  value to set. 
 *                                                                      \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''                               \lb
 *  ''FM_ACL_ACTIONEXT_SET_CONDITION''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''                                        \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a
 *  received frame, only the action of the highest precedence
 *  ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID (FM_LITERAL_U64(1) << 25)

/** Set the action rule precedence.  The param argument to the
 *  ''fmAddACLRuleExt'' function specifies the rule precedence
 *  value to set. 
 *                                                                      \lb\lb
 *  The precedence value entered must be included into the range defined
 *  by the proper API properties. For FM6000 devices, those properties are:
 *  "api.FM6000.ffu.ACLPrecedenceMin" and "api.FM6000.ffu.ACLPrecedenceMax".
 *  For FM10000 devices, those properties are:
 *  "api.FM10000.ffu.ACLPrecedenceMin" and "api.FM10000.ffu.ACLPrecedenceMax".
 *
 *  \chips  FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_SET_PRECEDENCE        (FM_LITERAL_U64(1) << 26)

/** Set the frame's source Tunnel ID and Source GLORT. The param
 *  argument to the ''fmAddACLRuleExt'' function specifies the new
 *  Tunnel ID and Source GLORT value to set. 
 *                                                                      \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''                               \lb
 *  ''FM_ACL_ACTIONEXT_SET_CONDITION''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''                                        \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a
 *  received frame, only the action of the highest precedence
 *  ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT  (FM_LITERAL_U64(1) << 27)

/** Tunnel frame if the matching condition is true. The
 *  tunnel ID must match one of the IDs used with a call to
 *  ''fmCreateVNTunnel''.
 *                                                                     \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_LOAD_BALANCE''                                       \lb
 *  ''FM_ACL_ACTIONEXT_REDIRECT''                                           \lb
 *  ''FM_ACL_ACTIONEXT_ROUTE''                                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''                                     \lb
 *  ''FM_ACL_ACTIONEXT_VN_TUNNEL''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_DMAC''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  
 *  \chips FM6000 */
#define FM_ACL_ACTIONEXT_VN_TUNNEL             (FM_LITERAL_U64(1) << 28)

/** Define a Multi-Stage ACL condition.  The param argument to the
 *  ''fmAddACLRuleExt'' function specifies the value to match on using
 *  the ACL condition ''FM_ACL_MATCH_TABLE1_CONDITION''. This action
 *  should only be used on ACLs located prior to the remap stage
 *  to be effective. ACL attribute ''FM_ACL_TABLE_SELECTION''
 *  indicates which FFU table to use for any specific ACL.
 *  The assigned value must not exceed the size of 15 bits.
 *                                                                      \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''                               \lb
 *  ''FM_ACL_ACTIONEXT_SET_CONDITION''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''                                        \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_CONDITION          (FM_LITERAL_U64(1) << 29)

/** Push a new outermost VLAN header onto the frame if the matching 
 *  condition is true. The ''fm_aclParamExt'' param argument to the
 *  ''fmAddACLRuleExt'' function indicates the new VLAN ID value to set.
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually exclusive
 *  actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN''                                           \lb
 *  ''FM_ACL_ACTIONEXT_PUSH_VLAN''                                          \lb
 *  ''FM_ACL_ACTIONEXT_POP_VLAN''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *
 *  \chips FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_PUSH_VLAN              (FM_LITERAL_U64(1) << 30)

/** Pop the outermost VLAN header from the frame if the matching 
 *  condition is true.
 *                                                                      \lb\lb
 *  For FM6000 devices this action is one of a set of mutually exclusive
 *  actions:                                                                \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN''                                           \lb
 *  ''FM_ACL_ACTIONEXT_PUSH_VLAN''                                          \lb
 *  ''FM_ACL_ACTIONEXT_POP_VLAN''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *                                                                      \lb\lb
 *  For FM10000 devices this action also pop VLAN2 header and must always be
 *  paired with ''FM_ACL_ACTIONEXT_SET_VLAN'' to specify the new VLAN that
 *  would be used for filtering.
 *
 *  \chips FM6000, FM10000 */
#define FM_ACL_ACTIONEXT_POP_VLAN               (FM_LITERAL_U64(1) << 31)

/** Set the destination MAC address if the matching condition is true. MAC
 *  lookup stage will match against the updated MAC address for any forwarding
 *  decision.
 *                                                                     \lb\lb
 *  This action is one of a set of mutually exclusive actions:              \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_LOAD_BALANCE''                                       \lb
 *  ''FM_ACL_ACTIONEXT_REDIRECT''                                           \lb
 *  ''FM_ACL_ACTIONEXT_ROUTE''                                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST''                                     \lb
 *  ''FM_ACL_ACTIONEXT_VN_TUNNEL''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_DMAC''                                           \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a received
 *  frame, only the action of the highest precedence ACL will take effect.
 *  \chips FM6000 */
#define FM_ACL_ACTIONEXT_SET_DMAC               (FM_LITERAL_U64(1) << 32)

/** Set the source MAC address if the matching condition is true. For
 *  internal use only. MAC learning will be performed against the
 *  original MAC and not the updated one.
 *
 *  \chips FM6000 */
#define FM_ACL_ACTIONEXT_SET_SMAC               (FM_LITERAL_U64(1) << 33)

/** Set the MPLS Traffic Class within an MPLS label. This action can be used
 *  to actually set a the same or different traffic class to two different MPLS
 *  labels. A 16-bit value is to carry both TCs. The lowest 8 bits are used for
 *  setting the TC of the first MPLS label (i.e. the outmost label) and the
 *  highest 8 bits are used for the second MPLS label. The MPLS TC is a 3-bit
 *  field within the MPLS label. It is actually a 3-bit field concatenated with
 *  the Bottom-of-Stack (BOS) bit for a total of 4 bits. The latter being the
 *  LSB bit. Note that BOS bit should be set to 0. As en example, to set the TC
 *  value 1 (<0010>) to the first label and value 7 (<1110>) to the second
 *  label, then value 0x0E02 must be used. The param argument to the
 *  ''fmAddACLRuleExt'' function specifies the new Traffic Class values to set.
 *                                                                      \lb\lb
 *  For FM6000 devices, this action is one of a set of mutually
 *  exclusive actions:                                                      \lb
 *                                                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_VLAN2''                                          \lb
 *  ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''                              \lb
 *  ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''                               \lb
 *  ''FM_ACL_ACTIONEXT_SET_CONDITION''                                      \lb
 *  ''FM_ACL_ACTIONEXT_SET_SMAC''                                           \lb
 *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''                                        \lb
 *                                                                          \lb
 *  If multiple ACLs specifying any of these actions hit on a
 *  received frame, only the action of the highest precedence
 *  ACL will take effect.
 *
 *  \chips  FM6000 */
#define FM_ACL_ACTIONEXT_SET_MPLS_TC (FM_LITERAL_U64(1) << 34)

/** Set one or multiple FFU Trig ID bits used as a matching condition at
 *  the Trigger Stage evaluation. Trig ID's must be previously allocated with
 *  ''fmAllocateTriggerResource''.
 *
 *  \chips FM10000 */
#define FM_ACL_ACTIONEXT_SET_TRIG_ID            (FM_LITERAL_U64(1) << 35)

/** Redirect the frame to one of the tunnel engines as specified by the
 *  tunnelGroup and tunnelRule fields of ''fm_aclParamExt'', ignoring the
 *  result of the the destination MAC address lookup. Tunnel Group must be
 *  previously created with ''fmCreateTunnel'' to be referenced. Tunnel Rule
 *  must match one of the previously created rules as specified with
 *  ''fmAddTunnelRule'' if the Tunnel Group is of type direct. For a
 *  hash lookup tunnel group, the tunnelRule field of ''fm_aclParamExt''
 *  refers to the TEP to match on if ''FM_TUNNEL_MATCH_VSI_TEP'' is
 *  selected, and must be set to zero otherwise.
 *
 *  \chips FM10000 */
#define FM_ACL_ACTIONEXT_REDIRECT_TUNNEL        (FM_LITERAL_U64(1) << 36)

/** Enable capturing egress timestamp of packets on ethernet ports. Port 
 *  attribute ''FM_PORT_EGRESS_TIMESTAMP_EVENTS'' should be enabled on the
 *  ethernet port in order to capture egress timestamp. This action is valid 
 *  only when the packets egress on ethernet ports, it is not applicable to
 *  PCIE ports.
 *
 *  \chips FM10000 */
#define FM_ACL_ACTIONEXT_CAPTURE_EGRESS_TIMESTAMP (FM_LITERAL_U64(1) << 37)

/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeScalar
 * Used as an argument to ''fmAddACLRule''. Some
 * ACL actions require an argument upon which
 * the action will be performed or against which
 * it will be filtered.
 **************************************************/
typedef fm_uint64    fm_aclParam;


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmAddACLRuleExt''.
 *  Some ACL actions require an argument upon which
 *  the action will be performed or filtered against.
 *                                                   \lb\lb
 *  Not all members of this structure need contain
 *  valid values, only those that are relevant to the
 *  ACL rule's action, as noted in the descriptions
 *  of each member, below.
 **************************************************/
typedef struct _fm_aclParamExt
{
    /** Logical port to mirror to, for use with ''FM_ACL_ACTIONEXT_MIRROR''. */
    fm_int    mirrorPort;

    /** Mirror group to mirror to, for use with ''FM_ACL_ACTIONEXT_MIRROR_GRP''.
     *  
     *  For FM6000 devices, this is a bit mask where bit N represents
     *  mirror group N. Multiple mirror groups may be set at once.
     *                                                                  \lb\lb
     *  For FM10000 devices, this field specifies the mirror group Id. */
    fm_byte   mirrorGrp;

    /** Policer number (see 'fmCreatePolicer') for use with the
     *  ''FM_ACL_ACTIONEXT_POLICE'' action. */
    fm_int    policer;

    /** New VLAN1 ID, for use with ''FM_ACL_ACTIONEXT_SET_VLAN''. */
    fm_uint16 vlan;

    /** New VLAN1 priority, for use with ''FM_ACL_ACTIONEXT_SET_VLAN_PRIORITY''.
     *  The VLAN priority is 4 bits and will be remapped by
     *  the ''FM_QOS_TX_PRIORITY_MAP'' port QoS attribute on egress.
     *  (Also see the ''FM_PORT_TXCFI'' port attribute.) */
    fm_byte   vlanPriority;

    /** New VLAN2 ID, for use with ''FM_ACL_ACTIONEXT_SET_VLAN2''. */
    fm_uint16 vlan2;

    /** New VLAN2 priority, for use with ''FM_ACL_ACTIONEXT_SET_VLAN2_PRIORITY''.
     *  The VLAN priority is 4 bits and will be remapped by
     *  the ''FM_QOS_TX_PRIORITY_MAP'' port QoS attribute on egress.
     *  (Also see the ''FM_PORT_TXCFI'' port attribute.) */
    fm_byte   vlanPriority2;

    /** New switch priority, for use with
     *  ''FM_ACL_ACTIONEXT_SET_SWITCH_PRIORITY''. */
    fm_byte   switchPriority;

    /** New DSCP value, for use with ''FM_ACL_ACTIONEXT_SET_DSCP''. */
    fm_byte   dscp;

    /** New user bits, for use with ''FM_ACL_ACTIONEXT_SET_USER''. */
    fm_byte   user;

    /** Mask indicating which user bits to set. */
    fm_byte   userMask;

    /** Load balancer number, for use with ''FM_ACL_ACTIONEXT_LOAD_BALANCE''. */
    fm_int    lbgNumber;

    /** Logical port number, for use with ''FM_ACL_ACTIONEXT_REDIRECT'' and
     *  ''FM_ACL_ACTIONEXT_SET_FLOOD_DEST'' actions. */
    fm_int    logicalPort;

    /** ECMP group id, for use with ''FM_ACL_ACTIONEXT_ROUTE''. */
    fm_int    groupId;

    /** L3 Hashing profile, for use with ''FM_ACL_ACTIONEXT_SET_L3_HASH_PROFILE''. */
    fm_int    l3HashProfile;

    /** New Source GLORT value, for use with ''FM_ACL_ACTIONEXT_SET_SRC_GLORT''
     *  and ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''. */
    fm_uint16 srcGlort;

    /** Assigned Distribution Tree value, for use with
     *  ''FM_ACL_ACTIONEXT_SET_TRILL_DIST_TREE''. */
    fm_int    distTree;

    /** Tunnel ID value, for use with ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''
     *  and ''FM_ACL_ACTIONEXT_SET_TUNNEL_ID_SGLORT''. */
    fm_uint16 tunnelId;

    /** Virtual Network Tunnel ID, for use with ''FM_ACL_ACTIONEXT_VN_TUNNEL''. */
    fm_int    vnTunnel;

    /** Virtual Network VSID (VNI), for use with ''FM_ACL_ACTIONEXT_VN_TUNNEL''
     *  and ''FM_ACL_ACTIONEXT_SET_INGRESS_TUNNEL_ID''. */
    fm_uint32 vni;

    /** Precedence value, for use with ''FM_ACL_ACTIONEXT_SET_PRECEDENCE''. */
    fm_byte   precedence;

    /** Condition value, for use with ''FM_ACL_ACTIONEXT_SET_CONDITION''. */
    fm_uint16 condition;

    /** Destination MAC address, for use with ''FM_ACL_ACTIONEXT_SET_DMAC''. */
    fm_macaddr   dmac;

    /** Source MAC address, for use with ''FM_ACL_ACTIONEXT_SET_SMAC''. */
    fm_macaddr   smac;

    /** MPLS Traffic Class value, for use with
     *  ''FM_ACL_ACTIONEXT_SET_MPLS_TC''. */
    fm_uint16 mplsTc;

    /** FFU Trig ID bits, for use with ''FM_ACL_ACTIONEXT_SET_TRIG_ID''. */
    fm_byte   trigId;

    /** Mask indicating which FFU Trig ID bits to set. This should normally
     *  only includes FFU Trig ID bits allocated with
     *  ''fmAllocateTriggerResource''. */
    fm_byte   trigIdMask;

    /** Tunnel Group, for use with ''FM_ACL_ACTIONEXT_REDIRECT_TUNNEL''. */
    fm_int    tunnelGroup;

    /** Tunnel Rule (direct tunnel group type) or TEP (hash tunnel group type),
     *  for use with ''FM_ACL_ACTIONEXT_REDIRECT_TUNNEL''. */
    fm_int    tunnelRule;

} fm_aclParamExt;


/**************************************************/
/** \ingroup typeEnum
 * Used as an argument to ''fmAddACLPort''.
 * These enumerated values are used to configure an
 * ACL for enforcement on either ingress or egress.
 **************************************************/
typedef enum
{
    /** Enforce ACL on ingress. 
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_TYPE_INGRESS = 0,

    /** Enforce ACL on egress. 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_TYPE_EGRESS,

    /** UNPUBLISHED: For internal use only. */
    FM_ACL_TYPE_MAX

} fm_aclType;


/**************************************************/
/** \ingroup typeEnum
 *  Used as an argument to ''fmSetACLRuleState''
 *  and ''fmGetACLRuleState''. These enumerated 
 *  values identify the state of an ACL rule. 
 **************************************************/
typedef enum
{
    /** ACL rule is invalid.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_RULE_ENTRY_STATE_INVALID,
    
    /** ACL rule is valid.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_RULE_ENTRY_STATE_VALID

} fm_aclEntryState;


/**************************************************/
/** \ingroup typeEnum
 * Used to define which VLAN Tag Type to match on 
 * when the ACL condition 
 * ''FM_ACL_MATCH_VLAN_TAG_TYPE'' is used.
 **************************************************/
typedef enum
{
    /** Only match on untagged frames or unrecognized tag types.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_NONE = 0,

    /** For FM3000 and FM4000 devices this matches on the VLAN tag
     *  defined by ''FM_PORT_PARSER_CVLAN_TAG''.
     *                                                                  \lb\lb
     *  For FM6000 and FM10000 devices this matches on the VLAN tag defined by
     *  ''FM_SWITCH_VLAN1_ETHERTYPE_A'' or ''FM_SWITCH_VLAN1_ETHERTYPE_B''.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_STANDARD,

    /** Match on the VLAN tag defined by ''FM_PORT_PARSER_VLAN_TAG_A''
     *  \chips  FM3000, FM4000 */
    FM_ACL_VLAN_TAG_TYPE_USER_A,

    /** Match on the VLAN tag defined by ''FM_PORT_PARSER_VLAN_TAG_B''
     *  \chips  FM3000, FM4000 */
    FM_ACL_VLAN_TAG_TYPE_USER_B,

    /** Match on single or double tagged frame with VLAN tag defined by
     *  ''FM_SWITCH_VLAN1_ETHERTYPE_A'' or ''FM_SWITCH_VLAN1_ETHERTYPE_B''.
     *  \chips  FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_VLAN1,

    /** Match on single or double tagged frame with VLAN tag defined by
     *  ''FM_SWITCH_VLAN2_ETHERTYPE_A'' or ''FM_SWITCH_VLAN2_ETHERTYPE_B''.
     *  \chips  FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_VLAN2,

    /** Match on single tagged frame with VLAN tag defined by
     *  ''FM_SWITCH_VLAN1_ETHERTYPE_A'' or ''FM_SWITCH_VLAN1_ETHERTYPE_B''.
     *  \chips  FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN1,

    /** Match on single tagged frame with VLAN tag defined by
     *  ''FM_SWITCH_VLAN2_ETHERTYPE_A'' or ''FM_SWITCH_VLAN2_ETHERTYPE_B''.
     *  \chips  FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_ONLY_VLAN2,

    /** Match on double tagged frame with CVLAN tag defined by
     *  ''FM_SWITCH_VLAN1_ETHERTYPE_A'' or ''FM_SWITCH_VLAN1_ETHERTYPE_B'' and
     *  SVLAN tag defined by ''FM_SWITCH_VLAN2_ETHERTYPE_A'' or
     *  ''FM_SWITCH_VLAN2_ETHERTYPE_B''.
     *  \chips  FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_VLAN1_VLAN2,

    /** Match on untagged VLAN2 frames or unrecognized tag types defined by
     *  ''FM_SWITCH_VLAN2_ETHERTYPE_A'' or ''FM_SWITCH_VLAN2_ETHERTYPE_B''.
     *  \chips  FM6000, FM10000 */
    FM_ACL_VLAN_TAG_TYPE_VLAN2_UNTAG

} fm_aclVlanTagType;


/**************************************************/
/** \ingroup typeEnum
 * Used to define which ISL Frame Type to match on 
 * when the ACL condition 
 * ''FM_ACL_MATCH_ISL_FTYPE'' is used.
 **************************************************/
typedef enum
{
    /** Match on all incoming traffic.
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ACL_ISL_FTYPE_NORMAL = 0,

    /** Match on IP routed frames.
     *  \chips  FM3000, FM4000 */
    FM_ACL_ISL_FTYPE_ROUTED,

    /** Match on specially handled frames.
     *                                                           \lb\lb
     *  For FM10000 devices this condition should only be used on ACLs
     *  that include the ''FM_ACL_SCENARIO_SPECIAL_GLORT'' scenario.
     *  \chips  FM6000, FM10000 */
    FM_ACL_ISL_FTYPE_SPECIAL

} fm_aclIslFrameType;


/**************************************************/
/** \ingroup typeEnum
 *  Referenced by ''fm_aclValue'', used to specify
 *  which type of frame to match on when the
 *  ''FM_ACL_MATCH_FRAME_TYPE''  ACL condition is 
 *  used.
 **************************************************/
typedef enum
{
    /** Match on non-IP frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAME_TYPE_NON_IP,

    /** Match on IPv4 frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAME_TYPE_IPV4,

    /** Match on IPv6 frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAME_TYPE_IPV6,

} fm_aclFrameType;


/**************************************************/
/** \ingroup typeEnum
 *  Referenced by ''fm_aclValue'', used to specify
 *  which type of frame to match on when the
 *  ''FM_ACL_MATCH_TRILL_TYPE''  ACL condition is 
 *  used.
 **************************************************/
typedef enum
{
    /** Match on Unicast TRILL type frames. */
    FM_ACL_TRILL_TYPE_UNICAST,

    /** Match on Multicast TRILL type frames. */
    FM_ACL_TRILL_TYPE_MULTICAST,

    /** Match on any TRILL type frames. */
    FM_ACL_TRILL_TYPE_ANY,

    /** Match on none TRILL type frames. */
    FM_ACL_TRILL_TYPE_NONE,

} fm_aclTrillType;


/**************************************************/
/** \ingroup typeEnum
 * Used to define which type of fragmented frame 
 * to match on when the ACL condition 
 * ''FM_ACL_MATCH_FRAG'' is used.
 **************************************************/
typedef enum
{
    /** Match on unfragmented frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAG_COMPLETE,

    /** Match on header fragment frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAG_HEAD,

    /** Match on subsequent fragment frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAG_SUB,

    /** Match on unfragmented or header fragment frames.
     *  \chips  FM6000, FM10000 */
    FM_ACL_FRAG_COMPLETE_OR_HEAD,

} fm_aclFragType;


/**************************************************/
/** \ingroup typeEnum
 *  Possible values for the ''FM_ACL_TABLE_SELECTION''
 *  ACL attribute.
 **************************************************/
typedef enum
{
    /** Use the next table resource available (default). */
    FM_ACL_TABLE_BEST_FIT = 0,

    /** Use the FFU table located after the remap stage.
     *                                                                  \lb\lb
     *  Note that all the ACLs with higher precedence must be assigned
     *  to the same table or use the value ''FM_ACL_TABLE_BEST_FIT''. */
    FM_ACL_TABLE_0,

    /** Use the FFU table located before the remap stage.
     *                                                                  \lb\lb
     *  Note that all the ACLs with lower precedence must be assigned
     *  to the same table or use the value ''FM_ACL_TABLE_BEST_FIT''. */
    FM_ACL_TABLE_1,

    /** Use the BST table located after the remap stage. This table selection
     *  is limited to some restrictions. */
    FM_ACL_TABLE_BST,

} fm_aclTable;


/**************************************************/
/** \ingroup typeEnum 
 *  Specifies the type of an ACL bank. 
 *  Referenced by ''fm_aclRulePolicerInfo''.
 **************************************************/
typedef enum
{
    /** This bank is not used. 
     *  \chips  FM6000, FM10000 */
    FM_ACL_BANK_TYPE_NONE = 0,

    /** This bank is a POLICER bank. 
     *  \chips  FM6000, FM10000 */
    FM_ACL_BANK_TYPE_POLICE,

    /** This bank is a COUNTER bank. 
     *  \chips  FM6000, FM10000 */
    FM_ACL_BANK_TYPE_COUNT

} fm_aclBankType;


/**************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmAddACLPortExt''.
 * This structure is used when associating ACLs with
 * ports, because you need to know both which port
 * to associate it with, and whether to associate
 * it as an ingress or egress ACL.
 **************************************************/
typedef struct _fm_aclPortAndType
{
    /** The logical port number */
    fm_int     port;

    /** Ingress or egress? */
    fm_aclType type;

} fm_aclPortAndType;


/**************************************************/
/** \ingroup typeStruct
 * Used as an argument to ''fmGetPortACLFirstExt''
 * and ''fmGetPortACLNextExt''. This structure is
 * used when querying the ACLs associated with a
 * port, because you need to know both the ACL
 * number and the type of the association.
 **************************************************/
typedef struct _fm_aclAclAndType
{
    /** The ACL number */
    fm_int     acl;

    /** Ingress or egress? */
    fm_aclType type;

} fm_aclAclAndType;


/**************************************************/
/** \ingroup typeStruct
 * Used for the information returned by ''fmGetACL''
 **************************************************/
typedef struct _fm_aclArguments
{
    /** The scenarios associated with the ACL, being a bitmask of
     *  scenarios, indicating when this ACL is valid (see
     *  ''ACL Scenario Masks'').*/
    fm_uint32 scenarios;

    /** The precedence of the ACL. */
    fm_int    precedence;

} fm_aclArguments;


/**************************************************/
/** \ingroup typeScalar
 * An ACL condition is used as an argument to
 * ''fmAddACLRule'' and ''fmAddACLRuleExt''. It 
 * comprises a bit mask representing a set of tests.
 * See ''ACL Condition Masks'' for definitions of 
 * each bit in the condition mask.
 *                                              \lb\lb
 *  Note that a rule will match on all frames if none
 *  of the condition bits are set.
 **************************************************/
typedef fm_uint64    fm_aclCondition;


/**************************************************/
/** \ingroup typeScalar
 * An ACL extended action is used as an argument to
 * ''fmAddACLRuleExt''. It comprises a bit mask
 * representing a set of actions. See
 * ''ACL Extended Action Masks'' for definitions of
 * each bit in the extended action mask.
 **************************************************/
typedef fm_uint64    fm_aclActionExt;


/**************************************************/
/** ACL Fragmentation Flag Masks
 *  \ingroup constAclFrag
 *  \page aclFragMasks
 *
 *  The following set of bit masks may be ORed
 *  together to produce a value for the flags member
 *  of an ''fm_aclValue'' structure.
 **************************************************/
/** \ingroup constAclFrag
 * @{ */

/** An IP option was flagged.  (Which options to flag are configured
 *  in the per-port PARSE_CFG register.) 
 *
 *  \chips  FM3000, FM4000, FM10000 */
#define FM_ACL_FLAG_IP_OPTIONS_FLAGGED   (1 << 0)

/** The Don't Fragment bit from the IPv4 header.  (This bit is always true
 *  for IPv6.) 
 *
 *  \chips  FM3000, FM4000, FM10000 */
#define FM_ACL_FLAG_DONTFRAG             (1 << 1)

/** The frame is the first fragment.  (This includes the case where
 *  the frame is unfragmented.) 
 *
 *  \chips  FM3000, FM4000, FM10000 */
#define FM_ACL_FLAG_HEADFRAG             (1 << 2)

/** The frame is not the last fragment. 
 *
 *  \chips  FM10000 */
#define FM_ACL_FLAG_MOREFRAG             (1 << 3)

/** @} (end of Doxygen group) */


/**************************************************/
/** ACL Condition Masks
 *  \ingroup constAclCond
 *  \page aclCondMasks
 *
 *  The following set of bit masks may be ORed
 *  together to produce an ''fm_aclCondition'' value.
 *  Each bit mask selects a field within the received
 *  frame that should be matched when an ACL is
 *  tested.
 *                                              \lb\lb
 *  The field in the frame being processed will be
 *  compared against values specified in the
 *  ''fm_aclValue'' structure. See the definition of
 *  that structure for the association between its
 *  various members and these condition bits.
 *                                              \lb\lb
 *  Note that a rule will match on all frames if none
 *  of the condition bits are set.
 **************************************************/
/** \ingroup constAclCond
 * @{ */

/** Match the source MAC address. 
 *
 *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SRC_MAC             (FM_LITERAL_U64(1) << 0)

/** Match the destination MAC address. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_DST_MAC             (FM_LITERAL_U64(1) << 1)

/** Match the Ethernet type.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_ETHERTYPE           (FM_LITERAL_U64(1) << 2)

/** Match the VLAN ID. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_VLAN                (FM_LITERAL_U64(1) << 4)

/** Match the VLAN priority. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_PRIORITY            (FM_LITERAL_U64(1) << 5)

/** Match the IP source address. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SRC_IP              (FM_LITERAL_U64(1) << 6)

/** Match the IP destination address. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_DST_IP              (FM_LITERAL_U64(1) << 7)

/** Match the TTL field in an IPv4 header or Hop Limit in an IPv6 header. 
 * 
 *  For FM6000 devices this condition does not match Hop Limit in an IPv6
 *  header.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_TTL                 (FM_LITERAL_U64(1) << 8)

/** Match the L4 protocol (IPv4) or Next Header (IPv6). 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_PROTOCOL            (FM_LITERAL_U64(1) << 9)

/** Match the miscellaneous flags. 
 *                                                                      \lb\lb
 * On FM6000 devices, use ''FM_ACL_MATCH_FRAG''.
 *
 *  \chips  FM3000, FM4000, FM10000 */
#define FM_ACL_MATCH_FLAGS               (FM_LITERAL_U64(1) << 10)

/** Match the differentiated services code point - the six most significant
 *  bits of the Type of Service octet (IPv4) or Traffic Class octet (IPv6). 
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_DSCP                (FM_LITERAL_U64(1) << 11)

/** Match the IPv6 Flow Label. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_FLOW                (FM_LITERAL_U64(1) << 12)

/** Match the TCP/UDP source port. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_L4_SRC_PORT         (FM_LITERAL_U64(1) << 13)

/** Match the TCP/UDP destination port. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_L4_DST_PORT         (FM_LITERAL_U64(1) << 14)

/** Match on a range of TCP/UDP source ports. 
 *                                                                      \lb\lb
 *  Note: Mixing use of this condition with ''FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK''
 *  may result in unintended behavior.
 *                                                                      \lb\lb
 *  On FM6000 devices, use ''FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK'' or 
 *  ''Mapper Services'' of type ''FM_MAPPER_L4_SRC''. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_L4_SRC_PORT_RANGE   (FM_LITERAL_U64(1) << 15)

/** Match on a range of TCP/UDP destination ports. 
 *                                                                      \lb\lb
 *  Note: Mixing use of this condition with ''FM_ACL_MATCH_L4_DST_PORT_WITH_MASK''
 *  may result in unintended behavior.
 *                                                                      \lb\lb
 *  On FM6000 devices, use ''FM_ACL_MATCH_L4_DST_PORT_WITH_MASK'' or 
 *  ''Mapper Services'' of type ''FM_MAPPER_L4_DST''. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_L4_DST_PORT_RANGE   (FM_LITERAL_U64(1) << 16)

/** Match the L4 deep inspection bit field. The first two 16-bit words of the 
 *  L4 payload will be compared against the L4 source port and L4 destination 
 *  port, respectively, as specified in ''fm_aclValue''. The following 8 bytes
 *  of the payload are then compared against the L4 deep inspection field
 *  subject to the L4 deep inspection mask.
 *                                                                      \lb\lb
 *  See also ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT''.
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_L4_DEEP_INSPECTION  (FM_LITERAL_U64(1) << 17)

/** Match the switch priority. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SWITCH_PRIORITY     (FM_LITERAL_U64(1) << 18)

/** Match on the TCP flags (which are really part of deep inspection, but
 *  we are abstracting that away).
 *                                                                      \lb\lb
 *  For FM6000 and FM10000 devices this condition must be paired with
 *  ''FM_ACL_MATCH_PROTOCOL''.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_TCP_FLAGS           (FM_LITERAL_U64(1) << 19)

/** Match on a TCP/UDP source port with a mask. 
 *                                                                      \lb\lb
 *  Note: Mixing use of this condition with ''FM_ACL_MATCH_L4_SRC_PORT_RANGE''
 *  may result in unintended behavior.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK (FM_LITERAL_U64(1) << 20)

/** Match on a TCP/UDP destination port with a mask.
 *                                                                      \lb\lb
 *  Note: Mixing use of this condition with ''FM_ACL_MATCH_L4_DST_PORT_RANGE''
 *  may result in unintended behavior.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_L4_DST_PORT_WITH_MASK (FM_LITERAL_U64(1) << 21)

/** Match on bytes within the payload of a non-IP frame. This condition
 *  can be used for L2 deep inspection, but it can also be used to match 
 *  bytes within the payload of an IP frame for data patterns that are
 *  not covered by the other ACL conditions.
 *                                                                      \lb\lb
 *  For FM3000, FM4000 and FM10000 devices this implementation supports
 *  up to 32 bytes of L2 deep inspection.
 *                                                                      \lb\lb
 *  For FM6000 devices this implementation supports up to 52 bytes
 *  of L2 deep inspection.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_NON_IP_PAYLOAD     (FM_LITERAL_U64(1) << 22)

/** Match on ingress logical port mask. Note that this condition will override
 *  the ACL's port association set with ''fmAddACLPort'' or ''fmAddACLPortExt''
 *  for ingress ACLs. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_INGRESS_PORT_MASK  (FM_LITERAL_U64(1) << 23)

/* (1 << 24) is defined below, outside this Doxygen block */

/** Match on ingress logical port set. Note that this condition will override
 *  the ACL's port association as set with ''fmAddACLPort'' or 
 *  ''fmAddACLPortExt'' for ingress ACLs and ingress port mask as set with
 *  the ''FM_ACL_MATCH_INGRESS_PORT_MASK'' condition.
 *                                                                      \lb\lb
 *  For FM6000 and FM10000 devices it is not supported to combine this condition with 
 *  the ACL's port association within one ACL as this is mutually exclusive.
 *                                                                      \lb\lb
 *  For FM6000 devices, this condition is not available for ACLs located in
 *  the BST.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_INGRESS_PORT_SET   (FM_LITERAL_U64(1) << 28)

/** Match on the VLAN tag type configured.
 * 
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_VLAN_TAG_TYPE      (FM_LITERAL_U64(1) << 29)

/** Match the Type of Service octet (IPv4) or Traffic Class octet (IPv6).
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_TOS                (FM_LITERAL_U64(1) << 30)

/** Match the source port mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SOURCE_MAP         (FM_LITERAL_U64(1) << 31)

/** Match the protocol mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_PROTOCOL_MAP       (FM_LITERAL_U64(1) << 32)

/** Match the L4 source port mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_L4_SRC_PORT_MAP    (FM_LITERAL_U64(1) << 33)

/** Match the L4 destination port mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_L4_DST_PORT_MAP    (FM_LITERAL_U64(1) << 34)

/** Match the destination MAC address mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_DST_MAC_MAP        (FM_LITERAL_U64(1) << 35)

/** Match the source MAC address mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SRC_MAC_MAP        (FM_LITERAL_U64(1) << 36)

/** Match the Ethernet type mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_ETH_TYPE_MAP       (FM_LITERAL_U64(1) << 37)

/** Match the IP length mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_IP_LENGTH_MAP      (FM_LITERAL_U64(1) << 38)

/** Match the IP destination address mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_DST_IP_MAP         (FM_LITERAL_U64(1) << 39)

/** Match the IP source address mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SRC_IP_MAP         (FM_LITERAL_U64(1) << 40)

/** Match on the L4 deep inspection bit field (extended version).
 *                                                                      \lb\lb
 *  For FM3000, FM4000 and FM10000 devices this implementation supports
 *  up to 32 bytes of L4 deep inspection.
 *                                                                      \lb\lb
 *  For FM6000 devices this implementation supports up to 40 bytes
 *  of L4 deep inspection.
 *                                                                      \lb\lb
 *  Note that the first two 16-bit words of the L4 payload will be compared 
 *  against the L4 source port and L4 destination port, respectively, as 
 *  specified in ''fm_aclValue''.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT (FM_LITERAL_U64(1) << 41)

/** Match on the IP length.
 *                                                                      \lb\lb
 *  On FM6000 devices, use ''FM_ACL_MATCH_IP_LENGTH_MAP''.
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_MATCH_IP_LENGTH          (FM_LITERAL_U64(1) << 42)

/** Match on ISL Frame Type.
 * 
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_ISL_FTYPE          (FM_LITERAL_U64(1) << 43)

/** Match on ISL USER.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_ISL_USER           (FM_LITERAL_U64(1) << 44)

/** Match the source glort.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_SRC_GLORT          (FM_LITERAL_U64(1) << 45)

/** Match the VLAN2 ID. 
 *
 *  \chips  FM6000, FM10000 */
#define FM_ACL_MATCH_VLAN2              (FM_LITERAL_U64(1) << 46)

/** Match the VLAN2 priority. 
 *
 *  \chips  FM6000, FM10000 */
#define FM_ACL_MATCH_PRIORITY2          (FM_LITERAL_U64(1) << 47)

/** Match the IP Fragment Type.
 * 
 *  \chips  FM6000, FM10000 */
#define FM_ACL_MATCH_FRAG               (FM_LITERAL_U64(1) << 48)

/** Match the destination glort.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_DST_GLORT          (FM_LITERAL_U64(1) << 49)

/** Match on the Frame Type.
 * 
 *  \chips  FM6000, FM10000 */
#define FM_ACL_MATCH_FRAME_TYPE         (FM_LITERAL_U64(1) << 50)

/** Match the VLAN ID mapper.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_MATCH_VLAN_MAP           (FM_LITERAL_U64(1) << 51)

/** Match predefined L4 header control data for a supported IP Protocol.
 *  This condition must be paired with ''FM_ACL_MATCH_PROTOCOL'', and the
 *  ''FM_PORT_CAPTURE_L4_ENTRY'' attribute must be enabled on the port.
 *  Supported protocol and extracted data are:
 *                                                                      \lb\lb
 *  TCP  : [Offset(4-bit) | reserved(3-bit) | ECN(3-bit) |Control(6-bit)]
 *                                                                      \lb\lb
 *  ICMP : [Type(8-bit) | Code(8-bit)]
 *
 *  \chips  FM6000 */
#define FM_ACL_MATCH_L4_ENTRY           (FM_LITERAL_U64(1) << 52)

/** Match the TRILL Source MAC Address (Outer Source MAC).
 *  
 *  \chips  FM6000 */
#define FM_ACL_MATCH_TRILL_SRC_MAC      (FM_LITERAL_U64(1) << 53)

/** Match on TRILL encapsulated frame.
 * 
 *  \chips  FM6000 */
#define FM_ACL_MATCH_TRILL_TYPE         (FM_LITERAL_U64(1) << 54)

/** Match on Source RBridge Nickname.
 *  
 *  \chips  FM6000 */
#define FM_ACL_MATCH_TRILL_SRB          (FM_LITERAL_U64(1) << 55)

/** Match on Remote RBridge Nickname.
 *  
 *  \chips  FM6000 */
#define FM_ACL_MATCH_TRILL_RRB          (FM_LITERAL_U64(1) << 56)

/** Match on Multi-Stage ACL action ''FM_ACL_ACTIONEXT_SET_CONDITION'' value.
 * 
 *  For FM6000 devices this condition should only be used on ACLs
 *  located after the remap stage to be effective. ACL attribute
 *  ''FM_ACL_TABLE_SELECTION'' indicates which FFU table to use
 *  for any specific ACL.
 *  
 *  \chips  FM6000 */
#define FM_ACL_MATCH_TABLE1_CONDITION   (FM_LITERAL_U64(1) << 57)

/** Match on Tunnel Network Identifier. Supported Tunneling protocol and
 *  extracted data are:
 *                                                                      \lb\lb
 *  VXLAN: VNI(24-bit)
 *  
 *  \chips  FM6000 */
#define FM_ACL_MATCH_TUNNEL_ID          (FM_LITERAL_U64(1) << 59)

/** Match on ACL scenario flags.
 *   
 *  \chips  FM6000, FM10000 */
#define FM_ACL_MATCH_SCENARIO_FLAGS     (FM_LITERAL_U64(1) << 60)

/** Match on a single ingress logical port. This logical port must refer to a
 *  single physical port. ACL Condition ''FM_ACL_MATCH_SRC_GLORT'' can be used
 *  to match on LAG or Multicast Group logical port.
 *   
 *  \chips  FM10000 */
#define FM_ACL_MATCH_SRC_PORT           (FM_LITERAL_U64(1) << 61)

/** @} (end of Doxygen group) */

/* For internal use only - don't include in API document: */

/* Match a mapped L4 source port. This condition is generated internally 
 * by the ACL compiler when optimizing an FM_ACL_MATCH_L4_SRC_PORT_RANGE
 * condition. */ 
#define FM_ACL_MATCH_MAP_L4_SRC_PORT    (FM_LITERAL_U64(1) << 24)

/* Match a mapped L4 destination port. This condition is generated
 * internally by the ACL compiler when optimizing an
 * FM_ACL_MATCH_L4_DST_PORT_RANGE condition. */
#define FM_ACL_MATCH_MAP_L4_DST_PORT    (FM_LITERAL_U64(1) << 25)

/* Match a mapped IP source address. This condition is generated internally
 * by the ACL compiler when optimizing an FM_ACL_MATCH_SRC_IP condition. */
#define FM_ACL_MATCH_MAP_SRC_IP         (FM_LITERAL_U64(1) << 26)

/* Match a mapped IP destination address. This condition is generated
 * internally by the ACL compiler when optimizing an FM_ACL_MATCH_DST_IP
 * condition. */
#define FM_ACL_MATCH_MAP_DST_IP         (FM_LITERAL_U64(1) << 27)

/* Match the second VLAN ID mapper. This may be used internally to match
 * on a specific uCode related attribute. */
#define FM_ACL_MATCH_VLAN2_MAP          (FM_LITERAL_U64(1) << 58)



/**************************************************/
/** ACL Scenario Flags 
 *  \ingroup constAclScenarioFlags
 *  \page aclScenarioFlags
 *
 *  The following set of bit masks may be ORed
 *  together to produce a value for the "scenarioFlags"
 *  and "scenarioFlagsMask" fields of ''fm_aclValue''.
 *                                              \lb\lb
 *  Note that these flags are FM6000 centric but partially
 *  supported on FM10000 as well.
 **************************************************/

/* Internal comment - don't include in API document: 
 * ACL scenario flag mask values are hard-coded from ucode files.
 * If ucode scenario values are changed, then we need to also change
 * these values. */

/** \ingroup constAclScenarioFlags
 * @{ */

/* ----- Frame Type ----- */

/** Match normal frame type. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAME_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAME_TYPE_NORMAL                  (0 << 0)

/** Match special frame type.
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAME_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAME_TYPE_SPECIAL                 (2 << 0)

/** Mask for frame type scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_FRAME_TYPE_XXX.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAME_TYPE_MASK                    (3 << 0)

/* ----- VLAN1 Tagged ----- */

/** Match frame with no first inner vlan tag.
 *  Use with ''FM_ACL_SCENARIO_FLAG_VLAN1_TAGGED_MASK''.
 *  
  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_NON_VLAN1_TAGGED                   (0 << 2)

/** Match frame with first inner vlan tag.
 *  Use with ''FM_ACL_SCENARIO_FLAG_VLAN1_TAGGED_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_VLAN1_TAGGED                       (1 << 2)

/** Mask for first inner vlan tag scenarios. 
 *  Use with FM_ACL_SCENARIO_FLAG_(XXX_)VLAN1_TAGGED.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_VLAN1_TAGGED_MASK                  (1 << 2)

/* ----- VLAN2 Tagged ----- */

/** Match frame with no second inner vlan tag. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_VLAN2_TAGGED_MASK''.
 *  
 *  \chips  FM6000, FM10000*/
#define FM_ACL_SCENARIO_FLAG_NON_VLAN2_TAGGED                   (0 << 3)

/** Match frame with second inner vlan tag. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_VLAN2_TAGGED_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_VLAN2_TAGGED                       (1 << 3)

/** Mask for second inner vlan tag scenarios. 
 *  Use with FM_ACL_SCENARIO_FLAG_(XXX_)VLAN2_TAGGED.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_VLAN2_TAGGED_MASK                  (1 << 3)

/* ----- ISL Type ----- */

/** Match frames with no ISL tag. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_ISL_TYPE_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_ISL_TYPE_NONE                      (0 << 4)

/** Match frames with VN-Tag. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_ISL_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ISL_TYPE_VNTAG                     (1 << 4)

/** Match frames with F56 Tag. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_ISL_TYPE_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_ISL_TYPE_F56                       (2 << 4)

/** Match frames with F64 Tag. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_ISL_TYPE_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_ISL_TYPE_F64                       (3 << 4)

/** Mask for ISL type scenarios. 
 *  Use with FM_ACL_SCENARIO_FLAG_ISL_TYPE_XXX.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_ISL_TYPE_MASK                      (3 << 4)

/* ----- Fragmentation Type ----- */

/** Match unfragmented frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAG_TYPE_UNFRAG_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAG_TYPE_UNFRAG                   (1 << 6)

/** Mask for unfragmented flag scenarios. 
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAG_TYPE_UNFRAG''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAG_TYPE_UNFRAG_MASK              (1 << 6)

/** Match unfragmented or header fragmented frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAG_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAG_TYPE_UNFRAG_OR_HEAD           (1 << 6)

/** Match subsequent fragmented frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAG_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAG_TYPE_SUB                      (2 << 6)

/** Match header fragmented frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_FRAG_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAG_TYPE_HEAD                     (3 << 6)

/** Mask for fragmentation type scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_FRAG_TYPE_XXX.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_FRAG_TYPE_MASK                     (3 << 6)

/* ----- IP Type ----- */

/** Match for non-IP frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_IP_TYPE_MASK''.
 *
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_IP_TYPE_NONE                       (0 << 8)

/** Match for IPV4 frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_IP_TYPE_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_IP_TYPE_IPV4                       (1 << 8)

/** Match for IPV6 frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_IP_TYPE_MASK''.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_IP_TYPE_IPV6                       (2 << 8)

/** Mask for IP type scenarios. 
 *  Use with FM_ACL_SCENARIO_FLAG_IP_TYPE_XXX.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_SCENARIO_FLAG_IP_TYPE_MASK                       (3 << 8)

/* ----- L3 Multicast ----- */

/** Match for non L3 multicast frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_L3_MULTICAST_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_NON_L3_MULTICAST                   (0 << 10)

/** Match for L3 multicast frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_L3_MULTICAST_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_L3_MULTICAST                       (1 << 10)

/** Mask for L3 Multicast type scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_(XXX_)L3_MULTICAST.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_L3_MULTICAST_MASK                  (1 << 10)

/* ----- Routable port ----- */

/** Match for non routable port.
 *  Use with ''FM_ACL_SCENARIO_FLAG_ROUTABLE_PORT_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_NON_ROUTABLE_PORT                  (0 << 11)

/** Match for routable port.
 *  Use with ''FM_ACL_SCENARIO_FLAG_ROUTABLE_PORT_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ROUTABLE_PORT                      (1 << 11)

/** Mask for routable port scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_(XXX_)ROUTABLE_PORT.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ROUTABLE_PORT_MASK                 (1 << 11)

/* ----- VN Tunnel ----- */

/** Match for non VN Tunnel frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_IPTUN_RX_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_NON_IPTUN_RX                       (0 << 12)

/** Match for VN Tunnel (VxLAN, NVGRE) frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_IPTUN_RX_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_IPTUN_RX                           (1 << 12)

/** Mask for VN Tunnel scenarios.
 *  Use with FM_ACL_SCENARIO_(XXX_)IPTUN_RX.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_IPTUN_RX_MASK                      (1 << 12)

/* ----- Routable DMAC ----- */

/** Match for non routable DMAC.
 *  Use with ''FM_ACL_SCENARIO_FLAG_ROUTABLE_DMAC_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_NON_ROUTABLE_DMAC                  (0 << 13)

/** Match for routable DMAC.
 *  Use with ''FM_ACL_SCENARIO_FLAG_ROUTABLE_DMAC_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ROUTABLE_DMAC                      (1 << 13)

/** Mask for routable port scenarios.
 *  Use with FM_ACL_SCENARIO_(XXX_)ROUTABLE_DMAC.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ROUTABLE_DMAC_MASK                 (1 << 13)

/* ----- Routable VLAN1 ----- */

/** Match for non routable first vlan.
 *  Use with ''FM_ACL_SCENARIO_FLAG_ROUTABLE_VID1_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_NON_ROUTABLE_VID1                  (0 << 14)

/** Match for routable first vlan.
 *  Use with ''FM_ACL_SCENARIO_FLAG_ROUTABLE_VID1_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ROUTABLE_VID1                      (1 << 14)

/** Mask for routable port scenarios.
 *  Use with FM_ACL_SCENARIO_(XXX_)ROUTABLE_VID1.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_ROUTABLE_VID1_MASK                 (1 << 14)

/* ----- TRILL Flags ----- */

/** Match for non Trill frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_TRILL_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_TRILL_NONE                         (0 << 15)

/** Match for any type Trill frames.
 *  Use with ''FM_ACL_SCENARIO_FLAG_TRILL_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_TRILL_ANY                          (1 << 15)

/** Mask for trill scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_TRILL_XXX.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_TRILL_MASK                         (1 << 15)

/** Match for unicast Trill frames
 *  Use with ''FM_ACL_SCENARIO_FLAG_TRILL_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_TRILL_TYPE_UNICAST                      (1 << 15)

/** Match for multicast Trill frames
 *  Use with ''FM_ACL_SCENARIO_FLAG_TRILL_TYPE_MASK''.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_TRILL_TYPE_MULTICAST                    (9 << 12)

/** Mask for trill type scenarios.
 *  Use with FM_ACL_SCENARIO_TRILL_TYPE_XXX.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_FLAG_TRILL_TYPE_MASK                    (9 << 12)

/* ----- MPLS Header ----- */

/** Match for non MPLS encapsulated frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_MPLS_ENCAP_MASK''.
 *  
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_FLAG_NON_MPLS_ENCAP                     (0 << 10)

/** Match for MPLS encapsulated frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_MPLS_ENCAP_MASK''.
 *  
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_FLAG_MPLS_ENCAP                         (1 << 10)

/** Mask for MPLS encapsulated type scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_(XXX_)MPLS_ENCAP.
 *  
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_FLAG_MPLS_ENCAP_MASK                    (1 << 10)

/* ----- Custom Tag Header ----- */

/** Match for non Custom tag parsed frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_CUSTOM_TAG_MASK''.
 *  
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_FLAG_NON_CUSTOM_TAG                     (0 << 11)

/** Match for Custom tag parsed frame.
 *  Use with ''FM_ACL_SCENARIO_FLAG_CUSTOM_TAG_MASK''.
 *  
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_FLAG_CUSTOM_TAG                         (1 << 11)

/** Mask for Custom tag parsed type scenarios.
 *  Use with FM_ACL_SCENARIO_FLAG_(XXX_)CUSTOM_TAG.
 *  
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_FLAG_CUSTOM_TAG_MASK                    (1 << 11)


/** @}(end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 * An ACL value is an argument to ''fmAddACLRule''
 * and ''fmAddACLRuleExt''. It consists of a
 * collection of packet field values that must be
 * matched to satisfy the ACL rule.
 *                                              \lb\lb
 * Except for the TCP/UDP port range members, each
 * value member has a corresponding mask member.
 * The mask selects which bits in the corresponding
 * value member are relevant for matching to a frame
 * (a zero bit in the mask indicates a dont-care bit
 * in the value).
 *                                              \lb\lb
 * Not all members of this structure need contain
 * valid values, only those that are relevant to the
 * ACL rule's condition (see 'fm_aclCondition').
 **************************************************/
typedef struct _fm_aclValue
{
    /** Source MAC address for the ''FM_ACL_MATCH_SRC_MAC'' condition. */
    fm_macaddr src;

    /** Source MAC address mask for the ''FM_ACL_MATCH_SRC_MAC'' condition. */
    fm_macaddr srcMask;

    /** Destination MAC address for the ''FM_ACL_MATCH_DST_MAC'' condition. */
    fm_macaddr dst;

    /** Destination MAC address mask for the ''FM_ACL_MATCH_DST_MAC'' condition. */
    fm_macaddr dstMask;

    /** Ethernet type for the ''FM_ACL_MATCH_ETHERTYPE'' condition. */
    fm_uint16  ethType;

    /** Ethernet type mask for the ''FM_ACL_MATCH_ETHERTYPE'' condition. */
    fm_uint16  ethTypeMask;

    /** VLAN ID for the ''FM_ACL_MATCH_VLAN'' condition. */
    fm_uint16  vlanId;

    /** VLAN ID mask for the ''FM_ACL_MATCH_VLAN'' condition. */
    fm_uint16  vlanIdMask;

    /** VLAN priority for the ''FM_ACL_MATCH_PRIORITY'' condition.
     *                                                                  \lb\lb
     *  The VLAN priority is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte    vlanPri;

    /** VLAN priority mask for the ''FM_ACL_MATCH_PRIORITY'' condition.
     *                                                                  \lb\lb
     *  The VLAN priority mask is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte    vlanPriMask;

    /** VLAN2 ID for the ''FM_ACL_MATCH_VLAN2'' condition. */
    fm_uint16  vlanId2;

    /** VLAN2 ID mask for the ''FM_ACL_MATCH_VLAN2'' condition. */
    fm_uint16  vlanId2Mask;

    /** VLAN2 priority for the ''FM_ACL_MATCH_PRIORITY2'' condition.
     *                                                                  \lb\lb
     *  The VLAN2 priority is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte    vlanPri2;

    /** VLAN2 priority mask for the ''FM_ACL_MATCH_PRIORITY2'' condition.
     *                                                                  \lb\lb
     *  The VLAN2 priority mask is a 4 bit field which includes the CFI bit
     *  as the low order bit at position 0. */
    fm_byte    vlanPri2Mask;

    /** Source IP address for the ''FM_ACL_MATCH_SRC_IP'' condition. */
    fm_ipAddr  srcIp;

    /** Source IP address mask for the ''FM_ACL_MATCH_SRC_IP'' condition. */
    fm_ipAddr  srcIpMask;

    /** Destination IP address for the ''FM_ACL_MATCH_DST_IP'' condition. */
    fm_ipAddr  dstIp;

    /** Destination IP address mask for the ''FM_ACL_MATCH_DST_IP'' condition. */
    fm_ipAddr  dstIpMask;

    /** IP Time To Live (IPv4) or Hop Limit (IPv6) for the ''FM_ACL_MATCH_TTL'' 
     *  condition. */
    fm_byte    ttl;

    /** IP Time To Live (IPv4) or Hop Limit (IPv6) mask for the 
     *  ''FM_ACL_MATCH_TTL'' condition. */
    fm_byte    ttlMask;

    /** L4 Protocol (IPv4) or Next Header (IPv6) for the 
     *  ''FM_ACL_MATCH_PROTOCOL'' condition. */
    fm_byte    protocol;

    /** L4 Protocol (IPv4) or Next Header (IPv6) mask for the 
     *  ''FM_ACL_MATCH_PROTOCOL'' condition. */
    fm_byte    protocolMask;

    /** Fragmentation flags for the ''FM_ACL_MATCH_FLAGS'' condition (see 
     *  ''ACL Fragmentation Flag Masks''). */
    fm_byte    flags;

    /** Fragmentation flags mask for the ''FM_ACL_MATCH_FLAGS'' condition (see 
     *  ''ACL Fragmentation Flag Masks''). */
    fm_byte    flagsMask;

    /** Differentiated services code point - the six most significant bits
     *  of the Type of Service octet (IPv4) or Traffic Class octet (IPv6) for 
     *  the ''FM_ACL_MATCH_DSCP'' condition. */
    fm_byte    dscp;

    /** Mask for differentiated services code point for the ''FM_ACL_MATCH_DSCP'' 
     *  condition. */
    fm_byte    dscpMask;

    /** IPv6 Flow Label for the ''FM_ACL_MATCH_FLOW'' condition. */
    fm_uint32  flow;

    /** IPv6 Flow Label mask for the ''FM_ACL_MATCH_FLOW'' condition. */
    fm_uint32  flowMask;

    /** TCP or UDP source port for the ''FM_ACL_MATCH_L4_SRC_PORT'' condition.
     *                                                                  \lb\lb
     *  The first TCP or UDP source port in a range of port numbers for the 
     *  ''FM_ACL_MATCH_L4_SRC_PORT_RANGE'' condition (FM3000 and FM4000 only).
     *                                                                  \lb\lb
     *  A base TCP or UDP source port value that is compared to the value in
     *  the packet after it has been masked with L4SrcMask for the 
     *  ''FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK'' condition.
     *                                                                  \lb\lb
     *  The first 16 bits of L4 payload (following the IP header) for the
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION'' and 
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT'' conditions. */
    fm_uint16  L4SrcStart;

    /** The last TCP or UDP source port in a range of port numbers for the 
     *  ''FM_ACL_MATCH_L4_SRC_PORT_RANGE'' condition (FM3000 and FM4000 
     *  only). */
    fm_uint16  L4SrcEnd;

    /** TCP or UDP source port mask for the 
     *  ''FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK'', 
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION'' and  
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT'' conditions. The field data in
     *  the packet will be ANDed with this mask prior to being compared to
     *  the L4SrcStart value. In the case of the L4 deep inspection conditions,
     *  the field data in the packet is the first 16 bits of L4 payload
     *  (following the IP header). */
    fm_uint16  L4SrcMask;

    /** TCP or UDP destination port for the ''FM_ACL_MATCH_L4_DST_PORT'' 
     *  condition.
     *                                                                  \lb\lb
     *  The first TCP or UDP destination port in a range of port numbers for 
     *  the ''FM_ACL_MATCH_L4_DST_PORT_RANGE'' condition (FM3000 and FM4000 only).
     *                                                                  \lb\lb
     *  A base TCP or UDP destination port value that is compared to the value 
     *  in the packet after it has been masked with L4DstMask for the 
     *  ''FM_ACL_MATCH_L4_DST_PORT_WITH_MASK'' condition.
     *                                                                  \lb\lb
     *  The second 16 bits of L4 payload (following the IP header) for the
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION'' and 
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT'' conditions. */
    fm_uint16  L4DstStart;

    /** The last TCP or UDP destination port in a range of port numbers for the 
     *  ''FM_ACL_MATCH_L4_DST_PORT_RANGE'' condition (FM3000 and FM4000 
     *  only). */
    fm_uint16  L4DstEnd;

    /** TCP or UDP destination port mask for the 
     *  ''FM_ACL_MATCH_L4_DST_PORT_WITH_MASK'', 
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION'' and  
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT'' conditions. The field data in
     *  the packet will be ANDed with this mask prior to being compared to
     *  the L4DstStart value. In the case of the L4 deep inspection conditions,
     *  the field data in the packet is the second 16 bits of L4 payload
     *  (following the IP header). */
    fm_uint16  L4DstMask;

    /** A contiguous 64-bit wide field extracted from the L4 area of the 
     *  frame for the ''FM_ACL_MATCH_L4_DEEP_INSPECTION'' condition. The 
     *  offset at which the bit field starts is set on a per-port basis and is 
     *  not specified as part of the ACL.
     *                                                                  \lb\lb
     *  This 64-bit value is a concatenation of the four 16-bit values L4A, 
     *  L4B, L4C, and L4D, with L4A being the most-significant bits and L4D 
     *  being the least-significant bits. 
     *                                                                  \lb\lb
     *  Note that when using ''FM_ACL_MATCH_L4_DEEP_INSPECTION'', the first
     *  32 bits of L4 payload following the IP header will appear in the
     *  L4 source port and destination port fields. */
    fm_uint64  L4DeepInspection;

    /** L4 deep inspection bit field mask for the 
     *  ''FM_ACL_MATCH_L4_DEEP_INSPECTION'' condition. */
    fm_uint64  L4DeepInspectionMask;

    /** Switch priority for the ''FM_ACL_MATCH_SWITCH_PRIORITY'' condition. */
    fm_byte    switchPri;

    /** Switch priority mask for the ''FM_ACL_MATCH_SWITCH_PRIORITY'' condition. */
    fm_byte    switchPriMask;

    /** TCP flags for the ''FM_ACL_MATCH_TCP_FLAGS'' condition. */
    fm_byte    tcpFlags;

    /** TCP flags mask for the ''FM_ACL_MATCH_TCP_FLAGS'' condition. */
    fm_byte    tcpFlagsMask;

    /** L4 entry data for the ''FM_ACL_MATCH_L4_ENTRY'' condition. */
    fm_uint16  L4Entry;

    /** L4 entry mask for the ''FM_ACL_MATCH_L4_ENTRY'' condition. */
    fm_uint16  L4EntryMask;

    /** An arbitrary contiguous field taken from the L3 portion of the
     *  frame for the ''FM_ACL_MATCH_NON_IP_PAYLOAD'' condition. The maximum 
     *  size of this field is 50 bytes, but the actual matched portion is 
     *  determined by the ''FM_PORT_PARSER_NOT_IP_PAYLOAD'' port attribute.  
     *  The bytes are interpreted in network order. */
    fm_byte    nonIPPayload[FM_MAX_ACL_NON_IP_PAYLOAD_BYTES];

    /** Up to 52-bytes may be matched for non-IP frames for the 
     *  ''FM_ACL_MATCH_NON_IP_PAYLOAD'' condition. */
    fm_byte    nonIPPayloadMask[FM_MAX_ACL_NON_IP_PAYLOAD_BYTES];

    /** Mask specifying the ingress logical ports to which this rule applies
     *  for the ''FM_ACL_MATCH_INGRESS_PORT_MASK'' condition.  Note that this
     *  mask will override the ACL's port association set with
     *  ''fmAddACLPort'' or ''fmAddACLPortExt'' for ingress ACLs. */
    fm_uint32  ingressPortMask;

    /** Used internally by the ACL compiler. */
    fm_byte    srcIpMapVal;

    /** Used internally by the ACL compiler. */
    fm_byte    dstIpMapVal;

    /** The port set number to which an ACL rule should apply for the 
     *  ''FM_ACL_MATCH_INGRESS_PORT_SET'' condition. The value can be a port 
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
     *  ingress port mask as set with the ''FM_ACL_MATCH_INGRESS_PORT_MASK''
     *  condition. */
    fm_int32   portSet;

    /** The logical port number to which an ACL rule should apply for the
     *  ''FM_ACL_MATCH_SRC_PORT'' condition. */
    fm_int     logicalPort;

    /** The VLAN tag type for the ''FM_ACL_MATCH_VLAN_TAG_TYPE'' condition. */
    fm_aclVlanTagType vlanTag;

    /** Type of Service octet (IPv4) or Traffic Class octet (IPv6) for the 
     *  ''FM_ACL_MATCH_TOS'' condition. */
    fm_byte   tos;

    /** Mask for Type of Service octet (IPv4) or Traffic Class octet (IPv6) for 
     *  the ''FM_ACL_MATCH_TOS'' condition. */
    fm_byte   tosMask;

    /** Mapped source port value for the ''FM_ACL_MATCH_SOURCE_MAP'' condition. */
    fm_byte mappedSourcePort;

    /** Mapped source port mask for the ''FM_ACL_MATCH_SOURCE_MAP'' condition. */
    fm_byte mappedSourcePortMask;

    /** Mapped protocol value for the ''FM_ACL_MATCH_PROTOCOL_MAP'' condition. */
    fm_byte mappedProtocol;

    /** Mapped protocol mask for the ''FM_ACL_MATCH_PROTOCOL_MAP'' condition. */
    fm_byte mappedProtocolMask;

    /** Mapped L4 source port value for the ''FM_ACL_MATCH_L4_SRC_PORT_MAP'' 
     *  condition. */
    fm_uint16 mappedL4SrcPort;

    /** Mapped L4 source port mask for the ''FM_ACL_MATCH_L4_SRC_PORT_MAP'' 
     *  condition. */
    fm_uint16 mappedL4SrcPortMask;

    /** Mapped L4 destination port value for the ''FM_ACL_MATCH_L4_DST_PORT_MAP'' 
     *  condition. */
    fm_uint16 mappedL4DstPort;

    /** Mapped L4 destination port mask for the ''FM_ACL_MATCH_L4_DST_PORT_MAP'' 
     *  condition. */
    fm_uint16 mappedL4DstPortMask;

    /** Mapped destination MAC value for the ''FM_ACL_MATCH_DST_MAC_MAP'' 
     *  condition. */
    fm_byte mappedDstMac;

    /** Mapped destination MAC mask for the ''FM_ACL_MATCH_DST_MAC_MAP'' 
     *  condition. */
    fm_byte mappedDstMacMask;

    /** Mapped source MAC value for the ''FM_ACL_MATCH_SRC_MAC_MAP'' condition. */
    fm_byte mappedSrcMac;

    /** Mapped source MAC mask for the ''FM_ACL_MATCH_SRC_MAC_MAP'' condition. */
    fm_byte mappedSrcMacMask;

    /** Mapped Ethernet type value for the ''FM_ACL_MATCH_ETH_TYPE_MAP'' 
     *  condition. */
    fm_byte mappedEthType;

    /** Mapped Ethernet type mask for the ''FM_ACL_MATCH_ETH_TYPE_MAP'' 
     *  condition. */
    fm_byte mappedEthTypeMask;

    /** Mapped IP length value for the ''FM_ACL_MATCH_IP_LENGTH_MAP'' condition. */
    fm_byte mappedIpLength;

    /** Mapped IP length mask for the ''FM_ACL_MATCH_IP_LENGTH_MAP'' condition. */
    fm_byte mappedIpLengthMask;

    /** Mapped destination IP address value for the ''FM_ACL_MATCH_DST_IP_MAP'' 
     *  condition. */
    fm_byte mappedDstIp;

    /** Mapped destination IP address mask for the ''FM_ACL_MATCH_DST_IP_MAP'' 
     *  condition. */
    fm_byte mappedDstIpMask;

    /** Mapped source IP address value for the ''FM_ACL_MATCH_SRC_IP_MAP'' 
     *  condition. */
    fm_byte mappedSrcIp;

    /** Mapped source IP address mask for the ''FM_ACL_MATCH_SRC_IP_MAP'' 
     *  condition. */
    fm_byte mappedSrcIpMask;

    /** Mapped VLAN ID value for the ''FM_ACL_MATCH_VLAN_MAP'' condition. */
    fm_uint16  mappedVlanId;

    /** Mapped VLAN ID mask for the ''FM_ACL_MATCH_VLAN_MAP'' condition. */
    fm_uint16  mappedVlanIdMask; 

    /** A set of bytes, extracted from the L4 area of the frame.
     *  The first two 16-bit words of L4 payload (following the IP 
     *  header) will always be matched against the values that appear in the 
     *  L4 source port and destination port members of this structure, even 
     *  if the frame does not have a recognized L4 protocol. This member is 
     *  used to match bytes following the first two 16-bit words of L4 payload.  
     *  Which bytes from the frame are compared to this value may be different 
     *  depending on a number of factors. See ''Deep Inspection'' in the
     *  API User Guide. */
    fm_byte L4DeepInspectionExt[FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES];

    /** L4 deep inspection extended bit field mask. */
    fm_byte L4DeepInspectionExtMask[FM_MAX_ACL_L4_DEEP_INSPECTION_BYTES];

    /** IP length for the ''FM_ACL_MATCH_IP_LENGTH'' condition. */
    fm_uint16 ipLength;

    /** IP length mask for the ''FM_ACL_MATCH_IP_LENGTH'' condition. */
    fm_uint16 ipLengthMask;

    /** ISL Frame Type for the ''FM_ACL_MATCH_ISL_FTYPE'' condition. */
    fm_byte fType;

    /** ISL User for the ''FM_ACL_MATCH_ISL_USER'' condition. */
    fm_byte islUser;

    /** ISL User Mask for the ''FM_ACL_MATCH_ISL_USER'' condition. */
    fm_byte islUserMask;

    /** Source GLORT value for the ''FM_ACL_MATCH_SRC_GLORT'' condition.
     *                                                                  \lb\lb
     *  Note that a GloRT associated with a particular logical port may be
     *  retrieved with the function ''fmGetStackGlort''. */
    fm_uint16 srcGlort;

    /** Source GLORT mask for the ''FM_ACL_MATCH_SRC_GLORT'' condition. */
    fm_uint16 srcGlortMask;

    /** Destination GLORT value for the ''FM_ACL_MATCH_DST_GLORT'' condition.
     *                                                                  \lb\lb
     *  Note that a GloRT associated with a particular logical port may be
     *  retrieved with the function ''fmGetStackGlort''. */
    fm_uint16 dstGlort;

    /** Destination GLORT mask for the ''FM_ACL_MATCH_DST_GLORT'' condition. */
    fm_uint16 dstGlortMask;

    /** Fragmentation Frame Type for the ''FM_ACL_MATCH_FRAG'' condition. */
    fm_aclFragType fragType;

    /** Frame Type for the ''FM_ACL_MATCH_FRAME_TYPE'' condition. */
    fm_aclFrameType frameType;

    /** TRILL Source MAC address for the ''FM_ACL_MATCH_TRILL_SRC_MAC''
     *  condition. */
    fm_macaddr trillSrc;

    /** TRILL Source MAC address mask for the ''FM_ACL_MATCH_TRILL_SRC_MAC''
     *  condition. */
    fm_macaddr trillSrcMask;

    /** TRILL Source RBridge Nickname for the ''FM_ACL_MATCH_TRILL_SRB''
     *  condition. */
    fm_uint16 trillSRB;

    /** TRILL Source RBridge Nickname mask for the ''FM_ACL_MATCH_TRILL_SRB''
     *  condition. */
    fm_uint16 trillSRBMask;

    /** TRILL Remote RBridge Nickname for the ''FM_ACL_MATCH_TRILL_RRB''
     *  condition. */
    fm_uint16 trillRRB;

    /** TRILL Remote RBridge Nickname mask for the ''FM_ACL_MATCH_TRILL_RRB''
     *  condition. */
    fm_uint16 trillRRBMask;

    /** TRILL Type for the ''FM_ACL_MATCH_TRILL_TYPE'' condition. */
    fm_aclTrillType trillType;

    /** Multi-Stage ACL Value for the ''FM_ACL_MATCH_TABLE1_CONDITION''
     *  condition. */
    fm_uint16 table1Condition;

    /** Multi-Stage ACL mask for the ''FM_ACL_MATCH_TABLE1_CONDITION''
     *  condition. For a BST ACL, this mask must include the following bits:
     *  0x7000 (due to API internally setting one additional control bit and
     *  a 4-bit granularity requirement for BST matching masks). */
    fm_uint16 table1ConditionMask;

    /** Mapped VLAN ID value for the FM_ACL_MATCH_VLAN2_MAP condition.
     *  For internal use only. */
    fm_uint16  mappedVlanId2;

    /** Mapped VLAN ID mask for the FM_ACL_MATCH_VLAN2_MAP condition.
     *  For internal use only. */
    fm_uint16  mappedVlanId2Mask; 

    /** Tunnel Network Identifier ACL Value for the ''FM_ACL_MATCH_TUNNEL_ID''
     *  condition. */
    fm_uint32 tunnelId;

    /** Tunnel Network Identifier ACL mask for the ''FM_ACL_MATCH_TUNNEL_ID''
     *  condition. */
    fm_uint32 tunnelIdMask;

    /** Scenario flags for the ''FM_ACL_MATCH_SCENARIO_FLAGS'' condition
     *  (see ''ACL Scenario Flags''). */
    fm_uint16  scenarioFlags;

    /** Scenario flags mask for the ''FM_ACL_MATCH_SCENARIO_FLAGS''
     *  condition (see ''ACL Scenario Flags''). */
    fm_uint16  scenarioFlagsMask;

} fm_aclValue;



/**************************************************/
/** \ingroup typeStruct
 * ACL Counters
 * Used as an argument to ''fmGetACLCountExt''.
 **************************************************/
typedef struct _fm_aclCounters
{
    /** Number of packets counted by the policer. */
    fm_uint64 cntPkts;

    /** Number of bytes counted by the policer. */
    fm_uint64 cntOctets;

} fm_aclCounters;


/**************************************************/
/** \ingroup typeStruct
 * ACL compiler statistics
 * Used as an argument to ''fmCompileACLExt''.
 **************************************************/
typedef struct _fm_aclCompilerStats
{
    /** Number of minslices used for ingress ACLs. */
    fm_uint     minSlicesIngress;

    /** Number of minslices used for egress ACLs. */
    fm_uint     minSlicesEgress;

    /** Number of minslices used for BST ACLs. */
    fm_uint     minSlicesBst;

    /** Number of minslices used for ingress and egress ACLs combined. This
     *  value can not equal minSlicesIngress + minSlicesEgress if an ingress
     *  ACL action share a minslice with an egress ACL condition. */
    fm_uint     minSlicesTotal;

    /** Number of policer/counter banck consume by the ACLs. */
    fm_uint     policerBanksUsed;

    /** Maximum number of CAM lines used by an ACL. */
    fm_uint     mostRulesUsed;

    /** Number of the ACL that used the greatest number of CAM lines. */
    fm_int      aclWithMostRules;

    /** Number of chunks used for egress ACLs. */
    fm_uint     chunksUsed;

    /** Number of ACLs skipped because they have no port association. */
    fm_uint     aclsSkipped;

    /** Number of rules skipped because they have no port association.
     *  Does not include rules in ACLs that were skipped entirely. */
    fm_uint     rulesSkipped;

    /** Number of L4 source port mapper slots consume by the ACLs. */
    fm_uint     l4SrcMapperSlots;

    /** Number of L4 destination port mapper slots consume by the ACLs. */
    fm_uint     l4DstMapperSlots;

    /** Number of IP mapper slots consume by the ACLs. */
    fm_uint     ipMapperSlots;

    /** Number of cascaded minslices group configured. */
    fm_uint     slicesUsed;
    
    /** First minslice used by the combined ingress and egress ACLs. */
    fm_uint     firstMinSliceUsed;

    /** Last minslice used by the combined ingress and egress ACLs. */
    fm_uint     lastMinSliceUsed;

} fm_aclCompilerStats;


/**************************************************/
/** \ingroup typeStruct
 * Data used with the ''FM_ACL_COMPILE_FLAG_TRY_ALLOC''
 * ACL compiler flag.
 * Used as an argument to ''fmCompileACLExt''.
 **************************************************/
typedef struct _fm_aclCompilerTryAlloc
{
    /** Number of the first ACL slice in the range to validate. */
    fm_int               aclFirstSlice;

    /** Number of the last ACL slice in the range to validate. */
    fm_int               aclLastSlice;

    /** ACL compiler statistics. Will be returned if the
     *  ''FM_ACL_COMPILE_FLAG_RETURN_STATS'' flag is set. */
    fm_aclCompilerStats  compilerStats;

} fm_aclCompilerTryAlloc;



/**************************************************/
/** \ingroup typeStruct
 * ACL rule policer information.
 * Used as an argument to ''fmGetACLRuleAttribute''.
 **************************************************/
typedef struct _fm_aclRulePolicerInfo
{
    /** Type of each policer bank associated with the ACL rule. */
    fm_aclBankType type[FM_MAX_ACL_RULE_POLICER_BANKS];

    /** Index of each policer bank associated with the ACL rule. */
    fm_uint32 index[FM_MAX_ACL_RULE_POLICER_BANKS];

} fm_aclRulePolicerInfo;



/*****************************************************************************/
/** \ingroup typeStruct
 * This structure provides information about FFU TCAM/BST slice usage for an
 * acl. It is used as the value argument for the ACL attribute
 * ''FM_ACL_SLICE_USAGE''. Some members of this structure are valid only for
 * certain types of acl.
 *****************************************************************************/
typedef struct _fm_aclSliceUsage
{
    /** The first slice of the block of TCAM/BST slices used to store
     *  condition keys for the acl. */
    fm_int  firstCondSlice;

    /** The last slice of the block of TCAM/BST slices used to store
     *  condition keys for the acl, and the first slice containing the
     *  acl actions. */
    fm_int  lastCondSlice;

    /** The last slice of the block of TCAM/BST slices used to store actions
     *  for the acl. Valid only for ingress acls. For TCAM acls, the action
     *  slices used are lastCondSlice to lastActionSlice. For BST acls, the
     *  action slices used are firstCondSlice to lastActionSlice. */
    fm_int  lastActionSlice;

    /** First chunk index of the cascade of chunks in which the egress
     *  acl is located. Valid only for egress acls. For egress acls, TCAM
     *  slices are divided into chunks, and multiple chunks are cascaded
     *  if the acl contains more than 32 rules. */
    fm_int  egressChunk;

    /** Number of chunks allocated for the egress acl. Valid only for
     *  egress acls. */
    fm_int  egressNumChunk;

} fm_aclSliceUsage;



/*****************************************************************************/
/** \ingroup typeStruct
 * This structure provides information about FFU TCAM/BST slice and row usage
 * for an acl rule. It is used as the value argument for the ACL rule attribute
 * ''FM_ACL_RULE_SLICE_USAGE''. Some members of this structure are valid only
 * for certain types of acl.
 *****************************************************************************/
typedef struct _fm_aclRuleSliceUsage
{
    /** The first slice of the block of TCAM/BST slices in which condition
     *  keys of the acl rule are located. */
    fm_int  firstCondSlice;

    /** The last slice of the block of TCAM/BST slices in which condition
     *  keys of acl rule are located, and the first slice containing the
     *  actions of acl rule. */
    fm_int  lastCondSlice;

    /** The last slice of the block of TCAM/BST slices in which actions of
     *  the acl rule are stored. Valid only for ingress acls. For acl rules
     *  in TCAM, the action slices used are lastCondSlice to lastActionSlice.
     *  For acl rules in BST, the action slices used are firstCondSlice to
     *  lastActionSlice. */
    fm_int  lastActionSlice;

    /** The row in the TCAM/BST slices that is allocated for the acl rule. */
    fm_int  rowPos;

    /** Partition index of the BST slice in which the acl rule is located.
     *  The BST slice is partitioned into multiple parts, and this field
     *  identifies the index at which the acl rule is located. It is valid
     *  only for acls allocated in BST. */
    fm_int  bstPartitionIndex;

} fm_aclRuleSliceUsage;




/**************************************************/
/** TCP Flags
 *  \ingroup constAclTcpFlag
 *  \page aclTcp
 *
 *  The following set of bit masks may be ORed
 *  together to produce a value for the
 *  tcpFlags member of an ''fm_aclValue''
 *  structure.
 **************************************************/
/** \ingroup constAclTcpFlag
 * @{ */

/** TCP FIN (finished) flag. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_TCP_FLAG_FIN                   (1 << 0)

/** TCP SYN (synchronize) flag. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_TCP_FLAG_SYN                   (1 << 1)

/** TCP RST (reset) flag. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_TCP_FLAG_RST                   (1 << 2)

/** TCP PSH (push) flag. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_TCP_FLAG_PSH                   (1 << 3)

/** TCP ACK (acknowledge) flag. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_TCP_FLAG_ACK                   (1 << 4)

/** TCP URG (urgent) flag. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_TCP_FLAG_URG                   (1 << 5)

/** @} (end of Doxygen group) */


/**************************************************/
/** ACL Scenario Masks
 *  \ingroup constAclScenarioMasks
 *  \page aclScenario
 *
 *  The following set of bit masks may be ORed
 *  together to produce a value for the "scenarios"
 *  argument to ''fmCreateACLExt''.
 *
 *  A valid mask must have at least one of the
 *  frame type bits set (NONIP, IPv4, IPv6) and
 *  at least one of the routing type bits set
 *  (SWITCHED, UNICAST_ROUTED, MULTICAST_ROUTED,
 *   SWITCHED_GLORT, UCAST_ROUTED_GLORT,
 *   MCAST_ROUTED_GLORT).
 **************************************************/
/** \ingroup constAclScenarioMasks
 * @{ */

/**********************************
 *  Frame Types
 **********************************/

/** This ACL applies to frames that are neither IPv4 nor IPv6. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_NONIP                  (1 << 0)

/** This ACL applies to IPv4 frames.
 *  (Ethertype is 0x0800, and Version field is 4.)
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_IPv4                   (1 << 1)

/** This ACL applies to IPv6 frames.
 *  (Ethertype is 0x86dd, and Version field is 6.)
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_IPv6                   (1 << 2)

/** This ACL applies to all frames, whether they are IP or not. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_ANY_FRAME_TYPE         (7 << 0)

/**********************************
 *  Routing Types
 **********************************/

/** This ACL applies to frames that are not being routed. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_SWITCHED               (1 << 4)

/** This ACL applies to unicast frames that are being routed. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_UNICAST_ROUTED         (1 << 5)

/** This ACL applies to multicast frames that are being routed. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_MULTICAST_ROUTED       (1 << 6)

/** This ACL applies to all frames, whether they are switched or routed. 
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_ANY_ROUTING_TYPE       (7 << 4)

/**********************************
 *  GLORT Types
 **********************************/

/** This ACL applies to frames that are glort switched.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_SWITCHED_GLORT         (1 << 7)

/** This ACL applies to unicast frames that are being glort routed.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_UCAST_ROUTED_GLORT     (1 << 8)

/** This ACL applies to multicast frames that are being glort routed.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_MCAST_ROUTED_GLORT     (1 << 9)

/** This ACL applies to all frames, whether they are glort switched or routed.
 *
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_SCENARIO_ANY_ROUTING_GLORT_TYPE (7 << 7)

/**********************************
 *  Tunnel Types
 **********************************/

/** This ACL applies to TRILL Unicast frames.
 *
 *  \chips FM6000 */
#define FM_ACL_SCENARIO_TRILL_UCAST            (1 << 10)

/** This ACL applies to TRILL Multicast frames.
 *
 *  \chips FM6000 */
#define FM_ACL_SCENARIO_TRILL_MCAST            (1 << 11)

/** This ACL applies to VN Tunnel (VxLAN, NVGRE) frames.
 *
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_VN_TUNNEL              (1 << 12)

/** This ACL applies to all tunnelled frames (TRILL, VxLAN, NVGRE).
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_ANY_TUNNEL_TYPE        (7 << 10)

/**********************************
 *  Tag Types
 **********************************/

/** This ACL applies to frames that are VN-Tagged.
 *  
 *  \chips  FM6000 */
#define FM_ACL_SCENARIO_VNTAG                   (1 << 13)

/**********************************
 *  Special GLORT Types
 **********************************/

/** This ACL applies to frames with special FTYPE.
 *
 *  \chips  FM10000 */
#define FM_ACL_SCENARIO_SPECIAL_GLORT           (1 << 14)


/** @} (end of Doxygen group) */


/**************************************************/
/** ACL Apply Flags
 *  \ingroup constAclApplyFlags
 *  \page aclApplyFlags
 *
 *  The following set of bit masks may be ORed
 *  together to produce a value for the "flags"
 *  argument to ''fmApplyACL''.
 **************************************************/
/** \ingroup constAclApplyFlags
 * @{ */

/** The ACL configuration will be applied to the hardware without disrupting
 *  traffic. 
 *                                                                      \lb\lb
 *  While not strictly required, it is highly recommended that ''fmCompileACL'' 
 *  first be called with the ''FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE'' flag to 
 *  validate the new ACL configuration before applying it to the hardware, 
 *  because if ''fmApplyACL'' is unable to update the hardware successfully,
 *  the hardware will be left in a corrupted state.
 *                                                                      \lb\lb
 *  Note that when this flag is used, hardware resources may be used less
 *  efficiently than if the ACL configuration were compiled and applied
 *  in the normal traffic-disrupting fashion.
 *                                                                      \lb\lb
 *  Note also that the ACL attribute ''FM_ACL_MODE'' should be set to its
 *  default value of FM_ACL_MODE_STANDARD for all ACLs when using this flag.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_APPLY_FLAG_NON_DISRUPTIVE            (1 << 0)

/** @} (end of Doxygen group) */

/* Placeholders for unimplemented flags - don't include in API document: */

/* This flag determines if the compiler should return the statistic in a
 * structure format. If this flag is set, the data type of value must be
 * of type ''fm_aclCompilerStats''. */
#define FM_ACL_APPLY_FLAG_RETURN_STATS              (1 << 1)

/* The compiler will only apply the internal ACL Id specified by the value.
 * The value must be of type ''fm_int''. This flag only works in parallel with
 * ''FM_ACL_APPLY_FLAG_NON_DISRUPTIVE''. */
#define FM_ACL_APPLY_FLAG_INTERNAL                  (1 << 2)


/**************************************************/
/** ACL Compiler Flags
 *  \ingroup constAclCompilerFlags
 *  \page aclCompilerFlags
 *
 *  The following set of bit masks may be ORed
 *  together to produce a value for the "flags"
 *  argument to ''fmCompileACL''.
 **************************************************/
/** \ingroup constAclCompilerFlags
 * @{ */

/** This flag determines what happens when multiple ACLs are applied
 *  to a port.  If this flag is specified, then one rule will hit in
 *  each ACL applied to the port, and the action from each of these
 *  hits will be executed.  (Conflicting actions will be resolved
 *  according to the "precedence" specified to ''fmCreateACLExt''.)
 *  If this flag is not specified, then the behavior will be similar
 *  to that of FM2000 ACLs: all ACLs applied to a port will effectively
 *  be concatenated, and only a single rule amongst all the ACLs will hit.
 *  In non-parallel mode, a deny all rule is implicitly added at the end
 *  of the concatenated list.  In parallel mode, there is no implicit deny. 
 *                                                                      \lb\lb
 *  On FM6000 devices, ACLs are always compiled as if this flag were enabled,
 *  whether or not it is actually specified in the call to ''fmCompileACL''.
 *  
 *  \chips  FM3000, FM4000 */
#define FM_ACL_COMPILE_FLAG_PARALLEL                (1 << 0)

/** L4 port mapper disabled mode.
 *                                                                      \lb\lb
 *  This flag is one of three that specify the extent to which the ACL
 *  compiler should use the L4 port mapper to optimize L4 port ranges.
 *  Only one of the L4 mapper flags should be specified at a time.
 *                                                                      \lb\lb
 *  In DISABLED mode, the compiler does not use the L4 port mapper at all.
 *  L4 port ranges will be expanded using multiple mask-and-value keys.
 *  This is the legacy behavior. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_COMPILE_FLAG_L4_MAPPER_DISABLED      (0 << 1)

/** L4 port mapper limited mode.
 *                                                                      \lb\lb
 *  This flag is one of three that specify the extent to which the ACL
 *  compiler should use the L4 port mapper to optimize L4 port ranges.
 *  Only one of the L4 mapper flags should be specified at a time.
 *                                                                      \lb\lb
 *  In LIMITED mode, the compiler only uses the L4 port mapper for ACLs
 *  that exceed the limit of 512 cache lines when the L4 port ranges
 *  are expanded. ACLs that do not exceed this limit will be expanded
 *  using multiple mask-and-value keys. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_COMPILE_FLAG_L4_MAPPER_LIMITED       (1 << 1)

/** L4 port mapper normal mode.
 *                                                                      \lb\lb
 *  This flag is one of three that specify the extent to which the ACL
 *  compiler should use the L4 port mapper to optimize L4 port ranges.
 *  Only one of the L4 mapper flags should be specified at a time.
 *                                                                      \lb\lb
 *  In NORMAL mode, all ACLs that specify L4 port ranges will be
 *  considered for optimization. The compiler chooses a combination of
 *  mapped and unmapped keys that makes optimal use of the FFU
 *  resources. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_COMPILE_FLAG_L4_MAPPER_NORMAL        (1 << 2)

/** IP address mapper disabled mode.
 *                                                                      \lb\lb
 *  This flag is one of two that specify the extent to which the ACL
 *  compiler should use the IP mapper to optimize IP addresses. Only
 *  one of the IP mapper flags should be specified at a time.
 *                                                                      \lb\lb
 *  In DISABLED mode, the compiler does not use the IP mapper at all.
 *  This is the legacy behavior. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_COMPILE_FLAG_IP_MAPPER_DISABLED      (0 << 3)

/** IP address mapper limited mode.
 *                                                                      \lb\lb
 *  This flag is one of two that specify the extent to which the ACL
 *  compiler should use the IP mapper to optimize IP addresses. Only
 *  one of the IP mapper flags should be specified at a time.
 *                                                                      \lb\lb
 *  In LIMITED mode, the compiler uses the IP mapper to match individual
 *  IPv6 addresses, replacing all the SIP or DIP address keys in an ACL
 *  with the corresponding mapper keys. This mode is effective when the
 *  ACLs specify a small number of orthogonal IPv6 addresses. 
 *
 *  \chips  FM3000, FM4000 */
#define FM_ACL_COMPILE_FLAG_IP_MAPPER_LIMITED       (1 << 3)

/** This flag determines if the compiler should return the statistic in a
 *  structure format. If this flag is set, the data type of value must be
 *  of type ''fm_aclCompilerStats''.
 *  
 *  \chips  FM3000, FM4000, FM6000, FM10000 */
#define FM_ACL_COMPILE_FLAG_RETURN_STATS            (1 << 5)

/** The compiler will validate that an updated ACL configuration can be applied
 *  to the hardware without disrupting traffic (with a call to ''fmApplyACL'' 
 *  using the ''FM_ACL_APPLY_FLAG_NON_DISRUPTIVE'' flag). No change is actually 
 *  made to the internal ACL data structures. If an error code other than 
 *  ''FM_OK'' is returned, the current ACL configuration cannot be applied to 
 *  the hardware without disrupting traffic. In such a case, it may still be 
 *  possible to apply the configuration in a disruptive fashion, but 
 *  ''fmCompileACL'' must first be called without this flag.
 *                                                                      \lb\lb
 *  Note that the ACL attribute ''FM_ACL_MODE'' should be set to its
 *  default value of FM_ACL_MODE_STANDARD for all ACLs when using this flag.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE          (1 << 6)

/** The compiler will validate that it can fit the current ACL configuration
 *  using the provided FFU slice range. This flag can be applied in parallel
 *  with ''FM_ACL_COMPILE_FLAG_RETURN_STATS'' or
 *  ''FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE''. The value must be of type
 *  ''fm_aclCompilerTryAlloc''. Switch attribute
 *  ''FM_FFU_SLICE_ALLOCATIONS'' may be used to dynamically modify the FFU
 *  slice allocation.
 *  
 *  \chips  FM6000, FM10000 */
#define FM_ACL_COMPILE_FLAG_TRY_ALLOC               (1 << 7)

/** @} (end of Doxygen group) */

/* Placeholders for unimplemented flags - don't include in API document: */

/* The compiler will only process the internal ACL Id specified by the value.
 * The value must be of type ''fm_int''. This flag only works in parallel with
 * ''FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE''. */
#define FM_ACL_COMPILE_FLAG_INTERNAL                (1 << 8)

/* L4 port mapper aggressive mode. */
#define FM_ACL_COMPILE_FLAG_L4_MAPPER_AGGRESSIVE    (3 << 1)

/*  IP address mapper normal mode.
 *                                                                      \lb\lb
 *  In NORMAL mode, the compiler uses the IP mapper to match IPv6 address
 *  prefixes, replacing the SIP or DIP address keys in an ACL with a
 *  combination of address and mapper keys. This mode is effective when
 *  the ACLs specify a small number of common IPv6 address prefixes. */
#define FM_ACL_COMPILE_FLAG_IP_MAPPER_NORMAL        (1 << 4)

/* IP mapper aggressive mode. */
#define FM_ACL_COMPILE_FLAG_IP_MAPPER_AGGRESSIVE    (3 << 3)

/* Convert mode to mapper flags. */
#define FM_ACL_COMPILE_L4_MAPPER_FLAGS(mode)    (((mode) & 0x03) << 1)
#define FM_ACL_COMPILE_IP_MAPPER_FLAGS(mode)    (((mode) & 0x03) << 3)

/* Convert mapper flags to mode. */
#define FM_ACL_COMPILE_L4_MAPPER_MODE(flags)    (((flags) >> 1) & 0x03)
#define FM_ACL_COMPILE_IP_MAPPER_MODE(flags)    (((flags) >> 3) & 0x03)



/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmCompileACL''. */
#define fmACLCompile(sw, statusText, statusTextLength, flags) \
        fmCompileACL( (sw), (statusText), (statusTextLength), (flags) )

/** A legacy synonym for ''fmApplyACL''. */
#define fmACLApply(sw, flags) \
        fmApplyACL( (sw), (flags) )

/** A legacy synonym for ''fmDeleteACLRule''. */
#define fmRemoveACLRule(sw, acl, rule) \
        fmDeleteACLRule( (sw), (acl), (rule) ) 

/** A legacy synonym for ''fmAddACLPort''. */
#define fmSetPortACL(sw, acl, port, type) \
        fmAddACLPort( (sw), (acl), (port), (type) ) 

/** A legacy synonym for ''fmAddACLPortExt''. */
#define fmSetPortACLExt(sw, acl, portAndType) \
        fmAddACLPortExt( (sw), (acl), (portAndType) ) 

/** A legacy synonym for ''fmGetACLNext''. */
#define fmGetNextACL(sw, currentAcl, nextAcl) \
        fmGetACLNext( (sw), (currentAcl), (nextAcl) ) 

/** A legacy synonym for ''fmGetACLFirst''. */
#define fmGetFirstACL(sw, firstAcl) \
        fmGetACLFirst( (sw), (firstAcl) ) 

/** A legacy synonym for ''fmGetPortACLFirst''. */
#define fmGetFirstPortACL(sw, port, firstACL) \
        fmGetPortACLFirst( (sw), (port), (firstACL) ) 

/** A legacy synonym for ''fmGetPortACLFirstExt''. */
#define fmGetFirstPortACLExt(sw, port, aclAndType) \
        fmGetPortACLFirstExt( (sw), (port), (aclAndType) ) 

/** A legacy synonym for ''fmGetPortACLNext''. */
#define fmGetNextPortACL(sw, port, currentACL, nextACL) \
        fmGetPortACLNext( (sw), (port), (currentACL), (nextACL) ) 

/** A legacy synonym for ''fmGetPortACLNextExt''. */
#define fmGetNextPortACLExt(sw, port, aclAndType) \
        fmGetPortACLNextExt( (sw), (port), (aclAndType) ) 

/** A legacy synonym for ''fmGetACLPortFirst''. */
#define fmGetFirstACLPort(sw, acl, portAndType) \
        fmGetACLPortFirst( (sw), (acl), (portAndType) ) 

/** A legacy synonym for ''fmGetACLPortNext''. */
#define fmGetNextACLPort(sw, acl, portAndType) \
        fmGetACLPortNext( (sw), (acl), (portAndType) ) 
        
/** A legacy synonym for ''fmGetACLRuleFirst''. */
#define fmGetFirstACLRule(sw, acl, firstRule, cond, value, action, param)) \
        fmGetACLRuleFirst( (sw), (acl), (firstRule), (cond), \
                           (value), (action), (param) ) 
        
/** A legacy synonym for ''fmGetACLRuleFirstExt''. */
#define fmGetFirstACLRuleExt(sw, acl, firstRule, cond, value, action, param) \
        fmGetACLRuleFirstExt( (sw), (acl), (firstRule), (cond), \
                           (value), (action), (param) ) 
        
/** A legacy synonym for ''fmClearACLPort''. */
#define fmClearPortACL(sw, acl) \
        fmClearACLPort( (sw), (acl) ) 

/** A legacy synonym for ''fmDeleteACLPort''. */
#define fmDeletePortACL(sw, acl, port) \
        fmDeleteACLPort( (sw), (acl), (port) ) 

/** A legacy synonym for ''fmDeleteACLPortExt''. */
#define fmDeletePortACLExt(sw, acl, portAndType) \
        fmDeleteACLPortExt( (sw), (acl), (portAndType) ) 

/** A legacy synonym for ''fmGetACLRuleIngressPortFirst''. */
#define fmGetFirstACLRuleIngressPort(sw, acl, rule, firstPort) \
        fmGetACLRuleIngressPortFirst( (sw), (acl), (rule), (firstPort) ) 

/** A legacy synonym for ''fmGetACLRuleNext''. */
#define fmGetNextACLRule(sw, acl, currentRule, nextRule, cond, value, action, param) \
        fmGetACLRuleNext( (sw), (acl), (currentRule), (nextRule), (cond), \
                          (value), (action), (param) ) 

/** A legacy synonym for ''fmGetACLRuleNextExt''. */
#define fmGetNextACLRuleExt(sw, acl, currentRule, nextRule, cond, value, action, param) \
        fmGetACLRuleNextExt( (sw), (acl), (currentRule), (nextRule), (cond), \
                          (value), (action), (param) ) 

/** A legacy synonym for ''fmGetACLRuleIngressPortNext''. */
#define fmGetNextACLRuleIngressPort(sw, acl, rule, currentPort, nextPort) \
        fmGetACLRuleIngressPortNext( (sw), (acl), (rule), (currentPort), (nextPort) ) 

/* Legacy synonyms for ACL portSets */

/** A legacy synonym for ''fmCreateACLPortSet''. */
#define fmCreateAclPortSet(sw, portSet) \
        fmCreateACLPortSet( (sw), (portSet) ) 

/** A legacy synonym for ''fmDeleteACLPortSet''. */
#define fmDeleteAclPortSet(sw, portSet) \
        fmDeleteACLPortSet( (sw), (portSet) ) 

/** A legacy synonym for ''fmAddACLPortSetPort''. */
#define fmAddAclPortSetPort(sw, portSet, port) \
        fmAddACLPortSetPort( (sw), (portSet), (port) ) 

/** A legacy synonym for ''fmDeleteACLPortSetPort''. */
#define fmRemoveAclPortSetPort(sw, portSet, port) \
        fmDeleteACLPortSetPort( (sw), (portSet), (port) )

/** A legacy synonym for ''fmGetACLPortSetFirst''. */
#define fmGetFirstAclPortSet(sw, portSet) \
        fmGetACLPortSetFirst( (sw), (portSet) ) 

/** A legacy synonym for ''fmGetACLPortSetNext''. */
#define fmGetNextAclPortSet(sw, currentPortSet, nextPortSet) \
        fmGetACLPortSetNext( (sw), (currentPortSet), (nextPortSet) ) 

/** A legacy synonym for ''fmGetACLPortSetPortFirst''. */
#define fmGetFirstAclPortSetPort(sw, portSet, port) \
        fmGetACLPortSetPortFirst( (sw), (portSet), (port) ) 

/** A legacy synonym for ''fmGetACLPortSetPortNext''. */
#define fmGetNextAclPortSetPort(sw, portSet, currentPort, nextPort) \
        fmGetACLPortSetPortNext( (sw), (portSet), (currentPort), (nextPort) ) 

/* Legacy synonyms for ACL portSets which are now generic portSets */

/** A legacy synonym for ''fmCreatePortSet''. */
#define fmCreateACLPortSet(sw, portSet) \
        fmCreatePortSet( (sw), (portSet) ) 

/** A legacy synonym for ''fmDeletePortSet''. */
#define fmDeleteACLPortSet(sw, portSet) \
        fmDeletePortSet( (sw), (portSet) ) 

/** A legacy synonym for ''fmAddPortSetPort''. */
#define fmAddACLPortSetPort(sw, portSet, port) \
        fmAddPortSetPort( (sw), (portSet), (port) ) 

/** A legacy synonym for ''fmDeletePortSetPort''. */
#define fmDeleteACLPortSetPort(sw, portSet, port) \
        fmDeletePortSetPort( (sw), (portSet), (port) )

/** A legacy synonym for ''fmGetPortSetFirst''. */
#define fmGetACLPortSetFirst(sw, portSet) \
        fmGetPortSetFirst( (sw), (portSet) ) 

/** A legacy synonym for ''fmGetPortSetNext''. */
#define fmGetACLPortSetNext(sw, currentPortSet, nextPortSet) \
        fmGetPortSetNext( (sw), (currentPortSet), (nextPortSet) ) 

/** A legacy synonym for ''fmGetPortSetPortFirst''. */
#define fmGetACLPortSetPortFirst(sw, portSet, port) \
        fmGetPortSetPortFirst( (sw), (portSet), (port) ) 

/** A legacy synonym for ''fmGetPortSetPortNext''. */
#define fmGetACLPortSetPortNext(sw, portSet, currentPort, nextPort) \
        fmGetPortSetPortNext( (sw), (portSet), (currentPort), (nextPort) ) 

/** A legacy synonym for ''FM_ERR_NO_FREE_PORT_SET''. */
#define FM_ERR_NO_FREE_ACL_PORT_SET           FM_ERR_NO_FREE_PORT_SET

/** A legacy synonym for ''FM_ERR_INVALID_PORT_SET''. */
#define FM_ERR_INVALID_ACL_PORT_SET           FM_ERR_INVALID_PORT_SET

/** A legacy synonym for ''FM_ERR_NO_PORT_SET''. */
#define FM_ERR_NO_ACL_PORT_SET                FM_ERR_NO_PORT_SET

/** A legacy synonym for ''FM_ERR_NO_PORT_SET_PORT''. */
#define FM_ERR_NO_ACL_PORT_SET_PORT           FM_ERR_NO_PORT_SET_PORT

/** A legacy synonym for ''FM_PORT_SET_ALL''. */
#define FM_ACL_PORT_SET_ALL                   FM_PORT_SET_ALL

/** A legacy synonym for ''FM_PORT_SET_ALL_BUT_CPU''. */
#define FM_ACL_PORT_SET_ALL_BUT_CPU           FM_PORT_SET_ALL_BUT_CPU

/** A legacy synonym for ''FM_PORT_SET_ALL_EXTERNAL''. */
#define FM_ACL_PORT_SET_ALL_EXTERNAL          FM_PORT_SET_ALL_EXTERNAL

/** A legacy synonym for ''FM_ACL_MATCH_SRC_GLORT''. */
#define FM_ACL_MATCH_SOURCE_GLORT             FM_ACL_MATCH_SRC_GLORT

/** @} (end of Doxygen group) */


/* A legacy synonym for FM_PORT_SET_UNUSED. */
#define FM_ACL_PORT_SET_UNUSED                FM_PORT_SET_UNUSED 

/* A legacy synonym for FM_PORT_SET_RESERVED. */
#define FM_ACL_PORT_SET_RESERVED              FM_PORT_SET_RESERVED


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

/* sets and removes ACL rules */
fm_status fmCreateACL(fm_int sw, fm_int acl);
fm_status fmCreateACLExt(fm_int    sw,
                         fm_int    acl,
                         fm_uint32 scenarios,
                         fm_int    precedence);
fm_status fmDeleteACL(fm_int sw, fm_int acl);
fm_status fmSetACLAttribute(fm_int sw, fm_int acl, fm_int attr, void *value);
fm_status fmGetACLAttribute(fm_int sw, fm_int acl, fm_int attr, void *value);

fm_status fmGetACLRuleAttribute(fm_int sw,
                                fm_int acl,
                                fm_int rule,
                                fm_int attr,
                                void*  value);

/* sets a condition and its associated value for the given ACL */
fm_status fmAddACLRule(fm_int             sw,
                       fm_int             acl,
                       fm_int             rule,
                       fm_aclCondition    cond,
                       const fm_aclValue *value,
                       fm_aclAction       action,
                       fm_aclParam        param);
fm_status fmAddACLRuleExt(fm_int                sw,
                          fm_int                acl,
                          fm_int                rule,
                          fm_aclCondition       cond,
                          const fm_aclValue *   value,
                          fm_aclActionExt       action,
                          const fm_aclParamExt *param);

fm_status fmDeleteACLRule(fm_int sw, fm_int acl, fm_int rule);

fm_status fmUpdateACLRule(fm_int                sw,
                          fm_int                acl,
                          fm_int                rule,
                          fm_aclCondition       cond,
                          const fm_aclValue *   value,
                          fm_aclActionExt       action,
                          const fm_aclParamExt *param);

fm_status fmSetACLRuleState(fm_int sw,
                            fm_int acl,
                            fm_int rule,
                            fm_aclEntryState state);

fm_status fmGetACLRuleState(fm_int sw,
                            fm_int acl,
                            fm_int rule,
                            fm_aclEntryState* state);

/* gets the rules within a given ACL */
fm_status fmGetACLRuleFirst(fm_int           sw,
                            fm_int           acl,
                            fm_int *         firstRule,
                            fm_aclCondition *cond,
                            fm_aclValue **   value,
                            fm_aclAction *   action,
                            fm_aclParam *    param);
fm_status fmGetACLRule(fm_int           sw,
                       fm_int           acl,
                       fm_int           rule,
                       fm_aclCondition *cond,
                       fm_aclValue *    value,
                       fm_aclActionExt *action,
                       fm_aclParamExt * param);
fm_status fmGetACLRuleFirstExt(fm_int           sw,
                               fm_int           acl,
                               fm_int *         firstRule,
                               fm_aclCondition *cond,
                               fm_aclValue *    value,
                               fm_aclActionExt *action,
                               fm_aclParamExt * param);

fm_status fmGetACLRuleNext(fm_int           sw,
                           fm_int           acl,
                           fm_int           currentRule,
                           fm_int *         nextRule,
                           fm_aclCondition *cond,
                           fm_aclValue **   value,
                           fm_aclAction *   action,
                           fm_aclParam *    param);
fm_status fmGetACLRuleNextExt(fm_int           sw,
                              fm_int           acl,
                              fm_int           currentRule,
                              fm_int *         nextRule,
                              fm_aclCondition *cond,
                              fm_aclValue *    value,
                              fm_aclActionExt *action,
                              fm_aclParamExt * param);

fm_status fmGetACL(fm_int           sw,
                   fm_int           acl,
                   fm_aclArguments *args);
fm_status fmGetACLFirst(fm_int  sw,
                        fm_int *firstAcl);
fm_status fmGetACLNext(fm_int  sw,
                       fm_int  currentAcl,
                       fm_int *nextAcl);


/* set and remove ACL associatons for a port */
fm_status fmAddACLPort(fm_int sw, fm_int acl, fm_int port, fm_aclType type);
fm_status fmAddACLPortExt(fm_int                   sw,
                          fm_int                   acl,
                          const fm_aclPortAndType *portAndType);
fm_status fmDeleteACLPort(fm_int sw, fm_int acl, fm_int port);
fm_status fmDeleteACLPortExt(fm_int                   sw,
                             fm_int                   acl,
                             const fm_aclPortAndType *portAndType);
fm_status fmClearACLPort(fm_int sw, fm_int acl);


/* get the ACLs associated with a port */
fm_status fmGetPortACLFirst(fm_int  sw,
                            fm_int  port,
                            fm_int *firstACL);
fm_status fmGetPortACLFirstExt(fm_int            sw,
                               fm_int            port,
                               fm_aclAclAndType *aclAndType);

fm_status fmGetPortACLNext(fm_int  sw,
                           fm_int  port,
                           fm_int  currentACL,
                           fm_int *nextACL);
fm_status fmGetPortACLNextExt(fm_int            sw,
                              fm_int            port,
                              fm_aclAclAndType *aclAndType);

fm_status fmGetACLPortFirst(fm_int             sw,
                            fm_int             acl,
                            fm_aclPortAndType *portAndType);
fm_status fmGetACLPortNext(fm_int             sw,
                           fm_int             acl,
                           fm_aclPortAndType *portAndType);


/* returns the count associated with a COUNT rule */
fm_status fmGetACLCount(fm_int     sw,
                        fm_int     acl,
                        fm_int     rule,
                        fm_uint64 *frameCount);
fm_status fmGetACLCountExt(fm_int          sw,
                           fm_int          acl,
                           fm_int          rule,
                           fm_aclCounters *counters);
fm_status fmResetACLCount(fm_int sw,
                          fm_int acl,
                          fm_int rule);

fm_status fmGetACLEgressCount(fm_int          sw,
                              fm_int          logicalPort,
                              fm_aclCounters *counters);
fm_status fmResetACLEgressCount(fm_int          sw,
                                fm_int          logicalPort);

fm_status fmCompileACL(fm_int    sw,
                       fm_text   statusText,
                       fm_int    statusTextLength,
                       fm_uint32 flags);
fm_status fmCompileACLExt(fm_int    sw,
                          fm_text   statusText,
                          fm_int    statusTextLength,
                          fm_uint32 flags,
                          void *    value);
fm_status fmApplyACL(fm_int sw, fm_uint32 flags);
fm_status fmApplyACLExt(fm_int    sw,
                        fm_uint32 flags,
                        void *    value);
fm_status fmGetACLRuleIngressPortFirst(fm_int  sw,
                                       fm_int  acl,
                                       fm_int  rule,
                                       fm_int *firstPort);
fm_status fmGetACLRuleIngressPortNext(fm_int  sw,
                                      fm_int  acl,
                                      fm_int  rule,
                                      fm_int  currentPort,
                                      fm_int *nextPort);

#endif /* __FM_FM_API_ACL_H */
