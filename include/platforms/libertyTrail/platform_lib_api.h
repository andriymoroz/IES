/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_lib_api.h
 * Creation Date:   June 2, 2014
 * Description:     Defines for platform shared library interfaces.
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

#ifndef __FM_PLATFORM_LIB_API_H
#define __FM_PLATFORM_LIB_API_H


/* Well-known bus types */
typedef enum
{
    /** Not applicable, does not pass down to shared library interface */
    FM_PLAT_BUS_NA = 0,

    /** Disable to all buses, mainly for debug */
    FM_PLAT_BUS_DISABLED,

    /** Bus to transceiver eeprom */
    FM_PLAT_BUS_XCVR_EEPROM,

    /** Bus to transceiver state */
    FM_PLAT_BUS_XCVR_STATE,

    /** Bus to transceiver eeprom */
    FM_PLAT_BUS_PHY,

    /** Provide through HwResourceId the bus number to select. The bus number
     *  being one of the bus%d in api.platform.lib.config.bus%d.i2cDevName */
    FM_PLAT_BUS_NUMBER,

    /** End of well-known buses */
    FM_PLAT_BUS_MAX,

} fm_platBusType;


/* Platform Transceiver state 
 * Note: Some of these might be only applicable to SFP+ or QSFP */
#define FM_PLAT_XCVR_PRESENT    (1 << 0)    /* Present */
#define FM_PLAT_XCVR_RXLOS      (1 << 1)    /* RXLOS */
#define FM_PLAT_XCVR_TXFAULT    (1 << 2)    /* TXFAULT */
#define FM_PLAT_XCVR_INTR       (1 << 3)    /* Interrupting */
#define FM_PLAT_XCVR_ENABLE     (1 << 4)    /* Enable or disable */
#define FM_PLAT_XCVR_LPMODE     (1 << 5)    /* Low power mode */

/* Port LED state */
typedef enum
{
    /* Turn off the LINK and TRAFFIC leds */
    FM_PLAT_PORT_LED_LINK_DOWN,

    /* Turn on the LINK led and turn OFF TRAFFIC led */
    FM_PLAT_PORT_LED_LINK_UP,

    /* Turn on/blink the TRAFFIC led (also indicate LINK up) */
    FM_PLAT_PORT_LED_BLINK_ON,

    /* Turn off/blink the TRAFFIC led (also indicate LINK up)
     * Used by the SW_CONTROL blink mode only */
    FM_PLAT_PORT_LED_BLINK_OFF,

} fm_platPortLedState;

/* Port LED speed indication */
typedef enum
{
    FM_PLAT_PORT_LED_SPEED_10M = 0,
    FM_PLAT_PORT_LED_SPEED_100M,
    FM_PLAT_PORT_LED_SPEED_1G,
    FM_PLAT_PORT_LED_SPEED_2P5G,
    FM_PLAT_PORT_LED_SPEED_10G,
    FM_PLAT_PORT_LED_SPEED_25G,
    FM_PLAT_PORT_LED_SPEED_40G,
    FM_PLAT_PORT_LED_SPEED_100G

} fm_platPortLedSpeed;


/* Use bits 3..0 to store the led  state (fm_platPortLedState)
 * Use bits 7..4 to store the port speed (fm_platPortLedSpeed)
 */
#define HW_LED_STATE_TO_STATE(x)        (x & 0x0F)
#define HW_LED_STATE_SET_STATE(x)       (x & 0xF)

#define HW_LED_STATE_TO_SPEED(x)        ((x & 0xF0) >> 4)
#define HW_LED_STATE_SET_SPEED(x)       ((x & 0xF) << 4)

/* Use bits 7..0 for HW resource ID
 * Use bits 11..8 to store the LED number
 */
#define HW_RESOURCE_ID_TO_IDX(x)        (x & 0xFF)
#define HW_RESOURCE_ID_SET_IDX(x)       (x & 0xFF)
#define HW_RESOURCE_ID_TO_LEDNUM(x)     ((x & 0xF00) >> 8)
#define HW_RESOURCE_ID_SET_LEDNUM(x)    ((x & 0xF) << 8)
#define HW_RESOURCE_ID_TO_VRMSUBCHAN(x) ((x & 0xF00) >> 8)

#endif /* __FM_PLATFORM_LIB_API_H */
