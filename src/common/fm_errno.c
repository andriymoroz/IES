/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_errno.c
 * Creation Date:   September 16, 2005
 * Description:     Constants and string table for error messages and
 *                  return constants
 *
 *                  THIS FILE IS AUTO-GENERATED BY THE BUILD SYSTEM, DO
 *                  NOT MODIFY THIS FILE.  MODIFY common/ERRORS INSTEAD.
 *                  MD5 HASH OF SOURCE IS 4fbb8924b63caa3798fab9a0102e9ae6.
 *
 *                  Use "make error-tables" to autogenerate this file.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

/* String table used by fmErrorMsg */
static const char *const fmErrorStrings[] =
{
    /* FM_OK */
    "OK.",

    /* FM_FAIL */
    "Unspecified failure.",

    /* FM_ERR_INVALID_ARGUMENT */
    "Invalid argument.",

    /* FM_ERR_UNSUPPORTED */
    "Feature is unsupported.",

    /* FM_ERR_NO_MEM */
    "Unable to allocate memory.",

    /* FM_ERR_NOT_FOUND */
    "Specified key was not found.",

    /* FM_ERR_ALREADY_EXISTS */
    "Specified key already exists.",

    /* FM_ERR_NO_MORE */
    "Reached end of iteration.",

    /* FM_ERR_MODIFIED_WHILE_ITERATING */
    "A data structure was modified while iterating over it.",

    /* FM_ERR_UNINITIALIZED */
    "A necessary initialization function has not been called first.",

    /* FM_ERR_ASSERTION_FAILED */
    "A program assertion check failed.",

    /* FM_ERR_INVALID_SWITCH */
    "Invalid switch number.",

    /* FM_ERR_INVALID_VLAN */
    "Invalid VLAN ID.",

    /* FM_ERR_BUFFER_FULL */
    "Buffer was too small to fit data.",

    /* FM_ERR_ADDR_BANK_FULL */
    "MA table bank was full.",

    /* FM_ERR_ADDR_NOT_FOUND */
    "Address was not found in MA Table.",

    /* FM_ERR_INVALID_ATTRIB */
    "Attribute does not exist or is unsupported.",

    /* FM_ERR_ALREADYUSED_PORT */
    "Port is already a member of a LAG.",

    /* FM_ERR_FULL_LAG */
    "No room for additional ports in this LAG.",

    /* FM_ERR_INVALID_LAG */
    "Invalid LAG identifier.",

    /* FM_ERR_INVALID_PORT */
    "Invalid port number.",

    /* FM_ERR_NO_FREE_LAG */
    "Reached maximum LAG count.",

    /* FM_ERR_INVALID_PORT_STATE */
    "Invalid port state.",

    /* FM_ERR_INVALID_SUBMODE */
    "Invalid submode.",

    /* FM_ERR_PENDING_LIST_FULL */
    "Pending list is full.",

    /* FM_ERR_INVALID_MIRROR */
    "Invalid mirror.",

    /* FM_ERR_INVALID_DSTMASK */
    "Invalid destination port mask specified.",

    /* FM_ERR_INVALID_SRCMASK */
    "Invalid source port mask.",

    /* FM_ERR_INVALID_MACADDR */
    "Invalid MAC address.",

    /* FM_ERR_INVALID_PRIORITY */
    "Invalid priority.",

    /* FM_ERR_INVALID_ADDR_MATCH */
    "Invalid address match.",

    /* FM_ERR_INVALID_TRIG */
    "Invalid trigger number.",

    /* FM_ERR_INVALID_ENTRYTYPE */
    "Invalid MAC Address Table entry type.",

    /* FM_ERR_NO_FREE_TRIG */
    "No free trigger slots remaining.",

    /* FM_ERR_NO_FREE_TRIG_RES */
    "No free trigger resources remaining.",

    /* FM_ERR_NO_VLANCOUNTER */
    "No VLAN counter resource available/allocated.",

    /* FM_ERR_BAD_SWITCH_CONFIG */
    "Current chip configuration is invalid.",

    /* FM_ERR_NO_PLL_LOCK */
    "Unable to lock frame handler PLL.",

    /* FM_ERR_NO_BOOT */
    "Switch's Boot FSM did not finish properly.",

    /* FM_ERR_INVALID_PORT_MIRROR_GROUP */
    "Port mirror group is invalid or already in use.",

    /* FM_ERR_NO_PORTS_IN_MIRROR_GROUP */
    "No (more) ports left in the port mirror group.",

    /* FM_ERR_INVALID_ACL */
    "ACL is invalid or already in use.",

    /* FM_ERR_INVALID_ACL_RULE */
    "ACL rule is invalid or already in use.",

    /* FM_ERR_ACL_DISABLED */
    "ACL subsystem has not been enabled.",

    /* FM_ERR_NO_RULES_IN_ACL */
    "No (more) rules left in ACL.",

    /* FM_ERR_NO_ACLS */
    "No (more) matching ACLs found.",

    /* FM_ERR_ACL_TABLE_FULL */
    "No more trigger hardware resources are available (FM2000 only).",

    /* FM_ERR_ACLS_TOO_BIG */
    "Insufficient hardware resources to support the compiled ACL configuration. See ''FM_FFU_SLICE_ALLOCATIONS''.",

    /* FM_ERR_NO_LAGS */
    "No (more) link aggregation groups in system.",

    /* FM_ERR_NO_PORTS_IN_LAG */
    "No (more) ports in link aggregation group.",

    /* FM_ERR_NO_SWITCHES */
    "No (more) switches.",

    /* FM_ERR_INVALID_ACL_IP_VERSION */
    "An IP protocol version was indicated for an ACL that is not available on the device being configured.",

    /* FM_ERR_INVALID_ACL_PRECEDENCE */
    "An ACL precedence value is out of range.",

    /* FM_ERR_INVALID_ACL_PARAM */
    "The parameter specified for a particular ACL rule option is not valid for that option.",

    /* FM_ERR_INVALID_POLICER */
    "Policer does not exist or cannot be created.",

    /* FM_ERR_INVALID_DSCP_MAP */
    "A policer DSCP down map entry is out of range.",

    /* FM_ERR_INVALID_SWPRI_MAP */
    "A policer switch priority down map entry is out of range.",

    /* FM_ERR_INVALID_ACL_IMAGE */
    "An invalid ACL binary image was specified.",

    /* FM_ERR_ACL_COMPILE */
    "The ACL compiler (''fmCompileACL'') was unable to produce a valid ACL binary image from the current ACL configuration, likely due to an invalid configuration (for example specifying an unknown mapper value).",

    /* FM_ERR_ACL_IMAGE_OVERFLOW */
    "The compiled ACL binary image will not fit into the space provided by the caller.",

    /* FM_ERR_INVALID_SWITCH_TYPE */
    "An unsupported switch type has been provided.",

    /* FM_ERR_PARITY */
    "A parity error was encountered when reading a register.",

    /* FM_ERR_INVALID_REGISTER_CONTENTS */
    "A register contained an illegal or uninterpretable value.",

    /* FM_ERR_INVALID_SLICE */
    "Invalid FFU slice specification.",

    /* FM_ERR_INVALID_SLOT */
    "The slot argument to an FFU mapper function is out of range.",

    /* FM_ERR_SWAG_DUPLICATE_LINK */
    "An attempt was made to add a duplicate link to a switch aggregate.",

    /* FM_ERR_SWAG_TRUNK_LIST_FULL */
    "A switch aggregate switch's trunk list is full - no more links may be added to the trunk.",

    /* FM_ERR_SWAG_NO_MORE_TRUNKS */
    "A switch aggregate switch has no more trunks available.",

    /* FM_ERR_LPORT_DESTS_UNAVAILABLE */
    "A block of logical port destination table entries cannot be allocated.",

    /* FM_ERR_LPORT_CAM_UNAVAILABLE */
    "A logical port CAM/RAM entry cannot be allocated.",

    /* FM_ERR_NO_SYMBOL_LOCK */
    "No symbol lock on SERDES (required for BIST).",

    /* FM_ERR_VLAN_ALREADY_EXISTS */
    "The vlan already exists and thus cannot be created.",

    /* FM_ERR_SWITCH_NOT_UP */
    "The specified switch is not up.",

    /* FM_ERR_BAD_BUFFER */
    "Attempt to operate on an invalid buffer pointer.",

    /* FM_ERR_FRAME_TOO_LARGE */
    "Frame is too large for sending over the CPU interface.",

    /* FM_ERR_INVALID_VCID */
    "The specified VLAN counter group ID does not reference a valid VLAN counter group.",

    /* FM_ERR_LOG_PORT_UNAVAILABLE */
    "No available unallocated logical ports.",

    /* FM_ERR_FFU_RES_OWNED */
    "This FFU resource is already owned by another component.",

    /* FM_ERR_NO_FFU_RES_FOUND */
    "No FFU resources are available.",

    /* FM_ERR_INVALID_PAUSE_MODE */
    "This property is not valid given the current pause mode.",

    /* FM_ERR_KEY_NOT_FOUND */
    "Key was not found in the attribute database.",

    /* FM_ERR_INVALID_VALUE */
    "Invalid value argument supplied to API function.",

    /* FM_ERR_ARP_TABLE_FULL */
    "The ARP table is full.",

    /* FM_ERR_DUPLICATE_ARP_ENTRY */
    "Duplicate ARP entry.",

    /* FM_ERR_TABLE_FULL */
    "Table is full.",

    /* FM_ERR_INVALID_INTERFACE */
    "Invalid IP interface.",

    /* FM_ERR_INVALID_MULTICAST_GROUP */
    "Invalid multicast group.",

    /* FM_ERR_INVALID_VRID */
    "Invalid Virtual Router ID.",

    /* FM_ERR_ECMP_GROUP_FULL */
    "Too many routes in ECMP Group.",

    /* FM_ERR_MCAST_GROUP_ACTIVE */
    "Multicast Group is active.",

    /* FM_ERR_MCAST_GROUP_ALREADY_ASSIGNED */
    "Multicast Group already attached to an address.",

    /* FM_ERR_TOO_MANY_VIRTUAL_ROUTERS */
    "Too many virtual routers in use.",

    /* FM_ERR_VLAN_ALREADY_ASSIGNED */
    "Vlan already assigned to an interface.",

    /* FM_ERR_TRIGGER_UNAVAILABLE */
    "No trigger is available.",

    /* FM_ERR_RATELIMITER_UNAVAILABLE */
    "No rate limiter is available.",

    /* FM_ERR_INVALID_STORM_CTRL */
    "Invalid storm controller.",

    /* FM_ERR_NO_STORM_CONTROLLERS */
    "No storm controller found.",

    /* FM_ERR_NO_STORM_CONDITION */
    "No storm controller condition found.",

    /* FM_ERR_NO_STORM_ACTION */
    "No storm controller action found.",

    /* FM_ERR_INVALID_STORM_COND */
    "Invalid storm controller condition.",

    /* FM_ERR_INVALID_QOS_MODE */
    "Attempt to set an attribute that is under API control when ''FM_AUTO_PAUSE_MODE'' is enabled.",

    /* FM_ERR_NO_POLICERS */
    "No policers found.",

    /* FM_ERR_STORM_COND_EXCEEDED */
    "Exceeding the maximum number of conditions for a storm controller.",

    /* FM_ERR_STORM_ACTION_EXCEEDED */
    "Exceeding the maximum number of actions for a storm controller.",

    /* FM_ERR_INVALID_ACTION */
    "Invalid storm controller action.",

    /* FM_ERR_UNKNOWN_REGISTER */
    "Unrecognized register name.",

    /* FM_ERR_REG_SNAPSHOT_FULL */
    "Register snapshot memory is full.",

    /* FM_ERR_MCAST_ADDR_NOT_ASSIGNED */
    "A Multicast Address has not been assigned to a multicast group.",

    /* FM_ERR_MCAST_GROUP_NOT_ACTIVE */
    "Multicast Group is not active.",

    /* FM_ERR_INVALID_STP_MODE */
    "Invalid spanning tree mode.",

    /* FM_ERR_NO_MIRROR_GROUPS_EXIST */
    "There are no mirror groups existing.",

    /* FM_ERR_PORT_ALREADY_MIRRORED */
    "The specified port is already added to another mirror group.",

    /* FM_ERR_NO_FREE_STORM_CTRL */
    "No more storm controllers can be created.",

    /* FM_ERR_MCAST_ADDRESS_IN_USE */
    "Multicast Address already assigned to a multicast group.",

    /* FM_ERR_USE_MCAST_FUNCTIONS */
    "Use Multicast Group functions to configure multicast addresses.",

    /* FM_ERR_NO_MCAST_RESOURCES */
    "There are not enough multicast resources to perform this operation.",

    /* FM_ERR_NOT_MULTICAST_ADDRESS */
    "Not a Multicast Address.",

    /* FM_ERR_VRID_ALREADY_IN_USE */
    "Virtual Router ID is already in use.",

    /* FM_ERR_PHYS_ROUTER_NOT_DELETABLE */
    "Physical Router may not be deleted.",

    /* FM_ERR_PORT_IS_IN_LAG */
    "Port is a member of a Link Aggregation Group.",

    /* FM_ERR_EGRESS_ACLS_REQUIRE_LAST_MINSLICE */
    "Egress ACLs require the highest-numbered slice be allocated.",

    /* FM_ERR_SWITCH_AGGREGATE_NOT_EMPTY */
    "Switch Aggregate is not empty.",

    /* FM_ERR_INVALID_SWITCH_AGGREGATE */
    "Invalid Switch Aggregate (SWAG) Identifier.",

    /* FM_ERR_SWITCH_ALREADY_IN_AGGREGATE */
    "Switch is already a switch aggregate member.",

    /* FM_ERR_SWITCH_IS_NOT_DOWN */
    "Switch is not down.",

    /* FM_ERR_SWITCH_NOT_IN_AGGREGATE */
    "Switch is not in a switch aggregate.",

    /* FM_ERR_NON_CONTIGUOUS_RESOURCE */
    "Can't return the range for an FFU resource, because it is not contiguous.",

    /* FM_ERR_INVALID_COUNTER_BANK */
    "Counter/policer bank number out of range.",

    /* FM_ERR_INVALID_COUNTER_INDEX */
    "Counter/policer index number out of range.",

    /* FM_ERR_INVALID_LBG_STATE */
    "This operation is invalid for the current mode of this LBG.",

    /* FM_ERR_INVALID_LBG_PORT_COUNT */
    "Invalid number of ports in the LBG.",

    /* FM_ERR_INVALID_LBG_DISTRIB */
    "The total usage for all members does not sum to 100%.",

    /* FM_ERR_INVALID_LBG_PORT_STATE */
    "The LBG member port is in an unknown state.",

    /* FM_ERR_INVALID_LBG_PORT_REDIRECT */
    "The LBG member port does not have a valid redirect target.",

    /* FM_ERR_NO_LBG_STANDBY_PORTS */
    "This LBG has no standby ports to redirect to.",

    /* FM_ERR_PHYSICAL_SWITCH_ONLY */
    "Request only valid with a physical switch.",

    /* FM_ERR_NO_FREE_RESOURCES */
    "Not enough free resources exist to complete this operation.",

    /* FM_ERR_INVALID_SFLOW_INSTANCE */
    "Invalid sFlow instance ID.",

    /* FM_ERR_INVALID_SFLOW_ATTR */
    "Invalid sFlow attribute.",

    /* FM_ERR_NO_SFLOW_PORT */
    "No sFlow port found.",

    /* FM_ERR_NO_TRIGGER_ID */
    "No trigger Id found.",

    /* FM_ERR_FIBM_TIMEOUT */
    "FIBM operation timeout.",

    /* FM_ERR_FIBM_NOT_ENABLED */
    "FIBM is not enabled.",

    /* FM_ERR_FIBM_INVALID_RESPONSE */
    "Invalid FIBM response.",

    /* FM_ERR_GLORT_RANGE_TOO_SMALL */
    "Glort range too small to address all logical ports.",

    /* FM_ERR_NO_FORWARDING_RULES */
    "There are no (more) forwarding rules.",

    /* FM_ERR_PORT_NOT_LBG_MEMBER */
    "Port is not a member of the load balancing group.",

    /* FM_ERR_INVALID_LBG */
    "Invalid load balancing group.",

    /* FM_ERR_TX_PACKET_QUEUE_FULL */
    "Transmit packet queue is full.",

    /* FM_ERR_PORT_IN_USE */
    "Port is in use.",

    /* FM_ERR_NO_LAG_RESOURCES */
    "There are not enough LAG resources to perform this operation.",

    /* FM_ERR_NO_LBG_RESOURCES */
    "There are not enough LBG resources to perform this operation.",

    /* FM_ERR_LAG_IN_USE */
    "LAG is already in use.",

    /* FM_ERR_NO_FREE_PRIORITY_MAPPER */
    "No priority mapper resources are available to be allocated.",

    /* FM_ERR_INVALID_PRIORITY_MAPPER */
    "Invalid priority mapper ID.",

    /* FM_ERR_INVALID_PRIORITY_MAP */
    "Invalid priority map.",

    /* FM_ERR_SWPRI_UNASSIGNED */
    "No switch priority assigned to priority mapper.",

    /* FM_ERR_INVALID_REGISTER */
    "Operation cannot be performed on the specified register.",

    /* FM_ERR_GLORT_IN_USE */
    "The specified glort is already in use.",

    /* FM_ERR_SWAG_NOT_UP */
    "The switch aggregate is not up.",

    /* FM_ERR_NO_GLORTS_ALLOCATED */
    "No glorts have been allocated using an fmAllocateStack* function.",

    /* FM_ERR_CVLAN_MAPPING_FULL */
    "No more CVLAN mappings can be created.",

    /* FM_ERR_MAPPER_FULL */
    "Mapper is full.",

    /* FM_ERR_INVALID_ADDR_MASK */
    "Invalid IP address mask.",

    /* FM_ERR_NO_ACTIVE_PORTS_IN_LBG */
    "No active ports in load balancing group.",

    /* FM_ERR_NO_FREE_PORT_SET */
    "No more port sets can be created.",

    /* FM_ERR_INVALID_PORT_SET */
    "Specified port set is not valid.",

    /* FM_ERR_NO_PORT_SET */
    "There are no (more) port sets.",

    /* FM_ERR_NO_PORT_SET_PORT */
    "There are no (more) port set ports.",

    /* FM_ERR_PORTSET_IS_INTERNAL */
    "Internal portsets cannot be modified.",

    /* FM_ERR_INVALID_NEW_ESCHED_CONFIG */
    "The new egress scheduler configuration is invalid and cannot be applied.",

    /* FM_ERR_INVALID_ACTIVE_ESCHED_CONFIG */
    "The active egress scheduler configuration has an error.",

    /* FM_ERR_STATIC_ADDR_EXISTS */
    "Static MA Table entry already exists.",

    /* FM_ERR_INVALID_INDEX */
    "Index is out of range.",

    /* FM_ERR_SPARE1 */
    "(Not used.)",

    /* FM_ERR_INVALID_FID */
    "Invalid FID number.",

    /* FM_ERR_INVALID_PORT_MAC */
    "Invalid port MAC number.",

    /* FM_ERR_INVALID_PORT_LANE */
    "Invalid port lane number.",

    /* FM_ERR_INVALID_ETH_MODE */
    "Invalid Ethernet interface mode.",

    /* FM_ERR_PER_LAG_ATTRIBUTE */
    "Per-Lag attribute cannot be set on a member port.",

    /* FM_ERR_NOT_PER_LAG_ATTRIBUTE */
    "Per-port attribute cannot be set on a lag logical port.",

    /* FM_ERR_ECMP_GROUP_IN_USE */
    "The ECMP Group is in use.",

    /* FM_ERR_ECMP_GROUP_IS_FULL */
    "The ECMP Group is full.",

    /* FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS */
    "Mixing Narrow and Wide Next Hops in an ECMP group is not supported.",

    /* FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE */
    "FFU slice redistribution insufficient for existing Unicast routes.",

    /* FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE */
    "FFU slice redistribution insufficient for existing Multicast routes.",

    /* FM_ERR_INSUFFICIENT_ACL_SPACE */
    "FFU slice redistribution insufficient for existing ACLs.",

    /* FM_ERR_INSUFFICIENT_CVLAN_SPACE */
    "FFU slice redistribution insufficient for existing CVLANs.",

    /* FM_ERR_TRAFFIC_STOP_REQUIRED */
    "Unable to perform the requested operation without stopping traffic.",

    /* FM_ERR_NEED_RESERVED_VLAN */
    "No VLAN reserved for Per-VLAN spanning tree mode.",

    /* FM_ERR_NO_ACL_RULE_INGRESS_PORT */
    "There are no (more) ACL rule ingress ports.",

    /* FM_ERR_INVALID_SCENARIO */
    "Invalid scenario key parameters.",

    /* FM_ERR_INVALID_MUX_SELECT */
    "Invalid key multiplexer selection.",

    /* FM_ERR_INVALID_REMAP_CFG */
    "Invalid FFU remap point profile configuration.",

    /* FM_ERR_INVALID_HASH_CFG */
    "Invalid L3 hash profile.",

    /* FM_ERR_INVALID_RULE */
    "Invalid FFU Rule.",

    /* FM_ERR_LOG_PORT_REQUIRED */
    "Requested operation requires a logical port.",

    /* FM_ERR_INVALID_MAPPER */
    "Invalid Mapper ID specified.",

    /* FM_ERR_INVALID_STATE */
    "API call invalid in current state.",

    /* FM_ERR_INVALID_CRM */
    "Invalid CRM identifier.",

    /* FM_ERR_CRM_UNAVAILABLE */
    "No CRM indexes are available.",

    /* FM_ERR_MCAST_INVALID_STATE */
    "Invalid Multicast Group State for this Operation.",

    /* FM_ERR_INVALID_LBG_MODE */
    "Invalid LBG mode.",

    /* FM_ERR_INVALID_LBG_PORT_TRANS */
    "Invalid LBG port mode transition.",

    /* FM_ERR_TUNNEL_IN_USE */
    "Tunnel is in use.",

    /* FM_ERR_NO_ROUTE_TO_HOST */
    "There are no routes that serve the specified IP address.",

    /* FM_ERR_LOCK_NOT_TAKEN */
    "Required lock not taken.",

    /* FM_ERR_LOCK_INIT */
    "Unable to initialize lock.",

    /* FM_ERR_LOCK_DESTROY */
    "Unable to destroy lock.",

    /* FM_ERR_UNABLE_TO_LOCK */
    "Unable to capture lock.",

    /* FM_ERR_UNABLE_TO_UNLOCK */
    "Unable to release lock.",

    /* FM_ERR_BAD_GETTIME */
    "Unable to get the current system time.",

    /* FM_ERR_NO_EVENTS_AVAILABLE */
    "No events available at this time.",

    /* FM_ERR_REMOVING_EVENT */
    "Error in removing event from queue.",

    /* FM_ERR_EVENT_QUEUE_FULL */
    "Unable to send an event to a thread because its event queue is full.",

    /* FM_ERR_UNABLE_TO_CREATE_THREAD */
    "Thread creation failed.",

    /* FM_ERR_UNABLE_TO_CREATE_COND */
    "Condition variable creation failed.",

    /* FM_ERR_UNABLE_TO_SIGNAL_COND */
    "Unable to send signal to condition variable.",

    /* FM_ERR_CONDWAIT_FAILED */
    "Wait on condition failed.",

    /* FM_ERR_EVENT_QUEUE_EMPTY */
    "Event queue is empty.",

    /* FM_ERR_HEAP_FULL */
    "Heap is full.",

    /* FM_ERR_LOCK_UNINITIALIZED */
    "The specified lock has not been initialized.",

    /* FM_ERR_SEM_TIMEOUT */
    "A timeout occurred while trying to capture a semaphore.",

    /* FM_ERR_LOCK_TIMEOUT */
    "A timeout occurred while trying to capture a lock.",

    /* FM_ERR_LOCK_PRECEDENCE */
    "A lock was captured or released out of order with respect to other locks.",

    /* FM_ERR_OPENING_INFO_DEVICE_NODE */
    "Unable to open info device node.",

    /* FM_ERR_OPENING_DEVICE_NODE */
    "Unable to open device node.",

    /* FM_ERR_BAD_IOCTL */
    "A device ioctl call failed.",

    /* FM_ERR_CANT_IDENTIFY_SWITCH */
    "The switch could not be identified.",

    /* FM_ERR_MDIO_NO_RESPONSE */
    "MDIO access error.",

    /* FM_ERR_I2C_NO_RESPONSE */
    "I2C access error",

    /* FM_ERR_TOO_MANY_PORTS_FOR_LBG_REDIRECT */
    "The total number of active and failover LBG ports exceeded the maximum allowed by LAG pruning.",

    /* FM_ERR_NO_VLANS_IN_MIRROR_GROUP */
    "No (more) vlans left in the mirror group.",

    /* FM_ERR_PORT_IS_INTERNAL */
    "Port is internal.",

    /* FM_ERR_DFE_NOT_STATIC */
    "Current DFE mode is not static.",

    /* FM_ERR_INCOMPATIBLE_DFE */
    "Desired DFE mode not compatible with interface.",

    /* FM_ERR_INVALID_TRAP_CODE */
    "Invalid trap code specified.",

    /* FM_ERR_FFU_RESOURCE_IN_USE */
    "FFU resource is already in use.",

    /* FM_ERR_MICROCODE_IMAGE_FILE_MISSING */
    "The microcode image file is missing.",

    /* FM_ERR_MICROCODE_IMAGE_INVALID */
    "The microcode image file has errors and is invalid.",

    /* FM_ERR_INVALID_SCHED_TOKEN */
    "Scheduler token not found or invalid.",

    /* FM_ERR_SCHED_INIT */
    "Unable to initialize scheduler.",

    /* FM_ERR_SCHED_OVERSUBSCRIBED */
    "The operation could not complete because it would oversubscribe the scheduler. ",

    /* FM_ERR_SCHED_VIOLATION */
    "A scheduler rule violation was detected, or no scheduler solution was found. ",

    /* FM_ERR_NO_FREE_TUNNEL_ID */
    "No more tunnel IDs can be created.",

    /* FM_ERR_EEPROM_VERSION_MISMATCH */
    "The EEPROM version used is not compatible with this API version.",

    /* FM_ERR_NO_EYE_DIAGRAM_SAMPLES */
    "No (more) eye diagram samples left.",

    /* FM_ERR_DFE_TIMEOUT */
    "Timeout waiting for DFE to complete (BIST).",

    /* FM_ERR_INVALID_GPIO_ID */
    "The specified GPIO signal ID is invalid.",

    /* FM_ERR_INVALID_GPIO_BANK */
    "The specified GPIO signal bank is invalid.",

    /* FM_ERR_INVALID_GPIO_STATE */
    "The specified operation is invalid in the current GPIO signal state.",

    /* FM_ERR_GPIO_NO_RESPONSE */
    "GPIO access error.",

    /* FM_ERR_INTERNAL_RESOURCE */
    "The specified resource is internal and can only be modified by API internals.",

    /* FM_ERR_STATE_MACHINE_HANDLE */
    "The specified state machine handle does not correspond to an instantiated state machine.",

    /* FM_ERR_STATE_MACHINE_TYPE */
    "There was a state machine type mismatch or the type is being used or it's unregistered.",

    /* FM_ERR_BOUND_STATE_MACHINE */
    "An attempt was made to bind a new State Transition Table to a state machine that is already bound.",

    /* FM_ERR_INVALID_TRANSACTION_ID */
    "Invalid mailbox transaction ID.",

    /* FM_ERR_CONFLICT_POLICER_PARAM */
    "Policer parameters conflict with the existing policer bank configuration.",

    /* FM_ERR_TUNNEL_TYPE */
    "The specified tunnel type parameter is invalid or not supported.",

    /* FM_ERR_TUNNEL_BIN_FULL */
    "The specified tunnel rule bin is full.",

    /* FM_ERR_TUNNEL_CONFLICT */
    "The specified tunnel rule has conflicting conditions or actions.",

    /* FM_ERR_TUNNEL_NO_ENCAP_FLOW */
    "The specified tunnel rule encap flow does not exist.",

    /* FM_ERR_TUNNEL_NO_FREE_GROUP */
    "No more tunnel group can be created on this engine.",

    /* FM_ERR_TUNNEL_LOOKUP_FULL */
    "No more tunnel lookup entries available on this engine.",

    /* FM_ERR_TUNNEL_GLORT_FULL */
    "No more tunnel glort entries available on this engine.",

    /* FM_ERR_TUNNEL_COUNT_FULL */
    "No more tunnel counter entries available on this engine.",

    /* FM_ERR_TUNNEL_FLOW_FULL */
    "No more tunnel flow entries available on this engine.",

    /* FM_ERR_TUNNEL_GROUP_FULL */
    "No more tunnel flow entries available on this group.",

    /* FM_ERR_TUNNEL_NO_COUNT */
    "The specified rule or encap flow does not define count action.",

    /* FM_ERR_TUNNEL_TEP_SIZE */
    "The specified tep size is invalid.",

    /* FM_ERR_TUNNEL_INVALID_ENTRY */
    "The specified tunnel group or rule is invalid.",

    /* FM_ERR_INVALID_MIRROR_PROFILE */
    "The specified mirror profile identifier is invalid.",

    /* FM_ERR_INVALID_PROFILE_INDEX */
    "The specified profile index is invalid.",

    /* FM_ERR_TUNNEL_NOT_CONFIGURED */
    "The specified tunnel has not yet been configured and cannot be used.",

    /* FM_ERR_READONLY_ATTRIB */
    "Attribute is read-only.",

    /* FM_ERR_INVALID_PEP_MODE */
    "Invalid PCI Express Endpoint configuration",

    /* FM_ERR_VN_TUNNEL_NOT_CONFIGURED */
    "The Virtual Network Tunnel has not been configured.",

    /* FM_ERR_MIRROR_NO_VLAN_ENCAP */
    "The Mirror Group must specify an encap vlan to work.",

    /* FM_ERR_INVALID_IN_VN_TRANSPARENT_MODE */
    "This request is not supported in Transparent mode.",

    /* FM_ERR_VIRTUAL_NETWORK_IN_USE */
    "Virtual Network is in use.",

    /* FM_ERR_PEP_PORT_NOT_SCHEDULED */
    "Pep port is not scheduled.",

    /* FM_ERR_PORT_NOT_VLAN_MEMBER */
    "Port is not a member of the vlan.",

    /* FM_ERR_VIRTUAL_ROUTER_ONLY */
    "Request only valid with a virtual router.",

    /* FM_ERR_FRAME_SIZE_EXCEEDS_MTU */
    "Frame size exceeds maximum transmission unit of network interface.",

    /* FM_ERR_TE_MODE */
    "This operation is invalid for the current TE mode.",

};


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
/** fmErrorMsg
 * \ingroup common
 *
 * \desc            Returns the error string for a given error code.
 *
 * \param[in]       err is the error code to translate. 
 *
 *
 * \return          The corresponding error string or an error message if
 *                  the code is invalid.
 *
 *****************************************************************************/
const char *fmErrorMsg(fm_int err)
{
    if ( (err < 0) || (err >= 285) )
    {
        return "Invalid error number (no such error)";
    }

    return fmErrorStrings[err];

}   /* end fmErrorMsg */




/*****************************************************************************/
/** fmErrorCode
 * \ingroup common
 *
 * \desc            Returns the error code given a string fragment.
 *
 * \param[in]       errString points to a string fragment to search for.
 *
 *
 * \return          The corresponding error code or -1 if the code is invalid.
 *
 *****************************************************************************/
fm_status fmErrorCode(const char *errString)
{
    fm_int err;

    for ( err = 0 ; err < 285 ; err++ )
    {
        if (strcasecmp(errString, fmErrorStrings[err]) == 0)
        {
            return err;
        }
    }

    return -1;

}   /* end fmErrorCode */
