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

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define DISP_LINE_LEN     16
#define MAX_STR_LEN       128
#define XCVR_EEPROM_SIZE  256
#define SFPP_EEPROM_SIZE  256
#define QSFP_EEPROM_SIZE  128

#define XCVR_STATE_FMT    "%15s: %s\n"

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

} /* end fmPlatformPrintBytes */




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

} /* end fmPlatformHexDump */




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

} /* fmPlatformDoDebug */




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
        } /* switch */

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

            fmPlatformXcvrEepromDumpBaseExt(eeprombuf);


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
            fmPlatformXcvrEepromDumpBaseExt(eeprombuf);

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
                                                &swNum,
                                                &hwResId,
                                                &portCfg);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (portCfg->intfType == FM_PLAT_INTF_TYPE_NONE)
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
