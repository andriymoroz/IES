/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_rbridge.c
 * Creation Date:   August 31, 2012 
 * Description:     RBridges API interface.
 *
 * Copyright (c) 2005 - 2012, Intel Corporation
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

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmCreateRBridge
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Creates a remote RBridge node for this network. This allows
 *                  frames to be properly encapsulated and routed to the
 *                  appropriate destination.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       rbridge points to an ''fm_remoteRBridge'' record describing
 *                  the remote RBridge to be added to virtual network.
 * 
 * \param[out]      tunnelId is the index returned by this function to reference
 *                  this specific node. This 12-bit tunnel ID is stored in a
 *                  MAC table entry and will be used to retrieve the next hop
 *                  RBridge information, such as the next RBridge MAC address
 *                  and the egress RBridge nickname.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_TABLE_FULL if the internal resources used to store
 *                  the remote RBridge nodes are full.
 * 
 *****************************************************************************/
fm_status fmCreateRBridge(fm_int            sw, 
                          fm_remoteRBridge *rbridge, 
                          fm_int *          tunnelId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE,
                     "sw=%d, bridge=%p, tunnelId=%p\n", 
                     sw, 
                     (void *) rbridge, 
                     (void *) tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateRBridge,
                       sw,
                       rbridge,
                       tunnelId);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmCreateRBridge */



/*****************************************************************************/
/** fmDeleteRBridge
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Deletes a RBridge node.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the index to reference this specific RBridge
 *                  node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NOT_FOUND if the specified tunnel ID is not a
 *                  valid tunnel.
 *                  
 *****************************************************************************/
fm_status fmDeleteRBridge(fm_int sw, fm_int tunnelId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d, tunnelId=%d\n", sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteRBridge,
                       sw,
                       tunnelId);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmDeleteRBridge */



/*****************************************************************************/
/** fmUpdateRBbridgeEntry
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Update the requested remote Rbridge entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the index to reference this specific RBridge
 *                  node.
 *
 * \param[in]       rbridge points to an ''fm_remoteRBridge'' record describing
 *                  the updated RBridge information.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmUpdateRBridgeEntry(fm_int            sw,
                               fm_int            tunnelId,
                               fm_remoteRBridge *rbridge)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, 
                     "sw=%d, tunnelId=%d\n", 
                     sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateRBridgeEntry,
                       sw,
                       tunnelId,
                       rbridge);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmUpdateRBridgeEntry */




/*****************************************************************************/
/** fmGetRBbridgeEntry
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the requested remote Rbridge from the list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the index to reference this specific RBridge
 *                  node.
 *
 * \param[out]      rbridge points to caller-provided storage into which the
 *                  function will write the structure information related
 *                  to this specific RBridge node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetRBridgeEntry(fm_int            sw,
                            fm_int            tunnelId,
                            fm_remoteRBridge *rbridge)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, 
                     "sw=%d, tunnelId=%d\n", 
                     sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgeEntry,
                       sw,
                       tunnelId,
                       rbridge);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgeEntry */




/*****************************************************************************/
/** fmGetRBbridgeFirst
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the first remote Rbridge in the list.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      tunnelId points to caller-provided storage into which the
 *                  function will write the first virtual tunnel ID.
 *
 * \param[out]      rbridge points to caller-provided storage into which the
 *                  function will write the structure information related
 *                  to the first RBridge node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetRBridgeFirst(fm_int            sw,
                            fm_int *          tunnelId,
                            fm_remoteRBridge *rbridge)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, 
                     "sw=%d, tunnelId=%p\n", 
                     sw, (void *) tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgeFirst,
                       sw,
                       tunnelId,
                       rbridge);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgeFirst */




/*****************************************************************************/
/** fmGetRBridgeNext
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the next remote RBridge.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentTunnelId is the last valid tunnel Id found by a
 *                  previous call to this function or to ''fmGetRBridgeFirst''.
 * 
 * \param[out]      nextTunnelId points to caller-provided storage into which
 *                  the function will write the next virtual tunnel ID.
 *
 * \param[out]      rbridge points to caller-provided storage into which the
 *                  function will write the structure information related
 *                  to the next RBridge node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no more virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetRBridgeNext(fm_int            sw,
                           fm_int            currentTunnelId,
                           fm_int *          nextTunnelId,
                           fm_remoteRBridge *rbridge)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgeNext,
                       sw,
                       currentTunnelId,
                       nextTunnelId,
                       rbridge);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgeNext */



/*****************************************************************************/
/** fmCreateRBridgeDistTree
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Creates an RBridge Distribution Tree.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       distTree points to an ''fm_distTree'' record describing
 *                  the distribution tree to be added to a virtual network.
 * 
 * \param[out]      tunnelId is the index returned by this function to reference
 *                  this distribution tree. This tunnel ID is associated with
 *                  a specific flood-set
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_TABLE_FULL if the internal resources used to store
 *                  the new RBridge Distribution Tree are full.
 * 
 *****************************************************************************/
fm_status fmCreateRBridgeDistTree(fm_int       sw, 
                                  fm_distTree *distTree, 
                                  fm_int *     tunnelId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE,
                     "sw=%d, distTree=%p, tunnelId=%p\n", 
                     sw, 
                     (void *) distTree, 
                     (void *) tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->CreateRBridgeDistTree,
                       sw,
                       distTree,
                       tunnelId);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmCreateRBridgeDistTree */



/*****************************************************************************/
/** fmDeleteRbridgeDistTree
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Deletes an RBridge Distibution Tree.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the index to reference this RBridge
 *                  distribution tree.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_NOT_FOUND if the specified tunnel ID is not a
 *                  valid tunnel.
 *                  
 *****************************************************************************/
fm_status fmDeleteRBridgeDistTree(fm_int sw, fm_int tunnelId)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d, tunnelId=%d\n", sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DeleteRBridgeDistTree,
                       sw,
                       tunnelId);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmDeleteRBridgeDistTree */



/*****************************************************************************/
/** fmUpdateRBridgeDistTree
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Updates the requested RBridge Distribution Tree info.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the index to reference this specific RBridge
 *                  Distribution Tree.
 * 
 * \param[in]       distTree points to an ''fm_distTree'' record describing
 *                  the updated distribution tree information.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmUpdateRBridgeDistTree(fm_int       sw,
                                  fm_int       tunnelId,
                                  fm_distTree *distTree )
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d, tunnelId=%d\n", sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateRBridgeDistTree,
                       sw,
                       tunnelId,
                       distTree);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmUpdateRBridgeDistTree */




/*****************************************************************************/
/** fmGetRBridgeDistTree
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the requested RBridge Distribution Tree info.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       tunnelId is the index to reference this specific RBridge
 *                  Distribution Tree.
 * 
 * \param[out]      distTree points to caller-provided storage into which the
 *                  function will write the structure information related
 *                  to this specific distribution tree node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetRBridgeDistTree(fm_int       sw,
                               fm_int       tunnelId,
                               fm_distTree *distTree )
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d, tunnelId=%d\n", sw, tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgeDistTree,
                       sw,
                       tunnelId,
                       distTree);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgeDistTree */



/*****************************************************************************/
/** fmGetRBridgeDistTreeFirst
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the first RBridge Distribution Tree.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      tunnelId points to caller-provided storage into which the
 *                  function will write the virtual tunnel ID associated
 *                  with the first RBridge Distribution Tree.
 *
 * \param[out]      distTree points to caller-provided storage into which the
 *                  function will write the structure information related
 *                  to the first distribution tree node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetRBridgeDistTreeFirst(fm_int       sw,
                                    fm_int *     tunnelId,
                                    fm_distTree *distTree)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, 
                     "sw=%d, tunnelId=%p\n", 
                     sw, (void *) tunnelId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgeDistTreeFirst,
                       sw,
                       tunnelId,
                       distTree);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgeDistTreeFirst */




/*****************************************************************************/
/** fmGetRBridgeDistTreeNext
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the next RBridge Distribution Tree.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentTunnelId is the last valid tunnel Id found by a
 *                  previous call to this function or
 *                  to ''fmGetRBridgeDistTreeFirst''.
 * 
 * \param[out]      nextTunnelId points to caller-provided storage into which 
 *                  the function will write the virtual tunnel ID
 *                  associated with the next RBridge Distribution Tree.
 *
 * \param[out]      distTree points to caller-provided storage into which the
 *                  function will write the structure information related
 *                  to the next distribution tree node.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if virtual networks are not supported.
 * \return          FM_ERR_NO_MORE if there are no virtual network tunnels.
 *
 *****************************************************************************/
fm_status fmGetRBridgeDistTreeNext(fm_int       sw,
                                   fm_int       currentTunnelId,
                                   fm_int *     nextTunnelId,
                                   fm_distTree *distTree)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgeDistTreeNext,
                       sw,
                       currentTunnelId,
                       nextTunnelId,
                       distTree);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgeDistTreeNext */




/*****************************************************************************/
/** fmSetRBridgePortHopCount
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Sets the per-port "Hop Count" value for unicast purposes.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[in]       value is the hop count value to set.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmSetRBridgePortHopCount(fm_int sw, fm_int port, fm_uint32 value)
{
    fm_switch *     switchPtr;
    fm_status       err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d, port=%d, hopCnt= %d\n", 
                     sw, port, value);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_ALL);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->SetRBridgePortHopCount,
                       sw,
                       port,
                       value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmSetRBridgePortHopCount */




/*****************************************************************************/
/** fmGetRBridgePortHopCount
 * \ingroup rbridge
 *
 * \chips           FM6000
 *
 * \desc            Gets the per-port "Hop Count" value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[out]      value points to caller-provided storage into which the
 *                  function will write the hop count value.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmGetRBridgePortHopCount(fm_int sw, fm_int port, fm_uint32 *value)
{
    fm_switch *     switchPtr;
    fm_status       err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_RBRIDGE, "sw=%d, port=%d\n", sw, port);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    VALIDATE_LOGICAL_PORT(sw, port, ALLOW_ALL);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetRBridgePortHopCount,
                       sw,
                       port,
                       value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_RBRIDGE, err);

}   /* end fmGetRBridgePortHopCount */
