/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_app_api.h
 * Creation Date:   June 2, 2014
 * Description:     Platform functions exported to applications.
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

#ifndef __FM_PLATFORM_APP_API_H
#define __FM_PLATFORM_APP_API_H


/* Should be used for input argument in function fmPlatformSetVrmVoltage and  */
/* fmPlatformGetVrmVoltage  */
typedef enum
{
    FM_PLAT_VRM_VDDS,
    FM_PLAT_VRM_VDDF,
    FM_PLAT_VRM_AVDD,

} fm_platVrmType;

/* Used for direction argument in function fmPlatformSwitchGpioSetDirection */
typedef enum 
{
    FM_PLAT_SW_GPIO_DIR_INPUT,
    FM_PLAT_SW_GPIO_DIR_OUTPUT,
    FM_PLAT_SW_GPIO_DIR_OPEN_DRAIN,
    FM_PLAT_SW_GPIO_DIR_MAX

} fm_platSwGpioDir;

/* Used for edge argument in function fmPlatformSwitchGpioUnmaskIntr */
typedef enum 
{
    FM_PLAT_SW_GPIO_RISING_EDGE,    /* Low to high transition interrupt */
    FM_PLAT_SW_GPIO_FALLING_EDGE,   /* High to low transition interrupt */
    FM_PLAT_SW_GPIO_BOTH_EDGE,      /* Interrupt for both transitions   */
    FM_PLAT_SW_GPIO_EDGE_MAX

} fm_platSwGpioIntr;


fm_status fmPlatformSwitchInsert(fm_int sw);
fm_status fmPlatformSwitchRemove(fm_int sw);
fm_status fmPlatformSetRegAccessMode(fm_int sw, fm_int mode);
fm_status fmPlatformSetInterruptPollingPeriod(fm_int sw, fm_int periodMsec);
fm_status fmPlatformReadUnlockCSR(fm_int sw, fm_uint32 addr, fm_uint32 *value);
fm_status fmPlatformWriteUnlockCSR(fm_int sw, fm_uint32 addr, fm_uint32 value);

fm_status fmPlatformGetName(fm_text name, fm_int size);

fm_status fmPlatformGetNumSwitches(fm_int *numSw);

fm_status fmPlatformGetPortList(fm_int sw,
                                fm_int *portList,
                                fm_int listSize,
                                fm_int *numPorts);

fm_status fmPlatformXcvrIsPresent(fm_int   sw,
                                  fm_int   port,
                                  fm_bool *present);

fm_status fmPlatformXcvrEnable(fm_int sw,
                               fm_int port,
                               fm_bool enable);

fm_status fmPlatformXcvrEnableLpMode(fm_int sw,
                                     fm_int port,
                                     fm_bool enable);

fm_status fmPlatformXcvrMemWrite(fm_int   sw,
                                 fm_int   port,
                                 fm_int   page,
                                 fm_int   offset,
                                 fm_byte *data,
                                 fm_int   length);

fm_status fmPlatformXcvrMemRead(fm_int   sw,
                                fm_int   port,
                                fm_int   page,
                                fm_int   offset,
                                fm_byte *data,
                                fm_int   length);

fm_status fmPlatformXcvrEepromRead(fm_int   sw,
                                   fm_int   port,
                                   fm_int   page,
                                   fm_int   offset,
                                   fm_byte *data,
                                   fm_int   length);

fm_status fmPlatformSetVrmVoltage(fm_int         sw,
                                  fm_platVrmType vrmId,
                                  fm_uint32      mVolt);

fm_status fmPlatformGetVrmVoltage(fm_int         sw,
                                  fm_platVrmType vrmId,
                                  fm_uint32 *    mVolt);


fm_status fmPlatformSwitchGpioSetDirection(fm_int           sw,
                                           fm_int           gpio,
                                           fm_platSwGpioDir direction,
                                           fm_int           value);

fm_status fmPlatformSwitchGpioGetDirection(fm_int            sw,
                                           fm_int            gpio,
                                           fm_platSwGpioDir *direction);

fm_status fmPlatformSwitchGpioMaskIntr(fm_int sw, fm_int gpio);

fm_status fmPlatformSwitchGpioUnmaskIntr(fm_int            sw,
                                         fm_int            gpio,
                                         fm_platSwGpioIntr edge);

fm_status fmPlatformSwitchGpioSetValue(fm_int sw, fm_int gpio, fm_int value);
fm_status fmPlatformSwitchGpioGetValue(fm_int sw, fm_int gpio, fm_int *value);

fm_status fmPlatformGetNominalSwitchVoltages(fm_int     sw,
                                             fm_uint32 *vdds,
                                             fm_uint32 *vddf);

#endif /* __FM_PLATFORM_APP_API_H */
