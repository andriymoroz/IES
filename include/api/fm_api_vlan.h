/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_vlan.h
 * Creation Date:   April 26, 2005
 * Description:     Structures and functions for dealing with 
 *                  VLAN configuration
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

#ifndef __FM_FM_API_VLAN_H
#define __FM_FM_API_VLAN_H

/*****************************************************************************
 * Macros & Constants
 *****************************************************************************/


/** The maximum number of VLANs.
 *  \ingroup constSystem */
#define FM_MAX_VLAN      4096


/*****************************************************************************
 * Types
 *****************************************************************************/


/****************************************************************************/
/** \ingroup constSTStates
 *
 * Spanning Tree States, used as an argument in calls to ''fmSetVlanPortState''
 * and ''fmSetSpanningTreePortState''.
 *                                                                      \lb\lb
 * Each state controls the following conditions:                            \lb
 *  Rx BPDU: Whether ingress (recevied) BPDU frames are forwarded to the CPU.   \lb
 *  Rx Traffic: Whether other ingress (recevied) frames are forwarded.          \lb
 *  Tx BPDU: Whether BPDU frames sent from the CPU application are transmitted. \lb
 *  Tx Traffic: Whether other transmitted frames are egressed.                  \lb
 *  Learn: Whether source MAC addresses on received frames are learned.         \lb
 *                                                                          \lb
 * In all states, network management frames (e.g., LACP, 802.1x) are
 * received and transmitted normally between the port and the CPU.
 ****************************************************************************/
enum _fm_stStates
{
    /** Rx BPDU: No                                                         \lb
     *  Rx Traffic: No                                                      \lb
     *  Tx BPDU: No                                                         \lb
     *  Tx Traffic: No                                                      \lb
     *  Learn: No                                                           */
    FM_STP_STATE_DISABLED = 0,

    /** Rx BPDU: Yes                                                        \lb
     *  Rx Traffic: No                                                      \lb
     *  Tx BPDU: Yes                                                        \lb
     *  Tx Traffic: No                                                      \lb
     *  Learn: No                                                           */
    FM_STP_STATE_LISTENING,

    /** Rx BPDU: Yes                                                        \lb
     *  Rx Traffic: No (except on FM2000)                                   \lb
     *  Tx BPDU: Yes                                                        \lb
     *  Tx Traffic: No (except on FM2000)                                   \lb
     *  Learn: Yes                                                          */
    FM_STP_STATE_LEARNING,

    /** Rx BPDU: Yes                                                        \lb
     *  Rx Traffic: Yes                                                     \lb
     *  Tx BPDU: Yes                                                        \lb
     *  Tx Traffic: Yes                                                     \lb
     *  Learn: Yes                                                          */
    FM_STP_STATE_FORWARDING,

    /** Rx BPDU: Yes                                                        \lb
     *  Rx Traffic: No                                                      \lb
     *  Tx BPDU: No                                                         \lb
     *  Tx Traffic: No                                                      \lb
     *  Learn: No                                                           */
    FM_STP_STATE_BLOCKING

};


/**************************************************/
/** \ingroup typeEnum
 *  Used as an argument to ''fmChangeVlanPortExt''
 *  and ''fmGetVlanPortTagExt''. Indicates which 
 *  VLAN field in the packet to consider when 
 *  deciding whether to include or exclude a VLAN 
 *  tag.
 **************************************************/
typedef enum
{
    /** Select VLAN1 */
    FM_VLAN_SELECT_VLAN1 = 0,

    /** Select VLAN2 */
    FM_VLAN_SELECT_VLAN2,

} fm_vlanSelect;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/


/* functions to mark valid or invalid a particular VLAN */
fm_status fmCreateVlan(fm_int sw, fm_uint16 vlanID);
fm_status fmDeleteVlan(fm_int sw, fm_uint16 vlanID);


/* functions to change membership and egress tagging state */
fm_status fmAddVlanPort(fm_int sw, fm_uint16 vlanID, fm_int port, fm_bool tag);
fm_status fmDeleteVlanPort(fm_int sw, fm_uint16 vlanID, fm_int port);
fm_status fmChangeVlanPort(fm_int    sw,
                           fm_uint16 vlanID,
                           fm_int    port,
                           fm_bool   tag);
fm_status fmChangeVlanPortExt(fm_int        sw,
                              fm_vlanSelect vlanSel,
                              fm_uint16     vlanID,
                              fm_int        port,
                              fm_bool       tag);


/* functions to retrieve a list of valid VLANs and a list of ports per VLAN */
fm_status fmGetVlanList(fm_int     sw,
                        fm_int *   nVlan,
                        fm_uint16 *vlanIDs,
                        fm_int     maxVlans);
fm_status fmGetVlanPortList(fm_int    sw,
                            fm_uint16 vlanID,
                            fm_int *  nPorts,
                            fm_int *  ports,
                            fm_int    maxPorts);


/* SNMP style functionas as per the spec for the above */
fm_status fmGetVlanFirst(fm_int sw, fm_int *firstID);
fm_status fmGetVlanNext(fm_int sw, fm_int startID, fm_int *nextID);
fm_status fmGetVlanPortFirst(fm_int sw, fm_int vlanID, fm_int *firstPort);
fm_status fmGetVlanPortNext(fm_int  sw,
                            fm_int  vlanID,
                            fm_int  startPort,
                            fm_int *nextPort);
fm_status fmGetVlanPortTag(fm_int sw, fm_int vlanID, fm_int port, fm_bool *tag);
fm_status fmGetVlanPortTagExt(fm_int        sw,
                              fm_vlanSelect vlanSel,
                              fm_int        vlanID,
                              fm_int        port,
                              fm_bool      *tag);


/* functions to modify the spanning tree state of a port in a VLAN */
fm_status fmSetVlanPortState(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    port,
                             fm_int    state);
fm_status fmGetVlanPortState(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    port,
                             fm_int *  state);


/* functions to update a list of ports in a VLAN */
fm_status fmAddVlanPortList(fm_int    sw,
                            fm_uint16 vlanID,
                            fm_int    numPorts,
                            fm_int *  portList,
                            fm_bool   tag);

fm_status fmDeleteVlanPortList(fm_int    sw,
                               fm_uint16 vlanID,
                               fm_int    numPorts,
                               fm_int *  portList);

fm_status fmSetVlanPortListState(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    numPorts,
                                 fm_int *  portList,
                                 fm_int    state);


/* attribute setting for VLAN settings */
fm_status fmGetVlanAttribute(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    attr,
                             void *    value);
fm_status fmSetVlanAttribute(fm_int    sw,
                             fm_uint16 vlanID,
                             fm_int    attr,
                             void *    value);

fm_status fmGetVlanPortAttribute(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    port,
                                 fm_int    attr,
                                 void *    value);


/* functions to manage customer VLANs for provider bridging */
fm_status fmAddCVlan(fm_int    sw,
                     fm_int    port,
                     fm_uint16 cVlan,
                     fm_uint16 sVlan);
fm_status fmDeleteCVlan(fm_int    sw,
                        fm_int    port,
                        fm_uint16 cVlan);
fm_status fmGetCVlanFirst(fm_int    sw,
                          fm_int *  firstPort,
                          fm_int *  firstCVlan);
fm_status fmGetCVlanNext(fm_int    sw,
                         fm_int     startPort,
                         fm_uint16  startCVlan,
                         fm_int *   nextPort,
                         fm_int *   nextCVlan);

fm_status fmGetSVlanFromPortCVlan(fm_int    sw,
                                  fm_int    port,
                                  fm_uint16 cVlan,
                                  fm_int *  sVlan);

fm_status fmDbgDumpCVlanCounter(fm_int sw);


#endif /* __FM_FM_API_VLAN_H */
