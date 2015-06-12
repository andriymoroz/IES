
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_stacking_int.h
 * Creation Date:   November 12, 2013
 * Description:     Internal prototypes for managing stacked intra and extra
 *                  switch aggregate systems on FM10000 devices.
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

#ifndef __FM_FM10000_API_STACKING_INT_H
#define __FM_FM10000_API_STACKING_INT_H


/***************************************************
 * The forwarding rule extension for FM10000 devices
 * including aspects related to the glort CAM.
 **************************************************/
typedef struct _fm10000_forwardRuleInternal
{
    /* The CAM entry backing this rule */
    fm_glortCamEntry *camEntry;

} fm10000_forwardRuleInternal;

fm_status fm10000CreateLogicalPortForGlort(fm_int    sw,
                                           fm_uint32 glort,
                                           fm_int *  logicalPort);

fm_status fm10000CreateLogicalPortForMailboxGlort(fm_int    sw,
                                                  fm_uint32 glort,
                                                  fm_int *  logicalPort);

fm_status fm10000CreateForwardingRule(fm_int sw,
                                      fm_int *ruleId,
                                      fm_forwardRule *rule);

fm_status fm10000DeleteForwardingRule(fm_int sw, fm_int ruleId);

fm_status fm10000RedirectCpuTrafficToPort(fm_int sw, fm_int port);

fm_status fm10000InformRedirectCPUPortLinkChange(fm_int sw,
                                                 fm_int port,
                                                 fm_portLinkStatus linkStatus);

#endif  /* __FM_FM10000_API_STACKING_INT_H */

