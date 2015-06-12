/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_stp_int.h
 * Creation Date:   July 2, 2013 using fm6000_api_stp_int.h as a template
 * Description:     Prototypes for internal FM10000 STP functions.
 *
 * Copyright (c) 2008 - 2013, Intel Corporation
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

#ifndef __FM_FM10000_API_STP_INT_H
#define __FM_FM10000_API_STP_INT_H

/*****************************************************************************
 * Macros & Constants
 *****************************************************************************/


/*****************************************************************************
 * Types
 *****************************************************************************/


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/


fm_status fm10000CreateSpanningTree(fm_int sw, fm_int stpInstance);

fm_status fm10000AddSpanningTreeVlan(fm_int sw,
                                     fm_int stpInstance,
                                     fm_int vlan);

fm_status fm10000DeleteSpanningTreeVlan(fm_int sw,
                                        fm_int stpInstance,
                                        fm_int vlan);

fm_status fm10000RefreshSpanningTree(fm_int sw,
                                     fm_stpInstanceInfo *instance,
                                     fm_int vlan,
                                     fm_int port);

fm_status fm10000ResetVlanSpanningTreeState(fm_int sw,
                                            fm_uint16
                                            vlanID);

fm_status fm10000EnableMultipleSpanningTreeMode(fm_int sw);
fm_status fm10000EnableSharedSpanningTreeMode(fm_int sw);

void fm10000DbgDumpSpanningTree(fm_int sw, fm_int instance);

#endif  /* __FM_FM10000_API_STP_INT_H */

