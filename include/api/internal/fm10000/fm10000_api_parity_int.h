/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_parity_int.h
 * Creation Date:   August 6, 2014
 * Description:     FM10000 parity error handling.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation.
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

#ifndef __FM_FM10000_API_PARITY_INT_H
#define __FM_FM10000_API_PARITY_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define GET_PARITY_INFO(sw) \
    &(((fm10000_switch *)GET_SWITCH_EXT(sw))->parityInfo)

#define GET_PARITY_LOCK(sw) \
    &(((fm10000_switch *)GET_SWITCH_EXT(sw))->parityLock)
                                    
#define TAKE_PARITY_LOCK(sw) \
    fmCaptureLock(GET_PARITY_LOCK(sw), FM_WAIT_FOREVER);

#define DROP_PARITY_LOCK(sw) \
    fmReleaseLock(GET_PARITY_LOCK(sw));


/******************************************
 * FM10000 SRAM identifiers. 
 *  
 * These symbols are used internally to 
 * specify an SRAM or group of SRAMs for
 * purposes of memory error injection and 
 * reporting.
 ******************************************/
enum
{
    /* Undefined value */
    FM10000_SRAM_UNDEF,

    /******************************************
     * FRAME and CROSSBAR memories.
     ******************************************/

    /* CORE_INTERRUPT_DETECT */
    FM10000_SRAM_CROSSBAR,

    /* SRAM_ERR_IP */
    FM10000_SRAM_FRAME_MEMORY,          /* 0..23 */

    /******************************************
     * FRAME HANDLER (HEAD) memories.
     ******************************************/

    /* ARP_SRAM_CTRL */
    FM10000_SRAM_ARP_TABLE,             /* 0..3 */
    FM10000_SRAM_ARP_USED,

    /* FFU_SRAM_CTRL */
    FM10000_SRAM_FFU_SLICE_SRAM,        /* 0..15 */

    /* FID_GLORT_LOOKUP_SRAM_CTRL */
    FM10000_SRAM_INGRESS_MST_TABLE,     /* 0..1 */
    FM10000_SRAM_EGRESS_MST_TABLE,

    /* FH_HEAD_OUTPUT_FIFO_SRAM_CTRL */
    FM10000_SRAM_FH_HEAD_FIFO,          /* 0..4 */

    /* GLORT_RAM_SRAM_CTRL */
    FM10000_SRAM_GLORT_RAM,

    /* GLORT_TABLE_SRAM_CTRL */
    FM10000_SRAM_GLORT_TABLE_SRAM,

    /* MAPPER_SRAM_CTRL */
    FM10000_SRAM_FFU_MAP_VLAN,

    /* MA_TABLE_SRAM_CTRL */
    FM10000_SRAM_MA_TABLE,              /* 0..7 */

    /* PARSER_EARLY_SRAM_CTRL */
    FM10000_SRAM_PARSER_PORT_CFG_2,

    /* PARSER_LATE_SRAM_CTRL */
    FM10000_SRAM_PARSER_PORT_CFG_3,
    FM10000_SRAM_RX_VPRI_MAP,

    /* VLAN_LOOKUP_SRAM_CTRL */
    FM10000_SRAM_INGRESS_VID_TABLE,     /* 0..1 */
    FM10000_SRAM_EGRESS_VID_TABLE,      /* 0..1 */

    /******************************************
     * FRAME HANDLER (TAIL) memories.
     ******************************************/

    /* EGRESS_PAUSE_SRAM_CTRL */
    FM10000_SRAM_CM_PAUSE_RCV_STATE,    /* 0..1 */

    /* POLICER_USAGE_SRAM_CTRL */
    FM10000_SRAM_POLICER_CFG_4K,        /* 0..1 */
    FM10000_SRAM_POLICER_CFG_512,       /* 0..1 */
    FM10000_SRAM_POLICER_STATE_4K,      /* 0..3 */
    FM10000_SRAM_POLICER_STATE_512,     /* 0..3 */

    /* RX_STATS_SRAM_CTRL */
    FM10000_SRAM_RX_STATS_BANK,         /* 0..11 */

    /* SAF_SRAM_CTRL */
    FM10000_SRAM_SAF_MATRIX,            /* 0..3 */

    /* TCN_SRAM_CTRL */
    FM10000_SRAM_MA_TCN_FIFO,

    /******************************************
     * MODIFY memories.
     ******************************************/

    /* MODIFY_IP */
    FM10000_SRAM_MOD_CM_TX_FIFO,
    FM10000_SRAM_MOD_CTRL_TO_DP_FIFO,   /* 0..3 */
    FM10000_SRAM_MOD_ESCHED_TX_FIFO,
    FM10000_SRAM_MOD_HEAD_STORAGE,
    FM10000_SRAM_MOD_MCAST_VLAN_TABLE,
    FM10000_SRAM_MOD_MGMT_RD_FIFO,
    FM10000_SRAM_MOD_MIRROR_PROFILE_TABLE,
    FM10000_SRAM_MOD_PER_PORT_CFG_1,
    FM10000_SRAM_MOD_PER_PORT_CFG_2,
    FM10000_SRAM_MOD_REFCOUNT,
    FM10000_SRAM_MOD_STATS_BANK_BYTE,   /* 0..1 */
    FM10000_SRAM_MOD_STATS_BANK_FRAME,  /* 0..1 */
    FM10000_SRAM_MOD_VID2_MAP,
    FM10000_SRAM_MOD_VPRI1_MAP,
    FM10000_SRAM_MOD_VPRI2_MAP,
    FM10000_SRAM_MOD_VLAN_TAG_VID1_MAP,

    /******************************************
     * SCHEDULER memories.
     ******************************************/

    /* SCHED_IP */
    FM10000_SRAM_SCHED_CONFIG,
    FM10000_SRAM_SCHED_ESCHED,
    FM10000_SRAM_SCHED_FIFO,
    FM10000_SRAM_SCHED_MONITOR,
    FM10000_SRAM_SCHED_RXQ,
    FM10000_SRAM_SCHED_SSCHED,
    FM10000_SRAM_SCHED_TXQ,

    /* SCHED_CONFIG_SRAM_CTRL */
    FM10000_SRAM_SCHED_RX_SCHEDULE,
    FM10000_SRAM_SCHED_TX_SCHEDULE,

    /* SCHED_ESCHED_SRAM_CTRL */
    FM10000_SRAM_SCHED_ESCHED_CFG_1,
    FM10000_SRAM_SCHED_ESCHED_CFG_2,
    FM10000_SRAM_SCHED_ESCHED_CFG_3,
    FM10000_SRAM_SCHED_ESCHED_DC,
    FM10000_SRAM_SCHED_ESCHED_UB,
    FM10000_SRAM_SCHED_ESCHED_PTR,
    FM10000_SRAM_SCHED_ESCHED_PAUSE,

    /* SCHED_FIFO_SRAM_CTRL */
    FM10000_SRAM_SCHED_FIFO_SCHEDULE_FC,
    FM10000_SRAM_SCHED_FIFO_RX_PORT,
    FM10000_SRAM_SCHED_FIFO_TX_PORT,
    FM10000_SRAM_SCHED_FIFO_TAIL_INFO,
    FM10000_SRAM_SCHED_FIFO_ARRAY_CTRL,
    FM10000_SRAM_SCHED_FIFO_MODIFY_CTRL,
    FM10000_SRAM_SCHED_FIFO_EGRESS_CTRL,

    /* SCHED_FREELIST_SRAM_CTRL */
    FM10000_SRAM_SCHED_FREELIST,

    /* SCHED_MONITOR_SRAM_CTRL */
    FM10000_SRAM_SCHED_MONITOR_DRR_Q_PERQ,
    FM10000_SRAM_SCHED_MONITOR_DRR_DC_PERQ,
    FM10000_SRAM_SCHED_MONITOR_DRR_CFG_PERPORT,
    FM10000_SRAM_SCHED_MONITOR_PEP_PERPORT,

    /* SCHED_RXQ_SRAM_CTRL */
    FM10000_SRAM_SCHED_RXQ_DATA,        /* 0..1 */
    FM10000_SRAM_SCHED_RXQ_LINK,
    FM10000_SRAM_SCHED_RXQ_FREELIST,
    FM10000_SRAM_SCHED_MCAST_DEST_TABLE,
    FM10000_SRAM_SCHED_MCAST_LEN_TABLE,
    FM10000_SRAM_SCHED_RXQ_OUTPUT_FIFO,

    /* SCHED_SSCHED_SRAM_CTRL */
    FM10000_SRAM_SCHED_SSCHED_RX_PERPORT,
    FM10000_SRAM_SCHED_SSCHED_TX_PERPORT,
    FM10000_SRAM_SCHED_SSCHED_LINK_STORAGE,
    FM10000_SRAM_SCHED_SSCHED_MODIFY_PF,

    /* SCHED_TXQ_SRAM_CTRL */
    FM10000_SRAM_SCHED_TXQ_HEAD_PERQ,
    FM10000_SRAM_SCHED_TXQ_PLINK_STATE,
    FM10000_SRAM_SCHED_TXQ_REP_PERQ,
    FM10000_SRAM_SCHED_TXQ_TAIL0_PERQ,
    FM10000_SRAM_SCHED_TXQ_TAIL1_PERQ,
    FM10000_SRAM_SCHED_TXQ_FREELIST,
    FM10000_SRAM_SCHED_TXQ_RXTIME,

    /******************************************
     * TUNNELING ENGINE memories.
     ******************************************/

    /* TE_SRAM_CTRL[0..1] */
    FM10000_SRAM_TE_DATA,
    FM10000_SRAM_TE_LOOKUP,
    FM10000_SRAM_TE_SIP,
    FM10000_SRAM_TE_VNI,
    FM10000_SRAM_TE_PAYLOAD_FIFO,
    FM10000_SRAM_TE_HEADER_FIFO,
    FM10000_SRAM_TE_STATS,
    FM10000_SRAM_TE_USED,

    /******************************************
     * EPL memories.
     ******************************************/

    /* EPL_ERROR_IP[0..8] */
    FM10000_SRAM_RX_JITTER_FIFO,        /* 0..3 */
    FM10000_SRAM_FEC_BUFFER,

    /* SERDES_IP[0..8] */
    FM10000_SRAM_SERDES_RX_FIFO,
    FM10000_SRAM_SERDES_TX_FIFO,

    /******************************************
     * MANAGEMENT memories.
     ******************************************/

    /* BSM_SRAM_CTRL */
    FM10000_SRAM_BSM_SCRATCH,

    /* CRM_SRAM_CTRL */
    FM10000_SRAM_CRM_DATA,              /* 0..1 */
    FM10000_SRAM_CRM_REGISTER,          /* 0..1 */
    FM10000_SRAM_CRM_PERIOD,            /* 0..1 */
    FM10000_SRAM_CRM_PARAM,

    /* FIBM_SRAM_CTRL */
    FM10000_SRAM_FIBM_RAM,

    FM10000_SRAM_MAX

};  /* FM10000 SRAM identifiers */


/******************************************
 * FM10000 memory repair types. 
 *  
 * These symbols are used internally to 
 * notify the parity repair task that a
 * register table needs to be repaired.
 ******************************************/
typedef enum
{
    FM_REPAIR_TYPE_NONE = 0,

    /* FH_HEAD memories */
    FM_REPAIR_ARP_TABLE,
    FM_REPAIR_ARP_USED,
    FM_REPAIR_EGRESS_MST_TABLE,
    FM_REPAIR_EGRESS_VID_TABLE,
    FM_REPAIR_FFU_SLICE_SRAM,
    FM_REPAIR_GLORT_RAM,
    FM_REPAIR_GLORT_TABLE,
    FM_REPAIR_INGRESS_MST_TABLE_0,
    FM_REPAIR_INGRESS_MST_TABLE_1,
    FM_REPAIR_INGRESS_VID_TABLE,
    FM_REPAIR_MA_TABLE_0,
    FM_REPAIR_MA_TABLE_1,
    FM_REPAIR_MAP_VLAN,
    FM_REPAIR_PARSER_PORT_CFG_2,
    FM_REPAIR_PARSER_PORT_CFG_3,
    FM_REPAIR_RX_VPRI_MAP,

    /* FH_TAIL memories */
    FM_REPAIR_POLICER_CFG_4K_0,
    FM_REPAIR_POLICER_CFG_4K_1,
    FM_REPAIR_POLICER_CFG_512_0,
    FM_REPAIR_POLICER_CFG_512_1,
    FM_REPAIR_POLICER_STATE_4K_0,
    FM_REPAIR_POLICER_STATE_4K_1,
    FM_REPAIR_POLICER_STATE_512_0,
    FM_REPAIR_POLICER_STATE_512_1,
    FM_REPAIR_RX_STATS_BANK,   /* 0..6 */
    FM_REPAIR_SAF_MATRIX,

    /* MODIFY memories */
    FM_REPAIR_MCAST_VLAN_TABLE,
    FM_REPAIR_MIRROR_PROFILE_TABLE,
    FM_REPAIR_MOD_PER_PORT_CFG_1,
    FM_REPAIR_MOD_PER_PORT_CFG_2,
    FM_REPAIR_MOD_STATS_BANK_BYTE_0,
    FM_REPAIR_MOD_STATS_BANK_BYTE_1,
    FM_REPAIR_MOD_STATS_BANK_FRAME_0,
    FM_REPAIR_MOD_STATS_BANK_FRAME_1,
    FM_REPAIR_MOD_VID2_MAP,
    FM_REPAIR_MOD_VLAN_TAG_VID1_MAP,
    FM_REPAIR_MOD_VPRI1_MAP,
    FM_REPAIR_MOD_VPRI2_MAP,

    /* SCHEDULER memories */
    FM_REPAIR_MCAST_DEST_TABLE,
    FM_REPAIR_MCAST_LEN_TABLE,
    FM_REPAIR_SCHED_DRR_CFG,
    FM_REPAIR_SCHED_ESCHED_CFG_1,
    FM_REPAIR_SCHED_ESCHED_CFG_2,
    FM_REPAIR_SCHED_ESCHED_CFG_3,
    FM_REPAIR_SCHED_RX_SCHEDULE,
    FM_REPAIR_SCHED_TX_SCHEDULE,

    /* TUNNEL_ENGINE memories */
    FM_REPAIR_TUNNEL_ENGINE_0,  /* 0..7 */
    FM_REPAIR_TUNNEL_ENGINE_1,  /* 0..7 */

    /* TCAM memories */
    FM_REPAIR_FFU_SLICE_TCAM,   /* 0..31 */
    FM_REPAIR_GLORT_CAM,

    FM_REPAIR_TYPE_MAX

} fm_repairType;


/******************************************
 * Parity error processing state. 
 *  
 * These values are used internally to 
 * represent the state of the parity API.
 ******************************************/
enum
{
    /** The parity error mechanism is inactive. */
    FM10000_PARITY_STATE_INACTIVE = 0,

    /** A parity error has been detected. */
    FM10000_PARITY_STATE_DECODE,

    /** An unrecoverable parity error has been detected. The API has
     *  disabled all parity interrupts, and is waiting for the
     *  application to reset the switch. */
    FM10000_PARITY_STATE_FATAL,
};


/******************************************
 * Parity error repair data. 
 *  
 * Used internally to specify additional 
 * information to the parity repair task 
 * for certain memories. 
 ******************************************/
typedef struct _fm10000_repairData
{
    fm_uint32   errMask;
    fm_uint32   uerrMask;

} fm10000_repairData;


/******************************************
 * Parity API information.
 * Stored in the FM10000 switch extension.
 ******************************************/
typedef struct _fm10000_parityInfo
{
    /** Contents of various Interrupt Pending registers.
     *  Saved by the first-stage parity interrupt handler. */
    fm_uint64   sram_ip;
    fm_uint64   mod_ip;
    fm_uint64   te_ip[FM10000_NUM_TUNNEL_ENGINES];
    fm_uint64   fh_head_ip;
    fm_uint32   fh_tail_ip;
    fm_uint32   sched_ip;
    fm_uint32   core_int;
    fm_uint32   crm_ip[FM10000_CRM_IP_WIDTH];

    /** Parity error processing state. */
    fm_int      parityState;

    /** Bit mask indicating which FRAME_MEMORY segments have reported
     *  memory errors. This word shadows the SRAM_ERR_IP register. */
    fm_uint64   sramErrHistory;

    /** Bit mask indicating which SRAMS are awaiting corrective action.
     *  Indexed by fm_repairType. */
    fm_uint64   pendingRepairs;

    /** Bit mask indicating which pending repairs are for uncorrectable
     *  errors. Indexed by fm_repairType. */
    fm_uint64   pendingUerrs;

    fm10000_repairData  ffuRamRepair;
    fm10000_repairData  ffuTcamRepair;
    fm10000_repairData  rxStatsRepair;
    fm10000_repairData  teErrRepair[FM10000_NUM_TUNNEL_ENGINES];

    fm_bool     interruptsEnabled;

} fm10000_parityInfo;


/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/******************************************
 * fm10000_api_parity.c
 ******************************************/

fm_status fm10000DbgDumpParity(fm_int sw);

fm_status fm10000GetParityAttribute(fm_int sw, fm_int attr, void * value);

fm_status fm10000GetParityErrorCounters(fm_int  sw,
                                        void *  counters,
                                        fm_uint size);

fm_status fm10000InitParity(fm_switch * switchPtr);

fm_status fm10000ResetParityErrorCounters(fm_int sw);

fm_status fm10000SetParityAttribute(fm_int sw, fm_int attr, void * value);

fm_status fm10000FreeParityResources(fm_switch * switchPtr);

/******************************************
 * fm10000_api_parity_decode.c
 ******************************************/

fm_status fm10000ParityErrorDecoder(fm_switch * switchPtr);

/******************************************
 * fm10000_api_parity_intr.c
 ******************************************/

fm_status fm10000CoreInterruptHandler(fm_switch * switchPtr);

fm_status fm10000CrmInterruptHandler(fm_switch * switchPtr);

fm_status fm10000FHHeadInterruptHandler(fm_switch * switchPtr);

fm_status fm10000FHTailInterruptHandler(fm_switch * switchPtr,
                                        fm_uint32 * fh_tail);

fm_status fm10000TEInterruptHandler(fm_switch * switchPtr,
                                    fm_int      index,
                                    fm_uint64 * intMask);

fm_status fm10000EnableParityInterrupts(fm_int sw);

fm_status fm10000DisableParityInterrupts(fm_int sw);

/******************************************
 * fm10000_api_parity_repair.c
 ******************************************/

void * fm10000ParityRepairTask(fm_int    sw,
                               fm_bool * switchProtected,
                               void *    args);

fm_status fm10000RequestParityRepair(fm_int    sw,
                                     fm_int    repairType,
                                     fm_uint32 auxData);

const char * fmRepairTypeToText(fm_int repairType);

#endif  /* __FM_FM10000_API_PARITY_INT_H */


