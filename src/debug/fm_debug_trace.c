/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_debug_trace.c
 * Creation Date:   April 27, 2006
 * Description:     Provide debugging functions.
 *
 * Copyright (c) 2006 - 2015, Intel Corporation
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

#define DRV_EVENT_COUNTER_DESC_LENGTH      \
    ( (fm_int)                             \
     ( ( sizeof(drvEventCounterDesc) /     \
        sizeof(drvEventCounterDesc[0]) ) - \
      1 ) )

#define TB_END_OF_DATA         0xffffffff


/**************************************************
 * Modes - See 'fmDbgTraceMode function' description
 * for an explanation of the various modes.
 **************************************************/

/* Modes */
#define MODE_RESERVED          0          /* Reserved value */
#define MODE_FREE_RUN          1
#define MODE_ONE_SHOT          2
#define MODE_TRIGGER           3
#define MODE_STOPPED           4
#define MODE_TRIGGERED         5
#define MODE_MAX               MODE_TRIGGERED


#define MODE_DESC_TABLE_SIZE        \
    ( (fm_int) ( sizeof(modeDesc) / \
                sizeof(modeDesc[0]) ) )

#define TB_TAIL_RESET_DEFAULT  10
#define EVENT_UNUSED           0x00000000

#define MAX_TRIGGERS                    \
    ( (fm_int)                          \
     ( sizeof(fmRootDebug->trigTable) / \
      sizeof(fmRootDebug->trigTable[0]) ) )
#define MAX_LINES_TO_DUMP      20

#define CHUNK_DUMP_PER_LINE    8


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/* NOTE: the following table must match the definition of fm_eventID! */

static const char *const drvEventCounterDesc[] =
{
    "FM_EVID_UNIDENTIFIED",
    "FM_EVID_LOW_FRAME_CTRL",
    "FM_EVID_LOW_LCI",
    "FM_EVID_LOW_MAC_SECURITY",
    "FM_EVID_HIGH_MAC_SECURITY",
    "FM_EVID_HIGH_TABLE_UPDATE",
    "FM_EVID_LOW_UPDATE_OVERFLOW",
    "FM_EVID_HIGH_EVENT_FRAME",
    "FM_EVID_HIGH_PKT_RECV",
    "FM_EVID_HIGH_PKT_RECV_NEW_BUFS",
    "FM_EVID_HIGH_PKT_SEND",
    "FM_EVID_HIGH_PORT",

    /* Add new entries above this line */
    "FM_EVID_OUT_OF_EVENTS",
    "FM_EVID_MAX"
};

static const char *const modeDesc[] =
{
    "Error! Should never be in this mode!)",                /* MODE_RESERVED  */
    "Free run (wrap when buffer full)",                     /* MODE_FREE_RUN  */
    "One shot (stop when buffer full)",                     /* MODE_ONE_SHOT  */
    "Trigger (stop when trigger event seen)",               /* MODE_TRIGGER   */
    "Stopped",                                              /* MODE_STOPPED   */
    "Triggered (stopped because trigger event seen)"        /* MODE_TRIGGERED */
};

/**************************************************
 * Event Code Description Table
 *
 * Event codes are any arbitrary value and are
 * used to identify the unique point in the code where
 * a call to fmDbgTracePost is made.
 *
 * This table can be used to print a description
 * unique to each event code.  If aux data is
 * recorded with the event code, it is recommended
 * that the description identify the meaning of
 * each data word.
 *
 * Normally, set exclude column to zero.  Change to
 * a one for any event code that you do not want to
 * be captured on initialization.  You can change
 * the exclusions at run-time.
 **************************************************/

typedef struct
{
    int         number;
    int         exclude;
    const char *name;

} EC_DESC;


static const EC_DESC eventCodeDesc[] =
{
    /*  Event Code      Exclude Description                                 */
    /*  -----------     ------- -------------------------------             */
    { 0x10000000,   1, "Example (args desc)"       },

    /* Enter new entries above this line.  Do not delete the following line. */
    { EVENT_UNUSED, 0, "(Unrecognized event code)" }
};


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
static void UnlockTB(void);
static void LockTB(void);
static void IncrementTBPtr(TRACE_ENTRY **ptrPtr);
static const char *FindECDesc(int eventCode);
static void TraceInCriticalSection(int *lockKey);
static void TraceOutCriticalSection(int lockKey);
static void ResetTraceBuffer(void);
static void TransitionMode(int mode);
static int CheckTriggerEvent(int eventCode);
static int FindTrigger(int eventCode, int *entry);
static int FindExclusion(int eventCode, int *entry);
static void DisplayMode(void);
static void DisplayTraceTime(void);
static void DisplayTriggerStatus(void);
static void DisplayExclusions(void);
static void InitTraceExclusions(void);
static void InitializeEventQueueDebugging(void);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************
 * InitTraceExclusions
 *
 * Description: Initialize the trace buffer exclusion table if it hasn't
 *              already been done.
 *
 * Arguments:   None
 *
 * Returns:     None
 *
 *****************************************************************************/
static void InitTraceExclusions(void)
{
    const EC_DESC *ecDescPtr;
    int            exclEntry;

    for (exclEntry = 0 ; exclEntry < FM_DBG_EXCLUSION_TABLE_SIZE ; exclEntry++)
    {
        fmRootDebug->exclusions[exclEntry] = EVENT_UNUSED;
        fmRootDebug->numberOfExclusions    = 0;
    }

    /**************************************************
     * Add hard-coded exclusions if any.
     **************************************************/

    ecDescPtr = eventCodeDesc;
    exclEntry = 0;

    while (ecDescPtr->number != EVENT_UNUSED
           && exclEntry < FM_DBG_EXCLUSION_TABLE_SIZE)
    {
        if (ecDescPtr->exclude)
        {
            fmRootDebug->exclusions[exclEntry] = ecDescPtr->number;
            ++exclEntry;
            ++fmRootDebug->numberOfExclusions;
        }

        ++ecDescPtr;
    }

}   /* end InitTraceExclusions */




/*********************************************************************
 * TraceInCriticalSection
 *
 *  Description:    Determine if we are at interrupt level.  If
 *                  not, take the trace buffer semaphore and
 *                  lock interrupts.
 *
 *                  This function allows the trace buffer to be
 *                  accessed from either interrupt level or task
 *                  level.
 *
 *                  This implementation can be used for
 *                  interrupt level and kernel code in VxWorks only.
 *                  We need a more sophisticated implementation to
 *                  support Linux.
 *
 *  Arguments:      lockKey points to a lock key.
 *
 *  Returns:        None.
 *
 *********************************************************************/
static void TraceInCriticalSection(int *lockKey)
{
#if 0

    if ( intContext() )
    {
        /**************************************************
         * We are executing at interrupt level.  We need
         * do nothing special since we can't be interrupted
         * anyway.
         **************************************************/

        return;
    }

    /**************************************************
     * We are in a task context.  We need to defend
     * against interrupts and task switches.
     *
     * Ideally we could use a semaphore for defending
     * against task switches, but that requires some
     * initialization and we don't have an init path
     * for this module at the moment.
     **************************************************/

    taskLock();
    *lockKey = intLock();
    return;

#else

    /* Make sure lockKey is initialized */
    *lockKey = 0;

#endif

}   /* end TraceInCriticalSection */




/*********************************************************************
 * TraceOutCriticalSection
 *
 *  Description:    Exit critical section entered with
 *                  TraceInCriticalSection.
 *
 *                  This implementation can be used for
 *                  interrupt level and kernel code in VxWorks only.
 *                  We need a more sophisticated implementation to
 *                  support Linux.
 *
 *  Arguments:      lockKey is the lock key used in the call to
 *                  TraceInCriticalSection.
 *
 *  Returns:        None.
 *
 *********************************************************************/
static void TraceOutCriticalSection(int lockKey)
{
#if 0
    intUnlock(lockKey);
    taskUnlock();
    return;

#else

    FM_NOT_USED(lockKey);

#endif

}   /* end TraceOutCriticalSection */




/**********************************************************************
 * IncrementTBPtr
 *
 * Description: Increment a trace buffer pointer, taking care to wrap
 *              if end of buffer encountered.
 *
 *              NOTE:   It is assumed that tasks and interrupts are
 *                      locked by the caller.
 *
 * Arguments:   ptrPtr is the address of the pointer to increment.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void IncrementTBPtr(TRACE_ENTRY **ptrPtr)
{
    ++(*ptrPtr);

    if (*ptrPtr >= fmRootDebug->traceBfr + FM_DBG_TRACE_BFR_SIZE)
    {
        *ptrPtr = fmRootDebug->traceBfr;
    }

}   /* end IncrementTBPtr */




/**********************************************************************
 * LockTB
 *
 * Description: Lock the trace buffer.
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void LockTB(void)
{
    int keyLock;

    TraceInCriticalSection(&keyLock);
    ++fmRootDebug->TBlock;
    TraceOutCriticalSection(keyLock);

}   /* end LockTB*/




/**********************************************************************
 * UnlockTB
 *
 * Description: Lock the trace buffer.
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void UnlockTB(void)
{
    int keyLock;

    TraceInCriticalSection(&keyLock);

    if (fmRootDebug->TBlock)
    {
        --fmRootDebug->TBlock;
    }

    TraceOutCriticalSection(keyLock);

}   /* end UnlockTB*/




/**********************************************************************
 * FindECDesc
 *
 * Description: Find event code description.
 *
 * Arguments:   eventCode is the event code to find.
 *
 * Returns:     Pointer to the description.
 *
 **********************************************************************/
static const char *FindECDesc(int eventCode)
{
    const EC_DESC *ecDescPtr;

    ecDescPtr = eventCodeDesc;

    while (ecDescPtr->number != EVENT_UNUSED && ecDescPtr->number != eventCode)
    {
        ++ecDescPtr;
    }

    return ecDescPtr->name;

}   /* end FindECDesc */




/**********************************************************************
 * ResetTraceBuffer
 *
 * Description: Reset the trace buffer so that it is empty.
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 * Note: It is assumed that the caller has wrapped the call to this
 * function between TraceInCriticalSection and TraceOutCriticalSection
 * and that the trace buffer is locked.
 *
 **********************************************************************/
void ResetTraceBuffer(void)
{
    fmRootDebug->TBInPtr        = fmRootDebug->traceBfr;
    fmRootDebug->TBOutPtr       = fmRootDebug->traceBfr;
    fmRootDebug->TBcount        = 0;
    fmRootDebug->TBtail         = 0;
    fmRootDebug->TBtriggerEvent = EVENT_UNUSED;
    fmGetTime(&fmRootDebug->traceStartTime);

}   /* end ResetTraceBuffer */




/**********************************************************************
 * TransitionMode
 *
 * Description: Transition from one TBmode value to another.  Different
 *              actions will be taken depending on what the start and
 *              end modes are.
 *
 * Arguments:   mode is the new mode.
 *
 * Returns:     None.
 *
 * Note: It is assumed that the caller has wrapped the call to this
 * function between TraceInCriticalSection and TraceOutCriticalSection
 * and that the trace buffer is locked.
 *
 **********************************************************************/
static void TransitionMode(int mode)
{
#define MODE_TRANS(from, to)  ( ( from * (MODE_MAX + 1) ) + to )

    int transition;

    transition = MODE_TRANS(fmRootDebug->TBmode, mode);

    switch (transition)
    {
        /**************************************************
         * Stopped or triggered to any other state: empty
         * the buffer first.
         **************************************************/
        case MODE_TRANS(MODE_STOPPED, MODE_FREE_RUN):
        case MODE_TRANS(MODE_STOPPED, MODE_ONE_SHOT):
        case MODE_TRANS(MODE_STOPPED, MODE_TRIGGER):
        case MODE_TRANS(MODE_TRIGGERED, MODE_FREE_RUN):
        case MODE_TRANS(MODE_TRIGGERED, MODE_ONE_SHOT):
        case MODE_TRANS(MODE_TRIGGERED, MODE_TRIGGER):
            ResetTraceBuffer();
            fmRootDebug->TBmode = mode;
            break;

            /**************************************************
             * Free-run or trigger to one-shot: empty the buffer
             * first.
             **************************************************/
        case MODE_TRANS(MODE_FREE_RUN, MODE_ONE_SHOT):
        case MODE_TRANS(MODE_TRIGGER, MODE_ONE_SHOT):
            ResetTraceBuffer();
            fmRootDebug->TBmode = mode;
            break;

            /**************************************************
             * Trigger to free-run: just keep running.
             **************************************************/
        case MODE_TRANS(MODE_TRIGGER, MODE_FREE_RUN):
            fmRootDebug->TBtail         = 0;
            fmRootDebug->TBtriggerEvent = EVENT_UNUSED;
            fmRootDebug->TBmode         = mode;
            break;

            /**************************************************
             * One-shot to trigger or free-run: If buffer is
             * already full, empty it.  Otherwise, just
             * transition.
             **************************************************/
        case MODE_TRANS(MODE_ONE_SHOT, MODE_FREE_RUN):
        case MODE_TRANS(MODE_ONE_SHOT, MODE_TRIGGER):

            if (fmRootDebug->TBcount >= FM_DBG_TRACE_BFR_SIZE)
            {
                ResetTraceBuffer();
            }

            fmRootDebug->TBtail         = 0;
            fmRootDebug->TBtriggerEvent = EVENT_UNUSED;
            fmRootDebug->TBmode         = mode;
            break;

            /**************************************************
             * Free run to trigger: just transition.
             **************************************************/
        case MODE_TRANS(MODE_FREE_RUN, MODE_TRIGGER):
            fmRootDebug->TBtail         = 0;
            fmRootDebug->TBtriggerEvent = EVENT_UNUSED;
            fmRootDebug->TBmode         = mode;
            break;

            /**************************************************
             * Any mode to stop: just stop.
             **************************************************/
        case MODE_TRANS(MODE_FREE_RUN, MODE_STOPPED):
        case MODE_TRANS(MODE_ONE_SHOT, MODE_STOPPED):
        case MODE_TRANS(MODE_TRIGGER, MODE_STOPPED):
        case MODE_TRANS(MODE_TRIGGERED, MODE_STOPPED):
            fmRootDebug->TBmode = mode;
            break;

            /**************************************************
             * From trigger to triggered.
             **************************************************/
        case MODE_TRANS(MODE_TRIGGER, MODE_TRIGGERED):
            fmRootDebug->TBmode = mode;
            break;

            /**************************************************
             * We should never be in MODE_RESERVED mode, so
             * these are non-sensical transitions, but we should
             * defend the case by recovering appropriately.
             **************************************************/
        case MODE_TRANS(MODE_RESERVED, MODE_FREE_RUN):
        case MODE_TRANS(MODE_RESERVED, MODE_ONE_SHOT):
        case MODE_TRANS(MODE_RESERVED, MODE_STOPPED):
        case MODE_TRANS(MODE_RESERVED, MODE_TRIGGER):
        case MODE_TRANS(MODE_RESERVED, MODE_TRIGGERED):
            fmRootDebug->TBmode = mode;
            break;

            /**************************************************
             * Non-sensical transitions: do nothing.
             **************************************************/
        case MODE_TRANS(MODE_FREE_RUN, MODE_TRIGGERED):
        case MODE_TRANS(MODE_ONE_SHOT, MODE_TRIGGERED):
        case MODE_TRANS(MODE_STOPPED, MODE_TRIGGERED):
        case MODE_TRANS(MODE_FREE_RUN, MODE_RESERVED):
        case MODE_TRANS(MODE_ONE_SHOT, MODE_RESERVED):
        case MODE_TRANS(MODE_STOPPED, MODE_RESERVED):
        case MODE_TRANS(MODE_TRIGGER, MODE_RESERVED):
        case MODE_TRANS(MODE_TRIGGERED, MODE_RESERVED):
            break;

            /**************************************************
             * Idempotent transitions: do nothing.
             **************************************************/

        case MODE_TRANS(MODE_RESERVED, MODE_RESERVED):
        case MODE_TRANS(MODE_FREE_RUN, MODE_FREE_RUN):
        case MODE_TRANS(MODE_ONE_SHOT, MODE_ONE_SHOT):
        case MODE_TRANS(MODE_TRIGGER, MODE_TRIGGER):
        case MODE_TRANS(MODE_STOPPED, MODE_STOPPED):
        case MODE_TRANS(MODE_TRIGGERED, MODE_TRIGGERED):
        default:
            break;

    }   /* end switch (transition) */

    /* end switch(transition) */

    return;

}   /* end TransitionMode */




/**********************************************************************
 * CheckTriggerEvent
 *
 * Description: Determine if the event is a triggering event.
 *
 * Arguments:   eventCode is the event code to check.
 *
 * Returns:     TRUE if event is a triggering event, else FALSE.
 *
 **********************************************************************/
static int CheckTriggerEvent(int eventCode)
{
    int i;
    int rtnCode = FALSE;

    /**************************************************
     * Scan the trigger table looking for this event
     * code.
     **************************************************/

    for (i = 0 ; i < MAX_TRIGGERS ; i++)
    {
        if (fmRootDebug->trigTable[i] == eventCode)
        {
            break;
        }
    }

    /**************************************************
     * If event code was found in trigger table, notify
     * caller.
     **************************************************/

    if (i < MAX_TRIGGERS)
    {
        rtnCode = TRUE;
    }

    return rtnCode;

}   /* end CheckTriggerEvent */




/**********************************************************************
 * FindTrigger
 *
 * Description: Search trigger table for event code.
 *
 * Arguments:   eventCode is the event code to search for.
 *
 *              entry is caller-allocated storage where this routine
 *              should store the index to the found eventCode, or to
 *              an available slot in the table if eventCode not found.
 *              If neither eventCode nor an empty slot are found, entry
 *              will be set to -1.
 *
 * Returns:     TRUE if found, else FALSE.
 *
 **********************************************************************/
static int FindTrigger(int eventCode, int *entry)
{
    int trigEntry;

    *entry = -1;

    for (trigEntry = 0 ; trigEntry < MAX_TRIGGERS ; trigEntry++)
    {
        if (fmRootDebug->trigTable[trigEntry] == eventCode)
        {
            *entry = trigEntry;
            return TRUE;
        }

        if (fmRootDebug->trigTable[trigEntry] == EVENT_UNUSED)
        {
            *entry = trigEntry;
        }
    }

    return FALSE;

}   /* end FindTrigger */




/**********************************************************************
 * FindExclusion
 *
 * Description: Search exclusion table for event code.
 *
 * Arguments:   eventCode is the event code to search for.
 *
 *              entry is caller-allocated storage where this routine
 *              should store the index to the found eventCode, or to
 *              an available slot in the table if eventCode not found.
 *              If neither eventCode nor an empty slot are found, entry
 *              will be set to -1.
 *
 * Returns:     TRUE if found, else FALSE.
 *
 **********************************************************************/
static int FindExclusion(int eventCode, int *entry)
{
    int exclEntry;

    /**************************************************
     * Make sure table is initialized.
     **************************************************/

    if (!fmRootDebug)
    {
        return FALSE;
    }

    *entry = -1;

    for (exclEntry = 0 ; exclEntry < FM_DBG_EXCLUSION_TABLE_SIZE ; exclEntry++)
    {
        if (fmRootDebug->exclusions[exclEntry] == eventCode)
        {
            *entry = exclEntry;
            return TRUE;
        }

        if (fmRootDebug->exclusions[exclEntry] == EVENT_UNUSED && *entry == -1)
        {
            *entry = exclEntry;
        }
    }

    return FALSE;

}   /* end FindExclusion */




/**********************************************************************
 * DisplayMode
 *
 * Description: Display the current mode and all possible modes.
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void DisplayMode(void)
{
    const char *modeStr;

    /**************************************************
     * Display the current mode.
     **************************************************/

    if (fmRootDebug->TBmode < MODE_DESC_TABLE_SIZE)
    {
        modeStr = modeDesc[fmRootDebug->TBmode];
    }
    else
    {
        modeStr = "(unknown)";
    }

    FM_LOG_PRINT("Current mode is: %d  %s.\n", fmRootDebug->TBmode, modeStr);

}   /* end DisplayMode */




/**********************************************************************
 * DisplayTraceTime
 *
 * Description: Display the trace time.
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void DisplayTraceTime(void)
{
    fm_timestamp currentTime;

    fmGetTime(&currentTime);
    FM_LOG_PRINT(
        "Trace start time (sec:msec): %" FM_FORMAT_64 "u.%06" FM_FORMAT_64
        "u, current time: %" FM_FORMAT_64 "u.%06" FM_FORMAT_64 "u.\n",
        fmRootDebug->traceStartTime.sec,
        fmRootDebug->traceStartTime.usec,
        currentTime.sec,
        currentTime.usec);

}   /* end DisplayTraceTime */




/**********************************************************************
 * DisplayTriggerStatus
 *
 * Description: Display the current trigger event codes and trigger
 *              status
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void DisplayTriggerStatus(void)
{
    int trigEntry;

    FM_LOG_PRINT("Triggering event codes:\n");

    for (trigEntry = 0 ; trigEntry < MAX_TRIGGERS ; trigEntry++)
    {
        if (fmRootDebug->trigTable[trigEntry] != EVENT_UNUSED)
        {
            FM_LOG_PRINT("   %08x\n", fmRootDebug->trigTable[trigEntry]);
        }
    }

    FM_LOG_PRINT("End of table.\n\n");

    if (fmRootDebug->TBmode == MODE_TRIGGERED)
    {
        FM_LOG_PRINT(
            "Capture stopped - triggered on event %08x %d events from end.\n",
            fmRootDebug->TBtriggerEvent, fmRootDebug->TBtailReset + 1);
    }
    else if (fmRootDebug->TBmode == MODE_TRIGGER)
    {
        if (fmRootDebug->TBtail)
        {
            FM_LOG_PRINT(
                "Capture triggered on event %08x - collecting %d more tail events.\n",
                fmRootDebug->TBtriggerEvent, fmRootDebug->TBtail);
        }
        else
        {
            FM_LOG_PRINT("Waiting for trigger event.\n");
        }
    }
    else
    {
        FM_LOG_PRINT("Not in trigger mode.\n");
    }

}   /* end DisplayTriggerStatus */




/**********************************************************************
 * DisplayExclusions
 *
 * Description: Dump the event trace exclusion table.
 *
 * Arguments:   None.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void DisplayExclusions(void)
{
    int exclEntry;

    if (!fmRootDebug)
    {
        FM_LOG_PRINT("Exclusion table not initialized.\n");
    }
    else
    {
        FM_LOG_PRINT("Excluded event codes:\n");

        for (exclEntry = 0 ;
             exclEntry < FM_DBG_EXCLUSION_TABLE_SIZE ;
             exclEntry++)
        {
            if (fmRootDebug->exclusions[exclEntry] != EVENT_UNUSED)
            {
                FM_LOG_PRINT("   %08x\n", fmRootDebug->exclusions[exclEntry]);
            }
        }

        FM_LOG_PRINT("End of table.\n\n");
    }

}   /* end DisplayExclusions */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/**********************************************************************
 * fmDbgInitTrace
 *
 * Description: performs debug facility initializations
 *
 * Arguments:   none
 *
 * Returns:     nothing
 *
 **********************************************************************/
void fmDbgInitTrace(void)
{
    fm_int i;

    /**************************************************
     * Set globals to default values
     **************************************************/
    fmRootDebug->traceBfr[0].eventCode = TB_END_OF_DATA;
    fmRootDebug->TBInPtr               = fmRootDebug->traceBfr;
    fmRootDebug->TBOutPtr              = fmRootDebug->traceBfr;
    fmRootDebug->TBmode                = MODE_FREE_RUN;
    fmRootDebug->TBtailReset           = TB_TAIL_RESET_DEFAULT;
    fmRootDebug->TBtriggerEvent        = EVENT_UNUSED;

    for (i = 0 ; i < MAX_TRIGGERS ; i++)
    {
        fmRootDebug->trigTable[i] = EVENT_UNUSED;
    }

    InitTraceExclusions();

    memset( fmRootDebug->dbgTimerMeas, 0, sizeof(fmRootDebug->dbgTimerMeas) );
    memset( fmRootDebug->dbgPacketSizeDist, 0,
           sizeof(fmRootDebug->dbgPacketSizeDist) );

    fmDbgTraceMode(1, 0);
    fmDbgTraceClear();
    InitializeEventQueueDebugging();

}   /* end fmDbgInitTrace */




/**********************************************************************
 * fmDbgDrvEventCount
 *
 * Description: Maintain diagnostic counters for the number of driver
 *              events allocated and freed.
 *
 * Arguments:   eventID identifies the event, and thus the counter
 *              to be updated.
 *
 *              alloc should be TRUE on an allocation and FALSE on free.
 *
 * Returns:     None.
 *
 **********************************************************************/
void fmDbgDrvEventCount(fm_eventID eventID, int alloc)
{
    if (eventID == FM_EVID_OUT_OF_EVENTS)
    {
        /* FM_EVID_OUT_OF_EVENTS event is treated differently:
         *  store the event count in numAllocs and the alloc value
         *  in numFrees.  This should contain some useful value for
         *  debugging */
        fmRootDebug->drvEventCounters[eventID].numAllocs++;
        fmRootDebug->drvEventCounters[eventID].numFrees = (fm_uint64) alloc;
    }
    else if (alloc)
    {
        ++fmRootDebug->drvEventCounters[eventID].numAllocs;
    }
    else
    {
        ++fmRootDebug->drvEventCounters[eventID].numFrees;
    }

}   /* end fmDbgDrvEventCount */




/*****************************************************************************/
/** fmDbgDrvEventDump
 * \ingroup diagEvents
 *
 * \desc            Display statistics on event buffer usage. These statistics
 *                  track the number of event buffers allocated and freed (by
 *                  event type) and how many times the event buffer pool went
 *                  dry.
 *
 * \note            Statistics are not switch-specific.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDrvEventDump(void)
{
    int         index;
    const char *evDesc;
    fm_uint64   totalAllocs       = 0;
    fm_uint64   totalFrees        = 0;
    fm_uint64   totalReidentifies = 0;

    FM_LOG_PRINT("\n         Allocs            Frees  "
                 "   Reidentifies  Event Type\n");

    for (index = 0 ; index < FM_EVID_MAX ; index++)
    {
        if (index > DRV_EVENT_COUNTER_DESC_LENGTH)
        {
            evDesc = "(unknown)";
        }
        else
        {
            evDesc = drvEventCounterDesc[index];
        }

        if (index < FM_EVID_OUT_OF_EVENTS)
        {
            totalAllocs       += fmRootDebug->drvEventCounters[index].numAllocs;
            totalFrees        += fmRootDebug->drvEventCounters[index].numFrees;
            totalReidentifies += fmRootDebug->drvEventCounters[index].
                                     numReidentifies;
            FM_LOG_PRINT("%15" FM_FORMAT_64 "d  %15" FM_FORMAT_64
                         "d  %15" FM_FORMAT_64 "d  %s\n",
                         fmRootDebug->drvEventCounters[index].numAllocs,
                         fmRootDebug->drvEventCounters[index].numFrees,
                         fmRootDebug->drvEventCounters[index].numReidentifies,
                         evDesc);
        }
        else
        {
            /* print totals then FM_EVID_OUT_OF_EVENTS counter */
            FM_LOG_PRINT("%15" FM_FORMAT_64 "d  %15" FM_FORMAT_64
                         "d  %15" FM_FORMAT_64 "d  %s\n\n",
                         totalAllocs, totalFrees, totalReidentifies, "Totals");
            FM_LOG_PRINT("Out-of-Events counter = %" FM_FORMAT_64
                         "d, Out-of-Events value = %" FM_FORMAT_64 "d\n",
                         fmRootDebug->drvEventCounters[index].numAllocs,
                         fmRootDebug->drvEventCounters[index].numFrees);
        }
    }

    FM_LOG_PRINT("\n");

}   /* end fmDbgDrvEventDump */




/*****************************************************************************/
/** fmDbgDrvEventClear
 * \ingroup diagEvents
 *
 * \desc            Reset the event buffer usage statistics.
 *
 * \note            Statistics are not switch-specific.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDrvEventClear(void)
{
    memset( fmRootDebug->drvEventCounters, 0,
           sizeof(fmRootDebug->drvEventCounters) );

}   /* end fmDbgDrvEventClear */




/*****************************************************************************/
/** fmDbgTraceDump
 * \ingroup diagTrace
 *
 * \desc            Display the contents of the trace buffer. The display
 *                  includes the following information for each event
 *                  posted to the trace buffer:
 *                      - Event index (into the buffer)
 *                      - Timestamp (seconds.microseconds)
 *                      - Event code (identifying the type of event)
 *                      - Three auxiliary data words
 *                      - Text description of event code and aux data words
 *
 * \note            No new events may be posted to the trace buffer while it
 *                  is being dumped so any events that occur during the
 *                  dump will be lost.
 *
 * \param[in]       start is the entry number *from the end* of the trace
 *                  buffer at which to start dumping events.  If zero,
 *                  the dump will begin from the first event in the
 *                  buffer.  If start exceeds the number of entries in the
 *                  trace buffer, it will be taken as zero and the dump will
 *                  begin from the first event in the buffer.
 *
 * \param[in]       end is the last entry *from the end* of the trace
 *                  buffer to dump.  If zero, the dump will end with the
 *                  last event in the buffer.  If end exceeds the number of
 *                  entries in the buffer or if end > start, then the dump will
 *                  fail.
 *
 * \param[in]       stop if set to 1 will set the trace mode to
 *                  MODE_STOPPED, which will freeze the trace buffer so that
 *                  no more events can be posted to it.  If set to 0, no
 *                  change will be made to the trace mode.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if invalid arguments.
 *
 *****************************************************************************/
fm_status fmDbgTraceDump(fm_int start, fm_int end, fm_int stop)
{
    const char * desc;
    fm_int       entry;
    fm_int       keyLock = 0;
    fm_int       dumpCount;
    fm_int       rtnCode = FM_OK;
    TRACE_ENTRY *outPtr;

    /**************************************************
     * Don't allow any new entries to be put in buffer
     * while we are dumping it.
     **************************************************/

    LockTB();

    /**************************************************
     * Validate arguments.
     **************************************************/

    if ( (start != 0 && end > start) || end > fmRootDebug->TBcount )
    {
        /* Invalid arguments. */
        rtnCode = FM_FAIL;
    }
    else if (start > fmRootDebug->TBcount || start == 0)
    {
        /* Ignore start argument. */
        start = fmRootDebug->TBcount;
    }

    /**************************************************
     * Proceed only if valid arguments.
     **************************************************/

    if (rtnCode == FM_OK)
    {
        /**************************************************
         * If stop argument set, then stop capture.
         **************************************************/

        if (stop)
        {
            TraceInCriticalSection(&keyLock);
            fmRootDebug->TBmode = MODE_STOPPED;
            TraceOutCriticalSection(keyLock);
        }

        /**************************************************
         * Calculate the range of entries that will be
         * dumped.
         **************************************************/

        if (end)
        {
            --end;
        }

        dumpCount = start - end;

        outPtr = fmRootDebug->TBOutPtr + fmRootDebug->TBcount - start;

        if (outPtr >= fmRootDebug->traceBfr + FM_DBG_TRACE_BFR_SIZE)
        {
            outPtr -= FM_DBG_TRACE_BFR_SIZE;
        }

        FM_LOG_PRINT("Dumping %d of %d entries from %d to %d:\n", dumpCount,
                     fmRootDebug->TBcount, start, end + 1);
        DisplayTraceTime();

        entry = start;

        while (dumpCount > 0)
        {
            FM_LOG_PRINT("%06d:  %08" FM_FORMAT_64 "u.%06" FM_FORMAT_64
                         "u  %08x  %08x  %08x  %08x",
                         entry,
                         outPtr->timestamp.sec,
                         outPtr->timestamp.usec,
                         outPtr->eventCode,
                         outPtr->data1,
                         outPtr->data2,
                         outPtr->data3);

            desc = FindECDesc(outPtr->eventCode);

            FM_LOG_PRINT("  %s\n", desc);

            IncrementTBPtr(&outPtr);
            --dumpCount;
            --entry;
        }

    }   /* end if (rtnCode == FM_OK) */

    UnlockTB();

    return rtnCode;

}   /* end fmDbgTraceDump */




/*****************************************************************************/
/** fmDbgTraceHelp
 * \ingroup diagTrace
 *
 * \desc            Display help for trace buffer commands.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgTraceHelp(void)
{
    FM_LOG_PRINT("fmDbgTraceDump(start, end, stop)\n");
    FM_LOG_PRINT(
        "    Dump the trace buffer contents starting from the 'start' entry\n");
    FM_LOG_PRINT("    from the last to the 'end' entry from the last.  If start=0,\n");
    FM_LOG_PRINT("    the dump will begin from the first entry in the buffer.  If\n");
    FM_LOG_PRINT("    end=0, the dump will end with the last entry in the buffer.\n");
    FM_LOG_PRINT(
        "    start must be greater than or equal to end.  If stop is non-zero,\n");
    FM_LOG_PRINT("    trace capture will be halted.\n\n");
    FM_LOG_PRINT("fmDbgTraceClear\n");
    FM_LOG_PRINT("    Empty the trace buffer of all contents.\n\n");
    FM_LOG_PRINT("fmDbgTraceMode(mode, tail)\n");
    FM_LOG_PRINT("    Set capture mode:\n");
    FM_LOG_PRINT("        0  (Display the current mode.)\n");
    FM_LOG_PRINT("        1  Free run (wrap when buffer full).\n");
    FM_LOG_PRINT("        2  One shot (stop when buffer full).\n");
    FM_LOG_PRINT("        3  Trigger - like 1, but stop on trigger event.\n");
    FM_LOG_PRINT("        4  Stop (cease capturing events).\n");
    FM_LOG_PRINT("    'tail' is the number of events to capture after\n");
    FM_LOG_PRINT("    seeing a trigger event before stopping.  If zero, it\n");
    FM_LOG_PRINT("    is not changed.  Defaults to %d.\n\n",
                 TB_TAIL_RESET_DEFAULT);
    FM_LOG_PRINT("fmDbgTraceTrigger(eventCode, addOrDelete)\n");
    FM_LOG_PRINT("    Add, delete or dipslay trigger events.  'eventCode is the\n");
    FM_LOG_PRINT("    event being added or deleted.  If zero, trigger status and\n");
    FM_LOG_PRINT(
        "    table are displayed.  'addOrDelete' should be 1 to add, 0 to delete.\n");
    FM_LOG_PRINT("    Ignored if eventCode is zero.  Up to %d event codes can be\n",
                 MAX_TRIGGERS);
    FM_LOG_PRINT("    specified as triggers.\n\n");
    FM_LOG_PRINT("fmDbgTraceExclude(eventCode, addOrDelete)\n");
    FM_LOG_PRINT("    Add, delete or dipslay excluded events.  'eventCode' is the\n");
    FM_LOG_PRINT("    event being added or deleted.  If zero, exclusion\n");
    FM_LOG_PRINT(
        "    table is displayed.  'addOrDelete' should be 1 to add, 0 to delete.\n");
    FM_LOG_PRINT("    Ignored if eventCode is zero.  Up to %d event codes can be\n",
                 FM_DBG_EXCLUSION_TABLE_SIZE);
    FM_LOG_PRINT("    excluded.\n\n");
    FM_LOG_PRINT("fmDbgTraceStatus\n");
    FM_LOG_PRINT("    Display current trace status (mode, triggers, exclusions).\n\n");

    return;

}   /* end fmDbgTraceHelp */




/*****************************************************************************/
/** fmDbgTraceStatus
 * \ingroup diagTrace
 *
 * \desc            Display the current trace buffer status. The display
 *                  includes the following information:
 *                      - Trace mode
 *                      - Trace start and current time (seconds:microseconds)
 *                      - Trigger event code list and trigger state
 *                      - Excluded event code list
 *                      - Size of the trace buffer
 *                      - Number of events in the trace buffer
 *
 * \note            This function does not affect the posting of new events to
 *                   the trace buffer.  Accordingly, it is preferred over
 *                   fmDbgTraceDump for determining the number of events
 *                   currently in the trace buffer.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgTraceStatus(void)
{
    DisplayMode();
    FM_LOG_PRINT("\n");
    DisplayTraceTime();
    FM_LOG_PRINT("\n");
    DisplayTriggerStatus();
    FM_LOG_PRINT("\n");
    DisplayExclusions();
    FM_LOG_PRINT("%d events in trace buffer.  Trace buffer size is %d events.\n",
                 fmRootDebug->TBcount, FM_DBG_TRACE_BFR_SIZE);
    return;

}   /* end fmDbgTraceStatus */




/*****************************************************************************/
/** fmDbgTracePost
 * \ingroup diagTrace
 *
 * \desc            Post an event to the trace buffer. This function may be
 *                  called from any place in the driver, API, ALPS or
 *                  application.
 *
 * \param[in]       eventCode identifies the specific event being posted.
 *                  Normally, the event code should be unique for every call
 *                  to this function, but there is no enforcement of this
 *                  rule.  Generally it is desireable to provide a text
 *                  description of the event being posted in the eventCodeDesc
 *                  table.  Accordingly, the eventCodeDesc table can be used
 *                  to manage the allocation of unique event codes.
 *
 *
 * \param[in]       data1 is the first word of auxiliary data.  Generally set
 *                  to zero if not used.
 *
 * \param[in]       data2 is the second word of auxiliary data.  Generally set
 *                  to zero if not used.
 *
 * \param[in]       data3 is the third word of auxiliary data.  Generally set
 *                  to zero if not used.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if invalid arguments.
 *
 *****************************************************************************/
fm_status fmDbgTracePost(fm_int    eventCode,
                         fm_uint32 data1,
                         fm_uint32 data2,
                         fm_uint32 data3)
{
    fm_timestamp currentTime;
    int          keyLock;
    int          eventExcluded;
    int          exclEntry;
    int          rtnCode = FM_FAIL;

    fmGetTime(&currentTime);
    TraceInCriticalSection(&keyLock);

    eventExcluded = FindExclusion(eventCode, &exclEntry);

    if (!fmRootDebug->TBlock && !eventExcluded)
    {
        switch (fmRootDebug->TBmode)
        {
            /**************************************************
             * Add new event to trace buffer.  If buffer full,
             * overwrite the oldest entry.
             **************************************************/
            case MODE_FREE_RUN:

                if (fmRootDebug->TBcount < FM_DBG_TRACE_BFR_SIZE)
                {
                    /* Buffer not full. */
                    ++fmRootDebug->TBcount;
                }
                else
                {
                    /* Buffer full. */
                    IncrementTBPtr(&fmRootDebug->TBOutPtr);
                }

                /* Add the new event. */
                fmRootDebug->TBInPtr->eventCode = eventCode;
                fmRootDebug->TBInPtr->data1     = data1;
                fmRootDebug->TBInPtr->data2     = data2;
                fmRootDebug->TBInPtr->data3     = data3;
                fmRootDebug->TBInPtr->timestamp = currentTime;
                IncrementTBPtr(&fmRootDebug->TBInPtr);
                rtnCode = FM_OK;
                break;

                /**************************************************
                 * Add new event to trace buffer, unless it is
                 * already full.
                 **************************************************/
            case MODE_ONE_SHOT:

                if (fmRootDebug->TBcount < FM_DBG_TRACE_BFR_SIZE)
                {
                    fmRootDebug->TBInPtr->eventCode = eventCode;
                    fmRootDebug->TBInPtr->data1     = data1;
                    fmRootDebug->TBInPtr->data2     = data2;
                    fmRootDebug->TBInPtr->data3     = data3;
                    fmRootDebug->TBInPtr->timestamp = currentTime;
                    IncrementTBPtr(&fmRootDebug->TBInPtr);
                    ++fmRootDebug->TBcount;
                    rtnCode = FM_OK;
                }
                else
                {
                    fmRootDebug->TBmode = MODE_STOPPED;
                }

                break;

                /**************************************************
                 * Add new event to trace buffer.  If buffer full,
                 * overwrite the oldest entry.  Watch for trigger
                 * event.  If trigger seen, count down TBtail number
                 * of additional events before transitioning to
                 * stopped mode.
                 **************************************************/
            case MODE_TRIGGER:

                if (fmRootDebug->TBcount < FM_DBG_TRACE_BFR_SIZE)
                {
                    /* Buffer not full. */
                    ++fmRootDebug->TBcount;
                }
                else
                {
                    /* Buffer full. */
                    IncrementTBPtr(&fmRootDebug->TBOutPtr);
                }

                /* Add the new event. */
                fmRootDebug->TBInPtr->eventCode = eventCode;
                fmRootDebug->TBInPtr->data1     = data1;
                fmRootDebug->TBInPtr->data2     = data2;
                fmRootDebug->TBInPtr->data3     = data3;
                fmRootDebug->TBInPtr->timestamp = currentTime;
                IncrementTBPtr(&fmRootDebug->TBInPtr);
                rtnCode = FM_OK;

                /**************************************************
                 * If not already triggered, see if this is a
                 * triggering event.
                 **************************************************/

                if ( !fmRootDebug->TBtail && CheckTriggerEvent(eventCode) )
                {
                    /* Add 1 for this event. */
                    fmRootDebug->TBtail         = fmRootDebug->TBtailReset + 1;
                    fmRootDebug->TBtriggerEvent = eventCode;
                }

                /**************************************************
                 * If we've already triggered, count down the
                 * number of tail events.
                 **************************************************/

                if (fmRootDebug->TBtail)
                {
                    --fmRootDebug->TBtail;

                    if (!fmRootDebug->TBtail)
                    {
                        /**************************************************
                         * We just captured the last tail event.  Transition
                         * to MODE_TRIGGERED state.
                         **************************************************/

                        fmRootDebug->TBmode = MODE_TRIGGERED;
                    }
                }

                break;


                /**************************************************
                 * Trace stopped.  Do not add event to trace
                 * buffer.
                 **************************************************/
            case MODE_STOPPED:
            case MODE_TRIGGERED:
                break;

                /**************************************************
                 * Invalid mode!
                 **************************************************/
            default:
                break;

        }   /* end switch (fmRootDebug->TBmode) */

        /* end switch(fmRootDebug->TBmode) */

    }

    /* end if (!fmRootDebug->TBlock) */

    TraceOutCriticalSection(keyLock);

    return rtnCode;

}   /* end fmDbgTracePost */




/*****************************************************************************/
/** fmDbgTraceClear
 * \ingroup diagTrace
 *
 * \desc            Clear the trace buffer of all events. This function may be
 *                  called from any place in the driver, API, ALPS or
 *                  application.
 *
 * \param           None.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if trace buffer locked.
 *
 *****************************************************************************/
fm_status fmDbgTraceClear(void)
{
    int keyLock;
    int rtnCode = FM_OK;

    TraceInCriticalSection(&keyLock);

    if (!fmRootDebug->TBlock)
    {
        LockTB();
        ResetTraceBuffer();
        UnlockTB();
    }
    else
    {
        rtnCode = FM_FAIL;
    }

    TraceOutCriticalSection(keyLock);

    return rtnCode;

}   /* end fmDbgTraceClear */




/*****************************************************************************/
/** fmDbgTraceMode
 * \ingroup diagTrace
 *
 * \desc            Set the trace buffer mode of operation.
 *
 * \param[in]       mode is the mode to set:
 *                      - 0 Display the current mode without changing it.
 *                      - 1 (MODE_FREE_RUN) After trace buffer fills, new
 *                           events will overwrite the oldest events.
 *                      - 2 (MODE_ONE_SHOT) After the trace buffer fills, no
 *                          new events will be written to the buffer.
 *                      - 3 (MODE_TRIGGER) Same as free run, but if an event
 *                          is written to the buffer that appears in the
 *                          trigger table, "tail" number of events will
 *                          be written, then no more.
 *                      - 4 (MODE_STOPPED) Freeze the trace buffer (stop
 *                          adding events to the buffer).
 *
 * \param[in]       tail is the number of events to capture after seeing a
 *                  trigger event for mode 2.  Ignored if zero or if mode
 *                  is not 2.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if trace buffer locked.
 *
 *****************************************************************************/
fm_status fmDbgTraceMode(fm_int mode, fm_int tail)
{
    int keyLock;
    int rtnCode = FM_OK;

    /**************************************************
     * If not display-only, set the new mode.
     **************************************************/

    if (mode != MODE_RESERVED)
    {
        TraceInCriticalSection(&keyLock);

        if (!fmRootDebug->TBlock)
        {
            LockTB();

            TransitionMode(mode);

            if (tail != 0 && mode == MODE_TRIGGER)
            {
                fmRootDebug->TBtailReset = tail;
            }

            UnlockTB();
        }
        else
        {
            rtnCode = FM_FAIL;
        }

        TraceOutCriticalSection(keyLock);

    }

    /* end if (mode != -1) */


#if 0

    if (rtnCode == FM_OK)
    {
        DisplayMode();

        /**************************************************
         * Indicate what the possible modes are.
         **************************************************/
        FM_LOG_PRINT("Possible modes:\n");
        FM_LOG_PRINT("        0  (Display the current mode.)\n");
        FM_LOG_PRINT("        1  Free run (wrap when buffer full).\n");
        FM_LOG_PRINT("        2  One shot (stop when buffer full).\n");
        FM_LOG_PRINT("        3  Trigger - like 0, but stop on trigger event.\n");
        FM_LOG_PRINT("        4  Stop (cease capturing events).\n");

    }   /* end if (rtnCode == FM_OK) */

#endif


    return rtnCode;

}   /* end fmDbgTraceMode */




/*****************************************************************************/
/** fmDbgTraceTrigger
 * \ingroup diagTrace
 *
 * \desc            Add or delete an event code from the trace buffer trigger
 *                  table or display the trigger table and trigger status.
 *
 * \param[in]       eventCode is the event code to add or delete.
 *                  If zero, the trigger table and trigger status will be
 *                  displayed.
 *
 * \param[in]       addOrDelete if 1, add the event code, if 0, delete
 *                  the event code.  Ignored if eventCode is zero.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if adding and the table is full or deleting and
 *                  the entry was not found.
 *
 *****************************************************************************/
fm_status fmDbgTraceTrigger(fm_int eventCode, fm_int addOrDelete)
{
    int trigEntry;
    int rtnCode = FM_OK;

    if (eventCode)
    {
        /**************************************************
         * Add or delete new trigger event.
         **************************************************/

        if (addOrDelete)
        {
            /**************************************************
             * Add new trigger.
             **************************************************/

            /* If not already in table... */
            if ( !FindTrigger(eventCode, &trigEntry) )
            {
                /* ...and there is a slot available... */
                if (trigEntry != -1)
                {
                    /* ...then add it. */
                    fmRootDebug->trigTable[trigEntry] = eventCode;
                }
                else
                {
                    /* Indicate table full. */
                    rtnCode = FM_FAIL;

                }

                /* end if (trigEntry != -1) */

            }

            /* end if (!FindTrigger(eventCode, &trigEntry)) */
        }
        else
        {
            /**************************************************
             * Delete trigger.
             **************************************************/

            /* Make sure entry is in table. */
            if ( FindTrigger(eventCode, &trigEntry) && trigEntry >= 0 )
            {
                /* Delete it. */
                fmRootDebug->trigTable[trigEntry] = EVENT_UNUSED;
            }
            else
            {
                /* Entry not in table. */
                rtnCode = FM_FAIL;
            }
        }

    }

    /* end if (eventCode) */

    /**************************************************
     * Dump trigger table and status.
     **************************************************/

    if (addOrDelete == 0 || MAX_TRIGGERS < MAX_LINES_TO_DUMP)
    {
        DisplayTriggerStatus();
    }

    /* end if (addOrDelete == 0 || MAX_TRIGGERS < MAX_LINES_TO_DUMP) */

    return rtnCode;

}   /* end fmDbgTraceTrigger */




/*****************************************************************************/
/** fmDbgTraceExclude
 * \ingroup diagTrace
 *
 * \desc            Add or delete an event code from the trace buffer exclusion
 *                  table or display the exclusion table contents.
 *
 * \param[in]       eventCode is the event code to add or delete.
 *                  If zero, the exclusion table will be displayed.
 *
 * \param[in]       addOrDelete if 1, add the event code, if 0, delete
 *                  the event code.  Ignored if eventCode is zero.
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if adding and the table is full or deleting and
 *                  the entry was not found.
 *
 *****************************************************************************/
fm_status fmDbgTraceExclude(fm_int eventCode, fm_int addOrDelete)
{
    int exclEntry = 0;
    int rtnCode = FM_OK;

    if (eventCode)
    {
        /**************************************************
         * Add or delete new excluded event.
         **************************************************/

        if (addOrDelete)
        {
            /**************************************************
             * Add new trigger.
             **************************************************/

            /* If not already in table... */
            if ( !FindExclusion(eventCode, &exclEntry) )
            {
                /* ...and there is a slot available... */
                if (exclEntry != -1)
                {
                    /* ...then add it. */
                    fmRootDebug->exclusions[exclEntry] = eventCode;
                    ++fmRootDebug->numberOfExclusions;
                }
                else
                {
                    /* Indicate table full. */
                    rtnCode = FM_FAIL;

                }

                /* end if (exclEntry != -1) */

            }

            /* end if (!FindExclusion(eventCode, &exclEntry)) */
        }
        else
        {
            /**************************************************
             * Delete trigger.
             **************************************************/

            /* Make sure entry is in table. */
            if ( FindExclusion(eventCode, &exclEntry) )
            {
                /* Delete it. */
                fmRootDebug->exclusions[exclEntry] = EVENT_UNUSED;

                if (fmRootDebug->numberOfExclusions > 0)
                {
                    --fmRootDebug->numberOfExclusions;
                }
            }
            else
            {
                /* Entry not in table. */
                rtnCode = FM_FAIL;
            }
        }

    }

    /* end if (eventCode) */

    /**************************************************
     * Dump exclusion table and status.
     **************************************************/

    if (addOrDelete == 0 || fmRootDebug->numberOfExclusions < MAX_LINES_TO_DUMP)
    {
        DisplayExclusions();
    }

    return rtnCode;

}   /* end fmDbgTraceExclude */




/*****************************************************************************
 * fmDbgTimerReset
 *
 * Description: Clears one or more timer statistics
 *
 * Arguments:   Timer # or -1 for all
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgTimerReset(int index)
{
    int i;

    if ( (index != -1) && ( (index >= 0) && (index < FM_DBG_MAX_TIMER_MEAS) ) )
    {
        fmRootDebug->dbgTimerMeas[index].avgTime = 0.0;
        fmRootDebug->dbgTimerMeas[index].minTime = -1.0;
        fmRootDebug->dbgTimerMeas[index].maxTime = 0.0;
        fmRootDebug->dbgTimerMeas[index].samples = 0;

    }
    else if (index == -1)
    {
        for (i = 0 ; i < FM_DBG_MAX_TIMER_MEAS ; i++)
        {
            fmRootDebug->dbgTimerMeas[i].avgTime = 0.0;
            fmRootDebug->dbgTimerMeas[i].minTime = -1.0;
            fmRootDebug->dbgTimerMeas[i].maxTime = 0.0;
            fmRootDebug->dbgTimerMeas[i].samples = 0;
        }
    }

}   /* end fmDbgTimerReset */




/*****************************************************************************
 * fmDbgTimerDump
 *
 * Description: Prints out a timer statistics
 *
 * Arguments:   None.
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgTimerDump()
{
    int i;

    FM_LOG_PRINT("TIMER %-10s %-10s %-10s %-10s\n", "AVG", "MIN", "MAX", "LSMP");
    FM_LOG_PRINT("---------------------------------------------------\n");

    for (i = 0 ; i < FM_DBG_MAX_TIMER_MEAS ; i++)
    {
        FM_LOG_PRINT("%-5d %-10f %-10f %-10f %-10f\n",
                     i,
                     fmRootDebug->dbgTimerMeas[i].avgTime,
                     fmRootDebug->dbgTimerMeas[i].minTime,
                     fmRootDebug->dbgTimerMeas[i].maxTime,
                     fmRootDebug->dbgTimerMeas[i].lastSample);
    }

}   /* end fmDbgTimerDump */




/*****************************************************************************
 * fmDbgTimerBeginSample
 *
 * Description: Adds a sample to the timer
 *
 * Arguments:   Timer # to start measuring at
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgTimerBeginSample(int index)
{
    if ( (index >= 0) && (index < FM_DBG_MAX_TIMER_MEAS) )
    {
        fmGetTime(&fmRootDebug->dbgTimerMeas[index].startTime);
    }

}   /* end fmDbgTimerBeginSample */




/*****************************************************************************
 * fmDbgTimerEndSample
 *
 * Description: Ends a sample to the timer
 *
 * Arguments:   Timer # to stop measuring at
 *
 * Returns:     nothing
 *
 *****************************************************************************/
void fmDbgTimerEndSample(int index)
{
    fm_float     sample;
    fm_timestamp sampleTimestamp;

    if ( (index >= 0) && (index < FM_DBG_MAX_TIMER_MEAS) )
    {
        fmGetTime(&fmRootDebug->dbgTimerMeas[index].endTime);

        if ( (fmRootDebug->dbgTimerMeas[index].startTime.sec == 0) &&
            (fmRootDebug->dbgTimerMeas[index].endTime.usec == 0) )
        {
            return;
        }

        fmSubTimestamps(&fmRootDebug->dbgTimerMeas[index].endTime,
                        &fmRootDebug->dbgTimerMeas[index].startTime,
                        &sampleTimestamp);

        sample = sampleTimestamp.sec + 1.0e-6f * (fm_float) sampleTimestamp.usec;

        fmRootDebug->dbgTimerMeas[index].lastSample = sample;

        if (fmRootDebug->dbgTimerMeas[index].samples > 0)
        {
            fmRootDebug->dbgTimerMeas[index].avgTime =
                ( (fmRootDebug->dbgTimerMeas[index].avgTime * fmRootDebug->
                       dbgTimerMeas[index].samples) +
                 sample ) / (fmRootDebug->dbgTimerMeas[index].samples + 1);
            fmRootDebug->dbgTimerMeas[index].samples++;
        }
        else
        {
            fmRootDebug->dbgTimerMeas[index].samples++;
            fmRootDebug->dbgTimerMeas[index].avgTime = sample;
        }

        if ( (sample < fmRootDebug->dbgTimerMeas[index].minTime) ||
            (fmRootDebug->dbgTimerMeas[index].minTime == -1) )
        {
            fmRootDebug->dbgTimerMeas[index].minTime = sample;
        }

        if (sample > fmRootDebug->dbgTimerMeas[index].maxTime)
        {
            fmRootDebug->dbgTimerMeas[index].maxTime = sample;
        }
    }

}   /* end fmDbgTimerEndSample */




/* Event queue debug API
 */
void InitializeEventQueueDebugging(void)
{
    fmCreateLock("Event Queue Debug", &fmRootDebug->dbgEventQueueListLock);
    fmTreeInit(&fmRootDebug->dbgEventQueueList);

}   /* InitializeEventQueueDebugging */




void fmDbgEventQueueCreated(fm_eventQueue *inQueue)
{
    fm_status err;

    fmCaptureLock(&fmRootDebug->dbgEventQueueListLock, 0);

    /* Add the event queue to the listing. */
    err = fmTreeInsert(&fmRootDebug->dbgEventQueueList,
                       (fm_uint64) (unsigned long) inQueue,
                       NULL);

    if (err != FM_OK)
    {
        FM_LOG_PRINT("FAIL: fmDbgEventQueueCreated: fmTreeInsert failed "
                     "with '%s' for queue '%s'\n",
                     fmErrorMsg(err),
                     inQueue->name);
    }

    /* Initialize the event queues debug information */
    inQueue->totalEventsPosted = 0;
    inQueue->totalEventsPopped = 0;
    inQueue->maxSize           = 0; 
    inQueue->avgTime           = 0;
    inQueue->minTime           = -1;
    inQueue->maxTime           = -1;

    fmReleaseLock(&fmRootDebug->dbgEventQueueListLock);

}   /* end fmDbgEventQueueCreated */




void fmDbgEventQueueDestroyed(fm_eventQueue *inQueue)
{
    fm_status err;

    fmCaptureLock(&fmRootDebug->dbgEventQueueListLock, 0);

    err = fmTreeRemove(&fmRootDebug->dbgEventQueueList,
                       (fm_uint64) (unsigned long) inQueue,
                       NULL);

    if (err != FM_OK)
    {
        FM_LOG_PRINT( "fmDbgEventQueueDestroyed: fmTreeRemove failed with '%s'\n",
                     fmErrorMsg(err) );
    }

    fmReleaseLock(&fmRootDebug->dbgEventQueueListLock);

}   /* end fmDbgEventQueueDestroyed */




void fmDbgEventQueueDump(void)
{
    fm_eventQueue * eventQueue;
    fm_treeIterator it;
    fm_status       err;
    fm_uint64       nextKey;
    void *          nextValue;

    fmCaptureLock(&fmRootDebug->dbgEventQueueListLock, 0);

    FM_LOG_PRINT("Avg. Time (s)| Min. Time (s)| Max Time. (s)| Posted | Popped | In flight | Max In Flight | Name\n");
    FM_LOG_PRINT("-----------------------------------------------------------------------------------------------\n"); 
    /* Just print out the event queue name for now. */
    for (fmTreeIterInit(&it, &fmRootDebug->dbgEventQueueList) ;
         ( err = fmTreeIterNext(&it, &nextKey, &nextValue) ) == FM_OK ; )
    {
        eventQueue = (fm_eventQueue *) (unsigned long) nextKey;

        if (eventQueue && eventQueue->name)
        {
            FM_LOG_PRINT("%13.8g|%14.8g|%14.8g|%8d|%8d|%11d|%15d| %s\n",             
                         eventQueue->avgTime,
                         eventQueue->minTime,
                         eventQueue->maxTime,
                         eventQueue->totalEventsPosted,
                         eventQueue->totalEventsPopped,                          
                         eventQueue->totalEventsPosted
                         - eventQueue->totalEventsPopped,
                         eventQueue->maxSize,                      
                         eventQueue->name);
        }
    }

    if (err != FM_ERR_NO_MORE)
    {
        FM_LOG_PRINT( "fmDbgEventQueueDump: fmTreeIterNext failed with '%s'\n",
                     fmErrorMsg(err) );
    }

    fmReleaseLock(&fmRootDebug->dbgEventQueueListLock);

}   /* end fmDbgEventQueueDump */




void fmDbgEventQueueEventPopped(fm_eventQueue *inQueue, fm_event *event)
{
    fm_timestamp deltaTimestamp;
    fm_float     deltaTime;

    fmSubTimestamps(&event->poppedTimestamp,
                    &event->postedTimestamp,
                    &deltaTimestamp);
    deltaTime = deltaTimestamp.sec + 1.0e-6f * (fm_float) deltaTimestamp.usec;

    if (inQueue->totalEventsPopped > 0)
    {
        inQueue->avgTime =
            ( (inQueue->avgTime * inQueue->totalEventsPopped) +
             deltaTime ) / (inQueue->totalEventsPopped + 1);
        inQueue->totalEventsPopped++;
    }
    else
    {
        inQueue->totalEventsPopped++;
        inQueue->avgTime = deltaTime;
    }

    if ( (deltaTime < inQueue->minTime) ||
        (inQueue->minTime == -1) )
    {
        inQueue->minTime = deltaTime;
    }

    if (deltaTime > inQueue->maxTime)
    {
        inQueue->maxTime = deltaTime;
    }

}   /* end fmDbgEventQueueEventPopped */
