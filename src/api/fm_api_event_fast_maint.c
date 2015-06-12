/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_fast_maint.c
 * Creation Date:   April 2, 2008
 * Description:     Generic thread wrapper for chip specific fast maintenance
 *                  thread.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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
/** fmFastMaintenanceTask
 * \ingroup intEvent
 *
 * \desc            Generic thread wrapper for chip specific fast maintenance
 *                  thread.
 *
 * \param[in]       args is the thread argument pointer.
 *
 * \return          None.
 *
 *****************************************************************************/
void *fmFastMaintenanceTask(void *args)
{
    fm_switch *  switchPtr;
    fm_thread *  eventHandler;
    fm_thread *  thread;
    fm_int       sw;
    fm_bool      doFastTask = FALSE;
    fm_int       delayTime;
    fm_timestamp curTime;
#if 0
    fm_timestamp nextRefreshTime;
    fm_int       refreshInterval;
#endif
    fm_int       msecCount = 1000;  /* Trigger remote refresh on first run */
    fm_bool      checkRemoteRefresh;

    /* grab arguments */
    thread       = FM_GET_THREAD_HANDLE(args);
    eventHandler = FM_GET_THREAD_PARAM(fm_thread, args);

    /* If logging is disabled, thread and eventHandler won't be used */
    FM_NOT_USED(thread);
    FM_NOT_USED(eventHandler);

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_FAST_MAINT,
                 "thread=%s eventHandler=%s\n",
                 thread->name,
                 eventHandler->name);

    /* First check to see if we need to call the fast maintenance task */
    do 
    {
        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if (!SWITCH_LOCK_EXISTS(sw))
            {
                continue;
            }

            PROTECT_SWITCH(sw);

            switchPtr = GET_SWITCH_PTR(sw);
            
            /***************************************************
             * Iff the switch has been fully booted AND a switch
             * specific fast maintenance task has been defined,
             * perform a switch specific fast maintenance
             * function call.
             **************************************************/
            if (switchPtr &&
                switchPtr->state == FM_SWITCH_STATE_UP &&
                switchPtr->FastMaintenanceTask)
            {
                doFastTask = TRUE;
            }

            UNPROTECT_SWITCH(sw);
        }
        
        fmDelay(5, 0);

    } 
    while (!doFastTask);


    delayTime = fmGetIntApiProperty(FM_AAK_API_FAST_MAINTENANCE_PERIOD,
                                    FM_AAD_API_FAST_MAINTENANCE_PERIOD);

    for ( ; ; )
    {
        /* Check for remote refresh timeouts once each quarter-second */
        checkRemoteRefresh = FALSE;
        msecCount += delayTime;

        if (msecCount >= 250)   /* 250 milliseconds per quarter-second */
        {
            msecCount = 0;

            if ( fmGetTime(&curTime) == FM_OK)
            {
                checkRemoteRefresh = TRUE;
            }
        }

        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if (!SWITCH_LOCK_EXISTS(sw))
            {
                continue;
            }

            PROTECT_SWITCH(sw);

            switchPtr = GET_SWITCH_PTR(sw);

#if 0
            /* Only check for remote refresh intervals if hardware aging
             * is active */
            if (switchPtr &&
                switchPtr->state == FM_SWITCH_STATE_UP &&
                checkRemoteRefresh &&
                switchPtr->macAgingTicks)
            {
                /* refresh interval is 1/3 the hardware aging time but
                 * don't refresh more often than twice per second */
                refreshInterval = switchPtr->macAgingTicks / 3;
                nextRefreshTime.sec  = refreshInterval / FM_TICKS_PER_SECOND;
                nextRefreshTime.usec = (refreshInterval % FM_TICKS_PER_SECOND)
                                        * 1000; /* 1000 usecs per msec */
                if ( (nextRefreshTime.sec == 0) &&
                     (nextRefreshTime.usec < 500000) )
                {
                    nextRefreshTime.usec = 500000; /* 500k usecs = 1/2 second */
                }

                fmAddTimestamps(&nextRefreshTime,
                                &switchPtr->macTableLastRemoteRefresh);

                if ( fmCompareTimestamps(&curTime, &nextRefreshTime) >= 0 )
                {
                    fmIssueMacMaintRequest(sw, FM_UPD_REFRESH_REMOTE);
                    switchPtr->macTableLastRemoteRefresh = curTime;
                }
            }
#endif

            /***************************************************
             * Iff the switch has been fully booted AND a switch
             * specific fast maintenance task has been defined,
             * perform a switch specific fast maintenance
             * function call.
             **************************************************/
            if (switchPtr &&
                switchPtr->state == FM_SWITCH_STATE_UP &&
                switchPtr->FastMaintenanceTask)
            {
                switchPtr->FastMaintenanceTask(sw, args);
            }

            UNPROTECT_SWITCH(sw);

        }   /* end for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++) */
        
        fmDelay(0, delayTime);

    }   /* end for ( ; ; ) */
  
    fmExitThread(thread);

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_EVENT_FAST_MAINT, NULL, "\n");

}   /* end fmFastMaintenanceTask */
