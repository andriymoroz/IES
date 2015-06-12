
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_lbg_int.h
 * Creation Date:   September, 2013
 * Description:     Prototypes for managing load balancing groups on FM10000
 *                  devices.
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

#ifndef __FM_FM10000_API_LBG_INT_H
#define __FM_FM10000_API_LBG_INT_H

/***************************************************
 * This structure contains the state information
 * for a single load balancing group on FM10000.
 **************************************************/
typedef struct _fm10000_LBGGroup
{
    /* The handle of the ARP block as returned by fm10000RequestArpBlock() */
    fm_uint16     arpBlockHandle;

    /* The index of the first ARP_TABLE entry. The ARP block size should be 
     * equal to the number of bins. */
    fm_uint16     arpBlockIndex;

} fm10000_LBGGroup;

/* Initialize/Cleanup LBG structures */
fm_status fm10000LBGInit(fm_int sw);
fm_status fm10000FreeLBGResource(fm_int sw);

/* Create and delete load balancing groups */
fm_status fm10000CreateLBG(fm_int        sw, 
                           fm_int *      lbgNumber, 
                           fm_LBGParams *params);
fm_status fm10000DeleteLBG(fm_int sw, fm_int lbgNumber);


/* Manage members of load balancing groups */
fm_status fm10000AddLBGPort(fm_int sw, fm_int lbgNumber, fm_int port);
fm_status fm10000DeleteLBGPort(fm_int sw, fm_int lbgNumber, fm_int port);


/* Handle setting attributes on load balancing groups */
fm_status fm10000SetLBGAttribute(fm_int sw,
                                 fm_int lbgNumber,
                                 fm_int attr,
                                 void * value);
fm_status fm10000GetLBGAttribute(fm_int sw,
                                 fm_int lbgNumber,
                                 fm_int attr,
                                 void * value);


/* Handle setting attributes on load balancing group members */
fm_status fm10000SetLBGPortAttribute(fm_int sw,
                                     fm_int lbgNumber,
                                     fm_int port,
                                     fm_int attr,
                                     void * value);
fm_status fm10000GetLBGPortAttribute(fm_int sw,
                                     fm_int lbgNumber,
                                     fm_int port,
                                     fm_int attr,
                                     void * value);

/* Get the next hop base index and length of a LBG group */
fm_status fm10000GetLBGInfo(fm_int  sw, 
                            fm_int  lbgNumber,
                            fm_int *arpBaseIndex, 
                            fm_int *arpBlockLength);

fm_status fm10000DbgDumpLBG(fm_int sw, fm_int lbgNumber);

fm_status fm10000GetPortParametersForLBG(fm_int sw,
                                         fm_int *numPorts,
                                         fm_int *numDestEntries);

fm_status fm10000AssignLBGPortResources(fm_int sw, void *params);

fm_status fm10000AllocateLBGs(fm_int sw,
                              fm_uint startGlort,
                              fm_uint glortSize,
                              fm_int *baseLbgHandle,
                              fm_int *numLbgs,
                              fm_int *step);

fm_status fm10000FreeLBGs(fm_int sw, fm_int baseLbgHandle);

#endif /* __FM_FM10000_API_LBG_INT_H */
