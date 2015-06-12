/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_parity.c
 * Creation Date:   August 6, 2014.
 * Description:     Parity error handler.
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


typedef struct
{
    /* Tracking counter index. */
    fm_trackingCounterIndex ctrIdx;

    /* Offset of counter variable in fm_switchMemErrCounters structure. */
    fm_uint                 ctrOffset;

} counterDesc;


#define GET_COUNTER_PTR(basePtr, desc)    \
    ( (fm_uint64 *) ( ( (char *) (basePtr) ) + (desc)->ctrOffset ) )


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

#define COUNTER_DESC(index, field)  \
        { index, offsetof(fm_switchMemErrCounters, field) }

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
 * \param[out]      counters points to the caller-supplied location in which
 *                  the counters are to be stored.
 * 
 * \param[in]       size is the size of the data area pointed to by counters.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetParityErrorCounters(fm_int  sw,
                                        void *  counters,
                                        fm_uint size)
{
    const counterDesc * ctrDesc;
    fm_status   retVal;
    fm_status   err;
    fm_uint     i;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d counters=%p size=%u\n",
                 sw,
                 counters,
                 size);

    if (counters == NULL || size != sizeof(fm_switchMemErrCounters))
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    memset(counters, 0, size);

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
    const counterDesc * ctrDesc;
    fm_status   retVal;
    fm_status   err;
    fm_uint     i;

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
    fm_switch *         switchPtr;
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
    fm10000_switch *switchExt;
    fm_int          sw;
    fm_status       err;

    sw         = switchPtr->switchNumber;
    switchExt  = switchPtr->extension;

    err = fmCreateLockV2("ParityLock",
                         sw,
                         FM_LOCK_PREC_PARITY,
                         &switchExt->parityLock);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PARITY, err);

ABORT:
    return FM_OK;

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
    fm10000_parityInfo * parityInfo;
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d attr=%d\n", sw, attr);

    if (value == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);

    err = FM_OK;

    switch (attr)
    {
        case FM_PARITY_INTERRUPTS:
            *(fm_bool *)value = parityInfo->interruptsEnabled;
            break;

        case FM_PARITY_REPAIRS:
            *(fm_bool *)value = switchPtr->parityRepairEnabled;
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

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
    fm10000_parityInfo * parityInfo;
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY, "sw=%d attr=%d\n", sw, attr);

    if (value == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr  = GET_SWITCH_PTR(sw);
    parityInfo = GET_PARITY_INFO(sw);

    err = FM_OK;

    switch (attr)
    {
        case FM_PARITY_INTERRUPTS:
            if (*(fm_bool *)value)
            {
                err = fm10000EnableParityInterrupts(sw);
            }
            else
            {
                err = fm10000DisableParityInterrupts(sw);
            }
            break;

        case FM_PARITY_REPAIRS:
            switchPtr->parityRepairEnabled = (*(fm_bool *)value != 0);
            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

}   /* end fm10000SetParityAttribute */
