/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_port.c
 * Creation Date:   June 2, 2014
 * Description:     Platform-specific port functions.
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

#define MAX_BUF_SIZE       256


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
/** EthModeToBitRate
 * \ingroup intPort
 *
 * \desc            This function returns the lane bit rate associated to a
 *                  given ethernet interface mode.
 *
 * \param[in]       ethMode is the ethernet interface mode
 *
 * \return          The lane bitrate (see ''fm_platSerdesBitRate'')
 * 
 *****************************************************************************/
static fm_platSerdesBitRate EthModeToBitRate(fm_ethMode ethMode)
{
    fm_platSerdesBitRate bitRate;

    switch ( ethMode )
    {
        case FM_ETH_MODE_XLAUI:
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_40GBASE_SR4:
            bitRate = FM_PLAT_SERDES_BITRATE_10G;
            break;

        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_100GBASE_KR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_SR4:
            bitRate = FM_PLAT_SERDES_BITRATE_25G;
            break;

        default:
            bitRate = FM_PLAT_SERDES_BITRATE_1G;
            break;

    }   /* end switch ( ethMode ) */

    return bitRate;

}   /* end EthModeToBitRate */




/*****************************************************************************/
/** NotifyEthModeChange
 * \ingroup intPlatform
 *
 * \desc            Called when ethernet mode change. This function can
 *                  be used to update default MAC or SERDES attributes that
 *                  are dependent on ethernet mode.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       mac is the mac number.
 *
 * \param[in]       mode is the new ethernet mode applied to the port.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status NotifyEthModeChange(fm_int     sw,
                                     fm_int     port,
                                     fm_int     mac,
                                     fm_ethMode mode)
{
    fm_status           status = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, 
                 "sw=%d, port=%d, mode=0x%x\n",
                 sw,
                 port,
                 mode);

    FM_NOT_USED(mac);

    fmPlatformMgmtNotifyEthModeChange(sw, port, mode);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end NotifyEthModeChange */




/*****************************************************************************/
/** GetPortSerdesTxCfg
 * \ingroup intPlatform
 *
 * \desc            Returns the SERDES preCursor, cursor and postCursor for
 *                  the specified port,lane and interface mode.
 * 
 *                  This assumes the port and lane number have been validated
 *                  by the caller.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       lane is the lane number.
 *
 * \param[in]       ethMode is the Ethernet interface mode.
 *
 * \param[out]      cursor points to storage where the SERDES tx cursor
 *                  will be written. Cannot be NULL.
 *
 * \param[out]      preCursor points to storage where the SERDES tx preCursor
 *                  will be written.  Cannot be NULL.
 *
 * \param[out]      postCursor points to storage where the SERDES tx postCursor
 *                  will be written.  Cannot be NULL.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status GetPortSerdesTxCfg(fm_int     sw,
                                    fm_int     port,
                                    fm_int     lane,
                                    fm_ethMode ethMode,
                                    fm_int *   cursor,
                                    fm_int *   preCursor,
                                    fm_int *   postCursor)
{
    fm_platSerdesBitRate bitRate;
    fm_platformCfgPort * portCfg;
    fm_platformCfgLane * laneCfg;
    fm_platformXcvrType  xcvrType;
    fm_int               xcvrLen;
    fm_int               appCfg;
    fm_status            status;
    fm_char              buf[MAX_BUF_SIZE+1];

    portCfg = fmPlatformCfgPortGet(sw, port);
    if (!portCfg)
    {
        return FM_ERR_INVALID_PORT;
    }

    status = fmPlatformMgmtGetTransceiverType(sw, port, &xcvrType, &xcvrLen);
    if (status)
    {
        return status;
    }

    laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);
    bitRate = EthModeToBitRate(ethMode);

    FM_SNPRINTF_S(buf,
                  MAX_BUF_SIZE,
                  FM_AAK_API_PLATFORM_KEEP_SERDES_CFG,
                  sw);
    appCfg = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_KEEP_SERDES_CFG);


    /* Get cursor value */
    if ( appCfg && (laneCfg->appCfgState & FM_STATE_SERDES_TX_CURSOR) )
    {
        /* Use the saved value */
        *cursor = laneCfg->appCfg.cursor;
    }
    else
    {
        if ( xcvrType == FM_PLATFORM_XCVR_TYPE_OPTICAL )
            *cursor = laneCfg->optical[bitRate].cursor;
        else
            *cursor = laneCfg->copper[bitRate].cursor;
    }

    /* Get preCursor value */
    if ( appCfg && (laneCfg->appCfgState & FM_STATE_SERDES_TX_PRECURSOR) )
    {
        /* Use the saved value */
        *preCursor = laneCfg->appCfg.preCursor;
    }
    else
    {
        if ( xcvrType == FM_PLATFORM_XCVR_TYPE_OPTICAL )
            *preCursor = laneCfg->optical[bitRate].preCursor;
        else
            *preCursor = laneCfg->copper[bitRate].preCursor;
    }

    /* Get postCursor value */
    if ( appCfg && (laneCfg->appCfgState & FM_STATE_SERDES_TX_POSTCURSOR) )
    {
        /* Use the saved value */
        *postCursor = laneCfg->appCfg.postCursor;
    }
    else
    {
        if ( xcvrType == FM_PLATFORM_XCVR_TYPE_OPTICAL )
            *postCursor = laneCfg->optical[bitRate].postCursor;
        else
            *postCursor = laneCfg->copper[bitRate].postCursor;
    }

    return FM_OK;

}   /* end GetPortSerdesTxCfg */




/*****************************************************************************/
/** SetInitialLaneTxEq
 * \ingroup intPlatform
 *
 * \desc            Set the initial lane Tx Equalization attributes
 *                  for the given port.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       portCfg is the pointer to the port config.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status SetInitialLaneTxEq(fm_int sw, fm_platformCfgPort *portCfg)
{
    fm_platformCfgPort * pCfg;
    fm_int               portIdx;
    fm_status            status;
    fm_bool              allLane;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    /**************************************************
     * For Quad port the cfg MUST be set using 
     * FM_PLAT_INTF_TYPE_QSFP_LANE0 only.
     **************************************************/

    if ( (portCfg->ethMode & FM_ETH_MODE_4_LANE_BIT_MASK) != 0 )
    {
        allLane = TRUE;
    }
    else if (portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3)
    {
        /* QSFP_LANE0, SFPP or NONE */
        allLane = FALSE;
    }
    else
    {
        /* QSFP LANE1, LANE2 or LANE3 */
        if (portCfg->ethMode == FM_ETH_MODE_DISABLED)
        {
            /* See if LANE0 for this quad port is set to 4-LANE eth mode */
            portIdx = 
                FM_PLAT_GET_SWITCH_CFG(sw)->epls[portCfg->epl].laneToPortIdx[0];
            pCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

            if ((pCfg->ethMode & FM_ETH_MODE_4_LANE_BIT_MASK) != 0 )
            {
                /* Config has been set when processing LANE0 */
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
            }
        }
        allLane = FALSE;
    }

    status = fmPlatformSetPortSerdesTxCfg(sw, 
                                          portCfg->port, 
                                          allLane, 
                                          portCfg->ethMode);

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end SetInitialLaneTxEq */




/*****************************************************************************/
/** SetLanePolarity
 * \ingroup intPlatform
 *
 * \desc            Set the lane polarity for the given port.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       portCfg is the pointer to the port config.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
static fm_status SetLanePolarity(fm_int sw, fm_platformCfgPort *portCfg)
{
    fm_platformCfgLane *laneCfg;
    fm_platformCfgPort *pCfg;
    fm_int              portIdx;
    fm_status           status;
    fm_int              port;
    fm_int              mac;
    fm_int              lane;
    fm_int              numLane;
    fm_int              polarity;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);

    status = FM_OK;
    mac = 0;
    port = portCfg->port;

    /**************************************************
     * For Quad port the polarity MUST be set using 
     * FM_PLAT_INTF_TYPE_QSFP_LANE0 only.
     **************************************************/

    if ( (portCfg->ethMode & FM_ETH_MODE_4_LANE_BIT_MASK) != 0 )
    {
        numLane = FM_PLAT_LANES_PER_EPL;
    }
    else if (portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
             portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3)
    {
        /* QSFP_LANE0, SFPP or NONE */
        numLane = 1;
    }
    else
    {
        /* QSFP LANE1, LANE2 or LANE3 */
        if (portCfg->ethMode == FM_ETH_MODE_DISABLED)
        {
            /* See if LANE0 for this quad port is set to 4-LANE eth mode */
            portIdx = 
                FM_PLAT_GET_SWITCH_CFG(sw)->epls[portCfg->epl].laneToPortIdx[0];
            pCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

            if ((pCfg->ethMode & FM_ETH_MODE_4_LANE_BIT_MASK) != 0 )
            {
                /* Polarity has been set when processing LANE0 */
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);
            }
        }
        numLane = 1;
    }

    for (lane = 0 ; lane < numLane ; lane++)
    {
        laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);

        /* Set the RX lane polarity. */

        polarity = (laneCfg->lanePolarity & FM_POLARITY_INVERT_RX) ? 1 : 0;

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Set rx polarity, port %d, lane %d: %d\n",
                         port,
                         lane,
                         polarity );

        status = fmSetPortAttributeV2(sw,
                                      port,
                                      mac,
                                      lane,
                                      FM_PORT_RX_LANE_POLARITY,
                                      &polarity);
        if (status != FM_OK)
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Unable to set rx polarity: port %d, lane %d\n",
                             port,
                             lane );
            break;
        }

        /* Set the TX lane polarity. */

        polarity = (laneCfg->lanePolarity & FM_POLARITY_INVERT_TX) ? 1 : 0;

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Set tx polarity, port %d, lane %d: %d\n",
                         port,
                         lane,
                         polarity );

        status = fmSetPortAttributeV2(sw,
                                      port,
                                      mac,
                                      lane,
                                      FM_PORT_TX_LANE_POLARITY,
                                      &polarity);
        if (status != FM_OK)
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Unable to set tx polarity: port %d, lane %d\n",
                             port,
                             lane );
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end SetLanePolarity */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/* fmPlatformGetPortCapabilities
 * \ingroup platform
 *
 * \desc            Returns a mask of capabilities for the given physical port.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       physPort is the physical port number.
 *
 * \param[out]      capabilities points to storage where a bitmap of
 *                  capabilities will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either sw or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformGetPortCapabilities(fm_int     sw,
                                        fm_int     physPort,
                                        fm_uint32 *capabilities)
{
    fm_platformCfgPort *portCfg;
    fm_switch *         switchPtr;
    fm_status           status;
    fm_int              logSw;
    fm_int              port;

    if (sw < FM_FIRST_FOCALPOINT || sw >= FM_PLAT_NUM_SW)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if (capabilities == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    *capabilities = 0;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    if ( (physPort < 0) || (physPort > switchPtr->maxPhysicalPort) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    status = fmPlatformMapPhysicalPortToLogical(sw, physPort, &logSw, &port);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    portCfg = fmPlatformCfgPortGet(sw, port);

    if (portCfg)
    {
        if ( FM_PLAT_PORT_IS_LAG_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_LAG_CAPABLE;
        }

        if ( FM_PLAT_PORT_IS_ROUTE_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_CAN_ROUTE;
        }

        if ( FM_PLAT_PORT_IS_10M_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_10M;
        }

        if ( FM_PLAT_PORT_IS_100M_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_100M;
        }

        if ( FM_PLAT_PORT_IS_1G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_1G;
        }

        if ( FM_PLAT_PORT_IS_2PT5G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_2PT5G;
        }

        if ( FM_PLAT_PORT_IS_5G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_5G;
        }

        if ( FM_PLAT_PORT_IS_10G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_10G;
        }

        if ( FM_PLAT_PORT_IS_20G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_20G;
        }

        if ( FM_PLAT_PORT_IS_25G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_25G;
        }

        if ( FM_PLAT_PORT_IS_40G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_40G;
        }

        if ( FM_PLAT_PORT_IS_100G_CAP(portCfg) )
        {
            *capabilities |= FM_PORT_CAPABILITY_SPEED_100G;
        }

    }   /* end if (portCfg) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformGetPortCapabilities */




/*****************************************************************************/
/** fmPlatformMapLogicalPortToPhysical
 * \ingroup platform
 *
 * \desc            Maps a logical port number to a (switch, physical port)
 *                  tuple.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *                                                                      \lb\lb
 *                  The (switch, physical port) tuple identifies the physical
 *                  port number of a particular switch device, as
 *                  identified on the pin-out of the device.
 *
 * \param[in]       logicalSwitch is the logical switch number.
 *
 * \param[in]       logicalPort is the logical port number.
 *
 * \param[out]      switchNum points to storage where the switch number of the
 *                  (switch, physical port) tuple should be stored.
 *
 * \param[out]      physPort points to storage where the physical port number
 *                  of the (switch, physical port) tuple should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapLogicalPortToPhysical(fm_int  logicalSwitch,
                                             fm_int  logicalPort,
                                             fm_int *switchNum,
                                             fm_int *physPort)
{
    fm_status           err = FM_OK;
    fm_platformCfgPort *portCfg;
#if FM_SUPPORT_SWAG
    fm_int              realSw;
    fm_int              realPort;
#endif

    /* NOTE: Logging is removed to reduce overhead */

    if ( (logicalSwitch >= FM_FIRST_FOCALPOINT) && (logicalSwitch < FM_PLAT_NUM_SW) )
    {
        /* physical switch */
        *switchNum = logicalSwitch;

        if (logicalPort <= 
            FM_PLAT_GET_SWITCH_CFG(logicalSwitch)->maxLogicalPortValue)
        {
            portCfg = fmPlatformCfgPortGet(logicalSwitch, logicalPort);

            if (portCfg)
            {
                *physPort = portCfg->physPort;
            }
            else
            {
                err = FM_ERR_INVALID_PORT;
            }
        }
        else
        {
            err = FM_ERR_INVALID_PORT;
        }
    }
#if FM_SUPPORT_SWAG
    else
    {
        /* Switch-aggregate logical switch */
        err = fmGetSwitchAndPortForSWAGPort(logicalSwitch,
                                            logicalPort,
                                            &realSw,
                                            &realPort);

        if (err != FM_OK)
        {
            return err;
        }

        err = fmPlatformMapLogicalPortToPhysical(realSw,
                                                 realPort,
                                                 switchNum,
                                                 physPort);
    }
#else
    else
    {
        return FM_ERR_INVALID_SWITCH;
    }
#endif
    return err;

}   /* end fmPlatformMapLogicalPortToPhysical */




/*****************************************************************************/
/** fmPlatformMapPhysicalPortToLogical
 * \ingroup platform
 *
 * \desc            Maps a (switch, physical port) tuple to a logical port
 *                  number.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *                                                                      \lb\lb
 *                  The (switch, physical port) tuple identifies the physical
 *                  port number of a particular switch device, as
 *                  identified on the pin-out of the device.
 *
 * \param[in]       switchNum is the switch number of the (switch, physical
 *                  port) tuple.
 *
 * \param[in]       physPort is the physical port number of the (switch,
 *                  physical port) tuple.
 *
 * \param[out]      logicalSwitch points to storage where the corresponding
 *                  logical switch number should be stored.
 *
 * \param[out]      logicalPort points to storage where the corresponding
 *                  logical port number should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_PORT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapPhysicalPortToLogical(fm_int  switchNum,
                                             fm_int  physPort,
                                             fm_int *logicalSwitch,
                                             fm_int *logicalPort)
{
    fm_platformState *ps;

    /* NOTE: Logging is removed to reduce overhead */

    if (switchNum < FM_FIRST_FOCALPOINT || switchNum >= FM_PLAT_NUM_SW)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    ps = GET_PLAT_STATE(switchNum);

    if (physPort >= 0 && physPort < ps->maxPorts)
    {
        *logicalSwitch = switchNum;
        *logicalPort   = ps->physicalToLogicalPortMap[physPort];

        if (*logicalPort < 0)
        {
            return FM_ERR_INVALID_PORT;
        }

        return FM_OK;
    }

    return FM_ERR_INVALID_PORT;

}   /* end fmPlatformMapPhysicalPortToLogical */




/*****************************************************************************/
/* fmPlatformMapLogicalPortToPlatform
 * \ingroup platform
 *
 * \desc            Maps a logical port number to a (swNum, hwResId) tuple.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *                                                                      \lb\lb
 *                  The (swNum, hwResId) tuple identifies the hardware 
 *                  resource for port of a particular switch device that is
 *                  consistent for the platform shared library to use.
 *
 * \param[in]       sw is the logical switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[out]      swNum points to storage where the platform switch number
 *                  should be stored.
 *
 * \param[out]      hwResId points to storage where the hardware resource ID
 *                  for the given port.
 *
 * \param[out]      portCfg points to storage where the fm_platformCfgPort structure
 *                  for the given port should be stored, if requested.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either sw or port are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapLogicalPortToPlatform(fm_int               sw,
                                             fm_int               port,
                                             fm_int *             swNum,
                                             fm_uint32 *          hwResId,
                                             fm_platformCfgPort **portCfg)
{
    fm_status           err = FM_OK;
    fm_platformCfgPort *pcfg;

    /* NOTE: Logging is removed to reduce overhead */

    if ( (swNum == NULL) || (hwResId == NULL))
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    if ( (sw >= FM_FIRST_FOCALPOINT) && (sw < FM_PLAT_NUM_SW) )
    {
        *swNum = FM_PLAT_GET_SWITCH_CFG(sw)->swNum;

        if (port <= FM_PLAT_GET_SWITCH_CFG(sw)->maxLogicalPortValue)
        {
            pcfg = fmPlatformCfgPortGet(sw, port);

            if (pcfg)
            {
                *hwResId = pcfg->hwResourceId;

                if (portCfg)
                {
                    *portCfg = pcfg;
                }
            }
            else
            {
                err = FM_ERR_INVALID_PORT;
            }
        }
        else
        {
            err = FM_ERR_INVALID_PORT;
        }
    }
    else
    {
        return FM_ERR_INVALID_SWITCH;
    }

    return err;

}   /* end fmPlatformMapLogicalPortToPlatform */




/*****************************************************************************/
/** fmMapPortLaneToEplLane
 * \ingroup platform
 *
 * \desc            Maps a logical port lane number to a EPL lane number.
 *                                                                      \lb\lb
 *                  Logical port numbers are used by system end users to
 *                  identify ports in the system CLI or other user interface
 *                  and typically match labelling on the system chassis.
 *                  Logical port numbers are system-global for systems that
 *                  consist of more than one switch device.
 *
 * \param[in]       sw is the logical switch number.
 *
 * \param[in]       logicalPort is the logical port number.
 *
 * \param[in]       portLaneNum is the logical port lane number.
 *
 * \param[out]      eplLane points to storage where the EPL lane number
 *                  should be stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if either switchNum or physPort are
 *                  not valid values.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformMapPortLaneToEplLane(fm_int  sw,
                                         fm_int  logicalPort,
                                         fm_int  portLaneNum,
                                         fm_int *eplLane)
{
    fm_status           err = FM_ERR_INVALID_PORT;
    fm_platformCfgPort *portCfg;

    if ( sw < FM_FIRST_FOCALPOINT || sw >= FM_PLAT_NUM_SW )
    {
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    if ( (portLaneNum < 0) || 
         (portLaneNum > (FM_PLAT_LANES_PER_EPL-1)) || 
         (eplLane == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, err);
    }

    portCfg = fmPlatformCfgPortGet(sw, logicalPort);

    if ( portCfg )
    {
        *eplLane = portCfg->lane[portLaneNum];
        err = FM_OK;
    }

ABORT:
    return err;

}   /* end fmMapPortLaneToEplLane */




/*****************************************************************************/
/* fmPlatformNotifyPortState
 * \ingroup platform
 *
 * \desc            Notify port state change from application configuration or
 *                  from port event. This function is called by the API whenever
 *                  the port state changes and could be used by the platform to
 *                  implement some actions depending on port state. An example
 *                  is changing the LED status for that port and the
 *                  transceiver enable state.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       mac is the mac number, if applicable.
 *
 * \param[in]       isConfig specifies whether the state is notified from the
 *                  port configuration or from port event notification.
 *
 * \param[in]       state indicates the port's state (see 'Port States') to
 *                  show.
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformNotifyPortState(fm_int  sw,
                                    fm_int  port,
                                    fm_int  mac,
                                    fm_bool isConfig,
                                    fm_int  state)
{
    fm_int      portIdx;
    fm_bool     disabled;

    FM_NOT_USED(mac);

    if (sw < FM_FIRST_FOCALPOINT || sw >= FM_PLAT_NUM_SW)
    {
        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
    }

    portIdx = fmPlatformCfgPortGetIndex(sw, port);
    if (portIdx < 0)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* Disable transceiver if port state is powered down */
    if (isConfig)
    {
        disabled = (state == FM_PORT_MODE_ADMIN_PWRDOWN);

        if (disabled != GET_PLAT_STATE(sw)->xcvrInfo[portIdx].disabled)
        {
            fmPlatformXcvrEnable(sw, port, !disabled);
        }
    }

    return fmPlatformLedSetPortState(sw, port, isConfig, state);

}   /* end fmPlatformNotifyPortState */




/*****************************************************************************/
/* fmPlatformNotifyPortAttribute
 * \ingroup platform
 *
 * \desc            This function is called by the API whenever one of the
 *                  following port attributes is configured by the
 *                  application:
 *
 *                  FM_PORT_TX_LANE_CURSOR, FM_PORT_TX_LANE_PRECURSOR
 *                  and FM_PORT_TX_LANE_POSTCURSOR.
 * 
 *                  This allow to the platform code to save the attribute
 *                  values and use them when fmPlatformMgmtNotifyEthModeChange
 *                  is called upon a FM_PORT_ETHERNET_INTERFACE_MODE change.
 * 
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       port is the port on which to operate.
 *
 * \param[in]       mac is the mac number, if applicable.
 *
 * \param[in]       lane is the port's zero-based lane number on which to
 *                  operate. May be specified as FM_PORT_LANE_NA for non-lane
 *                  oriented attributes.
 *
 * \param[in]       attribute is the port attribute (see 'Port Attributes')
 *                  to set.
 *
 * \param[in]       value points to the attribute value to set.
 *
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fmPlatformNotifyPortAttribute(fm_int sw,
                                        fm_int port,
                                        fm_int mac,
                                        fm_int lane,
                                        fm_int attribute,
                                        void * value)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgLane *laneCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d, port=%d lane=%d attr=%d\n",
                 sw, port, lane, attribute);

    FM_NOT_USED(mac);

    portCfg = FM_PLAT_GET_PORT_CFG(sw, port);
    laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);

    if ( ( attribute == FM_PORT_TX_LANE_CURSOR ||
           attribute == FM_PORT_TX_LANE_PRECURSOR ||
           attribute == FM_PORT_TX_LANE_POSTCURSOR ) &&
         ( (laneCfg->appCfgState & FM_STATE_SERDES_IN_PROGRESS) == 0 ) )
        {
            /* Save the value and set a bit indicating
               the value has been saved. */
            switch (attribute)
            {
                case FM_PORT_TX_LANE_CURSOR:
                    laneCfg->appCfg.cursor = *(fm_int *)value;
                    laneCfg->appCfgState |= FM_STATE_SERDES_TX_CURSOR;
                    break;

                case FM_PORT_TX_LANE_PRECURSOR:
                    laneCfg->appCfg.preCursor = *(fm_int *)value;
                    laneCfg->appCfgState |= FM_STATE_SERDES_TX_PRECURSOR;
                    break;

                case FM_PORT_TX_LANE_POSTCURSOR:
                    laneCfg->appCfg.postCursor = *(fm_int *)value;
                    laneCfg->appCfgState |= FM_STATE_SERDES_TX_POSTCURSOR;
                    break;
            }
        }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformNotifyPortAttribute */




/*****************************************************************************/
/* fmPlatformInitPortTables
 * \ingroup intPlatform
 *
 * \desc            Initializes the platform port mapping tables.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformInitPortTables(fm_int sw)
{
    fm_platformState *    ps;
    fm_platformCfgPort *  portCfg;
    fm_platformCfgSwitch *swCfg;
    fm_int                portIdx;
    fm_int                physPort;
    fm_int                logPort;
    fm_int                nbytes;
    errno_t               err;
    fm_status             status;

    ps = GET_PLAT_STATE(sw);
    swCfg = FM_PLAT_GET_SWITCH_CFG(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM,
                 "sw=%d, maxPorts=%d maxLogicalPort=%d\n",
                 sw, 
                 ps->maxPorts, 
                 swCfg->maxLogicalPortValue);

    ps->lportToPortTableIndex = NULL;
    status = FM_OK;

    /***************************************************
     * Allocate physical to logical port table.
     **************************************************/

    nbytes = sizeof(fm_int) * ps->maxPorts;

    ps->physicalToLogicalPortMap = fmAlloc(nbytes);
    if (ps->physicalToLogicalPortMap == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    err = FM_MEMSET_S(ps->physicalToLogicalPortMap, nbytes, -1, nbytes);
    if (err != 0)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_FAIL);
    }


    /***************************************************
     * Allocate logical port to portIndex table.
     **************************************************/

    nbytes = sizeof(fm_int) * (swCfg->maxLogicalPortValue + 1);

    ps->lportToPortTableIndex = (fm_int *) fmAlloc(nbytes);
    if (ps->lportToPortTableIndex == NULL)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_ERR_NO_MEM);
    }

    err = FM_MEMSET_S(ps->lportToPortTableIndex, nbytes, -1, nbytes);
    if (err != 0)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status = FM_FAIL);
    }

    /***************************************************
     * Generate the port mappings for faster lookup
     **************************************************/

    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);

        logPort  = portCfg->port;
        physPort = portCfg->physPort;

        /* Port sanity check */
        if (logPort < 0 || logPort > swCfg->maxLogicalPortValue)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid logical port %d \n"
                         "Valid range: 0 <= logPort <= %d\n",
                         logPort,
                         swCfg->maxLogicalPortValue);

            status = FM_ERR_INVALID_PORT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        if (physPort < 0 && physPort >= ps->maxPorts)
        {
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                         "Invalid physical port %d \n"
                         "Valid range: 0 <= physPort < %d\n",
                         physPort,
                         ps->maxPorts);

            status = FM_ERR_INVALID_PORT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PLATFORM, status);
        }

        ps->lportToPortTableIndex[logPort] = portIdx;
        ps->physicalToLogicalPortMap[physPort] = logPort;
    }

    return FM_OK;

ABORT:
    if (ps->physicalToLogicalPortMap != NULL)
    {
        fmFree(ps->physicalToLogicalPortMap);
        ps->physicalToLogicalPortMap = NULL;
    }

    if (ps->lportToPortTableIndex != NULL)
    {
        fmFree(ps->lportToPortTableIndex);
        ps->lportToPortTableIndex = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fmPlatformInitPortTables */




/*****************************************************************************/
/** fmPlatformSetPortSerdesTxCfg
 * \ingroup intPlatform
 *
 * \desc            Set SERDES TX Config based on port ethMode and module
 *                  type.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       port is the logical port number.
 *
 * \param[in]       applyToAllLane indicate whether it applies to all lanes
 *                  or just to one lane.
 *
 * \param[out]      mode is the port ethMode.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformSetPortSerdesTxCfg(fm_int     sw,
                                       fm_int     port,
                                       fm_bool    applyToAllLane,
                                       fm_ethMode mode)
{
    fm_platformCfgPort *portCfg;
    fm_platformCfgLane *laneCfg;
    fm_status           status;
    fm_int              cursor;
    fm_int              preCursor;
    fm_int              postCursor;
    fm_int              lane;
    fm_int              numLanes;

    MOD_TYPE_DEBUG("SetPortSerdesTxCfg: port %d:%d ethMode: %s allLane: %s\n",
                   sw,
                   port,
                   fmPlatformGetEthModeStr(mode),
                   (applyToAllLane) ? "TRUE" : "FALSE");

    portCfg = fmPlatformCfgPortGet(sw, port);

    if (portCfg == NULL)
    {
        return FM_ERR_INVALID_PORT;
    }

    numLanes = (applyToAllLane) ? 4 : 1;

    for (lane = 0 ; lane < numLanes ; lane++)
    {
        status = GetPortSerdesTxCfg(sw,
                                    port,
                                    lane,
                                    mode,
                                    &cursor,
                                    &preCursor,
                                    &postCursor);

        if (status != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                         "Unable to get SERDES config for port %d\n", port);
            return status;
        }

        MOD_TYPE_DEBUG("TxCfg: port %d:%d.%d cursor %d "
                       "preCursor %d postCursor %d\n",
                       sw,
                       port,
                       lane, 
                       cursor,
                       preCursor,
                       postCursor);

        laneCfg = FM_PLAT_GET_LANE_CFG(sw, portCfg, lane);

        /* Set a bit to indicate the attribute is being set by the
           platform so the value will not be saved. */
        laneCfg->appCfgState |= FM_STATE_SERDES_IN_PROGRESS;

        /* Set the TX lane cursor. */
        status = fmSetPortAttributeV2(sw,
                                      port,
                                      0,
                                      lane,
                                      FM_PORT_TX_LANE_CURSOR,
                                      &cursor);
        if (status != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                            port,
                            "Unable to set Tx lane cursor for "
                            "port %d, lane %d\n",
                            port,
                            lane);
        }

        /* Set the TX lane preCursor. */
        status = fmSetPortAttributeV2(sw,
                                      port,
                                      0,
                                      lane,
                                      FM_PORT_TX_LANE_PRECURSOR,
                                      &preCursor);
        if (status != FM_OK)
        {
            FM_LOG_ERROR_V2(FM_LOG_CAT_PORT,
                            port,
                            "Unable to set Tx lane preCursor for "
                            "port %d, lane %d\n",
                            port,
                            lane);
        }

        /* Set the TX lane postCursor. */
        status = fmSetPortAttributeV2(sw,
                                      port,
                                      0,
                                      lane,
                                      FM_PORT_TX_LANE_POSTCURSOR,
                                      &postCursor);
        if (status != FM_OK)
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Unable to set Tx lane postCursor for "
                             "port %d, lane %d\n",
                             port,
                             lane );
        }

        /* Clear the bit set above */
        laneCfg->appCfgState &= ~FM_STATE_SERDES_IN_PROGRESS;
    }

    return status;

} /* end fmPlatformSetPortSerdesTxCfg */




/*****************************************************************************/
/** fmPlatformPortInitialize
 * \ingroup intPlatform
 *
 * \desc            Called to send the initial port configuration to the API.
 *
 * \param[in]       sw is the switch number
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformPortInitialize(fm_int sw)
{
    fm_platformCfgPort *portCfg;
    fm_port *           portEntry;
    fm_status           status;
    fm_int              portIdx;
    fm_int              port;
    fm_int              mac;
    fm_int              anEnabled;
    fm_dfeMode          dfeMode;
    fm_text             anString;
    fm_bool             restoreDisableState;
    fm_int              configuredPorts;
    fm_ethMode          ethMode;
    fm_uint64           anBasePage;

    FM_LOG_ENTRY(FM_LOG_CAT_PLATFORM, "sw = %d\n", sw);


    /**************************************************
     * Initialize the physical PHY interface. Must be
     * called first as the PHY structure will be used
     * upon call of NotifyEthModeChange.
     **************************************************/

    status = fmInitializePhysicalInterfaces(sw, 
                                            fmRootApi->fmSwitchStateTable[sw]);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /**************************************************
     * Initialize the XCVR and read the module EEPROM 
     * to determine the presence and the type of the 
     * module.
     **************************************************/

    status = fmPlatformMgmtXcvrInitialize(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_PLATFORM, status);

    /**************************************************
     * Send the default port configuration read from
     * the platform configuration file.
     **************************************************/

    /* Dummy mac as it is not used on FM10000 */
    mac = 0;
    configuredPorts = 0;
    for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(sw) ; portIdx++)
    {
        portCfg = FM_PLAT_GET_PORT_CFG(sw, portIdx);
        port     = portCfg->port;

        /* Send port configuration for EPL ports only. */ 
        if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
        {
            configuredPorts++;
            continue;
        }

        /* Set the ethernet mode. */
        restoreDisableState = FALSE;
        if (portCfg->ethMode != FM_ETH_MODE_DISABLED)
        {
            status = fmSetPortAttributeV2(sw,
                                          port,
                                          mac,
                                          FM_PORT_LANE_NA,
                                          FM_PORT_ETHERNET_INTERFACE_MODE,
                                          &portCfg->ethMode);

            if (status != FM_OK)
            {
                FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Error setting port %d into Ethernet "
                                 "mode %s!\n",
                                 port,
                                 fmPlatformGetEthModeStr(portCfg->ethMode));
            }
        }

        /* Get notification from API for ethernet mode change,
         * so the platform can update any attributes specific
         * to each ethernet mode
         */
        portEntry = GET_PORT_PTR(sw, port);
        portEntry->phyInfo.notifyEthModeChange = NotifyEthModeChange;

        status = SetLanePolarity(sw, portCfg);
        if ( status != FM_OK )
        {
            restoreDisableState = TRUE;
        }

        status = SetInitialLaneTxEq(sw, portCfg);
        if ( status != FM_OK )
        {
            restoreDisableState = TRUE;
        }


        if ( status == FM_OK )
        {
            if (portCfg->ethMode == FM_ETH_MODE_AN_73)
            {
                /* Enable AN if ethMode is AN-73 */
                anEnabled = FM_PORT_AUTONEG_CLAUSE_73;
                anString  = "Clause 73";
                status = fmSetPortAttributeV2(sw,
                                              port,
                                              mac,
                                              FM_PORT_LANE_NA,
                                              FM_PORT_AUTONEG,
                                              &anEnabled);
            }
            else if (portCfg->ethMode == FM_ETH_MODE_SGMII)
            {
                /* Enable AN if ethMode is SGMII */
                anEnabled = FM_PORT_AUTONEG_SGMII;
                anString  = "SGMII";
                status = fmSetPortAttributeV2(sw,
                                              port,
                                              mac,
                                              FM_PORT_LANE_NA,
                                              FM_PORT_AUTONEG,
                                              &anEnabled);
            }

            if ( status != FM_OK )
            {
                FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Error configuring autoneg type %s "
                                 "on port %d\n",
                                 anString,
                                 port );
                restoreDisableState = TRUE;
            }
        }

        if (portCfg->ethMode == FM_ETH_MODE_AN_73)
        {
            /* Set the basepage ethMode is AN-73 */
            anBasePage = portCfg->an73Ability | 0x4001;
            status = fmSetPortAttributeV2(sw,
                                          port,
                                          mac,
                                          FM_PORT_LANE_NA,
                                          FM_PORT_AUTONEG_BASEPAGE,
                                          &anBasePage);
            if ( status != FM_OK )
            {
                FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Error configuring autoneg basepage "
                                 "on port %d\n",
                                 port );
                restoreDisableState = TRUE;
            }
        }

        dfeMode = portCfg->dfeMode;
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Set DFE mode %d on port %d\n",
                         dfeMode,
                         port);

        status = fmSetPortAttributeV2(sw,
                                      port,
                                      mac,
                                      FM_PORT_LANE_ALL,
                                      FM_PORT_DFE_MODE,
                                      &dfeMode );
        if ( status != FM_OK )
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Error setting port %d DFE mode %d!\n",
                             port,
                             dfeMode );
            restoreDisableState = TRUE;
        }

        if ( status == FM_OK )
        {
            configuredPorts++;
        }
        else
        {
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                             port,
                             "Port %d will be kept DISABLED\n", 
                             port );

            if ( restoreDisableState )
            {
                ethMode = FM_ETH_MODE_DISABLED;
                fmSetPortAttributeV2( sw,
                                      port,
                                      mac,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_ETHERNET_INTERFACE_MODE,
                                      &ethMode );
            }
        }

        status = FM_OK;
    }

    /* Fatal error if none of the ports was succesfully configured */
    if ( ( configuredPorts == 0 ) && ( FM_PLAT_NUM_PORT(sw) != 0 ) )
    {
        FM_LOG_WARNING( FM_LOG_CAT_PLATFORM,
                        "No ports configured on switch %d\n",
                        sw );
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, status);

}   /* end fmPlatformPortInitialize */

