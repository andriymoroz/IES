/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_parity_decode.c
 * Creation Date:   August 22, 2014.
 * Description:     Parity interrupt decode.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation.
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

#include <fm_sdk_fm10000_int.h>
#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Bit mask for all MOD_IP conditions that are unrecoverable. */
#define FM10000_INT_MOD_FATAL_ERR               \
     (FM10000_INT_MOD_CmTxInfoFifo_UERR |       \
      FM10000_INT_MOD_Ctrl2DpFifo0_UERR |       \
      FM10000_INT_MOD_Ctrl2DpFifo1_UERR |       \
      FM10000_INT_MOD_Ctrl2DpFifo2_UERR |       \
      FM10000_INT_MOD_Ctrl2DpFifo3_UERR |       \
      FM10000_INT_MOD_EschedTxInfoFifo_UERR |   \
      FM10000_INT_MOD_McVlanTable_UERR |        \
      FM10000_INT_MOD_MgmtRdFifo_UERR |         \
      FM10000_INT_MOD_MirProfTable_UERR |       \
      FM10000_INT_MOD_PerPortCfg1_UERR |        \
      FM10000_INT_MOD_PerPortCfg2_UERR |        \
      FM10000_INT_MOD_Vid2Map_UERR |            \
      FM10000_INT_MOD_Vpri1Map_UERR |           \
      FM10000_INT_MOD_Vpri2Map_UERR |           \
      FM10000_INT_MOD_VtagVid1Map_UERR)

/* Bit mask for all MOD_IP conditions that are self-repairing. */
#define FM10000_INT_MOD_SELF_REPAIR             \
      (FM10000_INT_MOD_CmTxInfoFifo_CERR |      \
       FM10000_INT_MOD_Ctrl2DpFifo0_CERR |      \
       FM10000_INT_MOD_Ctrl2DpFifo1_CERR |      \
       FM10000_INT_MOD_Ctrl2DpFifo2_CERR |      \
       FM10000_INT_MOD_Ctrl2DpFifo3_CERR |      \
       FM10000_INT_MOD_EschedTxInfoFifo_CERR |  \
       FM10000_INT_MOD_HeadStorage_CERR |       \
       FM10000_INT_MOD_HeadStorage_UERR |       \
       FM10000_INT_MOD_MgmtRdFifo_CERR |        \
       FM10000_INT_MOD_Refcount_CERR |          \
       FM10000_INT_MOD_StatsByteCnt0_CERR |     \
       FM10000_INT_MOD_StatsByteCnt1_CERR |     \
       FM10000_INT_MOD_StatsFrameCnt0_CERR |    \
       FM10000_INT_MOD_StatsFrameCnt1_CERR)

/* Bit mask for all MOD_IP conditions that are cumulative. */
#define FM10000_INT_MOD_CUMULATIVE              \
     (FM10000_INT_MOD_Refcount_UERR |           \
      FM10000_INT_MOD_StatsByteCnt0_UERR |      \
      FM10000_INT_MOD_StatsByteCnt1_UERR |      \
      FM10000_INT_MOD_StatsFrameCnt0_UERR |     \
      FM10000_INT_MOD_StatsFrameCnt1_UERR)

/* Bit mask for all MOD_IP conditions that are software-repairable. */
#define FM10000_INT_MOD_REPAIRABLE              \
     (FM10000_INT_MOD_McVlanTable_CERR |        \
      FM10000_INT_MOD_MirProfTable_CERR |       \
      FM10000_INT_MOD_PerPortCfg1_CERR |        \
      FM10000_INT_MOD_PerPortCfg2_CERR |        \
      FM10000_INT_MOD_Vid2Map_CERR |            \
      FM10000_INT_MOD_Vpri1Map_CERR |           \
      FM10000_INT_MOD_Vpri2Map_CERR |           \
      FM10000_INT_MOD_VtagVid1Map_CERR)

/* Bit mask for all SCHED_IP conditions that are unrecoverable. */
#define FM10000_INT_SCHED_FATAL_ERR             \
    (FM10000_INT_SCHED_EschedSram_UERR |        \
     FM10000_INT_SCHED_FifoSram_UERR |          \
     FM10000_INT_SCHED_RxqMcastSram_UERR |      \
     FM10000_INT_SCHED_SschedSram_UERR |        \
     FM10000_INT_SCHED_TxqSram_UERR)

/* Bit mask for all SCHED_IP conditions that are self-repairing. */
#define FM10000_INT_SCHED_SELF_REPAIR           \
    (FM10000_INT_SCHED_FifoSram_CERR |          \
     FM10000_INT_SCHED_FreelistSram_CERR |      \
     FM10000_INT_SCHED_SschedSram_CERR |        \
     FM10000_INT_SCHED_TxqSram_CERR)

/* Bit mask for all SCHED_IP conditions that are software-repairable. */
#define FM10000_INT_SCHED_REPAIRABLE            \
    (FM10000_INT_SCHED_SchedConfigSram_CERR |   \
     FM10000_INT_SCHED_SchedConfigSram_UERR)

/* Bit mask for the FFU_SLICE_TCAM CRM checksum interrupts. */
#define CRM_FFU_SLICE_TCAM_INT_MASK     ( (FM_LITERAL_U64(1) << 32) - 1 )

/* Bit mask for the GLORT_CAM CRM checksum interrupt. */
#define CRM_GLORT_CAM_INT_MASK          (FM_LITERAL_U64(1) << 32)

/* Bit mask for all CRM TCAM checksum interrupts. */
#define CRM_INTERRUPT_MASK  \
      (CRM_FFU_SLICE_TCAM_INT_MASK | CRM_GLORT_CAM_INT_MASK)

#define IDX0    (1 << 0)
#define IDX1    (1 << 1)
#define IDX2    (1 << 2)
#define IDX3    (1 << 3)
#define IDX4    (1 << 4)
#define IDX5    (1 << 5)
#define IDX6    (1 << 6)
#define IDX7    (1 << 7)
#define IDX8    (1 << 8)
#define IDX9    (1 << 9)
#define IDX10   (1 << 10)
#define IDX11   (1 << 11)

typedef struct errorCounters
{
    /* Total number of errors. */
    fm_uint32   errors;

    /* Number of self-repairing errors. */
    fm_uint32   transient;

    /* Number of software-repairable errors. */
    fm_uint32   repairable;

    /* Number of cumulative errors. */
    fm_uint32   cumulative;

} errorCounters;

typedef struct
{
    /* Bit number in register. */
    fm_int  bitNo;

    /* SRAM number. */
    fm_int  sramNo;

} sramDesc;



/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** CountBits
 * \ingroup intParity
 *
 * \desc            Counts the number of '1' bits in a word.
 *
 * \param[in]       value is the word to be counted.
 *
 * \return          The number of '1' bits in the argument.
 *
 *****************************************************************************/
static fm_uint32 CountBits(fm_uint64 value)
{
    fm_uint32   bitCount;

    bitCount = 0;

    while (value)
    {
        if (value & 1)
        {
            ++bitCount;
        }
        value >>= 1;
    }

    return bitCount;

}   /* end CountBits */




/*****************************************************************************/
/** MapModIpToSram
 * \ingroup intParity
 *
 * \desc            Maps the lowest '1' bit in the MOD_IP register
 *                  to its corresponding SRAM number.
 *
 * \param[in]       mod_ip is the value of the MOD_IP register.
 *
 * \return          SRAM number, or -1 if no match found.
 *
 *****************************************************************************/
static fm_int MapModIpToSram(fm_uint64 mod_ip)
{
    fm_uint i;

#define MODIFY_SRAM(bitName, sramName) \
    { FM10000_MOD_IP_b_ ## bitName ## _CERR, FM10000_SRAM_ ## sramName }

    static sramDesc modSramMap[] =
    {
        MODIFY_SRAM(HeadStorage, MOD_HEAD_STORAGE),
        MODIFY_SRAM(McVlanTable, MOD_MCAST_VLAN_TABLE),
        MODIFY_SRAM(PerPortCfg1, MOD_PER_PORT_CFG_1),
        MODIFY_SRAM(PerPortCfg2, MOD_PER_PORT_CFG_2),
        MODIFY_SRAM(Vpri1Map, MOD_VPRI1_MAP),
        MODIFY_SRAM(Vpri2Map, MOD_VPRI2_MAP),
        MODIFY_SRAM(MirProfTable, MOD_MIRROR_PROFILE_TABLE),
        MODIFY_SRAM(VtagVid1Map, MOD_VLAN_TAG_VID1_MAP),
        MODIFY_SRAM(Vid2Map, MOD_VID2_MAP),
        MODIFY_SRAM(Ctrl2DpFifo0, MOD_CTRL_TO_DP_FIFO),
        MODIFY_SRAM(Ctrl2DpFifo1, MOD_CTRL_TO_DP_FIFO),
        MODIFY_SRAM(Ctrl2DpFifo2, MOD_CTRL_TO_DP_FIFO),
        MODIFY_SRAM(Ctrl2DpFifo3, MOD_CTRL_TO_DP_FIFO),
        MODIFY_SRAM(StatsFrameCnt0, MOD_STATS_BANK_FRAME),
        MODIFY_SRAM(StatsFrameCnt1, MOD_STATS_BANK_FRAME),
        MODIFY_SRAM(StatsByteCnt0, MOD_STATS_BANK_BYTE),
        MODIFY_SRAM(StatsByteCnt1, MOD_STATS_BANK_BYTE),
        MODIFY_SRAM(Refcount, MOD_REFCOUNT),
        MODIFY_SRAM(EschedTxInfoFifo, MOD_ESCHED_TX_FIFO),
        MODIFY_SRAM(CmTxInfoFifo, MOD_CM_TX_FIFO),
        MODIFY_SRAM(MgmtRdFifo, MOD_MGMT_RD_FIFO)
    };

    for (i = 0 ; i < FM_NENTRIES(modSramMap) ; i++)
    {
        if (mod_ip & (FM_LITERAL_U64(3) << modSramMap[i].bitNo))
        {
            return modSramMap[i].sramNo;
        }
    }

    return -1;

}   /* end MapModIpToSram */




/*****************************************************************************/
/** MapSchedIpToSram
 * \ingroup intParity
 *
 * \desc            Maps the lowest '1' bit in the SCHED_IP register
 *                  to its corresponding SRAM number.
 *
 * \param[in]       sched_ip is the value of the SCHED_IP register.
 *
 * \return          SRAM number, or -1 if no match found.
 *
 *****************************************************************************/
static fm_int MapSchedIpToSram(fm_uint32 sched_ip)
{
    fm_uint i;

#define SCHED_SRAM(bitName, sramName) \
    { FM10000_SCHED_IP_l_ ## bitName, FM10000_SRAM_ ## sramName }

    static sramDesc schedSramMap[] =
    {
        SCHED_SRAM(SchedConfigSramErr, SCHED_CONFIG),
        SCHED_SRAM(FreelistSramErr, SCHED_FREELIST),
        SCHED_SRAM(SschedSramErr, SCHED_SSCHED),
        SCHED_SRAM(MonitorSramErr, SCHED_MONITOR),
        SCHED_SRAM(EschedSramErr, SCHED_ESCHED),
        SCHED_SRAM(TxqSramErr, SCHED_TXQ),
        SCHED_SRAM(RxqMcastSramErr, SCHED_RXQ),
        SCHED_SRAM(FifoSramErr, SCHED_FIFO)
    };

    for (i = 0 ; i < FM_NENTRIES(schedSramMap) ; i++)
    {
        if (sched_ip & (3 << schedSramMap[i].bitNo))
        {
            return schedSramMap[i].sramNo;
        }
    }

    return -1;

}   /* end MapSchedIpToSram */




/*****************************************************************************/
/** MapSchedMonitorErrToSram
 * \ingroup intParity
 *
 * \desc            Maps the lowest '1' bit in the SCHED_MONITOR_SRAM_CTRL
 *                  CErr or UErr field to its corresponding SRAM number.
 *
 * \param[in]       errVal is the value of the CErr or UErr field.
 *
 * \return          SRAM number, or -1 if no match found.
 *
 *****************************************************************************/
static fm_int MapSchedMonitorErrToSram(fm_uint16 errVal)
{
    fm_uint i;

    static sramDesc schedMonitorSramMap[] =
    {
        { 0, FM10000_SRAM_SCHED_MONITOR_DRR_Q_PERQ },
        { 1, FM10000_SRAM_SCHED_MONITOR_DRR_DC_PERQ },
        { 2, FM10000_SRAM_SCHED_MONITOR_DRR_CFG_PERPORT },
        { 3, FM10000_SRAM_SCHED_MONITOR_PEP_PERPORT },
    };

    for (i = 0 ; i < FM_NENTRIES(schedMonitorSramMap) ; i++)
    {
        if (errVal & (1 << schedMonitorSramMap[i].bitNo))
        {
            return schedMonitorSramMap[i].sramNo;
        }
    }

    return -1;

}   /* end MapSchedMonitorErrToSram */




/*****************************************************************************/
/** UpdateErrorCounts
 * \ingroup intParity
 *
 * \desc            Updates error counters based on the contents of the
 *                  IP registers. Additional errors will be counted
 *                  as the SRAM_CTRL registers are decoded.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateErrorCounts(fm_int sw)
{
    fm10000_parityInfo *parityInfo;
    fm_uint64           tempMask;
    fm_uint32           errCount;

    parityInfo = GET_PARITY_INFO(sw);

    /**************************************************
     * Get CROSSBAR error counts. 
     *  
     * All CROSSBAR errors reported by means of 
     * CORE_INTERRUPT_DETECT are immediately fatal, 
     * so we count them here. 
     **************************************************/

    tempMask = parityInfo->core_int & FM10000_INT_CORE_CROSSBAR_ERR;

    if (tempMask)
    {
        errCount = CountBits(tempMask);

        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_CROSSBAR, errCount);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_FATAL, errCount);
    }

    /**************************************************
     * Get MODIFY error counts. 
     *  
     * All MODIFY errors are reported by means of the 
     * MOD_IP register, so we count them here.
     **************************************************/

    if (parityInfo->mod_ip)
    {
        /* total errors */
        errCount = CountBits(parityInfo->mod_ip);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_MODIFY, errCount);

        /* fatal errors */
        errCount = CountBits(parityInfo->mod_ip & FM10000_INT_MOD_FATAL_ERR);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_FATAL, errCount);

        /* software-repairable errors */
        errCount = CountBits(parityInfo->mod_ip & FM10000_INT_MOD_REPAIRABLE);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_REPAIRABLE, errCount);

        /* cumulative errors */
        errCount = CountBits(parityInfo->mod_ip & FM10000_INT_MOD_CUMULATIVE);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_CUMULATIVE, errCount);

        /* self-repairing errors */
        errCount = CountBits(parityInfo->mod_ip & FM10000_INT_MOD_SELF_REPAIR);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, errCount);
    }

    /**************************************************
     * Get SCHEDULER error counts. 
     *  
     * If we detect a fatal error here, we will use 
     * the SCHED_IP register to obtain approximate 
     * error counts, rather than reading the individual 
     * SRAM_CTRL registers. Otherwise, all SCHEDULER
     * errors are counted by the decode functions. 
     **************************************************/

    tempMask = parityInfo->sched_ip & FM10000_INT_SCHED_FATAL_ERR;

    if (tempMask)
    {
        /* total errors */
        errCount = CountBits(parityInfo->sched_ip);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_SCHEDULER, errCount);

        /* fatal errors */
        errCount = CountBits(tempMask);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_FATAL, errCount);

        /* software-repairable errors */
        tempMask = parityInfo->sched_ip & FM10000_INT_SCHED_REPAIRABLE;
        errCount = CountBits(tempMask);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_REPAIRABLE, errCount);

        /* self-repairing errors */
        tempMask = parityInfo->sched_ip & FM10000_INT_SCHED_SELF_REPAIR;
        errCount = CountBits(tempMask);
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, errCount);
    }

    return FM_OK;

}   /* end UpdateErrorCounts */




/*****************************************************************************/
/** UpdateFhHeadIntMask
 * \ingroup intParity
 *
 * \desc            Updates the FH_HEAD interrupt mask to reenable any
 *                  parity interrupts we have finished processing.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       intMask specifies the set of interrupts we have just
 *                  finished processing.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateFhHeadIntMask(fm_switch * switchPtr,
                                     fm_uint64   intMask)
{
    fm_uint64   fh_head_im;
    fm_status   err;
    fm_int      sw;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT64(sw, FM10000_FH_HEAD_IM(0), &fh_head_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    fh_head_im &= ~intMask;

    err = switchPtr->WriteUINT64(sw, FM10000_FH_HEAD_IM(0), fh_head_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

ABORT:
    DROP_REG_LOCK(sw);

    return err;

}   /* end UpdateFhHeadIntMask */




/*****************************************************************************/
/** UpdateFhTailIntMask
 * \ingroup intParity
 *
 * \desc            Updates the FH_TAIL interrupt mask to reenable any
 *                  parity interrupts we have finished processing.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       intMask specifies the set of interrupts we have just
 *                  finished processing.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateFhTailIntMask(fm_switch * switchPtr,
                                     fm_uint32   intMask)
{
    fm_status   err;

    err = switchPtr->MaskUINT32(switchPtr->switchNumber,
                                FM10000_FH_TAIL_IM(),
                                intMask,
                                FALSE);

    return err;

}   /* end UpdateFhTailIntMask */




/*****************************************************************************/
/** UpdateModifyIntMask
 * \ingroup intParity
 *
 * \desc            Updates the MODIFY interrupt mask to reenable any
 *                  parity interrupts we have finished processing.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       intMask specifies the set of interrupts we have just
 *                  finished processing.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateModifyIntMask(fm_switch * switchPtr,
                                     fm_uint64   intMask)
{
    fm_uint64   mod_im;
    fm_status   err;
    fm_int      sw;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT64(sw, FM10000_MOD_IM(0), &mod_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    mod_im &= ~intMask;

    err = switchPtr->WriteUINT64(sw, FM10000_MOD_IM(0), mod_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

ABORT:
    DROP_REG_LOCK(sw);

    return err;

}   /* end UpdateModifyIntMask */




/*****************************************************************************/
/** UpdateSchedulerIntMask
 * \ingroup intParity
 *
 * \desc            Updates the SCHEDULER interrupt mask to reenable any
 *                  parity interrupts we have finished processing.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       intMask specifies the set of interrupts we have just
 *                  finished processing.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateSchedulerIntMask(fm_switch * switchPtr,
                                        fm_uint32   intMask)
{
    fm_status   err;

    err = switchPtr->MaskUINT32(switchPtr->switchNumber,
                                FM10000_SCHED_IM(),
                                intMask,
                                FALSE);

    return err;

}   /* end UpdateSchedulerIntMask */




/*****************************************************************************/
/** UpdateTeIntMask
 * \ingroup intParity
 *
 * \desc            Updates the TUNNEL_ENGINE interrupt mask to reenable any
 *                  parity interrupts we have finished processing.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       inst is the Tunneling Engine instance.
 * 
 * \param[in]       intMask specifies the set of interrupts we have just
 *                  finished processing.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateTeIntMask(fm_switch * switchPtr,
                                 fm_int      inst,
                                 fm_uint64   intMask)
{
    fm_uint64   te_im;
    fm_status   err;
    fm_int      sw;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT64(sw, FM10000_TE_IM(inst, 0), &te_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    te_im &= ~intMask;

    err = switchPtr->WriteUINT64(sw, FM10000_TE_IM(inst, 0), te_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

ABORT:
    DROP_REG_LOCK(sw);

    return err;

}   /* end UpdateTeIntMask */




/*****************************************************************************/
/** FatalError
 * \ingroup intParity
 *
 * \desc            Called by the decode stage of the interrupt handler
 *                  when it detects an unrecoverable parity error.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       memArea is the memory area in which the error was
 *                  detected.
 * 
 * \param[in]       fatalCount is the number of fatal errors to count.
 *                  Will be zero if we've already counted the error.
 * 
 * \param[in]       sramNo is the SRAM number to be reported.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FatalError(fm_switch *      switchPtr,
                            fm_parityMemArea memArea,
                            fm_uint32        fatalCount,
                            fm_int           sramNo)
{
    fm_eventParityError parityEvent;
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_status           retStatus;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    if (parityInfo->parityState >= FM10000_PARITY_STATE_FATAL)
    {
        return FM_OK;
    }

    retStatus = FM_OK;

    FM_LOG_FATAL(FM_LOG_CAT_PARITY,
                 "Unrecoverable error in %s memory\n",
                 fmParityMemAreaToText(memArea));

    if (fatalCount)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_FATAL, fatalCount);
    }

    err = fm10000DisableParityInterrupts(sw);
    FM_ERR_COMBINE(retStatus, err);

    FM_CLEAR(parityEvent);

    parityEvent.errType = FM_PARITY_ERRTYPE_SRAM_UNCORRECTABLE;
    parityEvent.paritySeverity = FM_PARITY_SEVERITY_FATAL;
    parityEvent.memoryArea = memArea;
    parityEvent.parityStatus = FM_PARITY_STATUS_FATAL_ERROR;
    parityEvent.sramNo = sramNo;

#if 1
    /* For debugging purposes, we will dump the event here... */
    fmDbgDumpParityErrorEvent(sw, &parityEvent);
#endif

    err = fmSendParityErrorEvent(sw, parityEvent, &fmRootApi->eventThread);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                     "Could not send parity error event\n");
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_EVENT_LOST, 1);
        FM_ERR_COMBINE(retStatus, err);
    }

    parityInfo->parityState = FM10000_PARITY_STATE_FATAL;

    return retStatus;

}   /* end FatalError */




/*****************************************************************************/
/** DeferredReset
 * \ingroup intParity
 *
 * \desc            Called by the decode stage of the interrupt handler
 *                  when it detects a deferred reset error condition.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       memArea is the memory area in which the error was
 *                  detected.
 * 
 * \param[in]       counter is the index of the counter that tracks the
 *                  number of occurrences of this error.
 * 
 * \param[in]       errorThresh is the threshold at which the API should
 *                  send a Deferred Reset notification to the application.
 * 
 * \param[in]       fatalThresh is the threshold at which the API should
 *                  send an Immediate Reset notification to the application.
 * 
 * \param[in]       sramNo is the SRAM number to be reported.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeferredReset(fm_switch *      switchPtr,
                               fm_parityMemArea memArea,
                               fm_trackingCounterIndex counter,
                               fm_uint32        errorThresh,
                               fm_uint32        fatalThresh,
                               fm_int           sramNo)
{
    fm_eventParityError parityEvent;
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           diagCount;
    fm_int              sw;
    fm_status           err;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    if (parityInfo->parityState >= FM10000_PARITY_STATE_FATAL)
    {
        return FM_OK;
    }

    fmDbgDiagCountIncr(sw, counter, 1);
    fmDbgDiagCountGet(sw, counter, &diagCount);

    /**************************************************
     * If we've reached or exceeded the fatal error 
     * threshold for this counter, send an immediate
     * reset notification to the application and enter 
     * fatal error state. 
     **************************************************/

    if (fatalThresh && diagCount >= fatalThresh)
    {
        return FatalError(switchPtr, memArea, 1, sramNo);
    }

    /**************************************************
     * If we've reached the error threshold for this 
     * resource, send a deferred reset notification to 
     * the application. 
     **************************************************/

    if (errorThresh && diagCount == errorThresh)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                     "Threshold error in %s memory\n",
                     fmParityMemAreaToText(memArea));

        FM_CLEAR(parityEvent);

        parityEvent.errType = FM_PARITY_ERRTYPE_SRAM_UNCORRECTABLE;
        parityEvent.paritySeverity = FM_PARITY_SEVERITY_CUMULATIVE;
        parityEvent.memoryArea = memArea;
        parityEvent.parityStatus = FM_PARITY_STATUS_DEFERRED_RESET;
        parityEvent.numErrors = (fm_int)diagCount;
        parityEvent.sramNo = sramNo;

#if 1
        /* For debugging purposes, we will dump the event here... */
        fmDbgDumpParityErrorEvent(sw, &parityEvent);
#endif

        err = fmSendParityErrorEvent(sw, parityEvent, &fmRootApi->eventThread);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                         "Could not send parity error event\n");
            fmDbgDiagCountIncr(sw, FM_CTR_PARITY_EVENT_LOST, 1);
        }

        return err;
    }

    return FM_OK;

}   /* end DeferredReset */




/*****************************************************************************/
/** RequestRepair
 * \ingroup intParity
 *
 * \desc            Called by the decode stage of the interrupt handler
 *                  when it detects a recoverable parity error.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       repairType is the type of repair to be performed.
 * 
 * \param[in]       isUerr is TRUE if the error is uncorrectable, FALSE
 *                  otherwise.
 * 
 * \param[in]       auxData is additional information about the memory
 *                  to be repaired.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RequestRepair(fm_int     sw,
                               fm_int     repairType,
                               fm_bool    isUerr,
                               fm_uint32  auxData)
{
    fm10000_parityInfo *parityInfo;
    fm_uint64           bitMask;
    fm_status           err;

    parityInfo = GET_PARITY_INFO(sw);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "sw=%d repairType=%s (%s) auxData=%04x\n",
                 sw,
                 fmRepairTypeToText(repairType),
                 (isUerr) ? "UERR" : "CERR",
                 auxData);

    if (repairType < 0 || repairType >= FM_REPAIR_TYPE_MAX)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    TAKE_PARITY_LOCK(sw);

    bitMask =  FM_LITERAL_U64(1) << repairType;

    parityInfo->pendingRepairs |= bitMask;

    if (isUerr)
    {
        parityInfo->pendingUerrs |= bitMask;
    }

    switch (repairType)
    {
        case FM_REPAIR_FFU_SLICE_SRAM:
            parityInfo->ffuRamRepair.errMask |= auxData;
            if (isUerr)
            {
                parityInfo->ffuRamRepair.uerrMask |= auxData;
            }
            break;

        case FM_REPAIR_FFU_SLICE_TCAM:
            parityInfo->ffuTcamRepair.errMask |= auxData;
            if (isUerr)
            {
                parityInfo->ffuTcamRepair.uerrMask |= auxData;
            }
            break;

        case FM_REPAIR_RX_STATS_BANK:
            parityInfo->rxStatsRepair.errMask |= auxData;
            if (isUerr)
            {
                parityInfo->rxStatsRepair.uerrMask |= auxData;
            }
            break;

        case FM_REPAIR_TUNNEL_ENGINE_0:
            parityInfo->teErrRepair[0].errMask |= auxData;
            if (isUerr)
            {
                parityInfo->teErrRepair[0].uerrMask |= auxData;
            }
            break;

        case FM_REPAIR_TUNNEL_ENGINE_1:
            parityInfo->teErrRepair[1].errMask |= auxData;
            if (isUerr)
            {
                parityInfo->teErrRepair[1].uerrMask |= auxData;
            }
            break;

        default:
            break;

    }   /* end switch (repairType) */

    DROP_PARITY_LOCK(sw);

    err = fmSignalSemaphore(&fmRootApi->parityRepairSemaphore);

    return err;

}   /* end RequestRepair */




/*****************************************************************************/
/** DecodeArpSramErr
 * \ingroup intParity
 *
 * \desc            Decodes ARP_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeArpSramErr(fm_switch *     switchPtr,
                                  errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_ARP_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_ARP_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_ARP_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_ARP_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal & (IDX0 | IDX1 | IDX2 | IDX3))
    {
        RequestRepair(sw, FM_REPAIR_ARP_TABLE, TRUE, 0);
    }
    else if (cerrVal & (IDX0 | IDX1 | IDX2 | IDX3))
    {
        RequestRepair(sw, FM_REPAIR_ARP_TABLE, FALSE, 0);
    }

    if (uerrVal & IDX4)
    {
        RequestRepair(sw, FM_REPAIR_ARP_USED, TRUE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeArpSramErr */




/*****************************************************************************/
/** DecodeFfuSramCtrl
 * \ingroup intParity
 *
 * \desc            Decodes FFU_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       idx is the FFU_SRAM_CTRL register to be decoded.
 * 
 * \param[in,out]   counters points to the error counters structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFfuSramCtrl(fm_switch *     switchPtr,
                                   fm_int          idx,
                                   errorCounters * counters)
{
    fm_uint32   regVal[FM10000_FFU_SRAM_CTRL_WIDTH];
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_int      i;
    fm_int      sliceId;
    fm_uint16   bitMask;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_FFU_SRAM_CTRL(idx, 0),
                                    FM10000_FFU_SRAM_CTRL_WIDTH,
                                    regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_FFU_SRAM_CTRL(idx, 0),
                                     FM10000_FFU_SRAM_CTRL_WIDTH,
                                     regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_ARRAY_GET_FIELD(regVal, FM10000_FFU_SRAM_CTRL, CErr);
    uerrVal = FM_ARRAY_GET_FIELD(regVal, FM10000_FFU_SRAM_CTRL, UErr);

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "idx=%d cerr=%04x uerr=%04x count=%u\n",
                 idx, cerrVal, uerrVal, errCount);

    /**************************************************
     * Get error counts.
     **************************************************/

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    for (i = 0 ; i < 4 ; i++)
    {
        sliceId = idx * 4 + i;

        bitMask = (IDX0 | IDX1) << (4 * i);

        if (uerrVal & bitMask)
        {
            RequestRepair(sw, FM_REPAIR_FFU_SLICE_SRAM, TRUE, 1 << sliceId);
        }
        else if (cerrVal & bitMask)
        {
            RequestRepair(sw, FM_REPAIR_FFU_SLICE_SRAM, FALSE, 1 << sliceId);
        }

    }   /* end for (i = 0 ; i < 4 ; i++) */

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeFfuSramCtrl */




/*****************************************************************************/
/** DecodeFFUSramErr
 * \ingroup intParity
 *
 * \desc            Decodes the FFUSramErr field.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       head_ip is the interrupt mask being processed.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFFUSramErr(fm_switch *     switchPtr,
                                  fm_uint64       head_ip,
                                  errorCounters * counters)
{
    fm_uint32   sramErr;
    fm_status   retStatus;
    fm_status   err;
    fm_int      idx;

    sramErr = (fm_uint32)FM_GET_FIELD64(head_ip, FM10000_FH_HEAD_IP, FFUSramErr);

    retStatus = FM_OK;

    for (idx = 0 ; idx < FM10000_FFU_SRAM_CTRL_ENTRIES ; idx++)
    {
        if (sramErr & 3)
        {
            err = DecodeFfuSramCtrl(switchPtr, idx, counters);
            FM_ERR_COMBINE(retStatus, err);
        }
        sramErr >>= 2;
    }

    return retStatus;

}   /* end DecodeFFUSramErr */




/*****************************************************************************/
/** DecodeFGLSramErr
 * \ingroup intParity
 *
 * \desc            Decodes FID_GLORT_LOOKUP_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFGLSramErr(fm_switch *     switchPtr,
                                  errorCounters * counters)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_FID_GLORT_LOOKUP_SRAM_CTRL(0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_FID_GLORT_LOOKUP_SRAM_CTRL(0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_FID_GLORT_LOOKUP_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_FID_GLORT_LOOKUP_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);
    counters->errors += errCount;

    cerrVal &= (IDX0 | IDX1 | IDX2);
    uerrVal &= (IDX0 | IDX1 | IDX2);

    errCount = CountBits(cerrVal) + CountBits(uerrVal);
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_INGRESS_MST_TABLE_0, TRUE, 0);
    }
    else if (cerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_INGRESS_MST_TABLE_0, FALSE, 0);
    }

    if (uerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_INGRESS_MST_TABLE_1, TRUE, 0);
    }
    else if (cerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_INGRESS_MST_TABLE_1, FALSE, 0);
    }

    if (uerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_EGRESS_MST_TABLE, TRUE, 0);
    }
    else if (cerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_EGRESS_MST_TABLE, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeFGLSramErr */




/*****************************************************************************/
/** DecodeFhHeadOutputFifoSramErr
 * \ingroup intParity
 *
 * \desc            Decodes FH_HEAD_OUTPUT_FIFO_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       intMask is the mask for the interrupts we are processing.
 * 
 * \param[in,out]   counters points to the error counters structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFhHeadOutputFifoSramErr(fm_switch *     switchPtr,
                                               fm_uint64       intMask,
                                               errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw,
                                FM10000_FH_HEAD_OUTPUT_FIFO_SRAM_CTRL(),
                                &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_FH_HEAD_OUTPUT_FIFO_SRAM_CTRL(),
                                 regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_FH_HEAD_OUTPUT_FIFO_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_FH_HEAD_OUTPUT_FIFO_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors    += errCount;
    counters->transient += errCount;

    /**************************************************
     * Update interrupt mask.
     **************************************************/

    intMask &= FM10000_INT_FH_HEAD_FhHeadOutputFifoSramErr;

    err = UpdateFhHeadIntMask(switchPtr, intMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeFhHeadOutputFifoSramErr */




/*****************************************************************************/
/** DecodeGlortRamSramErr
 * \ingroup intParity
 *
 * \desc            Decodes GLORT_RAM_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeGlortRamSramErr(fm_switch *     switchPtr,
                                       errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint32   cerrVal;
    fm_uint32   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_GLORT_RAM_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_GLORT_RAM_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_BIT(regVal, FM10000_GLORT_RAM_SRAM_CTRL, CErr);
    uerrVal = FM_GET_BIT(regVal, FM10000_GLORT_RAM_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* There's only one error bit, so xerrVal is the same as xerrCnt. */
    errCount = cerrVal + uerrVal;

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, FM_REPAIR_GLORT_RAM, TRUE, 0);
    }
    else if (cerrVal)
    {
        RequestRepair(sw, FM_REPAIR_GLORT_RAM, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeGlortRamSramErr */




/*****************************************************************************/
/** DecodeGlortTableSramErr
 * \ingroup intParity
 *
 * \desc            Decodes GLORT_TABLE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeGlortTableSramErr(fm_switch *     switchPtr,
                                         errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint32   cerrVal;
    fm_uint32   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_GLORT_TABLE_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_GLORT_TABLE_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_BIT(regVal, FM10000_GLORT_TABLE_SRAM_CTRL, CErr);
    uerrVal = FM_GET_BIT(regVal, FM10000_GLORT_TABLE_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* There's only one error bit, so xerrVal is the same as xerrCnt. */
    errCount = cerrVal + uerrVal;

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, FM_REPAIR_GLORT_TABLE, TRUE, 0);
    }
    else if (cerrVal)
    {
        RequestRepair(sw, FM_REPAIR_GLORT_TABLE, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeGlortTableSramErr */




/*****************************************************************************/
/** DecodeMapperSramErr
 * \ingroup intParity
 *
 * \desc            Decodes MAPPER_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counters structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeMapperSramErr(fm_switch *     switchPtr,
                                     errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint32   cerrVal;
    fm_uint32   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_MAPPER_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_MAPPER_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_BIT(regVal, FM10000_MAPPER_SRAM_CTRL, CErr);
    uerrVal = FM_GET_BIT(regVal, FM10000_MAPPER_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* There's only one error bit, so xerrVal is the same as xerrCnt. */
    errCount = cerrVal + uerrVal;

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, FM_REPAIR_MAP_VLAN, TRUE, 0);
    }
    else if (cerrVal)
    {
        RequestRepair(sw, FM_REPAIR_MAP_VLAN, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeMapperSramErr */




/*****************************************************************************/
/** DecodeMaTableSramCtrl
 * \ingroup intParity
 *
 * \desc            Decodes MA_TABLE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       idx is the MA_TABLE_SRAM_CTRL register to be decoded.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeMaTableSramCtrl(fm_switch *     switchPtr,
                                       fm_int          idx,
                                       errorCounters * counters)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_int      repairType;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_MA_TABLE_SRAM_CTRL(idx, 0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_MA_TABLE_SRAM_CTRL(idx, 0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_MA_TABLE_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_MA_TABLE_SRAM_CTRL, UErr);

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "idx=%d cerr=%04x uerr=%04x count=%u\n",
                 idx, cerrVal, uerrVal, errCount);

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    repairType = (idx == 0) ? FM_REPAIR_MA_TABLE_0 : FM_REPAIR_MA_TABLE_1;

    if (uerrVal)
    {
        RequestRepair(sw, repairType, TRUE, 0);
    }
    else if (cerrVal)
    {
        RequestRepair(sw, repairType, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeMaTableSramCtrl */




/*****************************************************************************/
/** DecodeMaTableSramErr
 * \ingroup intParity
 *
 * \desc            Decodes the MaTableSramErr field.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       head_ip is the interrupt mask being processed.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeMaTableSramErr(fm_switch *     switchPtr,
                                      fm_uint64       head_ip,
                                      errorCounters * counters)
{
    fm_uint32   sramErr;
    fm_status   retStatus;
    fm_status   err;
    fm_int      idx;

    sramErr = (fm_uint32)FM_GET_FIELD64(head_ip, FM10000_FH_HEAD_IP, MaTableSramErr);

    retStatus = FM_OK;

    for (idx = 0 ; idx < FM10000_MA_TABLE_SRAM_CTRL_ENTRIES ; idx++)
    {
        if (sramErr & 3)
        {
            err = DecodeMaTableSramCtrl(switchPtr, idx, counters);
            FM_ERR_COMBINE(retStatus, err);
        }
        sramErr >>= 2;
    }

    return retStatus;

}   /* end DecodeMaTableSramErr */




/*****************************************************************************/
/** DecodeParserEarlySramErr
 * \ingroup intParity
 *
 * \desc            Decodes PARSER_EARLY_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeParserEarlySramErr(fm_switch *     switchPtr,
                                          errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint32   cerrVal;
    fm_uint32   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_PARSER_EARLY_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_PARSER_EARLY_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_BIT(regVal, FM10000_PARSER_EARLY_SRAM_CTRL, CErr);
    uerrVal = FM_GET_BIT(regVal, FM10000_PARSER_EARLY_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* There's only one error bit, so xerrVal is the same as xerrCnt. */
    errCount = cerrVal + uerrVal;

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, FM_REPAIR_PARSER_PORT_CFG_2, TRUE, 0);
    }
    else if (cerrVal)
    {
        RequestRepair(sw, FM_REPAIR_PARSER_PORT_CFG_2, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeParserEarlySramErr */




/*****************************************************************************/
/** DecodeParserLateSramErr
 * \ingroup intParity
 *
 * \desc            Decodes PARSER_LATE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeParserLateSramErr(fm_switch *     switchPtr,
                                         errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_PARSER_LATE_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_PARSER_LATE_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_PARSER_LATE_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_PARSER_LATE_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_PARSER_PORT_CFG_3, TRUE, 0);
    }
    else if (cerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_PARSER_PORT_CFG_3, FALSE, 0);
    }

    if (uerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_RX_VPRI_MAP, TRUE, 0);
    }
    else if (cerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_RX_VPRI_MAP, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeParserLateSramErr */




/*****************************************************************************/
/** DecodeVlanSramErr
 * \ingroup intParity
 *
 * \desc            Decodes VLAN_LOOKUP_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeVlanSramErr(fm_switch *     switchPtr,
                                   errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_VLAN_LOOKUP_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_VLAN_LOOKUP_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_VLAN_LOOKUP_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_VLAN_LOOKUP_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal & (IDX0 | IDX1))
    {
        RequestRepair(sw, FM_REPAIR_INGRESS_VID_TABLE, TRUE, 0);
    }
    else if (cerrVal & (IDX0 | IDX1))
    {
        RequestRepair(sw, FM_REPAIR_INGRESS_VID_TABLE, FALSE, 0);
    }

    if (uerrVal & (IDX2 | IDX3))
    {
        RequestRepair(sw, FM_REPAIR_EGRESS_VID_TABLE, TRUE, 0);
    }
    else if (cerrVal & (IDX2 | IDX3))
    {
        RequestRepair(sw, FM_REPAIR_EGRESS_VID_TABLE, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeVlanSramErr */




/*****************************************************************************/
/** DecodeEgressPauseSramErr
 * \ingroup intParity
 *
 * \desc            Decodes EGRESS_PAUSE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeEgressPauseSramErr(fm_switch *     switchPtr,
                                          errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_EGRESS_PAUSE_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_EGRESS_PAUSE_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    /* Note: memories are parity-protected. CErr should always be zero. */
    cerrVal = FM_GET_FIELD(regVal, FM10000_EGRESS_PAUSE_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_EGRESS_PAUSE_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors    += errCount;
    counters->transient += errCount;

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeEgressPauseSramErr */




/*****************************************************************************/
/** DecodePolicerUsageSramErr
 * \ingroup intParity
 *
 * \desc            Decodes POLICER_USAGE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodePolicerUsageSramErr(fm_switch *     switchPtr,
                                           errorCounters * counters)
{
    fm_uint32   regVal[4];
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_POLICER_USAGE_SRAM_CTRL(0),
                                    FM10000_POLICER_USAGE_SRAM_CTRL_WIDTH,
                                    regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_POLICER_USAGE_SRAM_CTRL(0),
                                     FM10000_POLICER_USAGE_SRAM_CTRL_WIDTH,
                                     regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_ARRAY_GET_FIELD(regVal, FM10000_POLICER_USAGE_SRAM_CTRL, CErr);
    uerrVal = FM_ARRAY_GET_FIELD(regVal, FM10000_POLICER_USAGE_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* total errors */
    errCount = CountBits(cerrVal) + CountBits(uerrVal);
    counters->errors += errCount;

    /* repairable errors (configuration) */
    errCount  = CountBits(cerrVal & (IDX0 | IDX1 | IDX2 | IDX3));
    errCount += CountBits(uerrVal & (IDX0 | IDX1 | IDX2 | IDX3));
    counters->repairable += errCount;

    /* transient errors (state: monitor and notify) */
    errCount = CountBits(cerrVal & ~(IDX0 | IDX1 | IDX2 | IDX3));
    counters->transient += errCount;

    /* cumulative errors (state: scan and recover) */
    errCount = CountBits(uerrVal & ~(IDX0 | IDX1 | IDX2 | IDX3));
    counters->cumulative += errCount;

    /**************************************************
     * Handle config memory errors.
     **************************************************/

    if (uerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_4K_0, TRUE, 0);
    }
    else if (cerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_4K_0, FALSE, 0);
    }

    if (uerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_4K_1, TRUE, 0);
    }
    else if (cerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_4K_1, FALSE, 0);
    }

    if (uerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_512_0, TRUE, 0);
    }
    else if (cerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_512_0, FALSE, 0);
    }

    if (uerrVal & IDX3)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_512_1, TRUE, 0);
    }
    else if (cerrVal & IDX3)
    {
        RequestRepair(sw, FM_REPAIR_POLICER_CFG_512_1, FALSE, 0);
    }

    /**************************************************
     * Handle state memory errors.
     **************************************************/

    if (uerrVal & (IDX4 | IDX8))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_4K_0, TRUE, 0);
    }
    else if (cerrVal & (IDX4 | IDX8))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_4K_0, FALSE, 0);
    }

    if (uerrVal & (IDX5 | IDX9))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_4K_1, TRUE, 0);
    }
    else if (cerrVal & (IDX5 | IDX9))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_4K_1, FALSE, 0);
    }

    if (uerrVal & (IDX6 | IDX10))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_512_0, TRUE, 0);
    }
    else if (cerrVal & (IDX6 | IDX10))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_512_0, FALSE, 0);
    }

    if (uerrVal & (IDX7 | IDX11))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_512_1, TRUE, 0);
    }
    else if (cerrVal & (IDX7 | IDX11))
    {
        RequestRepair(sw, FM_REPAIR_POLICER_STATE_512_1, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodePolicerUsageSramErr */




/*****************************************************************************/
/** DecodeRxStatsSramErr
 * \ingroup intParity
 *
 * \desc            Decodes POLICER_USAGE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeRxStatsSramErr(fm_switch *     switchPtr,
                                      errorCounters * counters)
{
    fm_uint32   regVal[4];
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   cerrCnt;
    fm_uint32   uerrCnt;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_RX_STATS_SRAM_CTRL(0),
                                    FM10000_RX_STATS_SRAM_CTRL_WIDTH,
                                    regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_RX_STATS_SRAM_CTRL(0),
                                     FM10000_RX_STATS_SRAM_CTRL_WIDTH,
                                     regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_ARRAY_GET_FIELD(regVal, FM10000_RX_STATS_SRAM_CTRL, CErr);
    uerrVal = FM_ARRAY_GET_FIELD(regVal, FM10000_RX_STATS_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    cerrCnt = CountBits(cerrVal);
    uerrCnt = CountBits(uerrVal);

    counters->errors     += cerrCnt + uerrCnt;
    counters->transient  += cerrCnt;
    counters->cumulative += uerrCnt;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, FM_REPAIR_RX_STATS_BANK, TRUE, uerrVal);
        cerrVal &= ~uerrVal;
    }

    /**************************************************
     * Process correctable errors.
     **************************************************/

    if (cerrVal)
    {
        RequestRepair(sw, FM_REPAIR_RX_STATS_BANK, FALSE, cerrVal);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeRxStatsSramErr */




/*****************************************************************************/
/** DecodeSafSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SAF_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSafSramErr(fm_switch *     switchPtr,
                                  errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_SAF_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_SAF_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_SAF_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_SAF_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, FM_REPAIR_SAF_MATRIX, TRUE, 0);
    }
    else if (cerrVal)
    {
        RequestRepair(sw, FM_REPAIR_SAF_MATRIX, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSafSramErr */




/*****************************************************************************/
/** DecodeTcnSramErr
 * \ingroup intParity
 *
 * \desc            Decodes TCN_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeTcnSramErr(fm_switch *     switchPtr,
                                  errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint32   cerrVal;
    fm_uint32   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_TCN_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_TCN_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_BIT(regVal, FM10000_TCN_SRAM_CTRL, CErr);
    uerrVal = FM_GET_BIT(regVal, FM10000_TCN_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* There's only one error bit, so xerrVal is the same as xerrCnt. */
    errCount = cerrVal + uerrVal;

    counters->errors    += errCount;
    counters->transient += errCount;

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeTcnSramErr */




/*****************************************************************************/
/** DecodeSchedConfigSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_CONFIG_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedConfigSramErr(fm_switch *     switchPtr,
                                          errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_CONFIG_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_SCHED_CONFIG_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_SCHED_CONFIG_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_SCHED_CONFIG_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    counters->errors     += errCount;
    counters->repairable += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    if (uerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_RX_SCHEDULE, TRUE, 0);
    }
    else if (cerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_RX_SCHEDULE, FALSE, 0);
    }

    if (uerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_TX_SCHEDULE, TRUE, 0);
    }
    else if (cerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_TX_SCHEDULE, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedConfigSramErr */




/*****************************************************************************/
/** DecodeSchedEschedSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_ESCHED_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedEschedSramErr(fm_switch *     switchPtr,
                                          errorCounters * counters)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_SCHED_ESCHED_SRAM_CTRL(0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_SCHED_ESCHED_SRAM_CTRL(0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_ESCHED_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_ESCHED_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);
    counters->errors += errCount;

    errCount = CountBits(cerrVal & (IDX0 | IDX1 | IDX2));
    counters->repairable += errCount;

    errCount = CountBits(cerrVal & (IDX3 | IDX4 | IDX5 | IDX6));
    counters->transient += errCount;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        /* NOTE: screened in fm10000ParityErrorDecoder */
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, 1, 0);
        goto ABORT;
    }

    /**************************************************
     * Process correctable errors.
     **************************************************/

    if (cerrVal & IDX0)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_ESCHED_CFG_1, FALSE, 0);
    }

    if (cerrVal & IDX1)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_ESCHED_CFG_2, FALSE, 0);
    }

    if (cerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_ESCHED_CFG_3, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedEschedSramErr */




/*****************************************************************************/
/** DecodeSchedFifoSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_FIFO_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedFifoSramErr(fm_switch *     switchPtr,
                                        errorCounters * counters)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   cerrCnt;
    fm_uint32   uerrCnt;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_SCHED_FIFO_SRAM_CTRL(0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_SCHED_FIFO_SRAM_CTRL(0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_FIFO_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_FIFO_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    cerrCnt = CountBits(cerrVal);
    uerrCnt = CountBits(uerrVal);

    counters->errors    += cerrCnt + uerrCnt;
    counters->transient += cerrCnt;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        /* NOTE: screened in fm10000ParityErrorDecoder */
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, uerrCnt, 0);
        goto ABORT;
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedFifoSramErr */




/*****************************************************************************/
/** DecodeSchedFreelistSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_FREELIST_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedFreelistSramErr(fm_switch *     switchPtr,
                                            errorCounters * counters)
{
    fm_uint32   regVal;
    fm_status   err;
    fm_uint32   cerrVal;
    fm_uint32   uerrVal;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_FREELIST_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_SCHED_FREELIST_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_BIT(regVal, FM10000_SCHED_FREELIST_SRAM_CTRL, CErr);
    uerrVal = FM_GET_BIT(regVal, FM10000_SCHED_FREELIST_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    /* There's only one error bit, so xerrVal is the same as xerrCnt. */
    counters->errors     += cerrVal + uerrVal;
    counters->transient  += cerrVal;
    counters->cumulative += uerrVal;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        fm_uint32   errorThresh;
        fm_uint32   fatalThresh;

        errorThresh =
            fmGetIntApiProperty(FM_AAK_API_FM10000_PARITY_FREELIST_ERROR_THRESH,
                                FM_AAD_API_FM10000_PARITY_FREELIST_ERROR_THRESH);

        fatalThresh =
            fmGetIntApiProperty(FM_AAK_API_FM10000_PARITY_FREELIST_FATAL_THRESH,
                                FM_AAD_API_FM10000_PARITY_FREELIST_FATAL_THRESH);

        err = DeferredReset(switchPtr,
                            FM_PARITY_AREA_SCHEDULER,
                            FM_CTR_PARITY_AREA_FREELIST,
                            errorThresh,
                            fatalThresh,
                            FM10000_SRAM_SCHED_FREELIST);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedFreelistSramErr */




/*****************************************************************************/
/** DecodeSchedMonitorSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_MONITOR_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedMonitorSramErr(fm_switch *     switchPtr,
                                           errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint16   fatalVal;
    fm_uint32   errCount;
    fm_int      sramNo;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_MONITOR_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_SCHED_MONITOR_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_SCHED_MONITOR_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_SCHED_MONITOR_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    errCount = CountBits(cerrVal) + CountBits(uerrVal);
    counters->errors += errCount;

    errCount = CountBits(cerrVal & IDX2) + CountBits(uerrVal & IDX2);
    counters->repairable += errCount;

    errCount = CountBits(cerrVal & (IDX0 | IDX1 | IDX3));
    counters->transient += errCount;

    /**************************************************
     * Decode and process errors.
     **************************************************/

    fatalVal = uerrVal & (IDX0 | IDX1 | IDX3);

    if (fatalVal)
    {
        errCount = CountBits(fatalVal);
        sramNo = MapSchedMonitorErrToSram(fatalVal);
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, errCount, sramNo);
        goto ABORT;
    }

    if (uerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_DRR_CFG, TRUE, 0);
    }
    else if (cerrVal & IDX2)
    {
        RequestRepair(sw, FM_REPAIR_SCHED_DRR_CFG, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedMonitorSramErr */




/*****************************************************************************/
/** DecodeSchedRxqSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_RXQ_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedRxqSramErr(fm_switch *     switchPtr,
                                       errorCounters * counters)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   cerrCnt;
    fm_uint32   uerrCnt;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_SCHED_RXQ_SRAM_CTRL(0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_SCHED_RXQ_SRAM_CTRL(0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_RXQ_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_RXQ_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    cerrCnt = CountBits(cerrVal);
    uerrCnt = CountBits(uerrVal);

    counters->errors += cerrCnt + uerrCnt;

    errCount = CountBits(cerrVal & (IDX0 | IDX1 | IDX2 | IDX3 | IDX6));
    counters->transient += errCount;

    errCount = CountBits(cerrVal & (IDX4 | IDX5));
    counters->repairable += errCount;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        /* NOTE: screened in fm10000ParityErrorDecoder */
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, uerrCnt, 0);
        goto ABORT;
    }

    /**************************************************
     * Process correctable errors.
     **************************************************/

    if (cerrVal & IDX4)
    {
        RequestRepair(sw, FM_REPAIR_MCAST_DEST_TABLE, FALSE, 0);
    }

    if (cerrVal & IDX5)
    {
        RequestRepair(sw, FM_REPAIR_MCAST_LEN_TABLE, FALSE, 0);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedRxqSramErr */




/*****************************************************************************/
/** DecodeSchedSschedSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_SSCHED_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedSschedSramErr(fm_switch *     switchPtr,
                                          errorCounters * counters)
{
    fm_uint32   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   cerrCnt;
    fm_uint32   uerrCnt;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT32(sw, FM10000_SCHED_SSCHED_SRAM_CTRL(), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT32(sw, FM10000_SCHED_SSCHED_SRAM_CTRL(), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD(regVal, FM10000_SCHED_SSCHED_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD(regVal, FM10000_SCHED_SSCHED_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    cerrCnt = CountBits(cerrVal);
    uerrCnt = CountBits(uerrVal);

    counters->errors    += cerrCnt + uerrCnt;
    counters->transient += cerrCnt;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        /* NOTE: screened in fm10000ParityErrorDecoder */
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, uerrCnt, 0);
        goto ABORT;
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedSschedSramErr */




/*****************************************************************************/
/** DecodeSchedTxqSramErr
 * \ingroup intParity
 *
 * \desc            Decodes SCHED_TXQ_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in,out]   counters points to the error counter structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeSchedTxqSramErr(fm_switch *     switchPtr,
                                       errorCounters * counters)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   cerrCnt;
    fm_uint32   uerrCnt;
    fm_status   err;
    fm_int      sw;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_SCHED_TXQ_SRAM_CTRL(0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_SCHED_TXQ_SRAM_CTRL(0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_TXQ_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_SCHED_TXQ_SRAM_CTRL, UErr);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "cerr=%04x uerr=%04x\n", cerrVal, uerrVal);

    /**************************************************
     * Get error counts.
     **************************************************/

    cerrCnt = CountBits(cerrVal);
    uerrCnt = CountBits(uerrVal);

    counters->errors    += cerrCnt + uerrCnt;
    counters->transient += cerrCnt;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        /* NOTE: screened in fm10000ParityErrorDecoder */
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, uerrCnt, 0);
        goto ABORT;
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeSchedTxqSramErr */




/*****************************************************************************/
/** DecodeTeSramErr
 * \ingroup intParity
 *
 * \desc            Decodes TE_SRAM_CTRL errors.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       teId is the Tunneling Engine instance number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeTeSramErr(fm_switch * switchPtr, fm_int teId)
{
    fm_uint64   regVal;
    fm_uint16   cerrVal;
    fm_uint16   uerrVal;
    fm_uint32   errCount;
    fm_status   err;
    fm_int      sw;
    fm_int      repairType;
    fm_bool     regLockTaken;

    sw = switchPtr->switchNumber;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    err = switchPtr->ReadUINT64(sw, FM10000_TE_SRAM_CTRL(teId, 0), &regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = switchPtr->WriteUINT64(sw, FM10000_TE_SRAM_CTRL(teId, 0), regVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    cerrVal = FM_GET_FIELD64(regVal, FM10000_TE_SRAM_CTRL, CErr);
    uerrVal = FM_GET_FIELD64(regVal, FM10000_TE_SRAM_CTRL, UErr);

    errCount = CountBits(cerrVal) + CountBits(uerrVal);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "teId=%d cerr=%04x uerr=%04x count=%u\n",
                 teId, cerrVal, uerrVal, errCount);

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_TUNNEL_ENGINE, errCount);

    repairType =
        (teId == 0) ? FM_REPAIR_TUNNEL_ENGINE_0 : FM_REPAIR_TUNNEL_ENGINE_1;

    /**************************************************
     * Process uncorrectable errors.
     **************************************************/

    if (uerrVal)
    {
        RequestRepair(sw, repairType, TRUE, uerrVal);
        cerrVal &= ~uerrVal;
    }

    /**************************************************
     * Process correctable errors.
     **************************************************/

    if (cerrVal)
    {
        RequestRepair(sw, repairType, FALSE, cerrVal);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    return err;

}   /* end DecodeTeSramErr */




/*****************************************************************************/
/** DecodeModify
 * \ingroup intParity
 *
 * \desc            Decodes MODIFY parity interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeModify(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           mod_ip;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    err = FM_OK;

    mod_ip = parityInfo->mod_ip;
    parityInfo->mod_ip = 0;

    /**************************************************
     * Handle state memory errors.
     **************************************************/

    if (mod_ip & FM10000_INT_MOD_Refcount_UERR)
    {
        fm_uint32   errorThresh;
        fm_uint32   fatalThresh;

        errorThresh =
            fmGetIntApiProperty(FM_AAK_API_FM10000_PARITY_REFCOUNT_ERROR_THRESH,
                                FM_AAD_API_FM10000_PARITY_REFCOUNT_ERROR_THRESH);

        fatalThresh =
            fmGetIntApiProperty(FM_AAK_API_FM10000_PARITY_REFCOUNT_FATAL_THRESH,
                                FM_AAD_API_FM10000_PARITY_REFCOUNT_FATAL_THRESH);

        err = DeferredReset(switchPtr,
                            FM_PARITY_AREA_MODIFY,
                            FM_CTR_PARITY_AREA_REFCOUNT,
                            errorThresh,
                            fatalThresh,
                            FM10000_SRAM_MOD_REFCOUNT);

        if (parityInfo->parityState >= FM10000_PARITY_STATE_FATAL)
        {
            goto ABORT;
        }
    }

    if (mod_ip & FM10000_INT_MOD_StatsByteCnt0_UERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_BYTE_0, TRUE, 0);
    }
    else if (mod_ip & FM10000_INT_MOD_StatsByteCnt0_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_BYTE_0, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_StatsByteCnt1_UERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_BYTE_1, TRUE, 0);
    }
    else if (mod_ip & FM10000_INT_MOD_StatsByteCnt1_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_BYTE_1, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_StatsFrameCnt0_UERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_FRAME_0, TRUE, 0);
    }
    else if (mod_ip & FM10000_INT_MOD_StatsFrameCnt0_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_FRAME_0, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_StatsFrameCnt1_UERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_FRAME_1, TRUE, 0);
    }
    else if (mod_ip & FM10000_INT_MOD_StatsFrameCnt1_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_STATS_BANK_FRAME_1, FALSE, 0);
    }

    /**************************************************
     * Handle config memory errors.
     **************************************************/

    if (mod_ip & FM10000_INT_MOD_McVlanTable_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MCAST_VLAN_TABLE, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_MirProfTable_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MIRROR_PROFILE_TABLE, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_PerPortCfg1_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_PER_PORT_CFG_1, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_PerPortCfg2_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_PER_PORT_CFG_2, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_Vid2Map_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_VID2_MAP, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_Vpri1Map_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_VPRI1_MAP, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_Vpri2Map_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_VPRI2_MAP, FALSE, 0);
    }

    if (mod_ip & FM10000_INT_MOD_VtagVid1Map_CERR)
    {
        RequestRepair(sw, FM_REPAIR_MOD_VLAN_TAG_VID1_MAP, FALSE, 0);
    }

ABORT:
    if (parityInfo->parityState < FM10000_PARITY_STATE_FATAL)
    {
        err = UpdateModifyIntMask(switchPtr, mod_ip);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end DecodeModify */




/*****************************************************************************/
/** DecodeFhHead
 * \ingroup intParity
 *
 * \desc            Decodes FH_HEAD parity interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFhHead(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    errorCounters       counters;
    fm_uint64           head_ip;
    fm_status           retStatus;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    retStatus = FM_OK;

    head_ip = parityInfo->fh_head_ip;
    parityInfo->fh_head_ip = 0;

    FM_CLEAR(counters);

    if (head_ip & FM10000_INT_FH_HEAD_ArpSramErr)
    {
        err = DecodeArpSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_FFUSramErr)
    {
        err = DecodeFFUSramErr(switchPtr, head_ip, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_FGLSramErr)
    {
        err = DecodeFGLSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_FhHeadOutputFifoSramErr)
    {
        err = DecodeFhHeadOutputFifoSramErr(switchPtr, head_ip, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_GlortRamSramErr)
    {
        err = DecodeGlortRamSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_GlortTableSramErr)
    {
        err = DecodeGlortTableSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_MapperSramErr)
    {
        err = DecodeMapperSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_MaTableSramErr)
    {
        err = DecodeMaTableSramErr(switchPtr, head_ip, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_ParserEarlySramErr)
    {
        err = DecodeParserEarlySramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_ParserLateSramErr)
    {
        err = DecodeParserLateSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (head_ip & FM10000_INT_FH_HEAD_VlanSramErr)
    {
        err = DecodeVlanSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->parityState < FM10000_PARITY_STATE_FATAL)
    {
        err = UpdateFhHeadIntMask(switchPtr, head_ip);
        FM_ERR_COMBINE(retStatus, err);
    }

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_FH_HEAD, counters.errors);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, counters.transient);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_REPAIRABLE, counters.repairable);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_CUMULATIVE, counters.cumulative);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end DecodeFhHead */




/*****************************************************************************/
/** DecodeFhTail
 * \ingroup intParity
 *
 * \desc            Decodes FH_TAIL parity interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFhTail(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    errorCounters       counters;
    fm_uint32           tail_ip;
    fm_status           retStatus;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    retStatus = FM_OK;

    tail_ip = parityInfo->fh_tail_ip;
    parityInfo->fh_tail_ip = 0;

    FM_CLEAR(counters);

    if (tail_ip & FM10000_INT_FH_TAIL_EgressPauseSramErr)
    {
        err = DecodeEgressPauseSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (tail_ip & FM10000_INT_FH_TAIL_PolicerUsageSramErr)
    {
        err = DecodePolicerUsageSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (tail_ip & FM10000_INT_FH_TAIL_RxStatsSramErr)
    {
        err = DecodeRxStatsSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (tail_ip & FM10000_INT_FH_TAIL_SafSramErr)
    {
        err = DecodeSafSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (tail_ip & FM10000_INT_FH_TAIL_TcnSramErr)
    {
        err = DecodeTcnSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->parityState < FM10000_PARITY_STATE_FATAL)
    {
        err = UpdateFhTailIntMask(switchPtr, tail_ip);
        FM_ERR_COMBINE(retStatus, err);
    }

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_FH_TAIL, counters.errors);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, counters.transient);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_REPAIRABLE, counters.repairable);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_CUMULATIVE, counters.cumulative);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end DecodeFhTail */




/*****************************************************************************/
/** DecodeFrameMemoryErr
 * \ingroup intParity
 *
 * \desc            Decodes FRAME_MEMORY parity interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeFrameMemoryErr(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           sram_ip;
    fm_uint32           errCount;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    sram_ip = parityInfo->sram_ip;
    parityInfo->sram_ip = 0;

    /**************************************************
     * Errors in frame memory are transient, so we
     * just count the occurrences and continue. Note 
     * that the interrupt register is already unmasked. 
     **************************************************/

    errCount = CountBits(sram_ip);

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_ARRAY, errCount);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, errCount);

    parityInfo->sramErrHistory |= sram_ip;

    return FM_OK;

}   /* end DecodeFrameMemoryErr */




/*****************************************************************************/
/** DecodeScheduler
 * \ingroup intParity
 *
 * \desc            Decodes SCHEDULER parity interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeScheduler(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    errorCounters       counters;
    fm_uint32           sched_ip;
    fm_status           retStatus;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    retStatus = FM_OK;

    sched_ip = parityInfo->sched_ip;
    parityInfo->sched_ip = 0;

    FM_CLEAR(counters);

    /**************************************************
     * Handle SCHEDULER errors.
     **************************************************/

    if (sched_ip & FM10000_INT_SCHED_SchedConfigSramErr)
    {
        err = DecodeSchedConfigSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_EschedSramErr)
    {
        err = DecodeSchedEschedSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_FifoSramErr)
    {
        err = DecodeSchedFifoSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_FreelistSramErr)
    {
        err = DecodeSchedFreelistSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_MonitorSramErr)
    {
        err = DecodeSchedMonitorSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_RxqMcastSramErr)
    {
        err = DecodeSchedRxqSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_SschedSramErr)
    {
        err = DecodeSchedSschedSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (sched_ip & FM10000_INT_SCHED_TxqSramErr)
    {
        err = DecodeSchedTxqSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->parityState < FM10000_PARITY_STATE_FATAL)
    {
        err = UpdateSchedulerIntMask(switchPtr, sched_ip);
        FM_ERR_COMBINE(retStatus, err);
    }

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_SCHEDULER, counters.errors);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, counters.transient);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_REPAIRABLE, counters.repairable);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_CUMULATIVE, counters.cumulative);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end DecodeScheduler */




/*****************************************************************************/
/** DecodeTunnelEngine
 * \ingroup intParity
 *
 * \desc            Decodes TUNNEL_ENGINE parity interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 * 
 * \param[in]       teId is the Tunneling Engine instance number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeTunnelEngine(fm_switch * switchPtr, fm_int teId)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           te_ip;
    fm_status           retStatus;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d teId=%d\n", sw, teId);

    retStatus = FM_OK;

    te_ip = parityInfo->te_ip[teId];
    parityInfo->te_ip[teId] = 0;

    if (te_ip & FM10000_INT_TE_TeSramErr)
    {
        err = DecodeTeSramErr(switchPtr, teId);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->parityState < FM10000_PARITY_STATE_FATAL)
    {
        err = UpdateTeIntMask(switchPtr, teId, te_ip);
        FM_ERR_COMBINE(retStatus, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end DecodeTunnelEngine */




/*****************************************************************************/
/** DecodeCrm
 * \ingroup intParity
 *
 * \desc            Decodes CRM interrupts.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DecodeCrm(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    errorCounters       counters;
    fm_uint64           crmInt;
    fm_uint32           crmErr;
    fm_uint32           sliceMask;
    fm_uint32           errCount;
    fm_status           retStatus;
    fm_int              sw;
    fm_int              sliceId;
    fm_uint32           bitMask;
    fm_int              crmId;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    FM_CLEAR(counters);

    retStatus = FM_OK;

    /**************************************************
     * Decode CRM_IP interrupt bits.
     **************************************************/

    crmInt = FM_ARRAY_GET_FIELD64(parityInfo->crm_ip,
                                  FM10000_CRM_IP,
                                  InterruptPending);

    crmErr = FM_ARRAY_GET_FIELD(parityInfo->crm_ip,
                                FM10000_CRM_IP,
                                SramErr);

    FM_CLEAR(parityInfo->crm_ip);

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "crmInt=%08llx crmErr=%d\n",
                 crmInt,
                 crmErr);

    /**************************************************
     * Handle FFU_SLICE_TCAM checksum errors.
     **************************************************/

    sliceMask = (fm_uint32)(crmInt & CRM_FFU_SLICE_TCAM_INT_MASK);
    if (sliceMask && switchExt->isCrmStarted)
    {
        errCount = CountBits(sliceMask);

        counters.errors     += errCount;
        counters.repairable += errCount;

        for (sliceId = 0 ; sliceId < FM10000_FFU_SLICE_SRAM_ENTRIES_1 ; sliceId++)
        {
            bitMask = 1 << sliceId;
            if(bitMask & sliceMask)
            {
                crmId = FM10000_FFU_SLICE_CRM_ID(sliceId);
                FM_LOG_ERROR(FM_LOG_CAT_PARITY, "FM10000_CRM_EVENT_FAULT_IND for crmId %d\n", crmId);
                retStatus = NotifyCRMEvent(sw,  crmId, FM10000_CRM_EVENT_FAULT_IND);
            }
        }
        RequestRepair(sw, FM_REPAIR_FFU_SLICE_TCAM, TRUE, sliceMask);
    }

    /**************************************************
     * Handle GLORT_CAM checksum errors.
     **************************************************/

    if ( (crmInt & CRM_GLORT_CAM_INT_MASK ) && switchExt->isCrmStarted )
    {
        counters.errors     += 1;
        counters.repairable += 1;

        FM_LOG_ERROR(FM_LOG_CAT_PARITY, "FM10000_CRM_EVENT_FAULT_IND for crmId %d\n", FM10000_CRM_EVENT_FAULT_IND);
        retStatus = NotifyCRMEvent(sw,  FM10000_GLORT_CAM_CRM_ID, FM10000_CRM_EVENT_FAULT_IND);
        RequestRepair(sw, FM_REPAIR_GLORT_CAM, TRUE, 0);
    }

#if 0
    /**************************************************
     * Handle CRM parity errors.
     **************************************************/

    if (crmErr)
    {
        err = DecodeCrmSramErr(switchPtr, &counters);
        FM_ERR_COMBINE(retStatus, err);
    }
#endif

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_AREA_TCAM, counters.errors);
    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_REPAIRABLE, counters.repairable);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end DecodeCrm */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000ParityErrorDecoder
 * \ingroup intParity
 *
 * \desc            Second-stage parity interrupt handler.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ParityErrorDecoder(fm_switch * switchPtr)
{
    fm10000_switch *    switchExt;
    fm10000_parityInfo *parityInfo;
    fm_uint64           tempMask;
    fm_int              sramNo;
    fm_status           retStatus;
    fm_status           err;
    fm_int              sw;

    switchExt  = switchPtr->extension;
    parityInfo = &switchExt->parityInfo;
    sw = switchPtr->switchNumber;

    /* Return silently if we're not in decode state. */
    if (parityInfo->parityState != FM10000_PARITY_STATE_DECODE)
    {
        return FM_OK;
    }

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    retStatus = FM_OK;
    err = FM_OK;

    /**************************************************
     * Update error counts that can be determined from
     * the IP registers.
     **************************************************/

    UpdateErrorCounts(sw);

    /**************************************************
     * Some fatal error conditions can be detected
     * from the IP register alone. If we can, do so
     * now, and return without further processing.
     **************************************************/

    if (parityInfo->core_int & FM10000_INT_CORE_CROSSBAR_ERR)
    {
        sramNo = FM10000_SRAM_CROSSBAR;
        err = FatalError(switchPtr, FM_PARITY_AREA_CROSSBAR, 0, sramNo);
        goto ABORT;
    }

    tempMask = parityInfo->mod_ip & FM10000_INT_MOD_FATAL_ERR;

    if (tempMask)
    {
        sramNo = MapModIpToSram(tempMask);
        err = FatalError(switchPtr, FM_PARITY_AREA_MODIFY, 0, sramNo);
        goto ABORT;
    }

    tempMask = parityInfo->sched_ip & FM10000_INT_SCHED_FATAL_ERR;

    if (tempMask)
    {
        sramNo = MapSchedIpToSram(tempMask);
        err = FatalError(switchPtr, FM_PARITY_AREA_SCHEDULER, 0, sramNo);
        goto ABORT;
    }

    /**************************************************
     * Decode parity interrupts, map them to the
     * underlying SRAMs, and post requests to the 
     * sweeper task for corrective action. 
     **************************************************/

    if (parityInfo->sram_ip)
    {
        err = DecodeFrameMemoryErr(switchPtr);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->mod_ip)
    {
        err = DecodeModify(switchPtr);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->fh_head_ip)
    {
        err = DecodeFhHead(switchPtr);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->fh_tail_ip)
    {
        err = DecodeFhTail(switchPtr);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->sched_ip)
    {
        err = DecodeScheduler(switchPtr);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->te_ip[0])
    {
        err = DecodeTunnelEngine(switchPtr, 0);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->te_ip[1])
    {
        err = DecodeTunnelEngine(switchPtr, 1);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (parityInfo->crm_ip[0] || parityInfo->crm_ip[1] || parityInfo->crm_ip[2])
    {
        err = DecodeCrm(switchPtr);
        FM_ERR_COMBINE(retStatus, err);
    }

    parityInfo->parityState = FM10000_PARITY_STATE_INACTIVE;

ABORT:
    FM_ERR_COMBINE(retStatus, err);
    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end fm10000ParityErrorDecoder */

