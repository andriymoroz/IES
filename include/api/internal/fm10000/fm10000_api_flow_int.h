/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_flow_int.h
 * Creation Date:   December 6, 2013
 * Description:     FM10000 OpenFlow API interface.
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

#ifndef __FM_FM10000_API_FLOW_INT_H
#define __FM_FM10000_API_FLOW_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/** The maximum size allowed for a TCAM flow table on FM10000 device.
 *  \ingroup  constSystem */
#define FM10000_MAX_RULE_PER_FLOW_TABLE 4096

/** The maximum size allowed for a TE flow table on FM10000 device.
 *  \ingroup constSystem */
#define FM10000_MAX_RULE_PER_FLOW_TE_TABLE 16384

/* The maximum number of mapped portsets */
#define FM10000_FLOW_PORTSET_NUM    4

/* Base ACL position for Flow tables. */
#define FM10000_FLOW_BASE_ACL       21000000

/* Bitmask defining which conditions are supported in TCAM table. */
#define FM10000_FLOW_SUPPORTED_TCAM_CONDITIONS \
    (FM_FLOW_MATCH_SRC_MAC            | \
     FM_FLOW_MATCH_DST_MAC            | \
     FM_FLOW_MATCH_ETHERTYPE          | \
     FM_FLOW_MATCH_VLAN               | \
     FM_FLOW_MATCH_VLAN_PRIORITY      | \
     FM_FLOW_MATCH_SRC_IP             | \
     FM_FLOW_MATCH_DST_IP             | \
     FM_FLOW_MATCH_PROTOCOL           | \
     FM_FLOW_MATCH_L4_SRC_PORT        | \
     FM_FLOW_MATCH_L4_DST_PORT        | \
     FM_FLOW_MATCH_INGRESS_PORT_SET   | \
     FM_FLOW_MATCH_TOS                | \
     FM_FLOW_MATCH_FRAME_TYPE         | \
     FM_FLOW_MATCH_SRC_PORT           | \
     FM_FLOW_MATCH_TCP_FLAGS          | \
     FM_FLOW_MATCH_L4_DEEP_INSPECTION | \
     FM_FLOW_MATCH_L2_DEEP_INSPECTION | \
     FM_FLOW_MATCH_SWITCH_PRIORITY    | \
     FM_FLOW_MATCH_VLAN_TAG_TYPE      | \
     FM_FLOW_MATCH_VLAN2              | \
     FM_FLOW_MATCH_PRIORITY2          | \
     FM_FLOW_MATCH_FRAG               | \
     FM_FLOW_MATCH_LOGICAL_PORT)

/* Bitmask defining which conditions are supported in TE table. */
#define FM10000_FLOW_SUPPORTED_TE_CONDITIONS \
    (FM_FLOW_MATCH_SRC_MAC            | \
     FM_FLOW_MATCH_DST_MAC            | \
     FM_FLOW_MATCH_VLAN               | \
     FM_FLOW_MATCH_SRC_IP             | \
     FM_FLOW_MATCH_DST_IP             | \
     FM_FLOW_MATCH_PROTOCOL           | \
     FM_FLOW_MATCH_L4_SRC_PORT        | \
     FM_FLOW_MATCH_L4_DST_PORT        | \
     FM_FLOW_MATCH_VNI                | \
     FM_FLOW_MATCH_VSI_TEP)

/* Bitmask defining which conditions are supported in BST table.
 * Value is 0 as BST is not supported for fm10000. */
#define FM10000_FLOW_SUPPORTED_BST_CONDITIONS 0

/* Bitmask defining which actions are supported in TCAM table. */
#define FM10000_FLOW_SUPPORTED_TCAM_ACTIONS    \
    (FM_FLOW_ACTION_FORWARD             | \
     FM_FLOW_ACTION_FORWARD_NORMAL      | \
     FM_FLOW_ACTION_TRAP                | \
     FM_FLOW_ACTION_DROP                | \
     FM_FLOW_ACTION_DEFAULT             | \
     FM_FLOW_ACTION_COUNT               | \
     FM_FLOW_ACTION_REDIRECT_TUNNEL     | \
     FM_FLOW_ACTION_BALANCE             | \
     FM_FLOW_ACTION_ROUTE               | \
     FM_FLOW_ACTION_PERMIT              | \
     FM_FLOW_ACTION_DENY                | \
     FM_FLOW_ACTION_SET_VLAN            | \
     FM_FLOW_ACTION_PUSH_VLAN           | \
     FM_FLOW_ACTION_POP_VLAN            | \
     FM_FLOW_ACTION_SET_VLAN_PRIORITY   | \
     FM_FLOW_ACTION_SET_SWITCH_PRIORITY | \
     FM_FLOW_ACTION_SET_DSCP            | \
     FM_FLOW_ACTION_LOAD_BALANCE        | \
     FM_FLOW_ACTION_SET_FLOOD_DEST      | \
     FM_FLOW_ACTION_MIRROR_GRP)

/* Bitmask defining which actions are supported in TE table. */
#define FM10000_FLOW_SUPPORTED_TE_ACTIONS \
    (FM_FLOW_ACTION_SET_DIP     | \
     FM_FLOW_ACTION_SET_SIP     | \
     FM_FLOW_ACTION_SET_L4DST   | \
     FM_FLOW_ACTION_SET_L4SRC   | \
     FM_FLOW_ACTION_SET_TTL     | \
     FM_FLOW_ACTION_SET_DMAC    | \
     FM_FLOW_ACTION_SET_SMAC    | \
     FM_FLOW_ACTION_ENCAP_VNI   | \
     FM_FLOW_ACTION_ENCAP_SIP   | \
     FM_FLOW_ACTION_ENCAP_TTL   | \
     FM_FLOW_ACTION_ENCAP_L4DST | \
     FM_FLOW_ACTION_ENCAP_L4SRC | \
     FM_FLOW_ACTION_ENCAP_NGE   | \
     FM_FLOW_ACTION_COUNT       | \
     FM_FLOW_ACTION_DECAP_KEEP  | \
     FM_FLOW_ACTION_DECAP_MOVE  | \
     FM_FLOW_ACTION_FORWARD)

/* Bitmask defining which actions are supported in BST table.
 * Value is 0 as BST is not supported for fm10000. */
#define FM10000_FLOW_SUPPORTED_BST_ACTIONS 0

/* Maximum number of actions that can be set for a flow in created 
 * TCAM table. */
#define FM10000_TCAM_TABLE_MAX_ACTIONS 3

/* Maximum number of actions that can be set for a flow in created 
 * TE table. */
#define FM10000_TE_TABLE_MAX_ACTIONS   1

/*  Structure that contains information related to the Flow API table. */
typedef struct _fm10000_flowTableInfo
{
    /* Indicates if the table is created. */
    fm_bool                created;

    /* Indicates if the table supports priority. */
    fm_bool                withPriority;

    /* Indicates if the table supports default action. */
    fm_bool                withDefault;

    /* Counters stored for flows in table. */
    fm_flowCounters *      lastCnt;

    /* Use bit for flows in table.*/
    fm_bool *              useBit;

    /* Mapping for flows in table. */
    fm_int *               mapping;

    /* This variable count the number of reference to this particular flow. */
    fm_int *               useCnt;

    /* Indicates if flow ID is in use. */
    fm_bitArray            idInUse;

    /* Table condition. */
    fm_flowCondition       condition;

    /* Table condition matching masks. */
    fm_flowValue           condMasks;

    /* Indicates if the table supports counting. */
    fm_bool                countSupported;

    /* Table type. */
    fm_flowTableType       type;

    /* Tunnel Engine group. */
    fm_int                 group;

    /* Tunnel Engine. */
    fm_int                 te;

    /* Tunnel Engine encap/decap. */
    fm_bool                encap;

    /* ACL scenario bitmask. */
    fm_uint32              scenario;

    /* Preallocated FFU action slices. */
    fm_int                 preallocatedSlices;

    /* Maximum number of actions that can be set for a flow in a table. */
    fm_uint32              maxAction;

} fm10000_flowTableInfo;

/*  Structure that contains information related to the Flow API. */
typedef struct _fm10000_flowInfo
{
    /* Structure that holds Flow API tables information. */
    fm10000_flowTableInfo  table[FM_FLOW_MAX_TABLE_TYPE];

    /* Is Flow API initialized? */
    fm_bool                initialized;

    /* Default logical port. */
    fm_int                 defaultLogicalPort;

    /* Forward to CPU logical port. */
    fm_int                 fwdToCpuLogicalPort;

    /* Trap logical port. */
    fm_int                 trapLogicalPort;

    /* Drop logical port. */
    fm_int                 dropLogicalPort;

    /* Forward normal logical port. */
    fm_int                 fwdNormalLogicalPort;

    /* Port set mapping. */
    fm_int                 portSetMap[FM10000_FLOW_PORTSET_NUM];

    /* Port set mapping counter. */
    fm_int                 portSetCnt[FM10000_FLOW_PORTSET_NUM];

    /* Indicates if the ECMP balance group is used by Flow API */
    fm_bitArray            balanceGrpInUse;

    /* Indicates if Deep Inspection Profile is initialized for
      '' FM_FLOW_MATCH_TCP_FLAGS'' usage. */
    fm_bool                deepInspectionProfileInitialized;

} fm10000_flowInfo;


/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/

fm_status fm10000CreateFlowTCAMTable(fm_int           sw,
                                     fm_int           tableIndex,
                                     fm_flowCondition condition,
                                     fm_uint32        maxEntries,
                                     fm_uint32        maxAction);

fm_status fm10000DeleteFlowTCAMTable(fm_int           sw,
                                     fm_int           tableIndex);

fm_status fm10000CreateFlowTETable(fm_int           sw,
                                   fm_int           tableIndex,
                                   fm_flowCondition condition,
                                   fm_uint32        maxEntries,
                                   fm_uint32        maxAction);

fm_status fm10000DeleteFlowTETable(fm_int           sw,
                                   fm_int           tableIndex);

fm_status fm10000AddFlow(fm_int           sw,
                         fm_int           tableIndex,
                         fm_uint16        priority,
                         fm_uint32        precedence,
                         fm_flowCondition condition,
                         fm_flowValue *   condVal,
                         fm_flowAction    action,
                         fm_flowParam *   param,
                         fm_flowState     flowState,
                         fm_int *         flowId);

fm_status fm10000GetFlow(fm_int             sw,
                         fm_int             tableIndex,
                         fm_int             flowId,
                         fm_flowCondition * flowCond,
                         fm_flowValue *     flowValue,
                         fm_flowAction *    flowAction,
                         fm_flowParam *     flowParam,
                         fm_int *           priority,
                         fm_int *           precedence);

fm_status fm10000ModifyFlow(fm_int           sw,
                            fm_int           tableIndex,
                            fm_int           flowId,
                            fm_uint16        priority,
                            fm_uint32        precedence,
                            fm_flowCondition condition,
                            fm_flowValue *   condVal,
                            fm_flowAction    action,
                            fm_flowParam *   param);

fm_status fm10000GetFlowTableType(fm_int             sw,
                                  fm_int             tableIndex,
                                  fm_flowTableType * flowTableType);

fm_status fm10000GetFlowFirst(fm_int   sw,
                              fm_int * firstTable);

fm_status fm10000GetFlowNext(fm_int   sw,
                             fm_int   currentTable,
                             fm_int * nextTable);

fm_status fm10000GetFlowRuleFirst(fm_int   sw,
                                  fm_int   tableIndex,
                                  fm_int * firstRule);

fm_status fm10000GetFlowRuleNext(fm_int   sw,
                                 fm_int   tableIndex,
                                 fm_int   currentRule,
                                 fm_int * nextRule);

fm_status fm10000DeleteFlow(fm_int sw, fm_int tableIndex, fm_int flowId);

fm_status fm10000SetFlowState(fm_int       sw,
                              fm_int       tableIndex,
                              fm_int       flowId,
                              fm_flowState flowState);

fm_status fm10000GetFlowCount(fm_int           sw,
                              fm_int           tableIndex,
                              fm_int           flowId,
                              fm_flowCounters *counters);

fm_status fm10000ResetFlowCount(fm_int sw,
                                fm_int tableIndex,
                                fm_int flowId);

fm_status fm10000GetFlowUsed(fm_int   sw,
                             fm_int   tableIndex,
                             fm_int   flowId,
                             fm_bool  clear,
                             fm_bool *used);

fm_status fm10000SetFlowAttribute(fm_int sw,
                                  fm_int tableIndex,
                                  fm_int attr,
                                  void * value);

fm_status fm10000GetFlowAttribute(fm_int sw,
                                  fm_int tableIndex,
                                  fm_int attr,
                                  void * value);

fm_status fm10000FreeFlowResource(fm_int sw);

fm_status fm10000InitFlowApiForSWAG(fm_int sw,
                                    fm_int defaultLogicalPort,
                                    fm_int trapLogicalPort,
                                    fm_int fwdToCpuLogicalPort);

fm_status fm10000CreateFlowBalanceGrp(fm_int  sw,
                                      fm_int *groupId);

fm_status fm10000DeleteFlowBalanceGrp(fm_int sw,
                                      fm_int groupId);

fm_status fm10000AddFlowBalanceGrpEntry(fm_int sw,
                                        fm_int groupId,
                                        fm_int tableIndex,
                                        fm_int flowId);

fm_status fm10000DeleteFlowBalanceGrpEntry(fm_int sw,
                                           fm_int groupId,
                                           fm_int tableIndex,
                                           fm_int flowId);

fm_status fm10000AddFlowUser(fm_int sw,
                             fm_int tableIndex,
                             fm_int flowId);

fm_status fm10000DelFlowUser(fm_int sw,
                             fm_int tableIndex,
                             fm_int flowId);

fm_status fm10000GetFlowTableIndexUnused(fm_int  sw,
                                         fm_int *tableIndex);

fm_status fm10000GetFlowTableSupportedActions(fm_int           sw,
                                              fm_flowTableType flowTableType,
                                              fm_flowAction *  flowAction);

#endif /* __FM_FM10000_API_FLOW_INT_H */
