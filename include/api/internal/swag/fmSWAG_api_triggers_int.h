/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_triggers_int.h
 * Creation Date:   June 18, 2008
 * Description:     Contains functions dealing with switch-aggregate triggers.
 *
 * Copyright (c) 2008 - 2014, Intel Corporation
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

#ifndef __FM_FMSWAG_API_TRIGGERS_INT_H
#define __FM_FMSWAG_API_TRIGGERS_INT_H


fm_status fmSWAGTriggerInit(fm_int sw);
fm_status fmSWAGTriggerFree(fm_int sw);
fm_status fmSWAGCreateTrigger(fm_int  sw, 
                              fm_int  group, 
                              fm_int  rule, 
                              fm_bool isInternal,
                              fm_text name);
fm_status fmSWAGDeleteTrigger(fm_int  sw, 
                              fm_int  group, 
                              fm_int  rule, 
                              fm_bool isInternal);
fm_status fmSWAGSetTriggerCondition(fm_int                     sw, 
                                    fm_int                     group, 
                                    fm_int                     rule, 
                                    const fm_triggerCondition *cond,
                                    fm_bool                    isInternal);
fm_status fmSWAGSetTriggerAction(fm_int                  sw, 
                                 fm_int                  group, 
                                 fm_int                  rule, 
                                 const fm_triggerAction *action,
                                 fm_bool                 isInternal);
fm_status fmSWAGGetTrigger(fm_int               sw, 
                           fm_int               group, 
                           fm_int               rule, 
                           fm_triggerCondition *cond,
                           fm_triggerAction *   action);
fm_status fmSWAGGetTriggerFirst(fm_int sw, fm_int *group, fm_int *rule);
fm_status fmSWAGGetTriggerNext(fm_int sw, 
                               fm_int curGroup, 
                               fm_int curRule,
                               fm_int *nextGroup, 
                               fm_int *nextRule);
fm_status fmSWAGAllocateTriggerResource(fm_int                 sw, 
                                        fm_triggerResourceType res, 
                                        fm_uint32 *            value,
                                        fm_bool                isInternal);
fm_status fmSWAGFreeTriggerResource(fm_int                 sw, 
                                    fm_triggerResourceType res, 
                                    fm_uint32              value,
                                    fm_bool                isInternal);
fm_status fmSWAGGetTriggerResourceFirst(fm_int                 sw, 
                                        fm_triggerResourceType res, 
                                        fm_uint32 *            value);
fm_status fmSWAGGetTriggerResourceNext(fm_int                 sw, 
                                       fm_triggerResourceType res, 
                                       fm_uint32              curValue,
                                       fm_uint32 *            nextValue);
fm_status fmSWAGSetTriggerRateLimiter(fm_int             sw,
                                      fm_int             rateLimiterId,
                                      fm_rateLimiterCfg *cfg,
                                      fm_bool            isInternal);
fm_status fmSWAGGetTriggerRateLimiter(fm_int             sw,
                                      fm_int             rateLimiterId,
                                      fm_rateLimiterCfg *cfg);
fm_status fmSWAGSetTriggerAttribute(fm_int  sw, 
                                    fm_int  group, 
                                    fm_int  rule,
                                    fm_int  attr, 
                                    void *  value,
                                    fm_bool isInternal);
fm_status fmSWAGGetTriggerAttribute(fm_int sw, 
                                    fm_int group, 
                                    fm_int rule,
                                    fm_int attr, 
                                    void * value);

fm_status fmSWAGAllocateTrigger(fm_int                 sw,
                                fm_text                name,
                                fm_int *               triggerID,
                                fm_triggerRequestInfo *info);

fm_status fmSWAGFreeTrigger(fm_int sw, fm_int triggerID);


#endif /* __FM_FMSWAG_API_TRIGGERS_INT_H */
