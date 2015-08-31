/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_crm.c
 * Creation Date:   February 18, 2015
 * Description:     FM10000 Counter Rate Monitor (CRM) support.
 *
 * Copyright (c) 2009 - 2015, Intel Corporation
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


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FFU_SLICE_TCAM_STRIDE0  \
        (FM10000_FFU_SLICE_TCAM(0, 1, 0) - FM10000_FFU_SLICE_TCAM(0, 0, 0))

#define FFU_SLICE_TCAM_STRIDE1  \
        (FM10000_FFU_SLICE_TCAM(1, 0, 0) - FM10000_FFU_SLICE_TCAM(0, 0, 0))

#define GLORT_CAM_STRIDE0   \
        (FM10000_GLORT_CAM(1) - FM10000_GLORT_CAM(0))


#define FM10000_CRM_SM_HISTORY_SIZE      16
#define FM10000_CRM_SM_RECORD_SIZE       sizeof(int)

/**************************************************
 * CRM Command Codes.
 **************************************************/
enum
{
    /** Initialize a memory block. Interrupt is set upon completion of
     *  initialization. */
    CRM_CMD_SET = 0,

    /** Copy a memory block from one location to another. Interrupt is set
     *  upon completion of copy. */
    CRM_CMD_COPY = 1,

    /** Compute Checksum. Read table and compute a XOR over the entire
     *  table. If the register size is greater than 32 bits, then each
     *  32-bit word is added to the XOR. Interrupt is set if the checksum
     *  computed doesn't match the checksum expected. */
    CRM_CMD_CHECKSUM = 7,

};


/**************************************************
 * CRM Entry Configuration.
 **************************************************/
typedef struct
{
    /* CRM_COMMAND */
    fm_uint command;
    fm_uint regCount;

    /* CRM_REGISTER */
    fm_uint baseAddr;
    fm_uint regSize;
    fm_uint size1Shift;
    fm_uint stride1Shift;
    fm_uint size2Shift;
    fm_uint stride2Shift;

    /* CRM_PARAM */
    fm_uint param;

    /* Miscellaneous */
    fm_uint entryCount;

} fm_crmCfg;

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_status StopCrm(fm_int sw);
static fm_status WriteCrmEntry(fm_int sw, fm_int crmId, fm_crmCfg * cfg);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** ComputeChecksum
 * \ingroup intSwitch
 *
 * \desc            Computes the checksum for a CRM monitor.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to update.
 *
 * \param[out]      checksum points to the location to receive the checksum.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 *****************************************************************************/
static fm_status ComputeChecksum(fm_int sw, fm_int crmId, fm_uint32 * checksum)
{
    fm_uint32   indices[FM_REGS_CACHE_MAX_INDICES];
    fm_status   err;

    FM_CLEAR(indices);

    *checksum = 0;

    if (crmId >= 0 && crmId < FM10000_GLORT_CAM_CRM_ID)
    {
        indices[1] = crmId;

        err = fmRegCacheComputeChecksum(sw,
                                        &fm10000CacheFfuSliceTcam,
                                        indices,
                                        FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                                        checksum);
    }
    else if (crmId == FM10000_GLORT_CAM_CRM_ID)
    {
        err = fmRegCacheComputeChecksum(sw,
                                        &fm10000CacheGlortCam,
                                        indices,
                                        FM10000_GLORT_CAM_ENTRIES,
                                        checksum);
    }
    else
    {
        err = FM_ERR_INVALID_CRM;
    }

    return err;

}   /* end ComputeChecksum */




/*****************************************************************************/
/** GetShiftValue
 * \ingroup intSwitch
 *
 * \desc            Converts integer to shift count.
 *
 * \param[in]       value is the integer to be converted.
 *
 * \return          Power of 2 such that 2^count <= value <= 2^(count+1).
 *
 *****************************************************************************/
static fm_uint32 GetShiftValue(fm_uint32 value)
{
    fm_uint32 shiftValue = 0;

    while (value >= 2)
    {
        value >>= 1;
        shiftValue++;
    }

    return shiftValue;

}   /* end GetShiftValue */




/*****************************************************************************/
/** InitConfig
 * \ingroup intSwitch
 *
 * \desc            Initializes the CRM configuration for the specified
 *                  register set.
 *
 * \param[out]      cfg points to the CRM configuration structure to be
 *                  initialized.
 *
 * \param[in]       command is the CRM command to be performed.
 *
 * \param[in]       baseAddr is the start address. It cannot be NULL.
 *
 * \param[in]       wordCount is the size of each entry, expressed in words.
 *                  It cannot be zero.
 *
 * \param[in]       blockSize1 is the size of the first dimension of the
 *                  array. It cannot be zero.
 *
 * \param[in]       blockSize2 is the size of the second dimension of the
 *                  the array. It may be zero in the case of one-dimensional
 *                  arrays, and in this case arraySize3 must be also zero.
 *
 * \param[in]       blockSize3 is the size of third dimension of the array. It
 *                  may be zero in the case of one- or two-dimensional
 *                  arrays.
 *
 * \param[in]       blockStride0 is the stride in the first dimension of the
 *                  array. It cannot be zero.
 *
 * \param[in]       blockStride1 is the stride in the second dimension of the
 *                  the array. It may be zero in the case of one-dimensional
 *                  arrays.
 *
 * \param[in]       blockStride2 is the stride of third dimension of the
 *                  array. It may be zero in the case of one- or two-
 *                  dimensional arrays.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on invalid arguments.
 *
 *****************************************************************************/
static fm_status InitConfig(fm_crmCfg * cfg,
                            fm_int      command,
                            fm_uint     baseAddr,
                            fm_uint     wordCount,
                            fm_uint     blockSize1,
                            fm_uint     blockSize2,
                            fm_uint     blockSize3,
                            fm_uint     blockStride0,
                            fm_uint     blockStride1,
                            fm_uint     blockStride2)
{
    fm_uint adjust;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM,
                 "command=%d "
                 "addr=0x%04x words=%u "
                 "size1=%u size2=%u size3=%u "
                 "stride0=%u stride1=%u stride2=%u\n",
                 command,
                 baseAddr,
                 wordCount,
                 blockSize1,
                 blockSize2,
                 blockSize3,
                 blockStride0,
                 blockStride1,
                 blockStride2);

    FM_CLEAR(*cfg);
    cfg->size1Shift = 0x0f;
    cfg->size2Shift = 0x0f;

    /* Argument validation */
    if (baseAddr == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_ERR_INVALID_ARGUMENT);
    }

    if (wordCount == 0 ||
        blockSize1 == 0 ||
        blockStride0 == 0 ||
        (blockSize2 == 0 && blockSize3 != 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_ERR_INVALID_ARGUMENT);
    }

    if (blockSize1 != 0 && blockSize2 == 0)
    {
        /***************************************************
         * One-dimensional array.
         **************************************************/

        if (wordCount == 3 && blockStride0 == 4)
        {
            /* Special case: if wordCount == 3 and blockStride0 == 4,
             * it is OK to set wordCount = 4. */
            wordCount = 4;
        }
        else if (blockStride0 != wordCount)
        {
            /* CRM is not optimized to process non-contiguous memory blocks */
            FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_ERR_UNSUPPORTED);
        }

        /* set the number of registers to initialize */
        cfg->entryCount = blockSize1;
    }
    else if (blockSize2 != 0 && blockSize3 == 0)
    {
        /***************************************************
         * Two-dimensional array.
         **************************************************/

        if (wordCount == 3 && blockStride0 == 4)
        {
            /* Special case: if wordCount == 3 and blockStride0 == 4,
             * it is OK to set wordCount = 4. */
            wordCount = 4;
        }
        else if (blockStride0 != wordCount)
        {
            /* CRM is not optimized to process non-contiguous memory blocks */
            FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_ERR_UNSUPPORTED);
        }

        /* compute the number of registers to initialize */
        cfg->entryCount = blockSize1 * blockSize2;

        /* express the first dimension as a power of 2 */
        cfg->size1Shift   = GetShiftValue(blockSize1);
        cfg->stride1Shift = GetShiftValue(blockStride1);

        /* check if the shift value must be adjusted */
        if (blockSize1 != (1U << cfg->size1Shift))
        {
            /* blockSize1 cannot be expressed as a power of 2.
             * Check if it is safe to use a rounded up value for
             * cfg->size1Shift. In order to do so, verify that there are no
             * gaps using the rounded up shift value. Otherwise, another table
             * could be overwritten. */
            adjust = 1 << (cfg->size1Shift + 1);

            if (adjust * wordCount == blockStride1)
            {
                /* It is safe to use the rounded up value */
                cfg->size1Shift++;

                /* recalculate cfg->entryCount using adjust */
                cfg->entryCount = adjust * blockSize2 - blockStride1 + blockSize1;
            }
            else
            {
                /* unsupported case */
                FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_ERR_UNSUPPORTED);
            }
        }
    }
    else
    {
        /***************************************************
         * Three-dimensional array.
         **************************************************/

        cfg->entryCount = blockSize1 * blockSize2 * blockSize3;

        /* express the dimensions as a power of 2 */
        cfg->size1Shift   = GetShiftValue(blockSize1);
        cfg->stride1Shift = GetShiftValue(blockStride1);
        cfg->size2Shift   = GetShiftValue(blockSize2);
        cfg->stride2Shift = GetShiftValue(blockStride2);

        /* Check if the shift value must be adjusted.
         * In the case of 3 dimensional tables, there
         * are a few cases where the 1st dimension cannot
         * be expressed as a power of 2. It is safe to
         * round up the exponent for these cases.
         * There are no problems with the 2nd and 3rd dimensions. */
        if (blockSize1 != (1U << cfg->size1Shift))
        {
            cfg->size1Shift++;
        }
    }

    cfg->command  = command;
    cfg->regCount = 1;
    cfg->baseAddr = baseAddr;
    cfg->regSize  = wordCount - 1;

    /* The stride is calculated by shifting a '1' one more than
     * the number of times specified by the strideShift field.
     * In consequence, the calculated strideShift value must be
     * decremented. */
    if (cfg->stride1Shift)
    {
        cfg->stride1Shift--;
    }

    if (cfg->stride2Shift)
    {
        cfg->stride2Shift--;
    }

    FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_OK);

}   /* end InitConfig */




/*****************************************************************************/
/** InitMonitors
 * \ingroup intCrm
 *
 * \desc            Initializes the CRM monitors.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       setChk is TRUE if the TCAM checksum should be computed,
 *                  or FALSE if it should be zeroed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitMonitors(fm_int sw, fm_bool setChk)
{
    fm10000_crmInfo *crmInfo;
    fm_crmCfg        cfg;
    fm_int           crmId;
    fm_status        err;
    fm_bool          isValid;
    fm_char          crmTimerName[16];
    fm10000_switch * switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM,
                 "sw=%d setChk=%s\n",
                 sw,
                 FM_BOOLSTRING(setChk));

    crmInfo = GET_CRM_INFO(sw);
    switchExt = GET_SWITCH_EXT(sw);

    FM_CLEAR(*crmInfo);

    FM_LOG_DEBUG(FM_LOG_CAT_CRM, "Initializing CRM state machines and timers for sw %d\n", sw);
    for (crmId = 0 ; crmId < FM10000_CRM_COMMAND_ENTRIES ; crmId++)
    {
        if (crmId < FM10000_GLORT_CAM_CRM_ID)
        {
            /**************************************************
             * Monitors 0..31 verify the FFU TCAM checksums.
             * Each monitor checks the corresponding slice.
             **************************************************/

            err = InitConfig(&cfg,
                             CRM_CMD_CHECKSUM,
                             FM10000_FFU_SLICE_TCAM(crmId,0,0),
                             FM10000_FFU_SLICE_TCAM_WIDTH,
                             FM10000_FFU_SLICE_TCAM_ENTRIES_0,
                             FM10000_FFU_SLICE_TCAM_ENTRIES_1,
                             0,
                             FFU_SLICE_TCAM_STRIDE0,
                             FFU_SLICE_TCAM_STRIDE1,
                             0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

            cfg.regCount = FM10000_FFU_SLICE_TCAM_ENTRIES_0;

            if (setChk)
            {
                err = ComputeChecksum(sw, crmId, &cfg.param);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
            }

            isValid = TRUE;

            err = fmCreateStateMachine( crmId,
                                        FM10000_CRM_SM_HISTORY_SIZE,
                                        FM10000_CRM_SM_RECORD_SIZE,
                                        &switchExt->crmInfo.crmSmHandles[crmId] );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

            FM_SPRINTF_S( crmTimerName,
                          sizeof(crmTimerName),
                          "crm%02%02dTimer",
                          sw,
                          crmId );
            err = fmCreateTimer( crmTimerName,
                                 fmApiTimerTask,
                                 &switchExt->crmInfo.crmTimers[crmId] );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

        }
        else if (crmId == FM10000_GLORT_CAM_CRM_ID)
        {
            /**************************************************
             * Monitor 32 verifies the GLORT CAM checksum.
             **************************************************/

            err = InitConfig(&cfg,
                             CRM_CMD_CHECKSUM,
                             FM10000_GLORT_CAM(0),
                             FM10000_GLORT_CAM_WIDTH,
                             FM10000_GLORT_CAM_ENTRIES,
                             0,
                             0,
                             GLORT_CAM_STRIDE0,
                             0,
                             0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

            cfg.regCount = FM10000_GLORT_CAM_ENTRIES;

            if (setChk)
            {
                err = ComputeChecksum(sw, crmId, &cfg.param);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
            }

            isValid = TRUE;

            err = fmCreateStateMachine( crmId,
                                        FM10000_CRM_SM_HISTORY_SIZE,
                                        FM10000_CRM_SM_RECORD_SIZE,
                                        &switchExt->crmInfo.crmSmHandles[crmId] );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

            FM_SPRINTF_S( crmTimerName,
                          sizeof(crmTimerName),
                          "crm%02d%02dTimer",
                          sw,
                          crmId );
            err = fmCreateTimer( crmTimerName,
                                 fmApiTimerTask,
                                 &switchExt->crmInfo.crmTimers[crmId] );
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

        }
        else
        {
            /**************************************************
             * Monitors 33..63 are cleared.
             **************************************************/

            FM_CLEAR(cfg);

            isValid = FALSE;
        }

        err = WriteCrmEntry(sw, crmId, &cfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

        if (isValid)
        {
            crmInfo->validMask |= FM_LITERAL_U64(1) << crmId;
        }

    }   /* end for (crmId = 0 ; crmId < FM10000_CRM_COMMAND_ENTRIES ; ...) */

    crmInfo->firstIdx = FM10000_FFU_SLICE_CRM_ID(0);
    crmInfo->lastIdx  = FM10000_GLORT_CAM_CRM_ID;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end InitMonitors */




/*****************************************************************************/
/** MaskInterrupts
 * \ingroup intCrm
 *
 * \desc            Masks or unmasks interrupts for one or more CRM monitors.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmMask is a bit mask specifying the CRM monitors
 *                  to be enabled or disabled.
 *
 * \param[in]       on should be set to TRUE to turn bits on in the mask
 *                  (disable interrupts) or FALSE to turn bits off in the
 *                  mask (enable interrupts).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 *****************************************************************************/
static fm_status MaskInterrupts(fm_int sw, fm_uint64 crmMask, fm_bool on)
{
    fm_switch * switchPtr;
    fm_uint32   crmVal[FM10000_CRM_IP_WIDTH];
    fm_uint64   intMask;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM,
                 "sw=%d crmMask=0x%08llx on=%s\n",
                 sw,
                 crmMask,
                 FM_BOOLSTRING(on));

    switchPtr = GET_SWITCH_PTR(sw);
    TAKE_REG_LOCK(sw);
    /***************************************************
     * Clear pending interrupts before unmasking them.
     **************************************************/

    if (!on)
    {
        FM_CLEAR(crmVal);

        FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IP, InterruptPending, crmMask);

        err = switchPtr->WriteUINT32Mult(sw,
                                         FM10000_CRM_IP(0),
                                         FM10000_CRM_IP_WIDTH,
                                         crmVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

    /***************************************************
     * Update interrupt mask.
     **************************************************/

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    intMask = FM_ARRAY_GET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask);

    if (on)
    {
        intMask |= crmMask;
    }
    else
    {
        intMask &= ~crmMask;
    }

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask, intMask);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    DROP_REG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end MaskInterrupts */




/*****************************************************************************/
/** ResetCrm
 * \ingroup intCrm
 *
 * \desc            Resets the CRM hardware.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ResetCrm(fm_int sw)
{
    fm_switch * switchPtr;
    fm_uint32   crmVal[FM10000_CRM_IP_WIDTH];
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Mask interrupts.
     **************************************************/

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask, ~0);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * Halt the CRM.
     **************************************************/

    (void)StopCrm(sw);

    /**************************************************
     * Acknowledge pending interrupts.
     **************************************************/

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IP(0),
                                     FM10000_CRM_IP_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * Reset CTRL and STATUS registers.
     **************************************************/

    err = switchPtr->WriteUINT32(sw, FM10000_CRM_CTRL(), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    err = switchPtr->WriteUINT32(sw, FM10000_CRM_STATUS(), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end ResetCrm */




/*****************************************************************************/
/** StartCrm
 * \ingroup intCrm
 *
 * \desc            Starts the Counter Rate Monitor.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       firstIdx is the first monitor index.
 *
 * \param[in]       lastIdx is the last monitor index.
 *
 * \param[in]       continuous is FALSE for single-step operation,
 *                  TRUE for continuous execution.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status StartCrm(fm_int  sw,
                          fm_int  firstIdx,
                          fm_int  lastIdx,
                          fm_bool continuous)
{
    fm_switch * switchPtr;
    fm_status   err;
    fm_uint32   rv;
    fm_int      i;
    fm_bool     regLockTaken;
    fm10000_switch * switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM,
                 "sw=%d firstIdx=%d lastIdx=%d continuous=%d\n",
                 sw,
                 firstIdx,
                 lastIdx,
                 continuous);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    /**************************************************
     * Verify that CRM is stopped.
     **************************************************/

    err = switchPtr->ReadUINT32(sw, FM10000_CRM_STATUS(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    if (FM_GET_BIT(rv, FM10000_CRM_STATUS, Running))
    {
        err = FM_OK;
        goto ABORT;
    }

    /**************************************************
     * Set current command index to first index.
     **************************************************/

    FM_SET_FIELD(rv, FM10000_CRM_STATUS, CommandIndex, firstIdx);

    err = switchPtr->WriteUINT32(sw, FM10000_CRM_STATUS(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * Specify range of counters.
     * Set to run continuously.
     * Issue Run command.
     **************************************************/

    rv = 0;

    FM_SET_BIT(rv, FM10000_CRM_CTRL, Run, 1);
    FM_SET_FIELD(rv, FM10000_CRM_CTRL, FirstCommandIndex, firstIdx);
    FM_SET_FIELD(rv, FM10000_CRM_CTRL, LastCommandIndex, lastIdx);
    FM_SET_BIT(rv, FM10000_CRM_CTRL, ContinuousRun, continuous);

    err = switchPtr->WriteUINT32(sw, FM10000_CRM_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * Wait for CRM to report that it is running.
     **************************************************/

    for (i = 0 ; i < 100 ; i++)
    {
        /* Wait 1 millisecond before polling. */
        fmDelay(0, 1000000);

        err = switchPtr->ReadUINT32(sw, FM10000_CRM_STATUS(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

        if (FM_GET_BIT(rv, FM10000_CRM_STATUS, Running))
        {
            FM_LOG_DEBUG(FM_LOG_CAT_CRM, "CRM running after %d cycles\n", i);
            err = FM_OK;
            break;
        }
    }

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    /**************************************************
     * Start CRM state machines
     **************************************************/
    for (i = firstIdx ; i <= lastIdx ; i++)
    {
        err = fmStartStateMachine(switchExt->crmInfo.crmSmHandles[i],
                                  FM10000_BASIC_CRM_STATE_MACHINE,
                                  FM10000_CRM_STATE_UPDATING);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

    switchExt->isCrmStarted = TRUE;

    for (i = firstIdx ; i <= lastIdx ; i++)
    {
        err = NotifyCRMEvent(sw, i, FM10000_CRM_EVENT_RESUME_REQ);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

    goto ABORT;

    /**************************************************
     * Error if CRM hasn't started.
     **************************************************/

    FM_LOG_ERROR(FM_LOG_CAT_CRM, "Timeout waiting for CRM to start!\n");
    err = FM_FAIL;

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }
    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end StartCrm */




/*****************************************************************************/
/** StopCrm
 * \ingroup intCrm
 *
 * \desc            Stops the Counter Rate Monitor.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status StopCrm(fm_int sw)
{
    fm_switch *      switchPtr;
    fm_status        err;
    fm_uint32        rv;
    fm_int           i;
    fm10000_switch * switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    /**************************************************
     * Verify that CRM is running.
     **************************************************/

    err = switchPtr->ReadUINT32(sw, FM10000_CRM_STATUS(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    if (!FM_GET_BIT(rv, FM10000_CRM_STATUS, Running))
    {
        err = FM_OK;
        goto ABORT;
    }

    /**************************************************
     * Issue Stop command.
     **************************************************/

    err = switchPtr->ReadUINT32(sw, FM10000_CRM_CTRL(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    FM_SET_BIT(rv, FM10000_CRM_CTRL, Run, 0);

    err = switchPtr->WriteUINT32(sw, FM10000_CRM_CTRL(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);


    /**************************************************
     * Stop CRM state machines.
     **************************************************/

    if (switchExt->isCrmStarted)
    {
        for (i = 0 ; i < FM10000_NUM_CRM_IDS ; i++)
        {
            err = fmStopStateMachine(switchExt->crmInfo.crmSmHandles[i]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
        }

        switchExt->isCrmStarted = FALSE;
    }

    /**************************************************
     * Wait for CRM to report that it is stopped.
     **************************************************/

    for (i = 0 ; i < 100 ; i++)
    {
        /* Wait 1 millisecond before polling. */
        fmDelay(0, 1000000);

        err = switchPtr->ReadUINT32(sw, FM10000_CRM_STATUS(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

        if (!FM_GET_BIT(rv, FM10000_CRM_STATUS, Running))
        {
            FM_LOG_DEBUG(FM_LOG_CAT_CRM, "CRM stopped after %d cycles\n", i);
            err = FM_OK;
            goto ABORT;
        }
    }

    /**************************************************
     * Error if CRM hasn't stopped.
     **************************************************/

    FM_LOG_ERROR(FM_LOG_CAT_CRM, "Timeout waiting for CRM to stop!\n");
    err = FM_FAIL;

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end StopCrm */




/*****************************************************************************/
/** UpdateChecksum
 * \ingroup intCrm
 *
 * \desc            Updates the checksum for a CRM monitor.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateChecksum(fm_int sw, fm_int crmId)
{
    fm_switch * switchPtr;
    fm_uint32   checksum;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d crmId=%d\n", sw, crmId);

    switchPtr = GET_SWITCH_PTR(sw);

    err = ComputeChecksum(sw, crmId, &checksum);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    FM_LOG_DEBUG(FM_LOG_CAT_CRM, "checksum=0x%08x\n", checksum);

    err = switchPtr->WriteUINT32(sw, FM10000_CRM_PARAM(crmId), checksum);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end UpdateChecksum */




/*****************************************************************************/
/** WriteCrmEntry
 * \ingroup intSwitch
 *
 * \desc            Writes a CRM monitor configuration to the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to configure.
 *
 * \param[in]       cfg points to a structure containing the configuration
 *                  of the CRM entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status WriteCrmEntry(fm_int      sw,
                               fm_int      crmId,
                               fm_crmCfg * cfg)
{
    fm_switch * switchPtr;
    fm_uint64   rv64;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM,
                 "sw=%d crmId=%d "
                 "command=%d count=%u "
                 "baseAddr=%08x param=%08x\n",
                 sw,
                 crmId,
                 cfg->command,
                 cfg->regCount,
                 cfg->baseAddr,
                 cfg->param);

    switchPtr = GET_SWITCH_PTR(sw);

    if (crmId < 0 || crmId >= FM10000_CRM_COMMAND_ENTRIES)
    {
        FM_LOG_EXIT(FM_LOG_CAT_CRM, FM_ERR_INVALID_CRM);
    }

    /**************************************************
     * CRM_COMMAND
     **************************************************/

    rv64 = 0;

    /* Command to execute. */
    FM_SET_FIELD64(rv64, FM10000_CRM_COMMAND, Command, cfg->command);

#if 0
    /* Pointer to data section. (not used) */
    FM_SET_FIELD64(rv64, FM10000_CRM_COMMAND, DataIndex, 0);
#endif

    /* Number of registers to walk through. */
    FM_SET_FIELD64(rv64, FM10000_CRM_COMMAND, Count, cfg->regCount);

    err = switchPtr->WriteUINT64(sw, FM10000_CRM_COMMAND(crmId, 0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * CRM_REGISTER
     **************************************************/

    rv64 = 0;

    /* First register address. */
    FM_SET_FIELD64(rv64, FM10000_CRM_REGISTER, BaseAddress, cfg->baseAddr);

    /* Register size. */
    FM_SET_FIELD64(rv64, FM10000_CRM_REGISTER, Size, cfg->regSize);

    /* Register block size 1. */
    FM_SET_FIELD64(rv64, FM10000_CRM_REGISTER, BlockSize1Shift, cfg->size1Shift);

    /* Stride to next block 1. */
    FM_SET_FIELD64(rv64, FM10000_CRM_REGISTER, Stride1Shift, cfg->stride1Shift);

    /* Register block size 2. */
    FM_SET_FIELD64(rv64, FM10000_CRM_REGISTER, BlockSize2Shift, cfg->size2Shift);

    /* Stride to next block 2. */
    FM_SET_FIELD64(rv64, FM10000_CRM_REGISTER, Stride2Shift, cfg->stride2Shift);

    err = switchPtr->WriteUINT64(sw, FM10000_CRM_REGISTER(crmId, 0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * CRM_PARAM
     **************************************************/

    /* Command parameter. */
    err = switchPtr->WriteUINT32(sw, FM10000_CRM_PARAM(crmId), cfg->param);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * CRM_PERIOD
     **************************************************/

    /* Run command at maximum rate. */
    err = switchPtr->WriteUINT64(sw, FM10000_CRM_PERIOD(crmId, 0), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end WriteCrmEntry */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000DisableCrmMonitor
 * \ingroup intCrm
 *
 * \desc            Disables a CRM monitor.
 *
 * \note            The caller is responsible for validating the switch
 *                  and taking the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to disable.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 *****************************************************************************/
fm_status fm10000DisableCrmMonitor(fm_int sw, fm_int crmId)
{
    fm_switch * switchPtr;
    fm_uint32   crmVal[FM10000_CRM_IP_WIDTH];
    fm_uint64   intMask;
    fm_status   err;
    fm_bool     regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d crmId=%d\n", sw, crmId);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    if (crmId < 0 || crmId > FM10000_GLORT_CAM_CRM_ID)
    {
        err = FM_ERR_INVALID_CRM;
        goto ABORT;
    }

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    intMask  = FM_ARRAY_GET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask);

    intMask |= FM_LITERAL_U64(1) << crmId;

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask, intMask);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    err = NotifyCRMEvent(sw, crmId, FM10000_CRM_EVENT_SUSPEND_REQ);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000DisableCrmMonitor */




/*****************************************************************************/
/** fm10000EnableCrmMonitor
 * \ingroup intCrm
 *
 * \desc            Enables a CRM monitor.
 *
 * \note            The caller is responsible for validating the switch
 *                  and taking the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to enable.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 *****************************************************************************/
fm_status fm10000EnableCrmMonitor(fm_int sw, fm_int crmId)
{
    fm_switch * switchPtr;
    fm_uint32   crmVal[FM10000_CRM_IP_WIDTH];
    fm_uint64   intMask;
    fm_uint64   crmMask;
    fm_status   err;
    fm_bool     regLockTaken;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d crmId=%d\n", sw, crmId);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);
    regLockTaken = TRUE;

    if (crmId < 0 || crmId > FM10000_GLORT_CAM_CRM_ID)
    {
        err = FM_ERR_INVALID_CRM;
        goto ABORT;
    }

    crmMask = FM_LITERAL_U64(1) << crmId;

    /***************************************************
     * Clear the Interrupt Pending bit.
     **************************************************/

    FM_CLEAR(crmVal);

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IP, InterruptPending, crmMask);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IP(0),
                                     FM10000_CRM_IP_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /***************************************************
     * Enable interrupts for monitor.
     **************************************************/

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    intMask = FM_ARRAY_GET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask);

    intMask &= ~crmMask;

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask, intMask);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    DROP_REG_LOCK(sw);
    regLockTaken = FALSE;

    err = NotifyCRMEvent(sw, crmId, FM10000_CRM_EVENT_RESUME_REQ);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000EnableCrmMonitor */




/*****************************************************************************/
/** fm10000InitCrm
 * \ingroup intCrm
 *
 * \desc            Initializes the Counter Rate Monitor subsystem.
 *
 * \note            The caller is responsible for validating the switch
 *                  and taking the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitCrm(fm_int sw)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    TAKE_REG_LOCK(sw);

    /**************************************************
     * Initialize CRM hardware.
     **************************************************/

    err = ResetCrm(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    /**************************************************
     * Initialize TCAM monitors.
     **************************************************/

    err = InitMonitors(sw, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000InitCrm */




/*****************************************************************************/
/** fm10000StartCrmMonitors
 * \ingroup intCrm
 *
 * \desc            Starts the Counter Rate Monitor subsystem.
 *
 * \note            The caller is responsible for validating the switch
 *                  and taking the switch lock.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000StartCrmMonitors(fm_int sw)
{
    fm10000_crmInfo *crmInfo;
    fm_status        err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    crmInfo = GET_CRM_INFO(sw);

    /***************************************************
     * Start the CRM.
     **************************************************/

    err = StartCrm(sw, crmInfo->firstIdx, crmInfo->lastIdx, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000StartCrmMonitors */




/*****************************************************************************/
/** fmDbgDisableCrmInterrupts
 * \ingroup intCrm
 *
 * \desc            Disables CRM interrupts for the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDisableCrmInterrupts(fm_int sw)
{
    fm10000_crmInfo *crmInfo;
    fm_switch       *switchPtr;
    fm_uint32        crmVal[FM10000_CRM_IP_WIDTH];
    fm_uint64        intMask;
    fm_status        err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    crmInfo   = GET_CRM_INFO(sw);

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    intMask  = FM_ARRAY_GET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask);

    intMask |= crmInfo->validMask;

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask, intMask);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    DROP_REG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgDisableCrmInterrupts */




/*****************************************************************************/
/** fmDbgEnableCrmInterrupts
 * \ingroup intCrm
 *
 * \desc            Enables CRM interrupts for the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgEnableCrmInterrupts(fm_int sw)
{
    fm10000_crmInfo *crmInfo;
    fm_switch       *switchPtr;
    fm_uint32        crmVal[FM10000_CRM_IP_WIDTH];
    fm_uint64        intMask;
    fm_status        err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    crmInfo   = GET_CRM_INFO(sw);

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32Mult(sw,
                                    FM10000_CRM_IM(0),
                                    FM10000_CRM_IM_WIDTH,
                                    crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

    intMask = FM_ARRAY_GET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask);

    intMask &= ~crmInfo->validMask;

    FM_ARRAY_SET_FIELD64(crmVal, FM10000_CRM_IM, InterruptMask, intMask);

    err = switchPtr->WriteUINT32Mult(sw,
                                     FM10000_CRM_IM(0),
                                     FM10000_CRM_IM_WIDTH,
                                     crmVal);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    DROP_REG_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgEnableCrmInterrupts */




/*****************************************************************************/
/** fmDbgInitCrm
 * \ingroup intCrm
 *
 * \desc            Initializes the Counter Rate Monitor subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgInitCrm(fm_int sw)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fm10000InitCrm(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgInitCrm */




/*****************************************************************************/
/** fmDbgStartCrm
 * \ingroup intCrm
 *
 * \desc            Starts the Counter Rate Monitor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgStartCrm(fm_int sw)
{
    fm10000_crmInfo *crmInfo;
    fm_status        err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_REG_LOCK(sw);

    crmInfo = GET_CRM_INFO(sw);

    err = StartCrm(sw, crmInfo->firstIdx, crmInfo->lastIdx, TRUE);

    DROP_REG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgStartCrm */




/*****************************************************************************/
/** fmDbgStopCrm
 * \ingroup intCrm
 *
 * \desc            Stops the Counter Rate Monitor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgStopCrm(fm_int sw)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_REG_LOCK(sw);

    err = StopCrm(sw);

    DROP_REG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgStopCrm */




/*****************************************************************************/
/** fmDbgUpdateCrmChecksum
 * \ingroup intCrm
 *
 * \desc            Updates the checksum for a CRM monitor.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to update.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgUpdateCrmChecksum(fm_int sw, fm_int crmId)
{
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d crmId=%d\n", sw, crmId);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_REG_LOCK(sw);

    err = UpdateChecksum(sw, crmId);

    DROP_REG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgUpdateCrmChecksum */




/*****************************************************************************/
/** fmDbgUpdateCrmMonitors
 * \ingroup intCrm
 *
 * \desc            Updates the checksums for the TCAM monitors.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgUpdateCrmMonitors(fm_int sw)
{
    fm_int      crmId;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);
    TAKE_REG_LOCK(sw);

    /**************************************************
     * Initialize FFU_SLICE_TCAM checksums.
     **************************************************/

    for (crmId = 0 ; crmId < FM10000_GLORT_CAM_CRM_ID ; crmId++)
    {
        err = UpdateChecksum(sw, crmId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

    /**************************************************
     * Initialize GLORT_CAM checksum.
     **************************************************/

    err = UpdateChecksum(sw, crmId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);

ABORT:
    DROP_REG_LOCK(sw);
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fmDbgUpdateCrmMonitors */




/*****************************************************************************/
/** fm10000CrmMaskInterrupts
 * \ingroup intSwitch
 *
 * \desc            Masks interrupts for a CRM monitor.
 *
 * \param[in]       eventInfo is a pointer to a caller-allocated area
 *                  containing the generic event descriptor.
 *
 * \param[in]       userInfo is a pointer to a caller-allocated area containing
 *                  purpose-specific event info.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrmMaskInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status        err;
    fm_uint64        crmMask;
    fm_int           crmId;
    fm_int           sw;
    fm10000_crmInfo *crmInfo;

    err = FM_OK;
    crmId = ((fm10000_crmUserInfo *)userInfo)->crmId;
    sw = ((fm10000_crmUserInfo *)userInfo)->sw;

    TAKE_REG_LOCK(sw);

    if (crmId < 0 || crmId >= FM10000_CRM_COMMAND_ENTRIES)
    {
        err = FM_ERR_INVALID_CRM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

    crmInfo = GET_CRM_INFO(sw);

    crmMask = FM_LITERAL_U64(1) << crmId;
    crmMask &= crmInfo->validMask;

    if (crmMask == 0)
    {
        err = FM_ERR_INVALID_CRM;
        goto ABORT;
    }

    if ((crmInfo->errorMask & crmMask) == 0)
    {
        err = MaskInterrupts(sw, crmMask, TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

ABORT:

    DROP_REG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000CrmMaskInterrupts */




/*****************************************************************************/
/** fm10000CrmUnmaskInterrupts
 * \ingroup intSwitch
 *
 * \desc            Unmasks interrupts for a CRM monitor.
 *
 * \param[in]       eventInfo is a pointer to a caller-allocated area
 *                  containing the generic event descriptor.
 *
 * \param[in]       userInfo is a pointer to a caller-allocated area containing
 *                  purpose-specific event info.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrmUnmaskInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status        err;
    fm_uint64        crmMask;
    fm_int           crmId;
    fm_int           sw;
    fm10000_crmInfo *crmInfo;

    crmId = ((fm10000_crmUserInfo *)userInfo)->crmId;
    sw = ((fm10000_crmUserInfo *)userInfo)->sw;

    err = FM_OK;
    TAKE_REG_LOCK(sw);

    if (crmId < 0 || crmId >= FM10000_CRM_COMMAND_ENTRIES)
    {
        err = FM_ERR_INVALID_CRM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

    crmInfo = GET_CRM_INFO(sw);

    crmMask = FM_LITERAL_U64(1) << crmId;
    crmMask &= crmInfo->validMask;

    if (crmMask == 0)
    {
        err = FM_ERR_INVALID_CRM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }


    if ((crmInfo->errorMask & crmMask) == 0)
    {
        err = MaskInterrupts(sw, crmMask, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, err);
    }

ABORT:
    DROP_REG_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000CrmUnmaskInterrupts */




/*****************************************************************************/
/** fm10000CrmUpdateChecksum
 * \ingroup intSwitch
 *
 * \desc            Updates the checksum for a CRM monitor.
 *
 * \param[in]       eventInfo is a pointer to a caller-allocated area
 *                  containing the generic event descriptor.
 *
 * \param[in]       userInfo is a pointer to a caller-allocated area containing
 *                  purpose-specific event info.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_CRM if the CRM identifier is invalid.
 *
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fm10000CrmUpdateChecksum( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status err;
    fm_int    crmId;
    fm_int    sw;

    crmId = ((fm10000_crmUserInfo *)userInfo)->crmId;
    sw = ((fm10000_crmUserInfo *)userInfo)->sw;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d crmId=%d\n", sw, crmId);

    TAKE_REG_LOCK(sw);

    err = UpdateChecksum(sw, crmId);

    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}   /* end fm10000CrmUpdateChecksum */




/*****************************************************************************/
/** HandleCrmTimeout
 * \ingroup intSwitch
 *
 * \desc            Handles a timeout simulating the time needed for the switch
 *                  to update checksums properly.
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the crm user info
 *                  structure (type ''fm10000_crmUserInfo'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandleCrmTimeout( void *arg )
{
    fm_smEventInfo       eventInfo;
    fm10000_crmUserInfo *userInfo;
    fm_int               sw;
    fm_int               crmId;
    fm_bool              notifyTimeout;
    fm10000_switch *     switchExt;

    userInfo          = arg;
    sw                = userInfo->sw;
    crmId             = userInfo->crmId;
    notifyTimeout     = userInfo->notifyTimeout;
    switchExt         = GET_SWITCH_EXT(sw);

    if (notifyTimeout)
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_CRM,
                         crmId,
                         "Notifying timeout on sw %d crmId %d\n",
                         sw,
                         crmId );

        PROTECT_SWITCH( sw );

        eventInfo.smType = FM10000_BASIC_CRM_STATE_MACHINE;
        eventInfo.dontSaveRecord = FALSE;
        eventInfo.lock   = FM_GET_STATE_LOCK( sw );
        eventInfo.eventId = FM10000_CRM_EVENT_TIMEOUT_IND;
        fmNotifyStateMachineEvent( switchExt->crmInfo.crmSmHandles[crmId],
                                   &eventInfo,
                                   userInfo,
                                   &switchExt->crmInfo.logInfo );
        UNPROTECT_SWITCH( sw );
    }


}    /* end HandleCrmTimeout */




/*****************************************************************************/
/** fm10000CrmStartTimer
 * \ingroup intSwitch
 *
 * \desc            Starts 20 milisecond timer for specified CRM monitor.
 *
 * \param[in]       eventInfo is a pointer to a caller-allocated area
 *                  containing the generic event descriptor.
 *
 * \param[in]       userInfo is a pointer to a caller-allocated area containing
 *                  purpose-specific event info.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 *
 *****************************************************************************/
fm_status fm10000CrmStartTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status        err;
    fm_int           crmId;
    fm_int           sw;
    fm10000_switch * switchExt;
    fm_timestamp     timeout = { 0, 20000 };

    crmId       = ((fm10000_crmUserInfo *)userInfo)->crmId;
    sw = ((fm10000_crmUserInfo *)userInfo)->sw;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d, crmId=%d\n", sw, crmId);

    ((fm10000_crmUserInfo *)userInfo)->notifyTimeout = TRUE;

    switchExt = GET_SWITCH_EXT(sw);

    err = fmStartTimer(switchExt->crmInfo.crmTimers[crmId], &timeout, 1, HandleCrmTimeout, userInfo);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}    /* end fm10000CrmStartTimer */




/*****************************************************************************/
/** fm10000CrmCancelTimer
 * \ingroup intSwitch
 *
 * \desc            Cancels timer for specified CRM monitor.
 *
 * \param[in]       eventInfo is a pointer to a caller-allocated area
 *                  containing the generic event descriptor.
 *
 * \param[in]       userInfo is a pointer to a caller-allocated area containing
 *                  purpose-specific event info.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 *
 *****************************************************************************/
fm_status fm10000CrmCancelTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status          err;
    fm_int             sw;
    fm_int             crmId;
    fm10000_switch *   switchExt;

    crmId = ((fm10000_crmUserInfo *)userInfo)->crmId;
    sw    = ((fm10000_crmUserInfo *)userInfo)->sw;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d, crmId=%d\n", sw, crmId);

    switchExt = GET_SWITCH_EXT(sw);

    ((fm10000_crmUserInfo *)userInfo)->notifyTimeout = FALSE;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_CRM,
                     crmId,
                     "NO TIMEOUT NOTIFY ON sw %d crmId %d\n",
                     sw,
                     crmId );

    err = fmStopTimer(switchExt->crmInfo.crmTimers[crmId]);

    FM_LOG_EXIT(FM_LOG_CAT_CRM, err);

}    /* end fm10000CrmCancelTimer */




/*****************************************************************************/
/** NotifyCRMEvent
 * \ingroup intSwitch
 *
 * \desc            Notifies the CRM state machine about event occured.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       crmId is the CRM monitor to update.
 *
 * \param[in]       eventId is the ID of event.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_UNINITIALIZED if the state machine engine
 *                  is uninitialized.
 * \return          FM_ERR_INVALID_ARGUMENT if one of the arguments is invalid
 *
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 *
 * \return          FM_ERR_STATE_MACHINE_TYPE  the state machine type
 *                  indicated in the event info header does not match that of
 *                  the referred state machine
 *
 *****************************************************************************/
fm_status NotifyCRMEvent(fm_int sw, fm_int crmId, fm_int eventId)
{

    fm_smEventInfo   eventInfo;
    fm_status        status;
    fm10000_switch * switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d, crmId=%d, eventId=%d\n", sw, crmId, eventId);

    if (crmId >= FM10000_NUM_CRM_IDS)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ERROR(FM_LOG_CAT_CRM, "Invalid crmId\n");
    }

    switchExt = GET_SWITCH_EXT(sw);
    switchExt->crmInfo.crmUserInfo[crmId].sw = sw;
    switchExt->crmInfo.crmUserInfo[crmId].crmId = crmId;


    if ( switchExt->isCrmStarted )
    {
        eventInfo.smType = FM10000_BASIC_CRM_STATE_MACHINE;
        eventInfo.dontSaveRecord = FALSE;
        eventInfo.lock   = FM_GET_STATE_LOCK( sw );
        eventInfo.eventId = eventId;

        status = fmNotifyStateMachineEvent( switchExt->crmInfo.crmSmHandles[crmId],
                                            &eventInfo,
                                            &switchExt->crmInfo.crmUserInfo[crmId],
                                            &switchExt->crmInfo.logInfo[crmId] );
    }
    else
    {
        if (eventId == FM10000_CRM_EVENT_RESUME_REQ)
        {
            status = UpdateChecksum(sw, crmId);
        }
        else
        {
            status = FM_OK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_CRM, status);

}    /* end NotifyCRMEvent */




/*****************************************************************************/
/** fm10000LogCrmTransition
 * \ingroup intSwitch
 *
 * \desc            Logs CRM state machine transition.
 *
 * \note            The caller has already taken the necessary locks.
 *
 * \param[in]       record is the record to log.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000LogCrmTransition( fm_smTransitionRecord *record )
{
    fm_text currentState;
    fm_text event;

    if ( record->currentState != FM_STATE_UNSPECIFIED )
    {
        currentState = fm10000CrmStatesMap[record->currentState];
    }
    else
    {
        currentState = "N/A";
    }
    if ( record->eventInfo.eventId != FM_EVENT_UNSPECIFIED )
    {
        event = fm10000CrmEventsMap[record->eventInfo.eventId];
    }
    else
    {
        event = "N/A";
    }

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     record->smUserID,
                     "Port %d Transition: "
                     "'%s': '%s' ==> '%s'\n",
                     record->smUserID,
                     event,
                     currentState,
                     fm10000CrmEventsMap[record->nextState] );

    return FM_OK;

}   /* end fm10000LogCrmTransition */



/*****************************************************************************/
/** fm10000FreeCrmStructures
 * \ingroup intSwitch
 *
 * \desc            Frees CRM allocated structures: timers and state machines.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 * \return          FM_ERR_STATE_MACHINE_HANDLE if the specified handle does
 *                  not correspond to a valid state machine
 *
 * \return          FM_ERR_INVALID_ARGUMENT if one of the function arguments
 *                  was not valid (NULL pointer).
 *
 *****************************************************************************/
fm_status fm10000FreeCrmStructures(fm_int sw)
{
    int              i;
    fm10000_switch * switchExt;
    fm_status        status;
    fm_bool          isWhiteModel;

    FM_LOG_ENTRY(FM_LOG_CAT_CRM, "sw=%d\n", sw);

    switchExt = GET_SWITCH_EXT(sw);
    isWhiteModel = fmGetBoolApiProperty(FM_AAK_API_PLATFORM_IS_WHITE_MODEL,
                                        FM_AAD_API_PLATFORM_IS_WHITE_MODEL);

    if (isWhiteModel)
    {
        status = FM_OK;
        FM_LOG_EXIT(FM_LOG_CAT_CRM, status);
    }

    for ( i = 0 ; i < FM10000_NUM_CRM_IDS; i++ )
    {
        status = fmDeleteTimer(switchExt->crmInfo.crmTimers[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, status);

        if (switchExt->isCrmStarted)
        {
            status = fmStopStateMachine(switchExt->crmInfo.crmSmHandles[i]);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, status);
        }

        status = fmDeleteStateMachine(switchExt->crmInfo.crmSmHandles[i]);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_CRM, status);
    }


ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_CRM, status);

}   /* end fm10000FreeCrmStructures */
