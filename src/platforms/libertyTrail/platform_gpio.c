/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_gpio.c
 * Creation Date:   June 2, 2014
 * Description:     Platform Switch GPIO functions.
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define MAX_GPIO_NUM    (FM_PLAT_NUM_GPIO - 1)

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
/* fmPlatformGpioInit
 * \ingroup intPlatformGpio
 *
 * \desc            This function initializes switch GPIO functions.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioInit(fm_int sw)
{
    fm10000_switch *switchExt;
    fm_switch *     switchPtr;
    fm_status       err;
    fm_uint32       cfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    if ( GET_PLAT_STATE(sw)->family == FM_SWITCH_FAMILY_FM10000 )
    {
        switchPtr = GET_SWITCH_PTR(sw);

        /* Mask out all GPIO interrupts in GPIO_IM register */
        err = switchPtr->WriteUINT32(sw, FM10000_GPIO_IM(), 0xffffffff);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        /* Clear any pending interrupts in GPIO_IP register */
        err = switchPtr->WriteUINT32(sw, FM10000_GPIO_IP(), 0xffffffff);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        /* Initialize all GPIOs as input but not GPIO14. GPIO14 can be set as
           output by the NVM to control SwitchRdySignal. */

        /* Read current GPIO_CFG register */
        err = switchPtr->ReadUINT32(sw, FM10000_GPIO_CFG(), &cfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        /* Clear all bits in GPIO_CFG but not GPIO14 */
        cfg &= 0x4000;

        /* Write back GPIO_CFG */
        err = switchPtr->WriteUINT32(sw, FM10000_GPIO_CFG(), cfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        switchExt = switchPtr->extension;

        /* Enable global GPIO interrupt */
        FM_SET_UNNAMED_FIELD64(switchExt->interruptMaskValue,
                               FM10000_INTERRUPT_MASK_INT_b_GPIO,
                               1,
                               0);
    }
    else
    {
        err = FM_OK;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformGpioInit */




/*****************************************************************************/
/* fmPlatformGpioSetValue
 * \ingroup intPlatformGpio
 *
 * \desc            Set a GPIO pin state to the given value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the GPIO number on which to operate.
 *
 * \param[in]       value is the value to set. Zero for low, non-zero for high.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioSetValue(fm_int sw, fm_int gpio, fm_int value)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint32  data;

    if ( gpio > MAX_GPIO_NUM )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Read current GPIO_DATA register */
    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_DATA(), &data);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    if ( value == 0 )
    {
        data &= ~(1 << gpio);
    }
    else
    {
        data |= (1 << gpio);
    }

    /* Write new value */
    err = switchPtr->WriteUINT32(sw, FM10000_GPIO_DATA(), data);

ABORT:
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmPlatformGpioSetValue */




/*****************************************************************************/
/* fmPlatformGpioGetValue
 * \ingroup intPlatformGpio
 *
 * \desc            Return the current GPIO pin state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the GPIO number on which to operate.
 *
 * \param[out]      value points to the location where the function
 *                  will store the GPIO state value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioGetValue(fm_int sw, fm_int gpio, fm_int *value)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint32  data;

    if ( gpio > MAX_GPIO_NUM )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Read current GPIO_DATA register */
    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_DATA(), &data);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    *value = data & (1 << gpio);

    UNPROTECT_SWITCH(sw);

ABORT:
    return err;

}   /* end fmPlatformGpioGetValue */




/*****************************************************************************/
/* fmPlatformGpioSetDirection
 * \ingroup intPlatformGpio
 *
 * \desc            Set the GPIO pin direction.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the GPIO number on which to operate.
 *
 * \param[in]       direction is the pin direction to set.
 *
 * \param[in]       value is the initial pin state to set..
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioSetDirection(fm_int               sw,
                                     fm_int               gpio,
                                     fm_platGpioDirection direction,
                                     fm_int               value)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint32  cfg;

    if ( gpio > MAX_GPIO_NUM )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Read current GPIO_CFG register */
    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_CFG(), &cfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    if ( direction == FM_PLAT_GPIO_DIR_INPUT )
    {
        /* Clear the corresponding 'Dir' bit to set as input pin */
        cfg &= ~(1 << gpio);

        /* Write new CFG value */
        err = switchPtr->WriteUINT32(sw, FM10000_GPIO_CFG(), cfg);
    }
    else
    {
        /* Set the corresponding 'Dir' bit to set as output pin */
        cfg |= (1 << gpio);

        if ( direction == FM_PLAT_GPIO_DIR_OPEN_DRAIN )
        {
            /* Set the corresponding 'OpenDrain' bit */
            cfg |= ( 1 << (gpio + 16) );
        }
        else
        {
            /* Clear the corresponding 'OpenDrain' bit */
            cfg &= ~( 1 << (gpio + 16) );
        }

        /* Write new CFG value */
        err = switchPtr->WriteUINT32(sw, FM10000_GPIO_CFG(), cfg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

        /* Set the initial pin state */
        err = fmPlatformGpioSetValue(sw, gpio, value);
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmPlatformGpioSetDirection */




/*****************************************************************************/
/* fmPlatformGpioGetDirection
 * \ingroup intPlatformGpio
 *
 * \desc            Return the GPIO pin direction.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the GPIO number on which to operate.
 *
 * \param[out]      direction points to the location where the function
 *                  will store the GPIO direction value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioGetDirection(fm_int                sw,
                                     fm_int                gpio,
                                     fm_platGpioDirection *direction)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint32  cfg;

    if ( gpio > MAX_GPIO_NUM )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Read current GPIO_CFG register */
    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_CFG(), &cfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    if ( cfg & (1 << gpio) )
    {
        if ( cfg & ( 1 << (gpio + 16) ) )
        {
            *direction = FM_PLAT_GPIO_DIR_OPEN_DRAIN;
        }
        else
        {
            *direction = FM_PLAT_GPIO_DIR_OUTPUT;
        }
    }
    else
    {
        *direction = FM_PLAT_GPIO_DIR_INPUT;
    }

ABORT:
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmPlatformGpioGetDirection */




/*****************************************************************************/
/* fmPlatformGpioMaskIntr
 * \ingroup intPlatformGpio
 *
 * \desc            Mask interrupt for the given GPIO.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the GPIO number on which to operate.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioMaskIntr(fm_int sw, fm_int gpio)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint32  gpio_im;
    fm_uint32  mask;

    if ( gpio > MAX_GPIO_NUM )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Read current GPIO_IM register */
    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_IM(), &gpio_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Set the corresponding 'detectHighMask' bit */
    mask  = FM_GET_FIELD(gpio_im, FM10000_GPIO_IM, detectHighMask);
    mask |= ( 1 << gpio );
    FM_SET_FIELD(gpio_im, FM10000_GPIO_IM, detectHighMask, mask);

    /* Set the corresponding 'detectLowMask' bits */
    mask  = FM_GET_FIELD(gpio_im, FM10000_GPIO_IM, detectLowMask);
    mask |= ( 1 << gpio );
    FM_SET_FIELD(gpio_im, FM10000_GPIO_IM, detectLowMask, mask);

    /* Write new GPIO_IM value */
    err = switchPtr->WriteUINT32(sw, FM10000_GPIO_IM(), gpio_im);

ABORT:
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmPlatformGpioMaskIntr */




/*****************************************************************************/
/* fmPlatformGpioUnmaskIntr
 * \ingroup intPlatformGpio
 *
 * \desc            Unmask interrupt for the given GPIO.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the GPIO number on which to operate.
 *
 * \param[in]       edge is the signal transition (edge) that should cause an
 *                  interrupt (rising, falling or both edge interrupt).
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGpioUnmaskIntr(fm_int              sw,
                                   fm_int              gpio,
                                   fm_platGpioIntrEdge edge)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_uint32  gpio_im;
    fm_uint32  mask;

    if ( gpio > MAX_GPIO_NUM )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Read current GPIO_IM register */
    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_IM(), &gpio_im);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* Set or clear the corresponding 'detectHihhMask' based on edge setting */
    mask = FM_GET_FIELD(gpio_im, FM10000_GPIO_IM, detectHighMask);

    if ( edge == FM_PLAT_GPIO_INTR_RISING ||
         edge == FM_PLAT_GPIO_INTR_BOTH_EDGE )
    {
        /* Unmask this interrupt */
        mask &= ~( 1 << gpio );
    }
    else
    {
        /* Mask this interrupt */
        mask |= ( 1 << gpio );
    }
    FM_SET_FIELD(gpio_im, FM10000_GPIO_IM, detectHighMask, mask);

    /* Set or clear the corresponding 'detectLowMask' based on edge setting */
    mask = FM_GET_FIELD(gpio_im, FM10000_GPIO_IM, detectLowMask);

    if ( edge == FM_PLAT_GPIO_INTR_FALLING ||
         edge == FM_PLAT_GPIO_INTR_BOTH_EDGE )
    {
        /* Unmask this interrupt */
        mask &= ~( 1 << gpio );
    }
    else
    {
        /* Mask this interrupt */
        mask |= ( 1 << gpio );
    }
    FM_SET_FIELD(gpio_im, FM10000_GPIO_IM, detectLowMask, mask);

    /* Write new GPIO_IM value */
    err = switchPtr->WriteUINT32(sw, FM10000_GPIO_IM(), gpio_im);

ABORT:
    UNPROTECT_SWITCH(sw);

    return err;

}   /* end fmPlatformGpioUnmaskIntr */




/*****************************************************************************/
/** fmPlatformGpioInterruptHandler
 * \ingroup intPlatformGpio
 *
 * \desc            First-stage switch GPIO interrupt handler.
 * 
 * \note            The caller has taken the register lock.
 *
 * \param[in]       switchPtr points to the switch structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGpioInterruptHandler(fm_switch *switchPtr)
{
    fm_uint32 ipVal;
    fm_uint32 imVal;
    fm_int    sw;
    fm_int    gpio;
    fm_status err;

    sw = switchPtr->switchNumber;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d\n", sw);

    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_IP(), &ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    err = switchPtr->ReadUINT32(sw, FM10000_GPIO_IM(), &imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    MOD_INTR_DEBUG("Switch %d: IP=0x%x IM=0x%x\n", sw, ipVal, imVal);

    /* Mask out the interrupt */
    err = switchPtr->WriteUINT32(sw, FM10000_GPIO_IM(), ipVal | imVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Acknowledge the interrupts. */
    err = switchPtr->WriteUINT32(sw, FM10000_GPIO_IP(), ipVal);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PARITY, err);

    /* Handle only unmasked interrupts */
    ipVal &= ~imVal;
    MOD_INTR_DEBUG("Switch %d: ipVal=0x%x\n", sw, ipVal);

    if (ipVal)
    {
        for ( gpio = 0 ; gpio <= MAX_GPIO_NUM; gpio++ )
        {
            if ( ( ipVal & (1 << gpio) ) ||
                 ( ipVal & (1 << (FM10000_GPIO_IM_l_detectLowMask + gpio)) ) )
            {
                MOD_INTR_DEBUG("Switch %d: Interrupt GPIO %d\n", sw, gpio);

                if ( gpio == FM_PLAT_GET_SWITCH_CFG(sw)->gpioPortIntr )
                {
                    fmPlatformMgmtSignalInterrupt(sw, gpio);
                }
                else
                {
                    /* Notify the Application */
                    fmPlatformEventSendSwGpioInterrupt(sw,
                                                       gpio,
                                                       FM_EVENT_PRIORITY_LOW);

                    MOD_INTR_DEBUG("Switch %d: Notify APP GPIO %d interrupt\n",
                                    sw, 
                                    gpio);
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}   /* end fmPlatformGpioInterruptHandler */

