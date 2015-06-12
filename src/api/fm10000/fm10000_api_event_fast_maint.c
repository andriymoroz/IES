/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_event_fast_maint.c
 * Creation Date:   September 19, 2013
 * Description:     Fast Maintenance task for FM10000.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

/* The number of MA Table entries represented by one word in the
 * MAC Address Used Table. */
#define ENTRIES_PER_WORD    32

/* The number of entries in the MAC Address Used Table.
 * Note that we are only interested in the SMAC table. */
#define USED_TABLE_SIZE     FM10000_MA_USED_TABLE_ENTRIES_0

/* The number of registers the sweeper should process per invocation.
 * Should be an integral divisor of USED_TABLE_SIZE (512). This could
 * eventually become a field in the switch extension structure. */
#define USED_TABLE_SAMPLE_SIZE          (USED_TABLE_SIZE / 8)

/* The number of registers the sweeper should read and process at a time.
 * Controls the granularity with which we hold the L2_LOCK. Should be an
 * integral divisor of USED_TABLE_SAMPLE_SIZE. This could eventually 
 * become a field in the switch extension structure. */ 
#define USED_TABLE_SAMPLE_UNIT          1


#if 0
#define FM_AAK_API_FM10000_MA_USED_TABLE_SAMPLE_SIZE    "api.FM10000.usedTableSampleSize"
#define FM_AAT_API_FM10000_MA_USED_TABLE_SAMPLE_SIZE    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_MA_USED_TABLE_SAMPLE_SIZE    (512 / 8)

#define FM_AAK_API_FM10000_MA_USED_TABLE_SAMPLE_UNIT    "api.FM10000.usedTableSampleUnit"
#define FM_AAT_API_FM10000_MA_USED_TABLE_SAMPLE_UNIT    FM_API_ATTR_INT
#define FM_AAD_API_FM10000_MA_USED_TABLE_SAMPLE_UNIT    1
#endif

#define FM_AAK_API_FM10000_MA_USED_TABLE_AGING_FACTOR   "api.FM10000.usedTableAgingFactor"
#define FM_AAT_API_FM10000_MA_USED_TABLE_AGING_FACTOR   FM_API_ATTR_FLOAT
#define FM_AAD_API_FM10000_MA_USED_TABLE_AGING_FACTOR   0.5

#define FM_AAK_API_FM10000_MA_USED_TABLE_EXPIRY_FACTOR  "api.FM10000.usedTableExpiryFactor"
#define FM_AAT_API_FM10000_MA_USED_TABLE_EXPIRY_FACTOR  FM_API_ATTR_FLOAT
#define FM_AAD_API_FM10000_MA_USED_TABLE_EXPIRY_FACTOR  1.05

#define FM_AAK_API_FM10000_MA_USED_TABLE_READY_FACTOR   "api.FM10000.usedTableReadyFactor"
#define FM_AAT_API_FM10000_MA_USED_TABLE_READY_FACTOR   FM_API_ATTR_FLOAT
#define FM_AAD_API_FM10000_MA_USED_TABLE_READY_FACTOR   0.5


enum _fm_usedSweeperState
{
    /* Sweeper not initialized. */
    FM_USED_SWEEPER_INITIAL = 0,

    /* Waiting to start. */
    FM_USED_SWEEPER_IDLE,

    /* Sweep in progress. */
    FM_USED_SWEEPER_ACTIVE,

    /* Waiting for next sweep. */
    FM_USED_SWEEPER_READY,

};

typedef struct
{
    fm_int  young;
    fm_int  old;
    fm_int  expired;

} fm_sweepStats;


/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** EnterActiveState
 * \ingroup intFastMaint
 *
 * \desc            Make the transition to ACTIVE state.
 *
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       currentTime is the current value of the aging timer.
 *
 * \return          None.
 *
 *****************************************************************************/
static void EnterActiveState(fm_int sw, fm_uint64 currentTime)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_float        agingTicks;
    fm_float        agingFactor;
    fm_float        expiryFactor;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    agingFactor =
        fmGetFloatApiProperty(FM_AAK_API_FM10000_MA_USED_TABLE_AGING_FACTOR,
                              FM_AAD_API_FM10000_MA_USED_TABLE_AGING_FACTOR);

    expiryFactor =
        fmGetFloatApiProperty(FM_AAK_API_FM10000_MA_USED_TABLE_EXPIRY_FACTOR,
                              FM_AAD_API_FM10000_MA_USED_TABLE_EXPIRY_FACTOR);

    agingTicks = (fm_float) switchPtr->macAgingTicks;

    switchExt->usedTableAgingTime  = (fm_uint64) (agingTicks * agingFactor);
    switchExt->usedTableExpiryTime = (fm_uint64) (agingTicks * expiryFactor);

#if 0
    switchExt->usedTableSampleSize =
        fmGetIntApiProperty(FM_AAK_API_FM10000_MA_USED_TABLE_SAMPLE_SIZE,
                            FM_AAD_API_FM10000_MA_USED_TABLE_SAMPLE_SIZE);

    switchExt->usedTableSampleCount =
        fmGetIntApiProperty(FM_AAK_API_FM10000_MA_USED_TABLE_SAMPLE_COUNT,
                            FM_AAD_API_FM10000_MA_USED_TABLE_SAMPLE_COUNT);
#endif

    switchExt->usedTableSweeperIndex = 0;
    switchExt->usedTableNumExpired = 0;
    switchExt->usedTableLastSweepTime = currentTime;

    switchExt->usedTableSweeperState = FM_USED_SWEEPER_ACTIVE;

    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT,
                 "sw=%d state=ACTIVE currentTime=%llu\n",
                 sw,
                 currentTime);

}   /* end EnterActiveState */




/*****************************************************************************/
/** ProcessSample
 * \ingroup intFastMaint
 *
 * \desc            Processes a register sample from the MA_USED_TABLE.
 *
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       index is the index of the first entry in the
 *                  MA_USED_TABLE to be read.
 * 
 * \param[in]       numWords is the number of entries in the MA_USED_TABLE
 *                  to be read.
 * 
 * \param[in]       currentTime is the current value of the aging timer.
 * 
 * \param[in]       agingTime is the length of time required for an entry
 *                  to age from YOUNG to OLD.
 * 
 * \param[in]       expiryTime is the length of time required for an entry
 *                  to age out.
 * 
 * \param[in,out]   stats points to a structure containing a number of
 *                  counters that will be incremented to show the number
 *                  of MA Table entries that are updated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ProcessSample(fm_int          sw,
                               fm_int          index,
                               fm_int          numWords,
                               fm_uint64       currentTime,
                               fm_uint64       agingTime,
                               fm_uint64       expiryTime,
                               fm_sweepStats * stats)
{
    fm_internalMacAddrEntry * cachePtr;

    fm_switch *     switchPtr;
    fm_uint32       used[numWords];
    fm_status       status;
    fm_int          entryIndex;
    fm_int          i;
    fm_int          j;
    fm_uint64       elapsedTime;
    fm_sweepStats   sampleStats;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_CLEAR(sampleStats);

    FM_TAKE_L2_LOCK(sw);

    /* Read next sample from MA_USED_TABLE. */
    status = switchPtr->ReadUINT32Mult(sw,
                                       FM10000_MA_USED_TABLE(1, index),
                                       numWords,
                                       used);

    if (status != FM_OK)
    {
        FM_DROP_L2_LOCK(sw);
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_FAST_MAINT,
                     "Error reading MA_USED_TABLE: %s\n",
                     fmErrorMsg(status));
        goto ABORT;
    }

    /* Clear any hits we detected. */
    status = switchPtr->WriteUINT32Mult(sw,
                                        FM10000_MA_USED_TABLE(1, index),
                                        numWords,
                                        used);

    if (status != FM_OK)
    {
        FM_DROP_L2_LOCK(sw);
        FM_LOG_ERROR(FM_LOG_CAT_EVENT_FAST_MAINT,
                     "Error writing MA_USED_TABLE: %s\n",
                     fmErrorMsg(status));
        goto ABORT;
    }

    /* Process each word in the sample. */
    for (i = 0 ; i < numWords ; ++i)
    {
        /* Process each bit in the word. */
        for (j = 0 ; j < ENTRIES_PER_WORD ; ++j)
        {
            /* Get the MA Table entry corresponding to this bit. */
            entryIndex = (index + i) * ENTRIES_PER_WORD + j;
            cachePtr   = &switchPtr->maTable[entryIndex];

            /* We only care about dynamic entries that are eligible
             * for aging. If it's not one of these, keep going. */
            if (cachePtr->state != FM_MAC_ENTRY_STATE_OLD &&
                cachePtr->state != FM_MAC_ENTRY_STATE_YOUNG)
            {
                continue;
            }

            /* If the entry is USED, set its state to YOUNG
             * and restart its aging timer. */
            if (used[i] & (1 << j))
            {
                cachePtr->state = FM_MAC_ENTRY_STATE_YOUNG;
                cachePtr->agingCounter = currentTime;
                ++sampleStats.young;
                continue;
            }

            /* Get the age of this entry. */
            elapsedTime = currentTime - cachePtr->agingCounter;

            if (elapsedTime >= expiryTime)
            {
                /* The entry has aged out. */
                cachePtr->state = FM_MAC_ENTRY_STATE_EXPIRED;
                ++sampleStats.expired;
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT,
                             "expired: index=%d mac=%012llx vid=%u "
                             "elapsed=%llu\n",
                             entryIndex,
                             cachePtr->macAddress,
                             cachePtr->vlanID,
                             elapsedTime);
            }
            else if (cachePtr->state == FM_MAC_ENTRY_STATE_YOUNG &&
                     elapsedTime >= agingTime)
            {
                /* The entry has gone from YOUNG to OLD. */
                cachePtr->state = FM_MAC_ENTRY_STATE_OLD;
                ++sampleStats.old;
                FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT,
                             "aged: index=%d mac=%012llx vid=%u "
                             "elapsed=%llu\n",
                             entryIndex,
                             cachePtr->macAddress,
                             cachePtr->vlanID,
                             elapsedTime);
            }

        }   /* end for (j = 0 ; j < ENTRIES_PER_WORD ; ++j) */

    }   /* for (i = 0 ; i < numWords ; ++i) */

    FM_DROP_L2_LOCK(sw);

ABORT:
    stats->young   += sampleStats.young;
    stats->old     += sampleStats.old;
    stats->expired += sampleStats.expired;

    return status;

}   /* end ProcessSample */




/*****************************************************************************/
/** UsedSweeperActiveState
 * \ingroup intFastMaint
 *
 * \desc            The MAC Address USED Table sweep is in progress.
 *
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       currentTime is the current value of the aging timer.
 *
 * \return          None.
 *
 *****************************************************************************/
static void UsedSweeperActiveState(fm_int sw, fm_uint64 currentTime)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_sweepStats   stats;
    fm_int          upperBound;
    fm_int          numWords;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    FM_CLEAR(stats);

    upperBound = switchExt->usedTableSweeperIndex + USED_TABLE_SAMPLE_SIZE;
    if (upperBound > USED_TABLE_SIZE)
    {
        upperBound = USED_TABLE_SIZE;
    }

    numWords = USED_TABLE_SAMPLE_UNIT;

    while (switchExt->usedTableSweeperIndex < upperBound)
    {
        if ((switchExt->usedTableSweeperIndex + numWords) > upperBound)
        {
            numWords = upperBound - switchExt->usedTableSweeperIndex;
        }

        ProcessSample(sw,
                      switchExt->usedTableSweeperIndex,
                      numWords,
                      currentTime,
                      switchExt->usedTableAgingTime,
                      switchExt->usedTableExpiryTime,
                      &stats);

        switchExt->usedTableSweeperIndex += numWords;

    }   /* end while (switchExt->usedTableSweeperIndex < upperBound) */

    switchExt->usedTableNumExpired += stats.expired;

    if (switchExt->usedTableSweeperIndex >= USED_TABLE_SIZE)
    {
        if (stats.young || stats.old || stats.expired)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT,
                         "sw=%d young=%d old=%d expired=%d elapsed=%llu\n",
                         sw,
                         stats.young,
                         stats.old,
                         stats.expired,
                         currentTime - switchExt->usedTableLastSweepTime);
        }

        if (switchExt->usedTableNumExpired)
        {
            fm_maWorkTypeData   data;
            fm_status           err;

            FM_CLEAR(data);

            err = fmEnqueueMAPurge(sw, FM_UPD_FLUSH_EXPIRED, data, NULL, NULL);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_EVENT_FAST_MAINT,
                             "fmEnqueueMAPurge failed: %s\n",
                             fmErrorMsg(err));
            }
        }

        /* Enter READY state to wait for next pass. */
        switchExt->usedTableSweeperState = FM_USED_SWEEPER_READY;
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT, "sw=%d state=READY\n", sw);
    }

}   /* end UsedSweeperActiveState */




/*****************************************************************************/
/** UsedSweeperReadyState
 * \ingroup intFastMaint
 *
 * \desc            The MAC Address USED sweeper is waiting for the start
 *                  of the next pass.
 *
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       currentTime is the current value of the aging timer.
 *
 * \return          None.
 *
 *****************************************************************************/
static void UsedSweeperReadyState(fm_int sw, fm_uint64 currentTime)
{
    fm_switch *     switchPtr;
    fm10000_switch *switchExt;
    fm_uint64       elapsedTime;
    fm_uint64       startInterval;
    fm_float        readyFactor;

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = switchPtr->extension;

    if (switchPtr->macAgingTicks == 0)
    {
        /* Revert to IDLE state.*/
        switchExt->usedTableSweeperState = FM_USED_SWEEPER_IDLE;
        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT, "sw=%d state=IDLE\n", sw);
        return;
    }

    /* Elapsed time in milliseconds since start of previous pass. */
    elapsedTime = currentTime - switchExt->usedTableLastSweepTime;

    readyFactor =
        fmGetFloatApiProperty(FM_AAK_API_FM10000_MA_USED_TABLE_READY_FACTOR,
                              FM_AAD_API_FM10000_MA_USED_TABLE_READY_FACTOR);

    /* Minimum interval until start of next pass. */
    startInterval = (fm_uint64) (readyFactor * switchPtr->macAgingTicks);

    if (elapsedTime >= startInterval)
    {
        EnterActiveState(sw, currentTime);
    }

}   /* end UsedSweeperReadyState */




/*****************************************************************************/
/** UsedSweeperIdleState
 * \ingroup intFastMaint
 *
 * \desc            The MAC Address USED sweeper is waiting to start.
 * 
 * \param[in]       currentTime is the current value of the aging timer.
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \return          None.
 *
 *****************************************************************************/
static void UsedSweeperIdleState(fm_int sw, fm_uint64 currentTime)
{
    fm_switch *     switchPtr;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->macAgingTicks)
    {
        /* Advance sweeper to ACTIVE state.*/
        EnterActiveState(sw, currentTime);
    }

}   /* end UsedSweeperIdleState */




/*****************************************************************************/
/** fm10000InitUsedTableSweeper
 * \ingroup intFastMaint
 *
 * \desc            Initializes the MAC Table USED sweeper.
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \return          None.
 *
 *****************************************************************************/
static void fm10000InitUsedTableSweeper(fm_int sw)
{
    fm10000_switch *switchExt;

    switchExt = GET_SWITCH_EXT(sw);

    /* Advance sweeper to IDLE state. */
    switchExt->usedTableSweeperState = FM_USED_SWEEPER_IDLE;
    FM_LOG_DEBUG(FM_LOG_CAT_EVENT_FAST_MAINT, "sw=%d state=IDLE\n", sw);

}   /* end fm10000InitUsedTableSweeper */




/*****************************************************************************/
/** fm10000MacUsedTableSweeper
 * \ingroup intFastMaint
 *
 * \desc            Sweeps the Mac Address USED table to determine which
 *                  dynamic entries have been accessed, and ages out any
 *                  entries that have expired.
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \return          None.
 *
 *****************************************************************************/
static void fm10000MacUsedTableSweeper(fm_int sw)
{
    fm10000_switch *switchExt;
    fm_uint64       currentTime;

    switchExt = GET_SWITCH_EXT(sw);

    currentTime = fm10000GetAgingTimer();

    switch (switchExt->usedTableSweeperState)
    {
        case FM_USED_SWEEPER_INITIAL:
            fm10000InitUsedTableSweeper(sw);
            break;

        case FM_USED_SWEEPER_IDLE:
            UsedSweeperIdleState(sw, currentTime);
            break;

        case FM_USED_SWEEPER_ACTIVE:
            UsedSweeperActiveState(sw, currentTime);
            break;

        case FM_USED_SWEEPER_READY:
            UsedSweeperReadyState(sw, currentTime);
            break;
    }

}   /* end fm10000MacUsedTableSweeper */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000GetAgingTimer
 * \ingroup intFastMaint
 *
 * \desc            Returns the current value of the aging timer.
 *
 * \param[in]       None.
 *
 * \return          The current value of the aging timer.
 *
 *****************************************************************************/
fm_uint64 fm10000GetAgingTimer(void)
{
    fm_timestamp curTime;
    fm_uint64    agingTimer;

    if ( fmGetTime(&curTime) != FM_OK )
    {
        return 0;
    }

    /* Convert seconds to milliseconds. */
    agingTimer = curTime.sec * 1000;

    /* Add in milliseconds within the current second. */
    agingTimer += curTime.usec / 1000;

    return agingTimer;

}   /* end fm10000GetAgingTimer */




/*****************************************************************************/
/** fm10000FastMaintenanceTask
 * \ingroup intFastMaint
 *
 * \desc            Chip-specific fast maintenance thread.
 * 
 * \note            The caller has ensured that the switch is UP and has
 *                  taken the switch protection lock.
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       args is the thread argument pointer.
 *
 * \return          None.
 *
 *****************************************************************************/
void * fm10000FastMaintenanceTask(fm_int sw, void *args)
{
    fm_status err;

    FM_NOT_USED(args);

    fm10000MacUsedTableSweeper(sw);

    err = fm10000MTablePeriodicMaintenance(sw);
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_EVENT_FAST_MAINT, 
                     "MTableMaintenance returned error: %s\n" ,
                     fmErrorMsg(err));
    }

    return NULL;

}   /* end fm10000FastMaintenanceTask */
