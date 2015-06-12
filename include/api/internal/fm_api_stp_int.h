/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stp_int.h
 * Creation Date:   January 16, 2008
 * Description:     Structures and functions for dealing with spanning tree
 *                  instances.
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

#ifndef __FM_FM_API_STP_INT_H
#define __FM_FM_API_STP_INT_H

/*****************************************************************************
 * Macros & Constants
 *****************************************************************************/


/*****************************************************************************
 * Types
 *****************************************************************************/


typedef struct
{
    /* The instance number for reference */
    fm_int      instance;

    /* A bitmap of the VLANs, with set bits indicating membership */
    fm_bitArray vlans;

    /* A list of states per port in the system */
    fm_int *    states;

} fm_stpInstanceInfo;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/


fm_status fmAllocateStpInstanceTreeDataStructures(fm_switch *switchPtr);
fm_status fmInitStpInstanceTree(fm_switch *switchPtr);
fm_status fmDestroyStpInstanceTree(fm_switch *switchPtr);
fm_status fmFreeStpInstanceTreeDataStructures(fm_switch *switchPtr);
fm_status fmResetMultipleSpanningTreeState(fm_int sw);
fm_status fmRefreshSpanningTreeStateForVlan(fm_int sw, fm_int vlanID);
fm_status fmFindInstanceForVlan(fm_int sw, fm_int vlanID, fm_int *instance);

fm_status fmRefreshStpStateInternal(fm_switch *switchPtr, 
                                    fm_stpInstanceInfo *instance, 
                                    fm_int vlanID, 
                                    fm_int port);
fm_status AddSpanningTreeVlanInternal(fm_int sw,
                                      fm_int stpInstance,
                                      fm_stpInstanceInfo *instance,
                                      fm_int vlanID);
fm_status fmDeleteSpanningTreeInternal(fm_int sw, 
                                       fm_stpInstanceInfo *instance);
fm_status fmRefreshStpState(fm_int sw,
                            fm_int stpInstance,
                            fm_int vlanID,
                            fm_int port);

#endif /* __FM_FM_API_STP_INT_H */
