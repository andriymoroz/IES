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

#define GET_MAILBOX_INFO(sw) \
    &((fm_switch *)(GET_SWITCH_PTR( sw )))->mailboxInfo;

/* Get the switch aggregate id if exists, otherwise just return argument value. */
#define GET_SWITCH_AGGREGATE_ID_IF_EXIST(sw)\
    ( ((fm_switch *)(GET_SWITCH_PTR( sw )))->swag < 0 ) ? \
    (sw) : ( ((fm_switch *)(GET_SWITCH_PTR( sw )))->swag )

/* Bit defines for configurable fields */
#define FM_MAILBOX_BITS_l_0_7                                      0
#define FM_MAILBOX_BITS_h_0_7                                      15
#define FM_MAILBOX_BITS_l_0_15                                     0
#define FM_MAILBOX_BITS_h_0_15                                     15
#define FM_MAILBOX_BITS_l_0_31                                     0
#define FM_MAILBOX_BITS_h_0_31                                     31
#define FM_MAILBOX_BITS_l_8_15                                     8
#define FM_MAILBOX_BITS_h_8_15                                     15
#define FM_MAILBOX_BITS_l_16_23                                    16
#define FM_MAILBOX_BITS_h_16_23                                    23
#define FM_MAILBOX_BITS_l_16_31                                    16
#define FM_MAILBOX_BITS_h_16_31                                    31
#define FM_MAILBOX_BITS_l_24_31                                    24
#define FM_MAILBOX_BITS_h_24_31                                    31

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

/* fm_hostSrvCreateTable */
#define FM_MAILBOX_SRV_CREATE_TABLE_l_TABLE_INDEX                 0
#define FM_MAILBOX_SRV_CREATE_TABLE_h_TABLE_INDEX                 31
#define FM_MAILBOX_SRV_CREATE_TABLE_l_TABLE_TYPE                  0
#define FM_MAILBOX_SRV_CREATE_TABLE_h_TABLE_TYPE                  31
#define FM_MAILBOX_SRV_CREATE_TABLE_l_NUM_OF_ACT                  0
#define FM_MAILBOX_SRV_CREATE_TABLE_h_NUM_OF_ACT                  7
#define FM_MAILBOX_SRV_CREATE_TABLE_l_FLAGS                       8
#define FM_MAILBOX_SRV_CREATE_TABLE_h_FLAGS                       15
#define FM_MAILBOX_SRV_CREATE_TABLE_l_NUM_OF_ENTR                 0
#define FM_MAILBOX_SRV_CREATE_TABLE_h_NUM_OF_ENTR                 31
#define FM_MAILBOX_SRV_CREATE_TABLE_l_COND_BITMASK                0
#define FM_MAILBOX_SRV_CREATE_TABLE_h_COND_BITMASK                31
#define FM_MAILBOX_SRV_CREATE_TABLE_l_ACTION_BITMASK              0
#define FM_MAILBOX_SRV_CREATE_TABLE_h_ACTION_BITMASK              31

/* fm_hostSrvTable */
#define FM_MAILBOX_SRV_TABLE_l_TABLE_INDEX                        0
#define FM_MAILBOX_SRV_TABLE_h_TABLE_INDEX                        31

/* fm_hostSrvFlowTableRange */
#define FM_MAILBOX_SRV_TABLE_RANGE_l_FIRST_INDEX                  0
#define FM_MAILBOX_SRV_TABLE_RANGE_h_FIRST_INDEX                  31
#define FM_MAILBOX_SRV_TABLE_RANGE_l_LAST_INDEX                   0
#define FM_MAILBOX_SRV_TABLE_RANGE_h_LAST_INDEX                   31

/* Fields for GET_TABLES response. */
#define FM_MAILBOX_SRV_GET_TABLES_l_TABLE_INDEX                   0
#define FM_MAILBOX_SRV_GET_TABLES_h_TABLE_INDEX                   31
#define FM_MAILBOX_SRV_GET_TABLES_l_TABLE_TYPE                    0
#define FM_MAILBOX_SRV_GET_TABLES_h_TABLE_TYPE                    31
#define FM_MAILBOX_SRV_GET_TABLES_l_NUM_OF_ACT                    0
#define FM_MAILBOX_SRV_GET_TABLES_h_NUM_OF_ACT                    7
#define FM_MAILBOX_SRV_GET_TABLES_l_FLAGS                         8
#define FM_MAILBOX_SRV_GET_TABLES_h_FLAGS                         15
#define FM_MAILBOX_SRV_GET_TABLES_l_MAX_NUM_OF_ENTR               0
#define FM_MAILBOX_SRV_GET_TABLES_h_MAX_NUM_OF_ENTR               31
#define FM_MAILBOX_SRV_GET_TABLES_l_NUM_OF_EMPTY_ENTR             0
#define FM_MAILBOX_SRV_GET_TABLES_h_NUM_OF_EMPTY_ENTR             31
#define FM_MAILBOX_SRV_GET_TABLES_l_COND_BITMASK                  0
#define FM_MAILBOX_SRV_GET_TABLES_h_COND_BITMASK                  31
#define FM_MAILBOX_SRV_GET_TABLES_l_ACT_BITMASK                   0
#define FM_MAILBOX_SRV_GET_TABLES_h_ACT_BITMASK                   31

/* fm_hostSrvFlowEntry */
#define FM_MAILBOX_SRV_FLOW_ENTRY_l_TABLE_INDEX                   0
#define FM_MAILBOX_SRV_FLOW_ENTRY_h_TABLE_INDEX                   31
#define FM_MAILBOX_SRV_FLOW_ENTRY_l_FLOW_ID                       0
#define FM_MAILBOX_SRV_FLOW_ENTRY_h_FLOW_ID                       31
#define FM_MAILBOX_SRV_FLOW_ENTRY_l_PRIORITY                      0
#define FM_MAILBOX_SRV_FLOW_ENTRY_h_PRIORITY                      31
#define FM_MAILBOX_SRV_FLOW_ENTRY_l_GLORT                         0
#define FM_MAILBOX_SRV_FLOW_ENTRY_h_GLORT                         15
#define FM_MAILBOX_SRV_FLOW_ENTRY_l_COND_BITMASK                  0
#define FM_MAILBOX_SRV_FLOW_ENTRY_h_COND_BITMASK                  31
#define FM_MAILBOX_SRV_FLOW_ENTRY_l_ACT_BITMASK                   0
#define FM_MAILBOX_SRV_FLOW_ENTRY_h_ACT_BITMASK                   31

/* fm_hostSrvFlowHandle */
#define FM_MAILBOX_SRV_FLOW_HANDLE_l_TABLE_INDEX                  0
#define FM_MAILBOX_SRV_FLOW_HANDLE_h_TABLE_INDEX                  31
#define FM_MAILBOX_SRV_FLOW_HANDLE_l_FLOW_ID                      0
#define FM_MAILBOX_SRV_FLOW_HANDLE_h_FLOW_ID                      31

/* fm_hostSrvFlowRange */
#define FM_MAILBOX_SRV_FLOW_RANGE_l_TABLE_INDEX                   0
#define FM_MAILBOX_SRV_FLOW_RANGE_h_TABLE_INDEX                   31
#define FM_MAILBOX_SRV_FLOW_RANGE_l_FIRST_FLOW_ID                 0
#define FM_MAILBOX_SRV_FLOW_RANGE_h_FIRST_FLOW_ID                 31
#define FM_MAILBOX_SRV_FLOW_RANGE_l_LAST_FLOW_ID                  0
#define FM_MAILBOX_SRV_FLOW_RANGE_h_LAST_FLOW_ID                  31

/* fm_hostSrvNoOfVfs. */
#define FM_MAILBOX_SRV_NO_OF_VFS_l_GLORT                          0
#define FM_MAILBOX_SRV_NO_OF_VFS_h_GLORT                          15
#define FM_MAILBOX_SRV_NO_OF_VFS_l_NO_OF_VFS                      16
#define FM_MAILBOX_SRV_NO_OF_VFS_h_NO_OF_VFS                      31

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

/* fm_hostSrvTimestampModeResp */
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_l_GLORT                 0
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_h_GLORT                 15
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_l_MODE_ENABLED          16
#define FM_MAILBOX_SRV_TIMESTAMP_MODE_RESP_h_MODE_ENABLED          31

/* fm_hostSrvMasterClkOffset */
#define FM_MAILBOX_SRV_MASTER_CLK_OFFSET_l_OFFSET_LOW              0
#define FM_MAILBOX_SRV_MASTER_CLK_OFFSET_h_OFFSET_LOW              31
#define FM_MAILBOX_SRV_MASTER_CLK_OFFSET_l_OFFSET_UPP              0
#define FM_MAILBOX_SRV_MASTER_CLK_OFFSET_h_OFFSET_UPP              31

/* fm_hostSrvInnOutMac */
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_OUT_MAC_LOW                   0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_OUT_MAC_LOW                   31
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_OUT_MAC_UPP                   0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_OUT_MAC_UPP                   15
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_OUT_MAC_MASK_LOW              16
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_OUT_MAC_MASK_LOW              31
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_OUT_MAC_MASK_UPP              0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_OUT_MAC_MASK_UPP              31

#define FM_MAILBOX_SRV_INN_OUT_MAC_l_INN_MAC_LOW                   0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_INN_MAC_LOW                   31
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_INN_MAC_UPP                   0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_INN_MAC_UPP                   15
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_INN_MAC_MASK_LOW              16
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_INN_MAC_MASK_LOW              31
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_INN_MAC_MASK_UPP              0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_INN_MAC_MASK_UPP              31

#define FM_MAILBOX_SRV_INN_OUT_MAC_l_VNI                           0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_VNI                           31
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_OUTER_L4_PORT                 0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_OUTER_L4_PORT                 15
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_TUNNEL_TYPE                   16
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_TUNNEL_TYPE                   23
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_ACTION                        24
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_ACTION                        31
#define FM_MAILBOX_SRV_INN_OUT_MAC_l_GLORT                         0
#define FM_MAILBOX_SRV_INN_OUT_MAC_h_GLORT                         15

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

/* Flag indicating transaction header. */
#define FM_MAILBOX_HEADER_TRANSACTION_FLAG (1 << 0)
/* Flag indicating start of message. */
#define FM_MAILBOX_HEADER_START_OF_MESSAGE (1 << 1)
/* Flag indicating end of message. */
#define FM_MAILBOX_HEADER_END_OF_MESSAGE   (1 << 2)

/* Mailbox control header error types used in control header. */
#define FM_MAILBOX_ERR_TYPE_NONE                0
#define FM_MAILBOX_ERR_TYPE_INVALID_VERSION     1

/* Add/delete flag is bit 0 from fm_hostSrvMACUpdate action area. */
#define FM_HOST_SRV_MAC_UPDATE_b_ADD_DELETE                    0

/* MAC secure flag is bit 0 from fm_hostSrvMACUpdate flag area. */
#define FM_HOST_SRV_MAC_UPDATE_b_MAC_SECURE                    0

/* Action types for filtering inner/outer MAC adresses */
#define FM_HOST_SRV_INN_OUT_MAC_ACTION_TYPE_ADD                0
#define FM_HOST_SRV_INN_OUT_MAC_ACTION_TYPE_DEL                1

/* Value for no matching the VNI field when filtering inner/outer MAC */
#define FM_HOST_SRV_INN_OUT_MAC_VNI_NO_MATCH                   0xFFFFFFFF

/* For adding virtual ports as a listeners to flooding multicast
 * groups we will use the vlan = 0. */
#define FM_MAILBOX_DEF_VLAN_FOR_FLOOD_MCAST_GROUPS             0

/* Max number of actions supported by rules in flow table */
#define FM_MAILBOX_FLOW_TABLE_MAX_ACTION_SUPPORTED             1

/* The ACL rule that will be used for filtering INNER/OUTER MAC addresses */
#define FM_MAILBOX_MAC_FILTER_ACL                              24000000

/* Length of mailbox message entry in bits */
#define FM_MBX_ENTRY_BIT_LENGTH  32

/* Length of mailbox message entry in bytes */
#define FM_MBX_ENTRY_BYTE_LENGTH 4

/* Flags indicating who wants to create given flow table. Tables created on
 * driver demand are VF-accessible ones. */
#define FM_MAILBOX_USER_TABLE                                   (1 << 0)
#define FM_MAILBOX_DRIVER_TABLE                                 (1 << 1)

/* Mailbox message argument sizes in bytes used in argument headers. */
#define FM_HOST_SRV_ERR_TYPE_SIZE               (7 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_LPORT_MAP_TYPE_SIZE         (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_XCAST_MODE_TYPE_SIZE        (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_MAC_UPDATE_TYPE_SIZE        (3 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_CONFIG_TYPE_SIZE            (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_CREATE_TABLE_TYPE_SIZE      (8 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_TABLE_TYPE_SIZE             (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_FLOW_TABLE_RANGE_TYPE_SIZE  (2 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_GET_TABLES_TYPE_SIZE        (9 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_FLOW_ENTRY_TYPE_SIZE        (8 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_FLOW_HANDLE_TYPE_SIZE       (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_FLOW_RANGE_TYPE_SIZE        (3 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_NO_OF_VFS_TYPE_SIZE         (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_GET_1588_INFO_TYPE_SIZE     (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_TEST_MESSAGE_TYPE_SIZE      (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_PORT_TYPE_SIZE              (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_UPDATE_PVID_TYPE_SIZE       (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_PACKET_TIMESTAMP_TYPE_SIZE  (5 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_TMSTAMP_MODE_RESP_TYPE_SIZE (1 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE_SIZE (2 * FM_MBX_ENTRY_BYTE_LENGTH)
#define FM_HOST_SRV_INN_OUT_MAC_TYPE_SIZE       (9 * FM_MBX_ENTRY_BYTE_LENGTH)


/* Mailbox message IDs. Used in transaction headers. */
typedef enum _fm_mailboxMessageId
{
    /* ID for the TEST_MESSAGE message. */
    FM_MAILBOX_MSG_TEST_ID = 0x000,

    /* ID for the XCAST_MODES message. */
    FM_MAILBOX_MSG_SET_XCAST_MODES_ID = 0x001,

    /* ID for the UPDATE_MAC_FWD_RULE message. */
    FM_MAILBOX_MSG_UPDATE_MAC_FWD_RULE_ID = 0x002,

    /* ID for the FILTER_INNER_OUTER_MAC message. */
    FM_MAILBOX_MSG_INN_OUT_MAC_ID = 0x011,

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

    /* ID for the GET_TABLES message. */
    FM_MAILBOX_MSG_GET_TABLES_ID = 0x501,

    /* ID for the SET_RULES message. */
    FM_MAILBOX_MSG_SET_RULES_ID = 0x504,

    /* ID for the GET_RULES message. */
    FM_MAILBOX_MSG_GET_RULES_ID = 0x505,

    /* ID for the DEL_RULES message. */
    FM_MAILBOX_MSG_DEL_RULES_ID = 0x506,

    /* ID for the CREATE_TABLE message. */
    FM_MAILBOX_MSG_CREATE_TABLE_ID = 0x507,

    /* ID for the DESTROY_TABLE message. */
    FM_MAILBOX_MSG_DESTROY_TABLE_ID = 0x508,

    /* ID for the SET_NO_OF_VFS message. */
    FM_MAILBOX_MSG_SET_NO_OF_VFS_ID = 0x509,

    /* ID for the GET_HW_PLATFORM message. */
    FM_MAILBOX_MSG_GET_HW_PLATFORM_ID = 0x601,

    /* ID for the DELIVER_PACKET_TIMESTAMP message. */
    FM_MAILBOX_MSG_DELIVER_PACKET_TIMESTAMP_ID = 0x701,

    /* ID for the TX_TIMESTAMP_MODE message. */
    FM_MAILBOX_MSG_SET_TIMESTAMP_MODE_ID = 0x702,

    /* ID for the MASTER_CLK_OFFSET message. */
    FM_MAILBOX_MSG_MASTER_CLK_OFFSET_ID = 0x703

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

    /* Argument type for request message for DESTROY_TABLE service.
     * Used with fm_hostSrvTable argument. */
    FM_HOST_SRV_TABLE_TYPE = 0x0006,

    /* Argument type for request message for CREATE_TABLE service.
     * Used with fm_hostSrvCreateTable argument. */
    FM_HOST_SRV_CREATE_TABLE_TYPE = 0x0007,

    /* Argument type for response message for GET_TABLES service. */
    FM_HOST_SRV_TABLE_LIST_TYPE = 0x0008,

    /* Argument type for request message for SET_RULES service. Also used as an
     * argument type for response message for GET_RULES. */
    FM_HOST_SRV_FLOW_ENTRY_TYPE = 0x0009,

    /* Argument type for request message for DEL_RULES service.
     * Used with fm_hostSrvFlowHandle argument. */
    FM_HOST_SRV_FLOW_HANDLE_TYPE = 0x000a,

    /* Argument type for response message for GET_RULES service. */
    FM_HOST_SRV_FLOW_RANGE_TYPE = 0x000b,

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

    /* Argument type for response message for TX_TIMESTAMP_MODE
     * service. Used with fm_hostSrvTimestampModeResp argument. */
    FM_HOST_SRV_SET_TIMESTAMP_MODE_RESP_TYPE = 0x0012,

    /* Argument type for request message for FILTER_INNER_OUTER_MAC service.
     * Used with fm_hostSrvInnOutMac argument. */
    FM_HOST_SRV_INN_OUT_MAC_TYPE = 0x0013,

    /* Argument type for request message for MASTER_CLK_OFFSET
     * service. Used with fm_hostSrvMasterClkOffset argument. */
    FM_HOST_SRV_MASTER_CLK_OFFSET_TYPE = 0x0014,

    /* Argument type for request message for SET_NO_OF_VFS service. */
    FM_HOST_SRV_NO_OF_VFS_TYPE = 0x00015,

    /* Argument type for request message for GET_TABLES service. */
    FM_HOST_SRV_FLOW_TABLE_RANGE_TYPE = 0x0016,

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

/* Mailbox tunnel type modes for FILTER_INNER_OUTER_MAC request. */
typedef enum _fm_filterMacTunnelType
{
    /* VXLAN Virtual Network tunnel type */
    FM_MAILBOX_TUNNEL_TYPE_VXLAN,

    /* NVGRE Virtual Network tunnel type */
    FM_MAILBOX_TUNNEL_TYPE_NVGRE,

    /* GENEVE Virtual Network tunnel type */
    FM_MAILBOX_TUNNEL_TYPE_GENEVE,

    /** UNPUBLISHED: For internal use only. */
    FM_MAILBOX_TUNNEL_TYPE_MAX       /* Must be last */

} fm_filterMacTunnelType;

/* Mailbox table type modes for CREATE_TABLE and GET_TABLES requests. */
typedef enum _fm_mailboxTableType
{
    /* FFU TCAM flow table. */
    FM_MBX_TCAM_TABLE = 1,

    /* Tunnel Engine 0 flow table. */
    FM_MBX_TE_A_TABLE = 2,

    /* Tunnel Engine 1 flow table. */
    FM_MBX_TE_B_TABLE = 3

} fm_mailboxTableType;

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


/* Mailbox structure used as an argument for CREATE_TABLE message. */
typedef struct _fm_hostSrvCreateTable
{
    /* Match table index provided by driver. Note, that this in NOT the same
     * index as the one using when creating flow table.
     */
    fm_uint32 matchTableIndex;

    /* Flow table type. Supported types are:
     *
     * - FM_MBX_TCAM_TABLE
     * - FM_MBX_TE_A_TABLE
     * - FM_MBX_TE_B_TABLE. */
    fm_uint32 tableType;

    /* Number of actions to be supported. */
    fm_byte numOfAct;

    /* Flags field. */
    fm_byte flags;

    /* Max number of entries. */
    fm_uint32 numOfEntr;

    /* Condition bitmask. */
    fm_flowCondition condition;

    /* Action bitmask. */
    fm_flowAction action;

} fm_hostSrvCreateTable;

/* Mailbox structure used as an argument for DESTROY_TABLE message. */
typedef struct _fm_hostSrvTable
{
    /* Match table index provided by driver. Note, that this in NOT the same
     * index as the one using when creating flow table.
     */
    fm_uint32 matchTableIndex;

} fm_hostSrvTable;

/* Mailbox structure used as an argument for GET_TABLES message. */
typedef struct _fm_hostSrvFlowTableRange
{
    /* First flow table index to be returned. */
    fm_uint32 firstFlowTableIndex;

    /* Last flow table index to be returned. */
    fm_uint32 lastFlowTableIndex;

} fm_hostSrvFlowTableRange;

/* Mailbox structure used as an argument for SET_RULES and GET_RULES messages.*/
typedef struct _fm_hostSrvFlowEntry
{
    /* Match flow table index provided by driver. */
    fm_uint32 matchTableIndex;

    /* Match flow ID provided by driver. Note, that this in NOT the same
     * ID as the one returned when adding a flow.
     */
    fm_uint32 matchFlowId;

    /* Flow priority field. */
    fm_uint32 priority;

    /* Glort value indicating who request flow (PF/VF). */
    fm_uint16 glort;

    /* Flow condition mask */
    fm_flowCondition condition;

    /* Flow action mask. */
    fm_flowAction action;

    /* Flow values set according to read arguments. */
    fm_flowValue flowVal;

    /* Flow params set according to read arguments. */
    fm_flowParam flowParam;

} fm_hostSrvFlowEntry;

/* Mailbox structure used as an argument for DEL_RULES message. */
typedef struct _fm_hostSrvFlowHandle
{
    /* Match flow table index provided by driver. */
    fm_uint32 matchTableIndex;

    /* Match flow ID provided by driver. */
    fm_uint32 matchFlowId;

} fm_hostSrvFlowHandle;

/* Mailbox structure used as an argument for GET_RULES message. */
typedef struct _fm_hostSrvFlowRange
{
    /* Match table index provided by driver. Note, that this in NOT the same
     * index as the one using when creating flow table.
     */
    fm_uint32 matchTableIndex;

    /* First flow ID to be returned. */
    fm_uint32 firstFlowId;

    /* Last flow ID to be returned. */
    fm_uint32 lastFlowId;

} fm_hostSrvFlowRange;

/* Mailbox structure used as an argument for SET_NO_OF_VFS message. */
typedef struct _fm_hostSrvNoOfVfs
{
    /* Glort value indicating PF. */
    fm_uint16 glort;

    /* Number of VFs. */
    fm_uint16 noOfVfs;

} fm_hostSrvNoOfVfs;

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

/* Mailbox structure used as a return type for 
 * TX_TIMESTAMP_MODE message. */
typedef struct _fm_hostSrvTimestampModeResp
{
    /* Glort value. */
    fm_uint16 glort;

    /* Is TX_TIMESTAMP_MODE enabled. */
    fm_bool modeEnabled;

} fm_hostSrvTimestampModeResp;

/* Mailbox structure used as an argument and return type for 
 * MASTER_CLK_OFFSET message. */
typedef struct _fm_hostSrvMasterClkOffset
{
    /* Lower bytes of offset value. */
    fm_uint32 offsetValueLower;

    /* Upper bytes of offset value. */
    fm_uint32 offsetValueUpper;

} fm_hostSrvMasterClkOffset;

/* Mailbox structure used as an argument for 
 * FILTER_INNER_OUTER_MAC message. */
typedef struct _fm_hostSrvInnOutMac
{
    /* Outer MAC address to add */
    fm_macaddr outerMacAddr;

    /* Outer MAC address mask to add */
    fm_macaddr outerMacAddrMask;

    /* Inner MAC address to add */
    fm_macaddr innerMacAddr;

    /* Inner Mac address mask to add */
    fm_macaddr innerMacAddrMask;

    /* Virtual Network Identifier */
    fm_uint32 vni;

    /* Outer L4 Port */
    fm_uint16 outerL4Port;

    /* Tunnel type */
    fm_filterMacTunnelType tunnelType;

    /* Action field. Current action definitions are as follows:
     *
     * +--------------+
     * |    ADD (0)   |
     * +--------------+
     * |   DELETE (1) |
     * +--------------+
     */
    fm_byte action;

    /* Glort value */
    fm_uint16 glort;

    /* ACL number */
    fm_int acl;

    /* ACL rule number */
    fm_int aclRule;

} fm_hostSrvInnOutMac;

/* Tracks resources created on Host Interface requests via mailbox
 * for a given virtual port. */
typedef struct _fm_mailboxResources
{
    /* A tree holding MAC addresses. 
     * Entries would include {key = MAC_VLAN, value = NULL} pairs. */
    fm_tree mailboxMacResource;

    /* A tree holding flow tables. This tree would be used for PF glorts.
     * Entries would include
     * {key = MATCH_FLOW_TABLE_INDEX, value = fm_mailboxFlowTable} pairs.
     */
    fm_tree mailboxFlowTableResource;

    /* A tree holding internal flow tables and IDs for VF glorts as keys.
     * Values are match flow table indexes to proper cleanup both VF and PF
     * trees when deleting VF rule.
     * Entries would include
     * {key == FLOWID_FLOWTABLE, value == MATCH_FLOW_TABLE} pairs
    .*/
    fm_tree mailboxFlowMap;

    /* A tree holding inner + outer MAC filtering structures. */
    fm_customTree innOutMacResource;

    /* MAC entries counter per virtual port. */
    fm_int macEntriesAdded;

    /* Inner/Outer Mac filtering entries counter per virtual port. */
    fm_int innerOuterMacEntriesAdded;

    /* Flow entries counter per virtual port. */
    fm_int flowEntriesAdded;

    /* Number of VFs. This field will be used only for PFs. */
    fm_int noOfVfs;

} fm_mailboxResources;

/* Structure to track mcast MACs and VNIs used for FILTER_INNER_OUTER_MAC
 * message. Each mcastMac_VNI pair would be connected to mcast group. */
typedef struct _fm_mailboxMcastMacVni
{
    /* Mcast MAC address */
    fm_macaddr macAddr;

    /* Virtual network identifier */
    fm_uint32  vni;

    /* Multicast group number created for mcast inner mac filtering */
    fm_int mcastGroup;

} fm_mailboxMcastMacVni;

/* Structure to track flow tables created on match interface demand. */
typedef struct _fm_mailboxFlowTable
{
    /* Internal flow table index. */
    fm_int tableIndex;

    /* Match flow table index. */
    fm_uint32 matchTableIndex;

    /* Action bitmask. */
    fm_flowAction action;

    /* Flags bitmask. */
    fm_byte flags;

    /* A tree mapping match flow IDs and internal ones within flow table.
     * Entries would include
     * {key = MATCH_FLOW_ID, value = (key of INTERNAL_FLOW_ID and GLORT)}
     * pairs.
     */
    fm_tree matchFlowIdMap;

    /* A tree reverse mapping than previous one.
     * Entries would include {key =INTERNAL_FLOW_ID, value = MATCH_FLOW_ID}
     * pairs.
     */
    fm_tree internalFlowIdMap;

} fm_mailboxFlowTable;

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

    /* Indicates if switch configuration is set for filtering 
     * INNER/OUTER MAC addresses. */
    fm_bool innerOuterMacConfigurationSet;

    /* Id of ACL created for inner/outer mac filtering purpose. */
    fm_int aclIdForMacFiltering;

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

    /* Indicates which acl rule are currently used or free. */
    fm_bitArray innOutMacRuleInUse;

    /* Maximum number MAC entries that can be added on driver demand per PEP.*/
    fm_int maxMacEntriesToAddPerPep;

    /* Maximum number Inner/Outer Mac filtering entries
     * that can be added on driver demand per PEP.*/
    fm_int maxInnerOuterMacEntriesToAddPerPep;

    /* Maximum numer MAC entries that can be added on driver demand 
     * per virtual port. */
    fm_int maxMacEntriesToAddPerPort;

    /* Maximum number Inner/Outer Mac filtering entries
     * that can be added on driver demand per virtual port. */
    fm_int maxInnerOuterMacEntriesToAddPerPort;

    /* Maximum number of flow entries
     * that can be added on driver demand per VF. */
    fm_int maxFlowEntriesToAddPerVf;

    /* MAC entries counter per PEP. */
    fm_int *macEntriesAdded;

    /* Inner/Outer Mac filtering entries counter per PEP */
    fm_int *innerOuterMacEntriesAdded;

    /* A tree tracking mcast MAC addresses and VNIs needed for 
     * FILTER_INNER_OUTER_MAC message. This will be used for inner mcast MACs. */
    fm_customTree mcastMacVni;

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

fm_status fmCreateTableProcess(fm_int                   sw,
                               fm_int                   pepNb,
                               fm_mailboxControlHeader *ctrlHdr,
                               fm_mailboxMessageHeader *pfTrHdr);

fm_status fmDestroyTableProcess(fm_int                   sw,
                                fm_int                   pepNb,
                                fm_mailboxControlHeader *ctrlHdr,
                                fm_mailboxMessageHeader *pfTrHdr);

fm_status fmGetTablesProcess(fm_int                   sw,
                             fm_int                   pepNb,
                             fm_mailboxControlHeader *ctrlHdr,
                             fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetRulesProcess(fm_int                   sw,
                            fm_int                   pepNb,
                            fm_mailboxControlHeader *ctrlHdr,
                            fm_mailboxMessageHeader *pfTrHdr);

fm_status fmGetRulesProcess(fm_int                   sw,
                            fm_int                   pepNb,
                            fm_mailboxControlHeader *ctrlHdr,
                            fm_mailboxMessageHeader *pfTrHdr);

fm_status fmDelRulesProcess(fm_int                   sw,
                            fm_int                   pepNb,
                            fm_mailboxControlHeader *ctrlHdr,
                            fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetNoOfVfsProcess(fm_int                   sw,
                              fm_int                   pepNb,
                              fm_mailboxControlHeader *ctrlHdr,
                              fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetTimestampModeProcess(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *ctrlHdr,
                                    fm_mailboxMessageHeader *pfTrHdr);

fm_status fmFilterInnerOuterMacProcess(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr,
                                       fm_mailboxMessageHeader *pfTrHdr);

fm_status fmSetInnerOuterMacFilter(fm_int               sw,
                                   fm_int               pepNb,
                                   fm_hostSrvInnOutMac *macFilter);

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

fm_status fmAnnounceTxTimestampMode(fm_int  sw,
                                    fm_bool isTxTimestampEnabled);

fm_status fmMasterClkOffsetProcess(fm_int                   sw,
                                   fm_int                   pepNb,
                                   fm_mailboxControlHeader *ctrlHdr,
                                   fm_mailboxMessageHeader *pfTrHdr);

void fmFreeMailboxResources(void *value);

void fmFreeMcastMacVni(void *key, void *value);

void fmFreeSrvInnOutMac(void *key, void *value);

fm_status fmSetMgmtPepXcastModes(fm_int sw, 
                                 fm_int xCastLogPort, 
                                 fm_bool addToMcastGroup);

#endif /* __FM_FM_API_MAILBOX_INT_H */

