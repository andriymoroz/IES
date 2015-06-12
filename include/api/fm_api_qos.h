/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_qos.h
 * Creation Date:   May 26, 2005
 * Description:     Contains functions dealing with the QOS settings,
 *                  i.e. watermarks, priority maps, etc.
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_API_QOS_H
#define __FM_FM_API_QOS_H


/* functions to set and get port specific QOS parameters */
fm_status fmSetPortQOS(fm_int sw,
                       fm_int port,
                       fm_int attr,
                       fm_int index,
                       void * value);
fm_status fmGetPortQOS(fm_int sw,
                       fm_int port,
                       fm_int attr,
                       fm_int index,
                       void * value);


/* functions to set and get global switch level QOS parameters */
fm_status fmSetSwitchQOS(fm_int sw, fm_int attr, fm_int index, void *value);
fm_status fmGetSwitchQOS(fm_int sw, fm_int attr, fm_int index, void *value);
fm_status fmGetMemoryUsage(fm_int  sw,
                           fm_int  port,
                           fm_int  partition,
                           fm_int *globalUsage,
                           fm_int *partUsage,
                           fm_int *rxUsage,
                           fm_int *rxPartUsage,
                           fm_int *txUsage,
                           fm_int *txPartUsage,
                           fm_int *txPerClassUsage);
fm_status fmAddQDM(fm_int sw, fm_int port, fm_int tc, fm_int weight, fm_int cnt);
fm_status fmDelQDM(fm_int sw, fm_int port, fm_int tc);
fm_status fmResetQDM(fm_int sw, fm_int port, fm_int tc);
fm_status fmGetQDM(fm_int sw, fm_int port, fm_int tc, fm_int *delay);


#endif /* __FM_FM_API_QOS_H */
