/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_config_tlv.h
 * Creation Date:   May 15, 2015
 * Description:     Functions to encode/decode API/platform
 *                  properties to TLVs.
 *
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __FM_UTIL_CONFIG_TLV_H
#define __FM_UTIL_CONFIG_TLV_H

#define FM_TLV_MAX_BUF_SIZE                         (256 + 3)

/* API properties */
#define FM_TLV_API_EBI_CLOCK                        0x0001
#define FM_TLV_API_FH_REF_CLK                       0x0002
#define FM_TLV_API_STP_DEF_VLAN_MEMBER              0x0003
#define FM_TLV_API_DIR_SEND_TO_CPU                  0x0004
#define FM_TLV_API_STP_DEF_VLAN_NON_MEMBER          0x0005
#define FM_TLV_API_IDENTIFY_SWITCH                  0x0006
#define FM_TLV_API_BOOT_RESET                       0x0007
#define FM_TLV_API_AUTO_INSERT_SW                   0x0008
#define FM_TLV_API_DEV_RESET_TIME                   0x0009
#define FM_TLV_API_SWAG_EN_LINK                     0x000a
#define FM_TLV_API_EVENT_BLK_THRESHOLD              0x000b
#define FM_TLV_API_EVENT_UNBLK_THRESHOLD            0x000c
#define FM_TLV_API_EVENT_SEM_TIMEOUT                0x000d
#define FM_TLV_API_RX_DIRECTED_ENQ                  0x000e
#define FM_TLV_API_RX_DRV_DEST                      0x000f
#if 0
#define FM_TLV_API_BOOT_PORT_RESET_MASK             0x0010
#endif
#define FM_TLV_API_LAG_ASYNC_DEL                    0x0011
#define FM_TLV_API_EVENT_ON_STATIC_ADDR             0x0012
#define FM_TLV_API_EVENT_ON_DYN_ADDR                0x0013
#define FM_TLV_API_FLUSH_ON_PORT_DOWN               0x0014
#define FM_TLV_API_FLUSH_ON_VLAN_CHG                0x0015
#define FM_TLV_API_FLUSH_ON_LAG_CHG                 0x0016
#define FM_TLV_API_TCN_FIFO_BURST_SIZE              0x0017
#define FM_TLV_API_SWAG_INT_VLAN_STATS              0x0018
#define FM_TLV_API_PER_LAG_MGMT                     0x0019
/* 0x1a..0x1c deleted */
#define FM_TLV_API_PARITY_REPAIR_EN                 0x001d
#define FM_TLV_API_SWAG_MAX_ACL_PORT_SET            0x001e
#define FM_TLV_API_MAX_PORT_SETS                    0x001f
#define FM_TLV_API_PKT_RX_EN                        0x0020
#define FM_TLV_API_MC_SINGLE_ADDR                   0x0021
#define FM_TLV_API_PLAT_MODEL_POS                   0x0022
#define FM_TLV_API_VN_NUM_NH                        0x0023
#define FM_TLV_API_VN_ENCAP_PROTOCOL                0x0024
#define FM_TLV_API_VN_ENCAP_VER                     0x0025
#define FM_TLV_API_SUP_ROUTE_LOOKUP                 0x0026
#define FM_TLV_API_RT_MAINT_EN                      0x0027
#define FM_TLV_API_AUTO_VLAN2_TAG                   0x0028
#define FM_TLV_API_EVENT_ON_ADDR_CHG                0x0029


/* Undocumented API properties */
#define FM_TLV_API_INTR_HANDLER_DIS                 0x1000
#define FM_TLV_API_MA_TABLE_MAINT_EN                0x1001
#define FM_TLV_API_FAST_MAINT_EN                    0x1002
#define FM_TLV_API_FAST_MAINT_PER                   0x1003  
#if 0
#define FM_TLV_API_VEGAS_UPLNK_FIRST                0x1004
#endif
#define FM_TLV_API_STRICT_GLORT                     0x1005
#define FM_TLV_API_AUTO_RESET_WM                    0x1006
#define FM_TLV_API_AUTO_SUB_SW                      0x1007
#define FM_TLV_API_SWAG_AUTO_INT_PORT               0x1008
#define FM_TLV_API_SWAG_AUTO_NVVSI                  0x1009
#define FM_TLV_API_FIBM_TEST_RIG                    0x100a
#define FM_TLV_API_FIBM_TEST_PORT                   0x100b
#define FM_TLV_API_PORT_REMAP_TABLE                 0x100c
#define FM_TLV_API_PLAT_BYPASS_EN                   0x100e
#define FM_TLV_API_LAG_DEL_SEM_TIMEOUT              0x100f
#define FM_TLV_API_STP_EN_INT_PORT_CTRL             0x1010
#define FM_TLV_API_GEN_EEPROM_MODE                  0x1011
#define FM_TLV_API_MODEL_PORT_MAP_TYPE              0x1012
#define FM_TLV_API_MODEL_SW_TYPE                    0x1013
#define FM_TLV_API_MODEL_TX_EOT                     0x1014
#define FM_TLV_API_MODEL_LOG_EGR_INFO               0x1015
#define FM_TLV_API_PLAT_EN_REF_CLK                  0x1016
#define FM_TLV_API_PLAT_SET_REF_CLK                 0x1017
#define FM_TLV_API_PLAT_I2C_DEVNAME                 0x1018
#define FM_TLV_API_PLAT_PRI_BUF_Q                   0x1019
#define FM_TLV_API_PLAT_PKT_SCHED_TYPE              0x101a
#define FM_TLV_API_PLAT_SEP_BUF_POOL                0x101b
#define FM_TLV_API_PLAT_NUM_BUF_RX                  0x101c
#define FM_TLV_API_PLAT_NUM_BUF_TX                  0x101d
#define FM_TLV_API_PLAT_MODEL_PKT_INTF              0x101e
#define FM_TLV_API_PLAT_PKT_INTF                    0x101f
#define FM_TLV_API_PLAT_MODEL_TOPO                  0x1020
#define FM_TLV_API_PLAT_MODEL_USE_PATH              0x1021
#define FM_TLV_API_PLAT_DEV_BOARD_IP                0x1022
#define FM_TLV_API_PLAT_DEV_BOARD_PORT              0x1023
#define FM_TLV_API_PLAT_MODEL_DEVICE_CFG            0x1024
#define FM_TLV_API_PLAT_SBUS_SERVER_PORT            0x1025
#define FM_TLV_API_PLAT_IS_WHITE_MODEL              0x1026
#define FM_TLV_API_PLAT_TSW_OFF                     0x1027
#define FM_TLV_API_PLAT_LED_BLINK_EN                0x1028
#define FM_TLV_API_DBG_INT_LOG_CAT                  0x1029
#define FM_TLV_API_ADD_PEP_FLOOD                    0x102a
#define FM_TLV_API_IGNORE_BW_VIOLATION              0x102b
#define FM_TLV_API_IGNORE_BW_VIOLATION_NW           0x102c
#define FM_TLV_API_DFE_EARLY_LNK_UP                 0x102d
#define FM_TLV_API_DFE_ALLOW_KR_PCAL                0x102e
#define FM_TLV_API_GSME_TS_MODE                     0x102f
#define FM_TLV_API_MC_HNI_FLOODING                  0x1030
#define FM_TLV_API_HNI_MAC_ENTRIES_PER_PEP          0x1031
#define FM_TLV_API_HNI_MAC_ENTRIES_PER_PORT         0x1032
#define FM_TLV_API_HNI_INN_OUT_ENTRIES_PER_PEP      0x1033
#define FM_TLV_API_HNI_INN_OUT_ENTRIES_PER_PORT     0x1034
#define FM_TLV_API_PORT_ENABLE_STATUS_POLLING       0x1035
#define FM_TLV_API_DFE_ENABLE_SIGNALOK_DEBOUNCING   0x1036
#define FM_TLV_API_PLAT_MODEL_CHIP_VERSION          0x1037
#define FM_TLV_API_ADD_ALLOW_FTAG_VLAN_TAG          0x1038
#define FM_TLV_API_AN_TIMER_ALLOW_OUT_SPEC          0x1039

/* FM10K properties */
#define FM_TLV_FM10K_WMSELECT                       0x2000
#define FM_TLV_FM10K_CM_RX_SMP_PRIVATE_BYTES        0x2001
#define FM_TLV_FM10K_CM_TX_TC_HOG_BYTES             0x2002
#define FM_TLV_FM10K_CM_SMP_SD_VS_HOG_PERCENT       0x2003
#define FM_TLV_FM10K_CM_SMP_SD_JITTER_BITS          0x2004
#define FM_TLV_FM10K_CM_TX_SD_ON_PRIVATE            0x2005
#define FM_TLV_FM10K_CM_TX_SD_ON_SMP_FREE           0x2006
#define FM_TLV_FM10K_CM_PAUSE_BUFFER_BYTES          0x2007
#define FM_TLV_FM10K_MCAST_MAX_ENTRIES_PER_CAM      0x2008
#define FM_TLV_FM10K_FFU_UNICAST_SLICE_1ST          0x2009
#define FM_TLV_FM10K_FFU_UNICAST_SLICE_LAST         0x200a
#define FM_TLV_FM10K_FFU_MULTICAST_SLICE_1ST        0x200b
#define FM_TLV_FM10K_FFU_MULTICAST_SLICE_LAST       0x200c
#define FM_TLV_FM10K_FFU_ACL_SLICE_1ST              0x200d
#define FM_TLV_FM10K_FFU_ACL_SLICE_LAST             0x200e
#define FM_TLV_FM10K_FFU_MAPMAC_ROUTING             0x200f
#define FM_TLV_FM10K_FFU_UNICAST_PRECEDENCE_MIN     0x2010
#define FM_TLV_FM10K_FFU_UNICAST_PRECEDENCE_MAX     0x2011
#define FM_TLV_FM10K_FFU_MULTICAST_PRECEDENCE_MIN   0x2012
#define FM_TLV_FM10K_FFU_MULTICAST_PRECEDENCE_MAX   0x2013
#define FM_TLV_FM10K_FFU_ACL_PRECEDENCE_MIN         0x2014
#define FM_TLV_FM10K_FFU_ACL_PRECEDENCE_MAX         0x2015
#define FM_TLV_FM10K_FFU_ACL_STRICT_COUNT_POLICE    0x2016
#define FM_TLV_FM10K_INIT_UCAST_FLOODING_TRIGGERS   0x2017
#define FM_TLV_FM10K_INIT_MCAST_FLOODING_TRIGGERS   0x2018
#define FM_TLV_FM10K_INIT_BCAST_FLOODING_TRIGGERS   0x2019
#define FM_TLV_FM10K_FLOODING_TRAP_PRIORITY         0x201a
#define FM_TLV_FM10K_AUTONEG_GENERATE_EVENTS        0x201b
#define FM_TLV_FM10K_LINK_DEPENDS_ON_DFE            0x201c
#define FM_TLV_FM10K_VN_USE_SHARED_ENCAP_FLOWS      0x201d
#define FM_TLV_FM10K_VN_MAX_TUNNEL_RULES            0x201e
#define FM_TLV_FM10K_VN_TUNNEL_GROUP_HASH_SIZE      0x201f
#define FM_TLV_FM10K_VN_TE_VID                      0x2020
#define FM_TLV_FM10K_VN_ENCAP_ACL_NUM               0x2021
#define FM_TLV_FM10K_VN_DECAP_ACL_NUM               0x2022
#define FM_TLV_FM10K_MCAST_NUM_STACK_GROUPS         0x2023
#define FM_TLV_FM10K_VN_TUNNEL_ONLY_IN_INGRESS      0x2024
#define FM_TLV_FM10K_MTABLE_CLEANUP_WATERMARK       0x2025
#define FM_TLV_FM10K_SCHED_MODE                     0x2026
#define FM_TLV_FM10K_UPD_SCHED_ON_LNK_CHANGE        0x2027
#define FM_TLV_FM10K_PARITY_CRM_TIMEOUT             0x2028
#define FM_TLV_FM10K_INIT_RESERVED_MAC_TRIGGERS     0x2029


/* Undocumented FM10K properties  */
#define FM_TLV_FM10K_CREATE_REMOTE_LOGICAL_PORTS    0x2800
#define FM_TLV_FM10K_AUTONEG_CLAUSE_37_TIMEOUT      0x2801
#define FM_TLV_FM10K_AUTONEG_SGMII_TIMEOUT          0x2802
#define FM_TLV_FM10K_HNI_SERVICES_LOOPBACK          0x2803
#define FM_TLV_FM10K_ANTI_BUBBLE_WM                 0x2804
#define FM_TLV_FM10K_SERDES_OP_MODE                 0x2805
#define FM_TLV_FM10K_SERDES_DBG_LVL                 0x2806
#define FM_TLV_FM10K_PARITY_INTERRUPTS              0x2807
#define FM_TLV_FM10K_SCHED_OVERSPEED                0x280c
#define FM_TLV_FM10K_INTR_LINK_IGNORE_MASK          0x280d
#define FM_TLV_FM10K_INTR_AUTONEG_IGNORE_MASK       0x280e
#define FM_TLV_FM10K_INTR_SERDES_IGNORE_MASK        0x280f
#define FM_TLV_FM10K_INTR_PCIE_IGNORE_MASK          0x2810
#define FM_TLV_FM10K_INTR_MATCN_IGNORE_MASK         0x2811
#define FM_TLV_FM10K_INTR_FHTAIL_IGNORE_MASK        0x2812
#define FM_TLV_FM10K_INTR_SW_IGNORE_MASK            0x2813
#define FM_TLV_FM10K_INTR_TE_IGNORE_MASK            0x2814
#define FM_TLV_FM10K_PARITY_TCAM_MONITOR            0x2815
#define FM_TLV_FM10K_EEE_SPICO_INTR                 0x2816 
#define FM_TLV_FM10K_USE_ALTERNATE_SPICO_FW         0x2817 
#define FM_TLV_FM10K_ALLOW_KRPCAL_ON_EEE            0x2818 


/* Liberty Trail platform properties */
#define FM_TLV_PLAT_NUM_SW                          0x3000
#define FM_TLV_PLAT_NAME                            0x3001
#define FM_TLV_PLAT_FILE_LOCK_NAME                  0x3002
#define FM_TLV_PLAT_SW_NUMBER                       0x3003
#define FM_TLV_PLAT_SW_NUM_PORTS                    0x3004
#define FM_TLV_PLAT_SW_LED_POLL_PER                 0x3005
#define FM_TLV_PLAT_SW_LED_BLINK_MODE               0x3006
#define FM_TLV_PLAT_SW_XCVR_POLL_PER                0x3007
#define FM_TLV_PLAT_SW_PORT_MAPPING                 0x3008
#define FM_TLV_PLAT_SW_PORT_LANE_MAPPING            0x3009
#define FM_TLV_PLAT_SW_INT_PORT_MAPPING             0x300a
#define FM_TLV_PLAT_SW_TOPOLOGY                     0x300b
#define FM_TLV_PLAT_SW_ROLE                         0x300c
#define FM_TLV_PLAT_SW_PORT_DEF_HW_ID               0x300d
#define FM_TLV_PLAT_SW_PORTIDX_HW_ID                0x300e
#define FM_TLV_PLAT_SW_PORT_DEF_ETHMODE             0x300f
#define FM_TLV_PLAT_SW_PORTIDX_ETHMODE              0x3010
#define FM_TLV_PLAT_SW_PORTIDX_SPEED                0x3011
#define FM_TLV_PLAT_SW_PORT_DEF_LANE_POL            0x3012
#define FM_TLV_PLAT_SW_PORTIDX_LANE_ALL_POL         0x3013
#define FM_TLV_PLAT_SW_PORTIDX_LANE_POL             0x3014

#define FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_CU         0x3015
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_1G       0x3016
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_10G      0x3017
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_25G      0x3018
#define FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_OPT        0x3019
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_1G      0x301a
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_10G     0x301b
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_25G     0x301c

#define FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_CU         0x301d
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_1G       0x301e
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_10G      0x301f
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_25G      0x3020
#define FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_OPT        0x3021
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_1G      0x3022
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_10G     0x3023
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_25G     0x3024

#define FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_CU        0x3025
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_1G      0x3026
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_10G     0x3027
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_25G     0x3028
#define FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_OPT       0x3029
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_1G     0x302a
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_10G    0x302b
#define FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_25G    0x302c

#define FM_TLV_PLAT_SW_PORT_DEF_INTF_TYPE           0x302d
#define FM_TLV_PLAT_SW_PORTIDX_INTF_TYPE            0x302e
#define FM_TLV_PLAT_SW_PORT_DEF_CAP                 0x302f
#define FM_TLV_PLAT_SW_PORTIDX_CAP                  0x3030
#define FM_TLV_PLAT_SW_PORT_DEF_DFE_MODE            0x3031
#define FM_TLV_PLAT_SW_PORTIDX_DFE_MODE             0x3032
#define FM_TLV_PLAT_SW_PORTIDX_AN73_ABILITY         0x3033
#define FM_TLV_PLAT_SW_SHARED_LIB_NAME              0x3034
#define FM_TLV_PLAT_SW_SHARED_LIB_DISABLE           0x3035
#define FM_TLV_PLAT_SW_PORT_INTR_GPIO               0x3036
#define FM_TLV_PLAT_SW_I2C_RESET_GPIO               0x3037
#define FM_TLV_PLAT_SW_FLASH_WP_GPIO                0x3038
#define FM_TLV_PLAT_SW_KEEP_SERDES_CFG              0x3039
#define FM_TLV_PLAT_SW_NUM_PHYS                     0x303a
#define FM_TLV_PLAT_SW_PHY_MODEL                    0x303b
#define FM_TLV_PLAT_SW_PHY_ADDR                     0x303c
#define FM_TLV_PLAT_SW_PHY_HW_ID                    0x303d
#define FM_TLV_PLAT_SW_PHY_LANE_TXEQ                0x303e
#define FM_TLV_PLAT_SW_VRM_HW_ID                    0x303f
#define FM_TLV_PLAT_SW_MSI_ENABLE                   0x3040
#define FM_TLV_PLAT_SW_FH_CLOCK                     0x3041
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_1G       0x3042
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_10G      0x3043
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_25G      0x3044
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_1G      0x3045
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_10G     0x3046
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_25G     0x3047
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_1G       0x3048
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_10G      0x3049
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_25G      0x304a
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_1G      0x304b
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_10G     0x304c
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_25G     0x304d
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_1G      0x304e
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_10G     0x304f
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_25G     0x3050
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_1G     0x3051
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_10G    0x3052
#define FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_25G    0x3053
#define FM_TLV_PLAT_SW_PHY_APP_MODE                 0x3054
#define FM_TLV_PLAT_SW_VRM_USE_DEF_VOLTAGE          0x3055
#define FM_TLV_PLAT_SW_VDDS_USE_HW_RESOURCE_ID      0x3056
#define FM_TLV_PLAT_SW_VDDF_USE_HW_RESOURCE_ID      0x3057
#define FM_TLV_PLAT_SW_AVDD_USE_HW_RESOURCE_ID      0x3058
#define FM_TLV_PLAT_SW_PORTIDX_LANE_ALL_RX_TERM     0x3059
#define FM_TLV_PLAT_SW_PORTIDX_LANE_RX_TERM         0x305a
#define FM_TLV_PLAT_SW_I2C_CLKDIVIDER               0x305b

/* Undocumented Liberty Trail platform properties */
#define FM_TLV_PLAT_EBI_DEVNAME                     0x4000
#define FM_TLV_PLAT_DEV_MEM_OFF                     0x4001
#define FM_TLV_PLAT_NET_DEVNAME                     0x4002
#define FM_TLV_PLAT_UIO_DEVNAME                     0x4003
#define FM_TLV_PLAT_INTR_TIMEOUT_CNT                0x4004
#define FM_TLV_PLAT_DEBUG                           0x4005
#define FM_TLV_PLAT_BOOT_MODE                       0x4006
#define FM_TLV_PLAT_REG_ACCESS                      0x4007
#define FM_TLV_PLAT_PCIE_ISR                        0x4008
#define FM_TLV_PLAT_CPU_PORT                        0x4009
#define FM_TLV_PLAT_INTR_POLL_PER                   0x400a
#define FM_TLV_PLAT_PHY_EN_DEEMPHASIS               0x400b


/* Shared library properties */
#define FM_TLV_PLAT_LIB_I2C_DEVNAME                 0x5000
#define FM_TLV_PLAT_LIB_MUX_COUNT                   0x5001
#define FM_TLV_PLAT_LIB_MUX_MODEL                   0x5002
#define FM_TLV_PLAT_LIB_MUX_BUS                     0x5003
#define FM_TLV_PLAT_LIB_MUX_ADDR                    0x5004
#define FM_TLV_PLAT_LIB_MUX_PARENT_IDX              0x5005
#define FM_TLV_PLAT_LIB_MUX_PARENT_VAL              0x5006
#define FM_TLV_PLAT_LIB_IO_COUNT                    0x5007
#define FM_TLV_PLAT_LIB_IO_MODEL                    0x5008
#define FM_TLV_PLAT_LIB_IO_BUS                      0x5009
#define FM_TLV_PLAT_LIB_IO_ADDR                     0x500a
#define FM_TLV_PLAT_LIB_IO_PARENT_IDX               0x500b
#define FM_TLV_PLAT_LIB_IO_PARENT_VAL               0x500c
#define FM_TLV_PLAT_LIB_IO_LED_BLINK_PER            0x500d
#define FM_TLV_PLAT_LIB_IO_LED_BRIGHTNESS           0x500e

#define FM_TLV_PLAT_LIB_XCVR_DEF_MODABS_PIN         0x500f
#define FM_TLV_PLAT_LIB_XCVR_DEF_RXLOS_PIN          0x5010
#define FM_TLV_PLAT_LIB_XCVR_DEF_TXDISABLE_PIN      0x5011
#define FM_TLV_PLAT_LIB_XCVR_DEF_TXFAULT_PIN        0x5012
#define FM_TLV_PLAT_LIB_XCVR_DEF_MODPRESL_PIN       0x5013
#define FM_TLV_PLAT_LIB_XCVR_DEF_INTL_PIN           0x5014
#define FM_TLV_PLAT_LIB_XCVR_DEF_LPMODE_PIN         0x5015
#define FM_TLV_PLAT_LIB_XCVR_DEF_RESETL_PIN         0x5016

#define FM_TLV_PLAT_LIB_HWRESID_COUNT               0x5017
#define FM_TLV_PLAT_LIB_HWRESID_INTF_TYPE           0x5018
#define FM_TLV_PLAT_LIB_XCVR_IDX                    0x5019
#define FM_TLV_PLAT_LIB_XCVR_BASE_PIN               0x501a
#define FM_TLV_PLAT_LIB_XCVR_MODABS_PIN             0x501b
#define FM_TLV_PLAT_LIB_XCVR_RXLOS_PIN              0x501c
#define FM_TLV_PLAT_LIB_XCVR_TXDISABLE_PIN          0x501d
#define FM_TLV_PLAT_LIB_XCVR_TXFAULT_PIN            0x501e
#define FM_TLV_PLAT_LIB_XCVR_MODPRESL_PIN           0x501f
#define FM_TLV_PLAT_LIB_XCVR_INTL_PIN               0x5020
#define FM_TLV_PLAT_LIB_XCVR_LPMODE_PIN             0x5021
#define FM_TLV_PLAT_LIB_XCVR_RESETL_PIN             0x5022

#define FM_TLV_PLAT_LIB_XCVR_I2C_DEF_BUSSELTYPE     0x5023
#define FM_TLV_PLAT_LIB_XCVR_I2_BUSSELTYPE          0x5024
#define FM_TLV_PLAT_LIB_XCVR_MUX_IDX                0x5025
#define FM_TLV_PLAT_LIB_XCVR_MUX_VAL                0x5026
#define FM_TLV_PLAT_LIB_PORTLED_TYPE                0x5027
#define FM_TLV_PLAT_LIB_PORTLED_IO_IDX              0x5028
#define FM_TLV_PLAT_LIB_PORTLED_IO_PIN              0x5029
#define FM_TLV_PLAT_LIB_PORTLED_IO_LANE_PIN         0x502a
#define FM_TLV_PLAT_LIB_PORTLED_IO_USAGE            0x502b
#define FM_TLV_PLAT_LIB_PORTLED_IO_LANE_USAGE       0x502c
#define FM_TLV_PLAT_LIB_DEBUG                       0x502d
#define FM_TLV_PLAT_LIB_HWRESID_TYPE                0x502e
#define FM_TLV_PLAT_LIB_PHY_BUSSELTYPE              0x502f
#define FM_TLV_PLAT_LIB_PHY_MUX_IDX                 0x5030
#define FM_TLV_PLAT_LIB_PHY_MUX_VAL                 0x5031
#define FM_TLV_PLAT_LIB_PHY_BUS                     0x5032
#define FM_TLV_PLAT_LIB_VRM_BUSSELTYPE              0x5033
#define FM_TLV_PLAT_LIB_VRM_MODEL                   0x5034
#define FM_TLV_PLAT_LIB_VRM_MUX_IDX                 0x5035
#define FM_TLV_PLAT_LIB_VRM_MUX_VAL                 0x5036
#define FM_TLV_PLAT_LIB_VRM_MUX_ADDR                0x5037
#define FM_TLV_PLAT_LIB_VRM_MUX_BUS                 0x5038
#define FM_TLV_PLAT_LIB_BUS_I2C_EN_WR_RD            0x5039

fm_status fmUtilConfigPropertyEncodeTlv(fm_text property,
                                        fm_byte *tlv,
                                        fm_int length);

fm_status fmUtilConfigPropertyDecodeTlv(fm_byte *tlv,
                                        fm_text propBuf,
                                        fm_int bufSize);

#endif /* __FM_UTIL_CONFIG_TLV_H */
