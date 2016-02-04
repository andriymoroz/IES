/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_property.h
 * Creation Date:   2006
 * Description:     API Properties subsystem.
 *
 * Copyright (c) 2006 - 2016, Intel Corporation
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

#ifndef __FM_FM_PROPERTY_H
#define __FM_FM_PROPERTY_H

#define FM_API_ATTR_TEXT_MAX_LENGTH                 256


/**************************************************/
/** \ingroup typeEnum
 * Used as an argument to ''fmGetApiProperty'' and
 * ''fmSetApiProperty'' to identify the data type
 * of the API property.
 **************************************************/
typedef enum
{
    /** Property type is an integer. */
    FM_API_ATTR_INT = 0,

    /** Property type is a Boolean. */
    FM_API_ATTR_BOOL,

    /** Property type is a floating-point number. */
    FM_API_ATTR_FLOAT,

    /** Property type is a text string. */
    FM_API_ATTR_TEXT,

    /* -- Add new property types above this line -- */

    /** UNPUBLISHED: For internal use only. */
    FM_API_ATTR_MAX

} fm_apiAttrType;


/*****************************************************************************/
/* API Properties
 *
 * Each property has a key, a type and a default value. The key strings for
 * API properties are organized in a hierarchical dotted format.
 *
 * Properties are defined below in two sections. The first section includes 
 * properties that are documented and available for customer use and will be 
 * supported in future versions of the API. The second section is for
 * for Intel use only and do not appear in the API User Guide. Use of these
 * properties outside of Intel is not recommended unless guided by Intel
 * to do so.
 *
 * The key symbol name begins with AAK_ for API Attribute Key. The key value
 * is a string.
 *
 * The property type symbol name begins with AAT_ for API Attribute Type. It
 * is not typically used by the software, since the helper functions
 * fmGet*ApiProperty are per-type, but is required for documentation purposes.
 *
 * The default property symbol name begins with AAD_ for API Attribute
 * Default. The default value itself is a function of the property type.
 *
 * NOTA BENE! Once a key is defined, it must not be changed, as doing so
 * will break customer applications and render operating systems in the
 * field to fail. The reason is that API properties can be read in from
 * a file at run-time. These files specify the property key. If an existing
 * key becomes unrecognized, the specified property value will not be
 * used by the API.
 *****************************************************************************/

/************************************************************************
 ****                                                                ****
 ****               BEGIN DOCUMENTED PROPERTIES                      ****
 ****                                                                ****
 ************************************************************************/
 
/** \ingroup apiAttrib
 * @{ */
 
/** The default spanning tree state of a port for a VLAN of which it is a
 *  member. See ''Spanning Tree States'' for possible values. This property
 *  is also used to set the initial spanning tree state for all ports in all
 *  VLANs in shared spanning tree mode (see the ''FM_SPANNING_TREE_MODE''
 *  switch attribute). */
#define FM_AAK_API_STP_DEF_STATE_VLAN_MEMBER        "api.stp.defaultState.vlanMember"
#define FM_AAT_API_STP_DEF_STATE_VLAN_MEMBER        FM_API_ATTR_INT
#define FM_AAD_API_STP_DEF_STATE_VLAN_MEMBER        FM_STP_STATE_DISABLED

/** The flag indicating whether or not we allow the CPU to send to the CPU in
 *  the directed packet send mode */
#define FM_AAK_API_DIRECT_SEND_TO_CPU               "api.allowDirectSendToCpu"
#define FM_AAT_API_DIRECT_SEND_TO_CPU               FM_API_ATTR_BOOL
#define FM_AAD_API_DIRECT_SEND_TO_CPU               FALSE

/** The default spanning tree state of a port for a VLAN of which it is not
 *  a member. See ''Spanning Tree States'' for possible values. */
#define FM_AAK_API_STP_DEF_STATE_VLAN_NON_MEMBER    "api.stp.defaultState.vlanNonMember"
#define FM_AAT_API_STP_DEF_STATE_VLAN_NON_MEMBER    FM_API_ATTR_INT
#define FM_AAD_API_STP_DEF_STATE_VLAN_NON_MEMBER    FM_STP_STATE_DISABLED

/** Indicates whether the API should automatically identify the switch
 *  device type upon a ''FM_EVENT_SWITCH_INSERTED'' event. */
#define FM_AAK_DEBUG_BOOT_IDENTIFYSWITCH            "debug.boot.identifySwitch"
#define FM_AAT_DEBUG_BOOT_IDENTIFYSWITCH            FM_API_ATTR_BOOL
#define FM_AAD_DEBUG_BOOT_IDENTIFYSWITCH            TRUE

/** Indicates whether to reboot the switch following identification. */
#define FM_AAK_DEBUG_BOOT_RESET                     "debug.boot.reset"
#define FM_AAT_DEBUG_BOOT_RESET                     FM_API_ATTR_BOOL
#define FM_AAD_DEBUG_BOOT_RESET                     TRUE

/** Indicates whether the platform should automatically generate
 *  ''FM_EVENT_SWITCH_INSERTED'' events for all switches known at startup. */
#define FM_AAK_DEBUG_BOOT_AUTOINSERTSWITCH          "debug.boot.autoInsertSwitches"
#define FM_AAT_DEBUG_BOOT_AUTOINSERTSWITCH          FM_API_ATTR_BOOL
#define FM_AAD_DEBUG_BOOT_AUTOINSERTSWITCH          TRUE

/** Indicates the length of time in ns to hold the device in reset
 *  prior to identification */
#define FM_AAK_API_BOOT_RESET_TIME                  "api.boot.deviceResetTime"
#define FM_AAT_API_BOOT_RESET_TIME                  FM_API_ATTR_INT
#define FM_AAD_API_BOOT_RESET_TIME                  200000000

/** Specifies whether Switch-Aggregate links should be auto-enabled
 *  when they are added via ''fmAddSWAGLink''. */
#define FM_AAK_API_AUTO_ENABLE_SWAG_LINKS           "api.swag.autoEnableLinks"
#define FM_AAT_API_AUTO_ENABLE_SWAG_LINKS           FM_API_ATTR_BOOL
#define FM_AAD_API_AUTO_ENABLE_SWAG_LINKS           TRUE

/** Specifies the limit to block low priority event request when the number of
 *  free event falls below this limit.
 *                                                                      \lb\lb
 *  Note that the Default value will be FM_MAX_EVENTS/24 if FM_MAX_EVENTS is
 *  greater than 512, otherwise it will be set to 24.
 */
#define FM_AAK_API_FREE_EVENT_BLOCK_THRESHOLD       "api.event.blockThreshold"
#define FM_AAT_API_FREE_EVENT_BLOCK_THRESHOLD       FM_API_ATTR_INT
#if (FM_MAX_EVENTS > 512)
#define FM_AAD_API_FREE_EVENT_BLOCK_THRESHOLD       FM_MAX_EVENTS/24
#else
#define FM_AAD_API_FREE_EVENT_BLOCK_THRESHOLD       24
#endif

/** Specifies the limit to unblock low priority event request when the number of
 *  free event exceeds above this limit.
 *                                                                      \lb\lb
 *  Note that the Default value will be
 *  ((FM_MAX_EVENTS/24) + (FM_MAX_EVENTS/192) + 1) if FM_MAX_EVENTS is greater
 *  than 512, otherwise it will be set to 30.
 */
#define FM_AAK_API_FREE_EVENT_UNBLOCK_THRESHOLD     "api.event.unblockThreshold"
#define FM_AAT_API_FREE_EVENT_UNBLOCK_THRESHOLD     FM_API_ATTR_INT
#if (FM_MAX_EVENTS > 512)
#define FM_AAD_API_FREE_EVENT_UNBLOCK_THRESHOLD     ((FM_MAX_EVENTS/24) + (FM_MAX_EVENTS/192) + 1)
#else
#define FM_AAD_API_FREE_EVENT_UNBLOCK_THRESHOLD     30
#endif

/** Specifies the timeout (millisecond) value for the 
 *  semaphore blocking on event free.
 */
#define FM_AAK_API_EVENT_SEM_TIMEOUT                "api.event.semaphoreTimeout"
#define FM_AAT_API_EVENT_SEM_TIMEOUT                FM_API_ATTR_INT
#define FM_AAD_API_EVENT_SEM_TIMEOUT                1000

/** Indicates whether received packets are to be queued directly to the 
 *  application's event handler callback function (see ''fm_eventHandler'')
 *  from the packet receive thread. This will greatly improve the packet
 *  reception rate, but may not be appropriate for all platforms.
 *                                                                      \lb\lb
 *  The thread responsible for packet reception will queue the packet directly.
 *  On some platforms, such as the fm85xxep, the same thread is also 
 *  responsible for other tasks. Hence the application cannot take too long 
 *  in the event handler when processing the receive packet. For platforms
 *  like the fm85xxep, it is not appropriate to enable direct queueing.
 *                                                                      \lb\lb
 *  Another caveat is that with direct queueing, the application's event 
 *  handler can now be called simultaneously for packet receive events as
 *  well as other events. The application must ensure appropriate locking 
 *  around access to any resources shared bewteen the processing of received
 *  packets and other events.
 *                                                                      \lb\lb
 *  This property is not currently supported for switch aggregates or
 *  FM2000 family devices.
 */
#define FM_AAK_API_PACKET_RX_DIRECT_ENQUEUEING      "api.packet.rxDirectEnqueueing"
#define FM_AAT_API_PACKET_RX_DIRECT_ENQUEUEING       FM_API_ATTR_BOOL
#define FM_AAD_API_PACKET_RX_DIRECT_ENQUEUEING       FALSE

/** Specifies destinations to which received packets are to be forwarded
 *  by the driver. Possible destinations are: netdev, netlink processed
 *  by API, and netlink read directly by application. Only one of the
 *  two netlink destinations may be selected at a time. The default
 *  destinations are netdev and netlink processed by API.
 *  See ''Rx Packet Driver Destination Masks'' for possible values.
 *                                                                      \lb\lb
 *  This property is currently supported for FM6000 family devices.
 */
#define FM_AAK_API_PACKET_RX_DRV_DEST               "api.packet.rxDriverDestinations"
#define FM_AAT_API_PACKET_RX_DRV_DEST               FM_API_ATTR_INT
#define FM_AAD_API_PACKET_RX_DRV_DEST               (FM_PACKET_RX_DRV_DEST_NETDEV | \
                                                     FM_PACKET_RX_DRV_DEST_NETLINK_API)

/** Whether ''fmDeleteLAG'' should wait for LAG deletion to complete
 *  (meaning that any required MAC Table purges are finished) before
 *  returning to the caller.  FALSE means that the deletions must
 *  complete prior to function return, TRUE means that the function may
 *  return as soon as the purge is enqueued.  Note that TRUE means that
 *  the application may get ''FM_ERR_NO_FREE_LAG'' if all lag groups are
 *  in the "pending deletion" state when ''fmCreateLAG'' is called. */
#define FM_AAK_API_ASYNC_LAG_DELETION           "api.lag.asyncDeletion"
#define FM_AAT_API_ASYNC_LAG_DELETION           FM_API_ATTR_BOOL
#define FM_AAD_API_ASYNC_LAG_DELETION           TRUE

/** Whether the API should generate an FM_EVENT_ENTRY_LEARNED or
 *  FM_EVENT_ENTRY_AGED event when the application adds or deletes a static
 *  address to/from the MA table. */
#define FM_AAK_API_MA_EVENT_ON_STATIC_ADDR      "api.ma.eventOnStaticAddr"
#define FM_AAT_API_MA_EVENT_ON_STATIC_ADDR      FM_API_ATTR_BOOL
#define FM_AAD_API_MA_EVENT_ON_STATIC_ADDR      FALSE

/** Whether the API should generate an FM_EVENT_ENTRY_LEARNED or
 *  FM_EVENT_ENTRY_AGED event when the application adds or deletes a dynamic
 *  address to/from the MA table. */
#define FM_AAK_API_MA_EVENT_ON_DYNAMIC_ADDR     "api.ma.eventOnDynamicAddr"
#define FM_AAT_API_MA_EVENT_ON_DYNAMIC_ADDR     FM_API_ATTR_BOOL
#define FM_AAD_API_MA_EVENT_ON_DYNAMIC_ADDR     FALSE

/** Whether the API should generate an FM_EVENT_ENTRY_LEARNED or
 *  FM_EVENT_ENTRY_AGED event when the application modifies an address
 *  in the MA table. */
#define FM_AAK_API_MA_EVENT_ON_ADDR_CHANGE      "api.ma.eventOnAddrChange"
#define FM_AAT_API_MA_EVENT_ON_ADDR_CHANGE      FM_API_ATTR_BOOL
#define FM_AAD_API_MA_EVENT_ON_ADDR_CHANGE      FALSE

/** Indicates whether the API should flush all the addresses associated with
 *  a port from the MA Table when the port goes down. */
#define FM_AAK_API_MA_FLUSH_ON_PORT_DOWN       "api.ma.flushOnPortDown"
#define FM_AAT_API_MA_FLUSH_ON_PORT_DOWN       FM_API_ATTR_BOOL
#define FM_AAD_API_MA_FLUSH_ON_PORT_DOWN       TRUE

/** Indicates whether the API should flush from the MAC address table
 *  all the addresses associated with:
 *  (1) a VLAN when the VLAN is deleted;
 *  (2) a port/VLAN pair when a port is deleted from the VLAN;
 *  (3) a VLAN when changing the FM_VLAN_FID2_IVL VLAN attribute. */
#define FM_AAK_API_MA_FLUSH_ON_VLAN_CHANGE       "api.ma.flushOnVlanChange"
#define FM_AAT_API_MA_FLUSH_ON_VLAN_CHANGE       FM_API_ATTR_BOOL
#define FM_AAD_API_MA_FLUSH_ON_VLAN_CHANGE       TRUE

/** Indicates whether the API should flush from the MAC address table
 *  all the addresses associated with a port that is being added to or 
 *  deleted from a LAG. */
#define FM_AAK_API_MA_FLUSH_ON_LAG_CHANGE       "api.ma.flushOnLagChange"
#define FM_AAT_API_MA_FLUSH_ON_LAG_CHANGE       FM_API_ATTR_BOOL
#define FM_AAD_API_MA_FLUSH_ON_LAG_CHANGE       TRUE

/** Specifies the maximum number of TCN FIFO entries to be processed in
 *  a single cycle. */
#define FM_AAK_API_MA_TCN_FIFO_BURST_SIZE       "api.ma.tcnFifoBurstSize"
#define FM_AAT_API_MA_TCN_FIFO_BURST_SIZE       FM_API_ATTR_INT
#define FM_AAD_API_MA_TCN_FIFO_BURST_SIZE       512

/** Indicates whether the API should collect VLAN statistics for internal
 *  ports in a switch aggregate. */
#define FM_AAK_API_SWAG_INTERNAL_VLAN_STATS       "api.swag.internalPort.vlanStats"
#define FM_AAT_API_SWAG_INTERNAL_VLAN_STATS       FM_API_ATTR_BOOL
#define FM_AAD_API_SWAG_INTERNAL_VLAN_STATS       FALSE

/** Control whether port attributes should be set on each of the LAG member
 *  ports, as it is with API 2.x, or set on the LAG logical port. See ''Link
 *  Aggregation (LAG) Management'' in the API User Guide for more information.*/
#define FM_AAK_API_PER_LAG_MANAGEMENT               "api.perLagManagement"
#define FM_AAT_API_PER_LAG_MANAGEMENT               FM_API_ATTR_BOOL
#define FM_AAD_API_PER_LAG_MANAGEMENT               TRUE

/** Whether the parity repair thread is enabled. */
#define FM_AAK_API_PARITY_REPAIR_ENABLE         "api.parityRepair.enable"
#define FM_AAT_API_PARITY_REPAIR_ENABLE         FM_API_ATTR_BOOL
#define FM_AAD_API_PARITY_REPAIR_ENABLE         TRUE

/** Controls the maximum number of ACL port sets supported on a SWAG. */
#define FM_AAK_API_SWAG_MAX_ACL_PORT_SETS         "api.SWAG.maxACLPortSets"
#define FM_AAT_API_SWAG_MAX_ACL_PORT_SETS         FM_API_ATTR_INT
#define FM_AAD_API_SWAG_MAX_ACL_PORT_SETS         2048 

/** Controls the maximum number of port sets supported */
#define FM_AAK_API_MAX_PORT_SETS                  "api.portSets.maxPortSets"
#define FM_AAT_API_MAX_PORT_SETS                  FM_API_ATTR_INT
#define FM_AAD_API_MAX_PORT_SETS                  2048 

/** Enable the packet receive thread. This property must be set in
 *  ''fmPlatformInitialize'' for it to be effective. */
#define FM_AAK_API_PACKET_RECEIVE_ENABLE            "api.packetReceive.enable"
#define FM_AAT_API_PACKET_RECEIVE_ENABLE            FM_API_ATTR_BOOL
#define FM_AAD_API_PACKET_RECEIVE_ENABLE            TRUE 

/** Enables legacy multicast group restrictions which only allowed a
 *  single multicast address per group, required that the address be
 *  assigned prior to group activation, and automatically determined
 *  whether to use L2 or L3 hardware resources by examining the multicast
 *  address provided. TRUE means use the old restrictions, FALSE means
 *  use the new features. Default is FALSE. */
#define FM_AAK_API_1_ADDR_PER_MCAST_GROUP           "api.multicast.singleAddress"
#define FM_AAT_API_1_ADDR_PER_MCAST_GROUP           FM_API_ATTR_BOOL
#define FM_AAD_API_1_ADDR_PER_MCAST_GROUP           FALSE

/** Multiple instances of the White Model may be logically connected
 *  to simulate multiple-switch environments. Such an environment can utilize
 *  the API's stacking or SWAG support. Multiple switches may be managed by
 *  a single instance of the API (a single "unit") or each switch may be
 *  managed by its own private instance of the API (multiple "units") or a
 *  combination of both. When using a multiple unit environment, each
 *  instance of the API must be able to identify itself by a unit number.
 *  This property may be set when initializing an API instance to provide 
 *  the unit number. See Multiple Model Environments in the API User
 *  Guide for more information. */
#define FM_AAK_API_PLATFORM_MODEL_POSITION   "api.platform.model.position"
#define FM_AAT_API_PLATFORM_MODEL_POSITION   FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_MODEL_POSITION   0

/** Specifies the number of next-hop records to be reserved for use with
 *  virtual network tunnels. Note that the default value means that no
 *  virtual network tunnels will be supported, so applications that wish
 *  to use virtual networks MUST set the API property to the desired
 *  value. */
#define FM_AAK_API_NUM_VN_TUNNEL_NEXTHOPS         "api.vn.numNextHops"
#define FM_AAT_API_NUM_VN_TUNNEL_NEXTHOPS         FM_API_ATTR_INT
#define FM_AAD_API_NUM_VN_TUNNEL_NEXTHOPS         0

/** Specifies the encapsulation protocol to be used with NVGRE and
 *  Geneve virtual network tunnels. */
#define FM_AAK_API_VN_ENCAP_PROTOCOL              "api.vn.encapProtocol"
#define FM_AAT_API_VN_ENCAP_PROTOCOL               FM_API_ATTR_INT
#define FM_AAD_API_VN_ENCAP_PROTOCOL               0x6558

/** Specifies the encapsulation version to be used with NVGRE and
 *  Geneve virtual network tunnels. */
#define FM_AAK_API_VN_ENCAP_VERSION                "api.vn.encapVersion"
#define FM_AAT_API_VN_ENCAP_VERSION                FM_API_ATTR_INT
#define FM_AAD_API_VN_ENCAP_VERSION                0

/** Specifies whether route lookups by IP address are supported. Some
 *  API subsystems such as virtual networking require this feature, and
 *  will force the API property to TRUE. */
#define FM_AAK_API_SUPPORT_ROUTE_LOOKUPS          "api.routing.supportRouteLookups"
#define FM_AAT_API_SUPPORT_ROUTE_LOOKUPS          FM_API_ATTR_BOOL
#define FM_AAD_API_SUPPORT_ROUTE_LOOKUPS          FALSE

/** Controls whether the routing maintenance thread is installed (default) or
 *  not. The routing maintenance thread monitors the usage of FFU and BST
 *  resources for routing and moves routes. */
#define FM_AAK_API_ROUTING_MAINTENANCE_ENABLE        "api.routing.maintenance.enable"
#define FM_AAT_API_ROUTING_MAINTENANCE_ENABLE        FM_API_ATTR_BOOL
#define FM_AAD_API_ROUTING_MAINTENANCE_ENABLE        TRUE

/** Specifies whether automatic vlan2 tagging updates should take place.
 *  The default value is TRUE and will enable normal operation. Setting
 *  this property to FALSE enables Virtual-Network tunneling to function
 *  properly. The virtual networking subsystem will force this
 *  property to FALSE. */
#define FM_AAK_API_AUTO_VLAN2_TAGGING       "api.vlan.autoVlan2Tagging"
#define FM_AAT_API_AUTO_VLAN2_TAGGING       FM_API_ATTR_BOOL
#define FM_AAD_API_AUTO_VLAN2_TAGGING       TRUE

/** Whether the host system attached to the CPU management port is
 *  allowed to receive broadcast, unicast, and multicast flooding
 *  traffic. The default value is TRUE, which means that the host system
 *  is allowed to receive flooded traffic. If FALSE, the XCast mode of
 *  the CPU management port is set to NONE and the host system will not
 *  receive flooded traffic. Note that this does not control whether the
 *  switch application instance attached to the CPU port will receive
 *  flooding traffic. */
#define FM_AAK_API_CPU_PORT_XCAST_MODE      "api.allowXcastToCpuPortHostSystem"
#define FM_AAT_API_CPU_PORT_XCAST_MODE      FM_API_ATTR_BOOL
#define FM_AAD_API_CPU_PORT_XCAST_MODE      TRUE

/* -------- Add new DOCUMENTED api properties above this line! -------- */

/** @} (end of Doxygen group) */

/************************************************************************
 ****                                                                ****
 ****               END DOCUMENTED API PROPERTIES                    ****
 ****                                                                ****
 ****               BEGIN UNDOCUMENTED PROPERTIES                    ****
 ****                                                                ****
 ************************************************************************/
 
/************************************************************
 * The following properties are not documented in the API
 * User Guide as they are intended for Intel Internal Use
 * only.
 ************************************************************/

/* -------- Add new UNDOCUMENTED api properties below this line! -------- */

#define FM_AAK_DEBUG_BOOT_INTERRUPT_HANDLER         "debug.boot.interruptHandler.disable"
#define FM_AAD_DEBUG_BOOT_INTERRUPT_HANDLER         FALSE

/* Enable the MAC table maintenance thread. Should be adjusted at boot time or
 * left as-is. */
#define FM_AAK_API_MA_TABLE_MAINTENENANCE_ENABLE    "api.maTableMaintenance.enable"
#define FM_AAT_API_MA_TABLE_MAINTENENANCE_ENABLE    FM_API_ATTR_BOOL
#define FM_AAD_API_MA_TABLE_MAINTENENANCE_ENABLE    TRUE

/* Enable the fast maintenance thread. The change only takes affect when
 * called in platform initialization. */
#define FM_AAK_API_FAST_MAINTENANCE_ENABLE          "api.fastMaintenance.enable"
#define FM_AAT_API_FAST_MAINTENANCE_ENABLE          FM_API_ATTR_BOOL
#define FM_AAD_API_FAST_MAINTENANCE_ENABLE          TRUE 

/* Fast maintenance thread period (in ns) defaults to 20 ms.
 * The change only takes affect when called in platform initialization. */
#define FM_AAK_API_FAST_MAINTENANCE_PERIOD          "api.fastMaintenance.period"
#define FM_AAT_API_FAST_MAINTENANCE_PERIOD          FM_API_ATTR_INT
#define FM_AAD_API_FAST_MAINTENANCE_PERIOD          20000000

/* Disable the physical port GLORT LAG Filtering. */
#define FM_AAK_API_STRICT_GLORT_PHYSICAL            "api.strict.glortPhysical"
#define FM_AAT_API_STRICT_GLORT_PHYSICAL            FM_API_ATTR_BOOL
#define FM_AAD_API_STRICT_GLORT_PHYSICAL            TRUE

/* Control whether or not resetting the watermark registers when the 
 * FM_AUTO_PAUSE_MODE is set to FM_DISABLED. */
#define FM_AAK_API_RESET_WATERMARK_AT_PAUSE_OFF     "api.autoPauseOff.resetWatermark"
#define FM_AAT_API_RESET_WATERMARK_AT_PAUSE_OFF     FM_API_ATTR_BOOL
#define FM_AAD_API_RESET_WATERMARK_AT_PAUSE_OFF     TRUE

/* Control whether switch aggregates are to automatically maintain sub-switch
 * state for the application.
 */
#define FM_AAK_API_SWAG_AUTO_SUB_SWITCHES           "api.swag.autoSubSwitches"
#define FM_AAT_API_SWAG_AUTO_SUB_SWITCHES           FM_API_ATTR_BOOL
#define FM_AAD_API_SWAG_AUTO_SUB_SWITCHES           TRUE

/* Control whether switch aggregates are to automatically manage internal
 * (inter-switch) ports for the application.
 */
#define FM_AAK_API_SWAG_AUTO_INTERNAL_PORTS         "api.swag.autoInternalPorts"
#define FM_AAT_API_SWAG_AUTO_INTERNAL_PORTS         FM_API_ATTR_BOOL
#define FM_AAD_API_SWAG_AUTO_INTERNAL_PORTS         TRUE

/* Control whether switch aggregates are to automatically manage Virtual
 * Networking VSI values for switches in the SWAG. If enabled, only VSI
 * values 0-255 are available to the application and the SWAG switch will
 * duplicate VSI settings across all switches in the SWAG. If disabled,
 * the application is responsible for managing VSI usage on each switch in
 * the SWAG, where SWAG-level VSI values 0-255 are assigned to the first switch,
 * values 256-511 are assigned to the second switch, etc.
 */
#define FM_AAK_API_SWAG_AUTO_VN_VSI                 "api.swag.autoVNVsi"
#define FM_AAT_API_SWAG_AUTO_VN_VSI                 FM_API_ATTR_BOOL
#define FM_AAD_API_SWAG_AUTO_VN_VSI                 TRUE

/* Controls whether the platform enables bypass mode on startup.  This only has
 * an effect in platforms that support bypass mode. */
#define FM_AAK_API_PLATFORM_BYPASS_ENABLE   "api.platform.bypassEnable"
#define FM_AAT_API_PLATFORM_BYPASS_ENABLE   FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_BYPASS_ENABLE   FALSE

/* Delete semaphore timeout in seconds. defaults to 5 sec. */
#define FM_AAK_API_LAG_DELETE_SEMAPHORE_TIMEOUT   "api.lag.delSemaphore.timeout"
#define FM_AAT_API_LAG_DELETE_SEMAPHORE_TIMEOUT   FM_API_ATTR_INT
#define FM_AAD_API_LAG_DELETE_SEMAPHORE_TIMEOUT   5

/* The spanning tree state of internal ports is forced to
 * FM_STP_STATE_FORWARDING to ensure that SWAG links are not broken. To 
 * further protect theses links, the API prevents the STP state from being
 * changed on theses ports. Enabling this property will overide this
 * behavior and therefore enable changing the STP state on internal links */
#define FM_AAK_API_STP_ENABLE_INTERNAL_PORT_CTRL    "api.stp.enableInternalPortControl"
#define FM_AAT_API_STP_ENABLE_INTERNAL_PORT_CTRL    FM_API_ATTR_BOOL
#define FM_AAD_API_STP_ENABLE_INTERNAL_PORT_CTRL    FALSE
 
/* Selects the logical port map to use with the White Model. */
#define FM_AAK_API_PLATFORM_MODEL_PORT_MAP_TYPE     "api.platform.model.portMapType"
#define FM_AAT_API_PLATFORM_MODEL_PORT_MAP_TYPE     FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_MODEL_PORT_MAP_TYPE     0

/* Specifies the set of switches that are to be modeled in
 * the whiteModel platform. The set consists of one or more comma separated
 * entries. Each entry is formatted as switchNumber-switchFamily. The
 * switchNumber field specifies the switch number associated with the 
 * entry and should be a non-negative integer. The switchFamily field 
 * specifies the switch family to be modeled and is case-insensitive. */ 
#define FM_AAK_API_PLATFORM_MODEL_SWITCH_TYPE       "api.platform.model.switchType"
#define FM_AAT_API_PLATFORM_MODEL_SWITCH_TYPE       FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_MODEL_SWITCH_TYPE       "0-FM10000"

/* Specifies whether the white model packet queue interface should send
 * ''FM_MODEL_MSG_PACKET_EOT'' messages after each received packet
 * has been fully processed. */
#define FM_AAK_API_PLATFORM_MODEL_SEND_EOT          "api.platform.model.sendEOT"
#define FM_AAT_API_PLATFORM_MODEL_SEND_EOT          FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_MODEL_SEND_EOT          FALSE

/* Specifies whether the white model should put egress information from
 * ''FM_MODEL_MSG_SET_EGRESS_INFO'' messages into models.egressInfo file. */
#define FM_AAK_API_PLATFORM_MODEL_LOG_EGRESS_INFO   "api.platform.model.logEgressInfo"
#define FM_AAT_API_PLATFORM_MODEL_LOG_EGRESS_INFO   FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_MODEL_LOG_EGRESS_INFO   FALSE

/* Specifies whether the IEEE 1588 reference clock is enabled. */
#define FM_AAK_API_PLATFORM_ENABLE_REF_CLOCK        "api.platform.enableRefClock"
#define FM_AAT_API_PLATFORM_ENABLE_REF_CLOCK        FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_ENABLE_REF_CLOCK        TRUE

/* Specifies whether to initialize the IEEE 1588 reference clock
 * to the system time. */
#define FM_AAK_API_PLATFORM_SET_REF_CLOCK           "api.platform.setRefClock"
#define FM_AAT_API_PLATFORM_SET_REF_CLOCK           FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_SET_REF_CLOCK           FALSE

/* Specifies whether to enable priority buffer queues for prioritized buffer allocation
 * and receive event delivery. */
#define FM_AAK_API_PLATFORM_PRIORITY_BUFFER_QUEUES  "api.platform.priorityBufferQueues"
#define FM_AAT_API_PLATFORM_PRIORITY_BUFFER_QUEUES  FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_PRIORITY_BUFFER_QUEUES  FALSE

/* Specifies the type of packet scheduling to application. See 'fm_pktScheduleType'
 * for the type of packet scheduling available */
#define FM_AAK_API_PLATFORM_PKT_SCHED_TYPE           "api.platform.pktSchedType"
#define FM_AAT_API_PLATFORM_PKT_SCHED_TYPE           FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_PKT_SCHED_TYPE           0

/* Specifies whether to enable separate buffer pool */
#define FM_AAK_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE  "api.platform.separateBufferPoolEnable"
#define FM_AAT_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE  FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE  FALSE

/* Specifies number of RX buffers to be allocated */
#define FM_AAK_API_PLATFORM_NUM_BUFFERS_RX               "api.platform.numBuffersRx"
#define FM_AAT_API_PLATFORM_NUM_BUFFERS_RX               FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_NUM_BUFFERS_RX               -1

/* Specifies number of TX buffers to be allocated */
#define FM_AAK_API_PLATFORM_NUM_BUFFERS_TX               "api.platform.numBuffersTx"
#define FM_AAT_API_PLATFORM_NUM_BUFFERS_TX               FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_NUM_BUFFERS_TX               -1

/* Specifies the name of the network interface that should be used by
 * the white model to send and receive packets through the CPU port.
 * The default is send/receive through a white model bypass. */
#define FM_AAK_API_PLATFORM_MODEL_PKT_INTERFACE     "api.platform.model.pktInterface"
#define FM_AAT_API_PLATFORM_MODEL_PKT_INTERFACE     FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_MODEL_PKT_INTERFACE     "model-bypass"

/* Specifies the method of receiving or injecting a packet into the fabric.
 * 'raw' to use the raw packet socket handling interface.  
 * 'pti' to use the PTI (Packet Test Interface) to inject or receive packets 
 * via the FIBM port. */
#define FM_AAK_API_PLATFORM_PKT_INTERFACE       "api.platform.pktInterface"
#define FM_AAT_API_PLATFORM_PKT_INTERFACE       FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_PKT_INTERFACE       "raw"

/* Specifies the multi-switch topology to use for a multi-node/multi-switch
 * white model platform. Default is empty, meaning single node/single switch
 * environment. */
#define FM_AAK_API_PLATFORM_MODEL_TOPOLOGY_NAME     "api.platform.model.topology.name"
#define FM_AAT_API_PLATFORM_MODEL_TOPOLOGY_NAME     FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_MODEL_TOPOLOGY_NAME     ""

/* Specifies whether to use the model file path when opening the multi-switch
 * topology file. */
#define FM_AAK_API_PLATFORM_MODEL_TOPOLOGY_USE_MODEL_PATH  "api.platform.model.topology.file.useModelPath"
#define FM_AAT_API_PLATFORM_MODEL_TOPOLOGY_USE_MODEL_PATH  FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_MODEL_TOPOLOGY_USE_MODEL_PATH  TRUE

/* Specifies SERDES development board IP address. */
#define FM_AAK_API_PLATFORM_MODEL_DEV_BOARD_IP      "api.platform.model.devBoardIp"
#define FM_AAT_API_PLATFORM_MODEL_DEV_BOARD_IP      FM_API_ATTR_TEXT
#define FM_AAD_API_PLATFORM_MODEL_DEV_BOARD_IP      ""

/* Specifies SERDES development board TCP port. */
#define FM_AAK_API_PLATFORM_MODEL_DEV_BOARD_PORT    "api.platform.model.devBoardPort"
#define FM_AAT_API_PLATFORM_MODEL_DEV_BOARD_PORT    FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_MODEL_DEV_BOARD_PORT    90

/* Specifies the value of the DEVICE_CFG register for the White Model (FM10k) */
#define FM_AAK_API_PLATFORM_MODEL_DEVICE_CFG        "api.platform.model.deviceCfg"
#define FM_AAT_API_PLATFORM_MODEL_DEVICE_CFG        FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_MODEL_DEVICE_CFG        0x0000FF80

/* Specifies the value of the CHIP_VERSION register for the White Model. */
#define FM_AAK_API_PLATFORM_MODEL_CHIP_VERSION      "api.platform.model.chipVersion"
#define FM_AAT_API_PLATFORM_MODEL_CHIP_VERSION      FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_MODEL_CHIP_VERSION      0

/* Specifies the TCP port where the SERDES SBUS Server is listening on.
 * A non-zero value will enable the SBUS Server thread. This simulates
 * the aacs server for aapl utilities program to control the SERDES. */
#define FM_AAK_API_PLATFORM_SBUS_SERVER_PORT       "api.platform.sbusServer.tcpPort"
#define FM_AAT_API_PLATFORM_SBUS_SERVER_PORT       FM_API_ATTR_INT
#define FM_AAD_API_PLATFORM_SBUS_SERVER_PORT       0

/* Indicates whether the API is running on top of the White Model. */
#define FM_AAK_API_PLATFORM_IS_WHITE_MODEL          "api.platform.isWhiteModel"
#define FM_AAT_API_PLATFORM_IS_WHITE_MODEL          FM_API_ATTR_BOOL
#define FM_AAD_API_PLATFORM_IS_WHITE_MODEL          FALSE

/*  Indicates which categories (comma separated list) to enable when 
 *  API is initializing.
 *  e.g. for "FM_LOG_CAT_SWITCH | FM_LOG_CAT_PORT | FM_LOG_CAT_VLAN"
 *  "switch,port,vlan" */ 
#define FM_AAK_API_DEBUG_INIT_LOGGING_CAT               "api.debug.initLoggingCategories"
#define FM_AAT_API_DEBUG_INIT_LOGGING_CAT               FM_API_ATTR_TEXT
#define FM_AAD_API_DEBUG_INIT_LOGGING_CAT               ""

/* Indicates whether the PEP ports should be added to flooding lists 
 * at the init time by default. */
#define FM_AAK_API_PORT_ADD_PEPS_TO_FLOODING            "api.port.addPepsToFlooding"
#define FM_AAT_API_PORT_ADD_PEPS_TO_FLOODING            FM_API_ATTR_BOOL
#define FM_AAD_API_PORT_ADD_PEPS_TO_FLOODING            FALSE

/* Indicates if VLAN tagging is allowed for FTAG enabled ports. Use of this
 * property is not recommended. Note that VLAN tagged packets egressing PCIe
 * ports can be interpreted by a host system as double VLAN tagged, since these
 * packets carry VLAN information also in the FTAG. */
#define FM_AAK_API_PORT_ALLOW_FTAG_VLAN_TAGGING         "api.port.allowFtagVlanTagging"
#define FM_AAT_API_PORT_ALLOW_FTAG_VLAN_TAGGING         FM_API_ATTR_BOOL
#define FM_AAD_API_PORT_ALLOW_FTAG_VLAN_TAGGING         FALSE

/* Indicates whether to ignore scheduler bandwidth violation */
#define FM_AAK_API_SCH_IGNORE_BW_VIOLATION              "api.scheduler.ignoreBwViolation"
#define FM_AAT_API_SCH_IGNORE_BW_VIOLATION              FM_API_ATTR_BOOL
#define FM_AAD_API_SCH_IGNORE_BW_VIOLATION              FALSE

/* Indicates whether to ignore scheduler bandwidth violation
 * and do not print error and warning messages. Only for
 * serdes debugging */
#define FM_AAK_API_SCH_IGNORE_BW_VIOLATION_NO_WARNING   "api.scheduler.ignoreBwViolationNoWarning"
#define FM_AAT_API_SCH_IGNORE_BW_VIOLATION_NO_WARNING   FM_API_ATTR_BOOL
#define FM_AAD_API_SCH_IGNORE_BW_VIOLATION_NO_WARNING   FALSE

/* Indicates if SerDes Error Validation action execution is enabled
 * for SerDes Up states.
 * SerDes considered in Up state when not in 
 * DISABLED, CONFIGURED and POWERED-UP states. 
 */
#define FM_AAK_API_SERDES_ACTION_IN_UP_STATE            "api.serdes.actionUpState"
#define FM_AAT_API_SERDES_ACTION_IN_UP_STATE            FM_API_ATTR_BOOL
#define FM_AAD_API_SERDES_ACTION_IN_UP_STATE            FALSE

/* Indicates whether to early link up mode is enabled or not */
#define FM_AAK_API_DFE_ALLOW_EARLY_LINK_UP_MODE         "api.dfe.allowEarlyLinkUp"
#define FM_AAT_API_DFE_ALLOW_EARLY_LINK_UP_MODE         FM_API_ATTR_BOOL
#define FM_AAD_API_DFE_ALLOW_EARLY_LINK_UP_MODE         TRUE

/* Indicates whether to pCal on KR modes is enabled or not */
#define FM_AAK_API_DFE_ALLOW_KR_PCAL_MODE               "api.dfe.allowKrPcal"
#define FM_AAT_API_DFE_ALLOW_KR_PCAL_MODE               FM_API_ATTR_BOOL
#define FM_AAD_API_DFE_ALLOW_KR_PCAL_MODE               TRUE

/* Indicates whether signalOk debouncing is enabled or not */
#define FM_AAK_API_DFE_ENABLE_SIGNALOK_DEBOUNCING       "api.dfe.enableSigOkDebounce"
#define FM_AAT_API_DFE_ENABLE_SIGNALOK_DEBOUNCING       FM_API_ATTR_BOOL
#define FM_AAD_API_DFE_ENABLE_SIGNALOK_DEBOUNCING       TRUE

/* Indicates whether port status polling is enabled or not */
#define FM_AAK_API_PORT_ENABLE_STATUS_POLLING           "api.port.enableStatusPolling"
#define FM_AAT_API_PORT_ENABLE_STATUS_POLLING           FM_API_ATTR_BOOL
#define FM_AAD_API_PORT_ENABLE_STATUS_POLLING           TRUE

/* Indicates GSME timestamping mode. Valid values are: */
/* 0: system up time                                   */
/* 1: absolute time                                    */
/* 2: time since last history buffer clear             */
#define FM_AAK_API_GSME_TIMESTAMP_MODE                 "api.gsme.timestampMode"
#define FM_AAT_API_GSME_TIMESTAMP_MODE                 FM_API_ATTR_INT
#define FM_AAD_API_GSME_TIMESTAMP_MODE                 0

/* Indicates whether API handles multicast groups created on HNI request 
 * as flooding ones. This allows to flood multicast frames with given
 * MAC to virtual ports (mcast frames filtering).  */
#define FM_AAK_API_MULTICAST_HNI_FLOODING              "api.multicast.hni.flooding"
#define FM_AAT_API_MULTICAST_HNI_FLOODING              FM_API_ATTR_BOOL
#define FM_AAD_API_MULTICAST_HNI_FLOODING              TRUE

/* Specifies maximum number of MAC table entries per PEP port added 
 * on driver demand. */
#define FM_AAK_API_HNI_MAC_ENTRIES_PER_PEP       "api.hni.macEntriesPerPep"
#define FM_AAT_API_HNI_MAC_ENTRIES_PER_PEP       FM_API_ATTR_INT
#define FM_AAD_API_HNI_MAC_ENTRIES_PER_PEP       1024

/* Specifies maximum number of MAC table entries per virtual port added 
 * on driver demand. */
#define FM_AAK_API_HNI_MAC_ENTRIES_PER_PORT      "api.hni.macEntriesPerPort"
#define FM_AAT_API_HNI_MAC_ENTRIES_PER_PORT      FM_API_ATTR_INT
#define FM_AAD_API_HNI_MAC_ENTRIES_PER_PORT      64

/* Specifies maximum number of inner/outer MAC filtering entries 
 * per PEP port added on driver demand. */
#define FM_AAK_API_HNI_INN_OUT_ENTRIES_PER_PEP   "api.hni.innOutEntriesPerPep"
#define FM_AAT_API_HNI_INN_OUT_ENTRIES_PER_PEP   FM_API_ATTR_INT
#define FM_AAD_API_HNI_INN_OUT_ENTRIES_PER_PEP   1024

/* Specifies maximum number of inner/outer MAC filtering entries 
 * per virtual port added on driver demand. */
#define FM_AAK_API_HNI_INN_OUT_ENTRIES_PER_PORT  "api.hni.innOutEntriesPerPort"
#define FM_AAT_API_HNI_INN_OUT_ENTRIES_PER_PORT  FM_API_ATTR_INT
#define FM_AAD_API_HNI_INN_OUT_ENTRIES_PER_PORT  64

/* The number of GloRTs per PEP port added on driver demand. The value
 * must be a power of two. */
#define FM_AAK_API_HNI_GLORTS_PER_PEP       "api.hni.glortsPerPep"
#define FM_AAT_API_HNI_GLORTS_PER_PEP       FM_API_ATTR_INT
#define FM_AAD_API_HNI_GLORTS_PER_PEP       1024

/* Indicates whether it is possible to use values out of the valid range
 * for FM_PORT_AUTONEG_LINK_INHB_TIMER and FM_PORT_AUTONEG_LINK_INHB_TIMER_KX*/
#define FM_AAK_API_AN_INHBT_TIMER_ALLOW_OUT_OF_SPEC     "api.an.timerAllowOutSpec"
#define FM_AAT_API_AN_INHBT_TIMER_ALLOW_OUT_OF_SPEC     FM_API_ATTR_BOOL
#define FM_AAD_API_AN_INHBT_TIMER_ALLOW_OUT_OF_SPEC     FALSE

/* Indicates the periodic timer value in seconds the SerDes and SBus Master
 * is validated for unrecoverable errors.
 * Single timer is used for SerDes and SBus Master Validation. 
 * Valid value range 60 - 3600. */
#define FM_AAK_API_SERDES_VALIDATE_TIMER            "api.serdes.validateTimer"
#define FM_AAT_API_SERDES_VALIDATE_TIMER            FM_API_ATTR_INT
#define FM_AAD_API_SERDES_VALIDATE_TIMER            360

/* Whether SerDes Error Handling is enabled. */
#define FM_AAK_API_SERDES_VALIDATE                  "api.serdes.validate"
#define FM_AAT_API_SERDES_VALIDATE                  FM_API_ATTR_BOOL
#define FM_AAD_API_SERDES_VALIDATE                  FALSE

/* Whether SBus Master Error Handling is enabled. */
#define FM_AAK_API_SBM_VALIDATE                     "api.sbmaster.validate"
#define FM_AAT_API_SBM_VALIDATE                     FM_API_ATTR_BOOL
#define FM_AAD_API_SBM_VALIDATE                     FALSE

/* Specifies maximum number of flow entries per VF added on driver demand. */
#define FM_AAK_API_HNI_FLOW_ENTRIES_VF           "api.hni.flowEntriesPerVf"
#define FM_AAT_API_HNI_FLOW_ENTRIES_VF           FM_API_ATTR_INT
#define FM_AAD_API_HNI_FLOW_ENTRIES_VF           64

/************************************************************************
 ****                                                                ****
 ****              END UNDOCUMENTED API PROPERTIES                   ****
 ****                                                                ****
 ************************************************************************/


 
fm_status fmInitializeApiProperties(void);

fm_status fmSetApiProperty(fm_text        key,
                           fm_apiAttrType attrType,
                           void *         value);

fm_status fmGetApiProperty(fm_text        key,
                           fm_apiAttrType attrType,
                           void *         value);


/* Shortcuts for getting properties of a given type. */
fm_int fmGetIntApiProperty(fm_text key, fm_int defaultValue);
fm_bool fmGetBoolApiProperty(fm_text key, fm_bool defaultValue);
fm_float fmGetFloatApiProperty(fm_text key, fm_float defaultValue);
fm_text fmGetTextApiProperty(fm_text key, fm_text defaultValue);

void fmDbgDumpApiProperties(void);


/****************************************************
 * The following wrappers permit existing code to 
 * continue to use the legacy function names. We 
 * use these instead of macro synonyms to make them
 * visible to SWIG.
 ****************************************************/ 

fm_status fmSetApiAttribute(fm_text        key,
                            fm_apiAttrType attrType,
                            void *         value);

fm_status fmGetApiAttribute(fm_text        key,
                            fm_apiAttrType attrType,
                            void *         value);


/****************************************************
 * The following synonyms are not documented, 
 * but are used internally by legacy platform code.
 ****************************************************/ 

/* A legacy synonym for ''fmGetIntApiProperty''. */
#define fmGetIntApiAttribute(key, defaultValue) \
        fmGetIntApiProperty(key, defaultValue)

/* A legacy synonym for ''fmGetBoolApiProperty''. */
#define fmGetBoolApiAttribute(key, defaultValue) \
        fmGetBoolApiProperty(key, defaultValue)

/* A legacy synonym for ''fmGetFloatApiProperty''. */
#define fmGetFloatApiAttribute(key, defaultValue) \
        fmGetFloatApiProperty(key, defaultValue)

/* A legacy synonym for ''fmGetTextApiProperty''. */
#define fmGetTextApiAttribute(key, defaultValue) \
        fmGetTextApiProperty(key, defaultValue)


#endif /* __FM_FM_PROPERTY_H */
