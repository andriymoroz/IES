/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_replication.c 
 * Creation Date:   Mar 5, 2014  
 * Description:     Contains FM10000 specific implementations for the
 *                  replication API.
 *
 * Copyright (c) 2007 - 2014, Intel Corporation
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


static fm_status MoveListenersToReplicationGroup(fm_int                sw, 
                                                 fm_int                repliGroup,
                                                 fm_intMulticastGroup *mcastGroup);

static fm_status RemoveListenersFromReplicationGroup(fm_int                sw,
                                                     fm_int                repliGroup, 
                                                     fm_intMulticastGroup *mcastGroup);



/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** MoveListenersToReplicationGroup
 * \ingroup intMulticast
 *
 * \desc            Move a group of listeners to a replication group.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       mcastGroup points to the multicast group entry,
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if a listener port is invalid
 *
 *****************************************************************************/
static fm_status MoveListenersToReplicationGroup(fm_int                sw, 
                                                 fm_int                repliGroup,
                                                 fm_intMulticastGroup *mcastGroup)
{ 
    fm_status                err;
    fm_switch *              switchPtr;
    fm_int                   mcastLogicalPort;
    fm_intMulticastListener *intListener;
    fm_int                   listenerCount;
    fm_int                   i;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm10000_mtableEntry      listener;
    fm_port                 *portPtr;
    fm_int                   port;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "sw=%d repliGroup=%d mcastLogPort=%d\n",
                 sw, repliGroup, mcastGroup->logicalPort);

    switchPtr        = GET_SWITCH_PTR(sw);
    err              = FM_OK;
    listenerCount    = 0;
    mcastLogicalPort = mcastGroup->logicalPort;

    /* Get the number of listeners to move */
    listenerCount = (fm_int) fmTreeSize(&mcastGroup->listenerTree);

    if (listenerCount > 0)
    {
        i = 0;
        fmTreeIterInit(&iter, &mcastGroup->listenerTree);

        while (1)
        {
            err = fmTreeIterNext(&iter, &key, (void **) &intListener);

            if (err != FM_OK)
            {
                if (err != FM_ERR_NO_MORE)
                {
                    FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
                }

                err = FM_OK;
                break;
            }

            if (intListener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
            {
                continue;
            }

            if (i++ >= listenerCount)
            {
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }

            listener.vlan       = intListener->listener.info.portVlanListener.vlan;
            listener.vlanUpdate = TRUE;

            if ( (intListener->listener.info.portVlanListener.vlan == 0)
                 && (intListener->listener.info.portVlanListener.port == -1) )
            {
                port    = switchPtr->cpuPort;
                portPtr = NULL;
                
            }
            else
            {
                port = intListener->listener.info.portVlanListener.port;
                portPtr = GET_PORT_PTR(sw, port);

                if (!portPtr)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST,
                                        FM_ERR_INVALID_PORT);
                }
            }

            if ( (portPtr != NULL) && (portPtr->portType == FM_PORT_TYPE_VIRTUAL) )
            {
                err = fm10000MapVsiGlortToLogicalPort(sw,
                                                      portPtr->glort,
                                                      &listener.port);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                
                listener.dglortUpdate = TRUE;
                listener.dglort       = portPtr->glort;

               if (intListener->listener.info.portVlanListener.vlan == 0)
               {
                   listener.vlanUpdate = FALSE;
               }

            }
            else
            {
                listener.port         = port;
                listener.dglortUpdate = FALSE;
                listener.dglort =     0;
            }
           
            err = fm10000MTableAddListener(sw, 
                                           mcastLogicalPort, 
                                           repliGroup, 
                                           listener);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }

        /* Sanity check */
        if (i != listenerCount)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end MoveListenersToReplicationGroup */




/*****************************************************************************/
/** RemoveListenersFromReplicationGroup
 * \ingroup intMulticast
 *
 * \desc            Remove a group of listeners from a replication group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       mcastGroup points to the multicast group entry,
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if a listener port is invalid
 *
 *****************************************************************************/
static fm_status RemoveListenersFromReplicationGroup(fm_int                sw,
                                                     fm_int                repliGroup, 
                                                     fm_intMulticastGroup *mcastGroup)
{ 
    fm_status                err;
    fm_switch *              switchPtr;
    fm_int                   logicalPort;
    fm_intMulticastListener *intListener;
    fm10000_mtableEntry      mcastListener;
    fm_int                   listenerCount;
    fm_int                   i;
    fm_treeIterator          iter;
    fm_uint64                key;
    fm_port                 *portPtr;
    fm_int                   port;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "sw=%d repliGroup=%d mcastLogPort=%d\n",
                 sw, repliGroup, mcastGroup->logicalPort);

    switchPtr        = GET_SWITCH_PTR(sw);
    err              = FM_OK;
    listenerCount    = 0;
    logicalPort      = mcastGroup->logicalPort;

    /* Get the number of listeners to move */
    listenerCount = (fm_int) fmTreeSize(&mcastGroup->listenerTree);

    if (listenerCount > 0)
    {
        i = 0;
        fmTreeIterInit(&iter, &mcastGroup->listenerTree);

        while (1)
        {
            err = fmTreeIterNext(&iter, &key, (void **) &intListener);

            if (err != FM_OK)
            {
                if (err != FM_ERR_NO_MORE)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                }

                err = FM_OK;
                break;
            }

            if (intListener->listener.listenerType != FM_MCAST_GROUP_LISTENER_PORT_VLAN)
            {
                continue;
            }

            if (i++ >= listenerCount)
            {
                err = FM_FAIL;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }

            mcastListener.vlan       = intListener->listener.info.portVlanListener.vlan;
            mcastListener.vlanUpdate = TRUE;

            if ( (intListener->listener.info.portVlanListener.vlan == 0)
                 && (intListener->listener.info.portVlanListener.port == -1) )
            {
                port    = switchPtr->cpuPort;
                portPtr = NULL;
                
            }
            else
            {
                port = intListener->listener.info.portVlanListener.port;
                portPtr = GET_PORT_PTR(sw, port);

                if (!portPtr)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST,
                                        FM_ERR_INVALID_PORT);
                }
            }

            if ( (portPtr != NULL) && (portPtr->portType == FM_PORT_TYPE_VIRTUAL) )
            {
                err = fm10000MapVsiGlortToLogicalPort(sw,
                                                      portPtr->glort,
                                                      &mcastListener.port);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                
                mcastListener.dglortUpdate = TRUE;
                mcastListener.dglort       = portPtr->glort;
                
                if (intListener->listener.info.portVlanListener.vlan == 0)
                {
                    mcastListener.vlanUpdate = FALSE;
                }
            }
            else
            {
                mcastListener.port         = port;
                mcastListener.dglortUpdate = FALSE;
                mcastListener.dglort       = 0;
            }

            err = fm10000MTableDeleteListener(sw, 
                                             logicalPort, 
                                             repliGroup, 
                                             mcastListener,
                                             FALSE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }

        /* Sanity check */
        if (i != listenerCount)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end RemoveListenersFromReplicationGroup */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000ReserveReplicationGroupMcastIndex
 * \ingroup intMulticast
 *
 * \desc            Reserve a Replication group's multicast Index.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       group is the replication group number.
 *  
 * \param[in]       mcastLogPort is the logical port of the multicast group.
 *
 * \param[out]      mcastIndex points to caller-allocated storage where this
 *                  function should place the Replication group's mcastIndex.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ReserveReplicationGroupMcastIndex(fm_int  sw, 
                                                  fm_int  group,
                                                  fm_int  mcastLogPort, 
                                                  fm_int *mcastIndex)
{
    fm_status err;
    fm_int    mtableDestIndex;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "sw=%d repliGroup=%d mcastLogPort=%d mcastIndex=%p\n",
                 sw, group, mcastLogPort, (void *) mcastIndex);

    err = fm10000MTableReserveEntry(sw, group, mcastLogPort, &mtableDestIndex);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    *mcastIndex = mtableDestIndex;

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fm10000ReserveReplicationGroupMcastIndex */




/*****************************************************************************/
/** fm10000ReleaseReplicationGroupMcastIndex
 * \ingroup intMulticast
 *
 * \desc            Release a Replication group's multicast Index.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ReleaseReplicationGroupMcastIndex(fm_int sw,
                                                  fm_int repliGroup)
{
    fm_status err;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST, "sw=%d repliGroup=%d\n", sw, repliGroup);

    /* Free previous replication group's MCAST_DEST_TABLE entry */
    err = fm10000MTableFreeDestTableEntry(sw, repliGroup);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fm10000ReleaseReplicationGroupMcastIndex */




/*****************************************************************************/
/** fm10000MoveReplicationGroupMcastGroup
 * \ingroup intMulticast
 *
 * \desc            Move the mcast group to a given replication group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       groupHandle is the new replication group handle.
 *
 * \param[in]       mcastGroup is the mcast group number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_MULTICAST_GROUP if the replication group
 *                  is invalid.
 * \return          FM_ERR_INVALID_PORT if a listener port is invalid
 *
 *****************************************************************************/
fm_status fm10000MoveReplicationGroupMcastGroup(fm_int sw, 
                                               fm_int groupHandle,
                                               fm_int mcastGroup)
{
    fm_status               err=FM_OK;
    fm_switch *             switchPtr;
    fm_intReplicationGroup *repliGroup;
    fm_intReplicationGroup *prevGroup;
    fm_intMulticastGroup *  mcastGroupPtr;
    fm_intMulticastGroup *  mcastGroupTmp;
    fm10000_MulticastGroup *mcastGroupExt;
    fm_int                  groupDestIndex;
    fm_int                  newGroupId=FM_MCASTGROUP_REPLICATION_GROUP_DISABLE;
    fm_int                  mtableDestIndex;
    fm_int                  privateGroup=FALSE;
    fm_bool                 removeFromPreviousTree=FALSE;
    fm_treeIterator         iter;
    fm_uint64               key;
    fm_intMulticastEcmp *   ecmpGroup;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "sw=%d newrepliGroup=%d mcastGroup=%d\n",
                 sw,
                 groupHandle,
                 mcastGroup);

    switchPtr = GET_SWITCH_PTR(sw);

    mcastGroupPtr = fmFindMcastGroup(sw, mcastGroup);

    if ( mcastGroupPtr == NULL )
    {
        err = FM_ERR_INVALID_MULTICAST_GROUP;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    mcastGroupExt  = mcastGroupPtr->extension;
    groupDestIndex = -1;

    /* If the given new replication group is -1, i.e. disable, then we
     * need to allocate a private replication group for this mcast group
     * if the latter is active */

    if ( groupHandle == FM_MCASTGROUP_REPLICATION_GROUP_DISABLE )
    {
        if (mcastGroupPtr->activated)
        {
            privateGroup = TRUE;
    
            err = fmCreateReplicationGroupInt(sw, 
                                              &newGroupId, 
                                              mcastGroupPtr->logicalPort, 
                                              &mtableDestIndex,
                                              TRUE);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST, 
                         "newRepliGroup %d DestIndex=%d\n", 
                         newGroupId, mtableDestIndex);
        }
    }
    else
    {
        privateGroup = FALSE;
        newGroupId = groupHandle;
    }

    if (newGroupId != FM_MCASTGROUP_REPLICATION_GROUP_DISABLE)
    {
        /* We have to move the mcast group to a new replication group */

        repliGroup = findReplicationGroup(sw, newGroupId);

        if (repliGroup == NULL)
        {
            err = FM_ERR_INVALID_MULTICAST_GROUP;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    
        /* Search the tree of mcast groups for a matching entry. */
        err = fmTreeFind( &repliGroup->mcastGroupList,
                         (fm_uint64) mcastGroup,
                         (void **) &mcastGroupTmp );
    
        if (err == FM_OK)
        {
            /* Error if already in group */
            err = FM_ERR_ALREADY_EXISTS;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }

        groupDestIndex = repliGroup->hwDestIndex;
    
        if (mcastGroupPtr->activated)
        {
            /* This mcast group is active; move all its listeners to the
             * new replication group. */
    
            /* Update the multicast index attribute for this mcast group */
            err = switchPtr->SetLogicalPortAttribute(sw,
                                                     mcastGroupPtr->logicalPort,
                                                     FM_LPORT_MULTICAST_INDEX,
                                                     &groupDestIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            if (!mcastGroupPtr->readOnlyRepliGroup)
            {
                err = MoveListenersToReplicationGroup(sw, newGroupId, mcastGroupPtr);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }
            /* Update the NextHop entry to use the new replicate group. */
            mcastGroupExt->mtableDestIndex = groupDestIndex;
            err = fm10000UpdateNextHopMulticast(sw,
                                               mcastGroupPtr->ecmpGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    
            fmTreeIterInit(&iter, &mcastGroupPtr->ecmpTree);
            while (1)
            {
                err = fmTreeIterNext(&iter,
                                     &key,
                                     (void **) &ecmpGroup);
                if (err != FM_OK)
                {
                    if (err == FM_ERR_NO_MORE)
                    {
                        err = FM_OK;
                    }

                    break;
                }

                err = fm10000UpdateNextHopMulticast(sw,
                                                   ecmpGroup->ecmpId);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            /* Remove the mcastgroup's listeners from previous replication group */
            if (!mcastGroupPtr->readOnlyRepliGroup)
            {
                err = RemoveListenersFromReplicationGroup(sw, 
                                                      mcastGroupPtr->repliGroup, 
                                                      mcastGroupPtr);
                FM_LOG_ABORT_ON_ERR ( FM_LOG_CAT_MULTICAST, err );
            }

            /* Release the previous replication group if it was a private one */
            if (mcastGroupPtr->privateGroup)
            {
                /* No need to remove it from previous group Tree since
                 * the group will be deleted or freed */
                removeFromPreviousTree = FALSE;

                err =
                    fmDeleteReplicationGroupInt(sw,
                                                mcastGroupPtr->repliGroup,
                                                TRUE);
                FM_LOG_ABORT_ON_ERR ( FM_LOG_CAT_MULTICAST, err );
            }
            else
            {
                /* Remove mcast group from the list of the previous
                 * replication group */
                removeFromPreviousTree = TRUE;
            }
        }
        else if (mcastGroupPtr->repliGroup != 
                 FM_MCASTGROUP_REPLICATION_GROUP_DISABLE)
        {
            /* Remove mcast group from the list of the previous
             * replication group */
            removeFromPreviousTree = TRUE;
        }

        /* Insert the mcast group in the list of the new replication group */
        err = fmTreeInsert(&repliGroup->mcastGroupList,
                           (fm_uint64) mcastGroup,
                           (void *) mcastGroupPtr);
    
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    }
    else if (mcastGroupPtr->repliGroup != FM_MCASTGROUP_REPLICATION_GROUP_DISABLE)
    {
        /* Removing a none active mcast group from its replication group */
        privateGroup = FALSE;
        groupDestIndex = -1;
        removeFromPreviousTree = TRUE;
    }

    if (removeFromPreviousTree)
    {
        /* Remove mcast group from the list of the previous replication group */

        prevGroup = findReplicationGroup(sw, mcastGroupPtr->repliGroup);

        if (prevGroup == NULL)
        {
            err = FM_ERR_INVALID_MULTICAST_GROUP;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }

        err = fmTreeRemove(&prevGroup->mcastGroupList,
                           (fm_uint64) mcastGroup,
                           NULL );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "mcast group %d deleted from Replication group %d\n",
                     mcastGroup, prevGroup->handle);
    }

    /* Update mcast group with info from the new replication group */
    mcastGroupPtr->repliGroup      = newGroupId;
    mcastGroupPtr->privateGroup    = privateGroup;
    mcastGroupExt->mtableDestIndex = groupDestIndex;

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "mcastgroup %d moved to Replication group %d, index %d\n",
                 mcastGroup, mcastGroupPtr->repliGroup, groupDestIndex);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fm10000MoveReplicationGroupMcastGroup */

