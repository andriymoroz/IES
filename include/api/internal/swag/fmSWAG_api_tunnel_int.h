/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_tunnel_int.h
 * Creation Date:   May 27, 2014
 * Description:     Contains functions dealing with switch-aggregate tunnel.
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

#ifndef __FM_FMSWAG_API_TUNNEL_INT_H
#define __FM_FMSWAG_API_TUNNEL_INT_H


fm_status fmSWAGCreateTunnel(fm_int          sw,
                             fm_int *        group,
                             fm_tunnelParam *tunnelParam);
fm_status fmSWAGDeleteTunnel(fm_int sw, fm_int group);
fm_status fmSWAGGetTunnel(fm_int          sw,
                          fm_int          group,
                          fm_tunnelParam *tunnelParam);
fm_status fmSWAGGetTunnelFirst(fm_int sw, fm_int *firstGroup);
fm_status fmSWAGGetTunnelNext(fm_int  sw,
                              fm_int  currentGroup,
                              fm_int *nextGroup);
fm_status fmSWAGAddTunnelEncapFlow(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int                   encapFlow,
                                   fm_tunnelEncapFlow       field,
                                   fm_tunnelEncapFlowParam *param);
fm_status fmSWAGDeleteTunnelEncapFlow(fm_int sw,
                                      fm_int group,
                                      fm_int encapFlow);
fm_status fmSWAGUpdateTunnelEncapFlow(fm_int                   sw,
                                      fm_int                   group,
                                      fm_int                   encapFlow,
                                      fm_tunnelEncapFlow       field,
                                      fm_tunnelEncapFlowParam *param);
fm_status fmSWAGGetTunnelEncapFlow(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int                   encapFlow,
                                   fm_tunnelEncapFlow *     field,
                                   fm_tunnelEncapFlowParam *param);
fm_status fmSWAGGetTunnelEncapFlowFirst(fm_int                   sw,
                                        fm_int                   group,
                                        fm_int *                 firstEncapFlow,
                                        fm_tunnelEncapFlow *     field,
                                        fm_tunnelEncapFlowParam *param);
fm_status fmSWAGGetTunnelEncapFlowNext(fm_int                   sw,
                                       fm_int                   group,
                                       fm_int                   currentEncapFlow,
                                       fm_int *                 nextEncapFlow,
                                       fm_tunnelEncapFlow *     field,
                                       fm_tunnelEncapFlowParam *param);
fm_status fmSWAGAddTunnelRule(fm_int                   sw,
                              fm_int                   group,
                              fm_int                   rule,
                              fm_tunnelCondition       cond,
                              fm_tunnelConditionParam *condParam,
                              fm_tunnelAction          action,
                              fm_tunnelActionParam *   actParam);
fm_status fmSWAGDeleteTunnelRule(fm_int sw, fm_int group, fm_int rule);
fm_status fmSWAGUpdateTunnelRule(fm_int                   sw,
                                 fm_int                   group,
                                 fm_int                   rule,
                                 fm_tunnelCondition       cond,
                                 fm_tunnelConditionParam *condParam,
                                 fm_tunnelAction          action,
                                 fm_tunnelActionParam *   actParam);
fm_status fmSWAGGetTunnelRule(fm_int                   sw,
                              fm_int                   group,
                              fm_int                   rule,
                              fm_tunnelCondition *     cond,
                              fm_tunnelConditionParam *condParam,
                              fm_tunnelAction *        action,
                              fm_tunnelActionParam *   actParam);
fm_status fmSWAGGetTunnelRuleFirst(fm_int                   sw,
                                   fm_int                   group,
                                   fm_int *                 firstRule,
                                   fm_tunnelCondition *     cond,
                                   fm_tunnelConditionParam *condParam,
                                   fm_tunnelAction *        action,
                                   fm_tunnelActionParam *   actParam);
fm_status fmSWAGGetTunnelRuleNext(fm_int                   sw,
                                  fm_int                   group,
                                  fm_int                   currentRule,
                                  fm_int *                 nextRule,
                                  fm_tunnelCondition *     cond,
                                  fm_tunnelConditionParam *condParam,
                                  fm_tunnelAction *        action,
                                  fm_tunnelActionParam *   actParam);
fm_status fmSWAGGetTunnelRuleCount(fm_int             sw,
                                   fm_int             group,
                                   fm_int             rule,
                                   fm_tunnelCounters *counters);
fm_status fmSWAGGetTunnelEncapFlowCount(fm_int             sw,
                                        fm_int             group,
                                        fm_int             encapFlow,
                                        fm_tunnelCounters *counters);
fm_status fmSWAGGetTunnelRuleUsed(fm_int  sw,
                                  fm_int  group,
                                  fm_int  rule,
                                  fm_bool *used);
fm_status fmSWAGResetTunnelRuleCount(fm_int sw, fm_int group, fm_int rule);
fm_status fmSWAGResetTunnelEncapFlowCount(fm_int sw,
                                          fm_int group,
                                          fm_int encapFlow);
fm_status fmSWAGResetTunnelRuleUsed(fm_int sw, fm_int group, fm_int rule);
fm_status fmSWAGSetTunnelAttribute(fm_int sw,
                                   fm_int group,
                                   fm_int rule,
                                   fm_int attr,
                                   void * value);
fm_status fmSWAGGetTunnelAttribute(fm_int sw,
                                   fm_int group,
                                   fm_int rule,
                                   fm_int attr,
                                   void * value);
fm_status fmSWAGDbgDumpTunnel(fm_int sw);


#endif /* __FM_FMSWAG_API_TUNNEL_INT_H */
