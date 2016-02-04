/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_nat_int.h
 * Creation Date:  March 13, 2014
 * Description:    FM10000 NAT API.
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

#ifndef __FM_FM10000_FM_API_NAT_INT_H
#define __FM_FM10000_FM_API_NAT_INT_H

/** Base ACL Position for NAT Table */
#define FM10000_NAT_BASE_ACL         22222222

typedef struct _fm_fm10000NatTunnel
{
    /**  Encap Flow Id used to represent the tunnel */
    fm_int  encapFlow;

} fm_fm10000NatTunnel;


typedef struct _fm_fm10000NatRule
{
    /**  ACL Rule Id used that refer to this specific rule. Might be -1 if
     *   the NAT mode used is FM_NAT_MODE_RESOURCE. */
    fm_int  aclRule;

    /**  Tunnel Rule Id used that refer to this specific rule. */
    fm_int  tunnelRule;

} fm_fm10000NatRule;


typedef struct _fm_fm10000NatPrefilter
{
    /**  ACL Rule Id used that refer to this specific rule. */
    fm_int  aclRule;

} fm_fm10000NatPrefilter;


typedef struct _fm_fm10000NatRuleCond
{
    /**  Rule Condition mask (SHOULD NOT BE UPDATED IF NODE IN TREE) */
    fm_natCondition       condition;

    /**  Rule Condition param (SHOULD NOT BE UPDATED IF NODE IN TREE) */
    fm_natConditionParam  cndParam;

    /**  ECMP Group if more than 1 rule share the same condition. -1 indicate
     *   that no ECMP Group are created. */
    fm_int                ecmpGrp;

    /**  Nat Rule if only one entry refer to this condition. */
    fm_int                rule;

} fm_fm10000NatRuleCond;


typedef struct _fm_fm10000NatPrefix
{
    /**  Key is aclRule Id
     *   Value is a fm_tree* type that list all natRule Id that belongs to this
     *   acl rule. */
    fm_tree aclRule;

} fm_fm10000NatPrefix;


typedef struct _fm_fm10000NatTable
{
    /**  ACL Id used to forward frame to the TE */
    fm_int  acl;

    /**  Tunnel Group Id used to translate frames */
    fm_int  tunnelGrp;

    /**  Key is tunnel id
     *   Value is a fm_fm10000NatTunnel* type. */
    fm_tree tunnels;

    /**  Key is rule id
     *   Value is a fm_fm10000NatRule* type. */
    fm_tree rules;

    /**  Key is prefilter entry
     *   Value is a fm_fm10000NatPrefilter* type. */
    fm_tree prefilters;

    /**  Key is of type fm_fm10000NatRuleCond
     *   Value is a fm_fm10000NatRuleCond* type.
     *
     *   This is used to automatically build ECMP Group if multiple rules
     *   have the same condition but different action. This is only handled if
     *   the NAT mode used is FM_NAT_MODE_PERFORMANCE. */
    fm_customTree ruleCondition;

    /**  Indicates which ACL rule are currently used or free. */
    fm_bitArray   ruleInUse;

    /**  Key is (Dip Prefix | Sip Prefix)
     *   Value is a fm_fm10000NatPrefix* type.
     *
     *   This is only used for non full mask entries. */
    fm_tree prefixs;

    /**  Indicate the last aclRule currently used for full mask matching. */
    fm_int  prefixLimit;


} fm_fm10000NatTable;


typedef struct _fm_fm10000NatCfg
{
    /**  Key is a NAT Table
     *   Value is a fm_fm10000NatTable* type. */
    fm_tree             tables;


} fm_fm10000NatCfg;

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

fm_status fm10000NatInit(fm_int sw);
fm_status fm10000NatFree(fm_int sw);
fm_status fm10000CreateNatTable(fm_int sw, fm_int table, fm_natParam *natParam);
fm_status fm10000DeleteNatTable(fm_int sw, fm_int table);
fm_status fm10000SetNatTunnelDefault(fm_int               sw,
                                     fm_natTunnelDefault *tunnelDefault);
fm_status fm10000CreateNatTunnel(fm_int        sw,
                                 fm_int        table,
                                 fm_int        tunnel,
                                 fm_natTunnel *param);
fm_status fm10000DeleteNatTunnel(fm_int sw, fm_int table, fm_int tunnel);
fm_status fm10000AddNatRule(fm_int                sw,
                            fm_int                table,
                            fm_int                rule,
                            fm_natCondition       condition,
                            fm_natConditionParam *cndParam,
                            fm_natAction          action,
                            fm_natActionParam *   actParam);
fm_status fm10000DeleteNatRule(fm_int sw, fm_int table, fm_int rule);
fm_status fm10000AddNatPrefilter(fm_int                sw,
                                 fm_int                table,
                                 fm_int                entry,
                                 fm_natCondition       condition,
                                 fm_natConditionParam *cndParam);
fm_status fm10000DeleteNatPrefilter(fm_int sw, fm_int table, fm_int entry);
fm_status fm10000GetNatRuleCount(fm_int             sw,
                                 fm_int             table,
                                 fm_int             rule,
                                 fm_tunnelCounters *counters);
fm_status fm10000ResetNatRuleCount(fm_int sw, fm_int table, fm_int rule);


#endif  /* __FM_FM10000_FM_API_NAT_INT_H */

