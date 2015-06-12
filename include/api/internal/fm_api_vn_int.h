/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_vn_int.h
 * Creation Date:   Oct 11, 2012
 * Description:     Virtual Network Internal Definitions
 *
 * Copyright (c) 2012, Intel Corporation
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

#ifndef __FM_FM_API_VN_INT_H
#define __FM_FM_API_VN_INT_H


/* Internal IDs are 12-bits in length. This allows for a maximum of 4096
 * virtual networks. */
#define FM_MAX_NUM_VNS                  (1 << 12)


/*****************************************************************************
 * Structure to describe a virtual network.
 *****************************************************************************/
typedef struct _fm_virtualNetwork
{
    /* Virtual Service ID */
    fm_uint32       vsId;

    /* Descriptor for the virtual network */
    fm_vnDescriptor descriptor;

    /* Pointer to a switch-specific extension structure */
    void *          extension;

} fm_virtualNetwork;


/*****************************************************************************
 * Structure to describe a tunnel.
 * Note that a tunnel is not subordinate to a virtual network. A single tunnel
 * may handle traffic for multiple virtual networks.
 *****************************************************************************/
typedef struct _fm_vnTunnel
{
    /* Tunnel ID */
    fm_int            tunnelId;

    /* Tunnel Type */
    fm_vnTunnelType   tunnelType;

    /* Unique Traffic Identifier */
    fm_int            trafficIdentifier;

    /* Local IP Address */
    fm_ipAddr         localIp;

    /* Remote IP Address */
    fm_ipAddr         remoteIp;

    /* Virtual Router ID */
    fm_int            vrid;

    /* Remote IP Virtual Router ID
     * This is the vrid used when the remote IP address was added to the
     * vnTunnelsByIp tree. Since the vrid may be changed before the
     * remote IP address is changed, we have to keep a backup copy of
     * the value. */
    fm_int            remoteIpVrid;

    /* Multicast Group Number */
    fm_int            mcastGroup;

    /* Multicast DMAC */
    fm_macaddr        mcastDmac;

    /* Encapsulation TTL Value. */
    fm_uint           encapTTL;

    /* Pointer to the route that services this tunnel */
    fm_intRouteEntry *route;

    /* Pointer to a switch-specific extension structure */
    void *            extension;

} fm_vnTunnel;


fm_virtualNetwork *fmGetVN(fm_int sw, fm_uint32 vsId);
fm_vnTunnel *fmGetVNTunnel(fm_int sw, fm_int tunnelId);
fm_status fmVNAlloc(fm_int sw);
fm_status fmVNFree(fm_int sw);
fm_status fmVNInit(fm_int sw);
fm_status fmVNCleanup(fm_int sw);
fm_status fmNotifyVNTunnelAboutEcmpChange(fm_int sw, fm_intRouteEntry *route);
fm_status fmNotifyVNTunnelAboutRouteChange(fm_int sw);


#endif /* __FM_FM_API_VN_INT_H */

