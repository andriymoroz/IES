/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_app_api.c
 * Creation Date:   June 2, 2014
 * Description:     Platform functions exported to applications
 *
 * Copyright (c) 2014 - 2015, Intel Corporation
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
#include <platforms/common/switch/fm10000/fm10000_voltage_scaling.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define MAX_XCVR_MEM_WRITE_BYTES  32

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
/** fmPlatformGetName
 * \ingroup freedomApp
 *
 * \desc            Returns platform name.
 *
 * \param[out]      name points to storage where the platform name
 *                  will be written.
 *
 * \param[in]       size is the size of name buffer.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetName(fm_char *name, fm_int size)
{
    FM_STRNCPY_S(name, size, (FM_PLAT_GET_CFG)->name, FM_PLAT_MAX_CFG_STR_LEN);

    return FM_OK;

}   /* end fmPlatformGetName */




/*****************************************************************************/
/** fmPlatformGetNumSwitches
 * \ingroup freedomApp
 *
 * \desc            Returns the number of switches in the system.
 *
 * \param[out]      numSw points to a caller-supplied variable in which
 *                  the function will store the number of switches for this
 *                  platform.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetNumSwitches(fm_int *numSw)
{
    if (!numSw)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    *numSw = FM_PLAT_NUM_SW;

    return FM_OK;

}   /* end fmPlatformGetNumSwitches */




/*****************************************************************************/
/** fmPlatformGetPortList
 * \ingroup freedomApp
 *
 * \desc            Returns a list of the logical ports on the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      portList points to the array where the function will
 *                  store the list of logical ports for this switch.
 *
 * \param[in]       listSize is the size of the portList array.
 *
 * \param[out]      numPorts points to the location where the function
 *                  will store the number of logical ports in the list.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetPortList(fm_int  sw,
                                fm_int *portList,
                                fm_int  listSize,
                                fm_int *numPorts)
{
    fm_status status;

    if (sw < 0 || sw >= FM_PLAT_NUM_SW  ||
        !numPorts || !portList || listSize <= 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    status = fmGetCardinalPortList(sw, numPorts, portList, listSize);

    return status;

}   /* end fmPlatformGetPortList */




/*****************************************************************************/
/** fmPlatformI2cWriteRead
 * \ingroup platformApp
 *
 * \desc            Write to then immediately read from an I2C device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bus is one of the bus numbers provided through the
 *                  api.platform.lib.config.bus%d.i2cDevName property.
 *
 * \param[in]       address is the I2C device address (0x00 - 0x7F).
 *
 * \param[in,out]   data points to a 32-bit word from which data is written
 *                  and into which data is read.
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
fm_status fmPlatformI2cWriteRead(fm_int     sw,
                                 fm_int     bus,
                                 fm_int     address,
                                 fm_uint32 *data,
                                 fm_int     wl,
                                 fm_int     rl)
{
    fm_int          swNum;
    fm_byte         reg[4];
    fm_int          i;
    fm_status       status;
    fm_uint32       tmpv;
    fm_platformLib *libFunc;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, bus = %d, address = %d, "
                 "data = %p, wlength = %d rlength = %d\n",
                 sw,
                 bus,
                 address,
                 (void *) data,
                 wl,
                 rl);

    if (sw < 0 || sw >= FM_PLAT_NUM_SW)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    if (wl > 4 || rl > 4)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < wl ; i++)
    {
        reg[i] = data[0] >> ( 8 * (wl - i - 1) );
    }

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    swNum   = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_NUMBER, bus);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    status = libFunc->I2cWriteRead(swNum, address, reg, wl, rl);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    if (status == FM_OK && rl)
    {
        tmpv = 0;

        for (i = 0 ; i < rl ; i++)
        {
            /* Reading from I2C is always big endian */
            tmpv = (tmpv << 8) + reg[i];
        }

        *data = tmpv;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformI2cWriteRead */




/*****************************************************************************/
/** fmPlatformI2cRead
 * \ingroup platformApp
 *
 * \desc            Read from an I2C device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bus is the I2C bus number.
 *
 * \param[in]       address is the I2C device address (0x00 - 0x7F).
 *
 * \param[out]      data points to a caller-supplied 32-bit word in which
 *                  this function will store the 8-bit values read from
 *                  the device.
 *
 * \param[in]       length is the number of data bytes to read (1..4).
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cRead(fm_int     sw,
                            fm_int     bus,
                            fm_int     address,
                            fm_uint32 *data,
                            fm_int     length)
{
    return fmPlatformI2cWriteRead(sw, bus, address, data, 0, length);

}   /* end fmPlatformI2cRead */




/*****************************************************************************/
/** fmPlatformI2cWrite
 * \ingroup platformApp
 *
 * \desc            Write to an I2C device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bus is the I2C bus number.
 *
 * \param[in]       address is the I2C device address (0x00 - 0x7F).
 *
 * \param[in]       data is a 32-bit word containing the 8-bit data values
 *                  to be written to the device.
 *
 * \param[in]       length is the number of data bytes to be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWrite(fm_int    sw,
                             fm_int    bus,
                             fm_int    address,
                             fm_uint32 data,
                             fm_int    length)
{
    return fmPlatformI2cWriteRead(sw, bus, address, &data, length, 0);

}   /* end fmPlatformI2cWrite */




/*****************************************************************************/
/** fmPlatformI2cWriteLong
 * \ingroup platformApp
 *
 * \desc            Write up to 8 bytes to an I2C device.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       bus is one of the bus numbers provided through the
 *                  api.platform.lib.config.bus%d.i2cDevName property.
 *
 * \param[in]       address is the I2C device address (0x00 - 0x7F).
 *
 * \param[in]       data is a 64-bit word containing the 8-bit data values
 *                  to be written to the device.
 *
 * \param[in]       length is the number of data bytes to be written.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformI2cWriteLong(fm_int    sw,
                                 fm_int    bus,
                                 fm_int    address,
                                 fm_uint64 data,
                                 fm_int    length)
{
    fm_int          swNum;
    fm_byte         reg[8];
    fm_int          i;
    fm_status       status;
    fm_platformLib *libFunc;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, bus = %d, address = %d, "
                 "data = 0x%llx length = %d\n",
                 sw,
                 bus,
                 address,
                 data,
                 length);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    if (length > 8)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    for (i = 0 ; i < length ; i++)
    {
        reg[i] = data >> ( 8 * (length - i - 1) );
    }

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    swNum   = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_NUMBER, bus);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    status = libFunc->I2cWriteRead(swNum, address, reg, length, 0);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformI2cWriteLong */




/*****************************************************************************/
/** fmPlatformXcvrIsPresent
 * \ingroup freedomApp
 *
 * \desc            Determines whether a transceiver is present.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[out]      present points to an memory where this function will return
 *                  whether the module is present or not.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformXcvrIsPresent(fm_int   sw,
                                  fm_int   port,
                                  fm_bool *present)
{
    fm_status           status;
    fm_int              swNum;
    fm_uint32           hwResId;
    fm_uint32           xcvrStateValid;
    fm_uint32           xcvrState;
    fm_platformLib *    libFunc;
    fm_platformCfgPort *portCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, port = %d, present = %p\n",
                 sw,
                 port,
                 (void*) present);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->GetPortXcvrState )
    {
        /* Assume present when there is no support */
        *present = TRUE;
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    *present = FALSE;

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &sw,
                                                &swNum,
                                                &hwResId,
                                                &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (portCfg->intfType == FM_PLAT_INTF_TYPE_NONE ||
        portCfg->intfType == FM_PLAT_INTF_TYPE_PCIE)
    {
        /* Get xcvr state for SFPP and QSFP only */
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    if ((status = fmPlatformMgmtTakeSwitchLock(sw)) != FM_OK)
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_STATE, hwResId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    status = libFunc->GetPortXcvrState(swNum, 
                                       &hwResId, 
                                       1, 
                                       &xcvrStateValid, 
                                       &xcvrState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    *present = (xcvrStateValid & FM_PLAT_XCVR_PRESENT) &&
               (xcvrState & FM_PLAT_XCVR_PRESENT);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);
    fmPlatformMgmtDropSwitchLock(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformXcvrIsPresent */




/*****************************************************************************/
/** fmPlatformXcvrEnable
 * \ingroup freedomApp
 *
 * \desc            Enables/disables a transceiver.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[out]      enable specifies enable or disable the transceiver.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformXcvrEnable(fm_int sw, fm_int port, fm_bool enable)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, port = %d, enable = %d\n",
                 sw,
                 port,
                 enable);

    status = fmPlatformMgmtEnableXcvr(sw, port, enable);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformXcvrEnable */




/*****************************************************************************/
/** fmPlatformXcvrEnableLpMode
 * \ingroup freedomApp
 *
 * \desc            Enables/disables a transceiver low power mode.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[out]      enable specifies enable or disable the transceiver lp mode.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformXcvrEnableLpMode(fm_int  sw,
                                     fm_int  port,
                                     fm_bool enable)
{
    fm_status       status;
    fm_int          swNum;
    fm_uint32       hwResId;
    fm_uint32       xcvrStateValid;
    fm_uint32       xcvrState;
    fm_int          portIdx;
    fm_platformLib *libFunc;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, port = %d, enable = %d\n",
                 sw,
                 port,
                 enable);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    /* Save the transceiver enable state */
    portIdx = fmPlatformCfgPortGetIndex(sw, port);
    if (portIdx >= 0)
    {
        if (FM_PLAT_GET_PORT_CFG(sw, portIdx)->intfType != 
                                                        FM_PLAT_INTF_TYPE_QSFP_LANE0)
        {
            /* Only QSFP has LP mode support */
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
        }
    }

    if ( !libFunc->SetPortXcvrState )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
    }

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &sw,
                                                &swNum,
                                                &hwResId,
                                                NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if ((status = fmPlatformMgmtTakeSwitchLock(sw)) != FM_OK)
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }
    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_STATE, hwResId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    xcvrStateValid = FM_PLAT_XCVR_LPMODE;
    xcvrState      = enable ? FM_PLAT_XCVR_LPMODE : 0;

    status = libFunc->SetPortXcvrState(swNum, hwResId, xcvrStateValid, xcvrState);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);
    fmPlatformMgmtDropSwitchLock(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformXcvrEnableLpMode */





/*****************************************************************************/
/** fmPlatformXcvrMemWrite
 * \ingroup freedomApp
 *
 * \desc            Writes bytes to an SFP+/QSFP module on a given port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       page is the page number.
 *
 * \param[in]       offset is the offset from start of page.
 *
 * \param[in]       data points to an array containing the 8-bit data
 *                  values to be written to the device. The array must be
 *                  length bytes long.
 *
 * \param[in]       length is the number of data bytes to write.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformXcvrMemWrite(fm_int   sw,
                                 fm_int   port,
                                 fm_int   page,
                                 fm_int   offset,
                                 fm_byte *data,
                                 fm_int   length)
{
    fm_status           status;
    fm_int              address;
    fm_int              swNum;
    fm_uint32           hwResId;
    fm_bool             qsfp;
    fm_platformCfgPort *portCfg;
    fm_byte             bytes[MAX_XCVR_MEM_WRITE_BYTES + 1];
    fm_int              cnt;
    fm_platformLib *    libFunc;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, port = %d, page = %d, offset = %d length = %d\n",
                 sw,
                 port,
                 page,
                 offset,
                 length);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    if (length > MAX_XCVR_MEM_WRITE_BYTES)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &sw,
                                                &swNum,
                                                &hwResId,
                                                &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    switch (portCfg->intfType)
    {
        case FM_PLAT_INTF_TYPE_QSFP_LANE0:
        case FM_PLAT_INTF_TYPE_QSFP_LANE1:
        case FM_PLAT_INTF_TYPE_QSFP_LANE2:
        case FM_PLAT_INTF_TYPE_QSFP_LANE3:
            qsfp = TRUE;
            break;

        case FM_PLAT_INTF_TYPE_SFPP:
            qsfp = FALSE;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            break;
    }   /* switch */

    if ((status = fmPlatformMgmtTakeSwitchLock(sw)) != FM_OK)
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }
    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_EEPROM, hwResId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    address = 0x50;

    if (qsfp)
    {
        if (page > 3)
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if ( (offset + length) > 128 && page >= 0 )
        {
            /* Refer to SFF-8436 */
            bytes[0] = 127;
            bytes[1] = page;
            status   = libFunc->I2cWriteRead(swNum, address, bytes, 2, 0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            /* For some modules, such as Aphenol 566570001, need a delay here */
            fmDelay(0, 20 * 1000 * 1000);
        }
    }
    else
    {
        if (page > 1)
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if (page)
        {
            /* Set I2C address corresponding to page number */
            address = 0x51;
        }
    }

    /* Write the offset to be read from. */
    bytes[0] = offset;

    for (cnt = 0 ; cnt < length ; cnt++)
    {
        bytes[cnt + 1] = data[cnt];
    }

    status = libFunc->I2cWriteRead(swNum, address, bytes, length + 1, 0);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);
    fmPlatformMgmtDropSwitchLock(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformXcvrMemWrite */




/*****************************************************************************/
/** fmPlatformXcvrMemRead
 * \ingroup freedomApp
 *
 * \desc            Reads bytes from an SFP+/QSFP module on a given port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       page is the page number.
 *
 * \param[in]       offset is the offset from start of page.
 *
 * \param[out]      data points to an array where this function will store
 *                  the 8-bit data values read from the device. The array must
 *                  be length bytes long.
 *
 * \param[in]       length is the number of data bytes to read.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformXcvrMemRead(fm_int   sw,
                                fm_int   port,
                                fm_int   page,
                                fm_int   offset,
                                fm_byte *data,
                                fm_int   length)
{
    fm_status           status;
    fm_int              address;
    fm_int              swNum;
    fm_uint32           hwResId;
    fm_bool             qsfp;
    fm_platformCfgPort *portCfg;
    fm_platformLib     *libFunc;
    fm_byte             bytes[2];

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, port = %d, page = %d, offset = %d length = %d\n",
                 sw,
                 port,
                 page,
                 offset,
                 length);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &sw,
                                                &swNum,
                                                &hwResId,
                                                &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    switch (portCfg->intfType)
    {
        case FM_PLAT_INTF_TYPE_QSFP_LANE0:
        case FM_PLAT_INTF_TYPE_QSFP_LANE1:
        case FM_PLAT_INTF_TYPE_QSFP_LANE2:
        case FM_PLAT_INTF_TYPE_QSFP_LANE3:
            qsfp = TRUE;
            break;

        case FM_PLAT_INTF_TYPE_SFPP:
            qsfp = FALSE;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            break;
    }   /* switch */

    if ((status = fmPlatformMgmtTakeSwitchLock(sw)) != FM_OK)
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }
    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_EEPROM, hwResId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    address = 0x50;

    /* Set I2C address corresponding to page number */
    if (qsfp)
    {
        if (page > 3)
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if ( (offset + length) > 128 && page >= 0 )
        {
            /* Refer to SFF-8436 */
            bytes[0] = 127;
            bytes[1] = page;
            status   = libFunc->I2cWriteRead(swNum, address, bytes, 2, 0);

            if (status == FM_OK)
            {
                /* For some modules, such as Aphenol 566570001, need a delay here */
                fmDelay(0, 20 * 1000 * 1000);
            }
            else
            {
                /* For some modules, such as the Molex/74757-1031, the write to
                   the page register doesn't work.
                 
                   In that case do not return an error and most likely page 0
                   will be selected and the read to Upper Memory Map: Page 0
                   will work. */
            }
        }
    }
    else
    {
        if (page > 1)
        {
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if (page)
        {
            address = 0x51;
        }
    }

    /* Write the offset to be read from. */
    data[0] = offset & 0xFF;

    status = libFunc->I2cWriteRead(swNum, address, data, 1, length);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);
    fmPlatformMgmtDropSwitchLock(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformXcvrMemRead */




/*****************************************************************************/
/** fmPlatformXcvrEepromRead
 * \ingroup freedomApp
 *
 * \desc            Reads a byte (8 bits) from an SFP+/QSFP module's EEPROM
 *                  on a given port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       page is the page number.
 *
 * \param[in]       offset is the offset from start of page.
 *
 * \param[out]      data points to an array where this function will write
 *                  the 8-bit data values read from the device. The array must
 *                  be length bytes long.
 *
 * \param[in]       length is the number of data bytes to read.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformXcvrEepromRead(fm_int   sw,
                                   fm_int   port,
                                   fm_int   page,
                                   fm_int   offset,
                                   fm_byte *data,
                                   fm_int   length)
{
    fm_status           status;
    fm_int              swNum;
    fm_int              phySw;
    fm_uint32           hwResId;
    fm_platformCfgPort *portCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, port = %d, page = %d, offset = %d length = %d\n",
                 sw,
                 port,
                 page,
                 offset,
                 length);

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &phySw,
                                                &swNum,
                                                &hwResId,
                                                &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    switch (portCfg->intfType)
    {
        case FM_PLAT_INTF_TYPE_QSFP_LANE0:
        case FM_PLAT_INTF_TYPE_QSFP_LANE1:
        case FM_PLAT_INTF_TYPE_QSFP_LANE2:
        case FM_PLAT_INTF_TYPE_QSFP_LANE3:

            if (offset + length > 128)
            {
                return FM_ERR_INVALID_ARGUMENT;
            }

            offset += 128;
            break;

        case FM_PLAT_INTF_TYPE_SFPP:
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
            break;
    }   /* switch */


    status = fmPlatformXcvrMemRead(sw,
                                   port,
                                   page,
                                   offset,
                                   data,
                                   length);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformXcvrEepromRead */




/*****************************************************************************/
/** fmPlatformSetVrmVoltage
 * \ingroup freedomApp
 *
 * \desc            Set the voltage regulator module.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vrmId is the VRM ID (VDDS, VDDF, AVDD).
 *
 * \param[in]       mVolt is the voltage to set in milli-volt.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetVrmVoltage(fm_int         sw,
                                  fm_platVrmType vrmId,
                                  fm_uint32      mVolt)
{
    fm_status             status = FM_OK;
    fm_platformLib *      libFunc;
    fm_platformCfgSwitch *swCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, vrmId = %d, mVolt = %d\n",
                 sw,
                 vrmId,
                 mVolt);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if (vrmId >= FM_PLAT_MAX_VRM )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, status);
    }

    if (libFunc->SetVrmVoltage && swCfg->vrm.hwResourceId[vrmId] != -1)
    {
        status = libFunc->SetVrmVoltage(sw,
                                        swCfg->vrm.hwResourceId[vrmId],
                                        mVolt);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, status);
    }


ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformSetVrmVoltage */




/*****************************************************************************/
/** fmPlatformGetVrmVoltage
 * \ingroup freedomApp
 *
 * \desc            Get the actual voltage from voltage regulator module.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vrmId is the VRM ID (VDDS, VDDF, AVDD).
 *
 * \param[out]      mVolt is the caller allocated storage where the
 *                  function will place the voltage in milli-volt.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformGetVrmVoltage(fm_int         sw,
                                  fm_platVrmType vrmId,
                                  fm_uint32 *    mVolt)
{
    fm_status             status = FM_OK;
    fm_platformLib *      libFunc;
    fm_platformCfgSwitch *swCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw = %d, vrmId = %d\n",
                 sw,
                 vrmId);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if (vrmId >= FM_PLAT_MAX_VRM )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, status);
    }

    if (libFunc->GetVrmVoltage && swCfg->vrm.hwResourceId[vrmId] != -1)
    {
        status = libFunc->GetVrmVoltage(sw,
                                        swCfg->vrm.hwResourceId[vrmId],
                                        mVolt);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_PLATFORM, status);
    }

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformGetVrmVoltage */




/*****************************************************************************/
/** fmPlatformGetNominalSwitchVoltages
 * \ingroup freedomApp
 *
 * \desc            Return the nominal switch voltages (VDDS and VDDF).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      vdds is the caller allocated storage where the
 *                  function will place the VDDS voltage in milli-volt.
 * 
 * \param[out]      vddf is the caller allocated storage where the
 *                  function will place the VDDF voltage in milli-volt.
 *
 * \param[out]      defVoltages is the caller allocated storage where the
 *                  function will indicate whether the returned vdds and vddf
 *                  contain the default values defined in the EAS or not.
 *                  Default values are returned whenever the fuse box is not
 *                  programmed (contains 0).
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetNominalSwitchVoltages(fm_int     sw,
                                             fm_uint32 *vdds,
                                             fm_uint32 *vddf,
                                             fm_bool   *defVoltages)
{
    fm_fm10000NominalVoltages voltage;
    fm_switch *               switchPtr;
    fm_status                 status;

    VALIDATE_SWITCH_LOCK(sw);
    PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    status = fm10000GetNominalSwitchVoltages(sw, 
                                             &voltage,
                                             switchPtr->ReadUINT32);
    if ( status == FM_OK )
    {
        if (vdds)
        {
            *vdds = voltage.VDDS;
        }

        if (vddf)
        {
            *vddf = voltage.VDDF;
        }

        if (defVoltages) 
        {
            *defVoltages = voltage.defValUsed;
        }
    }

    UNPROTECT_SWITCH(sw);

    return status;

}   /* end fmPlatformGetNominalSwitchVoltages */




/*****************************************************************************/
/** fmPlatformSwitchGpioSetDirection
 * \ingroup freedomApp
 *
 * \desc            Set the switch GPIO pin direction.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the switch GPIO number on which to operate.
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
fm_status fmPlatformSwitchGpioSetDirection(fm_int           sw,
                                           fm_int           gpio,
                                           fm_platSwGpioDir direction,
                                           fm_int           value)
{
    fm_platGpioDirection dir;
    fm_status            err;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d gpio=%d direction=%d value=%d\n",
                 sw,
                 gpio,
                 direction,
                 value);

    if ( direction == FM_PLAT_SW_GPIO_DIR_INPUT )
    {
        dir = FM_PLAT_GPIO_DIR_INPUT;
    }
    else if ( direction == FM_PLAT_SW_GPIO_DIR_OUTPUT )
    {
        dir = FM_PLAT_GPIO_DIR_OUTPUT;
    }
    else if ( direction == FM_PLAT_SW_GPIO_DIR_OPEN_DRAIN )
    {
        dir = FM_PLAT_GPIO_DIR_OPEN_DRAIN;
    }
    else
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    err = fmPlatformGpioSetDirection(sw, gpio, dir, value);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}  /* end fmPlatformSwitchGpioSetDirection */




/*****************************************************************************/
/** fmPlatformSwitchGpioGetDirection
 * \ingroup freedomApp
 *
 * \desc            Return the switch GPIO pin direction.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the switch GPIO number on which to operate.
 *
 * \param[out]      direction points to the location where the function
 *                  will store the GPIO direction value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchGpioGetDirection(fm_int            sw,
                                           fm_int            gpio,
                                           fm_platSwGpioDir *direction)
{
    fm_platGpioDirection dir;
    fm_status            err;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d gpio=%d \n", sw, gpio);

    err = fmPlatformGpioGetDirection(sw, gpio, &dir);

    if ( err == FM_OK )
    {
        if ( dir == FM_PLAT_GPIO_DIR_INPUT )
        {
            *direction = FM_PLAT_SW_GPIO_DIR_INPUT;
        }
        else if ( dir == FM_PLAT_GPIO_DIR_OUTPUT )
        {
            *direction = FM_PLAT_SW_GPIO_DIR_OUTPUT;
        }
        else
        {
            *direction = FM_PLAT_SW_GPIO_DIR_OPEN_DRAIN;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}  /* end fmPlatformSwitchGpioGetDirection */




/*****************************************************************************/
/** fmPlatformSwitchGpioMaskIntr
 * \ingroup freedomApp
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
fm_status fmPlatformSwitchGpioMaskIntr(fm_int sw, fm_int gpio)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d gpio=%d \n", sw, gpio);

    err = fmPlatformGpioMaskIntr(sw, gpio);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}  /* end fmPlatformSwitchGpioMaskIntr */




/*****************************************************************************/
/** fmPlatformSwitchGpioUnmaskIntr
 * \ingroup freedomApp
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
fm_status fmPlatformSwitchGpioUnmaskIntr(fm_int            sw,
                                         fm_int            gpio,
                                         fm_platSwGpioIntr edge)
{
    fm_platGpioIntrEdge level;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d gpio=%d edge=%d\n",
                 sw,
                 gpio,
                 edge);

    if ( edge == FM_PLAT_SW_GPIO_RISING_EDGE )
    {
        level = FM_PLAT_GPIO_INTR_RISING;
    }
    else if ( edge == FM_PLAT_SW_GPIO_FALLING_EDGE )
    {
        level = FM_PLAT_GPIO_INTR_FALLING;
    }
    else if ( edge == FM_PLAT_SW_GPIO_BOTH_EDGE )
    {
        level = FM_PLAT_GPIO_INTR_BOTH_EDGE;
    }
    else
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    err = fmPlatformGpioUnmaskIntr(sw, gpio, level);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}  /* end fmPlatformSwitchGpioUnmaskIntr */




/*****************************************************************************/
/** fmPlatformSwitchGpioSetValue
 * \ingroup freedomApp
 *
 * \desc            Set a switch GPIO pin state to the given value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the switch GPIO number on which to operate.
 *
 * \param[in]       value is the value to set. Zero for low, non-zero for high.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchGpioSetValue(fm_int sw, fm_int gpio, fm_int value)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d gpio=%d value=%d\n",
                 sw,
                 gpio,
                 value);

    err = fmPlatformGpioSetValue(sw, gpio, value);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}  /* end fmPlatformSwitchGpioSetValue */




/*****************************************************************************/
/** fmPlatformSwitchGpioGetValue
 * \ingroup freedomApp
 *
 * \desc            Return the current switch GPIO pin state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       gpio is the switch GPIO number on which to operate.
 *
 * \param[out]      value points to the location where the function
 *                  will store the GPIO state value.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformSwitchGpioGetValue(fm_int sw, fm_int gpio, fm_int *value)
{
    fm_status err;
    fm_int    val;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw=%d gpio=%d\n", sw, gpio);

    err = fmPlatformGpioGetValue(sw, gpio, &val);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);

}  /* end fmPlatformSwitchGpioGetValue */
