
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_flow_int.h
 * Creation Date:   March 07, 2014
 * Description:     Structures and functions for dealing with Flow API.
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

#ifndef __FM_FM_API_FLOW_INT_H
#define __FM_FM_API_FLOW_INT_H

/* Converts fm_flowParam to fm_aclParam. */
fm_status fmConvertFlowToACLParam(fm_flowParam   *flowParam,
                                  fm_aclParamExt *aclParam);

/* Converts fm_aclParamExt to fm_flowParam */
fm_status fmConvertACLToFlowParam(fm_aclParamExt *aclParam, 
                                  fm_flowParam   *flowParam);

/* Converts fm_flowValue to fm_aclValue. */
fm_status fmConvertFlowToACLValue(fm_flowValue   *flowValue,
                                  fm_aclValue    *aclValue);

/* Converts fm_aclValue to fm_flowValue */
fm_status fmConvertACLToFlowValue(fm_aclValue    *aclValue,
                                  fm_flowValue   *flowValue);

/* Converts fm_aclCounters to fm_flowCounters. */
fm_status fmConvertACLToFlowCounters(fm_aclCounters   *aclCounters,
                                     fm_flowCounters  *flowCounters);

/* Converts fm_flowParam to fm_tunnelActionParam and fm_tunnelEncapFlowParam. */
fm_status fmConvertFlowToTEParams(fm_flowParam            *flowParam,
                                  fm_tunnelActionParam    *tunnelParam,
                                  fm_tunnelEncapFlowParam *encapParam);

/* Converts fm_tunnelActionParam and fm_tunnelEncapFlowParam to fm_flowParam */
fm_status fmConvertTEParamsToFlow(fm_tunnelActionParam    *tunnelParam,
                                  fm_tunnelEncapFlowParam *encapParam,
                                  fm_flowParam            *flowParam);

/* Converts fm_flowValue to fm_tunnelConditionParam. */
fm_status fmConvertFlowToTEValue(fm_flowValue            *flowValue,
                                 fm_tunnelConditionParam *teValue);

/*Converts fm_tunnelConditionParam to fm_flowValue */
fm_status fmConvertTEToFlowValue(fm_tunnelConditionParam *teValue,
                                 fm_flowValue            *flowValue);

/* Converts fm_tunnelCounters to fm_flowCounters. */
fm_status fmConvertTEToFlowCounters(fm_tunnelCounters   *tunnelCounters,
                                    fm_flowCounters     *flowCounters);

fm_status fmGetFlowTableIndexUnused(fm_int  sw,
                                    fm_int *tableIndex);

fm_status fmGetFlowTableSupportedActions(fm_int           sw,
                                         fm_flowTableType flowTableType,
                                         fm_flowAction *  flowAction);

#endif /* __FM_FM_API_FLOW_INT_H */
