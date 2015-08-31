/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:             fm_api_multicast.h
 * Creation Date:    February 7, 2007
 * Description:      Header file for routing services.
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

#ifndef __FM_FM_API_MULTICAST_H
#define __FM_FM_API_MULTICAST_H

/**********************************************************
 * Value for the FM_MCASTGROUP_SHARED_REPLICATION_GROUP 
 * attribute. 
 *********************************************************/
#define FM_MCASTGROUP_REPLICATION_GROUP_DISABLE  -1


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a normal multicast listener, being a VLAN/port combination that should
 * be included in a multicast.
 ****************************************************************************/

typedef struct _fm_multicastListener
{
    /** VLAN to which the multicast should be sent. */
    fm_uint16 vlan;

    /** Logical port to which the multicast should be sent. */
    fm_int    port;

    /** Whether the listener is added to a flooding multicast group.
     *  \chips  FM10000 */
    fm_bool   floodListener;

    /** xcastGlort to use if the listener is a floodListener.
     *  \chips  FM10000 */
    fm_int    xcastGlort;

    /** Flag to let the pipeline know that this is a remote
     *  listener. Use by tunneling protocol, such TRILL, VxLan, etc.
     *  
     *  \chips  FM6000 */
    fm_bool   remoteFlag;

} fm_multicastListener;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a VN tunnel multicast listener, being a Tunnel/VN combination that
 * should be included in a multicast.
 ****************************************************************************/
typedef struct _fm_mcastGroupVNListener
{
    /** Tunnel to which the multicast should be sent. */
    fm_int    tunnelId;

    /** Virtual Network over which the multicast should be sent. */
    fm_uint32 vni;

} fm_mcastGroupVNListener;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a Flow API multicast listener, being a Flow Table/Id combination
 * that should be included in a multicast.
 ****************************************************************************/
typedef struct _fm_mcastGroupFlowListener
{
    /** TE Flow table index to which the multicast should be sent. */
    fm_int    tableIndex;

    /** TE Flow id over which the multicast should be sent. */
    fm_int    flowId;

} fm_mcastGroupFlowListener;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a union of multicast group listeners.
 ****************************************************************************/
typedef union _fm_mcastGroupListenerInfo
{
    /** Local port/vlan multicast listener. */
    fm_multicastListener      portVlanListener;

    /** Virtual network tunnel listener. */
    fm_mcastGroupVNListener   vnListener;

    /** Flow API tunnel listener. */
    fm_mcastGroupFlowListener flowListener;

} fm_mcastGroupListenerInfo;


/****************************************************************************/
/** \ingroup typeEnum
 *
 * Multicast group listener types.
 ****************************************************************************/
typedef enum
{
    /** Local port/vlan multicast listener. */
    FM_MCAST_GROUP_LISTENER_PORT_VLAN = 0,

    /** Virtual network tunnel listener. */
    FM_MCAST_GROUP_LISTENER_VN_TUNNEL,

    /** Flow API tunnel listener. */
    FM_MCAST_GROUP_LISTENER_FLOW_TUNNEL,

    /** UNPUBLISHED: For internal use only. */
    FM_MCAST_GROUP_LISTENER_MAX,

} fm_mcastGroupListenerType;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines an enhanced multicast group listener record.
 ****************************************************************************/
typedef struct _fm_mcastGroupListener
{
    /** Listener type. */
    fm_mcastGroupListenerType listenerType;

    /** Listener information. */
    fm_mcastGroupListenerInfo info;

} fm_mcastGroupListener;


/****************************************************************************/
/** \ingroup constMcastGrpAttr
 *
 * Multicast Group attributes, used as an argument to
''fmSetMcastGroupAttribute'' and ''fmGetMcastGroupAttribute''.
 *                                                                      \lb\lb
 * For each attribute, the data type of the corresponding attribute value is
 * indicated.
 ****************************************************************************/
enum _fm_mcastGroupAttr
{
    /** Type fm_bool: Forward multicast group traffic 
     *  to the CPU instead of to the group's listeners:
     *  FM_ENABLED or FM_DISABLED (default). 
     * 
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_MCASTGROUP_FWD_TO_CPU = 0,

    /** Type fm_bool: Indicates if the given multicast group
     *  has forwarded L3 traffic.  The value of this attribute is undefined
     *  for groups registered to an L2 address.  Setting the value to FALSE
     *  resets the usage.  Setting the value to TRUE has no effect. 
     * 
     *   \chips  FM4000, FM6000, FM10000 */
    FM_MCASTGROUP_USED,

    /** Type fm_bool: Restricts multicast frames in the multicast
     *  group to be L2 switched only, with only a single copy of the frame
     *  egressing on each listener port: FM_ENABLED or FM_DISABLED 
     *  (default). When enabled, multicast frames will not be routed 
     *  even if the multicast group's multicast address (as set with 
     *  a call to ''fmSetMcastGroupAddress'') is an L3 address. 
     *                                                                  \lb\lb
     *  Note that if the multicast group's address is an L2 address, 
     *  the multicast frames will be L2 switched and not routed,
     *  even if this attribute is not enabled.
     *                                                                  \lb\lb
     *  If the ''FM_MCASTGROUP_L2_FLOOD_SET'' attribute is enabled,
     *  then this attribute must also be enabled.
     *                                                                  \lb\lb
     *  If the ''FM_MCASTGROUP_L2_VXLAN_DECAP'' attribute is
     *  enabled, then this attribute must also be enabled.
     *                                                                  \lb\lb
     *  Note that this attribute takes precedence over 
     *  ''FM_MCASTGROUP_L3_SWITCHING_ONLY'' if both are enabled.
     * 
     *   \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_MCASTGROUP_L2_SWITCHING_ONLY,

    /** Type ''fm_routeAction'': Specifies the action to be taken
     *   for this multicast group.  The default value specifies normal
     *   routing.  This attribute is ignored for Layer-2 multicast groups.
     * 
     *   \chips  FM4000, FM6000, FM10000 */
    FM_MCASTGROUP_ACTION,

    /** Type ''fm_routeState'': Specifies the state of the route used by this
     *  multicast group.  The default value specifies that the state is
     *  up. This attribute is ignored for Layer-2 multicast groups.
     * 
     *   \chips  FM4000, FM6000, FM10000 */
    FM_MCASTGROUP_STATE,

    /** Type fm_int: Specifies the replication group with
     *  which this multicast group is associated, as returned in a call to
     *  ''fmCreateReplicationGroup''. The default value for this attribute
     *  is FM_MCASTGROUP_REPLICATION_GROUP_DISABLE, which indicates that 
     *  the multicast group does not share frame replication hardware
     *  resources with any other multicast group (hardware resources will
     *  be allocated automatically with no need to call 
     *  ''fmCreateReplicationGroup''). Multicast groups share a replication
     *  group by specifying the same replication group ID. Only multicast
     *  groups with orthogonal sets of listener ports should share 
     *  replication groups.
     *
     *  \chips  FM6000, FM10000 */
    FM_MCASTGROUP_SHARED_REPLICATION_GROUP,

    /** Type fm_bool: Restricts multicast frames in the multicast
     *  group to be L2 switched only, but multiple copies of the frame may 
     *  egress on each port, one copy per VLAN: FM_ENABLED or FM_DISABLED 
     *  (default). When enabled, since the frames are only switched and not 
     *  routed, the DMAC and TTL are not changed.
     *                                                                  \lb\lb
     *  If the ''FM_MCASTGROUP_L3_FLOOD_SET'' attribute is enabled, then 
     *  this attribute must also be enabled. 
     *                                                                  \lb\lb
     *  Note that ''FM_MCASTGROUP_L2_SWITCHING_ONLY'' takes precedence over 
     *  this attribute if both are enabled.
     * 
     *   \chips  FM6000, FM10000 */       
    FM_MCASTGROUP_L3_SWITCHING_ONLY,

    /** Type fm_bool: Specifies if this multicast group defines an L3 flood
     *  set: FM_ENABLED or FM_DISABLED (default). A flood set multicast group
     *  identifies a set of listener destinations to which a frame should
     *  be switched on a MAC table miss (the frame's DMAC/VLAN pair does not
     *  appear in the MAC table).
     *                                                                  \lb\lb
     *  ''FM_MCASTGROUP_L3_SWITCHING_ONLY'' must also be enabled if this
     *  attribute is enabled.
     * 
     *   \chips  FM6000, FM10000 */
    FM_MCASTGROUP_L3_FLOOD_SET,

    /** Type fm_bool: Specifies if this multicast group defines a L2
     *  flood set: FM_ENABLED or FM_DISABLED (default). A flood set
     *  multicast group identifies a set of listener destinations to
     *  which a frame should be switched on a MAC table miss (the
     *  frame's DMAC/VLAN pair does not appear in the MAC table).
     *                                                                  \lb\lb
     *  ''FM_MCASTGROUP_L2_SWITCHING_ONLY'' must also be enabled if
     *  this attribute is enabled.
     * 
     *   \chips  FM6000, FM10000 */
    FM_MCASTGROUP_L2_FLOOD_SET,

    /** Type fm_bool: Specifies if this multicast group defines an L2 flood
     *  set in pair with a VxLan Decapsulation: FM_ENABLED or
     *  FM_DISABLED (default). A flood set multicast group identifies a set
     *  of listener destinations to which a frame should be switched on a MAC
     *  table miss (the frame's DMAC/VLAN pair does not appear in the MAC
     *  table).
     *                                                                  \lb\lb
     *  ''FM_MCASTGROUP_L2_SWITCHING_ONLY'' must also be enabled if this
     *  attribute is enabled.
     * 
     *   \chips  FM6000 */
    FM_MCASTGROUP_L2_VXLAN_DECAP,

    /** Type fm_bool: Specifies if this multicast group must also
     *  match on a VLAN that has its FM_VLAN_IP_TUNNELING attribute enabled:
     *  FM_ENABLED or FM_DISABLED (default).
     * 
     *   \chips  FM6000 */
    FM_MCASTGROUP_IP_TUNNELING,

    /** Type fm_bool: Specifies if the spanning tree state of the egress
     *  port/FID pair should be checked for this multicast group prior to
     *  adding entries to the replication table. FM_ENABLED or
     *  FM_DISABLED (default). Enabling this attribute will bypass the
     *  STP verification mechanism and add port/FID pairs to the replication
     *  table regardless of their STP state.
     *
     *   \chips  FM6000, FM10000 */
    FM_MCASTGROUP_BYPASS_EGRESS_STP_CHECK,

    /** Type fm_int: Specifies the MTU index to be used for this multicast
     *  group. Default value is zero, which means that entry 0 in the MTU
     *  table will be used. Valid values are zero to fifteen (seven for FM10000).
     *
     *  \chips FM4000, FM6000, FM10000 */
    FM_MCASTGROUP_MTU_INDEX,

    /** Type fm_bool: Specifies whether the shared replication group can be
     *  modified by this multicast group. Enabling this attribute allows one
     *  multicast group to use the other group's replication resources and
     *  configuration.
     *
     *  \chips FM6000, FM10000 */
    FM_MCASTGROUP_READ_ONLY_REPLI_GROUP,

    /** UNPUBLISHED: For internal use only (to support flooding ports). Type
     * fm_int: Set logical port for this multicast group. Enabling this
     * attribute allows multicast group to use the provided logical port.
     * Default value FM_LOGICAL_PORT_NONE indicates that logical port is not set
     * by this attribute. 
     *
     *  \chips FM10000 */
    FM_MCASTGROUP_LOGICAL_PORT,

    /** UNPUBLISHED: For internal use only. */
    FM_MCASTGROUP_ATTRIBUTE_MAX

};


/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmAddMcastGroupListener''. */
#define fmAddMulticastListener(sw, mcastGroup, listener) \
        fmAddMcastGroupListener( (sw), (mcastGroup), (listener) )

/** A legacy synonym for ''fmDeleteMcastGroupListener''. */
#define fmDeleteMulticastListener(sw, mcastGroup, listener) \
        fmDeleteMcastGroupListener( (sw), (mcastGroup), (listener) )

/** A legacy synonym for ''fmGetMcastGroupListenerFirst''. */
#define fmGetMulticastListenerFirst(sw, mcastGroup, firstListener) \
        fmGetMcastGroupListenerFirst( (sw), (mcastGroup), (firstListener) )

/** A legacy synonym for ''fmGetMcastGroupListenerNext''. */
#define fmGetMulticastListenerNext(sw, mcastGroup, currentListener, nextListener) \
        fmGetMcastGroupListenerNext((sw), (mcastGroup), (currentListener), (nextListener) )

/** A legacy synonym for ''fmGetMcastGroupListenerList''. */
#define fmGetMulticastListenerList(sw, mcastGroup, numListeners, listenerList, max) \
        fmGetMcastGroupListenerList( (sw), (mcastGroup), (numListeners), (listenerList), (max) )
        
/** A legacy synonym for ''fmFindMcastGroupByAddress''. */
#define fmSearchMcastGroupByAddress(sw, address, mcastGroup)    \
        fmFindMcastGroupByAddress( (sw), (address), (mcastGroup) )

/** @} (end of Doxygen group) */


fm_status fmCreateMcastGroup(fm_int  sw,
                             fm_int *mcastGroup);

fm_status fmDeleteMcastGroup(fm_int sw,
                             fm_int mcastGroup);

fm_status fmAddMcastGroupAddress(fm_int               sw,
                                 fm_int               mcastGroup,
                                 fm_multicastAddress *address);

fm_status fmDeleteMcastGroupAddress(fm_int               sw,
                                    fm_int               mcastGroup,
                                    fm_multicastAddress *address);

fm_status fmAddMcastGroupListener(fm_int                sw,
                                  fm_int                mcastGroup,
                                  fm_multicastListener *listener);

fm_status fmAddMcastGroupListenerV2(fm_int                 sw,
                                    fm_int                 mcastGroup,
                                    fm_mcastGroupListener *listener);

fm_status fmAddMcastGroupListenerList(fm_int                 sw,
                                      fm_int                 mcastGroup,
                                      fm_int                 numListeners,
                                      fm_multicastListener * listenerList);

fm_status fmAddMcastGroupListenerListV2(fm_int                 sw,
                                        fm_int                 mcastGroup,
                                        fm_int                 numListeners,
                                        fm_mcastGroupListener *listenerList);

fm_status fmDeleteMcastGroupListener(fm_int                sw,
                                     fm_int                mcastGroup,
                                     fm_multicastListener *listener);

fm_status fmDeleteMcastGroupListenerV2(fm_int                 sw,
                                       fm_int                 mcastGroup,
                                       fm_mcastGroupListener *listener);

fm_status fmGetMcastGroupList(fm_int  sw,
                              fm_int *numMcastGroups,
                              fm_int *mcastGroupList,
                              fm_int  max);

fm_status fmGetMcastGroupListenerList(fm_int                sw,
                                      fm_int                mcastGroup,
                                      fm_int *              numListeners,
                                      fm_multicastListener *listenerList,
                                      fm_int                maxListeners);

fm_status fmGetMcastGroupListenerListV2(fm_int                 sw,
                                        fm_int                 mcastGroup,
                                        fm_int *               numListeners,
                                        fm_mcastGroupListener *listenerList,
                                        fm_int                 maxListeners);

fm_status fmGetMcastGroupFirst(fm_int  sw,
                               fm_int *firstMcastNumber);

fm_status fmGetMcastGroupNext(fm_int  sw,
                              fm_int  currentMcastNumber,
                              fm_int *nextMcastNumber);

fm_status fmGetMcastGroupAddressList(fm_int               sw,
                                     fm_int               mcastGroup,
                                     fm_int *             numAddresses,
                                     fm_multicastAddress *addressList,
                                     fm_int               maxListeners);

fm_status fmGetMcastGroupAddressFirst(fm_int               sw,
                                      fm_int               mcastGroup,
                                      fm_multicastAddress *firstAddress);

fm_status fmGetMcastGroupAddressNext(fm_int               sw,
                                     fm_int               mcastGroup,
                                     fm_multicastAddress *currentAddress,
                                     fm_multicastAddress *nextAddress);

fm_status fmGetMcastGroupListenerFirst(fm_int                sw,
                                       fm_int                mcastGroup,
                                       fm_multicastListener *firstListener);

fm_status fmGetMcastGroupListenerFirstV2(fm_int                 sw,
                                         fm_int                 mcastGroup,
                                         fm_mcastGroupListener *firstListener);

fm_status fmGetMcastGroupListenerNext(fm_int                sw,
                                      fm_int                mcastGroup,
                                      fm_multicastListener *currentListener,
                                      fm_multicastListener *nextListener);

fm_status fmGetMcastGroupListenerNextV2(fm_int                 sw,
                                        fm_int                 mcastGroup,
                                        fm_mcastGroupListener *currentListener,
                                        fm_mcastGroupListener *nextListener);

fm_status fmSetMcastGroupAddress(fm_int               sw,
                                 fm_int               mcastGroup,
                                 fm_multicastAddress *address);

fm_status fmClearMcastGroupAddress(fm_int sw,
                                   fm_int mcastGroup);

fm_status fmGetMcastGroupAddress(fm_int               sw,
                                 fm_int               mcastGroup,
                                 fm_multicastAddress *address);

fm_status fmActivateMcastGroup(fm_int sw,
                               fm_int mcastGroup);

fm_status fmDeactivateMcastGroup(fm_int sw,
                                 fm_int mcastGroup);

fm_status fmFindMcastGroupByAddress(fm_int               sw,
                                    fm_multicastAddress *address,
                                    fm_int *             mcastGroup);

fm_status fmGetAvailableMulticastListenerCount(fm_int  sw,
                                               fm_int *count);

fm_status fmSetMcastGroupAttribute(fm_int sw,
                                   fm_int mcastGroup,
                                   fm_int attr,
                                   void * value);

fm_status fmGetMcastGroupAttribute(fm_int sw,
                                   fm_int mcastGroup,
                                   fm_int attr,
                                   void * value);

fm_status fmGetMcastGroupUsed(fm_int   sw,
                              fm_int   mcastGroup,
                              fm_bool *used,
                              fm_bool  resetFlag);

fm_status fmGetMcastGroupPort(fm_int  sw,
                              fm_int  mcastGroup,
                              fm_int *logPort);

fm_status fmAddMcastGroupEcmp(fm_int  sw,
                              fm_int  mcastGroup,
                              fm_int  lbsVlan,
                              fm_int *ecmpId);

fm_status fmDeleteMcastGroupEcmp(fm_int sw,
                                 fm_int mcastGroup,
                                 fm_int ecmpId);

fm_status fmGetMcastGroupHwIndex(fm_int  sw,
                                 fm_int  mcastGroup,
                                 fm_int *hwIndex);

#endif /* __FM_FM_API_MULTICAST_H */
