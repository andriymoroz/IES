/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_pca.h
 * Creation Date:   October 9, 2013
 * Description:     PCA devices functions
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

#ifndef __FM_UTIL_PCA_H
#define __FM_UTIL_PCA_H

/* PCA device model supported */
typedef enum
{
    PCA_MUX_UNKNOWN = 0,
    PCA_MUX_9541    = 1,
    PCA_MUX_9545    = 2,
    PCA_MUX_9546    = 3,
    PCA_MUX_9548    = 4,

} fm_pcaMuxModel;


typedef enum
{
    PCA_IO_UNKNOWN = 0,
    PCA_IO_9505    = 1,
    PCA_IO_9506    = 2,
    PCA_IO_9538    = 3,
    PCA_IO_9539    = 4,
    PCA_IO_9551    = 5,
    PCA_IO_9554    = 6,
    PCA_IO_9555    = 7,
    PCA_IO_9634    = 8,
    PCA_IO_9635    = 9,
    PCA_IO_9698    = 10,

} fm_pcaIoModel;


/* VRM device model supported */
typedef enum
{
	VRM_UNKNOWN  = 0,
	VRM_TPS40425 = 1,
	VRM_PX8847   = 2,

} fm_vrmModel;


typedef enum
{
    PCA_IO_REG_TYPE_INPUT,
    PCA_IO_REG_TYPE_OUTPUT,
    PCA_IO_REG_TYPE_IOC,
    PCA_IO_REG_TYPE_INTR,
    PCA_IO_REG_TYPE_MODE1,
    PCA_IO_REG_TYPE_MODE2,
    PCA_IO_REG_TYPE_PWM,
    PCA_IO_REG_TYPE_GRPPWM,
    PCA_IO_REG_TYPE_GRPFREQ,
    PCA_IO_REG_TYPE_LEDOUT,
    PCA_IO_REG_TYPE_SUBADDR,
    PCA_IO_REG_TYPE_ALLCALLADR,
    PCA_IO_REG_TYPE_PSC0,
    PCA_IO_REG_TYPE_PSC1,
    PCA_IO_REG_TYPE_PWM0,
    PCA_IO_REG_TYPE_PWM1,

}  fm_pcaIoRegType;

/* Per PCA9634/35 data sheet for LEDOUT register */
#define FM_PCA_LEDOUT_OFF           0
#define FM_PCA_LEDOUT_ON            2
#define FM_PCA_LEDOUT_BLINK         3

/* Per PCA9551 data sheet for LED Selector registers LS0/LS1  */
#define FM_PCA_LS_ON            0
#define FM_PCA_LS_OFF           1
#define FM_PCA_LS_BLINK0        2
#define FM_PCA_LS_BLINK1        3

/* Max number of bytes supported by PCA IO */
#define FM_PCA_IO_REG_MAX_SIZE             5

/* Auto increment reg address */
#define FM_PCA_IO_AUTO_INCR             0x80

/* PCA IO capabilities */
#define FM_PCA_IO_CAP_INTR          (1 << 0)    /* Support Interrupt */

#define FM_PCA9634_NUM_LEDS         8
#define FM_PCA9635_NUM_LEDS         16
#define FM_PCA9634_NUM_REGS         (0x11 + 1)
#define FM_PCA9635_NUM_REGS         (0x1B + 1)

/* Max number of registers supported by PCA LED driver */
#define FM_PCA_LED_MAX_REGS         FM_PCA9635_NUM_REGS

/* Max number of LEDs supported by the PCA LED driver */
#define FM_PCA_LED_MAX_LEDS         FM_PCA9635_NUM_LEDS

/* Number of LEDs per LEDOUT register (apply to PCA9551 and PCA9634/35) */
#define FM_PCA_LED_PER_LEDOUT       4

/* Number of LEDOUT register */
#define FM_PCA_LED_NUM_LEDOUT   (FM_PCA_LED_MAX_LEDS / FM_PCA_LED_PER_LEDOUT)


/* Device capabitilies */
typedef struct
{
    /* Number of bits on that PCA IO or
       Number of LEDs on that PCA LED driver */
    fm_uint numBits;

    /* Number of bytes on that PCA IO or
       Number of LEDOUT registers on that PCA LED driver */
    fm_uint numBytes;

    /* See FM_PCA_IO_CAP_XX */
    fm_uint32 cap;

} fm_pcaIoCap;


/* PCA IO expender registers */
typedef struct
{
    /* Input port */
    fm_byte input[FM_PCA_IO_REG_MAX_SIZE];

    /* Output port */
    fm_byte output[FM_PCA_IO_REG_MAX_SIZE];

    /* Input output configuration */
    fm_byte ioc[FM_PCA_IO_REG_MAX_SIZE];

    /* Interrupt configuration, if applicable */
    fm_byte intr[FM_PCA_IO_REG_MAX_SIZE];

} fm_pcaIoRegs;


/* PCA LED driver registers */
typedef struct
{
    /*************************************
     Variables specific to the PCA9634/35
    **************************************/
    /* Mode register 1 and 2 */
    fm_byte mode[2];

    /* Brightness control LEDn */
    fm_byte pwm[FM_PCA_LED_MAX_LEDS];

    /* group[0] duty cycle control and group[1] frequency */
    fm_byte group[2];

    /* addr[0..2]: I2C-bus subaddresses, addr[3] All Call address */
    fm_byte addr[4];

    /*************************************
     Variables specific to the PCA9551
    **************************************/

    /* Frequency Prescaler: PSC0, PSC1 */
    fm_byte psc0;
    fm_byte psc1;

    /* Pulse Width Moduleation: PWMSC0, PSC1 */
    fm_byte pwm0;
    fm_byte pwm1;

    /*************************************
     Variables shared between the PCA9551
     and the PCA9634/35.
    **************************************/

    /* LED output state. Correspond to LS0/LS1 registers for the PCA9551 */
    fm_byte ledout[FM_PCA_LED_NUM_LEDOUT];

    /* Value to write the LEDOUT register to control the LED */
    fm_int ledOn;
    fm_int ledOff;
    fm_int ledBlink;

} fm_pcaLedRegs;


/* PCA I/O device configuration */
typedef struct
{
    /* i2c bus where the device is located */
    fm_uint bus;

    /* I2C address of the device */
    fm_uint addr;

    /* I2C file descriptor */
    fm_uintptr fd;

    /* Indicates if the I2C_SMBUS_I2C_BLOCK_DATA transaction type is
     * supported by the linux SMBus driver associated to the i2c file desc.
     */
    fm_int i2cBlockSupported;

    /* I2C write/read function pointer to use to access this device */
    fm_utilI2cWriteReadHdnlFunc func;

    /* PCA device type */
    fm_pcaIoModel model;

    /* Device capabilities */
    fm_pcaIoCap devCap;

    /* PCA IO expender registers */
    fm_pcaIoRegs cachedRegs;

    /* PCA LED driver registers */
    fm_pcaLedRegs ledRegs;

    /* PCA LED driver blinking period */
    fm_uint ledBlinkPeriod;

    /* PCA LED driver brightness */
    fm_uint ledBrightness;

} fm_pcaIoDevice;


fm_status fmUtilPcaIoGetCap(fm_pcaIoModel model, fm_pcaIoCap *cap);

fm_status fmUtilPcaIoInit(fm_pcaIoDevice *dev);

fm_status fmUtilPcaIoDump(fm_pcaIoDevice *dev);

fm_status fmUtilPcaIoUpdateInputRegs(fm_pcaIoDevice *dev);

fm_status fmUtilPcaIoWriteRegs(fm_pcaIoDevice *dev,
                               fm_pcaIoRegType regType,
                               fm_uint         startReg,
                               fm_uint         numBytes);

fm_status fmUtilPcaIoReadRegs(fm_pcaIoDevice *dev,
                              fm_pcaIoRegType regType,
                              fm_uint         startReg,
                              fm_uint         numBytes);

fm_status fmUtilPca9541TakeBusControl(fm_uintptr                  handle, 
                                      fm_utilI2cWriteReadHdnlFunc func,
                                      fm_uint                     addr);


#endif /* __FM_UTIL_PCA_H */
