/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_lib.h
 * Creation Date:   June 2, 2014
 * Description:     Platform functions to handle dynamic linked libraries.
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

#ifndef __FM_PLATFORM_LIB_H
#define __FM_PLATFORM_LIB_H

#define FM_PLAT_LIB_INIT_FUNC_NAME            "fmPlatformLibInit"

#define FM_PLAT_DEBUG_FUNC_NAME               "fmPlatformLibDebugDump"
#define FM_PLAT_INIT_SW_FUNC_NAME             "fmPlatformLibInitSwitch"
#define FM_PLAT_RESET_SWITCH_FUNC_NAME        "fmPlatformLibResetSwitch"
#define FM_PLAT_I2C_RW_FUNC_NAME              "fmPlatformLibI2cWriteRead"
#define FM_PLAT_SEL_BUS_FUNC_NAME             "fmPlatformLibSelectBus"
#define FM_PLAT_GET_PORT_XCVR_STATE_FUNC_NAME "fmPlatformLibGetPortXcvrState"
#define FM_PLAT_SET_PORT_XCVR_STATE_FUNC_NAME "fmPlatformLibSetPortXcvrState"
#define FM_PLAT_SET_PORT_LED_FUNC_NAME        "fmPlatformLibSetPortLed"
#define FM_PLAT_ENABLE_PORT_INTR_FUNC_NAME    "fmPlatformLibEnablePortIntr"
#define FM_PLAT_GET_PORT_INTR_PEND_FUNC_NAME  "fmPlatformLibGetPortIntrPending"
#define FM_PLAT_POST_INIT_FUNC_NAME           "fmPlatformLibPostInit"
#define FM_PLAT_SET_VRM_VOLTAGE_FUNC_NAME     "fmPlatformLibSetVrmVoltage"
#define FM_PLAT_GET_VRM_VOLTAGE_FUNC_NAME     "fmPlatformLibGetVrmVoltage"

#define FM_PLAT_DEBUG_FUNC_NAME_SHORT               "DebugDump"
#define FM_PLAT_INIT_SW_FUNC_NAME_SHORT             "InitSwitch"
#define FM_PLAT_RESET_SWITCH_FUNC_NAME_SHORT        "ResetSwitch"
#define FM_PLAT_I2C_RW_FUNC_NAME_SHORT              "I2cWriteRead"
#define FM_PLAT_SEL_BUS_FUNC_NAME_SHORT             "SelectBus"
#define FM_PLAT_GET_PORT_XCVR_STATE_FUNC_NAME_SHORT "GetPortXcvrState"
#define FM_PLAT_SET_PORT_XCVR_STATE_FUNC_NAME_SHORT "SetPortXcvrState"
#define FM_PLAT_SET_PORT_LED_FUNC_NAME_SHORT        "SetPortLed"
#define FM_PLAT_ENABLE_PORT_INTR_FUNC_NAME_SHORT    "EnablePortIntr"
#define FM_PLAT_GET_PORT_INTR_PEND_FUNC_NAME_SHORT  "GetPortIntrPending"
#define FM_PLAT_POST_INIT_FUNC_NAME_SHORT           "PostInit"
#define FM_PLAT_SET_VRM_VOLTAGE_FUNC_NAME_SHORT     "SetVrmVoltage"
#define FM_PLAT_GET_VRM_VOLTAGE_FUNC_NAME_SHORT     "GetVrmVoltage"

typedef fm_status (*fm_platDoDebug)(fm_int sw,
                                    fm_uint32 hwResourceId,
                                    fm_text action,
                                    fm_text args);

typedef fm_status (*fm_platInitSwitchFunc)(fm_int swNum);

/* Switch reset interface */
typedef fm_status (*fm_platResetSwitchFunc)(fm_int swNum, fm_bool reset);

/* I2C interface */
typedef fm_status (*fm_platI2cWrRdFunc)(fm_int swNum,
                                        fm_int devAddr,
                                        fm_byte *data,
                                        fm_int   wl,
                                        fm_int   rl);

/* Transceiver management interfaces */
typedef fm_status (*fm_platSelectBusFunc)(fm_int swNum, fm_int bus, fm_uint32 hwResourseId);

typedef fm_status (*fm_platGetPortXcvrState)(fm_int swNum,
                                             fm_uint32 *hwResourseIdList,
                                             fm_int numPorts,
                                             fm_uint32 *xcvrStateValid,
                                             fm_uint32 *xcvrState);

typedef fm_status (*fm_platSetPortXcvrState)(fm_int swNum,
                                             fm_uint32 hwResourseId,
                                             fm_uint32 xcvrStateValid,
                                             fm_uint32 xcvrState);

/* LED interface */
typedef fm_status (*fm_platSetPortLed)(fm_int swNum,
                                       fm_uint32 *hwResourseIdList,
                                       fm_int numHwId,
                                       fm_platPortLedState *ledState);

/* Interrupt interfaces */
typedef fm_status (*fm_platEnablePortIntr)(fm_int swNum,
                                           fm_uint32 *hwResourseIdList,
                                           fm_int numPorts,
                                           fm_bool *enable);

typedef fm_status (*fm_platGetPortIntrPending)(fm_int swNum,
                                               fm_uint32 *hwResourseIdList,
                                               fm_int listSize,
                                               fm_int *numPorts);

typedef fm_status (*fm_platPostInit)(fm_int swNum,
                                     fm_int upStatus);

/* Voltage regulator module */
typedef fm_status (*fm_platSetVrmVoltage)(fm_int     sw,
                                          fm_uint32  hwResourceId,
                                          fm_uint32  mVolt);

typedef fm_status (*fm_platGetVrmVoltage)(fm_int     sw,
                                          fm_uint32  hwResourceId,
                                          fm_uint32 *mVolt);

#define FM_PLAT_GET_LIB_FUNCS_PTR(sw) (&(fmPlatformProcessState[0].libFuncs))

/* Structure to store platform shared library function pointers */
typedef struct _fm_platformLib
{
    fm_platDoDebug              DoDebug;
    fm_platInitSwitchFunc       InitSwitch;
    fm_platResetSwitchFunc      ResetSwitch;
    fm_platI2cWrRdFunc          I2cWriteRead;
    fm_platSelectBusFunc        SelectBus;
    fm_platGetPortXcvrState     GetPortXcvrState;
    fm_platSetPortXcvrState     SetPortXcvrState;
    fm_platSetPortLed           SetPortLed;
    fm_platEnablePortIntr       EnablePortIntr;
    fm_platGetPortIntrPending   GetPortIntrPending;
    fm_platPostInit             PostInit;
    fm_platSetVrmVoltage        SetVrmVoltage;
    fm_platGetVrmVoltage        GetVrmVoltage;

} fm_platformLib;


fm_status fmPlatformLibLoad(fm_int sw);


#endif /* __FM_PLATFORM_LIB_H */
