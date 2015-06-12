/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_flooding_int.h
 * Creation Date:   January 28, 2014
 * Description:     Header file for layer 2 frame flooding feature.
 *
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_FLOODING_INT_H
#define __FM_FM10000_API_FLOODING_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

typedef struct _fm10000_floodInfo
{
    /* Cached value of the api.FM10000.initUcastFloodingTriggers property. */
    fm_bool initUcastFlooding;

    /* Cached value of the api.FM10000.initMcastFloodingTriggers property. */
    fm_bool initMcastFlooding;

    /* Cached value of the api.FM10000.floodingTrapPriority property. */
    fm_int  trapPri;

    /* Multicast flooding trigger port sets. */
    fm_int  mcastDropSet;
    fm_int  mcastTrapSet;
    fm_int  mcastLogSet;

    /* Unicast flooding trigger port sets. */
    fm_int  ucastDropSet;
    fm_int  ucastTrapSet;
    fm_int  ucastLogSet;

    /* Auxiliary trigger port sets.*/
    fm_int  dropMaskSet;

    /* FFU resource identifier for the TRAP_ALWAYS ACL action trigger. */
    fm_int  trapAlwaysId;

} fm10000_floodInfo;


/*****************************************************************************
 * Public Function Prototypes
 *****************************************************************************/

fm_status fm10000InitFlooding(fm_int sw);
fm_status fm10000FreeFlooding(fm_int sw);

fm_status fm10000SetPortMcastFlooding(fm_int sw, fm_int port, fm_int value);
fm_status fm10000SetPortUcastFlooding(fm_int sw, fm_int port, fm_int value);

fm_status fm10000NotifyFloodingTrapAlwaysId(fm_int sw, fm_int trapAlwaysId);
fm_status fm10000SetFloodDestPort(fm_int  sw,
                                  fm_int  port,
                                  fm_bool state,
                                  fm_int  floodPort);

fm_status fm10000SetTrapPriorityUcastFlooding(fm_int sw, fm_int priority);
fm_status fm10000SetTrapPriorityMcastFlooding(fm_int sw, fm_int priority);

fm_status fm10000GetStateMcastTrapFlooding(fm_int sw, fm_bool * enabled);
fm_status fm10000GetStateUcastTrapFlooding(fm_int sw, fm_bool * enabled);

fm_status fm10000DbgDumpFlooding(fm_int sw);

#endif /* __FM_FM10000_API_FLOODING_INT_H */

