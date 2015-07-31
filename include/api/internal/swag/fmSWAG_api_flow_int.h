/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_flow_int.h
 * Creation Date:   June 2, 2014
 * Description:     SWAG OpenFlow API interface.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#ifndef __FM_FMSWAG_API_FLOW_INT_H
#define __FM_FMSWAG_API_FLOW_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/** The maximum size allowed for a TCAM flow table on SWAG device.
 *  \ingroup  constSystem */
#define FM_SWAG_MAX_RULE_PER_FLOW_TABLE 4096

/** The maximum size allowed for a TE flow table on SWAG device.
 *  \ingroup constSystem */
#define FM_SWAG_MAX_RULE_PER_FLOW_TE_TABLE 16384


/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/

fm_status fmSWAGCreateFlowTCAMTable(fm_int           sw, 
                                    fm_int           tableIndex, 
                                    fm_flowCondition condition,
                                    fm_uint32        maxEntries,
                                    fm_uint32        maxAction);

fm_status fmSWAGDeleteFlowTCAMTable(fm_int           sw, 
                                    fm_int           tableIndex);

fm_status fmSWAGCreateFlowTETable(fm_int           sw, 
                                  fm_int           tableIndex, 
                                  fm_flowCondition condition,
                                  fm_uint32        maxEntries,
                                  fm_uint32        maxAction);

fm_status fmSWAGDeleteFlowTETable(fm_int           sw, 
                                  fm_int           tableIndex);

fm_status fmSWAGAddFlow(fm_int           sw, 
                        fm_int           tableIndex,
                        fm_uint16        priority,
                        fm_uint32        precedence, 
                        fm_flowCondition condition,
                        fm_flowValue *   condVal,
                        fm_flowAction    action,
                        fm_flowParam *   param,
                        fm_flowState     flowState,
                        fm_int *         flowId);

fm_status fmSWAGGetFlow(fm_int             sw, 
                        fm_int             tableIndex,
                        fm_int             flowId,
                        fm_flowCondition * flowCond,
                        fm_flowValue *     flowValue,
                        fm_flowAction *    flowAction,
                        fm_flowParam *     flowParam,
                        fm_int *           priority,
                        fm_int *           precedence);

fm_status fmSWAGModifyFlow(fm_int           sw, 
                           fm_int           tableIndex,
                           fm_int           flowId,
                           fm_uint16        priority,
                           fm_uint32        precedence, 
                           fm_flowCondition condition,
                           fm_flowValue *   condVal,
                           fm_flowAction    action,
                           fm_flowParam *   param);

fm_status fmSWAGGetFlowTableType(fm_int             sw,
                                 fm_int             tableIndex,
                                 fm_flowTableType * flowTableType);

fm_status fmSWAGGetFlowFirst(fm_int   sw,
                             fm_int * firstTable);

fm_status fmSWAGGetFlowNext(fm_int   sw,
                            fm_int   currentTable,
                            fm_int * nextTable);

fm_status fmSWAGGetFlowRuleFirst(fm_int   sw,
                                 fm_int   tableIndex,
                                 fm_int * firstRule);

fm_status fmSWAGGetFlowRuleNext(fm_int   sw,
                                fm_int   tableIndex,
                                fm_int   currentRule,
                                fm_int * nextRule);

fm_status fmSWAGDeleteFlow(fm_int sw, fm_int tableIndex, fm_int flowId);

fm_status fmSWAGSetFlowState(fm_int       sw, 
                             fm_int       tableIndex, 
                             fm_int       flowId, 
                             fm_flowState flowState);

fm_status fmSWAGGetFlowCount(fm_int           sw, 
                             fm_int           tableIndex, 
                             fm_int           flowId,
                             fm_flowCounters *counters);

fm_status fmSWAGResetFlowCount(fm_int sw, 
                               fm_int tableIndex, 
                               fm_int flowId);

fm_status fmSWAGGetFlowUsed(fm_int   sw, 
                            fm_int   tableIndex, 
                            fm_int   flowId,
                            fm_bool  clear,
                            fm_bool *used);

fm_status fmSWAGSetFlowAttribute(fm_int sw,
                                 fm_int tableIndex,
                                 fm_int attr,
                                 void * value);

fm_status fmSWAGGetFlowAttribute(fm_int sw,
                                 fm_int tableIndex,
                                 fm_int attr,
                                 void * value);

fm_status fmSWAGFreeFlowResource(fm_int sw);

fm_status fmSWAGCreateFlowBalanceGrp(fm_int  sw,
                                     fm_int *groupId);

fm_status fmSWAGDeleteFlowBalanceGrp(fm_int sw,
                                     fm_int groupId);

fm_status fmSWAGAddFlowBalanceGrpEntry(fm_int sw,
                                       fm_int groupId,
                                       fm_int tableIndex,
                                       fm_int flowId);

fm_status fmSWAGDeleteFlowBalanceGrpEntry(fm_int sw,
                                          fm_int groupId,
                                          fm_int tableIndex,
                                          fm_int flowId);

#endif /* __FM_FMSWAG_API_FLOW_INT_H */
