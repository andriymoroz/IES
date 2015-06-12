
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_stats_int.c
 * Creation Date:   April 25, 2013
 * Description:     Definitions for fm10000 statistics helpers.
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

#ifndef __FM_FM10000_API_STATS_INT_H
#define __FM_FM10000_API_STATS_INT_H

fm_status fm10000ResetPortCounters(fm_int sw,
                                   fm_int port);

fm_status fm10000GetPortCounters(fm_int sw,
                                 fm_int port,
                                 fm_portCounters *counters);

fm_status fm10000GetVLANCounters(fm_int sw,
                                 fm_int vcid,
                                 fm_vlanCounters *counters);

fm_status fm10000ResetVLANCounters(fm_int sw,
                                   fm_int vcid);

fm_status fm10000GetCountersInitMode(fm_int sw, fm_uint32 *mode);

fm_status fm10000InitCounters(fm_int sw);

fm_status fm10000SetStatsFrameAdjustment(fm_int sw,
                                         fm_int physPort,
                                         fm_int nbBytes);


#endif	/* __FM_FM10000_API_STATS_INT_H */

