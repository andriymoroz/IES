/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_parity.c
 * Creation Date:   December 2009
 * Description:     Generic thread wrapper for chip specific Parity
 *                  sweeper thread.
 *
 * Copyright (c) 2009 - 2015, Intel Corporation.
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmParityRepairTask
 * \ingroup intParity
 * 
 * \chips           FM10000
 *
 * \desc            Generic thread wrapper for chip specific parity
 *                  repair handler.
 *
 * \param[in]       args contains a pointer to the thread information.
 *
 * \return          Should never exit.
 *
 *****************************************************************************/
void * fmParityRepairTask(void * args)
{
    fm_thread * thread;
    fm_switch * switchPtr;
    fm_thread * eventHandler;
    fm_status   err;
    fm_int      sw;
    fm_bool     switchProtected = FALSE;

    thread       = FM_GET_THREAD_HANDLE(args);
    eventHandler = FM_GET_THREAD_PARAM(fm_thread, args);

    /* If logging is disabled, thread and eventHandler won't be used */
    FM_NOT_USED(thread);
    FM_NOT_USED(eventHandler);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "thread=%s, eventHandler=%s\n",
                 thread->name,
                 eventHandler->name);

    /**************************************************
     * Loop forever.
     **************************************************/

    for (;;)
    {
        /* Wait for something to do. */
        err = fmWaitSemaphore(&fmRootApi->parityRepairSemaphore,
                              FM_WAIT_FOREVER);

        if (err != FM_OK && err != FM_ERR_SEM_TIMEOUT)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                         "Unexpected error from fmWaitSemaphore: %s\n",
                         fmErrorMsg(err));
            continue;
        }

        /* Process each active switch in turn. */
        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if (!SWITCH_LOCK_EXISTS(sw))
            {
                continue;
            }

            PROTECT_SWITCH(sw);
            switchProtected = TRUE;

            switchPtr = GET_SWITCH_PTR(sw);

            if ( switchPtr &&
                (switchPtr->state == FM_SWITCH_STATE_UP) &&
                 switchPtr->parityRepairEnabled &&
                 switchPtr->ParityRepairTask )
            {
                switchPtr->ParityRepairTask(sw, &switchProtected, args);
            }

            if (switchProtected)
            {
                UNPROTECT_SWITCH(sw);
            }
        }

        fmYield();
        
    }   /* for (;;) */

    /**************************************************
     * Should never exit.
     **************************************************/

    FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                 "fmParityRepairTask exiting inadvertently!\n");

    fmExitThread(thread);
    return NULL;

}   /* end fmParityRepairTask */




/*****************************************************************************/
/** fmSendParityErrorEvent
 * \ingroup intParity
 *
 * \desc            Send a Parity error event to the upper layer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       parityEvent contains the parity event error information.
 *
 * \param[in]       eventHandler points to the event handler to which the
 *                  parity event should be sent.
 *
 * \return          Should never exit.
 *
 *****************************************************************************/
fm_status fmSendParityErrorEvent(fm_int              sw, 
                                 fm_eventParityError parityEvent, 
                                 fm_thread *         eventHandler)
{
    fm_status            err;
    fm_event *           eventPtr;
    fm_eventParityError *parityErrEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d ParityEvent=%p\n",
                 sw, (void *) &parityEvent);

    eventPtr = fmAllocateEvent(sw,
                               FM_EVID_SYSTEM,
                               FM_EVENT_PARITY_ERROR,
                               FM_EVENT_PRIORITY_LOW);

    if (eventPtr == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_PARITY, "Out of event buffers\n");
        FM_LOG_EXIT(FM_LOG_CAT_PARITY, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    /* Init event structure */
    parityErrEvent = &eventPtr->info.fpParityErrorEvent;

    FM_MEMCPY_S( parityErrEvent,
                 sizeof(*parityErrEvent),
                 &parityEvent,
                 sizeof(parityEvent) );

    err = fmSendThreadEvent(eventHandler, eventPtr);

    if (err != FM_OK)
    {
        /* Free the event since we could not send it to thread */
        fmReleaseEvent(eventPtr);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fmSendParityErrorEvent */




/*****************************************************************************/
/** fmParityErrTypeToText
 * \ingroup intParity
 *
 * \desc            Returns the text representation of a parity error type.
 *
 * \param[in]       errType is the memory type (see ''fm_parityErrType'').
 *
 * \return          Pointer to a string representing the parity error type.
 *
 *****************************************************************************/
const char * fmParityErrTypeToText(fm_parityErrType errType)
{

    switch ( errType )
    {
        case FM_PARITY_ERRTYPE_NONE:
            return "NONE";

        case FM_PARITY_ERRTYPE_CACHE_MISMATCH:
            return "CACHE_MISMATCH";

        case FM_PARITY_ERRTYPE_SRAM_CORRECTED:
            return "SRAM_CORRECTED";

        case FM_PARITY_ERRTYPE_SRAM_UNCORRECTABLE:
            return "SRAM_UNCORRECTABLE";

        default:
            return "UNKNOWN";
    }

}   /* end fmParityErrTypeToText */




/*****************************************************************************/
/** fmParityMemAreaToText
 * \ingroup intParity
 *
 * \desc            Returns the text representation of a memory area.
 *
 * \param[in]       memArea is the memory type (see ''fm_parityMemArea'').
 *
 * \return          Pointer to a string representing the memory area.
 *
 *****************************************************************************/
const char * fmParityMemAreaToText(fm_parityMemArea memArea)
{

    switch (memArea)
    {
        case FM_PARITY_AREA_UNDEFINED:
            return "UNDEFINED";
    
        case FM_PARITY_AREA_ARRAY:
            return "ARRAY";

        case FM_PARITY_AREA_CROSSBAR:
            return "CROSSBAR";

        case FM_PARITY_AREA_EPL:
            return "EPL";
    
        case FM_PARITY_AREA_MODIFY:
            return "MODIFY";
    
        case FM_PARITY_AREA_PARSER:
            return "PARSER";
    
        case FM_PARITY_AREA_POLICER:
            return "POLICER";

        case FM_PARITY_AREA_SCHEDULER:
            return "SCHEDULER";

        case FM_PARITY_AREA_TCAM:
            return "TCAM";

        case FM_PARITY_AREA_TUNNEL_ENGINE:
            return "TUNNEL_ENGINE";

        default:
            return "UNKNOWN";

    }   /* end switch (memArea) */

}   /* end fmParityMemAreaToText */




/*****************************************************************************/
/** fmParitySeverityToText
 * \ingroup intParity
 *
 * \desc            Returns the text representation of a parity severity code.
 *
 * \param[in]       severity is the memory type (see ''fm_paritySeverity'').
 *
 * \return          Pointer to a string representing the severity of the
 *                  parity error.
 *
 *****************************************************************************/
const char * fmParitySeverityToText(fm_paritySeverity severity)
{

    switch ( severity )
    {
        case FM_PARITY_SEVERITY_UNDEFINED:
            return "UNDEFINED";

        case FM_PARITY_SEVERITY_USER_FIXABLE:
            return "USER_FIXABLE";

        case FM_PARITY_SEVERITY_TRANSIENT:
            return "TRANSIENT";

        case FM_PARITY_SEVERITY_CUMULATIVE:
            return "CUMULATIVE";

        case FM_PARITY_SEVERITY_FATAL:
            return "FATAL";

        case FM_PARITY_SEVERITY_CORRECTED:
            return "CORRECTED";

        default:
            return "UNKNOWN";
    }

}   /* end fmParitySeverityToText */




/*****************************************************************************/
/** fmParityStatusToText
 * \ingroup intParity
 *
 * \desc            Returns the text representation of a parity status code.
 *
 * \param[in]       status is the memory type (see ''fm_parityStatus'').
 *
 * \return          Pointer to a string representing the status of the
 *                  parity error.
 *
 *****************************************************************************/
const char * fmParityStatusToText(fm_parityStatus status)
{

    switch ( status )
    {
        case FM_PARITY_STATUS_NO_ERROR_DETECTED:
            return "NO_ERROR_DETECTED";

        case FM_PARITY_STATUS_ERROR_FIXED:
            return "ERROR_FIXED";

        case FM_PARITY_STATUS_FIX_FAILED:
            return "FIX_FAILED";

        case FM_PARITY_STATUS_NO_ACTION_REQUIRED:
            return "NO_ACTION_REQUIRED";

        case FM_PARITY_STATUS_FATAL_ERROR:
            return "FATAL_ERROR";

        case FM_PARITY_STATUS_ECC_CORRECTED:
            return "ECC_CORRECTED";

        case FM_PARITY_STATUS_NO_FIX_ATTEMPTED:
            return "NO_FIX_ATTEMPTED";

        case FM_PARITY_STATUS_DEFERRED_RESET:
            return "DEFERRED_RESET";

        case FM_PARITY_STATUS_UNDEFINED:
            return "UNDEFINED";

        default:
            return "UNKNOWN";
    }

}   /* end fmParityStatusToText */




/*****************************************************************************/
/** fmDbgDumpParityErrorEvent
 * \ingroup intParity
 *
 * \desc            Dumps a parity error event structure.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       parityEvent points to the parity event object.
 *
 * \return          None
 *
 *****************************************************************************/
void fmDbgDumpParityErrorEvent(fm_int                sw,
                               fm_eventParityError * parityEvent)
{
    fm_int  i;

    FM_LOG_PRINT("\nParity event on switch %d:\n", sw);

    FM_LOG_PRINT("  Error Type  : %s\n",
                 fmParityErrTypeToText(parityEvent->errType));

    FM_LOG_PRINT("  Severity    : %s\n",
                 fmParitySeverityToText(parityEvent->paritySeverity));

    FM_LOG_PRINT("  Status      : %s\n",
                 fmParityStatusToText(parityEvent->parityStatus));

    FM_LOG_PRINT("  Memory Area : %s\n",
                 fmParityMemAreaToText(parityEvent->memoryArea));

    switch ( parityEvent->errType )
    {
        case FM_PARITY_ERRTYPE_CACHE_MISMATCH:
            FM_LOG_PRINT("  Base Addr   : %08x\n", parityEvent->baseAddr);
            break;

        case FM_PARITY_ERRTYPE_SRAM_CORRECTED:
        case FM_PARITY_ERRTYPE_SRAM_UNCORRECTABLE:
            FM_LOG_PRINT("  SRAM Number : %d\n", parityEvent->sramNo);
            break;

        default:
            break;
    }

    if ( parityEvent->numIndices != 0 )
    {
        FM_LOG_PRINT("  Indices     :");
        for ( i = 0 ; i < (fm_int)parityEvent->numIndices ; i++ )
        {
            FM_LOG_PRINT(" %d", parityEvent->tableIndices[i]);
        }
        FM_LOG_PRINT("\n");
    }

    if ( parityEvent->numValidData != 0 )
    {
        FM_LOG_PRINT("  Bad Data    :");
        for ( i = (fm_int)parityEvent->numValidData - 1 ; i >= 0 ; i-- )
        {
            FM_LOG_PRINT(" %08x", parityEvent->badData[i]);
        }
        FM_LOG_PRINT("\n");

        FM_LOG_PRINT("  Good Data   :");
        for ( i = (fm_int)parityEvent->numValidData - 1 ; i >= 0 ; i-- )
        {
            FM_LOG_PRINT(" %08x", parityEvent->cachedData[i]);
        }
        FM_LOG_PRINT("\n");
    }

    if ( parityEvent->numErrors != 0 )
    {
        FM_LOG_PRINT("  Error Count : %d\n", parityEvent->numErrors);
    }

}   /* end fmDbgDumpParityErrorEvent */




/*****************************************************************************/
/** fmGetParityErrorCounters
 * \ingroup stats
 * 
 * \chips           FM10000
 *
 * \desc            Returns the parity error counters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      counters points to an ''fm_parityErrorCounters'' structure
 *                  to be filled in by this function.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetParityErrorCounters(fm_int                  sw,
                                   fm_parityErrorCounters *counters)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PARITY, 
                     "sw=%d counters=%p\n",
                     sw,
                     (void *)counters);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if (counters == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetParityErrorCounters,
                       sw,
                       counters);

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

}   /* end fmGetParityErrorCounters */




/*****************************************************************************/
/** fmResetParityErrorCounters
 * \ingroup stats
 * 
 * \chips           FM10000
 *
 * \desc            Resets the parity error counters.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmResetParityErrorCounters(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PARITY, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->ResetParityErrorCounters, sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

}   /* end fmResetParityErrorCounters */




/*****************************************************************************/
/** fmDbgDumpParity
 * \ingroup intDiag
 *
 * \chips           FM10000
 *
 * \desc            Dumps the state of the parity error subsystem.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpParity(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpParity, sw);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgDumpParity */




/*****************************************************************************/
/** fmDbgInjectParityError
 * \ingroup intDiag
 *
 * \chips           FM10000
 *
 * \desc            Causes a parity error to occur in a specified memory.
 *
 * \param[in]       sw is the switch to operate on.
 * 
 * \param[in]       memoryName is the name of the memory in which to inject
 *                  the parity error.
 * 
 * \param[in]       index is the index within the specified memory.
 * 
 * \param[in]       errMode specifies whether this is a correctable error,
 *                  an uncorrectable error, or both.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgInjectParityError(fm_int  sw,
                                 fm_text memoryName,
                                 fm_int  index,
                                 fm_int  errMode)
{
    fm_switch * switchPtr;
    fm_status   err;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err,
                       switchPtr->DbgInjectParityError,
                       sw,
                       memoryName,
                       index,
                       errMode);

    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmDbgInjectParityError */




/*****************************************************************************/
/** fmGetParityAttribute
 * \ingroup intSwitch
 *
 * \chips           FM10000
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
 * \return          FM_ERR_INVALID_ATTRIB unrecognized attr.
 *
 *****************************************************************************/
fm_status fmGetParityAttribute(fm_int sw, fm_int attr, void * value)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PARITY,
                     "sw=%d attr=%d value=%p\n",
                     sw, attr, value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->GetParityAttribute, sw, attr, value);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

}   /* end fmGetParityAttribute */




/*****************************************************************************/
/** fmSetParityAttribute
 * \ingroup intSwitch
 *
 * \chips           FM10000
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
fm_status fmSetParityAttribute(fm_int sw, fm_int attr, void * value)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PARITY,
                     "sw=%d attr=%d value=%p\n",
                     sw, attr, value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->SetParityAttribute, sw, attr, value);

    UNLOCK_SWITCH(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

}   /* end fmSetParityAttribute */




/*****************************************************************************/
/** fmDbgDumpParityConfig
 * \ingroup intParity
 *
 * \chips           FM10000
 *
 * \desc            Shows the parity configuration.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDbgDumpParityConfig(fm_int sw)
{
    fm_switch * switchPtr;
    fm_status   err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_PARITY,
                     "sw=%d\n",
                      sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY(err, switchPtr->DbgDumpParityConfig, sw);

    UNLOCK_SWITCH(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_PARITY, err);

}   /* end fmDbgDumpParityConfig */
