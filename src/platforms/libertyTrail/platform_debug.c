/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_debug.c
 * Creation Date:   June 2, 2014
 * Description:     Platform debug functions.
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
#include <platforms/util/fm_util_device_lock.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define DISP_LINE_LEN     16
#define MAX_STR_LEN       128
#define XCVR_EEPROM_SIZE  256
#define SFPP_EEPROM_SIZE  256
#define QSFP_EEPROM_SIZE  128

#define XCVR_STATE_FMT    "%15s: %s\n"

#define STR_EQ(str1, str2) (strcasecmp(str1, str2) == 0)

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
/* TakeLocks
 * \ingroup intPlatform
 *
 * \desc            Take appropriate locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status TakeLocks(fm_int sw)
{
    fm_status err;

    if ((err = fmPlatformMgmtTakeSwitchLock(sw)) == FM_OK)
    {
        if (GET_PLAT_PROC_STATE(sw)->fileLock >= 0)
        {
            fmUtilDeviceLockTake(GET_PLAT_PROC_STATE(sw)->fileLock);
        }

        TAKE_PLAT_I2C_BUS_LOCK(sw);
    }

    return err;

}   /* end TakeLocks */




/*****************************************************************************/
/* DropLock
 * \ingroup intPlatform
 *
 * \desc            Drop the locks
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static void DropLocks(fm_int sw)
{
    DROP_PLAT_I2C_BUS_LOCK(sw);

    if (GET_PLAT_PROC_STATE(sw)->fileLock >= 0)
    {
        fmUtilDeviceLockDrop(GET_PLAT_PROC_STATE(sw)->fileLock);
    }

    fmPlatformMgmtDropSwitchLock(sw);

}   /* end DropLocks */




/*****************************************************************************/
/* PhyI2cWriteRead
 * \ingroup intPlatform
 *
 * \desc            Wrapper function to libFunc->I2cWriteRead.
 *
 * \param[in]       handle is the switch number.
 *
 * \param[in]       device is the I2C device address.
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
static fm_status PhyI2cWriteRead(fm_uintptr handle,
                                 fm_uint    device,
                                 fm_byte   *data,
                                 fm_uint    wl,
                                 fm_uint    rl)
{
    fm_status       status;
    fm_int          sw;
    fm_platformLib *libFunc;

    sw = (fm_int) handle;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        return FM_ERR_UNSUPPORTED;
    }

    status = libFunc->I2cWriteRead(sw, device, data, wl, rl);

    return status;

}   /* end PhyI2cWriteRead */




/*****************************************************************************/
/** PrintBytesName
 * \ingroup intPlatform
 *
 * \desc            Print name and any printable characters in the buffer.
 *
 * \param[in]       name is the name to print if not NULL.
 *
 * \param[in]       addr is the address to print.
 *
 * \param[in]       buf is the buffer to print.
 *
 * \param[in]       len is the length of the buffer.
 *
 * \return          NONE
 *
 *****************************************************************************/
static void PrintBytesName(fm_text name, fm_int addr, fm_byte buf[], fm_int len)
{
    fm_int i;

    if (name)
    {
        FM_LOG_PRINT("%20s[%02x]: ", name, addr);
    }

    for (i = 0 ; i < len ; i++)
    {
        if ( isprint(buf[i]) )
        {
            FM_LOG_PRINT("%c", buf[i]);
        }
    }

    FM_LOG_PRINT("\n");

}   /* end PrintBytesName */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmPlatformPrintBytes
 * \ingroup intPlatform
 *
 * \desc            Print any printable characters in the buffer.
 *
 * \param[in]       buf is the buffer to print.
 *
 * \param[in]       len is the length of the buffer.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPlatformPrintBytes(fm_byte buf[], fm_int len)
{
    PrintBytesName(NULL, 0, buf, len);

}   /* end fmPlatformPrintBytes */




/*****************************************************************************/
/** fmPlatformHexDump
 * \ingroup intPlatform
 *
 * \desc            Print any printable characters in the buffer.
 *
 * \param[in]       addr is the address of the buffer.
 *
 * \param[in]       buf is the buffer to print.
 *
 * \param[in]       nbytes is the length of the buffer.
 *
 * \return          NONE
 *
 *****************************************************************************/
void fmPlatformHexDump(fm_int addr, fm_byte *buf, fm_int nbytes)
{
    fm_int linebytes;
    fm_int j;
    fm_int cnt;

    cnt = 0;

    do
    {
        linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

        FM_LOG_PRINT("%02x:", addr + cnt);

        for (j = 0 ; j < linebytes ; j++)
        {
            FM_LOG_PRINT(" %02x", buf[cnt + j]);
        }

        FM_LOG_PRINT("    ");

        for (j = 0 ; j < linebytes ; j++)
        {
            if ( (buf[cnt + j] < 0x20) || (buf[cnt + j] > 0x7e) )
            {
                FM_LOG_PRINT(".");
            }
            else
            {
                FM_LOG_PRINT("%c", buf[cnt + j]);
            }
        }

        FM_LOG_PRINT("\n");

        cnt    += linebytes;
        nbytes -= linebytes;

    }
    while (nbytes > 0);

}   /* end fmPlatformHexDump */




/*****************************************************************************/
/** fmPlatformDoDebug
 * \ingroup intPlatform
 *
 * \desc            Send debug command to shared library.
 *
 * \param[in]       sw is the sw number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       action is the action to execute.
 *
 * \param[in]       args is any extra arguments.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformDoDebug(fm_int  sw,
                            fm_int  port,
                            fm_text action,
                            fm_text args)
{
    fm_status       status;
    fm_platformLib *libFunc;
    fm_int          swNum;
    fm_uint32       hwResId;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->DoDebug )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
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
    status = libFunc->DoDebug(swNum, hwResId, action, args);
    DROP_PLAT_I2C_BUS_LOCK(sw);
    fmPlatformMgmtDropSwitchLock(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformDoDebug */




/*****************************************************************************/
/** fmPlatformDumpXcvrEeprom
 * \ingroup intPlatform
 *
 * \desc            Dump the content of transceiver module EEPROM.
 *
 * \param[in]       sw is the sw number.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformDumpXcvrEeprom(fm_int sw, fm_int port)
{
    fm_status           status;
    fm_int              swNum;
    fm_int              phySw;
    fm_uint32           hwResId;
    fm_platformCfgPort *portCfg;
    fm_int              nbytes;
    fm_int              addr;
    fm_int              page;
    fm_byte             eeprombuf[XCVR_EEPROM_SIZE];
    fm_bool             present;
    fm_int              qsfp;

    FM_LOG_PRINT("Switch: %d Port: %d EEPROM\n", sw, port);

    status = fmPlatformXcvrIsPresent(sw, port, &present);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (present)
    {
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
                qsfp = TRUE;
                break;

            case FM_PLAT_INTF_TYPE_SFPP:
                qsfp = FALSE;
                break;

            default:
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_PORT);
                break;
        }   /* end switch */

        if (qsfp)
        {
            nbytes = QSFP_EEPROM_SIZE; /* EEPROM size*/
            addr   = 0;
            page   = 0;

            FM_LOG_PRINT("Upper Memory Map: Page %d\n", page);

            status = fmPlatformXcvrEepromRead(sw,
                                              port,
                                              page,
                                              addr,
                                              eeprombuf,
                                              nbytes);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            fmPlatformHexDump(addr, eeprombuf, nbytes);

            fmPlatformXcvrEepromDumpBaseExt(eeprombuf, TRUE);


            FM_LOG_PRINT("Lower Memory Map:\n");
            /* Dump the status region */
            nbytes = QSFP_EEPROM_SIZE; /* EEPROM size*/
            addr   = 0;

            status = fmPlatformXcvrMemRead(sw,
                                           port,
                                           addr,
                                           addr,
                                           eeprombuf,
                                           nbytes);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            fmPlatformHexDump(addr, eeprombuf, nbytes);

            fmPlatformXcvrQsfpEepromDumpPage0(eeprombuf);
        }
        else
        {
            nbytes = SFPP_EEPROM_SIZE; /* EEPROM size*/
            addr   = 0;
            page   = 0;

            FM_LOG_PRINT("Page %d\n", page);

            status = fmPlatformXcvrEepromRead(sw,
                                              port,
                                              page,
                                              addr,
                                              eeprombuf,
                                              nbytes);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

            fmPlatformHexDump(addr, eeprombuf, nbytes);
            fmPlatformXcvrEepromDumpBaseExt(eeprombuf, FALSE);

            /* Only read if optical SFP */
            if ( eeprombuf[2] == 0x7 || eeprombuf[2] == 0xB)
            {
                addr = 0;
                page = 1;

                FM_LOG_PRINT("Page %d\n", page);

                status = fmPlatformXcvrEepromRead(sw,
                                                  port,
                                                  page,
                                                  addr,
                                                  eeprombuf,
                                                  nbytes);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

                fmPlatformHexDump(addr, eeprombuf, nbytes);
                fmPlatformXcvrSfppEepromDumpPage1(eeprombuf);
            }
        }
    }
    else
    {
        status = FM_OK;
        FM_LOG_PRINT("No module installed in port %d.\n", port);
    }


ABORT:
    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformDumpXcvrEeprom */




/*****************************************************************************/
/** fmPlatformDumpXcvrState
 * \ingroup intPlatform
 *
 * \desc            Dump transceiver state.
 *
 * \param[in]       sw is the sw number.
 *
 * \param[in]       port is the logical port number.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' codes as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformDumpXcvrState(fm_int sw, fm_int port)
{
    fm_status           status;
    fm_int              swNum;
    fm_uint32           hwResId;
    fm_uint32           xcvrStateValid;
    fm_uint32           xcvrState;
    fm_platformLib *    libFunc;
    fm_platformCfgPort *portCfg;


    FM_LOG_PRINT("Switch: %d Port: %2d\n", sw, port);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->GetPortXcvrState )
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
ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);
    fmPlatformMgmtDropSwitchLock(sw);

    if (status)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    if (xcvrStateValid & FM_PLAT_XCVR_ENABLE)
    {
        FM_LOG_PRINT(XCVR_STATE_FMT, "ENABLE",
                     (xcvrState & FM_PLAT_XCVR_ENABLE) ? "yes" : "no");
    }

    if (xcvrStateValid & FM_PLAT_XCVR_LPMODE)
    {
        FM_LOG_PRINT(XCVR_STATE_FMT, "LPMODE",
                     (xcvrState & FM_PLAT_XCVR_LPMODE) ? "yes" : "no");
    }

    if (xcvrStateValid & FM_PLAT_XCVR_PRESENT)
    {
        FM_LOG_PRINT(XCVR_STATE_FMT, "PRESENT",
                     (xcvrState & FM_PLAT_XCVR_PRESENT) ? "yes" : "no");
    }

    if (xcvrStateValid & FM_PLAT_XCVR_RXLOS)
    {
        FM_LOG_PRINT(XCVR_STATE_FMT, "RXLOS",
                     (xcvrState & FM_PLAT_XCVR_RXLOS) ? "yes" : "no");
    }

    if (xcvrStateValid & FM_PLAT_XCVR_TXFAULT)
    {
        FM_LOG_PRINT(XCVR_STATE_FMT, "TXFAULT",
                     (xcvrState & FM_PLAT_XCVR_TXFAULT) ? "yes" : "no");
    }

    if (xcvrStateValid & FM_PLAT_XCVR_INTR)
    {
        FM_LOG_PRINT(XCVR_STATE_FMT, "INTR",
                     (xcvrState & FM_PLAT_XCVR_INTR) ? "yes" : "no");
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformDumpXcvrState */




/*****************************************************************************/
/** fmPlatformPhyInternalToExternalPort
 * \ingroup intPlatform
 *
 * \desc            Get the PHY's external port number associated to the
 *                  specified internal port on the specified PHY.
 *
 * \param[in]       sw is the switch number on which to operate.
 * 
 * \param[in]       phyIdx is the PHY index on which to operate.
 *
 * \param[in]       port is the internal port number.
 *
 * \return          external port number or -1 if not found.
 *
 *****************************************************************************/
fm_int fmPlatformPhyInternalToExternalPort(fm_int sw,
                                           fm_int phyIdx,
                                           fm_int port)
{
    fm_gn2412LaneCfg *laneCfg;
    fm_int            extPort;
    fm_int            lane;

    extPort = -1;

    if ( phyIdx >= 0 && phyIdx < FM_PLAT_NUM_PHY(sw) )
    {
        for ( lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++ )
        {
            laneCfg = &FM_PLAT_GET_PHY_CFG(sw, phyIdx)->gn2412Lane[lane];

            if ( laneCfg->rxPort == port )
            {
                extPort = lane;
                break;
            }
        }
    }

    return extPort;

}   /* end fmPlatformPhyInternalToExternalPort */




/*****************************************************************************/
/** fmPlatformRetimerDumpInfo
 * \ingroup intPlatform
 *
 * \desc            Dump debug info for a given PHY/RETIMER.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       idx is the PHY/RETIMER index or the port index.
 *
 * \param[in]       cmd is the command to execude..
 * 
 * \param[in]       arg is any extra arguments.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformRetimerDumpInfo(fm_int  sw, 
                                    fm_int  idx, 
                                    fm_text cmd, 
                                    fm_text arg)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgPhy * phyCfg;
    fm_status           err;
    fm_byte             fwVersion[FM_GN2412_VERSION_NUM_LEN];
    fm_platformLib *    libFunc;
    fm_int              swNum;
    fm_int              phyIdx;

    err = FM_ERR_UNSUPPORTED;

    if ( (sw < 0) || (sw >= FM_PLAT_NUM_SW) )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Is idx a logical port number or PHY index */
    if ( arg != NULL && STR_EQ(arg, "port") )
    {
        /* Get the PHY index from the port */
        portCfg = fmPlatformCfgPortGet(sw, idx);
        if (!portCfg)
        {
            return FM_ERR_INVALID_PORT;
        }
        phyIdx = portCfg->phyNum;
    }
    else
    {
        phyIdx = idx;
    }

    if ( phyIdx < 0 || phyIdx > (FM_PLAT_NUM_PHY(sw) - 1) || cmd == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);
    swNum  = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;

    if ( phyCfg->model != FM_PLAT_PHY_GN2412 )
    {
        FM_LOG_PRINT("Unsupported PHY/RETIMER model\n");
        return FM_ERR_UNSUPPORTED;
    }

    if ( (err = TakeLocks(sw)) != FM_OK )
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    if ( libFunc->SelectBus )
    {
        err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    if (STR_EQ(cmd, "info"))
    {
        err = fmUtilGN2412GetFirmwareVersion((fm_uintptr) sw,
                                             PhyI2cWriteRead,
                                             phyCfg->addr,
                                             fwVersion);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, err);

        FM_LOG_PRINT("GN2412 retimer %d firmware version: %d.%d.%d.%d \n",
                     phyIdx,
                     fwVersion[0],
                     fwVersion[1],
                     fwVersion[2],
                     fwVersion[3]);

        fmUtilGN2412DumpTxEqualization((fm_uintptr) sw,
                                       PhyI2cWriteRead,
                                       phyCfg->addr);

        fmUtilGN2412DumpConnections((fm_uintptr) sw,
                                    PhyI2cWriteRead,
                                    phyCfg->addr);

        fmUtilGN2412DumpAppMode((fm_uintptr) sw,
                                PhyI2cWriteRead,
                                phyCfg->addr);
    }
    else if (STR_EQ(cmd, "imageVersion"))
    {
        err = fmUtilGN2412GetFirmwareVersion((fm_uintptr) sw,
                                             PhyI2cWriteRead,
                                             phyCfg->addr,
                                             fwVersion);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, err);

        FM_LOG_PRINT("GN2412 retimer %d firmware version: %d.%d.%d.%d \n",
                     phyIdx,
                     fwVersion[0],
                     fwVersion[1],
                     fwVersion[2],
                     fwVersion[3]);
    }
    else if (STR_EQ(cmd, "txEq"))
    {
        fmUtilGN2412DumpTxEqualization((fm_uintptr) sw,
                                       PhyI2cWriteRead,
                                       phyCfg->addr);
    }
    else if (STR_EQ(cmd, "status"))
    {
        fmUtilGN2412DumpAppStatus((fm_uintptr) sw,
                                  PhyI2cWriteRead,
                                  phyCfg->addr);
        fmUtilGN2412DumpAppRestartDiagCnt((fm_uintptr) sw,
                                          PhyI2cWriteRead,
                                          phyCfg->addr);
    }
    else if (STR_EQ(cmd, "app_status"))
    {
        if ( libFunc->SelectBus )
        {
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 0);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpAppStatusV2((fm_uintptr) sw,
                                        PhyI2cWriteRead,
                                        phyCfg->addr);
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 2);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpAppStatusV2((fm_uintptr) sw,
                                        PhyI2cWriteRead,
                                        phyCfg->addr);
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 1);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpAppStatusV2((fm_uintptr) sw,
                                        PhyI2cWriteRead,
                                        phyCfg->addr);
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 3);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpAppStatusV2((fm_uintptr) sw,
                                        PhyI2cWriteRead,
                                        phyCfg->addr);
            FM_LOG_PRINT("\n   ");
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 0);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpMissionCount((fm_uintptr) sw,
                                              PhyI2cWriteRead,
                                              phyCfg->addr);
    
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 2);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpMissionCount((fm_uintptr) sw,
                                              PhyI2cWriteRead,
                                              phyCfg->addr);
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 1);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpMissionCount((fm_uintptr) sw,
                                         PhyI2cWriteRead,
                                         phyCfg->addr);
    
            phyCfg = FM_PLAT_GET_PHY_CFG(sw, 3);
            err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
            fmUtilGN2412DumpMissionCount((fm_uintptr) sw,
                                         PhyI2cWriteRead,
                                         phyCfg->addr);
    
            FM_LOG_PRINT("\n");
        }
    }
    else if (STR_EQ(cmd, "statusL"))
    {
        fmUtilGN2412DumpAppRestartDiagCntV2((fm_uintptr) sw,
                                             PhyI2cWriteRead,
                                             phyCfg->addr);
    }
    else
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_PRINT("Valid retimers dump commands:    \n"
                     "info, imageVersion, and txEq\n");
    }

ABORT:
    DropLocks(sw);

    return err;

}   /* end fmPlatformRetimerDumpInfo */




/*****************************************************************************/
/* fmPlatformRetimerSetAppMode
 * \ingroup platformUtils
 *
 * \desc            Set the retimer application mode for the specified port.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the logical port number on which to operate.
 *
 * \param[in]       mode is the application mode to set. 
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformRetimerSetAppMode(fm_int sw, fm_int port, fm_int mode)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgPhy * phyCfg;
    fm_platformLib *    libFunc;
    fm_status           err;
    fm_uint32           hwResId;
    fm_int              phyIdx;
    fm_int              lane;
    fm_int              swNum;

    err = fmPlatformMapLogicalPortToPlatform(sw,
                                             port,
                                             &sw,
                                             &swNum,
                                             &hwResId,
                                             &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    phyIdx = portCfg->phyNum;

    if ( phyIdx < 0 || phyIdx > (FM_PLAT_NUM_PHY(sw) - 1) )
    {
        FM_LOG_PRINT("Port %d has no associated RETIMER\n", port);
        return FM_ERR_UNSUPPORTED;
    }

    phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);

    if ( phyCfg->model != FM_PLAT_PHY_GN2412 )
    {
        FM_LOG_PRINT("Unsupported PHY/RETIMER model\n");
        return FM_ERR_UNSUPPORTED;
    }

    if ( (err = TakeLocks(sw)) != FM_OK )
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    if ( libFunc->SelectBus )
    {
        err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /* Set the internal PHY's port */
    lane = portCfg->phyPort;

    FM_LOG_PRINT("Set app-mode %xh to internal port %d phy %d lane %d\n",
                 mode,
                 port,
                 phyIdx,
                 lane);

    err = fmUtilGN2412SetAppMode((fm_uintptr) sw,
                                 PhyI2cWriteRead,
                                 phyCfg->addr,
                                 lane,
                                 mode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, err);

    /* Get the corresponding external PHY's port */
    lane = 
        fmPlatformPhyInternalToExternalPort(sw, phyIdx, portCfg->phyPort);

    FM_LOG_PRINT("Set app-mode %xh to external port %d phy %d lane %d\n",
                 mode,
                 port,
                 phyIdx,
                 lane);

    err = fmUtilGN2412SetAppMode((fm_uintptr) sw,
                                 PhyI2cWriteRead,
                                 phyCfg->addr,
                                 lane,
                                 mode);

ABORT:
    DropLocks(sw);

    return err;

}   /* end fmPlatformRetimerSetAppMode */




/*****************************************************************************/
/* fmPlatformRetimerSetLaneTxEq
 * \ingroup platformUtils
 *
 * \desc            Set the Tx equalization coefficients (de-emphasis) for the
 *                  specified port.
 *
 * \param[in]       sw is the switch number on which to operate.
 *
 * \param[in]       port is the logical port number on which to operate.
 *
 * \param[in]       internal is a flag to indicate if the coefficient should
 *                  be set on the phy's port connected to the switch (internal)
 *                  or on the phy's port connected to the external side (SFP).
 * 
 * \param[in]       polarity is TX polarity.
 * 
 * \param[in]       preTap is the Transmitter Cm coefficient (pre-tap). 
 * 
 * \param[in]       attenuation is the attenuation setting for transmitter 
 *                  (output swing). 
 * 
 * \param[in]       postTap is the Transmitter C1 coefficient (post-tap). 
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformRetimerSetLaneTxEq(fm_int sw,
                                       fm_int port,
                                       fm_int internal,
                                       fm_int polarity,
                                       fm_int preTap,
                                       fm_int attenuation,
                                       fm_int postTap)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgPhy * phyCfg;
    fm_platformLib *    libFunc;
    fm_status           err;
    fm_uint32           hwResId;
    fm_int              phyIdx;
    fm_int              lane;
    fm_int              swNum;
    fm_int              currentPolarity;
    fm_int              currentPreTap;
    fm_int              currentAtt;
    fm_int              currentPostTap;

    err = fmPlatformMapLogicalPortToPlatform(sw,
                                             port,
                                             &sw,
                                             &swNum,
                                             &hwResId,
                                             &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    phyIdx = portCfg->phyNum;

    if ( phyIdx < 0 || phyIdx > (FM_PLAT_NUM_PHY(sw) - 1) )
    {
        FM_LOG_PRINT("Port %d has no associated RETIMER\n", port);
        return FM_ERR_UNSUPPORTED;
    }

    phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);

    if ( phyCfg->model != FM_PLAT_PHY_GN2412 )
    {
        FM_LOG_PRINT("Unsupported PHY/RETIMER model\n");
        return FM_ERR_UNSUPPORTED;
    }

    if (internal)
    {
        lane = portCfg->phyPort;
    }
    else
    {
        /* Get the corresponding external PHY's port */
        lane = 
            fmPlatformPhyInternalToExternalPort(sw, phyIdx, portCfg->phyPort);
    }

    if ( lane < 0 || lane >= FM_GN2412_NUM_LANES )
    {
        FM_LOG_PRINT("PHY's port number %d is out of range (%d..%d)\n",
                     lane,
                     0,
                     FM_GN2412_NUM_LANES-1);
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( (err = TakeLocks(sw)) != FM_OK )
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    if ( libFunc->SelectBus )
    {
        err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    /* Get current coefficient */
    err = fmUtilGN2412GetLaneTxEq((fm_uintptr) sw,
                                  PhyI2cWriteRead,
                                  phyCfg->addr,
                                  lane,
                                  &currentPolarity,
                                  &currentPreTap,
                                  &currentAtt,
                                  &currentPostTap);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);

    /* If one parameter is passed as -1 then use the current config. */

    polarity = (polarity < 0) ? currentPolarity : polarity;
    preTap   = (preTap < 0)   ? currentPreTap   : preTap;
    postTap  = (postTap < 0)  ? currentPostTap  : postTap;
    attenuation = (attenuation < 0) ? currentAtt : attenuation;

    FM_LOG_PRINT("Set port %d (phy %d lane %d) "
                 "preTap %d att %d postTap %d pol %xh\n",
                 port,
                 phyIdx,
                 lane,
                 preTap,
                 attenuation,
                 postTap,
                 polarity);

    err = fmUtilGN2412SetLaneTxEq((fm_uintptr) sw,
                                  PhyI2cWriteRead,
                                  phyCfg->addr,
                                  lane,
                                  polarity,
                                  preTap,
                                  attenuation,
                                  postTap);

ABORT:
    DropLocks(sw);

    return err;

}   /* end fmPlatformRetimerSetLaneTxEq */




/*****************************************************************************/
/* fmPlatformRetimerRegisterRead 
 * \ingroup platformUtils
 *
 * \desc            Read a value from a retimer register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       phyIdx is the retimer number on which to operate.
 *
 * \param[in]       reg is the register address
 *
 * \param[out]      value is a pointer to a caller-allocated area where this
 *                  function will return the content of the register 
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformRetimerRegisterRead(fm_int     sw, 
                                        fm_int     phyIdx,
                                        fm_int     reg, 
                                        fm_uint32 *value)
{
    fm_platformCfgPhy * phyCfg;
    fm_platformLib *    libFunc;
    fm_status           err;
    fm_int              swNum;
    fm_byte             val;

    if ( phyIdx < 0 || phyIdx > (FM_PLAT_NUM_PHY(sw) - 1) )
    {
        FM_LOG_PRINT("Invalid PHY/RETIMER number (%d)\n", phyIdx);
        return FM_ERR_INVALID_ARGUMENT;
    }

    phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);
    swNum  = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;

    if ( phyCfg->model != FM_PLAT_PHY_GN2412 )
    {
        FM_LOG_PRINT("Unsupported PHY/RETIMER model\n");
        return FM_ERR_UNSUPPORTED;
    }

    if ( (err = TakeLocks(sw)) != FM_OK )
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    if ( libFunc->SelectBus )
    {
        err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    err = fmUtilGN2412RegisterRead((fm_uintptr) sw,
                                   PhyI2cWriteRead,
                                   phyCfg->addr,
                                   (fm_uint16)reg,
                                   &val);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, err);

    *value = val;

ABORT:
    DropLocks(sw);

    return err;

}   /* end fmPlatformRetimerRegisterRead */




/*****************************************************************************/
/* fmPlatformRetimerRegisterWrite
 * \ingroup platformUtils
 *
 * \desc            Write a value to a retimer register.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       phyIdx is the retimer number on which to operate.
 *
 * \param[in]       reg is the register address.
 *
 * \param[out]      value is the value to write .
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformRetimerRegisterWrite(fm_int   sw, 
                                         fm_int    phyIdx,
                                         fm_int    reg, 
                                         fm_uint32 value)
{
    fm_platformCfgPhy * phyCfg;
    fm_platformLib *    libFunc;
    fm_status           err;
    fm_int              swNum;

    if ( phyIdx < 0 || phyIdx > (FM_PLAT_NUM_PHY(sw) - 1) )
    {
        FM_LOG_PRINT("Invalid PHY/RETIMER number (%d)\n", phyIdx);
        return FM_ERR_INVALID_ARGUMENT;
    }

    phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);
    swNum  = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;

    if ( phyCfg->model != FM_PLAT_PHY_GN2412 )
    {
        FM_LOG_PRINT("Unsupported PHY/RETIMER model\n");
        return FM_ERR_UNSUPPORTED;
    }

    if ( (err = TakeLocks(sw)) != FM_OK )
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, err);
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);
    if ( libFunc->SelectBus )
    {
        err = libFunc->SelectBus(swNum, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    err = fmUtilGN2412RegisterWrite((fm_uintptr) sw,
                                    PhyI2cWriteRead,
                                    phyCfg->addr,
                                    (fm_uint16)reg,
                                    value);
ABORT:
    DropLocks(sw);

    return err;

}   /* end fmPlatformRetimerRegisterWrite */



/*****************************************************************************/
/** fmPlatformRetimerDumpPortMap
 * \ingroup intPlatform
 *
 * \desc            Dump the retimer port mapping.
 *
 * \param[in]       sw is the switch number on which to operate.
 * 
 * \param[in]       port is the port number (-1 => all ports).
 *
 * \return          NONE
 *
 *****************************************************************************/
fm_status fmPlatformRetimerDumpPortMap(fm_int sw, fm_int port)
{
    fm_platformCfgPort *portCfg;
    fm_int              portIdx;
    fm_int              extPort;

    if ( port == -1 )
    {
        FM_LOG_PRINT("Logical  Retimer  Internal  External\n");
        FM_LOG_PRINT(" port      num      port      port  \n");

        /* Display all ports */
        for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
        {
            portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
            if ( portCfg->phyNum < 0 || 
                 portCfg->phyNum > (FM_PLAT_NUM_PHY(sw) - 1) )
            {
                /* No retimer associated to this port */
                continue;
            }

            /* Get the corresponding external PHY's port */
            extPort = fmPlatformPhyInternalToExternalPort(sw, 
                                                          portCfg->phyNum,
                                                          portCfg->phyPort);

            FM_LOG_PRINT("  %2d       %2d        %2d        %2d\n",
                         portCfg->port,
                         portCfg->phyNum,
                         portCfg->phyPort,
                         extPort);
        }
    }
    else
    {
        portCfg = fmPlatformCfgPortGet(sw, port);
        if (!portCfg)
        {
            FM_LOG_PRINT("Invalid port %d\n", port);
            return FM_ERR_INVALID_PORT;
        }

        if ( portCfg->phyNum < 0 || 
             portCfg->phyNum > (FM_PLAT_NUM_PHY(sw) - 1) )
        {
            FM_LOG_PRINT("Port %d has no associated RETIMER\n", port);
            return FM_ERR_INVALID_PORT;
        }

        FM_LOG_PRINT("Logical  Retimer   port #    port #\n");
        FM_LOG_PRINT(" port      num     to RRC    to SFP\n");

        /* Get the corresponding external PHY's port */
        extPort = fmPlatformPhyInternalToExternalPort(sw, 
                                                      portCfg->phyNum,
                                                      portCfg->phyPort);

        FM_LOG_PRINT("  %2d       %2d        %2d        %2d\n",
                     portCfg->port,
                     portCfg->phyNum,
                     portCfg->phyPort,
                     extPort);
    }
 
    return FM_OK;

}   /* end fmPlatformRetimerDumpPortMap */

