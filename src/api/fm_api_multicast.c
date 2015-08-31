/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_multicast.c
 * Creation Date:   October 8, 2007
 * Description:     Structures and functions for dealing with multicast groups.
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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
#define GET_MG_KEY(group, vlan)                                                 \
    ( ( ( (fm_uint64) group ) << 32 ) | ( (fm_uint64) vlan ) )

#define GET_LISTENER_KEY(port, vlan)                                            \
    ( ( ( (fm_uint64) port ) << 16 ) | ( (fm_uint64) vlan ) )

#define GET_VN_LISTENER_KEY(tunnelId, vni)                                      \
    ( ( ( (fm_uint64) tunnelId ) << 32 ) | ( (fm_uint64) vni ) )

#define GET_FLOW_LISTENER_KEY(tableIndex, flowId)                               \
    ( ( ( (fm_uint64) tableIndex ) << 32 ) | ( (fm_uint64) flowId ) )



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




/*****************************************************************************/
/** AllocateListener
 * \ingroup intMulticast
 *
 * \desc            Allocates a new fm_intMulticastListener structure and
 *                  populates the structure with basic information.
 *
 * \param[in]       group points to the multicast group structure.
 *
 * \param[in]       listener points to the listener description record.
 *
 * \param[out]      newListener points to caller-allocated storage into which
 *                  the listener pointer will be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if newListener is NULL.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
static fm_status AllocateListener(fm_intMulticastGroup *    group,
                                  fm_mcastGroupListener *   listener,
                                  fm_intMulticastListener **newListener)
{
    fm_int                   nbytes;
    fm_intMulticastListener *intListener;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "group=%p(%d), newListener=%p\n",
                 (void *) group,
                 group->handle,
                 (void *) newListener );
    FM_DBG_DUMP_LISTENER(listener);

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    if (newListener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    /* Allocate a new listener */
    nbytes = sizeof(fm_intMulticastListener);

    intListener = (fm_intMulticastListener *) fmAlloc(nbytes);

    if (intListener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_NO_MEM);
    }

    FM_CLEAR(*intListener);

    intListener->listener      = *listener;
    intListener->group         = group;

    if ( listener->listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN )
    {
        intListener->floodListener = 
                                  listener->info.portVlanListener.floodListener;
    }
    else
    {
        intListener->floodListener = FALSE;
    }

    FM_DLL_INIT_LIST(intListener, firstSubListener, lastSubListener);
    FM_DLL_INIT_NODE(intListener, nextSubListener, prevSubListener);

    *newListener = intListener;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end AllocateListener */




/*****************************************************************************/
/** AddSubListener
 * \ingroup intMulticast
 *
 * \desc            Adds a sub-listener to a multicast listener.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group structure.
 *
 * \param[in]       listener points to an ''fm_intMulticastListener'' structure
 *                  describing the primary listener (i.e., the LAG).
 *
 * \param[in]       port is the sub-listener port number, i.e., the LAG member.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddSubListener(fm_int                   sw,
                                fm_intMulticastGroup *   group,
                                fm_intMulticastListener *listener,
                                fm_int                   port)
{
    fm_switch *              switchPtr;
    fm_switchInfo            switchInfo;
    fm_status                err;
    fm_intMulticastListener *subListener;
    fm_mcastGroupListener    tempListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p (%d), listener = %p, port = %d\n",
                 sw,
                 (void *) group,
                 (group != NULL) ? group->handle : -1,
                 (void *) listener,
                 port);

    if (group == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    FM_DBG_DUMP_LISTENER(&listener->listener);

    if (listener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    err = fmGetSwitchInfo(sw, &switchInfo);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    FM_CLEAR(tempListener);

    tempListener = listener->listener;
    tempListener.info.portVlanListener.port = port;

    /* create a sub-listener record */
    err = AllocateListener(group, &tempListener, &subListener);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if ((switchInfo.switchFamily == FM_SWITCH_FAMILY_FM4000) ||
        (switchInfo.switchFamily == FM_SWITCH_FAMILY_REMOTE_FM4000))
    {
        subListener->listener.info.portVlanListener.remoteFlag = FALSE;
    }
    else
    {
        subListener->listener.info.portVlanListener.remoteFlag =
            listener->listener.info.portVlanListener.remoteFlag;
    }

    /* Add the sub-listener to the hardware */
    FM_API_CALL_FAMILY(err,
                       switchPtr->AddMulticastListener,
                       sw,
                       group,
                       subListener);

    if (err == FM_OK)
    {
        /* Add the sub-listener to the linked-list */
        fmAppendMcastSubListener(listener, subListener);
    }
    else
    {
        fmFree(subListener);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end AddSubListener */




/*****************************************************************************/
/** DeleteSubListener
 * \ingroup intMulticast
 *
 * \desc            Deletes a sub-listener from a multicast listener.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group structure.
 *
 * \param[in]       listener points to an ''fm_intMulticastListener'' structure
 *                  describing the primary listener (i.e., the LAG).
 *
 * \param[in]       port is the sub-listener port number, i.e., the LAG member.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteSubListener(fm_int                   sw,
                                   fm_intMulticastGroup *   group,
                                   fm_intMulticastListener *listener,
                                   fm_int                   port)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastListener *subListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p(%d), listener = %p, port = %d\n",
                 sw,
                 (void *) group,
                 (group != NULL) ? group->handle : -1,
                 (void *) listener,
                 port);

    err = FM_OK;

    if (group == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    FM_DBG_DUMP_LISTENER(&listener->listener);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Find the sub-listener record */
    subListener = listener->firstSubListener;

    while (subListener != NULL)
    {
        if (subListener->listener.listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN)
        {
            if (subListener->listener.info.portVlanListener.port == port)
            {
                break;
            }
        }

        subListener = subListener->nextSubListener;
    }

    if (subListener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    /* remove the sub-listener from the linked-list */
    fmRemoveMcastSubListener(listener, subListener);

    /* Remove the sub-listener from the hardware if the multicast
       group is activated */
    if (group->activated)
    {
        FM_API_CALL_FAMILY(err,
                           switchPtr->DeleteMulticastListener,
                           sw,
                           group,
                           subListener);
    }

    /* release the sub-listener memory */
    fmFree(subListener);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end DeleteSubListener */




/*****************************************************************************/
/** AddRemoteListener
 * \ingroup intMulticast
 *
 * \desc            Add a listener for a remote port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       intListener points to the listener.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddRemoteListener(fm_int                   sw,
                                   fm_intMulticastGroup *   group,
                                   fm_intMulticastListener *intListener)
{
    fm_int                           internalPort;
    fm_intMulticastInternalListener *internalPortListener;
    fm_multicastListener             tempListener;
    fm_status                        err;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d, group=%d, intListener = %p\n",
                 sw,
                 group->handle,
                 (void *) intListener);
    FM_DBG_DUMP_LISTENER(&intListener->listener);

    internalPort         = -1;
    internalPortListener = NULL;

    if (intListener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    /* Try to find an existing internal listener */
    err = fmGetInternalPortFromRemotePort(sw,
                                          intListener->listener.info.portVlanListener.port,
                                          &internalPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    internalPortListener = fmGetFirstMcastInternalListener(group);

    while (internalPortListener != NULL)
    {
        if (internalPortListener->port == internalPort)
        {
            break;
        }

        internalPortListener = fmGetNextMcastInternalListener(internalPortListener);
    }

    if (internalPortListener == NULL)
    {
        /* allocate a new internal listener */
        internalPortListener = fmAlloc( sizeof(fm_intMulticastInternalListener) );

        if (internalPortListener == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_NO_MEM);
        }

        FM_CLEAR(*internalPortListener);

        internalPortListener->port = internalPort;

        fmAppendMcastInternalListener(group, internalPortListener);
    }

    intListener->internal = internalPortListener;
    internalPortListener->listenerCount++;

    if (internalPortListener->listenerCount == 1)
    {
        FM_CLEAR(tempListener);

        tempListener.vlan = FM_INVALID_INTERFACE_VLAN;
        tempListener.port = internalPort;

        err = fmAddMcastGroupListener(sw, group->handle, &tempListener);

#if 0
        if (err != FM_OK)
        {
            fmRemoveMcastInternalListener(group, internalPortListener);
            fmFree(internalPortListener);
        }
#endif
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end AddRemoteListener */




/*****************************************************************************/
/** AddLagListener
 * \ingroup intMulticast
 *
 * \desc            Add a listener for a LAG port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       intListener points to the listener.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddLagListener(fm_int                   sw,
                                fm_intMulticastGroup *   group,
                                fm_intMulticastListener *intListener)
{
    fm_switch *           switchPtr;
    fm_int                lagPort;
    fm_int                lagIndex;
    fm_status             err;
    fm_mcastGroupListener tempListener;
    fm_int                portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int                numPorts;
    fm_int                i;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d, group=%d, intListener=%p\n",
                 sw,
                 group->handle,
                 (void *) intListener);
    FM_DBG_DUMP_LISTENER(&intListener->listener);

    if (intListener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = TAKE_LAG_LOCK(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmLogicalPortToLagIndex(sw,
                                  intListener->listener.info.portVlanListener.port,
                                  &lagIndex);
    if (err == FM_OK)
    {
        /* Get the list of ports member of this LAG */
        err = fmGetLAGMemberPorts(sw,
                                  lagIndex,
                                  &numPorts,
                                  portList,
                                  FM_MAX_NUM_LAG_MEMBERS,
                                  FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* For local port member, add port to listener directly.
           For remote port member, re-add the port member, it will be
           process as a remote port and a listener with the
           internal port will be used to reach this remote port. */
        for (i = 0 ; i < numPorts ; i++)
        {
            lagPort = portList[i];

            if (fmIsRemotePort(sw, lagPort))
            {
                tempListener = intListener->listener;
                tempListener.info.portVlanListener.port = lagPort;

                err = fmAddMcastGroupListenerV2(sw, group->handle, &tempListener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }
            else if (fmIsCardinalPort(sw, lagPort))
            {
                err = AddSubListener(sw, group, intListener, lagPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }
        }

    }   /* end if (err == FM_OK) */

ABORT:

    DROP_LAG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end AddLagListener */




/*****************************************************************************/
/** DeleteLagListener
 * \ingroup intMulticast
 *
 * \desc            Delete a listener for a LAG port.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       intListener points to the listener.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteLagListener(fm_int                   sw,
                                   fm_intMulticastGroup *   group,
                                   fm_intMulticastListener *intListener)
{
    fm_switch               *switchPtr;
    fm_int                   lagPort;
    fm_int                   lagIndex;
    fm_status                err;
    fm_mcastGroupListener    tempListener;
    fm_int                   portList[FM_MAX_NUM_LAG_MEMBERS];
    fm_int                   numPorts;
    fm_int                   i;
    fm_intMulticastListener *subListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d, group=%d, intListener=%p\n",
                 sw,
                 group->handle,
                 (void *) intListener);
    FM_DBG_DUMP_LISTENER(&intListener->listener);

    if (intListener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    err = TAKE_LAG_LOCK(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmLogicalPortToLagIndex(sw,
                                  intListener->listener.info.portVlanListener.port,
                                  &lagIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Get the list of ports member of this LAG */
    err = fmGetLAGMemberPorts(sw,
                              lagIndex,
                              &numPorts,
                              portList,
                              FM_MAX_NUM_LAG_MEMBERS,
                              FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    
    /* For local port member, the port will be deleted from listener directly.
     * For remote port member, re-delete the port member, it will be
     * processed as a remote port and a listener with the
     * internal port will be used to reach this remote port. */
    
    for (i = 0 ; i < numPorts ; i++)
    {
        lagPort = portList[i];

        if (fmIsRemotePort(sw, lagPort))
        {
            tempListener = intListener->listener;
            tempListener.info.portVlanListener.port = lagPort;

            err =
                fmDeleteMcastGroupListenerV2(sw, group->handle, &tempListener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        }
    }   /* end for (i = 0 ; i < numPorts ; i++) */
    
    /* For the remaining local port members, the ports will be deleted from
     * listener directly. */
    while (intListener->firstSubListener != NULL)
    {
        subListener = intListener->firstSubListener;
        lagPort = subListener->listener.info.portVlanListener.port;

        err = DeleteSubListener(sw, group, intListener, lagPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        
    }   /* end while (intListener->firstSubListener != NULL) */

ABORT:

    DROP_LAG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end DeleteLagListener */




/*****************************************************************************/
/** AddListenerToHardware
 * \ingroup intMulticast
 *
 * \desc            Add a listener to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       intListener points to the listener.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the port is invalid.
 *
 *****************************************************************************/
static fm_status AddListenerToHardware(fm_int                   sw,
                                       fm_intMulticastGroup *   group,
                                       fm_intMulticastListener *intListener)
{
    fm_switch *switchPtr;
    fm_port *  portPtr;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p(%d), intListener = %p\n",
                 sw,
                 (void *) group,
                 group->handle,
                 (void *) intListener);
    FM_DBG_DUMP_LISTENER(&intListener->listener);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = NULL;

    switch (intListener->listener.listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            portPtr = GET_PORT_PTR(sw, intListener->listener.info.portVlanListener.port);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            FM_API_CALL_FAMILY(err,
                               switchPtr->AddMulticastListener,
                               sw,
                               group,
                               intListener);
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    if (portPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_PORT);
    }

    switch (portPtr->portType)
    {
        case FM_PORT_TYPE_PHYSICAL:
        case FM_PORT_TYPE_CPU:
        case FM_PORT_TYPE_VIRTUAL:
            FM_API_CALL_FAMILY(err,
                               switchPtr->AddMulticastListener,
                               sw,
                               group,
                               intListener);
            break;

        case FM_PORT_TYPE_REMOTE:
            err = AddRemoteListener(sw, group, intListener);
            break;

        case FM_PORT_TYPE_LAG:
            err = AddLagListener(sw, group, intListener);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end AddListenerToHardware */




/*****************************************************************************/
/** DeleteListenerFromHardware
 * \ingroup intMulticast
 *
 * \desc            Delete a listener from a multicast group (leave).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       intListener points to an ''fm_intMulticastListener''
 *                  structure describing the listener to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteListenerFromHardware(fm_int                   sw,
                                            fm_intMulticastGroup *   group,
                                            fm_intMulticastListener *intListener)
{
    fm_switch *                      switchPtr;
    fm_port *                        portPtr;
    fm_status                        err;
    fm_intMulticastInternalListener *internalPortListener;
    fm_mcastGroupListener            tempListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p, intListener = %p\n",
                 sw,
                 (void *) group,
                 (void *) intListener);
    FM_DBG_DUMP_LISTENER(&intListener->listener);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = NULL;

    switch (intListener->listener.listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            portPtr = GET_PORT_PTR(sw, intListener->listener.info.portVlanListener.port);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            FM_API_CALL_FAMILY(err,
                               switchPtr->DeleteMulticastListener,
                               sw,
                               group,
                               intListener);
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    if (portPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_PORT);
    }

    /* remove the listener from the hardware */
    switch (portPtr->portType)
    {
        case FM_PORT_TYPE_PHYSICAL:
        case FM_PORT_TYPE_CPU:
        case FM_PORT_TYPE_VIRTUAL:
            FM_API_CALL_FAMILY(err,
                               switchPtr->DeleteMulticastListener,
                               sw,
                               group,
                               intListener);
            break;

        case FM_PORT_TYPE_REMOTE:
            internalPortListener = intListener->internal;

            if (internalPortListener == NULL)
            {
                err = FM_ERR_INVALID_PORT;
                break;
            }

            internalPortListener->listenerCount--;
            if (internalPortListener->listenerCount <= 0)
            {
                tempListener = intListener->listener;
                tempListener.info.portVlanListener.vlan = FM_INVALID_INTERFACE_VLAN;
                tempListener.info.portVlanListener.port = internalPortListener->port;

                err = fmDeleteMcastGroupListenerV2(sw,
                                                   group->handle,
                                                   &tempListener);

                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
                }

                fmRemoveMcastInternalListener(group, internalPortListener);
                fmFree(internalPortListener);
            }
            else
            {
                err = FM_OK;
            }
            break;

        case FM_PORT_TYPE_LAG:
            err = DeleteLagListener(sw, group, intListener);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            break;

        default:
            err = FM_OK;
            break;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end DeleteListenerFromHardware */




/*****************************************************************************/
/** DeleteMulticastListener
 * \ingroup intMulticast
 *
 * \desc            Delete a listener from a multicast group (leave).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       intListener points to an ''fm_intMulticastListener''
 *                  structure describing the listener to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteMulticastListener(fm_int                   sw,
                                         fm_intMulticastGroup *   group,
                                         fm_intMulticastListener *intListener)
{
    fm_port * portPtr;
    fm_status err;
    fm_uint64 listenerKey;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p, intListener = %p\n",
                 sw,
                 (void *) group,
                 (void *) intListener);
    FM_DBG_DUMP_LISTENER(&intListener->listener);

    portPtr = NULL;

    switch (intListener->listener.listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            portPtr     = GET_PORT_PTR(sw, intListener->listener.info.portVlanListener.port);
            listenerKey = GET_LISTENER_KEY(intListener->listener.info.portVlanListener.port,
                                           intListener->listener.info.portVlanListener.vlan);

            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            portPtr     = NULL;
            listenerKey = GET_VN_LISTENER_KEY(intListener->listener.info.vnListener.tunnelId,
                                              intListener->listener.info.vnListener.vni);
            break;

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            portPtr     = NULL;
            listenerKey = GET_FLOW_LISTENER_KEY(intListener->listener.info.flowListener.tableIndex,
                                                intListener->listener.info.flowListener.flowId);
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_UNSUPPORTED);
    }

    if (portPtr != NULL)
    {
        err = fmTreeRemove(&portPtr->mcastGroupList,
                           GET_MG_KEY(group->handle, intListener->listener.info.portVlanListener.vlan),
                           NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    if (group->activated)
    {
        /* remove the listener from the hardware */
        err = DeleteListenerFromHardware(sw, group, intListener);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    /* remove the listener from the listener tree and free the memory */
    err = fmTreeRemove(&group->listenerTree, listenerKey, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    fmFree(intListener);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end DeleteMulticastListener */




/*****************************************************************************/
/** AddAddressToHardware
 * \ingroup intMulticast
 *
 * \desc            Adds a multicast address to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       addrKey points to the multicast address key record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddAddressToHardware(fm_int                sw,
                                      fm_intMulticastGroup *group,
                                      fm_mcastAddrKey *     addrKey)
{
    fm_status          err;
    fm_macAddressEntry macEntry;
    fm_int             trigger;
    fm_routeEntry      routeEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d), addrKey = %p "
                  "(vlMode=%d, addrType=%d)\n",
                  sw,
                  (void *) group,
                  group->handle,
                  (void *) addrKey,
                  addrKey->vlMode,
                  addrKey->addr.addressType );

    if (addrKey->addr.addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
    {
        /* add mac address to MAC table */
        err = fmMcastBuildMacEntry(sw,
                                   group,
                                   &addrKey->addr.info.mac,
                                   &macEntry,
                                   &trigger);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmAddAddressToTableInternal(sw,
                                          &macEntry,
                                          trigger,
                                          TRUE,
                                          -1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }
    else
    {
        /* Add Multicast Route to Hardware */
        FM_CLEAR( routeEntry );
        routeEntry.routeType = FM_ROUTE_TYPE_MULTICAST;
        FM_MEMCPY_S( &routeEntry.data.multicast,
                     sizeof(routeEntry.data.multicast),
                     &addrKey->addr,
                     sizeof(addrKey->addr) );

        group->routePtrPtr = &addrKey->routePtr;

        err = fmAddRouteInternal(sw,
                                 &routeEntry,
                                 group->groupState,
                                 &group->groupAction);

        group->routePtrPtr = NULL;

        if (err == FM_ERR_ALREADY_EXISTS)
        {
            err = FM_OK;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end AddAddressToHardware */




/*****************************************************************************/
/** DeleteAddressFromHardware
 * \ingroup intMulticast
 *
 * \desc            Deletes a multicast group address from the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \param[in]       addrKey points to the address to be deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteAddressFromHardware(fm_int sw,
                                           fm_intMulticastGroup *group,
                                           fm_mcastAddrKey *     addrKey)
{
    fm_status                err;
    fm_macAddressEntry       macEntry;
    fm_routeEntry            routeEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d), addrKey = %p\n",
                  sw,
                  (void *) group,
                  group->handle,
                  (void *) addrKey );

    if (addrKey->addr.addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
    {
        /* remove mac address from MAC table */
        err = fmMcastBuildMacEntry(sw,
                                   group,
                                   &addrKey->addr.info.mac,
                                   &macEntry,
                                   NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmDeleteAddressFromTable(sw, &macEntry, FALSE, TRUE, -1);

        if (err == FM_ERR_ADDR_NOT_FOUND)
        {
            err = FM_OK;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }
    else
    {
        /* Remove Multicast Route from Hardware */
        FM_CLEAR( routeEntry );
        routeEntry.routeType = FM_ROUTE_TYPE_MULTICAST;
        FM_MEMCPY_S( &routeEntry.data.multicast,
                     sizeof(routeEntry.data.multicast),
                     &addrKey->addr,
                     sizeof(addrKey->addr) );

        group->routePtrPtr = &addrKey->routePtr;

        err = fmDeleteRouteInternal(sw, &routeEntry);

        group->routePtrPtr = NULL;

        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_OK;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end DeleteAddressFromHardware */




/*****************************************************************************/
/** ValidateListenerPort
 * \ingroup intMulticast
 *
 * \desc            Determines whether a multicast listener port is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port to be validated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if the listener port is not a valid
 *                  logical port.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
static fm_status ValidateListenerPort(fm_int sw, fm_int port)
{
    fm_switch * switchPtr;
    fm_port *   portPtr;

    if ( !fmIsValidPort(sw, port, ALLOW_CPU | ALLOW_LAG | ALLOW_REMOTE) )
    {
        return FM_ERR_INVALID_PORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL) &&
         (portPtr->lagIndex >= 0) && (switchPtr->swag < 0) )
    {
        /* Port is a LAG member, and cannot be directly addressed. */
        return FM_ERR_PORT_IS_IN_LAG;
    }

    return FM_OK;

}   /* end ValidateListenerPort */




/*****************************************************************************/
/** ValidateVNTunnelListener
 * \ingroup intMulticast
 *
 * \desc            Determines whether a VN Tunnel multicast listener.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       listener points to the listener descriptor record.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the listener is not valid.
 *
 *****************************************************************************/
static fm_status ValidateVNTunnelListener(fm_int                 sw,
                                          fm_mcastGroupListener *listener)
{
    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, listener = %p (tunnel=%d, vni=%u)\n",
                 sw,
                 (void *) listener,
                 listener->info.vnListener.tunnelId,
                 listener->info.vnListener.vni);

    if ( fmGetVNTunnel(sw, listener->info.vnListener.tunnelId) == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    if ( fmGetVN(sw, listener->info.vnListener.vni) == NULL )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end ValidateVNTunnelListener */




/*****************************************************************************/
/** ValidateFlowTunnelListener
 * \ingroup intMulticast
 *
 * \desc            Determines whether a Flow Tunnel multicast listener can be
 *                  added.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       listener points to the listener descriptor record.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the listener is not valid.
 *
 *****************************************************************************/
static fm_status ValidateFlowTunnelListener(fm_int                 sw,
                                            fm_mcastGroupListener *listener)
{
    fm_flowTableType flowTableType;
    fm_status err;
    fm_flowCondition flowCond;
    fm_flowValue flowValue;
    fm_flowAction flowAction;
    fm_flowParam flowParam;
    fm_int priority;
    fm_int precedence;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, listener = %p (tableIndex=%d, flowId=%d)\n",
                 sw,
                 (void *) listener,
                 listener->info.flowListener.tableIndex,
                 listener->info.flowListener.flowId);

    err = fmGetFlowTableType(sw,
                             listener->info.flowListener.tableIndex,
                             &flowTableType);
    if ( (err != FM_OK) || (flowTableType != FM_FLOW_TE_TABLE) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmGetFlow(sw, 
                    listener->info.flowListener.tableIndex,
                    listener->info.flowListener.flowId,
                    &flowCond,
                    &flowValue,
                    &flowAction,
                    &flowParam,
                    &priority,
                    &precedence);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end ValidateFlowTunnelListener */




/*****************************************************************************/
/** CreateListener
 * \ingroup intMulticast
 *
 * \desc            Allocates a listener object and adds it to the data
 *                  structures for a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupPtr points to the multicast group.
 *
 * \param[in]       listener points to a structure specifying the listener
 *                  to create.
 *
 * \param[out]      newListener points to caller-supplied storage to receive
 *                  a pointer to the newly created listener object.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateListener(fm_int                    sw,
                                fm_intMulticastGroup *    groupPtr,
                                fm_mcastGroupListener *   listener,
                                fm_intMulticastListener **newListener)
{
    fm_switchInfo            switchInfo;
    fm_port *                portPtr;
    fm_intMulticastListener *intListener;
    fm_uint64                listenerKey;
    fm_status                err;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %d\n",
                 sw,
                 groupPtr->handle);
    FM_DBG_DUMP_LISTENER(listener);

    if ( (err = fmGetSwitchInfo(sw, &switchInfo) ) != FM_OK )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }

    switch (listener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            portPtr     = GET_PORT_PTR(sw, listener->info.portVlanListener.port);
            listenerKey = GET_LISTENER_KEY(listener->info.portVlanListener.port,
                                           listener->info.portVlanListener.vlan);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            portPtr     = NULL;
            listenerKey = GET_VN_LISTENER_KEY(listener->info.vnListener.tunnelId,
                                              listener->info.vnListener.vni);
            break;

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            portPtr     = NULL;
            listenerKey = GET_FLOW_LISTENER_KEY(listener->info.flowListener.tableIndex,
                                                listener->info.flowListener.flowId);
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    /* Search the tree of listeners for a matching entry. */
    err = fmTreeFind(&groupPtr->listenerTree,
                     listenerKey,
                     (void **) &intListener);

    if (err == FM_OK)
    {
        /* Error if already in group. [Bugzilla 11355] */
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Search the tree of pre-lag listeners for a matching entry. */
    err = fmTreeFind(&groupPtr->preLagListenerTree,
                     listenerKey,
                     (void **) &intListener);

    if (err == FM_OK)
    {
        /* Error if already in group. */
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Allocate a new listener object. */
    err = AllocateListener(groupPtr, listener, &intListener);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if ( ( (switchInfo.switchFamily == FM_SWITCH_FAMILY_FM4000)
           || (switchInfo.switchFamily == FM_SWITCH_FAMILY_REMOTE_FM4000) )
         && (listener->listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN) )
    {
        intListener->listener.info.portVlanListener.remoteFlag = FALSE;
    }

    /* Add listener to multicast group. */
    err = fmTreeInsert(&groupPtr->listenerTree,
                       listenerKey,
                       (void *) intListener);

    if (err != FM_OK)
    {
        fmFree(intListener);
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    if (portPtr != NULL)
    {
        /* Add (groupId, vlanId) to logical port. */
        err = fmTreeInsert(&portPtr->mcastGroupList,
                           GET_MG_KEY(groupPtr->handle, listener->info.portVlanListener.vlan),
                           (void *) intListener);

        if (err != FM_OK)
        {
            (void)fmTreeRemoveCertain(&groupPtr->listenerTree, listenerKey, NULL);
            fmFree(intListener);
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }
    }

    *newListener = intListener;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end CreateListener */




/*****************************************************************************/
/** DeleteListener
 * \ingroup intMulticast
 *
 * \desc            Removes a listener object from the data structures
 *                  for a multicast group and deletes it. Used to clean up
 *                  after a failed call to CreateListener.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupPtr points to the multicast group.
 *
 * \param[in]       listener points to a structure specifying the listener
 *                  to delete.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteListener(fm_int                 sw,
                                fm_intMulticastGroup * groupPtr,
                                fm_mcastGroupListener *listener)
{
    fm_port *                portPtr;
    fm_intMulticastListener *intListener;
    fm_intMulticastListener *subListener;
    fm_uint64                listenerKey;
    fm_status                err;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %d\n",
                 sw,
                 groupPtr->handle);
    FM_DBG_DUMP_LISTENER(listener);

    switch (listener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            portPtr     = GET_PORT_PTR(sw, listener->info.portVlanListener.port);
            listenerKey = GET_LISTENER_KEY(listener->info.portVlanListener.port,
                                           listener->info.portVlanListener.vlan);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            portPtr     = NULL;
            listenerKey = GET_VN_LISTENER_KEY(listener->info.vnListener.tunnelId,
                                              listener->info.vnListener.vni);
            break;

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            portPtr     = NULL;
            listenerKey = GET_FLOW_LISTENER_KEY(listener->info.flowListener.tableIndex,
                                                listener->info.flowListener.flowId);
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    if (portPtr != NULL)
    {
        /* Remove (groupId, vlanId) from logical port. */
        (void)fmTreeRemove(&portPtr->mcastGroupList,
                           GET_MG_KEY(groupPtr->handle, listener->info.portVlanListener.vlan),
                           NULL);
    }

    /* Retrieve listener object. */
    err = fmTreeFind(&groupPtr->listenerTree,
                     listenerKey,
                     (void **) &intListener);

    if (err != FM_OK)
    {
        /* It's okay if we don't find the object. */
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    if (intListener->addedToChip)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST, "Deleting active listener!\n");
    }

    /* Remove listener from group. */
    (void)fmTreeRemoveCertain(&groupPtr->listenerTree,
                              listenerKey,
                              NULL);

    /* Delete any sub-listeners. */
    while (intListener->firstSubListener != NULL)
    {
        subListener = intListener->firstSubListener;

        DeleteSubListener(sw,
                          groupPtr,
                          intListener,
                          subListener->listener.info.portVlanListener.port);
    }

    /* Free listener object. */
    fmFree(intListener);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end DeleteListener */




/*****************************************************************************/
/** fmAllocateMcastGroupsInt
 * \ingroup intMulticast
 *
 * \desc            Allocate multicast groups given a glort range. The function
 *                  returns the base multicast handle and the number of handles
 *                  created. The caller can then emumerate these handles up to
 *                  the number of handles allocated. These handles will have
 *                  the same CAM resources across multiple switches, given the
 *                  input glort information is the same.
 *
 * \note            The return base handle might not be the same on different
 *                  switches. However the cam resources for
 *                  (baseMcastGroup + n*step) will be consistent on
 *                  different switches when using ''fmCreateStackMcastGroup''..
 *                  In addition, this API can also be used to allocate additional
 *                  multicast resources on standalone switches. In this case,
 *                  calling ''fmCreateMcastGroup'' will just provide a free entry
 *                  in this reserved pool.
 *
 * \param[in]       sw is the switch on which to operate. This value must be a
 *                  power of two.
 *
 * \param[in]       startGlort is the starting glort to use to reserve for
 *                  multicast groups.
 *
 * \param[in]       glortSize is the glort size to use.
 *
 * \param[out]      baseMcastGroup points to caller-allocated storage
 *                  where this function should place the base multicast group
 *                  handle of the newly allocated stacking multicast groups.
 *
 * \param[out]      numMcastGroups points to caller-allocated storage
 *                  where this function should place the number of multicast
 *                  groups allocated given the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage
 *                  where this function should place the step value, where
 *                  the caller can increment from base by to get
 *                  subsequent multicast groups
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort in the given
 *                  glort range is being used.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more resoures are available
 *
 *****************************************************************************/
fm_status fmAllocateMcastGroupsInt(fm_int    sw,
                                   fm_uint    startGlort,
                                   fm_uint    glortSize,
                                   fm_int    *baseMcastGroup,
                                   fm_int    *numMcastGroups,
                                   fm_int    *step)
{
    fm_status     err;
    fm_switch *   switchPtr;
    fm_int        baseMcastPort;
    fm_int        numMcastPorts;
    fm_int        off;
    fm_int        baseMcastHandle;
    fm_uintptr   curPort;
    fm_int        curHandle;
    fm_int        i;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, startGlort = %u, glortSize = %u, "
                 "baseMcastGroup = %p, numMcastGroups = %p, step = %p\n",
                 sw,
                 startGlort,
                 glortSize,
                 (void *) baseMcastGroup,
                 (void *) numMcastGroups,
                 (void *) step);

    *numMcastGroups = 0;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->AllocateMcastGroups,
                       sw,
                       startGlort,
                       glortSize,
                       &baseMcastPort,
                       &numMcastPorts,
                       &off);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Get a block of group handles for use with the logical ports that
     * were just allocated. */
    err = fmFindBitBlockInBitArray(&switchPtr->mcastHandles,
                                   0,
                                   numMcastPorts,
                                   FALSE,
                                   &baseMcastHandle);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (baseMcastHandle < 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_NO_MCAST_RESOURCES);
    }

    /* Reserve the block of handles */
    err = fmSetBitArrayBlock(&switchPtr->mcastHandles,
                             baseMcastHandle,
                             numMcastPorts,
                             TRUE);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Store the logical port for each handle. */
    for (i = 0, curPort = baseMcastPort, curHandle = baseMcastHandle ;
         i < numMcastPorts ;
         i++, curPort += off, curHandle++)
    {
        err = fmTreeInsert(&switchPtr->mcastHandlePortTree,
                           (fm_uint64) curHandle,
                           (void *) curPort);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    /* return handles to caller */
    *baseMcastGroup = baseMcastHandle;
    *numMcastGroups = numMcastPorts;
    *step           = 1;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAllocateMcastGroupsInt */




/*****************************************************************************/
/** fmFreeMcastGroupsInt
 * \ingroup intMulticast
 *
 * \desc            Free multicast groups previously created with
 *                  ''fmAllocateMcastGroupsInt''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseMcastGroup is the handle previously created with
 *                  ''fmAllocateMcastGroupsInt''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if handle is not found
 *                  in allocated entries.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fmFreeMcastGroupsInt(fm_int    sw,
                               fm_int    baseMcastGroup)
{
    fm_status         err;
    fm_switch *       switchPtr;
    fm_uintptr        basePort;
    fm_mcgAllocEntry *allocEntry;
    fm_int            numHandles;
    fm_int            curHandle;
    fm_int            i;


    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                     "sw = %d, baseMcastGroup = %d\n",
                     sw,
                     baseMcastGroup);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmTreeFind( &switchPtr->mcastHandlePortTree,
                      (fm_uint64) baseMcastGroup,
                      (void **) &basePort );

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Find the number of handles related to this stacking multicast groups */
    allocEntry = fmFindMcgEntryByHandle(sw, (fm_int) basePort);
    if ( !allocEntry || ((fm_int) basePort != allocEntry->baseHandle) )
    {
        /* Incorrect handle passed in */
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_MULTICAST_GROUP);
    }

    numHandles = allocEntry->numHandles;

    FM_API_CALL_FAMILY(err,
                       switchPtr->FreeMcastGroups,
                       sw,
                       (fm_int) basePort);

    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Remove the logical port for each handle from the tree */
    for (i = 0, curHandle = baseMcastGroup; i < numHandles; i++, curHandle++)
    {
        err = fmTreeRemove(&switchPtr->mcastHandlePortTree,
                           (fm_uint64) curHandle,
                           NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    /* Free the block of handles */
    err = fmSetBitArrayBlock(&switchPtr->mcastHandles,
                             baseMcastGroup,
                             numHandles,
                             FALSE);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmFreeMcastGroupsInt */




/*****************************************************************************/
/** fmCreateMcastGroupInt
 * \ingroup intMulticast
 *
 * \desc            Create a multicast group given a logical port
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   mcastGroup points to caller-allocated storage where this
 *                  function should place the multicast group number (handle)
 *                  of the newly created multicast group.  When the stacking
 *                  argument is TRUE, the memory pointed to by mcastGroup
 *                  must have been pre-initialized with the handle previously
 *                  created by fmAllocateStackMcastGroup.
 *
 * \param[in]       stacking is a flag to indicate if the function is
 *                  called by stacking API.
 *
 * \param[in]       internal refer to the multicast internal flag. Internal is a
 *                  flag defining if the multicast group is managed by one of the
 *                  API subsystem (mailbox, VN,...) or if this group is created
 *                  by the application.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  multicast group structure.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more multicast group
 *                  resoures are available.
 * \return          FM_ERR_LPORT_DESTS_UNAVAILABLE if a block of logical port
 *                  destination table entries cannot be allocated.
 *
 *****************************************************************************/
fm_status fmCreateMcastGroupInt(fm_int  sw,
                                fm_int *mcastGroup,
                                fm_bool stacking,
                                fm_bool internal)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_bool               lockTaken            = FALSE;
    fm_bool               handleAssigned       = FALSE;
    fm_bool               handleReserved       = FALSE;
    fm_bool               groupAllocated       = FALSE;
    fm_bool               groupInserted        = FALSE;
    fm_bool               portGroupInserted    = FALSE;
    fm_bool               logicalPortAllocated = FALSE;
    fm_bool               forceRPFAction       = FALSE;
    fm_intMulticastGroup *group                = NULL;
    fm_int                port;
    fm_int                handle;
    fm_int                useHandle;
    fm_uintptr            tmpUintPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %p, stacking = %d\n",
                 sw,
                 (void *) mcastGroup,
                 stacking);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->McastGroupInit == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        goto ABORT;
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    /* For stacking the group is passed in */
    if (stacking)
    {
        useHandle = *mcastGroup;

        if (useHandle == FM_HANDLE_NONE)
        {
            forceRPFAction = TRUE;
        }
    }
    else
    {
        /* Special case to allow a switch to specify the handle to be
         * used.  Initially used to allow SWAG switches to specify a
         * handle that will be usable across all switches in the SWAG. */
        if (switchPtr->AssignMcastGroup != NULL)
        {
            err = switchPtr->AssignMcastGroup(sw, &useHandle);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            handleAssigned = TRUE;
        }
        else
        {
            useHandle = FM_HANDLE_NONE;
        }
    }

    if (useHandle == FM_HANDLE_NONE)
    {
        err = fmFindBitInBitArray(&switchPtr->mcastHandles,
                                  0,
                                  FALSE,
                                  &handle);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        if (handle < 0)
        {
            err = FM_ERR_NO_MCAST_RESOURCES;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        err = fmSetBitArrayBit(&switchPtr->mcastHandles, handle, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        handleReserved = TRUE;

        port = FM_LOGICAL_PORT_NONE;
    }
    else
    {
        err = fmTreeFind( &switchPtr->mcastHandlePortTree,
                          (fm_uint64) useHandle,
                          (void **) &tmpUintPtr);

        if (err == FM_ERR_NOT_FOUND)
        {
            err = FM_ERR_INVALID_MULTICAST_GROUP;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        port   = (fm_int) tmpUintPtr;
        handle = useHandle;

        FM_API_CALL_FAMILY(err,
                           switchPtr->AllocLogicalPort,
                           sw,
                           FM_PORT_TYPE_MULTICAST,
                           1,
                           &port,
                           port);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        logicalPortAllocated    = TRUE;
    }

    group = (fm_intMulticastGroup *) fmAlloc( sizeof(fm_intMulticastGroup) );

    if (group == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    groupAllocated = TRUE;

    memset( group, 0, sizeof(fm_intMulticastGroup) );

    group->handle             = handle;
    group->localHandle        = (handleReserved) ? TRUE : FALSE;
    group->logicalPort        = port;
    group->localLogicalPort   = FALSE;
    group->ecmpGroup          = -1;
    group->fwdToCpu           = FALSE;
    group->stackGroup         = stacking;
    group->groupAction.action = FM_ROUTE_ACTION_ROUTE;
    group->groupState         = FM_ROUTE_STATE_UP;
    group->privateGroup       = TRUE;
    group->repliGroup         = -1;
    group->l2SwitchingOnly    = FALSE;
    group->l3SwitchingOnly    = FALSE;
    group->l3FloodSet         = FALSE;
    group->l2FloodSet         = FALSE;
    group->l2VxLanDecap       = FALSE;
    group->ipTunneling        = FALSE;
    group->updateHardware     = TRUE;
    group->attrLogicalPort    = FM_LOGICAL_PORT_NONE;
    group->internal           = internal;

    group->singleAddressMode  =
        fmGetBoolApiProperty(FM_AAK_API_1_ADDR_PER_MCAST_GROUP,
                             FM_AAD_API_1_ADDR_PER_MCAST_GROUP);

    if (forceRPFAction)
    {
        group->groupAction.action = FM_ROUTE_ACTION_RPF_FAILURE;
    }

    fmTreeInit(&group->listenerTree);
    fmTreeInit(&group->preLagListenerTree);

    if (switchPtr->CompareMulticastAddresses != NULL)
    {
        fmCustomTreeInit(&group->addressTree, switchPtr->CompareMulticastAddresses);
    }
    else
    {
        fmCustomTreeInit(&group->addressTree, fmCompareMulticastAddresses);
    }

    fmTreeInit(&group->ecmpTree);

    err = fmTreeInsert( &switchPtr->mcastTree,
                       (fm_uint64) group->handle,
                       (void *) group );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    groupInserted = TRUE;

    if (group->logicalPort != FM_LOGICAL_PORT_NONE)
    {
        err = fmTreeInsert( &switchPtr->mcastPortTree,
                           (fm_uint64) group->logicalPort,
                           (void *) group );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        portGroupInserted = TRUE;
    }

    /* Now add the multicast group into the hardware */
    FM_API_CALL_FAMILY(err, switchPtr->CreateMcastGroup, sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    *mcastGroup = group->handle;

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "multicast group created, handle %d\n",
                 group->handle);

ABORT:

    if (err != FM_OK)
    {
        if (portGroupInserted)
        {
            fmTreeRemoveCertain( &switchPtr->mcastPortTree,
                                (fm_uint64) group->logicalPort,
                                NULL );
        }

        if (groupInserted)
        {
            fmTreeRemoveCertain( &switchPtr->mcastTree,
                                 (fm_uint64) group->handle,
                                 NULL );
        }

        if (groupAllocated)
        {
            fmFree(group);
        }

        if (logicalPortAllocated)
        {
             fmFreeLogicalPort(sw, port);
        }

        if (handleReserved)
        {
            fmSetBitArrayBit(&switchPtr->mcastHandles,
                             handle,
                             FALSE);
        }

        if (handleAssigned)
        {
            if (switchPtr->UnassignMcastGroup != NULL)
            {
                switchPtr->UnassignMcastGroup(sw, useHandle);
            }
        }
    }

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmCreateMcastGroupInt */




/*****************************************************************************/
/** VerifyListener
 * \ingroup intMulticast
 *
 * \desc            Verify if listener is orthogonal to other multicast groups
 *                  sharing the same replication group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to internal multicast group structure
 *
 * \param[in]       listener points to internal multicast listeners structure
 *
 * \return          TRUE if listeners can be added.
 * \return          FALSE if replication groups is shared and listener is not
 *                  orthogonal to other multicast groups sharing the same
 *                  replication group.
 *
 *****************************************************************************/
static fm_bool VerifyListener(fm_int                   sw,
                              fm_intMulticastGroup *   group,
                              fm_intMulticastListener *listener)
{
    fm_switch               *switchPtr;
    fm_intReplicationGroup  *repliGroup;
    fm_status                status;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm_intMulticastGroup    *mcastGroupTmp;
    fm_treeIterator          iterListener;
    fm_intMulticastListener *intListener;

    switchPtr = GET_SWITCH_PTR(sw);

    if (group->privateGroup == TRUE)
    {
        return TRUE;

    }

    status = fmTreeFind( &switchPtr->replicationTree,
                         (fm_uint64) group->repliGroup,
                         (void **) &repliGroup );

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "No Replication group: %s\n",
                     fmErrorMsg(status));
        return FALSE;

    }

    fmTreeIterInit(&iter, &repliGroup->mcastGroupList);

    while (1)
    {
        status = fmTreeIterNext(&iter, &key, (void **) &mcastGroupTmp);

        if (status != FM_OK)
        {
            if (status == FM_ERR_NO_MORE)
            {
                return TRUE;
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                             "Multicast group retrieval: %s\n",
                             fmErrorMsg(status));
                return FALSE;

            }
            break;
        }

        if (mcastGroupTmp == NULL)
        {
            continue;
        }

        if (key == (fm_uint64) group->handle)
        {
            continue;
        }

        if (mcastGroupTmp->activated == FALSE)
        {
            continue;
        }

        if ( (mcastGroupTmp->privateGroup == FALSE) &&
             (mcastGroupTmp->readOnlyRepliGroup == TRUE) )
        {
            continue;
        }

        fmTreeIterInit(&iterListener, &mcastGroupTmp->listenerTree);
        while(1)
        {
            status = fmTreeIterNext( &iterListener,
                                  &key,
                                  (void **) &intListener );

            if (status == FM_ERR_NO_MORE)
            {
                break;
            }
            else if (status != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                             "Listener retrieval: %s\n",
                             fmErrorMsg(status));
                return FALSE;
            }

            if (intListener == NULL)
            {
                continue;
            }

            if (intListener->listener.listenerType != listener->listener.listenerType)
            {
                continue;
            }

            switch (intListener->listener.listenerType)
            {
                case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
                    if (intListener->listener.info.portVlanListener.port
                        == listener->listener.info.portVlanListener.port)
                    {
                        return FALSE;
                    }
                    break;

                default:
                    break;
            }
        } /* end while (1) */
    } /* end while (1) */

    return TRUE;

} /* end VerifyListener */




/*****************************************************************************/
/** UpdateMcastHNIFloodingGroups
 * \ingroup intMulticast
 *
 * \desc            Updates mcast group configured as HNI flooding with given
 *                  listener.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       listener points to an ''fm_mcastGroupListener'' structure
 *                  describing the listener to add/remove.
 *
 * \param[in]       state is TRUE if the listener should be added to mcast 
 *                  group, or FALSE if it should be removed from mcast group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
static fm_status UpdateMcastHNIFloodingGroups(fm_int                 sw,
                                              fm_int                 mcastGroup,
                                              fm_mcastGroupListener *listener,
                                              fm_bool                state)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, listener = %p, state = %d\n",
                 sw,
                 mcastGroup,
                 (void *) listener,
                 state);

    if (state)
    {
        status = fmAddMcastGroupListenerInternalForFlood(sw,
                                                         mcastGroup,
                                                         listener);

        if (status == FM_ERR_ALREADY_EXISTS)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Listener (port, vlan) = (%d, %d)"
                         " already added to mcast group %d\n",
                         listener->info.portVlanListener.port,
                         FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS,
                         mcastGroup);
        }
        else if (status == FM_ERR_PORT_IS_IN_LAG)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Port %d is a member of a Link Aggregation Group",
                         listener->info.portVlanListener.port);
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }
    else
    {
        status = fmDeleteMcastGroupListenerInternalForFlood(sw,
                                                            mcastGroup,
                                                            listener);

        if (status == FM_ERR_NOT_FOUND)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Listener (port, vlan) = (%d, %d)"
                         " already deleted from mcast group %d\n",
                         listener->info.portVlanListener.port,
                         FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS,
                         mcastGroup);
        }
        else if (status == FM_ERR_PORT_IS_IN_LAG)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Port %d is a member of a Link Aggregation Group",
                         listener->info.portVlanListener.port);
        }
        else
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    status = FM_OK;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end UpdateMcastHNIFloodingGroups */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCompareMulticastAddresses
 * \ingroup intMulticast
 *
 * \desc            Compares two fm_mcastAddrKey structures.
 *
 * \param[in]       key1 points to one of the keys.
 *
 * \param[in]       key2 points to the other key.
 *
 * \return          -1 if key1 comes before key2.
 * \return          0  if key1 is equal to key2.
 * \return          1  if key1 comes after key2.
 *
 *****************************************************************************/
fm_int fmCompareMulticastAddresses(const void *key1, const void *key2)
{
    const fm_mcastAddrKey *key1Ptr;
    const fm_mcastAddrKey *key2Ptr;
    fm_vlanLearningMode    vlMode;
    fm_routeEntry          route1;
    fm_routeEntry          route2;
    fm_int                 i = 0;

    key1Ptr = (fm_mcastAddrKey *) key1;
    key2Ptr = (fm_mcastAddrKey *) key2;

    if (key1Ptr->addr.addressType != key2Ptr->addr.addressType)
    {
        if (key1Ptr->addr.addressType < key2Ptr->addr.addressType)
        {
            return -1;
        }
        return 1;
    }

    if (key1Ptr->vlMode == FM_VLAN_LEARNING_MODE_INDEPENDENT)
    {
        vlMode = FM_VLAN_LEARNING_MODE_INDEPENDENT;
    }
    else if (key2Ptr->vlMode == FM_VLAN_LEARNING_MODE_INDEPENDENT)
    {
        vlMode = FM_VLAN_LEARNING_MODE_INDEPENDENT;
    }
    else
    {
        vlMode = FM_VLAN_LEARNING_MODE_SHARED;
    }

    if (key1Ptr->addr.addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
    {
        if (key1Ptr->addr.info.mac.destMacAddress
            < key2Ptr->addr.info.mac.destMacAddress)
        {
            i = -1;
        }
        else if (key1Ptr->addr.info.mac.destMacAddress
            > key2Ptr->addr.info.mac.destMacAddress)
        {
            i = 1;
        }

        else if (vlMode == FM_VLAN_LEARNING_MODE_INDEPENDENT)
        {
            if (key1Ptr->addr.info.mac.vlan < key2Ptr->addr.info.mac.vlan)
            {
                i = -1;
            }
            else if (key1Ptr->addr.info.mac.vlan > key2Ptr->addr.info.mac.vlan)
            {
                i = 1;
            }

            /* vlan1 are equal, compare vlan2 */
            else if (key1Ptr->addr.info.mac.vlan2 < key2Ptr->addr.info.mac.vlan2)
            {
                i = -1;
            }
            else if (key1Ptr->addr.info.mac.vlan2 > key2Ptr->addr.info.mac.vlan2)
            {
                i = 1;
            }
        }
    }
    else
    {
        FM_CLEAR( route1 );
        FM_CLEAR( route2 );

        route1.routeType = FM_ROUTE_TYPE_MULTICAST;
        FM_MEMCPY_S( &route1.data.multicast,
                     sizeof(route1.data.multicast),
                     &key1Ptr->addr,
                     sizeof(key1Ptr->addr) );

        route2.routeType = FM_ROUTE_TYPE_MULTICAST;
        FM_MEMCPY_S( &route2.data.multicast,
                     sizeof(route2.data.multicast),
                     &key2Ptr->addr,
                     sizeof(key2Ptr->addr) );

        i = fmCompareRoutes(&route1, &route2);
    }

    return i;

}   /* end fmCompareMulticastAddresses */




/*****************************************************************************/
/** fmFindMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Find a multicast group, given its handle.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       handle is the multicast group's handle.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_intMulticastGroup *fmFindMcastGroup(fm_int sw, fm_int handle)
{
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;
    fm_status             status;

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmTreeFind( &switchPtr->mcastTree,
                        (fm_uint64) handle,
                        (void **) &group );

    if (status != FM_OK)
    {
        group = NULL;
    }

    return group;

}   /* end fmFindMcastGroup */




/*****************************************************************************/
/** fmFindMcastGroupByPort
 * \ingroup intMulticast
 *
 * \desc            Find a multicast group, given its logical port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the multicast group's logical port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_intMulticastGroup *fmFindMcastGroupByPort(fm_int sw, fm_int port)
{
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;
    fm_status             status;

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmTreeFind( &switchPtr->mcastPortTree,
                        (fm_uint64) port,
                        (void **) &group );

    if (status != FM_OK)
    {
        group = NULL;
    }

    return group;

}   /* end fmFindMcastGroupByPort */




/*****************************************************************************/
/** fmFreeMcastGroupDataStructures
 * \ingroup intMulticast
 *
 * \desc            Perform deallocation for multicast group subsystem,
 *                  called at switch removal time.
 *
 * \param[in]       switchPtr points to the switch state structure of the
 *                  switch being removed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmFreeMcastGroupDataStructures(fm_switch *switchPtr)
{
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *listener;
    fm_treeIterator          iter;
    fm_status                status;
    fm_uint64                key;
    fm_uint64                listenerKey;
    fm_uint64                ecmpKey;
    fm_treeIterator          listenerIter;
    fm_treeIterator          ecmpIter;
    fm_customTreeIterator    addrIter;
    fm_intReplicationGroup * repliGroup;
    fm_intMulticastEcmp *    ecmpGroup;
    fm_mcastAddrKey         *addrKey;
    fm_mcastAddrKey         *addrValue;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw = %d\n", switchPtr->switchNumber);

    if (!fmTreeIsInitialized(&switchPtr->mcastTree))
    {
        /* Initialization has not been called due to
         * switch bring up error, so when cleanup
         * just return here.
         */
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    /* Remove all multicast groups from the tree and delete them */
    while (1)
    {
        fmTreeIterInit(&iter, &switchPtr->mcastTree);

        status = fmTreeIterNext( &iter, &key, (void **) &group );

        if (status != FM_OK)
        {
            break;
        }

        while (1)
        {
            fmTreeIterInit(&listenerIter, &group->listenerTree);

            status = fmTreeIterNext( &listenerIter,
                                    &listenerKey,
                                    (void **) &listener );

            if (status != FM_OK)
            {
                break;
            }

            /* remove the listener from the listener tree */
            status = fmTreeRemoveCertain(&group->listenerTree,
                                         listenerKey,
                                         NULL);
            if (status != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_MULTICAST,
                              "fmTreeRemoveCertain returned error %d: %s\n",
                              status,
                              fmErrorMsg(status) );
            }

            /* free the memory */
            fmFree(listener);
        }

        while (1)
        {
            fmTreeIterInit(&listenerIter, &group->preLagListenerTree);

            status = fmTreeIterNext(&listenerIter,
                                    &listenerKey,
                                    (void **) &listener );

            if (status != FM_OK)
            {
                break;
            }

            /* remove the listener from the listener tree */
            status = fmTreeRemoveCertain(&group->preLagListenerTree,
                                         listenerKey,
                                         NULL);
            if (status != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_MULTICAST,
                              "fmTreeRemoveCertain returned error %d: %s\n",
                              status,
                              fmErrorMsg(status) );
            }

            /* free the memory */
            fmFree(listener);
        }

        /* Remove ECMP from multicast group ECMP tree and delete them */
        while (1)
        {
            fmTreeIterInit(&ecmpIter, &group->ecmpTree);

            status = fmTreeIterNext(&ecmpIter,
                                    &ecmpKey,
                                    (void **) &ecmpGroup);

            if (status != FM_OK)
            {
                break;
            }

            /* remove the ECMP from the listener tree */
            status = fmTreeRemoveCertain(&group->ecmpTree,
                                         ecmpKey,
                                         NULL);

            if (status != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_MULTICAST,
                              "fmTreeRemoveCertain returned error %d: %s\n",
                              status,
                              fmErrorMsg(status) );
            }

            /* free the memory */
            fmFree(ecmpGroup);
        }

        /* Remove  adresses targeted by multicast group */
        while (1)
        {
            fmCustomTreeIterInit(&addrIter, &group->addressTree);

            status = fmCustomTreeIterNext(&addrIter,
                                          (void **) &addrKey,
                                          (void **) &addrValue);

            if (status != FM_OK)
            {
                break;
            }

            /* remove the address from the address tree */
            status = fmCustomTreeRemoveCertain(&group->addressTree,
                                               addrKey,
                                               NULL);

            if (status != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_MULTICAST,
                              "fmCustomTreeRemoveCertain returned error %d: %s\n",
                              status,
                              fmErrorMsg(status) );
            }

             status = fmCustomTreeRemove(&switchPtr->mcastAddressTree,
                                         addrKey,
                                         NULL);

            if (status != FM_OK)
            {
                FM_LOG_ERROR( FM_LOG_CAT_MULTICAST,
                              "fmCustomTreeRemove returned error %d: %s\n",
                              status,
                              fmErrorMsg(status) );
            }

            /* free the memory */
            fmFree(addrValue);
        }

        if (group->extension != NULL)
        {
            fmFree(group->extension);
            group->extension = NULL;
        }

        fmCustomTreeDestroy(&group->addressTree, NULL);

        /* Remove the group from the tree */
        status = fmTreeRemoveCertain(&switchPtr->mcastTree, key, NULL);

        if (status != FM_OK)
        {
            break;
        }

        /* release the logical port */
        if (group->logicalPort != FM_LOGICAL_PORT_NONE)
        {
            status = fmTreeRemove( &switchPtr->mcastPortTree,
                                  (fm_uint64) group->logicalPort,
                                  NULL );

            if (status != FM_OK)
            {
                break;
            }

            fmFreeLogicalPort(switchPtr->switchNumber, group->logicalPort);
        }

        /* release the group's memory */
        fmFree(group);
    }

    /* Remove all replication groups from the tree and delete them */
    while (1)
    {
        fmTreeIterInit(&iter, &switchPtr->replicationTree);

        status = fmTreeIterNext( &iter, &key, (void **) &repliGroup );

        if (status != FM_OK)
        {
            if (status == FM_ERR_NO_MORE)
            {
                status = FM_OK;
            }

            break;
        }

        /* Remove the group from the tree */
        status = fmTreeRemoveCertain(&switchPtr->replicationTree, key, NULL);

        if (status != FM_OK)
        {
            break;
        }

        /* release the group's memory */
        fmFree(repliGroup);
    }

    fmTreeDestroy(&switchPtr->mcastTree, NULL);
    fmCustomTreeDestroy(&switchPtr->mcastAddressTree, NULL);
    fmTreeDestroy(&switchPtr->mcastPortTree, NULL);
    fmDeleteBitArray(&switchPtr->mcastHandles);
    fmTreeDestroy(&switchPtr->mcastHandlePortTree, NULL);
    fmTreeDestroy(&switchPtr->replicationTree, NULL);
    fmDeleteBitArray(&switchPtr->replicationHandles);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmFreeMcastGroupDataStructures */




/*****************************************************************************/
/** fmMcastGroupInit
 * \ingroup intMulticast
 *
 * \desc            Perform initialization for multicast group subsystem,
 *                  called at switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory could not be allocated.
 *
 *****************************************************************************/
fm_status fmMcastGroupInit(fm_int sw)
{
    fm_status  status;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    fmTreeInit(&switchPtr->mcastTree);

    if (switchPtr->CompareMulticastAddresses != NULL)
    {
        fmCustomTreeInit(&switchPtr->mcastAddressTree, switchPtr->CompareMulticastAddresses);
    }
    else
    {
        fmCustomTreeInit(&switchPtr->mcastAddressTree, fmCompareMulticastAddresses);
    }

    fmTreeInit(&switchPtr->mcastPortTree);
    fmTreeInit(&switchPtr->mcastHandlePortTree);

    status = fmCreateBitArray(&switchPtr->mcastHandles,
                              FM_MAX_LOGICAL_PORT);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* Pre-reserve bit 0 - all multicast group handles are non-zero */
    status = fmSetBitArrayBit(&switchPtr->mcastHandles, 0, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* Replication group resources */
    fmTreeInit(&switchPtr->replicationTree);

    status = fmCreateBitArray(&switchPtr->replicationHandles,
                              FM_MAX_NUM_REPLICATION_GROUP);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmMcastGroupInit */




/*****************************************************************************/
/** fmCreateMcastGroup
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Create a multicast group.
 *
 * \note            This function should not be used after calling
 *                  ''fmAllocateStackMcastGroups'' (see ''Stacking and GloRT
 *                  Management'' in the API User Guide). Instead, use
 *                  ''fmCreateStackMcastGroup''.
 *
 * \note            The multicast group number returned by this function is
 *                  a handle, not a logical port number. The corresponding
 *                  logical port number, if it exists, may be retrieved
 *                  by calling ''fmGetMcastGroupPort''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      mcastGroup points to caller-allocated storage where this
 *                  function should place the multicast group number (handle)
 *                  of the newly created multicast group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MEM if not enough memory is available for the
 *                  multicast group structure.
 * \return          FM_ERR_NO_MCAST_RESOURCES if no more multicast group
 *                  resoures are available.
 * \return          FM_ERR_LPORT_DESTS_UNAVAILABLE if a block of logical port
 *                  destination table entries cannot be allocated.
 *
 *****************************************************************************/
fm_status fmCreateMcastGroup(fm_int sw, fm_int *mcastGroup)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %p\n",
                     sw,
                     (void *) mcastGroup);

    err = fmCreateMcastGroupInt(sw, mcastGroup, FALSE, FALSE);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmCreateMcastGroup */




/*****************************************************************************/
/** fmDeleteMcastGroupInt
 * \ingroup intMulticast
 *
 * \desc            Delete a multicast group previously created in a call to
 *                  ''fmCreateMcastGroup'', ''fmCreateStackMcastGroup'' or
 *                  ''fmCreateStackMcastGroupExt''. The multicast group must
 *                  first be deactivated by calling ''fmDeactivateMcastGroup''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to delete.
 *
 * \param[in]       internal refer to the multicast internal flag. This is used
 *                  for validation.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the multicast group is still
 *                  active.
 * \return          FM_ERR_INTERNAL_RESOURCE if the internal attribute does not
 *                  match the group type. 
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupInt(fm_int sw, fm_int mcastGroup, fm_bool internal)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *listener;
    fm_port *                portPtr;
    fm_uint64                key;
    fm_treeIterator          iter;
    fm_uint64                ecmpKey;
    fm_treeIterator          ecmpIter;
    fm_intMulticastEcmp *    ecmpGroup;
    fm_bool                  lockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d\n",
                 sw,
                 mcastGroup);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;
    group     = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    if (group->activated)
    {
        err = FM_ERR_MCAST_GROUP_ACTIVE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    if (group->internal != internal)
    {
        err = FM_ERR_INTERNAL_RESOURCE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmClearMcastGroupAddressesInt(sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* delete all the listeners */
    while (1)
    {
        fmTreeIterInit(&iter, &group->listenerTree);

        err = fmTreeIterNext( &iter, &key, (void **) &listener );

        if (err != FM_OK)
        {
            break;
        }

        err = DeleteMulticastListener(sw, group, listener);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* This seems like a double free - bug 11423 */
        /* fmFree(listener); */
    }

    /* delete all the listeners that are suspended because they are
     * in LAGs.
     */
    while (1)
    {
        fmTreeIterInit(&iter, &group->preLagListenerTree);

        err = fmTreeIterNext( &iter, &key, (void **) &listener );

        if (err != FM_OK)
        {
            break;
        }

        err = fmTreeRemoveCertain(&group->preLagListenerTree, key, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        portPtr = switchPtr->portTable[listener->listener.info.portVlanListener.port];

        err = fmTreeRemove(&portPtr->mcastGroupList,
                           GET_MG_KEY(group->handle,
                                      listener->listener.info.portVlanListener.vlan),
                           NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        fmFree(listener);
    }

    /* Remove ECMP from multicast group ECMP tree and delete them */
    while (1)
    {
        fmTreeIterInit(&ecmpIter, &group->ecmpTree);

        err = fmTreeIterNext(&ecmpIter,
                             &ecmpKey,
                             (void **) &ecmpGroup);

        if (err != FM_OK)
        {
            break;
        }

        err = fmDeleteECMPGroupInternal(sw, ecmpGroup->ecmpId);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* remove the ECMP from the listener tree */
        err = fmTreeRemoveCertain(&group->ecmpTree,
                                  ecmpKey,
                                  NULL);


        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* free the memory */
        fmFree(ecmpGroup);
    }

    /* delete the group from the hardware */
    FM_API_CALL_FAMILY(err, switchPtr->DeleteMcastGroup, sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmTreeRemove( &switchPtr->mcastTree,
                       (fm_uint64) group->handle,
                       NULL );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (group->logicalPort != FM_LOGICAL_PORT_NONE)
    {
        err = fmTreeRemove( &switchPtr->mcastPortTree,
                           (fm_uint64) group->logicalPort,
                           NULL );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmFreeLogicalPort(sw, group->logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    if (group->localHandle)
    {
        err = fmSetBitArrayBit(&switchPtr->mcastHandles,
                               group->handle,
                               FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    if ( !group->stackGroup && (switchPtr->UnassignMcastGroup != NULL) )
    {
        err = switchPtr->UnassignMcastGroup(sw, group->handle);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    fmFree(group);


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroupInt */




/*****************************************************************************/
/** fmDeleteMcastGroup
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a multicast group previously created in a call to
 *                  ''fmCreateMcastGroup'', ''fmCreateStackMcastGroup'' or
 *                  ''fmCreateStackMcastGroupExt''. The multicast group must
 *                  first be deactivated by calling ''fmDeactivateMcastGroup''.
 *
 * \note            Any listeners in the multicast group will be automatically
 *                  deleted by this function. There is no need to call
 *                  ''fmDeleteMcastGroupListener'' for each listener before
 *                  calling this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the multicast group is still
 *                  active.
 * \return          FM_ERR_INTERNAL_RESOURCE if the multicast group is internal.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroup(fm_int sw, fm_int mcastGroup)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d\n",
                     sw,
                     mcastGroup);

    err = fmDeleteMcastGroupInt(sw, mcastGroup, FALSE);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroup */




/*****************************************************************************/
/** fmAddMcastGroupListenerInternal
 * \ingroup intMulticast
 *
 * \desc            Add a listener to a multicast group (join).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listener should be
 *                  added.
 *
 * \param[in]       listener points to an ''fm_mcastGroupListener'' structure
 *                  describing the listener to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if this listener is already in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 * \return          FM_ERR_UNSUPPORTED if the listenerType field contains an
 *                  unknown value.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListenerInternal(fm_int                 sw,
                                          fm_int                 mcastGroup,
                                          fm_mcastGroupListener *listener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_bool                  routingLockTaken;
    fm_bool                  flowLockTaken;
    fm_bool                  mcastHNIFlooding;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, listener = %p\n",
                 sw,
                 mcastGroup,
                 (void *) listener);
    FM_DBG_DUMP_LISTENER(listener);

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    mcastHNIFlooding = fmGetBoolApiProperty(FM_AAK_API_MULTICAST_HNI_FLOODING,
                                            FM_AAD_API_MULTICAST_HNI_FLOODING);

    switchPtr          = GET_SWITCH_PTR(sw);
    routingLockTaken   = FALSE;
    flowLockTaken      = FALSE;
    group              = NULL;
    intListener        = NULL;

    switch (listener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            err = ValidateListenerPort(sw, listener->info.portVlanListener.port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            err = ValidateVNTunnelListener(sw, listener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            break;

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            err = ValidateFlowTunnelListener(sw, listener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            err = fmCaptureLock(&switchPtr->flowLock, FM_WAIT_FOREVER);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            flowLockTaken = TRUE;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    routingLockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = CreateListener(sw, group, listener, &intListener);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (group->activated)
    {
        err = AddListenerToHardware(sw, group, intListener);
        if (err != FM_OK)
        {
            DeleteListener(sw, group, listener);
        }
    }

ABORT:

    /* Check if group should be configured as requestes by HNI or not. */
    if ( (mcastHNIFlooding) && (err == FM_OK) && 
         (switchPtr->swag < 0)  && !(group->internal) )
    {
        if ( ( fmIsMcastGroupHNIFlooding(sw, group->handle) ) &&
             ( fmHasMcastGroupNonVirtualListeners(sw, group->handle) ) )
        {
            /* Mcast group shouldn't be configured as requested by HNI anymore */
            err = fmConfigureMcastGroupAsHNIFlooding(sw,
                                                     group->handle,
                                                     FALSE);
        }
        else if ( ( !fmIsMcastGroupHNIFlooding(sw, group->handle) ) &&
                  ( !fmHasMcastGroupNonVirtualListeners(sw, group->handle) ) )
        {
            /* Mcast group should be configured as requested by HNI */
            err = fmConfigureMcastGroupAsHNIFlooding(sw,
                                                     group->handle,
                                                     TRUE);
        }
    }

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (flowLockTaken)
    {
        fmReleaseLock(&switchPtr->flowLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListenerInternal */




/*****************************************************************************/
/** fmAddMcastGroupListenerInternalForFlood
 * \ingroup intMulticast
 *
 * \desc            Add a flood listener to a multicast group (join).
 *                  Flood listeners are being added when multicast is 
 *                  being configured as HNI flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listener should be
 *                  added.
 *
 * \param[in]       listener points to an ''fm_mcastGroupListener'' structure
 *                  describing the listener to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if this listener is already in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 * \return          FM_ERR_UNSUPPORTED if the listenerType field contains an
 *                  unknown value.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListenerInternalForFlood(fm_int                 sw,
                                                  fm_int                 mcastGroup,
                                                  fm_mcastGroupListener *listener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_bool                  lockTaken;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, listener = %p\n",
                 sw,
                 mcastGroup,
                 (void *) listener);
    FM_DBG_DUMP_LISTENER(listener);

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr   = GET_SWITCH_PTR(sw);
    lockTaken   = FALSE;
    group       = NULL;
    intListener = NULL;

    switch (listener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            err = ValidateListenerPort(sw, listener->info.portVlanListener.port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = CreateListener(sw, group, listener, &intListener);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    intListener->floodListener = TRUE;

    if (group->activated)
    {
        err = AddListenerToHardware(sw, group, intListener);
        if (err != FM_OK)
        {
            DeleteListener(sw, group, listener);
        }
    }

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListenerInternalForFlood */





/*****************************************************************************/
/** fmAddMcastGroupListener
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a listener to a multicast group (join).
 *
 * \note            See also ''fmAddMcastGroupListenerList'', which allows
 *                  multiple listeners to be added at a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listener should be
 *                  added.
 *
 * \param[in]       listener points to an ''fm_multicastListener'' structure
 *                  describing the listener to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if this listener is already in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListener(fm_int                sw,
                                  fm_int                mcastGroup,
                                  fm_multicastListener *listener)
{
    fm_status             err;
    fm_mcastGroupListener tempListener;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, listener = %p "
                     "(vlan %d, port %d)\n",
                     sw,
                     mcastGroup,
                     (void *) listener,
                     (listener != NULL) ? (int) listener->vlan : -1,
                     (listener != NULL) ? listener->port : -1);

    if (listener == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_CLEAR(tempListener);

    tempListener.listenerType          = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
    tempListener.info.portVlanListener = *listener;

    err = fmAddMcastGroupListenerInternal(sw, mcastGroup, &tempListener);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListener */




/*****************************************************************************/
/** fmAddMcastGroupListenerV2
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add a listener to a multicast group (join).
 *
 * \note            See also ''fmAddMcastGroupListenerListV2'', which allows
 *                  multiple listeners to be added at a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listener should be
 *                  added.
 *
 * \param[in]       listener points to an ''fm_mcastGroupListener'' structure
 *                  describing the listener to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if this listener is already in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListenerV2(fm_int                 sw,
                                    fm_int                 mcastGroup,
                                    fm_mcastGroupListener *listener)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, listener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) listener);
    FM_DBG_DUMP_LISTENER(listener);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmAddMcastGroupListenerInternal(sw, mcastGroup, listener);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListenerV2 */




/*****************************************************************************/
/** fmAddMcastGroupListenerListInternal
 * \ingroup intMulticast
 *
 * \desc            Add one or more listeners to a multicast group (join).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listeners should be
 *                  added.
 *
 * \param[in]       numListeners is the number of listeners in listenerList.
 *
 * \param[in]       listenerList is an array of ''fm_mcastGroupListener''
 *                  structures describing the listeners to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if a listener port is not a valid
 *                  logical port.
 * \return          FM_ERR_PORT_IS_IN_LAG if a listener port is a member
 *                  of a Link-Aggregation Group.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if a listener is already in the
 *                  multicast group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListenerListInternal(fm_int                 sw,
                                              fm_int                 mcastGroup,
                                              fm_int                 numListeners,
                                              fm_mcastGroupListener *listenerList)
{
    fm_switch *              switchPtr;
    fm_mcastGroupListener *  listener;
    fm_intMulticastGroup *   groupPtr;
    fm_intMulticastListener *intListener;
    fm_bool                  routingLockTaken;
    fm_bool                  flowLockTaken;
    fm_bool                  mcastHNIFlooding;
    fm_status                err;
    fm_int                   i;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, numListeners = %d "
                 "listenerList = %p\n",
                 sw,
                 mcastGroup,
                 numListeners,
                 (void *) listenerList);

    switchPtr        = GET_SWITCH_PTR(sw);
    routingLockTaken = FALSE;
    flowLockTaken    = FALSE;

    mcastHNIFlooding = fmGetBoolApiProperty(FM_AAK_API_MULTICAST_HNI_FLOODING,
                                            FM_AAD_API_MULTICAST_HNI_FLOODING);

    /*****************************************************
     * Validate the logical ports.
     *****************************************************/
    for (i = 0 ; i < numListeners ; i++)
    {
        listener = &listenerList[i];

        switch (listener->listenerType)
        {
            case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
                err = ValidateListenerPort(sw, listener->info.portVlanListener.port);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                break;

            case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
                err = ValidateVNTunnelListener(sw, listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                break;

            case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
                err = ValidateFlowTunnelListener(sw, listener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                if (flowLockTaken == FALSE)
                {
                    err = fmCaptureLock(&switchPtr->flowLock, FM_WAIT_FOREVER);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                    flowLockTaken = TRUE;
                }
                break;

            default:
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

    }   /* for (i = 0 ; i < numListeners ; i++) */

    /*****************************************************
     * Capture the routing lock.
     *****************************************************/

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    routingLockTaken = TRUE;

    /*****************************************************
     * Get a pointer to the multicast group object.
     *****************************************************/

    groupPtr = fmFindMcastGroup(sw, mcastGroup);
    if (groupPtr == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /*****************************************************
     * Create internal listener objects and add them to
     * the multicast group.
     *****************************************************/

    for (i = 0 ; i < numListeners ; i++)
    {
        listener = &listenerList[i];

        /* Create a listener for this (vlan, port). */
        err = CreateListener(sw, groupPtr, listener, &intListener);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* Activate the listener. */
        if (groupPtr->activated)
        {
            err = AddListenerToHardware(sw, groupPtr, intListener);
            if (err != FM_OK)
            {
                DeleteListener(sw, groupPtr, listener);
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }
        }

    }   /* for (i = 0 ; i < numListeners ; i++) */

ABORT:

    /* Check if group should be configured as requestes by HNI or not. */
    if ( (mcastHNIFlooding) && (err == FM_OK) && 
         (switchPtr->swag < 0)  && !(groupPtr->internal) )
    {
        if ( ( fmIsMcastGroupHNIFlooding(sw, groupPtr->handle) ) &&
             ( fmHasMcastGroupNonVirtualListeners(sw, groupPtr->handle) ) )
        {
            /* Mcast group shouldn't be configured as requested by HNI anymore */
            err = fmConfigureMcastGroupAsHNIFlooding(sw,
                                                     groupPtr->handle,
                                                     FALSE);
        }
        else if ( ( !fmIsMcastGroupHNIFlooding(sw, groupPtr->handle) ) &&
                  ( !fmHasMcastGroupNonVirtualListeners(sw, groupPtr->handle) ) )
        {
            /* Mcast group should be configured as requested by HNI */
            err = fmConfigureMcastGroupAsHNIFlooding(sw,
                                                     groupPtr->handle,
                                                     TRUE);
        }
    }

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (flowLockTaken)
    {
        fmReleaseLock(&switchPtr->flowLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListenerListInternal */




/*****************************************************************************/
/** fmAddMcastGroupListenerList
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add one or more listeners to a multicast group (join).
 *                  Provided as an efficient alternative to
 *                  ''fmAddMcastGroupListener'' when adding more than one
 *                  listener at a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listeners should be
 *                  added.
 *
 * \param[in]       numListeners is the number of listeners in listenerList.
 *
 * \param[in]       listenerList is an array of ''fm_multicastListener''
 *                  structures describing the listeners to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if a listener port is not a valid
 *                  logical port.
 * \return          FM_ERR_PORT_IS_IN_LAG if a listener port is a member
 *                  of a Link-Aggregation Group.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if a listener is already in the
 *                  multicast group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListenerList(fm_int                 sw,
                                      fm_int                 mcastGroup,
                                      fm_int                 numListeners,
                                      fm_multicastListener * listenerList)
{
    fm_multicastListener * listener;
    fm_mcastGroupListener *mgListenerList;
    fm_mcastGroupListener *mgListener;
    fm_status              err;
    fm_int                 i;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, numListeners = %d "
                     "listenerList = %p\n",
                     sw,
                     mcastGroup,
                     numListeners,
                     (void *) listenerList);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    mgListenerList = NULL;

    if (numListeners < 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }
    else if (numListeners == 0)
    {
        err = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /*****************************************************
     * Create an array of fm_mcastGroupListener objects
     * and populate the array from the fm_multicastListener
     * array.
     *****************************************************/
    i              = numListeners * sizeof(fm_mcastGroupListener);
    mgListenerList = fmAlloc(i);

    if (mgListenerList == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    FM_MEMSET_S(mgListenerList, i, 0, i);

    listener   = listenerList;
    mgListener = mgListenerList;

    for (i = 0 ; i < numListeners ; i++, listener++, mgListener++)
    {
        mgListener->listenerType          = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
        mgListener->info.portVlanListener = *listener;

    }   /* for (i = 0 ; i < numListeners ; i++) */

    /*****************************************************
     * Add the new listeners to the multicast group.
     *****************************************************/
    err = fmAddMcastGroupListenerListInternal(sw,
                                              mcastGroup,
                                              numListeners,
                                              mgListenerList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

ABORT:

    if (mgListenerList != NULL)
    {
        fmFree(mgListenerList);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListenerList */




/*****************************************************************************/
/** fmAddMcastGroupListenerListV2
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Add one or more listeners to a multicast group (join).
 *                  Provided as an efficient alternative to
 *                  ''fmAddMcastGroupListenerV2'' when adding more than one
 *                  listener at a time.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') to which the listeners should be
 *                  added.
 *
 * \param[in]       numListeners is the number of listeners in listenerList.
 *
 * \param[in]       listenerList is an array of ''fm_mcastGroupListener''
 *                  structures describing the listeners to be added.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_PORT if a listener port is not a valid
 *                  logical port.
 * \return          FM_ERR_PORT_IS_IN_LAG if a listener port is a member
 *                  of a Link-Aggregation Group.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_ALREADY_EXISTS if a listener is already in the
 *                  multicast group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupListenerListV2(fm_int                 sw,
                                        fm_int                 mcastGroup,
                                        fm_int                 numListeners,
                                        fm_mcastGroupListener *listenerList)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, numListeners = %d "
                     "listenerList = %p\n",
                     sw,
                     mcastGroup,
                     numListeners,
                     (void *) listenerList);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmAddMcastGroupListenerListInternal(sw,
                                              mcastGroup,
                                              numListeners,
                                              listenerList);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupListenerListV2 */




/*****************************************************************************/
/** fmDeleteMcastGroupListenerInternal
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a listener from a multicast group (leave).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be deleted.
 *
 * \param[in]       listener points to an ''fm_multicastListener'' structure
 *                  describing the listener to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_INVALID_LISTENER if listener is not found in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupListenerInternal(fm_int                 sw,
                                             fm_int                 mcastGroup,
                                             fm_mcastGroupListener *listener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_bool                  routingLockTaken;
    fm_bool                  flowLockTaken;
    fm_bool                  mcastHNIFlooding;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_uint64                listenerKey;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, mcastGroup = %d, listener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) listener);
    FM_DBG_DUMP_LISTENER(listener);

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr        = GET_SWITCH_PTR(sw);
    routingLockTaken = FALSE;
    flowLockTaken    = FALSE;

    mcastHNIFlooding = fmGetBoolApiProperty(FM_AAK_API_MULTICAST_HNI_FLOODING,
                                            FM_AAD_API_MULTICAST_HNI_FLOODING);

    switch (listener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            err = ValidateListenerPort(sw, listener->info.portVlanListener.port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            listenerKey = GET_LISTENER_KEY(listener->info.portVlanListener.port,
                                           listener->info.portVlanListener.vlan);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            err = ValidateVNTunnelListener(sw, listener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            listenerKey = GET_VN_LISTENER_KEY(listener->info.vnListener.tunnelId,
                                              listener->info.vnListener.vni);
            break;

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            err = ValidateFlowTunnelListener(sw, listener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            listenerKey = GET_FLOW_LISTENER_KEY(listener->info.flowListener.tableIndex,
                                                listener->info.flowListener.flowId);

            err = fmCaptureLock(&switchPtr->flowLock, FM_WAIT_FOREVER);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            flowLockTaken = TRUE;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    routingLockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Search the tree of listeners for a matching entry. */
    err = fmTreeFind( &group->listenerTree,
                     listenerKey,
                     (void **) &intListener );

    if (err != FM_OK)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = DeleteMulticastListener(sw, group, intListener);

ABORT:

    /* Check if group should be configured as requestes by HNI or not. */
    if ( (mcastHNIFlooding) && (err == FM_OK) && 
         (switchPtr->swag < 0)  && !(group->internal) )
    {
        if ( ( !fmIsMcastGroupHNIFlooding(sw, group->handle) ) &&
             ( !fmHasMcastGroupNonVirtualListeners(sw, group->handle) ) &&
             ( fmHasMcastGroupVirtualListeners(sw, group->handle) ) )
        {
            /* Mcast group should be configured as requested by HNI */
            err = fmConfigureMcastGroupAsHNIFlooding(sw,
                                                     group->handle,
                                                     TRUE);
        }
        else if ( ( fmIsMcastGroupHNIFlooding(sw, group->handle) ) &&
                  ( !fmHasMcastGroupVirtualListeners(sw, group->handle) ) )
        {
            /* Mcast group shouldn't be configured as requested by HNI */
            err = fmConfigureMcastGroupAsHNIFlooding(sw,
                                                     group->handle,
                                                     FALSE);
        }
    }

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (flowLockTaken)
    {
        fmReleaseLock(&switchPtr->flowLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroupListenerInternal */




/*****************************************************************************/
/** fmDeleteMcastGroupListenerInternalForFlood
 * \ingroup intMulticast
 *
 * \desc            Delete a flood listener from a multicast group (leave).
 *                  Flood listeners are being deleted when multicast is 
 *                  being configured as not HNI flooding.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be deleted.
 *
 * \param[in]       listener points to an ''fm_multicastListener'' structure
 *                  describing the listener to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_INVALID_LISTENER if listener is not found in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupListenerInternalForFlood(fm_int                 sw,
                                                     fm_int                 mcastGroup,
                                                     fm_mcastGroupListener *listener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_bool                  lockTaken = FALSE;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_uint64                listenerKey;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, mcastGroup = %d, listener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) listener);
    FM_DBG_DUMP_LISTENER(listener);

    if (listener == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    switch (listener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            err = ValidateListenerPort(sw, listener->info.portVlanListener.port);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            listenerKey = GET_LISTENER_KEY(listener->info.portVlanListener.port,
                                           listener->info.portVlanListener.vlan);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Search the tree of listeners for a matching entry. */
    err = fmTreeFind( &group->listenerTree,
                     listenerKey,
                     (void **) &intListener );

    if (err != FM_OK)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = DeleteMulticastListener(sw, group, intListener);

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroupListenerInternalForFlood */




/*****************************************************************************/
/** fmDeleteMcastGroupListener
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a listener from a multicast group (leave).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be deleted.
 *
 * \param[in]       listener points to an ''fm_multicastListener'' structure
 *                  describing the listener to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_INVALID_LISTENER if listener is not found in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupListener(fm_int                sw,
                                     fm_int                mcastGroup,
                                     fm_multicastListener *listener)
{
    fm_status             err;
    fm_mcastGroupListener tempListener;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, listener = %p "
                     "(vlan %d, port %d)\n",
                     sw,
                     mcastGroup,
                     (void *) listener,
                     (listener != NULL) ? (int) listener->vlan : -1,
                     (listener != NULL) ? listener->port : -1);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (listener == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    FM_CLEAR(tempListener);
    tempListener.listenerType          = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
    tempListener.info.portVlanListener = *listener;

    err = fmDeleteMcastGroupListenerInternal(sw, mcastGroup, &tempListener);

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroupListener */




/*****************************************************************************/
/** fmDeleteMcastGroupListenerV2
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Delete a listener from a multicast group (leave).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be deleted.
 *
 * \param[in]       listener points to an ''fm_mcastGroupListener'' structure
 *                  describing the listener to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_INVALID_LISTENER if listener is not found in the
 *                  multicast group.
 * \return          FM_ERR_PORT_IS_IN_LAG if the listener port is a member
 *                  of a Link-Aggregation Group.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupListenerV2(fm_int                 sw,
                                       fm_int                 mcastGroup,
                                       fm_mcastGroupListener *listener)
{
    fm_status err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, listener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) listener);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmDeleteMcastGroupListenerInternal(sw, mcastGroup, listener);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroupListenerV2 */




/*****************************************************************************/
/** fmGetMcastGroupList
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return a list of valid multicast group numbers.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      numMcastGroups points to caller allocated storage where
 *                  this function should place the number of valid multicast
 *                  groups returned in mcastGroupList.
 *
 * \param[out]      mcastGroupList is an array that this function will fill
 *                  with the list of valid multicast group numbers.
 *
 * \param[in]       max is the size of mcastGroupList, being the maximum number
 *                  of mmulticast group numbers that mcastGroupList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of valid multicast group numbers.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupList(fm_int  sw,
                              fm_int *numMcastGroups,
                              fm_int *mcastGroupList,
                              fm_int  max)
{
    fm_switch *           switchPtr;
    fm_int                listOffset;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_treeIterator       iter;
    fm_uint64             key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, numMcastGroups = %p, mcastGroupList = %p, "
                     "max = %d\n",
                     sw,
                     (void *) numMcastGroups,
                     (void *) mcastGroupList,
                     max);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }

    listOffset      = 0;
    *numMcastGroups = 0;

    fmTreeIterInit(&iter, &switchPtr->mcastTree);

    while (1)
    {
        err = fmTreeIterNext(&iter, &key, (void **) &group);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }

            break;
        }

        if (listOffset >= max)
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }

        mcastGroupList[listOffset] = group->handle;
        listOffset++;
        (*numMcastGroups)++;
    }

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupList */




/*****************************************************************************/
/** fmGetMcastGroupListenerList
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return a list of multicast listeners in a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener list
 *                  should be retrieved.
 *
 * \param[out]      numListeners points to caller allocated storage where
 *                  this function should place the number of listeners
 *                  returned in listenerList.
 *
 * \param[out]      listenerList is an array that this function will fill with
 *                  the list of multicast group listeners.
 *
 * \param[in]       maxListeners is the size of listenerList, being the
 *                  maximum number of multicast group listeners that
 *                  listenerList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of listeners.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupListenerList(fm_int                sw,
                                      fm_int                mcastGroup,
                                      fm_int *              numListeners,
                                      fm_multicastListener *listenerList,
                                      fm_int                maxListeners)
{
    fm_switch *              switchPtr;
    fm_int                   listOffset;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, numListeners = %p, "
                     "listenerList = %p, maxListeners = %d\n",
                     sw,
                     mcastGroup,
                     (void *) numListeners,
                     (void *) listenerList,
                     maxListeners);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    listOffset    = 0;
    *numListeners = 0;

    fmTreeIterInit(&iter, &group->listenerTree);

    while (1)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &intListener );

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }

            break;
        }

        if (intListener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
        {
            continue;
        }

        if (listOffset >= maxListeners)
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }

        listenerList[listOffset].vlan = intListener->listener.info.portVlanListener.vlan;
        listenerList[listOffset].port = intListener->listener.info.portVlanListener.port;
        listOffset++;
        (*numListeners)++;
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupListenerList */




/*****************************************************************************/
/** fmGetMcastGroupListenerListV2
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return a list of multicast listeners in a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener list
 *                  should be retrieved.
 *
 * \param[out]      numListeners points to caller allocated storage where
 *                  this function should place the number of listeners
 *                  returned in listenerList.
 *
 * \param[out]      listenerList is an array that this function will fill with
 *                  the list of multicast group listeners.
 *
 * \param[in]       maxListeners is the size of listenerList, being the
 *                  maximum number of multicast group listeners that
 *                  listenerList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of listeners.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupListenerListV2(fm_int                 sw,
                                        fm_int                 mcastGroup,
                                        fm_int *               numListeners,
                                        fm_mcastGroupListener *listenerList,
                                        fm_int                 maxListeners)
{
    fm_switch *              switchPtr;
    fm_int                   listOffset;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, numListeners = %p, "
                     "listenerList = %p, maxListeners = %d\n",
                     sw,
                     mcastGroup,
                     (void *) numListeners,
                     (void *) listenerList,
                     maxListeners);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    listOffset    = 0;
    *numListeners = 0;

    fmTreeIterInit(&iter, &group->listenerTree);

    while (1)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &intListener );

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }

            break;
        }

        if (listOffset >= maxListeners)
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }

        listenerList[listOffset] = intListener->listener;
        listOffset++;
        (*numListeners)++;
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupListenerListV2 */




/*****************************************************************************/
/** fmGetMcastGroupAddressList
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Return a list of multicast addresses attached to a
 *                  multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener list
 *                  should be retrieved.
 *
 * \param[out]      numAddresses points to caller allocated storage where
 *                  this function should place the number of addresses
 *                  returned in addressList.
 *
 * \param[out]      addressList is an array that this function will fill with
 *                  the list of multicast group addresses.
 *
 * \param[in]       max is the size of addressList, being the maximum number
 *                  of mmulticast group addresses that addressList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of addresses.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupAddressList(fm_int               sw,
                                     fm_int               mcastGroup,
                                     fm_int *             numAddresses,
                                     fm_multicastAddress *addressList,
                                     fm_int               max)
{
    fm_switch *           switchPtr;
    fm_int                listOffset;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_mcastAddrKey *     addrKey;
    fm_customTreeIterator iter;
    fm_uint64             key;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                      "sw = %d, mcastGroup = %d, numAddresses = %p, "
                      "addressList = %p, max = %d\n",
                      sw,
                      mcastGroup,
                      (void *) numAddresses,
                      (void *) addressList,
                      max );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    listOffset    = 0;
    *numAddresses = 0;

    fmCustomTreeIterInit(&iter, &group->addressTree);

    while (1)
    {
        err = fmCustomTreeIterNext( &iter, (void **) &key, (void **) &addrKey );

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }
            else
            {
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }

            break;
        }

        if (listOffset >= max)
        {
            err = FM_ERR_BUFFER_FULL;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        FM_MEMCPY_S( addressList + listOffset,
                     sizeof(*addressList),
                     &addrKey->addr,
                     sizeof(addrKey->addr) );

        listOffset++;
        (*numAddresses)++;
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupAddressList */




/*****************************************************************************/
/** fmGetMcastGroupFirst
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first mulicast group number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstMcastNumber points to caller-allocated storage where
 *                  this function should place the first multicast group
 *                  number. Will be set to -1 if no multicast groups found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if no multicast group found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupFirst(fm_int sw, fm_int *firstMcastNumber)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_treeIterator       iter;
    fm_uint64             key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, firstMcastNumber = %p\n",
                     sw,
                     (void *) firstMcastNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err == FM_OK)
    {
        fmTreeIterInit(&iter, &switchPtr->mcastTree);

        err = fmTreeIterNext( &iter, &key, (void **) &group);

        if (err == FM_OK)
        {
            *firstMcastNumber = group->handle;
        }

        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupFirst */




/*****************************************************************************/
/** fmGetMcastGroupNext
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next multicast group number, following
 *                  a prior call to this function or to
 *                  ''fmGetMcastGroupFirst''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentMcastNumber is the last multicast group number
 *                  found by a previous call to this function or to
 *                  ''fmGetMcastGroupFirst''.
 *
 * \param[out]      nextMcastNumber points to caller-allocated storage where
 *                  this function should place the number of the next multicast
 *                  group.  Will be set to -1 if no more multicast groups found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if no more multicast groups found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupNext(fm_int  sw,
                              fm_int  currentMcastNumber,
                              fm_int *nextMcastNumber)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_treeIterator       iter;
    fm_uint64             key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, currentMcastNumber = %d, nextMcastNumber = %p\n",
                     sw,
                     currentMcastNumber,
                     (void *) nextMcastNumber);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Note that this function initializes the iterator such that the
     * "next" item will be the current key's item.  This means we have
     * to call fmTreeIterNext twice to actually get the next item
     * in the tree.
     */
    err = fmTreeIterInitFromKey( &iter,
                                &switchPtr->mcastTree,
                                (fm_uint64) currentMcastNumber );

    if (err == FM_OK)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &group );

        if (err == FM_OK)
        {
            err = fmTreeIterNext( &iter, &key, (void **) &group );

            if (err == FM_OK)
            {
                *nextMcastNumber = group->handle;
            }
        }
    }

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupNext */




/*****************************************************************************/
/** fmGetMcastGroupListenerFirst
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first listener in a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be retrieved.
 *
 * \param[out]      firstListener points to caller-allocated storage where this
 *                  function should place the first listener in this multicast
 *                  group. Will be set to -1 if no listeners are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NO_MORE if no multicast group listeners found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupListenerFirst(fm_int                sw,
                                       fm_int                mcastGroup,
                                       fm_multicastListener *firstListener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, firstListener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) firstListener);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    fmTreeIterInit(&iter, &group->listenerTree);

    while (1)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &intListener);

        if (err != FM_OK)
        {
            break;
        }

        if (intListener->listener.listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN)
        {
            firstListener->vlan       = intListener->listener.info.portVlanListener.vlan;
            firstListener->port       = intListener->listener.info.portVlanListener.port;
            firstListener->remoteFlag = intListener->listener.info.portVlanListener.remoteFlag;
            break;
        }
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupListenerFirst */




/*****************************************************************************/
/** fmGetMcastGroupListenerFirstV2
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first listener in a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be retrieved.
 *
 * \param[out]      firstListener points to caller-allocated storage where this
 *                  function should place the first listener in this multicast
 *                  group. Will be set to -1 if no listeners are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NO_MORE if no multicast group listeners found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupListenerFirstV2(fm_int                 sw,
                                         fm_int                 mcastGroup,
                                         fm_mcastGroupListener *firstListener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, firstListener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) firstListener);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    fmTreeIterInit(&iter, &group->listenerTree);

    err = fmTreeIterNext( &iter, &key, (void **) &intListener);

    if (err == FM_OK)
    {
        *firstListener = intListener->listener;
     }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupListenerFirstV2 */




/*****************************************************************************/
/** fmGetMcastGroupListenerNext
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next listener in a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be retrieved.
 *
 * \param[in]       currentListener points to the last listener found by a
 *                  previous call to this function or to
 *                  ''fmGetMcastGroupListenerFirst''.
 *
 * \param[out]      nextListener points to caller-allocated storage where this
 *                  function should place the next listener in this multicast
 *                  group. Will be set to -1 if no more listeners are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NO_MCAST_GROUP_LISTENERS if no multicast group
 *                  listeners found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupListenerNext(fm_int                sw,
                                      fm_int                mcastGroup,
                                      fm_multicastListener *currentListener,
                                      fm_multicastListener *nextListener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, currentListener = %p, nextListener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) currentListener,
                     (void *) nextListener);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    key = GET_LISTENER_KEY(currentListener->port, currentListener->vlan);

    /* Note that this function initializes the iterator such that the
     * "next" item will be the current key's item.  This means we have
     * to call fmTreeIterNext twice to actually get the next item
     * in the tree.
     */
    err = fmTreeIterInitFromKey(&iter, &group->listenerTree, key);

    if (err == FM_OK)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &intListener );

        if (err == FM_OK)
        {
            while (1)
            {
                err = fmTreeIterNext( &iter, &key, (void **) &intListener );

                if (err != FM_OK)
                {
                    break;
                }

                if (intListener->listener.listenerType == FM_MCAST_GROUP_LISTENER_PORT_VLAN)
                {
                    nextListener->vlan       = intListener->listener.info.portVlanListener.vlan;
                    nextListener->port       = intListener->listener.info.portVlanListener.port;
                    nextListener->remoteFlag = intListener->listener.info.portVlanListener.remoteFlag;
                    break;
                }
            }
        }
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupListenerNext */




/*****************************************************************************/
/** fmGetMcastGroupListenerNextV2
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next listener in a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be retrieved.
 *
 * \param[in]       currentListener points to the last listener found by a
 *                  previous call to this function or to
 *                  ''fmGetMcastGroupListenerFirstV2''.
 *
 * \param[out]      nextListener points to caller-allocated storage where this
 *                  function should place the next listener in this multicast
 *                  group. Will be set to -1 if no more listeners are found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NO_MCAST_GROUP_LISTENERS if no multicast group
 *                  listeners found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupListenerNextV2(fm_int                 sw,
                                        fm_int                 mcastGroup,
                                        fm_mcastGroupListener *currentListener,
                                        fm_mcastGroupListener *nextListener)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, currentListener = %p, "
                     "nextListener = %p\n",
                     sw,
                     mcastGroup,
                     (void *) currentListener,
                     (void *) nextListener);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        UNPROTECT_SWITCH(sw);
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);
    }

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    switch (currentListener->listenerType)
    {
        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
            key = GET_LISTENER_KEY(currentListener->info.portVlanListener.port,
                                   currentListener->info.portVlanListener.vlan);
            break;

        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
            key = GET_VN_LISTENER_KEY(currentListener->info.vnListener.tunnelId,
                                      currentListener->info.vnListener.vni);
            break;

        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
            key = GET_FLOW_LISTENER_KEY(currentListener->info.flowListener.tableIndex,
                                        currentListener->info.flowListener.flowId);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Note that this function initializes the iterator such that the
     * "next" item will be the current key's item.  This means we have
     * to call fmTreeIterNext twice to actually get the next item
     * in the tree.
     */
    err = fmTreeIterInitFromKey(&iter, &group->listenerTree, key);

    if (err == FM_OK)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &intListener );

        if (err == FM_OK)
        {
            err = fmTreeIterNext( &iter, &key, (void **) &intListener );

            if (err == FM_OK)
            {
                *nextListener = intListener->listener;
            }
        }
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupListenerNextV2 */




/*****************************************************************************/
/** fmGetMcastGroupAddressFirst
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first address attached to a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the listener should
 *                  be retrieved.
 *
 * \param[out]      firstAddress points to caller-provided storage where this
 *                  function should place the first address in this multicast
 *                  group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NO_MORE if no multicast group addresses were found.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupAddressFirst(fm_int               sw,
                                      fm_int               mcastGroup,
                                      fm_multicastAddress *firstAddress)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_mcastAddrKey *     addrKey;
    fm_customTreeIterator iter;
    fm_mcastAddrKey *     addrValue;
    fm_bool               lockTaken = FALSE;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                      "sw = %d, mcastGroup = %d, firstAddress = %p\n",
                      sw,
                      mcastGroup,
                      (void *) firstAddress );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    fmCustomTreeIterInit(&iter, &group->addressTree);

    err = fmCustomTreeIterNext( &iter, (void *) &addrKey, (void **) &addrValue );

    if (err == FM_OK)
    {
        FM_MEMCPY_S( firstAddress,
                     sizeof(*firstAddress),
                     &addrKey->addr,
                     sizeof(addrKey->addr) );
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupAddressFirst */




/*****************************************************************************/
/** fmGetMcastGroupAddressNext
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next address attached to a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'') from which the address should
 *                  be retrieved.
 *
 * \param[in]       currentAddress points to the last address found by a
 *                  previous call to this function or to
 *                  ''fmGetMcastGroupAddressFirst''.
 *
 * \param[out]      nextAddress points to caller-provided storage where this
 *                  function should place the next address in this multicast
 *                  group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NOT_FOUND if currentAddress cannot be found in
 *                  the group's address list.
 * \return          FM_ERR_NO_MORE if there are no more addresses attached to
 *                  the group.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupAddressNext(fm_int               sw,
                                     fm_int               mcastGroup,
                                     fm_multicastAddress *currentAddress,
                                     fm_multicastAddress *nextAddress)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_mcastAddrKey *     addrKey;
    fm_mcastAddrKey *     addrValue;
    fm_mcastAddrKey       searchKey;
    fm_customTreeIterator iter;
    fm_bool               lockTaken = FALSE;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                      "sw = %d, mcastGroup = %d, currentAddress = %p, "
                      "nextAddress = %p\n",
                      sw,
                      mcastGroup,
                      (void *) currentAddress,
                      (void *) nextAddress );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    searchKey.vlMode = switchPtr->vlanLearningMode;

    FM_MEMCPY_S( &searchKey.addr,
                 sizeof(searchKey.addr),
                 currentAddress,
                 sizeof(*currentAddress) );

    /* Note that this function initializes the iterator such that the
     * "next" item will be the current key's item.  This means we have
     * to call fmCustomTreeIterNext twice to actually get the next item
     * in the tree. */
    err = fmCustomTreeIterInitFromKey( &iter,
                                       &group->addressTree,
                                       (void *) &searchKey );

    if (err == FM_OK)
    {
        err = fmCustomTreeIterNext( &iter,
                                    (void **) &addrKey,
                                    (void **) &addrValue );

        if (err == FM_OK)
        {
            err = fmCustomTreeIterNext( &iter,
                                        (void **) &addrKey,
                                        (void **) &addrValue );

            if (err == FM_OK)
            {
                FM_MEMCPY_S( nextAddress,
                             sizeof(*nextAddress),
                             &addrKey->addr,
                             sizeof(addrKey->addr) );
            }
        }
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupAddressNext */




/*****************************************************************************/
/** fmSetMcastGroupAddress
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set the address targeted by a multicast group.
 *                  This function does not activate the multicast group
 *                  (see ''fmActivateMcastGroup'').
 *
 * \note            This function may not be called for a multicast group that
 *                  is currently activated, i.e., the group must first be
 *                  deactivated using ''fmDeactivateMcastGroup'' before the
 *                  address can be changed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \param[in]       address points to the multicast address which is to be
 *                  used for this group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the group is currently active.
 * \return          FM_ERR_MCAST_ADDRESS_IN_USE if the address is already
 *                  in use by another multicast group.
 *
 *****************************************************************************/
fm_status fmSetMcastGroupAddress(fm_int               sw,
                                 fm_int               mcastGroup,
                                 fm_multicastAddress *address)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_int                i;
    fm_intMulticastGroup *group            = NULL;
    fm_bool               lockTaken        = FALSE;
    fm_bool               addressAllocated = FALSE;
    fm_ipAddr             destIpAddr;
    fm_mcastAddrKey *     addrKey;
    fm_char               textOut[200];


    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, address = %p\n",
                     sw,
                     mcastGroup,
                     (void *) address);

    if (address == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (address->addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
    {
        if ( !fmIsMulticastMacAddress(address->info.mac.destMacAddress) )
        {
            err = FM_ERR_NOT_MULTICAST_ADDRESS;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }
    }
    else
    {
        err = fmApplyMasksToMulticastAddress(address);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        fmGetMcastDestAddress(address, &destIpAddr);

        if ( !fmIsMulticastIPAddress(&destIpAddr) )
        {
            err = FM_ERR_NOT_MULTICAST_ADDRESS;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }
    }

    fmDbgBuildMulticastDescription(address, textOut);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST, "mcast addr = %s\n", textOut);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* if the group is active, return error */
    if (group->activated)
    {
        err = FM_ERR_MCAST_GROUP_ACTIVE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* See if the address is already in use by another group */
    err = fmFindMcastGroupByAddress(sw, address, &i);

    if (err == FM_OK)
    {
        /* It is in use.  Unless the caller is trying to add the
         * address to the same group that already has that address,
         * this is an error.  If it is the same group, just return
         * FM_OK and do nothing.
         */
        if (i != mcastGroup)
        {
            err = FM_ERR_MCAST_ADDRESS_IN_USE;
        }

        goto ABORT;
    }

    /* Remove any existing addresses from the address tree */
    err = fmClearMcastGroupAddressesInt(sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Copy the address into the group */
    addrKey = fmAlloc( sizeof(fm_mcastAddrKey) );

    if (addrKey == NULL)
    {
        err = FM_ERR_NO_MEM;

        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    memset( addrKey, 0, sizeof(fm_mcastAddrKey) );

    group->singleMcastAddr = addrKey;
    addressAllocated       = TRUE;

    addrKey->vlMode = switchPtr->vlanLearningMode;

    FM_MEMCPY_S( &addrKey->addr,
                 sizeof(addrKey->addr),
                 address,
                 sizeof(*address) );

    err = fmCustomTreeInsert( &group->addressTree,
                              (void *) addrKey,
                              (void *) addrKey );

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Set the address flags appropriately */
    if (address->addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
    {
        group->defaultVlan = address->info.mac.vlan;

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Updating group vlan to %d\n",
                     group->defaultVlan);
    }
    else
    {
        /* Capture the default VLAN if it was 0 and L3 routing is
           never going to happen on this group */
        if ( group->l2SwitchingOnly )
        {
            if (address->addressType == FM_MCAST_ADDR_TYPE_DSTIP_VLAN)
            {
                group->defaultVlan = address->info.dstIpVlanRoute.vlan;
            }
            else if (address->addressType == FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN)
            {
                group->defaultVlan = address->info.dstSrcIpVlanRoute.vlan;
            }
        }
    }

    /* Add the address and group to the address tree */
    err = fmCustomTreeInsert( &switchPtr->mcastAddressTree,
                             (void *) addrKey,
                             (void *) group );

    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_MULTICAST,
                     "mcastAddressTree insert failed for sw %d, group %d, "
                     "err %d (%s)\n",
                     sw,
                     group->handle,
                     err,
                     fmErrorMsg(err) );
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    if (switchPtr->SetMcastGroupAddress != NULL)
    {
        err = switchPtr->SetMcastGroupAddress(sw, group, address);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }


ABORT:

    if (err != FM_OK)
    {
        if (addressAllocated)
        {
            fmFree(group->singleMcastAddr);
            group->singleMcastAddr = NULL;
        }
    }

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmSetMcastGroupAddress */




/*****************************************************************************/
/** fmAddMcastGroupAddressInt
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Adds an L2 or L3 address to a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \param[in]       address points to the multicast address which is to be
 *                  added to this group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NOT_MULTICAST_ADDRESS if the address supplied is
 *                  not a multicast address.
 * \return          FM_ERR_MCAST_ADDRESS_IN_USE if the address is already
 *                  in use by another multicast group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupAddressInt(fm_int               sw,
                                    fm_int               mcastGroup,
                                    fm_multicastAddress *address)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_int                i;
    fm_intMulticastGroup *group;
    fm_bool               lockTaken;
    fm_bool               addrKeyAllocated;
    fm_bool               addressTreeAdded;
    fm_bool               mcastAddressTreeAdded;
    fm_ipAddr             destIpAddr;
    fm_mcastAddrKey *     addrKey;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, mcastGroup = %d, address = %p\n",
                  sw,
                  mcastGroup,
                  (void *) address );

    switchPtr             = GET_SWITCH_PTR(sw);
    group                 = NULL;
    addrKey               = NULL;
    lockTaken             = FALSE;
    addrKeyAllocated      = FALSE;
    addressTreeAdded      = FALSE;
    mcastAddressTreeAdded = FALSE;

    if (address->addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
    {
        if ( !fmIsMulticastMacAddress(address->info.mac.destMacAddress) )
        {
            err = FM_ERR_NOT_MULTICAST_ADDRESS;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }
    }
    else
    {
        err = fmApplyMasksToMulticastAddress(address);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        fmGetMcastDestAddress(address, &destIpAddr);

        if ( !fmIsMulticastIPAddress(&destIpAddr) )
        {
            err = FM_ERR_NOT_MULTICAST_ADDRESS;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* If the group is in single-address mode, this function cannot be
     * used. Use fmSetMcastGroupAddress instead. */
    if (group->singleAddressMode)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* See if the address is already in use by another group */
    err = fmFindMcastGroupByAddress(sw, address, &i);

    if (err == FM_OK)
    {
        /* It is in use.  Unless the caller is trying to add the
         * address to the same group that already has that address,
         * this is an error.  If it is the same group, just return
         * FM_OK and do nothing.
         */
        if (i != mcastGroup)
        {
            err = FM_ERR_MCAST_ADDRESS_IN_USE;
        }

        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Add the address into the group */
    addrKey = fmAlloc( sizeof(fm_mcastAddrKey) );
    if (addrKey == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    addrKeyAllocated = TRUE;
    memset( addrKey, 0, sizeof(fm_mcastAddrKey) );

    addrKey->vlMode = switchPtr->vlanLearningMode;

    FM_MEMCPY_S( &addrKey->addr,
                 sizeof(addrKey->addr),
                 address,
                 sizeof(*address) );

    err = fmCustomTreeInsert( &group->addressTree,
                              (void *) addrKey,
                              (void *) addrKey );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    addressTreeAdded = TRUE;

    /* Add the address and group to the address tree */
    err = fmCustomTreeInsert( &switchPtr->mcastAddressTree,
                             (void *) addrKey,
                             (void *) group );

    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_MULTICAST,
                     "mcastAddressTree insert failed for sw %d, group %d, "
                     "err %d (%s)\n",
                     sw,
                     group->handle,
                     err,
                     fmErrorMsg(err) );

        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    mcastAddressTreeAdded = TRUE;

    if (switchPtr->AddMcastGroupAddress != NULL)
    {
        err = switchPtr->AddMcastGroupAddress(sw, group, address);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }
    else
    {
        /* If the group is active, add the address to the hardware */
        if (group->activated)
        {
            err = AddAddressToHardware(sw, group, addrKey);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    }


ABORT:

    if (err != FM_OK)
    {
        if (mcastAddressTreeAdded)
        {
            fmCustomTreeRemoveCertain( &switchPtr->mcastAddressTree,
                                       (void *) addrKey,
                                       NULL );
        }

        if (addressTreeAdded)
        {
            fmCustomTreeRemoveCertain( &group->addressTree,
                                       (void *) addrKey,
                                       NULL );
        }

        if (addrKeyAllocated)
        {
            fmFree(addrKey);
        }
    }

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupAddressInt */




/*****************************************************************************/
/** fmAddMcastGroupAddress
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Adds an L2 or L3 address to a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \param[in]       address points to the multicast address which is to be
 *                  added to this group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NOT_MULTICAST_ADDRESS if the address supplied is
 *                  not a multicast address.
 * \return          FM_ERR_MCAST_ADDRESS_IN_USE if the address is already
 *                  in use by another multicast group.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupAddress(fm_int               sw,
                                 fm_int               mcastGroup,
                                 fm_multicastAddress *address)
{
    fm_status err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                      "sw = %d, mcastGroup = %d, address = %p\n",
                      sw,
                      mcastGroup,
                     (void *) address );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmAddMcastGroupAddressInt(sw, mcastGroup, address);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmAddMcastGroupAddress */




/*****************************************************************************/
/** fmDeleteMcastGroupAddress
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deletes an L2 or L3 address from a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \param[in]       address points to the multicast address which is to be
 *                  deleted from this group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_NOT_FOUND if the address could not be found in
 *                  the multicast group.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupAddress(fm_int               sw,
                                    fm_int               mcastGroup,
                                    fm_multicastAddress *address)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group     = NULL;
    fm_bool               lockTaken = FALSE;
    fm_mcastAddrKey       searchKey;
    fm_mcastAddrKey *     addrKey;


    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                      "sw = %d, mcastGroup = %d, address = %p\n",
                      sw,
                      mcastGroup,
                     (void *) address );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* If the group is in single-address mode, this function cannot be
     * used. Use fmClearMcastGroupAddress instead. */
    if (group->singleAddressMode)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Find the address in the group */
    FM_CLEAR( searchKey );

    searchKey.vlMode = switchPtr->vlanLearningMode;

    FM_MEMCPY_S( &searchKey.addr,
                 sizeof(searchKey.addr),
                 address,
                 sizeof(*address) );

    err = fmCustomTreeFind(&group->addressTree,
                           &searchKey,
                           (void **) &addrKey);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (switchPtr->DeleteMcastGroupAddress != NULL)
    {
        err = switchPtr->DeleteMcastGroupAddress(sw, group, address);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }
    else
    {
        /* If the group is active, remove the address from the hardware */
        if (group->activated)
        {
            err = DeleteAddressFromHardware(sw, group, addrKey);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    }

    /* Delete the address from the address tree */
    err = fmCustomTreeRemoveCertain( &group->addressTree,
                                     (void *) addrKey,
                                     NULL);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Remove the address/group from the switch's multicast address tree */
    err = fmCustomTreeRemove( &switchPtr->mcastAddressTree,
                              (void *) addrKey,
                              NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    fmFree(addrKey);


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeleteMcastGroupAddress */




/*****************************************************************************/
/** fmClearMcastGroupAddressesInt
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Removes all addresses targeted by a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmClearMcastGroupAddressesInt(fm_int                sw,
                                        fm_intMulticastGroup *group)
{
    fm_status             err;
    fm_switch *           switchPtr;
    fm_customTreeIterator iter;
    fm_mcastAddrKey *     addrKey;
    fm_mcastAddrKey *     addrValue;


    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, group = %p (%d)\n",
                  sw,
                  (void *) group,
                  group->handle );

    switchPtr = GET_SWITCH_PTR(sw);

    /* Retrieve and remove addresses from the group's address list */
    while (1)
    {
        fmCustomTreeIterInit(&iter, &group->addressTree);
        err = fmCustomTreeIterNext( &iter,
                                    (void **) &addrKey,
                                    (void **) &addrValue );

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
            break;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        if (group->activated)
        {
            err = DeleteAddressFromHardware(sw, group, addrKey);

            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }

        err = fmCustomTreeRemoveCertain(&group->addressTree, addrKey, NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* Remove this address from the address tree */
        err = fmCustomTreeRemove(&switchPtr->mcastAddressTree, addrKey, NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        fmFree(addrKey);
    }

    group->singleMcastAddr = NULL;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmClearMcastGroupAddressesInt */




/*****************************************************************************/
/** fmClearMcastGroupAddress
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Removes the address targeted by a multicast group.
 *                  This function may not be called for an activated
 *                   group, i.e., the group must be deactivated using
 *                  ''fmDeactivateMcastGroup'' before the address can be
 *                  removed.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the group is currently active.
 *
 *****************************************************************************/
fm_status fmClearMcastGroupAddress(fm_int sw,
                                   fm_int mcastGroup)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_bool               lockTaken = FALSE;


    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d\n",
                     sw,
                     mcastGroup);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* If the group is active, we can't delete the address */
    if (group->activated)
    {
        err = FM_ERR_MCAST_GROUP_ACTIVE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmClearMcastGroupAddressesInt(sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmClearMcastGroupAddress */




/*****************************************************************************/
/** fmGetMcastGroupAddress
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Get the address targeted by a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \param[in]       address points to caller-allocated storage into which
 *                  the multicast address will be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_ADDR_NOT_ASSIGNED if no address has been
 *                  set for the multicast group using the
 *                  ''fmSetMcastGroupAddress'' function.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupAddress(fm_int               sw,
                                 fm_int               mcastGroup,
                                 fm_multicastAddress *address)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;


    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, address = %p\n",
                     sw,
                     mcastGroup,
                     (void *) address);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err == FM_OK)
    {
        /* Find the multicast group */
        group = fmFindMcastGroup(sw, mcastGroup);

        if (group == NULL)
        {
            err = FM_ERR_INVALID_MULTICAST_GROUP;
        }

        /* If the group doesn't have an address, report an error */
        else if (group->singleMcastAddr == NULL)
        {
            err = FM_ERR_MCAST_ADDR_NOT_ASSIGNED;
        }

        else
        {
            /* return the address */
            FM_MEMCPY_S( address,
                         sizeof(*address),
                         &group->singleMcastAddr->addr,
                         sizeof(group->singleMcastAddr->addr) );
            err = FM_OK;
        }

        fmReleaseReadLock(&switchPtr->routingLock);

    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetMcastGroupAddress */




/*****************************************************************************/
/** fmActivateMcastGroupInt
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Activates a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the group is already active.
 * \return          FM_ERR_MCAST_ADDR_NOT_ASSIGNED if a multicast
 *                  address has not been assigned to the group.
 * \return          FM_FAIL if multicast group could not share a replication
 *                  group with other multicast groups due to a common listener
 *                  port, or an internal logic error occurs.
 * \return          FM_ERR_LPORT_DESTS_UNAVAILABLE if a block of logical port
 *                  destination table entries cannot be allocated.
 *
 *****************************************************************************/
fm_status fmActivateMcastGroupInt(fm_int sw,
                                  fm_int mcastGroup)
{
    fm_switch *               switchPtr;
    fm_status                 err;
    fm_status                 err1;
    fm_intMulticastGroup *    group;
    fm_intMulticastListener * intListener;
    fm_intMulticastListener **listenerList;
    fm_int                    listenerCount;
    fm_int                    nbytes;
    fm_int                    i;
    fm_treeIterator           iter;
    fm_uint64                 key;
    fm_int                    newPort;
    fm_bool                   routingLockTaken;
    fm_bool                   logicalPortAllocated;
    fm_bool                   portGroupInserted;
    fm_bool                   listenersAdded;
    fm_bool                   activatedGroup;
    fm_customTreeIterator     addrIter;
    fm_mcastAddrKey *         addrKey;
    fm_mcastAddrKey *         addrValue;
    fm_bool                   needEcmpGroup;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw = %d, mcastGroup = %d\n", sw, mcastGroup);

    switchPtr            = GET_SWITCH_PTR(sw);
    routingLockTaken     = FALSE;
    logicalPortAllocated = FALSE;
    portGroupInserted    = FALSE;
    listenersAdded       = FALSE;
    activatedGroup       = FALSE;
    needEcmpGroup        = TRUE;
    group                = NULL;
    listenerList         = NULL;
    listenerCount        = 0;

    if (switchPtr->ActivateMcastGroup == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    routingLockTaken = TRUE;

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* If the group was created in single-address mode, and doesn't have an
     * address, report an error. Single-address-mode groups automatically
     * determine L2/L3 resource needs based upon the address. */
    if ( group->singleAddressMode && (group->singleMcastAddr == NULL) )
    {
        err = FM_ERR_MCAST_ADDR_NOT_ASSIGNED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* If the group is already active, report an error */
    if (group->activated)
    {
        err = FM_ERR_MCAST_GROUP_ACTIVE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Does the ecmp group still required */
    switch (group->groupAction.action)
    {
        case FM_ROUTE_ACTION_ROUTE:
            if (group->l2SwitchingOnly)
            {
                needEcmpGroup = FALSE;
            }
            break;

        case FM_ROUTE_ACTION_DROP:
        case FM_ROUTE_ACTION_RPF_FAILURE:
        case FM_ROUTE_ACTION_NOP:
        default:
            needEcmpGroup = FALSE;
            break;
    }

    /* Does the group need a logical port? */
    if (group->logicalPort == FM_LOGICAL_PORT_NONE)
    {
        switch (group->groupAction.action)
        {
            case FM_ROUTE_ACTION_ROUTE:
                if (group->attrLogicalPort == FM_LOGICAL_PORT_NONE)
                {
                    newPort = FM_LOGICAL_PORT_ANY;

                    FM_API_CALL_FAMILY(err,
                                       switchPtr->AllocLogicalPort,
                                       sw,
                                       FM_PORT_TYPE_MULTICAST,
                                       1,
                                       &newPort,
                                       0);

                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                 "Switch %d: Allocated logical port %d "
                                 "for mcast group %d\n",
                                 sw,
                                 newPort,
                                 group->handle);

                    group->logicalPort      = newPort;
                    group->localLogicalPort = TRUE;
                    logicalPortAllocated    = TRUE;
                }
                else
                {
                    group->logicalPort      = group->attrLogicalPort;
                }

                err = fmTreeInsert( &switchPtr->mcastPortTree,
                                    (fm_uint64) group->logicalPort,
                                    (void *) group );

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                portGroupInserted = TRUE;

                break;

            case FM_ROUTE_ACTION_DROP:
            case FM_ROUTE_ACTION_RPF_FAILURE:
            case FM_ROUTE_ACTION_NOP:
            default:
                break;
        }
    }

    /* Activate the Multicast Group in the hardware */
    err = switchPtr->ActivateMcastGroup(sw, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    activatedGroup = TRUE;
    group->activated = TRUE;
    group->updateHardware = FALSE;

    /* Create an ECMP group for this multicast group, if one is needed */
    if (needEcmpGroup)
    {
        err = fmCreateECMPGroupInternal(sw,
                                        &group->ecmpGroup,
                                        NULL,
                                        group);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    /* Build a list of all listeners */
    listenerCount = (fm_int) fmTreeSize(&group->listenerTree);

    if ( (listenerCount > 0) && (group->logicalPort == FM_LOGICAL_PORT_NONE) )
    {
        err = FM_ERR_LOG_PORT_REQUIRED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    if (listenerCount > 0)
    {
        nbytes = listenerCount * sizeof(fm_intMulticastListener *);
        listenerList = fmAlloc(nbytes);

        if (listenerList == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        memset( listenerList, 0, nbytes);

        fmTreeIterInit(&iter, &group->listenerTree);
        i = 0;

        while (1)
        {
            err = fmTreeIterNext( &iter,
                                 &key,
                                 (void **) &intListener );

            if (err == FM_ERR_NO_MORE)
            {
                break;
            }

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            if (i >= listenerCount)
            {
                err = FM_FAIL;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }

            listenerList[i++] = intListener;
        }

        if (i != listenerCount)
        {
            err = FM_FAIL;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        if ( (group->privateGroup == FALSE) &&
             (group->readOnlyRepliGroup == FALSE) &&
             (listenerCount > 0) )
        {
            /* Verify if listenes are orthogonal ot other multicast groups
             * sharing the same replication group.  */
            for (i = 0 ; i < listenerCount ; i++)
            {
                if (!VerifyListener(sw,
                                    group,
                                    listenerList[i]))
                {
                    err = FM_FAIL;
                    FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
                }
            }
        }

        listenersAdded = TRUE;

        /* Add all listeners to the hardware */
        for (i = 0 ; i < listenerCount ; i++)
        {
            err = AddListenerToHardware(sw,
                                        group,
                                        listenerList[i]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    }

    group->updateHardware = TRUE;

    /* Add all multicast addresses to hardware */
    fmCustomTreeIterInit(&addrIter, &group->addressTree);

    while (1)
    {
        err = fmCustomTreeIterNext( &addrIter,
                                    (void **) &addrKey,
                                    (void **) &addrValue );

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = AddAddressToHardware(sw, group, addrKey);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

ABORT:

    if (err != FM_OK && err != FM_ERR_MCAST_GROUP_ACTIVE && group != NULL)
    {
        /* Remove all multicast addresses from hardware */
        fmCustomTreeIterInit(&addrIter, &group->addressTree);
        while (1)
        {

            err1 = fmCustomTreeIterNext( &addrIter,
                                        (void **) &addrKey,
                                        (void **) &addrValue );

            if (err1 == FM_ERR_NO_MORE && err1 != FM_OK)
            {
                break;
            }
            if (err1 != FM_OK)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Switch %d: fmActivateMcastGroup::Abort"
                             "failing in fmCustomTreeIterNext %d\n",
                             sw,
                             err1);
            }


            err1 = DeleteAddressFromHardware(sw, group, addrKey);
            if (err1 != FM_OK)
            {
                  FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Switch %d: fmActivateMcastGroup::Abort"
                             "failing in DeleteAddressFromHardware %d\n",
                             sw,
                             err1);
            }
        }

        group->updateHardware = FALSE;

        /* remove all listeners */
        if (listenersAdded)
        {
            for (i = 0 ; i < listenerCount ; i++)
            {
                err1 = DeleteListenerFromHardware(sw, group, listenerList[i]);
                if (err1 != FM_OK)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Switch %d: fmActivateMcastGroup::Abort"
                             "failing in DeleteListenerFromHardware %d\n",
                             sw,
                             err1);
                }
            }
        }
        group->updateHardware = TRUE;

        /* Delete the ECMP group, if one exists */
        if (group->ecmpGroup != -1)
        {
            err1 = fmDeleteECMPGroupInternal(sw, group->ecmpGroup);
            if (err1 != FM_OK)
                {
                   FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Switch %d: fmActivateMcastGroup::Abort"
                             "failing in fmDeleteECMPGroupInternal %d\n",
                             sw,
                             err1);
                }
            group->ecmpGroup = -1;
        }

        if ( activatedGroup && (switchPtr->DeactivateMcastGroup != NULL) )
        {
            err1 = switchPtr->DeactivateMcastGroup(sw, group);
            if (err1 != FM_OK)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Switch %d: fmActivateMcastGroup::Abort"
                             "failing in DeactivateMcastGroup %d\n",
                             sw,
                             err1);
                }
        }

        group->activated = FALSE;


        if (portGroupInserted)
        {
            fmTreeRemoveCertain( &switchPtr->mcastPortTree,
                                (fm_uint64) group->logicalPort,
                                NULL );
        }

        if (logicalPortAllocated)
        {
            FM_API_CALL_FAMILY(err1,
                               switchPtr->FreeLogicalPort,
                               sw,
                               group->logicalPort);

            group->logicalPort      = FM_LOGICAL_PORT_NONE;
            group->localLogicalPort = FALSE;
        }
    }

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    if (listenerList != NULL)
    {
        fmFree(listenerList);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmActivateMcastGroupInt */




/*****************************************************************************/
/** fmActivateMcastGroup
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Activates a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the group is already active.
 * \return          FM_ERR_MCAST_ADDR_NOT_ASSIGNED if a multicast
 *                  address has not been assigned to the group.
 * \return          FM_FAIL if multicast group could not share a replication
 *                  group with other multicast groups due to a common listener
 *                  port, or an internal logic error occurs.
 * \return          FM_ERR_LPORT_DESTS_UNAVAILABLE if a block of logical port
 *                  destination table entries cannot be allocated.
 *
 *****************************************************************************/
fm_status fmActivateMcastGroup(fm_int sw,
                               fm_int mcastGroup)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d\n",
                     sw,
                     mcastGroup);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmActivateMcastGroupInt(sw, mcastGroup);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmActivateMcastGroup */




/*****************************************************************************/
/** fmDeactivateMcastGroupInt
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deactivates a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fmDeactivateMcastGroupInt(fm_int sw, fm_int mcastGroup)
{
    fm_switch *              switchPtr;
    fm_status                err;
    fm_intMulticastGroup *   group;
    fm_bool                  lockTaken;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm_customTreeIterator    addrIter;
    fm_mcastAddrKey *        addrKey;
    fm_mcastAddrKey *        addrValue;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d\n",
                     sw,
                     mcastGroup);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (switchPtr->DeactivateMcastGroup == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    lockTaken = TRUE;

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* If the group is not active, report an error */
    else if (!group->activated)
    {
        err = FM_ERR_MCAST_GROUP_NOT_ACTIVE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Remove all multicast addresses from hardware */
    fmCustomTreeIterInit(&addrIter, &group->addressTree);

    while (1)
    {
        err = fmCustomTreeIterNext( &addrIter,
                                    (void **) &addrKey,
                                    (void **) &addrValue );

        if (err == FM_ERR_NO_MORE)
        {
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = DeleteAddressFromHardware(sw, group, addrKey);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    group->updateHardware = FALSE;

    /* Remove all listeners from the hardware */
    fmTreeIterInit(&iter, &group->listenerTree);

    while (1)
    {
        err = fmTreeIterNext( &iter, &key, (void **) &intListener );

        if (err != FM_OK)
        {
            break;
        }

        err = DeleteListenerFromHardware(sw, group, intListener);
        if (err != FM_OK)
        {
            group->updateHardware = TRUE;
            goto ABORT;
        }
    }

    group->updateHardware = TRUE;

    /* Delete the ECMP group, if one exists */
    if (group->ecmpGroup != -1)
    {
        err = fmDeleteECMPGroupInternal(sw, group->ecmpGroup);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        group->ecmpGroup = -1;
    }

    /* Deactivate the group in the hardware */
    if (switchPtr->DeactivateMcastGroup != NULL)
    {
        err = switchPtr->DeactivateMcastGroup(sw, group);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    /* Free the logical port */
    if (group->localLogicalPort)
    {
        fmTreeRemove( &switchPtr->mcastPortTree,
                     (fm_uint64) group->logicalPort,
                     NULL );

        FM_API_CALL_FAMILY(err,
                           switchPtr->FreeLogicalPort,
                           sw,
                           group->logicalPort);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        group->logicalPort      = FM_LOGICAL_PORT_NONE;
        group->localLogicalPort = FALSE;
    }
    else if (group->attrLogicalPort != FM_LOGICAL_PORT_NONE)
    {
        fmTreeRemove( &switchPtr->mcastPortTree,
                      (fm_uint64) group->logicalPort,
                      NULL );
        group->logicalPort      = FM_LOGICAL_PORT_NONE;
    }

    group->activated = FALSE;


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeactivateMcastGroupInt */




/*****************************************************************************/
/** fmDeactivateMcastGroup
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Deactivates a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is out of
 *                  range or is not the handle of an existing multicast group.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fmDeactivateMcastGroup(fm_int sw,
                                 fm_int mcastGroup)
{
    fm_status err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d\n",
                     sw,
                     mcastGroup);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmDeactivateMcastGroupInt(sw, mcastGroup);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, err);

}   /* end fmDeactivateMcastGroup */




/*****************************************************************************/
/** fmFindMcastGroupByAddress
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Searches for a multicast group number given a multicast
 *                  address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group number (returned by
 *                  ''fmCreateMcastGroup'').
 *
 * \param[in]       address points to the multicast address which is to be
 *                  used in the search.
 *
 * \param[out]      mcastGroup points to caller-allocated storage into which
 *                  the multicast group number is to be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_MCAST_ADDR_NOT_ASSIGNED if the multicast
 *                  address has not been assigned to any group.
 *
 *****************************************************************************/
fm_status fmFindMcastGroupByAddress(fm_int               sw,
                                    fm_multicastAddress *address,
                                    fm_int *             mcastGroup)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intMulticastGroup *group;
    fm_vlanLearningMode   vlMode;
    fm_mcastAddrKey       key;
    fm_char               textOut[200];

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, address = %p, mcastGroup = %p\n",
                 sw,
                 (void *) address,
                 (void *) mcastGroup);

    if (address == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    fmDbgBuildMulticastDescription(address, textOut);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST, "mcast addr = %s\n", textOut);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    vlMode = switchPtr->vlanLearningMode;

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err == FM_OK)
    {
        key.vlMode = vlMode;
        /* routePtr is set to NULL though it is not compared by CompareMulticastAddress() */
        key.routePtr = NULL;
        FM_MEMCPY_S( &key.addr, sizeof(key.addr), address, sizeof(*address));

        err = fmCustomTreeFind( &switchPtr->mcastAddressTree,
                               (void *) &key,
                               (void **) &group);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NOT_FOUND)
            {
                err = FM_OK;
            }

            group = NULL;
        }
    }

    if (err == FM_OK)
    {
        if (group != NULL)
        {
            *mcastGroup = group->handle;
        }
        else
        {
            err = FM_ERR_MCAST_ADDR_NOT_ASSIGNED;
        }
    }

    fmReleaseReadLock(&switchPtr->routingLock);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmFindMcastGroupByAddress */




/*****************************************************************************/
/** fmGetMcastDestAddress
 * \ingroup intRouter
 *
 * \desc            Retrieves the destination address from a multicast address.
 *
 * \param[in]       multicast points to the route entry.
 *
 * \param[out]      destAddr points to caller-provided storage into which
 *                  the destination address is placed.
 *
 * \return          none.
 *
 *****************************************************************************/
void fmGetMcastDestAddress(fm_multicastAddress *multicast, fm_ipAddr *destAddr)
{
    switch (multicast->addressType)
    {
        case FM_MCAST_ADDR_TYPE_DSTIP:
            FM_MEMCPY_S( destAddr,
                         sizeof(*destAddr),
                         &multicast->info.dstIpRoute.dstAddr,
                         sizeof(multicast->info.dstIpRoute.dstAddr) );
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
            FM_MEMCPY_S( destAddr,
                         sizeof(*destAddr),
                         &multicast->info.dstIpVlanRoute.dstAddr,
                         sizeof(multicast->info.dstIpVlanRoute.dstAddr) );
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
            FM_MEMCPY_S( destAddr,
                         sizeof(*destAddr),
                         &multicast->info.dstSrcIpRoute.dstAddr,
                         sizeof(multicast->info.dstSrcIpRoute.dstAddr) );
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
            FM_MEMCPY_S( destAddr,
                         sizeof(*destAddr),
                         &multicast->info.dstSrcIpVlanRoute.dstAddr,
                         sizeof(multicast->info.dstSrcIpVlanRoute.dstAddr) );
            break;

        default:
            memset( destAddr, 0, sizeof(fm_ipAddr) );
            break;

    }   /* end switch (multicast->addressType) */

}   /* end fmGetMcastDestAddress */




/*****************************************************************************/
/** fmApplyMasksToMulticastAddress
 * \ingroup intMulticast
 *
 * \desc            Applies prefix masks to multicast address values so that
 *                  they are all internally consistent. For instance, an IP
 *                  address such as 1.1.1.5/24 would be masked to 1.1.1.0/24.
 *
 * \param[in]       multicast points to the multicast address entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmApplyMasksToMulticastAddress(fm_multicastAddress *multicast)
{
    fm_int curlen;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST, "multicast = %p\n", (void *) multicast );

    switch (multicast->addressType)
    {
        case FM_MCAST_ADDR_TYPE_L2MAC_VLAN:
            /* No fields to mask */
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP:
            fmMaskIPAddress(&multicast->info.dstIpRoute.dstAddr,
                            multicast->info.dstIpRoute.dstPrefixLength);
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
            fmMaskIPAddress(&multicast->info.dstIpVlanRoute.dstAddr,
                            multicast->info.dstIpVlanRoute.dstPrefixLength);
            curlen = multicast->info.dstIpVlanRoute.vlanPrefixLength;
            if (curlen > 12)
            {
                curlen = 12;
            }
            multicast->info.dstIpVlanRoute.vlan &=
                0xfff & ~( ( 1 << (12 - curlen) ) - 1 );
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
            fmMaskIPAddress(&multicast->info.dstSrcIpRoute.dstAddr,
                            multicast->info.dstSrcIpRoute.dstPrefixLength);
            fmMaskIPAddress(&multicast->info.dstSrcIpRoute.srcAddr,
                            multicast->info.dstSrcIpRoute.srcPrefixLength);
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
            fmMaskIPAddress(&multicast->info.dstSrcIpVlanRoute.dstAddr,
                            multicast->info.dstSrcIpVlanRoute.dstPrefixLength);
            fmMaskIPAddress(&multicast->info.dstSrcIpVlanRoute.srcAddr,
                            multicast->info.dstSrcIpVlanRoute.srcPrefixLength);
            curlen = multicast->info.dstSrcIpVlanRoute.vlanPrefixLength;
            if (curlen > 12)
            {
                curlen = 12;
            }
            multicast->info.dstSrcIpVlanRoute.vlan &=
                0xfff & ~( ( 1 << (12 - curlen) ) - 1 );
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fmApplyMasksToMulticastAddress */




/*****************************************************************************/
/** fmMcastAddPortToLagNotify
 * \ingroup intMcast
 *
 * \desc            Called when a port is added to a lag, updates multicast
 *                  listener tables as needed.
 *
 * \note            This function assumes that the LAG and routing locks
 *                  have already been taken.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[in]       port is the logical port number for the port being added.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMcastAddPortToLagNotify(fm_int sw, fm_int lagIndex, fm_int port)
{
    fm_status                err;
    fm_switch *              switchPtr;
    fm_port *                portPtr;
    fm_port *                lagPortPtr;
    fm_int                   lagLogicalPort;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *listener;
    fm_int                   addedListenerCount = 0;
    fm_int                   deletedListenerCount = 0;
    fm_uint64                listenerKey;
    fm_bool                  internalLag;
    fm_mcastGroupListener    tempListener;

#if FM_SUPPORT_SWAG
    fm_int                   swagSw;
    fmSWAG_switch *          swagExt;
    fm_swagMember *          member;
    fm_int                   swagLagIndex;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    err = fmLagIndexToLogicalPort(sw, lagIndex, &lagLogicalPort);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "fmLagIndexToLogicalPort returned error %d\n",
                     err);
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }

    err =
        fmGetPortAttribute(sw, lagLogicalPort, FM_PORT_INTERNAL, &internalLag);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Ignore ports of type FM_PORT_TYPE_REMOTE with the exception of internal
     * LAG.
     * This can be received in Stacking or SWAG mode. */
    if( (internalLag == FALSE) && (portPtr->portType == FM_PORT_TYPE_REMOTE) )
    {
        FM_LOG_EXIT( FM_LOG_CAT_MULTICAST, FM_OK );
    }

#if FM_SUPPORT_SWAG
    
    if (portPtr->portType != FM_PORT_TYPE_REMOTE)
    {
        err = fmIsSwitchInASWAG(sw, &swagSw);

        if (err == FM_OK)
        {
            swagExt = GET_SWITCH_EXT(swagSw);
            member  = fmFindSwitchInSWAG(swagExt, sw);
            
            if (member == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_SWITCH_NOT_IN_AGGREGATE);
            }
            
            swagLagIndex = member->swagLagGroups[lagIndex];
            
            if (swagLagIndex >= 0)
            {
                err = fmMcastAddPortToLagNotify(swagSw,
                                                swagLagIndex,
                                                portPtr->swagPort);
                FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
            }
        }
    }
#endif

    lagPortPtr = GET_PORT_PTR(sw, lagLogicalPort);

    /* If the port is a member of any multicast groups, move its listener
     * entry to the pre-lag listener list and remove it from the
     * multicast group in the hardware if the group is activated */
    fmTreeIterInit(&iter, &portPtr->mcastGroupList);
    while (1)
    {
        err = fmTreeIterNext(&iter, &key, (void **) &listener);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }
            break;
        }

        if (listener == NULL)
        {
            continue;
        }

        group = listener->group;

        switch (listener->listener.listenerType)
        {
            case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
                listenerKey = GET_LISTENER_KEY(listener->listener.info.portVlanListener.port,
                                               listener->listener.info.portVlanListener.vlan);
                break;

            default:
                err = FM_FAIL;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        err = fmTreeRemove(&group->listenerTree, listenerKey, NULL);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmTreeInsert( &group->preLagListenerTree,
                           listenerKey,
                           (void *) listener );

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* Remove it only if the group is activated */
        if (group->activated)
        {
            FM_API_CALL_FAMILY(err,
                               switchPtr->DeleteMulticastListener,
                               sw,
                               group,
                               listener);
            if (err != FM_OK)
            {
                break;
            }

            deletedListenerCount++;
        }
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* If the LAG is a member of any multicast groups, add the new LAG
     * port to the multicast group in the hardware if the group is activated */
    fmTreeIterInit(&iter, &lagPortPtr->mcastGroupList);
    while (1)
    {
        err = fmTreeIterNext(&iter, &key, (void **) &listener);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }
            break;
        }

        /* Add it only if the group is activated */
        if (listener->group->activated)
        {
            if (fmIsRemotePort(sw, port))
            {
                tempListener = listener->listener;
                tempListener.info.portVlanListener.port = port;
                
                err =
                    fmAddMcastGroupListenerV2(sw,
                                              listener->group->handle,
                                              &tempListener);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);         
            }
            else
            {
                err = AddSubListener(sw, listener->group, listener, port);
                
                if (err != FM_OK)
                {
                    break;
                }
            }
            addedListenerCount++;
        }
    }

ABORT:

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "switch %d port %d deleted %d listeners "
                 "and added %d listeners for LAG %d (port %d), err=%d\n",
                 sw,
                 port,
                 deletedListenerCount,
                 addedListenerCount,
                 lagIndex,
                 lagLogicalPort,
                 err);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmMcastAddPortToLagNotify */




/*****************************************************************************/
/** fmMcastRemovePortFromLagNotify
 * \ingroup intMcast
 *
 * \desc            Called when a port is removed to a lag, updates multicast
 *                  listener tables as needed.
 *
 * \note            This function assumes that the LAG and routing locks
 *                  have already been taken.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \param[in]       port is the logical port number for the port being removed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMcastRemovePortFromLagNotify(fm_int sw,
                                         fm_int lagIndex,
                                         fm_int port)
{
    fm_status                err;
    fm_switch *              switchPtr;
    fm_port *                portPtr;
    fm_port *                lagPortPtr;
    fm_int                   lagLogicalPort;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *listener;
    fm_int                   addedListenerCount = 0;
    fm_int                   deletedListenerCount = 0;
    fm_uint64                listenerKey;
#if FM_SUPPORT_SWAG
    fm_int                   swagSw;
    fmSWAG_switch *          swagExt;
    fm_swagMember *          member;
    fm_int                   swagLagIndex;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, lagIndex = %d, port = %d\n",
                 sw,
                 lagIndex,
                 port);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, port);

    /* Ignore ports of type FM_PORT_TYPE_REMOTE.
     * This can be received in Stacking or SWAG mode. */

    if( portPtr->portType == FM_PORT_TYPE_REMOTE )
    {
        FM_LOG_EXIT( FM_LOG_CAT_MULTICAST, FM_OK );
    }

#if FM_SUPPORT_SWAG
    err = fmIsSwitchInASWAG(sw, &swagSw);

    if (err == FM_OK)
    {
        swagExt = GET_SWITCH_EXT(swagSw);
        member  = fmFindSwitchInSWAG(swagExt, sw);

        if (member == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_LAG, FM_ERR_SWITCH_NOT_IN_AGGREGATE);
        }

        swagLagIndex = member->swagLagGroups[lagIndex];
        err = fmMcastRemovePortFromLagNotify(swagSw,
                                             swagLagIndex,
                                             portPtr->swagPort);

        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }
#endif

    err = fmLagIndexToLogicalPort(sw, lagIndex, &lagLogicalPort);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "fmLagIndexToLogicalPort returned error %d\n",
                     err);
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }

    lagPortPtr = GET_PORT_PTR(sw, lagLogicalPort);

    if (lagPortPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    /* If the LAG is a member of any multicast groups, remove the
     * port from the multicast group in the hardware */
    fmTreeIterInit(&iter, &lagPortPtr->mcastGroupList);
    while (1)
    {
        err = fmTreeIterNext(&iter, &key, (void **) &listener);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }
            break;
        }

        err = DeleteSubListener(sw, listener->group, listener, port);

        if (err != FM_OK)
        {
            break;
        }

        deletedListenerCount++;
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* If the port is a member of any multicast groups, move its listener
     * entry from the pre-lag listener list back to the regular listener list
     * and add it to the multicast group in the hardware if the multicast
     * group is activated */
    fmTreeIterInit(&iter, &portPtr->mcastGroupList);
    while (1)
    {
        err = fmTreeIterNext(&iter, &key, (void **) &listener);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }
            break;
        }

        if (listener == NULL)
        {
            continue;
        }

        group = listener->group;

        switch (listener->listener.listenerType)
        {
            case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
                listenerKey = GET_LISTENER_KEY(listener->listener.info.portVlanListener.port,
                                               listener->listener.info.portVlanListener.vlan);
                break;

            case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
                listenerKey = GET_VN_LISTENER_KEY(listener->listener.info.vnListener.tunnelId,
                                                  listener->listener.info.vnListener.vni);
                break;

            case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
                listenerKey = GET_FLOW_LISTENER_KEY(listener->listener.info.flowListener.tableIndex,
                                                    listener->listener.info.flowListener.flowId);
                break;

            default:
                err = FM_FAIL;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        err = fmTreeRemove(&group->preLagListenerTree, listenerKey, NULL);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmTreeInsert( &group->listenerTree,
                           listenerKey,
                           (void *) listener );

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* Add it only if the group is activated */
        if (group->activated)
        {
            FM_API_CALL_FAMILY(err,
                               switchPtr->AddMulticastListener,
                               sw,
                               group,
                               listener);

            if (err != FM_OK)
            {
                break;
            }

            addedListenerCount++;
        }
    }

ABORT:

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "switch %d LAG %d port %d deleted %d listeners "
                 "and added %d listeners for port %d, err=%d\n",
                 sw,
                 lagIndex,
                 lagLogicalPort,
                 deletedListenerCount,
                 addedListenerCount,
                 port,
                 err);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmMcastRemovePortFromLagNotify */




/*****************************************************************************/
/** fmMcastDeleteLagNotify
 * \ingroup intMcast
 *
 * \desc            Called when a lag is being removed, updates multicast
 *                  listener tables as needed.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       lagIndex is the index of the LAG on the switch.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMcastDeleteLagNotify(fm_int sw, fm_int lagIndex)
{
    fm_status                err;
    fm_switch *              switchPtr;
    fm_port *                lagPortPtr;
    fm_int                   lagLogicalPort;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm_intMulticastListener *listener;
    fm_mcastGroupListener    lagPortListener;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, lagIndex = %d\n",
                 sw,
                 lagIndex);

    err = fmLagIndexToLogicalPort(sw, lagIndex, &lagLogicalPort);

    if (err == FM_ERR_UNSUPPORTED)
    {
        /* This is needed until logical ports are supported on FM2000. */
        err = FM_OK;
        goto ABORT;
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "fmLagIndexToLogicalPort returned error %d\n",
                     err);
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    lagPortPtr = switchPtr->portTable[lagLogicalPort];

    /* If the LAG is a member of any multicast groups, remove the
     * LAG from the multicast group
     */
    while (1)
    {
        /* Initialize the iterator inside the loop - fmDeleteMcastGroupListener
         * will remove the group from the tree, causing the iterator to
         * become invalid.  Since the group will have been removed from
         * the tree, re-initializing the iterator will give us the next group.
         */
        fmTreeIterInit(&iter, &lagPortPtr->mcastGroupList);

        /* Retrieve the next group */
        err = fmTreeIterNext(&iter, &key, (void **) &listener);

        if (err != FM_OK)
        {
            err = FM_OK;
            break;
        }

        /* Remove the port from the group */
        lagPortListener = listener->listener;

        err = fmDeleteMcastGroupListenerV2(sw,
                                           listener->group->handle,
                                           &lagPortListener);

        if (err != FM_OK)
        {
            break;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmMcastDeleteLagNotify */




/*****************************************************************************/
/** fmMcastDeleteVlanNotify
 * \ingroup intMcast
 *
 * \desc            Called when a vlan is being deleted, updates multicast
 *                  listener tables as needed.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vlan is the deleted vlan.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMcastDeleteVlanNotify(fm_int sw, fm_int vlan)
{
    fm_status                err = FM_OK;
    fm_switch *              switchPtr;
    fm_int                   i;
    fm_uint64                key;
    fm_treeIterator          iterGrp;
    fm_intMulticastGroup    *group;
    fm_treeIterator          iterListener;
    fm_intMulticastListener *listener;
    fm_mcastGroupListener *  listenerList;
    fm_int                   listenerListIndex;
    fm_customTreeIterator    iterAddress;
    fm_mcastAddrKey *        addrKey;
    fm_mcastAddrKey *        addrValue;
    fm_multicastAddress *    addressList;
    fm_int                   addressListIndex;
    fm_int                   nbAddresses;
    fm_virtualNetwork *      vn;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, vlan = %d\n",
                 sw,
                 vlan);

    switchPtr = GET_SWITCH_PTR(sw);

    fmTreeIterInit(&iterGrp, &switchPtr->mcastTree);

    while (fmTreeIterNext( &iterGrp, &key, (void **) &group) == FM_OK)
    {
        /* Delete any address that is associated to this vlan.
         * If the mcastGrp has only one address, then disable
         * the group */
        addressListIndex = 0;
        fmCustomTreeIterInit(&iterAddress, &group->addressTree);
        nbAddresses = group->addressTree.internalTree.size;
        if (nbAddresses > 0)
        {
            addressList = fmAlloc(sizeof(fm_multicastAddress) * nbAddresses);

            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_MULTICAST,
                                   addressList != NULL,
                                   err = FM_ERR_NO_MEM,
                                   "Unable to allocate memory");

            while (fmCustomTreeIterNext( &iterAddress,
                                         (void **) &addrKey,
                                         (void **) &addrValue) == FM_OK)
            {
                if ( ( addrKey->addr.addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN &&
                       addrKey->addr.info.mac.vlan == vlan ) ||
                     ( addrKey->addr.addressType == FM_MCAST_ADDR_TYPE_DSTIP_VLAN &&
                       addrKey->addr.info.dstIpVlanRoute.vlan == vlan &&
                       addrKey->addr.info.dstIpVlanRoute.vlanPrefixLength == 12) ||
                     ( addrKey->addr.addressType == FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN &&
                       addrKey->addr.info.dstSrcIpVlanRoute.vlan == vlan &&
                       addrKey->addr.info.dstSrcIpVlanRoute.vlanPrefixLength == 12) )
                {
                    addressList[addressListIndex].addressType = addrKey->addr.addressType;
                    addressList[addressListIndex].info = addrKey->addr.info;
                    addressList[addressListIndex].mcastGroup = addrKey->addr.mcastGroup;
                    addressListIndex++;
                 }
            }

            if (nbAddresses == 1 && addressListIndex == 1)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Deleting mcastGrp=%d address\n",
                             group->handle);

                err = fmDeactivateMcastGroup(sw, addressList[0].mcastGroup);
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_MULTICAST,
                                       err == FM_OK,
                                       fmFree(addressList),
                                       "Failed to deactivate multicast group\n");

                err = fmClearMcastGroupAddress(sw, addressList[0].mcastGroup);
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_MULTICAST,
                                       err == FM_OK,
                                       fmFree(addressList),
                                       "Failed to clear the multicast group address\n");
            }
            else
            {
                for (i = 0; i < addressListIndex; i++)
                {
                    err = fmDeleteMcastGroupAddress(sw,
                                                    addressList[i].mcastGroup,
                                                    &addressList[i]);
                    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_MULTICAST,
                                           err == FM_OK,
                                           fmFree(addressList),
                                           "Failed to remove the multicast group address\n");
                }
            }

            fmFree(addressList);

            /* Delete any listener that is associated to this vlan */
            listenerListIndex = 0;
            fmTreeIterInit(&iterListener, &group->listenerTree);

            if (group->listenerTree.internalTree.size > 0)
            {
                listenerList = fmAlloc(sizeof(fm_mcastGroupListener) *
                                       group->listenerTree.internalTree.size);
                FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_MULTICAST,
                                       listenerList != NULL,
                                       err = FM_ERR_NO_MEM,
                                       "Unable to allocate memory\n");

                while (fmTreeIterNext( &iterListener, &key, (void **) &listener) == FM_OK)
                {
                    switch (listener->listener.listenerType)
                    {
                        case FM_MCAST_GROUP_LISTENER_PORT_VLAN:
                            if (listener->listener.info.portVlanListener.vlan == vlan)
                            {
                                listenerList[listenerListIndex++] = listener->listener;
                            }
                            break;

                        case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:
                            vn = fmGetVN(sw, listener->listener.info.vnListener.vni);
                            if ( (vn != NULL) && (vn->descriptor.vlan == vlan) )
                            {
                                listenerList[listenerListIndex++] = listener->listener;
                            }
                            break;

                        case FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL:
                            /* Flow Tunnel are GLORT based */
                            break;

                        default:
                            err = FM_FAIL;
                            fmFree(listenerList);
                            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
                    }
                }

                for (i = 0; i < listenerListIndex; i++)
                {
                    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                 "Deleting mcastGrp=%d\n",
                                 group->handle);
                    FM_DBG_DUMP_LISTENER(&listenerList[i]);

                    err = fmDeleteMcastGroupListenerV2(sw,
                                                       group->handle,
                                                       &listenerList[i]);
                    FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_MULTICAST,
                                           err == FM_OK,
                                           fmFree(listenerList),
                                           "Failed to delete listener: (%d) %s\n",
                                           err, fmErrorMsg(err));
                }

                fmFree(listenerList);
            }
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmMcastDeleteVlanNotify */




/*****************************************************************************/
/** fmGetAvailableMulticastListenerCount
 * \ingroup intMulticast
 *
 * \desc            Determines how many unused multicast table entries remain.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      count points to caller-allocated storage where this
 *                  function should place the number of unused multicast
 *                  table entries.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetAvailableMulticastListenerCount(fm_int  sw,
                                               fm_int *count)
{
    fm_status      err;
    fm_switch *    switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST, "sw=%d \n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetAvailableMulticastListenerCount,
                       sw,
                       count);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fmGetAvailableMulticastListenerCount */




/*****************************************************************************/
/** fmSetMcastGroupAttributeInt
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a multicast group attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       attr is the multicast group attribute to set
 *                  (see 'Multicast Group Attributes').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support
 *                  multicast group attributes.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 * \return          FM_ERR_MCAST_GROUP_ALREADY_ASSIGNED if an attempt is
 *                  made to change the ''FM_MCASTGROUP_L2_SWITCHING_ONLY''
 *                  attribute when the multicast group already has a
 *                  multicast address.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the multicast group has
 *                  been activated and an attempt is made to change an
 *                  attribute that can only be modified when the group
 *                  is not active.
 *
 *****************************************************************************/
fm_status fmSetMcastGroupAttributeInt(fm_int sw,
                                      fm_int mcastGroup,
                                      fm_int attr,
                                      void * value)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_bool               lockTaken = FALSE;
    fm_intMulticastGroup *group;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, attr = %d, value = %p\n",
                 sw,
                 mcastGroup,
                 attr,
                 value);

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status == FM_OK)
    {
        lockTaken = TRUE;

        /* Find the multicast group */
        group = fmFindMcastGroup(sw, mcastGroup);

        if (group == NULL)
        {
            status = FM_ERR_INVALID_MULTICAST_GROUP;
        }

        else
        {
            FM_API_CALL_FAMILY(status,
                               switchPtr->SetMcastGroupAttribute,
                               sw,
                               group,
                               attr,
                               value);
        }
    }

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmSetMcastGroupAttributeInt */




/*****************************************************************************/
/** fmSetMcastGroupAttribute
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Set a multicast group attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       attr is the multicast group attribute to set
 *                  (see 'Multicast Group Attributes').
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support
 *                  multicast group attributes.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 * \return          FM_ERR_MCAST_GROUP_ALREADY_ASSIGNED if an attempt is
 *                  made to change the ''FM_MCASTGROUP_L2_SWITCHING_ONLY''
 *                  attribute when the multicast group already has a
 *                  multicast address.
 * \return          FM_ERR_MCAST_GROUP_ACTIVE if the multicast group has
 *                  been activated and an attempt is made to change an
 *                  attribute that can only be modified when the group
 *                  is not active.
 *
 *****************************************************************************/
fm_status fmSetMcastGroupAttribute(fm_int sw,
                                   fm_int mcastGroup,
                                   fm_int attr,
                                   void * value)
{
    fm_status status;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, attr = %d, value = %p\n",
                     sw,
                     mcastGroup,
                     attr,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    status = fmSetMcastGroupAttributeInt(sw, mcastGroup, attr, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmSetMcastGroupAttribute */




/*****************************************************************************/
/** fmGetMcastGroupAttributeInt
 * \ingroup intMulticast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve a multicast group attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       attr is the multicast group attribute to retrieve
 *                  (see 'Multicast Group Attributes').
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function is to place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support
 *                  multicast group attributes.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupAttributeInt(fm_int sw,
                                      fm_int mcastGroup,
                                      fm_int attr,
                                      void * value)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_bool               lockTaken = FALSE;
    fm_intMulticastGroup *group;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, attr = %d, value = %p\n",
                     sw,
                     mcastGroup,
                     attr,
                     value);

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status == FM_OK)
    {
        lockTaken = TRUE;

        /* Find the multicast group */
        group = fmFindMcastGroup(sw, mcastGroup);

        if (group == NULL)
        {
            status = FM_ERR_INVALID_MULTICAST_GROUP;
        }

        else
        {
            FM_API_CALL_FAMILY(status,
                               switchPtr->GetMcastGroupAttribute,
                               sw,
                               group,
                               attr,
                               value);
        }
    }

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupAttributeInt */




/*****************************************************************************/
/** fmGetMcastGroupAttribute
 * \ingroup multicast
 *
 * \chips           FM3000, FM4000, FM6000, FM10000
 *
 * \desc            Retrieve a multicast group attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       attr is the multicast group attribute to retrieve
 *                  (see 'Multicast Group Attributes').
 *
 * \param[out]      value points to caller-allocated storage where this
 *                  function is to place the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support
 *                  multicast group attributes.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupAttribute(fm_int sw,
                                   fm_int mcastGroup,
                                   fm_int attr,
                                   void * value)
{
    fm_status             status;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, attr = %d, value = %p\n",
                     sw,
                     mcastGroup,
                     attr,
                     value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    status = fmGetMcastGroupAttributeInt(sw, mcastGroup, attr, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupAttribute */




/*****************************************************************************/
/** fmMcastBuildMacEntry
 * \ingroup intMulticast
 *
 * \desc            Builds a mac entry for a Layer-2 multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       addr points to the L2 address entry for which a MAC
 *                  table entry is to be built.
 *
 * \param[out]      macEntry points to caller-provided storage into which
 *                  this function will write the MAC Address entry.
 *
 * \param[out]      trigger points to caller-provided storage into which
 *                  this function will place the required trigger id, or -1
 *                  if this group doesn't need a trigger.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmMcastBuildMacEntry(fm_int                  sw,
                               fm_intMulticastGroup *  group,
                               fm_multicastMacAddress *addr,
                               fm_macAddressEntry *    macEntry,
                               fm_int *                trigger)
{
    fm_switch *switchPtr;
    fm_status  status;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "sw = %d, "
                  "group = %p (handle=%d, port=%d), "
                  "addr = %p (addr=%012" FM_FORMAT_64 "X, vlan=%u, vlan2=%u), "
                  "macEntry = %p, "
                  "trigger = %p\n",
                  sw,
                  (void *) group,
                  group->handle,
                  group->logicalPort,
                  (void *) addr,
                  addr->destMacAddress,
                  addr->vlan,
                  addr->vlan2,
                  (void *) macEntry,
                  (void *) trigger );

    switchPtr = GET_SWITCH_PTR(sw);

    FM_MEMCPY_S( &macEntry->macAddress,
                 sizeof(macEntry->macAddress),
                 &addr->destMacAddress,
                 sizeof(addr->destMacAddress) );

    macEntry->vlanID   = addr->vlan;
    macEntry->vlanID2  = addr->vlan2;
    macEntry->type     = FM_ADDRESS_STATIC;
    macEntry->destMask = FM_DESTMASK_UNUSED;
    macEntry->port     = group->logicalPort;
    macEntry->age      = 0;
    macEntry->remoteID = 0;
    macEntry->remoteMac = 0;

    if ( (trigger != NULL) && (switchPtr->GetMcastGroupTrigger != NULL) )
    {
        status = switchPtr->GetMcastGroupTrigger(sw, group, trigger);

        if (status != FM_OK)
        {
            *trigger = -1;
            status   = FM_OK;
        }
    }
    else
    {
        status   = FM_OK;

        if (trigger != NULL)
        {
            *trigger = -1;
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "sw = %d, "
                 "macEntry = %p (addr=%012" FM_FORMAT_64 "X, vlan=%u, "
                 "type=%d, destMask=%X, port=%d), "
                 "trigger = %p (%d)\n",
                 sw,
                 (void *) macEntry,
                 macEntry->macAddress,
                 macEntry->vlanID,
                 macEntry->type,
                 macEntry->destMask,
                 macEntry->port,
                 (void *) trigger,
                 (trigger != NULL) ? *trigger : -1);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmMcastBuildMacEntry */




/*****************************************************************************/
/** fmRewriteMcastGroupMacAddresses
 * \ingroup intMcast
 *
 * \desc            Writes all mac address entries for a multicast group.
 *                  In general, this will be called when the hardware layer
 *                  has determined that the addresses need to be changed due
 *                  to some other change, such as adding the CPU port as a
 *                  forwarding port, which may cause a trigger to have to be
 *                  added to the MAC entries.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRewriteMcastGroupMacAddresses(fm_int                sw,
                                          fm_intMulticastGroup *group)
{
    fm_status             status;
    fm_customTreeIterator addrIter;
    fm_mcastAddrKey *     addrKey;
    fm_mcastAddrKey *     addrValue;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p\n",
                 sw,
                 (void *) group);

    /* No need to update the MAC table */
    if (group->updateHardware == FALSE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);
    }

    fmCustomTreeIterInit(&addrIter, &group->addressTree);

    while (1)
    {
        status = fmCustomTreeIterNext( &addrIter,
                                       (void **) &addrKey,
                                       (void **) &addrValue );

        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

        if (addrKey->addr.addressType == FM_MCAST_ADDR_TYPE_L2MAC_VLAN)
        {
            status = AddAddressToHardware(sw, group, addrKey);

            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmRewriteMcastGroupMacAddresses */




/*****************************************************************************/
/** fmSetMcastGroupRouteActiveFlags
 * \ingroup intMcast
 *
 * \desc            Sets the route-active flags for all L3 addresses
 *                  attached to a multicast group.
 *                  In general, this will be called when the hardware layer
 *                  has determined that the route states have changed and thus
 *                  the routes need to all be updated.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       group points to the multicast group entry.
 *
 * \param[in]       routeState contains the new route state to be applied
 *                  to the route.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetMcastGroupRouteActiveFlags(fm_int                sw,
                                          fm_intMulticastGroup *group,
                                          fm_routeState         routeState)
{
    fm_status             status;
    fm_customTreeIterator addrIter;
    fm_mcastAddrKey *     addrKey;
    fm_mcastAddrKey *     addrValue;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d, group = %p\n",
                 sw,
                 (void *) group);

    fmCustomTreeIterInit(&addrIter, &group->addressTree);

    while (1)
    {
        status = fmCustomTreeIterNext( &addrIter,
                                       (void **) &addrKey,
                                       (void **) &addrValue );

        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

        if (addrKey->routePtr != NULL)
        {
            addrKey->routePtr->state = routeState;

            status = fmSetRouteActiveFlag(sw, addrKey->routePtr, TRUE);

            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmSetMcastGroupRouteActiveFlags */




/*****************************************************************************/
/** fmGetMcastGroupUsedInt
 * \ingroup intMulticast
 *
 * \chips           FM4000, FM6000
 *
 * \desc            Determine if a multicast group has been used.  This only
 *                  works for IP-based multicast groups, since the hardware
 *                  supports the functionality using the ARP-Used table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[out]      used points to caller-allocated storage into which
 *                  this function will write TRUE if frames have been routed
 *                  for this multicast group, FALSE if they have not.
 *
 * \param[in]       resetFlag should be set to TRUE if the ARP-used bit for
 *                  this multicast group should be reset. Note that resetFlag
 *                  may not be set to TRUE when an ARP used snapshot cache
 *                  is in use, as specified in a prior call to
 *                  ''fmRefreshARPUsedCache''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if resetFlag is TRUE when an ARP
 *                  used cache is in use.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the
 *                  switch.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupUsedInt(fm_int   sw,
                                 fm_int   mcastGroup,
                                 fm_bool *used,
                                 fm_bool  resetFlag)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;
    fm_intEcmpGroup *     ecmpGroup;
    fm_customTreeIterator addrIter;
    fm_mcastAddrKey *     addrKey;
    fm_mcastAddrKey *     addrValue;
    fm_bool               foundRoute;
    fm_bool               tempUsed;
    fm_bool               anyUsed = FALSE;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "sw = %d, mcastGroup = %d, used = %p, resetFlag = %d\n",
                 sw,
                 mcastGroup,
                 (void *) used,
                 resetFlag );

    switchPtr = GET_SWITCH_PTR(sw);

    /* Find the multicast group */
    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
    }
    else if (switchPtr->GetMcastGroupUsed != NULL)
    {
        status = switchPtr->GetMcastGroupUsed(sw, group, used, resetFlag);
    }
    else
    {
        foundRoute = FALSE;

        fmCustomTreeIterInit(&addrIter, &group->addressTree);

        while (1)
        {
            status = fmCustomTreeIterNext( &addrIter,
                                           (void **) &addrKey,
                                           (void **) &addrValue );

            if (status == FM_ERR_NO_MORE)
            {
                status = FM_OK;
                break;
            }

            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

            if (addrKey->routePtr != NULL)
            {
                foundRoute = TRUE;

                ecmpGroup = switchPtr->ecmpGroups[addrKey->routePtr->ecmpGroupId];
                tempUsed  = FALSE;

                FM_API_CALL_FAMILY(status,
                                   switchPtr->GetECMPGroupARPUsed,
                                   sw,
                                   ecmpGroup,
                                   &tempUsed,
                                   resetFlag);

                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

                if (tempUsed)
                {
                    anyUsed = TRUE;
                }
            }
        }

        if (!foundRoute)
        {
            status = FM_ERR_UNSUPPORTED;
        }

        *used = anyUsed;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupUsedInt */




/*****************************************************************************/
/** fmGetMcastGroupUsed
 * \ingroup multicast
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Determine if a multicast group has been used.  This only
 *                  works for IP-based multicast groups, since the hardware
 *                  supports the functionality using the ARP-Used table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[out]      used points to caller-allocated storage into which
 *                  this function will write TRUE if frames have been routed
 *                  for this multicast group, FALSE if they have not.
 *
 * \param[in]       resetFlag should be set to TRUE if the ARP-used bit for
 *                  this multicast group should be reset. Note that resetFlag
 *                  may not be set to TRUE when an ARP used snapshot cache
 *                  is in use, as specified in a prior call to
 *                  ''fmRefreshARPUsedCache''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if resetFlag is TRUE when an ARP
 *                  used cache is in use.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the
 *                  switch.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupUsed(fm_int   sw,
                              fm_int   mcastGroup,
                              fm_bool *used,
                              fm_bool  resetFlag)
{
    fm_status  status;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, used = %p, resetFlag = %d\n",
                     sw,
                     mcastGroup,
                     (void *) used,
                     resetFlag );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status == FM_OK)
    {
        status = fmGetMcastGroupUsedInt(sw, mcastGroup, used, resetFlag);

        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupUsed */




/*****************************************************************************/
/** fmGetMcastGroupPortInt
 * \ingroup intMulticast
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns the multicast group's logical port, if one has
 *                  been assigned.  If a logical port has not yet been
 *                  allocated for the group, an error is returned.
 *
 * \note            For non-stacking multicast groups, logical ports are only
 *                  allocated when the group is activated and are freed when
 *                  the group is deactivated. Thus, an application cannot
 *                  retrieve the logical port for a non-stacking, unactivated
 *                  multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[out]      logPort points to caller-allocated storage into which
 *                  this function will write the logical port value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_INVALID_PORT if a logical port has not been
 *                  allocated for the multicast group.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupPortInt(fm_int  sw,
                                 fm_int  mcastGroup,
                                 fm_int *logPort)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, logPort = %p\n",
                     sw,
                     mcastGroup,
                     (void *) logPort);

    if (logPort == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status == FM_OK)
    {
        /* Find the multicast group */
        group = fmFindMcastGroup(sw, mcastGroup);

        if (group == NULL)
        {
            status = FM_ERR_INVALID_MULTICAST_GROUP;
        }
        else if (group->logicalPort == FM_LOGICAL_PORT_NONE)
        {
            status = FM_ERR_INVALID_PORT;
        }
        else
        {
            status   = FM_OK;
            *logPort = group->logicalPort;
        }

        fmReleaseReadLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupPortInt */



/*****************************************************************************/
/** fmGetMcastGroupPort
 * \ingroup multicast
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns the multicast group's logical port, if one has
 *                  been assigned.  If a logical port has not yet been
 *                  allocated for the group, an error is returned.
 *
 * \note            For non-stacking multicast groups, logical ports are only
 *                  allocated when the group is activated and are freed when
 *                  the group is deactivated. Thus, an application cannot
 *                  retrieve the logical port for a non-stacking, unactivated
 *                  multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[out]      logPort points to caller-allocated storage into which
 *                  this function will write the logical port value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_INVALID_PORT if a logical port has not been
 *                  allocated for the multicast group.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupPort(fm_int  sw,
                              fm_int  mcastGroup,
                              fm_int *logPort)
{
    fm_status             status;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, logPort = %p\n",
                     sw,
                     mcastGroup,
                     (void *) logPort);

    if (logPort == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    status = fmGetMcastGroupPortInt(sw, mcastGroup, logPort);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupPort */



/*****************************************************************************/
/** fmAddMcastGroupEcmp
 * \ingroup multicast
 *
 * \chips           FM6000
 *
 * \desc            Add an ECMP group to a multicast group. The ECMP group
 *                  will contain a single NextHop entry that carries a
 *                  VLAN ID used for loopback suppression in the replication
 *                  table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       lbsVlan is the VLAN ID to add in the ECMP group that
 *                  will be used for loopback suppression in the replication
 *                  table.
 *
 * \param[out]      ecmpId points to caller-allocated storage into which
 *                  this function will write the ECMP ID (handle).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_VLAN if lbsVlan is invalid.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 *
 *****************************************************************************/
fm_status fmAddMcastGroupEcmp(fm_int  sw,
                              fm_int  mcastGroup,
                              fm_int  lbsVlan,
                              fm_int *ecmpId)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;
    fm_ecmpGroupInfo      ecmpInfo;
    fm_bool               lockTaken = FALSE;
    fm_uint64             key;
    fm_intMulticastEcmp * ecmpGroup;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, vlan = %d, ecmpId = %p\n",
                     sw,
                     mcastGroup,
                     lbsVlan,
                     (void *) ecmpId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate loopback suppress vlan ID */
    if ( (lbsVlan < 0) || (lbsVlan > 4095) )
    {
        status =  FM_ERR_INVALID_VLAN;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    FM_CLEAR(ecmpInfo);
    ecmpInfo.wideNextHops = FALSE;
    ecmpInfo.numFixedEntries = 0;
    ecmpInfo.lbsVlan = lbsVlan;

    status = fmCreateECMPGroupInternal(sw,
                                       ecmpId,
                                       &ecmpInfo,
                                       group);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    ecmpGroup = (fm_intMulticastEcmp *) fmAlloc( sizeof(fm_intMulticastEcmp) );

    if (ecmpGroup == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    memset(ecmpGroup, 0, sizeof(fm_intMulticastEcmp));

    ecmpGroup->vlan   = lbsVlan;
    ecmpGroup->ecmpId = *ecmpId;

    key = (fm_uint64) (*ecmpId);
    /* Add ECMP to multicast group. */
    status = fmTreeInsert(&group->ecmpTree,
                          key,
                          (void *) ecmpGroup);

    if (status != FM_OK)
    {
        fmFree(ecmpGroup);
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmAddMcastGroupEcmp */




/*****************************************************************************/
/** fmDeleteMcastGroupEcmp
 * \ingroup multicast
 *
 * \chips           FM6000
 *
 * \desc            Delete an ECMP group from a multicast group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       ecmpId is the ECMP ID (handle) to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 *
 *****************************************************************************/
fm_status fmDeleteMcastGroupEcmp(fm_int sw,
                                 fm_int mcastGroup,
                                 fm_int ecmpId)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;
    fm_bool               lockTaken = FALSE;
    fm_uint64             key;
    fm_intMulticastEcmp * ecmpGroup;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, ecmpId = %d\n",
                     sw,
                     mcastGroup,
                     ecmpId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    status = fmDeleteECMPGroupInternal(sw, ecmpId);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    key = (fm_uint64) (ecmpId);
    /* Search the tree of ECMP for a matching entry. */
    status = fmTreeFind(&group->ecmpTree,
                        key,
                        (void **) &ecmpGroup);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* Remove ECMP from ECMP tree. */
    status = fmTreeRemove(&group->ecmpTree,
                          key,
                          NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    /* Free ECMP object. */
    fmFree(ecmpGroup);

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmDeleteMcastGroupEcmp */




/*****************************************************************************/
/** fmGetMcastGroupHwIndex
 * \ingroup multicast
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns the multicast hardware index for a multicast group.
 *                  This value is used by applications which have a need to
 *                  format wide next-hop records themselves, as in with NAT,
 *                  instead of allowing the API to format the records on their
 *                  behalf.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[out]      hwIndex points to caller-provided storage into which the
 *                  hardware index value will be written. If the multicast
 *                  group is not using any multicast hardware, the value
 *                  returned will be -1.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if hwIndex is NULL.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if this switch does not support this
 *                  request.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fmGetMcastGroupHwIndex(fm_int  sw,
                                 fm_int  mcastGroup,
                                 fm_int *hwIndex)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;
    fm_bool               lockTaken;

    FM_LOG_ENTRY_API( FM_LOG_CAT_MULTICAST,
                      "sw = %d, mcastGroup = %d, hwIndex = %p\n",
                      sw,
                      mcastGroup,
                      (void *) hwIndex );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if (hwIndex == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    lockTaken = TRUE;

    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    if (!group->activated)
    {
        status = FM_ERR_MCAST_GROUP_NOT_ACTIVE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    FM_API_CALL_FAMILY(status, switchPtr->GetMcastGroupHwIndex, sw, group, hwIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmGetMcastGroupHwIndex */




/*****************************************************************************/
/** fmConfigureMcastGroupAsFlooding
 * \ingroup intMulticast
 *
 * \desc            Configure mcast group as HNI flooding. These groups are
 *                  requested by HNI and have all multicast flooding ports
 *                  added as listeners. When disabling, all listeners
 *                  with flooding port will be deleted. This functionality 
 *                  is to allow filtering multicast flooded frames to
 *                  virtual ports.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \param[in]       isHNIFlooding flag should be set to TRUE to configure
 *                  multicast group as HNI flooding. To configure it
 *                  back, set it to FALSE.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if mcastGroup is invalid.
 * \return          FM_ERR_UNSUPPORTED if this switch does not support this
 *                  request.
 * \return          FM_ERR_MCAST_GROUP_NOT_ACTIVE if the group is not active.
 *
 *****************************************************************************/
fm_status fmConfigureMcastGroupAsHNIFlooding(fm_int  sw,
                                             fm_int  mcastGroup,
                                             fm_bool isHNIFlooding)
{
    fm_status             status;
    fm_switch *           switchPtr;
    fm_intMulticastGroup *group;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, mcastGroup = %d, isHNIFlooding = %d\n",
                     sw,
                     mcastGroup,
                     isHNIFlooding);

    switchPtr        = GET_SWITCH_PTR(sw);
    status           = FM_OK;

    group = fmFindMcastGroup(sw, mcastGroup);
    if (group == NULL)
    {
        status = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->ConfigureMcastGroupAsHNIFlooding,
                       sw,
                       mcastGroup,
                       isHNIFlooding);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);

    group->isHNIFlooding = isHNIFlooding;

ABORT:

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmConfigureMcastGroupAsHNIFlooding */




/*****************************************************************************/
/** fmIsMcastGroupHNIFlooding
 * \ingroup intMulticast
 *
 * \desc            Helper function to check whether a multicast group is
 *                  a flood mcast group requested by HNI.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \return          TRUE if mcast group is configured as requested by HNI.
 * \return          FALSE if otherwise
 *
 *****************************************************************************/
fm_bool fmIsMcastGroupHNIFlooding(fm_int sw,
                                  fm_int mcastGroup)
{
    fm_intMulticastGroup *group;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        return FALSE;
    }

    return group->isHNIFlooding;

}   /* end fmIsMcastGroupHNIFlooding */




/*****************************************************************************/
/** fmHasMcastGroupNonVirtualListeners
 * \ingroup intMulticast
 *
 * \desc            Helper function to check whether a multicast group has
 *                  listeners for ports that are not virtual. Flood
 *                  listeners should not be checked.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \return          TRUE if mcast group has listeners for ports
 *                  that are not virtual.
 * \return          FALSE if otherwise
 *
 *****************************************************************************/
fm_bool fmHasMcastGroupNonVirtualListeners(fm_int sw,
                                           fm_int mcastGroup)
{
    fm_status                status;
    fm_port *                portPtr;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    status = FM_OK;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        return FALSE;
    }

    fmTreeIterInit(&iter, &group->listenerTree);

    status = fmTreeIterNext( &iter, &key, (void **) &intListener);

    while (status != FM_ERR_NO_MORE)
    {
        if ( (status == FM_OK) && (!intListener->floodListener) )
        {
            if ( (intListener->listener.listenerType == FM_MCAST_GROUP_LISTENER_VN_TUNNEL) ||
                 (intListener->listener.listenerType == FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL) )
            {
                return TRUE;
            }
            else
            {
                portPtr = GET_PORT_PTR(sw, intListener->listener.info.portVlanListener.port);

                if ( (portPtr != NULL) &&
                     (portPtr->portType != FM_PORT_TYPE_VIRTUAL) )
                {
                    return TRUE;
                }
            }
        }
        else if ( (status != FM_OK) && ( status != FM_ERR_NO_MORE) )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Check listeners for mcastGroup = %d"
                         " failed with err = %d\n",
                         mcastGroup,
                         status);
            break;
        }

        status = fmTreeIterNext( &iter, &key, (void **) &intListener);
    }

    return FALSE;

}   /* end fmHasMcastGroupNonVirtualListeners */




/*****************************************************************************/
/** fmHasMcastGroupVirtualListeners
 * \ingroup intMulticast
 *
 * \desc            Helper function to check whether a multicast group has
 *                  listeners for virtual ports.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \return          TRUE if mcast group has listeners for ports
 *                  that are not virtual.
 * \return          FALSE if otherwise
 *
 *****************************************************************************/
fm_bool fmHasMcastGroupVirtualListeners(fm_int sw,
                                        fm_int mcastGroup)
{
    fm_status             status;
    fm_port *             portPtr;
    fm_multicastListener  listener;
    fm_multicastListener  nextListener; 

    status = fmGetMcastGroupListenerFirst(sw,
                                          mcastGroup,
                                          &listener);

    while (status != FM_ERR_NO_MORE)
    {
        if (status == FM_OK)
        {
            portPtr = GET_PORT_PTR(sw, listener.port);

            if ( (portPtr != NULL) &&
                 (portPtr->portType == FM_PORT_TYPE_VIRTUAL) )
            {
                return TRUE;
            }

            status = fmGetMcastGroupListenerNext(sw,
                                                 mcastGroup,
                                                 &listener,
                                                 &nextListener);

            if (status == FM_OK)
            {
                listener = nextListener;
            }
            else if (status != FM_ERR_NO_MORE)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Check listeners for mcastGroup = %d"
                             " failed with err = %d\n",
                             mcastGroup,
                             status);
                break;
            }
        }
        else if (status != FM_ERR_NO_MORE)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Check listeners for mcastGroup = %d"
                         " failed with err = %d\n",
                         mcastGroup,
                         status);
            break;
        }
    }

    return FALSE;

}   /* end fmHasMcastGroupVirtualListeners */




/*****************************************************************************/
/** fmHasMcastGroupNonFloodingListeners
 * \ingroup intMulticast
 *
 * \desc            Helper function to check whether a multicast group has
 *                  non-flooding listeners.
 *
 * \note            This function assumes that the caller has taken the
 *                  appropriate lock and made sure switch state is valid.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       mcastGroup is the multicast group ID number.
 *
 * \return          TRUE if mcast group has listeners for ports
 *                  that are not virtual.
 * \return          FALSE if otherwise
 *
 *****************************************************************************/
fm_bool fmHasMcastGroupNonFloodingListeners(fm_int sw,
                                            fm_int mcastGroup)
{
    fm_status                status;
    fm_intMulticastGroup *   group;
    fm_intMulticastListener *intListener;
    fm_treeIterator          iter;
    fm_uint64                key;

    status = FM_OK;

    group = fmFindMcastGroup(sw, mcastGroup);

    if (group == NULL)
    {
        return FALSE;
    }

    fmTreeIterInit(&iter, &group->listenerTree);

    status = fmTreeIterNext( &iter, &key, (void **) &intListener);

    while (status != FM_ERR_NO_MORE)
    {
        if ( (status == FM_OK) && (!intListener->floodListener) )
        {
            return TRUE;
        }
        else if ( (status != FM_OK) && ( status != FM_ERR_NO_MORE) )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Check listeners for mcastGroup = %d"
                         " failed with err = %d\n",
                         mcastGroup,
                         status);
            break;
        }

        status = fmTreeIterNext( &iter, &key, (void **) &intListener);
    }

    return FALSE;

}   /* end fmHasMcastGroupNonFloodingListeners */




/*****************************************************************************/
/** fmUpdateMcastHNIFloodingGroups
 * \ingroup intMulticast
 *
 * \desc            Updates mcast groups configured as HNI flooding.
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
fm_status fmUpdateMcastHNIFloodingGroups(fm_int  sw,
                                         fm_int  port,
                                         fm_bool state)
{
    fm_status             status;
    fm_mcastGroupListener tempListener;
    fm_int                mcastGroupNumber;
    fm_int                currentMcastGroupNumber;

    FM_LOG_ENTRY_API(FM_LOG_CAT_MULTICAST,
                     "sw = %d, port=%d, state=%d\n",
                     sw,
                     port,
                     state);

    status = fmGetMcastGroupFirst(sw, &mcastGroupNumber);

    while (status != FM_ERR_NO_MORE)
    {
        if ( fmIsMcastGroupHNIFlooding(sw, mcastGroupNumber) )
        {
            /* Add new flood listener */
            FM_CLEAR(tempListener);

            tempListener.listenerType = FM_MCAST_GROUP_LISTENER_PORT_VLAN;
            tempListener.info.portVlanListener.vlan =
                         FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS;
            tempListener.info.portVlanListener.port = port;

            status = UpdateMcastHNIFloodingGroups(sw,
                                                  mcastGroupNumber,
                                                  &tempListener,
                                                  state);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, status);
        }

        currentMcastGroupNumber = mcastGroupNumber;

        status = fmGetMcastGroupNext(sw,
                                     currentMcastGroupNumber,
                                     &mcastGroupNumber);
    }

    status = FM_OK;

ABORT:

    FM_LOG_EXIT_API(FM_LOG_CAT_MULTICAST, status);

}   /* end fmUpdateMcastHNIFloodingGroups */
