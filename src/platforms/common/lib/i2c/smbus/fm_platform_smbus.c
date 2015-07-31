/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_platform_smbus.c
 * Creation Date:   June 3, 2013
 * Description:     SMBUS functions.
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

#include <fm_sdk.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>
#ifdef I2C_SMBUS
#include <linux/i2c.h>
#else
/* Temporary fix for compile error on some dev platforms */
#define I2C_SLAVE                   0x703
#define I2C_SMBUS                   0x720
#define I2C_FUNCS                   0x705  /* Get the adapter functionality */

#define I2C_FUNC_SMBUS_READ_I2C_BLOCK    0x04000000 /* I2C-like block xfer  */
#define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK   0x08000000 /* w/ 1-byte reg. addr. */

#define I2C_FUNC_SMBUS_I2C_BLOCK         (I2C_FUNC_SMBUS_READ_I2C_BLOCK | \
                                          I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)

#define I2C_SMBUS_WRITE             0
#define I2C_SMBUS_READ              1
#define I2C_SMBUS_BYTE              1
#define I2C_SMBUS_BYTE_DATA         2
#define I2C_SMBUS_WORD_DATA         3
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7		/* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

#define I2C_SMBUS_BLOCK_MAX         32      /* As specified in SMBus standard */

union i2c_smbus_data
{
    __u8  byte;
    __u16 word;
    __u8  block[I2C_SMBUS_BLOCK_MAX + 2];    /* block[0] is used for length */
    /* and one more for user-space compatibility */

};
#endif

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
/* I2CSmbusAccess
 * \ingroup intPlatformSMBus
 *
 * \desc            Perform SMBUS IOCTL access to the kernel.
 *
 * \param[in]       fd is the file handle to the SMBUS device.
 *
 * \param[in]       readWrite specifies whether to read or write.
 *
 * \param[in]       command is the SMBUS command byte.
 *
 * \param[in]       size if the SMBUS size to read or write.
 *
 * \param[in,out]   data is the pointer to the i2c_smbus_data structure.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static inline __s32 I2CSmbusAccess(int fd, fm_char readWrite, fm_byte command,
                                   fm_int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;

    args.read_write = readWrite;
    args.command    = command;
    args.size       = size;
    args.data       = data;
    return ioctl(fd, I2C_SMBUS, &args);

} /* I2CSmbusAccess */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformSMBusI2cInit
 * \ingroup intPlatformSMBus
 *
 * \desc            Open the SMBUS I2C device to access the platform peripheral.
 *
 * \param[in]       devName is the SMBUS device name to open.
 *
 * \param[out]      i2cFd points to caller allocated storage where the file
 *                  descriptor of the I2C device will be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSMBusI2cInit(fm_text devName, int *i2cFd)
{
    int     fd;
    char    strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t strErrNum;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "devName = %s\n", devName);

    *i2cFd = -1;

    fd = open(devName, O_RDWR);

    if (fd < 0)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);

        if (strErrNum)
        {
            FM_SNPRINTF_S(strErrBuf, FM_STRERROR_BUF_SIZE, "%d", errno);
        }

        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                     "%s: Unable to open '%s'.\n",
                     strErrBuf,
                     devName);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_FAIL);
    }

    *i2cFd = fd;

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformSMBusI2cInit */





/*****************************************************************************/
/* fmPlatformSMBusIsI2cBlockSupported
 * \ingroup intPlatformSMBus
 *
 * \desc            Indicates whether the I2C_SMBUS_I2C_BLOCK_DATA transaction
 *                  type is supported by the linux SMBus driver associated
 *                  to the given file descriptor.
 *
 * \param[in]       fd is the file descriptor to the SMbus device.
 *
 * \param[out]      i2cSupport points to caller allocated storage where the
 *                  support status will be written. 
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSMBusIsI2cBlockSupported(int fd, int *i2cSupport)
{
    unsigned long funcs;
    fm_status     status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "fd = %d\n", fd);

    if (fd <= 0 || i2cSupport == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    /* Default to not supported */
    *i2cSupport = 0;

    if (ioctl(fd, I2C_FUNCS, &funcs) < 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    if ((funcs & I2C_FUNC_SMBUS_I2C_BLOCK) == I2C_FUNC_SMBUS_I2C_BLOCK)
    {
        /* Indicate I2C_FUNC_SMBUS_I2C_BLOCK is supported */
        *i2cSupport = 1;
    }

    status = FM_OK;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSMBusIsI2cBlockSupported */




/*****************************************************************************/
/* fmPlatformSMBusI2cWriteRead
 * \ingroup intPlatformSMBus
 *
 * \desc            Write to then immediately read from an SMBUS device using
 *                  SMBUS interface.
 *
 * \param[in]       fd is the file descriptor to the USB COM device.
 *
 * \param[in]       device is the SMBUS device address (0x00 - 0x7F).
 *
 * \param[in,out]   data points to an array from which data is written and
 *                  into which data is read.
 *
 * \param[in]       wl is the number of bytes to write.
 *
 * \param[in]       rl is the number of bytes to read.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSMBusI2cWriteRead(int      fd,
                                      fm_int   device,
                                      fm_byte *data,
                                      fm_int   wl,
                                      fm_int   rl)
{
    int                  ret;
    fm_int               cnt;
    fm_status            status = FM_OK;
    union i2c_smbus_data smbusData;
    fm_int               len;
    fm_int               off;

    if (fd <= 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    /* This code is process safe. Two processes can access different devices
     * on the same I2C bus without requiring locking */
    if (ioctl(fd, I2C_SLAVE, device) < 0)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    if (wl == 0 && rl == 1)
    {
        if ( I2CSmbusAccess(fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &smbusData) )
        {
            status = FM_ERR_I2C_NO_RESPONSE;
        }
        else
        {
            data[0] = 0x0FF & smbusData.byte;
        }
    }
    else if (rl == 0)
    {
        if (wl == 1)
        {
            if ( ( ret = I2CSmbusAccess(fd, I2C_SMBUS_WRITE, data[0],
                                        I2C_SMBUS_BYTE, NULL) ) < 0 )
            {
                status = FM_ERR_I2C_NO_RESPONSE;
            }
        }
        else if (wl == 2)
        {
            smbusData.byte = data[1];

            if ( ( ret = I2CSmbusAccess(fd, I2C_SMBUS_WRITE, data[0],
                                        I2C_SMBUS_BYTE_DATA, &smbusData) ) < 0 )
            {
                status = FM_ERR_I2C_NO_RESPONSE;
            }
        }
        else if (wl == 3)
        {
            smbusData.word = (data[2] << 8) | data[1];

            if ( ( ret = I2CSmbusAccess(fd, I2C_SMBUS_WRITE, data[0],
                                        I2C_SMBUS_WORD_DATA, &smbusData) ) < 0 )
            {
                status = FM_ERR_I2C_NO_RESPONSE;
            }
        }
        else if ( wl <= (I2C_SMBUS_BLOCK_MAX + 1) )
        {
            smbusData.block[0] = wl - 1;

            for (cnt = 1 ; cnt <= smbusData.block[0] ; cnt++)
            {
                smbusData.block[cnt] = data[cnt];
            }

            if ( I2CSmbusAccess(fd, I2C_SMBUS_WRITE, data[0],
                                I2C_SMBUS_I2C_BLOCK_DATA,
                                &smbusData) )
            {
                status = FM_ERR_I2C_NO_RESPONSE;
            }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "SmbusWriteRead wl %d rl %d not supported\n", wl, rl);
            status = FM_ERR_INVALID_ARGUMENT;
        }
    }
    else if (wl == 1 && rl == 1)
    {
        if ( I2CSmbusAccess(fd, I2C_SMBUS_READ, data[0],
                            I2C_SMBUS_BYTE_DATA,
                            &smbusData) )
        {
            status = FM_ERR_I2C_NO_RESPONSE;
        }
        else
        {
            data[0] = smbusData.byte & 0xFF;
        }
    }
    else if (wl == 1 && rl == 2)
    {
        if ( I2CSmbusAccess(fd, I2C_SMBUS_READ, data[0],
                            I2C_SMBUS_WORD_DATA,
                            &smbusData) )
        {
            status = FM_ERR_I2C_NO_RESPONSE;
        }
        else
        {
            data[0] = smbusData.word & 0xFF;
            data[1] = (smbusData.word >> 8) & 0xFF;
        }
    }
    else if (wl == 1 && rl > 0)
    {
        len = 0;
        off = data[0];

        /* For EEPROM read, need to split into 32 chunks
         * The rest should always be less than 32 bytes read */
        while (len < rl)
        {
            smbusData.block[0] = rl - len;
            if ( I2CSmbusAccess(fd, I2C_SMBUS_READ, off + len,
                                I2C_SMBUS_I2C_BLOCK_DATA,
                                &smbusData) )
            {
                status = FM_ERR_I2C_NO_RESPONSE;
                break;
            }
            else if (smbusData.block[0] <= 0)
            {
                /* Avoid possible infinite loop */
                status = FM_ERR_I2C_NO_RESPONSE;
                break;
            }
            else
            {
                for (cnt = 1 ; cnt <= smbusData.block[0] ; cnt++)
                {
                    data[len++] = smbusData.block[cnt];

                    if (len >= rl)
                    {
                        break;
                    }
                }
            }
        }
    }
    else if (wl == 3 && rl == 4)
    {
        /* Reading FM6000 register requires a write of 3 bytes for address and read back
         * of 4 bytes for value. However there is no such SMBUS command defined.
         * A method is to write 3 bytes for address, followed by a write read with writing of
         * 1 byte of zero. FM6000 will ignore the 1 byte write and return the 4 bytes register value */
        status = fmPlatformSMBusI2cWriteRead(fd, device, data, 3, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        data[0] = 0;
        status  = fmPlatformSMBusI2cWriteRead(fd, device, data, 1, rl);
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM, "SmbusWriteRead wl %d rl %d not supported\n", wl, rl);
        /* Need to add support here if feature is required */
        status = FM_ERR_UNSUPPORTED;
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSMBusI2cWriteRead */
