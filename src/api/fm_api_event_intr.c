/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_intr.c
 * Creation Date:   May 4, 2007
 * Description:     FocalPoint interrupt handler wrapper task
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


/*****************************************************************************
 * fmInterruptHandler
 *
 * Description: Generic Focalpoint interrupt handler task wrapper.
 *
 * Arguments:   data is a pointer to the device control block.
 *
 * Returns:     None.
 *
 * This task wakes up on a semaphore that may be signaled by the
 * interrupt level ISR or some other task.
 *
 * When the task wakes up, it will call the chip specific interrupt
 * handler.
 *
 * Then, if the interrupt was triggered by the ISR, the task will
 * re-enable the interrupt.
 *
 *****************************************************************************/
void *fmInterruptHandler(void *args)
{
    fm_int     sw;
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint    intrSource;
    fm_int     handleFibmSlave;

    /* There is a duplicate interrupt handler thread if FIBM is enabled
     * Since the interrupt thread processing for remote switch will be
     * suspended while doing registers read/ write
     */
    /* Args set to NULL for remote thread */
    handleFibmSlave = FM_GET_THREAD_PARAM(void, args) ? 0 : 1;

    FM_NOT_USED(args);
    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_INTR, "%p\n", args);

    /**************************************************
     * Loop forever, waiting for signals from ISR.
     **************************************************/

    while (TRUE)
    {
        /**************************************************
         * Wait for a signal from the ISR.
         **************************************************/

        FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Waiting for interrupt..\n");

        if (handleFibmSlave)
        {
            err = fmWaitSemaphore(&fmRootApi->fibmSlaveIntrAvail, FM_WAIT_FOREVER);
        }
        else
        {
            err = fmWaitSemaphore(&fmRootApi->intrAvail, FM_WAIT_FOREVER);
        }

        if (err != FM_OK)
        {
            FM_LOG_FATAL( FM_LOG_CAT_EVENT_INTR, "%s\n", fmErrorMsg(err) );

            continue;
        }

        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            /* Only process interrupt for same type of switch */
            if (!handleFibmSlave != !fmRootApi->isSwitchFibmSlave[sw])
            {
                continue;
            }

            if (!SWITCH_LOCK_EXISTS(sw))
            {
                continue;
            }

            err = fmPlatformGetInterrupt(sw,
                                         FM_INTERRUPT_SOURCE_ISR,
                                         &intrSource);


            if (err != FM_OK)
            {
                FM_LOG_FATAL( FM_LOG_CAT_EVENT_INTR, "%s\n", fmErrorMsg(err) );

                continue;
            }

            if (intrSource == FM_INTERRUPT_SOURCE_NONE)
            {
                continue;
            }

            FM_LOG_DEBUG(FM_LOG_CAT_EVENT_INTR, "Interrupt seen (source 0x%x)\n",
                         intrSource);

            PROTECT_SWITCH(sw);

            /* If the switch is not up yet, keep going */
            if ( (fmRootApi->fmSwitchStateTable[sw] == NULL)
                || !FM_IS_STATE_ALIVE(fmRootApi->fmSwitchStateTable[sw]->state) )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                             "Switch %d is not up, ignoring interrupts\n",
                             sw);

                UNPROTECT_SWITCH(sw);

                continue;
            }

            switchPtr = fmRootApi->fmSwitchStateTable[sw];

            /* Call the chip specific handler */
            switchPtr->InterruptHandler(switchPtr);

            UNPROTECT_SWITCH(sw);

            if (intrSource & FM_INTERRUPT_SOURCE_ISR)
            {
                /* Re-enable the interrupt */
                err = fmPlatformEnableInterrupt(sw, intrSource);

                if (err != FM_OK)
                {
                    FM_LOG_FATAL( FM_LOG_CAT_EVENT_INTR, "%s\n", fmErrorMsg(err) );

                    continue;
                }

            }   /* end if (intrSource & FM_INTERRUPT_SOURCE_ISR) */

        }   /* end for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++) */

    }   /* end while (TRUE) */

    /**************************************************
     * Should never exit.
     **************************************************/

    FM_LOG_FATAL(FM_LOG_CAT_EVENT_INTR, "Task exiting inadvertently!\n");

    return NULL;

}   /* end fmInterruptHandler */




/*****************************************************************************/
/** fmSendSoftwareEvent
 * \ingroup intSwitch
 *
 * \desc            Generate a software interrupt event. Generally called
 *                  in response to a software interrupt on FM4000 and FM6000
 *                  devices.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       events is a bit mask of software events as reported in
 *                  the chip's SW_IP register.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSendSoftwareEvent(fm_int sw, fm_uint32 events)
{
    fm_status         err = FM_OK;
    fm_event *        event;
    fm_eventSoftware *swEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_EVENT_INTR, 
                 "sw=%d events=0x%08x\n",
                 sw, 
                 events);

    event = fmAllocateEvent(sw,
                            FM_EVID_LOW_SOFTWARE,
                            FM_EVENT_SOFTWARE,
                            FM_EVENT_PRIORITY_HIGH);

    if (event == NULL)
    {
        FM_LOG_FATAL(FM_LOG_CAT_EVENT_INTR, "Out of event buffers\n");
        err = FM_ERR_NO_EVENTS_AVAILABLE;
        goto ABORT;
    }

    swEvent = &event->info.fpSoftwareEvent;
    memset( swEvent, 0, sizeof(fm_eventSoftware) );

    swEvent->activeEvents = events;

    err = fmSendThreadEvent(&fmRootApi->eventThread, event);

    if (err != FM_OK)
    {
        /* Free the event since we could not send it to thread */
        fmReleaseEvent(event);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_EVENT_INTR, err);

}   /* end fmSendSoftwareEvent */




