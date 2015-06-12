/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm_api_mcast_groups_int.h
 * Creation Date: October 8, 2007
 * Description: internal header file for multicast group services.
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

#ifndef __FM_FM_API_MCAST_GROUPS_INT_H
#define __FM_FM_API_MCAST_GROUPS_INT_H


#define FM_DBG_DUMP_LISTENER(x)                                                 \
    if ( (x) != NULL )                                                          \
    {                                                                           \
        switch ( (x)->listenerType )                                            \
        {                                                                       \
            case FM_MCAST_GROUP_LISTENER_PORT_VLAN:                             \
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,                              \
                             "    listener = %p, PORT_VLAN type: "              \
                             "vlan = %d, port = %d)\n",                         \
                             (void *) x,                                        \
                             (int) (x)->info.portVlanListener.vlan,             \
                             (x)->info.portVlanListener.port);                  \
                break;                                                          \
                                                                                \
            case FM_MCAST_GROUP_LISTENER_VN_TUNNEL:                             \
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,                              \
                             "    listener = %p, VN_TUNNEL type: "              \
                             "tunnelId = %d, vni = %u)\n",                      \
                             (void *) x,                                        \
                             (x)->info.vnListener.tunnelId,                     \
                             (x)->info.vnListener.vni);                         \
                break;                                                          \
                                                                                \
            default:                                                            \
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,                              \
                             "    listener = %p, UNKNOWN TYPE!\n",              \
                             (void *) x);                                       \
                break;                                                          \
        }                                                                       \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST, "listener pointer is NULL\n");       \
    }                                                                           \




typedef struct _fm_mcastAddrKey
{
    fm_vlanLearningMode     vlMode;
    fm_multicastAddress     addr;
    fm_intRouteEntry *      routePtr;

} fm_mcastAddrKey;


/***************************************************
 * Since groups are either in L2 or L3 mode, this
 * enum identifies which.
 **************************************************/
typedef enum _fm_mtableGroupType
{
    FM_MULTICAST_GROUP_TYPE_L2,
    FM_MULTICAST_GROUP_TYPE_L3

} fm_mtableGroupType;


/*****************************************************************************
 *
 * Multicast Group Entry
 *
 *****************************************************************************/
typedef struct _fm_intMulticastGroup
{
    /* The multicast group handle, which must be used to identify the multicast
     * group in all multicast group functions. */
    fm_int              handle;

    /* logical port associated with this group.  If no logical port has been
     * assigned to the group, logicalPort will be set to FM_LOGICAL_PORT_NONE. */
    fm_int              logicalPort;

    /* TRUE if this multicast group uses the older multicast group
     * restrictions: only a single multicast address per group, the address
     * must be specified before group activation, and L2 vs. L3 resources
     * are determined automatically unless the l2-switching-only flag is
     * set. */
    fm_bool             singleAddressMode;

    /* TRUE if the multicast group handle was allocated by the
     * fmCreateMcastGroupInt function, which implies that it must be
     * deallocated by fmDeleteMcastGroup.  Any handles allocated by
     * switch-specific AssignMcastGroup function must be released by
     * the appropriate switch-specific code. */
    fm_bool             localHandle;

    /* TRUE if the logical port (if any) was allocated by this group when
     * the group was activated.  This is used to determine whether to
     * release the logical port when deactivating the group. */
    fm_bool             localLogicalPort;

    /* Pointer to the multicast address set for the group if and only if
     * the group is in single-address mode. */
    fm_mcastAddrKey *   singleMcastAddr;

    /* TRUE if L3 vlan-replication (MTABLE) resources have been assigned to
     * the group. */
    fm_bool             hasL3Resources;

    /* TRUE if the group has been activated. */
    fm_bool             activated;

    /* ECMP Group ID for the group. */
    fm_int              ecmpGroup;

    /* TRUE if the group is supposed to forward frames to the CPU. */
    fm_bool             fwdToCpu;

    /* TRUE if an L3 group is supposed to use switching only.  In other words,
     * the group will not use vlan-replication resources, but will simply
     * switch multicast frames using the ingress vlan. */
    fm_bool             l2SwitchingOnly;

    /* TRUE if an L3 group is supposed to use switching only but with the usage
     * of the vlan-replication table. */
    fm_bool             l3SwitchingOnly;

    /* TRUE if an L3 group is configured to be used as a flood-set. */
    fm_bool             l3FloodSet;

    /* TRUE if an L2 group is configured to be used as a flood-set. */
    fm_bool             l2FloodSet;

    /* TRUE if an L2 group is configured to be used as a VxLan Decap flood-set. */
    fm_bool             l2VxLanDecap;

    /* TRUE if the group is being used to match on ip tunneling VLANS */
    fm_bool             ipTunneling;

    /* TRUE if egress STP state should not be checked prior to adding entries
     * to the replication table. */
    fm_bool             bypassEgressSTPCheck;

    /* TRUE if the group was created using the stacking API */
    fm_bool             stackGroup;

    /* TRUE if the group can update the hardware status */
    fm_bool             updateHardware;

    /* The routing action to be taken for multicast frames */
    fm_routeAction      groupAction;

    /* The routing state for the group. */
    fm_routeState       groupState;

    /* The group's default vlan, only used for L2 groups and for L3 groups
     * with the l2 switching only attribute set. */
    fm_uint16           defaultVlan;

    /* TRUE if this multicast group belongs to a private replication group. */
    fm_bool             privateGroup;

    /* the replication group for the mcast group */
    fm_int              repliGroup;

    /* The MTU index to use with this multicast group. */
    fm_int              mtuIndex;

    /* tree of multicast address keys for the group. */
    fm_customTree       addressTree;

    /* temporary pointer to a route entry pointer. When adding a multicast
     * route, the multicast code stores the pointer to the address-key's
     * route pointer so that the routing code can store the route pointer
     * for later use by the multicast code. */
    fm_intRouteEntry ** routePtrPtr;

    /* pointer to the switch extension structure */
    void *              switchExt;

    /* pointer to additional group information specific to the switch-type */
    void *              extension;

    /* Tree of multicast listeners (fm_intMulticastListener *) indexed by
     * (vlan, port) tuple. */
    fm_tree             listenerTree;

    /* tree containing listener tuples for ports that were listeners before
     * being added to a LAG, which causes them to be removed as listeners
     * (unless the LAG is also a listener).  They will be added back as
     * listeners, and removed from this tree, when they are removed from
     * membership in a LAG. */
    fm_tree             preLagListenerTree;

    /* Head/tail pointers for linked list of internal listeners. */
    FM_DLL_DEFINE_LIST(_fm_intMulticastInternalListener, firstIntListener, lastIntListener);

    /* Tree of multicast ECMP group (fm_intMulticastEcmp *) indexed by
     * ECMP handle. */
    fm_tree             ecmpTree;

    /* TRUE if repliGroup is a read-only resource owned by another multicast
     * group. */
    fm_bool             readOnlyRepliGroup;
    
    /* Group logical port set by attribute FM_MCASTGROUP_LOGICAL_PORT. Default
     * value FM_LOGICAL_PORT_NONE indicates that logical port was not set by
     * this attribute. Used for flooding ports. */
    fm_int              attrLogicalPort;

    /* TRUE if the group is internal. Internal is a flag defining if the
     * multicast group is managed by one of the API subsystem (mailbox, VN,...)
     * or if this group is created by the application. Internal multicast groups
     * cannot be created and removed by high level functions. */
    fm_bool     internal;

    /* TRUE if multicast group is configured as HNI flooding. These groups
     * should have all multicast flooding ports added as listeners. 
     * If application adds listener with port different
     * than VIRTUAL (FM_PORT_TYPE_VIRTUAL), all listeners with flooding ports
     * will be deleted. */
    fm_bool     isHNIFlooding;

} fm_intMulticastGroup;


/*****************************************************************************
 *
 * Multicast Group Listener
 *
 *****************************************************************************/
typedef struct _fm_intMulticastListener
{
    /* The customer's multicast group listener record. */
    fm_mcastGroupListener                    listener;

    /* Pointer to the internal listener, only if 'port' is a remote port. */
    struct _fm_intMulticastInternalListener *internal;

    /* Pointer to the parent group */
    fm_intMulticastGroup *                   group;

    /* TRUE if the listener has been added at the chip level */
    fm_bool                                  addedToChip;

    /* TRUE if the listener is added as flooding listener in multicast 
     * group created on the HNI request */
    fm_bool                                  floodListener;

    /* Head/tail pointers for linked list of sub-listeners. */
    FM_DLL_DEFINE_LIST(_fm_intMulticastListener, firstSubListener, lastSubListener);

    /* Next/previous pointers for linked list of sub-listeners. */
    FM_DLL_DEFINE_NODE(_fm_intMulticastListener, nextSubListener, prevSubListener);

} fm_intMulticastListener;


/*****************************************************************************
 *
 * Multicast Group Internal Listener
 *
 *****************************************************************************/
typedef struct _fm_intMulticastInternalListener
{
    /* The internal port number. */
    fm_int port;

    /* The number of remote listeners served by this internal port. */
    fm_int listenerCount;

    /* Next/previous pointers for linked list of internal listeners. */
    FM_DLL_DEFINE_NODE(_fm_intMulticastInternalListener, nextListener, prevListener);

} fm_intMulticastInternalListener;


/*****************************************************************************
 *
 * Multicast Group Internal ECMP
 *
 *****************************************************************************/
typedef struct _fm_intMulticastEcmp
{
    /* The lbs vlan */
    fm_int vlan;

    /* The ECMP ID */
    fm_int ecmpId;

} fm_intMulticastEcmp;


/*****************************************************************************
 * Manipulate linked list of Internal Listeners.
 *****************************************************************************/
#define fmGetFirstMcastInternalListener(groupPtr) \
    FM_DLL_GET_FIRST(groupPtr, firstIntListener)

#define fmGetLastMcastInternalListener(groupPtr) \
    FM_DLL_GET_LAST(groupPtr, lastIntListener)

#define fmGetNextMcastInternalListener(listener) \
    FM_DLL_GET_NEXT(listener, nextListener)

#define fmGetPreviousMcastInternalListener(listener) \
    FM_DLL_GET_PREVIOUS(listener, prevListener)

#define fmAppendMcastInternalListener(groupPtr, newListener)          \
    FM_DLL_INSERT_LAST(groupPtr, firstIntListener, lastIntListener,   \
                       newListener, nextListener, prevListener)

#define fmRemoveMcastInternalListener(groupPtr, internalListener)     \
    FM_DLL_REMOVE_NODE(groupPtr, firstIntListener, lastIntListener,   \
                       internalListener, nextListener, prevListener)

/*****************************************************************************
 * Manipulate linked list of Sub-listeners.
 *****************************************************************************/
#define fmAppendMcastSubListener(listenerPtr, subListenerPtr)           \
    FM_DLL_INSERT_LAST(listenerPtr, firstSubListener, lastSubListener,  \
                       subListenerPtr, nextSubListener, prevSubListener)

#define fmRemoveMcastSubListener(listenerPtr, subListenerPtr)           \
    FM_DLL_REMOVE_NODE(listenerPtr, firstSubListener, lastSubListener,  \
                       subListenerPtr, nextSubListener, prevSubListener)

fm_intMulticastGroup *fmFindMcastGroup(fm_int sw, fm_int handle);
fm_intMulticastGroup *fmFindMcastGroupByPort(fm_int sw, fm_int port);
void fmGetMcastDestAddress(fm_multicastAddress *multicast, fm_ipAddr *destAddr);
fm_status fmApplyMasksToMulticastAddress(fm_multicastAddress *multicast);

fm_status fmMcastGroupInit(fm_int sw);
fm_status fmCreateMcastGroupInt(fm_int  sw,
                                fm_int *mcastGroup,
                                fm_bool stacking,
                                fm_bool internal);
fm_status fmDeleteMcastGroupInt(fm_int sw, fm_int mcastGroup, fm_bool internal);
fm_status fmActivateMcastGroupInt(fm_int sw, fm_int mcastGroup);
fm_status fmDeactivateMcastGroupInt(fm_int sw, fm_int mcastGroup);
fm_status fmSetMcastGroupAttributeInt(fm_int sw,
                                      fm_int mcastGroup,
                                      fm_int attr,
                                      void * value);
fm_status fmGetMcastGroupAttributeInt(fm_int sw,
                                      fm_int mcastGroup,
                                      fm_int attr,
                                      void * value);
fm_status fmAddMcastGroupAddressInt(fm_int               sw,
                                    fm_int               mcastGroup,
                                    fm_multicastAddress *address);

fm_status fmAllocateMcastGroupsInt(fm_int    sw,
                                   fm_uint    startGlort,
                                   fm_uint    glortSize,
                                   fm_int    *baseMcastGroup,
                                   fm_int    *numMcastGroups,
                                   fm_int    *step);
fm_status fmFreeMcastGroupsInt(fm_int sw, fm_int baseMcastGroup);
fm_status fmFreeMcastGroupDataStructures(fm_switch *switchPtr);

fm_status fmMcastAddPortToLagNotify(fm_int sw, fm_int lagIndex, fm_int port);
fm_status fmMcastRemovePortFromLagNotify(fm_int sw, fm_int lagIndex, fm_int port);
fm_status fmMcastDeleteLagNotify(fm_int sw, fm_int lagIndex);
fm_status fmMcastDeleteVlanNotify(fm_int sw, fm_int vlan);
fm_status fmClearMcastGroupAddressesInt(fm_int                sw,
                                        fm_intMulticastGroup *group);

fm_status fmMcastBuildMacEntry(fm_int                  sw,
                               fm_intMulticastGroup *  group,
                               fm_multicastMacAddress *addr,
                               fm_macAddressEntry *    macEntry,
                               fm_int *                trigger);

fm_status fmRewriteMcastGroupMacAddresses(fm_int                sw,
                                          fm_intMulticastGroup *group);

fm_status fmSetMcastGroupRouteActiveFlags(fm_int                sw,
                                          fm_intMulticastGroup *group,
                                          fm_routeState         routeState);

fm_status fmGetMcastGroupUsedInt(fm_int   sw,
                                 fm_int   mcastGroup,
                                 fm_bool *used,
                                 fm_bool  resetFlag);

fm_int fmCompareMulticastAddresses(const void *key1,
                                   const void *key2);

fm_status fmAddMcastGroupListenerInternal(fm_int                 sw,
                                          fm_int                 mcastGroup,
                                          fm_mcastGroupListener *listener);

fm_status fmAddMcastGroupListenerInternalForFlood(fm_int                 sw,
                                                  fm_int                 mcastGroup,
                                                  fm_mcastGroupListener *listener);

fm_status fmDeleteMcastGroupListenerInternal(fm_int                 sw,
                                             fm_int                 mcastGroup,
                                             fm_mcastGroupListener *listener);

fm_status fmDeleteMcastGroupListenerInternalForFlood(fm_int                 sw,
                                                     fm_int                 mcastGroup,
                                                     fm_mcastGroupListener *listener);

fm_status fmAddMcastGroupListenerListInternal(fm_int                 sw,
                                              fm_int                 mcastGroup,
                                              fm_int                 numListeners,
                                              fm_mcastGroupListener *listenerList);
fm_status fmGetMcastGroupPortInt(fm_int  sw,
                                 fm_int  mcastGroup,
                                 fm_int *logPort);

fm_status fmConfigureMcastGroupAsHNIFlooding(fm_int  sw,
                                             fm_int  mcastGroup,
                                             fm_bool isHNIFlooding);

fm_bool fmIsMcastGroupHNIFlooding(fm_int sw,
                                  fm_int mcastGroup);

fm_bool fmHasMcastGroupNonVirtualListeners(fm_int sw,
                                           fm_int mcastGroup);

fm_bool fmHasMcastGroupVirtualListeners(fm_int sw,
                                        fm_int mcastGroup);

fm_status fmUpdateMcastHNIFloodingGroups(fm_int  sw,
                                         fm_int  port,
                                         fm_bool state);

#endif
