
/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_vn_int.h
 * Creation Date:   March 5, 2014
 * Description:     Contains FM10000 constants and functions used to support
 *                  virtual networks.
 *
 * Copyright (c) 2014, Intel Corporation
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

#ifndef __FM_FM10000_API_VN_INT_H
#define __FM_FM10000_API_VN_INT_H


/* Encapsulation and Decapsulation Tunnels */
#define FM_VN_ENCAP_GROUP_DMAC_VID      0
#define FM_VN_ENCAP_GROUP_DIP_VID       1
#define FM_VN_ENCAP_GROUP_DIRECT        2
#define FM_VN_DECAP_GROUP_DMAC_VID      3
#define FM_VN_DECAP_GROUP_DIP_VID       4
#define FM_VN_DECAP_GROUP_DIRECT        5
#define FM_VN_NUM_TUNNEL_GROUPS         6
#define FM_VN_NUM_ENCAP_TUNNEL_GROUPS   3

/* Encapsulation Flow Record Types: Transparent and VSwitch-Offload. */
#define FM_VN_ENCAP_FLOW_TRANSPARENT    0
#define FM_VN_ENCAP_FLOW_VSWITCH        1
#define FM_VN_NUM_ENCAP_FLOW_TYPES      2


/*****************************************************************************
 *
 *  Virtual Network Remote Address
 *
 *  This structure contains information describing a virtual network
 *  remote address.
 *
 *****************************************************************************/
typedef struct _fm10000_vnRemoteAddress
{
    /* Pointer to the virtual network record. */
    fm_virtualNetwork *                  vn;

    /* Pointer to the VN Tunnel record. */
    fm_vnTunnel *                        tunnel;

    /* Remote Address. */
    fm_vnAddress                         remoteAddress;

    /* Pointer to the remote address mask record, NULL if there is not a
     * remote address mask that covers this address. */
    struct _fm10000_vnRemoteAddressMask *addrMask;

    /* Pointer to the encap TEP rule. */
    struct _fm10000_vnEncapTep *         encapTep;

    /* Encapsulation ACL Rule. */
    fm_int                               encapAclRule;

    /* Encapsulation Tunnel Rule. -1 if not used. */
    fm_int                               encapTunnelRule;

    /* Decapsulation tunnel rule ID. */
    fm_int                               decapTunnelRule;

    /* Active Encap Tunnel Group. */
    fm_int                               encapTunnelGroup;

    /* Hash Encap tunnel group. */
    fm_int                               hashEncapGroup;

    /* Decap tunnel group. */
    fm_int                               decapTunnelGroup;

    /* Encapsulation tunnel rule IDs. Only used when using hash encap. */
    fm_int                               encapTunnelRules[FM10000_TE_VNI_ENTRIES_0];

} fm10000_vnRemoteAddress;


/*****************************************************************************
 *
 *  Virtual Network Remote Address Mask
 *
 *  This structure contains information describing a virtual network
 *  remote address mask.
 *
 *****************************************************************************/
typedef struct _fm10000_vnRemoteAddressMask
{
    /* Pointer to the virtual network record. */
    fm_virtualNetwork *vn;

    /* Pointer to the VN Tunnel record. NULL if the address mask does not
     * use a single tunnel. */
    fm_vnTunnel *      tunnel;

    /* Remote Address. */
    fm_vnAddress       remoteAddress;

    /* Address Bit Mask. */
    fm_vnAddress       addrMask;

    /* Pointer to the encap TEP rule. */
    struct _fm10000_vnEncapTep *         encapTep;

    /* Encapsulation ACL Rule. */
    fm_int             encapAclRule;

    /* Encapsulation Tunnel Rule. -1 if not used, in which case encapTep must
     * not be NULL. */
    fm_int             encapTunnelRule;

} fm10000_vnRemoteAddressMask;


/*****************************************************************************
 *
 *  Virtual Network Encap Tunnel End-Point Record
 *
 *  This structure contains information describing a virtual network
 *  encapsulation tunnel end-point.
 *
 *****************************************************************************/
typedef struct _fm10000_vnEncapTep
{
    /* Pointer to the virtual network record. */
    fm_virtualNetwork *vn;

    /* Pointer to the VN Tunnel record. */
    fm_vnTunnel *      tunnel;

    /* Use counter. */
    fm_int             useCount;

    /* Encapsulation Tunnel Rule. -1 if not used. */
    fm_int             encapTunnelRule;

} fm10000_vnEncapTep;


/*****************************************************************************
 *
 *  Virtual Network Information
 *
 *  This structure contains fm10000-specific information for a virtual network.
 *
 *****************************************************************************/
typedef struct _fm10000_virtualNetwork
{
    /* Pointer to the virtual network structure. */
    fm_virtualNetwork *vn;

    /* Primary VSI. Taken from internal ID in descriptor record if present.
     * Otherwise, the first VSI added to the VN is treated as the primary. */
    fm_int             primaryVsi;

    /* Tree of remote addresses associated with this virtual network. */
    fm_customTree      remoteAddresses;

    /* Tree of remote address masks. */
    fm_customTree      addressMasks;

    /* Broadcast/Flooding Floodset Multicast Group. */
    fm_int             floodsetMcastGroup;

    /* Broadcast/Flooding Floodset Encap ACL Rule. */
    fm_int             floodsetEncapAclRule;

    /* Tree of tunnels that service this virtual network. */
    fm_tree            tunnels;

} fm10000_virtualNetwork;


/*****************************************************************************
 *
 *  VN Tunnel information
 *
 *  This structure contains fm10000-specific information for a virtual network
 *  Tunnel.
 *
 *****************************************************************************/
typedef struct _fm10000_vnTunnel
{
    /* Pointer to the fm_vnTunnel structure. */
    fm_vnTunnel * tunnel;

    /* Encapsulation Tunnel Flow IDs. */
    fm_int        encapFlowIds[FM_VN_NUM_TUNNEL_GROUPS][FM_VN_NUM_ENCAP_FLOW_TYPES];

    /* TRUE if the Local IP Address has been initialized. */
    fm_bool       haveLocalIp;

    /* TRUE if the Remote IP Address has been initialized. */
    fm_bool       haveRemoteIp;

    /* Decapsulation ACL Rule Number. */
    fm_int        decapAclRule;

    /* Count of active tunnel rules by group and flow type. */
    fm_int        ruleCounts[FM_VN_NUM_TUNNEL_GROUPS][FM_VN_NUM_ENCAP_FLOW_TYPES];

    /* Tree of TEP encap tunnel rules. */
    fm_customTree tepRules;

} fm10000_vnTunnel;


/*****************************************************************************
 *
 *  VN Tunnel Usage information
 *
 *  This structure is used to track how many remote addresses are supported
 *  by a tunnel for a virtual network. It is used to determine when to stop
 *  flooding or broadcasting through a tunnel for a network.
 *
 *****************************************************************************/
typedef struct _fm10000_vnTunnelUseCount
{
    /* tunnel pointer */
    fm_vnTunnel *tunnel;

    /* use count */
    fm_int       useCount;

} fm10000_vnTunnelUseCount;


/*****************************************************************************
 *
 *  VN Decap ACL Rule information
 *
 *  This structure contains information describing a virtual network
 *  decapsulation ACL rule.
 *
 *****************************************************************************/
typedef struct _fm10000_vnDecapAclRule
{
    /* Pointer to the fm_vnTunnel record. */
    fm_vnTunnel *      tunnel;

    /* Pointer to the virtual network record. */
    fm_virtualNetwork *vn;

    /* Decap tunnel group. */
    fm_int             decapTunnelGroup;

    /* Decapsulation ACL Rule. */
    fm_int             aclRule;

    /* Number of tunnel rules using this decap acl rule. */
    fm_int             useCount;

} fm10000_vnDecapAclRule;




fm_status fm10000CreateVirtualNetwork(fm_int             sw,
                                     fm_virtualNetwork *vn);
fm_status fm10000DeleteVirtualNetwork(fm_int             sw,
                                     fm_virtualNetwork *vn);
fm_status fm10000UpdateVirtualNetwork(fm_int             sw,
                                     fm_virtualNetwork *vn,
                                     fm_vnDescriptor *  oldDescriptor);
fm_status fm10000FreeVirtualNetwork(fm_int             sw,
                                    fm_virtualNetwork *vn);
fm_status fm10000CreateVNTunnel(fm_int       sw,
                               fm_vnTunnel *tunnel);
fm_status fm10000DeleteVNTunnel(fm_int       sw,
                               fm_vnTunnel *tunnel);
fm_status fm10000SetVNTunnelAttribute(fm_int              sw,
                                     fm_vnTunnel *       tunnel,
                                     fm_vnTunnelAttrType attr,
                                     void *              value);
fm_status fm10000UpdateVNTunnelECMPGroup(fm_int sw, fm_vnTunnel *tunnel);
fm_status fm10000FreeVNTunnel(fm_int       sw,
                              fm_vnTunnel *tunnel);
fm_status fm10000GetVNTunnelGroupAndRule(fm_int              sw,
                                         fm_int              tunnelId,
                                         fm_bool             encap,
                                         fm_uint32           vni,
                                         fm_int *            group,
                                         fm_int *            rule,
                                         fm_tunnelGlortUser *glort);
fm_status fm10000UpdateTunnelUdpPort(fm_int sw);
fm_status fm10000AddVNRemoteAddress(fm_int             sw,
                                    fm_virtualNetwork *vn,
                                    fm_vnTunnel *      tunnel,
                                    fm_vnAddress *     addr);
fm_status fm10000DeleteVNRemoteAddress(fm_int             sw,
                                       fm_virtualNetwork *vn,
                                       fm_vnTunnel *      tunnel,
                                       fm_vnAddress *     addr);
fm_status fm10000AddVNRemoteAddressMask(fm_int             sw,
                                        fm_virtualNetwork *vn,
                                        fm_vnTunnel *      tunnel,
                                        fm_vnAddress *     addr,
                                        fm_vnAddress *     addrMask);
fm_status fm10000DeleteVNRemoteAddressMask(fm_int             sw,
                                           fm_virtualNetwork *vn,
                                           fm_vnAddress *     baseAddr,
                                           fm_vnAddress *     addrMask);
fm_status fm10000ConfigureVN(fm_int sw, fm_vnConfiguration *config);
fm_status fm10000AddVNDirectTunnelRule(fm_int     sw,
                                       fm_int     tunnelId,
                                       fm_uint32  vni,
                                       fm_int *   portNum,
                                       fm_uint32 *dglort);
fm_status fm10000DeleteVNDirectTunnelRule(fm_int     sw,
                                          fm_int     tunnelId,
                                          fm_uint32  vni,
                                          fm_int *   portNum,
                                          fm_uint32 *dglort);
fm_status fm10000AddVNLocalPort(fm_int             sw,
                                fm_virtualNetwork *vn,
                                fm_int             port);
fm_status fm10000DeleteVNLocalPort(fm_int             sw,
                                   fm_virtualNetwork *vn,
                                   fm_int             port);
fm_status fm10000AddVNVsi(fm_int             sw,
                          fm_virtualNetwork *vn,
                          fm_int             vsi);
fm_status fm10000DeleteVNVsi(fm_int             sw,
                             fm_virtualNetwork *vn,
                             fm_int             vsi);
fm_status fm10000IsVNTunnelInUseByACLs(fm_int   sw,
                                       fm_int   tunnelId,
                                       fm_bool *inUse);
fm_status fm10000FreeVNResources(fm_int sw);

#endif      /* __FM_FM10000_API_VN_INT_H */

