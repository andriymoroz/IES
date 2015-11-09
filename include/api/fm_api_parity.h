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
    FM_PARITY_TCAM_MONITORS,

    FM_POLICERS_CERR_ERROR,
    FM_POLICERS_CERR_FATAL,
    FM_POLICERS_UERR_ERROR,
    FM_POLICERS_UERR_FATAL,

    FM_STATS_CERR_ERROR,
    FM_STATS_CERR_FATAL,
    FM_STATS_UERR_ERROR,
    FM_STATS_UERR_FATAL,

    FM_FREELIST_UERR_ERROR,
    FM_FREELIST_UERR_FATAL,

    FM_REFCOUNT_UERR_ERROR,
    FM_REFCOUNT_UERR_FATAL,

    /** UNPUBLISHED: For internal use only. */
    FM_PARITY_ATTR_MAX

} fm_parityAttr;


/**************************************************/
/** \ingroup typeStruct
 * Switch-level parity error statistics. 
 * Used by fmGetParityErrorCounters.
 **************************************************/
typedef struct _fm_parityErrorCounters
{
    /** Number of INGRESS_XBAR_CTRL or EGRESS_XBAR_CTRL memory errors. */
    fm_uint64   cntCrossbarErrs;

    /** Number of ARRAY memory errors. */
    fm_uint64   cntArrayMemoryErrs;

    /** Number of FH_HEAD memory errors. */
    fm_uint64   cntFhHeadMemoryErrs;

    /** Number of FH_TAIL memory errors. */
    fm_uint64   cntFhTailMemoryErrs;

    /** Number of MODIFY memory errors. */
    fm_uint64   cntModifyMemoryErrs;

    /** Number of SCHEDULER memory errors. */
    fm_uint64   cntSchedMemoryErrs;

    /** Number of EPL or SERDES memory errors. */
    fm_uint64   cntEplMemoryErrs;

    /** Number of Tunneling Engine memory errors. */
    fm_uint64   cntTeMemoryErrs;

    /** Number of TCAM checksum errors in memory. */
    fm_uint64   cntTcamMemoryErrs;


    /** Number of TRANSIENT parity errors in memory. */
    fm_uint64   cntTransientErrs;

    /** Number of REPAIRABLE parity errors in memory. */
    fm_uint64   cntRepairableErrs;

    /** Number of CUMULATIVE parity errors in memory. */
    fm_uint64   cntCumulativeErrs;

    /** Number of FATAL parity errors in memory. */
    fm_uint64   cntFatalErrs;

} fm_parityErrorCounters;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/


/**************************************************
 * Public functions.
 **************************************************/

fm_status fmGetParityAttribute(fm_int sw, fm_int attr, void *value);

fm_status fmSetParityAttribute(fm_int sw, fm_int attr, void *value);

fm_status fmDbgDumpParityConfig(fm_int sw);

fm_status fmGetParityErrorCounters(fm_int                  sw,
                                   fm_parityErrorCounters *counters);

fm_status fmResetParityErrorCounters(fm_int sw);

/**************************************************
 * Internal functions.
 **************************************************/

void * fmParitySweeperTask(void *args);

void * fmParityRepairTask(void *args);

fm_status fmSendParityErrorEvent(fm_int              sw,
                                 fm_eventParityError parityEvent, 
                                 fm_thread          *eventHandler);

const char * fmParityErrTypeToText(fm_parityErrType errType);
const char * fmParityMemAreaToText(fm_parityMemArea memArea);
const char * fmParitySeverityToText(fm_paritySeverity severity);
const char * fmParityStatusToText(fm_parityStatus status);

/**************************************************
 * Debug functions.
 **************************************************/

fm_status fmDbgDumpParity(fm_int sw);

void fmDbgDumpParityErrorEvent(fm_int               sw,
                               fm_eventParityError *parityEvent);

fm_status fmDbgInjectParityError(fm_int  sw,
                                 fm_text memoryName,
                                 fm_int  index,
                                 fm_int  errMode);

#endif /* __FM_FM_API_PARITY_H */

