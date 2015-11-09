/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_sflow_int.h
 * Creation Date:   February 14, 2014
 * Description:     FM10000 sFlow API internal definitions.
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

#ifndef __FM_FM10000_API_SFLOW_INT_H
#define __FM_FM10000_API_SFLOW_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* The maximum number of sFlows that may be defined. */
#define FM10000_MAX_SFLOWS              4

#define FM10000_SFLOW_RULE_NONE         -1
#define FM10000_SFLOW_RES_ID_NONE       -1

#define FM10000_SFLOW_MAX_SAMPLE_RATE   0xffffff
#define FM10000_SFLOW_TRAPCODE_ID_START 12

/**************************************************
 * Information related to an sFlow.
 **************************************************/
typedef struct _fm10000_sflowEntry
{
    /* Mirror Id of the associated mirror. */
    fm_int          mirrorId;

    /* Type of sFlow (ingress or egress). */
    fm_sFlowType    sflowType;

    /* VLAN assigned to this sFlow, as specified by the FM_SFLOW_VLAN
     * attribute. Will be FM_SFLOW_VLAN_ANY if unassigned. */
    fm_uint16       vlanID;

    /* SFlow sampling rate, as specified by the FM_SFLOW_SAMPLE_RATE
     * attribute. */
    fm_uint         sampleRate;

    /* Whether this sFlow is valid. */
    fm_bool         isValid;

    /* Trap Code ID */
    fm_int          trapCodeId;

} fm10000_sflowEntry;


/*****************************************************************************
 * Public function prototypes.
 *****************************************************************************/

fm_status fm10000AddSFlowPort(fm_int sw, fm_int sFlowId, fm_int port);

fm_status fm10000CreateSFlow(fm_int sw, fm_int sFlowId, fm_sFlowType sFlowType);

fm_status fm10000DeleteSFlow(fm_int sw, fm_int sFlowId);

fm_status fm10000DeleteSFlowPort(fm_int sw, fm_int sFlowId, fm_int port);

fm_status fm10000FreeSFlows(fm_int sw);

fm_status fm10000InitSFlows(fm_int sw);

fm_status fm10000GetSFlowAttribute(fm_int sw, 
                                   fm_int sFlowId, 
                                   fm_int attr, 
                                   void * value);

fm_status fm10000GetSFlowPortFirst(fm_int   sw, 
                                   fm_int   sFlowId, 
                                   fm_int * firstPort);

fm_status fm10000GetSFlowPortNext(fm_int   sw,
                                  fm_int   sFlowId,
                                  fm_int   startPort,
                                  fm_int * nextPort);

fm_status fm10000GetSFlowPortList(fm_int   sw, 
                                  fm_int   sFlowId, 
                                  fm_int * numPorts, 
                                  fm_int * portList, 
                                  fm_int   maxPorts);

fm_status fm10000SetSFlowAttribute(fm_int sw, 
                                   fm_int sFlowId, 
                                   fm_int attr, 
                                   void * value);

fm_status fm10000CheckSFlowLogging(fm_int            sw,
                                   fm_eventPktRecv *pktEvent,
                                   fm_bool         *isPktSFlowLogged);

fm_status fm10000GetSFlowType(fm_int         sw, 
                              fm_int         sFlowId,
                              fm_sFlowType * sFlowType);

fm_status fm10000DbgDumpSFlows(fm_int sw);

#endif /* __FM_FM10000_API_SFLOW_INT_H */
