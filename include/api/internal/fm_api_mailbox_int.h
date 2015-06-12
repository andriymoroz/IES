/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mailbox_int.h
 * Creation Date:   2014
 * Description:     Structures and functions for dealing with mailbox.
 *
 * Copyright (c) 2014, Intel Corporation
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

#ifndef __FM_FM_API_MAILBOX_INT_H
#define __FM_FM_API_MAILBOX_INT_H

/* All defined low/high bit valuess are within 0..31 range as MBMEM register
 * entries are 32 bit long. */

/* Control header structure to manage request/response queues:
 *
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-------+-----------------------+-------+-----------------------+
 * |  Err  |      Request Head     |  Ver  |    Response Tail      |
 * +-------+-----------------------+-------+-----------------------+
 * |                                                               |
 * .                                                               .
 * .                                                               .
 * +-------+-----------------------+-------+-----------------------+
 * |  Err  |     Response Head     |  Ver  |     Request Tail      |
 * +-------+-----------------------+-------+-----------------------+
 * |                                                               |
 * .                                                               .
 * .                                                               .
 * +-------+-----------------------+-------+-----------------------+
 */


/* Mailbox message scheme for both request and response queues:
 *
 * +--------------------+
 * | Transaction header |
 * +--------------------+
 * | Argument header    |
 * +--------------------+
 * .                    .
 * . DATA               .
 * .                    .
 * +--------------------+
 */


/* Transaction/Argument header structure:
 * 
 *    3                   2                   1                   0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-----------------------+-------+-------------------------------+
 * |        Length         | Flags |           Type / ID           |
 * +-----------------------+-------+-------------------------------+
 */

/* Calculate PF glort value using min value from glort range.
   This assumes that first 128 glorts are used for VFs and VMDq. */
#define CALCULATE_PF_GLORT_VALUE(min_glort_value) \
    (min_glort_value + 128)
 

#define GET_MAILBOX_INFO(sw) \
    &((fm_switch *)(GET_SWITCH_PTR( sw )))->mailboxInfo;

/* Get the switch aggregate id if exists, otherwise just return argument value. */
#define GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw)\
    ( ((fm_switch *)(GET_SWITCH_PTR( sw )))->swag < 0 ) ? \
    (sw) : ( ((fm_switch *)(GET_SWITCH_PTR( sw )))->swag )

/* Bit defines for configurable fields */
#define FM_MAILBOX_BITS_l_0_15                                     0
#define FM_MAILBOX_BITS_h_0_15                                     15
#define FM_MAILBOX_BITS_l_16_31                                    16
#define FM_MAILBOX_BITS_h_16_31                                    31
#define FM_MAILBOX_BITS_l_0_31                                     0
#define FM_MAILBOX_BITS_h_0_31                                     31

/* Mailbox control header field bit definitions.  */
#define FM_MAILBOX_SM_CONTROL_HEADER_l_VERSION                     12
#define FM_MAILBOX_SM_CONTROL_HEADER_h_VERSION                     15
#define FM_MAILBOX_SM_CONTROL_HEADER_l_ERROR_FLAGS                 28
#define FM_MAILBOX_SM_CONTROL_HEADER_h_ERROR_FLAGS                 31
#define FM_MAILBOX_SM_CONTROL_HEADER_l_REQUEST_QUEUE_HEAD          16
#define FM_MAILBOX_SM_CONTROL_HEADER_h_REQUEST_QUEUE_HEAD          27
#define FM_MAILBOX_SM_CONTROL_HEADER_l_RESPONSE_QUEUE_TAIL         0
#define FM_MAILBOX_SM_CONTROL_HEADER_h_RESPONSE_QUEUE_TAIL         11

#define FM_MAILBOX_PF_CONTROL_HEADER_l_VERSION                 12
#define FM_MAILBOX_PF_CONTROL_HEADER_h_VERSION                 15
#define FM_MAILBOX_PF_CONTROL_HEADER_l_ERROR_FLAGS             28
#define FM_MAILBOX_PF_CONTROL_HEADER_h_ERROR_FLAGS             31
#define FM_MAILBOX_PF_CONTROL_HEADER_l_RESPONSE_QUEUE_HEAD     16
#define FM_MAILBOX_PF_CONTROL_HEADER_h_RESPONSE_QUEUE_HEAD     27
#define FM_MAILBOX_PF_CONTROL_HEADER_l_REQUEST_QUEUE_TAIL      0
#define FM_MAILBOX_PF_CONTROL_HEADER_h_REQUEST_QUEUE_TAIL      11

/* Mailbox message (transaction/argument) header field bit definitions. */
#define FM_MAILBOX_MESSAGE_HEADER_l_MESSAGE_TYPE                  0
#define FM_MAILBOX_MESSAGE_HEADER_h_MESSAGE_TYPE                  15
#define FM_MAILBOX_MESSAGE_HEADER_l_MESSAGE_FLAGS                 16
#define FM_MAILBOX_MESSAGE_HEADER_h_MESSAGE_FLAGS                 19
#define FM_MAILBOX_MESSAGE_HEADER_l_MESSAGE_LENGTH                20
#define FM_MAILBOX_MESSAGE_HEADER_h_MESSAGE_LENGTH                31

/* Mailbox message argument field definitions. 
 * Values are defined for each argument structure.
 */

/* fm_hostSrvErr */
#define FM_MAILBOX_SRV_ERR_l_STATUS_CODE                          0
#define FM_MAILBOX_SRV_ERR_h_STATUS_CODE                          31
#define FM_MAILBOX_SRV_ERR_l_MAC_TABLE_ROWS_USED                  0
#define FM_MAILBOX_SRV_ERR_h_MAC_TABLE_ROWS_USED                  31
#define FM_MAILBOX_SRV_ERR_l_MAC_TABLE_ROWS_AVAILABLE             0
#define FM_MAILBOX_SRV_ERR_h_MAC_TABLE_ROWS_AVAILABLE             31
#define FM_MAILBOX_SRV_ERR_l_NEXTHOP_TABLE_ROWS_USED              0
#define FM_MAILBOX_SRV_ERR_h_NEXTHOP_TABLE_ROWS_USED              31
#define FM_MAILBOX_SRV_ERR_l_NEXTHOP_TABLE_ROWS_AVAILABLE         0
#define FM_MAILBOX_SRV_ERR_h_NEXTHOP_TABLE_ROWS_AVAILABLE         31
#define FM_MAILBOX_SRV_ERR_l_FFU_RULES_USED                       0
#define FM_MAILBOX_SRV_ERR_h_FFU_RULES_USED                       31
#define FM_MAILBOX_SRV_ERR_l_FFU_RULES_AVAILABLE                  0
#define FM_MAILBOX_SRV_ERR_h_FFU_RULES_AVAILABLE                  31

/* fm_hostSrvLportMap */
#define FM_MAILBOX_SRV_LPORT_MAP_l_GLORT_VALUE                    0
#define FM_MAILBOX_SRV_LPORT_MAP_h_GLORT_VALUE                    15
#define FM_MAILBOX_SRV_LPORT_MAP_l_GLORT_MASK                     16
#define FM_MAILBOX_SRV_LPORT_MAP_h_GLORT_MASK                     31
#define FM_MAILBOX_SRV_LPORT_MAP_l_FIRST_LOGICAL_PORT             0
#define FM_MAILBOX_SRV_LPORT_MAP_h_FIRST_LOGICAL_PORT             15
#define FM_MAILBOX_SRV_LPORT_MAP_l_NUMBER_OF_LOGICAL_PORTS        16
#define FM_MAILBOX_SRV_LPORT_MAP_h_NUMBER_OF_LOGICAL_PORTS        31

/* fm_hostSrvPort */
#define FM_MAILBOX_SRV_PORT_l_FIRST_GLORT                         0
#define FM_MAILBOX_SRV_PORT_h_FIRST_GLORT                         15
#define FM_MAILBOX_SRV_PORT_l_GLORT_COUNT                         16
#define FM_MAILBOX_SRV_PORT_h_GLORT_COUNT                         31

/* fm_hostSrvUpdatePvid */
#define FM_MAILBOX_SRV_UPDATE_PVID_l_GLORT_VALUE         0
#define FM_MAILBOX_SRV_UPDATE_PVID_h_GLORT_VALUE         15
#define FM_MAILBOX_SRV_UPDATE_PVID_l_PVID_VALUE          16
#define FM_MAILBOX_SRV_UPDATE_PVID_h_PVID_VALUE          31

/* fm_hostSrvXcastMode */
#define FM_MAILBOX_SRV_XCAST_MODE_l_GLORT                         0
#define FM_MAILBOX_SRV_XCAST_MODE_h_GLORT                         15
#define FM_MAILBOX_SRV_XCAST_MODE_l_MODE                          16
#define FM_MAILBOX_SRV_XCAST_MODE_h_MODE                          31

/* fm_hostSrvMACUpdate */
#define FM_MAILBOX_SRV_MAC_UPDATE_l_MAC_ADDRESS_LOWER             0
#define FM_MAILBOX_SRV_MAC_UPDATE_h_MAC_ADDRESS_LOWER             31
#define FM_MAILBOX_SRV_MAC_UPDATE_l_MAC_ADDRESS_UPPER             0
#define FM_MAILBOX_SRV_MAC_UPDATE_h_MAC_ADDRESS_UPPER             15
#define FM_MAILBOX_SRV_MAC_UPDATE_l_VLAN                          16
#define FM_MAILBOX_SRV_MAC_UPDATE_h_VLAN                          31
#define FM_MAILBOX_SRV_MAC_UPDATE_l_GLORT                         0
#define FM_MAILBOX_SRV_MAC_UPDATE_h_GLORT                         15
#define FM_MAILBOX_SRV_MAC_UPDATE_l_FLAGS                         16
#define FM_MAILBOX_SRV_MAC_UPDATE_h_FLAGS                         23
#define FM_MAILBOX_SRV_MAC_UPDATE_l_ACTION                        24
#define FM_MAILBOX_SRV_MAC_UPDATE_h_ACTION                        31

/* fm_hostSrvConfig */
#define FM_MAILBOX_SRV_CONFIG_l_CONFIGURATION_ATTRIBUTE_CONSTANT  0 
#define FM_MAILBOX_SRV_CONFIG_h_CONFIGURATION_ATTRIBUTE_CONSTANT  15
#define FM_MAILBOX_SRV_CONFIG_l_VALUE                             16
#define FM_MAILBOX_SRV_CONFIG_h_VALUE                             31

/* fm_hostSrvCreateFlowTable */
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_l_TABLE_TYPE             0
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_h_TABLE_TYPE             3
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_l_RESERVED               4
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_h_RESERVED               31
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_l_TABLE_INDEX            0
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_h_TABLE_INDEX            31
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_l_FLOW_COND_BITMASK_LOW  0
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_h_FLOW_COND_BITMASK_LOW  31
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_l_FLOW_COND_BITMASK_UP   0
#define FM_MAILBOX_SRV_CREATE_FLOW_TABLE_h_FLOW_COND_BITMASK_UP   31

/* fm_hostSrvDeleteFlowTable */
#define FM_MAILBOX_SRV_DELETE_FLOW_TABLE_l_TABLE_INDEX            0
#define FM_MAILBOX_SRV_DELETE_FLOW_TABLE_h_TABLE_INDEX            23
#define FM_MAILBOX_SRV_DELETE_FLOW_TABLE_l_TABLE_TYPE             24
#define FM_MAILBOX_SRV_DELETE_FLOW_TABLE_h_TABLE_TYPE             31

/* fm_hostSrvUpdateFlow */
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_PRIORITY                     0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_PRIORITY                     15
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_RESERVED                     16
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_RESERVED                     31
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_TABLE_INDEX                  0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_TABLE_INDEX                  31
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_FLOW_ID                      0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_FLOW_ID                      31
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_FLOW_COND_BITMASK_LOW        0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_FLOW_COND_BITMASK_LOW        31
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_FLOW_COND_BITMASK_UP         0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_FLOW_COND_BITMASK_UP         31
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_FLOW_ACTION_BITMASK_LOW      0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_FLOW_ACTION_BITMASK_LOW      31
#define FM_MAILBOX_SRV_UPDATE_FLOW_l_FLOW_ACTION_BITMASK_UP       0
#define FM_MAILBOX_SRV_UPDATE_FLOW_h_FLOW_ACTION_BITMASK_UP       31

/* fm_hostSrvFlowState */
#define FM_MAILBOX_SRV_FLOW_STATE_l_TABLE_INDEX                   0
#define FM_MAILBOX_SRV_FLOW_STATE_h_TABLE_INDEX                   31
#define FM_MAILBOX_SRV_FLOW_STATE_l_FLOW_ID                       0
#define FM_MAILBOX_SRV_FLOW_STATE_h_FLOW_ID                       31
#define FM_MAILBOX_SRV_FLOW_STATE_l_FLOW_STATE                    0
#define FM_MAILBOX_SRV_FLOW_STATE_h_FLOW_STATE                    3
#define FM_MAILBOX_SRV_FLOW_STATE_l_RESERVED                      4
#define FM_MAILBOX_SRV_FLOW_STATE_h_RESERVED                      31

/* fm_hostSrvFlowHandle */
#define FM_MAILBOX_SRV_FLOW_HANDLE_l_FLOW_ID                      0
#define FM_MAILBOX_SRV_FLOW_HANDLE_h_FLOW_ID                      31

/* fm_hostSrvDeleteFlow */
#define FM_MAILBOX_SRV_DELETE_FLOW_l_TABLE_INDEX                   0
#define FM_MAILBOX_SRV_DELETE_FLOW_h_TABLE_INDEX                   31
#define FM_MAILBOX_SRV_DELETE_FLOW_l_FLOW_ID                       0
#define FM_MAILBOX_SRV_DELETE_FLOW_h_FLOW_ID                       31

/* fm_hostSrv1588 */
#define FM_MAILBOX_SRV_1588_l_TMP                                  0
#define FM_MAILBOX_SRV_1588_h_TMP                                  31

/* fm_hostSrvPacketTimestamp */
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_l_DGLORT                   0
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_h_DGLORT                   15
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_l_SGLORT                   16
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_h_SGLORT                   31
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_l_EGR_TIME_LOW             0
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_h_EGR_TIME_LOW             31
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_l_EGR_TIME_UP              0
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_h_EGR_TIME_UP              31
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_l_INGR_TIME_LOW            0
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_h_INGR_TIME_LOW            31
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_l_INGR_TIME_UP             0
#define FM_MAILBOX_SRV_PACKET_TIMESTAMP_h_INGR_TIME_UP             31

/* fm_hostSrvTimestampModeReq */
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_REQ_l_GLORT                  0
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_REQ_h_GLORT                  15
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_REQ_l_MODE                   16
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_REQ_h_MODE                   31

/* fm_hostSrvTimestampModeResp */
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_l_GLORT                 0
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_h_GLORT                 15
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_l_MAX_MODE              16
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_h_MAX_MODE              31
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_l_STATUS                0
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_h_STATUS                31

/* Mailbox control header versions used in control header. */
#define FM_MAILBOX_VERSION_RESET                                   0
#define FM_MAILBOX_VERSION_DEFAULT                                 1


/* SM control header update type categories. 
 * Used for updating particular fields in control header. */
#define FM_UPDATE_CTRL_HDR_RESPONSE_TAIL                           (1 << 0)
#define FM_UPDATE_CTRL_HDR_REQUEST_HEAD                            (1 << 1)
#define FM_UPDATE_CTRL_HDR_VERSION                                 (1 << 2)
#define FM_UPDATE_CTRL_HDR_ERROR                                   (1 << 3)

#define FM_UPDATE_CTRL_HDR_ALL  (FM_UPDATE_CTRL_HDR_RESPONSE_TAIL | \
                                 FM_UPDATE_CTRL_HDR_REQUEST_HEAD |  \
                                 FM_UPDATE_CTRL_HDR_VERSION | \
                                 FM_UPDATE_CTRL_HDR_ERROR )

/* Start of message is bit 0 from transaction header flags area */
#define FM_MAILBOX_MESSAGE_HEADER_b_START_OF_MESSAGE           0

/* Mailbox control header error types used in control header. */
#define FM_MAILBOX_ERR_TYPE_NONE                0
#define FM_MAILBOX_ERR_TYPE_INVALID_VERSION     1

/* Add/delete flag is bit 0 from fm_hostSrvMACUpdate action area. */
#define FM_HOST_SRV_MAC_UPDATE_b_ADD_DELETE                    0

/* MAC secure flag is bit 0 from fm10000_hostSrvMACUpdate flag area. */
#define FM_HOST_SRV_MAC_UPDATE_b_MAC_SECURE                    0

/* Flow ID indticating that new flow will be added via HNI Services. */
#define FM_MAILBOX_FLOW_ID_ADD_NEW                             0xFFFF

/* For adding virtual ports as a listeners to flooding multicast
 * groups we will use the vlan = 0. */
#define FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS             0

/* Max number of actions supported by rules in flow table */
#define FM_MAILBOX_FLOW_TABLE_MAX_ACTION_SUPPORTED             1

/* Length of mailbox message entry in bits */
#define FM_MAILBOX_QUEUE_ENTRY_BIT_LENGTH  32

/* Length of mailbox message entry in bytes */
#define FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH 4

/* Mailbox message argument sizes in bytes used in argument headers. */
#define FM_HOST_SRV_ERR_TYPE_SIZE               (7 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_LPORT_MAP_TYPE_SIZE         (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_XCAST_MODE_TYPE_SIZE        (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_MAC_UPDATE_TYPE_SIZE        (3 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_CONFIG_TYPE_SIZE            (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_CREATE_FLOW_TABLE_TYPE_SIZE (4 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_DELETE_FLOW_TABLE_TYPE_SIZE (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_DELETE_FLOW_TYPE_SIZE       (2 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_FLOW_HANDLE_TYPE_SIZE       (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_FLOW_STATE_TYPE_SIZE        (3 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_GET_1588_INFO_TYPE_SIZE     (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_TEST_MESSAGE_TYPE_SIZE      (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_PORT_TYPE_SIZE              (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_UPDATE_PVID_TYPE_SIZE       (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_PACKET_TIMESTAMP_TYPE_SIZE  (5 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_TMSTAMP_MODE_REQ_TYPE_SIZE  (1 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_TMSTAMP_MODE_RESP_TYPE_SIZE (2 * FM_MAILBOX_QUEUE_ENTRY_BYTE_LENGTH)


/* Mailbox message IDs. Used in transaction headers. */
typedef enum _fm_mailboxMessageId
{
    /* ID for the TEST_MESSAGE message. */
    FM_MAILBOX_MSG_TEST_ID = 0x000,

    /* ID for the XCAST_MODES message. */
    FM_MAILBOX_MSG_SET_XCAST_MODES_ID = 0x001,

    /* ID for the UPDATE_MAC_FWD_RULE message. */
    FM_MAILBOX_MSG_UPDATE_MAC_FWD_RULE_ID = 0x002,

    /* ID for the LPORT_MAP message. */
    FM_MAILBOX_MSG_MAP_LPORT_ID = 0x100,

    /* ID for the LPORT_CREATE message. */
    FM_MAILBOX_MSG_CREATE_LPORT_ID = 0x200,

    /* ID for the LPORT_DELETE message. */
    FM_MAILBOX_MSG_DELETE_LPORT_ID = 0x201,

    /* ID for the CONFIG message. */
    FM_MAILBOX_MSG_CONFIG_ID = 0x300,

    /* ID for the UPDATE_PVID message. */
    FM_MAILBOX_MSG_UPDATE_PVID_ID = 0x400,

    /* ID for the CREATE_FLOW_TABLE message. */
    FM_MAILBOX_MSG_CREATE_FLOW_TABLE_ID = 0x501,

    /* ID for the DELETE_FLOW_TABLE message. */
    FM_MAILBOX_MSG_DELETE_FLOW_TABLE_ID = 0x502,

    /* ID for the UPDATE_FLOW message. */
    FM_MAILBOX_MSG_UPDATE_FLOW_ID = 0x503,

    /* ID for the DELETE_FLOW message. */
    FM_MAILBOX_MSG_DELETE_FLOW_ID = 0x504,

    /* ID for the SET_FLOW_STATE message. */
    FM_MAILBOX_MSG_SET_FLOW_STATE_ID = 0x505,

    /* ID for the GET_HW_PLATFORM message. */
    FM_MAILBOX_MSG_GET_HW_PLATFORM_ID = 0x601,

    /* ID for the DELIVER_PACKET_TIMESTAMP message. */
    FM_MAILBOX_MSG_DELIVER_PACKET_TIMESTAMP_ID = 0x701,

    /* ID for the TX_TIMESTAMP_MODE message. */
    FM_MAILBOX_MSG_SET_TIMESTAMP_MODE_ID = 0x702

} fm_mailboxMessageId;


/* Mailbox message argument types. Used in argument headers. */
typedef enum _fm_mailboxMessageArgumentType
{
    /* Argument type for response message for most of the request.
     * Used with fm_hostSrvErr argument. */
    FM_HOST_SRV_RETURN_ERR_TYPE = 0x0000,

    /* Argument type for response message for LPORT_MAP service.
     * Used with fm_hostSrvLportMap argument. */
    FM_HOST_SRV_MAP_LPORT_TYPE = 0x0001,

    /* Argument type for request message for XCAST_MODES service.
     * Used with fm_hostSrvXcastMode argument. */
    FM_HOST_SRV_SET_XCAST_MODE_TYPE = 0x0002,

    /* Argument type for request message for UPDATE_MAC_FWD_RULE service.
     * Used with fm_hostSrvMACUpdate argument. */
    FM_HOST_SRV_UPDATE_MAC_TYPE = 0x0003,

    /* Argument type not used for now. */
    FM_HOST_SRV_UPDATE_VLAN_TYPE = 0x0004,

    /* Argument type for request message for CONFIG service.
     * Used with fm_hostSrvConfig argument. */
    FM_HOST_SRV_REQUEST_CONFIG_TYPE = 0x0005,

    /* Argument type for request message for CREATE_FLOW_TABLE service.
     * Used with fm_hostSrvCreateFlowTable argument. */
    FM_HOST_SRV_CREATE_FLOW_TABLE_TYPE = 0x0006,

    /* Argument type for request message for DELETE_FLOW_TABLE service.
     * Used with fm_hostSrvDeleteFlowTable argument. */
    FM_HOST_SRV_DELETE_FLOW_TABLE_TYPE = 0x0007,

    /* Argument type for request message for UPDATE_FLOW service.
     * Used with fm_hostSrvUpdateFlow argument. */
    FM_HOST_SRV_UPDATE_FLOW_TYPE = 0x0008,

    /* Argument type for request message for SET_FLOW_STATE service.
     * Used with fm_hostSrvFlowState argument. */
    FM_HOST_SRV_SET_FLOW_STATE_TYPE = 0x0009,

    /* Argument type for response message for UPDATE_FLOW service.
     * Used with fm_hostSrvFlowHandle argument. */
    FM_HOST_SRV_HANDLE_FLOW_TYPE = 0x000a,

    /* Argument type for request message for DELETE_FLOW service.
     * Used with fm_hostSrvDeleteFlow argument. */
    FM_HOST_SRV_DELETE_FLOW_TYPE = 0x000b,

    /* Argument type for request message for LPORT_CREATE and 
     * LPORT_DELETE services. Used with fm_hostSrvPort argument. */
    FM_HOST_SRV_CREATE_DELETE_LPORT_TYPE = 0x000c,

    /* Argument type for response message for UPDATE_PVID service.
     * Used with fm_hostSrvUpdatePvid argument. */
    FM_HOST_SRV_UPDATE_PVID_TYPE = 0x000d,

    /* Argument type for request message for HW_PLATFORM service.
     * Details are not defined for now. */
    FM_HOST_SRV_GET_HW_PLATFORM_TYPE = 0x000f,

    /* Argument type for response message for DELIVER_PACKET_TIMESTAMP
     * service. Used with fm_hostSrvPacketTimestamp argument. */
    FM_HOST_SRV_PACKET_TIMESTAMP_TYPE = 0x0010,

    /* Argument type for request message for TX_TIMESTAMP_MODE
     * service. Used with fm_hostSrvTimestampModeReq argument. */
    FM_HOST_SRV_SET_TIMESTAMP_MODE_REQ_TYPE = 0x0011,

    /* Argument type for response message for TX_TIMESTAMP_MODE
     * service. Used with fm_hostSrvTimestampModeResp argument. */
    FM_HOST_SRV_SET_TIMESTAMP_MODE_RESP_TYPE = 0x0012

} fm_mailboxMessageArgumentType;


/* Mailbox CONFIG request attribute constants. */
typedef enum _fm_mailboxConfigAttributeConstant
{
    FM_MAILBOX_CONFIG_PEP_LEARNING = 0x0000

} fm_mailboxConfigAttributeConstant;


/* Mailbox xcast modes for XCAST_MODES request. */
typedef enum _fm_xcastMode
{
    FM_HOST_SRV_XCAST_MODE_ALLMULTI = 0x0000,

    FM_HOST_SRV_XCAST_MODE_MULTI = 0x0001,

    FM_HOST_SRV_XCAST_MODE_PROMISC = 0x0002,

    FM_HOST_SRV_XCAST_MODE_NONE = 0x0003
 
} fm_xcastMode;

/* Mailbox flag controlling which flooding lists should 
   be updated when processing XCAST_MODES request. */
typedef enum _fm_mailboxXcastFloodingMode
{
    FM_MAILBOX_XCAST_FLOOD_MODE_UCAST = (1 << 0),

    FM_MAILBOX_XCAST_FLOOD_MODE_MCAST = (1 << 1),

    FM_MAILBOX_XCAST_FLOOD_MODE_BCAST = (1 << 2),

} fm_mailboxXcastFloodingMode;

/* Mailbox control header structure to manage request/response queues. */
typedef struct _fm_mailboxControlHeader
{
    /* Switch manager error flags */
    fm_uint16 smErrorFlags;

    /* Switch manager version */
    fm_uint16 smVersion;

    /* PF error flags */
    fm_uint16 pfErrorFlags;

    /* PF version */
    fm_uint16 pfVersion;

    /* Request queue head */
    fm_uint16 reqHead;

    /* Request queue tail */
    fm_uint16 reqTail;

    /* Response queue head */
    fm_uint16 respHead;

    /* Response queue tail */
    fm_uint16 respTail;

} fm_mailboxControlHeader;


/* Message header structure. Used as transaction and argument headers. */
typedef struct _fm_mailboxMessageHeader
{
    /* Transaction/argument type. 
     *
     * Supported transaction types are listed in fm_mailboxMessageId enum.
     * Supported argument types are listed in fm_mailboxMessageArgumentType enum. */
    fm_uint16 type;

    /* Message header flag field. Current deffinitions are as follows:
     * 
     * +---------------------------+
     * | Bit |      Definition     |
     * +---------------------------+
     * |  0  |   Start of message  |
     * +---------------------------+
     */ 
    fm_uint16 flags;

    /* Transaction/argument length. The value should be in bytes
     * and exclude the transaction/argument header. */
    fm_uint16 length;

} fm_mailboxMessageHeader;


/* Mailbox structure used as a return type for most of the request. */
typedef struct _fm_hostSrvErr
{
    /* Error code returned from API if any error showed up
     * while processing the message. */
    fm_uint32 statusCode;

    /* Number of used mac table entries. */
    fm_uint32 macTableRowsUsed;

    /* Number of available mac table entries. */
    fm_uint32 macTableRowsAvailable;

    /* Number of used nexthop table entries. */
    fm_uint32 nexthopTableRowsUsed;

    /* Number of available nexthop table entries. */
    fm_uint32 nexthopTableRowsAvailable;

    /* Number of used ffu rules. */
    fm_uint32 ffuRulesUsed;

    /* Number of available ffu rules. */
    fm_uint32 ffuRulesAvailable;

} fm_hostSrvErr;


/* Mailbox structure used as a return type for LPORT_MAP message. */
typedef struct _fm_hostSrvLportMap
{
    /* Glort value and mask to define glort range
     * that can be used for queue mapping. */

    fm_uint16 glortValue;

    fm_uint16 glortMask;

    /* Additional field, not sent to driver. */
    fm_uint16 glortsPerPep;

} fm_hostSrvLportMap;


/* Mailbox structure used as an argument for 
 * LPORT_CREATE and LPORT_DELETE messages. */
typedef struct _fm_hostSrvPort
{
    /* First glort to be used by host driver. */
    fm_uint16 firstGlort;

    /* Number of glorts to be used by host driver. */
    fm_uint16 glortCount;

} fm_hostSrvPort;


/* Mailbox structure used as an argument for XCAST_MODES message. */
typedef struct _fm_hostSrvXcastMode
{
    /* Glort on which to operate. */
    fm_uint16 glort;

    /* New xcast mode to set. */
    fm_xcastMode mode;

} fm_hostSrvXcastMode;


/* Mailbox structure used as an argument for UPDATE_MAC_FWD_RULE message. */
typedef struct _fm_hostSrvMACUpdate
{
    /* Lower bytes of mac address to add */
    fm_uint32 macAddressLower;

    /* Upper bytes of mac address to add */
    fm_uint16 macAddressUpper;

    /* Vlan value associated with the mac. */
    fm_uint16 vlan;

    /* Glort value to map MAC entry to. */
    fm_uint16 glort;

    /* Flag bit mask. Current definitions are as follows:
     *
     * +---------------------------------+
     * | Bit |      Definition           |
     * +---------------------------------+
     * |  0  | SECURE BIT (1 if secure)  |
     * +---------------------------------+
     */
    fm_uint16 flags;

    /* Action field. Current action definitions are as follows:
     *
     * +-------------------------------+
     * | Bit |      Definition         |
     * +-------------------------------+
     * |  0  |   ADD (0) / DELETE (1)  |
     * +-------------------------------+
     */
    fm_uint16 action;

} fm_hostSrvMACUpdate;


/* Mailbox structure used as an argument for CONFIG message. */
typedef struct _fm_hostSrvConfig
{
    /* Configuration attribute to be set. */
    fm_mailboxConfigAttributeConstant configurationAttributeConstant;

    /* Configuration value to be set. */
    fm_uint16 value;

} fm_hostSrvConfig;


/* Mailbox structure used as an argument for CREATE_FLOW_TABLE message. */
typedef struct _fm_hostSrvCreateFlowTable
{
    /* Flow table type. Supported types are:
     *
     * - FM_FLOW_TCAM_TABLE
     * - FM_FLOW_BST_TABLE
     * - FM_FLOW_TE_TABLE. */
    fm_uint16 tableType;

    /* Flow table index. */
    fm_uint32 tableIndex;

    /* Lower bytes of flow condition mask. */
    fm_uint32 flowConditionBitmaskLower;

    /* Upper bytes of flow condition mask. */
    fm_uint32 flowConditionBitmaskUpper;

} fm_hostSrvCreateFlowTable;


/* Mailbox structure used as an argument for DELETE_FLOW_TABLE message. */
typedef struct _fm_hostSrvDeleteFlowTable
{
    /* Flow table index. */
    fm_uint32 tableIndex;

    /* Flow table type. Supported types are:
     *
     * - FM_FLOW_TCAM_TABLE
     * - FM_FLOW_BST_TABLE
     * - FM_FLOW_TE_TABLE. */
    fm_uint16 tableType;

} fm_hostSrvDeleteFlowTable;


/* Mailbox structure used as an argument for UPDATE_FLOW message. */
typedef struct _fm_hostSrvUpdateFlow
{
    /* Flow priority field. */
    fm_uint16 priority;

    /* Flow table index. */
    fm_uint32 tableIndex;

    /* Flow ID:
     *
     * - When addding new flow, this value must be 0xFFFF.
     * - When updating an existing flow, this value must be set to ID of the flow. */
    fm_uint32 flowID;

    /* Flow condition mask */
    fm_flowCondition        condition;

    /* Flow action mask. */
    fm_flowAction           action;

    /* Flow values set according to read arguments. */
    fm_flowValue            flowVal;

    /* Flow params set according to read arguments. */
    fm_flowParam            flowParam;

} fm_hostSrvUpdateFlow;


/* Mailbox structure used as an argument for SET_FLOW_STATE message. */
typedef struct _fm_hostSrvFlowState
{
    /* Flow table index. */
    fm_uint32 tableIndex;

    /* Flow ID. */
    fm_uint32 flowID;

    /* Flow state. Supported values are:
     *
     * - FM_FLOW_STATE_STANDBY
     * - FM_FLOW_STATE_ENABLED. */
    fm_uint16 flowState;

} fm_hostSrvFlowState;


/* Mailbox structure used as a return type for UPDATE_FLOW message. */
typedef struct _fm_hostSrvFlowHandle
{
    /* Flow ID. */
    fm_uint32 flowID;

} fm_hostSrvFlowHandle;


/* Mailbox structure used as an argument for DELETE_FLOW message. */
typedef struct _fm_hostSrvDeleteFlow
{
    /* Flow table index. */
    fm_uint32 tableIndex;

    /* Flow ID. */
    fm_uint32 flowID;

} fm_hostSrvDeleteFlow;

/* Mailbox structure used as a return type for UPDATE_PVID message. */
typedef struct _fm_hostSrvUpdatePvid
{
    /* Glort value of virtual port. */
    fm_uint16 glort;

    /* Pvid value for given glort. */
    fm_uint16 pvid;

} fm_hostSrvUpdatePvid;


/* Mailbox structure used as a return type for 
 * DELIVER_PACKET_TIMESTAMP message. */
typedef struct _fm_hostSrvPacketTimestamp
{

    /* Sglort value used for PEP->PEP timestamps. */
    fm_uint16 sglort;

    /* Dglort value used for PEP->EPL timestamps. */
    fm_int16 dglort;

    /* 64-bit Egress Timestamp */
    fm_uint64 egressTimestamp;

    /* 64-bit Ingress Timestamp */
    fm_uint64 ingressTimestamp;

} fm_hostSrvPacketTimestamp;


/* Mailbox structure used as an argument for 
 * TX_TIMESTAMP_MODE message. */
typedef struct _fm_hostSrvTimestampModeReq
{

    /* Glort value. */
    fm_uint16 glort;

    /* Mode value to be set. */
    fm_portTxTimestampMode mode;

} fm_hostSrvTimestampModeReq;


/* Mailbox structure used as a return type for 
 * TX_TIMESTAMP_MODE message. */
typedef struct _fm_hostSrvTimestampModeResp
{

    /* Glort value. */
    fm_uint16 glort;

    /* Max mode value that can be set. */
    fm_portTxTimestampMode maxMode;

    /* Status value from setting mode.*/
    fm_status status;

} fm_hostSrvTimestampModeResp;

/* Tracks resources created on Host Interface requests via mailbox
 * for a given virtual port. */
typedef struct _fm_mailboxResources
{
    /* A tree holding MAC addresses. 
     * Entries would include {key == MAC_VLAN, value == NULL} pairs. */
    fm_tree mailboxMacResource;

    /* A tree holding flow tables and IDs. 
       Entries would include {key == FLOWID_FLOWTABLE, value == NULL} pairs. */
    fm_tree mailboxFlowResource;

    /* A tree holding flow tables.
       Entries would include {key = FLOW_TABLE, value = TABLE_TYPE } pairs. */
    fm_tree mailboxFlowTableResource;

} fm_mailboxResources;

/* Holds the state of mailbox related information. */
typedef struct _fm_mailboxInfo
{

    /* Starting glort number for mailbox. */
    fm_uint16 glortBase;

    /* Specifies the range of glorts per Pep. First glort is
     * calculated according to the formula:
     * firstGlortForPep = glortBase + (pepNumber * glortsPerPep). */
    fm_uint16 glortMask;

    /* Specifies number of glorts available per pep. */
    fm_uint16 glortsPerPep;

    /* Mcast group number created to manage unicast flooding for HNI services */
    fm_int mcastGroupForUcastFlood;

    /* Array to store number of virtual ports
     * added to a unicast flood glort portmask per PEP.
     * In SWAG configuration these number will be stored only by SWAG members. */
    fm_int *numberOfVirtualPortsAddedToUcastFlood;

    /* Mcast group number created to manage multicast flooding for HNI services */
    fm_int mcastGroupForMcastFlood;

    /* Array to store number of virtual ports
     * added to a multicast flood glort portmask per PEP. 
     * In SWAG configuration these number will be stored only by SWAG members. */
    fm_int *numberOfVirtualPortsAddedToMcastFlood;

    /* Mcast group number created to manage broadcast flooding for HNI services */
    fm_int mcastGroupForBcastFlood;

    /* Array to store number of virtual ports
     * added to a broadcast flood glort portmask per PEP.
     * In SWAG configuration these number will be stored only by SWAG members. */
    fm_int *numberOfVirtualPortsAddedToBcastFlood;

    /* Tree tracking resources used on host interface requests. */
    fm_tree mailboxResourcesPerVirtualPort;

    /* A tree holding default PVID value per GLORT related to virtual ports.
     * This is to cache PVID values even if virtual ports are deleted as
     * host driver calls LPORT_DELETE/LPORT_CREATE during resets. 
     * After such reset we want to keep PVID values. */
    fm_tree defaultPvidPerGlort;

} fm_mailboxInfo;

void fmSendHostSrvErrResponse(fm_int                        sw,
                              fm_int                        pepNb,
                              fm_status                     status,
                              fm_mailboxControlHeader *     ctrlHdr,
                              fm_mailboxMessageId           msgTypeId,
                              fm_mailboxMessageArgumentType argType);

fm_status fmMapLportProcess(fm_int                   sw,
                            fm_int                   pepNb,
                            fm_mailboxControlHeader *ctrlHdr,
                            fm_mailboxMessageHeader *pfTrHdr);

fm_status fmCreateLportProcess(fm_int                   sw,
                               fm_int                   pepNb,
                               fm_mailboxControlHeader *ctrlHdr,
                               fm_mailboxMessageHeader *pfTrHdr);

fm_status fmDeleteLportProcess(fm_int                   sw,
                               fm_int                   pepNb,
                               fm_mailboxControlHeader *ctrlHdr,
                               fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetXcastModesProcess(fm_int                   sw,
                                 fm_int                   pepNb,
                                 fm_mailboxControlHeader *ctrlHdr,
                                 fm_mailboxMessageHeader *pfTrHdr);

fm_status fmUpdateMacFwdRuleProcess(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *ctrlHdr,
                                    fm_mailboxMessageHeader *pfTrHdr);

fm_status fmConfigReqProcess(fm_int                   sw,
                             fm_int                   pepNb,
                             fm_mailboxControlHeader *ctrlHdr,
                             fm_mailboxMessageHeader *pfTrHdr);

fm_status fmCreateFlowTableProcess(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *pfTrHdr);

fm_status fmDeleteFlowTableProcess(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *pfTrHdr);

fm_status fmUpdateFlowProcess(fm_int                   sw,
                              fm_int                   pepNb,
                              fm_mailboxControlHeader *ctrlHdr,
                              fm_mailboxMessageHeader *pfTrHdr);

fm_status fmDeleteFlowProcess(fm_int                   sw,
                              fm_int                   pepNb,
                              fm_mailboxControlHeader *ctrlHdr,
                              fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetFlowStateProcess(fm_int                   sw,
                                fm_int                   pepNb,
                                fm_mailboxControlHeader *ctrlHdr,
                                fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetTimestampModeProcess(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *ctrlHdr,
                                    fm_mailboxMessageHeader *pfTrHdr);

fm_status fmUnknownRequestProcess(fm_int                   sw,
                                  fm_int                   pepNb,
                                  fm_mailboxControlHeader *ctrlHdr,
                                  fm_mailboxMessageHeader *pfTrHdr);

fm_status fmProcessLoopbackRequest(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *controlHeader);

fm_status fmDeliverPacketTimestamp(fm_int    sw,
                                   fm_uint16 sglort,
                                   fm_uint16 dglort,
                                   fm_uint64 egressTimestamp,
                                   fm_uint64 ingressTimestamp);

fm_status fmMailboxAllocateDataStructures(fm_int sw);

fm_status fmMailboxFreeDataStructures(fm_int sw);

fm_status fmMailboxInit(fm_int sw);

fm_status fmMailboxFreeResources(fm_int sw);

fm_status fmNotifyPvidUpdate(fm_int sw,
                             fm_int logicalPort,
                             fm_int pvid);

fm_status fmCleanupResourcesForPep(fm_int sw,
                                   fm_int pepNb);

fm_status fmFindInternalPortByMailboxGlort(fm_int    sw,
                                           fm_uint32 glort,
                                           fm_int *  logicalPort);

#endif /* __FM_FM_API_MAILBOX_INT_H */

