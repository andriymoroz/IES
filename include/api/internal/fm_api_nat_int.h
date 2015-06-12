/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_nat_int.h
 * Creation Date:   March 13, 2014
 * Description:     NAT API
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

#ifndef __FM_FM_API_NAT_INT_H
#define __FM_FM_API_NAT_INT_H


typedef struct _fm_natPrefilter
{
    /**  Prefilter Condition mask */
    fm_natCondition       condition;

    /**  Prefilter Condition param */
    fm_natConditionParam  cndParam;

} fm_natPrefilter;


typedef struct _fm_natRule
{
    /**  Rule Condition mask */
    fm_natCondition       condition;

    /**  Rule Condition param */
    fm_natConditionParam  cndParam;

    /**  Rule Action mask */
    fm_natAction          action;

    /**  Rule Action param */
    fm_natActionParam     actParam;

} fm_natRule;


typedef struct _fm_natTable
{
    /**  Table configuration */
    fm_natParam natParam;

    /**  Key is tunnel id
     *   Value is a fm_natTunnel* type. */
    fm_tree tunnels;

    /**  Key is rule id
     *   Value is a fm_natRule* type. */
    fm_tree rules;

    /**  Key is prefilter entry
     *   Value is a fm_natPrefilter* type. */
    fm_tree prefilters;

} fm_natTable;


typedef struct _fm_natInfo
{
    /**  Key is table id
     *   Value is a fm_natTable* type. */
    fm_tree tables;

    /**  Default tunnel configuration */
    fm_natTunnelDefault tunnelDefault;

} fm_natInfo;

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

fm_status fmNatInit(fm_int sw);
fm_status fmNatFree(fm_int sw);


#endif /* __FM_FM_API_NAT_INT_H */
