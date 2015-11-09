/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_sched.c
 * Creation Date:   June 2, 2014
 * Description:     Platform scheduler functions.
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


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformGenerateSchedulerPortList
 * \ingroup platform
 *
 * \desc            Generate the scheduler port list for the given switch.
 *                  The port list is based on the platform configuration.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGenerateSchedulerPortList(fm_int sw)
{
    fm_platformState *    ps;
    fm_schedulerPort *    portList;
    fm_platformCfgPort *  portCfg;
    fm_platformCfgSwitch *swCfg;
    fm_status             status;
    fm_int                portIdx;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    ps = GET_PLAT_STATE(sw);
    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    /* schedPortList should have been allocated in
     * fmPlatformAllocateSchedulerResources() */
    if (ps->schedPortList == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    status = FM_OK;

    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portList = &ps->schedPortList[portIdx];
        portCfg  = FM_PLAT_GET_PORT_CFG(sw, portIdx);

        portList->physPort = portCfg->physPort;

        switch (portCfg->portType)
        {
            case FM_PLAT_PORT_TYPE_EPL:
                portList->fabricPort = 
                    FM10000_EPL_LANE_TO_FABRIC_PORT(portCfg->epl, 
                                                    portCfg->lane[0]);

                portList->speed = (portCfg->speed > 100000) ? 100000 : portCfg->speed;
                break;

            case FM_PLAT_PORT_TYPE_PCIE:
                if (portCfg->pep == 8)
                {
                    portList->fabricPort = FM10000_PCIE8_TO_FABRIC_PORT;
                    portList->speed = 
                        (portCfg->speed > 10000) ? 10000 : portCfg->speed;
                }
                else
                {
                    portList->fabricPort = 
                        FM10000_PCIE_TO_FABRIC_PORT(portCfg->pep);
                    portList->speed = (portCfg->speed > 50000) ? 50000 : portCfg->speed;
                }
                break;

            case FM_PLAT_PORT_TYPE_TUNNEL:
                portList->speed = portCfg->speed;
                portList->fabricPort = 
                    FM10000_TE_TO_FABRIC_PORT(portCfg->tunnel);
                break;

            case FM_PLAT_PORT_TYPE_LOOPBACK:
                portList->speed = portCfg->speed;
                portList->fabricPort = 
                    FM10000_LOOPBACK_TO_FABRIC_PORT(portCfg->loopback);
                break;

            case FM_PLAT_PORT_TYPE_FIBM:
                portList->speed = portCfg->speed;
                portList->fabricPort = FM10000_FIBM_TO_FABRIC_PORT;
                break;

            default:
                portList->speed = 0;
                portList->fabricPort = -1;
                break;
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformGenerateSchedulerPortList */




/*****************************************************************************/
/* fmPlatformGetSchedulerPortList
 * \ingroup platform
 *
 * \desc            Fill up the received scheduler configuration structure
 *                  (port list) using the platform properties found.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      sc is a pointer to the caller-supplied scheduler config
 *                  structure and should be filled with automatic mode data.
 *                  Refer to fm_schedulerConfig for details on the limits
 *                  of certains structure fields and which fields are
 *                  required in each mode.
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if no token were found.
 * \return          FM_ERR_UNINITIALIZED if the scheduler token list
 *                  wasn't allocated.
 *
 *****************************************************************************/
fm_status fmPlatformGetSchedulerPortList(fm_int sw, fm_schedulerConfig *sc)
{
    fm_platformState *  ps;
    fm_status           err = FM_OK;
    
    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    ps = GET_PLAT_STATE(sw);

    if (ps->schedPortList)
    {
        err = fmPlatformGenerateSchedulerPortList(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        if (FM_PLAT_NUM_PORT(sw) <= 0)
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
        }

        sc->mode      = FM_SCHED_INIT_MODE_AUTOMATIC;
        sc->portList  = ps->schedPortList;
        sc->nbPorts   = FM_PLAT_NUM_PORT(sw);
    }
    else
    {
        err = FM_ERR_UNINITIALIZED;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformGetSchedulerPortList */




/*****************************************************************************/
/* fmPlatformAllocateSchedulerResources
 * \ingroup platform
 *
 * \desc            Allocate memory to store the scheduler portList or
 *                  token list used by automatic/manual scheduler
 *                  configurations. Since the scheduler list will be process
 *                  later, we allocate the maximum number of
 *                  tokens (512 + 1 idle).
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory allocation failed.
 *
 *****************************************************************************/
fm_status fmPlatformAllocateSchedulerResources(fm_int sw)
{
    fm_platformState *  ps;
    fm_int              size;
    fm_status           status;


    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    ps = GET_PLAT_STATE(sw);

    /* Allocate resources for automatic mode */
    size = sizeof(fm_schedulerPort) * FM_PLAT_NUM_PORT(sw);

    ps->schedPortList = fmAlloc(size);
    if (ps->schedPortList == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    /* Allocate resources for manual mode */

    /* The scheduler list register array consist of 2 port lists, an active
     * one and a standby one.  This is why we divide the entries count by 2.
     * We also have to add 1 for the implicit idle entry, which will be set
     * in this stucture */
    size = sizeof(fm_schedulerToken) *
           ((FM10000_SCHED_RX_SCHEDULE_ENTRIES / 2) + 1);

    ps->schedTokenList = fmAlloc(size);
    if (ps->schedTokenList == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    FM_MEMSET_S(ps->schedTokenList, size, 0, size);
    status = FM_OK;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformAllocateSchedulerResources */




/*****************************************************************************/
/* fmPlatformGetSchedulerTokenList
 * \ingroup platform
 *
 * \desc            Fill up the received scheduler configuration structure
 *                  (token list) using the platform properties found.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[out]      sc is a pointer to the caller-supplied scheduler config
 *                  structure and should be filled with manual mode data.
 *                  Refer to fm_schedulerConfig for details on the limits
 *                  of certains structure fields and which fields are
 *                  required in each mode.
 * 
 * \return          FM_OK if successful.
 * \return          FM_FAIL if no token were found.
 * \return          FM_ERR_UNINITIALIZED if the scheduler token list
 *                  wasn't allocated.
 *
 *****************************************************************************/
fm_status fmPlatformGetSchedulerTokenList(fm_int sw, fm_schedulerConfig *sc)
{
    fm_platformState *  ps;
    fm_status           status;


    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    ps = GET_PLAT_STATE(sw);

    if (ps->schedTokenList)
    {
        sc->mode      = FM_SCHED_INIT_MODE_MANUAL;
        sc->tokenList = ps->schedTokenList;
        sc->nbTokens  = fmPlatformCfgSchedulerGetTokenList(sw,
                                                           ps->schedTokenList);

        if (sc->nbTokens > 0)
        {
            status = FM_OK;
        }
        else
        {
            status = FM_FAIL;
        }
    }
    else
    {
        status = FM_ERR_UNINITIALIZED;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformGetSchedulerTokenList */




/*****************************************************************************/
/* fmPlatformGetSchedulerConfig
 * \ingroup platform
 *
 * \desc            Indicates to the API how the scheduler should be
 *                  initialized for an FM10000 device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      sc is a pointer to the caller-supplied scheduler config
 *                  structure and should be filled with the desired mode and
 *                  data. Refer to fm_schedulerConfig for details on the
 *                  limits of certains structure fields and which fields
 *                  are required in each mode.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetSchedulerConfig(fm_int sw, fm_schedulerConfig *sc)
{
    fm_schedulerConfigMode  mode;
    fm_status               status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d, fm_schedulerConfig=%p\n", 
                 sw, 
                 (void *) sc);

    mode = fmPlatformGetSchedulerConfigMode(sw);

    switch (mode)
    {
        case FM_SCHED_INIT_MODE_AUTOMATIC:
            status = fmPlatformGetSchedulerPortList(sw, sc);
            break;

        case FM_SCHED_INIT_MODE_MANUAL:
            status = fmPlatformGetSchedulerTokenList(sw, sc);
            break;

        default:
            status = FM_FAIL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformGetSchedulerConfig */
