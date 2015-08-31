/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_tunnel_int.h
 * Creation Date:  January 15, 2014
 * Description:    FM10000 Tunnel API.
 *
 * Copyright (c) 2014, Intel Corporation
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

#ifndef __FM_FM10000_FM_API_TUNNEL_INT_H
#define __FM_FM10000_FM_API_TUNNEL_INT_H


/* Defines which keys are shared between hash and search */
#define FM10000_TUNNEL_HASH_SEARCH_KEY   ( FM_TUNNEL_MATCH_VSI_TEP        | \
                                           FM_TUNNEL_MATCH_VNI            | \
                                           FM_TUNNEL_MATCH_DMAC           | \
                                           FM_TUNNEL_MATCH_SMAC           | \
                                           FM_TUNNEL_MATCH_VLAN           | \
                                           FM_TUNNEL_MATCH_DIP            | \
                                           FM_TUNNEL_MATCH_SIP            | \
                                           FM_TUNNEL_MATCH_L4SRC          | \
                                           FM_TUNNEL_MATCH_L4DST          | \
                                           FM_TUNNEL_MATCH_PROT    )

/* The number of bits in the HASH key generated for the lookup stage */
#define FM10000_TUNNEL_HASH_KEY_BITS           480

/* The number of bytes required to store the hask key */
#define FM10000_TUNNEL_HASH_KEY_BYTES         (FM10000_TUNNEL_HASH_KEY_BITS / 8)

/* Limit the number of teData entry per bin on hash lookup */
#define FM10000_TUNNEL_MAX_TE_DATA_BIN_SIZE     FM10000_TE_MAX_DATA_BIN_SIZE

/* Initial TeData swap size that also refer to the minimum value */
#define FM10000_TUNNEL_TE_DATA_MIN_SWAP_SIZE    20

/* That would be the trap code base used for TE0 which refer to 0x40..0x43 */
#define FM10000_TUNNEL_0_TRAP_CODE_BASE       0x40

/* That would be the trap code base used for TE1 which refer to 0x44..0x47 */
#define FM10000_TUNNEL_1_TRAP_CODE_BASE       0x44

/* Limit the usage of the TeData table to avoid situation where defrag would
 * be needed for every modification of the table. The current limit is 99% of
 * the table */
#define FM10000_TUNNEL_TE_DATA_MIN_FREE_SIZE   (FM10000_TE_DATA_ENTRIES_0 / 100)

typedef struct _fm_fm10000TunnelLookupBin
{
    /**  teData location of that bin */
    fm_fm10000TeLookup      teLookup;

    /**  Key is rule number
     *   Value is not used. */
    fm_tree                 rules;

} fm_fm10000TunnelLookupBin;


typedef struct _fm_fm10000TunnelRule
{
    /** Tunnel Condition (on hash lookup type) */
    fm_tunnelCondition      condition;

    /** Tunnel Condition Param (on hash lookup type) */
    fm_tunnelConditionParam condParam;

    /** Tunnel Action */
    fm_tunnelAction         action;

    /** Tunnel Action Param */
    fm_tunnelActionParam    actParam;

    /** Lookup bin used for that rule */
    fm_uint16               lookupBin;

    /** teData location of that specific rule */
    fm_uint16               dataPos;

    /** Counter location */
    fm_uint16               counter;

} fm_fm10000TunnelRule;


typedef struct _fm_fm10000EncapFlow
{
    /** Encap Flow Action */
    fm_tunnelEncapFlow      field;

    /** Encap Flow Action Param */
    fm_tunnelEncapFlowParam param;

    /**  teData location of that encap flow */
    fm_fm10000TeLookup      teLookup;

    /** Counter location */
    fm_uint16               counter;

    /**  Key is rule number
     *   Value is not used. */
    fm_tree                 rules;

} fm_fm10000EncapFlow;


typedef struct _fm_fm10000TunnelGrp
{
    /**  Group activate flag. */
    fm_bool             active;

    /**  Basic tunnel parameter as exported by top level api. */
    fm_tunnelParam      tunnelParam;

    /**  Low level tunnel parameter. */
    fm_fm10000TeDGlort  teDGlort;

    /**  Key is rule number
     *   Value is a fm_fm10000TunnelRule* type. */
    fm_tree             rules;

    /**  Key is an encap flow number
     *   Value is a fm_fm10000EncapFlow* type. */
    fm_tree             encapFlows;

    /**  Key is a tunnel lookup bin
     *   Value is a fm_fm10000TunnelLookupBin* type. */
    fm_tree             lookupBins;

    /**  Index of the first free tunnel lookup bin. This is to speed up the
     *   search of an available entry on direct mode. */
    fm_int              lookupBinFirstFreeEntry;

} fm_fm10000TunnelGrp;


typedef enum
{
    /** Entry refer to a bin number. */
    FM_FM10000_TUNNEL_TE_DATA_TYPE_BIN,

    /** Entry refer to an encap flow. */
    FM_FM10000_TUNNEL_TE_DATA_TYPE_ENCAPFLOW,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TUNNEL_TE_DATA_MAX

} fm_fm10000TunnelTeDataType;


/*****************************************************************************
 *
 *  TeData Block Control
 *
 *  This structure contains information that describes a single TeData block
 *
 *****************************************************************************/
typedef struct _fm_fm10000TunnelTeDataBlockCtrl
{
    /** index defines the block location */
    fm_uint16           index;

    /** length defines the block size */
    fm_uint16           length;

    /** tunnel group that refer to this teData Block. */
    fm_uint16           tunnelGrp;

    /** defines if the entry is an encap flow or a rule. */
    fm_fm10000TunnelTeDataType tunnelDataType;

    /** this could be either a bin or an encap flow. */
    fm_int              tunnelEntry;

} fm_fm10000TunnelTeDataBlockCtrl;


/*****************************************************************************
 *
 *  TeData Table Control
 *
 *  This structure contains information that describes and controls the
 *  TeData Table operation.
 *
 *****************************************************************************/
typedef struct _fm_fm10000TunnelTeDataCtrl
{
    /** teData handler used to map hardware index to block control. */
    fm_uint16              teDataHandler[FM10000_TE_DATA_ENTRIES_0];

    /** block control table that refer to dynamically allocated entry that
     *  specify the owner and chain property of any block. */
    fm_fm10000TunnelTeDataBlockCtrl *teDataBlkCtrl[FM10000_TE_DATA_ENTRIES_0];

    /** number of free entries in teDataHandler[] table */
    fm_int                 teDataFreeEntryCount;

    /** index of the first free entry in the teDataHandler[] table */
    fm_int                 teDataHandlerFirstFreeEntry;

    /** swap size currently defined located at the end of the teDataHandler[]
     *  table */
    fm_int                 teDataSwapSize;

    /** index of the last allocated position in the teDataBlkCtrl[] table */
    fm_uint                lastTeDataBlkCtrlIndex;

} fm_fm10000TunnelTeDataCtrl;


typedef struct _fm_fm10000TunnelCfg
{
    /**  Tunnel Group */
    fm_fm10000TunnelGrp        tunnelGrp[FM10000_TE_DGLORT_MAP_ENTRIES_1]
                                        [FM10000_TE_DGLORT_MAP_ENTRIES_0];

    /**  Reserved Counter Index */
    fm_bitArray                cntInUse[FM10000_TE_STATS_ENTRIES_1];

    /**  This is the structure used to keep track of the teData block */
    fm_fm10000TunnelTeDataCtrl teDataCtrl[FM10000_TE_DATA_ENTRIES_1];

    /**  Logical Port associated to each TE, -1 if none associated */
    fm_int                     tunnelPort[FM10000_TE_DGLORT_MAP_ENTRIES_1];

    /**  The tunnel engine's protocol mode */
    fm_teMode                  tunnelMode[FM10000_NUM_TE];

} fm_fm10000TunnelCfg;


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

fm_status fm10000TunnelInit(fm_int sw);
fm_status fm10000TunnelFree(fm_int sw);
fm_status fm10000CreateTunnel(fm_int          sw,
                              fm_int *        group,
                              fm_tunnelParam *tunnelParam);
fm_status fm10000DeleteTunnel(fm_int sw, fm_int group);

fm_status fm10000GetTunnel(fm_int          sw,
                           fm_int          group,
                           fm_tunnelParam *tunnelParam);
fm_status fm10000GetTunnelFirst(fm_int sw, fm_int *firstGroup);
fm_status fm10000GetTunnelNext(fm_int  sw,
                               fm_int  currentGroup,
                               fm_int *nextGroup);

fm_status fm10000AddTunnelEncapFlow(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int                   encapFlow,
                                    fm_tunnelEncapFlow       field,
                                    fm_tunnelEncapFlowParam *param);
fm_status fm10000DeleteTunnelEncapFlow(fm_int sw,
                                       fm_int group,
                                       fm_int encapFlow);
fm_status fm10000UpdateTunnelEncapFlow(fm_int                   sw,
                                       fm_int                   group,
                                       fm_int                   encapFlow,
                                       fm_tunnelEncapFlow       field,
                                       fm_tunnelEncapFlowParam *param);

fm_status fm10000GetTunnelEncapFlow(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int                   encapFlow,
                                    fm_tunnelEncapFlow *     field,
                                    fm_tunnelEncapFlowParam *param);
fm_status fm10000GetTunnelEncapFlowFirst(fm_int                   sw,
                                         fm_int                   group,
                                         fm_int *                 firstEncapFlow,
                                         fm_tunnelEncapFlow *     field,
                                         fm_tunnelEncapFlowParam *param);
fm_status fm10000GetTunnelEncapFlowNext(fm_int                   sw,
                                        fm_int                   group,
                                        fm_int                   currentEncapFlow,
                                        fm_int *                 nextEncapFlow,
                                        fm_tunnelEncapFlow *     field,
                                        fm_tunnelEncapFlowParam *param);

fm_status fm10000AddTunnelRule(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   rule,
                               fm_tunnelCondition       cond,
                               fm_tunnelConditionParam *condParam,
                               fm_tunnelAction          action,
                               fm_tunnelActionParam *   actParam);
fm_status fm10000DeleteTunnelRule(fm_int sw, fm_int group, fm_int rule);
fm_status fm10000UpdateTunnelRule(fm_int                   sw,
                                  fm_int                   group,
                                  fm_int                   rule,
                                  fm_tunnelCondition       cond,
                                  fm_tunnelConditionParam *condParam,
                                  fm_tunnelAction          action,
                                  fm_tunnelActionParam *   actParam);

fm_status fm10000GetTunnelRule(fm_int                   sw,
                               fm_int                   group,
                               fm_int                   rule,
                               fm_tunnelCondition *     cond,
                               fm_tunnelConditionParam *condParam,
                               fm_tunnelAction *        action,
                               fm_tunnelActionParam *   actParam);
fm_status fm10000GetTunnelRuleFirst(fm_int                   sw,
                                    fm_int                   group,
                                    fm_int *                 firstRule,
                                    fm_tunnelCondition *     cond,
                                    fm_tunnelConditionParam *condParam,
                                    fm_tunnelAction *        action,
                                    fm_tunnelActionParam *   actParam);
fm_status fm10000GetTunnelRuleNext(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int                   currentRule,
                                   fm_int *                 nextRule,
                                   fm_tunnelCondition *     cond,
                                   fm_tunnelConditionParam *condParam,
                                   fm_tunnelAction *        action,
                                   fm_tunnelActionParam *   actParam);

fm_status fm10000GetTunnelRuleCount(fm_int             sw,
                                    fm_int             group,
                                    fm_int             rule,
                                    fm_tunnelCounters *counters);
fm_status fm10000GetTunnelEncapFlowCount(fm_int             sw,
                                         fm_int             group,
                                         fm_int             encapFlow,
                                         fm_tunnelCounters *counters);
fm_status fm10000GetTunnelRuleUsed(fm_int   sw,
                                   fm_int   group,
                                   fm_int   rule,
                                   fm_bool *used);

fm_status fm10000ResetTunnelRuleCount(fm_int sw, fm_int group, fm_int rule);
fm_status fm10000ResetTunnelEncapFlowCount(fm_int sw,
                                           fm_int group,
                                           fm_int encapFlow);
fm_status fm10000ResetTunnelRuleUsed(fm_int sw, fm_int group, fm_int rule);

fm_status fm10000SetTunnelAttribute(fm_int sw,
                                    fm_int group,
                                    fm_int rule,
                                    fm_int attr,
                                    void * value);
fm_status fm10000GetTunnelAttribute(fm_int sw,
                                    fm_int group,
                                    fm_int rule,
                                    fm_int attr,
                                    void * value);

fm_status fm10000SetTunnelApiAttribute(fm_int sw,
                                       fm_int attr,
                                       void * value);
fm_status fm10000GetTunnelApiAttribute(fm_int sw,
                                       fm_int attr,
                                       void * value);

fm_status fm10000DbgDumpTunnel(fm_int sw);

 
#endif  /* __FM_FM10000_FM_API_TUNNEL_INT_H */

