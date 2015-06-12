/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_port.c
 * Creation Date:   May 18, 2007
 * Description:     Generic thread wrapper for chip specific debounce handler
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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
/** fmDebounceLinkStateTask
 * \ingroup intEvent
 *
 * \desc            Function to debounce link states, and pass link state events
 *                  up to the API event queue.
 *
 * \param[in]       args is the thread argument pointer.
 *
 * \return          None.
 *
 *****************************************************************************/
void *fmDebounceLinkStateTask(void *args)
{
    fm_thread *thread;
    fm_thread *eventHandler;
    fm_switch *switchPtr;
    fm_int     sw;
#if FM_SUPPORT_SWAG
    fm_int     swagId;
#endif

    /* grab arguments */
    thread       = FM_GET_THREAD_HANDLE(args);
    eventHandler = FM_GET_THREAD_PARAM(fm_thread, args);

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT,
                 "thread = %s, eventHandler = %s\n",
                 thread->name,
                 eventHandler->name);

    while (1)
    {
        /* process each switch and port.. */
        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if ( !SWITCH_LOCK_EXISTS(sw) )
            {
                continue;
            }

#if FM_SUPPORT_SWAG
            if (fmIsSwitchInASWAG(sw, &swagId) == FM_OK)
            {
                PROTECT_SWITCH(swagId);
                if ( (fmRootApi->fmSwitchStateTable[swagId] == NULL)
                    || (fmRootApi->fmSwitchStateTable[swagId]->state != FM_SWITCH_STATE_UP) )
                {
                    UNPROTECT_SWITCH(swagId);
                    continue;
                }
                UNPROTECT_SWITCH(swagId);
            }
#endif

            PROTECT_SWITCH(sw);

            /* If the switch is not up yet, keep going */
            if ( (fmRootApi->fmSwitchStateTable[sw] == NULL)
                || !FM_IS_STATE_ALIVE(fmRootApi->fmSwitchStateTable[sw]->state) )
            {
                UNPROTECT_SWITCH(sw);
                continue;
            }

            switchPtr = fmRootApi->fmSwitchStateTable[sw];

            /* Call the switch specific handler */
            FM_API_CALL_FAMILY_VOID(switchPtr->DebounceLinkStates,
                                    sw, 
                                    thread, 
                                    eventHandler);

            UNPROTECT_SWITCH(sw);
        }

        /* sleep for specified delay period (default is 1/4 second) */
        fmDelay(0, FM_API_LINK_STATE_DEBOUNCE_DELAY);
    }

    fmExitThread(thread);

    return NULL;

}   /* end fmDebounceLinkStateTask */
