/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_alos_logging.h
 * Creation Date:   June 18, 2007
 * Description:     The SDK logging subsystem
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

#ifndef __FM_FM_ALOS_LOGGING_H
#define __FM_FM_ALOS_LOGGING_H


/***************************************************/
/** The size of the character buffer used for
 *  collecting the call stack function names
 *  when ''FM_LOG_CALL_STACK'' is used. Should be
 *  set large enough to accommodate 
 *  ''FM_MAX_CALL_STACK_SIZE''.
 * \ingroup constSystem
 **************************************************/
#define FM_MAX_CALL_STACK_BFR_SIZE                  1024


/***************************************************/
/** The maximum call stack depth that will be reported
 *  by ''FM_LOG_CALL_STACK''.
 * \ingroup constSystem
 **************************************************/
#define FM_MAX_CALL_STACK_SIZE                      50


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are the different logging
 * destination types that may be specified when
 * generating an output log. This type is used as
 * an argument to the ''fmSetLoggingType'' function.
 *                                                                      \lb\lb
 * The "arg" argument to ''fmSetLoggingType'' depdends
 * on the log type selected. See below for details.
 *                                                                      \lb\lb
 * Prior to calling ''fmSetLoggingType'', all
 * logging will go to the console by default.
 **************************************************/
typedef enum
{
    /** Log to the console. The "arg" argument to ''fmSetLoggingType'' is
     *  not used and may be set to NULL. */
    FM_LOG_TYPE_CONSOLE = 0,

    /** Log to a file. The "arg" argument to ''fmSetLoggingType'' is the
     *  name of the file to output log messages to. */
    FM_LOG_TYPE_FILE,

    /** Log to a memory buffer. The "arg" argument to ''fmSetLoggingType'' is
     *  not used and may be set to NULL. */
    FM_LOG_TYPE_MEMBUF,

    /** Execute a call-back function, passing the log string to the function.
     *  The "arg" argument to ''fmSetLoggingType'' is a pointer to a structre
     *  of type ''fm_logCallBackSpec'' (see). If arg is NULL, all log messages
     *  will be suppressed.
     *                                                                  \lb\lb
     *  This logging type is useful for environments where the process in
     *  whose context the API executes does not own the console and logging
     *  output needs to be redirected by that process to another. */
    FM_LOG_TYPE_CALLBACK

} fm_loggingType;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are the different logging
 * Object ID range types that may be specified when
 * configuring an object ID range using 
 * ''fmSetLoggingCategoryConfig''
 **************************************************/
typedef enum
{
    /** infinite range, no log messages will be filtered out based on
     *  object ID  */
    FM_LOG_RANGE_TYPE_INFINITE = 0,

    /** finite object ID range to be identified via min and max object IDs */
    FM_LOG_RANGE_TYPE_FINITE ,

    /** empty range, all log messages will be filtered out based on object
     *  ID */
    FM_LOG_RANGE_TYPE_EMPTY,

    /** defined only for sanity-checking purposes, do not remove */
    FM_LOG_RANGE_TYPE_MAX

} fm_loggingRangeType;


/****************************************************************************/
/** \ingroup constLogAttr
 *
 *  Log Attributes are read-only and are used as an argument to 
 *  ''fmGetLoggingAttribute''.
 *                                                                      \lb\lb
 *  For each attribute, the data type of the corresponding attribute value is
 *  indicated.
 ****************************************************************************/
enum _fm_loggingAttr
{
    /** Type fm_bool: Indicates whether the logging system is enabled.
     *  Values are FM_ENABLED and FM_DISABLED. */
    FM_LOG_ATTR_ENABLED = 0,

    /** Type fm_int: The destination type, as set by ''fmSetLoggingType''. */
    FM_LOG_ATTR_LOG_TYPE,

    /** Type fm_uint64: Bit mask of category flags.
     *  See ''Log Categories'' for a list of flag bits. */
    FM_LOG_ATTR_CATEGORY_MASK,

    /** Type fm_uint64: Bit mask of level flags.
     *  See ''Log Levels'' for a list of flag bits. */
    FM_LOG_ATTR_LEVEL_MASK,

    /** Type fm_uint32: Bit mask of verbosity flags, as specified by
     *  ''fmSetLoggingVerbosity''. See ''Log Verbosity Flags'' for a list of
     *  flag bits. */
    FM_LOG_ATTR_VERBOSITY_MASK,

    /** Type fm_char: Name of the source code file on which to filter,
     *  as specified by ''fmSetLoggingFilter''.  */
    FM_LOG_ATTR_FILE_FILTER,

    /** Type fm_char: Name of the source function on which to filter, as
     *  specified by ''fmSetLoggingFilter''. */
    FM_LOG_ATTR_FUNCTION_FILTER,

    /** Type fm_char: Name of the file to which logging messages are sent,
     *  as specified by ''fmSetLoggingType''. */
    FM_LOG_ATTR_LOG_FILENAME,

    /** UNPUBLISHED: For internal use only. */
    FM_LOG_ATTRIBUTE_MAX

};  /* end enum _fm_loggingAttr */


/**************************************************/
/** \ingroup typeScalar
 * A logging call-back function, provided by
 * the application and called by the ALOS when
 * generating logging output. Register the call-back
 * as a member of the ''fm_logCallBackSpec''
 * structure that is passed to ''fmSetLoggingType''.
 *                                                                      \lb\lb
 * The call-back function returns void and takes
 * as arguments:
 *                                                                      \lb\lb
 *  fm_text buf - A buffer pointer that points to the
 *                string to be output.
 *                                                                      \lb\lb
 *  fm_voidptr cookie1 - Arbitrary data that the
 *                application registers as a member
 *                of the ''fm_logCallBackSpec''
 *                structure that is passed to
 *                ''fmSetLoggingType''.
 *                                                                      \lb\lb
 *  fm_voidptr cookie2 - Arbitrary data that the
 *                application registers as a member
 *                of the ''fm_logCallBackSpec''
 *                structure that is passed to
 *                ''fmSetLoggingType''.
 **************************************************/
typedef void (*fm_logCallBack)(fm_text buf, fm_voidptr cookie1, fm_voidptr cookie2);


/**************************************************/
/** \ingroup typeStruct
 *  Used as an argument to ''fmSetLoggingType''.
 *  The "arg" argument to ''fmSetLoggingType'' must
 *  be of type fm_logCallBackSpec when the logType
 *  argument of that function (type ''fm_loggingType'')
 *  is set to ''FM_LOG_TYPE_CALLBACK''.
 **************************************************/
typedef struct _fm_logCallBackSpec
{
    /** Pointer to the call-back function that will be called with the log
     *  output. */
    fm_logCallBack callBack;

    /** Passed to callBack as its second argument. */
    fm_voidptr     cookie1;

    /** Passed to callBack as its third argument. */
    fm_voidptr     cookie2;

} fm_logCallBackSpec;


/**************************************************/
/** Log Levels
 * \ingroup constLogLevel
 * \page logLevel
 *
 * Log messages are assigned a severity level.
 * These bit masks are used as flags for filtering
 * log messages as they are generated or dumped.
 * Up to 64 levels are supported.
 * When used as an argument to the logging functions
 * (e.g., ''fmEnableLoggingLevel''), multiple
 * bits may be ORed together.
 **************************************************/
 
/**************************************************
 * Note: When adding new log levels, be sure to
 * add a corresponding entry to the switch(logLevel) 
 * statement in the function fmLogMessage.
 **************************************************/

/** \ingroup constLogLevel
 * @{ */

/** Function entry point. */
#define FM_LOG_LEVEL_FUNC_ENTRY  \
    ( (FM_LITERAL_U64(1) << 0) | \
     (FM_LITERAL_U64(1) << 10) )

/** Function exit point. */
#define FM_LOG_LEVEL_FUNC_EXIT   \
    ( (FM_LITERAL_U64(1) << 1) | \
     (FM_LITERAL_U64(1) << 11) )

/** Warning - an unexpected event, behavior or result occurred, however the
 *  software is able to continue. */
#define FM_LOG_LEVEL_WARNING          (FM_LITERAL_U64(1) << 2)

/** Error - an unexpected event, behavior or result occurred that indicates
 *  a significant problem. */
#define FM_LOG_LEVEL_ERROR            (FM_LITERAL_U64(1) << 3)

/** Fatal Error - an unexpected event, behavior or result occurred that
 *  prevents the software from continuing to operate successfully. */
#define FM_LOG_LEVEL_FATAL            (FM_LITERAL_U64(1) << 4)

/** An informational message. */
#define FM_LOG_LEVEL_INFO             (FM_LITERAL_U64(1) << 5)

/** A message generated for debugging purposes. */
#define FM_LOG_LEVEL_DEBUG            (FM_LITERAL_U64(1) << 6)

/** Used by functions whose sole purpose is to print out information
 *  when called. This level indicates that printing should occur unconditionally
 *  regardless of what log category is used and whether logging is enabled at
 *  all. Typically the functions that employ FM_LOG_LEVEL_PRINT are supposed
 *  to generate output on-demand only. */
#define FM_LOG_LEVEL_PRINT            (FM_LITERAL_U64(1) << 7)

/** A message generated for debugging purposes. */
#define FM_LOG_LEVEL_DEBUG2           (FM_LITERAL_U64(1) << 8)

/** A message generated for debugging purposes. */
#define FM_LOG_LEVEL_DEBUG3           (FM_LITERAL_U64(1) << 9)

/** Function entry point for published functions. */
#define FM_LOG_LEVEL_FUNC_ENTRY_API         (FM_LITERAL_U64(1) << 10)

/** Function exit point for published functions. */
#define FM_LOG_LEVEL_FUNC_EXIT_API          (FM_LITERAL_U64(1) << 11)

/** Published function entry and exit points (combines
 *  ''FM_LOG_LEVEL_FUNC_ENTRY_API'' and ''FM_LOG_LEVEL_FUNC_EXIT_API''). */
#define FM_LOG_LEVEL_API              (FM_LITERAL_U64(3) << 10)

/** Function entry point for functions that are called frequently. */
#define FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE     (FM_LITERAL_U64(1) << 12)

/** Function exit point for frequently-called functions. */
#define FM_LOG_LEVEL_FUNC_EXIT_VERBOSE      (FM_LITERAL_U64(1) << 13)

/** Function entry point for frequently-called public functions. */
#define FM_LOG_LEVEL_FUNC_ENTRY_API_VERBOSE (FM_LITERAL_U64(1) << 14)

/** Function exit point for frequently-called public functions. */
#define FM_LOG_LEVEL_FUNC_EXIT_API_VERBOSE  (FM_LITERAL_U64(1) << 15)

/** A message generated for verbose debugging purposes. */
#define FM_LOG_LEVEL_DEBUG_VERBOSE          (FM_LITERAL_U64(1) << 16)

/** A message generated for verbose debugging purposes. */
#define FM_LOG_LEVEL_ASSERT                 (FM_LITERAL_U64(1) << 17)


/**************************************************
 * Note: When adding new log levels, be sure to
 * add a corresponding entry to the switch(logLevel) 
 * statement in the function fmLogMessage.
 **************************************************/

/***************************************************
 * Make the "ALL" log level include all logging levels
 * except for the "verbose" levels.
 **************************************************/
 
/** All log levels combined except for verbose levels. */
#define FM_LOG_LEVEL_ALL        \
    (FM_LOG_LEVEL_FUNC_ENTRY |  \
     FM_LOG_LEVEL_FUNC_EXIT  |  \
     FM_LOG_LEVEL_WARNING    |  \
     FM_LOG_LEVEL_ERROR      |  \
     FM_LOG_LEVEL_FATAL      |  \
     FM_LOG_LEVEL_INFO       |  \
     FM_LOG_LEVEL_DEBUG      |  \
     FM_LOG_LEVEL_PRINT      |  \
     FM_LOG_LEVEL_DEBUG2     |  \
     FM_LOG_LEVEL_DEBUG3     |  \
     FM_LOG_LEVEL_API        |  \
     FM_LOG_LEVEL_ASSERT)

/** All log levels including verbose levels. */
#define FM_LOG_LEVEL_ALL_VERBOSE            \
    (FM_LOG_LEVEL_ALL                    |  \
     FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE     |  \
     FM_LOG_LEVEL_FUNC_EXIT_VERBOSE      |  \
     FM_LOG_LEVEL_FUNC_ENTRY_API_VERBOSE |  \
     FM_LOG_LEVEL_FUNC_EXIT_API_VERBOSE  |  \
     FM_LOG_LEVEL_DEBUG_VERBOSE)

/** Default logging level. */
#define FM_LOG_LEVEL_DEFAULT    \
    (FM_LOG_LEVEL_FATAL   |     \
     FM_LOG_LEVEL_ERROR   |     \
     FM_LOG_LEVEL_PRINT   |     \
     FM_LOG_LEVEL_WARNING |     \
     FM_LOG_LEVEL_ASSERT)

/** No logging levels. */
#define FM_LOG_LEVEL_NONE       FM_LITERAL_U64(0)

/** @} (end of Doxygen group) */

/* helper macro used for computing filter mask */
#define FM_LOG_LEVEL(x) \
    ( ( 1 << ( (x) + 1 ) ) - 1 )


/* the magic number for checking initialization, see below */
#define FM_LOG_MAGIC_NUMBER           0xabf3138c


/**************************************************/
/** Log Categories
 * \ingroup constLogCategory
 * \page logCat
 *
 * Log messages are categorized by functionality.
 * These bit masks are used as flags for filtering
 * log messages as they are generated or dumped.
 * Up to 64 categories are supported.
 * When used as an argument to the logging functions
 * (e.g., ''fmEnableLoggingCategory''), multiple
 * bits may be ORed together.
 **************************************************/
/** \ingroup constLogCategory
 * @{ */

/** Logging subsystem. */
#define FM_LOG_CAT_LOGGING            (FM_LITERAL_U64(1) << 0)

/** Link up/down state. */
#define FM_LOG_CAT_LINK_STATE         (FM_LITERAL_U64(1) << 1)

/** VLANs. */
#define FM_LOG_CAT_VLAN               (FM_LITERAL_U64(1) << 2)

/** VLAN spanning tree state. */
#define FM_LOG_CAT_VLAN_STP           (FM_LITERAL_U64(1) << 3)

/** ALOS activity. */
#define FM_LOG_CAT_ALOS               (FM_LITERAL_U64(1) << 4)

/** Debugging activity. */
#define FM_LOG_CAT_DEBUG              (FM_LITERAL_U64(1) << 5)

/** PHY management. */
#define FM_LOG_CAT_PHY                (FM_LITERAL_U64(1) << 6)

/** Platform layer. */
#define FM_LOG_CAT_PLATFORM           (FM_LITERAL_U64(1) << 7)

/** Event handling. */
#define FM_LOG_CAT_EVENT              (FM_LITERAL_U64(1) << 8)

/** Packet transmission event handling. */
#define FM_LOG_CAT_EVENT_PKT_TX       (FM_LITERAL_U64(1) << 9)

/** Packet reception event handling. */
#define FM_LOG_CAT_EVENT_PKT_RX       (FM_LITERAL_U64(1) << 10)

/** MA Table maintenance activity. */
#define FM_LOG_CAT_EVENT_MAC_MAINT    (FM_LITERAL_U64(1) << 11)

/** Switch management. */
#define FM_LOG_CAT_SWITCH             (FM_LITERAL_U64(1) << 12)

/** Mailbox management */
#define FM_LOG_CAT_MAILBOX            (FM_LITERAL_U64(1) << 13)

/** ALOS Dynamic-Load Library. */
#define FM_LOG_CAT_ALOS_DLLIB         (FM_LITERAL_U64(1) << 14)

/** ALOS semaphore management. */
#define FM_LOG_CAT_ALOS_SEM           (FM_LITERAL_U64(1) << 15)

/** ALOS lock management. */
#define FM_LOG_CAT_ALOS_LOCK          (FM_LITERAL_U64(1) << 16)

/** ALOS reader-writer lock management. */
#define FM_LOG_CAT_ALOS_RWLOCK        (FM_LITERAL_U64(1) << 17)

/** ALOS thread management. */
#define FM_LOG_CAT_ALOS_THREAD        (FM_LITERAL_U64(1) << 18)

/** Port management. */
#define FM_LOG_CAT_PORT               (FM_LITERAL_U64(1) << 19)

/** SerDes management. */
#define FM_LOG_CAT_SERDES             (FM_LITERAL_U64(1) << 20)

/** ALOS time library */
#define FM_LOG_CAT_ALOS_TIME          (FM_LITERAL_U64(1) << 21)

/** White model. */
#define FM_LOG_CAT_MODEL              (FM_LITERAL_U64(1) << 22)

/** Parity error management. */
#define FM_LOG_CAT_PARITY             (FM_LITERAL_U64(1) << 23)

/** (Unused) */
#define FM_LOG_CAT_SPARE24            (FM_LITERAL_U64(1) << 24)

/** (Unused) */
#define FM_LOG_CAT_SPARE25            (FM_LITERAL_U64(1) << 25)

/** API Link-Aggregation. */
#define FM_LOG_CAT_LAG                (FM_LITERAL_U64(1) << 26)

/** (Unused) */
#define FM_LOG_CAT_SPARE27            (FM_LITERAL_U64(1) << 27)

/** (Unused) */
#define FM_LOG_CAT_SPARE28            (FM_LITERAL_U64(1) << 28)

/** Link-state debouncing events. */
#define FM_LOG_CAT_EVENT_PORT         (FM_LITERAL_U64(1) << 29)

/** NAT */
#define FM_LOG_CAT_NAT                (FM_LITERAL_U64(1) << 30)

/** Tunneling Engine. */
#define FM_LOG_CAT_TE                 (FM_LITERAL_U64(1) << 31)

/** Interrupt task handling. */
#define FM_LOG_CAT_EVENT_INTR         (FM_LITERAL_U64(1) << 32)

/** (Unused) */
#define FM_LOG_CAT_STATE_MACHINE      (FM_LITERAL_U64(1) << 33)

/** RBridge management.
 *  \chips  FM6000 */
#define FM_LOG_CAT_RBRIDGE            (FM_LITERAL_U64(1) << 34)

/** API property subsystem (formerly known as API attributes). */
#define FM_LOG_CAT_ATTR               (FM_LITERAL_U64(1) << 35)

/** Switch device multicast management. */
#define FM_LOG_CAT_MULTICAST          (FM_LITERAL_U64(1) << 36)

/** Switch device storm controller management. */
#define FM_LOG_CAT_STORM              (FM_LITERAL_U64(1) << 37)

/** Routing */
#define FM_LOG_CAT_ROUTING            (FM_LITERAL_U64(1) << 38)

/** Address table management */
#define FM_LOG_CAT_ADDR               (FM_LITERAL_U64(1) << 39)

/** Mirror groups */
#define FM_LOG_CAT_MIRROR             (FM_LITERAL_U64(1) << 40)

/** Quality of Service */
#define FM_LOG_CAT_QOS                (FM_LITERAL_U64(1) << 41)

/** Trigger management. */
#define FM_LOG_CAT_TRIGGER            (FM_LITERAL_U64(1) << 42)

/** Binary Search Tree (BST). */
#define FM_LOG_CAT_BST                (FM_LITERAL_U64(1) << 43)

/** Mapper */
#define FM_LOG_CAT_MAP                (FM_LITERAL_U64(1) << 44)

/** Filtering and Forwarding Unit */
#define FM_LOG_CAT_FFU                (FM_LITERAL_U64(1) << 45)

/** ACL compiler */
#define FM_LOG_CAT_ACL                (FM_LITERAL_U64(1) << 46)

/** Spanning Tree Instances */
#define FM_LOG_CAT_STP                (FM_LITERAL_U64(1) << 47)

/** Buffer management */
#define FM_LOG_CAT_BUFFER             (FM_LITERAL_U64(1) << 48)

/** Non-specific */
#define FM_LOG_CAT_GENERAL            (FM_LITERAL_U64(1) << 49)

/** API global */
#define FM_LOG_CAT_API                (FM_LITERAL_U64(1) << 50)

/** Load balancing groups */
#define FM_LOG_CAT_LBG                (FM_LITERAL_U64(1) << 51)

/** Switch Aggregates */
#define FM_LOG_CAT_SWAG               (FM_LITERAL_U64(1) << 52)

/** Port Auto-negotiation */
#define FM_LOG_CAT_PORT_AUTONEG       (FM_LITERAL_U64(1) << 53)

/** Stacking */
#define FM_LOG_CAT_STACKING           (FM_LITERAL_U64(1) << 54)

/** sFlow */
#define FM_LOG_CAT_SFLOW              (FM_LITERAL_U64(1) << 55)

/** Address offload table management.
 *  \chips  FM4000 */
#define FM_LOG_CAT_ADDR_OFFLOAD       (FM_LITERAL_U64(1) << 56)

/** Fast maintenance activity */
#define FM_LOG_CAT_EVENT_FAST_MAINT   (FM_LITERAL_U64(1) << 57)

/** Fulcrum In-Band Management */
#define FM_LOG_CAT_FIBM               (FM_LITERAL_U64(1) << 58)

/** Application */
#define FM_LOG_CAT_APPLICATION        (FM_LITERAL_U64(1) << 59)

/** CRM */
#define FM_LOG_CAT_CRM                (FM_LITERAL_U64(1) << 60)

/** Flow API */
#define FM_LOG_CAT_FLOW               (FM_LITERAL_U64(1) << 61)

/** Virtual Networks API */
#define FM_LOG_CAT_VN                 (FM_LITERAL_U64(1) << 62)

/** (Unused) */
#define FM_LOG_CAT_SPARE63            (FM_LITERAL_U64(1) << 63)

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_SWITCH. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_SWITCH_FM2000      FM_LOG_CAT_SWITCH

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_SWITCH. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_SWITCH_FM4000      FM_LOG_CAT_SWITCH

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_PORT. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_PORT_FM2000        FM_LOG_CAT_PORT

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_PORT. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_PORT_FM4000        FM_LOG_CAT_PORT

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_VLAN. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_VLAN_FM2000        FM_LOG_CAT_VLAN

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_VLAN. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_VLAN_FM4000        FM_LOG_CAT_VLAN

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_VLAN_STP. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_VLAN_STP_FM2000    FM_LOG_CAT_VLAN_STP

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_VLAN_STP. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_VLAN_STP_FM4000    FM_LOG_CAT_VLAN_STP

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_LAG. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_LAG_FM2000         FM_LOG_CAT_LAG

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_LAG. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_LAG_FM4000         FM_LOG_CAT_LAG

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_EVENT_PORT. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_EVENT_PORT_FM2000  FM_LOG_CAT_EVENT_PORT

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_EVENT_PORT. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_EVENT_PORT_FM4000  FM_LOG_CAT_EVENT_PORT

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_QOS. 
 *  \chips  FM2000 */
#define FM_LOG_CAT_QOS_FM2000         FM_LOG_CAT_QOS

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_QOS. 
 *  \chips  FM3000, FM4000 */
#define FM_LOG_CAT_QOS_FM4000         FM_LOG_CAT_QOS

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_SWITCH. 
 *  \chips  FM6000 */
#define FM_LOG_CAT_SWITCH_FM6000      FM_LOG_CAT_SWITCH

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_PORT. 
 *  \chips  FM6000 */
#define FM_LOG_CAT_PORT_FM6000        FM_LOG_CAT_PORT

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_QOS. 
 *  \chips  FM6000 */
#define FM_LOG_CAT_QOS_FM6000         FM_LOG_CAT_QOS

/** Legacy chip-specific category. Now functions the same as 
 *  FM_LOG_CAT_VLAN. 
 *  \chips  FM6000 */
#define FM_LOG_CAT_VLAN_FM6000        FM_LOG_CAT_VLAN

/** All logging categories. */
#define FM_LOG_CAT_ALL                FM_LITERAL_U64(0xffffffffffffffff)

/** All ALOS categories. */
#define FM_LOG_CAT_ALOS_ALL           (FM_LOG_CAT_ALOS_DLLIB        |   \
                                       FM_LOG_CAT_ALOS_SEM          |   \
                                       FM_LOG_CAT_ALOS_LOCK         |   \
                                       FM_LOG_CAT_ALOS_TIME         |   \
                                       FM_LOG_CAT_ALOS_RWLOCK       |   \
                                       FM_LOG_CAT_ALOS_THREAD)

/** All interrupt task categories. */
#define FM_LOG_CAT_EVENT_INTR_ALL     FM_LOG_CAT_EVENT_INTR

/** All port link-state event categories. */
#define FM_LOG_CAT_EVENT_PORT_ALL     (FM_LOG_CAT_EVENT_PORT        |   \
                                       FM_LOG_CAT_EVENT_PORT_FM2000 |   \
                                       FM_LOG_CAT_EVENT_PORT_FM4000)

/** All link aggregation group categories. */
#define FM_LOG_CAT_LAG_ALL            (FM_LOG_CAT_LAG               |   \
                                       FM_LOG_CAT_LAG_FM2000        |   \
                                       FM_LOG_CAT_LAG_FM4000)

/** All port management categories. */
#define FM_LOG_CAT_PORT_ALL           (FM_LOG_CAT_PORT              |   \
                                       FM_LOG_CAT_PORT_FM2000       |   \
                                       FM_LOG_CAT_PORT_FM4000       |   \
                                       FM_LOG_CAT_PORT_FM6000)

/** All quality of service categories. */
#define FM_LOG_CAT_QOS_ALL            (FM_LOG_CAT_QOS               |   \
                                       FM_LOG_CAT_QOS_FM2000        |   \
                                       FM_LOG_CAT_QOS_FM4000)

/** All switch management categories. */
#define FM_LOG_CAT_SWITCH_ALL         (FM_LOG_CAT_SWITCH            |   \
                                       FM_LOG_CAT_SWITCH_FM2000     |   \
                                       FM_LOG_CAT_SWITCH_FM4000     |   \
                                       FM_LOG_CAT_SWITCH_FM6000)

/** All VLAN activity categories. */
#define FM_LOG_CAT_VLAN_ALL           (FM_LOG_CAT_VLAN              |   \
                                       FM_LOG_CAT_VLAN_FM2000       |   \
                                       FM_LOG_CAT_VLAN_FM4000)

/** All VLAN spanning tree state categories. */
#define FM_LOG_CAT_VLAN_STP_ALL       (FM_LOG_CAT_VLAN_STP          |   \
                                       FM_LOG_CAT_VLAN_STP_FM2000   |   \
                                       FM_LOG_CAT_VLAN_STP_FM4000)

/** Default logging category. */
#define FM_LOG_CAT_DEFAULT            FM_LOG_CAT_ALL

/** No logging categories. */
#define FM_LOG_CAT_NONE               FM_LITERAL_U64(0)

/* Obsolete but defined for backwards compatibility */
#define FM_LOG_CAT_EVENT_ROUTING      FM_LOG_CAT_ROUTING

/** @} (end of Doxygen group) */


/**************************************************/
/** Log Verbosity Flags
 * \ingroup constLogVerbosity
 * \page logVerbose
 *
 * Each log message is output with a preamble
 * comprising a time stamp and the location in
 * the source code from which the log message was
 * generated. The preamble can make the total log
 * message quite long. The following set of flag bit
 * masks can be ORed together and used as an argument to
 * ''fmSetLoggingVerbosity'' to control which
 * parts of the preamble should or should not be
 * output. By default, all flags are enabled for
 * maximum verbosity.
 **************************************************/
/** \ingroup constLogVerbosity
 * @{ */

/** Include the date and timestamp in the logging message */
#define FM_LOG_VERBOSITY_DATE_TIME    (1 << 0)

/** Include the logging level in the logging message */
#define FM_LOG_VERBOSITY_LOG_LEVEL    (1 << 1)

/** Include the calling thread in the logging message */
#define FM_LOG_VERBOSITY_THREAD       (1 << 2)

/** Include the filename in the logging message */
#define FM_LOG_VERBOSITY_FILE         (1 << 3)

/** Include the function name in the logging message */
#define FM_LOG_VERBOSITY_FUNC         (1 << 4)

/** Include the line number in the logging message */
#define FM_LOG_VERBOSITY_LINE         (1 << 5)

/** All verbosity flags. */
#define FM_LOG_VERBOSITY_ALL            \
    (FM_LOG_VERBOSITY_DATE_TIME |       \
     FM_LOG_VERBOSITY_LOG_LEVEL |       \
     FM_LOG_VERBOSITY_THREAD    |       \
     FM_LOG_VERBOSITY_FILE      |       \
     FM_LOG_VERBOSITY_FUNC      |       \
     FM_LOG_VERBOSITY_LINE)

/** No verbosity flags. */
#define FM_LOG_VERBOSITY_NONE           0

/** Default verbosity in the logging message. */
#define FM_LOG_VERBOSITY_DEFAULT        \
    (FM_LOG_VERBOSITY_LOG_LEVEL |       \
     FM_LOG_VERBOSITY_THREAD    |       \
     FM_LOG_VERBOSITY_FUNC      |       \
     FM_LOG_VERBOSITY_LINE)

/** @} (end of Doxygen group) */


fm_status fmLoggingEnable(void);
fm_status fmLoggingDisable(void);

fm_status fmSetLoggingType(fm_loggingType logType,
                           fm_bool        clear,
                           void *         arg);

fm_status fmSetLoggingVerbosity(fm_uint32 verbosityMask);
fm_status fmGetLoggingVerbosity(fm_uint32 *verbosityMask);

fm_status fmEnableLoggingCategory(fm_uint64 categories);
fm_status fmDisableLoggingCategory(fm_uint64 categories);
fm_status fmEnableLoggingLevel(fm_uint64 levels);
fm_status fmDisableLoggingLevel(fm_uint64 levels);

fm_status fmSetLoggingFilter(fm_uint64   categoryMask,
                             fm_uint64   levelMask,
                             const char *functionFilter,
                             const char *fileFilter);

fm_status fmLogMessage(fm_uint64   categories,
                       fm_uint64   logLevel,
                       const char *srcFile,
                       const char *srcFunction,
                       fm_uint32   srcLine,
                       const char *format,
                       ...)
#ifdef __GNUC__
__attribute__ ( ( format(printf, 6, 7) ) )
#endif
;

fm_status fmLogMessageV2(fm_uint64   categories,
                         fm_uint64   logLevel,
                         fm_int      objectId,
                         const char *srcFile,
                         const char *srcFunction,
                         fm_uint32   srcLine,
                         const char *format,
                         ...)
#ifdef __GNUC__
__attribute__ ( ( format(printf, 7, 8) ) )
#endif
;


fm_status fmLogBufferDump(void *    threadID,
                          fm_text   srcFile,
                          fm_text   srcFunction,
                          fm_uint32 srcLine);

fm_status fmGetLogBuffer(fm_text buffer, fm_int size);

fm_status fmGetLoggingAttribute(fm_int  attr,
                                fm_int  size,
                                void *  value);

fm_status fmResetLogging(void);

void fmGetCallerName(fm_text buf,
                     fm_int  bufSize,
                     fm_int  callerCount,
                     fm_text delimiter);

fm_status fmSetLoggingCategoryConfig( fm_uint64           categoryMask,
                                      fm_loggingRangeType rangeType,
                                      fm_int              minObjectId,
                                      fm_int              maxObjectId,
                                      fm_bool             legacyLoggingOn );

fm_status fmGetLoggingCategoryConfig( fm_uint64            categoryMask,
                                      fm_loggingRangeType *rangeType,
                                      fm_int              *minObjectId,
                                      fm_int              *maxObjectId,
                                      fm_bool             *legacyLoggingOn );

#if defined(FM_ALOS_FAULT_INJECTION_POINTS) && (FM_ALOS_FAULT_INJECTION_POINTS==FM_ENABLED)

fm_status fmActivateFaultInjectionPoint(const char *functionName,
                                        fm_status   error);

fm_status fmFaultInjection(const char *functionName);

#endif

/**************************************************/
/** Logging Macros
 * \ingroup macroLog
 * \page logMacros
 *
 * The following macros are provided to simplify the
 * most common calls to ''fmLogMessage'' and
 * ''fmLogMessageV2''
 *                                                                      \lb\lb
 * NOTE: In order to get around some usage issues with
 * regard to variadic macros, the format string is
 * included as part of the variable argument list.
 **************************************************/

/** \ingroup macroLog
 * @{ */

/***************************************************************************/
/** UNPUBLISHED: Fault injection point macro. May be used in functions
 *  returning fm_status to simulate error.
 *
 *  Fault injection point must be activated by 
 *  fmActivateFaultInjectionPoint function.
 *
 *  This macro is enabled when FM_ALOS_FAULT_INJECTION_POINTS is set to 
 *  FM_ENABLED.
 ***************************************************************************/
#if defined(FM_ALOS_FAULT_INJECTION_POINTS) && (FM_ALOS_FAULT_INJECTION_POINTS==FM_ENABLED)
#define FM_FAULT_INJECTION_POINT()                                      \
    {                                                                   \
        fm_status faultInjectionError = fmFaultInjection(__func__);     \
        if ( faultInjectionError != FM_OK )                             \
        {                                                               \
            return faultInjectionError;                                 \
        }                                                               \
    }
#else
#define FM_FAULT_INJECTION_POINT()
#endif

/***************************************************************************/
/** UNPUBLISHED: Fault injection macro for log exit macros.
 *
 *  This macro is enabled when FM_ALOS_FAULT_INJECTION_ON_EXITS is set to
 *  FM_ENABLED.
 ***************************************************************************/
#if defined(FM_ALOS_FAULT_INJECTION_ON_EXITS) && (FM_ALOS_FAULT_INJECTION_ON_EXITS==FM_ENABLED)
#define FM_INJECT_FAULT_ON_EXIT()  FM_FAULT_INJECTION_POINT()
#else
#define FM_INJECT_FAULT_ON_EXIT()
#endif

/***************************************************************************/
/** A generic logging macro. May be used at any place in the code to
 *  generate a log message, but is typically called from the other
 *  logging macros.
 *                                                                      \lb\lb
 *  This macro (and thus all the others that depend on it) can be compiled out
 *  by undefining ''FM_ALOS_LOGGING_SUBSYSTEM''. This may result in improved
 *  CPU performance.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * level is a bit mask that specifies the log levels to which the message being
 * generated belongs (see ''Log Levels'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#ifndef FM_ALOS_LOGGING_SUBSYSTEM
#define FM_LOG_PRINTF(cat, level, ...)  { }
#else
#define FM_LOG_PRINTF(cat, level, ...)      \
    fmLogMessage( (cat), (level), __FILE__, \
                 __func__, __LINE__, __VA_ARGS__ )
#endif

/***************************************************************************/
/** A generic logging macro. May be used at any place in the code to
 *  generate a log message, but is typically called from the other
 *  logging macros.
 *                                                                      \lb\lb
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 *  This macro (and thus all the others that depend on it) can be compiled out
 *  by undefining ''FM_ALOS_LOGGING_SUBSYSTEM''. This may result in improved
 *  CPU performance.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * level is a bit mask that specifies the log levels to which the message being
 * generated belongs (see ''Log Levels'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#ifndef FM_ALOS_LOGGING_SUBSYSTEM
#define FM_LOG_PRINTF_V2(cat, level, objectId, ...)  { }
#else
#define FM_LOG_PRINTF_V2(cat, level, objectId, ...)      \
    fmLogMessageV2( (cat), (level), (objectId), __FILE__, \
                   __func__, __LINE__, __VA_ARGS__ )
#endif



/***************************************************************************/
/** A generic logging macro, similar to ''FM_LOG_PRINTF'', but independently
 *  enabled at compile time by ''FM_ALOS_FUNCTION_LOGGING'' and used for
 *  function entry and exit logging macros. This allows the high-volume
 *  function entry and exit logging macros to be compiled out, improving
 *  CPU performance, while not disabling the entire logging system.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * level is a bit mask that specifies the log levels to which the message being
 * generated belongs (see ''Log Levels'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#ifndef FM_ALOS_FUNCTION_LOGGING
#define FM_LOG_FUNC(cat, level, ...)  { }
#else
#define FM_LOG_FUNC(cat, level, ...)                \
   fmLogMessage((cat), (level), __FILE__,           \
                __func__, __LINE__, __VA_ARGS__ )
#endif

/***************************************************************************/
/** A generic logging macro, similar to ''FM_LOG_PRINTF'', but independently
 *  enabled at compile time by ''FM_ALOS_FUNCTION_LOGGING'' and used for
 *  function entry and exit logging macros. This allows the high-volume
 *  function entry and exit logging macros to be compiled out, improving
 *  CPU performance, while not disabling the entire logging system.
 *                                                                      \lb\lb
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * level is a bit mask that specifies the log levels to which the message being
 * generated belongs (see ''Log Levels'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#ifndef FM_ALOS_FUNCTION_LOGGING
#define FM_LOG_FUNC_V2(cat, level, objectId, ...)  { }
#else
#define FM_LOG_FUNC_V2(cat, level, objectId, ...)           \
   fmLogMessageV2((cat), (level), (objectId), __FILE__,     \
                  __func__, __LINE__, __VA_ARGS__ )
#endif



/***************************************************************************/
/** Log entry to a function. Generally invoked as the first executable line
 *  of code in a function.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_ENTRY(cat, ...) \
    FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_ENTRY, "Entering... " __VA_ARGS__ )

/***************************************************************************/
/** Log entry to a function. Generally invoked as the first executable line
 *  of code in a function. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_ENTRY_V2(cat, objectId, ...)           \
    FM_LOG_FUNC_V2( (cat),                            \
                     FM_LOG_LEVEL_FUNC_ENTRY,         \
                     (objectId),                      \
                     "Entering... " __VA_ARGS__ )



/***************************************************************************/
/** Log entry to a frequently-used function. Generally invoked as the first
 *  executable line of code in a function.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_VERBOSE) && (FM_ALOS_INCLUDE_LOG_VERBOSE==FM_DISABLED)
#define FM_LOG_ENTRY_VERBOSE(cat, ...)
#else
#define FM_LOG_ENTRY_VERBOSE(cat, ...)              \
    FM_LOG_FUNC((cat),                              \
                FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE,    \
                "Entering... " __VA_ARGS__ )
#endif

/***************************************************************************/
/** Log entry to a frequently-used function. Generally invoked as the first
 *  executable line of code in a function. The V2 version supports 
 *  objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_VERBOSE) && (FM_ALOS_INCLUDE_LOG_VERBOSE==FM_DISABLED)
#define FM_LOG_ENTRY_VERBOSE_V2(cat, (objectId)...)
#else
#define FM_LOG_ENTRY_VERBOSE_V2(cat, objectId, ...)    \
    FM_LOG_FUNC_V2( (cat),                             \
                   FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE,    \
                   (objectId),                         \
                   "Entering... " __VA_ARGS__ )
#endif



/***************************************************************************/
/** Log entry to a published function. Generally invoked as the first
 *  executable line of code in a function.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_ENTRY_API(cat, ...) \
    {                                                                                 \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_ENTRY_API, "Entering... " __VA_ARGS__ ); \
        FM_DBG_TRACK_FUNC();                                                          \
    }    

/***************************************************************************/
/** Log entry to a published function. Generally invoked as the first
 *  executable line of code in a function. The V2 version supports 
 *  objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_ENTRY_API_V2(cat, objectId, ...)        \
    {                                                  \
        FM_LOG_FUNC_V2( (cat),                         \
                         FM_LOG_LEVEL_FUNC_ENTRY_API,  \
                         (objectId),                   \
                         "Entering... " __VA_ARGS__ ); \
        FM_DBG_TRACK_FUNC();                           \
    }
        


/***************************************************************************/
/** Verbose Log entry to a published function. Generally invoked as the first
 *  executable line of code in a function.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_ENTRY_API_VERBOSE(cat, ...) \
    FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_ENTRY_API_VERBOSE, "Entering... " __VA_ARGS__ )

/***************************************************************************/
/** Verbose Log entry to a published function. Generally invoked as the 
 *  first executable line of code in a function. The V2 version supports
 *  objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_ENTRY_API_VERBOSE_V2(cat, objectId, ...)   \
    FM_LOG_FUNC_V2( (cat),                                \
                     FM_LOG_LEVEL_FUNC_ENTRY_API_VERBOSE, \
                     (objectId),                          \
                     "Entering... " __VA_ARGS__ )         \



/***************************************************************************/
/** Log entry to a function with no arguments.  Generally invoked as the
 *  first executable line of code in a function.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 ***************************************************************************/
#define FM_LOG_ENTRY_NOARGS(cat) \
    FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_ENTRY, "(no arguments)\n" )

/***************************************************************************/
/** Log entry to a function with no arguments.  Generally invoked as the
 *  first executable line of code in a function. The V2 version supports
 *  objectId-based filtering. 
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 ***************************************************************************/
#define FM_LOG_ENTRY_NOARGS_V2(cat, objectId)   \
    FM_LOG_FUNC_V2( (cat),                      \
                    (objectId),                 \
                     FM_LOG_LEVEL_FUNC_ENTRY,   \
                    "(no arguments)\n" )



/***************************************************************************/
/** Log entry to a function with no arguments.  Generally invoked as the
 *  first executable line of code in a function.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 ***************************************************************************/
#define FM_LOG_ENTRY_NOARGS_VERBOSE(cat) \
    FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE, "(no arguments)\n" )

/***************************************************************************/
/** Log entry to a function with no arguments.  Generally invoked as the
 *  first executable line of code in a function. The V2 version supports
 *  objectId-based filtering. 
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 ***************************************************************************/
#define FM_LOG_ENTRY_NOARGS_VERBOSE_V2(cat, objectId)   \
    FM_LOG_FUNC_V2( (cat),                              \
                    (objectId),                         \
                     FM_LOG_LEVEL_FUNC_ENTRY_VERBOSE,   \
                    "(no arguments)\n" )



/***************************************************************************/
/** Log exit from a function. Used wherever a function returns.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT(cat, errcode)                                             \
    {                                                                         \
        FM_INJECT_FAULT_ON_EXIT();                                            \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT, "Exit Status %d (%s)\n",  \
                      (errcode), fmErrorMsg( (errcode) ) );                   \
        return errcode;                                                       \
    }

/***************************************************************************/
/** Log exit from a function. Used wherever a function returns. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_V2(cat, objectId, errcode)                                \
   {                                                                          \
        FM_INJECT_FAULT_ON_EXIT();                                            \
        FM_LOG_FUNC_V2( (cat),                                                \
                      FM_LOG_LEVEL_FUNC_EXIT,                                 \
                     (objectId),                                              \
                     "Exit Status %d (%s)\n",                                 \
                      (errcode),                                              \
                     fmErrorMsg( (errcode) ) );                               \
        return errcode;                                                       \
   }



/***************************************************************************/
/** Log exit when the err code is != FM_OK.
 *                                                                      \lb\lb
 * This macro should be used to replace the typical pattern of checking
 * for an error return being non-zero.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes''). 
 *  
 ***************************************************************************/
#define FM_LOG_EXIT_ON_ERR(cat, errcode)                                       \
    {                                                                          \
        fm_status localError = (errcode);                                      \
        if ( (localError) != FM_OK )                                           \
        {                                                                      \
            FM_LOG_EXIT((cat), localError);                                    \
        }                                                                      \
    }

/***************************************************************************/
/** Log exit when the err code is != FM_OK. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used to replace the typical pattern of checking
 * for an error return being non-zero.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 ***************************************************************************/
#define FM_LOG_EXIT_ON_ERR_V2(cat, objectId, errcode)                         \
    {                                                                         \
        fm_status localError = (errcode);                                     \
        if ( (localError) != FM_OK )                                          \
        {                                                                     \
            FM_LOG_EXIT_V2((cat), (objectId), localError);                    \
        }                                                                     \
    }

/***************************************************************************/
/** Goto ABORT
 *                                                                      \lb\lb
 * This macro should be used to replace the typical pattern of going to a 
 * local ABORT label. This also logs a debug message to indicate why.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 ***************************************************************************/
#define FM_LOG_ABORT(cat, errcode)                                             \
    {                                                                          \
        FM_LOG_DEBUG( (cat), "Break to abort handler: %s\n",                   \
                      fmErrorMsg( (errcode) ) );                               \
        goto ABORT;                                                            \
    }


/***************************************************************************/
/** Goto ABORT when the err code is != FM_OK.
 *                                                                      \lb\lb
 * This macro should be used to replace the typical pattern of going to a 
 * local ABORT label when the error is not FM_OK.  This also logs a debug
 * message to indicate why.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 ***************************************************************************/
#define FM_LOG_ABORT_ON_ERR(cat, errcode)                                  \
    {                                                                      \
        fm_status localError = (errcode);                                  \
        if ( (localError) != FM_OK )                                       \
        {                                                                  \
            FM_LOG_ABORT((cat), localError);                               \
        }                                                                  \
    }

/***************************************************************************/
/** Goto ABORT when the err code is != FM_OK. 
 * The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used to replace the typical pattern of going to a 
 * local ABORT label when the error is not FM_OK.  This also logs a debug
 * message to indicate why.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 ***************************************************************************/
#define FM_LOG_ABORT_ON_ERR_V2(cat, objectId, errcode)                     \
    {                                                                      \
        fm_status localError = (errcode);                                  \
        if ( (localError) != FM_OK )                                       \
        {                                                                  \
            FM_LOG_DEBUG_V2((cat),                                         \
                            (objectId),                                    \
                            "Break to abort handler: %s\n",                \
                             fmErrorMsg((localError)));                    \
            goto ABORT;                                                    \
        }                                                                  \
    }



/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function returns.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_VERBOSE) && (FM_ALOS_INCLUDE_LOG_VERBOSE==FM_DISABLED)
#define FM_LOG_EXIT_VERBOSE(cat, errcode)
#else
#define FM_LOG_EXIT_VERBOSE(cat, errcode)                                    \
    {                                                                        \
        FM_INJECT_FAULT_ON_EXIT();                                           \
        FM_LOG_FUNC( (cat),                                                  \
                    FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                          \
                    "Exit Status %d (%s)\n",                                 \
                    (errcode),                                               \
                    fmErrorMsg( (errcode) ) );                               \
        return errcode;                                                      \
    }
#endif

/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function returns. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_VERBOSE) && (FM_ALOS_INCLUDE_LOG_VERBOSE==FM_DISABLED)
#define FM_LOG_EXIT_VERBOSE_V2(cat, objectId, errcode)
#else
#define FM_LOG_EXIT_VERBOSE_V2(cat, objectId, errcode)                       \
    {                                                                        \
        FM_INJECT_FAULT_ON_EXIT();                                           \
        FM_LOG_FUNC_V2( (cat),                                               \
                         FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                     \
                         (objectId),                                         \
                         "Exit Status %d (%s)\n",                            \
                         (errcode),                                          \
                         fmErrorMsg( (errcode) ) );                          \
        return errcode;                                                      \
    }
#endif



/***************************************************************************/
/** Verbose Log exit when the err code is != FM_OK
 *                                                                      \lb\lb
 * This macro should be used to replace the typical pattern of checking
 * for an error return being non-zero.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 ***************************************************************************/
#define FM_LOG_EXIT_ON_ERR_VERBOSE(cat, errcode)                               \
    {                                                                          \
        fm_status localError = (errcode);                                      \
        if ( (localError) != FM_OK )                                           \
        {                                                                      \
            FM_LOG_EXIT_VERBOSE((cat), localError);                            \
        }                                                                      \
    }



/***************************************************************************/
/** Log exit from a published function. Used wherever a function returns.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT_API''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_API(cat, errcode)                                              \
    {                                                                              \
        FM_INJECT_FAULT_ON_EXIT();                                                 \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT_API, "Exit Status %d (%s)\n",   \
                      (errcode), fmErrorMsg( (errcode) ) );                        \
        return errcode;                                                            \
    }

/***************************************************************************/
/** Log exit from a published function. Used wherever a function returns. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT_API''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_API_V2(cat, objectId, errcode)                           \
    {                                                                        \
        FM_INJECT_FAULT_ON_EXIT();                                           \
        FM_LOG_FUNC_V2( (cat),                                               \
                         FM_LOG_LEVEL_FUNC_EXIT_API,                         \
                        (objectId),                                          \
                        "Exit Status %d (%s)\n",                             \
                        (errcode),                                           \
                        fmErrorMsg( (errcode) ) );                           \
        return errcode;                                                      \
    }



/***************************************************************************/
/** Log exit from a frequently-called published function. Used wherever a
 *  function returns.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT_API''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_API_VERBOSE(cat, errcode)                       \
    {                                                               \
        FM_INJECT_FAULT_ON_EXIT();                                  \
        FM_LOG_FUNC( (cat),                                         \
                    FM_LOG_LEVEL_FUNC_EXIT_API_VERBOSE,             \
                    "Exit Status %d (%s)\n",                        \
                    (errcode),                                      \
                    fmErrorMsg( (errcode) ) );                      \
        return errcode;                                             \
    }

/***************************************************************************/
/** Log exit from a frequently-called published function. Used wherever a
 *  function returns. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used wherever a "return fm_status" statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT_API''
 * log level is enabled, this macro is particularly useful for determining
 * the root cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * errcode is the value being returned by the function (typically of type
 * fm_status - see ''Status Codes'').
 *
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_API_VERBOSE_V2(cat, objectId, errcode)          \
    {                                                               \
        FM_INJECT_FAULT_ON_EXIT();                                  \
        FM_LOG_FUNC_V2( (cat),                                      \
                         FM_LOG_LEVEL_FUNC_EXIT_API_VERBOSE,        \
                         (objectId),                                \
                         "Exit Status %d (%s)\n",                   \
                         (errcode),                                 \
                         fmErrorMsg( (errcode) ) );                 \
        return errcode;                                             \
    }



/***************************************************************************/
/** Log exit from a published function. Used wherever a function returns
 *  something other than an exit code.
 *                                                                      \lb\lb
 * This macro should be used where any other type of return statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT_API'' log level is
 * enabled, this macro is particularly useful for determining the root
 * cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * retval is the custom return value.  If it should be displayed in the
 * formatted printf string, it should also be included in the varargs
 * section of the macro call.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_API_CUSTOM(cat, retval, ...)                                       \
    {                                                                                  \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT_API, "Exiting... " __VA_ARGS__ );   \
        return retval;                                                                 \
    }

/***************************************************************************/
/** Log exit from a published function. Used wherever a function returns
 *  something other than an exit code.
 *                                                                      \lb\lb
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where any other type of return statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT_API'' log level is
 * enabled, this macro is particularly useful for determining the root
 * cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * retval is the custom return value.  If it should be displayed in the
 * formatted printf string, it should also be included in the varargs
 * section of the macro call.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_API_CUSTOM_V2(cat, objectId, retval, ...)   \
    {                                                           \
        FM_LOG_FUNC_V2( (cat),                                  \
                         FM_LOG_LEVEL_FUNC_EXIT_API,            \
                         (objectId),                            \
                         "Exiting... " __VA_ARGS__ );           \
        return retval;                                          \
    }



/***************************************************************************/
/** Log exit from a function. Used wherever a function exits but has no
 *  return type.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 ***************************************************************************/
#define FM_LOG_EXIT_VOID(cat)                                        \
    {                                                                \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT, "Exiting\n" );   \
        return;                                                      \
    }

/***************************************************************************/
/** Log exit from a function. Used wherever a function exits but has no
 *  return type. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_V2(cat, objectId)                          \
    {                                                               \
        FM_LOG_FUNC_V2( (cat),                                      \
                         FM_LOG_LEVEL_FUNC_EXIT,                    \
                         (objectId),                                \
                         "Exiting\n" );                             \
        return;                                                     \
    }



/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function exits but has
 *  no return type.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_VERBOSE(cat)                                       \
    {                                                                       \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT_VERBOSE, "Exiting\n" );  \
        return;                                                             \
    }

/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function exits but has
 *  no return type. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_VERBOSE_V2(cat, objectId)                         \
    {                                                                      \
        FM_LOG_FUNC_V2( (cat),                                             \
                          FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                  \
                         (objectId),                                       \
                         "Exiting\n" );                                    \
        return;                                                            \
    }
                                                                          


/***************************************************************************/
/** Log exit from a function. Used wherever a function exits but has no
 *  return type.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_VA(cat, ...)                                            \
    {                                                                            \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT, "Exiting... " __VA_ARGS__ ); \
        return;                                                                  \
    }

/***************************************************************************/
/** Log exit from a function. Used wherever a function exits but has no
 *  return type. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_VA_V2(cat, objectId, ...)                          \
    {                                                                       \
        FM_LOG_FUNC_V2( (cat),                                              \
                         FM_LOG_LEVEL_FUNC_EXIT,                            \
                        (objectId),                                         \
                        "Exiting... " __VA_ARGS__ );                        \
        return;                                                             \
    }



/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function exits but has 
 *  no return type.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_VA_VERBOSE(cat, ...)                               \
    {                                                                       \
        FM_LOG_FUNC( (cat),                                                 \
                    FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                         \
                    "Exiting\n" __VA_ARGS__ );                              \
        return;                                                             \
    }

/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function exits but has 
 *  no return type. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where a void return statement would normally
 * appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is enabled, this macro
 * is particularly useful for determining the root cause of an error that
 * is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_VOID_VA_VERBOSE_V2(cat, objectId, ...)                  \
    {                                                                       \
        FM_LOG_FUNC_V2( (cat),                                              \
                         FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                    \
                        (objectId),                                         \
                         "Exiting\n" __VA_ARGS__ );                         \
        return;                                                             \
    }



/***************************************************************************/
/** Log exit from a function. Used wherever a function returns something
 *  other than an exit code.
 *                                                                      \lb\lb
 * This macro should be used where any other type of return statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is
 * enabled, this macro is particularly useful for determining the root
 * cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * retval is the custom return value.  If it should be displayed in the
 * formatted printf string, it should also be included in the varargs
 * section of the macro call.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_CUSTOM(cat, retval, ...)                                       \
    {                                                                              \
        FM_LOG_FUNC( (cat), FM_LOG_LEVEL_FUNC_EXIT, "Exiting... " __VA_ARGS__ );   \
        return retval;                                                             \
    }

/***************************************************************************/
/** Log exit from a function. Used wherever a function returns something
 *  other than an exit code.
 *                                                                      \lb\lb
 * The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where any other type of return statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is
 * enabled, this macro is particularly useful for determining the root
 * cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * retval is the custom return value.  If it should be displayed in the
 * formatted printf string, it should also be included in the varargs
 * section of the macro call.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_CUSTOM_V2(cat, objectId, retval, ...)         \
    {                                                             \
        FM_LOG_FUNC_V2( (cat),                                    \
                         FM_LOG_LEVEL_FUNC_EXIT,                  \
                         (objectId),                              \
                         "Exiting... " __VA_ARGS__ );             \
        return retval;                                            \
    }



/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function returns
 *  something other than an exit code.
 *                                                                      \lb\lb
 * This macro should be used where any other type of return statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is
 * enabled, this macro is particularly useful for determining the root
 * cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * retval is the custom return value.  If it should be displayed in the
 * formatted printf string, it should also be included in the varargs
 * section of the macro call.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_CUSTOM_VERBOSE(cat, retval, ...)                        \
    {                                                                       \
        FM_LOG_FUNC( (cat),                                                 \
                    FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                         \
                    "Exiting... " __VA_ARGS__ );                            \
        return retval;                                                      \
    }

/***************************************************************************/
/** Verbose Log exit from a function. Used wherever a function returns
 *  something other than an exit code.
 *                                                                      \lb\lb
 * The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * This macro should be used where any other type of return statement
 * would normally appear. When the ''FM_LOG_LEVEL_FUNC_EXIT'' log level is
 * enabled, this macro is particularly useful for determining the root
 * cause of an error that is propagated through the API.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * retval is the custom return value.  If it should be displayed in the
 * formatted printf string, it should also be included in the varargs
 * section of the macro call.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_EXIT_CUSTOM_VERBOSE_V2(cat, objectId, retval, ...)       \
    {                                                                   \
        FM_LOG_FUNC_V2( (cat),                                          \
                         FM_LOG_LEVEL_FUNC_EXIT_VERBOSE,                \
                        (objectId),                                     \
                         "Exiting... " __VA_ARGS__ );                   \
        return retval;                                                  \
    }



/***************************************************************************/
/** Log a warning message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_WARNING'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_WARNING) && FM_ALOS_INCLUDE_LOG_WARNING == FM_DISABLED
 #define FM_LOG_WARNING(cat, ...) 
#else
 #define FM_LOG_WARNING(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_WARNING, __VA_ARGS__ )
#endif

/***************************************************************************/
/** Log a warning message. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_WARNING'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_WARNING) && FM_ALOS_INCLUDE_LOG_WARNING == FM_DISABLED
 #define FM_LOG_WARNING_V2(cat, objectId, ...) 
#else
 #define FM_LOG_WARNING_V2(cat, objectId, ...) \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_WARNING, (objectId), __VA_ARGS__ )
#endif



/***************************************************************************/
/** Log an error message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_ERROR'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_ERROR) && FM_ALOS_INCLUDE_LOG_ERROR == FM_DISABLED
 #define FM_LOG_ERROR(cat, ...) 
#else
 #define FM_LOG_ERROR(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_ERROR, __VA_ARGS__ ) 
#endif

/***************************************************************************/
/** Log an error message. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_ERROR'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_ERROR) && FM_ALOS_INCLUDE_LOG_ERROR == FM_DISABLED
 #define FM_LOG_ERROR_V2(cat, objectId, ...) 
#else
 #define FM_LOG_ERROR_V2(cat, objectId, ...) \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_ERROR, (objectId), __VA_ARGS__ ) 
#endif



/***************************************************************************/
/** Log a fatal error message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_FATAL'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_FATAL) && FM_ALOS_INCLUDE_LOG_FATAL == FM_DISABLED
 #define FM_LOG_FATAL(cat, ...) { } 
#else
 #define FM_LOG_FATAL(cat, ...)  \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_FATAL, __VA_ARGS__ ) 
#endif

/***************************************************************************/
/** Log a fatal error message. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_FATAL'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_FATAL) && FM_ALOS_INCLUDE_LOG_FATAL == FM_DISABLED
 #define FM_LOG_FATAL_V2(cat, objectId, ...) { } 
#else
 #define FM_LOG_FATAL_V2(cat, objectId, ...)  \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_FATAL, (objectId), __VA_ARGS__ ) 
#endif



/***************************************************************************/
/** Log an informational message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_INFO'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_INFO) && FM_ALOS_INCLUDE_LOG_INFO == FM_DISABLED
 #define FM_LOG_INFO(cat, ...)
#else
 #define FM_LOG_INFO(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_INFO, __VA_ARGS__ ) 
#endif

/***************************************************************************/
/** Log an informational message. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_INFO'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_INFO) && FM_ALOS_INCLUDE_LOG_INFO == FM_DISABLED
 #define FM_LOG_INFO_V2(cat, objectId, ...)
#else
 #define FM_LOG_INFO_V2(cat, objectId, ...) \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_INFO, (objectId), __VA_ARGS__ ) 
#endif



/***************************************************************************/
/** Log a debug message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG(cat, ...)
#else
 #define FM_LOG_DEBUG(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_DEBUG, __VA_ARGS__ )
#endif

/***************************************************************************/
/** Log a debug message. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG_V2(cat, ...)
#else
 #define FM_LOG_DEBUG_V2(cat, objectId,...) \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_DEBUG, (objectId),__VA_ARGS__ )
#endif



/***************************************************************************/
/** Log a verbose debug message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG_VERBOSE(cat, ...)
#else
 #define FM_LOG_DEBUG_VERBOSE(cat, ...)  \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_DEBUG_VERBOSE, __VA_ARGS__ ) 
#endif

/***************************************************************************/
/** Log a verbose debug message. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG_VERBOSE_V2(cat, objectId, ...)
#else
 #define FM_LOG_DEBUG_VERBOSE_V2(cat, objectId, ...)  \
    FM_LOG_PRINTF_V2( (cat),                          \
                       FM_LOG_LEVEL_DEBUG_VERBOSE,    \
                      (objectId),                     \
                       __VA_ARGS__ )                  
#endif



/***************************************************************************/
/** Log a debug level 2 message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG2(cat, ...)
#else
 #define FM_LOG_DEBUG2(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_DEBUG2, __VA_ARGS__ )
#endif

/***************************************************************************/
/** Log a debug level 2 message. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG2_V2(cat, ...)
#else
 #define FM_LOG_DEBUG2_V2(cat, objectId,...) \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_DEBUG2, (objectId),__VA_ARGS__ )
#endif



/***************************************************************************/
/** Log a debug level 3 message.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG3(cat, ...)
#else
 #define FM_LOG_DEBUG3(cat, ...) \
    FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_DEBUG3, __VA_ARGS__ )
#endif

/***************************************************************************/
/** Log a debug level 3 message. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_DEBUG3_V2(cat, ...)
#else
 #define FM_LOG_DEBUG3_V2(cat, objectId,...) \
    FM_LOG_PRINTF_V2( (cat), FM_LOG_LEVEL_DEBUG3, (objectId),__VA_ARGS__ )
#endif



/***************************************************************************/
/** Log an assertion failure and continue execution.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * cond is a condition that must be FALSE if the log message is to be
 * generated.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_ASSERT'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_ASSERT) && FM_ALOS_INCLUDE_LOG_ASSERT == FM_DISABLED
 #define FM_LOG_ASSERT(cat, cond, ...)
#else
 #define FM_LOG_ASSERT(cat, cond, ...) \
    if (!(cond)) \
    { \
        FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_ASSERT, __VA_ARGS__ );  \
    }
#endif

/***************************************************************************/
/** Log an assertion failure and continue execution. 
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * cond is a condition that must be FALSE if the log message is to be
 * generated.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_ASSERT'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_ASSERT) && FM_ALOS_INCLUDE_LOG_ASSERT == FM_DISABLED
 #define FM_LOG_ASSERT_V2(cat, objectId, cond, ...)
#else
 #define FM_LOG_ASSERT_V2(cat, objectId, cond, ...) \
    if (!(cond))                                    \
    {                                               \
        FM_LOG_PRINTF_V2( (cat),                    \
                          FM_LOG_LEVEL_ASSERT,      \
                          (objectId),               \
                           __VA_ARGS__ );           \
    }
#endif



/***************************************************************************/
/** Log an assertion failure and goto ABORT.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * cond is a condition that must be FALSE if the log message is to be
 * generated.
 *                                                                      \lb\lb
 * action is a C statement to be executed when the assertion fails. Typically
 * this will be to set a return code.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_ASSERT'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_ASSERT) && FM_ALOS_INCLUDE_LOG_ASSERT == FM_DISABLED
 #define FM_LOG_ABORT_ON_ASSERT(cat, cond, action, ...) \
    if (!(cond)) \
    { \
        (action); \
        goto ABORT; \
    }
#else
 #define FM_LOG_ABORT_ON_ASSERT(cat, cond, action, ...) \
    if (!(cond)) \
    { \
        FM_LOG_PRINTF( (cat), FM_LOG_LEVEL_ASSERT, __VA_ARGS__ ); \
        (action); \
        goto ABORT; \
    }
#endif

/***************************************************************************/
/** Log an assertion failure and goto ABORT.
 *  The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * cond is a condition that must be FALSE if the log message is to be
 * generated.
 *                                                                      \lb\lb
 * action is a C statement to be executed when the assertion fails. Typically
 * this will be to set a return code.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_ASSERT'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_ASSERT) && FM_ALOS_INCLUDE_LOG_ASSERT == FM_DISABLED
 #define FM_LOG_ABORT_ON_ASSERT_V2(cat, objectId, cond, action, ...) \
    if (!(cond)) \
    { \
        (action); \
        goto ABORT; \
    }
#else
 #define FM_LOG_ABORT_ON_ASSERT_V2(cat, objectId, cond, action, ...) \
    if (!(cond))                                     \
    {                                                \
        FM_LOG_PRINTF_V2( (cat),                     \
                           FM_LOG_LEVEL_ASSERT,      \
                           (objectId),               \
                            __VA_ARGS__ );           \
        (action);                                    \
        goto ABORT;                                  \
    }
#endif



/***************************************************************************/
/** Log the call stack.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * level is a bit mask that specifies the log levels to which the message being
 * generated belongs (see ''Log Levels'').
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_CALL_STACK(cat, level)
#else
 #define FM_LOG_CALL_STACK(cat, level)                                      \
    {                                                                       \
        char callStack[FM_MAX_CALL_STACK_BFR_SIZE];                         \
        fmGetCallerName(callStack,                                          \
                        FM_MAX_CALL_STACK_BFR_SIZE,                         \
                        FM_MAX_CALL_STACK_SIZE,                             \
                        "\n");                                              \
        FM_LOG_PRINTF( (cat),                                               \
                        (level),                                            \
                        "\nCall stack:\n%s\n(end stack)\n",                 \
                        callStack);                                         \
    }

#endif

/***************************************************************************/
/** Log the call stack. The V2 version supports objectId-based filtering.
 *                                                                      \lb\lb
 * cat is a bit mask that specifies the categories to which the message being
 * generated belongs (see ''Log Categories'').
 *                                                                      \lb\lb
 * level is a bit mask that specifies the log levels to which the message being
 * generated belongs (see ''Log Levels'').
 *                                                                      \lb\lb
 * objectId is the value of the object ID to be used for filtering.
 *                                                                      \lb\lb
 * The platform may disable this logging macro at compile time by setting
 * ''FM_ALOS_INCLUDE_LOG_DEBUG'' to FM_DISABLED in the platform_defines.h
 * header file.
 ***************************************************************************/
#if defined(FM_ALOS_INCLUDE_LOG_DEBUG) && FM_ALOS_INCLUDE_LOG_DEBUG == FM_DISABLED
 #define FM_LOG_CALL_STACK_V2(cat, level, objectId)
#else
 #define FM_LOG_CALL_STACK_V2(cat, level, objectId)                         \
    {                                                                       \
        char callStack[FM_MAX_CALL_STACK_BFR_SIZE];                         \
        fmGetCallerName(callStack,                                          \
                        FM_MAX_CALL_STACK_BFR_SIZE,                         \
                        FM_MAX_CALL_STACK_SIZE,                             \
                        "\n");                                              \
        FM_LOG_PRINTF_V2( (cat),                                            \
                          (level),                                          \
                          (objectId),                                       \
                          "\nCall stack:\n%s\n(end stack)\n",               \
                          callStack);                                       \
    }

#endif



/***************************************************************************/
/** Unconditionally log a printed message. This macro should be used by
 *  functions whose sole purpose is to print out information. Typically
 *  such functions are called on-demand only.
 *                                                                      \lb\lb
 * ... is any sequence of arguments that might be used in a call to printf,
 * such as a format string and related arguments.
 ***************************************************************************/
#define FM_LOG_PRINT(...) \
    FM_LOG_PRINTF(0, FM_LOG_LEVEL_PRINT, __VA_ARGS__)

/** @} (end of Doxygen group) */


#define FM_BOOLSTRING(b)  ( (b) ? "true" : "false" )

#endif /* __FM_FM_ALOS_LOGGING_H */
