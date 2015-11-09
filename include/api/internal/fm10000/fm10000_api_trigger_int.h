/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_trigger_int.h
 * Creation Date:   June 18th, 2013
 * Description:     Contains constants and functions used to support trigger.
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

#ifndef __FM_FM10000_API_TRIGGER_INT_H
#define __FM_FM10000_API_TRIGGER_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Fixed Trap Code selected to make sure LOG and TRAP use the same */
#define FM10000_MIRROR_IEEE_CODE            FM10000_TRAP_IEEE_MAC
#define FM10000_MIRROR_ICMP_TTL             FM10000_TRAP_ICMP_TTL
#define FM10000_MIRROR_TTL                  FM10000_TRAP_TTL

/* Variable Codes */
#define FM10000_MIRROR_FFU_CODE             0xA0
#define FM10000_MIRROR_ARP_CODE             0xA1

/**************************************************
 * Macros to compose/decompose the trigger key.
 **************************************************/

#define FM10000_TRIGGER_GROUP_MASK          FM_LITERAL_U64(0xFFFFFFFF00000000)
#define FM10000_TRIGGER_RULE_MASK           FM_LITERAL_U64(0x00000000FFFFFFFF)

#define FM10000_TRIGGER_GROUP_RULE_TO_KEY(grp, rule)        \
    ((((fm_uint64)(grp) << 32)) | rule)

#define FM10000_TRIGGER_KEY_TO_GROUP(key)   \
        ( (fm_int) (((key) & FM10000_TRIGGER_GROUP_MASK) >> 32) )

#define FM10000_TRIGGER_KEY_TO_RULE(key)    \
        ( (fm_int) ((key) & FM10000_TRIGGER_RULE_MASK) )


/**************************************************
 * Internal Trigger Group IDs.
 *  
 * The Group ID order determines precedence in 
 * case of conflict. Lowest wins. As stated in 
 * fmCreateTrigger(), the IDs should be < 10000. 
 **************************************************/
enum
{
    /* Mirror trigger group. */
    FM10000_TRIGGER_GROUP_MIRROR = 100,

    /* VLAN trigger group. */
    FM10000_TRIGGER_GROUP_VLAN = 200,

    /* ACL trigger group. */
    FM10000_TRIGGER_GROUP_ACL_SPECIAL = 300,

    /* Storm controllers can use up to 16 trigger groups
     * starting from this base. */
    FM10000_TRIGGER_GROUP_STORM_CTRL = 400,

    /* Security trigger group. */
    FM10000_TRIGGER_GROUP_SECURITY = 500,

    /* Flood control trigger groups. */
    FM10000_TRIGGER_GROUP_UCAST_FLOOD = 600,
    FM10000_TRIGGER_GROUP_MCAST_FLOOD = 610,
    FM10000_TRIGGER_GROUP_BCAST_FLOOD = 620,
    FM10000_TRIGGER_GROUP_MCAST_MASK_DROP_TRAP = 630,
    FM10000_TRIGGER_GROUP_MCAST_DROP_TRAP = 640,

    /* IP Options trigger group. */
    FM10000_TRIGGER_GROUP_IP_OPTIONS = 700,

    /* Routing trigger group. */
    FM10000_TRIGGER_GROUP_ROUTING = 900,

    /* Priority Mapper trigger group. */
    FM10000_TRIGGER_GROUP_PRIORITY_MAP = 1000,

};  /* end enum */


/* Definitions related to trigger resources */
#define FM10000_TRIGGER_RES_FREE                            0
#define FM10000_TRIGGER_RES_USED                            1

#define FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX                64
#define FM10000_TRIGGER_MAC_ADDR_TRIG_ID_RESERVED           2
#define FM10000_TRIGGER_MAC_ADDR_TRIG_ID_TOTAL                      \
            (FM10000_TRIGGER_MAC_ADDR_TRIG_ID_MAX -                 \
             FM10000_TRIGGER_MAC_ADDR_TRIG_ID_RESERVED)

#define FM10000_TRIGGER_VLAN_TRIG_ID_MAX                    64
#define FM10000_TRIGGER_VLAN_TRIG_ID_RESERVED               1
#define FM10000_TRIGGER_VLAN_TRIG_ID_TOTAL                          \
             (FM10000_TRIGGER_VLAN_TRIG_ID_MAX -                    \
              FM10000_TRIGGER_VLAN_TRIG_ID_RESERVED)

#define FM10000_TRIGGER_FFU_TRIG_ID_BITS                    8

#define FM10000_TRIGGER_RATE_LIMITER_ID_TOTAL               \
              FM10000_TRIGGER_RATE_LIM_CFG_1_ENTRIES

#define FM10000_TRIGGER_MIRROR_PROFILE_ID_TOTAL             \
              FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES

#define FM10000_INVALID_RATE_LIMITER_ID    FM10000_TRIGGER_RATE_LIMITER_ID_TOTAL

#define FM10000_NUM_MIRROR_PROFILES     FM10000_FH_MIRROR_PROFILE_TABLE_ENTRIES

/* HW definitions for match cases */
#define FM10000_TRIGGER_MATCHCASE_MATCHIFNOTEQUAL           0 
#define FM10000_TRIGGER_MATCHCASE_MATCHIFEQUAL              1
#define FM10000_TRIGGER_MATCHCASE_MATCHUNCONDITIONAL        2

/* HW definitions for random generators, see
 * FM10000_TRIGGER_CONDITION_CFG.MatchRandomNumber */
#define FM10000_TRIGGER_RAND_GEN_A                          0
#define FM10000_TRIGGER_RAND_GEN_B                          1

/* HW defines for frame class mask */
#define FM10000_TRIGGER_FRAME_CLASS_UCAST                (1 << 0)
#define FM10000_TRIGGER_FRAME_CLASS_BCAST                (1 << 1)
#define FM10000_TRIGGER_FRAME_CLASS_MCAST                (1 << 2)

/* HW defines for routed mask */
#define FM10000_TRIGGER_SWITCHED_FRAMES                  (1 << 0)
#define FM10000_TRIGGER_ROUTED_FRAMES                    (1 << 1)

/* HW defines for Ftype mask */
#define FM10000_TRIGGER_FTYPE_NORMAL                     (1 << 0)
#define FM10000_TRIGGER_FTYPE_SPECIAL                    (1 << 2) 

/* HW defines for handler action bits */
#define FM10000_TRIGGER_HA_FORWARD_SPECIAL          (FM_LITERAL_U64(1) << 0)
#define FM10000_TRIGGER_HA_DROP_PARSE_ERROR         (FM_LITERAL_U64(1) << 1)
#define FM10000_TRIGGER_HA_DROP_PARITY_ERROR        (FM_LITERAL_U64(1) << 2)
#define FM10000_TRIGGER_HA_TRAP_RESERVED_MAC        (FM_LITERAL_U64(1) << 3)
#define FM10000_TRIGGER_HA_TRAP_RESERVED_MAC_REMAP  (FM_LITERAL_U64(1) << 4)
/* Bit 5 is unused */
/* Bit 6 is unused */
/* Bit 7 is unused */
/* Bit 8 is unused */
/* Bit 9 is unused */
#define FM10000_TRIGGER_HA_DROP_CONTROL             (FM_LITERAL_U64(1) << 10)
#define FM10000_TRIGGER_HA_DROP_RESERVED_MAC        (FM_LITERAL_U64(1) << 11)
#define FM10000_TRIGGER_HA_DROP_TAG                 (FM_LITERAL_U64(1) << 12)
#define FM10000_TRIGGER_HA_DROP_INVALID_SMAC        (FM_LITERAL_U64(1) << 13)
#define FM10000_TRIGGER_HA_DROP_PORT_SV             (FM_LITERAL_U64(1) << 14)
#define FM10000_TRIGGER_HA_TRAP_CPU                 (FM_LITERAL_U64(1) << 15)
#define FM10000_TRIGGER_HA_DROP_VLAN_IV             (FM_LITERAL_U64(1) << 16)
#define FM10000_TRIGGER_HA_DROP_STP_INL             (FM_LITERAL_U64(1) << 17)
#define FM10000_TRIGGER_HA_DROP_STP_IL              (FM_LITERAL_U64(1) << 18)
#define FM10000_TRIGGER_HA_DROP_FFU                 (FM_LITERAL_U64(1) << 19)
#define FM10000_TRIGGER_HA_TRAP_FFU                 (FM_LITERAL_U64(1) << 20)
#define FM10000_TRIGGER_HA_TRAP_ICMP_TTL            (FM_LITERAL_U64(1) << 21)
#define FM10000_TRIGGER_HA_TRAP_IP_OPTION           (FM_LITERAL_U64(1) << 22)
#define FM10000_TRIGGER_HA_TRAP_MTU                 (FM_LITERAL_U64(1) << 23)
#define FM10000_TRIGGER_HA_TRAP_IGMP                (FM_LITERAL_U64(1) << 24)
#define FM10000_TRIGGER_HA_TRAP_TTL                 (FM_LITERAL_U64(1) << 25)
#define FM10000_TRIGGER_HA_DROP_TTL                 (FM_LITERAL_U64(1) << 26)
#define FM10000_TRIGGER_HA_DROP_DLF                 (FM_LITERAL_U64(1) << 27)
#define FM10000_TRIGGER_HA_DROP_GLORT_CAM_MISS      (FM_LITERAL_U64(1) << 28)
#define FM10000_TRIGGER_HA_DROP_NULL_DEST           (FM_LITERAL_U64(1) << 29)
#define FM10000_TRIGGER_HA_DROP_VLAN_EV             (FM_LITERAL_U64(1) << 30)
#define FM10000_TRIGGER_HA_DROP_POLICER             (FM_LITERAL_U64(1) << 31)
#define FM10000_TRIGGER_HA_DROP_STP_E               (FM_LITERAL_U64(1) << 32)
#define FM10000_TRIGGER_HA_DROP_LOOPBACK            (FM_LITERAL_U64(1) << 33)
#define FM10000_TRIGGER_HA_FORWARD_DGLORT           (FM_LITERAL_U64(1) << 34)
#define FM10000_TRIGGER_HA_FORWARD_FLOOD            (FM_LITERAL_U64(1) << 35)
#define FM10000_TRIGGER_HA_SWITCH_RESERVED_MAC      (FM_LITERAL_U64(1) << 36)
#define FM10000_TRIGGER_HA_FORWARD_FID              (FM_LITERAL_U64(1) << 37)
#define FM10000_TRIGGER_HA_LOG_FFU_I                (FM_LITERAL_U64(1) << 38)
#define FM10000_TRIGGER_HA_LOG_RESERVED_MAC         (FM_LITERAL_U64(1) << 39)
#define FM10000_TRIGGER_HA_LOG_ARP_REDIRECT         (FM_LITERAL_U64(1) << 40)
#define FM10000_TRIGGER_HA_LOG_MCST_ICMP_TTL        (FM_LITERAL_U64(1) << 41)
#define FM10000_TRIGGER_HA_LOG_IP_MCST_TTL          (FM_LITERAL_U64(1) << 42)
#define FM10000_TRIGGER_HA_PARSE_TIMEOUT            (FM_LITERAL_U64(1) << 43)
#define FM10000_TRIGGER_HA_MIRROR_FFU               (FM_LITERAL_U64(1) << 44)

/* HW defines for action values */
#define FM10000_TRIGGER_FORWARDING_ACTION_ASIS            0
#define FM10000_TRIGGER_FORWARDING_ACTION_FORWARD         1
#define FM10000_TRIGGER_FORWARDING_ACTION_REDIRECT        2
#define FM10000_TRIGGER_FORWARDING_ACTION_DROP            3

#define FM10000_TRIGGER_TRAP_ACTION_ASIS                  0
#define FM10000_TRIGGER_TRAP_ACTION_TRAP                  1
#define FM10000_TRIGGER_TRAP_ACTION_LOG                   2
#define FM10000_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG     3

#define FM10000_TRIGGER_MIRROR_ACTION_NONE                0
#define FM10000_TRIGGER_MIRROR_ACTION_MIRROR              1
                                                     
#define FM10000_TRIGGER_SWPRI_ACTION_ASIS                 0
#define FM10000_TRIGGER_SWPRI_ACTION_REASSIGN             1

#define FM10000_TRIGGER_VLAN_ACTION_ASIS                  0
#define FM10000_TRIGGER_VLAN_ACTION_REASSIGN              1

#define FM10000_TRIGGER_LEARN_ACTION_ASIS                 0
#define FM10000_TRIGGER_LEARN_ACTION_CANCEL               1

#define FM10000_TRIGGER_RATELIMIT_ACTION_ASIS             0
#define FM10000_TRIGGER_RATELIMIT_ACTION_RATELIMIT        1


/**************************************************
 * Mirror profile configuration.
 **************************************************/
typedef struct _fm10000_mirrorCfg
{
    /* Physical destination port for mirrored or logged copies. */
    fm_int      physPort;

    /* New DGLORT to use for mirrored or logged copies. */
    fm_uint32   glort;

    /* New VID to use for mirrored or logged copies. */
    fm_uint16   vlan;

    /* New VPRI to use for mirrored or logged copies. */
    fm_byte     vlanPri;

    /* Whether mirrored copies should be truncated. */
    fm_bool     truncate;

} fm10000_mirrorCfg;


/**************************************************
 * Mirror profile entry type.
 **************************************************/
enum _fm_mirrorProfileType
{
    FM10000_MIRROR_PROFILE_NONE,
    FM10000_MIRROR_PROFILE_INTRINSIC,
    FM10000_MIRROR_PROFILE_MIRROR,
    FM10000_MIRROR_PROFILE_LOG,
};


/**************************************************
 * Intrinsic mirror type.
 **************************************************/
enum _fm_intrinsicMirrorType
{
    FM_INTRINSIC_MIRROR_NONE,
    FM_INTRINSIC_MIRROR_FFU,
    FM_INTRINSIC_MIRROR_RESERVED_MAC,
    FM_INTRINSIC_MIRROR_ARP_REDIRECT,
    FM_INTRINSIC_MIRROR_ICMP,
    FM_INTRINSIC_MIRROR_TTL,
};


/**************************************************
 * Mirror profile entry.
 **************************************************/
typedef struct _fm10000_mirrorProfile
{
    /* Mirror profile entry type. */
    fm_int      entryType;

    /* The index where this profile is located in hardware, or -1. */
    fm_int      index;

    /* Intrinsic mirror type if this is an intrinsic profile. */
    fm_int      intrinsicType;

} fm10000_mirrorProfile;


/**************************************************
 * Structure that holds a trigger entry.
 **************************************************/
typedef struct _fm10000_triggerEntry
{
    /* The index where this trigger is located in HW */
    fm_uint32  index;

    /* The matchByPrecedence bit from FM10000_TRIGGER_CONDITION_CFG */
    fm_int  matchByPrec;

    /* A pointer to the condition as it was last set by fmSetTriggerCondition.
     * The pointer points to a copy of the condition and not the original
     * condition as passed by the caller. */
    fm_triggerCondition *cond;

    /* A pointer to the action as it was last set by fmSetTriggerAction.
     * The pointer points to a copy of the action and not the original
     * action as passed by the caller. */
    fm_triggerAction *action;

    /* If a trigger entry moves in HW, we must keep track of the counter
     * value. In a trigger movement from src to dst, we invalidate src once
     * src has been copied to dst. For every move, this parameter is
     * incremented with the src's entry counter value. By doing this a trigger
     * movement is non-disruptive. */
    fm_uint64 counterCache;

    /* Indicates whether this trigger has been created for internal use.
     * Internal triggers may not be modified/deleted by the public API. */
    fm_bool isInternal;

    /* Indicates whether the hardware mirror profile entry needs to be
     * copied along with the trigger. Will be true for triggers that
     * have a log action profile. May be true for triggers that have
     * a mirror action profile. */
    fm_bool isBound;

    /* Name of the trigger for diagnostics */
    fm_text name;

    /* The mirror profile index to be written to the hardware. */
    fm_uint32   mirrorIndex;

    /* The mirror profile handle for the logging action associated with
     * this trigger, or -1. */
    fm_int      logProfile;

} fm10000_triggerEntry;


/**************************************************
 * Structure to track trigger information state.
 **************************************************/
typedef struct _fm10000_triggerInfo
{
    /* The number of used trigger entries in HW */
    fm_int numUsedTriggers;
    
    /******************************************************************* 
     * Trigger resources.
     ******************************************************************/ 

    /* Tree to track triggers. The tree key is a concatenation of the group and
     * the rule number.
     * Key[63:32] = Group 
     * Key[31:0]  = Rule 
     * The tree value is a pointer to fm10000_triggerEntry structure */ 
    fm_tree triggerTree;

    /* Bit array to track usage of the MAC trig ID values */
    fm_bitArray usedMacTrigID;

    /* Bit array to track the isInternal state of a MAC trig ID resource */
    fm_bitArray macTrigIdInternal;

    /* Bit array to track usage of the vlan trig ID values */
    fm_bitArray usedVlanTrigID;

    /* Bit array to track the isInternal state of a vlan trig ID resource */
    fm_bitArray vlanTrigIdInternal;

    /* Bit array to track usage of the FFU trig ID bits */
    fm_bitArray usedFFUTrigIDBits;

    /* Bit array to track the isInternal state of a FFU trig ID resource */
    fm_bitArray FFUTrigIdInternal;

    /* Bit array to track usage of the rate-limiter ID values */
    fm_bitArray usedRateLimiterID;

    /* Bit array to track the isInternal state of a rate limiter resource */
    fm_bitArray rateLimiterIdInternal;

    /* Array to cache rate limiter portset configuration */
    fm_int rateLimPortSet[FM10000_TRIGGER_RATE_LIM_CFG_2_ENTRIES];
    
    /******************************************************************* 
     * Mirror profile resources.
     ******************************************************************/ 

    /* Bit array to track allocation of the profile handles. */
    fm_bitArray usedProfileHandle;

    /* Bit array to track the isInternal state of a profile handle resource. */
    fm_bitArray profileHandleInternal;

    /* Bit array to track usage of the mirror profile index resource. */
    fm_bitArray usedProfileIndex;

    /* Array of mirror profile entries, indexed by handle. */
    fm10000_mirrorProfile profileEntry[FM10000_NUM_MIRROR_PROFILES];

    /* Array of mirror profile handles, indexed by hardware mirror profile
     * identifier. Map entry will be -1 if the corresponding hardware mirror
     * profile is not in use. */
    fm_int  profileMap[FM10000_NUM_MIRROR_PROFILES];

} fm10000_triggerInfo;


/**************************************************
 * Trigger detail structure. 
 * Returned by fm10000DbgGetTriggerDetail.
 **************************************************/
typedef struct _fm_triggerDetail
{
    /* The index where this trigger is located in hardware. */
    fm_int      triggerIndex;

    /* The mirror profile index to be written to the hardware. */
    fm_uint32   mirrorIndex;

    /* The mirror profile handle for the logging action associated with
     * this trigger, or -1. */
    fm_int      logProfile;

} fm_triggerDetail;


/**************************************************
 * Mirror profile detail structure. 
 * Returned by fm10000DbgGetMirrorProfileDetail.
 **************************************************/
typedef struct _fm_mirrorProfileDetail
{
    /* The index where this profile is located in hardware. */
    fm_int      profileIndex;

    /* Mirror profile entry type.
     * See _fm_mirrorProfileType for values. */
    fm_int      entryType;

    /* Intrinsic mirror type if this is an intrinsic profile.
     * See _fm_intrinsicMirrorType for values. */
    fm_int      intrinsicType;

    /* Physical destination port for mirrored or logged copies. */
    fm_int      physPort;

    /* New DGLORT to use for mirrored or logged copies. */
    fm_uint32   glort;

} fm_mirrorProfileDetail;



/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/

/**************************************************
 * fm10000_api_trigger.c
 **************************************************/

fm_status fm10000InitTriggers(fm_switch *switchPtr);

fm_status fm10000DestroyTriggers(fm_switch *switchPtr);

fm_status fm10000CreateTrigger(fm_int  sw, 
                               fm_int  group, 
                               fm_int  rule, 
                               fm_bool isInternal, 
                               fm_text name);

fm_status fm10000DeleteTrigger(fm_int  sw, 
                               fm_int  group, 
                               fm_int  rule, 
                               fm_bool isInternal);

fm_status fm10000SetTriggerCondition(fm_int sw, 
                                     fm_int group, 
                                     fm_int rule, 
                                     const fm_triggerCondition *cond,
                                     fm_bool isInternal);

fm_status fm10000SetTriggerAction(fm_int sw, 
                                  fm_int group, 
                                  fm_int rule, 
                                  const fm_triggerAction *action,
                                  fm_bool isInternal);

fm_status fm10000GetTrigger(fm_int               sw, 
                            fm_int               group, 
                            fm_int               rule, 
                            fm_triggerCondition *cond,
                            fm_triggerAction *   action);

fm_status fm10000GetTriggerFirst(fm_int sw, fm_int *group, fm_int *rule);

fm_status fm10000GetTriggerNext(fm_int sw, 
                                fm_int curGroup, 
                                fm_int curRule,
                                fm_int *nextGroup, 
                                fm_int *nextRule);

fm_status fm10000AllocateTriggerResource(fm_int sw, 
                                         fm_triggerResourceType res, 
                                         fm_uint32 *value,
                                         fm_bool isInternal);

fm_status fm10000FreeTriggerResource(fm_int sw, 
                                     fm_triggerResourceType res, 
                                     fm_uint32 value,
                                     fm_bool isInternal);

fm_status fm10000GetTriggerResourceFirst(fm_int sw, 
                                         fm_triggerResourceType res, 
                                         fm_uint32 *value);

fm_status fm10000GetTriggerResourceNext(fm_int sw, 
                                        fm_triggerResourceType res, 
                                        fm_uint32 curValue,
                                        fm_uint32 *nextValue);

fm_status fm10000SetTriggerRateLimiter(fm_int sw,
                                       fm_int rateLimiterId,
                                       fm_rateLimiterCfg *cfg,
                                       fm_bool isInternal);

fm_status fm10000GetTriggerRateLimiter(fm_int sw,
                                       fm_int rateLimiterId,
                                       fm_rateLimiterCfg *cfg);

fm_status fm10000SetTriggerAttribute(fm_int sw, 
                                     fm_int group, 
                                     fm_int rule,
                                     fm_int attr, 
                                     void *value,
                                     fm_bool isInternal);

fm_status fm10000GetTriggerAttribute(fm_int sw, 
                                     fm_int group, 
                                     fm_int rule,
                                     fm_int attr, 
                                     void *value);

fm_status fm10000GetTriggerFromTrapCode(fm_int  sw,
                                        fm_int  trapCode,
                                        fm_int *group,
                                        fm_int *rule);

fm_status fm10000DbgDumpTriggers(fm_int sw);

/**************************************************
 * fm10000_api_mprofile.c
 **************************************************/

fm_status fm10000ConvertMirrorIndexToProfile(fm_int      sw,
                                             fm_uint32   mirrorIndex,
                                             fm_uint32 * mirrorProfile);

fm_status fm10000ConvertMirrorProfileToIndex(fm_int      sw,
                                             fm_uint32   mirrorProfile,
                                             fm_uint32 * mirrorIndex);

fm_status fm10000CopyMirrorProfile(fm_int sw,
                                   fm_int srcIndex,
                                   fm_int dstIndex);

fm_status fm10000CreateMirrorProfile(fm_int      sw,
                                     fm_uint32 * handle,
                                     fm_bool     isInternal);

fm_status fm10000DeleteLogProfile(fm_int sw, fm_uint32 handle);

fm_status fm10000DeleteMirrorProfile(fm_int    sw,
                                     fm_uint32 handle,
                                     fm_bool   isInternal);

fm_status fm10000FreeProfileIndex(fm_int sw, fm_uint32 index);

fm_status fm10000InitMirrorProfiles(fm_int sw);

fm_status fm10000ReleaseMirrorProfile(fm_int sw, fm_uint32 index);

fm_status fm10000ReserveMirrorProfile(fm_int sw, fm_uint32 index);

fm_status fm10000SetMirrorProfileAction(fm_int  sw,
                                        fm_int  group,
                                        fm_int  rule,
                                        fm10000_triggerEntry * trigEntry);

fm_status fm10000UpdateLogProfile(fm_int    sw,
                                  fm_uint32 handle,
                                  fm_uint32 logIndex);

fm_status fm10000GetMirrorProfileConfig(fm_int              sw,
                                        fm_uint32           handle,
                                        fm10000_mirrorCfg * config);

fm_status fm10000SetMirrorProfileConfig(fm_int              sw,
                                        fm_uint32           handle,
                                        fm10000_mirrorCfg * config);

/**************************************************
 * Debug functions.
 **************************************************/

fm_status fm10000DbgGetMirrorProfileDetail(fm_int                   sw,
                                           fm_uint32                handle,
                                           fm_mirrorProfileDetail * detail);

fm_status fm10000DbgGetTriggerDetail(fm_int             sw,
                                     fm_int             group,
                                     fm_int             rule,
                                     fm_triggerDetail * detail);

fm_status fm10000DbgDumpMirrorProfile(fm_int sw);

fm_status fmDbgMoveMirrorProfile(fm_int     sw,
                                 fm_uint32  handle,
                                 fm_int     dstIndex);

#endif /* __FM_FM10000_API_TRIGGER_INT_H */

