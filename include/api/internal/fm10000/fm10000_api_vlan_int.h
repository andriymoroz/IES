/* vim:et:sw=4:ts=4:tw=80:
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_vlan_int.h
 * Creation Date:   December 3, 2012
 * Description:     FM10xxx-specific VLAN definitions.
 *
 * Copyright (c) 2012 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_VLAN_INT_H
#define __FM_FM10000_API_VLAN_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

typedef struct _fm10000_vlanEntry
{
    /* Membership state mask, including CPU port #0.
     * One bit per port, indexed by physical port. */
    fm_portmask member;

    /* Egress tagging state mask for all ports.
     * One bit per port, indexed by cardinal port index. */
    fm_portmask tag;

    /* trigger to fire when this VLAN is accessed */
    fm_int      trigger;

    /* mtu index */
    fm_int      mtuIndex;

    /* egress vlan tag VID (FM10000_VLAN_TAG_VID1_MAP, VID field) */
    fm_int      egressVid;

    /* index into counters for this VLAN */
    fm_int      statIndex;

} fm10000_vlanEntry;




/*****************************************************************************
 * Public Function Prototypes
 *****************************************************************************/


fm_status fm10000AllocateVlanTableDataStructures(fm_switch *switchPtr);
fm_status fm10000FreeVlanTableDataStructures(fm_switch *switchPtr);
fm_status fm10000InitVlanTable(fm_switch *switchPtr);
fm_status fm10000CreateVlan(fm_int sw, fm_uint16 vlanID);
fm_status fm10000DeleteVlan(fm_int sw, fm_uint16 vlanID);
fm_status fm10000WriteVlanEntry(fm_int sw, fm_uint16 vlanID);
fm_status fm10000WriteTagEntry(fm_int sw, fm_uint16 vlanID);
fm_status fm10000ConfigureVlanLearningMode(fm_int sw, fm_vlanLearningMode mode);
fm_status fm10000WriteVlanEntryV2(fm_int sw, fm_uint16 vlanID, fm_int stpInstance);
fm_status fm10000SetVlanMembership(fm_int        sw,
                                   fm_vlanEntry *entry,
                                   fm_int        port,
                                   fm_bool       state);
fm_status fm10000GetVlanMembership(fm_int        sw,
                                   fm_vlanEntry *entry,
                                   fm_int        port,
                                   fm_bool *     state);
fm_status fm10000SetVlanTag(fm_int        sw,
                            fm_vlanSelect vlanSel,
                            fm_vlanEntry *entry,
                            fm_int        port,
                            fm_bool       tag);
fm_status fm10000GetVlanTag(fm_int        sw,
                            fm_vlanSelect vlanSel,
                            fm_vlanEntry *entry,
                            fm_int        port,
                            fm_bool *     tag);
fm_status fm10000SetVlanTrigger(fm_int    sw,
                                fm_uint16 vlanID,
                                fm_int    triggerId);
fm_status fm10000AddVlanPortList(fm_int    sw,
                                fm_uint16 vlanID,
                                fm_int    numPorts,
                                fm_int *  portList,
                                fm_bool   tag);
fm_status fm10000DeleteVlanPortList(fm_int    sw,
                                    fm_uint16 vlanID,
                                    fm_int    numPorts,
                                    fm_int *  portList);
fm_status fm10000GetVlanAttribute(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    attr,
                                  void *    value);
fm_status fm10000SetVlanAttribute(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    attr,
                                  void *    value);
fm_status fm10000GetVlanPortAttribute(fm_int    sw,
                                      fm_uint16 vlanID,
                                      fm_int    port,
                                      fm_int    attr,
                                      void *    value);
fm_status fm10000SetVlanCounterID(fm_int sw, fm_uint vlanID, fm_uint vcnt);
fm_status fm10000SetVlanPortState(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    port,
                                  fm_int    state);
fm_status fm10000GetVlanPortState(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    port,
                                  fm_int *  state);

fm_status fm10000DbgDumpVid(int sw);

#endif  /* __FM_FM10000_API_VLAN_INT_H */

