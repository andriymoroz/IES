/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_sflow.h
 * Creation Date:   May 29, 2008
 * Description:     Public definitions for the sFlow API.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#ifndef __FM_FM_API_SFLOW_H
#define __FM_FM_API_SFLOW_H

#define FM_SFLOW_VLAN_ANY           0
#define FM_SFLOW_NO_TRUNC          -1
#define FM_SFLOW_PRIORITY_ORIGINAL  FM_MIRROR_PRIORITY_ORIGINAL


/****************************************************************************/
/** \ingroup typeEnum
 *  sFlow types, used as an argument to ''fmCreateSFlow''.
 *                                                                      \lb\lb
 *  sFlows are not supported on FM2000 devices.
 ****************************************************************************/
typedef enum
{
    /** The sFlow monitors only ingress traffic. */
    FM_SFLOW_TYPE_INGRESS = 0,
    
    /** The sFlow monitors only egress traffic. */
    FM_SFLOW_TYPE_EGRESS
    
} fm_sFlowType;


/****************************************************************************/
/** \ingroup constSFlowAttr
 *
 *  sFlow Attributes, used as an argument to ''fmSetSFlowAttribute'' and 
 *  ''fmGetSFlowAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 *                                                                      \lb\lb
 *  sFlows are not supported on FM2000 devices.
 ****************************************************************************/
enum _fm_sFlowAttr
{
    /** Type fm_uint16: A VLAN ID to indicate the VLAN with which the sampled 
     *  traffic is associated. Set to FM_SFLOW_VLAN_ANY (default) to
     *  sample non-VLAN specific traffic. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_SFLOW_VLAN = 0,
 
    /** Type fm_uint: The sFlow sampling rate, expressed as a positive integer
     *  N, indicating that every Nth frame is to be sampled. The specified
     *  value will be rounded up or down to the nearest value supported
     *  by the hardware, so the value read back may not exactly match the
     *  value that was set. 
     *                                                              \lb\lb
     *  For FM3000/FM4000 and FM10000 devices, the value ranges from 1 (the
     *  default) to 16,777,215. The value set applies only to the specified
     *  sFlow.
     *                                                              \lb\lb
     *  For FM6000 devices, the value ranges from 1 to 65535 with 
     *  a default of 1. Only one sample rate can be set for the entire 
     *  switch. All sFlow instances will share the same value, which will be
     *  the last one set.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_SFLOW_SAMPLE_RATE,
 
    /** Type fm_int: The sFlow sample frame truncation length in bytes. Set to
     *  FM_SFLOW_NO_TRUNC (the default) to disable truncation. 
     *                                                                  \lb\lb
     *  For FM3000/FM4000 devices, the specified value will be rounded 
     *  down to the nearest multiple of 4. 
     *                                                                  \lb\lb
     *  For FM6000 devices, any value other than FM_SFLOW_NO_TRUNC 
     *  will enable truncation to a fixed length of 160 bytes.
     *
     *  \chips  FM3000, FM4000, FM6000 */
    FM_SFLOW_TRUNC_LENGTH,
    
    /** Type fm_int: A read-only attribute used to get an identifier for a
     *  specific sFlow. 
     *                                                                  \lb\lb
     *  For FM3000/FM4000 devices, the identifier is the trigger Number 
     *  used for sampling. 
     *                                                                  \lb\lb
     *  For FM6000 devices, the identifier is the frame destination GLORT.
     *                                                                  \lb\lb
     *  For FM10000 devices, the identifier is the rule number of the
     *  trigger used for sampling.
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_SFLOW_TRAP_CODE, 

    /** Type fm_uint64: A read-only attribute used to get the number of packets
     *  matching the sFlow conditions for the specific sFlow. This counter 
     *  represents the trigger count associated with the identified sFlow.
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_SFLOW_COUNT, 

    /** Type fm_int: The priority traffic class for the sampled traffic. Values
     *  range from 0 to 11 with the default being FM_SFLOW_PRIORITY_ORIGINAL,
     *  which indicates that the priority should be the original priority of
     *  the frame being sampled.
     *
     *  \chips FM6000 */
    FM_SFLOW_PRIORITY,

    /** UNPUBLISHED: For internal use only. */
    FM_SFLOW_ATTR_MAX
    
};  /* end enum _fm_sFlowAttr */


/*****************************************************************************
 * Public function prototypes.
 *****************************************************************************/

fm_status fmCreateSFlow(fm_int sw, fm_int sFlowId, fm_sFlowType sFlowType);

fm_status fmDeleteSFlow(fm_int sw, fm_int sFlowId);

fm_status fmAddSFlowPort(fm_int sw, fm_int sFlowId, fm_int port);

fm_status fmDeleteSFlowPort(fm_int sw, fm_int sFlowId, fm_int port);

fm_status fmGetSFlowPortFirst(fm_int sw, fm_int sFlowId, fm_int * firstPort);

fm_status fmGetSFlowPortNext(fm_int   sw,
                             fm_int   sFlowId,
                             fm_int   startPort,
                             fm_int * nextPort);

fm_status fmGetSFlowPortList(fm_int   sw, 
                             fm_int   sFlowId, 
                             fm_int * numPorts, 
                             fm_int * portList, 
                             fm_int   max);

fm_status fmSetSFlowAttribute(fm_int sw, 
                              fm_int sFlowId, 
                              fm_int attr, 
                              void * value);

fm_status fmGetSFlowAttribute(fm_int sw, 
                              fm_int sFlowId, 
                              fm_int attr, 
                              void * value);

fm_status fmGetSFlowType(fm_int         sw, 
                         fm_int         sFlowId,
                         fm_sFlowType * sFlowType);

fm_status fmCheckSFlowLogging(fm_int            sw, 
                              fm_eventPktRecv * pktEvent, 
                              fm_bool         * isPktSFlowLogged);

#endif /* __FM_FM_API_SFLOW_H */
