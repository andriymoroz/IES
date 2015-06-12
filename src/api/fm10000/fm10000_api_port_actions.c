/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_port_actions.c
 * Creation Date:   December 13, 2013
 * Description:     Action callbacks for the FM10000 port state machines
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

/* 1000BASE-X EEE Tsl timeout: 20 us. See Table 36-8 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_1000BASEX_TSL_TIMEOUT     200
#define FM10000_PORT_EEE_1000BASEX_TSL_TIMESCALE     0

/* 1000BASE-X EEE Tql timeout: 2.55 ms. See Table 36-8 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_1000BASEX_TQL_TIMEOUT     255 
#define FM10000_PORT_EEE_1000BASEX_TQL_TIMESCALE     2

/* 1000BASE-X EEE Tul timeout: 20 us. See Table 36-8 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_1000BASEX_TUL_TIMEOUT     200
#define FM10000_PORT_EEE_1000BASEX_TUL_TIMESCALE     0

/* 1000BASE-X EEE Tqr timeout: 3.5 ms. See Table 36-9 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_1000BASEX_TQR_TIMEOUT      35
#define FM10000_PORT_EEE_1000BASEX_TQR_TIMESCALE     3

/* 1000BASE-X EEE Twr timeout: 11 us. See Table 36-9 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_1000BASEX_TWR_TIMEOUT     220
#define FM10000_PORT_EEE_1000BASEX_TWR_TIMESCALE     0

/* 1000BASE-X EEE Twtf timeout: 1 ms. See Table 36-9 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_1000BASEX_TWTF_TIMEOUT     10
#define FM10000_PORT_EEE_1000BASEX_TWTF_TIMESCALE    3

/* 10GBASE-R EEE Alert Pattern */
#define FM10000_PORT_EEE_10GBASER_PATTERN   0xff00ff00

/* 10GBASE-R EEE Tsl timeout: 5 us. See Table 49-2 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_TSL_TIMEOUT      100
#define FM10000_PORT_EEE_10GBASER_TSL_TIMESCALE      0

/* 10GBASE-R EEE Tql timeout: 1.75 ms. See Table 49-2 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_TQL_TIMEOUT      175
#define FM10000_PORT_EEE_10GBASER_TQL_TIMESCALE      2

/* 10GBASE-R EEE Twl timeout: 11 us. See Table 49-2 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_TWL_TIMEOUT      220
#define FM10000_PORT_EEE_10GBASER_TWL_TIMESCALE      0

/* 10GBASE-R EEE T1u timeout: 1.2 us. See Table 49-2 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_T1U_TIMEOUT       24
#define FM10000_PORT_EEE_10GBASER_T1U_TIMESCALE      0

/* 10GBASE-R EEE Tqr timeout: 2.5 ms. See Table 49-3 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_TQR_TIMEOUT       25
#define FM10000_PORT_EEE_10GBASER_TQR_TIMESCALE      3

/* 10GBASE-R EEE Twr timeout: 11.5 us. See Table 49-3 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_TWR_TIMEOUT      230
#define FM10000_PORT_EEE_10GBASER_TWR_TIMESCALE      0

/* 10GBASE-R EEE Twtf timeout: 10 ms. See Table 49-3 in IEEE 802.3-2012-D3.1 */
#define FM10000_PORT_EEE_10GBASER_TWTF_TIMEOUT     100
#define FM10000_PORT_EEE_10GBASER_TWTF_TIMESCALE     3

/* Low Power Idle Hold timer */
#define FM10000_PORT_EEE_TX_LPI_HOLD_TIMEOUT        20
#define FM10000_PORT_EEE_TX_LPI_HOLD_TIMESCALE       1 /* 1us scale */


/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

static fm_status GetTxFifoConfig( fm_ethMode ethMode, 
                                  fm_uint *  pTxAntiBubbleWatermarkValue,
                                  fm_uint *  pTxRateFifoWatermarkValue,
                                  fm_uint *  pTxRateFifoSlowIncValue,
                                  fm_uint *  pTxRateFifoFastIncValue);

static fm_status SetFabricLoopbackFlags(fm_int     sw,
                                        fm_switch *switchPtr,
                                        fm_int     port,
                                        fm_int     ethMode,
                                        fm_bool    enableFabricLoopback);
static fm_status SetDrainMode( fm10000_portSmEventInfo *userInfo, 
                               fm_bool                  enabled );
static fm_status SerDesEventReq( fm_smEventInfo *eventInfo, 
                                 void           *userInfo,
                                 fm_int          eventId );
static fm_int ConvertAdminModeToTxLinkFault( fm_portMode mode );

static fm10000_laneBitrate EthModeToBitRate( fm_ethMode ethMode, 
                                             fm_uint32 speed );

static fm_status UnlinkAllLanes( fm10000_port *portExt );

static fm_status SendAnEventReq( fm_smEventInfo *eventInfo, 
                                 void           *userInfo,
                                 fm_int          eventId );

static fm_status SetTxFaultModeInMacCfg( fm_int sw,
                                         fm_int port,
                                         fm_int epl,
                                         fm_int physLane,
                                         fm_int mode );

static fm_status SetPepLoopback( fm_smEventInfo *eventInfo,
                                 void           *userInfo,
                                 fm_bool         enabled );

static fm_status ConfigureEeeLane( fm_smEventInfo *eventInfo,
                                   void           *userInfo );

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** GetTxFifoConfig
 * \ingroup intPort
 *
 * \desc            This function returns the configuration for the tx fifo
 *                  associated to a given ethernet interface mode.
 *
 * \param[in]       ethMode is the ethernet interface mode
 * 
 * \param[in]       pTxAntiBubbleWatermarkValue is a pointer to a user-allocated
 *                  storage where this function will return the fifo anti-bubble
 *                  watermark value for the given ethMode.
 * 
 * \param[in]       pTxRateFifoWatermarkValue is a pointer to a user-allocated
 *                  storage where this function will return the fifo watermark
 *                  value for the given ethMode.
 * 
 * \param[in]       pTxRateFifoSlowIncValue is a pointer to a user-allocated
 *                  storage where this function will return the value of
 *                  TxRateFifoSlowInc for the given ethMode.
 *
 * \param[in]       pTxRateFifoFastIncValue is a pointer to a user-allocated
 *                  storage where this function will return the value of
 *                  TxRateFifoFastInc for the given ethMode.
 *
 * \return          FM_OK if successful 
 * 
 *****************************************************************************/
fm_status GetTxFifoConfig( fm_ethMode ethMode,
                           fm_uint *  pTxAntiBubbleWatermarkValue,
                           fm_uint *  pTxRateFifoWatermarkValue,
                           fm_uint *  pTxRateFifoSlowIncValue,
                           fm_uint *  pTxRateFifoFastIncValue)
{
    fm_status   err;

    
    err = FM_OK;

    if (pTxAntiBubbleWatermarkValue == NULL ||
        pTxRateFifoWatermarkValue   == NULL ||
        pTxRateFifoSlowIncValue     == NULL ||
        pTxRateFifoFastIncValue     == NULL)
    {
        /* invalid parameter */
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ERROR(FM_LOG_CAT_PORT, "Invalid argument");
    }
    else
    {
        /* Note Well: these set of values has been empirically determined.
         * Do not modify these values if they cannot be validated properly */
        switch ( ethMode )
        {
            case FM_ETH_MODE_SGMII:
            case FM_ETH_MODE_1000BASE_X:
            case FM_ETH_MODE_1000BASE_KX:
            {
                *pTxAntiBubbleWatermarkValue = 4;
                *pTxRateFifoWatermarkValue   = 3;
                *pTxRateFifoSlowIncValue     = 1;
                *pTxRateFifoFastIncValue     = 2;
                break;
            }
            case FM_ETH_MODE_2500BASE_X:
            {
                *pTxAntiBubbleWatermarkValue = 4;
                *pTxRateFifoWatermarkValue   = 3;
                *pTxRateFifoSlowIncValue     = 3;
                *pTxRateFifoFastIncValue     = 4;
                break;
            }
            case FM_ETH_MODE_10GBASE_CR:
            case FM_ETH_MODE_10GBASE_SR:
            case FM_ETH_MODE_10GBASE_KR:
            {
                *pTxAntiBubbleWatermarkValue = 3;
                *pTxRateFifoWatermarkValue   = 3;
                *pTxRateFifoSlowIncValue     = 12;
                *pTxRateFifoFastIncValue     = 13;
                break;
            }
            case FM_ETH_MODE_AN_73:
            {
                *pTxAntiBubbleWatermarkValue = 3;
                *pTxRateFifoWatermarkValue   = 3;
                *pTxRateFifoSlowIncValue     = 3;
                *pTxRateFifoFastIncValue     = 4;
                break;
            }
            case FM_ETH_MODE_25GBASE_SR:
            case FM_ETH_MODE_25GBASE_KR:
            {
                *pTxAntiBubbleWatermarkValue = 6;
                *pTxRateFifoWatermarkValue   = 3;
                *pTxRateFifoSlowIncValue     = 32;
                *pTxRateFifoFastIncValue     = 33;
                break;
            }
            case FM_ETH_MODE_40GBASE_KR4:
            case FM_ETH_MODE_40GBASE_CR4:
            case FM_ETH_MODE_40GBASE_SR4:
            {
                *pTxAntiBubbleWatermarkValue = 4;
                *pTxRateFifoWatermarkValue   = 5;
                *pTxRateFifoSlowIncValue     = 51;
                *pTxRateFifoFastIncValue     = 52;
                break;
            }
            case FM_ETH_MODE_100GBASE_KR4:
            case FM_ETH_MODE_100GBASE_CR4:
            case FM_ETH_MODE_100GBASE_SR4:
            {
                *pTxAntiBubbleWatermarkValue = 4;
                *pTxRateFifoWatermarkValue   = 3;
                *pTxRateFifoSlowIncValue     = 129;
                *pTxRateFifoFastIncValue     = 130;
                break;
            }
    
            case FM_ETH_MODE_XLAUI:
            case FM_ETH_MODE_24GBASE_KR4:
            case FM_ETH_MODE_24GBASE_CR4:
            case FM_ETH_MODE_6GBASE_KR:
            case FM_ETH_MODE_6GBASE_CR:
            default:
            {
                /* non sopported cases: set default values and return an error */
                *pTxAntiBubbleWatermarkValue = 1;
                *pTxRateFifoWatermarkValue   = 0;
                *pTxRateFifoSlowIncValue     = 2;
                *pTxRateFifoFastIncValue     = 1;
                err = FM_FAIL;
                break;
            }            
    
        }   /* end switch ( ethMode ) */
    }

    return err;

}   /* end GetTxFifoConfig */




/*****************************************************************************/
/** SetFabricLoopbackFlags
 * \ingroup intPort
 *
 * \desc            Enables or disables the fabric loopback for the specified
 *                  port according to enableFabricLoopback. In the case of
 *                  multi-lane ethernet modes, fabric loopback will be configures
 *                  on all the 4 lanes on the associated EPL. Note that to enable
 *                  the fabric loopback, the active flag in the EPL_CFG_A register
 *                  must be reset.
 * 
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       switchPtr pointer to the switch descriptor structure.
 *
 * \param[in]       port is the logical port ID
 * 
 * \param[in]       ethMode is the Ethernet Mode configured on this port.
 *                  
 * \param[in]       enableFabricLoopback indicates that fabric loopback must
 *                  be enabled (TRUE) or disabled (FALSE)
 *
 * \return          FM_OK if successful 
 *
 *****************************************************************************/
static fm_status SetFabricLoopbackFlags(fm_int     sw,
                                        fm_switch *switchPtr,
                                        fm_int     port,
                                        fm_int     ethMode,
                                        fm_bool    enableFabricLoopback)
{
    fm_status     err;
    fm_int        epl;
    fm_int        lane;
    fm_uint32     eplCfgA;
    fm_uint32     curEplCfgA;


    if ( switchPtr == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* get the epl and port associated to this port */
        err = fm10000MapLogicalPortToEplLane(sw, port, &epl, &lane);
    
        if (err == FM_OK)
        {
            TAKE_REG_LOCK(sw);
    
            /* read-modify-write EPL_CFG_A to update the associated active flag(s) */
            err = switchPtr->ReadUINT32( sw, FM10000_EPL_CFG_A(epl), &eplCfgA );
            curEplCfgA = eplCfgA;
    
            if (err == FM_OK)
            {
                /* Note: to enable fabric loopback, the corresponding active flag
                 * in EPL_CFG_A must be set to FALSE (0) */
    
                /* check the ethernet mode to see if it is a multilane port */
                if ( !(ethMode & FM_ETH_MODE_4_LANE_BIT_MASK) )
                {
                    /* single lane case */
                    switch ( lane )
                    {
                        case 0:
                            FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_0, !enableFabricLoopback);
                            break;
                        case 1:
                            FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_1, !enableFabricLoopback);
                            break;
                        case 2:
                            FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_2, !enableFabricLoopback);
                            break;
                        case 3:
                            FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_3, !enableFabricLoopback);
                            break;
                        default:
                            err = FM_FAIL;
                    }
                    
                    if (curEplCfgA != eplCfgA)
                    {
                        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                         port,
                                         "Setting fabric loopback on single-lane port=%-2d, epl=%d, "
                                         "lane=%d: fabric loopback= %s ethMode = 0x%x\n",
                                         port,
                                         epl,
                                         lane,
                                         enableFabricLoopback? "Enabled": "Disabled",
                                         ethMode);
                    }
                }
                else
                {
                    /* multilane case */
                    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_0, !enableFabricLoopback);
                    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_1, !enableFabricLoopback);
                    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_2, !enableFabricLoopback);
                    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_3, !enableFabricLoopback);

                    if (curEplCfgA != eplCfgA)
                    {
                        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                         port,
                                         "Setting fabric loopback on multi-lane port=%-2d, epl=%d: "
                                         "fabric loopback= %s ethMode = 0x%x\n",
                                         port,
                                         epl,
                                         enableFabricLoopback? "Enabled": "Disabled",
                                         ethMode);
                    }
                }
    
                /* write back only if register value has changed */
                if (curEplCfgA != eplCfgA)
                {
                    err = switchPtr->WriteUINT32( sw, FM10000_EPL_CFG_A(epl), eplCfgA );
                }
            }
            DROP_REG_LOCK(sw);
        }
    }

    return err;
}   /* end SetFabricLoopbackFlags */




/*****************************************************************************/
/** SetDrainMode
 * \ingroup intPort
 *
 * \desc            Sets the Tx Drain mode for a given port to enabled or
 *                  disabled in the context of a port state machine event
 *
 * \param[in]       userInfo is the pointer to a user-allocated data structure
 *                  containing the purpose-specific event information
 *
 * \param[in]       enabled is a boolean argument indicating whether drain
 *                  mode is to be enabled or disabled
 *
 * \return          FM_OK if successful 
 *
 *****************************************************************************/
static fm_status SetDrainMode( fm10000_portSmEventInfo *userInfo, 
                               fm_bool                  enabled )
{
    fm_int        sw;
    fm_int        port;
    fm_int        physPort;
    fm_status     status;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_int        numPorts;

    switchPtr   = userInfo->switchPtr;
    portExt     = userInfo->portExt;
    sw          = switchPtr->switchNumber;
    port        = userInfo->portPtr->portNumber;
    physPort    = userInfo->portPtr->physicalPort;

    switch ( userInfo->info.config.ethMode )
    {
        /* four lanes */
        case FM_ETH_MODE_10GBASE_KX4:
        case FM_ETH_MODE_10GBASE_CX4:
        case FM_ETH_MODE_XAUI:
        case FM_ETH_MODE_XLAUI:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_40GBASE_SR4:
        case FM_ETH_MODE_100GBASE_KR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_SR4:
            numPorts = 4;
            break;
        default:
            numPorts = 1;
            break;
    }

    status = fm10000DrainPhysPort( sw, physPort, numPorts, enabled );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

ABORT:
    return status;
   
}   /* end SetDrainMode */




/*****************************************************************************/
/** SerDesEventReq
 * \ingroup intPort
 *
 * \desc            Sends a SerDes-level event notifications to the state
 *                  machines belonging to the lane(s) associated to a given
 *                  port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[in]       eventId is the SerDes event to send.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SerDesEventReq( fm_smEventInfo *eventInfo, 
                                 void           *userInfo,
                                 fm_int          eventId )
{
    fm_status       status = FM_OK;
    fm10000_port   *portExt;
    fm10000_lane   *laneExt;
    fm_int          sw;
    fm_int          port;
    fm_smEventInfo  laneEventInfo;

    FM_NOT_USED(eventInfo);

    sw      = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port    = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    laneEventInfo.eventId = eventId;
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        laneEventInfo.smType = laneExt->smType;
        laneEventInfo.lock   = FM_GET_STATE_LOCK( sw );
        laneEventInfo.dontSaveRecord = FALSE;
        status =  fmNotifyStateMachineEvent( laneExt->smHandle,
                                             &laneEventInfo,
                                             &laneExt->eventInfo,
                                             &laneExt->serDes );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

ABORT:
    return status;

}   /* end SerDesEventReq */




/*****************************************************************************/
/** ConvertAdminModeToTxLinkFault
 * \ingroup intPort
 *
 * \desc            Functions that maps the admin modes at the API level into
 *                  values of the TxFaultMode int he MAC_CFG registers
 *
 * \param[in]       mode the admin mode (see ''fm_portMode'')
 *
 * \return          An integer representing the TxFaultMode in the MAC_CFG
 *                  register
 *
 *****************************************************************************/
static fm_int ConvertAdminModeToTxLinkFault( fm_portMode mode )
{
    fm_int linkFault;

    /* Program the output pattern based on the TxFaultMode */
    if ( mode == FM_PORT_MODE_ADMIN_DOWN )
    {
        linkFault = 1;
    }
    else if ( mode == FM_PORT_MODE_REMOTE_FAULT )
    {
        linkFault = 3;
    }
    else if ( mode == FM_PORT_MODE_LOCAL_FAULT )
    {
        linkFault = 2;
    }
    else 
    {
        linkFault = 0;
    }

    return linkFault;

}   /* end ConvertAdminModeToTxLinkFault */




/*****************************************************************************/
/** EthModeToBitRate
 * \ingroup intPort
 *
 * \desc            This function returns the lane bit rate associated to a
 *                  given combination of ethernet interface mode and speed for
 *                  the parent logical port 
 *
 * \param[in]       ethMode is the ethernet interface mode
 *
 * \param[in]       speed is the port speed, to be used with PCS modes that
 *                  could be run at different speeds
 *
 * \return          The lane bitrate (see ''fm10000_laneBitrate'')
 * 
 *****************************************************************************/
static fm10000_laneBitrate EthModeToBitRate( fm_ethMode ethMode, 
                                             fm_uint32  speed )
{
    fm10000_laneBitrate bitRate;

    /* invoke the appropriate PCS Initialization depending on the ethMode */
    switch ( ethMode )
    {
        case FM_ETH_MODE_AN_73:
            if ( speed == 1000 )
            {
                /* clause 37 */
                bitRate = FM10000_LANE_BITRATE_1000MBPS;
            }
            else
            {
                /* clause 73 */
                bitRate = FM10000_LANE_BITRATE_2500MBPS;
            }
            break;
        case FM_ETH_MODE_SGMII:
        case FM_ETH_MODE_1000BASE_X:
        case FM_ETH_MODE_1000BASE_KX:
            bitRate = FM10000_LANE_BITRATE_1000MBPS;
            break;
        
        case FM_ETH_MODE_2500BASE_X:
            bitRate = FM10000_LANE_BITRATE_2500MBPS;
            break;

        case FM_ETH_MODE_6GBASE_KR:
        case FM_ETH_MODE_6GBASE_CR:
        case FM_ETH_MODE_24GBASE_CR4:
        case FM_ETH_MODE_24GBASE_KR4:
            bitRate = FM10000_LANE_BITRATE_6GBPS;
            break;

        case FM_ETH_MODE_XLAUI:
        case FM_ETH_MODE_10GBASE_CR:
        case FM_ETH_MODE_10GBASE_SR:
        case FM_ETH_MODE_10GBASE_KR:
        case FM_ETH_MODE_40GBASE_KR4:
        case FM_ETH_MODE_40GBASE_CR4:
        case FM_ETH_MODE_40GBASE_SR4:
            bitRate = FM10000_LANE_BITRATE_10GBPS;
            break;

        case FM_ETH_MODE_25GBASE_SR:
        case FM_ETH_MODE_25GBASE_KR:
        case FM_ETH_MODE_100GBASE_KR4:
        case FM_ETH_MODE_100GBASE_CR4:
        case FM_ETH_MODE_100GBASE_SR4:
            bitRate = FM10000_LANE_BITRATE_25GBPS;
            break;

        default:
            bitRate = FM10000_LANE_BITRATE_UNKNOWN;
            break;

    }   /* end switch ( ethMode ) */

    return bitRate;

}   /* end EthModeToBitRate */




/*****************************************************************************/
/** UnlinkAllLanes
 * \ingroup intPort
 *
 * \desc            Private function removing the link between a port and its
 *                  associated lanes when a port gets disabled
 *
 * \param[in]       portExt is the pointer is the pointer to the port extension
 *                  structure (type ''fm10000_port'')
 *
 * \return          FM_OK
 *
 *****************************************************************************/
static fm_status UnlinkAllLanes( fm10000_port *portExt )
{
    fm10000_lane *laneExt;

    /* start at the beginning */
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        /* Mark the lane as unused */
        laneExt->lane          = FM_PORT_LANE_NA;
        laneExt->parentPortExt = NULL;

        /* remove from the list */
        FM_DLL_REMOVE_NODE( portExt, 
                            firstLane, 
                            lastLane, 
                            laneExt, 
                            nextLane, 
                            prevLane );

        /* try next */
        laneExt = FM_DLL_GET_FIRST( portExt, firstLane );

    }   /* while ( laneExt != NULL ) */

    return FM_OK;

}   /* end UnlinkAllLanes */




/*****************************************************************************/
/** HandleDeferralTimeout
 * \ingroup intPort
 * 
 * \desc            Handles a the expiration of the timer used to defer the
 *                  completion of the PLL power up process
 *
 * \param[in]       arg is the pointer to the argument passed when the timer
 *                  was started, in this case the pointer to the lane extension
 *                  structure (type ''fm10000_lane'')
 *
 * \return          None
 *
 *****************************************************************************/
static void HandleDeferralTimeout( void *arg )
{
    fm_smEventInfo           eventInfo; 
    fm10000_portSmEventInfo *portEventInfo;
    fm10000_port            *portExt;
    fm_int                   sw;
    fm_int                   port;

    /* The callback argument is the pointer to the lane extension structure */
    portExt           = arg;
    eventInfo.smType  = portExt->smType;
    eventInfo.eventId = FM10000_PORT_EVENT_DEFTIMER_EXP_IND;
    portEventInfo     = &portExt->eventInfo;
    sw                = portEventInfo->switchPtr->switchNumber;
    port              = portExt->base->portNumber;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     port,
                     "Deferral timer expired on port %d (switch %d, portPtr=%p)\n",
                     port,
                     sw,
                     (void *)portExt->base );

    /* For the time being we're assuming that the data to be recorded
       is the SerDes ID */
    PROTECT_SWITCH( sw );
    eventInfo.lock = FM_GET_STATE_LOCK( sw );
    eventInfo.dontSaveRecord = FALSE;
    portExt->eventInfo.regLockTaken = FALSE;
    fmNotifyStateMachineEvent( portExt->smHandle,
                               &eventInfo,
                               portEventInfo,
                               &port );
    UNPROTECT_SWITCH( sw );

}   /* end HandleDeferralTimeout */




/*****************************************************************************/
/** SendAnEventReq
 * \ingroup intPort
 *
 * \desc            Action callback to send an event request to the AN state
 *                  machine
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[in]       eventId is the ID of the AN event to be sent                  
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SendAnEventReq( fm_smEventInfo *eventInfo, 
                                 void           *userInfo,
                                 fm_int          eventId )
{
    fm_smEventInfo  anEventInfo;
    fm_switch      *switchPtr;
    fm10000_port   *portExt;
    fm_int          sw;
    fm_int          port;

    portExt   = (( fm10000_portSmEventInfo *)userInfo)->portExt;
    switchPtr = (( fm10000_portSmEventInfo *)userInfo)->switchPtr;
    port      = portExt->base->portNumber;
    sw        = switchPtr->switchNumber;

    anEventInfo.smType         = portExt->anSmType;
    anEventInfo.eventId        = eventId;
    anEventInfo.lock           = FM_GET_STATE_LOCK( sw );
    anEventInfo.dontSaveRecord = FALSE;

    portExt->eventInfo.regLockTaken = FALSE;
    return fmNotifyStateMachineEvent( portExt->anSmHandle,
                                      &anEventInfo,
                                      &portExt->eventInfo,
                                      &port );

}   /* end SendAnEventReq */




/*****************************************************************************/
/** SetTxFaultModeInMacCfg
 * \ingroup intPort
 *
 * \desc            Local helper function to set the required fault mode in
 *                  the MAC_CFG register.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       port is the port number.
 * 
 * \param[in]       epl is the EPL number.
 * 
 * \param[in]       physLane is the physical lane number.
 * 
 * \param[in]       mode is the fault mode to be set.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SetTxFaultModeInMacCfg( fm_int sw,
                                         fm_int port,
                                         fm_int epl,
                                         fm_int physLane,
                                         fm_int mode )
{
    fm_status     status;
    fm_uint32     addr;
    fm_uint32     macCfg[FM10000_MAC_CFG_WIDTH];
    fm_switch    *switchPtr;

    switchPtr = GET_SWITCH_PTR( sw );

    /* Read the MAC_CFG register */
    addr = FM10000_MAC_CFG( epl, physLane, 0 );

    TAKE_REG_LOCK( sw );

    /* read MAC_CFG */
    status = switchPtr->ReadUINT32Mult( sw,
                                        addr,
                                        FM10000_MAC_CFG_WIDTH,
                                        macCfg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* program the TxFaultMode field based on the current admin mode */
    FM_ARRAY_SET_FIELD( macCfg, 
                        FM10000_MAC_CFG, 
                        TxFaultMode,
                        ConvertAdminModeToTxLinkFault( mode ) );

    /* write it back */
    status = switchPtr->WriteUINT32Mult( sw,
                                         addr,
                                         FM10000_MAC_CFG_WIDTH,
                                         macCfg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

ABORT:
    DROP_REG_LOCK( sw );

    return status;

}   /* end SetTxFaultModeInMacCfg */


/*****************************************************************************/
/** SetPepLoopback
 * \ingroup intPort
 *
 * \desc            Action that enables or disables fabric loopback for a given
 *                  PEP
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[in]       enabled set to TRUE if loopback is to be enabled, FALSE
 *                  otherwise
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status SetPepLoopback( fm_smEventInfo *eventInfo,
                                 void           *userInfo,
                                 fm_bool         enabled )
{
    fm_status status;
    fm_uint32 regAddr;
    fm_uint32 rv;
    fm_int    sw;
    fm_int    pep;
    fm_int    port;

    sw   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    pep  = ((fm10000_portSmEventInfo *)userInfo)->portExt->endpoint.pep;
    port = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;

    TAKE_REG_LOCK( sw );

    regAddr = FM10000_PCIE_CTRL_EXT();

    status = fm10000ReadPep(sw, regAddr, pep, &rv);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    FM_SET_BIT(rv, FM10000_PCIE_CTRL_EXT, SwitchLoopback, enabled);

    status = fm10000WritePep(sw, regAddr, pep, rv);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

ABORT:
    DROP_REG_LOCK( sw );
    return status;

}   /* end SetPepLoopback */


/*****************************************************************************/
/** ConfigureEeeLane
 * \ingroup intPort
 *
 * \desc            Configure the lane's related EEE parameters.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
static fm_status ConfigureEeeLane( fm_smEventInfo *eventInfo,
                                   void           *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm10000_port *portExt;
    fm_int        port;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm_uint32     regAddr;
    fm_uint32     laneCfg;
    fm_uint32     laneEnergyDetectCfg;
    fm_uint32     laneSignalDetectCfg;


    FM_NOT_USED(eventInfo);

    err = FM_OK;
    
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    port      = portExt->base->portNumber; 
    physLane  = portExt->nativeLaneExt->physLane;

    TAKE_REG_LOCK( sw );

    /* Configure LANE_CFG */

    regAddr = FM10000_LANE_CFG(epl, physLane);

    err = switchPtr->ReadUINT32( sw, regAddr, &laneCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    FM_SET_BIT(laneCfg, 
               FM10000_LANE_CFG, 
               EeeHwRxQuiet, 0);

    FM_SET_BIT(laneCfg, 
               FM10000_LANE_CFG, 
               EeeHwTxQuiet, 1);

    FM_SET_BIT(laneCfg, 
               FM10000_LANE_CFG, 
               EeeHwLpiDisable, 0);

    err = switchPtr->WriteUINT32( sw, regAddr, laneCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );


    /* Configure LANE_SERDES_CFG */ 
    /*                           */
    /*  - CoreToCntl[7:5] = 0    */
    /*                           */

    regAddr = FM10000_LANE_SERDES_CFG(epl, physLane);

    err = switchPtr->ReadUINT32( sw, regAddr, &laneCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    FM_SET_FIELD(laneCfg,
                 FM10000_LANE_SERDES_CFG,
                 CoreToCntl,
                 0x1106);

    err = switchPtr->WriteUINT32( sw, regAddr, laneCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );


    /* Configure LANE_ENERGY_DETECT_CFG */

    regAddr = FM10000_LANE_ENERGY_DETECT_CFG(epl, physLane);

    err = switchPtr->ReadUINT32( sw, regAddr, &laneEnergyDetectCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    FM_SET_BIT(laneEnergyDetectCfg, 
               FM10000_LANE_ENERGY_DETECT_CFG, 
               EdMaskRxSignalOk, 1);

    FM_SET_BIT(laneEnergyDetectCfg, 
               FM10000_LANE_ENERGY_DETECT_CFG, 
               EdMaskRxRdy, 1);

    FM_SET_BIT(laneEnergyDetectCfg, 
               FM10000_LANE_ENERGY_DETECT_CFG, 
               EdMaskRxActivity, 1);

    FM_SET_BIT(laneEnergyDetectCfg, 
               FM10000_LANE_ENERGY_DETECT_CFG, 
               EdMaskEnergyDetect, 0);

    err = switchPtr->WriteUINT32( sw, regAddr, laneEnergyDetectCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );


    /* Configure LANE_SIGNAL_DETECT_CFG */

    regAddr = FM10000_LANE_SIGNAL_DETECT_CFG(epl, physLane);

    err = switchPtr->ReadUINT32( sw, regAddr, &laneSignalDetectCfg);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, err );

    FM_SET_BIT(laneSignalDetectCfg, 
               FM10000_LANE_SIGNAL_DETECT_CFG, 
               SdMaskRxSignalOk, 0);

    FM_SET_BIT(laneSignalDetectCfg, 
               FM10000_LANE_SIGNAL_DETECT_CFG, 
               SdMaskRxRdy, 0);

    FM_SET_BIT(laneSignalDetectCfg, 
               FM10000_LANE_SIGNAL_DETECT_CFG, 
               SdMaskRxActivity, 1);

    FM_SET_BIT(laneSignalDetectCfg, 
               FM10000_LANE_SIGNAL_DETECT_CFG, 
               SdMaskEnergyDetect, 1);

    err = switchPtr->WriteUINT32( sw, regAddr, laneSignalDetectCfg);

ABORT:
     
    DROP_REG_LOCK( sw );

    return err;

}   /* end ConfigureEeeLane */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000PowerUpLane
 * \ingroup intPort
 *
 * \desc            Power up the lane(s) associated to a port in the context
 *                  of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000PowerUpLane( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm10000_lane   *laneExt;
    fm_int          eventId;

    FM_NOT_USED(eventInfo);

    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    switch ( portExt->eventInfo.info.admin.mode )
    {
        case FM_PORT_MODE_ADMIN_DOWN:
        case FM_PORT_MODE_REMOTE_FAULT:
        case FM_PORT_MODE_LOCAL_FAULT:
/* This causes a wrong transition at serdes level. To be reviewed */
#if 0
            eventId = FM10000_SERDES_EVENT_TX_POWERUP_REQ;
            break;
#endif
        default:
            eventId = FM10000_SERDES_EVENT_POWERUP_REQ;
            break;
    }

    /* prepare the lane mask */
    portExt->lockedLaneMask  = 0;
    portExt->desiredLaneMask = 0;
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        portExt->desiredLaneMask |= (1 << laneExt->physLane );
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    return SerDesEventReq( eventInfo, userInfo, eventId );

}   /* end fm10000PowerUpLane */




/*****************************************************************************/
/** fm10000NotifyEeeModeChange
 * \ingroup intPort
 *
 * \desc            Function that notifies the serdes of an Energy-Efficient
 *                  Ethernet mode change.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       port is the logical port on which to operate.
 *
 * \param[in]       eeeMode is the EEE mode: enabled or disabled.
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000NotifyEeeModeChange( fm_int  sw, fm_int  port, fm_bool eeeMode )
{
    fm10000_portSmEventInfo userInfo;
    fm_smEventInfo          eventInfo;
    fm_int                  eventId;


    userInfo.switchPtr = GET_SWITCH_PTR(sw);
    userInfo.portPtr   = GET_PORT_PTR(sw, port);
    userInfo.portExt   = GET_PORT_EXT( sw, port );

    if ( eeeMode )
    {
        eventId = FM10000_SERDES_EVENT_ENABLE_EEE_MODE_REQ;
    }
    else
    {
        eventId = FM10000_SERDES_EVENT_DISABLE_EEE_MODE_REQ;
    }

    return SerDesEventReq( &eventInfo, &userInfo, eventId );


}   /* end fm10000NotifyEeeModeChange */




/*****************************************************************************/
/** fm10000PowerDownLane
 * \ingroup intPort
 *
 * \desc            Power down the lane(s) associated to a port in the context
 *                  of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000PowerDownLane( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SerDesEventReq( eventInfo, 
                           userInfo, 
                           FM10000_SERDES_EVENT_POWERDOWN_REQ );

}   /* end fm10000PowerDownLane */




/*****************************************************************************/
/** fm10000ConfigureLane
 * \ingroup intPort
 *
 * \desc            Configure the lane(s) associated to a port in the context
 *                  of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ConfigureLane( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status            err;
    fm_int               eventId;
    fm_ethMode           ethMode;
    fm_pepMode           pepMode;
    fm_uint32            speed;
    fm10000_laneBitrate  bitRate;
    fm10000_port        *portExt;
    fm10000_lane        *laneExt;

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    /* Processing is similar, but not identical for Ethernet and PCIE ports */
    if ( portExt->ring == FM10000_SERDES_RING_EPL )
    {
        /* ethernet port */
        ethMode = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
        speed   = ((fm10000_portSmEventInfo *)userInfo)->info.config.speed;

        /* the event id depends on whether or not the port is being disabled */
        if ( ethMode == FM_ETH_MODE_DISABLED )
        {
            eventId = FM10000_SERDES_EVENT_DISABLE_REQ;
        }
        else
        {
            eventId = FM10000_SERDES_EVENT_CONFIGURE_REQ;

            laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
            while ( laneExt != NULL )
            {
                bitRate = EthModeToBitRate( ethMode, speed );
                laneExt->eventInfo.info.bitRate = bitRate;
                laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
            }
        }

        /* Energy-Efficient Ethernet related parameters */
        err = ConfigureEeeLane( eventInfo, userInfo );

    }
    else
    {
        /* PCIE port */
        pepMode = ((fm10000_portSmEventInfo *)userInfo)->info.config.pepMode;

        /* the event id depends on whether or not the port is being disabled */
        if ( pepMode == FM_PORT_PEP_MODE_DISABLED )
        {
            eventId = FM10000_SERDES_EVENT_DISABLE_REQ;
        }
        else
        {
            eventId = FM10000_SERDES_EVENT_CONFIGURE_REQ;
        }
    }

    err = SerDesEventReq( eventInfo, userInfo, eventId );

    return err;

}   /* end fm10000ConfigureLane */




/*****************************************************************************/
/** fm10000ConfigureLaneForAn73
 * \ingroup intPort
 *
 * \desc            Inconditionally configure for AN-73 the lane(s) associated
 *                  to a port in the context of an event notified on that
 *                  port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ConfigureLaneForAn73( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status            err;
    fm10000_laneBitrate  bitRate;
    fm10000_port        *portExt;
    fm10000_lane        *laneExt;

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        bitRate = FM10000_LANE_BITRATE_2500MBPS;
        laneExt->eventInfo.info.bitRate = bitRate;
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    /* Energy-Efficient Ethernet related parameters */
    err = ConfigureEeeLane( eventInfo, userInfo );


    err = SerDesEventReq( eventInfo, userInfo, FM10000_SERDES_EVENT_CONFIGURE_REQ );

    return err;

}   /* end fm10000ConfigureLaneForAn73 */




/*****************************************************************************/
/** fm10000ReleaseSchedBw
 * \ingroup intPort
 *
 * \desc            Release the associated scheduler BW from this port.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ReleaseSchedBw( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status            err;
    fm_int               sw;
    fm_ethMode           ethMode;
    fm_int               numLanes;
    fm_int               physPort;
    fm_schedulerPortMode portLaneConfig;
    fm_int               schedMode;
    fm10000_port        *portExt;
    
    err     = FM_OK;
    sw      = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    portLaneConfig = FM_SCHED_PORT_MODE_NONE;
    physPort = ((fm10000_portSmEventInfo *)userInfo)->portPtr->physicalPort;

    /* Scheduler is reconfigured only for EPL ports */
    if ( ((fm10000_portSmEventInfo *)userInfo)->portExt->ring == FM10000_SERDES_RING_EPL )
    {
        ethMode  = portExt->ethMode;

        /* the event id depends on whether or not the port is being disabled */
        if ( ethMode != FM_ETH_MODE_DISABLED )
        {
            err = fm10000GetNumEthLanes( ethMode, &numLanes );

            if (err == FM_OK)
            {
                if ( numLanes == 1 )
                {
                    /* single-lane ports */
                    portLaneConfig = FM_SCHED_PORT_MODE_SINGLE;
                }
                else
                {
                    /* multi-lane port */
                    portLaneConfig = FM_SCHED_PORT_MODE_QUAD;
                }
                
            }
        }
    }

    if (err == FM_OK)
    {
        err = fm10000GetSchedMode(sw, &schedMode);

        if (err == FM_OK)
        {
            if (schedMode == FM10000_SCHED_MODE_STATIC)
            {
                /* Do Nothing */
            }
            else if (schedMode == FM10000_SCHED_MODE_DYNAMIC)
            {
                err = fm10000ReserveSchedBw(sw, 
                                            physPort, 
                                            0, 
                                            portLaneConfig);
            }
        }
    }
    
    return err;    

}   /* end fm10000ReleaseSchedBw */




/*****************************************************************************/
/** fm10000UpdateSchedBw
 * \ingroup intPort
 *
 * \desc            Release the associated scheduler BW from this port.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000UpdateSchedBw( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status            err;
    fm_int               sw;
    fm_ethMode           ethMode;
    fm_uint32            speed;
    fm_int               numLanes;
    fm_int               physPort;
    fm_schedulerPortMode portLaneConfig;
    fm_int               schedMode;
    fm10000_port        *portExt;
    
    err     = FM_OK;
    sw      = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    speed = 0;
    portLaneConfig = FM_SCHED_PORT_MODE_NONE;
    physPort = ((fm10000_portSmEventInfo *)userInfo)->portPtr->physicalPort;

    /* Scheduler is reconfigured only for EPL ports */
    if ( ((fm10000_portSmEventInfo *)userInfo)->portExt->ring == FM10000_SERDES_RING_EPL )
    {
        ethMode  = portExt->ethMode;

        /* the event id depends on whether or not the port is being disabled */
        if ( ethMode != FM_ETH_MODE_DISABLED )
        {
            err = fm10000GetNumEthLanes( ethMode, &numLanes );

            if (err == FM_OK)
            {
                if ( numLanes == 1 )
                {
                    /* single-lane ports */
                    portLaneConfig = FM_SCHED_PORT_MODE_SINGLE;
                }
                else
                {
                    /* multi-lane port */
                    portLaneConfig = FM_SCHED_PORT_MODE_QUAD;
                }
            }


            if (err == FM_OK)
            {
                speed = fm10000GetPortSpeed(ethMode);
            }
        }
    }

    if (err == FM_OK)
    {
        err = fm10000GetSchedMode(sw, &schedMode);

        if (err == FM_OK)
        {
            if (schedMode == FM10000_SCHED_MODE_STATIC)
            {
                /* Do Nothing */
            }
            else if (schedMode == FM10000_SCHED_MODE_DYNAMIC)
            {
                err = fm10000ReserveSchedBw(sw, 
                                            physPort, 
                                            speed, 
                                            portLaneConfig);

                if (err == FM_OK && speed != 0)
                {
                    err = fm10000RegenerateSchedule(sw);
                }
            }
        }
    }
    
    return err;    

}   /* end fm10000UpdateSchedBw */




/*****************************************************************************/
/** fm10000ReconfigureScheduler
 * \ingroup intPort
 *
 * \desc            Reconfigure the scheduler according to the bitrate and if
 *                  and the number of lanes (single or multi) used by the port.
 *                  The scheduler is reconfigured only if the port is in the
 *                  EPL ring.
 *                  This function is called each time the Ethernet mode is set,
 *                  even if the speed and/or the lane configuration (single/multi)
 *                  have not changed. It is up to the specific scheduler functions
 *                  to determine if an effective scheduler update is required.
 *                  Although it is not strictly necessary, this function is also
 *                  called when a port becomes down.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ReconfigureScheduler( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status            err;
    fm_int               sw;
    fm_ethMode           ethMode;
    fm_uint32            speed;
    fm_int               numLanes;
    fm_int               physPort;
    fm_schedulerPortMode portLaneConfig;
    fm_int               schedMode;
    fm_int               portMode;

    err = FM_OK;
    portLaneConfig = FM_SCHED_PORT_MODE_NONE;

    sw = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;

    portMode = ((fm10000_portSmEventInfo *)userInfo)->portPtr->mode;

    /* Scheduler is reconfigured only for EPL ports */
    if ( ((fm10000_portSmEventInfo *)userInfo)->portExt->ring == FM10000_SERDES_RING_EPL )
    {
        /* ethernet port */
        physPort = ((fm10000_portSmEventInfo *)userInfo)->portPtr->physicalPort ;
        ethMode  = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
        speed    = ((fm10000_portSmEventInfo *)userInfo)->info.config.speed;

        /* the event id depends on whether or not the port is being disabled */
        if ( ethMode != FM_ETH_MODE_DISABLED )
        {
            err = fm10000GetNumEthLanes( ethMode, &numLanes );

            if (err == FM_OK)
            {
                if ( numLanes == 1 )
                {
                    /* single-lane ports */
                    portLaneConfig = FM_SCHED_PORT_MODE_SINGLE;
                }
                else
                {
                    /* multi-lane port */
                    portLaneConfig = FM_SCHED_PORT_MODE_QUAD;
                }
                
            }
        }
        if (err == FM_OK )
        {
            FM_LOG_DEBUG_V2(FM_LOG_CAT_PORT, ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber,
                            "Reconfiguring scheduler for port %d, physPort=%d, ethernetMode=%d for speed=%d, laneConfig=%d\n",
                            ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber,
                            physPort,
                            ethMode,
                            speed,
                            portLaneConfig);   

            err = fm10000GetSchedMode(sw, &schedMode);

            if (err == FM_OK)
            {
                if (schedMode == FM10000_SCHED_MODE_STATIC)
                {
                    err = fm10000UpdateSchedPort(sw,
                                                 physPort,
                                                 speed,
                                                 portLaneConfig);
                }
                else if ( (schedMode == FM10000_SCHED_MODE_DYNAMIC) && 
                          (portMode != FM_PORT_MODE_ADMIN_PWRDOWN) )
                {
                    err = fm10000ReserveSchedBw(sw, 
                                                physPort, 
                                                speed, 
                                                portLaneConfig);
                    if (err == FM_OK)
                    {
                        if (speed == 0)
                        {
                            err = fm10000UpdateSchedPort(sw,
                                                        physPort,
                                                        speed,
                                                        portLaneConfig);
                        }
                        else
                        {
                            err = fm10000RegenerateSchedule(sw);
                        }
                    }
                }
            }
        }
    }

    return err;

}   /* end fm10000ReconfigureScheduler */




/*****************************************************************************/
/** fm10000NotifyApiPortUp
 * \ingroup intPort
 *
 * \desc            Notify the API user that a port is UP in the context
 *                  of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000NotifyApiPortUp( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status         status;
    fm_int            sw;
    fm_int            port;
    fm_int            physPort;
    fm_port          *portPtr;
    fm10000_portAttr *portAttrExt;


    FM_NOT_USED(eventInfo);

    portPtr     = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;

    sw          = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port        = ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber;
    physPort    = portPtr->physicalPort;


    portPtr->linkUp = TRUE;
    status          = FM_OK;
    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
         || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (portPtr->portNumber != 0) ) )
    {
        if ( portAttrExt->negotiatedEeeModeEnabled == TRUE )
        {
            status = fm10000StartDeferredLpiTimer(eventInfo,userInfo);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        /* We may need to drop the state lock */
        //  FM_DROP_STATE_LOCK( sw );
        status = fm10000SendLinkUpDownEvent( sw,
                                             physPort,
                                             0,
                                             TRUE,
                                             FM_EVENT_PRIORITY_LOW );
        // FM_TAKE_STATE_LOCK( sw );

    }

ABORT:

    return status;

}   /* end fm10000NotifyApiPortUp */




/*****************************************************************************/
/** fm10000NotifyApiPortDown
 * \ingroup intPort
 *
 * \desc            Notify the API user that a port is DOWN in the context
 *                  of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000NotifyApiPortDown( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status status;
    fm_int    sw;
    fm_int    physPort;
    fm_port  *portPtr;
    fm10000_port *portExt;
    fm_int port;

    FM_NOT_USED(eventInfo);

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portExt  = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port     = portExt->base->portNumber; 
    physPort = portPtr->physicalPort;

    status = FM_OK;

    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)   ||
          ( (portPtr->portType == FM_PORT_TYPE_CPU) && (portPtr->portNumber != 0) ))
    {
        /* Notify the serdes about that link down for EEE purpose */
        status = fm10000NotifyEeeModeChange( sw, port, FALSE );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

        if (portPtr->linkUp == TRUE)
        {
            portPtr->linkUp = FALSE;
            status = fm10000SendLinkUpDownEvent( sw,
                                                 physPort,
                                                 0,
                                                 FALSE,
                                                 FM_EVENT_PRIORITY_LOW );
        }
    }
ABORT:

    return status;

}   /* end fm10000NotifyApiPortDown */




/*****************************************************************************/
/** fm10000InitPcs
 * \ingroup intPort
 *
 * \desc            Initialize the PCS layer for a given port in the context
 *                  of a port state machine configuration request
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000InitPcs( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm10000_port     *portExt;
    fm_status         status;
    fm10000_pcsTypes  pcsType;
    fm_ethMode        ethMode;
    fm_uint32         speed;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw        = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    ethMode   = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
    speed     = ((fm10000_portSmEventInfo *)userInfo)->info.config.speed;


    /* Return gracefully if we're in bypass mode */
    if (fmPlatformBypassEnabled(sw))
    {
        return FM_OK;
    }

    pcsType = fm10000GetPcsType( ethMode, speed );

    /* invoke the appropriate PCS Initialization depending on the ethMode */
    switch ( pcsType )
    {
        case FM10000_PCS_SEL_DISABLE:
            status = FM_OK;
            break;

        case FM10000_PCS_SEL_AN_73:
            status = fm10000InitAn73( eventInfo, userInfo );
            break;

        case FM10000_PCS_SEL_SGMII_10:
        case FM10000_PCS_SEL_SGMII_100:
        case FM10000_PCS_SEL_SGMII_1000:
        case FM10000_PCS_SEL_1000BASEX:
            status = fm10000Init1000BaseX( eventInfo, userInfo );
            break;

        case FM10000_PCS_SEL_10GBASER:
            status = fm10000Init10GBaseR( eventInfo, userInfo );
            break;

        case FM10000_PCS_SEL_40GBASER:
        case FM10000_PCS_SEL_100GBASER:
            status = fm10000InitMlBaseR( eventInfo, userInfo );
            break;

        default:
            status = FM_ERR_UNSUPPORTED;
            break;
       
    }   /* end switch ( pcsType ) */

    return status;

}   /* end fm10000InitPcs */




/*****************************************************************************/
/** fm10000Init1000BaseX
 * \ingroup intPort
 *
 * \desc            Function that configures the PCS layer for a given port
 *                  to 1000Base-X in the context of a port state machine
 *                  configuration request
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000Init1000BaseX( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_int            physLane;
    fm10000_portAttr *attr;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm_uint32         pcs1000BaseXAddr;
    fm_uint32         pcs1000BaseXEeeAddr;
    fm_uint32         pcs1000BaseXReg;
    fm_uint32         pcs1000BaseXEeeReg[FM10000_PCS_1000BASEX_EEE_CFG_WIDTH];
    fm_status         status;
    fm_bool           eeeMode;
    fm_bool           regLockTaken=FALSE;


    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    /* Extract the port info from the port event info structure */
    attr      = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    port      = portExt->base->portNumber; 

    /* here we're assuming that the only lane in the linked lane list is
       the native lane. It should be a reasonable assumption */
    physLane  = portExt->nativeLaneExt->physLane;

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* fill out the register template */
    pcs1000BaseXAddr = FM10000_PCS_1000BASEX_CFG( epl, physLane );
    pcs1000BaseXReg  = 0;

    FM_SET_BIT( pcs1000BaseXReg,
                FM10000_PCS_1000BASEX_CFG,
                LsMaskSeenI1orI2,
                FALSE );
    FM_SET_BIT( pcs1000BaseXReg,
                FM10000_PCS_1000BASEX_CFG,
                DisableCheckEnd,
                FALSE );
    FM_SET_BIT( pcs1000BaseXReg,
                FM10000_PCS_1000BASEX_CFG,
                IgnoreDisparityError,
                FALSE );
    FM_SET_BIT( pcs1000BaseXReg,
                FM10000_PCS_1000BASEX_CFG,
                Tx_8b10bDisparityReset,
                FALSE );
    FM_SET_BIT( pcs1000BaseXReg,
                FM10000_PCS_1000BASEX_CFG,
                Tx_8b10bInjectDisparityErr,
                FALSE );

    FM_SET_BIT( pcs1000BaseXReg,
                FM10000_PCS_1000BASEX_CFG,
                EnEee,
                attr->negotiatedEeeModeEnabled );

    /* Program EEE low-level timers */
    pcs1000BaseXEeeAddr = FM10000_PCS_1000BASEX_EEE_CFG( epl, physLane, 0 );
    FM_MEMSET_S( pcs1000BaseXEeeReg, 
                 sizeof(pcs1000BaseXEeeReg), 
                 0,
                 sizeof(pcs1000BaseXEeeReg) );

    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        TxTslTime,
                        FM10000_PORT_EEE_1000BASEX_TSL_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        TxTslUnit,
                        FM10000_PORT_EEE_1000BASEX_TSL_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        TxTqlTime,
                        FM10000_PORT_EEE_1000BASEX_TQL_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        TxTqlUnit,
                        FM10000_PORT_EEE_1000BASEX_TQL_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        TxTulTime,
                        FM10000_PORT_EEE_1000BASEX_TUL_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        TxTulUnit,
                        FM10000_PORT_EEE_1000BASEX_TUL_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        RxTqrTime,
                        FM10000_PORT_EEE_1000BASEX_TQR_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        RxTqrUnit,
                        FM10000_PORT_EEE_1000BASEX_TQR_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        RxTwrTime,
                        FM10000_PORT_EEE_1000BASEX_TWR_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        RxTwrUnit,
                        FM10000_PORT_EEE_1000BASEX_TWR_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        RxTwtfTime,
                        FM10000_PORT_EEE_1000BASEX_TWTF_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs1000BaseXEeeReg,
                        FM10000_PCS_1000BASEX_EEE_CFG,
                        RxTwtfUnit,
                        FM10000_PORT_EEE_1000BASEX_TWTF_TIMESCALE );


    /* Write the FM10000_PCS_1000BASEX_EEE_CFG register */
    status = switchPtr->WriteUINT32Mult( sw, 
                                         pcs1000BaseXEeeAddr,
                                         FM10000_PCS_1000BASEX_EEE_CFG_WIDTH,
                                         pcs1000BaseXEeeReg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );


    /* Write the FM10000_PCS_1000BASEX_CFG register */
    status = switchPtr->WriteUINT32( sw, pcs1000BaseXAddr, pcs1000BaseXReg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    FM_FLAG_DROP_REG_LOCK( sw );

    /* Notify the serdes about the EEE mode */
    eeeMode = ( attr->negotiatedEeeModeEnabled | (attr->dbgEeeMode != 0) );
    status = fm10000NotifyEeeModeChange( sw, port, eeeMode );

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }


    return status;

}   /* end fm10000Init1000BaseX */




/*****************************************************************************/
/** fm10000Init10GBaseR
 * \ingroup intPort
 *
 * \desc            Function that configures the PCS layer for a given port
 *                  to 10GBase-R in the context of a port state machine
 *                  configuration request
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000Init10GBaseR( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_int            physLane;
    fm10000_portAttr *attr;
    fm_switch        *switchPtr;
    fm_port          *portPtr;
    fm10000_port     *portExt;
    fm_uint32         pcs10GBaserAddr;
    fm_uint32         pcs10GBaserReg;
    fm_uint32         pcs10GBaserEeeAddr;
    fm_uint32         pcs10GBaserEeeReg[FM10000_PCS_10GBASER_EEE_CFG_WIDTH];
    fm_status         status;
    fm_bool           eeeMode;
    fm_bool           regLockTaken=FALSE;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    /* Extract the port info from the port event info structure */
    attr      = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portPtr   = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    sw   = switchPtr->switchNumber;
    port = portPtr->portNumber;
    epl  = portExt->endpoint.epl;

    /* here we're assuming that the only lane in the linked lane list is
       the native lane. It should be a reasonable assumption */
    physLane = portExt->nativeLaneExt->physLane;

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* fill out the register template */
    pcs10GBaserAddr = FM10000_PCS_10GBASER_CFG( epl, physLane );
    pcs10GBaserReg  = 0;

    FM_SET_BIT( pcs10GBaserReg,
                FM10000_PCS_10GBASER_CFG,
                ScramblerBypass,
                FALSE );
    FM_SET_BIT( pcs10GBaserReg,
                FM10000_PCS_10GBASER_CFG,
                DescramblerBypass,
                FALSE );
    FM_SET_BIT( pcs10GBaserReg,
                FM10000_PCS_10GBASER_CFG,
                LsMaskSeenI,
                FALSE );
    FM_SET_BIT( pcs10GBaserReg,
                FM10000_PCS_10GBASER_CFG,
                LsMaskHiBer,
                FALSE );

    FM_SET_BIT( pcs10GBaserReg,
                FM10000_PCS_10GBASER_CFG,
                EnEee,
                attr->negotiatedEeeModeEnabled );


    /* Program EEE low-level timers */
    pcs10GBaserEeeAddr = FM10000_PCS_10GBASER_EEE_CFG( epl, physLane, 0 );
    FM_MEMSET_S( pcs10GBaserEeeReg, 
                 sizeof(pcs10GBaserEeeReg), 
                 0,
                 sizeof(pcs10GBaserEeeReg) );

    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        Pattern,
                        FM10000_PORT_EEE_10GBASER_PATTERN );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxTslTime,
                        FM10000_PORT_EEE_10GBASER_TSL_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxTslUnit,
                        FM10000_PORT_EEE_10GBASER_TSL_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxTqlTime,
                        FM10000_PORT_EEE_10GBASER_TQL_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxTqlUnit,
                        FM10000_PORT_EEE_10GBASER_TQL_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxTwlTime,
                        FM10000_PORT_EEE_10GBASER_TWL_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxTwlUnit,
                        FM10000_PORT_EEE_10GBASER_TWL_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxT1uTime,
                        FM10000_PORT_EEE_10GBASER_T1U_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        TxT1uUnit,
                        FM10000_PORT_EEE_10GBASER_T1U_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        RxTqrTime,
                        FM10000_PORT_EEE_10GBASER_TQR_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        RxTqrUnit,
                        FM10000_PORT_EEE_10GBASER_TQR_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        RxTwrTime,
                        FM10000_PORT_EEE_10GBASER_TWR_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        RxTwrUnit,
                        FM10000_PORT_EEE_10GBASER_TWR_TIMESCALE );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        RxTwtfTime,
                        FM10000_PORT_EEE_10GBASER_TWTF_TIMEOUT );
    FM_ARRAY_SET_FIELD( pcs10GBaserEeeReg,
                        FM10000_PCS_10GBASER_EEE_CFG,
                        RxTwtfUnit,
                        FM10000_PORT_EEE_10GBASER_TWTF_TIMESCALE );


    /* Write the FM10000_PCS_10GBASER_EEE_CFG register */
    status = switchPtr->WriteUINT32Mult( sw, 
                                         pcs10GBaserEeeAddr,
                                         FM10000_PCS_10GBASER_EEE_CFG_WIDTH,
                                         pcs10GBaserEeeReg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );



    /* Write the register */
    status = switchPtr->WriteUINT32( sw, pcs10GBaserAddr, pcs10GBaserReg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    FM_FLAG_DROP_REG_LOCK( sw );

    /* Notify the serdes about the EEE mode */
    eeeMode = ( attr->negotiatedEeeModeEnabled | (attr->dbgEeeMode != 0) );
    status = fm10000NotifyEeeModeChange( sw, port, eeeMode );

ABORT:

    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    return status;

}   /* end fm10000Init10GBaseR */




/*****************************************************************************/
/** fm10000InitMlBaseR
 * \ingroup intPort
 *
 * \desc            Function that configures the PCS layer for a 40G or 100G
 *                  Multi-Lane port in the context of a port state machine
 *                  configuration request
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000InitMlBaseR( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            epl;
    fm_switch        *switchPtr;
    fm_uint32         addr;
    fm_uint32         pcsMlBaseR;
    fm_status         status;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    /* Extract the port info from the port event info structure */
    epl       = ((fm10000_portSmEventInfo *)userInfo)->portExt->endpoint.epl;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;

    TAKE_REG_LOCK( sw );

    /* fill out the register template */
    addr = FM10000_PCS_ML_BASER_CFG( epl );
    pcsMlBaseR = 0;

    FM_SET_FIELD( pcsMlBaseR,
                  FM10000_PCS_ML_BASER_CFG,
                  AmTimeout,
                  0x3FFF );
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                ManualLaneSel,
                FALSE );
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                ScramblerBypass,
                FALSE );
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                DescramblerBypass,
                FALSE );
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                LsMaskSeenI,
                FALSE );
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                LsMaskHiBer,
                FALSE );

    /* This feature may not be kept. It should be enabled to optimize
       latency and it may make sense to tie it up to the LOW_LATENCY
       attribute */
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                AmSoonEn,
                FALSE );

    /* Should use an attribute for this? */
    FM_SET_BIT( pcsMlBaseR,
                FM10000_PCS_ML_BASER_CFG,
                UseCl91Align,
                FALSE );

    /* Write the register and return */
    status = switchPtr->WriteUINT32( sw, addr, pcsMlBaseR );

    /* Make sure taking the register lock is really needed */
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000InitMlBaseR */




/*****************************************************************************/
/** fm10000InitAn73
 * \ingroup intPort
 *
 * \desc            Function that configures the PCS layer for Clause 73
 *                  auto-negotiation in the context of a port state machine
 *                  configuration request
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000InitAn73( fm_smEventInfo *eventInfo, void *userInfo )
{
    FM_NOT_USED(eventInfo);
    FM_NOT_USED(userInfo);
    return FM_OK;

}   /* end fm10000InitAn73 */




/*****************************************************************************/
/** fm10000WriteEplCfgA
 * \ingroup intPort
 *
 * \desc            Sets the active bits in the EPL, which selects which
 *                  physLane on the EPL get muxed into the fabric.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
  *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteEplCfgA( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_status        status;
    fm_uint32        eplCfgA;
    fm_uint          addr;
    fm_int           i;
    fm_int           j;
    fm_int           fabricPort;
    fm_int           fabricPort2;
    fm_int           otherPorts[FM10000_PORTS_PER_EPL];
    fm_bool          active[FM10000_PORTS_PER_EPL];
    fm_ethMode       ethMode[FM10000_PORTS_PER_EPL];
    fm_ethMode       newEthMode;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    port        = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    epl         = portExt->endpoint.epl;
    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw          = switchPtr->switchNumber;
    fabricPort  = portExt->fabricPort;

    /* get the list of logical ports in the same EPL */
    for ( i = 0 ; i < FM10000_PORTS_PER_EPL ; i++ )
    {
        fabricPort2 = (fabricPort & EPL_PORT_GROUP_MASK) | i;

        /* Get that physical port's logical port number so we can access
         * its state. */
        status = fm10000MapFabricPortToLogicalPort( sw, 
                                                    fabricPort2, 
                                                    &otherPorts[i] );
        if ( status != FM_OK )
        {
            /* this lane isn't used */
            otherPorts[i] = -1;
        }
    }

    addr = FM10000_EPL_CFG_A(epl);

    TAKE_REG_LOCK(sw);

    status = switchPtr->ReadUINT32( sw, addr, &eplCfgA );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    /* Get the physical port number */
    newEthMode     = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;

    /* If port's ethMode on the target EPL is multi-lane, the active
     * bits must all be the same. */
    if (newEthMode & FM_ETH_MODE_4_LANE_BIT_MASK) 
    {
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

        active[0] = (portAttrExt->fabricLoopback == FM_PORT_LOOPBACK_OFF) &
                    (newEthMode != FM_ETH_MODE_DISABLED);

        for (j = 0 ; j < FM10000_PORTS_PER_EPL ; j++)
        {
            active[j] = active[0];
        }
    }
    else
    {
        /* For each of the 4 ports serviced by the EPL... */
        for ( i = 0 ; i < FM10000_PORTS_PER_EPL ; i++ )
        {
            if ( otherPorts[i] < 0 )
            {
                /* This could happen if at least one port of an EPL isn't used,
                   simply mark it as inactive and continue with next port. */
                active[i] = FALSE;
                continue;
            }

            portExt     = GET_PORT_EXT(sw, otherPorts[i]);
            portAttrExt = GET_FM10000_PORT_ATTR(sw, otherPorts[i]);

            ethMode[i] = portExt->ethMode;

            if (port == otherPorts[i])
            {
                ethMode[i] = newEthMode;
            }

            active[i] = (portAttrExt->fabricLoopback == FM_PORT_LOOPBACK_OFF) &
                        (ethMode[i] != FM_ETH_MODE_DISABLED);
        }
    }

    /* Set the Active field of all 4 channels according to whether the
     * associated ports have this mac as the active mac */
    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_0, active[0]);
    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_1, active[1]);
    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_2, active[2]);
    FM_SET_BIT(eplCfgA, FM10000_EPL_CFG_A, Active_3, active[3]);

    /* Should set up all other EPL_CFG_A register fields. */
    FM_SET_FIELD( eplCfgA, FM10000_EPL_CFG_A, Timeout, 19);
    FM_SET_FIELD( eplCfgA, FM10000_EPL_CFG_A, SkewTolerance, 38);

    /* Update the hardware. */
    status = switchPtr->WriteUINT32( sw, addr, eplCfgA );
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

ABORT:
    DROP_REG_LOCK(sw);

    return status;

}   /* end fm10000WriteEplCfgA */




/*****************************************************************************/
/** fm10000WriteEplCfgB
 * \ingroup intPort
 *
 * \desc            Configures the EPL_CFG_B register.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000WriteEplCfgB( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_int            physLane;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_int            pcsType[FM10000_PORTS_PER_EPL];
    fm_int            qplMode;
    fm_status         status;
    fm_uint32         eplCfgB;
    fm_uint           addr;
    fm_int            i;
    fm_int            fabricPort;
    fm_int            otherFabricPort;
    fm_int            otherPorts[FM10000_PORTS_PER_EPL];
    fm_bool           disabled = TRUE;
    fm_bool           laneCountChange;
    fm_bool           portIsDisabled;
    fm_bool           portWillBeDisabled;
    fm_ethMode        newEthMode;
    fm_uint32         newSpeed;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    port        = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    newEthMode  = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
    newSpeed    = ((fm10000_portSmEventInfo *)userInfo)->info.config.speed;

    sw         = switchPtr->switchNumber;
    epl        = portExt->endpoint.epl;
    physLane   = portExt->nativeLaneExt->physLane;

    /* Determine if we're switching from single- to multi-lane or viceversa */
    laneCountChange = FALSE;
    if (( ( portExt->ethMode ^ newEthMode ) & FM_ETH_MODE_4_LANE_BIT_MASK )) 
    {
        laneCountChange = TRUE;
    }

    /* Determine if the port is currently disabled */
    portIsDisabled = FALSE;
    if ( portExt->ethMode == FM_ETH_MODE_DISABLED )
    {
        portIsDisabled = TRUE;
    }

    /* Determine if the port will be disabled */
    portWillBeDisabled = FALSE;
    if ( newEthMode == FM_ETH_MODE_DISABLED )
    {
        portWillBeDisabled = TRUE;
    }


    /* Determine the other logical ports on the same EPL for later use */
    fabricPort = portExt->fabricPort;
    for ( i = 0 ; i < FM10000_PORTS_PER_EPL ; i++ )
    {
        otherFabricPort = (fabricPort & EPL_PORT_GROUP_MASK) | i;

        status = fm10000MapFabricPortToLogicalPort( sw,
                                                    otherFabricPort, 
                                                    &otherPorts[i] );
        if ( status != FM_OK )
        {
            otherPorts[i] = -1;
        }
    }


    addr = FM10000_EPL_CFG_B( epl );

    TAKE_REG_LOCK( sw );

    status = switchPtr->ReadUINT32( sw, addr, &eplCfgB );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* read the PCS type for all lanes for later processing */
    pcsType[0] = FM_GET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port0PcsSel );
    pcsType[1] = FM_GET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port1PcsSel );
    pcsType[2] = FM_GET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port2PcsSel );
    pcsType[3] = FM_GET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port3PcsSel );
    qplMode    = FM_GET_FIELD( eplCfgB, FM10000_EPL_CFG_B, QplMode );

    /* set the PCS type to disabled unless the port is already disabled */
    if ( !portIsDisabled )
    {
        /* if there is a lane count change disable PCS on all lanes */
        if ( laneCountChange )
        {
            pcsType[0] =  FM10000_PCS_SEL_DISABLE;
            pcsType[1] =  FM10000_PCS_SEL_DISABLE;
            pcsType[2] =  FM10000_PCS_SEL_DISABLE;
            pcsType[3] =  FM10000_PCS_SEL_DISABLE;
        }
        else
        {
            pcsType[physLane] =  FM10000_PCS_SEL_DISABLE;
        }

        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port0PcsSel, pcsType[0] );
        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port1PcsSel, pcsType[1] );
        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port2PcsSel, pcsType[2] );
        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port3PcsSel, pcsType[3] );

        /* Write back with one or more lanes now disabled */
        status = switchPtr->WriteUINT32( sw, addr, eplCfgB );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    }   /* end if ( !portIsDisabled ) */


    /* Switching from Quad mode on non-zero lane will cause
     * PORT_STATUS.SerXmit = 0 unless EPL_CFG_B.Port0PcsSel
     * is toggled with value > 1.
     */
    if ((qplMode > FM10000_QPL_MODE_L1_L1_L1_L1) && 
        !(newEthMode & FM_ETH_MODE_4_LANE_BIT_MASK))
    {
        FM_LOG_DEBUG( FM_LOG_CAT_PORT, 
                     "Port %d Switching out of 4-lane mode\n", port);
        eplCfgB = 0x20000;
        status = switchPtr->WriteUINT32( sw, addr, eplCfgB );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        eplCfgB = 0x10000;
        status = switchPtr->WriteUINT32( sw, addr, eplCfgB );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        eplCfgB = 0x16666;
        status = switchPtr->WriteUINT32( sw, addr, eplCfgB );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        eplCfgB = 0x10000;
        status = switchPtr->WriteUINT32( sw, addr, eplCfgB );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }


    /* now set the PCS type to the appropriate one unless it's disabled
       in that case we've done the job already */
    if ( !portWillBeDisabled )
    {
        /* scan for unused lanes on this EPL */
        for ( i = 0 ; i < FM10000_PORTS_PER_EPL ; i++ )
        {
            if ( otherPorts[i] < 0 )
            {
                /* lane unused, simply mark it as DISABLED. */
                pcsType[i] = FM10000_PCS_SEL_DISABLE;
            }
            disabled &= ( pcsType[i] == FM10000_PCS_SEL_DISABLE );
        }


        pcsType[physLane] = fm10000GetPcsType( newEthMode, newSpeed );
        disabled &= ( pcsType[physLane] == FM10000_PCS_SEL_DISABLE );

        /* lane-to-port allocation based on the Ethernet Interface Mode */
        if ( disabled )
        {
            FM_SET_FIELD( eplCfgB, 
                          FM10000_EPL_CFG_B, 
                          QplMode, 
                          FM10000_QPL_MODE_XX_XX_XX_XX );
        }
        else if ( newEthMode & FM_ETH_MODE_4_LANE_BIT_MASK )
        {
            /* any of the multi-lane modes */
            if ( physLane == 0 )
            {
                /* Multi-lane with port 0 as the master port */
                FM_SET_FIELD( eplCfgB, 
                              FM10000_EPL_CFG_B, 
                              QplMode, 
                              FM10000_QPL_MODE_L4_XX_XX_XX );
            }
            else if ( physLane == 1 )
            {
                /* Multi-lane with port 1 as the master port */
                FM_SET_FIELD( eplCfgB, 
                              FM10000_EPL_CFG_B, 
                              QplMode, 
                              FM10000_QPL_MODE_XX_L4_XX_XX );
            }
            else if ( physLane == 2 )
            {
                /* Multi-lane with port 2 as the master port */
                FM_SET_FIELD( eplCfgB, 
                              FM10000_EPL_CFG_B, 
                              QplMode, 
                              FM10000_QPL_MODE_XX_XX_L4_XX );
            }
            else
            {
                /* Multi-lane with port 3 as the master port */
                FM_SET_FIELD( eplCfgB, 
                              FM10000_EPL_CFG_B, 
                              QplMode, 
                              FM10000_QPL_MODE_XX_XX_XX_L4 );
            }
        }
        else
        {
            /* single lane mode */
            FM_SET_FIELD( eplCfgB, 
                          FM10000_EPL_CFG_B, 
                          QplMode, 
                          FM10000_QPL_MODE_L1_L1_L1_L1 );
        }

        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port0PcsSel, pcsType[0] );
        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port1PcsSel, pcsType[1] );
        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port2PcsSel, pcsType[2] );
        FM_SET_FIELD( eplCfgB, FM10000_EPL_CFG_B, Port3PcsSel, pcsType[3] );

        status = switchPtr->WriteUINT32( sw, addr, eplCfgB );
        FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    }   /* if ( !portWillBeDisabled ) */

ABORT:
    DROP_REG_LOCK(sw);

    return status;

}   /* end fm10000WriteEplCfgB */




/*****************************************************************************/
/** fm10000WriteMac
 * \ingroup intPort
 *
 * \desc            Configures the MAC_CFG register.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000WriteMac( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_int            physLane;
    fm_port          *portPtr;
    fm_portAttr      *portAttr;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_status         status;
    fm_status         statusLoc;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_uint32         pcsType;
    fm_uint           addr;
    fm_uint32         antiBubbleWm;
    fm_ethMode        ethMode;
    fm_uint32         speed;
    fm_uint           txAntiBubbleWatermarkValue;
    fm_uint           txRateFifoWatermarkValue;
    fm_uint           txRateFifoSlowIncValue;
    fm_uint           txRateFifoFastIncValue;
    fm_bool           dicEnable;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    port        = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    epl         = ((fm10000_portSmEventInfo *)userInfo)->portExt->endpoint.epl;
    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portPtr     = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    ethMode     = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
    speed       = ((fm10000_portSmEventInfo *)userInfo)->info.config.speed;
    sw          = switchPtr->switchNumber;
    physLane    = portExt->nativeLaneExt->physLane;


    addr = FM10000_MAC_CFG(epl, physLane, 0);

    pcsType = fm10000GetPcsType( ethMode, speed );
    switch (pcsType)
    {
        case FM10000_PCS_SEL_10GBASER:
        case FM10000_PCS_SEL_40GBASER:
        case FM10000_PCS_SEL_100GBASER:
            dicEnable = portAttr->dicEnable;
            break;

        default:
            dicEnable = FALSE;
            break;

    }   /* end switch (pcsType) */


    TAKE_REG_LOCK(sw);


    status = switchPtr->ReadUINT32Mult(sw,
                                       addr,
                                       FM10000_MAC_CFG_WIDTH,
                                       macCfg);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    fm10000WriteMacDicEnable( sw, epl, physLane, macCfg, dicEnable );
    fm10000WriteMacIfg( sw, epl, physLane, macCfg, portAttr->ifg );
    fm10000WriteMacMinColumns( sw, 
                               epl, 
                               physLane, 
                               macCfg, 
                               portAttrExt->txPadSize );
    fm10000WriteMacTxClockCompensation( sw, 
                                        epl, 
                                        physLane, 
                                        macCfg, 
                                        portAttr->txClkCompensation,
                                        pcsType,
                                        speed );

    FM_ARRAY_SET_BIT( macCfg,
                      FM10000_MAC_CFG,
                      PreambleMode,
                      (portAttr->islTagFormat == FM_ISL_TAG_F56) );
    FM_ARRAY_SET_FIELD( macCfg, 
                        FM10000_MAC_CFG, 
                        RxMinFrameLength,
                        portAttr->minFrameSize );
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        RxMaxFrameLength,
                        portAttr->maxFrameSize );
    FM_ARRAY_SET_BIT( macCfg,
                      FM10000_MAC_CFG,
                      RxIgnoreIfgErrors,
                      portAttr->ignoreIfgErrors );

    FM_ARRAY_SET_FIELD(macCfg,
                       FM10000_MAC_CFG,
                       TxFcsMode,
                       portAttrExt->txFcsMode);


    antiBubbleWm = fmGetIntApiProperty( FM_AAK_API_FM10000_ANTI_BUBBLE_WM,
                                        FM_AAD_API_FM10000_ANTI_BUBBLE_WM );
    FM_ARRAY_SET_FIELD( macCfg, 
                        FM10000_MAC_CFG, 
                        TxAntiBubbleWatermark, 
                        antiBubbleWm );

    /* Need low latency configuration */

    /* Configure IEEE 1588 timestamping. */
    FM_ARRAY_SET_BIT( macCfg, 
                      FM10000_MAC_CFG, 
                      Ieee1588Enable,
                      portAttrExt->generateTimestamps );

    /* set the default tx drain mode to drain when link is down */
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxDrainMode,
                        FM10000_PORT_TX_DRAIN_ON_LINK_DOWN );

    /* TX LPI configuration. Need adaptive activity timeout */
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxPcActTimeout,
                        portAttrExt->txPcActTimeout );
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxPcActTimeScale,
                        portAttrExt->txPcActTimescale );
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxLpiTimeout,
                        portAttrExt->txLpiTimeout );
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxLpiTimescale,
                        portAttrExt->txLpiTimescale );
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxLpiHoldTimeout,
                        FM10000_PORT_EEE_TX_LPI_HOLD_TIMEOUT );
    FM_ARRAY_SET_FIELD( macCfg,
                        FM10000_MAC_CFG,
                        TxLpiHoldTimescale,
                        FM10000_PORT_EEE_TX_LPI_HOLD_TIMESCALE );

    /* Enabled exit LPI automatic mode */
    FM_ARRAY_SET_BIT( macCfg, FM10000_MAC_CFG, TxLpiAutomatic, TRUE );

    /* start in normal mode, i.e. do not send /LI/ codes */
    FM_ARRAY_SET_BIT( macCfg, FM10000_MAC_CFG, TxLpIdleRequest, FALSE );

    FM_ARRAY_SET_FIELD( macCfg, 
                        FM10000_MAC_CFG, 
                        TxFaultMode,
                        ConvertAdminModeToTxLinkFault( portPtr->mode ) );

    /* set TxRateFifoWatermark, TxRateFifoFastInc and TxRateFifoSlowInc
     * The values to be set depend on the port speed */

    if (ethMode != FM_ETH_MODE_DISABLED)
    {
        statusLoc = GetTxFifoConfig (ethMode,
                                     &txAntiBubbleWatermarkValue,
                                     &txRateFifoWatermarkValue,
                                     &txRateFifoSlowIncValue,
                                     &txRateFifoFastIncValue);
        if (statusLoc == FM_OK)
        {
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxAntiBubbleWatermark,
                                txAntiBubbleWatermarkValue );
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxRateFifoWatermark,
                                txRateFifoWatermarkValue );
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxRateFifoFastInc,
                                txRateFifoFastIncValue );
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxRateFifoSlowInc,
                                txRateFifoSlowIncValue );
        }
        else if (statusLoc == FM_ERR_INVALID_ARGUMENT)
        {
            /* this is a non recoverable error */
            status = statusLoc;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }
        else
        {
            /* only log the error here (do not copy local status into status),
             * this should be a recoverable error */
            FM_LOG_ERROR_V2(FM_LOG_CAT_PORT, port, "Invalid ethernet mode configuring MAC Tx Fifo\n");
        }    

    }   /* end if (ethMode != FM_ETH_MODE_DISABLED) */

    /* write it back */
    status = switchPtr->WriteUINT32Mult( sw,
                                         addr,
                                         FM10000_MAC_CFG_WIDTH,
                                         macCfg );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

ABORT:
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000WriteMac */




/*****************************************************************************/
/** fm10000EnableDrainMode
 * \ingroup intPort
 *
 * \desc            Enables Tx Drain mode in the context of a port state
 *                  machine event
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000EnableDrainMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    FM_NOT_USED( eventInfo );
    return SetDrainMode( userInfo, TRUE );

}   /* end fm10000EnableDrainMode */




/*****************************************************************************/
/** fm10000DisableDrainMode
 * \ingroup intPort
 *
 * \desc            Disables Tx Drain mode in the context of a port state
 *                  machine event
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DisableDrainMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    FM_NOT_USED( eventInfo );
    return SetDrainMode( userInfo, FALSE );

}   /* end fm10000DisableDrainMode */



/*****************************************************************************/
/** fm10000EnableLinkInterrupts
 * \ingroup intPort
 *
 * \desc            Enables all relevant link interrupts in the context of a
 *                  state machine event on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 ****************************************************************************/
fm_status fm10000EnableLinkInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_status     status;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    /* Unmask the required interrupts */
    TAKE_REG_LOCK( sw );

    status = switchPtr->MaskUINT32( sw, 
                                    FM10000_LINK_IP(epl, physLane), 
                                    ~portExt->linkInterruptMask, 
                                    FALSE );

    status = switchPtr->MaskUINT32( sw, 
                                    FM10000_LINK_IM(epl, physLane),
                                    portExt->linkInterruptMask,
                                    FALSE );
    
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000EnableLinkInterrupts */




/*****************************************************************************/
/** fm10000DisableLinkInterrupts
 * \ingroup intPort
 *
 * \desc            Disables all relevant link interrupts in the context of a
 *                  state machine event on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DisableLinkInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_status     status;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    /* Mask the required interrupts */
    TAKE_REG_LOCK( sw );
    status = switchPtr->MaskUINT32( sw, 
                                    FM10000_LINK_IM(epl, physLane),
                                    portExt->linkInterruptMask,
                                    TRUE );
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000DisableLinkInterrupts */




/*****************************************************************************/
/** fm10000PowerUpLaneRx
 * \ingroup intPort
 *
 * \desc            Power up the receiver of the lane(s) associated to a port
 *                  in the context of an event notified on that port's state
 *                  machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000PowerUpLaneRx( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm10000_lane   *laneExt;

    portExt    = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    /* prepare the lane mask */
    portExt->lockedLaneMask  = 0;
    portExt->desiredLaneMask = 0;
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        portExt->desiredLaneMask |= (1 << laneExt->physLane );
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    return SerDesEventReq( eventInfo, 
                           userInfo, 
                           FM10000_SERDES_EVENT_RX_POWERUP_REQ );

}   /* end fm10000PowerUpLaneRx */




/*****************************************************************************/
/** fm10000ResetPortModuleState
 * \ingroup intPort
 *
 * \desc            Internal function to reset the state of specified port
 *                  module to its default value, which is:                 \lb
 *                   FM_PORT_XCVRSIG_MODPRES = asserted                    \lb
 *                   FM_PORT_XCVRSIG_RXLOS   = de-asserted.                \lb
 *
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ResetPortModuleState( fm_smEventInfo *eventInfo, void *userInfo ) 
{
    fm_status     status;
    fm_status     fStatus;
    fm_int        sw;
    fm_int        port;
    fm10000_lane *laneExt;
    fm10000_port *portExt;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw      = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port    = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    fStatus = FM_OK;
    status  = FM_OK;

    /* prepare the lane mask */
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        status = fm10000SetPortModuleState( sw,
                                            port,
                                            laneExt->lane,
                                            FM_PORT_XCVRSIG_MODPRES );
        if (status != FM_OK)
        {
            /* if there is an error: trace it and continue with the other
               lanes */
            FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                             port,
                             "Cannot reset port module state: port=%d lane=%d\n",
                             port,
                             laneExt->lane );

            /* keep only the first error code */
            fStatus = (fStatus ==  FM_OK) ? status : fStatus;
        }

        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }


    return (fStatus == FM_OK) ? status : fStatus;

}   /* end fm10000ResetPortModuleState */




/*****************************************************************************/
/** fm10000EnableBistMode
 * \ingroup intPort
 *
 * \desc            Enable a test mode on the lane(s) associated to a port in
 *                  the context of an event notified on that port's state
 *                  machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000EnableBistMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status       err;
    fm_int          subMode;
    fm10000_port   *portExt;
    fm10000_lane   *pLaneExt;

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    pLaneExt = FM_DLL_GET_FIRST( portExt, firstLane );

    subMode  = ((fm10000_portSmEventInfo *)userInfo)->info.admin.submode;
    while ( pLaneExt != NULL )
    {
        pLaneExt->eventInfo.info.bistSubmode = subMode;
        pLaneExt = FM_DLL_GET_NEXT( pLaneExt, nextLane );
    }

    err = SerDesEventReq( eventInfo, userInfo, FM10000_SERDES_EVENT_ENABLE_BIST_REQ );

    return err;

}   /* end fm10000EnableBistMode */




/*****************************************************************************/
/** fm10000PowerDownLaneTx
 * \ingroup intPort
 *
 * \desc            Power down the transmitter(s) of the lane(s) associated to
 *                  a port in the context of an event notified on that port's
 *                  state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000PowerDownLaneTx( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SerDesEventReq( eventInfo, 
                           userInfo, 
                           FM10000_SERDES_EVENT_TX_POWERDOWN_REQ );

}   /* end fm10000PowerDownLaneTx */




/*****************************************************************************/
/** fm10000EnableLoopback
 * \ingroup intPort
 *
 * \desc            Enable loopback on the lane(s) associated to a port in the
 *                  context of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000EnableLoopback( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SerDesEventReq( eventInfo, 
                           userInfo, 
                           FM10000_SERDES_EVENT_LOOPBACK_ON_REQ );

}   /* end fm10000EnableLoopback */




/*****************************************************************************/
/** fm10000PowerDownLaneRx
 * \ingroup intPort
 *
 * \desc            Power down the receiver(s) of the lane(s) associated to
 *                  a port in the context of an event notified on that port's
 *                  state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000PowerDownLaneRx( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SerDesEventReq( eventInfo, 
                           userInfo, 
                           FM10000_SERDES_EVENT_RX_POWERDOWN_REQ );

}   /* end fm10000PowerDownLaneRx */




/*****************************************************************************/
/** fm10000DisablePhy
 * \ingroup intPort
 *
 * \desc            Disables the PHY, if any, in the context of a state machine
 *                  event on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DisablePhy( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int      sw;
    fm_int      physPort;
    fm_port    *portPtr;
    fm_status   status = FM_OK;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    physPort = portPtr->physicalPort;

    if ( portPtr->phyInfo.phyDisable )
    {
        status = portPtr->phyInfo.phyDisable( sw, physPort, 0, portPtr );
    }

    return status;

}   /* end fm10000DisablePhy */




/*****************************************************************************/
/** fm10000SetTxFaultMode
 * \ingroup intPort
 *
 * \desc            Set the required fault mode
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetTxFaultMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_int        port;
    fm10000_port *portExt;
    fm_int        mode;

    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt  = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port     = portExt->base->portNumber;
    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

    mode    = ((fm10000_portSmEventInfo *)userInfo)->info.admin.mode;

    return SetTxFaultModeInMacCfg( sw, port, epl, physLane, mode );

}   /* end fm10000SetTxFaultMode */




/*****************************************************************************/
/** fm10000RestoreTxFaultMode
 * \ingroup intPort
 *
 * \desc            Restore the current Tx Fault Mode
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000RestoreTxFaultMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_int        port;
    fm10000_port *portExt;
    fm_port      *portPtr;

    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portExt  = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port     = portPtr->portNumber;
    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

    return SetTxFaultModeInMacCfg( sw, port, epl, physLane, portPtr->mode );

}   /* end fm10000RestoreTxFaultMode */




/*****************************************************************************/
/** fm10000SetTxFaultModeToNormal
 * \ingroup intPort
 *
 * \desc            Set the required fault mode to normal
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000SetTxFaultModeToNormal( fm_smEventInfo *eventInfo, 
                                         void           *userInfo )
{
    fm_int        sw;
    fm_int        epl;
    fm_int        physLane;
    fm_int        port;
    fm10000_port *portExt;

    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt  = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port     = portExt->base->portNumber;
    epl      = portExt->endpoint.epl;
    physLane = portExt->nativeLaneExt->physLane;

    return SetTxFaultModeInMacCfg( sw, port, epl, physLane, FM_PORT_MODE_UP );

}   /* end fm10000SetTxFaultModeToNormal */





/*****************************************************************************/
/** fm10000DisableBistMode
 * \ingroup intPort
 *
 * \desc            Enable a test mode on the lane(s) associated to a port in
 *                  the context of an event notified on that port's state
 *                  machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000DisableBistMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SerDesEventReq( eventInfo,
                           userInfo,
                           FM10000_SERDES_EVENT_DISABLE_BIST_REQ );

}   /* end fm10000DisableBistMode */




/*****************************************************************************/
/** fm10000EnablePhy
 * \ingroup intPort
 *
 * \desc            Enables the PHY, if any, in the context of a state machine
 *                  event on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000EnablePhy( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int      sw;
    fm_int      physPort;
    fm_port    *portPtr;
    fm_status   status = FM_OK;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    physPort = portPtr->physicalPort;

    if ( portPtr->phyInfo.phyEnable )
    {
        status = portPtr->phyInfo.phyEnable( sw, physPort, 0, portPtr );
    }

    return status;

}   /* end fm10000EnablePhy */




/*****************************************************************************/
/** fm10000PowerUpLaneTx
 * \ingroup intPort
 *
 * \desc            Power up the transmitter of the lane(s) associated to a
 *                  port in the context of an event notified on that port's
 *                  state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000PowerUpLaneTx( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port   *portExt;
    fm10000_lane   *laneExt;

    portExt    = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    /* prepare the lane mask */
    portExt->lockedLaneMask  = 0;
    portExt->desiredLaneMask = 0;
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        portExt->desiredLaneMask |= (1 << laneExt->physLane );
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    return SerDesEventReq( eventInfo,
                           userInfo,
                           FM10000_SERDES_EVENT_TX_POWERUP_REQ );

}   /* end fm10000PowerUpLaneTx */




/*****************************************************************************/
/** fm10000DisableLoopback
 * \ingroup intPort
 *
 * \desc            Disable loopback on the lane(s) associated to a port in the
 *                  context of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000DisableLoopback( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SerDesEventReq( eventInfo,
                           userInfo,
                           FM10000_SERDES_EVENT_LOOPBACK_OFF_REQ );

}   /* end fm10000DisableLoopback */




/*****************************************************************************/
/** fm10000EnableFabricLoopback
 * \ingroup intPort
 *
 * \desc            Enable loopback on the lane(s) associated to a port in the
 *                  context of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000EnableFabricLoopback( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_switch    *switchPtr;
    fm_int        port;
    fm_ethMode    ethMode;
    fm10000_port *portExt;


    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    port      = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    portExt   = GET_PORT_EXT(sw, port);
    ethMode   = portExt->ethMode;

    err = SetFabricLoopbackFlags(sw, switchPtr, port, ethMode, TRUE);

    return err;

}   /* end fm10000EnableFabricLoopback */




/*****************************************************************************/
/** fm10000DisableFabricLoopback
 * \ingroup intPort
 *
 * \desc            Disable loopback on the lane(s) associated to a port in the
 *                  context of an event notified on that port's state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * 
 *****************************************************************************/
fm_status fm10000DisableFabricLoopback( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status     err;
    fm_int        sw;
    fm_switch    *switchPtr;
    fm_int        port;
    fm_ethMode    ethMode;
    fm10000_port *portExt;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    port    = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    portExt   = GET_PORT_EXT(sw, port);
    ethMode   = portExt->ethMode;

    err = SetFabricLoopbackFlags(sw, switchPtr, port, ethMode, FALSE);

    return err;

}   /* end fm10000DisableFabricLoopback */




/*****************************************************************************/
/** fm10000ClearEplFifo
 * \ingroup intPort
 *
 * \desc            Clear the EPL_FIFO_ERROR_STATUS register for the lane
 *                  associated on a given port in the context of a state
 *                  machine event on that port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ClearEplFifo( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status     status;
    fm_uint32     clearMask;
    fm_int        sw;
    fm_int        port;
    fm_int        epl;
    fm_int        physLane;
    fm_uint32     addr;
    fm_switch    *switchPtr;
    fm10000_port *portExt;

    FM_NOT_USED( eventInfo );

    port      = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    sw        = switchPtr->switchNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    /* clear the TxFifoError and RxFifoError bit corresponding to this lane */
    clearMask = 0;
    clearMask |= (( 1 << physLane ) << 0);
    clearMask |= (( 1 << physLane ) << 4);

    addr   = FM10000_EPL_FIFO_ERROR_STATUS( epl );

    /* No real need to take the lock */
    TAKE_REG_LOCK( sw );
    status = switchPtr->WriteUINT32( sw, addr, clearMask );
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000ClearEplFifo */




/*****************************************************************************/
/** fm10000StartDeferralTimer
 * \ingroup intPort
 *
 * \desc            Action starting an 10 msec timer deferring the completion
 *                  of the SerDes PLL power up process
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StartDeferralTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;
    fm_timestamp  timeout = { 0, 10000 };

    FM_NOT_USED(eventInfo);

    /* start an 10 msec port-level timer. The callback will generate
       a FM10000_PORT_EVENT_DEFTIMER_EXP_IND event */
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    return  fmStartTimer( portExt->timerHandle,
                          &timeout,
                          1, 
                          HandleDeferralTimeout,
                          portExt );

}   /* end fm10000StartDeferralTimer */




/*****************************************************************************/
/** fm10000StopDeferralTimer
 * \ingroup intPort
 *
 * \desc            Action stopping the 8 msec timer deferring the completion
 *                  of the SerDes PLL power up process
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StopDeferralTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;

    FM_NOT_USED(eventInfo);

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    return  fmStopTimer( portExt->timerHandle );

}   /* end fm10000StopDeferralTimer */




/*****************************************************************************/
/** fm10000StartAnWatchDogTimer
 * \ingroup intPort
 *
 * \desc            Action starting an 1 sec watchdog timer to monitor AN
 *                  operation.
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StartAnWatchDogTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;
    fm_timestamp  timeout = { 2, 0 };

    FM_NOT_USED(eventInfo);

    /* start an 1 sec port-level timer. The callback will generate
       a FM10000_PORT_EVENT_DEFTIMER_EXP_IND event */
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    return  fmStartTimer( portExt->timerHandle,
                          &timeout,
                          1, 
                          HandleDeferralTimeout,
                          portExt );

}   /* end fm10000StartAnWatchDogTimer */




/*****************************************************************************/
/** fm10000StopAnWatchDogTimer
 * \ingroup intPort
 *
 * \desc            Action stopping the AN watchdog timer 
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StopAnWatchDogTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;

    FM_NOT_USED(eventInfo);

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    return  fmStopTimer( portExt->timerHandle );

}   /* end fm10000StopAnWatchDogTimer */




/*****************************************************************************/
/** fm10000StartDeferredLpiTimer
 * \ingroup intPort
 *
 * \desc            Action starting an 100 msec delay timer to enable Low
 *                  Power Idle mode
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StartDeferredLpiTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;
    fm_timestamp  timeout = { 1, 0 };

    FM_NOT_USED(eventInfo);

    /* start an 1 sec port-level timer. The callback will generate
       a FM10000_PORT_EVENT_DEFTIMER_EXP_IND event that will be used
       to enable LPI mode */
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    return  fmStartTimer( portExt->timerHandle,
                          &timeout,
                          1, 
                          HandleDeferralTimeout,
                          portExt );

}   /* end fm10000StartDeferredLpiTimer */




/*****************************************************************************/
/** fm10000StopDeferredLpiTimer
 * \ingroup intPort
 *
 * \desc            Action stopping the deferred LPI enable timer 
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000StopDeferredLpiTimer( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;

    FM_NOT_USED(eventInfo);

    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    return  fmStopTimer( portExt->timerHandle );

}   /* end fm10000StopDeferredLpiTimer */




/*****************************************************************************/
/** fm10000DeferredLpiMode
 * \ingroup intPort
 *
 * \desc            Action entering to the LPI mode is the port is silent
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000DeferredLpiMode( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status         status;
    fm_port          *portPtr;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_int            physLane;
    fm_uint           addr;
    fm_uint32         portStatus;
    fm_bool           eeeSlPcSilent;

    FM_NOT_USED(eventInfo);

    portPtr     = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw          = switchPtr->switchNumber;
    port        = portPtr->portNumber;
    epl         = portExt->endpoint.epl;
    physLane    = portExt->nativeLaneExt->physLane;

    status = FM_OK;

    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
         || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (portPtr->portNumber != 0) ) )
    {
        if ( portAttrExt->negotiatedEeeModeEnabled == TRUE )
        {
            /* Energy-Efficient Ethernet (EEE) Enabled */

            /* Read the port status and extract the relevant field */
            addr = FM10000_PORT_STATUS( epl, physLane );
            status = switchPtr->ReadUINT32( sw, addr, &portStatus );
            FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

            /* Check for traffic activity */
            eeeSlPcSilent = FM_GET_BIT( portStatus, 
                                        FM10000_PORT_STATUS, 
                                        EeeSlPcSilent );
            if (eeeSlPcSilent)
            {
                /* Port is silent; go in Low Power Idle mode */
                status = fm10000EnableLowPowerIdle( eventInfo, userInfo );
                FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);
            }
        }
    }
ABORT:
    return  status;

}   /* end fm10000DeferredLpiMode */




/*****************************************************************************/
/** fm10000ConfigureDfe
 * \ingroup intPort
 *
 * \desc            Action sending a DFE configuration request to the SerDes
 *                  state machine
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000ConfigureDfe( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status       status = FM_OK;
    fm10000_port   *portExt;
    fm10000_lane   *laneExt;
    fm_int          sw;
    fm_int          port;
    fm_int          serDes;
    fm_int          lane;
    fm_dfeMode      dfeMode;
    fm_smEventInfo  laneEventInfo;


    FM_NOT_USED(eventInfo);

    sw      = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port    = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    dfeMode = ((fm10000_portSmEventInfo *)userInfo)->info.dfe.mode;
    lane    = ((fm10000_portSmEventInfo *)userInfo)->info.dfe.lane;

    /* determine the serDes ID based on port and relative lane ID */
    status = fm10000MapPortLaneToSerdes( sw, port, lane, &serDes );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT, 
                     port, 
                     "port=%d lane=%d serDes=%d\n",
                     port, 
                     lane,
                     serDes );

    /* prepare the event notification */
    laneExt = GET_LANE_EXT( sw, serDes );
    laneEventInfo.eventId = FM10000_SERDES_EVENT_CONFIGURE_DFE_REQ;
    laneEventInfo.smType = laneExt->smType;
    laneEventInfo.lock = FM_GET_STATE_LOCK( sw );
    laneEventInfo.dontSaveRecord = FALSE;
    laneExt->eventInfo.info.dfeMode = dfeMode;

    /* then send it */
    status =  fmNotifyStateMachineEvent( laneExt->smHandle,
                                         &laneEventInfo,
                                         &laneExt->eventInfo,
                                         &laneExt->serDes );
ABORT:
    return status;

}   /* end fm10000ConfigureDfe */




/*****************************************************************************/
/** fm10000RestoreDfe
 * \ingroup intPort
 *
 * \desc            Action restoring the current DFE configuration for all
 *                  lanes belonging to a given port
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000RestoreDfe( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status       status = FM_OK;
    fm10000_port   *portExt;
    fm10000_lane   *laneExt;
    fm_int          sw;
    fm_int          port;
    fm_ethMode      ethMode;
    fm_dfeMode      dfeMode;
    fm_smEventInfo  laneEventInfo;

    FM_NOT_USED(eventInfo);

    sw      = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    port    = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    if ( eventInfo->eventId == FM10000_PORT_EVENT_CONFIG_REQ )
    {
        ethMode = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
    }
    else
    {
        ethMode = portExt->ethMode;
    }

    laneEventInfo.eventId = FM10000_SERDES_EVENT_CONFIGURE_DFE_REQ;
    laneEventInfo.lock = FM_GET_STATE_LOCK( sw );
    laneEventInfo.dontSaveRecord = FALSE;

    /* scan the list of lanes associated to this port */
    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        /* prepare the event notification */
        laneEventInfo.smType = laneExt->smType;

        status = fm10000MapEthModeToDfeMode( sw, 
                                             port, 
                                             laneExt->lane, 
                                             ethMode,
                                             &dfeMode);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        laneExt->eventInfo.info.dfeMode = dfeMode;

        /* then send it */
        status =  fmNotifyStateMachineEvent( laneExt->smHandle,
                                             &laneEventInfo,
                                             &laneExt->eventInfo,
                                             &laneExt->serDes );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        /* next lane, if any */
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );

    }   /* while ( laneExt != NULL) */

ABORT:
    return status;

}   /* end fm10000RestoreDfe */




/*****************************************************************************/
/** fm10000LinkPortToLanes
 * \ingroup intPort
 *
 * \desc            Action linking a port to one or more lanes depending on the
 *                  ethernet interface mode on the port
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000LinkPortToLanes( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status         status;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm10000_lane     *laneExt;
    fm_ethMode        curEthMode;
    fm_ethMode        newEthMode;
    fm_int            fabricPort;
    fm_int            baseFabricPort;
    fm_int            serDes;
    fm_int            baseSerDes;
    fm_serdesRing     ring;
    fm_int            numLanes;
    fm_int            i;
    fm_int            sw;
    fm_int            port;
    fm_pepMode        oldPepMode;
    fm_pepMode        newPepMode;

    sw        = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    port        = portExt->base->portNumber;

    /* do it only if there's a lane count change */
    status = FM_OK;
    if ( portExt->ring == FM10000_SERDES_RING_EPL )
    {
        newEthMode = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
        curEthMode = portExt->ethMode;

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port, 
                         "port=%d newEthMode=0x%08x curEthMode=0x%08x\n",
                         port, 
                         newEthMode, 
                         curEthMode );
        

        if ( ( (newEthMode ^ curEthMode) & 
              ( FM_ETH_MODE_MULTI_LANE_MASK | FM_ETH_MODE_ENABLED_BIT_MASK ) ) )
        {
            status = fm10000GetNumEthLanes( newEthMode, &numLanes );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            /* first unlink all current lanes */
            status = UnlinkAllLanes( portExt );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            fabricPort = portExt->fabricPort;

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port, 
                             "port=%d fabricPort=%d newEthMode=0x%08x numLanes=%d\n",
                             port, fabricPort, newEthMode, numLanes );

            if ( newEthMode & FM_ETH_MODE_4_LANE_BIT_MASK )
            {
                baseFabricPort = ( fabricPort & EPL_PORT_GROUP_MASK );
            }
            else
            {
                baseFabricPort = fabricPort;
            }

            /* ethernet ports */
            for ( i = 0 ; i < numLanes ; i++ )
            {
                status = fm10000MapFabricPortToSerdes( sw, 
                                                       baseFabricPort + i,
                                                       &serDes, 
                                                       &ring );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );


                laneExt = GET_LANE_EXT( sw, serDes );

                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port, 
                                 "port=%d baseFabricPort=%d "
                                 "serDes=%d laneExt=%p\n",
                                 port, 
                                 baseFabricPort, 
                                 serDes, 
                                 (void *)laneExt );

                FM_DLL_INSERT_LAST( portExt, 
                                    firstLane, 
                                    lastLane, 
                                    laneExt,
                                    nextLane, 
                                    prevLane );

                laneExt->lane          = i;
                laneExt->parentPortExt = portExt;
            }

        }   

    }
    else
    {
        newPepMode = ((fm10000_portSmEventInfo *)userInfo)->info.config.pepMode;
        oldPepMode = portAttrExt->pepMode;

        if ( oldPepMode != newPepMode )
        {
            /* first unlink all current lanes */
            status = UnlinkAllLanes( portExt );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            baseSerDes = portExt->nativeLaneExt->serDes;

            /* PCIE ports */
            FM_LOG_DEBUG_V2(FM_LOG_CAT_PORT,
                            port,
                            "Configuring port %d (PEP %d) for mode %d\n",
                            port, 
                            portExt->endpoint.pep,
                            newPepMode );
            switch ( newPepMode )
            {
                case FM_PORT_PEP_MODE_1X1:
                    /* 1x only on PEP #8 */
                    if ( portExt->endpoint.pep == 8 )
                    {
                        numLanes = 1;
                    }
                    else
                    {
                        status = FM_ERR_INVALID_PEP_MODE;
                        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                    }
                    break;

                case FM_PORT_PEP_MODE_1X8:
                    /* 8x only on even-numbered PEPs other than PEP #8 */
                    if ( ( portExt->endpoint.pep %  2 ) == 0  && 
                         ( portExt->endpoint.pep != 8 ) )
                    {
                        numLanes = 8;
                    }
                    else
                    {
                        status = FM_ERR_INVALID_PEP_MODE;
                        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                    }
                    break;

                case FM_PORT_PEP_MODE_2X4:
                    /* 4x on all PEPs other than PEP #8 */
                    if ( portExt->endpoint.pep != 8 )
                    {
                        numLanes = 4;
                    }
                    else
                    {
                        status = FM_ERR_INVALID_PEP_MODE;
                        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                    }
                    break;

                default:
                    status = FM_ERR_INVALID_PEP_MODE;
                    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                    break;

            }   /* end switch ( numLanes ) */

            for (i = 0 ; i < numLanes  ; i++)
            {
                                
                laneExt = GET_LANE_EXT( sw, baseSerDes + i );

                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Adding SerDes %d to Port %d (laneExt=%p)\n",
                                 baseSerDes + i,
                                 port,
                                 (void *)laneExt );

                FM_DLL_INSERT_LAST( portExt, 
                                    firstLane, 
                                    lastLane, 
                                    laneExt,
                                    nextLane, 
                                    prevLane );

                laneExt->lane          = i;
                laneExt->parentPortExt = portExt;
            }

        }   /* end if ( oldPepMode != newPepMode ) */

    }   /* end else (i.e. PCIE ports) */


ABORT:
    return status;

}   /* end fm10000LinkPortToLanes */




/*****************************************************************************/
/** fm10000UnlinkPortFromLanes
 * \ingroup intPort
 *
 * \desc            Action removing the link between a port and its associated
 *                  lanes when a port gets disabled
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000UnlinkPortFromLanes( fm_smEventInfo *eventInfo, void *userInfo )
{   
    fm10000_port *portExt;
    
    portExt = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    return UnlinkAllLanes( portExt ) ;

}   /* end fm10000UnlinkPortFromLanes */





/*****************************************************************************/
/** fm10000CheckLanesReady
 * \ingroup intPort
 *
 * \desc            Conditional transition from the POWERINGUP state, where
 *                  the condition is whether or not PLL lock has been reached
 *                  on all lanes
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a poniter to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000CheckLanesReady( fm_smEventInfo *eventInfo, 
                                  void           *userInfo, 
                                  fm_int         *nextState )
{
    fm_status     status = FM_OK;
    fm_int        port;
    fm_int        physLane;
    fm10000_port *portExt;

    port     = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    physLane = ((fm10000_portSmEventInfo *)userInfo)->info.physLane;
    portExt  = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    *nextState = FM10000_PORT_STATE_POWERING_UP;

    portExt->lockedLaneMask |= ( 1 << physLane );
    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT, port, 
                     "LOCKED=0x%08x DESIRED=0x%08x\n",
                     portExt->lockedLaneMask,
                     portExt->desiredLaneMask );
    if ( portExt->lockedLaneMask == portExt->desiredLaneMask )
    {
        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                         port,
                         "Port %d: all lanes gained lock\n",
                         port );
        status = fm10000Restart100gSyncDetection (eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000StartDeferralTimer( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        *nextState = FM10000_PORT_STATE_DEFERRED_UP;
    }
        
ABORT:
    return status;

}   /* end fm10000CheckLanesReady */




/*****************************************************************************/
/** fm10000ProcessDeferralTimer
 * \ingroup intPort
 *
 * \desc            Conditional transition from the DEFERREDUP state, where
 *                  the condition is the current admin mode
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a poniter to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ProcessDeferralTimer( fm_smEventInfo *eventInfo, 
                                       void           *userInfo, 
                                       fm_int         *nextState )
{
    fm_status         status;
    fm_switch        *switchPtr;
    fm_port          *portPtr;
    fm_portAttr      *portAttr;
    fm10000_port     *portExt;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_int            sw;
    fm_int            port;
    fm10000_portAttr *portAttrExt;
    fm_ethMode        ethMode;

    FM_NOT_USED( eventInfo );

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portPtr   = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portAttr  = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;

    sw        = switchPtr->switchNumber;
    port      = portPtr->portNumber;
    ethMode   = portExt->ethMode;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     port,
                     "Deferral timer expired on port %d (switch %d, portPtr=%p)\n",
                     port,
                     sw,
                     (void *)portExt->base );

    /* stop deferral timer inconditionally, ignore errors here */
    fm10000StopDeferralTimer(eventInfo,userInfo);

    /* all PLLs have gained lock, next steps depend on the admin mode */
    switch ( portPtr->mode )
    {
        case FM_PORT_MODE_UP:
            if (portAttrExt->fabricLoopback != FM_PORT_LOOPBACK_TX2RX)
            {
                status = fm10000SetupAdminModeUp( eventInfo, userInfo, nextState );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
            }
            else
            {
                /* fabricloopback is set */

                /* disable interrupts */
                status =  fm10000DisableLinkInterrupts(eventInfo,userInfo);

                /* set fabric loopback */
                if (status == FM_OK)
                {
                    status = SetFabricLoopbackFlags(sw,switchPtr,port,ethMode,TRUE); 
                }

                /* send port UP notification */
                if (status == FM_OK)
                {
                    status = fm10000NotifyApiPortUp(eventInfo,userInfo);
                }

                /* set next state */
                if (status == FM_OK)
                {
                    *nextState = FM10000_PORT_STATE_UP;
                }
            }

            break;
            
        case FM_PORT_MODE_REMOTE_FAULT:
        case FM_PORT_MODE_LOCAL_FAULT:
        case FM_PORT_MODE_ADMIN_DOWN:

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "Port %d: setting up admin fault %d\n",
                             port,
                             portPtr->mode );

            status = FM_OK;
            if (portAttrExt->fabricLoopback != FM_PORT_LOOPBACK_TX2RX)
            {
                *nextState = FM10000_PORT_STATE_ADMIN_FAULT;
                
            }
            else
            {
                /* fabricloopback is set */

                /* disable interrupts */
                status =  fm10000DisableLinkInterrupts(eventInfo,userInfo);

                /* set fabric loopback */
                if (status == FM_OK)
                {
                    status = SetFabricLoopbackFlags(sw,switchPtr,port,ethMode,TRUE); 
                }

                /* send port UP notification */
                if (status == FM_OK)
                {
                    status = fm10000NotifyApiPortUp(eventInfo,userInfo);
                }

                /* set next state */
                if (status == FM_OK)
                {
                    *nextState = FM10000_PORT_STATE_UP;
                }
            }
            
            break;

        case FM_PORT_MODE_BIST:
            if (portAttrExt->fabricLoopback != FM_PORT_LOOPBACK_TX2RX)
            {
                FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                                 port,
                                 "Port %d: enabling BIST submode %d\n",
                                 port,
                                 portPtr->submode );
       
                /* notify the serdes state machine that to enable bist */
                status = fm10000EnableBistMode( eventInfo, userInfo );
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

                *nextState = FM10000_PORT_STATE_BIST;
            }
            else
            {
                status = FM_ERR_INVALID_STATE;

                FM_LOG_ERROR_V2(FM_LOG_CAT_PORT, port, 
                                "Cannot configure BIST while fabric loopback is set\n");
                    
            }
            break;

        default:
            /* Is it true this should never happen? */
            status = FM_ERR_INVALID_STATE;
            *nextState = FM10000_PORT_STATE_DEFERRED_UP;
            break;

    }       /* end switch ( portPtr->mode ) */



ABORT:
    return status;

}   /* end fm10000ProcessDeferralTimer */




/*****************************************************************************/
/** fm10000LogPortStateTransition
 * \ingroup intPort
 *
 * \desc            Log callback for one or more registered port-level State
 *                  Transition tables
 * 
 * \param[in]       record is the pointer to a caller-allocated structure
 *                  containing the port state transition information to be
 *                  logged
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000LogPortStateTransition( fm_smTransitionRecord *record )
{
    fm_text currentState;
    fm_text event;

    if ( record->currentState != FM_STATE_UNSPECIFIED )
    {
        currentState = fm10000PortStatesMap[record->currentState];
    }
    else
    {
        currentState = "N/A";
    }
    if ( record->eventInfo.eventId != FM_EVENT_UNSPECIFIED )
    {
        event = fm10000PortEventsMap[record->eventInfo.eventId];
    }
    else
    {
        event = "N/A";
    }

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     record->smUserID,
                     "Port %d Transition: "
                     "'%s': '%s' ==> '%s'\n",
                     record->smUserID,
                     event,
                     currentState,
                     fm10000PortStatesMap[record->nextState] );

    return FM_OK;

}   /* end fm10000LogPortStateTransition */




/*****************************************************************************/
/** fm10000UpdatePcieModeAndSpeed
 * \ingroup intPort
 *
 * \desc            Action updating the port speed and PCIE  mode attributes 
 *                  for a PCIE link at the end of LTSSM negotiation
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000UpdatePcieModeAndSpeed( fm_smEventInfo *eventInfo, 
                                         void           *userInfo )
{
    fm_int             sw;
    fm_int             pep;
    fm_switch         *switchPtr;
    fm10000_port      *portExt;
    fm_portAttr       *portAttr;
    fm10000_portAttr  *portAttrExt;
    fm_status          status;
    fm_uint32          reg;
    fm_uint32          addr;
    fm_uint32          speed;
    fm_int             port;
    fm_uint32          regField;
    fm_pcieMode        mode;
    fm_timestamp       start;
    fm_timestamp       end;
    fm_timestamp       diff;
    fm_uint            delTime;
    fm_uint            loopCnt;


    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    port        = portExt->base->portNumber;

    if ( eventInfo->eventId == FM10000_PORT_EVENT_LINK_UP_IND ||
         eventInfo->eventId == FM10000_PORT_EVENT_CONFIG_REQ  )
    {
        sw        = switchPtr->switchNumber;
        pep       = portExt->endpoint.pep;
        addr      = FM10000_PCIE_CFG_PCIE_LINK_CTRL();

        /* Read the link status and control register */
        TAKE_REG_LOCK( sw );
        status = fm10000ReadPep( sw, addr, pep, &reg );
        DROP_REG_LOCK( sw );

        /* Transition from D3 -> D0  could result pep to return -1 */
        delTime = 0;
        loopCnt = 0;
        fmGetTime(&start);
        while (delTime < 500 && (reg == 0xFFFFFFFF))
        {
            loopCnt++;
            fmGetTime(&end);
            fmSubTimestamps(&end, &start, &diff);
            delTime = diff.sec*1000 + diff.usec/1000;
            TAKE_REG_LOCK( sw );
            status = fm10000ReadPep( sw, addr, pep, &reg );
            DROP_REG_LOCK( sw );
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SERDES, status);

            if (reg != 0xFFFFFFFF)
            {
                FM_LOG_WARNING(FM_LOG_CAT_PORT,
                               "Pep %d read PCIE_CTRL_LINK retry done in "
                               "%llu usec %u interations\n",
                                pep, diff.sec*1000000 + diff.usec, loopCnt );
                break;
            }
        }


        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        /* determine the lane speed */
        regField = FM_GET_FIELD( reg, 
                                 FM10000_PCIE_CFG_PCIE_LINK_CTRL, 
                                 CurrentLinkSpeed );
        switch ( regField )
        {
            case 1:
                speed = 2500;
                break;

            case 2:
                speed = 5000;
                break;

            case 3:
                speed = 8000;
                break;

            default:
                FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                                 port, 
                                 "Invalid CurrentLinkSpeed value %d on port %d. LINK_CTRL 0x%x\n",
                                 regField,
                                 port,
                                 reg );
                status = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                break;
        }


        /* determine the link width */
        regField = FM_GET_FIELD( reg, 
                                 FM10000_PCIE_CFG_PCIE_LINK_CTRL, 
                                 CurrentLinkWidth );
        switch ( regField )
        {
            case 1:
                mode = FM_PORT_PCIE_MODE_X1;
                break;

            case 2:
                mode = FM_PORT_PCIE_MODE_X2;
                speed *= 2;
                break;

            case 4:
                mode = FM_PORT_PCIE_MODE_X4;
                speed *= 4;
                break;

            case 8:
                mode = FM_PORT_PCIE_MODE_X8;
                speed *= 8;
                break;

            default:
                FM_LOG_ERROR_V2( FM_LOG_CAT_PORT, 
                                 port, 
                                 "Invalid CurrentLinkWidth value %d on port %d. LINK_CTRL 0x%x\n",
                                 regField,
                                 port,
                                 reg );
                status = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
                break;
        }

        FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT, 
                         port, 
                         "width=%d bandwidth=%d speed=%d\n",
                         FM_GET_FIELD( reg, FM10000_PCIE_CFG_PCIE_LINK_CTRL, CurrentLinkWidth ),
                         FM_GET_FIELD( reg, FM10000_PCIE_CFG_PCIE_LINK_CTRL, CurrentLinkSpeed ),
                         speed );

    }
    else
    {
        speed  = 0;
        mode   =  FM_PORT_PCIE_MODE_DISABLED;
        status = FM_OK;
    }


    portExt->speed        = speed;
    portAttrExt->pcieMode = mode;
    portAttr->speed       = speed;

ABORT:
    return status;

}   /* end fm10000UpdatePcieSpeed */




/*****************************************************************************/
/** fm10000UpdatePcieLanePolarity
 * \ingroup intPort
 *
 * \desc            Action updating the lane polarity for a PCIE link
 *                  at the end of LTSSM negotiation
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000UpdatePcieLanePolarity( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status              status;
    fm_laneAttr           *laneAttr;
    fm10000_lane          *laneExt;
    fm10000_port          *portExt;
    fm_int                 sw;
    fm_int                 serDes;
    fm10000SerdesPolarity  polarity;

    sw        = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    laneExt = FM_DLL_GET_FIRST( portExt, firstLane );
    while ( laneExt != NULL )
    {
        serDes   = laneExt->serDes;
        laneAttr = GET_LANE_ATTR(sw, serDes);

        /* For PCIE we'll always assume Tx Lane Polarity is never inverted
           but we'll issue a warning if we find otherwise later on. Also,
           we'll assume Polarity is never inverted if the link is down */

        laneAttr->txPolarity = FALSE;
        laneAttr->rxPolarity = FALSE;
        if ( eventInfo->eventId == FM10000_PORT_EVENT_LINK_UP_IND ||
             eventInfo->eventId == FM10000_PORT_EVENT_CONFIG_REQ  )
        {
            status = fm10000SerdesGetPolarity( sw, serDes, &polarity );
            if ( status != FM_OK )
            {
                FM_LOG_ERROR_V2( FM_LOG_CAT_PORT,
                                 portExt->base->portNumber,
                                 "Error '%s' reading polarity on "
                                 "PCIE port %d, serDes %d\n",
                                 fmErrorMsg( status ),
                                 portExt->base->portNumber,
                                 serDes );
            }
            else
            {
                switch ( polarity )
                {
                    case FM10000_SERDES_POLARITY_INVERT_RX:

                        laneAttr->rxPolarity = TRUE;
                        break;

                    case FM10000_SERDES_POLARITY_INVERT_TX_RX:

                        laneAttr->rxPolarity = TRUE;
                        /* intentional fall-through to the next case block */

                    case FM10000_SERDES_POLARITY_INVERT_TX:

                        FM_LOG_WARNING_V2( FM_LOG_CAT_PORT,
                                           portExt->base->portNumber,
                                           "Tx Polarity Inverted on "
                                           "PCIE port %d, serDes %d\n",
                                           portExt->base->portNumber,
                                           serDes );
                        break;
                
                    case FM10000_SERDES_POLARITY_NONE:
                    default:
                        break;
                }
            }
        }

        /* next lane */
        laneExt = FM_DLL_GET_NEXT( laneExt, nextLane );
    }

    /* return OK regardless (intentional) */
    return FM_OK;

}   /* end fm10000UpdatePcieLanePolarity */




/*****************************************************************************/
/** fm10000UpdatePcieLaneReversal
 * \ingroup intPort
 *
 * \desc            Action updating the lane ordering for a PCIE link
 *                  at the end of LTSSM negotiation
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000UpdatePcieLaneReversal( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_portAttr *portAttr;
    fm_switch   *switchPtr;
    fm_int       pep;
    fm_int       sw;
    fm_int       port;
    fm_uint32    reg;
    fm_uint32    addr;
    fm_status    status;

    portAttr  = ((fm10000_portSmEventInfo *)userInfo)->portAttr;

    /* assume direct ordering. For Tx that's always the case */
    portAttr->rxLaneOrdering = FM_PORT_LANE_ORDERING_NORMAL;
    portAttr->txLaneOrdering = FM_PORT_LANE_ORDERING_NORMAL;

    /* check if the link up and if yes to read the current lane reversal status
       from the PORT LOGIC register */
    status = FM_OK;
    if ( eventInfo->eventId == FM10000_PORT_EVENT_LINK_UP_IND ||
         eventInfo->eventId == FM10000_PORT_EVENT_CONFIG_REQ  )
    {
        switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
        pep  = ((fm10000_portSmEventInfo *)userInfo)->portExt->endpoint.pep; 
        port = ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber;
        sw  = switchPtr->switchNumber;

        /* get the PORT LOGIC register containing debug info related to
           the current lane reversal status */
        
        /* Forcing pepId=0 because the PEP offset is calculated later */
        addr = FM10000_GET_PL_ADDR(FM10000_PCIE_PL_REG_02C(), 0);

        status = fm10000ReadPep(sw, addr, pep, &reg);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        /* check if the receiver has detected lane reversal */
        if (FM_GET_BIT(reg, FM10000_PCIE_PL_REG_02C, fieldA))
        {
            /* yes */
            portAttr->rxLaneOrdering = FM_PORT_LANE_ORDERING_INVERT;
        }
    }

ABORT:
    return status;

}   /* end fm10000UpdatePcieLaneReversal */




/*****************************************************************************/
/** fm10000EnablePcieInterrupts
 * \ingroup intPort
 *
 * \desc            Action enabling all PCIE interrupts
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000EnablePcieInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int          sw;
    fm_int          pep;
    fm_switch    *  switchPtr;
    fm10000_switch *switchExt;
    fm10000_port *  portExt;
    fm_status       status;
    
    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    sw        = switchPtr->switchNumber;
    switchExt = GET_SWITCH_EXT(sw);
    pep       = portExt->endpoint.pep;
    
    TAKE_REG_LOCK( sw );

    FM_SET_UNNAMED_FIELD64(switchExt->interruptMaskValue,
                           FM10000_INTERRUPT_MASK_INT_b_PCIE_0 + pep,
                           1,
                           0);
    status = switchPtr->WriteUINT64(sw,
                                    switchExt->interruptMaskReg,
                                    switchExt->interruptMaskValue);
    
    DROP_REG_LOCK( sw );
    
    return status;

}   /* end fm10000EnablePcieInterrupts */




/*****************************************************************************/
/** fm10000DisablePcieInterrupts
 * \ingroup intPort
 *
 * \desc            Action disabling all PCIE interrupts
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DisablePcieInterrupts( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int          sw;
    fm_int          pep;
    fm_switch    *  switchPtr;
    fm10000_switch *switchExt;
    fm10000_port *  portExt;
    fm_status       status;
    
    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    sw        = switchPtr->switchNumber;
    switchExt = GET_SWITCH_EXT(sw);
    pep       = portExt->endpoint.pep;
    
    TAKE_REG_LOCK( sw );

    FM_SET_UNNAMED_FIELD64(switchExt->interruptMaskValue,
                           FM10000_INTERRUPT_MASK_INT_b_PCIE_0 + pep,
                           1,
                           1);
    status = switchPtr->WriteUINT64(sw,
                                    switchExt->interruptMaskReg,
                                    switchExt->interruptMaskValue);
    
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000DisablePcieInterrupts */




/*****************************************************************************/
/** fm10000AnStart
 * \ingroup intPort
 *
 * \desc            Action callback to start Auto-negotiation on a given port
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000AnStart( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm10000_port *portExt;
    fm_portAttr  *portAttr;

    portExt  = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr = ((fm10000_portSmEventInfo *)userInfo)->portAttr;

    if ( eventInfo->eventId != FM10000_PORT_EVENT_AN_CONFIG_REQ )
    {
        /* restore the AN config in the port event info */
        portExt->eventInfo.info.anConfig.autoNegMode = 
            portAttr->autoNegMode;
        portExt->eventInfo.info.anConfig.autoNegBasePage = 
            portAttr->autoNegBasePage;
        portExt->eventInfo.info.anConfig.autoNegNextPages = 
            portAttr->autoNegNextPages;
    }

    return SendAnEventReq( eventInfo, userInfo, FM10000_AN_EVENT_START_REQ );

}   /* end fm10000AnStart */




/*****************************************************************************/
/** fm10000AnStop
 * \ingroup intPort
 *
 * \desc            Action callback to stop Auto-negotiation on a given port
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 * 
 *****************************************************************************/
fm_status fm10000AnStop( fm_smEventInfo *eventInfo, void *userInfo )
{
    return SendAnEventReq( eventInfo, userInfo, FM10000_AN_EVENT_STOP_REQ );

}   /* end fm10000AnStop */




/*****************************************************************************/
/** fm10000ReconfigurePortForAn
 * \ingroup intPort
 *
 * \desc            Action callback to reconfigure a port to run
 *                  auto-negotiation after having entered a negotiated
 *                  ethernet mode. It applies to Clause 73 only
 * 
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ReconfigurePortForAn( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_status         status;
    fm_int            port;
    fm_portAttr      *portAttr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;

    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    port        = portExt->base->portNumber;

    /* the reconfiguration is needed only for Clause 73 */
    status = FM_OK;
    if ( portAttrExt->ethMode == FM_ETH_MODE_AN_73 )
    {
        /* restore AN73 PCS mode */
        portExt->eventInfo.info.config.ethMode = FM_ETH_MODE_AN_73;
        portExt->eventInfo.info.config.speed   = 0;

        status = fm10000ResetPortModuleState( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ReconfigureScheduler(eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000LinkPortToLanes( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteEplCfgA( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteEplCfgB( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteMac( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000InitAn73( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ConfigureLaneForAn73( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );


        /* AN73 was restored successfully */
        portExt->ethMode = FM_ETH_MODE_AN_73;
        portExt->speed   = 0;
        portAttr->speed  = 0;

    }

ABORT:
    return status;

}   /* end fm10000ReconfigurePortForAn */




/*****************************************************************************/
/** fm10000ConfigureDeviceAndCheckState
 * \ingroup intPort
 *
 * \desc            Conditional transition for PCIE endpoint that performs
 *                  the configuration and then transition to DOWN or UP
 *                  depending on the current device state
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a poniter to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ConfigureDeviceAndCheckState( fm_smEventInfo *eventInfo, 
                                               void           *userInfo, 
                                               fm_int         *nextState )
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_int     sw;
    fm_int     port;
    fm_int     pep;
    fm_uint32  addr;
    fm_uint32  reg;

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    pep     = ((fm10000_portSmEventInfo *)userInfo)->portExt->endpoint.pep;
    port    = ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber;
    sw      = switchPtr->switchNumber;

    /* perform the necessary actions, but the nextState is conditional */
    status = fm10000LinkPortToLanes( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    status = fm10000ConfigureLane( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* read the device power state */
    addr = FM10000_PCIE_FACTPS();
    TAKE_REG_LOCK( sw );
    status = fm10000ReadPep( sw, addr, pep, &reg );
    DROP_REG_LOCK( sw );

    FM_LOG_DEBUG_V2(FM_LOG_CAT_PORT, port, "PCIE_FACTPS=0x%08x\n", reg);

    /* is it D0a? */
    if ( (status == FM_OK) &&
         (FM_GET_FIELD( reg, FM10000_PCIE_FACTPS, Func0PowerState) == 2 ) )
    {
        /* yes, transition to UP */
        *nextState = FM10000_PORT_STATE_UP;

        status = fm10000InitPepMailbox( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000DisablePepLoopback( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000UpdatePcieModeAndSpeed( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000UpdatePcieLanePolarity( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000UpdatePcieLaneReversal( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000NotifyApiPortUp( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }
    else
    {
        /* no, go DOWN and wait for the DeviceStateChange interrupt */
        *nextState = FM10000_PORT_STATE_DOWN;
    }
    
ABORT:
    status = fm10000EnablePcieInterrupts( eventInfo, userInfo );

    return status;

}   /* end fm10000ConfigureDeviceAndCheckState */




/*****************************************************************************/
/** fm10000CheckPortStatus
 * \ingroup intPort
 *
 * \desc            Conditional transition callback executed when
 *                  auto-negotiation completes using Clause 37 and SGMII to
 *                  enter the negotiated mode
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a poniter to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000CheckPortStatus( fm_smEventInfo *eventInfo, 
                                  void           *userInfo, 
                                  fm_int         *nextState )
{
    fm_status     status;
    fm_switch    *switchPtr;
    fm_port      *portPtr;
    fm_portAttr  *portAttr;
    fm10000_port *portExt;
    fm_uint32     macCfg[FM10000_MAC_CFG_WIDTH];
    fm_int        sw;
    fm_int        port;

    FM_NOT_USED( eventInfo );

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portPtr   = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    portAttr  = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;

    sw        = switchPtr->switchNumber;
    port      = portPtr->portNumber;


    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     port,
                     "Deferral timer expired on port %d (switch %d, portPtr=%p)\n",
                     port,
                     sw,
                     (void *)portExt->base );

    /* all PLLs have gained lock, next steps depend on the admin mode */
    switch ( portPtr->mode )
    {
        case FM_PORT_MODE_UP:

            status = fm10000SetupAdminModeUp( eventInfo, userInfo, nextState );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            break;

        case FM_PORT_MODE_REMOTE_FAULT:
        case FM_PORT_MODE_LOCAL_FAULT:
        case FM_PORT_MODE_ADMIN_DOWN:

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "Port %d: setting up admin fault %d\n",
                             port,
                             portPtr->mode );

            status = fm10000AnStop( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            status = fm10000RestoreTxFaultMode( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            *nextState = FM10000_PORT_STATE_ADMIN_FAULT;
            break;

        case FM_PORT_MODE_BIST:

            FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                             port,
                             "Port %d: enabling BIST submode %d\n",
                             port,
                             portPtr->submode );

            status = fm10000AnStop( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            /* notify the serdes state machine that to enable bist */
            /* Do we need to disable link and AN interrupts?  */
            status = SerDesEventReq( eventInfo,
                                     userInfo,
                                     FM10000_SERDES_EVENT_ENABLE_BIST_REQ );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

            *nextState = FM10000_PORT_STATE_BIST;
            break;

        default:
            /* Is it true this should never happen? */
            status = FM_ERR_INVALID_STATE;
            break;

    }       /* end switch ( portPtr->mode ) */

ABORT:
    return status;

}   /* end fm10000CheckPortStatus */




/*****************************************************************************/
/** fm10000AnRestart
 * \ingroup intPort
 *
 * \desc            Conditional transition callback to be executed when
 *                  auto-negotiation is reconfigured by the application or
 *                  restarted by the AN state machine
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a poniter to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000AnRestart( fm_smEventInfo *eventInfo, 
                            void           *userInfo, 
                            fm_int         *nextState )
{
    fm_status         status;
    fm_port          *portPtr;
    fm_portAttr      *portAttr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_int            port;
    fm_int            eventId;

    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portPtr     = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    port        = portPtr->portNumber;
    eventId     = eventInfo->eventId;

    status = fm10000StopDeferredLpiTimer(eventInfo,userInfo);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* notify that the port is down */
    status = fm10000NotifyApiPortDown( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* Disable BIST (if already disabled the SerDes will ignore it */
    status = fm10000DisableBistMode( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* make user the fault mode is correct */
    status = fm10000SetTxFaultModeToNormal( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    /* processing needed to restart it, depends on the AN type */
    if ( portAttrExt->ethMode == FM_ETH_MODE_AN_73 )
    {
        /* this is effectively a reconfiguration from the current mode to
           AN-73. In the following make the action callbacks think that the 
           original event was a configuration request. An alternative
           could be to recursively notify a CONFIG_REQ event. After it returns
           this function would set the final next state as the next state
           of the 2nd-level event notification */
        portExt->eventInfo.info.config.ethMode = FM_ETH_MODE_AN_73;
        portExt->eventInfo.info.config.speed   = 0;
        eventInfo->eventId = FM10000_PORT_EVENT_CONFIG_REQ;

        /* restore AN73 PCS mode */

        status = fm10000DisablePhy( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000PowerDownLane( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ResetPortModuleState( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ReconfigureScheduler(eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000LinkPortToLanes( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteEplCfgA( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteEplCfgB( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteMac( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000InitAn73( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ConfigureLaneForAn73( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000EnablePhy( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000RestoreDfe( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ConfigureLoopback( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000PowerUpLane( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ClearEplFifo( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        /* AN73 was restored successfully */
        portExt->ethMode = FM_ETH_MODE_AN_73;
        portExt->speed   = 0;
        portAttr->speed  = 0;

        /* nowe we have to wait for the SerDes to be powered up */
        *nextState = FM10000_PORT_STATE_POWERING_UP;

    }
    else
    {
        if ( eventInfo->eventId == FM10000_PORT_EVENT_LOOPBACK_ON_REQ )
        {
            status = fm10000EnableLoopback( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        if ( eventInfo->eventId == FM10000_PORT_EVENT_LOOPBACK_OFF_REQ )
        {
            status = fm10000DisableLoopback( eventInfo, userInfo );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        /* start auto-negotiation */
        status = fm10000AnStart( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000StartAnWatchDogTimer(eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        /* auto-negotiation is in progress */
        *nextState = FM10000_PORT_STATE_AUTONEG;

    }

ABORT:
    /* restore the original eventID (it may have changed) */
    eventInfo->eventId = eventId;

    return status;

}   /* end fm10000AnRestart */




/*****************************************************************************/
/** fm10000EnterNegotiatedMode
 * \ingroup intPort
 *
 * \desc            Conditional transition callback executed when the AN
 *                  state machine notifies the successful completion of the
 *                  auto-negotiation and the port can enter its negotiatied
 *                  mode
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a poniter to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000EnterNegotiatedMode( fm_smEventInfo *eventInfo, 
                                      void           *userInfo, 
                                      fm_int         *nextState )
{
    fm_status         status;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_portAttr      *portAttr;
    fm_int            port;
    fm_ethMode        ethMode;
    fm_int            hcd;
    fm_uint32         speed;
    fm_int            eventId;
    fm_int            sw;
    fm_uint32         oldSpeed;

    sw          = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port        = portExt->base->portNumber;
    hcd         = ((fm10000_portSmEventInfo *)userInfo)->hcd;
    eventId     = eventInfo->eventId;

    /* different processing depending on auto-negotiation mode */
    if ( portAttrExt->ethMode == FM_ETH_MODE_AN_73 )
    {
        /* Clause 73 */
        ethMode = fm10000An73HcdToEthMode( hcd );
        speed   = fm10000GetPortSpeed( ethMode );

        if ( ethMode == FM_ETH_MODE_DISABLED )
        {
            status = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        /* Verify if EEE has been successfully negotiated if enabled */
        if (portAttrExt->eeeEnable)
        {
            status = fm10000AnVerifyEeeNegotiation( sw, port, ethMode );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        /* this is effectively a reconfiguration from AN-73 to the negotiated
           mode. In the following make the action callbacks think that the 
           original event was a configuration request. An alternative
           could be to recursively notify a CONFIG_REQ event. After it returns
           this function would set the final next state as the next state
           of the 2nd-level event notification */
        portExt->eventInfo.info.config.ethMode = ethMode;
        portExt->eventInfo.info.config.speed   = speed;
        eventInfo->eventId = FM10000_PORT_EVENT_CONFIG_REQ;

        status = fm10000StopAnWatchDogTimer(eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000DisablePhy( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000PowerDownLane( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ResetPortModuleState( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ReconfigureScheduler(eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000LinkPortToLanes( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteEplCfgA( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteEplCfgB( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000WriteMac( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000InitPcs( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ConfigureLane( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000EnablePhy( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000RestoreDfe( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ConfigureLoopback( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        portExt->eventInfo.info.admin.mode = FM_PORT_MODE_UP;
        portExt->eventInfo.info.admin.submode = 0;
        status = fm10000PowerUpLane( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000ClearEplFifo( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        /* The ethMode switch from AN73 to the negotiated mode was
           successful */
        portExt->ethMode = ethMode;
        oldSpeed         = portExt->speed;
        portExt->speed   = speed;
        portAttr->speed  = speed;

        if (oldSpeed != portExt->speed)
        {
            status = fm10000UpdateAllSAFValues(sw);
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        *nextState = FM10000_PORT_STATE_POWERING_UP;
    }
    else
    {
        /* Clause 37 or SGMII */
        status = fm10000CheckPortStatus( eventInfo, userInfo, nextState );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    }

ABORT:
    /* restore the original eventID (it may have changed) */
    eventInfo->eventId = eventId;

    return status;

}   /* end fm10000EnterNegotiatedMode */




/*****************************************************************************/
/** fm10000ProcessDeferralTimerWithAn
 * \ingroup intPort
 *
 * \desc            Conditional transition from the DEFERREDUP state, where
 *                  the condition is the current admin mode. This conditional
 *                  transition applies only to the port-level state machine to
 *                  support auto-negotiation
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \param[out]      nextState is a pointer to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000ProcessDeferralTimerWithAn( fm_smEventInfo *eventInfo, 
                                             void           *userInfo, 
                                             fm_int         *nextState )
{
    fm_status         status;
    fm10000_port     *portExt;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;
    fm_int            port;

    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    port        = portExt->base->portNumber;

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT_AUTONEG, 
                     port, 
                     "Configure EthMode=0x%08x - Current EthMode=0x%08x\n",
                     portAttrExt->ethMode,
                     portExt->ethMode );

    if ( portAttrExt->ethMode == FM_ETH_MODE_AN_73  && 
         portExt->ethMode     != FM_ETH_MODE_AN_73)
    {
        /* if we're running Clause 73 and we completed auto-negotiation, we've
           entered the negotiated mode and the PLLs are now ready */
        status = fm10000CheckPortStatus( eventInfo, userInfo, nextState );
    }
    else
    {
        /* in all other cases, if we got here we need to start 
           auto-negotiation */ 

        status = fm10000AnStart( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        status = fm10000StartAnWatchDogTimer(eventInfo,userInfo);
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

        *nextState = FM10000_PORT_STATE_AUTONEG;
    }

ABORT:
    return status;

}   /* end fm10000ProcessDeferralTimerWithAn */




/*****************************************************************************/
/** fm10000ConfigureLoopback
 * \ingroup intPort
 *
 * \desc            Configure near loopback on the lane(s) associated to a
 *                  port in the context of an event notified on that port's
 *                  state machine. Near loopback is a serial loopback 
 *                  implemented at serdes level.
 *                  Note well: fabric loopback takes precedence over serdes
 *                  (near and far) loopbacks: if fabric loopback is enabled
 *                  serdes loopback are forced to the disabled state.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ConfigureLoopback( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            eventId;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;

    portAttr = ( ( fm10000_portSmEventInfo *)userInfo )->portAttr;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;

    if ( portAttr->serdesLoopback    == FM_PORT_LOOPBACK_TX2RX &&
         portAttrExt->fabricLoopback != FM_PORT_LOOPBACK_TX2RX)
    {
        eventId = FM10000_SERDES_EVENT_LOOPBACK_ON_REQ;
    }
    else
    {
        eventId = FM10000_SERDES_EVENT_LOOPBACK_OFF_REQ;
    }

    return SerDesEventReq( eventInfo, userInfo, eventId );

}   /* end fm10000ConfigureLoopback */




/*****************************************************************************/
/** fm10000ConfigureFarLoopback
 * \ingroup intPort
 *
 * \desc            Configure far loopback on the lane(s) associated to a port
 *                  in the context of an event notified on that port's state
 *                  machine. Far loopback, also called repeater mode, is a
 *                  parallel loopback performed at serdes level where Tx clock
 *                  reference is taken from the Rx clock. This is different
 *                  than the fabric loopback, which is also a parallel loopback,
 *                  but performed at EPL level.
 *                  Note well: fabric loopback takes precedence over serdes
 *                  (near and far) loopbacks: if fabric loopback is enabled
 *                  serdes loopback are forced to the disabled state.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ConfigureFarLoopback( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            eventId;
    fm_ethMode        ethMode;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm_int            sw;
    fm_int            port;

    portAttr    = ((fm10000_portSmEventInfo *)userInfo)->portAttr;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw          = switchPtr->switchNumber;
    port        = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portExt     = GET_PORT_EXT(sw, port);

    ethMode     = (eventInfo->eventId == FM10000_PORT_EVENT_CONFIG_REQ) ?
                  ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode :
                  portExt->ethMode;

    if ( portAttr->serdesLoopback    == FM_PORT_LOOPBACK_RX2TX &&
         portAttrExt->fabricLoopback != FM_PORT_LOOPBACK_TX2RX && 
         ethMode != FM_ETH_MODE_SGMII                          &&
         ethMode != FM_ETH_MODE_AN_73 )
    {
        eventId = FM10000_SERDES_EVENT_FAR_LOOPBACK_ON_REQ;
    }
    else
    {
        eventId = FM10000_SERDES_EVENT_FAR_LOOPBACK_OFF_REQ;
    }

    return SerDesEventReq( eventInfo, userInfo, eventId );

}   /* end fm10000ConfigureFarLoopback */




/*****************************************************************************/
/** fm10000SetupAdminModeUp
 * \ingroup intPort
 *
 * \desc            Conditional transition that checks the port status and
 *                  transitions to the Up, Local Fault or Remote Fault states 
 *                  when the admin mode is up
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[out]      nextState is a pointer to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000SetupAdminModeUp ( fm_smEventInfo *eventInfo, 
                                    void           *userInfo,
                                    fm_int         *nextState )
{
    fm_status     status;
    fm_uint32     addr;
    fm_uint32     portStatus;
    fm_int        sw;
    fm_int        port;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_int        linkFault;

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port      = portExt->base->portNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;


    /* read the port status register */
    addr = FM10000_PORT_STATUS( epl, physLane );

    status = switchPtr->ReadUINT32( sw, addr, &portStatus );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     port,
                     "Port %d: PORT_STATUS=0x%08x\n",
                     port,
                     portStatus );

    /* set the next state depending on the link fault value */
    linkFault = FM_GET_FIELD( portStatus, 
                              FM10000_PORT_STATUS, 
                              LinkFaultDebounced );
    if ( linkFault == 0 )
    {
        /* no fault */
        *nextState = FM10000_PORT_STATE_UP;
        status     = fm10000NotifyApiPortUp( eventInfo, userInfo );
    }
    else if ( linkFault == 2 )
    {
        /* remote fault */
        *nextState = FM10000_PORT_STATE_REMOTE_FAULT;
    }
    else
    {
        /* assume local fault for anything else */
        *nextState = FM10000_PORT_STATE_LOCAL_FAULT;
    }

ABORT:
    return status;

}   /* end fm10000SetupAdminModeUp */




/*****************************************************************************/
/** fm10000ExitBistMode
 * \ingroup intPort
 *
 * \desc            Conditional transition callback that disable BIST and
 *                  transitions to the state indicated by the port status
 *                  register
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[out]      nextState is a pointer to caller-allocated storage where
 *                  this function will place the next state.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ExitBistMode ( fm_smEventInfo *eventInfo, 
                                void           *userInfo,
                                fm_int         *nextState )
{
    fm_status status;
    fm_int    port;

    port = ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber;

    status = fm10000DisableBistMode( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    status = fm10000SetupAdminModeUp( eventInfo, userInfo, nextState );

ABORT:
    return status;

}   /* end fm10000ExitBistMode */




/*****************************************************************************/
/** fm10000ExitAdminFaultMode
 * \ingroup intPort
 *
 * \desc            Conditional transition callback that restores the normal
 *                  fault mode and transitions to the state indicated by the
 *                  port status register
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[out]      nextState is a pointer to caller-allocated storage where
 *                  this function will place the next state.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ExitAdminFaultMode ( fm_smEventInfo *eventInfo, 
                                      void           *userInfo,
                                      fm_int         *nextState )
{
    fm_status status;
    fm_int    port;

    port = ((fm10000_portSmEventInfo *)userInfo)->portExt->base->portNumber;

    status = fm10000SetTxFaultModeToNormal( eventInfo, userInfo );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    status = fm10000SetupAdminModeUp( eventInfo, userInfo, nextState );

ABORT:
    return status;

}   /* end fm10000ExitAdminFaultMode */




/*****************************************************************************/
/** fm10000EnableLowPowerIdle
 * \ingroup intPort
 *
 * \desc            Action enabling the EEE Low Power Idle mode on a given port
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 ****************************************************************************/
fm_status fm10000EnableLowPowerIdle( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int            sw;
    fm_int            port;
    fm_int            epl;
    fm_int            physLane;
    fm_switch        *switchPtr;
    fm10000_port     *portExt;
    fm10000_portAttr *portAttrExt;
    fm_uint32         macCfg[FM10000_MAC_CFG_WIDTH];
    fm_uint           addr;
    fm_status         status;

    /* the generic event info is unused in this function */
    FM_NOT_USED( eventInfo );

    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port        = ((fm10000_portSmEventInfo *)userInfo)->portPtr->portNumber;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;
    sw          = switchPtr->switchNumber;
    epl         = portExt->endpoint.epl;
    physLane    = portExt->nativeLaneExt->physLane;


    TAKE_REG_LOCK(sw);

    addr = FM10000_MAC_CFG(epl, physLane, 0);

    status = switchPtr->ReadUINT32Mult(sw,
                                       addr,
                                       FM10000_MAC_CFG_WIDTH,
                                       macCfg);
    FM_LOG_ABORT_ON_ERR_V2(FM_LOG_CAT_PORT, port, status);

    if ( !(portAttrExt->dbgEeeMode & FM10000_EEE_DBG_DISABLE_TX) )
    {
        /* Workaround for EEE */
        /* in 1000Base-X, to prevent the first packet to be dropped when
           exiting the LPI mode, we must set the DrainMode to HOLD_NORMAL  */
        if ( (portExt->ethMode == FM_ETH_MODE_1000BASE_KX) ||
             (portExt->ethMode == FM_ETH_MODE_1000BASE_X) )
        {
            FM_ARRAY_SET_FIELD( macCfg,
                                FM10000_MAC_CFG,
                                TxDrainMode,
                                FM10000_PORT_TX_HOLD_ON_LINK_DOWN );

            status = switchPtr->WriteUINT32Mult( sw,
                                                 addr,
                                                 FM10000_MAC_CFG_WIDTH,
                                                 macCfg );
            FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
        }

        /* Request transmit of Low Power Idle generation, i.e. send /LI/ codes */
        FM_ARRAY_SET_BIT( macCfg, FM10000_MAC_CFG, TxLpIdleRequest, TRUE );

        status = switchPtr->WriteUINT32Mult( sw,
                                             addr,
                                             FM10000_MAC_CFG_WIDTH,
                                             macCfg );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }

ABORT:
    DROP_REG_LOCK( sw );

    return status;

}   /* end fm10000EnableLowPowerIdle */

/*****************************************************************************/
/** fm10000ProcessDisableFabricLoopback
 * \ingroup intPort
 *
 * \desc            Conditional transition that disables the fabric loopback
 *                  and transitions to the Up, Local Fault or Remote Fault
 *                  states.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \param[out]      nextState is a pointer to caller-allocated storage where
 *                  this function will place the next state.
 * 
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000ProcessDisableFabricLoopback ( fm_smEventInfo *eventInfo, 
                                                void           *userInfo,
                                                fm_int         *nextState )
{
    fm_status     status;
    fm_uint32     addr;
    fm_uint32     portStatus;
    fm_int        sw;
    fm_int        port;
    fm_int        epl;
    fm_int        physLane;
    fm_switch    *switchPtr;
    fm10000_port *portExt;
    fm_int        linkFault;

    switchPtr = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw        = switchPtr->switchNumber;
    portExt   = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    port      = portExt->base->portNumber;
    epl       = portExt->endpoint.epl;
    physLane  = portExt->nativeLaneExt->physLane;

    /* read the port status register */
    addr = FM10000_PORT_STATUS( epl, physLane );

    status = switchPtr->ReadUINT32( sw, addr, &portStatus );
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    FM_LOG_DEBUG_V2( FM_LOG_CAT_PORT,
                     port,
                     "Port %d: PORT_STATUS=0x%08x\n",
                     port,
                     portStatus );


    /* set the next state depending on the link fault value */
    linkFault = FM_GET_FIELD( portStatus, 
                              FM10000_PORT_STATUS, 
                              LinkFaultDebounced );
    if ( linkFault == 0 )
    {
        /* no fault */
        *nextState = FM10000_PORT_STATE_UP;
    }
    else if ( linkFault == 2 )
    {
        /* remote fault */
        *nextState = FM10000_PORT_STATE_REMOTE_FAULT;
    }
    else
    {
        /* assume local fault for anything else */
        *nextState = FM10000_PORT_STATE_LOCAL_FAULT;
    }


    if (*nextState != FM10000_PORT_STATE_UP)
    {
        status = fm10000NotifyApiPortDown( eventInfo, userInfo );
        FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );
    }
    
    status = fm10000EnableDrainMode( eventInfo, userInfo);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    status = fm10000DisableFabricLoopback(eventInfo,userInfo);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    status = fm10000DisableDrainMode( eventInfo, userInfo);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

    status = fm10000EnableLinkInterrupts(eventInfo,userInfo);
    FM_LOG_ABORT_ON_ERR_V2( FM_LOG_CAT_PORT, port, status );

ABORT:
    return status;

}   /* end fm10000ProcessDisableFabricLoopback */

/*****************************************************************************/
/** fm10000InitPepMailbox
 * \ingroup intPort
 *
 * \desc            Action that initializes the mailbox for a PEP that just
 *                  came out of reset
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000InitPepMailbox( fm_smEventInfo *eventInfo,
                                 void           *userInfo )
{
    fm_int          sw;
    fm_int          pep;
    fm_switch      *switchPtr;
    fm10000_port   *portExt;
    fm_status       status;
    fm_uint32       rv;
    fm_uint32       regAddr;
    fm_mailboxInfo *info;

    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    sw          = switchPtr->switchNumber;
    pep         = portExt->endpoint.pep;

    /* Enable interrupts in PCIE_IM/PCIE_GMBX registers */
    regAddr = FM10000_PCIE_IM();

    status = fm10000SetMailboxGlobalInterrupts(sw, pep, TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    status = fm10000ReadPep( sw, regAddr, pep, &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    FM_SET_BIT( rv, FM10000_PCIE_IP, Mailbox, 0);

    status = fm10000WritePep(sw, regAddr, pep, rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

    info = &switchPtr->mailboxInfo;
    info->numberOfVirtualPortsAddedToUcastFlood[pep] = 0;
    info->numberOfVirtualPortsAddedToMcastFlood[pep] = 0;
    info->numberOfVirtualPortsAddedToBcastFlood[pep] = 0;

    status = fm10000ResetPepMailboxVersion(sw, pep);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MAILBOX, status);

ABORT:
    return status;

}   /* end fm10000InitPepMailbox */


/*****************************************************************************/
/** fm10000EnablePepLoopback
 * \ingroup intPort
 *
 * \desc            Action that enables fabric loopback for a given PEP
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000EnablePepLoopback( fm_smEventInfo *eventInfo,
                                    void           *userInfo )
{
    return SetPepLoopback( eventInfo, userInfo, TRUE );

}   /* end fm10000EnablePepLoopback */


/*****************************************************************************/
/** fm10000DisablePepLoopback
 * \ingroup intPort
 *
 * \desc            Action that disables fabric loopback for a given PEP
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 *
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 *
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000DisablePepLoopback( fm_smEventInfo *eventInfo,
                                     void           *userInfo )
{
    return SetPepLoopback( eventInfo, userInfo, FALSE );

}   /* end fm10000DisablePepLoopback */


/*****************************************************************************/
/** fm10000NotifyEthModeChange
 * \ingroup intPort
 *
 * \desc            Function that notifies the platform layer of an Ethernet
 *                  mode change, so that it can perform certain platform-
 *                  related operations (i.e transceiver management etc.)
 *                  PHY
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 *****************************************************************************/
fm_status fm10000NotifyEthModeChange( fm_smEventInfo *eventInfo, void *userInfo )
{
    fm_int      sw;
    fm_int      port;
    fm_port    *portPtr;
    fm_status   status = FM_OK;
    fm_ethMode  ethMode;

    /* the generic event info is unsed in this function */
    FM_NOT_USED( eventInfo );

    sw       = ((fm10000_portSmEventInfo *)userInfo)->switchPtr->switchNumber;
    portPtr  = ((fm10000_portSmEventInfo *)userInfo)->portPtr;
    ethMode  = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;
    port     = portPtr->portNumber;

    if ( portPtr->phyInfo.notifyEthModeChange )
    {
        status = portPtr->phyInfo.notifyEthModeChange( sw, 
                                                       port,
                                                       FM_PORT_ACTIVE_MAC,
                                                       ethMode );
    }

    return status;

}   /* end fm10000NotifyEthModeChange */



/*****************************************************************************/
/** fm10000Restart100gSyncDetection
 * \ingroup intPort
 *
 * \desc            Function that restart the synchronization process for
 *                  100G ports. The ethernet mode is verified before performing
 *                  any action, so it may be called inconditionally.
 *
 * \param[in]       eventInfo pointer to a caller-allocated area containing
 *                  the generic event descriptor (unsed in this function)
 * 
 * \param[in]       userInfo pointer to a caller-allocated area containing the
 *                  purpose specific event descriptor (cast to
 *                  ''fm10000_portSmEventInfo'' in this function)
 * 
 * \return          FM_OK if successful
 * \return          Other ''Status Codes'' as appropriate in case of failure.
 *
 *****************************************************************************/
fm_status fm10000Restart100gSyncDetection( fm_smEventInfo *eventInfo,
                                           void           *userInfo )
{
    fm_int              sw;
    fm_ethMode          ethMode;
    fm_switch          *switchPtr;
    fm10000_port       *portExt;
    fm10000_portAttr   *portAttrExt;
    fm_status           status;
    fm_int              epl;
    fm_uint64           regValue;
    fm_uint32           regAddr;


    switchPtr   = ((fm10000_portSmEventInfo *)userInfo)->switchPtr;
    sw          = switchPtr->switchNumber;
    portExt     = ((fm10000_portSmEventInfo *)userInfo)->portExt;
    portAttrExt = ((fm10000_portSmEventInfo *)userInfo)->portAttrExt;

    status = FM_OK;

    /* get the ethernet mode */
    ethMode = ((fm10000_portSmEventInfo *)userInfo)->info.config.ethMode;

    /* only for 100G eth modes */
    if (  portAttrExt->ethMode == FM_ETH_MODE_100GBASE_SR4  ||
        ( portAttrExt->ethMode == FM_ETH_MODE_AN_73         &&
         (portExt->ethMode == FM_ETH_MODE_100GBASE_KR4      ||
          portExt->ethMode == FM_ETH_MODE_100GBASE_CR4     )))
    {
        epl     = ((fm10000_portSmEventInfo *)userInfo)->portExt->endpoint.epl;
        regAddr = FM10000_RS_FEC_CFG(epl,0);

        TAKE_REG_LOCK( sw );

        status = switchPtr->ReadUINT64( sw, regAddr, &regValue);

        if ( status == FM_OK )
        {
            FM_SET_BIT64(regValue, FM10000_RS_FEC_CFG, RestartLock, 1);

            switchPtr->WriteUINT64( sw, regAddr, regValue);

            FM_SET_BIT64(regValue, FM10000_RS_FEC_CFG, RestartLock, 0);

            switchPtr->WriteUINT64( sw, regAddr, regValue);
        }

        DROP_REG_LOCK(sw);
    }

    return status;

}   /* end fm10000Restart100gSyncDetection */



