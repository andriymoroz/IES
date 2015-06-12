/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_trigger.h
 * Creation Date:   May 29, 2008
 * Description:     Prototypes and structure definitions for triggers.
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

#ifndef __FM_FM_API_TRIGGER_H
#define __FM_FM_API_TRIGGER_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Public defines to be used in fm_triggerConditionCfg.matchFrameClassMask */
#define FM_TRIGGER_FRAME_CLASS_UCAST                (1 << 0)
#define FM_TRIGGER_FRAME_CLASS_BCAST                (1 << 1)
#define FM_TRIGGER_FRAME_CLASS_MCAST                (1 << 2)

/* Public defines to be used in fm_triggerConditionCfg.matchRoutedMask */
#define FM_TRIGGER_SWITCHED_FRAMES                  (1 << 0)
#define FM_TRIGGER_ROUTED_FRAMES                    (1 << 1)

/* Public defines to be used in fm_triggerConditionCfg.matchFtypeMask */
#define FM_TRIGGER_FTYPE_NORMAL                     (1 << 0)
#define FM_TRIGGER_FTYPE_SPECIAL                    (1 << 2) 


/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of trigger condition matching cases
 **************************************************/
typedef enum
{
    /** Associated frame property must not equal the corresponding
     *  trigger parameter in order to match. */
    FM_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL = 0, 

    /** Associated frame property must equal the corresponding
     *  trigger parameter in order to match. */
    FM_TRIGGER_MATCHCASE_MATCHIFEQUAL, 

    /** Match unconditionally */
    FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL, 

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_MATCHCASE_MAX

} fm_triggerMatchCase;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of trigger TX Destination mask matching cases
 **************************************************/
typedef enum
{
    /** Match if the destination mask of the frame does not
     *  contain any of the ports in fm_triggerConditionCfg.txPortset.
     *  When used with an empty portSet, this trigger condition will always
     *  hit (even for dropped frames).  */
    FM_TRIGGER_TX_MASK_DOESNT_CONTAIN = 0, 

    /** Match if the destination mask of the frame contains one or more
     *  ports defined in fm_triggerConditionCfg.txPortset.
     *  When used with an empty portSet, this trigger condition will
     *  NEVER hit. */
    FM_TRIGGER_TX_MASK_CONTAINS, 

    /** Match if the destination mask of the frame matches the same ports
     *  defined in fm_triggerConditionCfg.txPortset.
     *  When used with an empty portSet, this trigger condition will hit
     *  on dropped frames only. */
    FM_TRIGGER_TX_MASK_EQUALS, 

    /** Match if the destination mask of the frame does not exactly match
     *  the ports defined in fm_triggerConditionCfg.txPortset.
     *  When used with an empty portSet, this trigger condition will always
     *  hit except when the frame is dropped. */
    FM_TRIGGER_TX_MASK_DOESNT_EQUAL,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_TX_MASK_MAX

} fm_triggerTxMatchCase;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of trigger random seeds that can be used 
 * for trigger random matching. 
 **************************************************/
typedef enum
{
    /** Random generator A */ 
    FM_TRIGGER_RAND_GEN_A = 0,

    /** Random generator B */ 
    FM_TRIGGER_RAND_GEN_B,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_RAND_MATCH_MODE_MAX

} fm_triggerRandGenerator;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger forwarding actions.
 **************************************************/
typedef enum
{
    /** Do not change the pipeline forwarding decision.   */
    FM_TRIGGER_FORWARDING_ACTION_ASIS = 0,

    /** Forward the frame with a new destination glort. Destination
     *  glort bits will be overridden from newDestGlort whose
     *  corresponding newDestGlortMask bits are '1'. This action can
     *  also be used to undo a drop decision from earlier in the
     *  frame processing pipeline. When a frame is undropped, its
     *  glort and destination mask will be restored just before being
     *  dropped. */
    FM_TRIGGER_FORWARDING_ACTION_FORWARD,

    /** Redirect a frame by overriding the destination mask and
     *  destination glort. The destination mask will be overridden by
     *  newDestPortset and the destination glort will be overridden by
     *  newDestGlort, whose corresponding newDestGlortMask bits are set to
     *  '1'. If the filterDestMask parameter is enabled, LAG filtering will
     *  apply to this new destination mask. */
    FM_TRIGGER_FORWARDING_ACTION_REDIRECT,

    /** Remove one or more ports from the destination mask. The
     *  ports to be removed can be configured in the dropMask
     *  portset. */
    FM_TRIGGER_FORWARDING_ACTION_DROP,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_FORWARDING_ACTION_MAX

} fm_triggerForwardingAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger trapping actions.
 **************************************************/
typedef enum
{
    /** No change to the CPU trapping decision */
    FM_TRIGGER_TRAP_ACTION_ASIS = 0,

    /** Trap the frame to the CPU. This action overrides the
     *  forwardingAction. */
    FM_TRIGGER_TRAP_ACTION_TRAP, 

    /** Log the frame to the CPU */
    FM_TRIGGER_TRAP_ACTION_LOG, 
     
    /** Override a trap or log decision (by canceling it) from an
     *  earlier point in the frame processing pipeline. */
    FM_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG, 

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_TRAP_ACTION_MAX

} fm_triggerTrapAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger mirroring actions.
 **************************************************/
typedef enum
{
    /** Do not mirror */
    FM_TRIGGER_MIRROR_ACTION_NONE = 0,

    /** Enable mirroring. The mirrorSelect parameter should be
     *  configured to select which mirror should be used (mirror0
     *  or mirror1). The mirrorProfile should select which
     *  profile of the given mirror should be used. */
    FM_TRIGGER_MIRROR_ACTION_MIRROR, 

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_MIRROR_ACTION_MAX

} fm_triggerMirrorAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger actions for setting switch 
 * priority.
 **************************************************/
typedef enum
{
    /** Leave the switch priority unchanged */
    FM_TRIGGER_SWPRI_ACTION_ASIS = 0,

    /** Override the switch priority of the frame with the value in
     *  newSwitchPri */
    FM_TRIGGER_SWPRI_ACTION_REASSIGN,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_SWPRI_ACTION_MAX

} fm_triggerSwpriAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger actions for setting 
 * the egress vlan1.
 **************************************************/
typedef enum
{
    /** Leave the vlan1 unchanged */
    FM_TRIGGER_VLAN_ACTION_ASIS = 0,

    /** Override the vlan1 of the frame with the value in
     *  newVlan1 */
    FM_TRIGGER_VLAN_ACTION_REASSIGN,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_VLAN_ACTION_MAX

} fm_triggerVlanAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger learning actions.
 **************************************************/
typedef enum
{
    /** Do not take any action for learning */
    FM_TRIGGER_LEARN_ACTION_ASIS = 0,

    /** Do not add an entry to the MAC table change
     *  notification FIFO, even if the SMAC is new or is moved. */
    FM_TRIGGER_LEARN_ACTION_CANCEL,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_LEARN_ACTION_MAX

} fm_triggerLearningAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger rate-limiting actions.
 **************************************************/
typedef enum
{
    /** Do not not apply rate-limiting */
    FM_TRIGGER_RATELIMIT_ACTION_ASIS = 0,

    /** Apply a rate-limiter to this trigger. The ID of the rate
     *  limiter should be configured in rateLimitNum. Rate limiters
     *  ID's can be allocated with ''fmAllocateTriggerResource'' and
     *  configured with ''fmSetTriggerRateLimiter'' */
    FM_TRIGGER_RATELIMIT_ACTION_RATELIMIT,

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_RATELIMIT_ACTION_MAX

} fm_triggerRateLimitAction;




/**************************************************/
/** \ingroup typeEnum 
 *  
 * List of possible trigger resources to allocate 
 * with fmAllocateTriggerResource. These resources 
 * are shared between the API and application. 
 **************************************************/
typedef enum
{
    /** Allocate a MAC Address trig ID. There are 62 MAC address
     *  trig ID's available.
     *  
     *  \note    This resource is currently reserved for internal use. */
    FM_TRIGGER_RES_MAC_ADDR = 0,

    /** Allocate a VLAN trig ID. There are 63 vlan trig ID's
     *  available.
     *  
     *  \note    This resource is currently reserved for internal use. */
    FM_TRIGGER_RES_VLAN,

    /** Allocate a FFU trig ID action bit. FFU trigger ID's are made
     *  of a value and mask. When an action bit is allocated, the
     *  position of the allocated bit is returned.  It is possible to
     *  allocate more than one bit for a feature. The allocated
     *  bit(s) can then be used in the FFU/Trigger in the value/mask
     *  form, where mask should only contain the allocated bit(s)
     *  and the value is the state of the bit(s). Note that this
     *  resource is shared between the API and application. Bits are
     *  allocated on a first come basis and may not be contiguous.
     *  There are 8 bits available. */
    FM_TRIGGER_RES_FFU, 

    /** Allocate a rate-limiter ID. There are 16 rate-limiter ID's
     *  available. */
    FM_TRIGGER_RES_RATE_LIMITER, 

    /** Allocate a mirror profile. There are a maximum of 64 mirror profile
     *  handles available.
     *  
     *  \note   This resource is currently reserved for internal use. */
    FM_TRIGGER_RES_MIRROR_PROFILE, 

    /* ---- Add new values above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_TRIGGER_RES_MAX,

} fm_triggerResourceType;




/**************************************************/
/** \ingroup typeScalar
 * A trigger handler action condition mask is used 
 * as a parameter in the ''fm_triggerCondition'' 
 * structure. It comprises a bit mask representing 
 * the set of valid frame processing pipeline 
 * actions for a trigger to hit. See 
 * ''Trigger Handler Action Condition Masks'' for 
 * definitions of each bit in the condition mask.
 *                                              \lb\lb
 *  Note that at least one condition bit must be set
 *  for a trigger to hit.
 **************************************************/
typedef fm_uint64    fm_triggerHaCondition;




/**************************************************/
/** Trigger Handler Action Condition Masks
 *  \ingroup constTrigHandActCond
 *  \page trigHandActCondPage
 *
 *  The following set of bit masks may be ORed
 *  together to produce an ''fm_triggerHaCondition''
 *  value. Each bit mask selects a frame processing
 *  pipeline action to match on.
 *                                              \lb\lb
 *  Note that at least one action bit must be set
 *  for a trigger to hit.
 **************************************************/
/** \ingroup constTrigHandActCond
 * @{ */

/** Match on specially handled frames. */
#define FM_TRIGGER_HA_FORWARD_SPECIAL      (FM_LITERAL_U64(1) << 0)

/** Match on dropped frames due to header parse error. */
#define FM_TRIGGER_HA_DROP_PARSE_ERROR     (FM_LITERAL_U64(1) << 1)

/** Match on a frame drop due a parity error detected. */
#define FM_TRIGGER_HA_DROP_PARITY_ERROR    (FM_LITERAL_U64(1) << 2)

/** Match on trapped MAC control frames (BPDU, 802.1x, GARP, LACP,
 *  IEEE_OTHER). */
#define FM_TRIGGER_HA_TRAP_RESERVED_MAC    (FM_LITERAL_U64(1) << 3)

/** Match on trapped MAC control frames (BPDU, 802.1x, GARP, LACP, IEEE_OTHER)
 *  that were remapped to another priority. */
#define FM_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP  (FM_LITERAL_U64(1) << 4)

/* Bit 5 is unused */
/* Bit 6 is unused */
/* Bit 7 is unused */
/* Bit 8 is unused */
/* Bit 9 is unused */

/** Match on dropped IEEE MAC Control Ethertype (including pause frames). */
#define FM_TRIGGER_HA_DROP_CONTROL         (FM_LITERAL_U64(1) << 10)

/** Match on dropped MAC control frames (BPDU, 802.1x, GARP, LACP,
 *  IEEE_OTHER). */
#define FM_TRIGGER_HA_DROP_RESERVED_MAC    (FM_LITERAL_U64(1) << 11)

/** Match on dropped frames due to vlan tagging rule violations. See
 *  ''FM_PORT_DROP_UNTAGGED'' and ''FM_PORT_DROP_TAGGED''. */
#define FM_TRIGGER_HA_DROP_TAG             (FM_LITERAL_U64(1) << 12)

/** Match on dropped frames due to invalid SMAC (NULL, Multicast,
 *  Broadcast). */
#define FM_TRIGGER_HA_DROP_INVALID_SMAC    (FM_LITERAL_U64(1) << 13)

/** Match on dropped frames due to MAC security violation (moved
 *  address). */
#define FM_TRIGGER_HA_DROP_PORT_SV         (FM_LITERAL_U64(1) << 14)

/** Match on trapped frames that had the CPU's MAC address. */
#define FM_TRIGGER_HA_TRAP_CPU             (FM_LITERAL_U64(1) << 15)

/** Match on dropped frames due to ingress vlan membership
 *  violation. */
#define FM_TRIGGER_HA_DROP_VLAN_IV         (FM_LITERAL_U64(1) << 16)

/** Match on dropped frames due to ingress spanning-tree check
 *  (ingress non-learning state). */
#define FM_TRIGGER_HA_DROP_STP_INL         (FM_LITERAL_U64(1) << 17)

/** Match on dropped frames due to ingress spanning-tree check
 *  (ingress learning state). */
#define FM_TRIGGER_HA_DROP_STP_IL          (FM_LITERAL_U64(1) << 18)

/** Match on dropped frames due to the FFU's drop action. */
#define FM_TRIGGER_HA_DROP_FFU             (FM_LITERAL_U64(1) << 19)

/** Match on trapped frames due to the FFU's trap action. */
#define FM_TRIGGER_HA_TRAP_FFU             (FM_LITERAL_U64(1) << 20)

/** Match on ICMP trapped frames that have TTL <= 1. */
#define FM_TRIGGER_HA_TRAP_ICMP_TTL        (FM_LITERAL_U64(1) << 21)

/** Match on trapped frames that have IP options. */
#define FM_TRIGGER_HA_TRAP_IP_OPTION       (FM_LITERAL_U64(1) << 22)

/** Match on trapped frames due to MTU violation. */
#define FM_TRIGGER_HA_TRAP_MTU             (FM_LITERAL_U64(1) << 23)

/** Match on IGMP trapped frames. */
#define FM_TRIGGER_HA_TRAP_IGMP            (FM_LITERAL_U64(1) << 24)

/** Match on trapped frames that have a TTL <= 1 (non-ICMP frames). */
#define FM_TRIGGER_HA_TRAP_TTL             (FM_LITERAL_U64(1) << 25)

/** Match on dropped frames that have a TTL <= 1. */
#define FM_TRIGGER_HA_DROP_TTL             (FM_LITERAL_U64(1) << 26)

/** Match on dropped frames due to flood control for frames that
 *  have a destination lookup failure. */
#define FM_TRIGGER_HA_DROP_DLF             (FM_LITERAL_U64(1) << 27)

/** Match on dropped frames due to a GLORT_CAM miss. */
#define FM_TRIGGER_HA_DROP_GLORT_CAM_MISS  (FM_LITERAL_U64(1) << 28)

/** Match on dropped frames that have a NULL destination mask. */
#define FM_TRIGGER_HA_DROP_NULL_DEST       (FM_LITERAL_U64(1) << 29)

/** Match on dropped frames that have an egress VLAN violation. */
#define FM_TRIGGER_HA_DROP_VLAN_EV         (FM_LITERAL_U64(1) << 30)

/** Match on dropped frames due to policing. */
#define FM_TRIGGER_HA_DROP_POLICER         (FM_LITERAL_U64(1) << 31)

/** Match on dropped frames due to egress spanning tree check. */
#define FM_TRIGGER_HA_DROP_STP_E           (FM_LITERAL_U64(1) << 32)

/** Match on dropped frames due to loopback suppression. */
#define FM_TRIGGER_HA_DROP_LOOPBACK        (FM_LITERAL_U64(1) << 33)

/** Match on FFU/ARP DGLORT forwarded frames. */
#define FM_TRIGGER_HA_FORWARD_DGLORT       (FM_LITERAL_U64(1) << 34)

/** Match on flood forwarded frames. */
#define FM_TRIGGER_HA_FORWARD_FLOOD        (FM_LITERAL_U64(1) << 35)

/** Match on switched MAC control frames (BPDU, 802.1x, GARP, LACP,
 *  IEEE_OTHER). */
#define FM_TRIGGER_HA_SWITCH_RESERVED_MAC  (FM_LITERAL_U64(1) << 36)

/** Match on normally forwarded frames. */
#define FM_TRIGGER_HA_FORWARD_FID          (FM_LITERAL_U64(1) << 37)

/** Match on logged frames due to an ingress FFU log action. */
#define FM_TRIGGER_HA_LOG_FFU_I            (FM_LITERAL_U64(1) << 38)

/** Match on logged MAC control frames (BPDU, 802.1x, GARP, LACP,
 *  IEEE_OTHER). */
#define FM_TRIGGER_HA_LOG_RESERVED_MAC     (FM_LITERAL_U64(1) << 39)

/** Match on logged frames due to an egress FFU log action
 *  (Egress ACL). */
#define FM_TRIGGER_HA_LOG_ARP_REDIRECT     (FM_LITERAL_U64(1) << 40)

/** Match on ICMP multicast logged frames that have a TTL <= 1. */
#define FM_TRIGGER_HA_LOG_MCST_ICMP_TTL    (FM_LITERAL_U64(1) << 41)

/** Match on logged IP multicast frames that have TTL <= 1. */
#define FM_TRIGGER_HA_LOG_IP_MCST_TTL      (FM_LITERAL_U64(1) << 42)

/** Match on frames for which header parsing was too long to be completely
 *  parsed. */
#define FM_TRIGGER_HA_PARSE_TIMEOUT        (FM_LITERAL_U64(1) << 43)

/** Match on mirrored frames due to a FFU mirror action. */
#define FM_TRIGGER_HA_MIRROR_FFU           (FM_LITERAL_U64(1) << 44)

/** @} (end of Doxygen group) */




/****************************************************************************/
/** \ingroup typeStruct
 *
 * The trigger condition configuration is used to select which fields
 * the trigger should match/notMatch on. The default values 
 * specified are applied when the condition is initialized with 
 * ''fmInitTriggerCondition'' 
 ****************************************************************************/
typedef struct _fm_triggerConditionCfg
{
    /** Define the matching mode for the source MAC address trig ID
     *  from the MAC table. Trig ID's can be allocated with
     *  ''fmAllocateTriggerResource''. If not set to
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', an ID must be
     *  configured in saId. The default setting is
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''.
     *                                                                  \lb\lb
     *  Note: This field is currently reserved for internal use. It
     *  should be set to ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchSA;

    /** Define the matching mode for the destination MAC address
     *  trig ID from the MAC table. Trig ID's can be allocated with
     *  ''fmAllocateTriggerResource''. If not set to
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', an ID must be
     *  configured in daId. The default setting is
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''.
     *                                                                  \lb\lb
     *  Note: This field is currently reserved for internal use. It
     *  should be set to ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchDA;

    /** Match depending on whether a source MAC address lookup was
     *  performed and was found to exist in the MAC Table. The
     *  default setting is ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchHitSA;

    /**Match depending on whether a destination MAC address was
     *  found in the MAC Table. The default setting is
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchHitDA;

    /** Match depending on whether the source or destination MAC
     *  address was found in the MAC Table (or both). The default
     *  setting is ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchHitSADA;

    /** Define the matching mode against the vlan Trig ID from the
     *  vlan lookup table. Trig ID's can be allocated with
     *  ''fmAllocateTriggerResource''. If not set to
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', an ID must be
     *  configured in vidId. The default setting is
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''.
     *                                                          \lb\lb
     *  Note: This field is currently reserved for internal use. It
     *  should be set to ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchVlan;

    /** Define the matching mode against the FFU Trig ID as set by
     *  the ACL action ''FM_ACL_ACTIONEXT_SET_TRIG_ID''. Trig ID's can
     *  be allocated with ''fmAllocateTriggerResource''. If not set to
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', an ID must be
     *  configured in ffuId and ffuIdMask. The default setting is
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchFFU;

    /** Define the matching mode for the frame's switch priority. If
     *  not set to ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', a
     *  priority must be configured in switchPri. The default
     *  setting is ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchSwitchPri;

    /** Define the matching mode for the frame's ethertype. If not
     *  set to ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', an etherype
     *  and mask must be configured in etherType and etherTypeMask.
     *  The default setting is 
     *  ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''.  */
    fm_triggerMatchCase   matchEtherType;

    /** Define the matching mode for the destination glort. If not
     *  set to ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL'', a glort and
     *  mask must be configured in destGlort and destGlortMask. The
     *  default setting is ''FM_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL''. */
    fm_triggerMatchCase   matchDestGlort;

    /** The bit mask of enabled frame classes to match on
     *  (unicast/multicast/broadcast). For a trigger to hit, there must be at
     *  least one frame class enabled.
     *                                                                  \lb\lb
     *  The following values may be ORed to produce the desired
     *  frame class mask:                                               \lb
     *  FM_TRIGGER_FRAME_CLASS_UCAST                                    \lb
     *  FM_TRIGGER_FRAME_CLASS_MCAST                                    \lb
     *  FM_TRIGGER_FRAME_CLASS_BCAST                                    \lb
     *                                                                  \lb
     *  The default value is:                                           \lb
     *   (FM_TRIGGER_FRAME_CLASS_UCAST |                                \lb
     *    FM_TRIGGER_FRAME_CLASS_MCAST |                                \lb
     *    FM_TRIGGER_FRAME_CLASS_BCAST). */
    fm_uint32             matchFrameClassMask;

    /** The bit mask of switching decisions to match on. For a
     *  trigger to hit, there must be at least one switching
     *  decision enabled.
     *                                                                  \lb\lb
     *  The following values may be ORed to produce the desired
     *  bit mask:                                                       \lb
     *  FM_TRIGGER_SWITCHED_FRAMES                                      \lb
     *  FM_TRIGGER_ROUTED_FRAMES                                        \lb
     *                                                                  \lb
     *  The default value is:                                           \lb
     *   (FM_TRIGGER_SWITCHED_FRAMES |                                  \lb
     *    FM_TRIGGER_ROUTED_FRAMES) */
    fm_uint32             matchRoutedMask;

    /** The bit mask of frame types (fType) to match on. For a
     *  trigger to hit, there must be at least one frame type
     *  enabled.
     *                                                                  \lb\lb
     *  The following values may be ORed to produce the desired
     *  frame type bit mask:                                            \lb
     *  FM_TRIGGER_FTYPE_NORMAL                                         \lb
     *  FM_TRIGGER_FTYPE_SPECIAL                                        \lb
     *                                                                  \lb
     *  The default value is FM_TRIGGER_FTYPE_NORMAL. */
    fm_uint32             matchFtypeMask;

    /** Define the TX Destination Mask matching type. This matching type
     *  defines how the txPortset will be compared against the frame's
     *  destination mask. The default setting is
     *  ''FM_TRIGGER_TX_MASK_DOESNT_CONTAIN''. */
    fm_triggerTxMatchCase matchTx;

    /** Enable or disable random number matching. If enabled,
     *  (FM_ENABLED) the randGenerator and randMatchThreshold
     *  parameters from ''fm_triggerCondition''.param must be
     *  configured. The default value is FM_DISABLED. */
    fm_bool               matchRandomNumber;

    /** Portset handle of the portset containing the list of source
     *  ports to match on. Portsets can be created with
     *  ''fmCreatePortSet''. If there are no ports in the portset,
     *  the trigger will never hit. The static internal portsets can
     *  be used to match on any port. See ''FM_PORT_SET_ALL'' and
     *  ''FM_PORT_SET_ALL_BUT_CPU''. The default value is
     *  ''FM_PORT_SET_NONE''. */
    fm_int                rxPortset;

    /** Portset handle for the portset containing the list of
     *  destination ports used for comparison with the frame's destination
     *  mask. The type of comparison is defined by
     *  ''fm_triggerConditionCfg''.matchTx 
     *                                                                  \lb\lb
     *  The static internal portsets can be used to match on any port.
     *  See ''FM_PORT_SET_ALL'' and ''FM_PORT_SET_ALL_BUT_CPU''.
     *  The default value is ''FM_PORT_SET_NONE''. */
    fm_int                txPortset;

    /** Handler Action Mask. Each bit corresponds to a frame
     *  processing pipeline action to match on. Each action for
     *  which this trigger should hit needs to be ORed in this
     *  parameter.
     *                                                                  \lb\lb
     *  The frame processing pipeline's action will be anded with
     *  this value, a nonzero result represents a hit.
     *                                                                  \lb\lb
     *  Note that at least one condition bit must be set for a
     *  trigger to hit. See ''Trigger Handler Action Condition Masks''
     *  for a complete list of bits.
     *                                                                  \lb\lb
     *  The default value is:                                           \lb
     *  (''FM_TRIGGER_HA_FORWARD_DGLORT'' |                             \lb
     *   ''FM_TRIGGER_HA_FORWARD_FLOOD'' |                              \lb
     *   ''FM_TRIGGER_HA_FORWARD_FID'') */
    fm_triggerHaCondition HAMask;

} fm_triggerConditionCfg;




/****************************************************************************/
/** \ingroup typeStruct
 *
 * The trigger condition parameters for the selected matching conditions.
 * Values only need to be configured for fields that have been selected to
 * match on. Other values can be left blank.
 ****************************************************************************/
typedef struct _fm_triggerConditionParam
{
    /** The source MAC address trigger ID to match on. MAC
     *  trigger IDs can be allocated with ''fmAllocateTriggerResource''.
     *                                          
     *  \note   This field is currently reserved for internal use. */
    fm_uint32 saId;

    /** The destination MAC address trigger ID to match on. MAC
     *  trigger IDs can be allocated with ''fmAllocateTriggerResource''.
     *  
     *  \note   This field is currently reserved for internal use. */
    fm_uint32 daId;

    /** The vlan trigger ID to match on. Vlan trigger IDs can be
     *  allocated with ''fmAllocateTriggerResource''.
     *  
     *  \note   This field is currently reserved for internal use. */
    fm_uint32 vidId;

    /** The switch priority to match on. */
    fm_uint32 switchPri;

    /** The FFU trigger ID to match on. Note that the pipeline's FFU
     *  trigger ID as set by the pipeline will be anded with
     *  ffuIdMask before comparison. The mask may be used to hit on a 
     *  particular bit or set of bits. FFU trigger action bits can be
     *  allocated with ''fmAllocateTriggerResource''. */
    fm_uint32 ffuId;

    /** The FFU trigger ID mask to apply before comparison. */
    fm_uint32 ffuIdMask;
    
    /** The ethertype to match on (non-vlan ethertype). */
    fm_uint32 etherType;

    /** The ethertype mask to apply before comparison. */
    fm_uint32 etherTypeMask;
    
    /** The destination glort to match on. */
    fm_uint32 destGlort;

    /** The destination glort mask to apply before comparison. */
    fm_uint32 destGlortMask;
    
    /** When random matching is enabled, this parameter determines
     *  which random number generator should be used. Each random
     *  number generator is seeded differently.
     *                                                                  \lb\lb
     *  Use FM_TRIGGER_RAND_GEN_A or FM_TRIGGER_RAND_GEN_B. */
    fm_triggerRandGenerator randGenerator;

    /** When random matching is enabled, this parameter defines
     *  the random threshold below which the trigger should hit.
     *                                                                  \lb\lb
     *  If (random value <= threshold) this trigger hits
     *                                                                  \lb\lb
     *  Note that the threshold will be rounded to the nearest
     *  value supported by the hardware. When this value is read, it
     *  may differ from the value set as the rounded value will be
     *  returned.
     *                                                                  \lb\lb
     *  The random number generator generates a 24-bit value (0 to
     *  16,777,215).
     */
    fm_uint32               randMatchThreshold;

} fm_triggerConditionParam;




/****************************************************************************/
/** \ingroup typeStruct
 *
 * A trigger condition is used as an argument to 
 *  ''fmInitTriggerCondition'' and ''fmSetTriggerCondition''.
 *  The structure consists of a collection of conditions and values for
 *  these conditions that define under which conditions a
 *  trigger should hit.
 *  
 *  Note that not all members of this structure need to contain
 *  valid values, only those that are relevant for the chosen
 *  conditions.
 * 
 *  The default values specified are applied when the condition is
 *  initialized with ''fmInitTriggerCondition'' 
 ****************************************************************************/
typedef struct _fm_triggerCondition
{
    /** Selection of condition fields to match on. The default
     *  values for members of this structures are defined in
     *  ''fm_triggerConditionCfg''.  */
    fm_triggerConditionCfg   cfg;  

    /** Values to match on for the selected match conditions. Values
     *  only need to be configured for fields that have been
     *  selected to match on. Other values can be left blank. */
    fm_triggerConditionParam param;  
                                        
} fm_triggerCondition;




/****************************************************************************/
/** \ingroup typeStruct
 *
 * Selection of actions to take when a trigger hits.
 * 
 * The default values specified are applied when the action is
 * initialized with ''fmInitTriggerAction'' 
 ****************************************************************************/
typedef struct _fm_triggerActionCfg
{
    /** Forwarding action to take when trigger is hit. See 
     *  ''fm_triggerForwardingAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_FORWARDING_ACTION_ASIS''. */
    fm_triggerForwardingAction forwardingAction;

    /** Trap action to take when trigger is hit. See 
     *  ''fm_triggerTrapAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_TRAP_ACTION_ASIS''. */
    fm_triggerTrapAction       trapAction;

    /** Mirror action to take when trigger is hit. See 
     *  ''fm_triggerMirrorAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_MIRROR_ACTION_NONE''. */
    fm_triggerMirrorAction     mirrorAction;

    /** Switch priority action to take when trigger is hit. See 
     *  ''fm_triggerSwpriAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_SWPRI_ACTION_ASIS''. */
    fm_triggerSwpriAction      switchPriAction;

    /** VLAN action to take when trigger is hit. See 
     *  ''fm_triggerVlanAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_VLAN_ACTION_ASIS''. */
    fm_triggerVlanAction       vlan1Action;

    /** Learning action to take when trigger is hit. See 
     *  ''fm_triggerLearningAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_LEARN_ACTION_ASIS''. */
    fm_triggerLearningAction   learningAction;

    /** Rate limiting action to take when trigger is hit. See 
     *  ''fm_triggerRateLimitAction'' for possible values. The default 
     *  value is ''FM_TRIGGER_RATELIMIT_ACTION_ASIS''. */
    fm_triggerRateLimitAction  rateLimitAction;

} fm_triggerActionCfg;




/****************************************************************************/
/** \ingroup typeStruct
 *
 * The trigger action parameters for the selected actions. Values are only
 * needed if the action specifies so. 
 ****************************************************************************/
typedef struct _fm_triggerActionParam
{
    /** This parameter should be used when the forwardingAction is
     *  either FM_TRIGGER_FORWARDING_ACTION_FORWARD or
     *  FM_TRIGGER_FORWARDING_ACTION_REDIRECT.
     *                                                                  \lb\lb
     *  The new destination glort bits to update in the frame's
     *  destination glort. Note that only the bits whose
     *  corresponding bits in newDestGlortMask are set to '1' will
     *  be updated. */
    fm_uint32   newDestGlort;

    /** This parameter should be used when the forwardingAction is
     *  either FM_TRIGGER_FORWARDING_ACTION_FORWARD or
     *  FM_TRIGGER_FORWARDING_ACTION_REDIRECT.
     *                                                                  \lb\lb
     *  The mask of destination glort bits that should be updated
     *  with newDestGlort bits. */
    fm_uint32   newDestGlortMask;

    /** This parameter should be used when the forwardingAction is
     *  FM_TRIGGER_FORWARDING_ACTION_REDIRECT.
     *                                                                  \lb\lb
     *  Portset handle of the portset containing the list of ports
     *  that will overwrite the destination mask. Portsets
     *  can be created with ''fmCreatePortSet''. */
    fm_int      newDestPortset;

    /** This parameter should be used when the forwardingAction is
     *  FM_TRIGGER_FORWARDING_ACTION_REDIRECT.
     *                                                                  \lb\lb
     *  This parameter should be set to TRUE if LAG filtering should
     *  be applied on the new destination mask as set in
     *  newDestPortset. */
    fm_bool     filterDestMask;

    /** This parameter should be used when the forwardingAction is
     *  FM_TRIGGER_FORWARDING_ACTION_DROP.
     *                                                                  \lb\lb
     *  Portset handle of the portset containing the list of ports
     *  that should be removed from destination mask. Portsets can
     *  be created with ''fmCreatePortSet''. */
    fm_int      dropPortset;

    /** This parameter should be used when the switchPriAction is
     *  set to FM_TRIGGER_SWPRI_ACTION_REASSIGN.
     *                                                                  \lb\lb
     *  The new switch priority to reassign. */
    fm_uint32   newSwitchPri;

    /** This parameter should be used when the vlan1Action is set to
     *  FM_TRIGGER_VLAN_ACTION_REASSIGN.
     *                                                                  \lb\lb
     *  The new egress vlan1 to reassign. */
    fm_uint32   newVlan1;

    /** This parameter should be used when the rateLimitAction is
     *  set to FM_TRIGGER_RATELIMIT_ACTION_RATELIMIT.
     *                                                                  \lb\lb
     *  The rate-limiter ID of the rate-limiter to use. Rate limiters
     *  ID's can be allocated with
     *  ''fmAllocateTriggerResource'' and configured with
     *  ''fmSetTriggerRateLimiter''. */
    fm_uint32   rateLimitNum;
    
    /** This parameter should be used when the mirrorAction is
     *  set to FM_TRIGGER_MIRROR_ACTION_MIRROR.
     *                                                                  \lb\lb
     *  Select which mirror should be used with this trigger. For
     *  FM10000, there are only two mirrors (0 and 1). */
    fm_uint32   mirrorSelect;

    /** This parameter should be used when the mirrorAction is
     *  set to FM_TRIGGER_MIRROR_ACTION_MIRROR.
     *                                                                  \lb\lb
     *  Select which profile ID should be used for the given
     *  mirrorSelect. Mirror Profile IDs can be allocated with
     *  ''fmAllocateTriggerResource''. */
    fm_uint32   mirrorProfile;

} fm_triggerActionParam;




/****************************************************************************/
/** \ingroup typeStruct
 *
 * A trigger action is used as an argument to 
 * ''fmInitTriggerAction'' and ''fmSetTriggerAction''.
 * The structure consists of a collection of actions to take when a
 * trigger hits and associated values for these actions.
 *  
 * Note that not all members of this structure need to contain
 * valid values, only those that are relevant for the chosen
 * actions.
 * 
 * The default values specified are applied when the action is
 * initialized with ''fmInitTriggerAction'' 
 ****************************************************************************/
typedef struct _fm_triggerAction
{
    /** Selection of actions to take when a trigger hits. */
    fm_triggerActionCfg    cfg;

    /** Values for the selected actions. Note that not all members
     *  of this structure need to contain valid values, only those
     *  that are relevant for the chosen actions.  */
    fm_triggerActionParam  param;
    
} fm_triggerAction;




/****************************************************************************/
/** \ingroup typeStruct
 *
 * A trigger rate-limiter structure can be used with
 * ''fmSetTriggerRateLimiter'' to configure the rate and capacity of a
 * rate limitter.
 ****************************************************************************/
typedef struct _fm_rateLimiterCfg
{
    /** The rate-limiter bucket capacity in kilobytes (units of 1024
     *  byte). The capacity represents the maximum traffic burst
     *  size. The capacity should not exceed 4095KB. */
    fm_uint32 capacity;

    /** The rate at which the token bucket is refilled in kilobits
     *  per second (10,000 => 10Mbps, 10,000,000 => 10Gbps). The
     *  rate represents the maximum bandwidth that may be consumed
     *  by traffic that hits on the rate-limiter (after an initial
     *  burst whose size is controlled). The actual value will be
     *  the nearest value supported by the hardware, so the value
     *  retrieved with a call to ''fmGetTriggerRateLimiter'' may not
     *  exactly match the value set with
     *  ''fmSetTriggerRateLimiter''.                                    \lb
     *                                                                  \lb
     *  Note that the minimum/maximum rates supported are :             \lb
     *    minRate(kbps) = 31.25 * frameHandlerFreq(MHz)                 \lb
     *    maxRate(kbps) = 1023750 * frameHandlerFreq(MHz) */
    fm_uint32 rate;

    /** Portset handle of the portset containing the list of ports
     *  that should be removed from destination mask when the rate
     *  limiter exceeds its configured rate. Portsets can be created
     *  with ''fmCreatePortSet''. */
    fm_int dropPortset;

    /** The token bucket usage level in bytes. This parameter is read-only. */
    fm_int usage;

} fm_rateLimiterCfg;




/****************************************************************************/
/** \ingroup intTypeStruct
 *
 * Defines hints passed to the trigger allocation function, 
 * ''fmAllocateTrigger''.  Some hints may apply only to certain device families.
 ****************************************************************************/

typedef struct _fm_triggerRequestInfo
{
    /** Indicates if a rate limiter must be allocated for the
     *  the trigger.  See the FM4000 datasheet for how to program a trigger
     *  rate limiter. */
    fm_bool requestRateLimiter;

} fm_triggerRequestInfo;


/*****************************************************************************
 * Public Function Prototypes.
 *****************************************************************************/


fm_status fmInitTriggerCondition(fm_int sw, fm_triggerCondition *cond);
fm_status fmInitTriggerAction(fm_int sw, fm_triggerAction *action);
fm_status fmCreateTrigger(fm_int sw, fm_int group, fm_int rule);
fm_status fmDeleteTrigger(fm_int sw, fm_int group, fm_int rule);
fm_status fmSetTriggerCondition(fm_int                     sw, 
                                fm_int                     group, 
                                fm_int                     rule, 
                                const fm_triggerCondition *cond);
fm_status fmSetTriggerAction(fm_int                  sw, 
                             fm_int                  group, 
                             fm_int                  rule, 
                             const fm_triggerAction *action);
fm_status fmGetTrigger(fm_int               sw, 
                       fm_int               group, 
                       fm_int               rule, 
                       fm_triggerCondition *cond,
                       fm_triggerAction *   action);

fm_status fmGetTriggerFirst(fm_int sw, fm_int *group, fm_int *rule);
fm_status fmGetTriggerNext(fm_int sw, 
                           fm_int curGroup, 
                           fm_int curRule,
                           fm_int *nextGroup, 
                           fm_int *nextRule);

fm_status fmSetTriggerRateLimiter(fm_int sw, 
                                  fm_int rateLimiterId, 
                                  fm_rateLimiterCfg *cfg);
fm_status fmGetTriggerRateLimiter(fm_int sw, 
                                  fm_int rateLimiterId, 
                                  fm_rateLimiterCfg *cfg);

fm_status fmAllocateTriggerResource(fm_int sw, 
                                    fm_triggerResourceType res, 
                                    fm_uint32 *value);
fm_status fmFreeTriggerResource(fm_int sw, 
                                fm_triggerResourceType res, 
                                fm_uint32 value);
fm_status fmGetTriggerResourceFirst(fm_int sw, 
                                    fm_triggerResourceType res, 
                                    fm_uint32 *value);
fm_status fmGetTriggerResourceNext(fm_int sw, 
                                   fm_triggerResourceType res, 
                                   fm_uint32 curValue,
                                   fm_uint32 *nextValue);
fm_status fmSetTriggerAttribute(fm_int sw, 
                                fm_int group, 
                                fm_int rule,
                                fm_int attr, 
                                void *value);
fm_status fmGetTriggerAttribute(fm_int sw, 
                                fm_int group, 
                                fm_int rule,
                                fm_int attr, 
                                void *value);

fm_status fmAllocateTrigger(fm_int sw, 
                            fm_int *trigger, 
                            fm_triggerRequestInfo *info);

fm_status fmAllocateTriggerExt(fm_int sw, 
                               fm_text name, 
                               fm_int *trigger, 
                               fm_triggerRequestInfo *info);

fm_status fmFreeTrigger(fm_int sw, fm_int trigger); 

#endif /* __FM_FM_API_TRIGGER_H */
