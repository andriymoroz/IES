/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_acl_int.h
 * Creation Date:   June 12, 2008
 * Description:     Contains functions dealing with switch-aggregate ACLs.
 *
 * Copyright (c) 2008 - 2011, Intel Corporation
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

#ifndef __FM_FMSWAG_API_ACL_INT_H
#define __FM_FMSWAG_API_ACL_INT_H


fm_status fmSWAGCreateACL(fm_int    sw,
                          fm_int    acl,
                          fm_uint32 scenarios,
                          fm_int    precedence);
fm_status fmSWAGDeleteACL(fm_int sw, fm_int acl);
fm_status fmSWAGValidateACLAttribute(fm_int sw, fm_int attr);
fm_status fmSWAGSetACLAttribute(fm_int sw,
                                fm_int acl,
                                fm_int attr,
                                void * value);
fm_status fmSWAGAddACLRule(fm_int                sw,
                           fm_int                acl,
                           fm_int                rule,
                           fm_aclCondition       cond,
                           const fm_aclValue *   value,
                           fm_aclActionExt       action,
                           const fm_aclParamExt *param);
fm_status fmSWAGDeleteACLRule(fm_int sw,
                              fm_int acl,
                              fm_int rule);
fm_status fmSWAGUpdateACLRule(fm_int sw,
                              fm_int acl,
                              fm_int rule);
fm_status fmSWAGSetACLRuleState(fm_int sw,
                                fm_int acl,
                                fm_int rule);
fm_status fmSWAGAddACLPort(fm_int     sw,
                           fm_int     acl,
                           fm_int     port,
                           fm_aclType type);
fm_status fmSWAGDeleteACLPort(fm_int                   sw,
                              fm_int                   acl,
                              const fm_aclPortAndType *portAndType);
fm_status fmSWAGClearACLPort(fm_int sw, fm_int acl);
fm_status fmSWAGACLCompile(fm_int    sw,
                           fm_text   statusText,
                           fm_int    statusTextLength,
                           fm_uint32 flags,
                           void *    value);
fm_status fmSWAGACLApplyExt(fm_int    sw,
                            fm_uint32 flags,
                            void *    value);
fm_status fmSWAGGetACLCountExt(fm_int          sw,
                               fm_int          acl,
                               fm_int          rule,
                               fm_aclCounters *counters);
fm_status fmSWAGResetACLCount(fm_int sw,
                              fm_int acl,
                              fm_int rule);
fm_status fmSWAGGetACLEgressCount(fm_int          sw,
                                  fm_int          logicalPort,
                                  fm_aclCounters *counters);
fm_status fmSWAGResetACLEgressCount(fm_int sw,
                                    fm_int logicalPort);
fm_status fmSWAGInitACLs(fmSWAG_switch *extension);
fm_status fmSWAGDestroyACLs(fmSWAG_switch *extension);
fm_status fmSWAGAddMapperEntry(fm_int             sw,
                               fm_mapper          mapper,
                               void *             value,
                               fm_mapperEntryMode mode);
fm_status fmSWAGDeleteMapperEntry(fm_int             sw,
                                  fm_mapper          mapper,
                                  void *             value,
                                  fm_mapperEntryMode mode);
fm_status fmSWAGClearMapper(fm_int             sw,
                            fm_mapper          mapper,
                            fm_mapperEntryMode mode);
fm_status fmSWAGGetMapperSize(fm_int     sw,
                              fm_mapper  mapper,
                              fm_uint32 *mapperSize);
fm_status fmSWAGGetMapperL4PortKey(fm_int     sw,
                                   fm_mapper  mapper,
                                   fm_l4PortMapperValue *portMapValue,
                                   fm_uint64 *key);


#endif /* __FM_FMSWAG_API_ACL_INT_H */
