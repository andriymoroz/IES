/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_nat_int.h
 * Creation Date:   June 2, 2014
 * Description:     Contains functions dealing with switch-aggregate nat.
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

#ifndef __FM_FMSWAG_API_NAT_INT_H
#define __FM_FMSWAG_API_NAT_INT_H


fm_status fmSWAGCreateNatTable(fm_int sw, fm_int table, fm_natParam *natParam);
fm_status fmSWAGDeleteNatTable(fm_int sw, fm_int table);
fm_status fmSWAGSetNatTunnelDefault(fm_int               sw,
                                    fm_natTunnelDefault *tunnelDefault);
fm_status fmSWAGCreateNatTunnel(fm_int        sw,
                                fm_int        table,
                                fm_int        tunnel,
                                fm_natTunnel *param);
fm_status fmSWAGDeleteNatTunnel(fm_int sw, fm_int table, fm_int tunnel);
fm_status fmSWAGAddNatRule(fm_int                sw,
                           fm_int                table,
                           fm_int                rule,
                           fm_natCondition       condition,
                           fm_natConditionParam *cndParam,
                           fm_natAction          action,
                           fm_natActionParam *   actParam);
fm_status fmSWAGDeleteNatRule(fm_int sw, fm_int table, fm_int rule);
fm_status fmSWAGAddNatPrefilter(fm_int                sw,
                                fm_int                table,
                                fm_int                entry,
                                fm_natCondition       condition,
                                fm_natConditionParam *cndParam);
fm_status fmSWAGDeleteNatPrefilter(fm_int sw, fm_int table, fm_int entry);
fm_status fmSWAGGetNatRuleCount(fm_int             sw,
                                fm_int             table,
                                fm_int             rule,
                                fm_tunnelCounters *counters);
fm_status fmSWAGResetNatRuleCount(fm_int sw, fm_int table, fm_int rule);


#endif /* __FM_FMSWAG_API_NAT_INT_H */
