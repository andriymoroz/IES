/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_gpio.h
 * Creation Date:   April 2015
 * Description:     Platform switch GPIO functions.
 *
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __FM_PLATFORM_GPIO_H
#define __FM_PLATFORM_GPIO_H

/* GPIO pin direction */
typedef enum 
{
    FM_PLAT_GPIO_DIR_INPUT,
    FM_PLAT_GPIO_DIR_OUTPUT,
    FM_PLAT_GPIO_DIR_OPEN_DRAIN,
    FM_PLAT_GPIO_DIR_MAX

} fm_platGpioDirection;

/* GPIO interrupt edge */
typedef enum 
{
    FM_PLAT_GPIO_INTR_RISING,       /* Low to high transition interrupt */
    FM_PLAT_GPIO_INTR_FALLING,      /* High to low transition interrupt */
    FM_PLAT_GPIO_INTR_BOTH_EDGE,    /* Interrupt for both transitions   */
    FM_PLAT_GPIO_INTR_MAX

} fm_platGpioIntrEdge;

fm_status fmPlatformGpioInit(fm_int sw);

fm_status fmPlatformGpioSetValue(fm_int sw, fm_int gpio, fm_int value);

fm_status fmPlatformGpioGetValue(fm_int sw, fm_int gpio, fm_int *value);

fm_status fmPlatformGpioSetDirection(fm_int               sw,
                                     fm_int               gpio,
                                     fm_platGpioDirection direction,
                                     fm_int               value);

fm_status fmPlatformGpioGetDirection(fm_int                sw,
                                     fm_int                gpio,
                                     fm_platGpioDirection *direction);

fm_status fmPlatformGpioMaskIntr(fm_int sw, fm_int gpio);

fm_status fmPlatformGpioUnmaskIntr(fm_int              sw,
                                   fm_int              gpio,
                                   fm_platGpioIntrEdge edge);

#endif /* __FM_PLATFORM_GPIO_H */
