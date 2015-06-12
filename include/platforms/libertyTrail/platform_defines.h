/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_defines.h
 * Creation Date:   June 2, 2014
 * Description:     Platform specific definitions
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

#ifndef __FM_PLATFORM_DEFINES_H
#define __FM_PLATFORM_DEFINES_H


/*****************************************************************************
 * Platform-Specific Constants
 *****************************************************************************/

/** The maximum number of physical switches in the system. This includes any
 *  switches managed by the platform using FIBM. Note that this constant will
 *  be removed in a future release of the Liberty Trail platform. 
 *  \ingroup constSystem
 */
#ifdef PLATFORM_NUM_FOCALPOINTS
#define FM_MAX_NUM_FOCALPOINTS             PLATFORM_NUM_FOCALPOINTS
#else
#define FM_MAX_NUM_FOCALPOINTS             1
#endif

/* The mapped memory types */
#define FM_MEM_TYPE_CSR                     0

/* The lock for the platform state */
#define FM_PLAT_INFO                        (FM_MEM_TYPE_CSR + 1)

/* The lock for the I2C bus */
#define FM_PLAT_I2C_BUS                     (FM_PLAT_INFO + 1)

/* The number of platform locks to allocate. */
#define FM_MAX_PLAT_LOCKS                   (FM_PLAT_I2C_BUS + 1)

/* The queue size to hold packets for transmit */
#define FM_PACKET_QUEUE_SIZE                256

/* CPU maximum frame size */
#define FM_PORT_0_MAX_FRAME_SIZE            15870

/* Enumerated data type for storing the type of switch needed in white model */
typedef enum _fm_platformSwitchType
{
   FM_PLATFORM_SWITCH_TYPE_UNKNOWN = 0,
   FM_PLATFORM_SWITCH_TYPE_FM4000,
   FM_PLATFORM_SWITCH_TYPE_FM6000,
   FM_PLATFORM_SWITCH_TYPE_FM10000,
   FM_PLATFORM_SWITCH_TYPE_SWAG,

} fm_platformSwitchType;

/*****************************************************************************
 * Customer-Configurable ALOS Constants
 *****************************************************************************/
#define FM_ALOS_INTERNAL_MAX_LOCKS          256
#define FM_ALOS_INTERNAL_MAX_DBG_RW_LOCKS   64
#define FM_ALOS_INTERNAL_MAX_SEMAPHORES     1000
#define FM_ALOS_INTERNAL_DYN_LOAD_LIBS      2


/**************************************************/
/** The base address at which to map the 
 *  API shared library data space and
 *  platform layer memory-mapped I/O. This
 *  value must be a virtual memory address that is
 *  not otherwise used by the system.
 * \ingroup constSystem
 **************************************************/
/* 0x60000000 is a decent location, fairly far away
 * from anything else on PPC, IA-32, and AMD-64
 * Linux. */
#define FM_SHARED_MEMORY_ADDR               0x60000000


/**************************************************/
/** The number of bytes of of memory that will be 
 *  memory mapped for use as API shared 
 *  library data space, starting at the address 
 *  specified by ''FM_SHARED_MEMORY_ADDR''. 
 *  This size does not include memory mapped I/O 
 *  space that may be mapped by the platform layer.
 * \ingroup constSystem
 **************************************************/
#define FM_SHARED_MEMORY_SIZE               0x11000000


/***************************************************/
/** The pthread stack size
 * \ingroup intConstSystem
 **************************************************/
#define FM_ALOS_PTHREAD_STACK_SIZE          (512 * 1024)


/***************************************************
 * The line size for a line in a log; may contain
 * multiple newlines.
 **************************************************/
#define FM_LOG_MAX_LINE_SIZE                256
#define FM_LOG_MAX_LINES                    1000
#define FM_LOG_MAX_FILTER_LEN               16384
#define FM_MAX_FILENAME_LENGTH              256


/***************************************************/
/** The number of threads we expect to use the
 *  rw locks, not the maximum number of
 * supported threads.
 * \ingroup constSystem
 **************************************************/
#define FM_MAX_THREADS                      64


/***************************************************
 * The following log control macros are used to
 * compile in or out various logging macros that
 * are useful for debugging. Compiling them out
 * may be desireable to increase overall performance
 * of the API.
 **************************************************/

/** \ingroup constSystem
 * @{ */

/** Controls whether all logging functions are enabled or compiled out (except
 *  those functions that log entry to or exit from a function. */
#define FM_ALOS_LOGGING_SUBSYSTEM

/** Controls whether logging functions that log entry to or exit from a
 *  function are enabled or compiled out. */
#define FM_ALOS_FUNCTION_LOGGING

/** Controls whether logging functions that log entry to or exit from ALOS
 *  lock and reader-writer lock functions are enabled or compiled out.
 *  #undef to compile out. Normally left disabled to improve API performance.*/
#undef FM_ALOS_LOCK_FUNCTION_LOGGING

/** Controls whether ''FM_LOG_ENTRY_VERBOSE'' and
 *  ''FM_LOG_EXIT_VERBOSE'' are enabled or compiled out. Use
 *  FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out these logging macros will result in better
 *  packet throughput to and from the CPU at the cost of losing
 *  debug logging capability. */
#define FM_ALOS_INCLUDE_LOG_VERBOSE         FM_ENABLED

/** Controls whether ''FM_LOG_DEBUG'', ''FM_LOG_DEBUG_VERBOSE'',
 *  ''FM_LOG_DEBUG2'' and ''FM_LOG_DEBUG3'' are enabled or compiled out.
 *  Use FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out these logging macros will result in better packet throughput
 *  to and from the CPU at the cost of losing debug logging capability. */
#define FM_ALOS_INCLUDE_LOG_DEBUG           FM_ENABLED

/** Controls whether ''FM_LOG_WARNING'' is enabled or compiled out.
 *  Use FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out this logging macro will result in better packet throughput
 *  to and from the CPU at the cost of losing warning message logging
 *  capability. */
#define FM_ALOS_INCLUDE_LOG_WARNING         FM_ENABLED

/** Controls whether ''FM_LOG_ERROR'' is enabled or compiled out.
 *  Use FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out this logging macro will result in better packet throughput
 *  to and from the CPU at the cost of losing error message logging
 *  capability. */
#define FM_ALOS_INCLUDE_LOG_ERROR           FM_ENABLED

/** Controls whether ''FM_LOG_FATAL'' is enabled or compiled out.
 *  Use FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out this logging macro will result in better packet throughput
 *  to and from the CPU at the cost of losing fatal message logging
 *  capability. */
#define FM_ALOS_INCLUDE_LOG_FATAL           FM_ENABLED

/** Controls whether ''FM_LOG_INFO'' is enabled or compiled out.
 *  Use FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out this logging macro will result in better packet throughput
 *  to and from the CPU at the cost of losing info message logging
 *  capability. */
#define FM_ALOS_INCLUDE_LOG_INFO            FM_ENABLED

/** Controls whether ''FM_LOG_ASSERT'' and ''FM_LOG_ABORT_ON_ASSERT''
 *  are enabled or compiled out.
 *  Use FM_ENABLED to include or FM_DISABLED to compile out.
 *  Compiling out this logging macro will result in better packet throughput
 *  to and from the CPU at the cost of losing info message logging
 *  capability. */
#define FM_ALOS_INCLUDE_LOG_ASSERT          FM_ENABLED

/** @} (end of Doxygen group) */


/**************************************************
 * Don't let FM_ALOS_FUNCTION_LOGGING be enabled
 * without FM_ALOS_LOGGING_SUBSYSTEM
 **************************************************/

#ifndef FM_ALOS_LOGGING_SUBSYSTEM
#undef FM_ALOS_FUNCTION_LOGGING
#endif


/***************************************************/
/** \ingroup constSystem
 *
 *  This constant is used to enable the lock inversion 
 *  defense mechanism within the API. The lock 
 *  inversion defense generates error messages if 
 *  multi-threaded access to (and within) the 
 *  API could possibly lead to a lock
 *  inversion. Locks are assigned a precedence value
 *  and an error message is generated if two locks
 *  are taken out of order. Generation of an error
 *  message does not necessarily constitute an
 *  operational failure; it merely serves notice
 *  that operational failure is possible and the
 *  inversion should be corrected.
 *                                              \lb\lb
 *  Intel performs all API regression testing with
 *  the lock inversion defense enabled and corrects
 *  any detected inversions before releasing the
 *  software. Users may wish to leave the defense
 *  enabled on the chance that a use case resulting
 *  in an inversion was not detected during Intel's
 *  testing.
 *                                              \lb\lb
 *  If this constant is not defined by the platform
 *  layer, the lock inversion defense will default
 *  to enabled. 
 **************************************************/
#define FM_LOCK_INVERSION_DEFENSE       FM_ENABLED


/*****************************************************************************
 * Customer-Configurable API Constants
 *****************************************************************************/

/** The maximum number of logical ports supported in the system.
 *  \ingroup constSystem
 */
#define FM_MAX_LOGICAL_PORT                 0xFFFF


/** The number of event buffers to allocate.
 *  \ingroup constSystem
 */
#define FM_MAX_EVENTS                       4096


/** The event queue size for API threads that require a large event queue.
 *  Memory will not be allocated until an event is actually sent to the
 *  thread. This value dictates the maximum number of events that can be in
 *  the queue at once.
 *  \ingroup constSystem
 */
#define FM_EVENT_QUEUE_SIZE_LARGE           4096


/** The event queue size for API threads that require a small event queue.
 *  Memory will not be allocated until an event is actually sent to the
 *  thread. This value dictates the maximum number of events that can be in
 *  the queue at once.
 *  \ingroup constSystem
 */
#define FM_EVENT_QUEUE_SIZE_SMALL           1024


/** The event queue size for API threads that do not use an event queue.
 *  Memory will not be allocated until an event is actually sent to the
 *  thread. This value dictates the maximum number of events that can be in
 *  the queue at once.
 *  \ingroup constSystem
 */
#define FM_EVENT_QUEUE_SIZE_NONE            1


/** The delay in nano-seconds between link-state debounce checks
 *  \ingroup constSystem
 */
#define FM_API_LINK_STATE_DEBOUNCE_DELAY    250000000           /* nanosecs */


/** The delay in seconds between MAC Table Maintenance Polls
 *  \ingroup constSystem
 */
#define FM_API_MAC_TABLE_MAINT_DELAY        3


/** The minimum delay in seconds we enforce between MA TABLE
 *  Maintenance tasks. Note that we make sure that the maximum delay
 *  between the MA Table Maintenance tasks is FM_API_MAC_TABLE_MAINT_DELAY.
 * \ingroup constSystem
 */
#define FM_API_MAC_TABLE_MAINT_THROTTLE     0


/** The maximum number of MA Table updates (learn/age) reported together in
 *  a single FM_EVENT_TABLE_UPDATE event.
 *  \ingroup constSystem */
#define FM_TABLE_UPDATE_BURST_SIZE          64


/** The number of LAGs that will be usable for the local switch
 *  only (i.e. not in a multi switch system). The handles for these
 *  lags will be automatically generated when fmCreateLAG() is called.
 *  \ingroup intConstSystem */
#ifdef FM_SUPPORT_SWAG
#define FM_NUM_LOCAL_LAGS                   0
#else
#define FM_NUM_LOCAL_LAGS                   24
#endif

/** The number of LAGs that will be usable for a multi switch system.
 *  The handles for these lags must be allocated with a call to
 *  fmAllocateStackLAGs().
 *  \ingroup intConstSystem */
#ifdef FM_SUPPORT_SWAG
#define FM_NUM_STACK_LAGS                   128
#else
#define FM_NUM_STACK_LAGS                   104
#endif

/** The maximum number of link aggregation groups that may be created.
 *  Note that this number may be greater than the number of LAGs supported
 *  by one switch device.
 *  \ingroup constSystem */
#define FM_MAX_NUM_LAGS                     (FM_NUM_LOCAL_LAGS + FM_NUM_STACK_LAGS)


/** The maximum number of ports that can belong to a single link aggregation
 *  group. Note that this number may be greater than the number of ports
 *  per LAG supported by one switch device.
 *  \ingroup constSystem */
#define FM_MAX_NUM_LAG_MEMBERS              32


/** The maximum number of multicast groups. Note that this number must be a
 *  multiple of 256.
 *  \ingroup constSystem */
#define FM_MAX_NUM_MULTICAST_GROUP          4096


/** The maximum number of replication groups.
 *  \ingroup intConstSystem */
#define FM_MAX_NUM_REPLICATION_GROUP        4096


/** The default VLAN number that is reserved for internal use by the API
 *  for FM4000 devices. One VLAN must be held in reserve for use in 
 *  the per-VLAN and multiple spanning tree modes (see the 
 *  ''FM_SPANNING_TREE_MODE'' switch attribute). If either of these spanning
 *  tree modes is to be used, the recommended value for the reserved VLAN 
 *  is 4095. If only shared spanning tree mode will be used, the reserved 
 *  VLAN can be set to zero to indicate that no VLAN is held in reserve.
 *                                                                     \lb\lb
 *  The ''FM_RESERVED_VLAN'' switch attribute cannot be used to change the 
 *  reserved VLAN at run-time on FM4000 devices.
 *  \ingroup constSystem
 *  \chips  FM4000 */
#define FM_FM4000_RESERVED_VLAN             0


/** The number of TCN FIFO entries that will be processed before allowing
 *  other MA Table maintenance operations to occur.
 *  \ingroup constSystem
 */
#define FM_MAX_MA_TABLE_FIFO_ITERATIONS     512

/** Controls whether BST Validation functions are called when BST contents
 *  are being manipulated. Use FM_ENABLED to include the BST validation
 *  functions or FM_DISABLED to compile out these function calls.
 *  Compiling out these validation macro calls will result in faster BST
 *  update execution times at the possible cost of not uncovering BST
 *  API errors.
 *  
 *  \ingroup constSystem */
#define FM_PLATFORM_VALIDATE_BST_STRUCTS    FM_DISABLED

/** Controls whether BST Lookup Validation functions are called when BST
 *  contents are being manipulated. Use FM_ENABLED to include the BST Lookup
 *  validation functions or FM_DISABLED to compile out these function calls.
 *  Compiling out these validation macro calls will result in faster BST
 *  update execution times at the possible cost of not uncovering BST
 *  API errors.
 *  
 *  \ingroup constSystem */
#define FM_PLATFORM_VALIDATE_BST_LOOKUP     FM_DISABLED


/*****************************************************************************
 * Customer-Configurable Per-Switch Sizing Constants
 *  Note that some switch models may support less than the numbers used here,
 *  and will report the appropriate error if an invalid value is used, but
 *  that no switches will be able to support more than the maximum values
 *  listed in this section.
 *****************************************************************************/

/** Define the maximum number of traffic classes supported. */
#define FM_MAX_TRAFFIC_CLASSES              8


/** Define the maximum number of memory partitions supported per device. */
#define FM_MAX_MEMORY_PARTITIONS            2


/** Define the maximum number of switch priorities supported per device. */
#define FM_MAX_SWITCH_PRIORITIES            16


/** Define the maximum number of VLAN priorities supported per device. */
#define FM_MAX_VLAN_PRIORITIES              16


/** Define the maximum number of DSCP priorities supported per device. */
#define FM_MAX_DSCP_PRIORITIES              64


/*****************************************************************************
 * Customer-Configurable Switch Aggregate Sizing Constants
 *****************************************************************************/

/** The maximum number of switch aggregates.
 *  \ingroup constSystem */
#define FM_MAX_SWITCH_AGGREGATES            1


/*****************************************************************************
 * Customer-Configurable Switch Aggregate Topology Solvers
 *****************************************************************************/

/** Whether Fat Tree Switch Aggregate Topology is supported.
 *  Use FM_ENABLED to enable Fat Tree support, FM_DISABLED to disallow it.
 *  \ingroup constSystem
 */
#define FM_INCLUDE_FAT_TREE_TOPOLOGY        FM_ENABLED

/** Whether Ring Switch Aggregate Topology is supported.
 *  Use FM_ENABLED to enable Ring support, FM_DISABLED to disallow it.
 *  \ingroup constSystem
 */
#define FM_INCLUDE_RING_TOPOLOGY            FM_DISABLED

/** Whether Mesh Switch Aggregate Topology is supported.
 *  \ingroup constSystem
 *  Use FM_ENABLED to enable Mesh support, FM_DISABLED to disallow it. */
#define FM_INCLUDE_MESH_TOPOLOGY           FM_ENABLED

/** Maximum number of switches in a switch aggregate.
 *  \ingroup constSystem */
#define FM_MAX_SWITCHES_PER_SWAG            FM_MAX_NUM_FOCALPOINTS

/** Maximum number of VN Tunnels supported per switch in a SWAG.
 *  \ingroup constSystem
 *
 *  If not defined in the platform_defines.h file, it defaults to
 *  32768. */
#define FM_MAX_VN_TUNNELS_PER_SWAG_SWITCH  32768


/*****************************************************************************
 * Customer-Configurable Switch Aggregate Constants
 * --  Used for GLORT Resource Allocations --
 *****************************************************************************/

/** The maximum number of local ports supported on a single physical switch
 *  within a switch aggregate.
 *  \ingroup constSystem
 */
#define FM_MAX_LOCAL_PORTS_PER_SWAG_SWITCH  64

/** The maximum number of LAGs supported by a switch aggregate.
 *  \ingroup constSystem
 */
#define FM_MAX_LAGS_PER_SWAG                FM_MAX_NUM_LAGS

/** The maximum number of customer LAGs supported by a switch aggregate.
 *  \ingroup constSystem */
#define FM_MAX_EXTERNAL_LAGS_PER_SWAG      32

/** The maximum number of ports supported per internal or external LAG
 *  in a switch aggregate.
 *  \ingroup constSystem
 */
#define FM_MAX_PORTS_PER_SWAG_LAG           16

/** The maximum number of glorts supported per LAG in a switch aggregate.
 *  \ingroup constSystem */
#define FM_SWAG_GLORTS_PER_LAG             32

/** The maximum number of Load-Balancing Groups supported by a switch
 *  aggregate.
 *  \ingroup constSystem
 */
#define FM_MAX_LBGS_PER_SWAG                512

/** The maximum number of bins per LBG in a switch aggregate.
 *  \ingroup constSystem
 */
#define FM_MAX_LBG_BINS_PER_SWAG_LBG        16

/** The maximum number of multicast groups supported in a switch aggregate.
 *  \ingroup constSystem
 */
#define FM_MAX_MCAST_GROUPS_PER_SWAG        4096

/** The maximum number of Next Hops per ECMP Group supported in a switch
 *  aggregate.
 * \ingroup constSystem  */
#define FM_MAX_ECMP_GROUP_SIZE_SWAG        16

#define FM_MAX_L234_LBGS_PER_SWAG   128

#define FM_SWAG_GLORTS_PER_L234_LBG 1

#define FM_MAX_PORTS_PER_SWAG_L234LBG 16

/*****************************************************************************
 * Maximum number of switches possible (physical and logical)
 *****************************************************************************/
#define FM_MAX_NUM_SWITCHES \
    (FM_MAX_NUM_FOCALPOINTS + FM_MAX_SWITCH_AGGREGATES + 1)

#ifdef PLATFORM_FIRST_FOCALPOINT
#define FM_FIRST_FOCALPOINT                PLATFORM_FIRST_FOCALPOINT
#else
#define FM_FIRST_FOCALPOINT                0
#endif

#define FM_LAST_FOCALPOINT      \
    (FM_FIRST_FOCALPOINT + FM_MAX_NUM_FOCALPOINTS - 1)


/*****************************************************************************
 * Customer-Configurable Debug Constants
 *****************************************************************************/
#define FM_DBG_MAX_MONITORED_REGS           16
#define FM_DBG_MAX_SNAPSHOT_REGS            700000

/*
 * Note that the calling-sequence to the fmDbgCompareChipSnapshots function
 * assumes that there will only be a maximum of 32 snapshots.  If this
 * number ever needs to be increased, that calling-sequence will have to
 * be changed. (It receives a bit-mask of the snapshots to be compared.)
 */
#define FM_DBG_MAX_SNAPSHOTS                32
#define FM_DBG_MAX_PACKET_SIZE              10240
#define FM_DBG_TRACE_BFR_SIZE               1024
#define FM_DBG_EXCLUSION_TABLE_SIZE         128
#define FM_DBG_MAX_TIMER_MEAS               16


/** The maximum number of diagnostic eye diagram data sets that
 *  may be captured and retained simultaneously.
 *  \ingroup constSystem
 */
#define FM_DBG_MAX_EYE_DIAGRAMS             4


/*****************************************************************************
 * Platform-Specific Constants
 *****************************************************************************/

/** Default EBI (External Bus Interface) clock speed in MHZ. The EBI is the bus
 *  between the CPU and the switch device. This parameter can be overridden
 *  by the ''api.system.ebi.clock'' API property.
 *  \ingroup constSystem
 *  \chips  FM2000, FM4000 */ 
#define FM_EBI_MHZ                          0


/*****************************************************************************
 * Per-Switch Routing Support Constants
 *****************************************************************************/

/** \ingroup constSystem
 * @{ */

/* FM_MAX_ROUTES defaults to 24 FFU slices * 1024 entries per slice */
/** The maximum number of routes permitted. */
#define FM_MAX_ROUTES                       (24 * 1024)

/** The maximum number of ARP table entries permitted. */
#define FM_MAX_ARPS                         16384

/** The maximum number of router interfaces permitted. */
#define FM_MAX_IP_INTERFACES                1024

/** The maximum number of virtual router identifiers permitted. Virtual
 *  router identifiers are in the range 1 to FM_MAX_VIRTUAL_ROUTERS-1.
 *  Note that the number of routers that may actually be created depends
 *  on the underlying switch. */
#define FM_MAX_VIRTUAL_ROUTERS              256

/** The maximum frame size used for internal ports in a SWAG.  Can be reduced
 *  if jumbo frame support is not required. */
#define FM_INTERNAL_PORT_MAX_FRAME_SIZE    15352

/** The default 12-bit Lag Hash Rotation value to use for internal LAGs
 *  in a switch aggregate. */
#define FM_DEFAULT_SWAG_LAG_HASH_ROT_VAL   3

/** The default LAG Hash Rotation to use for internal LAGs in a switch
 *  aggregate.  As required by LAG attribute FM_LAG_HASH_ROTATION,
 *  a value of zero implies rotation A.  A value of 1 implies rotation B. */
#define FM_DEFAULT_SWAG_LAG_HASH_ROTATION  1

/** @} (end of Doxygen group) */

#define FM_GLOBAL_NETDEV_NAME               "fm0-0"


/**************************************************
 * White Model Support
 **************************************************/

/**************************************************/
/** White Model Constants
 *  \ingroup constModel
 *  \page whiteModelConstants
 *
 *  When the White Model is instantiated by a
 *  platform layer implementation, the platform
 *  must define the following set of constants.
 **************************************************/
/*- \ingroup constModel
 * @{ */

/** In conjunction with ''FM_MODEL_TICK_NANOSECS'', indicates the 
 *  periodicity with which calls will be made to fm10000ModelTick by the 
 *  platform layer implementation. That function will use these values to
 *  calculate actual elapsed wall clock time for MAC address table aging and
 *  other time-based processing. FM_MODEL_TICK_SECS is the number
 *  of whole seconds between calls to fm10000ModelTick (and may be zero
 *  for sub-second periodicities) and ''FM_MODEL_TICK_NANOSECS'' is 
 *  the additional fractional part of a second in nanoseconds. */
#define FM_MODEL_TICK_SECS                  0

/** See ''FM_MODEL_TICK_SECS''. */
#define FM_MODEL_TICK_NANOSECS              1000000


/** The maximum packet size for messages sent to the model. Note that 
 *  the model can only change the first 256 bytes of a packet and 
 *  potentially add 48 bytes. The rest is a simple copy. So there isn't much 
 *  value in using the model for packets greater than 256. Packet size 
 *  must be big enough to allow expansion of up to 48 bytes. Packet size
 *  must also be big enough to support jumbo frames. */
#define FM_MODEL_MAX_PACKET_SIZE            /* 65791 */ 32767

/** The maximum size in units of bytes reserved for sideband type-length-value
 *  (TLV) elements. */
#define FM_MODEL_MAX_TLV_SIZE               256

/** When the white model packet queue interface is used, this constant 
 *  indicates the TCP port number on which the packet queue interface will 
 *  listen for command and data packets. If specified as zero, a port number 
 *  will be automatically allocated. Whether specified or automatically 
 *  allocated, the actual port number used will be reported in the first 
 *  line of the text file models.packetServer. See ''White Model Packet 
 *  Queue Interface'' in the Software API User Guide for more information. */
#define FM_MODEL_PKTQ_SERVER_PORT           0

/** The white model packet queue interface implements a queue to hold packets 
 *  egressing the switch model. When packets are removed from the queue,
 *  they are either passed into the API (if they egressed the CPU port on
 *  the switch model) or are sent on a socket previously registered on the
 *  egress port by the application. When the white model packet queue interface 
 *  is used, this constant indicates the maximum number of packets that the
 *  queue will hold. */
#define FM_MODEL_PKTQ_MAX_PACKETS           100

/*- @} (end of Doxygen group) */


/***************************************************************************/
/*- \ingroup macroPlatform
 *  When the White Model is instantiated by a platform layer 
 *  implementation, the platform must define this macro, which provides 
 *  a pointer to the white model state structure.
 *                                                                      \lb\lb
 *  sw is the switch number for the model instance whose state structure
 *  is to be retrieved.
 *                                                                      \lb\lb
 *  This sample implementation is from the altaWhiteModel platform where
 *  the pointer to the model state is kept in the platform's own state
 *  structure, but there is no requirement for this to be the case. For
 *  example, this macro could be implemented to call a function which
 *  returns the pointer.
 ***************************************************************************/
#define FM_MODEL_GET_STATE(sw) (fmRootPlatform->platformState[(sw)].chipModel)


/***************************************************************************/
/*  Platform Macros
 ***************************************************************************/

/** \ingroup macroPlatform
 * @{ */

/***************************************************************************/
/** Called by the API, this macro captures the switch register lock,
 *  defined by the platform, for ensuring atomic access to switch registers.
 *  Normally this lock is used within the platform layer to make multi-word
 *  register and read-modify-write accesses be atomic.
 *                                                                      \lb\lb
 *  The API will use these macros to operate on the same lock when atomic
 *  access is required within the API for read-modify-write operations or
 *  to keep multiple registers in sync.
 *                                                                      \lb\lb
 *  sw is the switch on which to operate.
 ***************************************************************************/
#define FM_PLAT_TAKE_REG_LOCK(sw)                                                    \
    fmCaptureLock(&fmRootPlatform->platformState[(sw)].accessLocks[FM_MEM_TYPE_CSR], \
                  FM_WAIT_FOREVER);

/***************************************************************************/
/** Called by the API, this macro releases the switch register lock
 *  previously captured by ''FM_PLAT_TAKE_REG_LOCK''.
 *                                                                      \lb\lb
 *  sw is the switch on which to operate.
 ***************************************************************************/
#define FM_PLAT_DROP_REG_LOCK(sw)                                                    \
    fmReleaseLock(&fmRootPlatform->platformState[(sw)].accessLocks[FM_MEM_TYPE_CSR]);


/** @} (end of Doxygen group) */

#endif /* __FM_PLATFORM_DEFINES_H */
