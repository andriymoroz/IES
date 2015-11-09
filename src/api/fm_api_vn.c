/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm_api_vn.c
 * Creation Date: Sep. 10, 2012
 * Description: Virtual Network Services
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

#include <fm_sdk_int.h>


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
static fm_status ConfigureTunnelRoute(fm_int            sw,
                                      fm_vnTunnel *     tunnel,
                                      fm_intRouteEntry *route);
static fm_status UnconfigureTunnelRoute(fm_int sw, fm_vnTunnel *tunnel);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** ConfigureTunnelRoute
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Configures a route and its related ECMP Group for use
 *                  as a virtual network tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the virtual network tunnel record.
 *
 * \param[in]       route points to the internal route record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureTunnelRoute(fm_int            sw,
                                      fm_vnTunnel *     tunnel,
                                      fm_intRouteEntry *route)
{
    fm_status  status;
    fm_switch *switchPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, route = %p, tunnel = %p\n",
                  sw,
                  (void *) route,
                  (void *) tunnel );

    switchPtr = GET_SWITCH_PTR(sw);

    if (tunnel->route != NULL)
    {
        /* Clean up the previous route */
        status = UnconfigureTunnelRoute(sw, tunnel);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    tunnel->route = route;

    if (route == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    if ( fmTreeSize(&route->vnTunnelsTree) == 0 )
    {
        status = fmCustomTreeInsert( &switchPtr->vnTunnelRoutes, route, route );
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmTreeInsert(&route->vnTunnelsTree, tunnel->tunnelId, tunnel);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end ConfigureTunnelRoute */




/*****************************************************************************/
/** UnconfigureTunnelRoute
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Unconfigures a route and its related ECMP Group from use
 *                  as a virtual network tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the virtual network tunnel record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UnconfigureTunnelRoute(fm_int sw, fm_vnTunnel *tunnel)
{
    fm_status         status;
    fm_switch *       switchPtr;
    fm_intRouteEntry *route;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p\n",
                  sw,
                  (void *) tunnel );

    switchPtr = GET_SWITCH_PTR(sw);
    route     = tunnel->route;
    status    = FM_OK;

    if (route != NULL)
    {
        /* Clean up the previous route */
        status = fmTreeRemove(&route->vnTunnelsTree, tunnel->tunnelId, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        if ( fmTreeSize(&route->vnTunnelsTree) == 0 )
        {
            status = fmCustomTreeRemove(&switchPtr->vnTunnelRoutes, route, NULL);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
        }

        tunnel->route = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end UnconfigureTunnelRoute */




/*****************************************************************************/
/** CreateVNTunnel
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Creates a Virtual Network Tunnel given the tunnel ID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelType contains the Tunnel type.
 *
 * \param[in]       tunnelId it the tunnel ID.
 *
 * \param[out]      tunnelPtrPtr points to caller-provided storage into
 *                  which the pointer to the allocated tunnel structure
 *                  will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TABLE_FULL if the hardware resources used to support
 *                  the tunnel are full.
 *
 *****************************************************************************/
fm_status CreateVNTunnel(fm_int          sw,
                         fm_vnTunnelType tunnelType,
                         fm_int          tunnelId,
                         fm_vnTunnel **  tunnelPtrPtr)
{
    fm_switch *  switchPtr;
    fm_status    status;
    fm_vnTunnel *tunnel;
    fm_bool      addedToTree;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnelType = %d, tunnelId = %d, tunnelPtrPtr = %p\n",
                  sw,
                  tunnelType,
                  tunnelId,
                  (void *) tunnelPtrPtr );

    switchPtr   = GET_SWITCH_PTR(sw);
    addedToTree = FALSE;

    /* Create and initialize a new tunnel record */
    tunnel = fmAlloc( sizeof(fm_vnTunnel) );

    if (tunnel == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_CLEAR(*tunnel);

    tunnel->tunnelId          = tunnelId;
    tunnel->tunnelType        = tunnelType;
    tunnel->mcastGroup        = -1;
    tunnel->trafficIdentifier = tunnelId + 1;
    tunnel->encapTTL          = ~0;

    FM_API_CALL_FAMILY(status, switchPtr->CreateVNTunnel, sw, tunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if ( (tunnelId != FM_VN_DECAPSULATION_VLAN2_NVGRE)
         && (tunnelId != FM_VN_DECAPSULATION_VLAN2_VXLAN) )
    {
        status = fmTreeInsert(&switchPtr->vnTunnels, tunnelId, tunnel);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        addedToTree = TRUE;
    }

    status = fmSetBitArrayBit(&switchPtr->vnTunnelsInUse, tunnelId, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (tunnelPtrPtr != NULL)
    {
        *tunnelPtrPtr = tunnel;
    }


ABORT:

    if (status != FM_OK)
    {
        if (tunnel != NULL)
        {
            if (addedToTree)
            {
                fmTreeRemove(&switchPtr->vnTunnels, tunnelId, NULL);
            }

            fmFree(tunnel);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end CreateVNTunnel */




/*****************************************************************************/
/** DeleteVNTunnel
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Deletes a Virtual Network tunnel.
 *
 * \note            All remote hosts using this tunnel will be deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnel points to the tunnel record
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_TUNNEL_IN_USE if the tunnel is in use.
 *
 *****************************************************************************/
static fm_status DeleteVNTunnel(fm_int sw, fm_vnTunnel *tunnel)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_bool    tunnelInUse;
    fm_int     tunnelId;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, tunnel = %p, tunnelId = %u\n",
                  sw,
                  (void *) tunnel,
                  tunnel->tunnelId );

    switchPtr = GET_SWITCH_PTR(sw);
    tunnelId  = tunnel->tunnelId;

    status = fmIsVNTunnelInUseByACLs(sw, tunnelId, &tunnelInUse);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if (tunnelInUse)
    {
        status = FM_ERR_TUNNEL_IN_USE;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->DeleteVNTunnel, sw, tunnel);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    status = UnconfigureTunnelRoute(sw, tunnel);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    if ( (tunnelId != FM_VN_DECAPSULATION_VLAN2_NVGRE)
         && (tunnelId != FM_VN_DECAPSULATION_VLAN2_VXLAN) )
    {
        status = fmTreeRemoveCertain(&switchPtr->vnTunnels, tunnelId, NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmSetBitArrayBit(&switchPtr->vnTunnelsInUse, tunnelId, FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    fmFree(tunnel);

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end DeleteVNTunnel */




/*****************************************************************************/
/** FreeVNTunnel
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Releases a tunnel record. Called during switch shutdown.
 *
 * \param[in]       tunPtr points to the tunnel record.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void FreeVNTunnel(void *tunPtr)
{
    fm_vnTunnel *tunnel;

    tunnel = tunPtr;

    if (tunnel->extension != NULL)
    {
        fmFree(tunnel->extension);
    }

    fmFree(tunnel);

}   /* end FreeVNTunnel */




/*****************************************************************************/
/** GetVNList
 * \ingroup intVN
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Returns a list of Virtual Networks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       maxVNs is the size of vsidList and internalIdList, being
 *                  the maximum number of Virtual Networks that can be
 *                  contained inside vsidList and internalIdList.
 *
 * \param[out]      numVNs points to caller-provided storage into which
 *                  will be stored the number of virtual networks stored in
 *                  vlanList and vsidList.
 *
 * \param[out]      vsidList is an array, maxVNs elements in length, that this
 *                  function will fill with the list of virtual subscriber IDs.
 *
 * \param[in]       descriptorList points to an array, maxVNs elements in length,
 *                  that this function will fill with the descriptor records
 *                  associated with each virtual network returned in vsidList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no virtual networks.
 * \return          FM_ERR_BUFFER_FULL if maxVNs was too small to accommodate
 *                  the entire list of virtual networks.
 *
 *****************************************************************************/
static fm_status GetVNList(fm_int           sw,
                           fm_int           maxVNs,
                           fm_int *         numVNs,
                           fm_uint32 *      vsidList,
                           fm_vnDescriptor *descriptorList)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_uint64          vsid;
    fm_virtualNetwork *vn;
    fm_int             curVN;
    fm_treeIterator    iter;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, maxVNs = %d, numVNs = %p, vsidList = %p, "
                  "descriptorList = %p\n",
                  sw,
                  maxVNs,
                  (void *) numVNs,
                  (void *) vsidList,
                  (void *) descriptorList );

    switchPtr = GET_SWITCH_PTR(sw);
    curVN     = 0;

    fmTreeIterInit(&iter, &switchPtr->virtualNetworks);

    while (1)
    {
        status = fmTreeIterNext( &iter, &vsid, (void **) &vn );

        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        if (curVN >= maxVNs)
        {
            status = FM_ERR_BUFFER_FULL;
            break;
        }

        vsidList[curVN]       = (fm_uint32) vsid;
        descriptorList[curVN] = vn->descriptor;

        ++curVN;
    }

ABORT:

    *numVNs = curVN;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end GetVNList */




/*****************************************************************************/
/** GetVNTunnelList
 * \ingroup intVN
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Returns a list of virtual network tunnels.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       maxTunnels is the size of tunnelIds, being the maximum
 *                  number of tunnels that can be contained inside the array.
 *
 * \param[out]      numTunnels points to caller-provided storage into which
 *                  will be stored the number of tunnels returned.
 *
 * \param[out]      tunnelIds is an array, maxTunnels in length, that this
 *                  function will fill with the IDs for each tunnel.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 * \return          FM_ERR_BUFFER_FULL if maxTunnels was too small to
 *                  accomodate the entire list of virtual network tunnels.
 *
 *****************************************************************************/
static fm_status GetVNTunnelList(fm_int  sw,
                                 fm_int  maxTunnels,
                                 fm_int *numTunnels,
                                 fm_int *tunnelIds)
{
    fm_switch *     switchPtr;
    fm_status       status;
    fm_uint64       tunId64;
    fm_vnTunnel *   tunnel;
    fm_int          index;
    fm_treeIterator iter;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, maxTunnels = %d, numTunnels = %p, "
                  "tunnelIds = %p\n",
                  sw,
                  maxTunnels,
                  (void *) numTunnels,
                  (void *) tunnelIds );

    switchPtr = GET_SWITCH_PTR(sw);
    index     = 0;

    fmTreeIterInit(&iter, &switchPtr->vnTunnels);

    while (1)
    {
        status = fmTreeIterNext( &iter, &tunId64, (void **) &tunnel );

        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

        if (index >= maxTunnels)
        {
            status = FM_ERR_BUFFER_FULL;
            break;
        }

        tunnelIds[index] = (fm_uint32) tunId64;

        ++index;
    }


ABORT:

    *numTunnels = index;

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end GetVNTunnelList */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCreateVN
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Creates a Virtual Network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vsId is the 24-bit virtual network ID
 *
 * \param[in]       descriptor points to a record that contains descriptive
 *                  information about the virtual network. To ensure
 *                  compatibility with future changes, this record
 *                  should be pre-initialized by calling ''FM_CLEAR''
 *                  before initialization of individual fields.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_ALREADY_EXISTS if a virtual network already exists
 *                  using this vsId or if the internalID is already in use
 *                  for another virtual network.
 * \return          FM_ERR_TABLE_FULL if all available virtual networks are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_INVALID_ARGUMENT if any of the arguments are invalid.
 *
 *****************************************************************************/
fm_status fmCreateVN(fm_int           sw,
                     fm_uint32        vsId,
                     fm_vnDescriptor *descriptor)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_virtualNetwork *vn;
    fm_bool            routingLockTaken;
    fm_bool            addedToTree;
    fm_bool            lbgLockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vsId = %u, descriptor=%p\n",
                      sw,
                      vsId,
                      (void *) descriptor );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    vn               = NULL;
    addedToTree      = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (descriptor == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_VN, "internalId = %u\n", descriptor->internalId);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (vsId & 0xff000000) != 0 )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    if ( (descriptor->internalId >= FM_MAX_NUM_VNS)
         && (descriptor->internalId != (fm_uint) ~0) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Determine if the vsId is already in use */
    vn = fmGetVN(sw, vsId);

    if (vn != NULL)
    {
        /* VN already exists */
        vn     = NULL;      /* Don't free the existing record */
        status = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Determine if the internal ID is already in use. */
    if ( (descriptor->internalId != (fm_uint) ~0)
         && (switchPtr->vnInternalIds[descriptor->internalId] != NULL) )
    {
        vn     = NULL;      /* Don't free the existing record */
        status = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Create and initialize a new virtual network record */
    vn = fmAlloc( sizeof(fm_virtualNetwork) );

    if (vn == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_CLEAR(*vn);
    vn->vsId       = vsId;
    vn->descriptor = *descriptor;

    status = fmTreeInsert(&switchPtr->virtualNetworks, vsId, vn);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    addedToTree = TRUE;

    if (descriptor->internalId != (fm_uint) ~0)
    {
        switchPtr->vnInternalIds[descriptor->internalId] = vn;
    }

    FM_API_CALL_FAMILY(status, switchPtr->CreateVirtualNetwork, sw, vn);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);


ABORT:

    if (status != FM_OK)
    {
        if (vn != NULL)
        {
            if (addedToTree)
            {
                fmTreeRemoveCertain(&switchPtr->virtualNetworks, vsId, NULL);

                if ( descriptor->internalId != (fm_uint) ~0 )
                {
                    switchPtr->vnInternalIds[descriptor->internalId] = NULL;
                }
            }

            fmFree(vn);
        }
    }

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmCreateVN */




/*****************************************************************************/
/** fmDeleteVN
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Deletes a Virtual Network, removing the association
 *                  between the vsId and the internalId.
 *
 * \note            All remote hosts using this virtual network will be deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vsId is the 24-bit NVGRE virtual subscriber ID or VxLAN
 *                  Network Identifier
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vsId is not associated with a
 *                  virtual network.
 * \return          FM_ERR_VIRTUAL_NETWORK_IN_USE if the virtual network is in use.
 *
 *****************************************************************************/
fm_status fmDeleteVN(fm_int sw, fm_uint32 vsId)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_virtualNetwork *vn;
    fm_bool            routingLockTaken;
    fm_bool            lbgLockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d, vsId = %u\n", sw, vsId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Find the VN record */
    vn = fmGetVN(sw, vsId);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->DeleteVirtualNetwork, sw, vn);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    status = fmTreeRemoveCertain(&switchPtr->virtualNetworks, vsId, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (vn->descriptor.internalId != (fm_uint) ~0)
    {
        switchPtr->vnInternalIds[vn->descriptor.internalId] = NULL;
    }

    fmFree(vn);


ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDeleteVN */




/*****************************************************************************/
/** fmUpdateVN
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Updates a Virtual Network in the virtual network table by
 *                  changing the internalId associated with a virtual network
 *                  subscriber ID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vsId is the 24-bit NVGRE virtual subscriber ID or VxLAN
 *                  Network Identifier
 *
 * \param[in]       descriptor points to a record that contains descriptive
 *                  information about the virtual network. To ensure
 *                  compatibility with future changes, this record
 *                  should be pre-initialized by calling ''FM_CLEAR''
 *                  before initialization of individual fields.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_TABLE_FULL if all available virtual networks are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_INVALID_ARGUMENT if bits 24-31 of vsId are non-zero
 *                  or bits 12-31 of internalId are non-zero.
 * \return          FM_ERR_ALREADY_EXISTS if the specified internal-id is
 *                  already in use.
 *
 *****************************************************************************/
fm_status fmUpdateVN(fm_int           sw,
                     fm_uint32        vsId,
                     fm_vnDescriptor *descriptor)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_virtualNetwork *vn;
    fm_bool            routingLockTaken;
    fm_bool            lbgLockTaken;
    fm_vnDescriptor    oldDescriptor;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vsId = %u, descriptor = %p\n",
                      sw,
                      vsId,
                      (void *) descriptor );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (descriptor == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_VN, "internalId = %u\n", descriptor->internalId);

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Find the VN record */
    vn = fmGetVN(sw, vsId);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Determine if the new descriptor is identical to the existing one */
    if (descriptor->internalId == vn->descriptor.internalId)
    {
        /* Nothing to do */
        status = FM_OK;
        goto ABORT;
    }

    /* Determine if the new internal ID is already in use. */
    if ( (descriptor->internalId != (fm_uint) ~0 )
         && (switchPtr->vnInternalIds[descriptor->internalId] != NULL) )
    {
        status = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    oldDescriptor  = vn->descriptor;
    vn->descriptor = *descriptor;

    FM_API_CALL_FAMILY(status, switchPtr->UpdateVirtualNetwork, sw, vn, &oldDescriptor);

    if (status != FM_OK)
    {
        vn->descriptor = oldDescriptor;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (oldDescriptor.internalId != (fm_uint) ~0)
    {
        switchPtr->vnInternalIds[oldDescriptor.internalId]  = NULL;
    }

    if (descriptor->internalId != (fm_uint) ~0)
    {
        switchPtr->vnInternalIds[descriptor->internalId] = vn;
    }


ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmUpdateVN */




/*****************************************************************************/
/** fmCreateVNTunnel
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Creates a Virtual Network Tunnel, which is a source and
 *                  destination of encapsulated packets for virtual networks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelType contains the Tunnel type.
 *
 * \param[out]      tunnelId points to caller-provided storage into which the
 *                  ID value used to identify this tunnel will be written.
 *                  This tunnel ID is used in the IP_TUNNEL ACL action to link
 *                  each tunneling ACL rule to the appropriate tunnel.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if tunnelType is invalid or if
 *                  tunnelId is NULL.
 * \return          FM_ERR_NO_FFU_RES_FOUND if the configured next-hop
 *                  resources cannot be reserved. This should only be
 *                  possible on the first call to this function after
 *                  booting the switch, or if, after all tunnels have been
 *                  deleted, the API attribute is changed before creating
 *                  any new tunnels.
 * \return          FM_ERR_TABLE_FULL if the hardware resources used to support
 *                  the tunnel are full.
 *
 *****************************************************************************/
fm_status fmCreateVNTunnel(fm_int          sw,
                           fm_vnTunnelType tunnelType,
                           fm_int *        tunnelId)
{
    fm_switch *  switchPtr;
    fm_status    status;
    fm_vnTunnel *tunnel;
    fm_bool      lockTaken;
    fm_int       index;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, tunnelType = %d, tunnelId = %p\n",
                      sw,
                      tunnelType,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (tunnelId == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    if ( ( (tunnelType == FM_VN_TUNNEL_TYPE_VXLAN_IPV4) ||
           (tunnelType == FM_VN_TUNNEL_TYPE_VXLAN_IPV6) )
         && (switchPtr->switchFamily == FM_SWITCH_FAMILY_FM6000)
         && (switchPtr->vxlanDecapsulationTunnel == NULL) )
    {
        /**************************************************
         * Create and configure pseudo-tunnel for
         * FM_VN_DECAPSULATION_VLAN2_VXLAN.
         **************************************************/
        status = CreateVNTunnel(sw,
                                FM_VN_TUNNEL_TYPE_MAX,
                                FM_VN_DECAPSULATION_VLAN2_VXLAN,
                                &switchPtr->vxlanDecapsulationTunnel);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (tunnelType == FM_VN_TUNNEL_TYPE_NVGRE)
         && (switchPtr->switchFamily == FM_SWITCH_FAMILY_FM6000)
         && (switchPtr->nvgreDecapsulationTunnel == NULL) )
    {
        /**************************************************
         * Create and configure pseudo-tunnel for
         * FM_VN_DECAPSULATION_VLAN2_NVGRE.
         **************************************************/
        status = CreateVNTunnel(sw,
                                FM_VN_TUNNEL_TYPE_MAX,
                                FM_VN_DECAPSULATION_VLAN2_NVGRE,
                                &switchPtr->nvgreDecapsulationTunnel);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Find an available tunnel ID */
    status = fmFindBitInBitArray(&switchPtr->vnTunnelsInUse, 1, FALSE, &index);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if (index < 0)
    {
        status = FM_ERR_TABLE_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Create and initialize a new tunnel record */
    status = CreateVNTunnel(sw, tunnelType, index, &tunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    *tunnelId = index;


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmCreateVNTunnel */




/*****************************************************************************/
/** fmDeleteVNTunnel
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Deletes a Virtual Network tunnel.
 *
 * \note            All remote hosts using this tunnel will be deleted.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the ID number of the tunnel to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if the specified tunnel ID is not
 *                  a valid tunnel.
 * \return          FM_ERR_TUNNEL_IN_USE if the tunnel is in use.
 *
 *****************************************************************************/
fm_status fmDeleteVNTunnel(fm_int sw, fm_int tunnelId)
{
    fm_switch *  switchPtr;
    fm_status    status;
    fm_vnTunnel *tunnel;
    fm_bool      lockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d, tunnelId = %u\n", sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    lockTaken = FALSE;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;
    tunnel    = fmGetVNTunnel(sw, tunnelId);

    if (tunnel == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = DeleteVNTunnel(sw, tunnel);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    if ( fmTreeSize(&switchPtr->vnTunnels) == 0 )
    {
        if (switchPtr->vxlanDecapsulationTunnel != NULL)
        {
            /**************************************************
             * Delete pseudo-tunnel for FM_VN_DECAPSULATION_VLAN2_VXLAN.
             **************************************************/
            status = DeleteVNTunnel(sw, switchPtr->vxlanDecapsulationTunnel);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            switchPtr->vxlanDecapsulationTunnel = NULL;
        }

        if (switchPtr->nvgreDecapsulationTunnel != NULL)
        {
            /**************************************************
             * Delete pseudo-tunnel for FM_VN_DECAPSULATION_VLAN2_NVGRE.
             **************************************************/
            status = DeleteVNTunnel(sw, switchPtr->nvgreDecapsulationTunnel);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            switchPtr->nvgreDecapsulationTunnel = NULL;
        }
    }

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDeleteVNTunnel */




/*****************************************************************************/
/** fmSetVNTunnelAttribute
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Modifies a virtual network tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel Id.
 *
 * \param[in]       attr is the tunnel attribute type. See ''fm_vnTunnelAttrType''.
 *
 * \param[in]       value points to the attribute's new value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ATTRIB if the attribute type is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if tunnelId is not a valid tunnel
 *                  ID for the virtual network or if value is NULL.
 * \return          FM_ERR_NO_ROUTE_TO_HOST if there is no route to the
 *                  destination IP address.
 * \return          FM_ERR_TABLE_FULL if there is no room in the tunneling
 *                  hardware for the default tunnel rule for this tunnel.
 *
 *****************************************************************************/
fm_status fmSetVNTunnelAttribute(fm_int              sw,
                                 fm_int              tunnelId,
                                 fm_vnTunnelAttrType attr,
                                 void *              value)
{
    fm_switch *       switchPtr;
    fm_status         status;
    fm_vnTunnel *     tunnel;
    fm_bool           routingLockTaken;
    fm_bool           lbgLockTaken;
    fm_intRouteEntry *route;
    fm_int            vrid;
    fm_intRouteEntry *oldRoute;
    fm_ipAddr         oldRemoteIp;
    fm_int            oldVrid;
    fm_ipAddr         remoteIp;
    fm_bool           remoteIpChanged;
    fm_uint           encapTTL;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, tunnelId = %d, attr = %d, value=%p\n",
                     sw,
                     tunnelId,
                     attr,
                     value);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    remoteIpChanged  = FALSE;
    FM_CLEAR(oldRemoteIp);
    oldVrid          = 0;
    oldRoute         = NULL;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken   = TRUE;
    tunnel      = fmGetVNTunnel(sw, tunnelId);

    if (tunnel == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    switch (attr)
    {
        case FM_VNTUNNEL_ATTR_LOCAL_IP:
            tunnel->localIp = *( (fm_ipAddr *) value );
            break;

        case FM_VNTUNNEL_ATTR_REMOTE_IP:
            remoteIp = *( (fm_ipAddr *) value );

            if ( !fmIsIPAddressEmpty(&remoteIp) )
            {
                if ( fmIsUnicastIPAddress(&remoteIp) )
                {
                    /* Try to find a route */
                    status = fmGetIntRouteForIP(sw,
                                                tunnel->vrid,
                                                &remoteIp,
                                                &route);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
                }
                else
                {
                    route = NULL;
                }
            }
            else
            {
                route = NULL;
            }

            oldRoute             = tunnel->route;
            oldRemoteIp          = tunnel->remoteIp;
            oldVrid              = tunnel->remoteIpVrid;
            tunnel->remoteIp     = remoteIp;
            tunnel->remoteIpVrid = tunnel->vrid;
            remoteIpChanged      = TRUE;

            if (route != NULL)
            {
                status = ConfigureTunnelRoute(sw, tunnel, route);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }
            else
            {
                status = UnconfigureTunnelRoute(sw, tunnel);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }
            break;

        case FM_VNTUNNEL_ATTR_VRID:
            vrid = *( (fm_int *) value );
            status = fmValidateVirtualRouterId(sw, vrid, NULL);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

            tunnel->vrid = vrid;
            break;

        case FM_VNTUNNEL_ATTR_MCAST_GROUP:
            tunnel->mcastGroup = *( (fm_int *) value );
            break;

        case FM_VNTUNNEL_ATTR_MCAST_DMAC:
            tunnel->mcastDmac = *( (fm_macaddr *) value );
            break;

        case FM_VNTUNNEL_ATTR_TRAFFIC_IDENTIFIER:
            /* Set by the family-specific function */
            break;

        case FM_VNTUNNEL_ATTR_TUNNEL_TYPE:
            /* This is a read-only attribute */
            status = FM_ERR_INVALID_ATTRIB;
            break;

        case FM_VNTUNNEL_ATTR_ENCAP_TTL:
            encapTTL = *( (fm_uint *) value );
            if ( (encapTTL > 255) && (encapTTL != (fm_uint) ~0) )
            {
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            tunnel->encapTTL = encapTTL;
            break;

        case FM_VNTUNNEL_ATTR_NSH_BASE_HDR:
            if (tunnel->tunnelType != FM_VN_TUNNEL_TYPE_GPE_NSH)
            {
                status = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            tunnel->nsh.baseHdr = *( (fm_vnNshBaseHdr *) value );
            break;

        case FM_VNTUNNEL_ATTR_NSH_SERVICE_HDR:
            if (tunnel->tunnelType != FM_VN_TUNNEL_TYPE_GPE_NSH)
            {
                status = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            tunnel->nsh.serviceHdr = *( (fm_vnNshServiceHdr *) value );
            break;

        case FM_VNTUNNEL_ATTR_NSH_DATA:
            if (tunnel->tunnelType != FM_VN_TUNNEL_TYPE_GPE_NSH)
            {
                status = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            tunnel->nsh.context = *( (fm_vnNshData *) value );
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->SetVNTunnelAttribute,
                       sw,
                       tunnel,
                       attr,
                       value);


ABORT:

    if ( (status != FM_OK) && remoteIpChanged )
    {
        tunnel->remoteIp     = oldRemoteIp;
        tunnel->remoteIpVrid = oldVrid;

        if (oldRoute != NULL)
        {
            ConfigureTunnelRoute(sw, tunnel, oldRoute);
        }
        else
        {
            UnconfigureTunnelRoute(sw, tunnel);
        }
    }

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmSetVNTunnelAttribute */




/*****************************************************************************/
/** fmGetVNTunnelAttribute
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieves a virtual network tunnel attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel Id.
 *
 * \param[in]       attr is the tunnel attribute type. See ''fm_vnTunnelAttrType''.
 *
 * \param[out]      value points to caller-provided storage into which
 *                  the attributes value will be written. See
 *                  ''fm_vnTunnelAttrType'' for the data types of each attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if tunnelId is not a valid tunnel
 *                  ID for the virtual network, if the attribute type is
 *                  invalid, or if value is NULL.
 *
 *****************************************************************************/
fm_status fmGetVNTunnelAttribute(fm_int              sw,
                                 fm_int              tunnelId,
                                 fm_vnTunnelAttrType attr,
                                 void *              value)
{
    fm_switch *  switchPtr;
    fm_status    status;
    fm_vnTunnel *tunnel;
    fm_bool      lockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, tunnelId = %d, attr = %d, value=%p\n",
                     sw,
                     tunnelId,
                     attr,
                     value);

    lockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;
    tunnel    = fmGetVNTunnel(sw, tunnelId);

    if (tunnel == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = FM_OK;

    switch (attr)
    {
        case FM_VNTUNNEL_ATTR_REMOTE_IP:
            *( (fm_ipAddr *) value ) = tunnel->remoteIp;
            break;

        case FM_VNTUNNEL_ATTR_LOCAL_IP:
            *( (fm_ipAddr *) value ) = tunnel->localIp;
            break;

        case FM_VNTUNNEL_ATTR_VRID:
            *( (fm_int *) value ) = tunnel->vrid;
            break;

        case FM_VNTUNNEL_ATTR_MCAST_GROUP:
            *( (fm_int *) value ) = tunnel->mcastGroup;
            break;

        case FM_VNTUNNEL_ATTR_MCAST_DMAC:
            *( (fm_macaddr *) value ) = tunnel->mcastDmac;
            break;

        case FM_VNTUNNEL_ATTR_TRAFFIC_IDENTIFIER:
            *( (fm_int *) value ) = tunnel->trafficIdentifier;
            break;

        case FM_VNTUNNEL_ATTR_TUNNEL_TYPE:
            *( (fm_vnTunnelType *) value ) = tunnel->tunnelType;
            break;

        case FM_VNTUNNEL_ATTR_ENCAP_TTL:
            *( (fm_uint *) value ) = tunnel->encapTTL;
            break;

        case FM_VNTUNNEL_ATTR_NSH_BASE_HDR:
            if (tunnel->tunnelType != FM_VN_TUNNEL_TYPE_GPE_NSH)
            {
                status = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            *( (fm_vnNshBaseHdr *) value ) = tunnel->nsh.baseHdr;
            break;

        case FM_VNTUNNEL_ATTR_NSH_SERVICE_HDR:
            if (tunnel->tunnelType != FM_VN_TUNNEL_TYPE_GPE_NSH)
            {
                status = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            *( (fm_vnNshServiceHdr *) value ) = tunnel->nsh.serviceHdr;
            break;

        case FM_VNTUNNEL_ATTR_NSH_DATA:
            if (tunnel->tunnelType != FM_VN_TUNNEL_TYPE_GPE_NSH)
            {
                status = FM_ERR_INVALID_ATTRIB;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
            }

            *( (fm_vnNshData *) value ) = tunnel->nsh.context;
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNTunnelAttribute */




/*****************************************************************************/
/** fmGetVNList
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Returns a list of Virtual Networks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       maxVNs is the size of vsidList and internalIdList, being
 *                  the maximum number of Virtual Networks that can be
 *                  contained inside vsidList and internalIdList.
 *
 * \param[out]      numVNs points to caller-provided storage into which
 *                  will be stored the number of virtual networks stored in
 *                  vlanList and vsidList.
 *
 * \param[out]      vsidList is an array, maxVNs elements in length, that this
 *                  function will fill with the list of virtual subscriber IDs.
 *
 * \param[in]       descriptorList points to an array, maxVNs elements in length,
 *                  that this function will fill with the descriptor records
 *                  associated with each virtual network returned in vsidList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if maxVNs is <= 0 or any of the
 *                  pointer arguments are NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual networks.
 * \return          FM_ERR_BUFFER_FULL if maxVNs was too small to accommodate
 *                  the entire list of virtual networks.
 *
 *****************************************************************************/
fm_status fmGetVNList(fm_int           sw,
                      fm_int           maxVNs,
                      fm_int *         numVNs,
                      fm_uint32 *      vsidList,
                      fm_vnDescriptor *descriptorList)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, maxVNs = %d, numVNs = %p, vsidList = %p, "
                      "descriptorList = %p\n",
                      sw,
                      maxVNs,
                      (void *) numVNs,
                      (void *) vsidList,
                      (void *) descriptorList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (numVNs == NULL) || (vsidList == NULL) || (descriptorList == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    status = GetVNList(sw, maxVNs, numVNs, vsidList, descriptorList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNList */




/*****************************************************************************/
/** fmGetVNFirst
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Gets the first virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      searchToken points to caller-provided storage into which
 *                  the function will place a search token used for future
 *                  calls to fmGetVNNext.
 *
 * \param[out]      vsId points to caller-provided storage into which the
 *                  function will write the first virtual subscriber ID.
 *
 * \param[out]      descriptor points to caller-provided storage into which
 *                  the descriptive information for the virtual network will
 *                  be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual networks.
 *
 *****************************************************************************/
fm_status fmGetVNFirst(fm_int           sw,
                       fm_int *         searchToken,
                       fm_uint32 *      vsId,
                       fm_vnDescriptor *descriptor)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_uint64          vsid64;
    fm_virtualNetwork *vn;
    fm_treeIterator    iter;
    fm_bool            lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, searchToken = %p, vsid = %p, descriptor = %p\n",
                      sw,
                      (void *) searchToken,
                      (void *) vsId,
                      (void *) descriptor );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (vsId == NULL) || (descriptor == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    fmTreeIterInit(&iter, &switchPtr->virtualNetworks);

    status = fmTreeIterNext( &iter, &vsid64, (void **) &vn );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    *vsId       = (fm_uint32) vsid64;
    *descriptor = vn->descriptor;

    *searchToken = *vsId;


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNFirst */




/*****************************************************************************/
/** fmGetVNNext
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Gets the next virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   searchToken points to caller-provided storage containing
 *                  a search token provided by an earlier call to
 *                  ''fmGetVNFirst'' or  ''fmGetVNNext''.
 *
 * \param[out]      vsId points to caller-provided storage into which the
 *                  function will write the next virtual subscriber ID.
 *
 * \param[out]      descriptor points to caller-provided storage into which
 *                  the descriptive information for the virtual network will
 *                  be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no more virtual networks.
 *
 *****************************************************************************/
fm_status fmGetVNNext(fm_int           sw,
                      fm_int *         searchToken,
                      fm_uint32 *      vsId,
                      fm_vnDescriptor *descriptor)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_uint64          vsid64;
    fm_virtualNetwork *vn;
    fm_treeIterator    iter;
    fm_bool            lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, searchToken = %p, vsid = %p, descriptor = %p\n",
                      sw,
                      (void *) searchToken,
                      (void *) vsId,
                      (void *) descriptor );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (vsId == NULL) || (descriptor == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    vsid64 = *searchToken;

    status = fmTreeIterInitFromKey(&iter,
                                   &switchPtr->virtualNetworks,
                                   vsid64);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* get the previous virtual network */
    status = fmTreeIterNext( &iter, &vsid64, (void **) &vn );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* get the next virtual network */
    status = fmTreeIterNext( &iter, &vsid64, (void **) &vn );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    *vsId       = (fm_uint32) vsid64;
    *descriptor = vn->descriptor;

    *searchToken = *vsId;


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNNext */




/*****************************************************************************/
/** fmGetVNTunnelList
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Returns a list of virtual network tunnels.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       maxTunnels is the size of tunnelIds, being the maximum
 *                  number of tunnels that can be contained inside the array.
 *
 * \param[out]      numTunnels points to caller-provided storage into which
 *                  will be stored the number of tunnels returned.
 *
 * \param[out]      tunnelIds is an array, maxTunnels in length, that this
 *                  function will fill with the IDs for each tunnel.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if maxTunnels is <= 0,
 *                  or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 * \return          FM_ERR_BUFFER_FULL if maxTunnels was too small to
 *                  accomodate the entire list of virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetVNTunnelList(fm_int  sw,
                            fm_int  maxTunnels,
                            fm_int *numTunnels,
                            fm_int *tunnelIds)
{
    fm_switch *     switchPtr;
    fm_status       status;
    fm_bool         lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, maxTunnels = %d, numTunnels = %p, "
                      "tunnelIds = %p\n",
                      sw,
                      maxTunnels,
                      (void *) numTunnels,
                      (void *) tunnelIds );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (numTunnels == NULL) || (tunnelIds == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    status = GetVNTunnelList(sw, maxTunnels, numTunnels, tunnelIds);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNTunnelList */




/*****************************************************************************/
/** fmGetVNTunnelFirst
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Gets the first virtual network tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      searchToken points to caller-provided storage into which
 *                  the function will place a search token used for future
 *                  calls to ''fmGetVNTunnelNext''.
 *
 * \param[out]      tunnelId points to caller-provided storage into which the
 *                  function will write the first virtual network tunnel ID.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetVNTunnelFirst(fm_int  sw,
                             fm_int *searchToken,
                             fm_int *tunnelId)
{
    fm_switch *     switchPtr;
    fm_status       status;
    fm_uint64       tunId64;
    fm_vnTunnel *   tunnel;
    fm_treeIterator iter;
    fm_bool         lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, searchToken = %p, tunnelId = %p\n",
                      sw,
                      (void *) searchToken,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (tunnelId == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    fmTreeIterInit(&iter, &switchPtr->vnTunnels);

    status = fmTreeIterNext( &iter, &tunId64, (void **) &tunnel );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    *tunnelId    = (fm_uint32) tunId64;
    *searchToken = *tunnelId;


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNTunnelFirst */




/*****************************************************************************/
/** fmGetVNTunnelNext
 * \ingroup virtualNetwork
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Gets the next virtual network tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   searchToken points to caller-provided storage containing
 *                  a search token provided by an earlier call to
 *                  ''fmGetVNTunnelFirst'' or  ''fmGetVNTunnelNext''.
 *
 * \param[out]      tunnelId points to caller-provided storage into which the
 *                  function will write the tunneld ID for the next virtual
 *                  network tunnel.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no more virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetVNTunnelNext(fm_int  sw,
                            fm_int *searchToken,
                            fm_int *tunnelId)
{
    fm_switch *     switchPtr;
    fm_status       status;
    fm_uint64       tunId64;
    fm_vnTunnel *   tunnel;
    fm_treeIterator iter;
    fm_bool         lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, searchToken = %p, tunnelId = %p\n",
                      sw,
                      (void *) searchToken,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (tunnelId == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    tunId64 = *searchToken;

    status = fmTreeIterInitFromKey(&iter, &switchPtr->vnTunnels, tunId64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Get the previous record */
    status = fmTreeIterNext( &iter, &tunId64, (void **) &tunnel );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    /* Get the next record */
    status = fmTreeIterNext( &iter, &tunId64, (void **) &tunnel );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    *tunnelId    = (fm_uint32) tunId64;
    *searchToken = *tunnelId;


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNTunnelNext */




/*****************************************************************************/
/** fmVNAlloc
 * \ingroup intRouter
 *
 * \desc            Allocate resources needed by the virtual network
 *                  subsystem for a switch. Called during switch insertion.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNINITIALIZED if API attribute tree was not
 *                  initialized.
 * \return          FM_ERR_NOT_FOUND if API attribute does not exist in
 *                  attributeTree.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmVNAlloc(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    status = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels > 0)
    {
        /**************************************************
        * Virtual Networking requires that the routing
        * subsystem support the route lookup feature.
        **************************************************/
        GET_PROPERTY()->supportRouteLookups = TRUE; 
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fmVNAlloc */




/*****************************************************************************/
/** fmVNFree
 * \ingroup intRouter
 *
 * \desc            Release all virtual network resources held by a switch.
 *                  Called during switch removal.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmVNFree(fm_int sw)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /* Nothing to do */

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fmVNFree */




/*****************************************************************************/
/** fmVNInit
 * \ingroup intRouter
 *
 * \desc            Perform initialization for virtual network subsystem.
 *                  Called at switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if insufficient next-hops have
 *                  been reserved for tunnel use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 *
 *****************************************************************************/
fm_status fmVNInit(fm_int sw)
{
    fm_switch *  switchPtr;
    fm_status    status;
    fm_int       i;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /**************************************************
     * Allocate memory.
     **************************************************/
    i = sizeof(fm_virtualNetwork *) * FM_MAX_NUM_VNS;

    switchPtr->vnInternalIds = fmAlloc(i);

    if (switchPtr->vnInternalIds == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_ERR_NO_MEM);
    }

    FM_MEMSET_S(switchPtr->vnInternalIds, i, 0, i);

    /**************************************************
     * Create trees
     **************************************************/
    fmTreeInit(&switchPtr->virtualNetworks);
    fmTreeInit(&switchPtr->vnTunnels);
    fmCustomTreeInit(&switchPtr->vnTunnelRoutes, fmCompareIntRoutes);

    /**************************************************
     * Create bit arrays
     **************************************************/
    status = fmCreateBitArray(&switchPtr->vnTunnelsInUse,
                              switchPtr->maxVNTunnels);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /**************************************************
     * VN Tunnel Id 0 should not be used as it is reserved.
     **************************************************/
    status = fmSetBitArrayBit(&switchPtr->vnTunnelsInUse,
                              0,
                              TRUE);

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fmVNInit */



/*****************************************************************************/
/** fmVNCleanup
 * \ingroup intRouter
 *
 * \desc            Releases memory used by the virtual networking subsystem
 *                  to support a specified switch.
 *                  Called when a switch is going down.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmVNCleanup(fm_int sw)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_treeIterator    iter1;
    fm_virtualNetwork *vn;
    fm_uint64          vnKey;
    fm_vnTunnel *      tunnel;
    fm_uint64          tunnelKey;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    /**************************************************
     * Destroy trees
     **************************************************/
    if ( fmCustomTreeIsInitialized(&switchPtr->vnTunnelRoutes) )
    {
        fmCustomTreeDestroy(&switchPtr->vnTunnelRoutes, NULL);
    }

    if ( fmTreeIsInitialized(&switchPtr->virtualNetworks) )
    {
        fmTreeIterInit(&iter1, &switchPtr->virtualNetworks);

        while ( (status = fmTreeIterNext(&iter1, &vnKey, (void **) &vn ) ) == FM_OK )
        {
            if (vn->extension != NULL)
            {
                if (switchPtr->FreeVirtualNetwork != NULL)
                {
                    switchPtr->FreeVirtualNetwork(sw, vn);
                }
                else
                {
                    fmFree(vn->extension);
                    vn->extension = NULL;
                }
            }
        }

        fmTreeDestroy(&switchPtr->virtualNetworks, fmFree);
    }

    if ( fmTreeIsInitialized( &switchPtr->vnTunnels) )
    {
        fmTreeIterInit(&iter1, &switchPtr->vnTunnels);

        while ( (status = fmTreeIterNext(&iter1, &tunnelKey, (void **) &tunnel ) ) == FM_OK )
        {
            if (tunnel->extension != NULL)
            {
                if (switchPtr->FreeVNTunnel != NULL)
                {
                    switchPtr->FreeVNTunnel(sw, tunnel);
                }
            }
        }

        fmTreeDestroy(&switchPtr->vnTunnels, FreeVNTunnel);
    }

    if (switchPtr->FreeVNResources != NULL)
    {
        switchPtr->FreeVNResources(sw);
    }

    /**************************************************
     * Destroy bit arrays
     **************************************************/
    status = fmDeleteBitArray(&switchPtr->vnTunnelsInUse);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

    /**************************************************
     * Free memory.
     **************************************************/
    if (switchPtr->vnInternalIds != NULL)
    {
        fmFree(switchPtr->vnInternalIds);
        switchPtr->vnInternalIds = NULL;
    }

    if (switchPtr->vxlanDecapsulationTunnel != NULL)
    {
        FreeVNTunnel(switchPtr->vxlanDecapsulationTunnel);
    }

    if (switchPtr->nvgreDecapsulationTunnel != NULL)
    {
        FreeVNTunnel(switchPtr->nvgreDecapsulationTunnel);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);

}   /* end fmVNCleanup */




/*****************************************************************************/
/** fmGetVN
 * \ingroup intVN
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Given a vsId, returns the pointer to the virtual network
 *                  record.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vsId is the virtual network identifier.
 *
 * \return          Pointer to the virtual network record if successful.
 * \return          NULL if the virtual network record was not found.
 *
 *****************************************************************************/
fm_virtualNetwork *fmGetVN(fm_int sw, fm_uint32 vsId)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,"sw = %d, vsId = %d\n", sw, vsId);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        vn = NULL;
    }
    else
    {
        status = fmTreeFind( &switchPtr->virtualNetworks, vsId, (void **) &vn);

        if (status != FM_OK)
        {
            vn = NULL;
        }
    }

    FM_LOG_EXIT_CUSTOM( FM_LOG_CAT_VN, vn, "vn = %p\n", (void *) vn );

}   /* end fmGetVN */




/*****************************************************************************/
/** fmGetVNTunnel
 * \ingroup intVN
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Given a tunnel ID, returns the pointer to the tunnel record.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the tunnel identifier.
 *
 * \return          Pointer to the tunnel record if successful.
 * \return          NULL if the tunnel record was not found.
 *
 *****************************************************************************/
fm_vnTunnel *fmGetVNTunnel(fm_int sw, fm_int tunnelId)
{
    fm_switch *  switchPtr;
    fm_status    status;
    fm_vnTunnel *tunnel;

    FM_LOG_ENTRY(FM_LOG_CAT_VN,"sw = %d, tunnelId = %d\n", sw, tunnelId);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        tunnel = NULL;
    }
    else
    {
        status = fmTreeFind( &switchPtr->vnTunnels, tunnelId, (void **) &tunnel);

        if (status != FM_OK)
        {
            tunnel = NULL;
        }
    }

    FM_LOG_EXIT_CUSTOM( FM_LOG_CAT_VN, tunnel, "tunnel = %p\n", (void *) tunnel );

}   /* end fmGetVNTunnel */




/*****************************************************************************/
/** fmNotifyVNTunnelAboutEcmpChange
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Updates tunnel information for all tunnels that use
 *                  a specified route.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmNotifyVNTunnelAboutEcmpChange(fm_int sw, fm_intRouteEntry *route)
{
    fm_status       status;
    fm_switch *     switchPtr;
    fm_treeIterator iter;
    fm_uint64       tunnelId;
    fm_vnTunnel *   tunnel;

    FM_LOG_ENTRY( FM_LOG_CAT_VN,
                  "sw = %d, route = %p\n",
                  sw,
                  (void *) route );

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    if ( fmTreeSize(&route->vnTunnelsTree) == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    if (switchPtr->UpdateVNTunnelECMPGroup == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    fmTreeIterInit(&iter, &route->vnTunnelsTree);

    while (1)
    {
        status = fmTreeIterNext( &iter, &tunnelId, (void **) &tunnel );
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        status = switchPtr->UpdateVNTunnelECMPGroup(sw, tunnel);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fmNotifyVNTunnelAboutEcmpChange */




/*****************************************************************************/
/** fmNotifyVNTunnelAboutRouteChange
 * \ingroup intVN
 *
 * \chips           FM6000
 *
 * \desc            Updates tunnel routes when some change has occurred in
 *                  the routing table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmNotifyVNTunnelAboutRouteChange(fm_int sw)
{
    fm_status         status;
    fm_switch *       switchPtr;
    fm_treeIterator   iter;
    fm_uint64         key;
    fm_vnTunnel *     tunnel;
    fm_intRouteEntry *route;

    FM_LOG_ENTRY(FM_LOG_CAT_VN, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    if ( fmTreeSize(&switchPtr->vnTunnels) == 0 )
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    if (switchPtr->UpdateVNTunnelECMPGroup == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_VN, FM_OK);
    }

    fmTreeIterInit(&iter, &switchPtr->vnTunnels);

    while (1)
    {
        status = fmTreeIterNext( &iter,
                                 &key,
                                 (void **) &tunnel );
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

        if ( !fmIsIPAddressEmpty(&tunnel->remoteIp)
             && fmIsUnicastIPAddress(&tunnel->remoteIp) )
        {
            status = fmGetIntRouteForIP(sw, tunnel->vrid, &tunnel->remoteIp, &route);

            if (status == FM_ERR_NO_ROUTE_TO_HOST)
            {
                route = NULL;

                status = ConfigureTunnelRoute(sw, tunnel, route);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

                if (switchPtr->UpdateVNTunnelECMPGroup != NULL)
                {
                    status = switchPtr->UpdateVNTunnelECMPGroup(sw, tunnel);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
                }
            }
            else if (status != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_VN,
                              "Error %d (%s) while finding route for tunnel %d\n",
                              status,
                              fmErrorMsg(status),
                              tunnel->tunnelId );
                continue;
            }
        }
        else
        {
            route = NULL;
        }

        if (route != tunnel->route)
        {
            if (route != NULL)
            {
                status = ConfigureTunnelRoute(sw, tunnel, route);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);

                if (switchPtr->UpdateVNTunnelECMPGroup != NULL)
                {
                    status = switchPtr->UpdateVNTunnelECMPGroup(sw, tunnel);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_VN, status);
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_VN, status);

}   /* end fmNotifyVNTunnelAboutRouteChange */




/*****************************************************************************/
/** fmAddVNRemoteAddress
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Adds a remote address to a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       tunnelId is the tunnel Id.
 *
 * \param[in]       addr points to the address to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, if tunnelId is not a valid tunnel
 *                  ID, or if addr is NULL.
 *
 *****************************************************************************/
fm_status fmAddVNRemoteAddress(fm_int           sw,
                               fm_uint32        vni,
                               fm_int           tunnelId,
                               fm_vnAddress *   addr)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_virtualNetwork *vn;
    fm_vnTunnel *      tunnel;
    fm_bool            routingLockTaken;
    fm_bool            lbgLockTaken;
    fm_char            textAddr[100];

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, vni = %u, tunnelId = %d, addr = %p\n",
                     sw,
                     vni,
                     tunnelId,
                     (void *) addr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addr == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Get the Tunnel Record */
    tunnel = fmGetVNTunnel(sw, tunnelId);

    if (tunnel == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC)
    {
        fmDbgConvertMacAddressToString(addr->macAddress, textAddr);
    }
    else
    {
        fmDbgConvertIPAddressToString(&addr->ipAddress, textAddr);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_VN, "    address = %s\n", textAddr);

    FM_API_CALL_FAMILY(status, switchPtr->AddVNRemoteAddress, sw, vn, tunnel, addr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmAddVNRemoteAddress */




/*****************************************************************************/
/** fmDeleteVNRemoteAddress
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Deletes a remote address from a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       tunnelId is the tunnel Id.
 *
 * \param[in]       addr points to the address to be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, if tunnelId is not a valid tunnel
 *                  ID, or if addr is NULL.
 *
 *****************************************************************************/
fm_status fmDeleteVNRemoteAddress(fm_int           sw,
                                  fm_uint32        vni,
                                  fm_int           tunnelId,
                                  fm_vnAddress *   addr)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_virtualNetwork *vn;
    fm_vnTunnel *      tunnel;
    fm_bool            routingLockTaken;
    fm_bool            lbgLockTaken;
    fm_char            textAddr[100];

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, vni = %u, tunnelId = %d, addr = %p\n",
                     sw,
                     vni,
                     tunnelId,
                     (void *) addr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (addr == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    /* Get the Tunnel Record */
    tunnel = fmGetVNTunnel(sw, tunnelId);

    if (tunnel == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC)
    {
        fmDbgConvertMacAddressToString(addr->macAddress, textAddr);
    }
    else
    {
        fmDbgConvertIPAddressToString(&addr->ipAddress, textAddr);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_VN, "    address = %s\n", textAddr);

    FM_API_CALL_FAMILY(status, switchPtr->DeleteVNRemoteAddress, sw, vn, tunnel, addr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDeleteVNRemoteAddress */




/*****************************************************************************/
/** fmAddVNRemoteAddressMask
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Specifies a range of addresses which will be added to the remote
 *                  address mask table. By declaring a range, FFU resources may be used more
 *                  efficiently, since a single FFU rule can be used to direct the
 *                  entire range of traffic to the Tunneling Engine. Otherwise,
 *                  each remote address consumes a separate FFU rule for encapsulation.
 *                  Addresses in the range do not have to be served by the same tunnel.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       tunnelId is the tunnel ID for the address range. If -1,
 *                  no Tunneling Engine rule will be created for this address
 *                  range, and the addresses may be served by one or more
 *                  tunnels by adding each remote address individually. If
 *                  tunnelId is a valid tunnel ID, a single Tunneling Engine
 *                  rule will be created for the entire range and individual
 *                  remote addresses do not need to be added using
 *                  ''fmAddVNRemoteAddress''.
 *
 * \param[in]       baseAddr points to the base address in the range.
 *
 * \param[in]       addrMask points to the bit-mask to be applied to the
 *                  base address.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or if baseAddr is NULL.
 * \return          FM_ERR_INVALID_IN_VN_TRANSPARENT_MODE if the virtual
 *                  network is in transparent mode.
 *
 *****************************************************************************/
fm_status fmAddVNRemoteAddressMask(fm_int        sw,
                                   fm_uint32     vni,
                                   fm_int        tunnelId,
                                   fm_vnAddress *baseAddr,
                                   fm_vnAddress *addrMask)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_virtualNetwork *vn;
    fm_vnTunnel *      tunnel;
    fm_bool            routingLockTaken;
    fm_bool            lbgLockTaken;
    fm_char            textAddr1[100];
    fm_char            textAddr2[100];

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, vni = %u, tunnelId = %d, baseAddr = %p, addrMask = %p\n",
                     sw,
                     vni,
                     tunnelId,
                     (void *) baseAddr,
                     (void *) addrMask);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (baseAddr == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (tunnelId >= 0)
    {
        tunnel = fmGetVNTunnel(sw, tunnelId);

        if (tunnel == NULL)
        {
            /* Tunnel doesn't exist */
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
        }
    }
    else
    {
        tunnel = NULL;
    }

    if (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC)
    {
        fmDbgConvertMacAddressToString(baseAddr->macAddress, textAddr1);
        fmDbgConvertMacAddressToString(addrMask->macAddress, textAddr2);
    }
    else
    {
        fmDbgConvertIPAddressToString(&baseAddr->ipAddress, textAddr1);
        fmDbgConvertIPAddressToString(&addrMask->ipAddress, textAddr2);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_VN, "    baseAddr = %s, addrMask = %s\n", textAddr1, textAddr2);

    FM_API_CALL_FAMILY(status,
                       switchPtr->AddVNRemoteAddressMask,
                       sw,
                       vn,
                       tunnel,
                       baseAddr,
                       addrMask);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmAddVNRemoteAddressMask */




/*****************************************************************************/
/** fmDeleteVNRemoteAddressMask
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Specifies a range of addresses which will be removed from the remote
 *                  address mask table. By declaring a range, FFU resources may be used more
 *                  efficiently, since a single FFU rule can be used to direct the
 *                  entire range of traffic to the Tunneling Engine. Otherwise,
 *                  each remote address consumes a separate FFU rule for encapsulation.
 *                  Addresses in the range do not have to be served by the same tunnel.
 *
 * \note            This function does not remove the actual remote addresses. It only
 *                  removes the single FFU rule, replacing it with individual rules as
 *                  needed to support the remote addresses in the remote address table.
 *                  When removing remote addresses, efficiency will be maximized if each
 *                  remote address is first removed from the remote address table before
 *                  the remote address mask is removed from the mask table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       baseAddr points to the base address in the range.
 *
 * \param[in]       addrMask points to the bit-mask to be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID or if baseAddr is NULL.
 *
 *****************************************************************************/
fm_status fmDeleteVNRemoteAddressMask(fm_int        sw,
                                      fm_uint32     vni,
                                      fm_vnAddress *baseAddr,
                                      fm_vnAddress *addrMask)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_virtualNetwork *vn;
    fm_bool            routingLockTaken;
    fm_bool            lbgLockTaken;
    fm_char            textAddr1[100];
    fm_char            textAddr2[100];

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, vni = %u, baseAddr = %p, addrMask = %p\n",
                     sw,
                     vni,
                     (void *) baseAddr,
                     (void *) addrMask);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    lbgLockTaken     = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (baseAddr == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureLock(&switchPtr->lbgInfo.lbgLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ACL, status);

    lbgLockTaken = TRUE;

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (vn->descriptor.addressType == FM_VN_ADDR_TYPE_MAC)
    {
        fmDbgConvertMacAddressToString(baseAddr->macAddress, textAddr1);
        fmDbgConvertMacAddressToString(addrMask->macAddress, textAddr2);
    }
    else
    {
        fmDbgConvertIPAddressToString(&baseAddr->ipAddress, textAddr1);
        fmDbgConvertIPAddressToString(&addrMask->ipAddress, textAddr2);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_VN, "    baseAddr = %s, addrMask = %s\n", textAddr1, textAddr2);

    FM_API_CALL_FAMILY(status,
                       switchPtr->DeleteVNRemoteAddressMask,
                       sw,
                       vn,
                       baseAddr,
                       addrMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (lbgLockTaken)
    {
        fmReleaseLock(&switchPtr->lbgInfo.lbgLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDeleteVNRemoteAddressMask */




/*****************************************************************************/
/** fmGetVNRemoteAddressList
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Returns a list of remote addresses of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       maxAddresses is the size of addrList and tunnelIdList,
 *                  being the maximum number of remote addresses that can be
 *                  contained inside these arrays.
 *
 * \param[out]      numAddresses points to caller-provided storage into which
 *                  will be stored the number of remote addresses stored in
 *                  addrList and tunnelIdList.
 *
 * \param[out]      addrList is an array, maxAddresses elements in length,
 *                  that this function will fill with remote addresses.
 *
 * \param[out]      tunnelIdList is an array, maxAddresses elements in length,
 *                  that this function will fill with tunnel IDs for each
 *                  remote address returned in addrList.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if maxAddresses is <= 0, vni is not
 *                  a valid virtual network ID, or either of the pointer
 *                  arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_BUFFER_FULL if maxAddresses was too small
 *                  to accommodate the entire list of remote addresses.
 *
 *****************************************************************************/
fm_status fmGetVNRemoteAddressList(fm_int        sw,
                                   fm_uint32     vni,
                                   fm_int        maxAddresses,
                                   fm_int *      numAddresses,
                                   fm_vnAddress *addrList,
                                   fm_int *      tunnelIdList)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, maxAddresses = %d, numAddresses = %p, "
                      "addrList = %p, tunnelIdList = %p\n",
                      sw,
                      vni,
                      maxAddresses,
                      (void *) numAddresses,
                      (void *) addrList,
                      (void *) tunnelIdList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (maxAddresses <= 0) || (numAddresses == NULL) ||
         (addrList == NULL) || (tunnelIdList == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNRemoteAddressList,
                       sw,
                       vn,
                       maxAddresses,
                       numAddresses,
                       addrList,
                       tunnelIdList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNRemoteAddressList */




/*****************************************************************************/
/** fmGetVNRemoteAddressFirst
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the first remote address of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to
 *                  ''fmGetVNRemoteAddressNext''.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  function will write the first remote address.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID of the remote address will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no remote addresses.
 *
 *****************************************************************************/
fm_status fmGetVNRemoteAddressFirst(fm_int        sw,
                                    fm_uint32     vni,
                                    fm_voidptr *  searchToken,
                                    fm_vnAddress *addr,
                                    fm_int *      tunnelId)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, addr = %p, tunnelId = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) addr,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (addr == NULL) || (tunnelId == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNRemoteAddressFirst,
                       sw,
                       vn,
                       searchToken,
                       addr,
                       tunnelId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNRemoteAddressFirst */




/*****************************************************************************/
/** fmGetVNRemoteAddressNext
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the next remote address of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fmGetVNRemoteAddressFirst''.
 *                  It will be updated by this function with a new value
 *                  to be used in a subsequent call to this function.
 *
 * \param[out]      addr points to caller-provided storage into which the
 *                  function will write the next remote address.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID of the remote address will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more remote addresses.
 *
 *****************************************************************************/
fm_status fmGetVNRemoteAddressNext(fm_int        sw,
                                   fm_uint32     vni,
                                   fm_voidptr *  searchToken,
                                   fm_vnAddress *addr,
                                   fm_int *      tunnelId)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, addr = %p, tunnelId = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) addr,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (addr == NULL) || (tunnelId == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNRemoteAddressNext,
                       sw,
                       vn,
                       searchToken,
                       addr,
                       tunnelId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNRemoteAddressNext */




/*****************************************************************************/
/** fmGetVNRemoteAddressMaskList
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Returns a list of address ranges present in the remote
 *                  address mask table of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       maxAddrMasks is the size of baseAddrList, addrMaskList
 *                  and tunnelIdList, being the maximum number of remote
 *                  address masks that can be contained inside these arrays.
 *
 * \param[out]      numAddrMasks points to caller-provided storage into which
 *                  will be stored the number of remote address masks stored
 *                  in baseAddrList, addrMaskList and tunnelIdList.
 *
 * \param[out]      baseAddrList is an array, maxAddrMasks elements in length,
 *                  that this function will fill with base addresses of each
 *                  address range.
 *
 * \param[out]      addrMaskList is an array, maxAddrMasks elements in length,
 *                  that this function will fill with bit-masks of each address
 *                  range.
 *
 * \param[out]      tunnelIdList is an array, maxAddrMasks elements in length,
 *                  that this function will fill with tunnel IDs for each
 *                  address range returned in baseAddrList and addrMaskList
 *                  (-1 for address ranges not associated with one particular
 *                  Tunneling Engine rule).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if maxAddrMasks is <= 0, vni is not
 *                  a valid virtual network ID, or either of the pointer
 *                  arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_BUFFER_FULL if maxAddrMasks was too small
 *                  to accommodate the entire list of remote address masks.
 *
 *****************************************************************************/
fm_status fmGetVNRemoteAddressMaskList(fm_int        sw,
                                       fm_uint32     vni,
                                       fm_int        maxAddrMasks,
                                       fm_int *      numAddrMasks,
                                       fm_vnAddress *baseAddrList,
                                       fm_vnAddress *addrMaskList,
                                       fm_int *      tunnelIdList)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, maxAddrMasks = %d, numAddrMasks = %p, "
                      "baseAddrList = %p, addrMaskList = %p, tunnelIdList = %p\n",
                      sw,
                      vni,
                      maxAddrMasks,
                      (void *) numAddrMasks,
                      (void *) baseAddrList,
                      (void *) addrMaskList,
                      (void *) tunnelIdList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (maxAddrMasks <= 0) || (numAddrMasks == NULL) ||
         (baseAddrList == NULL) ||(addrMaskList == NULL) ||
         (tunnelIdList == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNRemoteAddressMaskList,
                       sw,
                       vn,
                       maxAddrMasks,
                       numAddrMasks,
                       baseAddrList,
                       addrMaskList,
                       tunnelIdList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNRemoteAddressMaskList */




/*****************************************************************************/
/** fmGetVNRemoteAddressMaskFirst
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the first address range in the remote address mask
 *                  table of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to
 *                  ''fmGetVNRemoteAddressMaskNext''.
 *
 * \param[out]      baseAddr points to caller-provided storage into which the
 *                  function will write the base address of the first address
 *                  range.
 *
 * \param[out]      addrMask points to caller-provided storage into which the
 *                  function will write the bit-mask of the first address
 *                  range.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID for the address range will be written
 *                  (-1 if the address range is not associated with one
 *                  particular Tunneling Engine rule).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no remote address masks.
 *
 *****************************************************************************/
fm_status fmGetVNRemoteAddressMaskFirst(fm_int        sw,
                                        fm_uint32     vni,
                                        fm_voidptr *  searchToken,
                                        fm_vnAddress *baseAddr,
                                        fm_vnAddress *addrMask,
                                        fm_int *      tunnelId)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, baseAddr = %p, addrMask = %p, tunnelId = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) baseAddr,
                      (void *) addrMask,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (baseAddr == NULL) ||
         (addrMask == NULL) || (tunnelId == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNRemoteAddressMaskFirst,
                       sw,
                       vn,
                       searchToken,
                       baseAddr,
                       addrMask,
                       tunnelId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNRemoteAddressMaskFirst */




/*****************************************************************************/
/** fmGetVNRemoteAddressMaskNext
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the next address range in the remote address mask
 *                  table of a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fmGetVNRemoteAddressMaskFirst''.
 *                  It will be updated by this function with a new value
 *                  to be used in a subsequent call to this function.
 *
 * \param[out]      baseAddr points to caller-provided storage into which the
 *                  function will write the base address of the next address
 *                  range.
 *
 * \param[out]      addrMask points to caller-provided storage into which the
 *                  function will write the bit-mask of the next address
 *                  range.
 *
 * \param[out]      tunnelId points to caller-provided storage into which
 *                  the tunnel ID for the address range will be written
 *                  (-1 if the address range is not associated with one
 *                  particular Tunneling Engine rule).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more remote address masks.
 *
 *****************************************************************************/
fm_status fmGetVNRemoteAddressMaskNext(fm_int        sw,
                                       fm_uint32     vni,
                                       fm_voidptr *  searchToken,
                                       fm_vnAddress *baseAddr,
                                       fm_vnAddress *addrMask,
                                       fm_int *      tunnelId)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, baseAddr = %p, addrMask = %p, tunnelId = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) baseAddr,
                      (void *) addrMask,
                      (void *) tunnelId );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (baseAddr == NULL) ||
         (addrMask == NULL) || (tunnelId == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNRemoteAddressMaskNext,
                       sw,
                       vn,
                       searchToken,
                       baseAddr,
                       addrMask,
                       tunnelId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNRemoteAddressMaskNext */




/*****************************************************************************/
/** fmConfigureVN
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Configures the Virtual Networking API.
 *
 * \note            This function should be called prior to any
 *                  attempt to create any virtual networks or tunnels. If
 *                  there is a need to call it after tunnels and/or networks
 *                  have been created, all tunnels and networks should be
 *                  deleted before this function is called, since some settings
 *                  may only take effect when the first network or tunnel is
 *                  created.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       config points to the configuration record to be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, if tunnelId is not a valid tunnel
 *                  ID, or if addr is NULL.
 *
 *****************************************************************************/
fm_status fmConfigureVN(fm_int sw, fm_vnConfiguration *config)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    routingLockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, config = %p\n",
                     sw,
                     (void *) config);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (config == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    FM_API_CALL_FAMILY(status, switchPtr->ConfigureVN, sw, config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmConfigureVN */




/*****************************************************************************/
/** fmConfigureVNDefaultGpe
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Configures the Virtual Networking API default values
 *                  of the VXLAN-GPE fields. Supported only when encapsulation
 *                  tunnel engine is operating in
 *                  ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 * \note            This function should be called prior to any
 *                  attempt to create any virtual networks or tunnels. If
 *                  there is a need to call it after tunnels and/or networks
 *                  have been created, all tunnels and networks should be
 *                  deleted before this function is called, since some settings
 *                  may only take effect when the first network or tunnel is
 *                  created.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       defaultGpe points to the VXLAN-GPE configuration record
 *                  to be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, if tunnelId is not a valid tunnel
 *                  ID, or if addr is NULL.
 * \return          FM_ERR_TE_MODE if encapsulation tunnel engine is not
 *                  configured for ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 *****************************************************************************/
fm_status fmConfigureVNDefaultGpe(fm_int sw, fm_vnGpeCfg *defaultGpe)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    routingLockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, defaultGpe = %p\n",
                     sw,
                     (void *) defaultGpe);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (defaultGpe == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    FM_API_CALL_FAMILY(status, switchPtr->ConfigureVNDefaultGpe, sw, defaultGpe);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmConfigureVNDefaultGpe */




/*****************************************************************************/
/** fmConfigureVNDefaultNsh
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Configures the Virtual Networking API default values
 *                  of the NSH fields. Supported only when encapsulation
 *                  tunnel engine is operating in
 *                  ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 * \note            This function should be called prior to any
 *                  attempt to create any virtual networks or tunnels. If
 *                  there is a need to call it after tunnels and/or networks
 *                  have been created, all tunnels and networks should be
 *                  deleted before this function is called, since some settings
 *                  may only take effect when the first network or tunnel is
 *                  created.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       defaultNsh points to the NSH configuration record to be
 *                  used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, if tunnelId is not a valid tunnel
 *                  ID, or if addr is NULL.
 * \return          FM_ERR_TE_MODE if encapsulation tunnel engine is not
 *                  configured for ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 *****************************************************************************/
fm_status fmConfigureVNDefaultNsh(fm_int sw, fm_vnNshCfg *defaultNsh)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    routingLockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, defaultNsh = %p\n",
                     sw,
                     (void *) defaultNsh);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (defaultNsh == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    FM_API_CALL_FAMILY(status, switchPtr->ConfigureVNDefaultNsh, sw, defaultNsh);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmConfigureVNDefaultNsh */




/*****************************************************************************/
/** fmGetVNConfiguration
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Retrieves the Virtual Networking API configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      config points to caller-provided storage into which
 *                  the Virtual Networking API configuration will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if config is NULL.
 *
 *****************************************************************************/
fm_status fmGetVNConfiguration(fm_int sw, fm_vnConfiguration *config)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    lockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, config = %p\n",
                     sw,
                     (void *) config);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    lockTaken = FALSE;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (config == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    FM_API_CALL_FAMILY(status, switchPtr->GetVNConfiguration, sw, config);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNConfiguration */




/*****************************************************************************/
/** fmGetVNDefaultGpe
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Retrieves the Virtual Networking API default values
 *                  of the VXLAN-GPE fields. Supported only when encapsulation
 *                  tunnel engine is operating in
 *                  ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      defaultGpe points to caller-provided storage into which
 *                  VXLAN-GPE configuration will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if config is NULL.
 * \return          FM_ERR_TE_MODE if encapsulation tunnel engine is not
 *                  configured for ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 *****************************************************************************/
fm_status fmGetVNDefaultGpe(fm_int sw, fm_vnGpeCfg *defaultGpe)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    lockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, defaultGpe = %p\n",
                     sw,
                     (void *) defaultGpe);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    lockTaken = FALSE;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (defaultGpe == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    FM_API_CALL_FAMILY(status, switchPtr->GetVNDefaultGpe, sw, defaultGpe);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNDefaultGpe */




/*****************************************************************************/
/** fmGetVNDefaultNsh
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Retrieves the Virtual Networking API default values
 *                  of the NSH fields. Supported only when encapsulation
 *                  tunnel engine is operating in
 *                  ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      defaultNsh points to caller-provided storage into which
 *                  NSH configuration will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if config is NULL.
 * \return          FM_ERR_TE_MODE if encapsulation tunnel engine is not
 *                  configured for ''FM_TUNNEL_API_MODE_VXLAN_GPE_NSH'' mode.
 *
 *****************************************************************************/
fm_status fmGetVNDefaultNsh(fm_int sw, fm_vnNshCfg *defaultNsh)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    lockTaken;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN,
                     "sw = %d, defaultNsh = %p\n",
                     sw,
                     (void *) defaultNsh);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    lockTaken = FALSE;
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if (defaultNsh == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    FM_API_CALL_FAMILY(status, switchPtr->GetVNDefaultNsh, sw, defaultNsh);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);
    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNDefaultNsh */




/*****************************************************************************/
/** fmAddVNLocalPort
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Adds a local port to the broadcast and flooding flood-set
 *                  for a virtual network. Only needed if the virtual network
 *                  supports flooding, as specified in the ''fm_vnDescriptor''
 *                  structure provided in a call to ''fmCreateVN'' or
 *                  ''fmUpdateVN''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID.
 *
 * \param[in]       port is the logical port number for the VM or ethernet port
 *                  that is a member of the virtual network. This value will be
 *                  used in conjunction with the virtual network's vlan value
 *                  to form the flood-set listener tuple.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID or if the port is invalid.
 *
 *****************************************************************************/
fm_status fmAddVNLocalPort(fm_int sw, fm_uint32 vni, fm_int port)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_bool            routingLockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d, vni = %u, port = %d\n", sw, vni, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->AddVNLocalPort, sw, vn, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmAddVNLocalPort */




/*****************************************************************************/
/** fmDeleteVNLocalPort
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Deletes a local port from the broadcast and flooding flood-set
 *                  for a virtual network. Only needed if the virtual network
 *                  supports flooding, as specified in the ''fm_vnDescriptor''
 *                  structure provided in a call to ''fmCreateVN'' or
 *                  ''fmUpdateVN''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID.
 *
 * \param[in]       port is the logical port number for the VM or ethernet port
 *                  that is no longer a member of the virtual network. This
 *                  value will be used in conjunction with the virtual network's
 *                  vlan value to remove the listener tuple from the flood-set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID or if the port is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteVNLocalPort(fm_int sw, fm_uint32 vni, fm_int port)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_bool            routingLockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d, vni = %u, port = %d\n", sw, vni, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->DeleteVNLocalPort, sw, vn, port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDeleteVNLocalPort */




/*****************************************************************************/
/** fmGetVNLocalPortList
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Returns a list of local ports in the virtual network's
 *                  broadcast and flooding flood-set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       maxPorts is the size of portList, being the maximum number
 *                  of local ports that can be contained inside the array.
 *
 * \param[out]      numPorts points to caller-provided storage into which
 *                  will be stored the number of local ports stored
 *                  in portList.
 *
 * \param[out]      portList is an array, maxPorts elements in length,
 *                  that this function will fill with logical port numbers
 *                  for each local port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if maxPorts is <= 0, vni is not
 *                  a valid virtual network ID, or either of the pointer
 *                  arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_BUFFER_FULL if maxPorts was too small to accommodate
 *                  the entire list of local ports.
 *
 *****************************************************************************/
fm_status fmGetVNLocalPortList(fm_int    sw,
                               fm_uint32 vni,
                               fm_int    maxPorts,
                               fm_int *  numPorts,
                               fm_int *  portList)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, maxPorts = %d, numPorts = %p, "
                      "portList = %p\n",
                      sw,
                      vni,
                      maxPorts,
                      (void *) numPorts,
                      (void *) portList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (maxPorts <= 0) || (numPorts == NULL) || (portList == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNLocalPortList,
                       sw,
                       vn,
                       maxPorts,
                       numPorts,
                       portList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNLocalPortList */




/*****************************************************************************/
/** fmGetVNLocalPortFirst
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the first local port in the virtual network's
 *                  broadcast and flooding flood-set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_mcastGroupListener, where this function will store
 *                  a token to be used in a subsequent call to
 *                  ''fmGetVNLocalPortNext''.
 *
 * \param[out]      port points to caller-provided storage into which the
 *                  function will write the logical port number of the first
 *                  local port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no local ports.
 *
 *****************************************************************************/
fm_status fmGetVNLocalPortFirst(fm_int                 sw,
                                fm_uint32              vni,
                                fm_mcastGroupListener *searchToken,
                                fm_int *               port)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, port = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) port );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (port == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNLocalPortFirst,
                       sw,
                       vn,
                       searchToken,
                       port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNLocalPortFirst */




/*****************************************************************************/
/** fmGetVNLocalPortNext
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the next local port in the virtual network's broadcast
 *                  and flooding flood-set.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_mcastGroupListener that has been filled in by a prior
 *                  call to this function or to ''fmGetVNLocalPortFirst''.
 *                  It will be updated by this function with a new value
 *                  to be used in a subsequent call to this function.
 *
 * \param[out]      port points to caller-provided storage into which the
 *                  function will write the logical port number of the next
 *                  local port.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more local ports.
 *
 *****************************************************************************/
fm_status fmGetVNLocalPortNext(fm_int                 sw,
                               fm_uint32              vni,
                               fm_mcastGroupListener *searchToken,
                               fm_int *               port)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, port = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) port );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (port == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNLocalPortNext,
                       sw,
                       vn,
                       searchToken,
                       port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNLocalPortNext */




/*****************************************************************************/
/** fmAddVNVsi
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Adds a VSI value to a virtual network. Supported only when
 *                  the virtual network is operating in vswitch-offload mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID.
 *
 * \param[in]       vsi is the vsi value that is now a member of the virtual
 *                  network.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID or if the vsi is invalid.
 * \return          FM_ERR_ALREADY_EXISTS if the VSI is already in use by
 *                  another VN.
 * \return          FM_ERR_UNSUPPORTED if the VN is not in vswitch-offload mode.
 *
 *****************************************************************************/
fm_status fmAddVNVsi(fm_int sw, fm_uint32 vni, fm_int vsi)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_bool            routingLockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d, vni = %u, vsi = %d\n", sw, vni, vsi);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->AddVNVsi, sw, vn, vsi);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmAddVNVsi */




/*****************************************************************************/
/** fmDeleteVNVsi
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Deletes a VSI from a virtual network. Supported only when
 *                  the virtual network is operating in vswitch-offload mode.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID.
 *
 * \param[in]       vsi is the VSI number that is no longer a member of the
 *                  virtual network.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID or if the vsi is invalid.
 * \return          FM_ERR_UNSUPPORTED if the VN is not in vswitch-offload mode.
 * \return          FM_ERR_NOT_FOUND if the VSI was not assigned to this VN.
 *
 *****************************************************************************/
fm_status fmDeleteVNVsi(fm_int sw, fm_uint32 vni, fm_int vsi)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_bool            routingLockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d, vni = %u, vsi = %d\n", sw, vni, vsi);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    routingLockTaken = FALSE;
    switchPtr        = GET_SWITCH_PTR(sw);

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    routingLockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->DeleteVNVsi, sw, vn, vsi);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDeleteVNVsi */




/*****************************************************************************/
/** fmGetVNVsiList
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Returns a list of VSIs in a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in]       maxVsis is the size of vsiList, being the maximum number
 *                  of VSI numbers that can be contained inside the array.
 *
 * \param[out]      numVsis points to caller-provided storage into which
 *                  will be stored the number of VSIs stored in vsiList.
 *
 * \param[out]      vsiList is an array, maxVsis elements in length,
 *                  that this function will fill with VSI numbers.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if maxVsis is <= 0, vni is not
 *                  a valid virtual network ID, or either of the pointer
 *                  arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_BUFFER_FULL if maxVsis was too small to accommodate
 *                  the entire list of VSIs.
 *
 *****************************************************************************/
fm_status fmGetVNVsiList(fm_int    sw,
                         fm_uint32 vni,
                         fm_int    maxVsis,
                         fm_int *  numVsis,
                         fm_int *  vsiList)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, maxVsis = %d, numVsis = %p, "
                      "vsiList = %p\n",
                      sw,
                      vni,
                      maxVsis,
                      (void *) numVsis,
                      (void *) vsiList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (maxVsis <= 0) || (numVsis == NULL) || (vsiList == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNVsiList,
                       sw,
                       vni,
                       maxVsis,
                       numVsis,
                       vsiList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNVsiList */




/*****************************************************************************/
/** fmGetVNVsiFirst
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the first VSI in a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[out]      searchToken points to caller-allocated storage where this
 *                  function will store a token to be used in a subsequent
 *                  call to ''fmGetVNVsiNext''.
 *
 * \param[out]      vsi points to caller-provided storage into which the
 *                  function will write the first VSI number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no VSIs.
 *
 *****************************************************************************/
fm_status fmGetVNVsiFirst(fm_int    sw,
                          fm_uint32 vni,
                          fm_int *  searchToken,
                          fm_int *  vsi)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, vsi = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) vsi );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (vsi == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNVsiFirst,
                       sw,
                       vni,
                       searchToken,
                       vsi);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNVsiFirst */




/*****************************************************************************/
/** fmGetVNVsiNext
 * \ingroup virtualNetwork
 *
 * \chips           FM10000
 *
 * \desc            Gets the next VSI in a virtual network.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vni is the Virtual Network ID number.
 *
 * \param[in,out]   searchToken points to caller-allocated storage that has
 *                  been filled in by a prior call to this function or to
 *                  ''fmGetVNVsiFirst''. It will be updated by this function
 *                  with a new value to be used in a subsequent call to this
 *                  function.
 *
 * \param[out]      vsi points to caller-provided storage into which the
 *                  function will write the next VSI number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if vni is not a valid virtual
 *                  network ID, or either of the pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NOT_FOUND if searchToken is invalid.
 * \return          FM_ERR_NO_MORE if there are no more VSIs.
 *
 *****************************************************************************/
fm_status fmGetVNVsiNext(fm_int    sw,
                         fm_uint32 vni,
                         fm_int *  searchToken,
                         fm_int *  vsi)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_virtualNetwork *vn;

    FM_LOG_ENTRY_API( FM_LOG_CAT_VN,
                      "sw = %d, vni = %u, searchToken = %p, vsi = %p\n",
                      sw,
                      vni,
                      (void *) searchToken,
                      (void *) vsi );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    if ( (searchToken == NULL) || (vsi == NULL) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    /* Get the VN record */
    vn = fmGetVN(sw, vni);

    if (vn == NULL)
    {
        /* VN doesn't exist */
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetVNVsiNext,
                       sw,
                       vni,
                       searchToken,
                       vsi);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmGetVNVsiNext */




/*****************************************************************************/
/** fmDbgDumpVN
 * \ingroup intDebug
 *
 * \chips           FM10000
 *
 * \desc            Display the virtual networks status of a switch
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpVN(fm_int sw)
{
    fm_switch *        switchPtr;
    fm_status          status;
    fm_bool            lockTaken;
    fm_int             numVNs;
    fm_uint32          vsidList[FM_MAX_NUM_VNS];
    fm_vnDescriptor    vnDescriptorList[FM_MAX_NUM_VNS];
    fm_int             numTunnels;
    fm_int *           tunnelIds;
    fm_int             i;

    FM_LOG_ENTRY_API(FM_LOG_CAT_VN, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;
    tunnelIds = NULL;

    if (switchPtr->maxVNTunnels <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    tunnelIds = fmAlloc(sizeof(fm_int) * switchPtr->maxVNTunnels);
    if (tunnelIds == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    lockTaken = TRUE;

    FM_LOG_PRINT("\n*******************************************************************************\n");
    FM_LOG_PRINT("***   VN API state of switch %d\n", sw);
    FM_LOG_PRINT("*******************************************************************************\n\n");

    FM_API_CALL_FAMILY(status, switchPtr->DbgDumpVN, sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);


    FM_LOG_PRINT("\nVirtual Networks\n");
    FM_LOG_PRINT("*******************************************************************************\n\n");

    status = GetVNList(sw,
                       FM_MAX_NUM_VNS,
                       &numVNs,
                       vsidList,
                       vnDescriptorList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    for (i = 0; i < numVNs; i++)
    {
        FM_API_CALL_FAMILY(status,
                           switchPtr->DbgDumpVirtualNetwork,
                           sw,
                           vsidList[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

    FM_LOG_PRINT("\nTunnels\n");
    FM_LOG_PRINT("*******************************************************************************\n\n");


    status = GetVNTunnelList(sw,
                             switchPtr->maxVNTunnels,
                             &numTunnels,
                             tunnelIds);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);

    for (i = 0; i < numTunnels; i++)
    {
        FM_API_CALL_FAMILY(status,
                           switchPtr->DbgDumpVNTunnel,
                           sw,
                           tunnelIds[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_VN, status);
    }

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    if (tunnelIds != NULL)
    {
        fmFree(tunnelIds);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_VN, status);

}   /* end fmDbgDumpVN */


