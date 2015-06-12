/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_util_spi.c
 * Creation Date:   June 4, 2013
 * Description:     Functions for using RRC to access SPI flash.
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include <fm_std.h>
#include <common/fm_common.h>
#include <fm_alos_logging.h>
#include <api/internal/fm10000/fm10000_api_regs_int.h>

#include <platforms/util/fm_util.h>
#include <platforms/util/fm10000/fm10000_util_spi.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_int debug = 0;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/



/*****************************************************************************/
/* GetTimeIntervalMsec
 * \ingroup intPlatform
 *
 * \desc            Return the time interval between two timestamps.
 *
 * \param[in]       begin is the begin timestamp.
 *
 * \param[in]       end is the end timestamp. If NULL, use the current timestamp
 *
 * \return          Number of milliseconds.
 *
 *****************************************************************************/
static fm_uint GetTimeIntervalMsec(struct timeval *begin, struct timeval *end)
{
    struct timeval endT;
    struct timeval diff;

    if (end == NULL)
    {
        gettimeofday(&endT, NULL);
    }
    else
    {
        endT.tv_sec = end->tv_sec;
        endT.tv_usec = end->tv_usec;
    }

    diff.tv_sec  = endT.tv_sec - begin->tv_sec;
    diff.tv_usec = endT.tv_usec - begin->tv_usec;
    
    return (diff.tv_sec * 1000 + diff.tv_usec / 1000);

}   /* end GetTimeIntervalMsec */



/*****************************************************************************/
/* SpiEnable
 * \ingroup intPlatform
 *
 * \desc            Enable Switch SPI interface.
 *
 *
 *****************************************************************************/
static fm_status SpiEnable(fm_uintptr             handle,
                           fm_utilRegRead32Func   readFunc,
                           fm_utilRegWrite32Func  writeFunc)
{
    fm_status status;
    fm_uint32 spiCtrl = 0;

    status = readFunc(handle, FM10000_SPI_CTRL(), &spiCtrl);

    /* keep current freq setting and set SPI Enable */
    spiCtrl &= 0x3ff;
    spiCtrl |= 1<<10;
    status = writeFunc(handle, FM10000_SPI_CTRL(), spiCtrl);

    return status;
    
} /* SpiEnable */



/*****************************************************************************/
/* SpiDisable
 * \ingroup intPlatform
 *
 * \desc            Disable Switch SPI interface.
 *
 *
 *****************************************************************************/
static fm_status SpiDisable(fm_uintptr             handle,
                     fm_utilRegRead32Func   readFunc,
                     fm_utilRegWrite32Func  writeFunc)
{
    fm_status status;
    fm_uint32 spiCtrl = 0;

    status = readFunc(handle, FM10000_SPI_CTRL(), &spiCtrl);

    /* keep current freq setting and set SPI Enable */
    spiCtrl &= 0x3ff;
    status = writeFunc(handle, FM10000_SPI_CTRL(), spiCtrl);

    return status;
    
} /* SpiDisable */



/*****************************************************************************/
/* SetSpiCtrlReg
 * \ingroup intPlatform
 *
 * \desc         Writes SPI_CTRL with value and wait until the operation is
 *               completed, then writes again SPI_CTRL setting command = 0.
 *               This is required because SPI_CTRL is an idempotent register.
 *
 *
 *****************************************************************************/
static fm_status SetSpiCtrlReg(fm_uintptr             handle,
                               fm_utilRegRead32Func   readFunc,
                               fm_utilRegWrite32Func  writeFunc,
                               fm_uint32              value)

{
    fm_status status;
    fm_uint32 spiCtrl;
    struct timeval startTime;
    fm_bool   isTimeout;

    status = writeFunc(handle, FM10000_SPI_CTRL(), value);
    if (status) return status;

    gettimeofday(&startTime, NULL); 
    isTimeout = FALSE;
    do
    {
        if (isTimeout)
        {
            printf("ERROR: Timeout waiting for SPI_CTRL.Busy=0. 0x%02x\n", spiCtrl);
            return FM_FAIL;
        }
        
        if (GetTimeIntervalMsec(&startTime, NULL) > 50)
        {
            isTimeout = TRUE;
        }
        
        status = readFunc(handle, FM10000_SPI_CTRL(), &spiCtrl);
        
        if (status) break;        
    } while (FM_GET_BIT(spiCtrl, FM10000_SPI_CTRL, Busy));

    /* write back SPI_CTRL with command = 0 */
    spiCtrl &= 0xffff87ff;
    status = writeFunc(handle, FM10000_SPI_CTRL(), spiCtrl);

    return status;

} /* SetSpiCtrlReg */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fm10000UtilSpiDoOpCode
 * \ingroup intPlatform
 *
 * \desc            Perform SPI commands.
 *
 *
 *****************************************************************************/
fm_status fm10000UtilSpiDoOpCode(fm_uintptr             handle,
                                 fm_utilRegRead32Func   readFunc,
                                 fm_utilRegWrite32Func  writeFunc,
                                 fm_byte               *bytes,
                                 fm_uint                numWrite,
                                 fm_uint                numRead)
{
    fm_status status = FM_OK;
    fm_uint32 spiCtrl;
    fm_uint32 data;
    fm_uint   cnt;
    fm_uint   opCode;
    fm_uint   opCodeSize;

    if (numWrite > 4)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }
    if (numRead > 4)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    status = SpiEnable(handle, readFunc, writeFunc);
    if (status) return status;

    status = readFunc(handle, FM10000_SPI_CTRL(), &spiCtrl);
    if (status) return status;

    spiCtrl &= 0x7ff;

    opCodeSize = numWrite;
    opCode = 0;
    for (cnt = 0; cnt < opCodeSize; cnt++)
    {
        opCode |= (bytes[opCodeSize - cnt - 1] << (cnt*8));
    }
    status = writeFunc(handle, FM10000_SPI_HEADER(), opCode);
    if (status) return status;

    FM_SET_FIELD(spiCtrl, FM10000_SPI_CTRL, Command, numRead?0x1:0x9);
    FM_SET_FIELD(spiCtrl, FM10000_SPI_CTRL, HeaderSize, opCodeSize);
    status = SetSpiCtrlReg(handle, readFunc, writeFunc, spiCtrl);
    if (status) return status;

    if (debug & 1)
    {
          printf("SPI HDR 0x%x CTRL 0x%08x => CM:%d HS:%d DS:%d DSM:%d Busy:%d\n",
               opCode, spiCtrl,
               FM_GET_FIELD(spiCtrl, FM10000_SPI_CTRL, Command),
               FM_GET_FIELD(spiCtrl, FM10000_SPI_CTRL, HeaderSize),
               FM_GET_FIELD(spiCtrl, FM10000_SPI_CTRL, DataSize),
               FM_GET_FIELD(spiCtrl, FM10000_SPI_CTRL, DataShiftMethod),
               FM_GET_BIT(spiCtrl, FM10000_SPI_CTRL, Busy));
    }

    if (numRead)
    {
        spiCtrl &= 0x7ff;
        FM_SET_FIELD(spiCtrl, FM10000_SPI_CTRL, Command, 0xC);
        status = SetSpiCtrlReg(handle, readFunc, writeFunc, spiCtrl);
        if (status) return status;

        status = readFunc(handle, FM10000_SPI_RX_DATA(), &data);
        if (status) return status;

        if (debug & 1) printf("SPI CTRL 0x%08x DATA 0x%08x\n", spiCtrl, data);

        if (status == FM_OK)
        {
            for (cnt = 0 ; cnt < numRead ; cnt++)
            {
                bytes[cnt] = (data >> (8*(3-cnt)));
            }
        }
    }

    status = SpiDisable(handle, readFunc, writeFunc);

    return status;

} /* fm10000UtilSpiDoOpCode */



/*****************************************************************************/
/* fm10000UtilSpiReadFlash
 * \ingroup intPlatform
 *
 * \desc            Read number of bytes from the SPI flash.
 *
 *
 *****************************************************************************/
fm_status fm10000UtilSpiReadFlash(fm_uintptr             handle,
                                  fm_utilRegRead32Func   readFunc,
                                  fm_utilRegWrite32Func  writeFunc,
                                  fm_uint                address,
                                  fm_byte               *data,
                                  fm_int                 len,
                                  fm_int                 freqKhz)
{
    fm_status status = FM_OK;

    fm_int    cnt;
    fm_int    freq;
    fm_int    numRead;

    fm_uint32 rxData;
    fm_uint32 spiCtrl;
    fm_uint32 header;

    if (len <= 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    status = SpiEnable(handle, readFunc, writeFunc);
    if (status) return status;

    status = readFunc(handle, FM10000_SPI_CTRL(), &spiCtrl);
    if (status) return status;

    if (freqKhz > 0)
    {
        freq = ((100000/freqKhz)/2)-1;
        if (freq < 0)
        {
            freq = 0;
        }
        FM_SET_FIELD(spiCtrl, FM10000_SPI_CTRL, Freq, freq);
    }

    spiCtrl &= 0x7ff;

    /* header: command (1 byte: READ_BYTES) + address (3 bytes) */
    header = (0x3 << 24) | (address & 0xffffff);
    status = writeFunc(handle, FM10000_SPI_HEADER(), header);
    if (status) return status;

    if (debug & 1) printf("HDR 0x%08x CTRL 0x%08x\n", header, spiCtrl);

    /* first loop only: set send header flag. Following loops: shift only data. */
    spiCtrl |= (0x1 << 11);

    cnt = 0;
    while (cnt < len)
    {
        /* determine the number of data bytes to read [1..4] */
        numRead = (len - cnt) > 3 ? 4 : (len - cnt);
        /* set 'shift data' flag and number of data bytes */
        spiCtrl |= ((0x4 << 11) | ((numRead & 0x03) << 17));

        /* send command to the flash */
        status = SetSpiCtrlReg(handle, readFunc, writeFunc, spiCtrl);
        if (status) return status;

        /* get data */
        status = readFunc(handle, FM10000_SPI_RX_DATA(), &rxData);
        if (status) return status;

        if (debug & 1) printf("CTRL 0x%08x RXDATA 0x%08x\n", spiCtrl, rxData);

        /* push the read data into the array */
        while (numRead)
        {
            numRead--;
            data[cnt++] = (rxData >> (numRead * 8)) & 0xff;
        }

        spiCtrl &= 0x7ff;
    }

    /* release CS */
    spiCtrl |= (0x8 << 11);
    status = SetSpiCtrlReg(handle, readFunc, writeFunc, spiCtrl);
    if (status) return status;

    status = SpiDisable(handle, readFunc, writeFunc);

    return status;

}   /* end fm10000UtilSpiReadFlash */



/*****************************************************************************/
/* fm10000UtilSpiWriteFlash
 * \ingroup intPlatform
 *
 * \desc            Write number of bytes to the SPI flash.
 *
 *
 *****************************************************************************/
fm_status fm10000UtilSpiWriteFlash(fm_uintptr             handle,
                                   fm_utilRegRead32Func   readFunc,
                                   fm_utilRegWrite32Func  writeFunc,
                                   fm_uint                address,
                                   fm_byte               *data,
                                   fm_int                 len,
                                   fm_int                 freqKhz)
{
    fm_status status = FM_OK;

    fm_int    cnt;
    fm_int    freq;
    fm_int    numWrite;

    fm_uint32 txData;
    fm_uint32 spiCtrl;
    fm_uint32 header;

    if (len <= 0 && len > 256)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ((address/256) != (address+len-1)/256)
    {
        printf("ERROR: Buffer cannot span two pages. Address 0x%x len %d\n", address, len);
        return FM_ERR_INVALID_ARGUMENT;
    }

    if (debug & 1)
    {
        printf("ADDR 0x%x: ", address);
        for (cnt = 0; cnt < len; cnt++)
        {
            printf("%02x ", data[cnt]);
        }
        printf("\n");
    }

    status = SpiEnable(handle, readFunc, writeFunc);
    if (status) return status;

    status = readFunc(handle, FM10000_SPI_CTRL(), &spiCtrl);
    if (status) return status;

    if (freqKhz > 0)
    {
        freq = ((100000/freqKhz)/2)-1;
        if (freq < 0)
        {
            freq = 0;
        }
        FM_SET_FIELD(spiCtrl, FM10000_SPI_CTRL, Freq, freq);
    }

    FM_SET_BIT(spiCtrl, FM10000_SPI_CTRL, Enable, 1);
    spiCtrl &= 0x7ff;

    /* header: command (1 byte: PAGE_PROGRAM) + address (3 bytes) */
    header = (0x2 << 24) | (address & 0xffffff);
    status = writeFunc(handle, FM10000_SPI_HEADER(), header);
    if (status) return status;

    if (debug & 1) printf("HDR 0x%08x CTRL 0x%08x\n", header, spiCtrl);

    /* first loop only: set send header flag. Following loops: shift only data. */
    spiCtrl |= (0x1 << 11);

    cnt = 0;
    while (cnt < len)
    {
        /* determine the number of data bytes to send [1..4] */
        numWrite = (len - cnt) > 3 ? 4 : (len - cnt);
        /* set 'shift data' flag and number of data bytes */
        spiCtrl |= ((0x4 << 11) | ((numWrite & 0x03) << 17));

        txData = 0;
        while (numWrite--)
        {
            txData = (txData << 8) | data[cnt++];
        }

        if (debug & 1) printf("CTRL 0x%08x TXDATA 0x%08x\n", spiCtrl, txData);

        /* set data to be written */
        status = writeFunc(handle, FM10000_SPI_TX_DATA(), txData);
        if (status) return status;

        /* send command to the flash */
        status = SetSpiCtrlReg(handle, readFunc, writeFunc, spiCtrl);
        if (status) return status;

        spiCtrl &= 0x7ff;
    }

    /* release CS */
    spiCtrl |= (0x8 << 11);
    status = SetSpiCtrlReg(handle, readFunc, writeFunc, spiCtrl);
    if (status) return status;

    status = SpiDisable(handle, readFunc, writeFunc);
    if (status) return status;

    return status;

} /* fm10000UtilSpiWriteFlash */

