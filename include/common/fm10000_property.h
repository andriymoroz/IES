/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_property.h
 * Creation Date:   June 16, 2014
 * Description:     FM10000-specific API properties.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_PROPERTY_H
#define __FM_FM10000_PROPERTY_H

/************************************************************************
 ****                                                                ****
 ****               BEGIN DOCUMENTED PROPERTIES                      ****
 ****                                                                ****
 ************************************************************************/
 
/** \ingroup apiAttrib10
 * @{ */

/** Selects a watermark methodology for FM10000 devices. Valid values are:
 *                                                                      \lb\lb
 *  "disabled" - Watermarks are disabled (except for Global).
 *                                                                      \lb\lb
 *  "lossy" - All memory is dedicated to a lossy watermark configuration.
 *                                                                      \lb\lb
 *  "lossy_lossless" - Memory is split between lossy and lossless 
 *  configurations.
 *                                                                      \lb\lb
 *  "lossless" - All memory is dedicated to a lossless watermark 
 *  configuration. */
#define FM_AAK_API_FM10000_WMSELECT                  "api.FM10000.wmSelect"
#define FM_AAT_API_FM10000_WMSELECT                  FM_API_ATTR_TEXT
#define FM_AAD_API_FM10000_WMSELECT                  "lossy"

/** Specifies the amount of memory (in bytes) to allocate to each port for
 *  RX private (CM_RX_SMP_PRIVATE_WM register). */
#define FM_AAK_API_FM10000_CM_RX_SMP_PRIVATE_BYTES  "api.FM10000.cmRxSmpPrivateBytes"
#define FM_AAT_API_FM10000_CM_RX_SMP_PRIVATE_BYTES  FM_API_ATTR_INT
#define FM_AAD_API_FM10000_CM_RX_SMP_PRIVATE_BYTES  14336

/** Specifies the amount of memory (in bytes) to allocate to each port for
 *  the TX hog watermark. This value will be limited during automatic
 *  watermark calculation to minimize the occurrence of the scenario
 *  when all TX queue entries are consumed. */
#define FM_AAK_API_FM10000_CM_TX_TC_HOG_BYTES      "api.FM10000.cmTxTcHogBytes"
#define FM_AAT_API_FM10000_CM_TX_TC_HOG_BYTES      FM_API_ATTR_INT
#define FM_AAD_API_FM10000_CM_TX_TC_HOG_BYTES      2097152

/** Specifies the congestion management SMP soft drop versus hog watermark
 *  delta to be used in the "lossy" and "lossy_lossless" watermark schemes 
 *  (specified by ''api.FM10000.wmSelect'').  The delta is specified as a
 *  percentage of the soft drop hog watermark. */
#define FM_AAK_API_FM10000_CM_SMP_SD_VS_HOG_PERCENT "api.FM10000.cmSmpSdVsHogPercent"
#define FM_AAT_API_FM10000_CM_SMP_SD_VS_HOG_PERCENT FM_API_ATTR_INT
#define FM_AAD_API_FM10000_CM_SMP_SD_VS_HOG_PERCENT 80

/** Specifies the number of jitter bits to be used in the congestion management
 *  soft drop algorithm for the "lossy" and "lossy_lossless" watermark schemes
 *  (specified by ''api.FM10000.wmSelect'').  Valid values range from 0 (jitter
 *  disabled) to 7 (the maximum amount of jitter). */
#define FM_AAK_API_FM10000_CM_SMP_SD_JITTER_BITS    "api.FM10000.cmSmpSdJitterBits"
#define FM_AAT_API_FM10000_CM_SMP_SD_JITTER_BITS    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_CM_SMP_SD_JITTER_BITS    7

/** Specifies whether congestion management should apply a random soft drop
 *  when TX private is exceeded. */
#define FM_AAK_API_FM10000_CM_TX_SD_ON_PRIVATE       "api.FM10000.cmTxSdOnPrivate"
#define FM_AAT_API_FM10000_CM_TX_SD_ON_PRIVATE       FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_CM_TX_SD_ON_PRIVATE       FALSE

/** Specifies whether congestion management should apply a random soft drop 
 *  based on total SMP usage. */
#define FM_AAK_API_FM10000_CM_TX_SD_ON_SMP_FREE     "api.FM10000.cmTxSdOnSmpFree"
#define FM_AAT_API_FM10000_CM_TX_SD_ON_SMP_FREE     FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_CM_TX_SD_ON_SMP_FREE     TRUE

/** Specifies the amount of memory (in bytes) to reserve for pause frame
 * response time across a link.  This is used for the "lossless" and
 * "lossy_lossless" watermark scheme (specified by ''api.FM10000.wmSelect''). */
#define FM_AAK_API_FM10000_CM_PAUSE_BUFFER_BYTES     "api.FM10000.cmPauseBufferBytes"
#define FM_AAT_API_FM10000_CM_PAUSE_BUFFER_BYTES     FM_API_ATTR_INT
#define FM_AAD_API_FM10000_CM_PAUSE_BUFFER_BYTES     30720

/** The number of destination entries associated with each GLORT CAM
 *  entry for multicast groups. The value must be a multiple of 64 with
 *  a maximum value of 256. */
#define FM_AAK_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM    "api.FM10000.mcastMaxEntriesPerCam"
#define FM_AAT_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM    FM_MCG_MAX_ENTRIES_PER_GLORT

/** The first FFU slice on the FM10000 allocated for unicast routing. The value
 *  of this property must be less than or equal to the value for
 *  ''api.FM10000.ffu.unicastSliceRangeLast''.
 *  This property has been deprecated in favor of the FM_FFU_SLICE_ALLOCATIONS
 *  switch attribute. */
#define FM_AAK_API_FM10000_FFU_UNICAST_SLICE_1ST     "api.FM10000.ffu.unicastSliceRangeFirst"
#define FM_AAT_API_FM10000_FFU_UNICAST_SLICE_1ST     FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_UNICAST_SLICE_1ST     0

/** The last FFU slice on the FM10000 allocated for unicast routing. The value
 *  of this property must be greater than or equal to the value for
 *  ''api.FM10000.ffu.unicastSliceRangeFirst'' and less than the value for
 *  ''api.FM10000.ffu.multicastSliceRangeLast''.
 *  This property has been deprecated in favor of the FM_FFU_SLICE_ALLOCATIONS
 *  switch attribute. */
#define FM_AAK_API_FM10000_FFU_UNICAST_SLICE_LAST    "api.FM10000.ffu.unicastSliceRangeLast"
#define FM_AAT_API_FM10000_FFU_UNICAST_SLICE_LAST    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_UNICAST_SLICE_LAST    15

/** The first FFU slice on the FM10000 allocated for multicast routing. The
 *  value of this property must be less than or equal to the value for
 *  ''api.FM10000.ffu.multicastSliceRangeLast'' and greater than or equal to the
 *  value for ''api.FM10000.ffu.unicastSliceRangeFirst''.
 *  This property has been deprecated in favor of the FM_FFU_SLICE_ALLOCATIONS
 *  switch attribute. */
#define FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_1ST   "api.FM10000.ffu.multicastSliceRangeFirst"
#define FM_AAT_API_FM10000_FFU_MULTICAST_SLICE_1ST   FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_1ST   0

/** The last FFU slice on the FM10000 allocated for multicast routing. The
 *  value of this property must be greater than or equal to the value for
 *  ''api.FM10000.ffu.multicastSliceRangeFirst''.
 *  This property has been deprecated in favor of the FM_FFU_SLICE_ALLOCATIONS
 *  switch attribute. */
#define FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_LAST  "api.FM10000.ffu.multicastSliceRangeLast"
#define FM_AAT_API_FM10000_FFU_MULTICAST_SLICE_LAST  FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_LAST  15

/** The first FFU slice on the FM10000 allocated for ACLs.
 *  This property has been deprecated in favor of the FM_FFU_SLICE_ALLOCATIONS
 *  switch attribute. */
#define FM_AAK_API_FM10000_FFU_ACL_SLICE_1ST         "api.FM10000.ffu.aclSliceRangeFirst"
#define FM_AAT_API_FM10000_FFU_ACL_SLICE_1ST         FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_ACL_SLICE_1ST         16

/** The last FFU slice on the FM10000 allocated for ACLs.
 *  This property has been deprecated in favor of the FM_FFU_SLICE_ALLOCATIONS
 *  switch attribute. */
#define FM_AAK_API_FM10000_FFU_ACL_SLICE_LAST        "api.FM10000.ffu.aclSliceRangeLast"
#define FM_AAT_API_FM10000_FFU_ACL_SLICE_LAST        FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_ACL_SLICE_LAST        31

/** The number of entries in the MAC Mapper that are reserved for router
 *  MAC addresses including all the virtual router MAC addresses. The value 
 *  of this property must be greater than or equal to 1 if routing is 
 *  enabled. The maximum valid value of this property is 14. */
#define FM_AAK_API_FM10000_FFU_MAPMAC_ROUTING        "api.FM10000.ffu.mapMAC.reservedForRouting"
#define FM_AAT_API_FM10000_FFU_MAPMAC_ROUTING        FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_MAPMAC_ROUTING        8

/** The minimum precedence value allowed for unicast FFU entries. The value of
 *  this property must be less than or equal to the value for
 *  ''api.FM10000.ffu.unicastPrecedenceMax''. This property will be used as the
 *  default value for unicast FFU entries. */
#define FM_AAK_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN    "api.FM10000.ffu.unicastPrecedenceMin"
#define FM_AAT_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN    1

/** The maximum precedence value allowed for unicast FFU entries. The value
 *  of this property must be greater than or equal to the value for
 *  ''api.FM10000.ffu.unicastPrecedenceMin''. */
#define FM_AAK_API_FM10000_FFU_UNICAST_PRECEDENCE_MAX    "api.FM10000.ffu.unicastPrecedenceMax"
#define FM_AAT_API_FM10000_FFU_UNICAST_PRECEDENCE_MAX    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_UNICAST_PRECEDENCE_MAX    3

/** The minimum precedence value allowed for multicast FFU entries. The value
 *  of this property must be less than or equal to the value for
 *  ''api.FM10000.ffu.multicastPrecedenceMax''. This property will be used as
 *  the default value for multicast FFU entries. */
#define FM_AAK_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN    "api.FM10000.ffu.multicastPrecedenceMin"
#define FM_AAT_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN    1

/** The maximum precedence value allowed for multicast FFU entries. The value
 *  of this property must be greater than or equal to the value for
 *  ''api.FM10000.ffu.multicastPrecedenceMin''. */
#define FM_AAK_API_FM10000_FFU_MULTICAST_PRECEDENCE_MAX    "api.FM10000.ffu.multicastPrecedenceMax"
#define FM_AAT_API_FM10000_FFU_MULTICAST_PRECEDENCE_MAX    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_MULTICAST_PRECEDENCE_MAX    3

/** The minimum precedence value allowed for ACL rules. The value of this
 *  property must be less than or equal to the value for
 *  ''api.FM10000.ffu.ACLPrecedenceMax''. This property will be used as the
 *  default value for ACL entries. */
#define FM_AAK_API_FM10000_FFU_ACL_PRECEDENCE_MIN    "api.FM10000.ffu.ACLPrecedenceMin"
#define FM_AAT_API_FM10000_FFU_ACL_PRECEDENCE_MIN    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_ACL_PRECEDENCE_MIN    1

/** The maximum precedence value allowed for ACL rules. The value of this
 *  property must be greater than or equal to the value for
 *  ''api.FM10000.ffu.ACLPrecedenceMin''. FM10000 only supports 4 precedence
 *  level (0..3). */
#define FM_AAK_API_FM10000_FFU_ACL_PRECEDENCE_MAX    "api.FM10000.ffu.ACLPrecedenceMax"
#define FM_AAT_API_FM10000_FFU_ACL_PRECEDENCE_MAX    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FFU_ACL_PRECEDENCE_MAX    3

/** Enable strict counter and policer validation, permitting a policer bank to 
 *  be linked to set of mutually exclusive ACL/Scenario tuple. If disabled,
 *  every ACL would be able to use the count or police action, however,
 *  only the highest precedence rule will be the one that counts or
 *  polices if multiple ACLs hit in parallel. */
#define FM_AAK_API_FM10000_FFU_ACL_STRICT_COUNT_POLICE "api.FM10000.ffu.ACLStrictCountPolice"
#define FM_AAT_API_FM10000_FFU_ACL_STRICT_COUNT_POLICE FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_FFU_ACL_STRICT_COUNT_POLICE TRUE

/** Whether to initialize unicast flooding triggers at boot time. */
#define FM_AAK_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS "api.FM10000.initUcastFloodingTriggers"
#define FM_AAT_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS FALSE

/** Whether to initialize multicast flooding triggers at boot time. */
#define FM_AAK_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS "api.FM10000.initMcastFloodingTriggers"
#define FM_AAT_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS FALSE

/** Whether to initialize broadcast flooding triggers at boot time. */
#define FM_AAK_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS "api.FM10000.initBcastFloodingTriggers"
#define FM_AAT_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS FALSE

/** Whether to initialize reserved MAC control triggers at boot
 *  time. */
#define FM_AAK_API_FM10000_INIT_RESERVED_MAC_TRIGGERS "api.FM10000.initReservedMacTriggers"
#define FM_AAT_API_FM10000_INIT_RESERVED_MAC_TRIGGERS FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_INIT_RESERVED_MAC_TRIGGERS TRUE

/** Force the switch priority of packets trapped using the
 *  FM_PORT_MCAST_FLOODING or FM_PORT_UCAST_FLOODING port attribute.
 *  By default, trapped frames will keep whatever switch priority they are
 *  assigned on ingress, until overridden with this property. */
#define FM_AAK_API_FM10000_FLOODING_TRAP_PRIORITY "api.FM10000.floodingTrapPriority"
#define FM_AAT_API_FM10000_FLOODING_TRAP_PRIORITY FM_API_ATTR_INT
#define FM_AAD_API_FM10000_FLOODING_TRAP_PRIORITY -1

/** Controls whether auto-negotiation events are reported to the application
 *  in a FM_EVENT_PORT event for the FM10000. */
#define FM_AAK_API_FM10000_AUTONEG_GENERATE_EVENTS   "api.FM10000.autoNeg.generateEvents"
#define FM_AAT_API_FM10000_AUTONEG_GENERATE_EVENTS   FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_AUTONEG_GENERATE_EVENTS   FALSE

/** Whether the link status is dependent on the DFE Tuning process
 *  having completed (successfully or not) on a given port. Note that
 *  when this property is set to TRUE and a DFE Mode configuration
 *  change occurs that makes the DFE process restart, a
 *  FM_PORT_STATUS_LINK_DOWN event will be generated until DFE
 *  completes again. */
#define FM_AAK_API_FM10000_LINK_DEPENDS_ON_DFE     "api.FM10000.linkDependsOnDfe"
#define FM_AAT_API_FM10000_LINK_DEPENDS_ON_DFE     FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_LINK_DEPENDS_ON_DFE     TRUE

/** Whether to use shared encapsulation flows during VN tunneling. Shared
 *  flows conserve Tunneling Engine space but increase latency. */
#define FM_AAK_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS "api.FM10000.vn.useSharedEncapFlows"
#define FM_AAT_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS FALSE

/** The maximum number of remote addresses the Virtual Networking API should
 *  support. Note that this value applies to each type of remote address
 *  separately. */
#define FM_AAK_API_FM10000_VN_MAX_TUNNEL_RULES  "api.FM10000.vn.maxRemoteAddresses"
#define FM_AAT_API_FM10000_VN_MAX_TUNNEL_RULES  FM_API_ATTR_INT
#define FM_AAD_API_FM10000_VN_MAX_TUNNEL_RULES  16384

/** The desired hash size for VN tunnel groups. This consumes entries in the
 *  Tunneling Engine lookup hash space. The default value allows for 8 equal
 *  areas of 4K entries each. */
#define FM_AAK_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE  "api.FM10000.vn.tunnelGroupHashSize"
#define FM_AAT_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE  FM_API_ATTR_INT
#define FM_AAD_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE  4096

/** The VLAN Id that will be used for all traffic coming from the
 *  encapsulation tunnel engine back into the switch for further routing. */
#define FM_AAK_API_FM10000_VN_TE_VID "api.FM10000.vn.teVID"
#define FM_AAT_API_FM10000_VN_TE_VID FM_API_ATTR_INT
#define FM_AAD_API_FM10000_VN_TE_VID 4093

/** The ACL rule that will be used for VN Encapsulation rules. */
#define FM_AAK_API_FM10000_VN_ENCAP_ACL_NUM "api.FM10000.vn.encapAclNumber"
#define FM_AAT_API_FM10000_VN_ENCAP_ACL_NUM FM_API_ATTR_INT
#define FM_AAD_API_FM10000_VN_ENCAP_ACL_NUM 23000000

/** The ACL rule that will be used for VN Dncapsulation rules. */
#define FM_AAK_API_FM10000_VN_DECAP_ACL_NUM "api.FM10000.vn.decapAclNumber"
#define FM_AAT_API_FM10000_VN_DECAP_ACL_NUM FM_API_ATTR_INT
#define FM_AAD_API_FM10000_VN_DECAP_ACL_NUM 22000000

/** The number of Multicast Groups that will be available stack-global 
 *  in a default multicast groups glort configuration. The glorts for these
 *  multicast groups will be available at the beginning of the default global
 *  multicast groups glort space. The handles for these multicast groups must be
 *  allocated with a call to fmAllocateStackMcastGroups.
 *  In a default configuration other multicast groups will be available for the
 *  local switch only. The handles for these multicast groups will be generated
 *  automatically. The glorts for these local multical groups will be
 *  automatically taken at the end of the default global multicast glort space.
 *  \ingroup intConstSystem */
#define FM_AAK_API_FM10000_MCAST_NUM_STACK_GROUPS "api.FM10000.mcast.numStackGroups"
#define FM_AAT_API_FM10000_MCAST_NUM_STACK_GROUPS FM_API_ATTR_INT
#define FM_AAD_API_FM10000_MCAST_NUM_STACK_GROUPS 0

/** Whether to remove TE ports from internal port masks. Intel's SWAG
 *  implementation handles all frame encapsulation and decapsulation in the
 *  ingress switch. Thus, frames that traverse internal links to another
 *  switch in the SWAG should never go to the TE on those switches for
 *  encapsulation or decapsulation. In particular, failure to enforce this
 *  will cause multicast frames to be encapsulated multiple times. */
#define FM_AAK_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS    "api.FM10000.vn.TunnelOnlyOnIngress"
#define FM_AAT_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS    FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS    TRUE

/** The watermark for MTable garbage collection, expressed in
 *  percentage of the available space in the table. */
#define FM_AAK_API_FM10000_MTABLE_CLEANUP_WATERMARK     "api.FM10000.mtable.cleanupWatermark"
#define FM_AAT_API_FM10000_MTABLE_CLEANUP_WATERMARK     FM_API_ATTR_INT
#define FM_AAD_API_FM10000_MTABLE_CLEANUP_WATERMARK     50

/** The mode in which the scheduler should operate:
 *                                                                      \lb\lb
 *  "static" (FM10000_SCHED_MODE_STATIC). In this mode, the scheduler
 *  bandwidth is determined at init based on the
 *  api.platform.config.switch.%d.portIndex.%d.speed properties
 *  specified in the Liberty Trail config file. After initialization, it
 *  is only possible to reallocate that same bandwidth amongst ports
 *  within the same QPC (Quad Port Channel). For example, a 100G capable
 *  EPL port (that was assigned 100G) can be split into 4x25G. If the
 *  port is powered down, or put into disabled state, the scheduler BW
 *  (100G) remains reserved for that EPL port.
 *                                                                      \lb\lb
 *  "dynamic" (FM10000_SCHED_MODE_DYNAMIC). In this mode, the scheduler
 *  bandwidth is determined at run-time (dynamic) based on the Ethernet
 *  mode for EPL ports, and based on the
 *  api.platform.config.switch.%d.portIndex.%d.speed property for PCIe,
 *  TE, or Loopback ports. Thus, ports that are not used (powered down
 *  or in disabled mode) do not keep their scheduler bandwidth, and that
 *  bandwidth becomes available to other ports if needed. To have the
 *  scheduler BW be adjusted on link state change, see
 *  ''api.FM10000.updateSchedOnLinkChange''. */
#define FM_AAK_API_FM10000_SCHED_MODE   "api.FM10000.schedMode"
#define FM_AAT_API_FM10000_SCHED_MODE   FM_API_ATTR_TEXT
#define FM_AAD_API_FM10000_SCHED_MODE   "static"

/** Whether the scheduler bandwidth assignments should be updated on
 *  link down/up when using the scheduler dynamic mode (see
 *  ''api.FM10000.schedMode''). When enabled, the bandwidth of ports
 *  that go into link down is released, and is re-assigned on link up.
 *  For EPL ports, when the link is up, the BW assigned to the port will
 *  match the ethernet mode. For PCIe ports, when the link is up the BW
 *  assigned will be determined from the
 *  api.platform.config.switch.%d.portIndex.%d.speed property in the
 *  Liberty Trail config file. */
#define FM_AAK_API_FM10000_UPD_SCHED_ON_LNK_CHANGE     "api.FM10000.updateSchedOnLinkChange"
#define FM_AAT_API_FM10000_UPD_SCHED_ON_LNK_CHANGE     FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_UPD_SCHED_ON_LNK_CHANGE     FALSE


/** CRM timeout in milliseconds.
 *  When changing Frame Handler clock frequency for lower than 700MHz
 *  CRM timeout needs to be scaled.
 *  Values lower than 20ms will be ignored and default 20ms will be set. */
#define FM_AAK_API_FM10000_CRM_TIMEOUT   "api.FM10000.parity.crmTimeout"
#define FM_AAT_API_FM10000_CRM_TIMEOUT   FM_API_ATTR_INT
#define FM_AAD_API_FM10000_CRM_TIMEOUT   20

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

#if 0
/** Step value at which 1588 SYSTIME should increment for each tick.   */
#define FM_AAK_API_FM10000_1588_SYSTIME_STEP "api.FM10000.1588.systimeStep"
#define FM_AAT_API_FM10000_1588_SYSTIME_STEP FM_API_ATTR_INT
#define FM_AAD_API_FM10000_1588_SYSTIME_STEP 10
#endif

/* Specifies whether the API should automatically create logical ports
 * for remote glorts. */
#define FM_AAK_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS  "api.FM10000.createRemoteLogicalPorts"
#define FM_AAT_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS  FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS  FALSE

/* Controls the auto-negotiation state machine link timeout in micro seconds for clause 37 */
#define FM_AAK_API_FM10000_AUTONEG_CLAUSE_37_TIMEOUT "api.FM10000.autoNeg.clause37.timeout"
#define FM_AAT_API_FM10000_AUTONEG_CLAUSE_37_TIMEOUT FM_API_ATTR_INT
#define FM_AAD_API_FM10000_AUTONEG_CLAUSE_37_TIMEOUT 15000

/* Controls the auto-negotiation state machine timeout in micro seconds for SGMII */
#define FM_AAK_API_FM10000_AUTONEG_SGMII_TIMEOUT     "api.FM10000.autoNeg.SGMII.timeout"
#define FM_AAT_API_FM10000_AUTONEG_SGMII_TIMEOUT     FM_API_ATTR_INT
#define FM_AAD_API_FM10000_AUTONEG_SGMII_TIMEOUT     1600

/* Whether to use loopback mode for host network interface services. */
#define FM_AAK_API_FM10000_HNI_SERVICES_LOOPBACK    "api.FM10000.useHniServicesLoopback"
#define FM_AAT_API_FM10000_HNI_SERVICES_LOOPBACK    FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_HNI_SERVICES_LOOPBACK    FALSE

/* EPL TX FIFO Anti-Bubble Watermark
 * 
 * The Port Channel Anti-Bubble FIFO provides buffering to increase the
 * word-to-word jitter tolerance.  TxAntiBubbleWatermark specifies the
 * number of words that will be stored in the FIFO before a frame will
 * be scheduled for egress.  The minimum setting is 1, the maximum
 * setting is 62.  The settings 0 and 63 should not be used, 
 * the behavior is undefined.
 *
 * The value set in the TxAntiBubbleWatermark will affect the latency.
 * Higher the value is, more increase will be seen on the latency. */
#define FM_AAK_API_FM10000_ANTI_BUBBLE_WM       "api.FM10000.antiBubbleWm"
#define FM_AAT_API_FM10000_ANTI_BUBBLE_WM       FM_API_ATTR_INT
#define FM_AAD_API_FM10000_ANTI_BUBBLE_WM       1

/* Specifies the operational mode at SerDes level. There are
 * 5 possible combinations, see fm_serDesOpMode for details.
 * This property allow selecting the kind of SerDes state machine (basic
 * or stub), whether to use the LANE_SAI_xxx registers or the SBus to
 * access serdes registers, and to indicate if the test board is
 * available or not. This last mode is automatically selected if
 * a valid test board IP address is specified. */
#define FM_AAK_API_FM10000_SERDES_OP_MODE       "api.FM10000.serdesOpMode"
#define FM_AAT_API_FM10000_SERDES_OP_MODE       FM_API_ATTR_INT
#define FM_AAD_API_FM10000_SERDES_OP_MODE       FM_SERDES_OPMODE_CHIP_VIA_SAI

/* Specifies the debug level used for some SerDes level functions.
 * Level 0 (default) disables the extra debug functions. */
#define FM_AAK_API_FM10000_SERDES_DBG_LVL       "api.FM10000.serdesDbgLevel"
#define FM_AAT_API_FM10000_SERDES_DBG_LVL       FM_API_ATTR_INT
#define FM_AAD_API_FM10000_SERDES_DBG_LVL       0

/* Whether to enable parity interrupts. */
#define FM_AAK_API_FM10000_PARITY_INTERRUPTS    "api.FM10000.parity.enableInterrupts"
#define FM_AAT_API_FM10000_PARITY_INTERRUPTS    FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_PARITY_INTERRUPTS    TRUE

/* Whether to start the TCAM monitors. */
#define FM_AAK_API_FM10000_START_TCAM_MONITORS  "api.FM10000.parity.startTcamMonitors"
#define FM_AAT_API_FM10000_START_TCAM_MONITORS  FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_START_TCAM_MONITORS  TRUE

/* Scheduler Overspeed.
 *  
 * so:         Scheduler Overspeed (value to set in this property) 
 * overspeed:  Overspeed in percentage 
 *  
 * so = overspeed * 1e10 
 *  
 * e.g. overspeed = 0.8% 
 * 80,000,000 = 0.008 * 1e10 
 *  
 * The effective overspeed will be rounded to the closest higher overspeed 
 * available. */
#define FM_AAK_API_FM10000_SCHED_OVERSPEED    "api.FM10000.schedOverspeed"
#define FM_AAT_API_FM10000_SCHED_OVERSPEED    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_SCHED_OVERSPEED    80000000

/* Properties to override default interrupt handler behaviour */
/* override interrupts for Link status change (LINK_IM register) */
#define FM_AAK_API_FM10000_INTR_LINK_IGNORE_MASK       "api.FM10000.intr.linkIgnoreMask"
#define FM_AAT_API_FM10000_INTR_LINK_IGNORE_MASK       FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_LINK_IGNORE_MASK       0

/* override interrupts for Auto-Negotiation status change (AN_IM register) */
#define FM_AAK_API_FM10000_INTR_AUTONEG_IGNORE_MASK    "api.FM10000.intr.autoNegIgnoreMask"
#define FM_AAT_API_FM10000_INTR_AUTONEG_IGNORE_MASK    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_AUTONEG_IGNORE_MASK    0

/* override SerDes interrupts (SERDES_IM register) */
#define FM_AAK_API_FM10000_INTR_SERDES_IGNORE_MASK     "api.FM10000.intr.serdesIgnoreMask"
#define FM_AAT_API_FM10000_INTR_SERDES_IGNORE_MASK     FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_SERDES_IGNORE_MASK     0

/* override PCIe interrupts (PCIE_IM register) */
#define FM_AAK_API_FM10000_INTR_PCIE_IGNORE_MASK       "api.FM10000.intr.pcieIgnoreMask"
#define FM_AAT_API_FM10000_INTR_PCIE_IGNORE_MASK       FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_PCIE_IGNORE_MASK       0

/* override MAC Table Change Notification  interrupts (MAC_TCN_IM register) */
#define FM_AAK_API_FM10000_INTR_MATCN_IGNORE_MASK      "api.FM10000.intr.maTcnIgnoreMask"
#define FM_AAT_API_FM10000_INTR_MATCN_IGNORE_MASK      FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_MATCN_IGNORE_MASK      0

/* override Frame Handler tail interrupts (FH_TAIL_IM register) */
#define FM_AAK_API_FM10000_INTR_FHTAIL_IGNORE_MASK     "api.FM10000.intr.fhTailIgnoreMask"
#define FM_AAT_API_FM10000_INTR_FHTAIL_IGNORE_MASK     FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_FHTAIL_IGNORE_MASK     0

/* override Software interrupts (SW_IM register) */
#define FM_AAK_API_FM10000_INTR_SW_IGNORE_MASK         "api.FM10000.intr.swIgnoreMask"
#define FM_AAT_API_FM10000_INTR_SW_IGNORE_MASK         FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_SW_IGNORE_MASK         0

/* override Tunneling Engine interrupts (TE_IM register) */
#define FM_AAK_API_FM10000_INTR_TE_IGNORE_MASK         "api.FM10000.intr.teIgnoreMask"
#define FM_AAT_API_FM10000_INTR_TE_IGNORE_MASK         FM_API_ATTR_INT
#define FM_AAD_API_FM10000_INTR_TE_IGNORE_MASK         0

/* Whether to enable EEE SPICO interrupts. */
#define FM_AAK_API_FM10000_ENABLE_EEE_SPICO_INTR    "api.FM10000.enable.eeeSpicoIntr"
#define FM_AAT_API_FM10000_ENABLE_EEE_SPICO_INTR    FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_ENABLE_EEE_SPICO_INTR    FALSE

/* Whether to use the alternate Spico firmware. See
 * fm10000_serdes_swap_code_prd1 and fm10000_serdes_swap_code_prd2 in
 * fm10000_api_spico_code.c. By default, this option is FALSE, and
 * the image defined by fm10000_serdes_swap_code_prd1 is used. */
#define FM_AAK_API_FM10000_USE_ALTERNATE_SPICO_FW   "api.FM10000.useAlternateSpicoFw"
#define FM_AAT_API_FM10000_USE_ALTERNATE_SPICO_FW   FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_USE_ALTERNATE_SPICO_FW   FALSE

/* Whether to run pCal after KR training is complete and EEE has
 * been negociated. This feature is provided only for debug and must 
 * remain DISABLED by default, because it is not currently supported 
 * by the FW */
#define FM_AAK_API_FM10000_ALLOW_KRPCAL_ON_EEE      "api.FM10000.allowKrPcalOnEee"
#define FM_AAT_API_FM10000_ALLOW_KRPCAL_ON_EEE      FM_API_ATTR_BOOL
#define FM_AAD_API_FM10000_ALLOW_KRPCAL_ON_EEE      FALSE

/************************************************************************
 ****                                                                ****
 ****              END UNDOCUMENTED API PROPERTIES                   ****
 ****                                                                ****
 ************************************************************************/

#endif /* __FM_FM10000_PROPERTY_H */

