/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_phy.c
 * Creation Date:   June 2, 2014
 * Description:     Platform PHY functions.
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
#include <platforms/util/fm_util.h>
#include <platforms/util/retimer/fm_util_gn2412.h>
#include <platforms/util/fm_util_device_lock.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/
#define SFPP_PHY_ADDR 0x56

/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** WriteSfppPhy
 * \ingroup intPlatform
 *
 * \desc            Write a 16-bit value to SFP+ PHY register, asssuming
 *                  proper bus has been selected.
 *
 * \param[in]       sw is the switch to operate on.
 *
 * \param[in]       swNum is the switch number to operate on.
 *
 * \param[in]       offset is the register offset.
 *
 * \param[out]      value is a 16-bit value to write.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status WriteSfppPhy(fm_int sw, fm_int swNum, fm_int offset, fm_int value)
{
    fm_status       status;
    fm_byte         i2cReg[3];
    fm_int          i2cDev;
    fm_platformLib *libFunc;

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->I2cWriteRead )
    {
        return FM_ERR_UNSUPPORTED;
    }

    i2cDev = SFPP_PHY_ADDR;

    i2cReg[0] = offset;
    i2cReg[1] = (value >> 8) & 0xFF;
    i2cReg[2] = (value >> 0) & 0xFF;

    status = libFunc->I2cWriteRead(swNum, i2cDev, i2cReg, 3, 0);

    FM_LOG_DEBUG(FM_LOG_CAT_PORT_AUTONEG,
                 "PHY Addr 0x%x: data 0x%02x%02x%02x. status %d\n",
                 i2cDev, i2cReg[0], i2cReg[1], i2cReg[2], status);

    return status;

}   /* end WriteSfppPhy */




/*****************************************************************************/
/** AutoNegEnable1000BaseTPhy
 * \ingroup intPlatform
 *
 * \desc            Enables auto negotiation for 1000BaseT phys.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       physPort is the physical port of the phy port.
 *
 * \param[in]       mac is the mac number of physical port. Not used on
 *                  Liberty Trail.
 *
 * \param[out]      pPort is the port state structure.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status AutoNegEnable1000BaseTPhy(fm_int   sw,
                                           fm_int   physPort,
                                           fm_int   mac,
                                           fm_port *pPort)
{
    fm_status  status;
    fm_int     port;

    FM_NOT_USED(pPort);
    FM_NOT_USED(mac);

    status = fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                        physPort, &port);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

    status = fmPlatformMgmtConfigSfppXcvrAutoNeg(sw, port, TRUE);
    if (status == FM_ERR_UNSUPPORTED)
    {
        /* No support, then just call the function directly */
        fmPlatformPhyEnable1000BaseTAutoNeg(sw, port, TRUE);

        /* Cannot return error here since we don't know if this port
         * has a PHY that can be access via I2C.
         */
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT_AUTONEG, status);

}   /* end AutoNegEnable1000BaseTPhy */




/*****************************************************************************/
/** AutoNegDisable1000BaseTPhy
 * \ingroup intPlatform
 *
 * \desc            Disables auto negotiation for 1000BaseT phys.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       physPort is the physical port of the phy port.
 *
 * \param[in]       mac is the mac number of physical port. Not used on
 *                  Liberty Trail.
 *
 * \param[out]      pPort is the port state structure.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status AutoNegDisable1000BaseTPhy(fm_int   sw,
                                            fm_int   physPort,
                                            fm_int   mac,
                                            fm_port *pPort)
{
    fm_status  status;
    fm_int     port;

    FM_NOT_USED(pPort);
    FM_NOT_USED(mac);

    status = fmMapPhysicalPortToLogical(GET_SWITCH_PTR(sw),
                                        physPort, &port);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

    status = fmPlatformMgmtConfigSfppXcvrAutoNeg(sw, port, FALSE);
    if (status == FM_ERR_UNSUPPORTED)
    {
        /* No support, then just call the function directly */
        fmPlatformPhyEnable1000BaseTAutoNeg(sw, port, FALSE);

        /* Cannot return error here since we don't know if this port
         * has a PHY that can be access via I2C.
         */
        status = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_PORT_AUTONEG, status);

}   /* end AutoNegDisableBase1000TPhy */



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
/** InitializeGN2412
 * \ingroup intPlatform
 *
 * \desc            Initializes the GN2412 retimer
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       phyIdx is the retimer number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status InitializeGN2412(fm_int sw, fm_int phyIdx)
{
    fm_platformCfgPhy *  phyCfg;
    fm_status            status;
    fm_byte              fwVersion[FM_GN2412_VERSION_NUM_LEN];
    fm_byte              errCode0;
    fm_byte              errCode1;
    fm_gn2412Cfg         cfg;
    fm_platformLib *     libFunc;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY, "sw=%d, phyIdx=%d\n", sw, phyIdx);

    status = FM_OK;

    phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);
    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ((status = fmPlatformMgmtTakeSwitchLock(sw)) != FM_OK)
    {
         FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    if (GET_PLAT_PROC_STATE(sw)->fileLock >= 0)
    {
        fmUtilDeviceLockTake(GET_PLAT_PROC_STATE(sw)->fileLock);
    }

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    if ( libFunc->SelectBus )
    {
        status = libFunc->SelectBus(sw, FM_PLAT_BUS_PHY, phyCfg->hwResourceId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
    }

    status = fmUtilGN2412GetFirmwareVersion((fm_uintptr) sw,
                                            PhyI2cWriteRead,
                                            phyCfg->addr,
                                            fwVersion);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, status);

    FM_LOG_PRINT("GN2412 retimer %d firmware version: %d.%d.%d.%d \n",
                 phyIdx,
                 fwVersion[0],
                 fwVersion[1],
                 fwVersion[2],
                 fwVersion[3]);

    if ( fwVersion[0] != FM_GN2412_FW_VER_MAJOR ||
         fwVersion[1] != FM_GN2412_FW_VER_MINOR )
    {
        FM_LOG_ERROR(FM_LOG_CAT_PHY, 
                     "Unsupported GN2412 retimer (index %d) firmware version\n"
                     "  Firmware version: %d.%d.%d.%d \n"
                     "  Expected version: %d.%d.x.x\n",
                     phyIdx,
                     fwVersion[0],
                     fwVersion[1],
                     fwVersion[2],
                     fwVersion[3],
                     FM_GN2412_FW_VER_MAJOR,
                     FM_GN2412_FW_VER_MINOR);

        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT(FM_LOG_CAT_PHY, status);
    }

    /* Read the boot error code (val,error_string) */

    status = fmUtilGN2412GetBootErrorCode((fm_uintptr) sw,
                                          PhyI2cWriteRead,
                                          phyCfg->addr,
                                          &errCode0,
                                          &errCode1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, status);

    if ( errCode0 != 0 || errCode1 != 0 )
    {
        FM_LOG_ERROR(FM_LOG_CAT_PHY, 
                     "The GN2412 retimer (index %d) failed to boot \n"
                     "  ERROR_CODE_0: 0x%02x \n"
                     "  ERROR_CODE_1: 0x%02x \n",
                     phyIdx,
                     errCode0,
                     errCode1);

        status = FM_FAIL;
        FM_LOG_ABORT(FM_LOG_CAT_PHY, status);
    }

    /* Get the default timebase 0 configuration for 156.25MHz / 5156,25MHz */
    status = fmUtilGN2412GetTimebaseCfg(0,
                                        FM_GN2412_REFCLK_156P25,
                                        FM_GN2412_OUTFREQ_5156P25,
                                        &cfg.timebase[0]);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, status);

    /* Get the default timebase 1 configuration for 156.25MHz / 5156,25MHz */
    status = fmUtilGN2412GetTimebaseCfg(1,
                                        FM_GN2412_REFCLK_156P25,
                                        FM_GN2412_OUTFREQ_5156P25,
                                        &cfg.timebase[1]);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, status);

    /* Select refClk_1 input pin for both timebases */
    cfg.timebase[0].refClkSel = 1;
    cfg.timebase[1].refClkSel = 1;

    cfg.setTxEq = 
        (FM_PLAT_GET_SWITCH_CFG(sw)->enablePhyDeEmphasis) ? TRUE : FALSE;

    FM_MEMCPY_S(cfg.lane, 
                sizeof(cfg.lane), 
                phyCfg->gn2412Lane, 
                sizeof(phyCfg->gn2412Lane));

    status = fmUtilGN2412Initialize((fm_uintptr) sw,
                                    PhyI2cWriteRead,
                                    phyCfg->addr,
                                    &cfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PHY, status);

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    if (GET_PLAT_PROC_STATE(sw)->fileLock >= 0)
    {
        fmUtilDeviceLockDrop(GET_PLAT_PROC_STATE(sw)->fileLock);
    }

    fmPlatformMgmtDropSwitchLock(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end InitializeGN2412 */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmInitializePhysicalInterfaces
 * \ingroup intPlatform
 *
 * \desc            Initializes Physical PHY interfaces for a platform
 *
 * \param[in]       sw is the switch number
 *
 * \param[in,out]   pSwitch points to the switch state table
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmInitializePhysicalInterfaces(fm_int     sw,
                                         fm_switch *pSwitch)
{
    fm_status           status = FM_OK;
    fm_int              portIdx;
    fm_platformCfgPort *portCfg;
    fm_int              port;
    fm_port            *portEntry;
    fm_platformCfgPhy * phyCfg;
    fm_int              phyIdx;

    FM_LOG_ENTRY(FM_LOG_CAT_PHY,
                 "sw=%d, pSwitch=%p\n",
                 sw,
                 (void *) pSwitch);

    for ( phyIdx = 0 ; phyIdx < FM_PLAT_NUM_PHY(sw) ; phyIdx++ )
    {
        phyCfg = FM_PLAT_GET_PHY_CFG(sw, phyIdx);

        if ( phyCfg->model == FM_PLAT_PHY_GN2412 )
        {
            status = InitializeGN2412(sw, phyIdx);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PHY, status);
        }
    }

    /* Assume first one is for CPU */
    for (portIdx = 1 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
        port = portCfg->port;

        portEntry = GET_PORT_PTR(sw, port);
        if (!portEntry)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PHY,
                         "NULL portEntry for port %d", port);
            continue;
        }

        portEntry->phyInfo.phyType = FM_PHY_TYPE_NOT_PRESENT;
        portEntry->phyInfo.phyOutputEnable = NULL;

        if (portCfg->intfType == FM_PLAT_INTF_TYPE_SFPP)
        {
            /* For SGMII/1000BaseT SFP+ module */
            portEntry->phyInfo.phyAutoNegDisable = AutoNegDisable1000BaseTPhy;
            portEntry->phyInfo.phyAutoNegEnable  = AutoNegEnable1000BaseTPhy;
        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_PHY, status);

}   /* end fmInitializePhysicalInterfaces */



/*****************************************************************************/
/** fmPlatformPhyEnable1000BaseTAutoNeg
 * \ingroup intPlatform
 *
 * \desc            Enable/Disable auto negotiation for 1000BaseT PHY.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port of the PHY port.
 *
 * \param[in]       enable is whether to enable or disable auto negotiation.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformPhyEnable1000BaseTAutoNeg(fm_int   sw,
                                              fm_int   port,
                                              fm_bool  enable)
{
    fm_status  status;
    fm_bool    isSGMII;
    fm_ethMode ethMode;
    fm_int     swNum;
    fm_uint32  hwResId;
    fm_platformLib *libFunc;

    FM_LOG_ENTRY(FM_LOG_CAT_PORT_AUTONEG,
                 "sw=%d, port=%d, enable=%d\n",
                 sw, port, enable);

    status = fmGetPortAttributeV2(sw, 
                                  port,
                                  0,
                                  FM_PORT_LANE_NA,
                                  FM_PORT_ETHERNET_INTERFACE_MODE,
                                  &ethMode);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

    switch (ethMode)
    {
        case FM_ETH_MODE_SGMII:
            isSGMII = TRUE;
            break;

        case FM_ETH_MODE_1000BASE_X:
            isSGMII = FALSE;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_PORT_AUTONEG, FM_OK);
            break;

    }   /* end switch (ethMode) */

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &swNum,
                                                &hwResId,
                                                NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->SelectBus )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_EEPROM, hwResId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    if (enable)
    {
        /* Refer to Marvell 88E1111 datasheet */
        if (isSGMII)
        {
            /* Flip to fiber page */
            status = WriteSfppPhy(sw, swNum, 0x16, 0x1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

            /* Enable SGMII auto neg */
            status = WriteSfppPhy(sw, swNum, 0x0, 0x1140);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);
            fmDelay(0, 500*1000*1000);

            /* Flip to copper page */
            status = WriteSfppPhy(sw, swNum, 0x16, 0x0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

            /* Also enable 10/100 speed, only full-duplex
             * FM6000 does not support half-duplex */
            status = WriteSfppPhy(sw, swNum, 0x4, 0x0150);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

            /* Enable SGMII mode */
            status = WriteSfppPhy(sw, swNum, 0x1b, 0x9084);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);
        }
        else
        {
            /* 1000BaseT */
            /* Flip to fiber page */
            status = WriteSfppPhy(sw, swNum, 0x16, 0x1);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

            /* Enable auto neg */
            status = WriteSfppPhy(sw, swNum, 0x0, 0x1140);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);
            fmDelay(0, 500*1000*1000);

            /* Flip to copper page */
            status = WriteSfppPhy(sw, swNum, 0x16, 0x0);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

            /* Disable 10/100 speed */
            status = WriteSfppPhy(sw, swNum, 0x4, 0x0c01);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

            /* Enable 1000BaseT mode */
            status = WriteSfppPhy(sw, swNum, 0x1b, 0x9088);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);
        }

        /* Do soft reset for config to take effect */
        status = WriteSfppPhy(sw, swNum, 0x0, 0x9140);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);
    }
    else
    {
        /* Flip to fiber page */
        status = WriteSfppPhy(sw, swNum, 0x16, 0x1);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

        /* Disable auto neg */
        status = WriteSfppPhy(sw, swNum, 0x0, 0x0140);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

        /* Flip to copper page */
        status = WriteSfppPhy(sw, swNum, 0x16, 0x0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

        /* Do soft reset with AN disable for config to take effect */
        status = WriteSfppPhy(sw, swNum, 0x0, 0x8140);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);
    }

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PORT_AUTONEG, status);

} /* end fmPlatformPhyEnable1000BaseTAutoNeg */




/*****************************************************************************/
/** fmPlatformPhyDump1000BaseT
 * \ingroup intPlatform
 *
 * \desc            Dump 1000BaseT PHY registers.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the logical port of the PHY port.
 *
 * \param[in]       page is the PHY page select.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformPhyDump1000BaseT(fm_int   sw,
                                     fm_int   port,
                                     fm_int   page)
{
    fm_status  status;
    fm_int     swNum;
    fm_uint32  hwResId;
    fm_int     reg;
    fm_byte    data[64];
    fm_platformLib *libFunc;

    FM_LOG_PRINT("Port %d Page %d:\n", port, page);

    status = fmPlatformMapLogicalPortToPlatform(sw,
                                                port,
                                                &swNum,
                                                &hwResId,
                                                NULL);
    if (status)
    {
        FM_LOG_PRINT("%s: Unable to map switch %d port %d.\n",
                     fmErrorMsg(status), sw, port);
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);
    }

    libFunc = FM_PLAT_GET_LIB_FUNCS_PTR(sw);

    if ( !libFunc->SelectBus )
    {
        FM_LOG_PRINT("No SelectBus function supported\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    if ( !libFunc->I2cWriteRead )
    {
        FM_LOG_PRINT("No I2cWriteRead function supported\n");
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_UNSUPPORTED);
    }

    TAKE_PLAT_I2C_BUS_LOCK(sw);

    status = libFunc->SelectBus(swNum, FM_PLAT_BUS_XCVR_EEPROM, hwResId);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /* Select page */
    status = WriteSfppPhy(sw, swNum, 0x16, page ? 1 : 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT_AUTONEG, status);

    data[0] = 0; /* start at reg 0 */

    status = libFunc->I2cWriteRead(swNum, SFPP_PHY_ADDR, data, 1, 64);
    if (status)
    {
        FM_LOG_PRINT("%s: I2C Access failed.\n", fmErrorMsg(status));
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    for (reg = 0 ; reg < 64 ; reg ++)
    {
        FM_LOG_PRINT("%02x", data[reg]);
        if (reg % 2 == 1)
        {
            FM_LOG_PRINT(" ");
        }
        if (reg % 16 == 15)
        {
            FM_LOG_PRINT("\n");
        }
    }

ABORT:
    DROP_PLAT_I2C_BUS_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

} /* end fmPlatformPhyDump1000BaseT */
