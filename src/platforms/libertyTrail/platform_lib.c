/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_lib.c
 * Creation Date:   June 2, 2014
 * Description:     Functions to handle optional shared libraries.
 *
 * Copyright (c) 2014, Intel Corporation
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
/* fmPlatformLibLoad
 * \ingroup intPlatform
 *
 * \desc            Load platform library functions.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformLibLoad(fm_int sw)
{
    fm_status          status;
    fm_platformCfgLib *libCfg;
    fm_platformLib    *libFunc;

    fm_int             libHandle;
    fm_int             tempHandle;
    void *             funcAddr;

    union
    {
        fm_platResetSwitchFunc    swResetFunc;
        fm_platI2cWrRdFunc        i2cWrRdFunc;
        fm_platInitSwitchFunc     initSwitchFunc;
        fm_platSelectBusFunc      selectBusFunc;
        fm_platGetPortXcvrState   getPortXcvrStateFunc;
        fm_platSetPortXcvrState   setPortXcvrStateFunc;
        fm_platSetPortLed         setPortLedFunc;
        fm_platEnablePortIntr     enablePortIntrFunc;
        fm_platGetPortIntrPending getPortIntrPendFunc;
        fm_platDoDebug            doDebugFunc;
        fm_platPostInit           PostInitFunc;
        fm_platSetVrmVoltage      SetVrmVoltageFunc;
        fm_platGetVrmVoltage      GetVrmVoltageFunc;
        void *                    obj;

    } alias;

    if (sw < 0 || sw >= FM_PLAT_NUM_SW)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    libCfg = FM_PLAT_GET_LIBS_CFG(sw);
    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    /* NOTE: All supports are optional so don't return error unless neccessary */

    libHandle = -1;
    if ( strlen(libCfg->libName) )
    {
        status = fmLoadAndInitDynamicLib(libCfg->libName,
                                         FM_PLAT_LIB_INIT_FUNC_NAME,
                                         NULL,
                                         &tempHandle);
        libHandle = status ? -1 : tempHandle;
    }

    /* Switch reset library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_RESET_SWITCH_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_RESET_SWITCH_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj            = funcAddr;
            libFunc->ResetSwitch = alias.swResetFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_RESET_SWITCH_FUNC_NAME);
        }
    }

    /* I2C library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_I2C_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_I2C_RW_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj             = funcAddr;
            libFunc->I2cWriteRead = alias.i2cWrRdFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_I2C_RW_FUNC_NAME);
        }
    }

    /* Debug function */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_DEBUG_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_DEBUG_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj        = funcAddr;
            libFunc->DoDebug = alias.doDebugFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_DEBUG_FUNC_NAME);
        }
    }

    /* Transceiver library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_INIT_SW_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_INIT_SW_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj           = funcAddr;
            libFunc->InitSwitch = alias.initSwitchFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_INIT_SW_FUNC_NAME);
        }
    }

    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_SEL_BUS_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_SEL_BUS_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj          = funcAddr;
            libFunc->SelectBus = alias.selectBusFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s\n",
                           fmErrorMsg(status),
                           FM_PLAT_SEL_BUS_FUNC_NAME);
        }
    }

    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_GET_PORT_XCVR_STATE_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_GET_PORT_XCVR_STATE_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj                 = funcAddr;
            libFunc->GetPortXcvrState = alias.getPortXcvrStateFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_GET_PORT_XCVR_STATE_FUNC_NAME);
        }
    }

    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_SET_PORT_XCVR_STATE_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_SET_PORT_XCVR_STATE_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj                 = funcAddr;
            libFunc->SetPortXcvrState = alias.setPortXcvrStateFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_SET_PORT_XCVR_STATE_FUNC_NAME);
        }
    }

    /* LED library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_SET_PORT_LED_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_SET_PORT_LED_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj           = funcAddr;
            libFunc->SetPortLed = alias.setPortLedFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_SET_PORT_LED_FUNC_NAME);
        }
    }

    /* Interrupt library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_ENABLE_PORT_INTR_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_ENABLE_PORT_INTR_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj               = funcAddr;
            libFunc->EnablePortIntr = alias.enablePortIntrFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_ENABLE_PORT_INTR_FUNC_NAME);
        }
    }

    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_GET_PORT_INTR_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_GET_PORT_INTR_PEND_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj                   = funcAddr;
            libFunc->GetPortIntrPending = alias.getPortIntrPendFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_GET_PORT_INTR_PEND_FUNC_NAME);
        }
    }

    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_POST_INIT_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_POST_INIT_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj         = funcAddr;
            libFunc->PostInit = alias.PostInitFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_POST_INIT_FUNC_NAME);
        }
    }

    /* VRM set library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_SET_VRM_VOLTAGE_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_SET_VRM_VOLTAGE_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj              = funcAddr;
            libFunc->SetVrmVoltage = alias.SetVrmVoltageFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_SET_VRM_VOLTAGE_FUNC_NAME);
        }
    }

    /* VRM get library support */
    if ( (libHandle >= 0) && !(libCfg->disableFuncIntf & FM_PLAT_DISABLE_GET_VRM_VOLTAGE_FUNC) )
    {
        status = fmGetDynamicLoadSymbol(libHandle, FM_PLAT_GET_VRM_VOLTAGE_FUNC_NAME, &funcAddr);

        if (status == FM_OK)
        {
            alias.obj              = funcAddr;
            libFunc->GetVrmVoltage = alias.GetVrmVoltageFunc;
        }
        else
        {
            FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                           "%s: Unable to load %s.\n",
                           fmErrorMsg(status),
                           FM_PLAT_GET_VRM_VOLTAGE_FUNC_NAME);
        }
    }

    return FM_OK;

}   /* end fmPlatformLibLoad */
