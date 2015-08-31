/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_parity_repair.c
 * Creation Date:   September 11, 2014.
 * Description:     Parity error repair task.
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

#include <fm_sdk_fm10000_int.h>
#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define NUM_FFU_SLICES          FM10000_FFU_SLICE_SRAM_ENTRIES_1
#define NUM_RX_STATS_BANKS      FM10000_RX_STATS_BANK_ENTRIES_1

#define TE_SRAM_DATA            (1 << 0)
#define TE_SRAM_LOOKUP          (1 << 1)
#define TE_SRAM_SIP             (1 << 2)
#define TE_SRAM_VNI             (1 << 3)
#define TE_SRAM_PAYLOAD_FIFO    (1 << 4)
#define TE_SRAM_HEADER_FIFO     (1 << 5)
#define TE_SRAM_STATS           (1 << 6)
#define TE_SRAM_USED            (1 << 7)

#define MOD_STATS_QUANTUM       8

typedef struct _fm_regDesc
{
    const char *regName;
    fm_uint32   regAddr;
    fm_int      width;
    fm_int      entries;
    fm_int      stride;
    fm_int      quantum;

} fm_regDesc;


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




fm_status fm10000RepairArpTable(fm_int sw)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairArpTable(%d)\n",
                 sw);

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairArpTable */




fm_status fm10000RepairArpUsed(fm_int sw)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairArpUsed(%d)\n",
                 sw);

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairArpUsed */




fm_status fm10000RepairMaTable(fm_int sw, fm_int index)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairMaTable(%d, %d)\n",
                 sw,
                 index);

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairMaTable */




fm_status fm10000RepairPolicerState4K(fm_int sw, fm_int index, fm_bool isUerr)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairPolicerState4K(%d, %d, %s)\n",
                 sw,
                 index,
                 (isUerr) ? "UERR" : "CERR");

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairPolicerState4K */




fm_status fm10000RepairPolicerState512(fm_int sw, fm_int index, fm_bool isUerr)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairPolicerState512(%d, %d, %s)\n",
                 sw,
                 index,
                 (isUerr) ? "UERR" : "CERR");

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairPolicerState512 */




fm_status fm10000RepairRxStatsBank(fm_int sw, fm_int index, fm_bool isUerr)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairRxStatsBank(%d, %d, %s)\n",
                 sw,
                 index,
                 (isUerr) ? "UERR" : "CERR");

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairRxStatsBank */




fm_status fm10000RepairTeStats(fm_int sw, fm_int teId)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairTeStats(%d, %d)\n",
                 sw,
                 teId);

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairTeStats */




fm_status fm10000RepairTeUsed(fm_int sw, fm_int teId)
{

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "UNIMPLEMENTED: fm10000RepairTeUsed(%d, %d)\n",
                 sw,
                 teId);

    return FM_ERR_UNSUPPORTED;

}   /* end fm10000RepairTeUsed */




/*****************************************************************************/
/** RefreshRegisterTable
 * \ingroup intParity
 *
 * \desc            Refreshes the specified register table by reading it
 *                  and writing it back. A long as the memory only has
 *                  single-bit errors, ECC will ensure that the read values
 *                  are correct, and writing the values back should clear
 *                  any soft errors.
 * 
 *                  Note that this process will cause additional correctable
 *                  errors to be reported as the locations that have single-bit
 *                  errors are read. The caller will need to reset the CErr
 *                  indication in the SRAM bit control register (and possibly
 *                  the IP register) in order.
 * 
 *                  This technique has an inherent flaw: if an uncorrectable
 *                  error occurs during the read, the value read will be
 *                  incorrect, and that value will be written back to the
 *                  SRAM. The location will now have an incorrect value but
 *                  its ECC bits will be valid, so the value will appear to
 *                  be correct. This technique is therefore appropriate only
 *                  when no soft state is available, or when an uncorrectable
 *                  error will require a reset anyway.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       regDesc points to the register table descriptor.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RefreshRegisterTable(fm_int sw, const fm_regDesc * regDesc)
{
    fm_uint32   regVal[64];
    fm_switch * switchPtr;
    fm_int      index;
    fm_uint32   regAddr;
    fm_int      limit;
    fm_int      quantum;
    fm_int      nwords;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d regName=%s\n",
                 sw,
                 regDesc->regName);

    if (regDesc->stride != regDesc->width)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                     "%s: stride (%d) != width (%d)\n",
                     regDesc->regName,
                     regDesc->stride,
                     regDesc->width);
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_ASSERTION_FAILED);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    quantum = (fm_int)FM_NENTRIES(regVal) / regDesc->width;
    if (regDesc->quantum > 0 && regDesc->quantum < quantum)
    {
        quantum = regDesc->quantum;
    }

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "%s: addr %08x width %d entries %d quantum %d cycles %d last %d\n",
                 regDesc->regName,
                 regDesc->regAddr,
                 regDesc->width,
                 regDesc->entries,
                 quantum,
                 regDesc->entries / quantum,
                 regDesc->entries % quantum);

    for (index = 0 ; index < regDesc->entries ; index += quantum)
    {
        regAddr = regDesc->regAddr + (index * regDesc->stride);

        limit = index + quantum;
        if (limit > regDesc->entries)
        {
            limit = regDesc->entries;
        }
        nwords = (limit - index) * regDesc->width;

#if 0
        FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                     "regAddr %08x nwords %d\n",
                     regAddr, nwords);
#endif

        TAKE_REG_LOCK(sw);

        err = switchPtr->ReadUINT32Mult(sw, regAddr, nwords, regVal);

        if (err == FM_OK)
        {
            err = switchPtr->WriteUINT32Mult(sw, regAddr, nwords, regVal);
        }

        DROP_REG_LOCK(sw);

        if (err != FM_OK)
        {
            break;
        }

    }   /* end for (index = 0 ; index < maxIndex ; index += 32) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end RefreshRegisterTable */




/*****************************************************************************/
/** RestoreFromCache
 * \ingroup intParity
 *
 * \desc            Repairs the specified register table by rewriting its
 *                  contents from the software cache.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       regName is the register table name.
 *
 * \param[in]       regSet points to the register set descriptor.
 * 
 * \param[in]       index1 is the outer register table index.
 * 
 * \param[in]       nEntries is the number of register entries to write.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RestoreFromCache(fm_int                sw,
                                  const char *          regName,
                                  const fm_cachedRegs * regSet,
                                  fm_int                index1,
                                  fm_int                nEntries)
{
    fm_uint32   indices[FM_REGS_CACHE_MAX_INDICES];
    fm_status   err;

    if (regSet->nIndices == 2)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                     "Restoring %s[%d] from cache\n",
                     regName, index1);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_PARITY, "Restoring %s from cache\n", regName);
    }

    FM_CLEAR(indices);
    indices[1] = index1;

    err = fmRegCacheWriteFromCache(sw, regSet, indices, nEntries);

    if (err == FM_OK)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_STATUS_FIXED, 1);
    }
    else
    {
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_STATUS_FIX_FAILED, 1);
    }

    return err;

}   /* end RestoreFromCache */




/*****************************************************************************/
/** RepairFfuSliceSram
 * \ingroup intParity
 *
 * \desc            Repairs FFU_SLICE_SRAM memories.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       auxData points to the parity error repair data.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairFfuSliceSram(fm_int sw, fm10000_repairData * auxData)
{
    fm_status   retStatus;
    fm_status   err;
    fm_int      sliceId;
    fm_uint32   bitMask;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if 0
    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "errMask=%08x uerrMask=%08x\n",
                 auxData->errMask,
                 auxData->uerrMask);
#endif

    retStatus = FM_OK;

    for (sliceId = 0 ; sliceId < NUM_FFU_SLICES ; sliceId++)
    {
        bitMask = 1 << sliceId;

        if (auxData->errMask & bitMask)
        {
            /* Restore from register cache. */
            err = RestoreFromCache(sw,
                                   "FFU_SLICE_SRAM",
                                   &fm10000CacheFfuSliceSram,
                                   sliceId,
                                   FM10000_FFU_SLICE_SRAM_ENTRIES_0);
            FM_ERR_COMBINE(retStatus, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end RepairFfuSliceSram */




/*****************************************************************************/
/** RepairFfuSliceTcam
 * \ingroup intParity
 *
 * \desc            Repairs FFU_SLICE_TCAM memories.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       auxData points to the parity error repair data.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairFfuSliceTcam(fm_int sw, fm10000_repairData * auxData)
{
    fm_status          retStatus;
    fm_status          err;
    fm_int             sliceId;
    fm_uint32          bitMask;
    fm_int             crmId;
    fm10000_switch *   switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if 0
    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "errMask=%08x uerrMask=%08x\n",
                 auxData->errMask,
                 auxData->uerrMask);
#endif

    switchExt = GET_SWITCH_EXT(sw);
    retStatus = FM_OK;

    if (!switchExt->isCrmStarted)
    {
        retStatus = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, retStatus);
    }

    for (sliceId = 0 ; sliceId < NUM_FFU_SLICES ; sliceId++)
    {
        bitMask = 1 << sliceId;

        if (auxData->errMask & bitMask)
        {
            /* Restore from register cache. */
            err = RestoreFromCache(sw,
                                   "FFU_SLICE_TCAM",
                                   &fm10000CacheFfuSliceTcam,
                                   sliceId,
                                   FM10000_FFU_SLICE_TCAM_ENTRIES_0);
            FM_ERR_COMBINE(retStatus, err);

            crmId = FM10000_FFU_SLICE_CRM_ID(sliceId);

            err = NotifyCRMEvent(sw, crmId, FM10000_CRM_EVENT_REPAIR_IND);
            FM_ERR_COMBINE(retStatus, err);
        }
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end RepairFfuSliceTcam */




/*****************************************************************************/
/** RepairGlortRam
 * \ingroup intParity
 *
 * \desc            Repairs GLORT_RAM.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairGlortRam(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if (FM10000_USE_GLORT_RAM_CACHE)
    status = RestoreFromCache(sw,
                              "GLORT_RAM",
                              &fm10000CacheGlortRam,
                              0,
                              FM10000_GLORT_RAM_ENTRIES);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairGlortRam */




/*****************************************************************************/
/** RepairGlortCam
 * \ingroup intParity
 *
 * \desc            Repairs GLORT_CAM.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairGlortCam(fm_int sw)
{
    fm_status          status;
    fm_status          err;
    fm10000_switch *   switchExt;
    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);

    if (!switchExt->isCrmStarted)
    {
        status = FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, status);
    }

#if defined(FM10000_USE_GLORT_CAM_CACHE)

    status = RestoreFromCache(sw,
                              "GLORT_CAM",
                              &fm10000CacheGlortCam,
                              0,
                              FM10000_GLORT_CAM_ENTRIES);

    err = NotifyCRMEvent(sw, FM10000_GLORT_CAM_CRM_ID, FM10000_CRM_EVENT_REPAIR_IND);
    FM_ERR_COMBINE(status, err);

#else
    status = FM_ERR_UNSUPPORTED;
#endif

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairGlortCam */




/*****************************************************************************/
/** RepairGlortDestTable
 * \ingroup intParity
 *
 * \desc            Repairs GLORT_DEST_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairGlortDestTable(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc glortTable =
    {
        "GLORT_DEST_TABLE",
        FM10000_GLORT_DEST_TABLE(0, 0),
        FM10000_GLORT_DEST_TABLE_WIDTH,
        FM10000_GLORT_DEST_TABLE_ENTRIES,
        FM10000_GLORT_DEST_TABLE(1, 0) - FM10000_GLORT_DEST_TABLE(0, 0)
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        status = RefreshRegisterTable(sw, &glortTable);
    }
    else
    {
        /* to be implemented */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairGlortDestTable */




/*****************************************************************************/
/** RepairEgressMstTable
 * \ingroup intParity
 *
 * \desc            Repairs EGRESS_MST_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairEgressMstTable(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if (FM10000_USE_MST_TABLE_CACHE)
    status = RestoreFromCache(sw,
                              "EGRESS_MST_TABLE",
                              &fm10000CacheEgressMstTable,
                              0,
                              FM10000_EGRESS_MST_TABLE_ENTRIES);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairEgressMstTable */




/*****************************************************************************/
/** RepairEgressVidTable
 * \ingroup intParity
 *
 * \desc            Repairs EGRESS_VID_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairEgressVidTable(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "EGRESS_VID_TABLE",
                              &fm10000CacheEgressVidTable,
                              0,
                              FM10000_EGRESS_VID_TABLE_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairEgressVidTable */




/*****************************************************************************/
/** RepairIngressMstTable
 * \ingroup intParity
 *
 * \desc            Repairs INGRESS_MST_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the outer register table index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairIngressMstTable(fm_int sw, fm_int index)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d index=%d\n", sw, index);

#if (FM10000_USE_MST_TABLE_CACHE)
    status = RestoreFromCache(sw,
                              "INGRESS_MST_TABLE",
                              &fm10000CacheIngressMstTable,
                              index,
                              FM10000_INGRESS_MST_TABLE_ENTRIES_0);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairIngressMstTable */




/*****************************************************************************/
/** RepairIngressVidTable
 * \ingroup intParity
 *
 * \desc            Repairs INGRESS_VID_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairIngressVidTable(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "INGRESS_VID_TABLE",
                              &fm10000CacheIngressVidTable,
                              0,
                              FM10000_INGRESS_VID_TABLE_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairIngressVidTable */




/*****************************************************************************/
/** RepairMapVlan
 * \ingroup intParity
 *
 * \desc            Repairs FFU_MAP_VLAN table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairMapVlan(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "FFU_MAP_VLAN",
                              &fm10000CacheFfuMapVlan,
                              0,
                              FM10000_FFU_MAP_VLAN_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairMapVlan */




/*****************************************************************************/
/** RepairParserPortCfg2
 * \ingroup intParity
 *
 * \desc            Repairs PARSER_PORT_CFG_2.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairParserPortCfg2(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if (FM10000_USE_PORT_CFG_CACHE)
    status = RestoreFromCache(sw,
                              "PARSER_PORT_CFG_2",
                              &fm10000CacheParserPortCfg2,
                              0,
                              FM10000_PARSER_PORT_CFG_2_ENTRIES);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairParserPortCfg2 */




/*****************************************************************************/
/** RepairParserPortCfg3
 * \ingroup intParity
 *
 * \desc            Repairs PARSER_PORT_CFG_3.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairParserPortCfg3(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if (FM10000_USE_PORT_CFG_CACHE)
    status = RestoreFromCache(sw,
                              "PARSER_PORT_CFG_3",
                              &fm10000CacheParserPortCfg3,
                              0,
                              FM10000_PARSER_PORT_CFG_3_ENTRIES);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairParserPortCfg3 */




/*****************************************************************************/
/** RepairRxVpriMap
 * \ingroup intParity
 *
 * \desc            Repairs RX_VPRI_MAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairRxVpriMap(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "RX_VPRI_MAP",
                              &fm10000CacheRxVpriMap,
                              0,
                              FM10000_RX_VPRI_MAP_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairRxVpriMap */




/*****************************************************************************/
/** RepairPolicerCfg4K
 * \ingroup intParity
 *
 * \desc            Repairs POLICER_CFG_4K.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the outer register table index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairPolicerCfg4K(fm_int sw, fm_int index)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d index=%d\n", sw, index);

    status = RestoreFromCache(sw,
                              "POLICER_CFG_4K",
                              &fm10000CachePolicerCfg4k,
                              index,
                              FM10000_POLICER_CFG_4K_ENTRIES_0);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairPolicerCfg4K */




/*****************************************************************************/
/** RepairPolicerCfg512
 * \ingroup intParity
 *
 * \desc            Repairs POLICER_CFG_512.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       index is the outer register table index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairPolicerCfg512(fm_int sw, fm_int index)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d index=%d\n", sw, index);

    status = RestoreFromCache(sw,
                              "POLICER_CFG_512",
                              &fm10000CachePolicerCfg512,
                              index,
                              FM10000_POLICER_CFG_512_ENTRIES_0);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairPolicerCfg512 */




/*****************************************************************************/
/** RepairRxStatsBank
 * \ingroup intParity
 *
 * \desc            Repairs RX_STATS_BANK memories.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       auxData points to the parity error repair data.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairRxStatsBank(fm_int sw, fm10000_repairData * auxData)
{
    fm_status   retStatus;
    fm_status   err;
    fm_int      index;
    fm_uint32   bitMask;
    fm_bool     isUerr;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d errMask=%02x uerrMask=%02x\n",
                 sw,
                 auxData->errMask,
                 auxData->uerrMask);

    retStatus = FM_OK;

    for (index = 0 ; index < NUM_RX_STATS_BANKS ; index++)
    {
        bitMask = 1 << index;

        if (auxData->errMask & bitMask)
        {
            isUerr = (auxData->uerrMask & bitMask) != 0;

            err = fm10000RepairRxStatsBank(sw, index, isUerr);
            FM_ERR_COMBINE(retStatus, err);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end RepairRxStatsBank */




/*****************************************************************************/
/** RepairSafMatrix
 * \ingroup intParity
 *
 * \desc            Repairs SAF_MATRIX.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairSafMatrix(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "SAF_MATRIX",
                              &fm10000CacheSafMatrix,
                              0,
                              FM10000_SAF_MATRIX_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairSafMatrix */




/*****************************************************************************/
/** RepairMcastVlanTable
 * \ingroup intParity
 *
 * \desc            Repairs MOD_MCAST_VLAN_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairMcastVlanTable(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc mcastVlanTable =
    {
        "MOD_MCAST_VLAN_TABLE",
        FM10000_MOD_MCAST_VLAN_TABLE(0, 0),
        FM10000_MOD_MCAST_VLAN_TABLE_WIDTH,
        FM10000_MOD_MCAST_VLAN_TABLE_ENTRIES - 1,
        FM10000_MOD_MCAST_VLAN_TABLE(1, 0) - FM10000_MOD_MCAST_VLAN_TABLE(0, 0)
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        /* TODO: restore from soft state. */
        status = RefreshRegisterTable(sw, &mcastVlanTable);
    }
    else
    {
        /* Should never occur (immediate reset in decoder). */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairMcastVlanTable */




/*****************************************************************************/
/** RepairMirrorProfileTable
 * \ingroup intParity
 *
 * \desc            Repairs MOD_MIRROR_PROFILE_TABLE.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairMirrorProfileTable(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "MOD_MIRROR_PROFILE_TABLE",
                              &fm10000CacheModMirrorProfTable,
                              0,
                              FM10000_MOD_MIRROR_PROFILE_TABLE_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairMirrorProfileTable */




/*****************************************************************************/
/** RepairModPerPortCfg1
 * \ingroup intParity
 *
 * \desc            Repairs MOD_PER_PORT_CFG_1.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModPerPortCfg1(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if (FM10000_USE_PORT_CFG_CACHE)
    status = RestoreFromCache(sw,
                              "MOD_PER_PORT_CFG_1",
                              &fm10000CacheModPerPortCfg1,
                              0,
                              FM10000_MOD_PER_PORT_CFG_1_ENTRIES);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModPerPortCfg1 */




/*****************************************************************************/
/** RepairModPerPortCfg2
 * \ingroup intParity
 *
 * \desc            Repairs MOD_PER_PORT_CFG_2.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModPerPortCfg2(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

#if (FM10000_USE_PORT_CFG_CACHE)
    status = RestoreFromCache(sw,
                              "MOD_PER_PORT_CFG_2",
                              &fm10000CacheModPerPortCfg2,
                              0,
                              FM10000_MOD_PER_PORT_CFG_2_ENTRIES);
#else
    status = FM_ERR_UNSUPPORTED;
#endif

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModPerPortCfg2 */




/*****************************************************************************/
/** RepairModStatsBankByte0
 * \ingroup intParity
 *
 * \desc            Repairs MOD_STATS_BANK_BYTE[0].
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModStatsBankByte0(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc statsBankByte0 =
    {
        "MOD_STATS_BANK_BYTE[0]",
        FM10000_MOD_STATS_BANK_BYTE(0, 0, 0),
        FM10000_MOD_STATS_BANK_BYTE_WIDTH,
        FM10000_MOD_STATS_BANK_BYTE_ENTRIES_0,
        FM10000_MOD_STATS_BANK_BYTE(0, 1, 0) - FM10000_MOD_STATS_BANK_BYTE(0, 0, 0),
        MOD_STATS_QUANTUM
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        /* TODO: monitor and notify (in decoder?) */
        status = RefreshRegisterTable(sw, &statsBankByte0);
    }
    else
    {
        /* TODO: scan and recover */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModStatsBankByte0 */




/*****************************************************************************/
/** RepairModStatsBankByte1
 * \ingroup intParity
 *
 * \desc            Repairs MOD_STATS_BANK_BYTE[1].
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModStatsBankByte1(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc statsBankByte1 =
    {
        "MOD_STATS_BANK_BYTE[1]",
        FM10000_MOD_STATS_BANK_BYTE(1, 0, 0),
        FM10000_MOD_STATS_BANK_BYTE_WIDTH,
        FM10000_MOD_STATS_BANK_BYTE_ENTRIES_0,
        FM10000_MOD_STATS_BANK_BYTE(1, 1, 0) - FM10000_MOD_STATS_BANK_BYTE(1, 0, 0),
        MOD_STATS_QUANTUM
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        /* TODO: monitor and notify (in decoder?) */
        status = RefreshRegisterTable(sw, &statsBankByte1);
    }
    else
    {
        /* TODO: scan and recover */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModStatsBankByte1 */




/*****************************************************************************/
/** RepairModStatsBankFrame0
 * \ingroup intParity
 *
 * \desc            Repairs MOD_STATS_BANK_FRAME[0].
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModStatsBankFrame0(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc statsBankFrame0 =
    {
        "MOD_STATS_BANK_FRAME[0]",
        FM10000_MOD_STATS_BANK_FRAME(0, 0, 0),
        FM10000_MOD_STATS_BANK_FRAME_WIDTH,
        FM10000_MOD_STATS_BANK_FRAME_ENTRIES_0,
        FM10000_MOD_STATS_BANK_FRAME(0, 1, 0) - FM10000_MOD_STATS_BANK_FRAME(0, 0, 0),
        MOD_STATS_QUANTUM
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        /* TODO: monitor and notify (in decoder?) */
        status = RefreshRegisterTable(sw, &statsBankFrame0);
    }
    else
    {
        /* TODO: scan and recover */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModStatsBankFrame0 */




/*****************************************************************************/
/** RepairModStatsBankFrame1
 * \ingroup intParity
 *
 * \desc            Repairs MOD_STATS_BANK_FRAME[1].
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModStatsBankFrame1(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc statsBankFrame1 =
    {
        "MOD_STATS_BANK_FRAME[1]",
        FM10000_MOD_STATS_BANK_FRAME(1, 0, 0),
        FM10000_MOD_STATS_BANK_FRAME_WIDTH,
        FM10000_MOD_STATS_BANK_FRAME_ENTRIES_0,
        FM10000_MOD_STATS_BANK_FRAME(1, 1, 0) - FM10000_MOD_STATS_BANK_FRAME(1, 0, 0),
        MOD_STATS_QUANTUM
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        /* TODO: monitor and notify (in decoder?) */
        status = RefreshRegisterTable(sw, &statsBankFrame1);
    }
    else
    {
        /* TODO: scan and recover */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModStatsBankFrame1 */




/*****************************************************************************/
/** RepairModVid2Map
 * \ingroup intParity
 *
 * \desc            Repairs MOD_VID2_MAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModVid2Map(fm_int sw, fm_bool isUerr)
{
    fm_status   status;

    static const fm_regDesc modVid2Map =
    {
        "MOD_VID2_MAP",
        FM10000_MOD_VID2_MAP(0, 0),
        FM10000_MOD_VID2_MAP_WIDTH,
        FM10000_MOD_VID2_MAP_ENTRIES,
        FM10000_MOD_VID2_MAP(1, 0) - FM10000_MOD_VID2_MAP(0, 0)
    };

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    if (!isUerr)
    {
        /* NOTE: API does not maintain soft state. */
        status = RefreshRegisterTable(sw, &modVid2Map);
    }
    else
    {
        /* Should never occur (immediate reset in decoder). */
        status = FM_ERR_UNSUPPORTED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModVid2Map */




/*****************************************************************************/
/** RepairModVpri1Map
 * \ingroup intParity
 *
 * \desc            Repairs MOD_VPRI1_MAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModVpri1Map(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "MOD_VPRI1_MAP",
                              &fm10000CacheModVpri1Map,
                              0,
                              FM10000_MOD_VPRI1_MAP_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModVpri1Map */




/*****************************************************************************/
/** RepairModVpri2Map
 * \ingroup intParity
 *
 * \desc            Repairs MOD_VPRI2_MAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModVpri2Map(fm_int sw)
{
    fm_status   status;

    status = RestoreFromCache(sw,
                              "MOD_VPRI2_MAP",
                              &fm10000CacheModVpri2Map,
                              0,
                              FM10000_MOD_VPRI2_MAP_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModVpri2Map */




/*****************************************************************************/
/** RepairModVtagVid1Map
 * \ingroup intParity
 *
 * \desc            Repairs MOD_VLAN_TAG_VID1_MAP.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairModVtagVid1Map(fm_int sw)
{
    fm_status   status;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    status = RestoreFromCache(sw,
                              "MOD_VLAN_TAG_VID1_MAP",
                              &fm10000CacheModVlanTagVid1Map,
                              0,
                              FM10000_MOD_VLAN_TAG_VID1_MAP_ENTRIES);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, status);

}   /* end RepairModVtagVid1Map */




/*****************************************************************************/
/** RepairTunnelEngine
 * \ingroup intParity
 *
 * \desc            Repairs TUNNEL_ENGINE memories.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       teId is the tunneling engine identifier.
 * 
 * \param[in]       auxData points to the error-specific auxiliary repair data.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RepairTunnelEngine(fm_int               sw,
                                    fm_int               teId,
                                    fm10000_repairData * auxData)
{
    fm_status   retStatus;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d teId=%d\n", sw, teId);

    FM_LOG_DEBUG(FM_LOG_CAT_PARITY,
                 "errMask=%04x uerrMask=%04x\n",
                 auxData->errMask,
                 auxData->uerrMask);

    retStatus = FM_OK;

    if (auxData->errMask & TE_SRAM_DATA)
    {
        err = RestoreFromCache(sw,
                               "TE_DATA",
                               &fm10000CacheTeData,
                               teId,
                               FM10000_TE_DATA_ENTRIES_0);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (auxData->errMask & TE_SRAM_LOOKUP)
    {
        err = RestoreFromCache(sw,
                               "TE_LOOKUP",
                               &fm10000CacheTeLookup,
                               teId,
                               FM10000_TE_LOOKUP_ENTRIES_0);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (auxData->errMask & TE_SRAM_SIP)
    {
        err = RestoreFromCache(sw,
                               "TE_SIP",
                               &fm10000CacheTeSip,
                               teId,
                               FM10000_TE_SIP_ENTRIES_0);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (auxData->errMask & TE_SRAM_VNI)
    {
        err = RestoreFromCache(sw,
                               "TE_VNI",
                               &fm10000CacheTeVni,
                               teId,
                               FM10000_TE_VNI_ENTRIES_0);
        FM_ERR_COMBINE(retStatus, err);
    }

    if (auxData->errMask & TE_SRAM_PAYLOAD_FIFO)
    {
        /* Self-repair. */
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, 1);
    }

    if (auxData->errMask & TE_SRAM_HEADER_FIFO)
    {
        /* Self-repair. */
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, 1);
    }

    if (auxData->uerrMask & TE_SRAM_STATS)
    {
        err = fm10000RepairTeStats(sw, teId);
        FM_ERR_COMBINE(retStatus, err);
    }
    else if (auxData->errMask & TE_SRAM_STATS)
    {
        /* Self-repair. */
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, 1);
    }

    if (auxData->uerrMask & TE_SRAM_USED)
    {
        err = fm10000RepairTeUsed(sw, teId);
        FM_ERR_COMBINE(retStatus, err);
    }
    else if (auxData->errMask & TE_SRAM_USED)
    {
        /* Self-repair. */
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_SEVERITY_TRANSIENT, 1);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retStatus);

}   /* end RepairTunnelEngine */




/*****************************************************************************/
/** PerformRepair
 * \ingroup intParity
 *
 * \desc            Scans the bitmasks indicating which SRAMS need to be
 *                  repaired, processing each SRAM individually.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       switchProtected points to a Boolean that indicates
 *                  whether "sw" is protected.
 * 
 * \param[in]       eventHandler points to the event handler to which the
 *                  parity event should be sent.
 * 
 * \param[in]       repairType is the type of repair to perform.
 *
 * \param[in]       isUerr is TRUE if the error is uncorrectable.
 * 
 * \param[in]       auxData points to the repair-specific auxiliary data
 *                  structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status PerformRepair(fm_int               sw,
                               fm_bool *            switchProtected,
                               fm_thread *          eventHandler,
                               fm_int               repairType,
                               fm_bool              isUerr,
                               fm10000_repairData * auxData)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d repairType=%s (%s)\n",
                 sw,
                 fmRepairTypeToText(repairType),
                 (isUerr) ? "UERR" : "CERR");

    err = FM_ERR_UNSUPPORTED;

    switch (repairType)
    {
        case FM_REPAIR_TYPE_NONE:
            err = FM_OK;
            break;

        /******************************************
         * FH_HEAD memories.
         ******************************************/

        case FM_REPAIR_ARP_TABLE:
            err = fm10000RepairArpTable(sw);
            break;

        case FM_REPAIR_ARP_USED:
            err = fm10000RepairArpUsed(sw);
            break;

        case FM_REPAIR_EGRESS_MST_TABLE:
            err = RepairEgressMstTable(sw);
            break;

        case FM_REPAIR_EGRESS_VID_TABLE:
            err = RepairEgressVidTable(sw);
            break;

        case FM_REPAIR_FFU_SLICE_SRAM:
            err = RepairFfuSliceSram(sw, auxData);
            break;

        case FM_REPAIR_GLORT_RAM:
            err = RepairGlortRam(sw);
            break;

        case FM_REPAIR_GLORT_TABLE:
            err = RepairGlortDestTable(sw, isUerr);
            break;

        case FM_REPAIR_INGRESS_MST_TABLE_0:
            err = RepairIngressMstTable(sw, 0);
            break;

        case FM_REPAIR_INGRESS_MST_TABLE_1:
            err = RepairIngressMstTable(sw, 1);
            break;

        case FM_REPAIR_INGRESS_VID_TABLE:
            err = RepairIngressVidTable(sw);
            break;

        case FM_REPAIR_MA_TABLE_0:
            err = fm10000RepairMaTable(sw, 0);
            break;

        case FM_REPAIR_MA_TABLE_1:
            err = fm10000RepairMaTable(sw, 1);
            break;

        case FM_REPAIR_MAP_VLAN:
            err = RepairMapVlan(sw);
            break;

        case FM_REPAIR_PARSER_PORT_CFG_2:
            err = RepairParserPortCfg2(sw);
            break;

        case FM_REPAIR_PARSER_PORT_CFG_3:
            err = RepairParserPortCfg3(sw);
            break;

        case FM_REPAIR_RX_VPRI_MAP:
            err = RepairRxVpriMap(sw);
            break;

        /******************************************
         * FH_TAIL memories.
         ******************************************/

        case FM_REPAIR_POLICER_CFG_4K_0:
            err = RepairPolicerCfg4K(sw, 0);
            break;

        case FM_REPAIR_POLICER_CFG_4K_1:
            err = RepairPolicerCfg4K(sw, 1);
            break;

        case FM_REPAIR_POLICER_CFG_512_0:
            err = RepairPolicerCfg512(sw, 0);
            break;

        case FM_REPAIR_POLICER_CFG_512_1:
            err = RepairPolicerCfg512(sw, 1);
            break;

        case FM_REPAIR_POLICER_STATE_4K_0:
            err = fm10000RepairPolicerState4K(sw, 0, isUerr);
            break;

        case FM_REPAIR_POLICER_STATE_4K_1:
            err = fm10000RepairPolicerState4K(sw, 1, isUerr);
            break;

        case FM_REPAIR_POLICER_STATE_512_0:
            err = fm10000RepairPolicerState512(sw, 0, isUerr);
            break;

        case FM_REPAIR_POLICER_STATE_512_1:
            err = fm10000RepairPolicerState512(sw, 1, isUerr);
            break;

        case FM_REPAIR_RX_STATS_BANK:
            err = RepairRxStatsBank(sw, auxData);
            break;

        case FM_REPAIR_SAF_MATRIX:
            err = RepairSafMatrix(sw);
            break;

        /******************************************
         * MODIFY memories.
         ******************************************/

        case FM_REPAIR_MCAST_VLAN_TABLE:
            err = RepairMcastVlanTable(sw, isUerr);
            break;

        case FM_REPAIR_MIRROR_PROFILE_TABLE:
            err = RepairMirrorProfileTable(sw);
            break;

        case FM_REPAIR_MOD_PER_PORT_CFG_1:
            err = RepairModPerPortCfg1(sw);
            break;

        case FM_REPAIR_MOD_PER_PORT_CFG_2:
            err = RepairModPerPortCfg2(sw);
            break;

        case FM_REPAIR_MOD_STATS_BANK_BYTE_0:
            err = RepairModStatsBankByte0(sw, isUerr);
            break;

        case FM_REPAIR_MOD_STATS_BANK_BYTE_1:
            err = RepairModStatsBankByte1(sw, isUerr);
            break;

        case FM_REPAIR_MOD_STATS_BANK_FRAME_0:
            err = RepairModStatsBankFrame0(sw, isUerr);
            break;

        case FM_REPAIR_MOD_STATS_BANK_FRAME_1:
            err = RepairModStatsBankFrame1(sw, isUerr);
            break;

        case FM_REPAIR_MOD_VID2_MAP:
            err = RepairModVid2Map(sw, isUerr);
            break;

        case FM_REPAIR_MOD_VPRI1_MAP:
            err = RepairModVpri1Map(sw);
            break;

        case FM_REPAIR_MOD_VPRI2_MAP:
            err = RepairModVpri2Map(sw);
            break;

        case FM_REPAIR_MOD_VLAN_TAG_VID1_MAP:
            err = RepairModVtagVid1Map(sw);
            break;

        /******************************************
         * SCHEDULER memories.
         ******************************************/

#if 0
        case FM_REPAIR_MCAST_DEST_TABLE:
        case FM_REPAIR_MCAST_LEN_TABLE:
        case FM_REPAIR_SCHED_DRR_CFG:
        case FM_REPAIR_SCHED_ESCHED_CFG_1:
        case FM_REPAIR_SCHED_ESCHED_CFG_2:
        case FM_REPAIR_SCHED_ESCHED_CFG_3:
        case FM_REPAIR_SCHED_RX_SCHEDULE:
        case FM_REPAIR_SCHED_TX_SCHEDULE:
#endif

        /******************************************
         * TUNNEL_ENGINE memories.
         ******************************************/

        case FM_REPAIR_TUNNEL_ENGINE_0:
            err = RepairTunnelEngine(sw, 0, auxData);
            break;

        case FM_REPAIR_TUNNEL_ENGINE_1:
            err = RepairTunnelEngine(sw, 1, auxData);
            break;

        /******************************************
         * TCAM memories.
         ******************************************/

        case FM_REPAIR_FFU_SLICE_TCAM:
            err = RepairFfuSliceTcam(sw, auxData);
            break;

        case FM_REPAIR_GLORT_CAM:
            err = RepairGlortCam(sw);
            break;

        default:
            break;

    }   /* end switch (repairType) */

    if (err == FM_ERR_UNSUPPORTED)
    {
        fmDbgDiagCountIncr(sw, FM_CTR_PARITY_REPAIR_INVALID, 1);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end PerformRepair */




/*****************************************************************************/
/** SweepPendingRepairs
 * \ingroup intParity
 *
 * \desc            Scans the bitmasks indicating which SRAMS need to be
 *                  repaired, processing each SRAM individually.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   switchProtected points to a Boolean that indicates
 *                  whether "sw" is protected.
 * 
 * \param[in]       eventHandler points to the event handler to which the
 *                  parity event should be sent.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SweepPendingRepairs(fm_int      sw,
                                     fm_bool *   switchProtected,
                                     fm_thread * eventHandler)
{
    fm10000_parityInfo *parityInfo;
    fm10000_repairData  auxData;
    fm_int              repairType;
    fm_uint64           bitMask;
    fm_bool             found;
    fm_bool             isUerr;

    static const fm10000_repairData ZERO_REPAIR_DATA = { 0 };

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    parityInfo = GET_PARITY_INFO(sw);

    if ( (parityInfo->parityState < FM10000_PARITY_STATE_FATAL) &&
         (parityInfo->pendingRepairs != 0) )
    {
        for (repairType = 0 ; repairType < FM_REPAIR_TYPE_MAX ; repairType++)
        {
            bitMask = FM_LITERAL_U64(1) << repairType;

            FM_CLEAR(auxData);

            TAKE_PARITY_LOCK(sw);

            found = (parityInfo->pendingRepairs & bitMask) != 0;

            if (found)
            {
                isUerr = (parityInfo->pendingUerrs & bitMask) != 0;

                parityInfo->pendingRepairs &= ~bitMask;
                parityInfo->pendingUerrs   &= ~bitMask;

                switch (repairType)
                {
                    case FM_REPAIR_FFU_SLICE_SRAM:
                        auxData = parityInfo->ffuRamRepair;
                        parityInfo->ffuRamRepair = ZERO_REPAIR_DATA;
                        break;

                    case FM_REPAIR_FFU_SLICE_TCAM:
                        auxData = parityInfo->ffuTcamRepair;
                        parityInfo->ffuTcamRepair = ZERO_REPAIR_DATA;
                        break;

                    case FM_REPAIR_RX_STATS_BANK:
                        auxData = parityInfo->rxStatsRepair;
                        parityInfo->rxStatsRepair = ZERO_REPAIR_DATA;
                        break;

                    case FM_REPAIR_TUNNEL_ENGINE_0:
                        auxData = parityInfo->teErrRepair[0];
                        parityInfo->teErrRepair[0] = ZERO_REPAIR_DATA;
                        break;

                    case FM_REPAIR_TUNNEL_ENGINE_1:
                        auxData = parityInfo->teErrRepair[1];
                        parityInfo->teErrRepair[1] = ZERO_REPAIR_DATA;
                        break;

                    default:
                        break;

                }   /* end switch (repairType) */

            }   /* end if (found) */

            DROP_PARITY_LOCK(sw);

            if (found)
            {
                PerformRepair(sw,
                              switchProtected,
                              eventHandler,
                              repairType,
                              isUerr,
                              &auxData);
            }

        }   /* end for (index = 0 ; index < FM_REPAIR_TYPE_MAX ; index++) */
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PARITY, FM_OK);

}   /* end SweepPendingRepairs */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fmRepairTypeToText
 * \ingroup intParity
 *
 * \desc            Returns the text representation of a repair type.
 *
 * \param[in]       repairType is the memory repair type (see ''fm_repairType'').
 *
 * \return          Pointer to a string representing the repair type.
 *
 *****************************************************************************/
const char * fmRepairTypeToText(fm_int repairType)
{

    switch (repairType)
    {
        case FM_REPAIR_TYPE_NONE:
            return "NONE";

        /**************************************************
         * FH_HEAD repairs.
         **************************************************/

        case FM_REPAIR_ARP_TABLE:
            return "ARP_TABLE";

        case FM_REPAIR_ARP_USED:
            return "ARP_USED";

        case FM_REPAIR_EGRESS_MST_TABLE:
            return "EGRESS_MST_TABLE";

        case FM_REPAIR_EGRESS_VID_TABLE:
            return "EGRESS_VID_TABLE";

        case FM_REPAIR_FFU_SLICE_SRAM:
            return "FFU_SLICE_RAM";

        case FM_REPAIR_GLORT_RAM:
            return "GLORT_RAM";

        case FM_REPAIR_GLORT_TABLE:
            return "GLORT_TABLE";

        case FM_REPAIR_INGRESS_MST_TABLE_0:
            return "INGRESS_MST_TABLE_0";

        case FM_REPAIR_INGRESS_MST_TABLE_1:
            return "INGRESS_MST_TABLE_1";

        case FM_REPAIR_INGRESS_VID_TABLE:
            return "INGRESS_VID_TABLE";

        case FM_REPAIR_MA_TABLE_0:
            return "MA_TABLE_0";

        case FM_REPAIR_MA_TABLE_1:
            return "MA_TABLE_1";

        case FM_REPAIR_MAP_VLAN:
            return "MAP_VLAN";

        case FM_REPAIR_PARSER_PORT_CFG_2:
            return "PARSER_PORT_CFG_2";

        case FM_REPAIR_PARSER_PORT_CFG_3:
            return "PARSER_PORT_CFG_3";

        case FM_REPAIR_RX_VPRI_MAP:
            return "RX_VPRI_MAP";

        /**************************************************
         * FH_TAIL repairs.
         **************************************************/

        case FM_REPAIR_POLICER_CFG_4K_0:
            return "POLICER_CFG_4K_0";

        case FM_REPAIR_POLICER_CFG_4K_1:
            return "POLICER_CFG_4K_1";

        case FM_REPAIR_POLICER_CFG_512_0:
            return "POLICER_CFG_512_0";

        case FM_REPAIR_POLICER_CFG_512_1:
            return "POLICER_CFG_512_1";

        case FM_REPAIR_POLICER_STATE_4K_0:
            return "POLICER_STATE_4K_0";

        case FM_REPAIR_POLICER_STATE_4K_1:
            return "POLICER_STATE_4K_1";

        case FM_REPAIR_POLICER_STATE_512_0:
            return "POLICER_STATE_512_0";

        case FM_REPAIR_POLICER_STATE_512_1:
            return "POLICER_STATE_512_1";

        case FM_REPAIR_RX_STATS_BANK:
            return "RX_STATS_BANK";

        case FM_REPAIR_SAF_MATRIX:
            return "SAF_MATRIX";

        /**************************************************
         * MODIFY repairs.
         **************************************************/

        case FM_REPAIR_MCAST_VLAN_TABLE:
            return "MCAST_VLAN_TABLE";

        case FM_REPAIR_MIRROR_PROFILE_TABLE:
            return "MIRROR_PROFILE_TABLE";

        case FM_REPAIR_MOD_PER_PORT_CFG_1:
            return "MOD_PER_PORT_CFG_1";

        case FM_REPAIR_MOD_PER_PORT_CFG_2:
            return "MOD_PER_PORT_CFG_2";

        case FM_REPAIR_MOD_STATS_BANK_BYTE_0:
            return "MOD_STATS_BANK_BYTE_0";

        case FM_REPAIR_MOD_STATS_BANK_BYTE_1:
            return "MOD_STATS_BANK_BYTE_1";

        case FM_REPAIR_MOD_STATS_BANK_FRAME_0:
            return "MOD_STATS_BANK_FRAME_0";

        case FM_REPAIR_MOD_STATS_BANK_FRAME_1:
            return "MOD_STATS_BANK_FRAME_1";

        case FM_REPAIR_MOD_VID2_MAP:
            return "MOD_VID2_MAP";

        case FM_REPAIR_MOD_VLAN_TAG_VID1_MAP:
            return "MOD_VLAN_TAG_VID1_MAP";

        case FM_REPAIR_MOD_VPRI1_MAP:
            return "MOD_VPRI1_MAP";

        case FM_REPAIR_MOD_VPRI2_MAP:
            return "MOD_VPRI2_MAP";

        /**************************************************
         * SCHEDULER repairs.
         **************************************************/

        case FM_REPAIR_MCAST_DEST_TABLE:
            return "MCAST_DEST_TABLE";

        case FM_REPAIR_MCAST_LEN_TABLE:
            return "MCAST_LEN_TABLE";

        case FM_REPAIR_SCHED_DRR_CFG:
            return "SCHED_DRR_CFG";

        case FM_REPAIR_SCHED_ESCHED_CFG_1:
            return "SCHED_ESCHED_CFG_1";

        case FM_REPAIR_SCHED_ESCHED_CFG_2:
            return "SCHED_ESCHED_CFG_2";

        case FM_REPAIR_SCHED_ESCHED_CFG_3:
            return "SCHED_ESCHED_CFG_3";

        case FM_REPAIR_SCHED_RX_SCHEDULE:
            return "SCHED_RX_SCHEDULE";

        case FM_REPAIR_SCHED_TX_SCHEDULE:
            return "SCHED_TX_SCHEDULE";

        /**************************************************
         * TUNNEL_ENGINE repairs.
         **************************************************/

        case FM_REPAIR_TUNNEL_ENGINE_0:
            return "TUNNEL_ENGINE_0";

        case FM_REPAIR_TUNNEL_ENGINE_1:
            return "TUNNEL_ENGINE_1";

        /**************************************************
         * TCAM repairs.
         **************************************************/

        case FM_REPAIR_FFU_SLICE_TCAM:
            return "FFU_SLICE_TCAM";

        case FM_REPAIR_GLORT_CAM:
            return "GLORT_CAM";

        default:
            return "UNKNOWN";

    }   /* end switch (repairType) */

}   /* end fmRepairTypeToText */




/*****************************************************************************/
/** fm10000ParityRepairTask
 * \ingroup intSwitch
 *
 * \desc            Switch-specific parity repair task.
 *                  Called via the ParityRepairTask function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   switchProtected points to a Boolean that indicates
 *                  whether "sw" is currently protected.
 *
 * \param[in]       args contains a pointer to the thread information.
 *
 * \return          NULL.
 *
 *****************************************************************************/
void * fm10000ParityRepairTask(fm_int    sw,
                               fm_bool * switchProtected,
                               void *    args)
{
    fm_thread * thread;
    fm_thread * eventHandler;

    thread       = FM_GET_THREAD_HANDLE(args);
    eventHandler = FM_GET_THREAD_PARAM(fm_thread, args);

    FM_NOT_USED(thread);    /* If logging is disabled, thread won't be used */

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    fmDbgDiagCountIncr(sw, FM_CTR_PARITY_REPAIR_DISPATCH, 1);

    SweepPendingRepairs(sw, switchProtected, eventHandler);

    FM_LOG_EXIT_CUSTOM_VERBOSE(FM_LOG_CAT_PARITY, NULL, "\n");

}   /* end fm10000ParityRepairTask */

