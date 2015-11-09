/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_xcvr_mgmt.c
 * Creation Date:   June 2, 2014
 * Description:     Platform LED functions.
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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


/*****************************************************************************/
/* GetLedSpeed
 * \ingroup intPlatformMgmtLed
 *
 * \desc            This function returns the fm_platPortLedSpeed associated
 *                  to the given port speed.
 *
 * \param[in]       speed is the port speed
 *
 * \return          ledSpeed one of the fm_platPortLedSpeed
 * 
 *****************************************************************************/
static fm_platPortLedSpeed GetLedSpeed(fm_uint32 speed)
{
    fm_uint32 ledSpeed;

    if ( speed > 40000 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_100G;
    }
    else if ( speed > 25000 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_40G;
    }
    else if ( speed > 10000 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_25G;
    }
    else if ( speed > 2500 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_10G;
    }
    else if ( speed > 1000 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_2P5G;
    }
    else if ( speed > 100 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_1G;
    }
    else if ( speed > 10 )
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_100M;
    }
    else
    {
        ledSpeed = FM_PLAT_PORT_LED_SPEED_10M;
    }

    return ledSpeed;

}   /* end GetLedSpeed */




/*****************************************************************************/
/* GetPortTrafficLedState
 * \ingroup intPlatformMgmtLed
 *
 * \desc            Returns the traffic state for the given port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       portIdx is the port index
 *
 * \param[out]      trafficState points to caller allocated storage where the
 *                  traffic indication is returned.
 *
 * \return          FM_OK if sucessful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetPortTrafficLedState(fm_int  sw, 
                                        fm_int  portIdx, 
                                        fm_int *trafficState)
{
    fm_platformCfgPort *portCfg;
    fm_switch *         switchPtr;
    fm_uint32           val32;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

        /* Read the RX/TX traffic indication from the switch */
        switchPtr->ReadUINT32(sw,
                              FM10000_PORT_STATUS(portCfg->epl,
                                                  portCfg->lane[0]),
                              &val32);

        if ( ( FM_GET_BIT(val32, FM10000_PORT_STATUS, Transmitting) ) ||
             ( FM_GET_BIT(val32, FM10000_PORT_STATUS, Receiving) ) )
        {
            *trafficState = 1;
        }
        else
        {
            *trafficState = 0;
        }
    }
    else
    {
        *trafficState = 0;
    }

    return FM_OK;

}   /* end GetPortTrafficLedState */




/*****************************************************************************/
/* fmPlatformLedThread
 * \ingroup intPlatformLed
 *
 * \desc            Thread to handle LED status.
 *
 * \param[in]       args contains thread-initialization parameters
 *
 * \return          None.
 *
 *****************************************************************************/
void *fmPlatformLedThread(void *args)
{
    fm_thread *           thread;
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPort *  portCfg;
    fm_platformLib     *  libFunc;
    fm_platLedInfo *      ledInfo;
    fm_int *              ledPortList;
    fm_uint32 *           hwResIdList;
    fm_platPortLedState * ledStateList;
    fm_platPortLedState   ledState;
    fm_platPortLedSpeed   ledSpeed;
    fm_int                sw;
    fm_int                i;
    fm_int                count;
    fm_int                cnt;
    fm_int                portIdx;
    fm_int                portLedPeriodMsec;
    fm_int                portLedPeriodSec;
    fm_int                portLedPeriodNsec;
    fm_int                trafficState;
    fm_int                ledPortCnt;
    fm_int                activePortCnt;
    fm_bool               addPort;
    fm_bool               up;

    /* grab arguments */
    thread = FM_GET_THREAD_HANDLE(args);
    sw     = *(FM_GET_THREAD_PARAM(fm_int, args));

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "thread = %s\n", thread->name);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    activePortCnt = 0;

    if ( !libFunc->SetPortLed )
    {
        /* No support so no need to keep the thread alive */
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to enable LED polling\n"
                     "libFunc->SetPortLed is disabled from LT config file\n");
        fmExitThread(thread);
        return NULL;
    }

    /* Do not enable polling until the switch is UP */
    up = FALSE;
    while (!up)
    {
        fmDelay(1, 0);
        fmGetSwitchState(sw, &up);
    }

    /* Pre-allocate memory to be used within the polling thread */
    ledPortList  = fmAlloc( FM_PLAT_NUM_PORT(sw) * sizeof(fm_int) );
    hwResIdList  = fmAlloc( FM_PLAT_NUM_PORT(sw) * sizeof(fm_uint32) );
    ledStateList = fmAlloc( FM_PLAT_NUM_PORT(sw) * 
                            sizeof(fm_platPortLedState) );

    if ( ledPortList && hwResIdList && ledStateList )
    {
        /* Create the list of ports that need LED software support */
        ledPortCnt = 0;
        for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
        {
            portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

            /* Don't add PCIE ports since traffic notification isn't required */
            if ( !FM_PLAT_PORT_IS_SW_LED(portCfg) ||
                 portCfg->intfType == FM_PLAT_INTF_TYPE_PCIE )
            {
                continue;
            }

            ledPortList[ledPortCnt] = portIdx;
            ledPortCnt++;
            MOD_LED_DEBUG("Add port %d to LED port list\n", portCfg->port);
        }
    }
    else
    {
        /* Unable to allocate all the required lists above */
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to enable LED polling\n");

        /* Free those that were allocated */
        if (ledPortList != NULL)  fmFree(ledPortList);
        if (hwResIdList != NULL)  fmFree(hwResIdList);
        if (ledStateList != NULL) fmFree(ledStateList);

        fmExitThread(thread);
        return NULL;
    }

    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    portLedPeriodMsec = swCfg->ledPollPeriodMsec;
    portLedPeriodSec  = portLedPeriodMsec / 1000;
    portLedPeriodNsec = (portLedPeriodMsec % 1000) * 1000 * 1000;
    count = 0;

    MOD_LED_DEBUG("sw=%d total num port: %d, pollingPeriod: %d msec\n"
                  "swCfg->ledBlinkMode %d\n", 
                  sw, 
                  ledPortCnt,
                  portLedPeriodMsec,
                  swCfg->ledBlinkMode);

    while (1)
    {
        /* To cover the case the switch is brought down */
        fmGetSwitchState(sw, &up);

        if (!up)
        {
            goto ABORT;
        }

        if (++count % 2)
        {
            /* Required within GetPortTrafficLedState */
            PROTECT_SWITCH(sw);

            for (cnt = 0, i = 0 ; cnt < ledPortCnt ; cnt++)
            {
                portIdx = ledPortList[cnt];
                portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
                ledInfo = &GET_PLAT_STATE(sw)->ledInfo[portIdx];
                addPort = FALSE;

                /* Handle only the ports that are UP */
                if (ledInfo->linkState)
                {
                    ledInfo->linkState2 = 1;
                    GetPortTrafficLedState(sw, portIdx, &trafficState);

                    if (trafficState)
                    {
                        /* Indicate presence of traffic */
                        ledState = FM_PLAT_PORT_LED_BLINK_ON;
                    }
                    else
                    {
                        /* No traffic then just indicate link UP */
                        ledState = FM_PLAT_PORT_LED_LINK_UP;
                    }

                    if ( (swCfg->ledBlinkMode == FM_LED_BLINK_MODE_NO_BLINK ||
                          swCfg->ledBlinkMode == FM_LED_BLINK_MODE_HW_ASSISTED) &&
                         (ledInfo->trafficState != trafficState) )
                    {
                        /* Provide new LED state set above */
                        addPort = TRUE;
                        ledInfo->trafficState = trafficState;
                    }
                    else if (swCfg->ledBlinkMode == 
                                                 FM_LED_BLINK_MODE_SW_CONTROL)
                    {
                        if (trafficState)
                        {
                            /* Turn ON traffic LED */
                            addPort = TRUE;
                            ledInfo->trafficState = trafficState;
                        }
                        else if (ledInfo->trafficState != trafficState)
                        {
                            /* No traffic then just indicate link UP */
                            addPort = TRUE;
                            ledInfo->trafficState = trafficState;
                        }
                    }
                }
                else if (ledInfo->linkState2)
                {
                    /* To avoid race condition with fmPlatformLedSetPortState */
                    ledInfo->linkState2 = 0;
                    ledInfo->trafficState = 0;
                    addPort = TRUE;
                    ledState = FM_PLAT_PORT_LED_LINK_DOWN;
                    MOD_LED_DEBUG("Force DOWN port %d hwId=0x%x\n", 
                                  portCfg->port,
                                  portCfg->hwResourceId);
                }

                if (addPort)
                {
                    hwResIdList[i]  = portCfg->hwResourceId;
                    ledStateList[i] = (HW_LED_STATE_SET_STATE(ledState) |
                                       HW_LED_STATE_SET_SPEED(ledInfo->speed));

                    MOD_LED_DEBUG("Add port %d to list at index %d "
                                  "hwId=0x%x ledState 0x%x\n", 
                                  portCfg->port,
                                  i,
                                  hwResIdList[i],
                                  ledStateList[i]);
                    i++;
                }
            }

            /* Save the number of port in ledStateList */
            activePortCnt = i;

            UNPROTECT_SWITCH(sw);
        }
        else if (swCfg->ledBlinkMode == FM_LED_BLINK_MODE_SW_CONTROL)
        {
            for (cnt = 0, i = 0 ; cnt < activePortCnt ; cnt++)
            {
                ledState = HW_LED_STATE_TO_STATE(ledStateList[cnt]);
                ledSpeed = HW_LED_STATE_TO_SPEED(ledStateList[cnt]);

                /* Force OFF if there is traffic indication from last poll */
                if (ledState == FM_PLAT_PORT_LED_BLINK_ON)
                {
                    ledState = FM_PLAT_PORT_LED_BLINK_OFF;

                    hwResIdList[i]  = hwResIdList[cnt];
                    ledStateList[i] = ( HW_LED_STATE_SET_STATE(ledState) |
                                        HW_LED_STATE_SET_SPEED(ledSpeed) );
                    MOD_LED_DEBUG("Blink OFF: add to list at idx %d "
                                  "hwId=0x%x ledState 0x%x\n", 
                                  i,
                                  hwResIdList[i],
                                  ledStateList[i]);
                    i++;
                }
            }

            /* Save the number of port in ledStateList */
            activePortCnt = i;
        }
        else
        {
            activePortCnt = 0;
        }


        if (activePortCnt)
        {
            if ( fmPlatformMgmtTakeSwitchLock(sw) == FM_OK )
            {
                MOD_LED_DEBUG("SetPortLed: port count %d\n", activePortCnt);
                TAKE_PLAT_I2C_BUS_LOCK(sw);
                libFunc->SetPortLed(swCfg->swNum,
                                    hwResIdList,
                                    activePortCnt,
                                    ledStateList);
                DROP_PLAT_I2C_BUS_LOCK(sw);
                fmPlatformMgmtDropSwitchLock(sw);
            }
            else
            {
                MOD_LED_DEBUG("LedThread: sw=%d unable to get lock\n", sw);
            }
        }

ABORT:
        fmDelay(portLedPeriodSec, portLedPeriodNsec);

    }   /* end while (1) */

}   /* end fmPlatformLedThread */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformLedInit
 * \ingroup intPlatformLed
 *
 * \desc            This function initializes LED functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLedInit(fm_int sw)
{
    fm_status status = FM_OK;
    fm_int    portIdx;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    /* See if there are ports with software driven LED */
    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        if ( FM_PLAT_PORT_IS_SW_LED( FM_PLAT_GET_PORT_CFG(sw, portIdx) ) )
        {
            break;
        }
    }

    if ( portIdx >= FM_PLAT_NUM_PORT(sw) )
    {
        /* No port, so no need to enable LED thread */
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    if ( !(FM_PLAT_GET_LIB_FUNCS_PTR(sw)->SetPortLed) )
    {
        /* No support */
        FM_LOG_PRINT("LED management thread disabled from LT config file \n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    /* NOTE: This structure can be indexed by portIdx similar to
     *        fm_platformCfgPort
     */
    GET_PLAT_STATE(sw)->ledInfo = 
                    fmAlloc( FM_PLAT_NUM_PORT(sw) * sizeof(fm_platLedInfo) );

    if ( GET_PLAT_STATE(sw)->ledInfo == NULL )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
    }

    memset( GET_PLAT_STATE(sw)->ledInfo,
            0,
            FM_PLAT_NUM_PORT(sw) * sizeof(fm_platLedInfo) );

    if (FM_PLAT_GET_SWITCH_CFG(sw)->ledPollPeriodMsec <= 0)
    {
        FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                       "LED management thread not started\n");
    }
    else
    {
        status = fmCreateThread("LED Thread",
                                FM_EVENT_QUEUE_SIZE_NONE,
                                &fmPlatformLedThread,
                                &(GET_PLAT_STATE(sw)->sw),
                                &GET_PLAT_STATE(sw)->ledThread);
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformLedInit */




/*****************************************************************************/
/* fmPlatformLedSetPortState
 * \ingroup platform
 *
 * \desc            Set the port LED status given the API port status. 
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       isConfig specifies whether the state is notified from the
 *                  port configuration or from port event notification.
 *
 * \param[in]       state indicates the port's state (see 'Port States') to
 *                  show.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLedSetPortState(fm_int  sw,
                                    fm_int  port,
                                    fm_bool isConfig,
                                    fm_int  state)
{
    fm_platPortLedState ledLinkState;
    fm_platPortLedState ledState;
    fm_platPortLedSpeed ledSpeed;
    fm_platformCfgPort *portCfg;
    fm_platformLib *    libFunc;
    fm_int              portIdx;
    fm_uint32           hwResId;
    fm_uint32           speed;
    fm_status           status;

    if ( !GET_PLAT_STATE(sw)->ledInfo )
    {
        /* No support or not used */
        return FM_OK;
    }

    portIdx = fmPlatformCfgPortGetIndex(sw, port);
    if ( portIdx < 0 )
    {
        return FM_ERR_INVALID_PORT;
    }

    portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

    MOD_LED_DEBUG("port %d state %d isConfig %d\n", port, state, isConfig);

    /* Is software driven port LED */
    if ( !FM_PLAT_PORT_IS_SW_LED(portCfg) ||
         portCfg->intfType == FM_PLAT_INTF_TYPE_NONE )
    {
        /* No just return OK */
        return FM_OK;
    }

    if ( isConfig && portCfg->intfType == FM_PLAT_INTF_TYPE_PCIE )
    {
        /* Ignore port state change from the application for PCIE ports, as
           the port PCIE state machine provides real port state prior the
           application set port state. */
        return FM_OK;
    }
    else if ( isConfig || state != FM_PORT_STATE_UP )
    {
        /**************************************************
         * Turn OFF the LED if 
         * - the port is not UP 
         * or 
         * - the application changed the port state.
         *   In that case the API port state machine will
         *   provide the proper port state afterward. 
         **************************************************/

        ledLinkState = FM_PLAT_PORT_LED_LINK_DOWN;
        GET_PLAT_STATE(sw)->ledInfo[portIdx].linkState = 0;
    }
    else
    {
        ledLinkState = FM_PLAT_PORT_LED_LINK_UP;
        GET_PLAT_STATE(sw)->ledInfo[portIdx].linkState = 1;
    }

    /* Get the actual port speed from the API */
    status = fmGetPortAttribute(sw, port, FM_PORT_SPEED, &speed);
    if ( status != FM_OK )
    {
        /* Should not have an error here, but in case of error just
           force 10G and continue. */
        speed = 10000;
    }

    /* Save speed in led info structure */
    ledSpeed = GetLedSpeed(speed);
    GET_PLAT_STATE(sw)->ledInfo[portIdx].speed = ledSpeed;

    if ( fmPlatformMgmtTakeSwitchLock(sw) == FM_OK )
    {
        hwResId  = portCfg->hwResourceId;
        ledState = HW_LED_STATE_SET_STATE(ledLinkState) |
                   HW_LED_STATE_SET_SPEED(ledSpeed);

        MOD_LED_DEBUG("LedSetPortState: port %d hwId=0x%x ledState 0x%x\n", 
                      portCfg->port,
                      hwResId,
                      ledState);
        libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

        TAKE_PLAT_I2C_BUS_LOCK(sw);

        libFunc->SetPortLed(FM_PLAT_GET_SWITCH_CFG(sw)->swNum,
                            &hwResId,
                            1,
                            &ledState);

        DROP_PLAT_I2C_BUS_LOCK(sw);
        fmPlatformMgmtDropSwitchLock(sw);
    }
    else
    {
        MOD_LED_DEBUG("LedSetPortState: sw=%d unable to get lock\n", sw);
    }

    return FM_OK;

}   /* end fmPlatformLedSetPortState */
