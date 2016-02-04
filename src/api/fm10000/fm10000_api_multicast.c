/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_multicast.c
 * Creation Date:   August 19, 2013
 * Description:     FM10000-specific multicast services.
 *
 * Copyright (c) 2007 - 2016, Intel Corporation
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

#include <fm_sdk_fm10000_int.h>

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
static fm_status AddListenerToGroup(fm_int                   sw,
                                    fm_intMulticastGroup *   group,
                                    fm_intMulticastListener *listener);
static fm_status RemoveListenerFromGroup(fm_int                   sw,
                                         fm_intMulticastGroup *   group,
                                         fm_intMulticastListener *listener);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** EnableMtableGroup
 * \ingroup intMulticast
 *
 * \desc            Enable mtable group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group's table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status EnableMtableGroup(fm_int sw, fm_intMulticastGroup *group)
{
    fm_status              status;
    fm_int                 mcastIndex;
    fm10000_MulticastGroup *groupExt;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d)\n",
                  sw,
                  (void *) group,
                  group->handle );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "default VLAN for group %d is %d\n",
                 group->handle,
                 group->defaultVlan);
    
    groupExt = group->extension;
    mcastIndex = groupExt->mtableDestIndex;

    if (group->repliGroup == FM_MCASTGROUP_REPLICATION_GROUP_DISABLE)
    {
        /* A private replication group will be allocated */
        group->privateGroup = TRUE;
    }
    
    status = fm10000MTableEnableGroup(sw,
                                      group->logicalPort,
                                      FM_MULTICAST_GROUP_TYPE_L3,
                                      group->defaultVlan,
                                      &group->repliGroup,
                                      &mcastIndex);
    
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    groupExt->mtableDestIndex = mcastIndex;
    
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end EnableMtableGroup */




/*****************************************************************************/
/** AddListenerToGroup
 * \ingroup intMulticast
 *
 * \desc            Add a multicast listener to the hardware resources
 *                  used by the group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       listener points to the multicast listener entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if a listener port is invalid
 *
 *****************************************************************************/
static fm_status AddListenerToGroup(fm_int                   sw,
                                    fm_intMulticastGroup *   group,
                                    fm_intMulticastListener *listener)
{
    fm_status               status;
    fm_switch *             switchPtr;
    fm_portmask             destMask;
    fm_bool                 isPhysicalPort;
    fm_bool                 isInternalPort;
    fm_int                  port;
    fm10000_mtableEntry     mtableEntry;
    fm_port                *portPtr;
    fm10000_MulticastGroup *groupExt;
    fm_uint16               vlan;
    fm_uint32               dglort;
    fm_int                  tunnelEngine;
    fm_int                  tunnelGroup;
    fm10000_switch *        switchExt;
    fm_tunnelGlortUser      glortUser;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw=%d group=%p<%d> listener=%p\n",
                  sw,
                  (void *) group,
                  group ? group->handle : -1,
                  (void *) listener);

    if (group == NULL || listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_DBG_DUMP_LISTENER(&listener->listener);

    if (listener->addedToChip)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    switch (listener->listener.listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            port = listener->listener.info.portVlanListener.port;
            vlan = listener->listener.info.portVlanListener.vlan;
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            status = fm10000AddVNDirectTunnelRule(sw,
                                                  listener->listener.info.vnListener.tunnelId,
                                                  listener->listener.info.vnListener.vni,
                                                  &port,
                                                  &dglort);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            FM_CLEAR(mtableEntry);
            mtableEntry.port         = port;
            mtableEntry.vlan         = 0;
            mtableEntry.vlanUpdate   = FALSE;
            mtableEntry.dglortUpdate = TRUE;
            mtableEntry.dglort       = dglort;

            status = fm10000MTableAddListener(sw,
                                              group->logicalPort,
                                              group->repliGroup,
                                              mtableEntry);
            if (status == FM_OK)
            {
                listener->addedToChip = TRUE;

                FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                              "mcast group %p (%d), vn tunnel listener "
                              "added to mtable, tunnel %d, vni %u, "
                              "port %d, update glort %X\n",
                              (void *) group,
                              group->handle,
                              listener->listener.info.vnListener.tunnelId,
                              listener->listener.info.vnListener.vni,
                              port,
                              dglort );
            }

            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:

            status = fm10000GetFlowAttribute(sw,
                                             listener->listener.info.flowListener.tableIndex,
                                             FM_FLOW_TABLE_TUNNEL_ENGINE,
                                             (void*)&tunnelEngine);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            if ( (tunnelEngine < 0) ||
                 (tunnelEngine >= FM10000_TE_DGLORT_MAP_ENTRIES_1) )
            {
                status = FM_FAIL;
                FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);
            }

            switchExt = GET_SWITCH_EXT(sw);
            port = switchExt->tunnelCfg->tunnelPort[tunnelEngine];

            status = fm10000GetFlowAttribute(sw,
                                             listener->listener.info.flowListener.tableIndex,
                                             FM_FLOW_TABLE_TUNNEL_GROUP,
                                             (void*)&tunnelGroup);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            status = fm10000GetTunnelAttribute(sw,
                                               tunnelGroup,
                                               listener->listener.info.flowListener.flowId,
                                               FM_TUNNEL_GLORT_USER,
                                               &glortUser);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            FM_CLEAR(mtableEntry);
            mtableEntry.port         = port;
            mtableEntry.vlan         = 0;
            mtableEntry.vlanUpdate   = FALSE;
            mtableEntry.dglortUpdate = TRUE;
            mtableEntry.dglort       = glortUser.glort;

            status = fm10000MTableAddListener(sw,
                                              group->logicalPort,
                                              group->repliGroup,
                                              mtableEntry);
            if (status == FM_OK)
            {
                listener->addedToChip = TRUE;

                status = fm10000AddFlowUser(sw,
                                            listener->listener.info.flowListener.tableIndex,
                                            listener->listener.info.flowListener.flowId);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

                FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                              "mcast group %p (%d), flow tunnel listener "
                              "added to mtable, tableIndex %d, flowId %d, "
                              "port %d, update glort %X\n",
                              (void *) group,
                              group->handle,
                              listener->listener.info.flowListener.tableIndex,
                              listener->listener.info.flowListener.flowId,
                              port,
                              glortUser.glort );
            }

            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    if ( (port == -1) && (vlan == 0) )
    {
        port           = switchPtr->cpuPort;
        isInternalPort = TRUE;
        isPhysicalPort = TRUE;
        portPtr        = NULL;
    }
    else
    {
        portPtr = GET_PORT_PTR(sw, port);

        if (!portPtr)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_PORT);
        }

        isPhysicalPort = fmIsCardinalPort(sw, port);
        isInternalPort = fmIsInternalPort(sw, port);
    }

    if (isPhysicalPort
        && !isInternalPort
        && group->hasL3Resources 
        && !group->readOnlyRepliGroup)
    {
        FM_CLEAR(mtableEntry);

        mtableEntry.vlan       = vlan;
        mtableEntry.port       = port;
    
        if (listener->floodListener)
        {
            /* We do not want to update vlan for mcast flood filtering. */
            mtableEntry.vlanUpdate = FALSE;
            if ( portPtr != NULL )
            {
                mtableEntry.dglortUpdate = TRUE;
                mtableEntry.dglort = 
                            listener->listener.info.portVlanListener.xcastGlort;
            }
        }
        else
        {
            mtableEntry.vlanUpdate = TRUE;
        }
 
        status = fm10000MTableAddListener(sw,
                                          group->logicalPort,
                                          group->repliGroup,
                                          mtableEntry);
        if (status == FM_OK)
        {
            listener->addedToChip = TRUE;

            FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                          "mcast group %p (%d), listener added to mtable, "
                          "port %d, vlan %u\n",
                          (void *) group,
                          group->handle,
                          port,
                          vlan );
        }
    }
    else if ( (portPtr != NULL) && (portPtr->portType == FM_PORT_TYPE_VIRTUAL) )
    {
        groupExt = group->extension;

        if ( !groupExt->hasPepListeners )
        {
            if (!group->hasL3Resources)
            {
                status = EnableMtableGroup(sw, group);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
            }
            groupExt->hasPepListeners = TRUE;
        }

        status = fm10000MapVirtualGlortToLogicalPort(sw,
                                                     portPtr->glort,
                                                     &mtableEntry.port);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

        mtableEntry.vlan         = vlan;

        if (vlan > FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS)
        {
            mtableEntry.vlanUpdate   = TRUE;
        }
        else
        {
            /* VLAN 0 indicates that we do not want to update vlan.
               This is used for mcast flood filtering support. */
            mtableEntry.vlanUpdate   = FALSE; 
        }

        mtableEntry.dglortUpdate = TRUE;
        mtableEntry.dglort       = portPtr->glort;

        status = fm10000MTableAddListener(sw,
                                          group->logicalPort,
                                          group->repliGroup,
                                          mtableEntry);
        if (status == FM_OK)
        {
            listener->addedToChip = TRUE;

            FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                          "mcast group %p (%d), listener added to mtable, "
                          "port %d, vlan %u\n",
                          (void *) group,
                          group->handle,
                          port,
                          listener->listener.info.portVlanListener.vlan );
        }
    }
    else if ( group->l2SwitchingOnly
             || isInternalPort
             || !group->singleAddressMode
             || (vlan == group->defaultVlan)
             || group->readOnlyRepliGroup)
    {
        if ( group->readOnlyRepliGroup )
        {
            FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                          "mcast group %p (%d) readOnlyRepliGroup set, "
                          "using %d repli-group in read-only mode\n",
                          (void *) group,
                          group->handle,
                          group->repliGroup);
        }
        status = fmGetLogicalPortAttribute(sw,
                                           group->logicalPort,
                                           FM_LPORT_DEST_MASK,
                                           &destMask);
        if (status != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);
        }

        status = fmEnablePortInPortMask(sw, &destMask, port);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

        status = fmSetLogicalPortAttribute(sw,
                                           group->logicalPort,
                                           FM_LPORT_DEST_MASK,
                                           &destMask);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

        FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                      "mcast group %p (%d) dest mask updated: %08X%08X%08X\n",
                      (void *) group,
                      group->handle,
                      destMask.maskWord[2],
                      destMask.maskWord[1],
                      destMask.maskWord[0] );
    }
    else
    {
        /* L2 only, single-address-mode, and listener is on a different vlan
         * - just silently ignore the listener */
        status = FM_OK;

        FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                      "mcast group %p (%d) listener ignored, port %d vlan %u\n",
                      (void *) group,
                      group->handle,
                      port,
                      vlan );
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end AddListenerToGroup */




/*****************************************************************************/
/** RemoveListenerFromGroup
 * \ingroup intMulticast
 *
 * \desc            Remove a multicast listener from the hardware resources
 *                  used by the group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the multicast group entry
 *
 * \param[in]       listener points to the multicast listener entry
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if a listener port is invalid
 *
 *****************************************************************************/
static fm_status RemoveListenerFromGroup(fm_int                   sw,
                                         fm_intMulticastGroup *   group,
                                         fm_intMulticastListener *listener)
{
    fm_status           status;
    fm_switch *         switchPtr;
    fm_portmask         destMask;
    fm_bool             isInternalPort;
    fm_int              port;
    fm10000_mtableEntry mtableEntry;
    fm_port            *portPtr;
    fm_uint16           vlan;
    fm_uint32           dglort;
    fm_int              tunnelEngine;
    fm_int              tunnelGroup;
    fm10000_switch *    switchExt;
    fm_tunnelGlortUser  glortUser;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw=%d group=%p<%d> listener=%p\n",
                  sw,
                  (void *) group,
                  group ? group->handle : -1,
                  (void *) listener);

    if (group == NULL || listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_DBG_DUMP_LISTENER(&listener->listener);

    switch (listener->listener.listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            port = listener->listener.info.portVlanListener.port;
            vlan = listener->listener.info.portVlanListener.vlan;
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            status = fm10000DeleteVNDirectTunnelRule(sw,
                                                     listener->listener.info.vnListener.tunnelId,
                                                     listener->listener.info.vnListener.vni,
                                                     &port,
                                                     &dglort);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            FM_CLEAR(mtableEntry);
            mtableEntry.port         = port;
            mtableEntry.vlan         = 0;
            mtableEntry.vlanUpdate   = FALSE;
            mtableEntry.dglortUpdate = TRUE;
            mtableEntry.dglort       = dglort;

            status = fm10000MTableDeleteListener(sw,
                                                 group->logicalPort,
                                                 group->repliGroup,
                                                 mtableEntry,
                                                 TRUE);
            if (status == FM_OK)
            {
                listener->addedToChip = FALSE;

                FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                              "mcast group %p (%d), vn tunnel listener "
                              "deleted from mtable, tunnel %d, vni %u, "
                              "port %d, update glort %X\n",
                              (void *) group,
                              group->handle,
                              listener->listener.info.vnListener.tunnelId,
                              listener->listener.info.vnListener.vni,
                              port,
                              dglort );
            }

            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:

            status = fm10000GetFlowAttribute(sw,
                                             listener->listener.info.flowListener.tableIndex,
                                             FM_FLOW_TABLE_TUNNEL_ENGINE,
                                             (void*)&tunnelEngine);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            if ( (tunnelEngine < 0) ||
                 (tunnelEngine >= FM10000_TE_DGLORT_MAP_ENTRIES_1) )
            {
                status = FM_FAIL;
                FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);
            }

            switchExt = GET_SWITCH_EXT(sw);
            port = switchExt->tunnelCfg->tunnelPort[tunnelEngine];

            status = fm10000GetFlowAttribute(sw,
                                             listener->listener.info.flowListener.tableIndex,
                                             FM_FLOW_TABLE_TUNNEL_GROUP,
                                             (void*)&tunnelGroup);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            status = fm10000GetTunnelAttribute(sw,
                                               tunnelGroup,
                                               listener->listener.info.flowListener.flowId,
                                               FM_TUNNEL_GLORT_USER,
                                               &glortUser);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            FM_CLEAR(mtableEntry);
            mtableEntry.port         = port;
            mtableEntry.vlan         = 0;
            mtableEntry.vlanUpdate   = FALSE;
            mtableEntry.dglortUpdate = TRUE;
            mtableEntry.dglort       = glortUser.glort;

            status = fm10000MTableDeleteListener(sw,
                                                 group->logicalPort,
                                                 group->repliGroup,
                                                 mtableEntry,
                                                 TRUE);
            if (status == FM_OK)
            {
                listener->addedToChip = FALSE;

                status = fm10000DelFlowUser(sw,
                                            listener->listener.info.flowListener.tableIndex,
                                            listener->listener.info.flowListener.flowId);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

                FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                              "mcast group %p (%d), flow tunnel listener "
                              "deleted to mtable, tableIndex %d, flowId %d, "
                              "port %d, update glort %X\n",
                              (void *) group,
                              group->handle,
                              listener->listener.info.flowListener.tableIndex,
                              listener->listener.info.flowListener.flowId,
                              port,
                              glortUser.glort );
            }

            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    if ( (port == -1) && (vlan == 0) )
    {
        port           = switchPtr->cpuPort;
        isInternalPort = TRUE;
        portPtr        = NULL;
    }
    else
    {
        isInternalPort = fmIsInternalPort(sw, port);
        
        portPtr = GET_PORT_PTR(sw, port);

        if (!portPtr)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_PORT);
        }
    }

    if (listener->addedToChip)
    {
        FM_CLEAR(mtableEntry);

        mtableEntry.vlan       = vlan;
        mtableEntry.vlanUpdate = TRUE;
        mtableEntry.port       = port;

        if ( (portPtr != NULL) && (portPtr->portType == FM_PORT_TYPE_VIRTUAL) )
        {
            status = fm10000MapVirtualGlortToLogicalPort(sw,
                                                         portPtr->glort,
                                                         &mtableEntry.port);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            mtableEntry.dglortUpdate = TRUE;
            mtableEntry.dglort       = portPtr->glort;

            if (vlan == 0)
            {
                mtableEntry.vlanUpdate = FALSE;
            }

        }
        else if (listener->floodListener)
        {
            mtableEntry.vlanUpdate = FALSE;
            if ( portPtr != NULL )
            {
                mtableEntry.dglortUpdate = TRUE;
                mtableEntry.dglort = 
                            listener->listener.info.portVlanListener.xcastGlort;
            }
        }

        status = fm10000MTableDeleteListener(sw,
                                            group->logicalPort,
                                            group->repliGroup,
                                            mtableEntry,
                                            TRUE);
        if (status == FM_OK)
        {
            listener->addedToChip = FALSE;
        }
    }
    else if ( group->l2SwitchingOnly
             || isInternalPort
             || !group->singleAddressMode
             || (vlan == group->defaultVlan)
             || group->readOnlyRepliGroup )
    {
        status = fmGetLogicalPortAttribute(sw,
                                           group->logicalPort,
                                           FM_LPORT_DEST_MASK,
                                           &destMask);

        if (status != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);
        }

        fmDisablePortInPortMask(sw, &destMask, port);

        status = fmSetLogicalPortAttribute(sw,
                                           group->logicalPort,
                                           FM_LPORT_DEST_MASK,
                                           &destMask);
    }
    else
    {
        /* L2 only and listener was on a different vlan and was ignored,
         * so continue to ignore it. */
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end RemoveListenerFromGroup */




/*****************************************************************************/
/** AddFloodingPortsToMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Function adds flooding ports to a multicast group.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate locks and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_ALREADY_EXISTS if this listener is already in the
 *                  multicast group.
 *
 *****************************************************************************/
static fm_status AddFloodingPortsToMcastGroup(fm_int sw,
                                              fm_int mcastGroup)
{
    fm_status             status;
    fm_status             cleanupStatus;
    fm_multicastListener  listener;
    fm_multicastListener  nextListener;
    fm_mcastGroupListener tempListener;
    fm_mailboxInfo *      info;
    fm_portmask           mcastFloodDestMask;
    fm_int                i;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d\n",
                 sw,
                 mcastGroup);

    info          = GET_MAILBOX_INFO(sw);

    FM_PORTMASK_DISABLE_ALL(&mcastFloodDestMask);

    /* Get mcast flooding portmask. */
    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_MCAST,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &mcastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* Adding ports from mcast flooding portmask as listeners */
    for (i = 0 ; i < FM_PORTMASK_NUM_BITS ; i++)
    {
        if ( FM_PORTMASK_IS_BIT_SET(&mcastFloodDestMask, i) )
        {
            FM_CLEAR(tempListener);

            tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
            tempListener.info.portVlanListener.vlan = 
                         FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
            tempListener.info.portVlanListener.port = GET_LOGICAL_PORT(sw, i);

            status = fmAddMcastGroupListenerInternalForFlood(sw,
                                                             mcastGroup,
                                                             &tempListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    /* Add virtual ports from flooding list */
    FM_CLEAR(listener);
    FM_CLEAR(nextListener);
    status = fmGetMcastGroupListenerFirst(sw,
                                          info->mcastGroupForMcastFlood,
                                          &listener);

    while (status != FM_ERR_NO_MORE)
    {
        if (status == FM_OK)
        {
            FM_CLEAR(tempListener);

            tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
            tempListener.info.portVlanListener.vlan = listener.vlan;
            tempListener.info.portVlanListener.port = listener.port;

            status = fmAddMcastGroupListenerInternalForFlood(sw,
                                                             mcastGroup,
                                                             &tempListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            status = fmGetMcastGroupListenerNext(sw,
                                                 info->mcastGroupForMcastFlood,
                                                 &listener,
                                                 &nextListener);
            if (status == FM_OK)
            {
                listener = nextListener;
            }
            else if (status != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
            }
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    status = FM_OK;


ABORT:

    /* Cleanup if error */
    if (status != FM_OK)
    {
        /* Try to remove all flooding listeners already added */

        for (i = 0 ; i < FM_PORTMASK_NUM_BITS ; i++)
        {
            if ( FM_PORTMASK_IS_BIT_SET(&mcastFloodDestMask, i) )
            {
                FM_CLEAR(tempListener);

                tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
                tempListener.info.portVlanListener.vlan =
                             FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
                tempListener.info.portVlanListener.port = GET_LOGICAL_PORT(sw, i);


                cleanupStatus = fmDeleteMcastGroupListenerInternalForFlood(sw,
                                                                           mcastGroup,
                                                                           &tempListener);

                if ( (cleanupStatus != FM_OK) &&
                     (cleanupStatus != FM_ERR_NOT_FOUND) )
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                 "Cleanup for mcastGroup = %d"
                                 " listener port = %d vlan = %d"
                                 " failed with err = %d\n",
                                 mcastGroup,
                                 i,
                                 FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS,
                                 cleanupStatus);
                }
            }
        }

        cleanupStatus = fmGetMcastGroupListenerFirst(sw,
                                                     info->mcastGroupForMcastFlood,
                                                     &listener);

        while (cleanupStatus != FM_ERR_NO_MORE)
        {
            if (cleanupStatus == FM_OK)
            {
                FM_CLEAR(tempListener);

                tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
                tempListener.info.portVlanListener.vlan = listener.vlan;
                tempListener.info.portVlanListener.port = listener.port;

                cleanupStatus = fmDeleteMcastGroupListenerInternalForFlood(sw,
                                                                           mcastGroup,
                                                                           &tempListener);

                if ( (cleanupStatus != FM_OK) &&
                     (cleanupStatus != FM_ERR_NOT_FOUND) )
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                 "Cleanup for mcastGroup = %d"
                                 " listener port = %d vlan = %d"
                                 " failed with err = %d\n",
                                 mcastGroup,
                                 listener.port,
                                 listener.vlan,
                                 cleanupStatus);
                }

                cleanupStatus = fmGetMcastGroupListenerNext(sw,
                                                            info->mcastGroupForMcastFlood,
                                                            &listener,
                                                            &nextListener);

                if (cleanupStatus == FM_OK)
                {
                    listener = nextListener;
                }
                else if (cleanupStatus != FM_ERR_NO_MORE)
                {
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, cleanupStatus);
                }
            }
            else if (cleanupStatus != FM_ERR_NO_MORE)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST, 
                             "Cleanup for mcastGroup = %d"
                             " failed with err = %d\n",
                             mcastGroup,
                             cleanupStatus);
                break;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end AddFloodingPortsToMcastGroup */




/*****************************************************************************/
/** RemoveFloodingPortsFromMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Function removes flooding ports from a multicast group.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate locks and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NOT_FOUND if listener is not found in the
 *                  multicast group.
 *
 *****************************************************************************/
static fm_status RemoveFloodingPortsFromMcastGroup(fm_int sw,
                                                   fm_int mcastGroup)
{
    fm_status             status;
    fm_status             cleanupStatus;
    fm_multicastListener  listener;
    fm_multicastListener  nextListener;
    fm_mcastGroupListener tempListener;
    fm_mailboxInfo *      info;
    fm_portmask           mcastFloodDestMask;
    fm_int                i;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d\n",
                 sw,
                 mcastGroup);

    info          = GET_MAILBOX_INFO(sw);

    FM_PORTMASK_DISABLE_ALL(&mcastFloodDestMask);

    /* Get mcast flooding portmask. */
    status = fmGetLogicalPortAttribute(sw,
                                       FM_PORT_MCAST,
                                       FM_LPORT_DEST_MASK,
                                       (void *) &mcastFloodDestMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* Removing mcast flooding portmask ports */
    for (i = 0 ; i < FM_PORTMASK_NUM_BITS ; i++)
    {
        if ( FM_PORTMASK_IS_BIT_SET(&mcastFloodDestMask, i) )
        {
            FM_CLEAR(tempListener);

            tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
            tempListener.info.portVlanListener.vlan =
                         FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
            tempListener.info.portVlanListener.port = GET_LOGICAL_PORT(sw, i);


            status = fmDeleteMcastGroupListenerInternalForFlood(sw,
                                                                mcastGroup,
                                                                &tempListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    /* Remove virtual ports */
    FM_CLEAR(listener);
    FM_CLEAR(nextListener);
    status = fmGetMcastGroupListenerFirst(sw,
                                          info->mcastGroupForMcastFlood,
                                          &listener);

    while (status != FM_ERR_NO_MORE)
    {
        if (status == FM_OK)
        {
            FM_CLEAR(tempListener);

            tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
            tempListener.info.portVlanListener.vlan = listener.vlan;
            tempListener.info.portVlanListener.port = listener.port;

            status = fmDeleteMcastGroupListenerInternalForFlood(sw,
                                                                mcastGroup,
                                                                &tempListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            status = fmGetMcastGroupListenerNext(sw,
                                                 info->mcastGroupForMcastFlood,
                                                 &listener,
                                                 &nextListener);
            if (status == FM_OK)
            {
                listener = nextListener;
            }
            else if (status != FM_ERR_NO_MORE)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
            }
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    status = FM_OK;


ABORT:

    /* Cleanup if error */
    if (status != FM_OK)
    {
        /* Try to add all flooding listeners already removed */

        for (i = 0 ; i < FM_PORTMASK_NUM_BITS ; i++)
        {
            if ( FM_PORTMASK_IS_BIT_SET(&mcastFloodDestMask, i) )
            {
                FM_CLEAR(tempListener);

                tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
                tempListener.info.portVlanListener.vlan =
                             FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
                tempListener.info.portVlanListener.port = GET_LOGICAL_PORT(sw, i);

                cleanupStatus = fmAddMcastGroupListenerInternalForFlood(sw,
                                                                        mcastGroup,
                                                                        &tempListener);

                if ( (cleanupStatus != FM_OK) &&
                     (cleanupStatus != FM_ERR_ALREADY_EXISTS) )
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                 "Cleanup for mcastGroup = %d"
                                 " listener port = %d vlan = %d"
                                 " failed with err = %d\n",
                                 mcastGroup,
                                 i,
                                 FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS,
                                 cleanupStatus);
                }
            }
        }

        cleanupStatus = fmGetMcastGroupListenerFirst(sw,
                                                     info->mcastGroupForMcastFlood,
                                                     &listener);

        while (cleanupStatus != FM_ERR_NO_MORE)
        {
            if (cleanupStatus == FM_OK)
            {
                FM_CLEAR(tempListener);

                tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
                tempListener.info.portVlanListener.vlan = listener.vlan;
                tempListener.info.portVlanListener.port = listener.port;

                cleanupStatus = fmAddMcastGroupListenerInternalForFlood(sw,
                                                                        mcastGroup,
                                                                        &tempListener);

                if ( (cleanupStatus != FM_OK) &&
                     (cleanupStatus != FM_ERR_ALREADY_EXISTS) )
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                 "Cleanup for mcastGroup = %d"
                                 " listener port = %d vlan = %d"
                                 " failed with err = %d\n",
                                 mcastGroup,
                                 listener.port,
                                 listener.vlan,
                                 cleanupStatus);
                }

                cleanupStatus = fmGetMcastGroupListenerNext(sw,
                                                            info->mcastGroupForMcastFlood,
                                                            &listener,
                                                            &nextListener);

                if (cleanupStatus == FM_OK)
                {
                    listener = nextListener;
                }
                else if (cleanupStatus != FM_ERR_NO_MORE)
                {
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, cleanupStatus);
                }
            }
            else if (cleanupStatus != FM_ERR_NO_MORE)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Cleanup for mcastGroup = %d"
                             " failed with err = %d\n",
                             mcastGroup,
                             cleanupStatus);
                break;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end RemoveFloodingPortsFromMcastGroup */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000McastGroupInit
 * \ingroup intMulticast
 *
 * \desc            Perform initialization for multicast group subsystem,
 *                  called at switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       swagInit is FALSE if this function is being called
 *                  during initial switch boot, TRUE if it is being called
 *                  after all SWAG multicast group glorts have been allocated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000McastGroupInit(fm_int sw, fm_bool swagInit)
{
    fm_status       status;
    fm_switch      *switchPtr;
    fm_glortRange  *glorts;
    fm_int          baseHandle;
    fm_int          numHandles;
    fm_int          step;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw=%d, swagInit=%d\n", sw, swagInit);

    switchPtr = GET_SWITCH_PTR(sw);
    glorts    = &switchPtr->glortRange;

    /* If the switch is in a SWAG, defer execution until after SWAG
     * multicast groups have been allocated to prevent local multicast
     * glorts from being hidden behind SWAG multicast glorts. */
    if ( (switchPtr->swag >= 0) && !swagInit )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    if (glorts->mcastCount > 0)
    {
        /* This is preallocated at startup and never freed. */
        status = fmAllocateMcastHandles(sw,
                                        glorts->mcastBaseGlort, 
                                        glorts->mcastCount, 
                                        &baseHandle,
                                        &numHandles,
                                        &step);
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000McastGroupInit */




/*****************************************************************************/
/** fm10000AllocateMcastGroups
 * \ingroup intMulticast
 *
 * \desc            Allocate pool of multicast groups given a glort range. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the start glort to reserve for multicast.
 *
 * \param[in]       glortSize is the size of the glort space to reserve. This
 *                  value must be a power of two.
 *
 * \param[out]      baseMcastGroupHandle points to caller-provided storage
 *                  where this function should store base handle for this
 *                  resource space.
 *
 * \param[out]      numMcastGroups points to caller-provided storage where
 *                  this function should store the number of handles from the
 *                  base, which the caller can use to create multicast groups
 *                  from the base.
 *
 * \param[out]      step points to caller-allocated storage 
 *                  where this function should place the step value, where
 *                  the caller can increment from base by to get
 *                  subsequent multicast groups
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if input parameters fail checking.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort in the given
 *                  glort range is being used.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more resoures are available
 *
 *****************************************************************************/
fm_status fm10000AllocateMcastGroups(fm_int  sw,
                                     fm_uint startGlort,
                                     fm_uint glortSize,
                                     fm_int *baseMcastGroupHandle,
                                     fm_int *numMcastGroups,
                                     fm_int *step)
{
    fm_status status;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, startGlort = 0x%x, glortSize = 0x%x, "
                  "baseMcastGroupHandle = %p, numMcastGroups = %p, "
                  "step = %p\n",
                  sw,
                  startGlort,
                  glortSize,
                  (void *) baseMcastGroupHandle,
                  (void *) numMcastGroups,
                  (void *) step );

    status =  fmAllocateMcastHandles(sw, 
                                     startGlort,
                                     glortSize,
                                     baseMcastGroupHandle,
                                     numMcastGroups,
                                     step);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000AllocateMcastGroups */




/*****************************************************************************/
/** fm10000FreeMcastGroups
 * \ingroup intMulticast
 *
 * \desc            Free the multicast resources allocated by
 *                  ''fm6000AllocateMcastGroups''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseMcastGroupHandle is the base handle returned from
 *                  ''fm6000AllocateMcastResources''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if handle is not found
 *                  in allocated entries.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fm10000FreeMcastGroups(fm_int sw,
                                 fm_int baseMcastGroupHandle)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, baseMcastGroupHandle = %d\n",
                 sw,
                 baseMcastGroupHandle);

    status =  fmFreeMcastHandles(sw, baseMcastGroupHandle);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000FreeMcastGroups */




/*****************************************************************************/
/** fm10000CreateMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Create a multicast group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the multicast group entry
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CreateMcastGroup(fm_int sw, fm_intMulticastGroup *group)
{
    fm10000_MulticastGroup *groupExt;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "sw=%d group=%p<%d>\n",
                 sw,
                 (void *) group,
                 group ? group->handle : -1 );

    FM_NOT_USED(sw);

    if (group == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    groupExt = fmAlloc( sizeof(fm10000_MulticastGroup) );
    if (groupExt == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_NO_MEM);
    }

    groupExt->mtableDestIndex = -1;
    groupExt->hasPepListeners = FALSE;
    group->extension          = groupExt;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000CreateMcastGroup */




/*****************************************************************************/
/** fm10000DeleteMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Delete a multicast group, releasing all of its resources.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the multicast group entry
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteMcastGroup(fm_int sw, fm_intMulticastGroup *group)
{
    FM_NOT_USED(sw);

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw=%d group=%p<%d>\n",
                  sw,
                  (void *) group,
                  group ? group->handle : -1 );

    FM_LOG_ASSERT(FM_LOG_CAT_MULTICAST,
                  group != NULL,
                  "group is NULL!\n");

    if (group == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_ASSERT(FM_LOG_CAT_MULTICAST,
                  !group->hasL3Resources,
                  "group still has L3 resources during group deletion!");

    fmFree(group->extension);
    group->extension = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000DeleteMcastGroup */




/*****************************************************************************/
/** fm10000AddMulticastListener
 * \ingroup intMulticast
 *
 * \desc            Add a multicast listener to a multicast group.
 * \note            This function assumes that it will only be executed
 *                  when the group is active.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       listener points to the listener entry which is being
 *                  added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if group or listener is null.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fm10000AddMulticastListener(fm_int                   sw,
                                     fm_intMulticastGroup *   group,
                                     fm_intMulticastListener *listener)
{
    fm_status status;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw=%d group=%p<%d> listener=%p\n",
                  sw,
                  (void *) group,
                  group ? group->handle : -1,
                  (void *) listener);
    FM_DBG_DUMP_LISTENER(&listener->listener);

    if (group == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    if (!group->activated)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_MCAST_GROUP_NOT_ACTIVE);
    }

    status = AddListenerToGroup(sw, group, listener);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000AddMulticastListener */




/*****************************************************************************/
/** fm10000DeleteMulticastListener
 * \ingroup intMulticast
 *
 * \desc            Delete a multicast listener from a multicast group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       listener points to the listener entry which is being
 *                  deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteMulticastListener(fm_int                   sw,
                                         fm_intMulticastGroup *   group,
                                         fm_intMulticastListener *listener)
{
    fm_status status;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw=%d group=%p<%d> listener=%p\n",
                  sw,
                  (void *) group,
                  group ? group->handle : -1,
                  (void *) listener);
    FM_DBG_DUMP_LISTENER(&listener->listener);

    status = RemoveListenerFromGroup(sw, group, listener);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000DeleteMulticastListener */




/*****************************************************************************/
/** fm10000ActivateMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Activates a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group's table.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fm10000ActivateMcastGroup(fm_int sw, fm_intMulticastGroup *group)
{
    fm_status   status;
    fm_bool     needL3Resources;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d)\n",
                  sw,
                  (void *) group,
                  group->handle );

    if (group->l2SwitchingOnly)
    {
        needL3Resources = FALSE;
    }
    else if (group->singleAddressMode)
    {
        if ( (group->logicalPort != FM_LOGICAL_PORT_NONE)
            && (group->singleMcastAddr->addr.addressType
                != FM_MCAST_ADDR_TYPE_L2MAC_VLAN) )
        {
            needL3Resources = TRUE;
        }
        else
        {
            needL3Resources = FALSE;
        }
    }
    else if (group->logicalPort != FM_LOGICAL_PORT_NONE)
    {
        needL3Resources = TRUE;
    }
    else
    {
        needL3Resources = FALSE;
    }

    if (needL3Resources)
    {
        status = EnableMtableGroup(sw, group);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

        group->hasL3Resources = TRUE;
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000ActivateMcastGroup */




/*****************************************************************************/
/** fm10000DeactivateMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Deactivates a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group's table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeactivateMcastGroup(fm_int sw, fm_intMulticastGroup *group)
{
    fm_status               status;
    fm10000_MulticastGroup *groupExt;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d)\n",
                  sw,
                  (void *) group,
                  group->handle );

    groupExt = group->extension;
    if (group->hasL3Resources || groupExt->hasPepListeners)
    {
        status = fm10000MTableDisableGroup(sw, 
                                          group->logicalPort,
                                          group->repliGroup, 
                                          group->privateGroup);
        if (status == FM_OK)
        {
            group->hasL3Resources     = FALSE;
            groupExt->hasPepListeners = FALSE;

            if (group->privateGroup)
            {
                /* The private replication group has been deleted */
                group->repliGroup = FM_MCASTGROUP_REPLICATION_GROUP_DISABLE;
                groupExt->mtableDestIndex = -1;
            }
        }
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000DeactivateMcastGroup */




/*****************************************************************************/
/** fm10000McastAddPortToLagReserveResources
 * \ingroup intMcast
 *
 * \desc            Called when a port is going to be added to a lag,
 *                  determines whether sufficient multicast listener
 *                  table entries are available.
 *                  NOTE: THIS FUNCTION TAKES THE ROUTER LOCK AND HOLDS
 *                  IT.  THE fm10000McastReleaseReservation FUNCTION MUST BE
 *                  CALLED TO RELEASE THIS LOCK AFTER THE ADDPORT OPERATION
 *                  COMPLETES OR FAILS.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       lagIndex is the internal index of the LAG to which
 *                  the port is being added.
 *
 * \param[in]       port is the logical port number for the port being added.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000McastAddPortToLagReserveResources(fm_int sw,
                                                   fm_int lagIndex,
                                                   fm_int port)
{
    fm_switch *switchPtr;
    fm_port   *portPtr;
    fm_port   *lagPortPtr;
    fm_int     portListeners;
    fm_int     lagListeners;
    fm_int     availableListeners;
    fm_int     remainingListeners;
    fm_int     lagLogicalPort;
    fm_status  status;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    status = fmLagIndexToLogicalPort(sw, lagIndex, &lagLogicalPort);

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "fmLagIndextoLogicalPort returned error %d\n",
                     status);

        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);
    }

    switchPtr  = GET_SWITCH_PTR(sw);
    lagPortPtr = GET_PORT_PTR(sw, lagLogicalPort);
    portPtr    = GET_PORT_PTR(sw, port);

    /* Take the Route Lock.  Unless an error occurs later in this function,
     * the lock will NOT be released before the function returns to its
     * caller - the lock WILL BE held!
     */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* get the number of multicast-group/vlan pairs attached to the port */
    portListeners = (fm_int) fmTreeSize(&portPtr->mcastGroupList);

    /* get the number of multicast-group/vlan pairs attached to the LAG */
    lagListeners = (fm_int) fmTreeSize(&lagPortPtr->mcastGroupList);

    /* get the number of available hardware listener entries */
    status = fm10000GetAvailableMulticastListenerCount(sw, &availableListeners);
    if (status != FM_OK)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);
    }

    /* Determine if sufficient resources are available */
    remainingListeners = availableListeners + portListeners - lagListeners;

    /* If there aren't enough, report an error */
    if (remainingListeners < 0)
    {
        status = FM_ERR_NO_MCAST_RESOURCES;
        fmReleaseWriteLock(&switchPtr->routingLock);
    }
    else
    {
        status = FM_OK;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_MULTICAST,
                       status,
                       "status = %d, portListeners = %d, lagListeners = %d, "
                       "avail = %d\n",
                       status,
                       portListeners,
                       lagListeners,
                       availableListeners);

}   /* end fm10000McastAddPortToLagReserveResources */




/*****************************************************************************/
/** fm10000McastReleaseReservation
 * \ingroup intMcast
 *
 * \desc            Called to release any pending multicast listener
 *                  resource reservation.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000McastReleaseReservation(fm_int sw)
{
    fm_switch *              switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Release the Route Lock. */
    fmReleaseWriteLock(&switchPtr->routingLock);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000McastReleaseReservation */




/*****************************************************************************/
/** fm10000FreeMcastResource
 * \ingroup intMcast
 *
 * \desc            Free the multicast-related resource.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
fm_status fm10000FreeMcastResource(fm_int sw)
{
    fm_switch *         switchPtr;
    fm_logicalPortInfo *lportInfo;
    fm_int              i;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->portTable == NULL)
    {
        /* Not init yet, probably failed in middle of initialization */
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    for (i = 0 ; i < FM_MAX_LOGICAL_PORT ; i++)
    {
        if (switchPtr->portTable[i])
        {
            fmTreeDestroy(&switchPtr->portTable[i]->mcastGroupList, NULL);
            if (switchPtr->portTable[i]->portType == FM_PORT_TYPE_MULTICAST)
            {
                /* Need to free the port before the allocated mcast can be freed */
                fm10000FreeLogicalPort(sw, i);
            }
        }
    }

    /* Free any allocated mcast resources */
    lportInfo = &switchPtr->logicalPortInfo;
    for (i = 0 ; i < FM_MCG_ALLOC_TABLE_SIZE ; i++)
    {
        if (lportInfo->mcgAllocTable[i].glortSize)
        {
            fmFreeMcastHandles(sw, lportInfo->mcgAllocTable[i].baseHandle);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000FreeMcastResource */




/*****************************************************************************/
/** fm10000SetMcastGroupAttribute
 * \ingroup intMcast
 *
 * \desc            Set a multicast group attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       attr is the multicast group attribute to set
 *                  (see 'Multicast Group Attributes').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the multicast group has
 *                  been activated and an attempt is made to change an
 *                  attribute that can only be modified when the group
 *                  is not active.
 *
 *****************************************************************************/
fm_status fm10000SetMcastGroupAttribute(fm_int                sw,
                                        fm_intMulticastGroup *group,
                                        fm_int                attr,
                                        void *                value)
{
    fm_status               status = FM_OK;
    fm_bool                 enabled;
    fm_routeState           routeState;
    fm_int                  newRepliGroup;
    fm_int                  currentRepliGroup;
    fm_intMulticastListener listener;
    fm_int                  mtuIndex;
    fm_int                  logicalPort;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p, attr = %d, value = %p\n",
                  sw,
                  (void *) group,
                  attr,
                  (void *) value );

    switch (attr)
    {
        case FM_MCASTGROUP_FWD_TO_CPU:
            enabled = *( (fm_bool *) value );

            if (enabled == group->fwdToCpu)
            {
                status = FM_OK;
                break;
            }

            group->fwdToCpu = enabled;

            FM_CLEAR(listener);

            listener.listener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
            listener.listener.info.portVlanListener.port = -1;
            listener.listener.info.portVlanListener.vlan = 0;

            if (enabled)
            {
                status = AddListenerToGroup(sw, group, &listener);
            }
            else
            {
                status = RemoveListenerFromGroup(sw, group, &listener);
            }
            break;

        case FM_MCASTGROUP_L2_SWITCHING_ONLY:
            enabled = *( (fm_bool *) value );

            /* This parameter cannot be set for an active group. */
            if (group->activated)
            {
                status = FM_ERR_MCAST_GROUP_ACTIVE;
                break;
            }

            group->l2SwitchingOnly = enabled;
            break;

        case FM_MCASTGROUP_USED:
            if (*(( fm_bool *) value) == FALSE)
            {
                fm_bool unused;

                status = fmGetMcastGroupUsedInt(sw,
                                                group->handle,
                                                &unused,
                                                TRUE);
            }
            break;

        case FM_MCASTGROUP_ACTION:
            if (group->activated)
            {
                status = FM_ERR_MCAST_GROUP_ACTIVE;
                break;
            }

            group->groupAction = *( (fm_routeAction *) value );
            break;

        case FM_MCASTGROUP_STATE:
            routeState = *( (fm_routeState *) value );

            if (group->groupState != routeState)
            {
                group->groupState = routeState;

                status = fmSetMcastGroupRouteActiveFlags(sw, group, routeState);
            }
            break;

        case FM_MCASTGROUP_SHARED_REPLICATION_GROUP:
            newRepliGroup = *( (fm_int *) value );
            currentRepliGroup = group->privateGroup ? -1 : group->repliGroup;

            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "mcastgroup %d newRepliGroup %d, currentRepliGroup %d\n",
                         group->handle,
                         newRepliGroup,
                         currentRepliGroup);

            if ( newRepliGroup != currentRepliGroup )
            {
                status = fmMoveReplicationGroupMcastGroupInt(sw, 
                                                             newRepliGroup, 
                                                             group->handle);
            }
            break;

        case FM_MCASTGROUP_L3_SWITCHING_ONLY:
            enabled = *( (fm_bool *) value );

            group->l3SwitchingOnly = enabled;

            if (group->activated)
            {
                status = fm10000UpdateNextHopMulticast(sw, group->ecmpGroup);
            }
            else
            {
                status = FM_OK;
            }
            break;

        case FM_MCASTGROUP_L3_FLOOD_SET:
            enabled = *( (fm_bool *) value );

            group->l3FloodSet = enabled;

            if (group->activated)
            {
                status = fm10000UpdateNextHopMulticast(sw, group->ecmpGroup);
            }
            else
            {
                status = FM_OK;
            }
            break;

        case FM_MCASTGROUP_L2_FLOOD_SET:
            enabled = *( (fm_bool *) value );

            /* This parameter cannot be set for an active group. */
            if (group->activated)
            {
                status = FM_ERR_MCAST_GROUP_ACTIVE;
                break;
            }

            group->l2FloodSet = enabled;
            break;

        case FM_MCASTGROUP_BYPASS_EGRESS_STP_CHECK:
            enabled = *( (fm_bool *) value );

            /* This parameter cannot be set for an active group. */
            if (group->activated)
            {
                status = FM_ERR_MCAST_GROUP_ACTIVE;
                break;
            }

            group->bypassEgressSTPCheck = enabled;
            break;

        case FM_MCASTGROUP_MTU_INDEX:
            mtuIndex = *( (fm_int *) value );
            if ( (mtuIndex < 0) || ( mtuIndex >= (fm_int) FM10000_MTU_TABLE_ENTRIES ) )
            {
                status = FM_ERR_INVALID_ARGUMENT;
                break;
            }

            group->mtuIndex = mtuIndex;

            if (group->activated)
            {
                status = fm10000UpdateNextHopMulticast(sw, group->ecmpGroup);
            }
            else
            {
                status = FM_OK;
            }
            break;

        case FM_MCASTGROUP_READ_ONLY_REPLI_GROUP:
            enabled = *( (fm_bool *) value );
            /* This parameter cannot be set for an active group. */
            if (group->activated)
            {
                status = FM_ERR_MCAST_GROUP_ACTIVE;
                break;
            }

            group->readOnlyRepliGroup = enabled;
            break;

        case FM_MCASTGROUP_LOGICAL_PORT:
            logicalPort = *( (fm_int *) value );
            /* This parameter cannot be set for an active group. */
            if (group->activated)
            {
                status = FM_ERR_MCAST_GROUP_ACTIVE;
                break;
            }

            group->attrLogicalPort = logicalPort;
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000SetMcastGroupAttribute */




/*****************************************************************************/
/** fm10000GetMcastGroupAttribute
 * \ingroup intMcast
 *
 * \desc            Get a multicast group attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       attr is the multicast group attribute to get
 *                  (see 'Multicast Group Attributes').
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function is to place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetMcastGroupAttribute(fm_int                sw,
                                        fm_intMulticastGroup *group,
                                        fm_int                attr,
                                        void *                value)
{
    fm_status status;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p, attr = %d, value = %p\n",
                  sw,
                  (void *) group,
                  attr,
                  (void *) value );
        
    switch (attr)
    {
        case FM_MCASTGROUP_FWD_TO_CPU:
            *( (fm_bool *) value ) = group->fwdToCpu;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_USED:
            status = fmGetMcastGroupUsedInt(sw,
                                            group->handle,
                                            (fm_bool *) value,
                                            TRUE);
            break;

        case FM_MCASTGROUP_L2_SWITCHING_ONLY:
            *( (fm_bool *) value ) = group->l2SwitchingOnly;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_ACTION:
            *( (fm_routeAction *) value ) = group->groupAction;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_STATE:
            *( (fm_routeState *) value ) = group->groupState;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_SHARED_REPLICATION_GROUP:
            if (group->privateGroup)
            {
                *( (fm_int *) value ) = FM_MCASTGROUP_REPLICATION_GROUP_DISABLE;
            }
            else
            {
                *( (fm_int *) value ) = group->repliGroup;
            }
            status = FM_OK;
            break;

        case FM_MCASTGROUP_L3_SWITCHING_ONLY:
            *( (fm_bool *) value ) = group->l3SwitchingOnly;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_L3_FLOOD_SET:
            *( (fm_bool *) value ) = group->l3FloodSet;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_L2_FLOOD_SET:
            *( (fm_bool *) value ) = group->l2FloodSet;
            status = FM_OK;
            break;

        case FM_MCASTGROUP_BYPASS_EGRESS_STP_CHECK:
            *( (fm_bool *) value ) = group->bypassEgressSTPCheck;
            status = FM_OK;
            break;
        case FM_MCASTGROUP_READ_ONLY_REPLI_GROUP:
            *( (fm_bool *) value ) = group->readOnlyRepliGroup;
            status = FM_OK;
            break;

       case FM_MCASTGROUP_LOGICAL_PORT:
            *( (fm_int *) value ) = group->attrLogicalPort;
            status = FM_OK;
            break;

       case FM_MCASTGROUP_MTU_INDEX:
            *( (fm_int *) value ) = group->mtuIndex;
            status = FM_OK;
            break;

        default:
            status = FM_ERR_INVALID_ATTRIB;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000GetMcastGroupAttribute */




/*****************************************************************************/
/** fm10000GetHardwareMcastGlortRange
 * \ingroup intMcast
 *
 * \desc            Get the glort range usable for multicast groups.
 *
 * \param[out]      mcastGlortBase points to caller-provided storage into
 *                  which the base glort value will be written.
 *
 * \param[out]      mcastGlortCount points to caller-provided storage into
 *                  which the number of usable glorts will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetHardwareMcastGlortRange(fm_uint32 *mcastGlortBase,
                                            fm_uint32 *mcastGlortCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                "mcastGlortBase=%p, mcastGlortCount=%p\n",
                 (void *) mcastGlortBase,
                 (void *) mcastGlortCount);

    if (mcastGlortBase != NULL)
    {
        *mcastGlortBase = FM10000_GLORT_MCAST_BASE;
    }

    if (mcastGlortCount != NULL)
    {
        *mcastGlortCount = FM10000_GLORT_MCAST_SIZE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000GetHardwareMcastGlortRange */




/*****************************************************************************/
/** fm10000GetMcastGroupHwIndex
 * \ingroup intMcast
 *
 * \desc            Gets the multicast group's mcast dest table index.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[out]      hwIndex points to caller-provided storage into which
 *                  this function is to place the index value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetMcastGroupHwIndex(fm_int                sw,
                                      fm_intMulticastGroup *group,
                                      fm_int *              hwIndex)
{
    fm10000_MulticastGroup *groupExt;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d), hwIndex = %p\n",
                  sw,
                  (void *) group,
                  group->handle,
                  (void *) hwIndex );

    groupExt = group->extension;
    *hwIndex = groupExt->mtableDestIndex;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000GetMcastGroupHwIndex */




/*****************************************************************************/
/** fm10000ConfigureMcastGroupAsHNIFlooding
 * \ingroup intMulticast
 *
 * \desc            Configure mcast group as HNI flooding. These groups are
 *                  requested by HNI and have all multicast flooding ports
 *                  added as listeners. When disabling, all listeners
 *                  with flooding port will be deleted. This functionality 
 *                  is to allow filtering multicast flooded frames to
 *                  virtual ports.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       isHNIFlooding flag should be set to TRUE to configure
 *                  multicast group as HNI flooding. To configure it
 *                  back, set it to FALSE.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if this switch does not support this
 *                  request.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fm10000ConfigureMcastGroupAsHNIFlooding(fm_int  sw,
                                                  fm_int  mcastGroup,
                                                  fm_bool isHNIFlooding)
{
    fm_status             status;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, isHNIFlooding = %d\n",
                 sw,
                 mcastGroup,
                 isHNIFlooding);

    if (isHNIFlooding)
    {
        status = AddFloodingPortsToMcastGroup(sw, mcastGroup);
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }
    else
    {
        status = RemoveFloodingPortsFromMcastGroup(sw, mcastGroup);
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000ConfigureMcastGroupAsHNIFlooding */




/*****************************************************************************/
/** fm10000UpdateMcastHNIFloodingGroups
 * \ingroup intMulticast
 *
 * \desc            Updates mcast groups configured as HNI flooding. This is a
 *                  chip specific function calling the top-level function to
 *                  update multicast groups.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the port number to be updated.
 *
 * \param[in]       state is TRUE if the port should be added to mcast groups,
 *                  or FALSE if it should be removed from mcast groups.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fm10000UpdateMcastHNIFloodingGroups(fm_int  sw,
                                              fm_int  port,
                                              fm_bool state)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_int     swToExecute;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, port=%d, state=%d\n",
                 sw,
                 port,
                 state);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->state != FM_SWITCH_STATE_UP)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

#if FM_SUPPORT_SWAG
    swToExecute = GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw);
#else
    swToExecute = sw;
#endif

    status = fmUpdateMcastHNIFloodingGroups(swToExecute,
                                            port,
                                            state);
    FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fm10000UpdateMcastHNIFloodingGroups */
