/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_vlan_int.h
 * Creation Date:   January 1, 2005
 * Description:     Structures and functions for dealing with VLAN 
 *                  configuration
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

#ifndef __FM_FM_API_VLAN_INT_H
#define __FM_FM_API_VLAN_INT_H


/*****************************************************************************
 * Macros & Constants
 *****************************************************************************/

/* the vlan tag types */
#define FM_VLAN_TAG_TYPE_8100  0x8100
#define FM_VLAN_TAG_TYPE_9100  0x9100
#define FM_VLAN_TAG_TYPE_9200  0x9200


/*****************************************************************************
 * Types
 *****************************************************************************/

/* structure to hold VLAN entry */
typedef struct _fm_vlanEntry
{
    /* The vlan number this entry refers to. */
    fm_uint16 vlanId;

    /* Used internally to store validity of this entry */
    fm_bool   valid;

    /* When true, indicates that this VLAN allows traffic to
     * flood back to itself.
     */
    fm_bool   reflect;

    /* Indicates that IGMP trapping is permitted on this VLAN.
     * 
     * \chips 4000, 5000, 6000
     */
    fm_bool   trapIGMP;

    /* Indicates that frames may route on this VLAN.
     * 
     * \chips 4000, 5000, 6000
     */
    fm_bool   routable;

    /* Pointer to the switch specific extension */
    void *    vlanExt;

} fm_vlanEntry;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/

/* initializes state of Vlan table */
fm_status fmAllocateVlanTableDataStructures(fm_switch *switchPtr);
fm_status fmInitVlanTable(fm_switch *switchPtr);
fm_status fmFreeVlanTableDataStructures(fm_switch *switchPtr);

/* Internal functions */
fm_status fmCreateVlanInt(fm_int sw, fm_uint16 vlanID);
fm_status fmAddVlanPortInternal(fm_int sw, fm_uint16 vlanID, fm_int port, fm_bool tag);
fm_status fmAddInternalPortsToVlan(fm_int sw, fm_uint16 vlanID);

/* helper functions to abstract entry */
fm_status fmSetVlanMembership(fm_int        sw,
                              fm_vlanEntry *entry,
                              fm_int        port,
                              fm_bool       state);
fm_status fmSetVlanTag(fm_int        sw,
                       fm_vlanSelect vlanSel,
                       fm_vlanEntry *entry,
                       fm_int        port,
                       fm_bool       tag);
fm_status fmGetVlanMembership(fm_int        sw,
                              fm_vlanEntry *entry,
                              fm_int        port,
                              fm_bool *     state);
fm_status fmGetVlanTag(fm_int        sw,
                       fm_vlanSelect vlanSel,
                       fm_vlanEntry *entry,
                       fm_int        port,
                       fm_bool *     tag);

fm_status fmGetVlanPortStateInternal(fm_int    sw,
                                     fm_uint16 vlanID,
                                     fm_int    port,
                                     fm_int *  state);

fm_status fmSetVlanPortStateInternal(fm_int    sw,
                                     fm_uint16 vlanID,
                                     fm_int    port,
                                     fm_int    state);

/* Port list extraction utilities. */
fm_status fmExtractVlanPhysicalPortList(fm_int   sw,
                                        fm_int   numVlanPorts,
                                        fm_int * vlanPortList,
                                        fm_int * numPhysPorts,
                                        fm_int * physPortList,
                                        fm_int   maxPhysPorts);

fm_status fmExtractVlanLagPortList(fm_int   sw,
                                   fm_int   numVlanPorts,
                                   fm_int * vlanPortList,
                                   fm_int * numLagPorts,
                                   fm_int * lagPortList,
                                   fm_int   maxLagPorts);

/* counter helper */
fm_status fmSetVlanCounterID(fm_int sw, fm_uint vlanID, fm_uint vcnt);


#endif /* __FM_FM_API_VLAN_INT_H */
