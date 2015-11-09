/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_crm_int.h
 * Creation Date:   February 18, 2015
 * Description:     Definitions for the Counter Rate Monitor subsystem.
 *
 * Copyright (c) 2015, Intel Corporation.
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

#ifndef __FM_FM10000_API_CRM_INT_H
#define __FM_FM10000_API_CRM_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define FM10000_FFU_SLICE_CRM_ID(slice)  (slice)
#define FM10000_GLORT_CAM_CRM_ID         32
#define FM10000_NUM_CRM_IDS              (FM10000_GLORT_CAM_CRM_ID + 1)
#define FM10000_CRM_TIMEOUT              20

#define GET_CRM_INFO(sw) \
    &(((fm10000_switch *)GET_SWITCH_EXT(sw))->crmInfo)


typedef struct _fm10000_crmUserInfo
{
    fm_int    crmId;
    fm_int    sw;
    fm_bool   notifyTimeout;

} fm10000_crmUserInfo;

/******************************************
 * CRM information.
 * Stored in the FM10000 switch extension. 
 * Protected by REG_LOCK. 
 ******************************************/
typedef struct _fm10000_crmInfo
{
    /** CRM monitors that have been initialized and are in use. */
    fm_uint64            validMask;

    /** CRM monitors that are masked due to TCAM checksum error. */
    fm_uint64            errorMask;

    /** CRM monitors that are masked because the API is updating TCAM. */
    fm_uint64            updateMask;

    /** First CRM monitor index. */
    fm_int               firstIdx;

    /** Last CRM monitor index. */
    fm_int               lastIdx;

    fm_smHandle         crmSmHandles[FM10000_NUM_CRM_IDS];
    fm10000_crmUserInfo crmUserInfo[FM10000_NUM_CRM_IDS];
    fm_timerHandle      crmTimers[FM10000_NUM_CRM_IDS];
    fm_uint64           logInfo[FM10000_NUM_CRM_IDS];


} fm10000_crmInfo;

/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

fm_status fm10000DisableCrmMonitor(fm_int sw, fm_int crmId);
fm_status fm10000EnableCrmMonitor(fm_int sw, fm_int crmId);
fm_status fm10000InitCrm(fm_int sw);
fm_status fm10000StartCrmMonitors(fm_int sw);
fm_status fm10000StopCrmMonitors(fm_int sw);
fm_status fm10000CrmUpdateChecksum(fm_smEventInfo *eventInfo, void *userInfo);
fm_status fm10000FreeCrmStructures(fm_int sw);

fm_status fmDbgDisableCrmInterrupts(fm_int sw);
fm_status fmDbgEnableCrmInterrupts(fm_int sw);
fm_status fmDbgInitCrm(fm_int sw);
fm_status fmDbgStartCrm(fm_int sw);
fm_status fmDbgStopCrm(fm_int sw);
fm_status fmDbgUpdateCrmChecksum(fm_int sw, fm_int crmId);
fm_status fmDbgUpdateCrmMonitors(fm_int sw);

fm_status fm10000NotifyCRMEvent(fm_int sw, fm_int crmId, fm_int eventId);

#endif /* __FM_FM10000_API_CRM_INT_H */

