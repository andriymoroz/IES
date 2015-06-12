/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_parity.h
 * Creation Date:   August 15, 2014
 * Description:     Parity Error management.
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

#ifndef __FM_FM_API_PARITY_H
#define __FM_FM_API_PARITY_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/**************************************************/
/** \ingroup intTypeEnum 
 * Parity subsystem attribute. 
 * Used as an argument to fmSetParityAttribute 
 * and fmGetParityAttribute. 
 **************************************************/
typedef enum
{
    FM_PARITY_INTERRUPTS,
    FM_PARITY_REPAIRS,

    /** UNPUBLISHED: For internal use only. */
    FM_PARITY_ATTR_MAX

} fm_parityAttr;


/**************************************************/
/** \ingroup intTypeStruct
 * Switch-level parity error statistics. 
 * Used by fmGetParityErrorCounters.
 **************************************************/
typedef struct _fm_switchMemErrCounters
{
    fm_uint64   cntCrossbarErrs;
    fm_uint64   cntArrayMemoryErrs;
    fm_uint64   cntFhHeadMemoryErrs;
    fm_uint64   cntFhTailMemoryErrs;
    fm_uint64   cntModifyMemoryErrs;
    fm_uint64   cntSchedMemoryErrs;
    fm_uint64   cntEplMemoryErrs;
    fm_uint64   cntTeMemoryErrs;
    fm_uint64   cntTcamMemoryErrs;

    fm_uint64   cntTransientErrs;
    fm_uint64   cntRepairableErrs;
    fm_uint64   cntCumulativeErrs;
    fm_uint64   cntFatalErrs;

} fm_switchMemErrCounters;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/


/**************************************************
 * Public functions.
 **************************************************/

fm_status fmGetParityAttribute(fm_int sw, fm_int attr, void *value);

fm_status fmSetParityAttribute(fm_int sw, fm_int attr, void *value);

fm_status fmGetParityErrorCounters(fm_int  sw,
                                   void *  counters,
                                   fm_uint size);

fm_status fmResetParityErrorCounters(fm_int sw);

/**************************************************
 * Internal functions.
 **************************************************/

void * fmParitySweeperTask(void * args);

void * fmParityRepairTask(void * args);

fm_status fmSendParityErrorEvent(fm_int sw, 
                                 fm_eventParityError parityEvent, 
                                 fm_thread *eventHandler);

const char * fmParityErrTypeToText(fm_parityErrType errType);
const char * fmParityMemAreaToText(fm_parityMemArea memArea);
const char * fmParitySeverityToText(fm_paritySeverity severity);
const char * fmParityStatusToText(fm_parityStatus status);

/**************************************************
 * Debug functions.
 **************************************************/

fm_status fmDbgDumpParity(fm_int sw);

void fmDbgDumpParityErrorEvent(fm_int                sw,
                               fm_eventParityError * parityEvent);

fm_status fmDbgInjectParityError(fm_int  sw,
                                 fm_text memoryName,
                                 fm_int  index,
                                 fm_int  errMode);

#endif /* __FM_FM_API_PARITY_H */

