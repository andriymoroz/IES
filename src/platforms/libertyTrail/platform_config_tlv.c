/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            platform_config_tlv.c
 * Creation Date:   May 21, 2015
 * Description:     Functions to load tlv configuration.
 *
 * Copyright (c) 2015, Intel Corporation
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
#include <platforms/util/fm_util_config_tlv.h>


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Re-defined to have shorter name */
#define BPS_1G      FM_PLAT_SERDES_BITRATE_1G
#define BPS_10G     FM_PLAT_SERDES_BITRATE_10G
#define BPS_25G     FM_PLAT_SERDES_BITRATE_25G

#define LANE_ALL    -1

#define TLV_ETHMODE_AUTODETECT 15

/* Pair of switch number and port configuration on the switch */
typedef struct
{
    /* Switch number */
    fm_int swIdx;

    /* Pointer to the port configuration structure */
    fm_platformCfgPort *portCfg;

} fm_portCfgSwitch;


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
/* CopyTlvStr
 * \ingroup intPlatform
 *
 * \desc            Copy TLV bytes into string buffer.
 *
 * \param[in]       dest is the pointer to the string buffer.
 *
 * \param[in]       destSize is the size of the dest buffer.
 *
 * \param[in]       src is the pointer to the TLV buffer.
 *
 * \param[in]       srcSize is the size of the TLV buffer.
 *
 * \return          Integer equivalent of the TLV bytes. 
 *
 *****************************************************************************/
static void CopyTlvStr(fm_text dest, fm_int destSize, fm_byte *src, fm_int srcSize)
{
    fm_int len;

    /* Reserve one for null terminated character */
    len = destSize - 1;
    if (len > srcSize)
    {
        len = srcSize;
    }

    FM_MEMCPY_S(dest, destSize, src, len);
    dest[len] = '\0';

}   /* end CopyTlvStr */




/*****************************************************************************/
/* GetTlvInt
 * \ingroup intPlatform
 *
 * \desc            Get up to 32-bit integer from TLV bytes.
 *                  Note: All encoding less than 32-bit will be assumed
 *                  as unsigned integer.
 *
 * \param[in]       tlv is an array of bytes.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \return          Integer equivalent of the TLV bytes. 
 *
 *****************************************************************************/
static fm_int GetTlvInt(fm_byte *tlv, fm_int tlvLen)
{
    fm_int j;
    fm_int value;

    if (tlvLen > 4)
    {
        tlvLen = 4;
    }

    value = tlv[0];
    for (j = 1; j < tlvLen; j++)
    {
        value <<= 8;
        value  |= (tlv[j] & 0xFF);
    }

    return value;

}   /* end GetTlvInt */




/*****************************************************************************/
/* GetTlvUint64
 * \ingroup intPlatform
 *
 * \desc            Get up to unsigned 64-bit integer from TLV bytes.
 *
 * \param[in]       tlv is an array of bytes.
 *
 * \param[in]       tlvLen is the size of the TLV value.
 *
 * \return          Unsigned integer equivalent of the TLV bytes. 
 *
 *****************************************************************************/
static fm_uint64 GetTlvUint64(fm_byte *tlv, fm_int tlvLen)
{
    fm_int j;
    fm_uint64 value;

    if (tlvLen > 8)
    {
        tlvLen = 8;
    }

    value = tlv[0];
    for (j = 1; j < tlvLen; j++)
    {
        value <<= 8;
        value  |= (tlv[j] & 0xFFUL);
    }

    return value;

}   /* end GetTlvUint64 */




/*****************************************************************************/
/* GetTlvBool
 * \ingroup intPlatform
 *
 * \desc            Get boolean from TLV byte.
 *
 * \param[in]       tlv is the pointer to a byte.
 *
 * \return          Integer equivalent of the TLV bytes. 
 *
 *****************************************************************************/
static fm_bool GetTlvBool(fm_byte *tlv)
{

    return  ((*tlv) ? TRUE : FALSE);

}   /* end GetTlvBool */






/*****************************************************************************/
/* CheckHwResourceId
 * \ingroup intPlatform
 *
 * \desc            Check if hwResourceId is valid.
 *
 * \param[in]       sw is the switch index on which to operate.
 *
 * \param[in]       portCfg is the pointer to the port config.
 *                  This pointer is inserted to hwResourceIdList if the HW ID
 *                  is used for the first time.
 *
 * \param[in,out]   hwResourceIdList points to an array of ''fm_platformCfgPort''
 *                  structures. The array must be large enough to hold the
 *                  maximum number of possible HW Resource ID entries
 *                  (''FM_NUM_HW_RES_ID'').
 *
 * \param[in]       defaultId is the default hwResourceId value.
 *
 * \return          FM_OK if ID is valid.
 * \return          FM_ERR_UNINITIALIZED if NULL pointer is passed as an argument
 * \return          FM_ERR_INVALID_ARGUMENT if ID is not valid.
 * \return          FM_ERR_ALREADY_EXISTS if ID already exists and cannot be used
 *                  again.
 *
 *****************************************************************************/
static fm_status CheckHwResourceId(fm_int              sw,
                                   fm_platformCfgPort *portCfg,
                                   fm_portCfgSwitch   *hwResourceIdList,
                                   fm_uint32           defaultId)
{
    fm_uint             hwIdx;
    fm_platformCfgPort *oldPortCfg;

    if ( (portCfg          == NULL) ||
         (hwResourceIdList == NULL) )
    {
        return FM_ERR_UNINITIALIZED;
    }

    hwIdx      = HW_RESOURCE_ID_TO_IDX(portCfg->hwResourceId);
    oldPortCfg = NULL;

    /* Try to insert the non-default HW ID
     * to the array and check if it is unique */
    if (portCfg->hwResourceId != defaultId)
    {
        if (hwIdx >= FM_NUM_HW_RES_ID)
        {
            return FM_ERR_INVALID_ARGUMENT;
        }

        if (hwResourceIdList[hwIdx].portCfg == NULL)
        {
            hwResourceIdList[hwIdx].swIdx = sw;
            hwResourceIdList[hwIdx].portCfg = portCfg;
        }
        else
        {
            oldPortCfg = hwResourceIdList[hwIdx].portCfg;

            /* The ID already exists, if ports are not part
             * of the same QSFP interface, then return error */
            if ( ( sw != hwResourceIdList[hwIdx].swIdx ) ||
                 ( portCfg->epl != oldPortCfg->epl     ) ||
                 ( portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE0 &&
                   portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
                   portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
                   portCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3 ) ||
                   ( oldPortCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE0 &&
                     oldPortCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE1 &&
                     oldPortCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE2 &&
                     oldPortCfg->intfType != FM_PLAT_INTF_TYPE_QSFP_LANE3 ) )
            {
                FM_LOG_WARNING(FM_LOG_CAT_PLATFORM,
                               "Ports %d (sw=%d, EPL=%d) and %d (sw=%d, EPL="
                               "%d) have the same hwResId - they are on %s "
                               "(ports must be a part of the same QFSP "
                               "interface, otherwise the hwResId should be "
                               "unique).\n",
                               oldPortCfg->port,
                               hwResourceIdList[hwIdx].swIdx,
                               oldPortCfg->epl,
                               portCfg->port,
                               sw,
                               portCfg->epl,
                               ( (sw != hwResourceIdList[hwIdx].swIdx ) ||
                                 (portCfg->epl != oldPortCfg->epl ) ?
                                 "different EPLs" :
                                 "the same EPL, but the interfaceType of at "
                                 "least one of them is not set to QSFP_LANE" ));

                return FM_ERR_ALREADY_EXISTS;
            }
        }
    }

    return FM_OK;

}   /* end CheckHwResourceId */





/*****************************************************************************/
/* GetTlvEthMode
 * \ingroup intPlatform
 *
 * \desc            Get ethernet mode from TLV byte.
 *
 * \param[in]       tlv is a byte encoding of the ethernet mode.
 *
 * \return          The equivalent ethernet mode. 
 *
 *****************************************************************************/
static fm_ethMode GetTlvEthMode(fm_byte tlv)
{

    switch (tlv)
    {
        default:
        case 0:
            return FM_ETH_MODE_DISABLED;
        case 1:
            return FM_ETH_MODE_1000BASE_KX;
        case 2:
            return FM_ETH_MODE_1000BASE_X;
        case 3:
            return FM_ETH_MODE_2500BASE_X;
        case 4:
            return FM_ETH_MODE_6GBASE_CR;
        case 5:
            return FM_ETH_MODE_10GBASE_KX4;
        case 6:
            return FM_ETH_MODE_10GBASE_CX4;
        case 7:
            return FM_ETH_MODE_10GBASE_CR;
        case 8:
            return FM_ETH_MODE_10GBASE_SR;
        case 9:
            return FM_ETH_MODE_24GBASE_CR4;
        case 10:
            return FM_ETH_MODE_25GBASE_SR;
        case 11:
            return FM_ETH_MODE_40GBASE_SR4;
        case 12:
            return FM_ETH_MODE_100GBASE_SR4;
        case 13:
            return FM_ETH_MODE_AN_73;
        case 14:
            return FM_ETH_MODE_SGMII;
        case TLV_ETHMODE_AUTODETECT:
            return FM_ETH_MODE_DISABLED;
    }

}   /* end GetTlvEthMode */




/*****************************************************************************/
/* SetPortDefTxEq
 * \ingroup intPlatform
 *
 * \desc            Set default TX equalization for all ports on a switch.
 *
 * \param[in]       swIdx is the index to the given switch.
 *
 * \param[in]       tlvType is the TX equalization type.
 *
 * \param[in]       value is the TX equalization value to set.
 *
 * \return          NONE 
 *
 *****************************************************************************/

void SetPortDefTxEq(fm_int swIdx,
                    fm_uint tlvType,
                    fm_byte value)
{
    fm_int                portIdx;
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgLane *  laneCfg;
    fm_platSerdesBitRate  bps;
    fm_int                epl;
    fm_int                eplLane;

    /* For all ports on the switch */
    swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
    for (portIdx = 0 ; portIdx < swCfg->numPorts ; portIdx++)
    {
        for (epl = 0 ; epl < FM_PLAT_NUM_EPL ; epl++)
        {
            for (eplLane = 0 ; eplLane < FM_PLAT_LANES_PER_EPL ; eplLane++)
            {
                for (bps = FM_PLAT_SERDES_BITRATE_1G;
                     bps < FM_PLAT_SERDES_BITRATE_MAX;
                     bps++)
                {
                    laneCfg = &swCfg->epls[epl].lane[eplLane];
                    switch (tlvType)
                    {
                        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_CU:
                            laneCfg->copper[bps].preCursor = value;
                            break;
                        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_CU:
                            laneCfg->copper[bps].cursor = value;
                            break;
                        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_CU:
                            laneCfg->copper[bps].postCursor = value;
                            break;
                        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_OPT:
                            laneCfg->optical[bps].preCursor = value;
                            break;
                        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_OPT:
                            laneCfg->optical[bps].cursor = value;
                            break;
                        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_OPT:
                            laneCfg->optical[bps].postCursor = value;
                            break;
                    }
                }
            }
        }
    }

}   /* end SetPortDefTxEq */




/*****************************************************************************/
/* SetPortTxEq
 * \ingroup intPlatform
 *
 * \desc            Set default TX equalization for a port on a switch.
 *
 * \param[in]       swIdx is the index to the given switch.
 *
 * \param[in]       portIdx is the index to the given port.
 *
 * \param[in]       lane is the lane number of a given port.
 *
 * \param[in]       eqType is the TX equalization type.
 *
 * \param[in]       value is the TX equalization value to set.
 *
 * \return          NONE 
 *
 *****************************************************************************/
void SetPortTxEq(fm_int swIdx,
                 fm_int portIdx,
                 fm_int lane,
                 fm_uint eqType,
                 fm_byte value)
{
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPort *  portCfg;
    fm_platformCfgLane *  laneCfg;
    fm_int                epl;
    fm_int                eplLane;
    fm_int                startLane;
    fm_int                endLane;



    portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
    if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "Port %d (PortIdx %d) is not EPL type.\n", portCfg->port, portIdx);
        return;
    }

    epl = portCfg->epl;
    if (epl >= FM_PLAT_NUM_EPL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "epl %d is out of range\n",
            epl);
        return;
    }

    if (lane == LANE_ALL)
    {
        startLane = 0;
        endLane = (portCfg->intfType == FM_PLAT_INTF_TYPE_QSFP_LANE0) ? 4 : 1;
    }
    else
    {
        startLane = lane;
        endLane = lane + 1;
    }
    for (lane = startLane ; lane < endLane ; lane++)
    {
        eplLane = portCfg->lane[lane];
        swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
        laneCfg = &swCfg->epls[epl].lane[eplLane];
        switch (eqType)
        {
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_1G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_1G:
                laneCfg->copper[BPS_1G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_10G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_10G:
                laneCfg->copper[BPS_10G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_25G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_25G:
                laneCfg->copper[BPS_25G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_1G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_1G:
                laneCfg->optical[BPS_1G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_10G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_10G:
                laneCfg->optical[BPS_10G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_25G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_25G:
                laneCfg->optical[BPS_25G].preCursor = value;
                break;

            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_1G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_1G:
                laneCfg->copper[BPS_1G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_10G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_10G:
                laneCfg->copper[BPS_10G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_25G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_25G:
                laneCfg->copper[BPS_25G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_1G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_1G:
                laneCfg->optical[BPS_1G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_10G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_10G:
                laneCfg->optical[BPS_10G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_25G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_25G:
                laneCfg->optical[BPS_25G].preCursor = value;
                break;

            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_1G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_1G:
                laneCfg->copper[BPS_1G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_10G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_10G:
                laneCfg->copper[BPS_10G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_25G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_25G:
                laneCfg->copper[BPS_25G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_1G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_1G:
                laneCfg->optical[BPS_1G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_10G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_10G:
                laneCfg->optical[BPS_10G].preCursor = value;
                break;
            case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_25G:
            case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_25G:
                laneCfg->optical[BPS_25G].preCursor = value;
                break;
            default:
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Invalid EQ TLV type 0x%x\n", eqType);

                break;
        }
    }

}   /* end SetPortTxEq */




/*****************************************************************************/
/* fmPlatformLoadLTCfgTlv
 * \ingroup intPlatform
 *
 * \desc            Load platform LT config TLV configuration.
 *
 * \param[in]       tlv is an array of encoded TLV bytes
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLoadLTCfgTlv(fm_byte *tlv)
{
    fm_status             status;
    fm_int                swIdx;
    fm_platformCfg *      platCfg;
    fm_platformCfgLib *   libCfg;
    fm_platformCfgPort *  portCfg;
    fm_platformCfgLane *  laneCfg;
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPhy *   phyCfg;
    fm_gn2412LaneCfg *    phyLaneCfg;
    fm_int                portIdx;
    fm_int                epl;
    fm_int                eplLane;
    fm_int                lane;
    fm_int                phyIdx;
    fm_uint64             categoryMask;
    fm_uint64             valU64;
    fm_int                off;
#ifdef FM_SUPPORT_SWAG
    fm_platformCfgPort *  portCfg2;
    fm_int                portIdx2;
#endif
    /* hardcode values */
    fm_int                crossConnect[FM_GN2412_NUM_LANES] = { 6,7,8,9,10,11,
                                                                0,1,2,3,4,5 };
    fm_uint               tlvType;
    fm_uint               tlvLen;

    /* Global configuration */
    platCfg = FM_PLAT_GET_CFG;


    tlvType = (tlv[0] << 8) | tlv[1];
    tlvLen = tlv[2];

    switch (tlvType)
    {
        case FM_TLV_PLAT_DEBUG:
            platCfg->debug = GetTlvInt(tlv + 3, 4);
            categoryMask   = 0;

            if (platCfg->debug & CFG_DBG_ENABLE_PLAT_LOG)
            {
                FM_LOG_PRINT("Enabling platform debug logging\n");
                categoryMask |= FM_LOG_CAT_PLATFORM;
            }

            if (platCfg->debug & CFG_DBG_ENABLE_PORT_LOG)
            {
                FM_LOG_PRINT("Enabling port debug logging\n");
                categoryMask |= FM_LOG_CAT_PORT;
            }

            if (platCfg->debug & CFG_DBG_ENABLE_INTR_LOG)
            {
                FM_LOG_PRINT("Enabling interrupt debug logging\n");
                categoryMask |= FM_LOG_CAT_EVENT_INTR;
            }

            if (categoryMask != 0)
            {
                fmSetLoggingFilter(categoryMask, FM_LOG_LEVEL_ALL, NULL, NULL);
            }

            break;

        case FM_TLV_PLAT_NUM_SW:
            platCfg->numSwitches = GetTlvInt(tlv + 3, 1);
            /* Allocate the switch configuration structures */
            fmRootPlatform->cfg.switches =
                fmAlloc( platCfg->numSwitches * sizeof(fm_platformCfgSwitch) );

            if (fmRootPlatform->cfg.switches == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
            }

            for (swIdx = 0; swIdx < platCfg->numSwitches; swIdx++)
            {
                swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
                /* Initialize some of the EPL fields */
                for ( epl = 0 ; epl < FM_PLAT_NUM_EPL ; epl++ )
                {
                    FM_MEMSET_S( swCfg->epls[epl].laneToPortIdx,
                                 sizeof(swCfg->epls[epl].laneToPortIdx),
                                 FM_PLAT_UNDEFINED,
                                 sizeof(swCfg->epls[epl].laneToPortIdx) );
                }

                swCfg->intrPollPeriodMsec = FM_AAD_API_PLATFORM_INT_POLL_MSEC;
                swCfg->xcvrPollPeriodMsec = FM_AAD_API_PLATFORM_XCVR_POLL_MSEC;
                swCfg->msiEnabled         = FM_AAD_API_PLATFORM_MSI_ENABLED;
                swCfg->fhClock            = FM_AAD_API_PLATFORM_FH_CLOCK;
                swCfg->i2cClkDivider      = FM_AAD_API_LIB_I2C_CLKDIVIDER;
                FM_STRNCPY_S(swCfg->devMemOffset,
                     FM_PLAT_MAX_CFG_STR_LEN,
                     FM_AAD_API_PLATFORM_DEVMEM_OFFSET,
                     FM_PLAT_MAX_CFG_STR_LEN);
                FM_STRNCPY_S(swCfg->netDevName,
                     FM_PLAT_MAX_CFG_STR_LEN,
                     FM_AAD_API_PLATFORM_NETDEV_NAME,
                     FM_PLAT_MAX_CFG_STR_LEN);
                FM_STRNCPY_S(swCfg->uioDevName,
                     FM_PLAT_MAX_CFG_STR_LEN,
                     FM_AAD_API_PLATFORM_UIO_DEV_NAME,
                     FM_PLAT_MAX_CFG_STR_LEN);
                swCfg->gpioPortIntr = FM_PLAT_UNDEFINED;
                swCfg->gpioI2cReset = FM_PLAT_UNDEFINED;
                swCfg->gpioFlashWP  = FM_PLAT_UNDEFINED;
                swCfg->schedListSelect = FM_AAD_API_PLATFORM_SCHED_LIST_SELECT;
                swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDS] = FM_PLAT_UNDEFINED;
                swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDF] = FM_PLAT_UNDEFINED;
                swCfg->vrm.hwResourceId[FM_PLAT_VRM_AVDD] = FM_PLAT_UNDEFINED;
            }
            break;

        case FM_TLV_PLAT_NAME:
            CopyTlvStr(platCfg->name, FM_PLAT_MAX_CFG_STR_LEN, tlv + 3, tlvLen);
            break;
        case FM_TLV_PLAT_FILE_LOCK_NAME:
            CopyTlvStr(platCfg->fileLockName, FM_PLAT_MAX_CFG_STR_LEN, tlv + 3, tlvLen);
            break;
        case FM_TLV_PLAT_SW_NUMBER:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->swNum = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_SW_NUM_PORTS:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->numPorts = GetTlvInt(tlv + 4, 1);


            /* Allocate the port configuration structures */
            fmRootPlatform->cfg.switches[swIdx].ports =
                fmAlloc( swCfg->numPorts * sizeof(fm_platformCfgPort) );

            if (fmRootPlatform->cfg.switches[swIdx].ports == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
            }

            for (portIdx = 0 ; portIdx < swCfg->numPorts ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);

                portCfg->portIdx = portIdx;
                portCfg->portType = FM_PLAT_PORT_TYPE_NONE;
                portCfg->epl = FM_PLAT_UNDEFINED;
                portCfg->pep = FM_PLAT_UNDEFINED;
                portCfg->tunnel = FM_PLAT_UNDEFINED;
                portCfg->loopback = FM_PLAT_UNDEFINED;
                portCfg->phyNum = FM_PLAT_UNDEFINED;
                portCfg->phyPort = FM_PLAT_UNDEFINED;
                portCfg->hwResourceId = FM_DEFAULT_HW_RES_ID;

                /* The physical port is the same as portIndex */
                portCfg->physPort = portIdx;

                for (lane = 0 ; lane < FM_PLAT_LANES_PER_EPL ; lane++)
                {
                    /* Default the lane ordering to one-to-one between
                       the QSFP lanes and the EPL lanes. */
                    portCfg->lane[lane] = lane;
                }

                for (epl = 0 ; epl < FM_PLAT_NUM_EPL ; epl ++)
                {
                    for (eplLane = 0 ; eplLane < FM_PLAT_LANES_PER_EPL; eplLane++)
                    {
                        laneCfg = &swCfg->epls[epl].lane[eplLane];
                        laneCfg->rxTermination = FM_PLAT_UNDEFINED;
                    }
                }

                portCfg->intfType = FM_PLAT_INTF_TYPE_NONE;
                portCfg->dfeMode = FM_DFE_MODE_ONE_SHOT;
                portCfg->an73Ability = FM_PLAT_AN73_DEFAULT_ABILITIES; 
            }

            break;
        case FM_TLV_PLAT_SW_LED_POLL_PER:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->ledPollPeriodMsec = GetTlvInt(tlv + 4, 2);
            break;
        case FM_TLV_PLAT_SW_LED_BLINK_MODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->ledBlinkMode = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_SW_XCVR_POLL_PER:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->xcvrPollPeriodMsec = GetTlvInt(tlv + 4, 2);
            break;
        case FM_TLV_PLAT_INTR_POLL_PER:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->intrPollPeriodMsec = GetTlvInt(tlv + 4, 2);
            break;
        case FM_TLV_PLAT_SW_PORT_MAPPING:
        case FM_TLV_PLAT_SW_PORT_LANE_MAPPING:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            if (tlvType == FM_TLV_PLAT_SW_PORT_LANE_MAPPING)
            {
                lane = GetTlvInt(tlv + 5, 1);
                if (lane >= FM_PLAT_LANES_PER_EPL)
                {
                     FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Lane %d is out of range\n", lane);
                    return FM_ERR_INVALID_PORT;
                }
                off = 1;
            }
            else
            {
                lane = 0;
                off = 0;
            }

            portCfg->port = GetTlvInt(tlv + 6 + off, 2);
            if (portCfg->port > swCfg->maxLogicalPortValue)
            {
                swCfg->maxLogicalPortValue = portCfg->port;
            }
            switch (tlv[5 + off] & 0xF)
            {
                case 0:
                    portCfg->portType = FM_PLAT_PORT_TYPE_PCIE;
                    portCfg->pep = GetTlvInt(tlv + 8 + off, 1);
                break;
                case 1:
                    portCfg->portType = FM_PLAT_PORT_TYPE_EPL;
                    portCfg->epl = GetTlvInt(tlv + 8 + off, 1);
                    portCfg->lane[lane] = (GetTlvInt(tlv + 5 + off, 1) >> 4) & 0xF;
                break;
                case 2:
                    portCfg->portType = FM_PLAT_PORT_TYPE_LOOPBACK;
                    portCfg->loopback = GetTlvInt(tlv + 8 + off, 1);
                break;
                case 3:
                    portCfg->portType = FM_PLAT_PORT_TYPE_TUNNEL;
                    portCfg->tunnel = GetTlvInt(tlv + 8 + off, 1);
                break;
                case 4:
                    portCfg->portType = FM_PLAT_PORT_TYPE_FIBM;
                break;
                case 5:
                    portCfg->portType = FM_PLAT_PORT_TYPE_EPL;
#ifdef FM_SUPPORT_SWAG
                    portCfg->swagLink.swId =  GetTlvInt(tlv + 8 + off, 1);
                    portCfg->swagLink.swPort = GetTlvInt(tlv + 9 + off, 2);

                    /* Internal port will be processed after */
                    portCfg->swagLink.logicalPort = portCfg->port;
                    portCfg->swagLink.partnerLogicalPort = -1;
                    portCfg->swagLink.partnerSwitch = -1;
                    portCfg->swagLink.partnerPort = -1;

                    if (portCfg->swagLink.swPort == 0)
                    {
                        portCfg->swagLink.type = FM_SWAG_LINK_CPU;
                    }
                    else
                    {
                        portCfg->swagLink.type = FM_SWAG_LINK_EXTERNAL;
                    }
#endif
                break;
                case 6:
                    portCfg->portType = FM_PLAT_PORT_TYPE_EPL;
                    portCfg->epl = GetTlvInt(tlv + 8 + off, 1);
                    portCfg->lane[lane] = (GetTlvInt(tlv + 5 + off, 1) >> 4) & 0xF;
                    portCfg->phyNum = GetTlvInt(tlv + 9 + off, 1);
                    portCfg->phyPort = GetTlvInt(tlv + 10 + off, 1);
                break;
                default:
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "Unknown TLV portType %d\n",
                        tlv[5 + off] & 0xF);
                break;
            }
            /**************************************************
             * Get port speed for scheduler BW allocation
             **************************************************/

            switch (portCfg->portType)
            {
                case FM_PLAT_PORT_TYPE_EPL:
                    /* Defaulting to 2500, but should always be equal
                     * or higher than default ethernet mode */
                    portCfg->speed = 2500;
                    break;

                case FM_PLAT_PORT_TYPE_PCIE:
                    /* Default to 50000, should be set to a maximum of 25G on
                     * a x4 PEP and 10G on a x1 PEP. */
                    portCfg->speed = 50000;
                    break;
                case FM_PLAT_PORT_TYPE_TUNNEL:
                    portCfg->speed = 100000;
                    break;
                case FM_PLAT_PORT_TYPE_LOOPBACK:
                    portCfg->speed = 25000;
                    break;
                case FM_PLAT_PORT_TYPE_FIBM:
                    portCfg->speed = 2500;
                    break;
                default:
                    portCfg->speed = 0;
            }

            break;
#ifdef FM_SUPPORT_SWAG
        case FM_TLV_PLAT_SW_INT_PORT_MAPPING:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            /* Not used field */
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }

            portIdx = GetTlvInt(tlv + 5, 2);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);

            portIdx2 = GetTlvInt(tlv + 7, 2);
            if (portIdx2 >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg2 = FM_PLAT_GET_PORT_CFG(swIdx, portIdx2);

            portCfg->swagLink.type = FM_SWAG_LINK_INTERNAL;
            portCfg->swagLink.partnerLogicalPort = portIdx2;
            portCfg->swagLink.partnerSwitch = portCfg2->swagLink.swId;
            portCfg->swagLink.partnerPort = portCfg2->swagLink.swPort;

            portCfg2->swagLink.type = FM_SWAG_LINK_INTERNAL;
            portCfg2->swagLink.partnerLogicalPort = portIdx;
            portCfg2->swagLink.partnerSwitch = portCfg->swagLink.swId;
            portCfg2->swagLink.partnerPort = portCfg->swagLink.swPort;
            break;
        case FM_TLV_PLAT_SW_TOPOLOGY:
            platCfg->topology = GetTlvInt(tlv + 3, 1);
            break;
        case FM_TLV_PLAT_SW_ROLE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->switchRole = GetTlvInt(tlv + 4, 1);;
            break;
#endif
        case FM_TLV_PLAT_SW_PORTIDX_SPEED:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->speed = GetTlvInt(tlv + 5, 4);
            break;

        case FM_TLV_PLAT_SW_PORT_DEF_HW_ID:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                portCfg->hwResourceId = GetTlvInt(tlv + 4, 4);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_HW_ID:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->hwResourceId = GetTlvInt(tlv + 5, 4);
            break;
        case FM_TLV_PLAT_SW_PORT_DEF_ETHMODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                portCfg->ethMode = GetTlvEthMode(tlv[4]);
                portCfg->autodetect = (tlv[4] == TLV_ETHMODE_AUTODETECT);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_ETHMODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->ethMode = GetTlvEthMode(tlv[5]);
            portCfg->autodetect = (tlv[5] == TLV_ETHMODE_AUTODETECT);
            break;
        case FM_TLV_PLAT_SW_PORT_DEF_LANE_POL:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
                {
                    continue;
                }
                for (eplLane = 0 ; eplLane < FM_PLAT_LANES_PER_EPL; eplLane++)
                {
                    epl = portCfg->epl;
                    if (epl >= FM_PLAT_NUM_EPL)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                            "epl %d is out of range\n",
                            epl);
                        return FM_ERR_INVALID_PORT;
                    }
                    laneCfg = &swCfg->epls[epl].lane[eplLane];
                    laneCfg->lanePolarity = GetTlvInt(tlv + 4, 1);
                }
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_LANE_ALL_POL:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
            {
                return FM_ERR_INVALID_PORT;
            }
            for (eplLane = 0 ; eplLane < FM_PLAT_LANES_PER_EPL; eplLane++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                epl = portCfg->epl;
                if (epl >= FM_PLAT_NUM_EPL)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "epl %d is out of range\n",
                        epl);
                    return FM_ERR_INVALID_PORT;
                }
                laneCfg = &swCfg->epls[epl].lane[eplLane];
                laneCfg->lanePolarity = GetTlvInt(tlv + 5, 1);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_LANE_POL:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Port %d is not EPL type. PortIdx %d\n", portCfg->port, portIdx);
                return FM_ERR_INVALID_PORT;
            }
            epl = portCfg->epl;
            if (epl >= FM_PLAT_NUM_EPL)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "epl %d is out of range\n",
                    epl);
                return FM_ERR_INVALID_PORT;
            }
            lane = GetTlvInt(tlv + 5, 1);
            if (lane >= FM_PLAT_LANES_PER_EPL)
            {
                 FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Lane %d is out of range\n", lane);
                return FM_ERR_INVALID_PORT;
            }
            eplLane = portCfg->lane[lane];
            laneCfg = &swCfg->epls[epl].lane[eplLane];
            laneCfg->lanePolarity = GetTlvInt(tlv + 6, 1);
            break;
        case FM_TLV_PLAT_SW_PORTIDX_LANE_ALL_RX_TERM:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
            {
                return FM_ERR_INVALID_PORT;
            }
            for (eplLane = 0 ; eplLane < FM_PLAT_LANES_PER_EPL; eplLane++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                epl = portCfg->epl;
                if (epl >= FM_PLAT_NUM_EPL)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                        "epl %d is out of range\n",
                        epl);
                    return FM_ERR_INVALID_PORT;
                }
                laneCfg = &swCfg->epls[epl].lane[eplLane];
                laneCfg->rxTermination = GetTlvInt(tlv + 5, 1);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_LANE_RX_TERM:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            if (portCfg->portType != FM_PLAT_PORT_TYPE_EPL)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Port %d is not EPL type. PortIdx %d\n", portCfg->port, portIdx);
                return FM_ERR_INVALID_PORT;
            }
            epl = portCfg->epl;
            if (epl >= FM_PLAT_NUM_EPL)
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "epl %d is out of range\n",
                    epl);
                return FM_ERR_INVALID_PORT;
            }
            lane = GetTlvInt(tlv + 5, 1);
            if (lane >= FM_PLAT_LANES_PER_EPL)
            {
                 FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Lane %d is out of range\n", lane);
                return FM_ERR_INVALID_PORT;
            }
            eplLane = portCfg->lane[lane];
            laneCfg = &swCfg->epls[epl].lane[eplLane];
            laneCfg->rxTermination = GetTlvInt(tlv + 6, 1);
            break;
        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_CU:
        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_CU:
        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_CU:
        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_PRE_OPT:
        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_CUR_OPT:
        case FM_TLV_PLAT_SW_PORT_DEF_TXEQ_POST_OPT:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            SetPortDefTxEq(swIdx, tlvType, GetTlvInt(tlv + 4, 1));
            break;
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_1G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_10G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_CU_25G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_1G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_10G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_PRE_OPT_25G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_1G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_10G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_CU_25G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_1G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_10G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_CUR_OPT_25G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_1G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_10G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_CU_25G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_1G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_10G:
        case FM_TLV_PLAT_SW_PORTIDX_TXEQ_POST_OPT_25G:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            SetPortTxEq(swIdx, portIdx, LANE_ALL,
                tlvType, GetTlvInt(tlv + 5, 1));
            break;
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_1G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_10G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_CU_25G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_1G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_10G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_PRE_OPT_25G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_1G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_10G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_CU_25G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_1G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_10G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_CUR_OPT_25G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_1G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_10G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_CU_25G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_1G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_10G:
        case FM_TLV_PLAT_SW_PORTIDX_LANE_TXEQ_POST_OPT_25G:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            lane = GetTlvInt(tlv + 5, 1);
            if (lane >= FM_PLAT_LANES_PER_EPL)
            {
                 FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Lane %d is out of range\n", lane);
                return FM_ERR_INVALID_PORT;
            }
            SetPortTxEq(swIdx, portIdx, lane,
                tlvType, GetTlvInt(tlv + 6, 1));
            break;
        case FM_TLV_PLAT_SW_PORT_DEF_INTF_TYPE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                portCfg->intfType = GetTlvInt(tlv + 4, 1);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_INTF_TYPE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->intfType = GetTlvInt(tlv + 5, 1);
            switch (portCfg->intfType)
            {
                case FM_PLAT_INTF_TYPE_QSFP_LANE0:
                case FM_PLAT_INTF_TYPE_QSFP_LANE1:
                case FM_PLAT_INTF_TYPE_QSFP_LANE2:
                case FM_PLAT_INTF_TYPE_QSFP_LANE3:
                    lane = portCfg->intfType - FM_PLAT_INTF_TYPE_QSFP_LANE0;

                    swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
                    swCfg->epls[portCfg->epl].laneToPortIdx[lane] = portCfg->portIdx;
                    break;
                default:
                    break;
            }
            break;
        case FM_TLV_PLAT_SW_PORT_DEF_CAP:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                portCfg->cap = GetTlvInt(tlv + 4, 8);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_CAP:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->cap = GetTlvInt(tlv + 5, 8);
            break;
        case FM_TLV_PLAT_SW_PORT_DEF_DFE_MODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            for (portIdx = 0 ; portIdx < FM_PLAT_NUM_PORT(swIdx) ; portIdx++)
            {
                portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
                portCfg->dfeMode = GetTlvInt(tlv + 4, 1);
            }
            break;
        case FM_TLV_PLAT_SW_PORTIDX_DFE_MODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->dfeMode = GetTlvInt(tlv + 5, 1);
            break;
        case FM_TLV_PLAT_SW_PORTIDX_AN73_ABILITY:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            portIdx = GetTlvInt(tlv + 4, 1);
            if (portIdx >= FM_PLAT_NUM_PORT(swIdx))
            {
                return FM_ERR_INVALID_PORT;
            }
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);
            portCfg->an73Ability = GetTlvInt(tlv + 5, 4);
            break;
        case FM_TLV_PLAT_SW_SHARED_LIB_NAME:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            libCfg = FM_PLAT_GET_LIBS_CFG(swIdx);
            CopyTlvStr(libCfg->libName, FM_PLAT_MAX_CFG_STR_LEN, tlv + 4, tlvLen - 1);
            break;
        case FM_TLV_PLAT_SW_SHARED_LIB_DISABLE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            libCfg = FM_PLAT_GET_LIBS_CFG(swIdx);
            libCfg->disableFuncIntf = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_SW_PORT_INTR_GPIO:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->gpioPortIntr = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_SW_I2C_RESET_GPIO:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->gpioI2cReset = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_SW_FLASH_WP_GPIO:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->gpioFlashWP = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_SW_KEEP_SERDES_CFG:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->keepSerdesCfg = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_SW_NUM_PHYS:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->numPhys = GetTlvInt(tlv + 4, 1);
            /* Allocate the PHY configuration structures */
            swCfg->phys = fmAlloc( swCfg->numPhys * sizeof(fm_platformCfgPhy) );

            if (swCfg->phys == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_NO_MEM);
            }
            break;
        case FM_TLV_PLAT_SW_PHY_MODEL:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            phyIdx = GetTlvInt(tlv + 4, 1);
            if (phyIdx >= FM_PLAT_NUM_PHY(swIdx))
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyCfg = FM_PLAT_GET_PHY_CFG(swIdx, phyIdx);
            phyCfg->model = GetTlvInt(tlv + 5, 1);
            if (phyCfg->model == FM_PLAT_PHY_GN2412)
            {
                for (lane = 0 ; lane < FM_GN2412_NUM_LANES ; lane++)
                {
                    phyLaneCfg = &phyCfg->gn2412Lane[lane];
                    phyLaneCfg->preTap = FM_GN2412_DEF_LANE_PRE_TAP;
                    phyLaneCfg->attenuation = FM_GN2412_DEF_LANE_ATT;
                    phyLaneCfg->postTap = GetTlvInt(tlv + 8, 1);
                    phyLaneCfg->polarity = FM_GN2412_DEF_LANE_POST_TAP;
                    phyLaneCfg->appMode = 0x74;
                    phyLaneCfg->rxPort = crossConnect[lane];
                }
            }
            break;
        case FM_TLV_PLAT_SW_PHY_ADDR:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            phyIdx = GetTlvInt(tlv + 4, 1);
            if (phyIdx >= FM_PLAT_NUM_PHY(swIdx))
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyCfg = FM_PLAT_GET_PHY_CFG(swIdx, phyIdx);
            phyCfg->addr = GetTlvInt(tlv + 5, 2);
            break;
        case FM_TLV_PLAT_SW_PHY_HW_ID:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            phyIdx = GetTlvInt(tlv + 4, 1);
            if (phyIdx >= FM_PLAT_NUM_PHY(swIdx))
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyCfg = FM_PLAT_GET_PHY_CFG(swIdx, phyIdx);
            phyCfg->hwResourceId = GetTlvInt(tlv + 5, 4);
            break;
        case FM_TLV_PLAT_SW_PHY_LANE_TXEQ:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            phyIdx = GetTlvInt(tlv + 4, 1);
            if (phyIdx >= FM_PLAT_NUM_PHY(swIdx))
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyCfg = FM_PLAT_GET_PHY_CFG(swIdx, phyIdx);
            lane = GetTlvInt(tlv + 5, 1);
            if ( phyCfg->model == FM_PLAT_PHY_GN2412  &&
                 lane < FM_GN2412_NUM_LANES )
            {
                phyLaneCfg = &phyCfg->gn2412Lane[lane];
                phyLaneCfg->preTap = GetTlvInt(tlv + 6, 1);
                phyLaneCfg->attenuation = GetTlvInt(tlv + 7, 1);
                phyLaneCfg->postTap = GetTlvInt(tlv + 8, 1);
                phyLaneCfg->polarity = GetTlvInt(tlv + 9, 1);
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Unknown PHY model %d\n", phyCfg->model);
                return FM_ERR_INVALID_ARGUMENT;
            }
            break;
        case FM_TLV_PLAT_SW_PHY_APP_MODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            phyIdx = GetTlvInt(tlv + 4, 1);
            if (phyIdx >= FM_PLAT_NUM_PHY(swIdx))
            {
                return FM_ERR_INVALID_ARGUMENT;
            }
            phyCfg = FM_PLAT_GET_PHY_CFG(swIdx, phyIdx);
            lane = GetTlvInt(tlv + 5, 1);
            if ( phyCfg->model == FM_PLAT_PHY_GN2412  &&
                 lane < FM_GN2412_NUM_LANES )
            {
                phyLaneCfg = &phyCfg->gn2412Lane[lane];
                phyLaneCfg->appMode = GetTlvInt(tlv + 6, 1);
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                    "Unknown PHY model %d\n", phyCfg->model);
                return FM_ERR_INVALID_ARGUMENT;
            }
            break;
        case FM_TLV_PLAT_SW_MSI_ENABLE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->msiEnabled = GetTlvBool(tlv + 4);
            break;
        case FM_TLV_PLAT_SW_FH_CLOCK:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->fhClock = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_DEV_MEM_OFF:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            valU64 = GetTlvUint64(tlv + 4, 8);
            FM_SNPRINTF_S(swCfg->devMemOffset,
                         FM_PLAT_MAX_CFG_STR_LEN,
                         "0x%llx",
                         valU64);
            break;
        case FM_TLV_PLAT_NET_DEVNAME:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            CopyTlvStr(swCfg->netDevName,
                       FM_PLAT_MAX_CFG_STR_LEN,
                       tlv + 4,
                       tlvLen - 1);
            break;
        case FM_TLV_PLAT_UIO_DEVNAME:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            CopyTlvStr(swCfg->uioDevName,
                       FM_PLAT_MAX_CFG_STR_LEN,
                       tlv + 4,
                       tlvLen - 1);
            break;
        case FM_TLV_PLAT_INTR_TIMEOUT_CNT:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->intrTimeoutCnt = GetTlvInt(tlv + 4, 2);
        case FM_TLV_PLAT_BOOT_MODE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->bootMode = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_REG_ACCESS:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->regAccess = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_PCIE_ISR:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->pcieISR = GetTlvInt(tlv + 4, 1);
            break;
        case FM_TLV_PLAT_CPU_PORT:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->cpuPort = GetTlvInt(tlv + 4 , 2);
            break;
        case FM_TLV_PLAT_PHY_EN_DEEMPHASIS:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->enablePhyDeEmphasis = GetTlvBool(tlv + 4);
            break;
        case FM_TLV_PLAT_SW_VDDS_USE_HW_RESOURCE_ID:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDS]  = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_SW_VDDF_USE_HW_RESOURCE_ID:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->vrm.hwResourceId[FM_PLAT_VRM_VDDF]  = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_SW_AVDD_USE_HW_RESOURCE_ID:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->vrm.hwResourceId[FM_PLAT_VRM_AVDD]  = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_SW_VRM_USE_DEF_VOLTAGE:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->vrm.useDefVoltages = GetTlvInt(tlv + 4, 4);
            break;
        case FM_TLV_PLAT_SW_I2C_CLKDIVIDER:
            swIdx = GetTlvInt(tlv + 3, 1);
            if (swIdx >= platCfg->numSwitches)
            {
                return FM_ERR_INVALID_SWITCH;
            }
            swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);
            swCfg->i2cClkDivider = GetTlvInt(tlv + 4, 1);
            break;
        default:
            status = FM_ERR_INVALID_ARGUMENT;
            break;
    }

    return FM_OK;

}   /* end fmPlatformLoadLTCfgTlv */



/*****************************************************************************/
/* fmPlatformLoadLibCfgTlv
 * \ingroup intPlatform
 *
 * \desc            Load shared lib config TLV configuration.
 *
 * \param[in]       tlv is an array of encoded TLV bytes
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLoadLibCfgTlv(fm_byte *tlv)
{
    fm_platformCfgLib *libCfg;
    fm_uint   tlvLen;
    fm_int    swIdx;
    fm_uint   newTlvBufSize;
    fm_byte  *newTlvBuf;


    if (!fmRootPlatform->cfg.switches)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
            "TLV [0x%02x%02x] must be after numSwitches config\n",
            tlv[0], tlv[1]);
        return FM_FAIL;
    }

    tlvLen = tlv[2] + 3;

    swIdx = 0; /*Assume 0 for now */
    libCfg = FM_PLAT_GET_LIBS_CFG(swIdx);

    if (tlvLen > (libCfg->tlvCfgBufSize - libCfg->tlvCfgLen))
    {
        newTlvBufSize = libCfg->tlvCfgBufSize + 1024;
        newTlvBuf = fmAlloc(newTlvBufSize);
        if (newTlvBuf == NULL)
        {
            return FM_ERR_NO_MEM;
        }
        if (libCfg->tlvCfgBuf)
        {
            FM_MEMCPY_S(newTlvBuf, newTlvBufSize, 
                libCfg->tlvCfgBuf, libCfg->tlvCfgLen);
            fmFree(libCfg->tlvCfgBuf);
        }
        libCfg->tlvCfgBuf = newTlvBuf;
        libCfg->tlvCfgBufSize = newTlvBufSize;
    }

    FM_MEMCPY_S(libCfg->tlvCfgBuf + libCfg->tlvCfgLen,
                libCfg->tlvCfgBufSize - libCfg->tlvCfgLen, 
                tlv, tlvLen);
    libCfg->tlvCfgLen += tlvLen;

    return FM_OK;

}   /* end fmPlatformLoadLibCfgTlv  */




/*****************************************************************************/
/* fmPlatformLoadTlv
 * \ingroup intPlatform
 *
 * \desc            Load TLV property.
 *
 * \param[in]       tlv is an array of encoded TLV bytes
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmPlatformLoadTlv(fm_byte *tlv)
{
    fm_status       status;
    fm_uint         tlvType;
    fm_platformCfg *platCfg;
    fm_char         propTxt[256];

    tlvType = (tlv[0] << 8) | tlv[1];

    platCfg = FM_PLAT_GET_CFG;

    if (platCfg->debug & CFG_DBG_CONFIG)
    {
        fmUtilConfigPropertyDecodeTlv(tlv, propTxt, sizeof(propTxt));
        FM_LOG_PRINT("prop 0x%04x: [%s]\n", tlvType, propTxt);
    }

    if (tlvType < 0x2000)
    {
        status = fmLoadApiPropertyTlv(tlv);
    }
    else if (tlvType < 0x4000)
    {
        status = fmPlatformLoadLTCfgTlv(tlv);
    }
    else
    {
        status = fmPlatformLoadLibCfgTlv(tlv);        
    }

    if (status)
    {
        FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "%s: Unable to load tlv 0x%04x\n",
                 fmErrorMsg(status), tlvType);

    }

    return status;

}   /* end fmPlatformLoadTlv */




/*****************************************************************************/
/** fmPlatformCfgVerifyAndUpdate
 * \ingroup intPlatform
 *
 * \desc            This function can be used to verify for consistency after
 *                  config is loaded. This also updates fields in config structure
 *                  that are dependent on loaded config.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformCfgVerifyAndUpdate(void)
{
    fm_int                swIdx;
    fm_platformCfgSwitch *swCfg;
    fm_platformCfgPort *  portCfg;
    fm_int                portIdx;
    fm_portCfgSwitch      tempHwResourceIdList[FM_NUM_HW_RES_ID];


    FM_MEMSET_S(tempHwResourceIdList,
                FM_NUM_HW_RES_ID * sizeof(fm_portCfgSwitch),
                0,
                FM_NUM_HW_RES_ID * sizeof(fm_portCfgSwitch));

    for (swIdx = 0; swIdx < FM_PLAT_GET_CFG->numSwitches; swIdx++)
    {
        swCfg = FM_PLAT_GET_SWITCH_CFG(swIdx);

        for (portIdx = 0 ; portIdx < swCfg->numPorts ; portIdx++)
        { 
            portCfg = FM_PLAT_GET_PORT_CFG(swIdx, portIdx);

            /**************************************************
             * Make sure that HW Resource ID is unique.
             **************************************************/

            CheckHwResourceId(swIdx,
                              portCfg,
                              tempHwResourceIdList,
                              FM_DEFAULT_HW_RES_ID);

            /**************************************************
             *  Make sure the CPU port is mapped to a PCIE port
             *  if the register access needs it.
             **************************************************/

            if (portCfg->port == swCfg->cpuPort)
            {
#ifndef FM_LT_WHITE_MODEL_SUPPORT
               if (portCfg->portType != FM_PLAT_PORT_TYPE_PCIE
#ifdef FM_SUPPORT_SWAG
                    && portCfg->swagLink.type != FM_SWAG_LINK_CPU
#endif
                   )
                {
                    if (swCfg->regAccess == FM_PLAT_REG_ACCESS_PCIE)
                    {
                        FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                                     "CPU port %d is not mapped to a PCIE port\n",
                                     swCfg->cpuPort);
                        FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_ERR_INVALID_ARGUMENT);
                    }
                }
#endif
                swCfg->mgmtPep = portCfg->pep;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_PLATFORM, FM_OK);

}   /* end fmPlatformCfgVerifyAndUpdate */




/*****************************************************************************/
/** fmPlatformLoadTlvFile
 * \ingroup intPlatform
 *
 * \desc            Loads TLV properties from a file.
 *
 * \param[in]       fileName is the full path to a text file to load.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmPlatformLoadTlvFile(fm_text fileName)
{

    fm_status       status = FM_OK;
    FILE           *fp;
    fm_uint         numRead;
    char            strErrBuf[FM_STRERROR_BUF_SIZE];
    errno_t         strErrNum;
    fm_byte         tlv[256+4];
    fm_uint         tlvLen;
    fm_uint         tlvType;

    if (fileName == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    fp = fopen(fileName, "rt");

    if (fp == NULL)
    {
        strErrNum = FM_STRERROR_S(strErrBuf, FM_STRERROR_BUF_SIZE, errno);
        if (strErrNum == 0)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to open '%s' - '%s'\n", fileName, strErrBuf );
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to open '%s' - '%d'\n", fileName, errno );
        }
        return FM_FAIL;
    }

    /* Read one TLV at a time */
    while ((numRead = fread(tlv, 1, 3, fp)) > 0)
    {
        if (numRead != 3)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Expected 3 bytes read but got %d", numRead );
            fclose(fp);
            return FM_FAIL;
        }
        tlvLen = tlv[2];
        tlvType = (tlv[0] << 8) | tlv[1];

        numRead = fread(tlv + 3, 1, tlvLen, fp);
        if (tlvLen != numRead)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Expected %d bytes read for TLV type %04x but got %d", tlvLen, tlvType, numRead );
            fclose(fp);
            return FM_FAIL;            
        }

        status = fmPlatformLoadTlv(tlv);
        if (status != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PLATFORM,
                "Unable to decode TLV type [%04x]\n", tlvType);
        }
    }

    fclose(fp);

    if (FM_PLAT_GET_CFG->debug & CFG_DBG_CONFIG)
    {
        fmPlatformCfgDump();
    }

    if (status == FM_OK)
    {
        status = fmPlatformCfgVerifyAndUpdate();
    }

    return status;

}    /* end fmPlatformLoadTlvFile */

