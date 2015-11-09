/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_parity.c
 * Creation Date:   August 6, 2014.
 * Description:     Parity error handler.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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


typedef struct
{
    /* Tracking counter index. */
    fm_trackingCounterIndex ctrIdx;

    /* Offset of counter variable in fm_parityErrorCounters structure. */
    fm_uint                 ctrOffset;

} counterDesc;


#define GET_COUNTER_PTR(basePtr, desc)    \
    ( (fm_uint64 *) ( ( (char *) (basePtr) ) + (desc)->ctrOffset ) )

#define WARNING_PARITY_THRESH_LEVEL 1
#define FATAL_PARITY_THRESH_LEVEL   10

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

#define COUNTER_DESC(index, field)  \
        { index, offsetof(fm_parityErrorCounters, field) }

static const counterDesc counterDescTable[] =
{
    COUNTER_DESC(FM_CTR_PARITY_AREA_CROSSBAR,       cntCrossbarErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_ARRAY,          cntArrayMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_FH_HEAD,        cntFhHeadMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_FH_TAIL,        cntFhTailMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_MODIFY,         cntModifyMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_SCHEDULER,      cntSchedMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_EPL,            cntEplMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_TUNNEL_ENGINE,  cntTeMemoryErrs),
    COUNTER_DESC(FM_CTR_PARITY_AREA_TCAM,           cntTcamMemoryErrs),

    COUNTER_DESC(FM_CTR_PARITY_SEVERITY_TRANSIENT,  cntTransientErrs),
    COUNTER_DESC(FM_CTR_PARITY_SEVERITY_REPAIRABLE, cntRepairableErrs),
    COUNTER_DESC(FM_CTR_PARITY_SEVERITY_CUMULATIVE, cntCumulativeErrs),
    COUNTER_DESC(FM_CTR_PARITY_SEVERITY_FATAL,      cntFatalErrs),
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** fmParityStateToText
 * \ingroup intParity
 *
 * \desc            Returns the text representation of the parity state.
 *
 * \param[in]       parityState is the parity error processing state.
 *
 * \return          Pointer to a string representing the parity state.
 *
 *****************************************************************************/
static const char * fmParityStateToText(fm_int parityState)
{

    switch (parityState)
    {
        case FM10000_PARITY_STATE_INACTIVE:
            return "INACTIVE";

        case FM10000_PARITY_STATE_DECODE:
            return "DECODE";

        case FM10000_PARITY_STATE_FATAL:
            return "FATAL";

        default:
            return "UNKNOWN";

    }   /* end switch (parityState) */

}   /* end fmParityStateToText */




/*****************************************************************************/
/** DumpPendingRepairs
 * \ingroup intDiag
 *
 * \desc            Dumps the pending repairs.
 *
 * \param[in]       parityInfo points to the parity information structure.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DumpPendingRepairs(fm10000_parityInfo * parityInfo)
{
    fm_int              repairType;
    fm_uint64           bitMask;
    fm_char             auxData1[16];
    fm_char             auxData2[16];
    fm_bool             isUerr;

    FM_LOG_PRINT("\n");
    FM_LOG_PRINT("No  Auxiliary Data     xErr  Repair Type\n");
    FM_LOG_PRINT("--  -----------------  ----  ------------------------\n");

    for (repairType = 1 ; repairType < FM_REPAIR_TYPE_MAX ; repairType++)
    {
        bitMask = FM_LITERAL_U64(1) << repairType;

        if (parityInfo->pendingRepairs & bitMask)
        {
            isUerr = (parityInfo->pendingUerrs & bitMask) != 0;

            auxData1[0] = 0;
            auxData2[0] = 0;

            switch (repairType)
            {
                case FM_REPAIR_FFU_SLICE_SRAM:
                    FM_SPRINTF_S(auxData1,
                                 sizeof(auxData1),
                                 "%08x",
                                 parityInfo->ffuRamRepair.errMask);
                    FM_SPRINTF_S(auxData2,
                                 sizeof(auxData2),
                                 "%08x",
                                 parityInfo->ffuRamRepair.uerrMask);
                    break;

                case FM_REPAIR_FFU_SLICE_TCAM:
                    FM_SPRINTF_S(auxData1,
                                 sizeof(auxData1),
                                 "%08x",
                                 parityInfo->ffuTcamRepair.errMask);
                    FM_SPRINTF_S(auxData2,
                                 sizeof(auxData2),
                                 "%08x",
                                 parityInfo->ffuTcamRepair.uerrMask);
                    break;

                case FM_REPAIR_RX_STATS_BANK:
                    FM_SPRINTF_S(auxData1,
                                 sizeof(auxData1),
                                 "%04x",
                                 parityInfo->rxStatsRepair.errMask);
                    FM_SPRINTF_S(auxData2,
                                 sizeof(auxData2),
                                 "%04x",
                                 parityInfo->rxStatsRepair.uerrMask);
                    break;

                case FM_REPAIR_TUNNEL_ENGINE_0:
                    FM_SPRINTF_S(auxData1,
                                 sizeof(auxData1),
                                 "%04x",
                                 parityInfo->teErrRepair[0].errMask);
                    FM_SPRINTF_S(auxData2,
                                 sizeof(auxData2),
                                 "%04x",
                                 parityInfo->teErrRepair[0].uerrMask);
                    break;

                case FM_REPAIR_TUNNEL_ENGINE_1:
                    FM_SPRINTF_S(auxData1,
                                 sizeof(auxData1),
                                 "%04x",
                                 parityInfo->teErrRepair[1].errMask);
                    FM_SPRINTF_S(auxData2,
                                 sizeof(auxData2),
                                 "%04x",
                                 parityInfo->teErrRepair[1].uerrMask);
                    break;

                default:
                    break;

            }   /* end switch (repairType) */

            FM_LOG_PRINT("%2d  %-8s %-8s  %4s  %s\n",
                         repairType,
                         auxData1,
                         auxData2,
                         (isUerr) ? "UERR" : "CERR",
                         fmRepairTypeToText(repairType));

        }   /* end if (parityInfo->pendingRepairs & bitMask) */

    }   /* end for (repairType = 1 ; repairType < FM_REPAIR_TYPE_MAX ; ...) */

    return FM_OK;

}   /* end DumpPendingRepairs */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000GetParityErrorCounters
 * \ingroup intStats
 *
 * \desc            Returns the parity error counters.
 *                  Called through the GetParityErrorCounters switch pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      counters points to the parity error counters structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetParityErrorCounters(fm_int                  sw,
                                        fm_parityErrorCounters *counters)
{
    const counterDesc *ctrDesc;
    fm_status          retVal;
    fm_status          err;
    fm_uint            i;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d counters=%p\n",
                 sw,
                 (void *) counters);

    if (counters == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    FM_MEMSET_S(counters,
                sizeof(fm_parityErrorCounters),
                0,
                sizeof(fm_parityErrorCounters));

    retVal = FM_OK;

    for (i = 0 ; i < FM_NENTRIES(counterDescTable) ; i++)
    {
        ctrDesc = &counterDescTable[i];

        if (ctrDesc->ctrIdx == 0 && ctrDesc->ctrOffset == 0)
        {
            break;
        }

        err = fmDbgDiagCountGet(sw,
                                ctrDesc->ctrIdx,
                                GET_COUNTER_PTR(counters, ctrDesc));
        FM_ERR_COMBINE(retVal, err);

    }   /* end for (i = 0 ; i < FM_NENTRIES(counterDescTable) ; i++) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retVal);

}   /* end fm10000GetParityErrorCounters */




/*****************************************************************************/
/** fm10000ResetParityErrorCounters
 * \ingroup intStats
 *
 * \desc            Resets the memory error counters.
 *                  Called through the ResetParityErrorCounters switch pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ResetParityErrorCounters(fm_int sw)
{
    const counterDesc *ctrDesc;
    fm_status          retVal;
    fm_status          err;
    fm_uint            i;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    retVal = FM_OK;

    for (i = 0 ; i < FM_NENTRIES(counterDescTable) ; i++)
    {
        ctrDesc = &counterDescTable[i];

        if (ctrDesc->ctrIdx == 0 && ctrDesc->ctrOffset == 0)
        {
            break;
        }

        err = fmDbgDiagCountClear(sw, ctrDesc->ctrIdx);
        FM_ERR_COMBINE(retVal, err);

    }   /* end for (i = 0 ; i < FM_NENTRIES(counterDescTable) ; i++) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, retVal);

}   /* end fm10000ResetParityErrorCounters */




/*****************************************************************************/
/** fm10000DbgDumpParity
 * \ingroup intDiag
 *
 * \desc            Dumps the state of the parity error subsystem.
 *                  Called through the DbgDumpParity function pointer.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpParity(fm_int sw)
{
    fm_switch          *switchPtr;
    fm10000_parityInfo *parityInfo;

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);

    FM_LOG_PRINT("========== Parity Status ==========\n");
    FM_LOG_PRINT("parityState    : %s\n",
                 fmParityStateToText(parityInfo->parityState));
    FM_LOG_PRINT("interrupts     : %s\n",
                 (parityInfo->interruptsEnabled) ? "enabled" : "disabled");
    FM_LOG_PRINT("repairs        : %s\n",
                 (switchPtr->parityRepairEnabled) ? "enabled" : "disabled");

    FM_LOG_PRINT("sramErrHistory : %06llx %06llx\n",
                 (parityInfo->sramErrHistory >> 24),
                  parityInfo->sramErrHistory & 0xFFFFFF);

    if (parityInfo->pendingRepairs)
    {
        DumpPendingRepairs(parityInfo);
    }

    return FM_OK;

}   /* end fm10000DbgDumpParity */




/*****************************************************************************/
/** fm10000InitParity
 * \ingroup intParity
 *
 * \desc            Initializes the parity error handling subsystem.
 *
 * \param[in]       switchPtr points to the switch state structure.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitParity(fm_switch * switchPtr)
{
    fm10000_switch     *switchExt;
    fm_int              sw;
    fm_status           err;
    fm_int              crmTimeout;
    fm10000_parityInfo *parityInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    sw         = switchPtr->switchNumber;
    switchExt  = switchPtr->extension;
    parityInfo = GET_PARITY_INFO(sw);

    err = fmCreateLockV2("ParityLock",
                         sw,
                         FM_LOCK_PREC_PARITY,
                         &switchExt->parityLock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    err = fm10000InitParityThresholds(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

    crmTimeout = GET_FM10000_PROPERTY()->parityCrmTimeout;

    if (crmTimeout >= FM10000_CRM_TIMEOUT)
    {
        parityInfo->crmTimeout.sec = 0;
        parityInfo->crmTimeout.usec = crmTimeout * 1000;
    }
    else
    {
        parityInfo->crmTimeout.sec = 0;
        parityInfo->crmTimeout.usec =  FM10000_CRM_TIMEOUT * 1000;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fm10000InitParity */




/*****************************************************************************/
/** fm10000FreeParityResources
 * \ingroup intParity
 *
 * \desc            Frees the Parity resources. This is to undo what is
 *                  done in fm10000InitParity.
 *
 * \param[in]       switchPtr points to the switch state structure.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeParityResources(fm_switch * switchPtr)
{
    fm10000_switch *switchExt;
    fm_status       err;

    switchExt = switchPtr->extension;

    err = fmDeleteLock(&switchExt->parityLock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

ABORT:
    return FM_OK;

}   /* end fm10000FreeParityResources */




/*****************************************************************************/
/** fm10000GetParityAttribute
 * \ingroup intSwitch
 *
 * \desc            Retrieves a parity error attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the parity attribute to retrieve (see
 *                  fm_parityAttr).
 *
 * \param[out]      value points to the location where this function
 *                  is to store the attribute value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not recognized.
 *
 *****************************************************************************/
fm_status fm10000GetParityAttribute(fm_int sw, fm_int attr, void * value)
{
    fm10000_parityInfo *parityInfo;
    fm_switch          *switchPtr;
    fm_status           err;
    fm_bool             isWhiteModel;
    fm10000_switch     *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d attr=%d\n", sw, attr);

    if (value == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);
    switchExt  = GET_SWITCH_EXT(sw);

    isWhiteModel = fmGetBoolApiProperty(FM_AAK_API_PLATFORM_IS_WHITE_MODEL,
                                        FM_AAD_API_PLATFORM_IS_WHITE_MODEL);

    err = FM_OK;

    switch (attr)
    {
        case FM_PARITY_INTERRUPTS:
            *(fm_bool *)value = parityInfo->interruptsEnabled;
            break;

        case FM_PARITY_REPAIRS:
            *(fm_bool *)value = switchPtr->parityRepairEnabled;
            break;

        case FM_PARITY_TCAM_MONITORS:
            if (!isWhiteModel)
            {
                *(fm_bool *) value = switchExt->isCrmStarted;
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_POLICERS_CERR_ERROR:
            *(fm_uint32 *) value = parityInfo->policersCerrError;
            break;

        case FM_POLICERS_CERR_FATAL:
            *(fm_uint32 *) value = parityInfo->policersCerrFatal;
            break;

        case FM_POLICERS_UERR_ERROR:
            *(fm_uint32 *) value = parityInfo->policersUerrError;
            break;

        case FM_POLICERS_UERR_FATAL:
            *(fm_uint32 *) value = parityInfo->policersUerrFatal;
            break;

        case FM_STATS_CERR_ERROR:
            *(fm_uint32 *) value = parityInfo->statsCerrError;
            break;

        case FM_STATS_CERR_FATAL:
            *(fm_uint32 *) value = parityInfo->statsCerrFatal;
            break;

        case FM_STATS_UERR_ERROR:
            *(fm_uint32 *) value = parityInfo->statsUerrError;
            break;

        case FM_STATS_UERR_FATAL:
            *(fm_uint32 *) value = parityInfo->statsUerrFatal;
            break;

        case FM_FREELIST_UERR_ERROR:
            *(fm_uint32 *) value = parityInfo->freelistUerrError;
            break;

        case FM_FREELIST_UERR_FATAL:
            *(fm_uint32 *) value = parityInfo->freelistUerrFatal;
            break;

        case FM_REFCOUNT_UERR_ERROR:
            *(fm_uint32 *) value = parityInfo->refcountUerrError;
            break;

        case FM_REFCOUNT_UERR_FATAL:
            *(fm_uint32 *) value = parityInfo->refcountUerrFatal;
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fm10000GetParityAttribute */




/*****************************************************************************/
/** fm10000SetParityAttribute
 * \ingroup intSwitch
 *
 * \desc            Sets a parity error attribute value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the parity attribute to set (see fm_parityAttr).
 *
 * \param[in]       value points to the attribute value to set.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not recognized.
 * \return          FM_ERR_READONLY_ATTRIB if the attribute is read-only.
 *
 *****************************************************************************/
fm_status fm10000SetParityAttribute(fm_int sw, fm_int attr, void * value)
{
    fm10000_parityInfo *parityInfo;
    fm_switch          *switchPtr;
    fm_bool             isWhiteModel;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d attr=%d\n", sw, attr);

    if (value == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);

    isWhiteModel = fmGetBoolApiProperty(FM_AAK_API_PLATFORM_IS_WHITE_MODEL,
                                        FM_AAD_API_PLATFORM_IS_WHITE_MODEL);

    err = FM_OK;

    switch (attr)
    {
        case FM_PARITY_INTERRUPTS:
            if (*(fm_bool *) value)
            {
                err = fm10000EnableParityInterrupts(sw);
            }
            else
            {
                err = fm10000DisableParityInterrupts(sw);
            }
            break;

        case FM_PARITY_REPAIRS:
            switchPtr->parityRepairEnabled = (*(fm_bool *) value != 0);
            break;

        case FM_PARITY_TCAM_MONITORS:
            if (!isWhiteModel)
            {
                if (*(fm_bool *) value)
                {
                    err = fm10000StartCrmMonitors(sw);
                }
                else
                {
                    err = fm10000StopCrmMonitors(sw);
                }
            }
            else
            {
                err = FM_ERR_UNSUPPORTED;
            }
            break;

        case FM_POLICERS_CERR_ERROR:
             parityInfo->policersCerrError = (*(fm_uint32 *) value);
             break;

        case FM_POLICERS_CERR_FATAL:
             parityInfo->policersCerrFatal = (*(fm_uint32 *) value);
             break;

        case FM_POLICERS_UERR_ERROR:
             parityInfo->policersUerrError = (*(fm_uint32 *) value);
             break;

        case FM_POLICERS_UERR_FATAL:
             parityInfo->policersUerrFatal = (*(fm_uint32 *) value);
             break;

        case FM_STATS_CERR_ERROR:
             parityInfo->statsCerrError = (*(fm_uint32 *) value);
             break;

        case FM_STATS_CERR_FATAL:
             parityInfo->statsCerrFatal = (*(fm_uint32 *) value);
             break;

        case FM_STATS_UERR_ERROR:
             parityInfo->statsUerrError = (*(fm_uint32 *) value);
             break;

        case FM_STATS_UERR_FATAL:
             parityInfo->statsUerrFatal = (*(fm_uint32 *) value);
             break;

        case FM_FREELIST_UERR_ERROR:
             parityInfo->freelistUerrError = (*(fm_uint32 *) value);
             break;

        case FM_FREELIST_UERR_FATAL:
             parityInfo->freelistUerrFatal = (*(fm_uint32 *) value);
             break;

        case FM_REFCOUNT_UERR_ERROR:
             parityInfo->refcountUerrError = (*(fm_uint32 *) value);
             break;

        case FM_REFCOUNT_UERR_FATAL:
             parityInfo->refcountUerrFatal = (*(fm_uint32 *) value);
             break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fm10000SetParityAttribute */




/*****************************************************************************/
/** fm10000DbgDumpParityConfig
 * \ingroup intParity
 *
 * \desc            Shows the parity configuration including thresholds.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpParityConfig(fm_int sw)
{
    fm10000_parityInfo *parityInfo;
    fm10000_switch     *switchExt;
    fm_bool             isWhiteModel;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                     "sw=%d\n",
                     sw);

    parityInfo = GET_PARITY_INFO(sw);
    switchExt  = GET_SWITCH_EXT(sw);

    isWhiteModel = fmGetBoolApiProperty(FM_AAK_API_PLATFORM_IS_WHITE_MODEL,
                                        FM_AAD_API_PLATFORM_IS_WHITE_MODEL);

    FM_LOG_PRINT("\nParity configuration for switch %d\n\n", sw);

    FM_LOG_PRINT("Interrupts Enabled            : ");
    FM_LOG_PRINT(parityInfo->interruptsEnabled ? "ON\n" : "OFF\n");

    FM_LOG_PRINT("Tcam monitors Started         : ");
    if (isWhiteModel)
    {
        FM_LOG_PRINT("UNSUPPORTED");
    }
    else
    {
        FM_LOG_PRINT(switchExt->isCrmStarted ? "ON" : "OFF");
    }
    FM_LOG_PRINT("\n\n");

    FM_LOG_PRINT("Policers CERR error           : %d\n",
                 parityInfo->policersCerrError);
    FM_LOG_PRINT("Policers CERR fatal           : %d\n",
                 parityInfo->policersCerrFatal);
    FM_LOG_PRINT("Policers UERR error           : %d\n",
                 parityInfo->policersUerrError);
    FM_LOG_PRINT("Policers UERR fatal           : %d\n\n",
                 parityInfo->policersUerrFatal);

    FM_LOG_PRINT("Stats CERR error              : %d\n",
                 parityInfo->statsCerrError);
    FM_LOG_PRINT("Stats CERR fatal              : %d\n",
                 parityInfo->statsCerrFatal);
    FM_LOG_PRINT("Stats UERR error              : %d\n",
                 parityInfo->statsUerrError);
    FM_LOG_PRINT("Stats UERR fatal              : %d\n\n",
                 parityInfo->statsUerrFatal);

    FM_LOG_PRINT("Freelist UERR error           : %d\n",
                 parityInfo->freelistUerrError);
    FM_LOG_PRINT("Freelist UERR fatal           : %d\n\n",
                 parityInfo->freelistUerrFatal);

    FM_LOG_PRINT("Refcount UERR error           : %d\n",
                 parityInfo->refcountUerrError);
    FM_LOG_PRINT("Refcount UERR fatal           : %d\n\n",
                 parityInfo->refcountUerrFatal);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);

}   /* end fm10000DbgDumpParityConfig */




/*****************************************************************************/
/** fm10000InitParityThresholds
 * \ingroup intParity
 *
 * \desc            Initializes the parity error thresholds with defaults.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitParityThresholds(fm_int sw)
{
    fm10000_parityInfo *parityInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                     "sw=%d\n",
                     sw);

    parityInfo = GET_PARITY_INFO(sw);

    parityInfo->policersCerrError = WARNING_PARITY_THRESH_LEVEL;
    parityInfo->policersCerrFatal = FATAL_PARITY_THRESH_LEVEL;
    parityInfo->policersUerrError = WARNING_PARITY_THRESH_LEVEL;
    parityInfo->policersUerrFatal = FATAL_PARITY_THRESH_LEVEL;

    parityInfo->statsCerrError    = WARNING_PARITY_THRESH_LEVEL;
    parityInfo->statsCerrFatal    = FATAL_PARITY_THRESH_LEVEL;
    parityInfo->statsUerrError    = WARNING_PARITY_THRESH_LEVEL;
    parityInfo->statsUerrFatal    = FATAL_PARITY_THRESH_LEVEL;

    parityInfo->freelistUerrError = WARNING_PARITY_THRESH_LEVEL;
    parityInfo->freelistUerrFatal = FATAL_PARITY_THRESH_LEVEL;

    parityInfo->refcountUerrError = WARNING_PARITY_THRESH_LEVEL;
    parityInfo->refcountUerrFatal = FATAL_PARITY_THRESH_LEVEL;

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_OK);

}   /* end fm10000InitializeParityThresholds */
