/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_int.h
 * Creation Date:   April 17, 2007
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#ifndef __FM_FM_DEBUG_INT_H
#define __FM_FM_DEBUG_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/*****************************************************************************
 *  Identifies miscellaneous attributes used by the debug function.
 *  Used as an argument to fmDbgSetMiscAttribute(). 
 *****************************************************************************/
typedef enum
{
    /* Used to indicate to read the FM_PORT_MASK attribute instead
       of reading the PORT_CFG_2 register from the switch. */
    FM_DBG_ATTR_PORT_CFG2 = 0,

} fm_dbgMiscAttribute;


/**************************************************/
/** \ingroup intTypeScalar
 * A debug register dump callback function for
 * outputing or archiving the results of a
 * register dump. This function returns
 * fm_bool indicating if it is done (TRUE) or still
 * busy and there should be no intermediate dumping
 * (FALSE). Takes as arguments:
 *                                                                      \lb\lb
 *  sw - The switch from which the register
 *       was dumped.
 *                                                                      \lb\lb
 * regID - The index into the register table.
 *                                                                      \lb\lb
 * regAddress - The absolute address of the register
 *              as dumped, including any indexing.
 *                                                                      \lb\lb
 * regSize - The number of words in register.
 *                                                                      \lb\lb
 * isStatReg - TRUE if register is a statistics register.
 *                                                                      \lb\lb
 * regValue1 - low-order 64 bits of register.
 *                                                                      \lb\lb
 * regValue2 - high-order 64 bits of register.
 *                                                                      \lb\lb
 * callbackInfo - Cookie provided to callback.
 **************************************************/
typedef fm_bool (*fm_regDumpCallback)(fm_int    sw,
                                      fm_int    regID,
                                      fm_uint   regAddress,
                                      fm_int    regSize,
                                      fm_bool   isStatReg,
                                      fm_uint64 regValue1,
                                      fm_uint64 regValue2,
                                      fm_voidptr callbackInfo);


/*****************************************************************************
 * Structures and Typedefs
 *****************************************************************************/

typedef struct
{
    fm_int    regId;
    fm_uint   regAddress;
    fm_int    regSize;
    fm_bool   isStatReg;
    fm_uint64 regValue1;
    fm_uint64 regValue2;

} fmDbgFulcrumRegisterSnapshot;

typedef struct
{
    fm_int                       sw;
    fm_timestamp                 timestamp;
    fm_int                       regCount;
    fmDbgFulcrumRegisterSnapshot registers[FM_DBG_MAX_SNAPSHOT_REGS];

} fmDbgFulcrumSnapshot;


typedef struct
{
    fm_int               sw;
    fm_int               port;
    fm_int               mac;
    fm_int               lane;
    fm_int               sampleCount;
    fm_eyeDiagramSample *eyeDiagram;

} fmDbgEyeDiagram;


#ifdef FM_DBG_NEED_TRACK_FUNC

typedef struct
{
    const char *address;
    int         lineNum;
    fm_uint64   execCount;

} fmDbgExecTraceEntry;

#define FM_DBG_MAX_EXEC_TRACE_ENTRIES  500

struct _fm_debugTrace
{
    fmDbgExecTraceEntry fmDbgExecTraceTable[FM_DBG_MAX_EXEC_TRACE_ENTRIES];
    fm_int              fmDbgExecTraceEntryCount;
    fm_uint64           fmDbgExecTraceOverflows;

};

#else
struct _fm_debugTrace;
#endif

typedef struct _fm_debugTrace    fm_debugTrace;

typedef struct
{
    /* Set sw to -1 to monitor all switches. */
    fm_int    sw;
    fm_uint32 regOffset;

} fmDbgMonitoredRegInfo;

/**************************************************
 * Driver event counters.
 *
 * Note: driver events are not the same thing as
 * trace events.
 **************************************************/

typedef struct
{
    fm_uint64 numAllocs;
    fm_uint64 numFrees;
    fm_uint64 numReidentifies;

} drvEventCounter;

/**************************************************
 * Trace buffer
 *
 * The trace buffer is 4 words per entry.  The first
 * word is an event code.  The remaining 3 words are
 * event code specific.  Any value may be used for
 * all words, except the event code must never be
 * 00000000 or FFFFFFFF, which are reserved for
 * future use.
 **************************************************/
typedef struct
{
    int          eventCode;
    unsigned int data1;
    unsigned int data2;
    unsigned int data3;
    fm_timestamp timestamp;

} TRACE_ENTRY;

typedef struct
{
    int          samples;
    fm_float     avgTime;
    fm_float     minTime;
    fm_float     maxTime;
    fm_float     lastSample;

    fm_timestamp startTime;
    fm_timestamp endTime;

} fmTimerMeasurement;


typedef struct _fm_rootDebug
{
    /**************************************************
     * fm_debug.c
     **************************************************/

    /* stores diagnostic information per switch */
    fm_switchDiagnostics  fmSwitchDiagnostics[FM_MAX_NUM_SWITCHES];

    /* Stores global diagnostic information. */
    fm_globalDiagnostics  fmGlobalDiagnostics;

    /* Only used if FM_DBG_NEED_TRACK_FUNC is true */
    fm_debugTrace *       trace;

    /* used for tracking packet receive size histograms */
    int                   dbgPacketSizeDist[FM_DBG_MAX_PACKET_SIZE];

    /* generic debug subsystem lock */
    fm_lock               fmDbgLock;

    /**************************************************
     * fm_debug_regs.c
     **************************************************/
    int                   fmDbgMonitoredRegCount;
    fmDbgMonitoredRegInfo fmDbgMonitoredRegs[FM_DBG_MAX_MONITORED_REGS];

    /**************************************************
     * fm_debug_snapshots.c
     **************************************************/
    fmDbgFulcrumSnapshot *fmDbgSnapshots[FM_DBG_MAX_SNAPSHOTS];


    /**************************************************
     * fm_debug_eye_diagrams.c
     **************************************************/
    fmDbgEyeDiagram      *fmDbgEyeDiagrams[FM_DBG_MAX_EYE_DIAGRAMS];
    

    /**************************************************
     * fm_debug_trace.c
     **************************************************/
    drvEventCounter       drvEventCounters[FM_EVID_MAX];
    TRACE_ENTRY           traceBfr[FM_DBG_TRACE_BFR_SIZE];
    TRACE_ENTRY *         TBInPtr;  /* Input pointer */
    TRACE_ENTRY *         TBOutPtr; /* Output pointer */
    int                   TBcount;
    fm_timestamp          traceStartTime;

    /**************************************************
     * When TBlock is non-zero, fmDbgTracePost (which
     * can be called by tasks or ISRs) cannot write
     * to the trace buffer.  fmDbgTracePost cannot
     * touch TBlock.
     **************************************************/
    int                   TBlock;
    int                   TBmode;

    /**************************************************
     * TBtail is the number of events to add to the
     * trace buffer following a trigger event for
     * MODE_TRIGGER.
     **************************************************/
    int                   TBtailReset;
    int                   TBtail;
    int                   TBtriggerEvent;

    /**************************************************
     * Trigger table
     *
     * Event codes appearing in this table will cause
     * event capture to cease if TBmode = MODE_TRIGGER.
     **************************************************/
    int                   trigTable[5];

    /**************************************************
     * Exclusion table
     *
     * Event codes in this table will not be posted to
     * the trace buffer.  Can be used to filter the
     * trace buffer at run-time.
     **************************************************/
    int                   exclusions[FM_DBG_EXCLUSION_TABLE_SIZE];
    int                   numberOfExclusions;
    fmTimerMeasurement    dbgTimerMeas[FM_DBG_MAX_TIMER_MEAS];

    /* Event queue debugging globals. */
    fm_tree               dbgEventQueueList;
    fm_lock               dbgEventQueueListLock;

} fm_rootDebug;

extern fm_rootDebug *fmRootDebug;

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

void      fmDbgInitSnapshots(void);
void      fmDbgInitTrace(void);
fm_status fmDbgInitEyeDiagrams(void);

fm_bool fmDbgPrintRegValue(fm_int    sw,
                           fm_int    regId,
                           fm_uint   regAddress,
                           fm_int    regSize,
                           fm_bool   isStatReg,
                           fm_uint64 regValue1,
                           fm_uint64 regValue2,
                           fm_voidptr callbackInfo);

fm_status fmDbgSetMiscAttribute(fm_int sw, fm_uint attr, fm_int value);


#endif /* __FM_FM_DEBUG_INT_H */
