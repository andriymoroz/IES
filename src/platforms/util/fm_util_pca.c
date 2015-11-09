/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_util_pca.c
 * Creation Date:   October 9, 2013
 * Description:     Functions for PCA devices.
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fm_sdk.h>

#include <platforms/util/fm_util.h>
#include <platforms/util/fm_util_pca.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define MAX_PCA9506_IO     5
#define MAX_PCA9554_IO     1
#define MAX_PCA9555_IO     2
#define MAX_PCA9551_IO     2

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
/* WritePca9506Registers
 * \ingroup platformUtils
 *
 * \desc            Write the content of the given cached registers to
 *                  the given PCA9506 device.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to write.
 * 
 * \param[in]       startReg first register offset to write.
 * 
 * \param[in]       numBytes number of registers to write.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status WritePca9506Registers(fm_pcaIoDevice *dev,
                                       fm_pcaIoRegType regType,
                                       fm_uint         startReg,
                                       fm_uint         numBytes)
{
    fm_uint   i;
    fm_byte * dataPtr;
    fm_byte   data[MAX_PCA9506_IO+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    if ( (startReg + numBytes) > MAX_PCA9506_IO)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_OUTPUT:
            ioReg = 0x08;
            dataPtr = dev->cachedRegs.output;
        break;

        case PCA_IO_REG_TYPE_IOC:
            ioReg = 0x18;
            dataPtr = dev->cachedRegs.ioc;
            break;

        case PCA_IO_REG_TYPE_INTR:
            ioReg = 0x20;
            dataPtr = dev->cachedRegs.intr;
        break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    if (dev->i2cBlockSupported)
    {
        /* Use auto-increment to write to registers */
        data[0] = (ioReg + startReg) | FM_PCA_IO_AUTO_INCR;
        for (i = 0 ; i < numBytes ; i++)
        {
            data[1 + i] = dataPtr[i + startReg];
        }
        status = dev->func(dev->fd, dev->addr, data, 1 + numBytes, 0);
    }
    else
    {
        /* Write to registers one byte at the time */
        for (i = 0 ; i < numBytes ; i++)
        {
            data[0] = ioReg + startReg + i;
            data[1] = dataPtr[i + startReg];

            status = dev->func(dev->fd, dev->addr, data, 2, 0);
            if (status != FM_OK)
            {
                break;
            }
        }
    }

    return status;

}   /* end WritePca9506Registers */




/*****************************************************************************/
/* WritePca9555Registers
 * \ingroup platformUtils
 *
 * \desc            Write the content of the given cached registers to
 *                  the given PCA9554/9555 device.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to write.
 * 
 * \param[in]       startReg first register offset to write.
 * 
 * \param[in]       numBytes number of registers to write.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status WritePca9555Registers(fm_pcaIoDevice *dev,
                                       fm_pcaIoRegType regType,
                                       fm_uint         startReg,
                                       fm_uint         numBytes)
{
    fm_uint   i;
    fm_byte * dataPtr;
    fm_byte   data[MAX_PCA9555_IO+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    if ( (startReg + numBytes) > MAX_PCA9555_IO)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_OUTPUT:
            ioReg = dev->devCap.numBytes * 0x01;
            dataPtr = dev->cachedRegs.output;
        break;

        case PCA_IO_REG_TYPE_IOC:
            ioReg = dev->devCap.numBytes * 0x03;
            dataPtr = dev->cachedRegs.ioc;
            break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    /* Write to registers one byte at the time */
    for (i = 0 ; i < numBytes ; i++)
    {
        data[0] = ioReg + startReg + i;
        data[1] = dataPtr[i + startReg];

        status = dev->func(dev->fd, dev->addr, data, 2, 0);
        if (status != FM_OK)
        {
            break;
        }
    }

    return status;

}   /* end WritePca9555Registers */




/*****************************************************************************/
/* WritePca9551Registers
 * \ingroup platformUtils
 *
 * \desc            Write the content of the given cached registers to
 *                  the given PCA9551 device.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to write.
 * 
 * \param[in]       startReg first register offset to write.
 * 
 * \param[in]       numBytes number of registers to write.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status WritePca9551Registers(fm_pcaIoDevice *dev,
                                       fm_pcaIoRegType regType,
                                       fm_uint         startReg,
                                       fm_uint         numBytes)
{
    fm_uint   i;
    fm_byte * dataPtr;
    fm_byte   data[MAX_PCA9551_IO+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    if ( (startReg + numBytes) > MAX_PCA9555_IO)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_PSC0:
            ioReg = 0x1;
            dataPtr = &dev->ledRegs.psc0;
            break;

        case PCA_IO_REG_TYPE_PWM0:
            ioReg = 0x2;
            dataPtr = &dev->ledRegs.pwm0;
            break;

        case PCA_IO_REG_TYPE_PSC1:
            ioReg = 0x3;
            dataPtr = &dev->ledRegs.psc1;
            break;

        case PCA_IO_REG_TYPE_PWM1:
            ioReg = 0x4;
            dataPtr = &dev->ledRegs.pwm1;
            break;

        case PCA_IO_REG_TYPE_OUTPUT:
        case PCA_IO_REG_TYPE_LEDOUT:
            ioReg = 0x5;
            dataPtr = &dev->ledRegs.ledout[0];
        break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    if (dev->i2cBlockSupported)
    {
        /* Use auto-increment to write to registers */
        data[0] = (ioReg + startReg) | 0x10;
        for (i = 0 ; i < numBytes ; i++)
        {
            data[1 + i] = dataPtr[i + startReg];
        }
        status = dev->func(dev->fd, dev->addr, data, 1 + numBytes, 0);
    }
    else
    {
        /* Write to registers one byte at the time */
        for (i = 0 ; i < numBytes ; i++)
        {
            data[0] = ioReg + startReg + i;
            data[1] = dataPtr[i + startReg];

            status = dev->func(dev->fd, dev->addr, data, 2, 0);
            if (status != FM_OK)
            {
                break;
            }
        }
    }

    return status;

}   /* end WritePca9551Registers */




/*****************************************************************************/
/* WritePca9634Registers
 * \ingroup platformUtils
 *
 * \desc            Write the content of the given cached registers to
 *                  the given PCA9634/35 device.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to write.
 * 
 * \param[in]       startReg first register offset to write.
 * 
 * \param[in]       numBytes number of registers to write.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status WritePca9634Registers(fm_pcaIoDevice *dev,
                                       fm_pcaIoRegType regType,
                                       fm_uint         startReg,
                                       fm_uint         numBytes)
{
    fm_uint   i;
    fm_uint   maxNumRegs;
    fm_byte * dataPtr;
    fm_byte   data[FM_PCA_LED_MAX_REGS+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    maxNumRegs = (dev->model == PCA_IO_9634) ? FM_PCA9634_NUM_REGS :
                                               FM_PCA9635_NUM_REGS;
    if ( (startReg + numBytes) > maxNumRegs)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_MODE1:
            ioReg = 0x0;
            dataPtr = &dev->ledRegs.mode[0];
            break;

        case PCA_IO_REG_TYPE_MODE2:
            ioReg = 0x1;
            dataPtr = &dev->ledRegs.mode[1];
            break;

        case PCA_IO_REG_TYPE_PWM:
            ioReg = 0x2;
            dataPtr = &dev->ledRegs.pwm[0];
            break;

        case PCA_IO_REG_TYPE_GRPPWM:
            ioReg = (dev->model == PCA_IO_9634) ? 0xA : 0x12;
            dataPtr = &dev->ledRegs.group[0];
            break;

        case PCA_IO_REG_TYPE_GRPFREQ:
            ioReg = (dev->model == PCA_IO_9634) ? 0xB : 0x13;
            dataPtr = &dev->ledRegs.group[1];
            break;

        case PCA_IO_REG_TYPE_LEDOUT:
        case PCA_IO_REG_TYPE_OUTPUT:
            ioReg = (dev->model == PCA_IO_9634) ? 0xC : 0x14;
            dataPtr = &dev->ledRegs.ledout[0];
            break;

        case PCA_IO_REG_TYPE_SUBADDR:
            ioReg = (dev->model == PCA_IO_9634) ? 0xE : 0x18;
            dataPtr = &dev->ledRegs.addr[0];
            break;

        case PCA_IO_REG_TYPE_ALLCALLADR:
            ioReg = (dev->model == PCA_IO_9634) ? 0x11 : 0x1B;
            dataPtr = &dev->ledRegs.addr[3];
            break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    if (dev->i2cBlockSupported)
    {
        /* Use auto-increment to write to registers */
        data[0] = (ioReg + startReg) | FM_PCA_IO_AUTO_INCR;
        for (i = 0 ; i < numBytes ; i++)
        {
            data[1 + i] = dataPtr[i + startReg];
        }
        status = dev->func(dev->fd, dev->addr, data, 1 + numBytes, 0);
    }
    else
    {
        /* Write to registers one byte at the time */
        for (i = 0 ; i < numBytes ; i++)
        {
            data[0] = ioReg + startReg + i;
            data[1] = dataPtr[i + startReg];

            status = dev->func(dev->fd, dev->addr, data, 2, 0);
            if (status != FM_OK)
            {
                break;
            }
        }
    }

    return status;

}   /* end WritePca9634Registers */




/*****************************************************************************/
/* ReadPca9506Registers
 * \ingroup platformUtils
 *
 * \desc            Read the PCA9506 registers and store them in the
 *                  cached registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to read.
 * 
 * \param[in]       startReg first register offset to read.
 * 
 * \param[in]       numBytes number of registers to read.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ReadPca9506Registers(fm_pcaIoDevice *dev,
                                      fm_pcaIoRegType regType,
                                      fm_uint         startReg,
                                      fm_uint         numBytes)
{
    fm_uint   i;
    fm_byte * dataPtr;
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    if ( (startReg + numBytes) > MAX_PCA9506_IO)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_INPUT:
            ioReg = 0x00;
            dataPtr = &dev->cachedRegs.input[startReg];
        break;

        case PCA_IO_REG_TYPE_OUTPUT:
            ioReg = 0x08;
            dataPtr = &dev->cachedRegs.output[startReg];
        break;

        case PCA_IO_REG_TYPE_IOC:
            ioReg = 0x18;
            dataPtr = &dev->cachedRegs.ioc[startReg];
            break;

        case PCA_IO_REG_TYPE_INTR:
            ioReg = 0x20;
            dataPtr = &dev->cachedRegs.intr[startReg];
        break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    if (dev->i2cBlockSupported)
    {
        /* Use auto-increment to read the registers */
        *dataPtr = (ioReg + startReg) | FM_PCA_IO_AUTO_INCR;

        status = dev->func(dev->fd, dev->addr, dataPtr, 1, numBytes);
    }
    else
    {
        /* Read the registers one byte at the time */
        for (i = 0 ; i < numBytes ; i++)
        {
            dataPtr[i] = ioReg + startReg + i;

            status = dev->func(dev->fd, dev->addr, &dataPtr[i], 1, 1);
            if (status != FM_OK)
            {
                break;
            }
        }
    }

    return status;

}   /* end ReadPca9506Registers */




/*****************************************************************************/
/* ReadPca9555Registers
 * \ingroup platformUtils
 *
 * \desc            Read the PCA9554/9555 registers and store them in the
 *                  cached registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to read.
 * 
 * \param[in]       startReg first register offset to read.
 * 
 * \param[in]       numBytes number of registers to read.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ReadPca9555Registers(fm_pcaIoDevice *dev,
                                      fm_pcaIoRegType regType,
                                      fm_uint         startReg,
                                      fm_uint         numBytes)
{
    fm_byte * dataPtr;
    fm_byte   data[MAX_PCA9555_IO+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    if ( (startReg + numBytes) > MAX_PCA9555_IO)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_INPUT:
            ioReg = 0x00;
            dataPtr = &dev->cachedRegs.input[startReg];
        break;

        case PCA_IO_REG_TYPE_OUTPUT:
            ioReg = dev->devCap.numBytes * 0x01;
            dataPtr = &dev->cachedRegs.output[startReg];
        break;

        case PCA_IO_REG_TYPE_IOC:
            ioReg = dev->devCap.numBytes * 0x03;
            dataPtr = &dev->cachedRegs.ioc[startReg];
            break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    *dataPtr = ioReg + startReg;
    status = dev->func(dev->fd, dev->addr, dataPtr, 1, numBytes);

    return status;

}   /* end ReadPca9555Registers */




/*****************************************************************************/
/* ReadPca9551Registers
 * \ingroup platformUtils
 *
 * \desc            Read the PCA9551 registers and store them in the
 *                  cached registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to read.
 * 
 * \param[in]       startReg first register offset to read.
 * 
 * \param[in]       numBytes number of registers to read.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ReadPca9551Registers(fm_pcaIoDevice *dev,
                                      fm_pcaIoRegType regType,
                                      fm_uint         startReg,
                                      fm_uint         numBytes)
{
    fm_uint   i;
    fm_byte * dataPtr;
    fm_byte   data[MAX_PCA9551_IO+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    if ( (startReg + numBytes) > MAX_PCA9551_IO)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_PSC0:
            ioReg = 0x1;
            dataPtr = &dev->ledRegs.psc0;
            break;

        case PCA_IO_REG_TYPE_PWM0:
            ioReg = 0x2;
            dataPtr = &dev->ledRegs.pwm0;
            break;

        case PCA_IO_REG_TYPE_PSC1:
            ioReg = 0x3;
            dataPtr = &dev->ledRegs.psc1;
            break;

        case PCA_IO_REG_TYPE_PWM1:
            ioReg = 0x4;
            dataPtr = &dev->ledRegs.pwm1;
            break;

        case PCA_IO_REG_TYPE_OUTPUT:
        case PCA_IO_REG_TYPE_LEDOUT:
            ioReg = 0x5;
            dataPtr = &dev->ledRegs.ledout[0];
        break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    if (dev->i2cBlockSupported)
    {
        /* Use auto-increment to read the registers */
        dataPtr[0] = (ioReg + startReg) | 0x10;

        status = dev->func(dev->fd, dev->addr, dataPtr, 1, numBytes);
    }
    else
    {
        /* Read the registers one byte at the time */
        for (i = 0 ; i < numBytes ; i++)
        {
            dataPtr[i] = ioReg + startReg + i;

            status = dev->func(dev->fd, dev->addr, &dataPtr[i], 1, 1);
            if (status != FM_OK)
            {
                break;
            }
        }
    }

    return status;

}   /* end ReadPca99551Registers */




/*****************************************************************************/
/* ReadPca9634Registers
 * \ingroup platformUtils
 *
 * \desc            Read the PCA9634/35 registers and store them in the
 *                  cached registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to read.
 * 
 * \param[in]       startReg first register offset to read.
 * 
 * \param[in]       numBytes number of registers to read.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ReadPca9634Registers(fm_pcaIoDevice *dev,
                                      fm_pcaIoRegType regType,
                                      fm_uint         startReg,
                                      fm_uint         numBytes)
{
    fm_uint   i;
    fm_uint   maxNumRegs;
    fm_byte * dataPtr;
    fm_byte   data[FM_PCA_LED_MAX_REGS+1];
    fm_byte   ioReg;
    fm_status status = FM_FAIL;

    maxNumRegs = (dev->model == PCA_IO_9634) ? FM_PCA9634_NUM_REGS :
                                               FM_PCA9635_NUM_REGS;
    if ( (startReg + numBytes) > maxNumRegs)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (regType)
    {
        case PCA_IO_REG_TYPE_MODE1:
            ioReg = 0x0;
            dataPtr = &dev->ledRegs.mode[0];
            break;

        case PCA_IO_REG_TYPE_MODE2:
            ioReg = 0x1;
            dataPtr = &dev->ledRegs.mode[1];
            break;

        case PCA_IO_REG_TYPE_PWM:
            ioReg = 0x2;
            dataPtr = &dev->ledRegs.pwm[0];
            break;

        case PCA_IO_REG_TYPE_GRPPWM:
            ioReg = (dev->model == PCA_IO_9634) ? 0xA : 0x12;
            dataPtr = &dev->ledRegs.group[0];
            break;

        case PCA_IO_REG_TYPE_GRPFREQ:
            ioReg = (dev->model == PCA_IO_9634) ? 0xB : 0x13;
            dataPtr = &dev->ledRegs.group[1];
            break;

        case PCA_IO_REG_TYPE_LEDOUT:
        case PCA_IO_REG_TYPE_OUTPUT:
            ioReg = (dev->model == PCA_IO_9634) ? 0xC : 0x14;
            dataPtr = &dev->ledRegs.ledout[0];
            break;

        case PCA_IO_REG_TYPE_SUBADDR:
            ioReg = (dev->model == PCA_IO_9634) ? 0xE : 0x18;
            dataPtr = &dev->ledRegs.addr[0];
            break;

        case PCA_IO_REG_TYPE_ALLCALLADR:
            ioReg = (dev->model == PCA_IO_9634) ? 0x11 : 0x1B;
            dataPtr = &dev->ledRegs.addr[3];
            break;

        default:
            return FM_ERR_INVALID_ARGUMENT;
    }

    if (dev->i2cBlockSupported)
    {
        /* Use auto-increment to read the registers */
        dataPtr[0] = (ioReg + startReg) | FM_PCA_IO_AUTO_INCR;

        status = dev->func(dev->fd, dev->addr, dataPtr, 1, numBytes);
    }
    else
    {
        /* Read the registers one byte at the time */
        for (i = 0 ; i < numBytes ; i++)
        {
            dataPtr[i] = ioReg + startReg + i;

            status = dev->func(dev->fd, dev->addr, &dataPtr[i], 1, 1);
            if (status != FM_OK)
            {
                break;
            }
        }
    }

    return status;

}   /* end ReadPca9634Registers */




/*****************************************************************************/
/* InitPca9506
 * \ingroup platformUtils
 *
 * \desc            Init PCA9506 IO device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitPca9506(fm_pcaIoDevice *dev)
{
    fm_status status;

    if (dev == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Set I/O Configuration registers */
    status = WritePca9506Registers(dev, 
                                   PCA_IO_REG_TYPE_IOC, 
                                   0, 
                                   MAX_PCA9506_IO);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to IOC reg of PCA9506 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set Output Port registers */
    status = WritePca9506Registers(dev, 
                                   PCA_IO_REG_TYPE_OUTPUT, 
                                   0, 
                                   MAX_PCA9506_IO);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to OP reg of PCA9506 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set Mask Interrupt registers */
    status = WritePca9506Registers(dev, 
                                   PCA_IO_REG_TYPE_INTR, 
                                   0, 
                                   MAX_PCA9506_IO);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to INTR reg of PCA9506 dev 0x%x.\n",
                     dev->addr);
    }

    return FM_OK;

}   /* end InitPca9506 */




/*****************************************************************************/
/* InitPca9555
 * \ingroup platformUtils
 *
 * \desc            Init PCA9554/9555 IO device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitPca9555(fm_pcaIoDevice *dev)
{
    fm_status status;

    /* The PCA9554 provides 8-bit parallel input/output (I/O) port extension
     * organized in 1 banks of 8 I/Os.
     * 
     * The PCA9555 provides 16-bit parallel input/output (I/O) port extension
     * organized in 2 banks of 8 I/Os.
     *
     * Bank0: I/O:0     Bank1: I/O:0
     *        I/O:1            I/O:1
     *        I/O:2            I/O:2
     *        I/O:3            I/O:3
     *        I/O:4            I/O:4
     *        I/O:5            I/O:5
     *        I/O:6            I/O:6
     *        I/O:7            I/O:7
     *
     * The I/O usage is defined in the fm_platform_attrbutes.cfg file.
     */

    /* Set Configuration Port registers */
    status = WritePca9555Registers(dev, 
                                   PCA_IO_REG_TYPE_IOC, 
                                   0, 
                                   dev->devCap.numBytes);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to IOC reg of PCA9554/55 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set Output Port registers */
    status = WritePca9555Registers(dev, 
                                   PCA_IO_REG_TYPE_OUTPUT, 
                                   0, 
                                   dev->devCap.numBytes);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to OP reg of PCA9554/55 dev 0x%x.\n",
                     dev->addr);
    }

    return status;

}   /* end InitPca9555 */




/*****************************************************************************/
/* InitPca9551
 * \ingroup platformUtils
 *
 * \desc            Init PCA9551 device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitPca9551(fm_pcaIoDevice *dev)
{
    fm_status status;

    if (dev == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Set PSC0 register */
    status = WritePca9551Registers(dev, 
                                   PCA_IO_REG_TYPE_PSC0, 
                                   0, 
                                   1);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to PSC0 reg of PCA9551 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set PSC1 register */
    status = WritePca9551Registers(dev, 
                                   PCA_IO_REG_TYPE_PSC1, 
                                   0, 
                                   1);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to PSC1 reg of PCA9551 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set PWM0 register */
    status = WritePca9551Registers(dev, 
                                   PCA_IO_REG_TYPE_PWM0, 
                                   0, 
                                   1);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to PWM0 reg of PCA9551 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set PWM1 register */
    status = WritePca9551Registers(dev, 
                                   PCA_IO_REG_TYPE_PWM1, 
                                   0, 
                                   1);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to PWM1 reg of PCA9551 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set LEDOUT registers */
    status = WritePca9551Registers(dev, 
                                   PCA_IO_REG_TYPE_LEDOUT, 
                                   0, 
                                   2);

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to LEDOUT reg of PCA9551 dev 0x%x.\n",
                     dev->addr);
    }

    return status;

}   /* end InitPca9551 */




/*****************************************************************************/
/* InitPca9634
 * \ingroup platformUtils
 *
 * \desc            Init PCA9634/35 device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitPca9634(fm_pcaIoDevice *dev)
{
    fm_status status;
    fm_uint   i;

    if (dev == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Set GRPPWM and GRPFREQ registers */
    status = WritePca9634Registers(dev, 
                                   PCA_IO_REG_TYPE_GRPPWM, 
                                   0, 
                                   2);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to GROUP regs of PCA9634 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set subaddresses and all call address registers */
    status = WritePca9634Registers(dev, 
                                   PCA_IO_REG_TYPE_SUBADDR, 
                                   0, 
                                   4);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to SUBADR regs of PCA9634 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set PWM registers. The FM10000 I2C master is limited to 12 bytes per
     * transaction, o split it into 8-bytes transaction. */
    for ( i = 0 ; i < dev->devCap.numBits ; i += 8 )
    {
        status = WritePca9634Registers(dev, 
                                       PCA_IO_REG_TYPE_PWM, 
                                       i, 
                                       8);
        if (status != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to write to PWM regs of PCA9634 dev 0x%x.\n",
                         dev->addr);
        }
    }

    /* Set LEDOUT registers */
    status = WritePca9634Registers(dev, 
                                   PCA_IO_REG_TYPE_LEDOUT, 
                                   0, 
                                   dev->devCap.numBytes);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to LEDOUT regs of PCA9634 dev 0x%x.\n",
                     dev->addr);
    }

    /* Set MODE1/2 registers */
    status = WritePca9634Registers(dev, 
                                   PCA_IO_REG_TYPE_MODE1, 
                                   0, 
                                   2);
    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to write to MODE1/2 regs of PCA9634 dev 0x%x.\n",
                     dev->addr);
    }

    return FM_OK;

}   /* end InitPca9634 */




/*****************************************************************************/
/* DumpPca9506
 * \ingroup platformUtils
 *
 * \desc            Dump PCA9506 device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DumpPca9506(fm_pcaIoDevice *dev)
{
    fm_status     status;
    fm_int        i;
    fm_int        numBytes;
    fm_int        numLoop;
    fm_byte       ip[MAX_PCA9506_IO + 1];
    fm_byte       op[MAX_PCA9506_IO + 1];
    fm_byte       pi[MAX_PCA9506_IO + 1];
    fm_byte       ioc[MAX_PCA9506_IO + 1];
    fm_byte       msk[MAX_PCA9506_IO + 1];
    fm_uintptr    fd;
    fm_int        addr;
    fm_pcaIoRegs *cachedRegs;
    fm_utilI2cWriteReadHdnlFunc func;

    fd   = dev->fd;
    func = dev->func;
    addr = dev->addr;
    cachedRegs = &dev->cachedRegs;

    if (dev->i2cBlockSupported)
    {
        numBytes = MAX_PCA9506_IO;
        numLoop = 1;
    }
    else
    {
        numBytes = 1;
        numLoop = MAX_PCA9506_IO;
    }

    for (i = 0 ; i < numLoop ; i++)
    {
        /* Read Input Port registers */
        ip[i] = (0x00+i) | FM_PCA_IO_AUTO_INCR;
        status = func(fd, addr, &ip[i], 1, numBytes);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Read Output Port registers */
        op[i] = (0x08+i) | FM_PCA_IO_AUTO_INCR;
        status = func(fd, addr, &op[i], 1, numBytes);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Read Polarity Inversion registers */
        pi[i] = (0x10+i) | FM_PCA_IO_AUTO_INCR;
        status = func(fd, addr, &pi[i], 1, numBytes);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Read I/O Configuration registers */
        ioc[i] = (0x18+i) | FM_PCA_IO_AUTO_INCR;
        status = func(fd, addr, &ioc[i], 1, numBytes);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        /* Read Mask Interrupt registers */
        msk[i] = (0x20+i) | FM_PCA_IO_AUTO_INCR;
        status = func(fd, addr, &msk[i], 1, numBytes);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    numBytes = MAX_PCA9506_IO;

    /* Input Port registers */
    FM_LOG_PRINT("IP[0x00]  : ");

    for (i = (numBytes - 1) ; i >= 0 ; i--)
    {
        FM_LOG_PRINT("%02x ", ip[i]);
    }
    FM_LOG_PRINT(" <= LSB\n");

    if (cachedRegs)
    {
        FM_LOG_PRINT("IP CACHED : ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->input[i]);
        }
        FM_LOG_PRINT("\n");
    }

    /* Output Port registers */
    FM_LOG_PRINT("OP[0x08]  : ");

    for (i = (numBytes - 1) ; i >= 0 ; i--)
    {
        FM_LOG_PRINT("%02x ", op[i]);
    }
    FM_LOG_PRINT("\n");

    if (cachedRegs)
    {
        FM_LOG_PRINT("OP CACHED : ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->output[i]);
        }
        FM_LOG_PRINT("\n");
    }

    /* I/O Configuration registers */
    FM_LOG_PRINT("IOC[0x18] : ");

    for (i = (numBytes - 1) ; i >= 0 ; i--)
    {
        FM_LOG_PRINT("%02x ", ioc[i]);
    }
    FM_LOG_PRINT("\n");

    if (cachedRegs)
    {
        FM_LOG_PRINT("IOC CACHED: ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->ioc[i]);
        }
    }
    FM_LOG_PRINT("\n");

    /* mask interrupts */
    FM_LOG_PRINT("INT[0x20] : ");

    for (i = (numBytes - 1) ; i >= 0 ; i--)
    {
        FM_LOG_PRINT("%02x ", msk[i]);
    }
    FM_LOG_PRINT("\n");

    if (cachedRegs)
    {
        FM_LOG_PRINT("INT CACHED: ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->intr[i]);
        }
    }
    FM_LOG_PRINT("\n");

    /* Polarity Inversion */
    FM_LOG_PRINT("PI[0x10]  : ");

    for (i = (numBytes - 1) ; i >= 0 ; i--)
    {
        FM_LOG_PRINT("%02x ", pi[i]);
    }
    FM_LOG_PRINT("\n");

ABORT:
    return status;

}   /* end DumpPca9506 */




/*****************************************************************************/
/* DumpPca9555
 * \ingroup platformUtils
 *
 * \desc            Dump PCA9555 device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status DumpPca9555(fm_pcaIoDevice *dev)
{
    fm_status     status;
    fm_byte       data[FM_PCA_IO_REG_MAX_SIZE+1];
    fm_int        i;
    fm_int        numBytes;
    fm_uintptr    fd;
    fm_int        addr;
    fm_pcaIoRegs *cachedRegs;
    fm_pcaIoCap * devCap;
    fm_utilI2cWriteReadHdnlFunc func;

    if (dev == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    fd     = dev->fd;
    func   = dev->func;
    addr   = dev->addr;
    devCap = &dev->devCap;
    cachedRegs = &dev->cachedRegs;

    numBytes = dev->devCap.numBytes;

    /* Input Port registers */
    data[0] = 0x00;

    status = func(fd, addr, data, 1, numBytes);

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read IP regs of PCA9554/55 device 0x%x.\n",
                     addr);
    }
    else
    {
        FM_LOG_PRINT("IP[0x00]  : ");

        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", data[i]);
        }
        FM_LOG_PRINT(" <= LSB\n");
    }

    if (cachedRegs)
    {
        FM_LOG_PRINT("IP CACHED : ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->input[i]);
        }
        FM_LOG_PRINT("\n");
    }

    /* Output Port registers */
    data[0] = numBytes * 0x01;

    status = func(fd, addr, data, 1, numBytes);

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read OP regs of PCA9554/55 device 0x%x.\n",
                     addr);
    }
    else
    {
        FM_LOG_PRINT("OP[0x%02x]  : ", numBytes * 0x01);

        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", data[i]);
        }
        FM_LOG_PRINT("\n");
    }

    if (cachedRegs)
    {
        FM_LOG_PRINT("OP CACHED : ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->output[i]);
        }
        FM_LOG_PRINT("\n");
    }

    /* I/O Configuration registers */
    data[0] = numBytes * 0x03;

    status = func(fd, addr, data, 1, numBytes);

    if (status != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Unable to read IOC regs of PCA9554/55 device 0x%x.\n",
                     addr);
    }
    else
    {
        FM_LOG_PRINT("IOC[0x%02x] : ", numBytes * 0x03);

        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", data[i]);
        }
        FM_LOG_PRINT("\n");
    }

    if (cachedRegs)
    {
        FM_LOG_PRINT("IOC CACHED: ");
        for (i = (numBytes - 1) ; i >= 0 ; i--)
        {
            FM_LOG_PRINT("%02x ", cachedRegs->ioc[i]);
        }
    }
    FM_LOG_PRINT("\n");


    return FM_OK;

}   /* end DumpPca9555 */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/



/*****************************************************************************/
/* fmUtilPcaIoGetCap
 * \ingroup platformUtils
 *
 * \desc            Return capabilities for specified PCA IO model.
 *
 * \param[in]       model is the PCA model as defined in fm_pcaIoModel.
 *
 * \param[in]       devCap is the pointer to the fm_pcaIoCap where the 
 *                  function will set the model capabilities.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPcaIoGetCap(fm_pcaIoModel model, fm_pcaIoCap *devCap)
{

    if (devCap == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switch (model)
    {
        case PCA_IO_9538:
            devCap->numBits = 8;
            devCap->numBytes = 1;
            devCap->cap = FM_PCA_IO_CAP_INTR;
        break;

        case PCA_IO_9539:
            devCap->numBits = 16;
            devCap->numBytes = 2;
            devCap->cap = FM_PCA_IO_CAP_INTR;
        break;

        case PCA_IO_9554:
            devCap->numBits = 8;
            devCap->numBytes = MAX_PCA9554_IO;
            devCap->cap = 0;
        break;

        case PCA_IO_9555:
            devCap->numBits = 16;
            devCap->numBytes = MAX_PCA9555_IO;
            devCap->cap = 0;
        break;

        case PCA_IO_9505:
        case PCA_IO_9506:
        case PCA_IO_9698:
            devCap->numBits = 40;
            devCap->numBytes = MAX_PCA9506_IO;
            devCap->cap = FM_PCA_IO_CAP_INTR;
        break;

        case PCA_IO_9551:
            /* Number of LED pins */
            devCap->numBits = 8;

            /* Number of LEDOUT registers */
            devCap->numBytes = MAX_PCA9551_IO;
        break;

        case PCA_IO_9634:
            /* Number of LED pins */
            devCap->numBits = FM_PCA9634_NUM_LEDS;

            /* Number of LEDOUT registers */
            devCap->numBytes = FM_PCA9634_NUM_LEDS / FM_PCA_LED_PER_LEDOUT;
        break;

        case PCA_IO_9635:
            /* Number of LED pins */
            devCap->numBits = FM_PCA9635_NUM_LEDS;

            /* Number of LEDOUT registers */
            devCap->numBytes = FM_PCA9635_NUM_LEDS / FM_PCA_LED_PER_LEDOUT;
        break;

        case PCA_IO_UNKNOWN:
            devCap->numBits = 0;
            devCap->cap = 0;
        break;
    }

    return FM_OK;

}   /* end fmUtilPcaIoGetCap */




/*****************************************************************************/
/* fmUtilPcaIoInit
 * \ingroup platformUtils
 *
 * \desc            Init PCA IO device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPcaIoInit(fm_pcaIoDevice *dev)
{
    fm_status status;

    switch (dev->model)
    {
        case PCA_IO_9505:
        case PCA_IO_9506:
        case PCA_IO_9698:
            status = InitPca9506(dev);
            break;

        case PCA_IO_9554:
        case PCA_IO_9555:
        case PCA_IO_9538:
        case PCA_IO_9539:
            status = InitPca9555(dev);
            break;

        case PCA_IO_9551:
            status = InitPca9551(dev);
            break;

        case PCA_IO_9634:
        case PCA_IO_9635:
            status = InitPca9634(dev);
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    return status;

}   /* end fmUtilPcaIoInit */




/*****************************************************************************/
/* fmUtilPcaIoDump
 * \ingroup platformUtils
 *
 * \desc            Dump PCA IO device registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPcaIoDump(fm_pcaIoDevice *dev)
{
    fm_status status;

    switch (dev->model)
    {
        case PCA_IO_9505:
        case PCA_IO_9506:
        case PCA_IO_9698:
            status = DumpPca9506(dev);
            break;

        case PCA_IO_9554:
        case PCA_IO_9555:
        case PCA_IO_9538:
        case PCA_IO_9539:
            status = DumpPca9555(dev);
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    return status;

}   /* end fmUtilPcaIoDump */




/*****************************************************************************/
/* fmUtilPcaIoUpdateInputRegs
 * \ingroup platformUtils
 *
 * \desc            Update cached registers from hardware.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPcaIoUpdateInputRegs(fm_pcaIoDevice *dev)
{
    fm_status status;

    switch (dev->model)
    {
        case PCA_IO_9505:
        case PCA_IO_9506:
        case PCA_IO_9698:
            status = ReadPca9506Registers(dev, 
                                          PCA_IO_REG_TYPE_INPUT, 
                                          0, 
                                          MAX_PCA9506_IO);
            break;

        case PCA_IO_9554:
        case PCA_IO_9555:
        case PCA_IO_9538:
        case PCA_IO_9539:
            status = ReadPca9555Registers(dev, 
                                          PCA_IO_REG_TYPE_INPUT, 
                                          0, 
                                          dev->devCap.numBytes);

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    return status;

}   /* end fmUtilPcaIoUpdateInputRegs */




/*****************************************************************************/
/* fmUtilPcaIoWriteRegs
 * \ingroup platformUtils
 *
 * \desc            Write the content of the given cached registers to
 *                  the given PCA device.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to write.
 * 
 * \param[in]       startReg first register offset to write.
 * 
 * \param[in]       numBytes number of registers to write.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPcaIoWriteRegs(fm_pcaIoDevice *dev,
                               fm_pcaIoRegType regType,
                               fm_uint         startReg,
                               fm_uint         numBytes)
{
    fm_status status;

    switch (dev->model)
    {
        case PCA_IO_9505:
        case PCA_IO_9506:
        case PCA_IO_9698:
            status = WritePca9506Registers(dev, regType, startReg, numBytes);
            break;

        case PCA_IO_9554:
        case PCA_IO_9555:
        case PCA_IO_9538:
        case PCA_IO_9539:
            status = WritePca9555Registers(dev, regType, startReg, numBytes);
            break;

        case PCA_IO_9551:
            status = WritePca9551Registers(dev, regType, startReg, numBytes);
            break;

        case PCA_IO_9634:
        case PCA_IO_9635:
            status = WritePca9634Registers(dev, regType, startReg, numBytes);
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    return status;

}   /* end fmUtilPcaIoWriteRegs */



/*****************************************************************************/
/* fmUtilPcaIoReadRegs
 * \ingroup platformUtils
 *
 * \desc            Read the given PCA device registers and store it in the
 *                  cached registers.
 *
 * \param[in]       dev pointer to fm_pcaIoDevice structure.
 * 
 * \param[in]       regType type of registers to read.
 * 
 * \param[in]       startReg first register offset to read.
 * 
 * \param[in]       numBytes number of registers to read.
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPcaIoReadRegs(fm_pcaIoDevice  *dev,
                              fm_pcaIoRegType  regType,
                              fm_uint          startReg,
                              fm_uint          numBytes)
{
    fm_status status;

    switch (dev->model)
    {
        case PCA_IO_9505:
        case PCA_IO_9506:
        case PCA_IO_9698:
            status = ReadPca9506Registers(dev, regType, startReg, numBytes);
            break;

        case PCA_IO_9554:
        case PCA_IO_9555:
        case PCA_IO_9538:
        case PCA_IO_9539:
            status = ReadPca9555Registers(dev, regType, startReg, numBytes);
            break;

        case PCA_IO_9551:
            status = ReadPca9551Registers(dev, regType, startReg, numBytes);
            break;

        case PCA_IO_9634:
        case PCA_IO_9635:
            status = ReadPca9634Registers(dev, regType, startReg, numBytes);
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    return status;

}   /* end fmUtilPcaIoReadRegs */



/*****************************************************************************/
/* fmUtilPca9541TakeBusControl
 * \ingroup platformUtils
 *
 * \desc            Determine if the master device has control of the i2c bus,
 *                  if not then it performs the appropriate write to Control
 *                  register to take control of the i2c bus.
 *
 * \param[in]       handle is the handle to the device.
 * 
 * \param[in]       func is the function pointer to access i2c registers.
 * 
 * \param[in]       addr is the device i2c address
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmUtilPca9541TakeBusControl(fm_uintptr                  handle, 
                                      fm_utilI2cWriteReadHdnlFunc func,
                                      fm_uint                     addr)
{
    fm_status status;
    fm_byte   data[2];
    fm_int    nextVal;
    fm_int    loop;

    /* Table 7, from PCA9541 datasheet */
    static fm_int busCtrlSeq[16] = {4,4,5,5,-1,4,5,-1,-1,0,1,-1,0,0,1,1};

    /* Arbitrary number */
    loop = 8;
    while (loop--)
    {
        /* Read the current status of the i2c bus from the Control Register */
        data[0] = 0x1;
        status = func(handle, addr, data, 1, 1);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

       
        /* Get the next byte to write to take bus control */

        nextVal = busCtrlSeq[ (data[0] & 0xf) ];

        if (nextVal == -1)
        {
            /* Has control */
            return FM_OK;
        }

        /* Write to Control register to take control of the bus*/
        data[0] = 0x1;
        data[1] = nextVal;
        status = func(handle, addr, data, 2, 0);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    FM_LOG_PRINT("Fail to take bus control on 9541 (addr=0x%x)\n", addr);
    
    return FM_FAIL;

}   /* end fmUtilPca9541TakeBusControl */
