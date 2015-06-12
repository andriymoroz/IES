/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_acl_int.h
 * Creation Date:  June 14, 2013
 * Description:    Internal ACL data structures for FM10000.
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#ifndef __FM_FM10000_API_ACL_INT_H
#define __FM_FM10000_API_ACL_INT_H

//Abstract value that can only be part of the low 32 bit position
#define FM10000_ABSTRACT_NONE                           0
#define FM10000_ABSTRACT_DIP_7_0                        1
#define FM10000_ABSTRACT_DIP_15_8                       2
#define FM10000_ABSTRACT_DIP_23_16                      3
#define FM10000_ABSTRACT_DIP_31_24                      4
#define FM10000_ABSTRACT_DIP_39_32                      5
#define FM10000_ABSTRACT_DIP_47_40                      6
#define FM10000_ABSTRACT_DIP_55_48                      7
#define FM10000_ABSTRACT_DIP_63_56                      8
#define FM10000_ABSTRACT_DIP_71_64                      9
#define FM10000_ABSTRACT_DIP_79_72                      10
#define FM10000_ABSTRACT_DIP_87_80                      11
#define FM10000_ABSTRACT_DIP_95_88                      12
#define FM10000_ABSTRACT_DIP_103_96                     13
#define FM10000_ABSTRACT_DIP_111_104                    14
#define FM10000_ABSTRACT_DIP_119_112                    15
#define FM10000_ABSTRACT_DIP_127_120                    16
#define FM10000_ABSTRACT_SIP_7_0                        17
#define FM10000_ABSTRACT_SIP_15_8                       18
#define FM10000_ABSTRACT_SIP_23_16                      19
#define FM10000_ABSTRACT_SIP_31_24                      20
#define FM10000_ABSTRACT_SIP_39_32                      21
#define FM10000_ABSTRACT_SIP_47_40                      22
#define FM10000_ABSTRACT_SIP_55_48                      23
#define FM10000_ABSTRACT_SIP_63_56                      24
#define FM10000_ABSTRACT_SIP_71_64                      25
#define FM10000_ABSTRACT_SIP_79_72                      26
#define FM10000_ABSTRACT_SIP_87_80                      27
#define FM10000_ABSTRACT_SIP_95_88                      28
#define FM10000_ABSTRACT_SIP_103_96                     29
#define FM10000_ABSTRACT_SIP_111_104                    30
#define FM10000_ABSTRACT_SIP_119_112                    31
#define FM10000_ABSTRACT_SIP_127_120                    32
#define FM10000_ABSTRACT_DMAC_7_0                       33
#define FM10000_ABSTRACT_DMAC_15_8                      34
#define FM10000_ABSTRACT_DMAC_23_16                     35
#define FM10000_ABSTRACT_DMAC_31_24                     36
#define FM10000_ABSTRACT_DMAC_39_32                     37
#define FM10000_ABSTRACT_DMAC_47_40                     38
#define FM10000_ABSTRACT_SMAC_7_0                       39
#define FM10000_ABSTRACT_SMAC_15_8                      40
#define FM10000_ABSTRACT_SMAC_23_16                     41
#define FM10000_ABSTRACT_SMAC_31_24                     42
#define FM10000_ABSTRACT_SMAC_39_32                     43
#define FM10000_ABSTRACT_SMAC_47_40                     44
#define FM10000_ABSTRACT_DGLORT_7_0                     45
#define FM10000_ABSTRACT_DGLORT_15_8                    46
#define FM10000_ABSTRACT_SGLORT_7_0                     47
#define FM10000_ABSTRACT_SGLORT_15_8                    48
#define FM10000_ABSTRACT_VLAN_VPRI1_7_0                 49
#define FM10000_ABSTRACT_VLAN_VPRI1_15_8                50
#define FM10000_ABSTRACT_VLAN_VPRI2_7_0                 51
#define FM10000_ABSTRACT_VLAN_VPRI2_15_8                52
#define FM10000_ABSTRACT_TYPE_7_0                       53
#define FM10000_ABSTRACT_TYPE_15_8                      54
#define FM10000_ABSTRACT_L4DST_7_0                      55
#define FM10000_ABSTRACT_L4DST_15_8                     56
#define FM10000_ABSTRACT_L4SRC_7_0                      57
#define FM10000_ABSTRACT_L4SRC_15_8                     58
#define FM10000_ABSTRACT_MAP_L4DST_7_0                  59
#define FM10000_ABSTRACT_MAP_L4DST_15_8                 60
#define FM10000_ABSTRACT_MAP_L4SRC_7_0                  61
#define FM10000_ABSTRACT_MAP_L4SRC_15_8                 62
#define FM10000_ABSTRACT_L4A_7_0                        63
#define FM10000_ABSTRACT_L4A_15_8                       64
#define FM10000_ABSTRACT_L4B_7_0                        65
#define FM10000_ABSTRACT_L4B_15_8                       66
#define FM10000_ABSTRACT_L4C_7_0                        67
#define FM10000_ABSTRACT_L4C_15_8                       68
#define FM10000_ABSTRACT_L4D_7_0                        69
#define FM10000_ABSTRACT_L4D_15_8                       70
#define FM10000_ABSTRACT_MAP_VLAN_VPRI1_7_0             71
#define FM10000_ABSTRACT_MAP_VLAN_VPRI1_15_8            72

//Abstract value that can be configured in any position
//These key can also be splitted in 4-bit block and merged
//with case scenario under some condition
#define FM10000_ABSTRACT_MAP_DIP                        73
#define FM10000_ABSTRACT_MAP_SIP                        74
#define FM10000_ABSTRACT_MAP_DMAC                       75
#define FM10000_ABSTRACT_MAP_SMAC                       76
#define FM10000_ABSTRACT_MAP_PROT                       77
#define FM10000_ABSTRACT_MAP_LENGTH                     78
#define FM10000_ABSTRACT_MAP_SRC                        79
#define FM10000_ABSTRACT_MAP_TYPE                       80
#define FM10000_ABSTRACT_USER_3_0                       81
#define FM10000_ABSTRACT_USER_7_4                       82
#define FM10000_ABSTRACT_FTYPE                          83
#define FM10000_ABSTRACT_SWPRI                          84
#define FM10000_ABSTRACT_IPMISC_3_0                     85
#define FM10000_ABSTRACT_IPMISC_7_4                     86
#define FM10000_ABSTRACT_TOS_3_0                        87
#define FM10000_ABSTRACT_TOS_7_4                        88
#define FM10000_ABSTRACT_PROT_3_0                       89
#define FM10000_ABSTRACT_PROT_7_4                       90
#define FM10000_ABSTRACT_TTL_3_0                        91
#define FM10000_ABSTRACT_TTL_7_4                        92
#define FM10000_ABSTRACT_SRC_PORT_3_0                   93
#define FM10000_ABSTRACT_SRC_PORT_7_4                   94
#define FM10000_ABSTRACT_VID_3_0                        95
#define FM10000_ABSTRACT_VID_7_4                        96
#define FM10000_ABSTRACT_VID_11_8                       97
#define FM10000_ABSTRACT_VPRI                           98
#define FM10000_ABSTRACT_RXTAG_3_0                      99
#define FM10000_ABSTRACT_RXTAG_7_4                      100

#define FM10000_NUM_ABSTRACT                            101

#define FM10000_UNUSED_KEY                              254
#define FM10000_INVALID_KEY                             255

#define FM10000_4BITS_ABSTRACT_KEY                      4
#define FM10000_8BITS_ABSTRACT_KEY                      8

#define FM10000_FIRST_4BITS_ABSTRACT_KEY                73

#define FM10000_SPECIAL_PORT_PER_ACL_KEY                0xfffaafff

#define FM10000_MAX_RULE_PER_ACL_PART                   1024

#define FM10000_4BITS_KEY_DIFFICULTY                    1
#define FM10000_8BITS_KEY_DIFFICULTY                    2

#define FM10000_ACL_PORTSET_PER_RULE_FIRST              0
#define FM10000_ACL_PORTSET_PER_RULE_NUM                4
#define FM10000_ACL_PORTSET_OWNER_POS                   4
#define FM10000_ACL_PORTSET_TYPE_GLOBAL                 1
#define FM10000_ACL_PORTSET_TYPE_POS                    32

#define FM10000_ACL_EGRESS_CHUNK_NUM                    32
#define FM10000_ACL_EGRESS_CHUNK_SIZE                   32

#define FM10000_ACL_EGRESS_SLICE_POS                    31

#define FM10000_ACL_POLICER_4K_BANK_NUM                 2
#define FM10000_ACL_POLICER_512_BANK_NUM                2

#define FM10000_ACL_MAX_ACTIONS_PER_RULE                5

#define FM10000_ACL_NUM_KEY_POS                         5
#define FM10000_ACL_NUM_KEY_MASK                        0xffffffffLL
#define FM10000_ACL_PART_KEY_POS                        0
#define FM10000_ACL_PART_KEY_MASK                       0x1fLL

#define FM10000_MAX_ACL_NON_IP_PAYLOAD_BYTES            32
#define FM10000_MAX_ACL_L4_DEEP_INSPECTION_BYTES        32

#define FM10000_ACL_TRIGGER_RULE_TRAP_ALWAYS             1

#define FM_ACL_GET_MASTER_KEY(aclId) (((fm_uint64)aclId << FM10000_ACL_NUM_KEY_POS) & ~FM10000_ACL_PART_KEY_MASK)

#define FM10000_ACL_SUP_COND        ( FM_ACL_MATCH_SRC_MAC |                   \
                                      FM_ACL_MATCH_DST_MAC |                   \
                                      FM_ACL_MATCH_ETHERTYPE |                 \
                                      FM_ACL_MATCH_VLAN |                      \
                                      FM_ACL_MATCH_PRIORITY |                  \
                                      FM_ACL_MATCH_VLAN2 |                     \
                                      FM_ACL_MATCH_PRIORITY2 |                 \
                                      FM_ACL_MATCH_SRC_IP |                    \
                                      FM_ACL_MATCH_DST_IP |                    \
                                      FM_ACL_MATCH_FLAGS |                     \
                                      FM_ACL_MATCH_TTL |                       \
                                      FM_ACL_MATCH_PROTOCOL |                  \
                                      FM_ACL_MATCH_DSCP |                      \
                                      FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK |     \
                                      FM_ACL_MATCH_L4_DST_PORT_WITH_MASK |     \
                                      FM_ACL_MATCH_TCP_FLAGS |                 \
                                      FM_ACL_MATCH_L4_DEEP_INSPECTION_EXT |    \
                                      FM_ACL_MATCH_SWITCH_PRIORITY |           \
                                      FM_ACL_MATCH_VLAN_TAG_TYPE |             \
                                      FM_ACL_MATCH_SOURCE_MAP |                \
                                      FM_ACL_MATCH_PROTOCOL_MAP |              \
                                      FM_ACL_MATCH_DST_MAC_MAP |               \
                                      FM_ACL_MATCH_SRC_MAC_MAP |               \
                                      FM_ACL_MATCH_ETH_TYPE_MAP |              \
                                      FM_ACL_MATCH_IP_LENGTH_MAP |             \
                                      FM_ACL_MATCH_DST_IP_MAP |                \
                                      FM_ACL_MATCH_SRC_IP_MAP |                \
                                      FM_ACL_MATCH_L4_SRC_PORT_MAP |           \
                                      FM_ACL_MATCH_L4_DST_PORT_MAP |           \
                                      FM_ACL_MATCH_VLAN_MAP |                  \
                                      FM_ACL_MATCH_NON_IP_PAYLOAD |            \
                                      FM_ACL_MATCH_TOS |                       \
                                      FM_ACL_MATCH_ISL_FTYPE |                 \
                                      FM_ACL_MATCH_ISL_USER |                  \
                                      FM_ACL_MATCH_SRC_GLORT |                 \
                                      FM_ACL_MATCH_DST_GLORT |                 \
                                      FM_ACL_MATCH_FRAG |                      \
                                      FM_ACL_MATCH_FRAME_TYPE |                \
                                      FM_ACL_MATCH_INGRESS_PORT_SET |          \
                                      FM_ACL_MATCH_SCENARIO_FLAGS |            \
                                      FM_ACL_MATCH_SRC_PORT )

typedef struct _fm_fm10000AclRule
{
    /**  ACL ID */
    fm_int     aclNumber;

    /**  Rule ID */
    fm_int     ruleNumber;

} fm_fm10000AclRule;


typedef struct _fm_fm10000CompiledPolicerEntry
{
    /**  Counter or Policer entry */
    fm_bool            countEntry;

    /**  Policer Id translated */
    fm_int             policerId;

    /**  CIR configuration for this entry */
    fm_ffuPolicerState committed;

    /**  EIR configuration for this entry */
    fm_ffuPolicerState excess;

    /**  List of ACL/Rule that currently uses this policer entry. The
     *   type of the elements are fm_fm10000AclRule* */
    fm_dlist           policerRules;

} fm_fm10000CompiledPolicerEntry;


typedef struct _fm_fm10000CompiledPolicers
{
    /**  Key ACL Id
     *   Value is a fm_int* type */
    fm_tree           acl;

    /**  Last Policer Index configured on that bank */
    fm_uint16         indexLastPolicer;

    /**  Color Source configured on that bank */
    fm_ffuColorSource ingressColorSource;

    /**  Is markDSCP configured on that bank? */
    fm_bool           markDSCP;

    /**  Is markSwitchPri configured on that bank? */
    fm_bool           markSwitchPri;

    /**  Key Policer Index
     *   Value is a fm_fm10000CompiledPolicerEntry* type. */
    fm_tree           policerEntry;

} fm_fm10000CompiledPolicers;


typedef struct _fm_fm10000CompiledAclRule
{
    /**  ACL ID */
    fm_int     aclNumber;

    /**  Rule ID */
    fm_int     ruleNumber;

    /**  Physical position of this rule ID */
    fm_int     physicalPos;

    /**  Key configuration for each slice starting at position 0 */
    fm_fm10000FfuSliceKey sliceKey[FM10000_FFU_SLICE_VALID_ENTRIES];

    /**  Number of action slices defined for this rule */
    fm_int     numActions;

    /**  Action slice configuration */
    fm_ffuAction actions[FM10000_ACL_MAX_ACTIONS_PER_RULE];

    /**  Only defined for Egress ACL and represent the egress ACL Drop action
     *   of this specific rule. */
    fm_bool egressDropActions;

    /**  A maximum of 2 policer/counter index can be configure. This 16 bits
     *   index represent the policer index of each bank. Value of 0 indicate
     *   no policer index for this bank. */
    fm_uint16  policerIndex[FM_FM10000_POLICER_BANK_MAX];

    /**  Value that must be added to the policerIndex counter that refer to this
     *   rule counter. This is used to cover non disruptive policerIndex
     *   movement (Packets) */
    fm_uint64  cntAdjustPkts;

    /**  Value that must be added to the policerIndex counter that refer to this
     *   rule counter. This is used to cover non disruptive policerIndex
     *   movement (Octets) */
    fm_uint64  cntAdjustOctets;

    /**  PortSetId of this specific rule, must be defined to
     *   FM_PORT_SET_ALL if no portSet are specific to this rule. */
    fm_int     portSetId;

    /**  Rule can hit or not */
    fm_bool    valid;

} fm_fm10000CompiledAclRule;


typedef struct _fm_fm10000CompiledAclInstance
{
    /**  Key ACL Id | ACL Part
     *   Value is a fm_fm10000CompiledAcl* type. */
    fm_tree   acl;

    /**  Defines if this instance currently shares FFU slice resources. When
     *   the total number of rules defined on all the ACLs that belongs to
     *   this instance exceed the maximum number of rules defined by a slice
     *   (1024 rules), this flag is unset and all the ACLs are defined on
     *   separate set of FFU slices. */
    fm_bool   mergedAcls;

    /**  Slice position and scenarios currently used by this instance. This
     *   is only valid when the mergedAcls flag is set. */
    fm_ffuSliceInfo sliceInfo;

    /**  Total number of rules from all the ACLs that belong to this
     *   instance. */
    fm_uint   numRules;

} fm_fm10000CompiledAclInstance;


typedef struct _fm_fm10000CompiledAcl
{
    /**  ACL Id */
    fm_int     aclNum;

    /**  ACL Parts in case of ACL that needs to extend on multiple sets */
    fm_int     aclParts;

    /**  first ACL Part that start the group of sets */
    fm_bool    firstAclPart;

    /**  Instance from which this ACL belongs to. This is only set when
     *   ACLs are shares mutually exclusive scenario on a single set of FFU
     *   slices. By default all ACLs are not linked to an instance
     *   (FM_ACL_NO_INSTANCE). */
    fm_int     aclInstance;

    /**  Key portSetType or portSet Id
     *   Value is a fm_portSet* type. */
    fm_tree   *portSetId;

    /**  Slice position, mux and scenario configured for this ACL */
    fm_ffuSliceInfo sliceInfo;

    /**  Slice key multiplexer selection for each slice defined in sliceInfo
     *   and starting at the position 0 */
    fm_byte    muxSelect[FM_FFU_SELECTS_PER_MINSLICE *
                         FM10000_FFU_SLICE_VALID_ENTRIES];

    /**  This array specify which mux are currently used by one or multiple
     *   rules. The granularity of each field is 4 bit so the 40 bit key is
     *   divided in 10 x 4-bit block. */
    fm_uint16  muxUsed[FM10000_FFU_SLICE_VALID_ENTRIES];

    /**  Case location for each slice defined in sliceInfo and starting at
     *   the position 0 */
    fm_ffuCaseLocation  caseLocation[FM10000_FFU_SLICE_VALID_ENTRIES];

    /**  Number of rules configured in this ACL */
    fm_uint    numRules;

    /**  Only defined for Egress ACL and represent the first set ID
     *   that define this egress ACL. This ID can be seen in the reverse
     *   direction so that if multiple chunk are merged together, the
     *   one carried with this variable is the last one. */
    fm_int     chunk;

    /**  Only defined for Egress ACL and represent the number of set needed
     *   for this egress ACL */
    fm_int     numChunk;

    /**  Key is rule number
     *   Value is a fm_fm10000CompiledAclRule* type. */
    fm_tree    rules;

    /**  The ACL configuration referred by FM_ACL_KEEP_UNUSED_KEYS */
    fm_bool    aclKeepUnusedKeys;

    /** is this ACL internal? */
    fm_bool    internal;

} fm_fm10000CompiledAcl;


typedef struct _fm_fm10000CompiledAcls
{
    /**  Key ACL Id | ACL Part
     *   Value is a fm_fm10000CompiledAcl* type. */
    fm_tree   ingressAcl;

    /**  Key ACL Id
     *   Value is a fm_fm10000CompiledAcl* type. */
    fm_tree   egressAcl;

    /**  Key portSetType | portSet Id
     *   Value is a fm_portSet* type. */
    fm_tree   portSetId;

    /**  Key Instance Id
     *   Value is a fm_fm10000CompiledAclInstance* type. */
    fm_tree   instance;

    /**  Policer Bank Configuration */
    fm_fm10000CompiledPolicers policers[FM_FM10000_POLICER_BANK_MAX];

    /**  Key policer Id
     *   Value is the policer index. */
    fm_tree   policersId[FM_FM10000_POLICER_BANK_MAX];

    /**  Bitmask indicating which portSet are actually used.
     *   SRC_MAP_OWNER | SRC_MAP(4 bits)
     *   SRC_MAP_OWNER == 0 for ACL and 1 for mapper. */
    fm_uint32 usedPortSet;

    /**  sliceValid contains one bit per slice indicating it has
     *   at least one valid TCAM rule. */
    fm_uint32 sliceValid;

    /**  actionValid contains one bit per slice indicating it has
     *   at least one valid action rule. */
    fm_uint32 actionValid;

    /**  prevSliceValid contains one bit per slice indicating it has
     *   at least one valid TCAM rule. This value is the last one processed
     *   by the fmUpdateMasterValid() function. */
    fm_uint32 prevSliceValid;

    /**  prevActionValid contains one bit per slice indicating it has
     *   at least one valid action rule. This value is the last one processed
     *   by the fmUpdateMasterValid() function. */
    fm_uint32 prevActionValid;

    /**  chunkValid contains one bit per chunk indicating it has
     *   at least one valid rule. */
    fm_uint32 chunkValid;

    /**  compiler stats */
    fm_aclCompilerStats compilerStats;

    /**  Key Ecmp Group Id
     *   Value is a fm_dlist* type. */
    fm_tree   ecmpGroups;

    /**  Key Typical mapper key
     *   Value is a fm_XyzMapperValue entry */
    fm_tree   mappers;

    /**  indicate if the compiled image is valid */
    fm_bool   valid;

} fm_fm10000CompiledAcls;


/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/* Function located in fm10000_api_acl.c */
void fm10000InitAclErrorReporter(fm_aclErrorReporter *   errReport,
                                 fm_text                 statusText,
                                 fm_int                  statusTextLength);

void fm10000FormatAclStatus(fm_aclErrorReporter *errReport,
                            fm_bool              isError,
                            const char *         format,
                            ...)
#ifdef __GNUC__
    __attribute__((format(printf, 3, 4)));
#else
    ;
#endif

fm_status fm10000ACLCompile(fm_int    sw,
                            fm_text   statusText,
                            fm_int    statusTextLength,
                            fm_uint32 flags,
                            void *    value);

fm_status fm10000ACLApplyExt(fm_int    sw,
                             fm_uint32 flags,
                             void *    value);

fm_status fm10000GetACLCountExt(fm_int          sw,
                                fm_int          acl,
                                fm_int          rule,
                                fm_aclCounters *counters);

fm_status fm10000ResetACLCount(fm_int sw,
                               fm_int acl,
                               fm_int rule);

fm_status fm10000GetACLEgressCount(fm_int          sw,
                                   fm_int          logicalPort,
                                   fm_aclCounters *counters);

fm_status fm10000ResetACLEgressCount(fm_int sw,
                                     fm_int logicalPort);

fm_status fm10000GetACLRuleAttribute(fm_int sw,
                                     fm_int acl,
                                     fm_int rule,
                                     fm_int attr,
                                     void*  value);

fm_status fm10000AclInit(fm_int sw);

fm_status fm10000AclFree(fm_int sw);

fm_status fm10000UpdateACLRule(fm_int sw,
                               fm_int acl,
                               fm_int rule);

fm_status fm10000SetACLRuleState(fm_int sw,
                                 fm_int acl,
                                 fm_int rule);

fm_status fm10000NotifyAclEcmpChange(fm_int    sw,
                                     fm_int    groupId,
                                     fm_int    oldIndex,
                                     fm_uint16 newIndex,
                                     fm_int    pathCount,
                                     fm_int    pathCountType);
fm_status fm10000ValidateAclTriggerId(fm_int   sw,
                                      fm_int   trigId,
                                      fm_bool *referenced);
fm_status fm10000ValidateAclArpBaseIndexId(fm_int    sw,
                                           fm_uint16 arpBlockIndex,
                                           fm_bool * referenced);
fm_status fm10000ValidateAclEcmpId(fm_int   sw,
                                   fm_int   groupId,
                                   fm_bool *referenced,
                                   fm_int  *acl,
                                   fm_int  *rule);
fm_status fm10000UpdateAclTriggerMask(fm_int sw,
                                      fm_int oldMask,
                                      fm_int newMask);
fm_int fm10000GetACLInUseSliceCount(fm_int sw);
fm_status fm10000AclProcessFFUPartitionChange(fm_int                        sw,
                                              const fm_ffuSliceAllocations *newAllocations,
                                              fm_bool                       simulated);

fm_status fm10000GetAclTrapAlwaysId(fm_int sw, fm_int *ffuResId);

/* Function located in fm10000_api_acl_mapper.c */
fm_status fm10000AddMapperEntry(fm_int             sw,
                                fm_mapper          mapper,
                                void *             value,
                                fm_mapperEntryMode mode);

fm_status fm10000DeleteMapperEntry(fm_int             sw,
                                   fm_mapper          mapper,
                                   void *             value,
                                   fm_mapperEntryMode mode);

fm_status fm10000ClearMapper(fm_int             sw,
                             fm_mapper          mapper,
                             fm_mapperEntryMode mode);

fm_status fm10000GetMapperSize(fm_int     sw,
                               fm_mapper  mapper,
                               fm_uint32 *mapperSize);

fm_status fm10000GetMapperL4PortKey(fm_int     sw,
                                    fm_mapper  mapper,
                                    fm_l4PortMapperValue *portMapValue,
                                    fm_uint64 *key);

fm_status fm10000VerifyMappers(fm_int               sw,
                               fm_aclErrorReporter *errReport,
                               fm_bool *            srcMap);

fm_status fm10000ApplyMappers(fm_int sw);

/* Function located in fm10000_api_acl_non_disrupt.c */
fm_status fm10000NonDisruptCompile(fm_int                  sw,
                                   fm_fm10000CompiledAcls *cacls,
                                   fm_int                  internalAcl,
                                   fm_bool                 apply);
fm_status fm10000NonDisruptQuickUpdate(fm_int sw, fm_int acl, fm_int rule);

/* Function located in fm10000_api_acl_policer.c */
fm_status fm10000ConvertPolicerAttributeToState(fm_policerConfig *  attributes,
                                                fm_ffuPolicerState *committed,
                                                fm_ffuPolicerState *excess);
fm_status fm10000PreallocatePolicerBank(fm_int                     sw,
                                        fm_aclErrorReporter *      errReport,
                                        fm_fm10000CompiledAcls *   cacls,
                                        fm_aclRule *               rule,
                                        fm_fm10000CompiledAclRule *compiledAclRule,
                                        fm_bool                    strictCount);
fm_status fm10000ConfigurePolicerBank(fm_int                     sw,
                                      fm_aclErrorReporter *      errReport,
                                      fm_fm10000CompiledAcls *   cacls,
                                      fm_aclRule *               rule,
                                      fm_fm10000CompiledAclRule *compiledAclRule,
                                      fm_bool                    strictCount);
fm_status fm10000ApplyPolicerCfg(fm_int                      sw,
                                 fm_fm10000CompiledPolicers *policers);
fm_status fm10000NonDisruptCleanPolicerRules(fm_int                     sw,
                                             fm_fm10000CompiledAcls *   cacls,
                                             fm_fm10000CompiledAclRule *compiledAclRule,
                                             fm_uint32                  bank,
                                             fm_bool                    apply);
fm_status fm10000NonDisruptCleanPolicers(fm_int                  sw,
                                         fm_fm10000CompiledAcls *cacls,
                                         fm_bool                 apply);
fm_status fm10000SetPolicerAttribute(fm_int      sw,
                                     fm_int      policer,
                                     fm_int      attr,
                                     const void *value);
fm_status fm10000ValidateACLAttribute(fm_int sw, fm_int attr);
fm_status fm10000GetAclFfuRuleUsage(fm_int  sw,
                                    fm_int *ffuRuleUsed,
                                    fm_int *ffuRuleAvailable);
fm_status fm10000ValidateAclLogicalPort(fm_int      sw,
                                        fm_int      logicalPort,
                                        fm_bool *   referenced);

#endif /* __FM_FM10000_API_ACL_INT_H */
