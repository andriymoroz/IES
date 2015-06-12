/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_vn_int.h
 * Creation Date:   August 29, 2014
 * Description:     Contains constants and functions used to support virtual networks.
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

#ifndef __FM_FMSWAG_API_VN_INT_H
#define __FM_FMSWAG_API_VN_INT_H


#define FM_SWAG_VSI_TO_SW_NUM(swagVsi)  ( (swagVsi / FM_SWAG_MAX_VSI_PER_SWITCH) + FM_FIRST_FOCALPOINT )

#define FM_SWAG_VSI_TO_SW_VSI(swagVsi)  (swagVsi % FM_SWAG_MAX_VSI_PER_SWITCH)

#define FM_SW_VSI_TO_SWAG_VSI(sw, vsi)  ( ( (sw - FM_FIRST_FOCALPOINT) * FM_SWAG_MAX_VSI_PER_SWITCH ) + vsi)


/*****************************************************************************
 *
 *  Structure Definitions
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  VN Remote Address Information
 *
 *  This structure contains information describing a virtual network
 *  remote address.
 *
 *****************************************************************************/
typedef struct _fmSWAG_vnRemoteAddress
{
    /* Pointer to the fm_virtual Network record. */
    fm_virtualNetwork *vn;

    /* Pointer to the fm_vnTunnel record. */
    fm_vnTunnel *      tunnel;

    /* Remote Address. */
    fm_vnAddress       remoteAddress;

} fmSWAG_vnRemoteAddress;


/*****************************************************************************
 *
 *  Virtual Network Remote Address Mask
 *
 *  This structure contains information describing a virtual network
 *  remote address mask.
 *
 *****************************************************************************/
typedef struct _fmSWAG_vnRemoteAddressMask
{
    /* Pointer to the fm_virtual Network record. */
    fm_virtualNetwork *vn;

    /* Pointer to the fm_vnTunnel record or NULL. */
    fm_vnTunnel *      tunnel;

    /* Remote Address. */
    fm_vnAddress       remoteAddress;

    /* Address Bit Mask. */
    fm_vnAddress       addrMask;

} fmSWAG_vnRemoteAddressMask;


/*****************************************************************************
 *
 *  VN Local Port Information
 *
 *  This structure contains information describing a virtual network
 *  local port.
 *
 *****************************************************************************/
typedef struct _fmSWAG_vnLocalPort
{
    /* Pointer to the fm_virtual Network record. */
    fm_virtualNetwork *vn;

    /* Port Number. */
    fm_int             port;

} fmSWAG_vnLocalPort;


/*****************************************************************************
 *
 *  Virtual Network Information
 *
 *  This structure contains SWAG-specific information for a virtual network.
 *
 *****************************************************************************/
typedef struct _fmSWAG_virtualNetwork
{
    /* Pointer to the virtual network structure. */
    fm_virtualNetwork *vn;

    /* Tree of remote addresses associated with this virtual network. */
    fm_customTree      remoteAddresses;

    /* Tree of remote address masks. */
    fm_customTree      addressMasks;

    /* Tree of local ports. */
    fm_tree            localPorts;

} fmSWAG_virtualNetwork;


/*****************************************************************************
 *
 *  VN Tunnel information
 *
 *  This structure contains SWAG-specific information for a virtual network
 *  Tunnel.
 *
 *****************************************************************************/
typedef struct _fmSWAG_vnTunnel
{
    /* Pointer to the fm_vnTunnel structure. */
    fm_vnTunnel * tunnel;

} fmSWAG_vnTunnel;




/*****************************************************************************
 *
 *  Function Prototypes and Function Macros
 *
 *****************************************************************************/


fm_status fmSWAGConfigureVN(fm_int sw, fm_vnConfiguration *config);
fm_status fmSWAGCreateVirtualNetwork(fm_int sw, fm_virtualNetwork *vn);
fm_status fmSWAGDeleteVirtualNetwork(fm_int             sw,
                                     fm_virtualNetwork *vn);
fm_status fmSWAGUpdateVirtualNetwork(fm_int             sw,
                                     fm_virtualNetwork *vn,
                                     fm_vnDescriptor *  oldDescriptor);
fm_status fmSWAGFreeVirtualNetwork(fm_int             sw,
                                   fm_virtualNetwork *vn);
fm_status fmSWAGCreateVNTunnel(fm_int sw, fm_vnTunnel *tunnel);
fm_status fmSWAGDeleteVNTunnel(fm_int sw, fm_vnTunnel *tunnel);
fm_status fmSWAGSetVNTunnelAttribute(fm_int              sw,
                                     fm_vnTunnel *       tunnel,
                                     fm_vnTunnelAttrType attr,
                                     void *              value);
fm_status fmSWAGFreeVNTunnel(fm_int sw, fm_vnTunnel *tunnel);
fm_status fmSWAGAddVNRemoteAddress(fm_int             sw,
                                   fm_virtualNetwork *vn,
                                   fm_vnTunnel *      tunnel,
                                   fm_vnAddress *     addr);
fm_status fmSWAGDeleteVNRemoteAddress(fm_int             sw,
                                      fm_virtualNetwork *vn,
                                      fm_vnTunnel *      tunnel,
                                      fm_vnAddress *     addr);
fm_status fmSWAGAddVNRemoteAddressMask(fm_int             sw,
                                       fm_virtualNetwork *vn,
                                       fm_vnTunnel *      tunnel,
                                       fm_vnAddress *     baseAddr,
                                       fm_vnAddress *     addrMask);
fm_status fmSWAGDeleteVNRemoteAddressMask(fm_int             sw,
                                          fm_virtualNetwork *vn,
                                          fm_vnAddress *     baseAddr,
                                          fm_vnAddress *     addrMask);
fm_status fmSWAGAddVNLocalPort(fm_int             sw,
                               fm_virtualNetwork *vn,
                               fm_int             port);
fm_status fmSWAGDeleteVNLocalPort(fm_int             sw,
                                  fm_virtualNetwork *vn,
                                  fm_int             port);
fm_status fmSWAGApplyVNConfigToSwitch(fm_int swagId, fm_int sw);
fm_status fmSWAGAddVNVsi(fm_int sw, fm_virtualNetwork *vn, fm_int vsi);
fm_status fmSWAGDeleteVNVsi(fm_int sw, fm_virtualNetwork *vn, fm_int vsi);
fm_status fmSWAGFreeVNResources(fm_int sw);


#endif  /* __FM_FMSWAG_API_VN_INT_H */

