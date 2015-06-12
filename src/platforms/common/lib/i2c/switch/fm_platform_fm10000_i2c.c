/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_fm10000_i2c.c
 * Creation Date:   May 16, 2014
 * Description:     Functions to access I2C via switch I2C master
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

#include <fm_sdk.h>
#include <unistd.h>
#include <sys/time.h>

/* NOTE: This file is being shared also with other utils programs */

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define I2C_MAX_LEN 12
#define I2C_TIMEOUT 2000

#define FM10000_MGMT_BASE                        (0x000000)
#define FM10000_I2C_CFG                          ((0x000C19) + (FM10000_MGMT_BASE))
                                                 /* 0x000C19 FM10000_MGMT_BASE */
#define FM10000_I2C_CFG_WIDTH                    1

#define FM10000_I2C_DATA(index)                  ((0x000001) * ((index) - 0) + (0x000C1C) + (FM10000_MGMT_BASE))
                                                 /* 0x000C1C FM10000_MGMT_BASE */
#define FM10000_I2C_DATA_WIDTH                   1
#define FM10000_I2C_DATA_ENTRIES                 3

#define FM10000_I2C_CTRL                         ((0x000C20) + (FM10000_MGMT_BASE))
                                                 /* 0x000C20 FM10000_MGMT_BASE */
#define FM10000_I2C_CTRL_WIDTH                   1

#define FM10000_I2C_CFG_b_Enable                 0
#define FM10000_I2C_CFG_l_Addr                   1
#define FM10000_I2C_CFG_h_Addr                   7
#define FM10000_I2C_CFG_l_Divider                8
#define FM10000_I2C_CFG_h_Divider                19
#define FM10000_I2C_CFG_b_InterruptMask          20
#define FM10000_I2C_CFG_l_DebounceFilterCountLimit 21
#define FM10000_I2C_CFG_h_DebounceFilterCountLimit 27

#define FM10000_I2C_DATA_l_Data                  0
#define FM10000_I2C_DATA_h_Data                  31

#define FM10000_I2C_CTRL_l_Addr                  0
#define FM10000_I2C_CTRL_h_Addr                  7
#define FM10000_I2C_CTRL_l_Command               8
#define FM10000_I2C_CTRL_h_Command               9
#define FM10000_I2C_CTRL_l_LengthW               10
#define FM10000_I2C_CTRL_h_LengthW               13
#define FM10000_I2C_CTRL_l_LengthR               14
#define FM10000_I2C_CTRL_h_LengthR               17
#define FM10000_I2C_CTRL_l_LengthSent            18
#define FM10000_I2C_CTRL_h_LengthSent            21
#define FM10000_I2C_CTRL_l_CommandCompleted      22
#define FM10000_I2C_CTRL_h_CommandCompleted      25
#define FM10000_I2C_CTRL_b_InterruptPending      26


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static fm_int i2cDebug = 0x0;


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** ReadSwitchMemMap
 * \ingroup intPlatformfm10000I2c
 *
 * \desc            Read a CSR register.
 *
 * \param[in]       memPtr is the memory pointer to the switch.
 *
 * \param[in]       addr contains the CSR register address to read
 *
 * \param[out]      value points to storage where this function will place
 *                  the read register value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status ReadSwitchMemMap(fm_uint32 *memPtr, 
                                  fm_uint32  address, 
                                  fm_uint32 *value)
{
    if (memPtr == NULL)
    {
        return FM_FAIL;
    }

    *value = *(memPtr+address);

    return FM_OK;

}   /* end ReadSwitchMemMap */




/*****************************************************************************/
/** WriteSwitchMemMap
 * \ingroup intPlatformfm10000I2c
 *
 * \desc            Write a CSR register.
 *
 * \param[in]       memPtr is the memory pointer to the switch.
 *
 * \param[in]       address contains the CSR register address to read.
 *
 * \param[in]       value is the data value to write to the register.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status WriteSwitchMemMap(fm_uint32 *memPtr,
                                   fm_uint32  address,
                                   fm_uint32  value)
{
    if (memPtr == NULL)
    {
        return FM_FAIL;
    }

    *(memPtr+address) = value;

    return FM_OK;

}   /* end WriteSwitchMemMap */




/*****************************************************************************/
/* GetTimeIntervalMsec
 * \ingroup intPlatformfm10000I2c
 *
 * \desc            Return the time interval between two timestamps.
 *
 * \param[in]       begin is the begin timestamp.
 *
 * \param[in]       end is the end timestamp. If NULL, use the current
 *                  timestamp
 *
 * \return          Number of milli seconds.
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
        endT.tv_sec  = end->tv_sec;
        endT.tv_usec = end->tv_usec;
    }

    diff.tv_sec  = endT.tv_sec - begin->tv_sec;
    diff.tv_usec = endT.tv_usec - begin->tv_usec;

    return diff.tv_sec * 1000 * 1000 + diff.tv_usec;

}   /* end GetTimeIntervalMsec */




/*****************************************************************************/
/* _fmPlatformFm10000I2cWriteRead
 * \ingroup intPlatformfm10000I2c
 *
 * \desc            Write to then immediately read from an I2C device with
 *                  handling max response bytes using switch as I2C master.
 *
 * \param[in]       handle is the memory pointer to the switch.
 *
 * \param[in]       device is the I2C device address (0x00 - 0x7F).
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
static fm_status _fmPlatformFm10000I2cWriteRead(fm_uintptr handle,
                                                fm_uint    device,
                                                fm_byte   *data,
                                                fm_uint    wl,
                                                fm_uint    rl)
{
    fm_status      status;
    fm_uint32      regValue;
    fm_uint        i;
    fm_uint        j;
    fm_bool        isTimeout;
    struct timeval startTime;
    fm_uint32     *memPtr;
    fm_uint32      tmp;

    if (i2cDebug & 0x4) 
    {
        FM_LOG_PRINT("fmPlatformfm10000I2cWriteRead "
                     "handle=%p device=0x%x wl=%d rl=%d\n",
                      (void*)handle, device, wl, rl);
    }

    memPtr = (fm_uint32*)handle;

    if (rl > I2C_MAX_LEN)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (wl > I2C_MAX_LEN)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (data == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < (wl+3)/4 ; i++)
    {
        regValue = 0;
        for (j = 0 ; j < 4 ; j++)
        {
            if ((i*4 + j) < wl)
            {
                regValue |= (data[i*4 + j] << ((3 - j)*8));
            }
        }
        status = WriteSwitchMemMap(memPtr, FM10000_I2C_DATA(i), regValue);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        if (i2cDebug & 0x4)
        {
            FM_LOG_PRINT("WRITEDATA#%d : 0x%08x\n",i, regValue); 
        }
    }

    regValue = 0;
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Addr, (device  << 1));
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Command, 0);
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, LengthW, wl);
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, LengthR, rl);

    if (i2cDebug & 0x4)
    {
         FM_LOG_PRINT("WRITE: eg 0x%08x Cmd %d wl %d rl %d "
                      "Comp %x LenSent %d intr %d\n",
                      regValue,
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                      FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));
    }

    status = WriteSwitchMemMap(memPtr, FM10000_I2C_CTRL, regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    status = ReadSwitchMemMap(memPtr, FM10000_I2C_CTRL, &tmp);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    if (i2cDebug & 0x4)
    {
        FM_LOG_PRINT("READ: %d reg 0x%08x Cmd %d wl %d rl %d "
                     "Comp %x LenSent %d intr %d\n", status, tmp,
                     FM_GET_FIELD(tmp, FM10000_I2C_CTRL, Command),
                     FM_GET_FIELD(tmp, FM10000_I2C_CTRL, LengthW),
                     FM_GET_FIELD(tmp, FM10000_I2C_CTRL, LengthR),
                     FM_GET_FIELD(tmp, FM10000_I2C_CTRL, CommandCompleted),
                     FM_GET_FIELD(tmp, FM10000_I2C_CTRL, LengthSent),
                     FM_GET_BIT(tmp, FM10000_I2C_CTRL, InterruptPending));
    }

    /* Now change command to start the transaction */
    if (rl == 0) 
    {
        /* Write only allow write of 0 length */
        FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Command, 1);
    }
    else if ((wl > 0) && (rl > 0))
    {
        /* Write Read */
        FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Command, 2);
    }
    else
    {
        /* Read */
        FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Command, 3);
    }

    if (i2cDebug & 0x4) 
    {
        FM_LOG_PRINT("WRITE2: reg 0x%08x Cmd %d wl %d rl %d "
                     "Comp %x LenSent %d intr %d\n", regValue,
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                     FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));
    }

    status = WriteSwitchMemMap(memPtr, FM10000_I2C_CTRL, regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    status = ReadSwitchMemMap(memPtr, FM10000_I2C_CTRL, &regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    if (i2cDebug & 0x4) 
    {
        FM_LOG_PRINT("READ2: %d reg 0x%08x Cmd %d wl %d rl %d "
                     "Comp %x LenSent %d intr %d\n", status, regValue,
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                     FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));
    }

    /* Poll to check for command done */
    gettimeofday(&startTime, NULL);
    isTimeout = FALSE;
    do
    {
        usleep(10000);
        if (isTimeout)
        {
            FM_LOG_ERROR(
                   FM_LOG_CAT_PLATFORM, 
                   "Dev=0x%02x: Timeout (%d msec) waiting "
                   "for I2C_CTRL(0x%x).CommandCompleted!=0. 0x%02x\n",
                   device,
                   I2C_TIMEOUT,
                   FM10000_I2C_CTRL, 
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted));

            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_I2C_NO_RESPONSE);
        }

        /* Variable isTimeout is used to improve the timeout detecting */
        if (GetTimeIntervalMsec(&startTime, NULL) > I2C_TIMEOUT)
        {
            isTimeout = TRUE;
        }

        status = ReadSwitchMemMap(memPtr, FM10000_I2C_CTRL, &regValue);
        if (i2cDebug & 0x4)
        {
            FM_LOG_PRINT(
                   "STATUS: %d reg 0x%08x cmd %d wl %d rl %d "
                   "Comp %x LenSent %d intr %d\n", status, regValue,
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                   FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));
        }

        if (status) break;

    } while (FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted) == 0);

    if (FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted) != 1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                     "Dev=0x%02x: I2C Command completed with error 0x%x. "
                     "I2C_CTRL(0x%x)=0x%x\n",
                     device,
                     FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                     FM10000_I2C_CTRL,
                     regValue);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_I2C_NO_RESPONSE);
    }

    for (i = 0 ; i < (rl+3)/4 ; i++)
    {
        status = ReadSwitchMemMap(memPtr, FM10000_I2C_DATA(i), &regValue);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

        if (i2cDebug & 0x4)
        {
            FM_LOG_PRINT("READDATA#%d : 0x%08x\n",i, regValue);
        }

        for (j = 0 ; j < 4 ; j++)
        {
            if ((i*4 + j) < rl)
            {
                data[i*4 + j] = (regValue >> ((3 - j)*8)) & 0xFF;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);


}   /* end _fmPlatformFm10000I2cWriteRead */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformFm10000I2cSetDebug
 * \ingroup intPlatformfm10000I2c
 *
 * \desc            This function sets i2cDebug value.
 *
 * \param[in]       value is the debug value.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPlatformFm10000I2cSetDebug(fm_int value)
{
    FM_LOG_PRINT("i2cDebug is set to 0x%x from 0x%x\n", value, i2cDebug);
    i2cDebug = value;

} /* end fmPlatformFm10000I2cSetDebug */




/*****************************************************************************/
/* fmPlatformFm10000I2cWriteRead
 * \ingroup intPlatformfm10000I2c
 *
 * \desc            Write to then immediately read from an I2C device with
 *                  handling max response bytes using switch as I2C master.
 *
 * \param[in]       handle is the memory pointer to the switch.
 *
 * \param[in]       device is the I2C device address (0x00 - 0x7F).
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
fm_status fmPlatformFm10000I2cWriteRead(fm_uintptr handle,
                                        fm_uint    device,
                                        fm_byte   *data,
                                        fm_uint    wl,
                                        fm_uint    rl)
{
    fm_status status;
    fm_uint   readLen;
    fm_uint   cnt;

    status = FM_FAIL;

    if (rl == 0) /* Allow 0 wl */
    {
        return _fmPlatformFm10000I2cWriteRead(handle, device, data, wl, 0);
    }
    else
    {
        cnt = 0;
        while (cnt < rl)
        {
            if (cnt < (rl - (rl % I2C_MAX_LEN)))
            {
                readLen = I2C_MAX_LEN;
            }
            else
            {
                readLen = rl % I2C_MAX_LEN;
            }

            status = _fmPlatformFm10000I2cWriteRead(handle, device, data + cnt, (cnt==0)?wl:0, readLen);
            if (status != FM_OK)
            {
                break;
            }
            cnt += readLen;
        }
    }

    return status;

}   /* end fmPlatformFm10000I2cWriteRead */
