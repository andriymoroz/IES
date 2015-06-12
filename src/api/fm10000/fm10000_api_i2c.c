/* vim:et:sw=4:ts=4:tw=79:
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_i2c.c
 * Creation Date:   October 30, 2014
 * Description:     FM10xxx-specific I2C functions
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define I2C_TIMEOUT_MSEC    2000
#define I2C_MAX_LEN         12

/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Function Prototypes
 *****************************************************************************/



/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/* I2cWriteRead
 * \ingroup intSwitch 
 *
 * \desc            Write to then immediately read from an I2C device with
 *                  handling max response bytes using switch as I2C master.
 *
 * \param[in]       sw is the switch number.
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
static fm_status I2cWriteRead(fm_int     sw,
                              fm_uint    device,
                              fm_byte   *data,
                              fm_uint    wl,
                              fm_uint    rl)
{
    fm_status      status;
    fm_switch     *switchPtr;
    fm_uint32      regValue;
    fm_uint        i;
    fm_uint        j;
    fm_bool        isTimeout;
    fm_timestamp   start;
    fm_timestamp   end;
    fm_timestamp   diff;
    fm_uint        delTime;
    fm_uint32      tmp;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_SWITCH,
                         "sw=%d device=0x%x wl=%d rl=%d\n",
                         sw, device, wl, rl);

    if (rl > I2C_MAX_LEN)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    if (wl > I2C_MAX_LEN)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    if (data == NULL)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    switchPtr = GET_SWITCH_PTR(sw);

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
        status = switchPtr->WriteUINT32(sw, FM10000_I2C_DATA(i), regValue);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, status);

        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH, "WRITEDATA#%d : 0x%08x\n",i, regValue); 
    }

    regValue = 0;
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Addr, (device  << 1));
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, Command, 0);
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, LengthW, wl);
    FM_SET_FIELD(regValue, FM10000_I2C_CTRL, LengthR, rl);

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH, "WRITE: eg 0x%08x Cmd %d wl %d rl %d "
                  "Comp %x LenSent %d intr %d\n",
                  regValue,
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                  FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));

    status = switchPtr->WriteUINT32(sw, FM10000_I2C_CTRL(), regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    status = switchPtr->ReadUINT32(sw, FM10000_I2C_CTRL(), &tmp);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH, "READ: %d reg 0x%08x Cmd %d wl %d rl %d "
                  "Comp %x LenSent %d intr %d\n", status, tmp,
                  FM_GET_FIELD(tmp, FM10000_I2C_CTRL, Command),
                  FM_GET_FIELD(tmp, FM10000_I2C_CTRL, LengthW),
                  FM_GET_FIELD(tmp, FM10000_I2C_CTRL, LengthR),
                  FM_GET_FIELD(tmp, FM10000_I2C_CTRL, CommandCompleted),
                  FM_GET_FIELD(tmp, FM10000_I2C_CTRL, LengthSent),
                  FM_GET_BIT(tmp, FM10000_I2C_CTRL, InterruptPending));

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

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH, "WRITE2: reg 0x%08x Cmd %d wl %d rl %d "
                  "Comp %x LenSent %d intr %d\n", regValue,
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                  FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));

    status = switchPtr->WriteUINT32(sw, FM10000_I2C_CTRL(), regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    status = switchPtr->ReadUINT32(sw, FM10000_I2C_CTRL(), &regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH, "READ2: %d reg 0x%08x Cmd %d wl %d rl %d "
                  "Comp %x LenSent %d intr %d\n", status, regValue,
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                  FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                  FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));

    /* Poll to check for command done */
    fmGetTime(&start);
    isTimeout = FALSE;
    do
    {
        fmDelay(0, 1000*500);
        if (isTimeout)
        {
            FM_LOG_ERROR(
                   FM_LOG_CAT_SWITCH, 
                   "Dev=0x%02x: Timeout (%d msec) waiting "
                   "for I2C_CTRL(0x%x).CommandCompleted!=0. 0x%02x\n",
                   device,
                   I2C_TIMEOUT_MSEC,
                   FM10000_I2C_CTRL(), 
                   FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted));

            FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_ERR_I2C_NO_RESPONSE);
        }

        fmGetTime(&end);
        fmSubTimestamps(&end, &start, &diff);
        delTime = diff.sec*1000 + diff.usec/1000;
        /* Variable isTimeout is used to improve the timeout detecting */
        if (delTime > I2C_TIMEOUT_MSEC)
        {
            isTimeout = TRUE;
        }

        status = switchPtr->ReadUINT32(sw, FM10000_I2C_CTRL(), &regValue);
        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH,
                      "STATUS: %d reg 0x%08x cmd %d wl %d rl %d "
                      "Comp %x LenSent %d intr %d\n", status, regValue,
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, Command),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthW),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthR),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, LengthSent),
                      FM_GET_BIT(regValue, FM10000_I2C_CTRL, InterruptPending));

        if (status) break;

    } while (FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted) == 0);

    if (FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted) != 1)
    {
        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH,
                      "Dev=0x%02x: I2C Command completed with error 0x%x. "
                      "I2C_CTRL(0x%x)=0x%x\n",
                      device,
                      FM_GET_FIELD(regValue, FM10000_I2C_CTRL, CommandCompleted),
                      FM10000_I2C_CTRL(),
                      regValue);
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, FM_ERR_I2C_NO_RESPONSE);
    }

    for (i = 0 ; i < (rl+3)/4 ; i++)
    {
        status = switchPtr->ReadUINT32(sw, FM10000_I2C_DATA(i), &regValue);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, status);

        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_SWITCH,"READDATA#%d : 0x%08x\n",i, regValue);

        for (j = 0 ; j < 4 ; j++)
        {
            if ((i*4 + j) < rl)
            {
                data[i*4 + j] = (regValue >> ((3 - j)*8)) & 0xFF;
            }
        }
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_SWITCH, status);


}   /* end I2cWriteRead */



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fm10000I2cWriteRead
 * \ingroup intSwitch
 *
 * \desc            Write to then immediately read from an I2C device with
 *                  handling max response bytes using switch as I2C master.
 *
 * \param[in]       sw is the switch number.
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
fm_status fm10000I2cWriteRead(fm_int     sw,
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
        return I2cWriteRead(sw, device, data, wl, 0);
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

            status = I2cWriteRead(sw, device, data + cnt, (cnt==0)?wl:0, readLen);
            if (status != FM_OK)
            {
                break;
            }
            cnt += readLen;
        }
    }

    return status;

} /* fm10000I2cWriteRead  */


