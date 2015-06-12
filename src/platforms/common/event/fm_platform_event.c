/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_event.c
 * Creation Date:   November 1, 2012
 * Description:     Platform Event Notification functions.
 *
 * Copyright (c) 2006 - 2012, Intel Corporation
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
#include <platforms/common/event/fm_platform_event.h>

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
/** fmPlatformEventSendPortXcvrState
 * \ingroup intPlatform
 *
 * \desc            Function to update SFP+ or QSFP module state to the application.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       mac is the MAC number for the port.
 *
 * \param[in]       lane is the lane number for the port if applicable.
 *
 * \param[in]       xcvrSignals a bitmask where each bit represents the status
 *                  of a given signal (see the ''Transceiver Signals'' 
 *                  definitions).
 *
 * \param[in]       priority is the priority to send the event at
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformEventSendPortXcvrState(fm_int           sw,
                                           fm_int           port,
                                           fm_int           mac,
                                           fm_int           lane,
                                           fm_uint32        xcvrSignals,
                                           fm_eventPriority priority)
{
    fm_status                   status;
    fm_event *                  event;
    fm_eventPlatform           *platformEvent;
    fm_eventPlatformPortXcvr   *xcvrEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d port=%d mac=%d lane=%d xcvrSignals=0x%x priority=%d\n",
                 sw,
                 port,
                 mac,
                 lane,
                 xcvrSignals,
                 priority);

    event = fmAllocateEvent(sw, FM_EVID_PLATFORM, FM_EVENT_PLATFORM, priority);

    if (event == NULL)
    {
#if 0 
        fmAddEventFreeNotify(sw,
                             EVENT_FREE_NOTIFY_LINK_TRANSITION,
                             EventFreeHandler);
#endif

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    platformEvent = &event->info.fpPlatformEvent;

    FM_PLAT_INIT_EVENT_PLATFORM(platformEvent,
                                FM_EVENT_PLATFORM_TYPE_PORT_XCVR,
                                fm_eventPlatformPortXcvr*);

    xcvrEvent              = (fm_eventPlatformPortXcvr*)platformEvent->eventData;
    xcvrEvent->port        = port;
    xcvrEvent->mac         = mac;
    xcvrEvent->lane        = lane;
    xcvrEvent->xcvrSignals = xcvrSignals;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (status != FM_OK)
    {
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformEventSendPortXcvrState */




/*****************************************************************************/
/** fmPlatformEventSendPscStatus
 * \ingroup intPlatform
 *
 * \desc            Function to update PSC status to the application.
 *
 * \param[in]       psc is the PSC number.
 *
 * \param[in]       pscStatus a bitmask where each bit represents the PSC status
 *                  (see the ''FM_PSC_STATUS'' definitions).
 *
 * \param[in]       priority is the priority to send the event at
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformEventSendPscStatus(fm_int           psc,
                                       fm_uint32        pscStatus,
                                       fm_eventPriority priority)
{
    fm_status                   status;
    fm_event *                  event;
    fm_eventPlatform           *platformEvent;
    fm_eventPlatformPscStatus  *pscEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "psc=%d status=0x%x priority=%d\n",
                 psc,
                 pscStatus,
                 priority);

    event = fmAllocateEvent(FM_FIRST_FOCALPOINT,
                            FM_EVID_PLATFORM,
                            FM_EVENT_PLATFORM,
                            priority);

    if (event == NULL)
    {
        /* Need to add callback to resend message */

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    platformEvent = &event->info.fpPlatformEvent;

    FM_PLAT_INIT_EVENT_PLATFORM(platformEvent,
                                FM_EVENT_PLATFORM_TYPE_PSC_STATUS,
                                fm_eventPlatformPscStatus*);

    pscEvent              = (fm_eventPlatformPscStatus*)platformEvent->eventData;
    pscEvent->psc         = psc;
    pscEvent->status      = pscStatus;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (status != FM_OK)
    {
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformEventSendPscStatus */




/*****************************************************************************/
/** fmPlatformEventSendPsuStatus
 * \ingroup intPlatform
 *
 * \desc            Function to update PSU status to the application.
 *
 * \param[in]       psu is the PSU number.
 *
 * \param[in]       psuStatus a bitmask where each bit represents the PSU status
 *                  (see the ''FM_PSU_STATUS'' definitions).
 *
 * \param[in]       priority is the priority to send the event at
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformEventSendPsuStatus(fm_int           psu,
                                       fm_uint32        psuStatus,
                                       fm_eventPriority priority)
{
    fm_status                   status;
    fm_event *                  event;
    fm_eventPlatform           *platformEvent;
    fm_eventPlatformPsuStatus  *psuEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "psu=%d status=0x%x priority=%d\n",
                 psu,
                 psuStatus,
                 priority);

    event = fmAllocateEvent(FM_FIRST_FOCALPOINT,
                            FM_EVID_PLATFORM,
                            FM_EVENT_PLATFORM,
                            priority);

    if (event == NULL)
    {
        /* Need to add callback to resend message */

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    platformEvent = &event->info.fpPlatformEvent;

    FM_PLAT_INIT_EVENT_PLATFORM(platformEvent,
                                FM_EVENT_PLATFORM_TYPE_PSU_STATUS,
                                fm_eventPlatformPsuStatus*);

    psuEvent              = (fm_eventPlatformPsuStatus*)platformEvent->eventData;
    psuEvent->psu         = psu;
    psuEvent->status      = psuStatus;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (status != FM_OK)
    {
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformEventSendPsuStatus */




/*****************************************************************************/
/** fmPlatformEventSendFanStatus
 * \ingroup intPlatform
 *
 * \desc            Function to update fan status to the application.
 *
 * \param[in]       fan is the fan number.
 *
 * \param[in]       fanStatus a bitmask where each bit represents the fan status
 *                  (see the ''FM_FAN_STATUS'' definitions).
 *
 * \param[in]       priority is the priority to send the event at
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformEventSendFanStatus(fm_int           fan,
                                       fm_uint32        fanStatus,
                                       fm_eventPriority priority)
{
    fm_status                   status;
    fm_event *                  event;
    fm_eventPlatform           *platformEvent;
    fm_eventPlatformFanStatus  *fanEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "fan=%d status=0x%x priority=%d\n",
                 fan,
                 fanStatus,
                 priority);

    event = fmAllocateEvent(FM_FIRST_FOCALPOINT,
                            FM_EVID_PLATFORM,
                            FM_EVENT_PLATFORM,
                            priority);

    if (event == NULL)
    {
        /* Need to add callback to resend message */

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    platformEvent = &event->info.fpPlatformEvent;

    FM_PLAT_INIT_EVENT_PLATFORM(platformEvent,
                                FM_EVENT_PLATFORM_TYPE_FAN_STATUS,
                                fm_eventPlatformFanStatus*);

    fanEvent              = (fm_eventPlatformFanStatus*)platformEvent->eventData;
    fanEvent->fan         = fan;
    fanEvent->status      = fanStatus;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (status != FM_OK)
    {
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformEventSendFanStatus */




/*****************************************************************************/
/** fmPlatformSendSwTemperature
 * \ingroup intPlatform
 *
 * \desc            Function to update switch temperature to the application.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       type is the event type of FM_EVENT_PLATFORM_TYPE_SW_WARN_TEMP
 *                  or FM_EVENT_PLATFORM_TYPE_SW_OVER_TEMP.
 *
 * \param[in]       temperature is the switch temperature in centi-Celsius.
 *
 * \param[in]       priority is the priority to send the event at
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformEventSendSwTemperature(fm_int           sw,
                                           fm_uint32        type,
                                           fm_int           temperature,
                                           fm_eventPriority priority)
{
    fm_status                   status;
    fm_event *                  event;
    fm_eventPlatform           *platformEvent;
    fm_eventPlatformSwTemperature *tempEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d type=%d temp=%d priority=%d\n",
                 sw,
                 type,
                 temperature,
                 priority);

    switch (type)
    {
        case FM_EVENT_PLATFORM_TYPE_SW_WARN_TEMP:
        case FM_EVENT_PLATFORM_TYPE_SW_NORMAL_TEMP:
        case FM_EVENT_PLATFORM_TYPE_SW_OVER_TEMP:
            break;
        default:
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    event = fmAllocateEvent(FM_FIRST_FOCALPOINT,
                            FM_EVID_PLATFORM,
                            FM_EVENT_PLATFORM,
                            priority);

    if (event == NULL)
    {
        /* Need to add callback to resend message */

        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    platformEvent = &event->info.fpPlatformEvent;

    FM_PLAT_INIT_EVENT_PLATFORM(platformEvent, type, fm_eventPlatformSwTemperature*);

    tempEvent              = (fm_eventPlatformSwTemperature*)platformEvent->eventData;
    tempEvent->sw          = sw;
    tempEvent->temperature = temperature;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (status != FM_OK)
    {
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformEventSendSwTemperature */




/*****************************************************************************/
/** fmPlatformEventSendSwGpioInterrupt
 * \ingroup intPlatform
 *
 * \desc            Function to notify the application about GPIO interrupt.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       gpio is the switch GPIO number.
 * 
 * \param[in]       priority is the priority to send the event at
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformEventSendSwGpioInterrupt(fm_int           sw,
                                             fm_int           gpio,
                                             fm_eventPriority priority)
{
    fm_status                   status;
    fm_event *                  event;
    fm_eventPlatform           *platformEvent;
    fm_eventPlatformSwGpioIntr *gpioEvent;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d gpio=%d\n", sw, gpio);

    event = fmAllocateEvent(FM_FIRST_FOCALPOINT,
                            FM_EVID_PLATFORM,
                            FM_EVENT_PLATFORM,
                            priority);

    if (event == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_EVENTS_AVAILABLE);
    }

    platformEvent = &event->info.fpPlatformEvent;

    FM_PLAT_INIT_EVENT_PLATFORM(platformEvent,
                                FM_EVENT_PLATFORM_TYPE_SW_GPIO_INTR,
                                fm_eventPlatformSwGpioIntr*);

    gpioEvent       = (fm_eventPlatformSwGpioIntr*)platformEvent->eventData;
    gpioEvent->sw   = sw;
    gpioEvent->gpio = gpio;

    status = fmSendThreadEvent(&fmRootApi->eventThread, event);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

ABORT:
    if (status != FM_OK)
    {
        fmReleaseEvent(event);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformEventSendSwGpioInterrupt */

