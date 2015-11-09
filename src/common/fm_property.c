/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_property.c
 * Creation Date:   September 10, 2007
 * Description:     Contains a subsystem for containing run-time accessible
 *                  properties.  The subsystem begins empty and is intended
 *                  to be set by other components (platform, ALOS).
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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

#include <fm_sdk_int.h>
#include <platforms/util/fm_util_config_tlv.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define _FORMAT_I     "    %-30s: %d\n"
#define _FORMAT_H     "    %-30s: 0x%x\n"
#define _FORMAT_T     "    %-30s: %s\n"



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
/* CopyTlvStr
 * \ingroup intApi
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
 * \ingroup intApi
 *
 * \desc            Get up to 32-bit integer from TLV bytes.
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
    for (j = 1 ; j < tlvLen ; j++)
    {
        value <<= 8;
        value  |= (tlv[j] & 0xFF);
    }

    /* Extend the sign to 32-bit */
    if (tlv[tlvLen - 1] & 0x80)
    {
        for (j = tlvLen ; j < 4 ; j++)
        {
            value |= (0xFF <<( j*8));
        }
    }

    return value;

}   /* end GetTlvInt */




/*****************************************************************************/
/* GetTlvBool
 * \ingroup intApi
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



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fmInitializeApiProperties
 * \ingroup intApi
 *
 * \desc            Initializes the API properties subsystem. These properties
 *                  are used to provide run-time and boot-time access to
 *                  configuration parameters at the API level.
 *
 * \param           None.
 *
 * \return          FM_OK always.
 *
 *****************************************************************************/
fm_status fmInitializeApiProperties(void)
{
    fm_status err = FM_OK;
    fm_property *prop;
#if defined(FM_SUPPORT_FM10000)
    fm10000_property *fm10kProp;
#endif

    prop = GET_PROPERTY();

    prop->defStateVlanMember = FM_AAD_API_STP_DEF_STATE_VLAN_MEMBER;
    prop->directSendToCpu = FM_AAD_API_DIRECT_SEND_TO_CPU;
    prop->defStateVlanNonMember = FM_AAD_API_STP_DEF_STATE_VLAN_NON_MEMBER;
    prop->bootIdentifySw = FM_AAD_DEBUG_BOOT_IDENTIFYSWITCH;
    prop->bootReset = FM_AAD_DEBUG_BOOT_RESET;
    prop->autoInsertSwitches = FM_AAD_DEBUG_BOOT_AUTOINSERTSWITCH;
    prop->deviceResetTime = FM_AAD_API_BOOT_RESET_TIME;
    prop->swagAutoEnableLinks = FM_AAD_API_AUTO_ENABLE_SWAG_LINKS;
    prop->eventBlockThreshold = FM_AAD_API_FREE_EVENT_BLOCK_THRESHOLD;
    prop->eventUnblockThreshold = FM_AAD_API_FREE_EVENT_UNBLOCK_THRESHOLD;
    prop->eventSemTimeout = FM_AAD_API_EVENT_SEM_TIMEOUT;
    prop->rxDirectEnqueueing = FM_AAD_API_PACKET_RX_DIRECT_ENQUEUEING;
    prop->rxDriverDestinations = FM_AAD_API_PACKET_RX_DRV_DEST;
    prop->lagAsyncDeletion = FM_AAD_API_ASYNC_LAG_DELETION;
    prop->maEventOnStaticAddr = FM_AAD_API_MA_EVENT_ON_STATIC_ADDR;
    prop->maEventOnDynAddr = FM_AAD_API_MA_EVENT_ON_DYNAMIC_ADDR;
    prop->maEventOnAddrChange = FM_AAD_API_MA_EVENT_ON_ADDR_CHANGE;
    prop->maFlushOnPortDown = FM_AAD_API_MA_FLUSH_ON_PORT_DOWN;
    prop->maFlushOnVlanChange = FM_AAD_API_MA_FLUSH_ON_VLAN_CHANGE;
    prop->maFlushOnLagChange = FM_AAD_API_MA_FLUSH_ON_LAG_CHANGE;
    prop->maTcnFifoBurstSize = FM_AAD_API_MA_TCN_FIFO_BURST_SIZE;
    prop->swagIntVlanStats = FM_AAD_API_SWAG_INTERNAL_VLAN_STATS;
    prop->perLagManagement = FM_AAD_API_PER_LAG_MANAGEMENT;
    prop->parityRepairEnable = FM_AAD_API_PARITY_REPAIR_ENABLE;
    prop->swagMaxAclPortSets = FM_AAD_API_SWAG_MAX_ACL_PORT_SETS;
    prop->maxPortSets = FM_AAD_API_MAX_PORT_SETS;
    prop->packetReceiveEnable = FM_AAD_API_PACKET_RECEIVE_ENABLE;
    prop->multicastSingleAddress = FM_AAD_API_1_ADDR_PER_MCAST_GROUP;
    prop->modelPosition = FM_AAD_API_PLATFORM_MODEL_POSITION;
    prop->vnNumNextHops = FM_AAD_API_NUM_VN_TUNNEL_NEXTHOPS;
    prop->vnEncapProtocol = FM_AAD_API_VN_ENCAP_PROTOCOL;
    prop->vnEncapVersion = FM_AAD_API_VN_ENCAP_VERSION;
    prop->supportRouteLookups = FM_AAD_API_SUPPORT_ROUTE_LOOKUPS;
    prop->routeMaintenanceEnable = FM_AAD_API_ROUTING_MAINTENANCE_ENABLE;
    prop->autoVlan2Tagging = FM_AAD_API_AUTO_VLAN2_TAGGING;
  
    prop->interruptHandlerDisable = FM_AAD_DEBUG_BOOT_INTERRUPT_HANDLER;
    prop->maTableMaintenanceEnable = FM_AAD_API_MA_TABLE_MAINTENENANCE_ENABLE;
    prop->fastMaintenanceEnable = FM_AAD_API_FAST_MAINTENANCE_ENABLE;
    prop->fastMaintenancePer = FM_AAD_API_FAST_MAINTENANCE_PERIOD;
    prop->strictGlotPhysical = FM_AAD_API_STRICT_GLORT_PHYSICAL;
    prop->resetWmAtPauseOff = FM_AAD_API_RESET_WATERMARK_AT_PAUSE_OFF;
    prop->swagAutoSubSwitches = FM_AAD_API_SWAG_AUTO_SUB_SWITCHES;
    prop->swagAutoIntPorts = FM_AAD_API_SWAG_AUTO_INTERNAL_PORTS;
    prop->swagAutoVNVsi = FM_AAD_API_SWAG_AUTO_VN_VSI;
    prop->byPassEnable = FM_AAD_API_PLATFORM_BYPASS_ENABLE;
    prop->lagDelSemTimeout = FM_AAD_API_LAG_DELETE_SEMAPHORE_TIMEOUT;
    prop->stpEnIntPortCtrl = FM_AAD_API_STP_ENABLE_INTERNAL_PORT_CTRL;
    prop->modelPortMapType = FM_AAD_API_PLATFORM_MODEL_PORT_MAP_TYPE;
    FM_SNPRINTF_S(prop->modelSwitchType,
            sizeof(prop->modelSwitchType), "%s",
            FM_AAD_API_PLATFORM_MODEL_SWITCH_TYPE);
    prop->modelSendEOT = FM_AAD_API_PLATFORM_MODEL_SEND_EOT;
    prop->modelLogEgressInfo = FM_AAD_API_PLATFORM_MODEL_LOG_EGRESS_INFO;
    prop->enableRefClock = FM_AAD_API_PLATFORM_ENABLE_REF_CLOCK;
    prop->setRefClock = FM_AAD_API_PLATFORM_SET_REF_CLOCK;
    prop->priorityBufQueues = FM_AAD_API_PLATFORM_PRIORITY_BUFFER_QUEUES;
    prop->pktSchedType = FM_AAD_API_PLATFORM_PKT_SCHED_TYPE;
    prop->separateBufPoolEnable = FM_AAD_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE;
    prop->numBuffersRx = FM_AAD_API_PLATFORM_NUM_BUFFERS_RX;
    prop->numBuffersTx = FM_AAD_API_PLATFORM_NUM_BUFFERS_TX;
    FM_SNPRINTF_S(prop->modelPktInterface,
            sizeof(prop->modelPktInterface), "%s",
            FM_AAD_API_PLATFORM_MODEL_PKT_INTERFACE);
    FM_SNPRINTF_S(prop->pktInterface,
            sizeof(prop->pktInterface), "%s",
            FM_AAD_API_PLATFORM_PKT_INTERFACE);
    FM_SNPRINTF_S(prop->modelTopologyName,
            sizeof(prop->modelTopologyName), "%s",
            FM_AAD_API_PLATFORM_MODEL_TOPOLOGY_NAME);
    prop->modelUseModelPath = FM_AAD_API_PLATFORM_MODEL_TOPOLOGY_USE_MODEL_PATH;
    FM_SNPRINTF_S(prop->modelDevBoardIp,
            sizeof(prop->modelDevBoardIp), "%s",
            FM_AAD_API_PLATFORM_MODEL_DEV_BOARD_IP);
    prop->modelDevBoardPort = FM_AAD_API_PLATFORM_MODEL_DEV_BOARD_PORT;
    prop->modelDeviceCfg = FM_AAD_API_PLATFORM_MODEL_DEVICE_CFG;
    prop->modelChipVersion = FM_AAD_API_PLATFORM_MODEL_CHIP_VERSION;
    prop->sbusServerPort = FM_AAD_API_PLATFORM_SBUS_SERVER_PORT;
    prop->isWhiteModel = FM_AAD_API_PLATFORM_IS_WHITE_MODEL;
    FM_SNPRINTF_S(prop->initLoggingCat,
            sizeof(prop->initLoggingCat), "%s",
            FM_AAD_API_DEBUG_INIT_LOGGING_CAT);
    prop->addPepsToFlooding = FM_AAD_API_PORT_ADD_PEPS_TO_FLOODING;
    prop->allowFtagVlanTagging = FM_AAD_API_PORT_ALLOW_FTAG_VLAN_TAGGING;
    prop->ignoreBwViolation = 0;
    prop->dfeAllowEarlyLinkUp = FM_AAD_API_DFE_ALLOW_EARLY_LINK_UP_MODE;
    prop->dfeAllowKrPcal = FM_AAD_API_DFE_ALLOW_KR_PCAL_MODE;
    prop->dfeEnableSigOkDebounce = FM_AAD_API_DFE_ENABLE_SIGNALOK_DEBOUNCING;
    prop->enableStatusPolling = FM_AAD_API_PORT_ENABLE_STATUS_POLLING;
    prop->gsmeTimestampMode = FM_AAD_API_GSME_TIMESTAMP_MODE;
    prop->hniMcastFlooding = FM_AAD_API_MULTICAST_HNI_FLOODING;
    prop->hniMacEntriesPerPep = FM_AAD_API_HNI_MAC_ENTRIES_PER_PEP;
    prop->hniMacEntriesPerPort = FM_AAD_API_HNI_MAC_ENTRIES_PER_PORT;
    prop->hniInnOutEntriesPerPep = FM_AAD_API_HNI_INN_OUT_ENTRIES_PER_PEP;
    prop->hniInnOutEntriesPerPort = FM_AAD_API_HNI_INN_OUT_ENTRIES_PER_PORT;
    prop->anTimerAllowOutSpec = FM_AAD_API_AN_INHBT_TIMER_ALLOW_OUT_OF_SPEC;


#if defined(FM_SUPPORT_FM10000)
    fm10kProp = GET_FM10000_PROPERTY();
    FM_SNPRINTF_S(fm10kProp->wmSelect,
            sizeof(fm10kProp->wmSelect), "%s",
            FM_AAD_API_FM10000_WMSELECT);
    fm10kProp->cmRxSmpPrivBytes = FM_AAD_API_FM10000_CM_RX_SMP_PRIVATE_BYTES;
    fm10kProp->cmTxTcHogBytes = FM_AAD_API_FM10000_CM_TX_TC_HOG_BYTES;
    fm10kProp->cmSmpSdVsHogPercent = FM_AAD_API_FM10000_CM_SMP_SD_VS_HOG_PERCENT;
    fm10kProp->cmSmpSdJitterBits = FM_AAD_API_FM10000_CM_SMP_SD_JITTER_BITS;
    fm10kProp->cmTxSdOnPrivate = FM_AAD_API_FM10000_CM_TX_SD_ON_PRIVATE;
    fm10kProp->cmTxSdOnSmpFree = FM_AAD_API_FM10000_CM_TX_SD_ON_SMP_FREE;
    fm10kProp->cmPauseBufferBytes = FM_AAD_API_FM10000_CM_PAUSE_BUFFER_BYTES;
    fm10kProp->mcastMaxEntriesPerCam = FM_AAD_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM;
    fm10kProp->ffuUcastSliceRangeFirst = FM_AAD_API_FM10000_FFU_UNICAST_SLICE_1ST;
    fm10kProp->ffuUcastSliceRangeLast = FM_AAD_API_FM10000_FFU_UNICAST_SLICE_LAST;
    fm10kProp->ffuMcastSliceRangeFirst = FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_1ST;
    fm10kProp->ffuMcastSliceRangeLast = FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_LAST;
    fm10kProp->ffuAclSliceRangeFirst = FM_AAD_API_FM10000_FFU_ACL_SLICE_1ST;
    fm10kProp->ffuAclSliceRangeLast = FM_AAD_API_FM10000_FFU_ACL_SLICE_LAST;
    fm10kProp->ffuMapMacResvdForRoute = FM_AAD_API_FM10000_FFU_MAPMAC_ROUTING;
    fm10kProp->ffuUcastPrecedenceMin = FM_AAD_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN;
    fm10kProp->ffuUcastPrecedenceMax = FM_AAD_API_FM10000_FFU_UNICAST_PRECEDENCE_MAX;
    fm10kProp->ffuMcastPrecedenceMin = FM_AAD_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN;
    fm10kProp->ffuMcastPrecedenceMax = FM_AAD_API_FM10000_FFU_MULTICAST_PRECEDENCE_MAX;
    fm10kProp->ffuAclPrecedenceMin = FM_AAD_API_FM10000_FFU_ACL_PRECEDENCE_MIN;
    fm10kProp->ffuAclPrecedenceMax = FM_AAD_API_FM10000_FFU_ACL_PRECEDENCE_MAX;
    fm10kProp->ffuAclStrictCountPolice = FM_AAD_API_FM10000_FFU_ACL_STRICT_COUNT_POLICE;
    fm10kProp->initUcastFloodTriggers = FM_AAD_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS;
    fm10kProp->initMcastFloodTriggers = FM_AAD_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS;
    fm10kProp->initBcastFloodTriggers = FM_AAD_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS;
    fm10kProp->initResvdMacTriggers = FM_AAD_API_FM10000_INIT_RESERVED_MAC_TRIGGERS;
    fm10kProp->floodingTrapPriority = FM_AAD_API_FM10000_FLOODING_TRAP_PRIORITY;
    fm10kProp->autonegGenerateEvents = FM_AAD_API_FM10000_AUTONEG_GENERATE_EVENTS;
    fm10kProp->linkDependsOfDfe = FM_AAD_API_FM10000_LINK_DEPENDS_ON_DFE;
    fm10kProp->vnUseSharedEncapFlows = FM_AAD_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS;
    fm10kProp->vnMaxRemoteAddress = FM_AAD_API_FM10000_VN_MAX_TUNNEL_RULES;
    fm10kProp->vnTunnelGroupHashSize = FM_AAD_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE;
    fm10kProp->vnTeVid = FM_AAD_API_FM10000_VN_TE_VID;
    fm10kProp->vnEncapAclNumber = FM_AAD_API_FM10000_VN_ENCAP_ACL_NUM;
    fm10kProp->vnDecapAclNumber = FM_AAD_API_FM10000_VN_DECAP_ACL_NUM;
    fm10kProp->mcastNumStackGroups = FM_AAD_API_FM10000_MCAST_NUM_STACK_GROUPS;
    fm10kProp->vnTunnelOnlyOnIngress = FM_AAD_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS;
    fm10kProp->mtableCleanupWm = FM_AAD_API_FM10000_MTABLE_CLEANUP_WATERMARK;
    FM_SNPRINTF_S(fm10kProp->schedMode,
            sizeof(fm10kProp->schedMode), "%s",
            FM_AAD_API_FM10000_SCHED_MODE);
    fm10kProp->updateSchedOnLinkChange = FM_AAD_API_FM10000_UPD_SCHED_ON_LNK_CHANGE;

    fm10kProp->createRemoteLogicalPorts = FM_AAD_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS;
    fm10kProp->autonegCl37Timeout = FM_AAD_API_FM10000_AUTONEG_CLAUSE_37_TIMEOUT;
    fm10kProp->autonegSgmiiTimeout = FM_AAD_API_FM10000_AUTONEG_SGMII_TIMEOUT;
    fm10kProp->useHniServicesLoopback = FM_AAD_API_FM10000_HNI_SERVICES_LOOPBACK;
    fm10kProp->antiBubbleWm = FM_AAD_API_FM10000_ANTI_BUBBLE_WM;
    fm10kProp->serdesOpMode = FM_AAD_API_FM10000_SERDES_OP_MODE;
    fm10kProp->serdesDbgLevel = FM_AAD_API_FM10000_SERDES_DBG_LVL;
    fm10kProp->parityEnableInterrupts = FM_AAD_API_FM10000_PARITY_INTERRUPTS;
    fm10kProp->parityStartTcamMonitors = FM_AAD_API_FM10000_START_TCAM_MONITORS;
    fm10kProp->parityCrmTimeout = FM_AAD_API_FM10000_CRM_TIMEOUT;
    fm10kProp->schedOverspeed = FM_AAD_API_FM10000_SCHED_OVERSPEED;
    fm10kProp->intrLinkIgnoreMask = FM_AAD_API_FM10000_INTR_LINK_IGNORE_MASK;
    fm10kProp->intrAutonegIgnoreMask = FM_AAD_API_FM10000_INTR_AUTONEG_IGNORE_MASK;
    fm10kProp->intrSerdesIgnoreMask = FM_AAD_API_FM10000_INTR_SERDES_IGNORE_MASK;
    fm10kProp->intrPcieIgnoreMask = FM_AAD_API_FM10000_INTR_PCIE_IGNORE_MASK;
    fm10kProp->intrMaTcnIgnoreMask = FM_AAD_API_FM10000_INTR_MATCN_IGNORE_MASK;
    fm10kProp->intrFhTailIgnoreMask = FM_AAD_API_FM10000_INTR_FHTAIL_IGNORE_MASK;
    fm10kProp->intrSwIgnoreMask = FM_AAD_API_FM10000_INTR_SW_IGNORE_MASK;
    fm10kProp->intrTeIgnoreMask = FM_AAD_API_FM10000_INTR_TE_IGNORE_MASK;
    fm10kProp->enableEeeSpicoIntr = FM_AAD_API_FM10000_ENABLE_EEE_SPICO_INTR;
    fm10kProp->useAlternateSpicoFw = FM_AAD_API_FM10000_USE_ALTERNATE_SPICO_FW;
    fm10kProp->allowKrPcalOnEee = FM_AAD_API_FM10000_ALLOW_KRPCAL_ON_EEE;
#endif

    err = fmCreateLock("API Property Lock", 
                       &fmRootAlos->propertyLock);

    FM_LOG_EXIT(FM_LOG_CAT_ATTR, err);

}   /* end fmInitializeApiProperties */






/*****************************************************************************/
/* fmLoadApiPropertyTlv
 * \ingroup intApi
 *
 * \desc            Load API TLV property.
 *
 * \param[in]       tlv is an array of encoded TLV bytes
 *
 * \return          FM_OK if successful.
 * \return          Other ''Status Codes'' as appropriate in case of
 *                  failure.
 *
 *****************************************************************************/
fm_status fmLoadApiPropertyTlv(fm_byte *tlv)
{
    fm_property *prop;
#if defined(FM_SUPPORT_FM10000)
    fm10000_property *fm10kProp;
#endif
    fm_uint     tlvType;
    fm_uint     tlvLen;


    prop = GET_PROPERTY();
    fm10kProp  = GET_FM10000_PROPERTY();

    tlvType = (tlv[0] << 8) | tlv[1];
    tlvLen = tlv[2];

    switch (tlvType)
    {
        case FM_TLV_API_STP_DEF_VLAN_MEMBER:
            prop->defStateVlanMember = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_DIR_SEND_TO_CPU:
            prop->directSendToCpu = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_STP_DEF_VLAN_NON_MEMBER:
            prop->defStateVlanNonMember = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_IDENTIFY_SWITCH:
            prop->bootIdentifySw = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_BOOT_RESET:
            prop->bootReset = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_AUTO_INSERT_SW:
            prop->autoInsertSwitches = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_DEV_RESET_TIME:
            prop->deviceResetTime = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_SWAG_EN_LINK:
            prop->swagAutoEnableLinks = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_EVENT_BLK_THRESHOLD:
            prop->eventBlockThreshold = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_EVENT_UNBLK_THRESHOLD:
            prop->eventUnblockThreshold = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_EVENT_SEM_TIMEOUT:
            prop->eventSemTimeout = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_RX_DIRECTED_ENQ:
            prop->rxDirectEnqueueing = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_RX_DRV_DEST:
            prop->rxDriverDestinations = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_LAG_ASYNC_DEL:
            prop->lagAsyncDeletion = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_EVENT_ON_STATIC_ADDR:
            prop->maEventOnStaticAddr = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_EVENT_ON_DYN_ADDR:
            prop->maEventOnDynAddr = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_EVENT_ON_ADDR_CHG:
            prop->maEventOnAddrChange = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_FLUSH_ON_PORT_DOWN:
            prop->maFlushOnPortDown = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_FLUSH_ON_VLAN_CHG:
            prop->maFlushOnVlanChange = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_FLUSH_ON_LAG_CHG:
            prop->maFlushOnLagChange = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_TCN_FIFO_BURST_SIZE:
            prop->maTcnFifoBurstSize = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_SWAG_INT_VLAN_STATS:
            prop->swagIntVlanStats = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PER_LAG_MGMT:
            prop->perLagManagement = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PARITY_REPAIR_EN:
            prop->parityRepairEnable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_SWAG_MAX_ACL_PORT_SET:
            prop->swagMaxAclPortSets = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_MAX_PORT_SETS:
            prop->maxPortSets = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PKT_RX_EN:
            prop->packetReceiveEnable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_MC_SINGLE_ADDR:
            prop->multicastSingleAddress = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_MODEL_POS:
            prop->modelPosition = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_VN_NUM_NH:
            prop->vnNumNextHops = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_VN_ENCAP_PROTOCOL:
            prop->vnEncapProtocol = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_VN_ENCAP_VER:
            prop->vnEncapVersion = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_SUP_ROUTE_LOOKUP:
            prop->supportRouteLookups = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_RT_MAINT_EN:
            prop->routeMaintenanceEnable = FM_AAD_API_ROUTING_MAINTENANCE_ENABLE;
        break;
        case FM_TLV_API_AUTO_VLAN2_TAG:
            prop->autoVlan2Tagging = GetTlvBool(tlv + 3);
        break;

        case FM_TLV_API_INTR_HANDLER_DIS:
            prop->interruptHandlerDisable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_MA_TABLE_MAINT_EN:
            prop->maTableMaintenanceEnable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_FAST_MAINT_EN:
            prop->fastMaintenanceEnable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_FAST_MAINT_PER:
            prop->fastMaintenancePer = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_STRICT_GLORT:
            prop->strictGlotPhysical = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_AUTO_RESET_WM:
            prop->resetWmAtPauseOff = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_AUTO_SUB_SW:
            prop->swagAutoSubSwitches = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_SWAG_AUTO_INT_PORT:
            prop->swagAutoIntPorts = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_SWAG_AUTO_NVVSI:
            prop->swagAutoVNVsi = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_BYPASS_EN:
            prop->byPassEnable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_LAG_DEL_SEM_TIMEOUT:
            prop->lagDelSemTimeout = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_STP_EN_INT_PORT_CTRL:
            prop->stpEnIntPortCtrl = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_MODEL_PORT_MAP_TYPE:
            prop->modelPortMapType = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_MODEL_SW_TYPE:
            CopyTlvStr(prop->modelSwitchType,
                    sizeof(prop->modelSwitchType),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_API_MODEL_TX_EOT:
            prop->modelSendEOT = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_MODEL_LOG_EGR_INFO:
            prop->modelLogEgressInfo = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_EN_REF_CLK:
            prop->enableRefClock = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_SET_REF_CLK:
            prop->setRefClock = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_PRI_BUF_Q:
            prop->priorityBufQueues = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_PKT_SCHED_TYPE:
            prop->pktSchedType = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_SEP_BUF_POOL:
            prop->separateBufPoolEnable = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_NUM_BUF_RX:
            prop->numBuffersRx = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_NUM_BUF_TX:
            prop->numBuffersTx = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_MODEL_PKT_INTF:
            CopyTlvStr(prop->modelPktInterface,
                    sizeof(prop->modelPktInterface),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_PKT_INTF:
            CopyTlvStr(prop->pktInterface,
                    sizeof(prop->pktInterface),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_MODEL_TOPO:
            CopyTlvStr(prop->modelTopologyName,
                    sizeof(prop->modelTopologyName),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_MODEL_USE_PATH:
            prop->modelUseModelPath = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PLAT_DEV_BOARD_IP:
            CopyTlvStr(prop->modelDevBoardIp,
                    sizeof(prop->modelDevBoardIp),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_DEV_BOARD_PORT:
            prop->modelDevBoardPort = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_MODEL_DEVICE_CFG:
            prop->modelDeviceCfg = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_MODEL_CHIP_VERSION:
            prop->modelChipVersion = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_SBUS_SERVER_PORT:
            prop->sbusServerPort = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_PLAT_IS_WHITE_MODEL:
            prop->isWhiteModel = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_DBG_INT_LOG_CAT:
            CopyTlvStr(prop->initLoggingCat,
                    sizeof(prop->initLoggingCat),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_API_ADD_PEP_FLOOD:
            prop->addPepsToFlooding = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_ADD_ALLOW_FTAG_VLAN_TAG:
            prop->allowFtagVlanTagging = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_IGNORE_BW_VIOLATION:
            prop->ignoreBwViolation = GetTlvBool(tlv + 3) ? 1 : 0;
        break;
        case FM_TLV_API_IGNORE_BW_VIOLATION_NW:
            prop->ignoreBwViolation = GetTlvBool(tlv + 3) ? 2 : 0;
        break;
        case FM_TLV_API_DFE_EARLY_LNK_UP:
            prop->dfeAllowEarlyLinkUp = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_DFE_ALLOW_KR_PCAL:
            prop->dfeAllowKrPcal = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_DFE_ENABLE_SIGNALOK_DEBOUNCING:
            prop->dfeEnableSigOkDebounce = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_PORT_ENABLE_STATUS_POLLING:
            prop->enableStatusPolling = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_GSME_TS_MODE:
            prop->gsmeTimestampMode = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_MC_HNI_FLOODING:
            prop->hniMcastFlooding = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_API_HNI_MAC_ENTRIES_PER_PEP:
            prop->hniMacEntriesPerPep = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_HNI_MAC_ENTRIES_PER_PORT:
            prop->hniMacEntriesPerPort = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_HNI_INN_OUT_ENTRIES_PER_PEP:
            prop->hniInnOutEntriesPerPep = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_HNI_INN_OUT_ENTRIES_PER_PORT:
            prop->hniInnOutEntriesPerPort = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_API_AN_TIMER_ALLOW_OUT_SPEC:
            prop->anTimerAllowOutSpec = GetTlvBool(tlv + 3);
        break;

#if defined(FM_SUPPORT_FM10000)
        case FM_TLV_FM10K_WMSELECT:
            CopyTlvStr(fm10kProp->wmSelect,
                    sizeof(fm10kProp->wmSelect),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_CM_RX_SMP_PRIVATE_BYTES:
            fm10kProp->cmRxSmpPrivBytes = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_CM_TX_TC_HOG_BYTES:
            fm10kProp->cmTxTcHogBytes = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_CM_SMP_SD_VS_HOG_PERCENT:
            fm10kProp->cmSmpSdVsHogPercent = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_CM_SMP_SD_JITTER_BITS:
            fm10kProp->cmSmpSdJitterBits = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_CM_TX_SD_ON_PRIVATE:
            fm10kProp->cmTxSdOnPrivate = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_CM_TX_SD_ON_SMP_FREE:
            fm10kProp->cmTxSdOnSmpFree = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_CM_PAUSE_BUFFER_BYTES:
            fm10kProp->cmPauseBufferBytes = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_MCAST_MAX_ENTRIES_PER_CAM:
            fm10kProp->mcastMaxEntriesPerCam = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_UNICAST_SLICE_1ST:
            fm10kProp->ffuUcastSliceRangeFirst = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_UNICAST_SLICE_LAST:
            fm10kProp->ffuUcastSliceRangeLast = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_MULTICAST_SLICE_1ST:
            fm10kProp->ffuMcastSliceRangeFirst = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_MULTICAST_SLICE_LAST:
            fm10kProp->ffuMcastSliceRangeLast = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_ACL_SLICE_1ST:
            fm10kProp->ffuAclSliceRangeFirst = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_ACL_SLICE_LAST:
            fm10kProp->ffuAclSliceRangeLast = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_MAPMAC_ROUTING:
            fm10kProp->ffuMapMacResvdForRoute = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_UNICAST_PRECEDENCE_MIN:
            fm10kProp->ffuUcastPrecedenceMin = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_UNICAST_PRECEDENCE_MAX:
            fm10kProp->ffuUcastPrecedenceMax = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_MULTICAST_PRECEDENCE_MIN:
            fm10kProp->ffuMcastPrecedenceMin = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_MULTICAST_PRECEDENCE_MAX:
            fm10kProp->ffuMcastPrecedenceMax = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_ACL_PRECEDENCE_MIN:
            fm10kProp->ffuAclPrecedenceMin = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_ACL_PRECEDENCE_MAX:
            fm10kProp->ffuAclPrecedenceMax = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_FFU_ACL_STRICT_COUNT_POLICE:
            fm10kProp->ffuAclStrictCountPolice = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_INIT_UCAST_FLOODING_TRIGGERS:
            fm10kProp->initUcastFloodTriggers = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_INIT_MCAST_FLOODING_TRIGGERS:
            fm10kProp->initMcastFloodTriggers = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_INIT_BCAST_FLOODING_TRIGGERS:
            fm10kProp->initBcastFloodTriggers = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_INIT_RESERVED_MAC_TRIGGERS:
            fm10kProp->initResvdMacTriggers = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_FLOODING_TRAP_PRIORITY:
            fm10kProp->floodingTrapPriority = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_AUTONEG_GENERATE_EVENTS:
            fm10kProp->autonegGenerateEvents = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_LINK_DEPENDS_ON_DFE:
            fm10kProp->linkDependsOfDfe = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_VN_USE_SHARED_ENCAP_FLOWS:
            fm10kProp->vnUseSharedEncapFlows = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_VN_MAX_TUNNEL_RULES:
            fm10kProp->vnMaxRemoteAddress = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_VN_TUNNEL_GROUP_HASH_SIZE:
            fm10kProp->vnTunnelGroupHashSize = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_VN_TE_VID:
            fm10kProp->vnTeVid = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_VN_ENCAP_ACL_NUM:
            fm10kProp->vnEncapAclNumber = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_VN_DECAP_ACL_NUM:
            fm10kProp->vnDecapAclNumber = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_MCAST_NUM_STACK_GROUPS:
            fm10kProp->mcastNumStackGroups = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_VN_TUNNEL_ONLY_IN_INGRESS:
            fm10kProp->vnTunnelOnlyOnIngress = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_MTABLE_CLEANUP_WATERMARK:
            fm10kProp->mtableCleanupWm = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_SCHED_MODE:
            CopyTlvStr(fm10kProp->schedMode,
                    sizeof(fm10kProp->schedMode),
                    tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_UPD_SCHED_ON_LNK_CHANGE:
            fm10kProp->updateSchedOnLinkChange = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_CREATE_REMOTE_LOGICAL_PORTS:
            fm10kProp->createRemoteLogicalPorts = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_AUTONEG_CLAUSE_37_TIMEOUT:
            fm10kProp->autonegCl37Timeout = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_AUTONEG_SGMII_TIMEOUT:
            fm10kProp->autonegSgmiiTimeout = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_HNI_SERVICES_LOOPBACK:
            fm10kProp->useHniServicesLoopback = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_ANTI_BUBBLE_WM:
            fm10kProp->antiBubbleWm = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_SERDES_OP_MODE:
            fm10kProp->serdesOpMode = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_SERDES_DBG_LVL:
            fm10kProp->serdesDbgLevel = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_PARITY_INTERRUPTS:
            fm10kProp->parityEnableInterrupts = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_PARITY_TCAM_MONITOR:
            fm10kProp->parityStartTcamMonitors = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_PARITY_CRM_TIMEOUT:
            fm10kProp->parityCrmTimeout = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_SCHED_OVERSPEED:
            fm10kProp->schedOverspeed = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_LINK_IGNORE_MASK:
            fm10kProp->intrLinkIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_AUTONEG_IGNORE_MASK:
            fm10kProp->intrAutonegIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_SERDES_IGNORE_MASK:
            fm10kProp->intrSerdesIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_PCIE_IGNORE_MASK:
            fm10kProp->intrPcieIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_MATCN_IGNORE_MASK:
            fm10kProp->intrMaTcnIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_FHTAIL_IGNORE_MASK:
            fm10kProp->intrFhTailIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_SW_IGNORE_MASK:
            fm10kProp->intrSwIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_INTR_TE_IGNORE_MASK:
            fm10kProp->intrTeIgnoreMask = GetTlvInt(tlv + 3, tlvLen);
        break;
        case FM_TLV_FM10K_EEE_SPICO_INTR:
            fm10kProp->enableEeeSpicoIntr = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_USE_ALTERNATE_SPICO_FW:
            fm10kProp->useAlternateSpicoFw = GetTlvBool(tlv + 3);
        break;
        case FM_TLV_FM10K_ALLOW_KRPCAL_ON_EEE:
            fm10kProp->allowKrPcalOnEee = GetTlvBool(tlv + 3);
        break;
#endif

        default:
            FM_LOG_FATAL(FM_LOG_CAT_PLATFORM,
                "Unhandled TLV type 0x%04x\n",
                tlvType);
            return FM_ERR_INVALID_ARGUMENT;
    }

    return FM_OK;

} /* end fmLoadApiPropertyTlv */




/*****************************************************************************/
/** fmSetApiProperty
 * \ingroup api
 *
 * \desc            Adds the given property to the database via the given key.
 *
 * \param[in]       key is the dotted string key (see ''API Properties'').
 *
 * \param[in]       attrType is one of the valid type constants.
 *
 * \param[in]       value points to caller supplied storage where the specific
 *                  value is stored. Its type is assumed to match attrType.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmSetApiProperty(fm_text        key,
                           fm_apiAttrType attrType,
                           void *         value)
{
    fm_status      err;
    fm_status      err2;
    fm_byte        tlv[FM_TLV_MAX_BUF_SIZE];
    fm_char        line[512];

    FM_LOG_ENTRY_API(FM_LOG_CAT_ATTR,
                     "key=%s type=%d value=%p\n",
                     key,
                     attrType,
                     value);

    err = fmCaptureLock(&fmRootAlos->propertyLock, FM_WAIT_FOREVER);
    if (err != FM_OK)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ATTR, err);
    }

    
    switch (attrType)
    {
        case FM_API_ATTR_INT:
            FM_SPRINTF_S(line,
                        sizeof(line),
                        "%s int %d",
                        key,
                        *(fm_int*)value);
            break;

        case FM_API_ATTR_BOOL:
            FM_SPRINTF_S(line,
                        sizeof(line),
                        "%s bool %d",
                        key,
                        *(fm_bool*)value);
            break;

        case FM_API_ATTR_FLOAT:
            FM_SPRINTF_S(line,
                        sizeof(line),
                        "%s float %f",
                        key,
                        *(fm_float*)value);
            break;

        case FM_API_ATTR_TEXT:
            FM_SPRINTF_S(line,
                        sizeof(line),
                        "%s text %s",
                        key,
                        (fm_text)value);
            break;

        default:
            FM_LOG_FATAL(FM_LOG_CAT_ATTR,
                         "Unknown type %d for set of property %s\n",
                         attrType, key);
            goto ABORT;

    }   /* end switch (attrType) */

    err = fmUtilConfigPropertyEncodeTlv(line, tlv, sizeof(tlv));

    if (err)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ATTR,
            "Unable to encode API property: [%s]\n", line);
        goto ABORT;
    }

    err = fmLoadApiPropertyTlv(tlv);

ABORT:

    err2 = fmReleaseLock(&fmRootAlos->propertyLock);

    if (err == FM_OK && err2)
    {
        err = err2;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_ATTR, err);

}   /* end fmSetApiProperty */




/*****************************************************************************/
/** fmGetApiProperty
 * \ingroup api
 *
 * \desc            Retrieves the value of the given key from the database.
 *                  Note that for text valued keys, the returned pointer
 *                  points to the internally stored value of the key,
 *                  so the caller must make sure to not modify this string.
 *
 * \param[in]       key is the dotted string key (see ''API Properties'').
 *
 * \param[in]       attrType is the expected type of the key. See
 *                  ''fm_apiAttrType''.
 *
 * \param[in]       value points to caller allocated storage where the key's
 *                  value will be stored.
 *
 * \return          FM_OK on success.
 * \return          FM_ERR_KEY_NOT_FOUND if the key is invalid.
 *
 *****************************************************************************/
fm_status fmGetApiProperty(fm_text        key,
                           fm_apiAttrType attrType,
                           void *         value)
{
    fm_status       err       = FM_OK;
    fm_apiAttrType  expType   = FM_API_ATTR_MAX;
    fm_int          valInt    = 0;
    fm_bool         valBool   = FALSE;
    fm_float        valFloat  = 0;
    fm_text         valText   = NULL;
    fm_property     *prop;
#if defined(FM_SUPPORT_FM10000)
    fm10000_property *fm10kProp;
#endif

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_ATTR,
                         "key=%s value=%p\n",
                         key,
                         value);

    err = fmCaptureLock(&fmRootAlos->propertyLock, FM_WAIT_FOREVER);
    if (err != FM_OK)
    {
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ATTR, err);
    }

    prop = GET_PROPERTY();

    if (strcmp(key, FM_AAK_API_STP_DEF_STATE_VLAN_MEMBER) == 0)
    {
        valInt = prop->defStateVlanMember;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_DIRECT_SEND_TO_CPU) == 0)
    {
        valBool = prop->directSendToCpu;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_STP_DEF_STATE_VLAN_NON_MEMBER) == 0)
    {
        valInt = prop->defStateVlanNonMember;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_DEBUG_BOOT_IDENTIFYSWITCH) == 0)
    {
        valBool = prop->bootIdentifySw;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_DEBUG_BOOT_RESET) == 0)
    {
        valBool = prop->bootReset;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_DEBUG_BOOT_AUTOINSERTSWITCH) == 0)
    {
        valBool = prop->autoInsertSwitches;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_BOOT_RESET_TIME) == 0)
    {
        valInt = prop->deviceResetTime;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_AUTO_ENABLE_SWAG_LINKS) == 0)
    {
        valBool = prop->swagAutoEnableLinks;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_FREE_EVENT_BLOCK_THRESHOLD) == 0)
    {
        valInt = prop->eventBlockThreshold;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_FREE_EVENT_UNBLOCK_THRESHOLD) == 0)
    {
        valInt = prop->eventUnblockThreshold;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_EVENT_SEM_TIMEOUT) == 0)
    {
        valInt = prop->eventSemTimeout;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PACKET_RX_DIRECT_ENQUEUEING) == 0)
    {
        valBool = prop->rxDirectEnqueueing;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PACKET_RX_DRV_DEST) == 0)
    {
        valInt = prop->rxDriverDestinations;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_ASYNC_LAG_DELETION) == 0)
    {
        valBool = prop->lagAsyncDeletion;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_EVENT_ON_STATIC_ADDR) == 0)
    {
        valBool = prop->maEventOnStaticAddr;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_EVENT_ON_DYNAMIC_ADDR) == 0)
    {
        valBool = prop->maEventOnDynAddr;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_EVENT_ON_ADDR_CHANGE) == 0)
    {
        valBool = prop->maEventOnAddrChange;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_FLUSH_ON_PORT_DOWN) == 0)
    {
        valBool = prop->maFlushOnPortDown;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_FLUSH_ON_VLAN_CHANGE) == 0)
    {
        valBool = prop->maFlushOnVlanChange;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_FLUSH_ON_LAG_CHANGE) == 0)
    {
        valBool = prop->maFlushOnLagChange;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_TCN_FIFO_BURST_SIZE) == 0)
    {
        valInt = prop->maTcnFifoBurstSize;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_SWAG_INTERNAL_VLAN_STATS) == 0)
    {
        valBool = prop->swagIntVlanStats;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PER_LAG_MANAGEMENT) == 0)
    {
        valBool = prop->perLagManagement;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PARITY_REPAIR_ENABLE) == 0)
    {
        valBool = prop->parityRepairEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_SWAG_MAX_ACL_PORT_SETS) == 0)
    {
        valInt = prop->swagMaxAclPortSets;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_MAX_PORT_SETS) == 0)
    {
        valInt = prop->maxPortSets;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PACKET_RECEIVE_ENABLE) == 0)
    {
        valBool = prop->packetReceiveEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_1_ADDR_PER_MCAST_GROUP) == 0)
    {
        valBool = prop->multicastSingleAddress;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_POSITION) == 0)
    {
        valInt = prop->modelPosition;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_NUM_VN_TUNNEL_NEXTHOPS) == 0)
    {
        valInt = prop->vnNumNextHops;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_VN_ENCAP_PROTOCOL) == 0)
    {
        valInt = prop->vnEncapProtocol;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_VN_ENCAP_VERSION) == 0)
    {
        valInt = prop->vnEncapVersion;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_SUPPORT_ROUTE_LOOKUPS) == 0)
    {
        valBool = prop->supportRouteLookups;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_ROUTING_MAINTENANCE_ENABLE) == 0)
    {
        valBool = prop->routeMaintenanceEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_AUTO_VLAN2_TAGGING) == 0)
    {
        valBool = prop->autoVlan2Tagging;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_DEBUG_BOOT_INTERRUPT_HANDLER) == 0)
    {
        valBool = prop->interruptHandlerDisable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MA_TABLE_MAINTENENANCE_ENABLE) == 0)
    {
        valBool = prop->maTableMaintenanceEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_FAST_MAINTENANCE_ENABLE) == 0)
    {
        valBool = prop->fastMaintenanceEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_FAST_MAINTENANCE_PERIOD) == 0)
    {
        valInt = prop->fastMaintenancePer;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_STRICT_GLORT_PHYSICAL) == 0)
    {
        valBool = prop->strictGlotPhysical;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_RESET_WATERMARK_AT_PAUSE_OFF) == 0)
    {
        valBool = prop->resetWmAtPauseOff;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_SWAG_AUTO_SUB_SWITCHES) == 0)
    {
        valBool = prop->swagAutoSubSwitches;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_SWAG_AUTO_INTERNAL_PORTS) == 0)
    {
        valBool = prop->swagAutoIntPorts;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_SWAG_AUTO_VN_VSI) == 0)
    {
        valBool = prop->swagAutoVNVsi;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_BYPASS_ENABLE) == 0)
    {
        valBool = prop->byPassEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_LAG_DELETE_SEMAPHORE_TIMEOUT) == 0)
    {
        valInt = prop->lagDelSemTimeout;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_STP_ENABLE_INTERNAL_PORT_CTRL) == 0)
    {
        valBool = prop->stpEnIntPortCtrl;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_PORT_MAP_TYPE) == 0)
    {
        valInt = prop->modelPortMapType;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_SWITCH_TYPE) == 0)
    {
        valText = prop->modelSwitchType;
        expType = FM_API_ATTR_TEXT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_SEND_EOT) == 0)
    {
        valBool = prop->modelSendEOT;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_LOG_EGRESS_INFO) == 0)
    {
        valBool = prop->modelLogEgressInfo;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_ENABLE_REF_CLOCK) == 0)
    {
        valBool = prop->enableRefClock;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_SET_REF_CLOCK) == 0)
    {
        valBool = prop->setRefClock;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_PRIORITY_BUFFER_QUEUES) == 0)
    {
        valBool = prop->priorityBufQueues;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_PKT_SCHED_TYPE) == 0)
    {
        valInt = prop->pktSchedType;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_SEPARATE_BUFFER_POOL_ENABLE) == 0)
    {
        valBool = prop->separateBufPoolEnable;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_NUM_BUFFERS_RX) == 0)
    {
        valInt = prop->numBuffersRx;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_NUM_BUFFERS_TX) == 0)
    {
        valInt = prop->numBuffersTx;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_PKT_INTERFACE) == 0)
    {
        valText = prop->modelPktInterface;
        expType = FM_API_ATTR_TEXT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_PKT_INTERFACE) == 0)
    {
        valText = prop->pktInterface;
        expType = FM_API_ATTR_TEXT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_TOPOLOGY_NAME) == 0)
    {
        valText = prop->modelTopologyName;
        expType = FM_API_ATTR_TEXT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_TOPOLOGY_USE_MODEL_PATH) == 0)
    {
        valBool = prop->modelUseModelPath;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_DEV_BOARD_IP) == 0)
    {
        valText = prop->modelDevBoardIp;
        expType = FM_API_ATTR_TEXT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_DEV_BOARD_PORT) == 0)
    {
        valInt = prop->modelDevBoardPort;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_DEVICE_CFG) == 0)
    {
        valInt = prop->modelDeviceCfg;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_MODEL_CHIP_VERSION) == 0)
    {
        valInt = prop->modelChipVersion;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_SBUS_SERVER_PORT) == 0)
    {
        valInt = prop->sbusServerPort;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_PLATFORM_IS_WHITE_MODEL) == 0)
    {
        valBool = prop->isWhiteModel;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_DEBUG_INIT_LOGGING_CAT) == 0)
    {
        valText = prop->initLoggingCat;
        expType = FM_API_ATTR_TEXT;
    }
    else if (strcmp(key, FM_AAK_API_PORT_ADD_PEPS_TO_FLOODING) == 0)
    {
        valBool = prop->addPepsToFlooding;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PORT_ALLOW_FTAG_VLAN_TAGGING) == 0)
    {
        valBool = prop->allowFtagVlanTagging;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_SCH_IGNORE_BW_VIOLATION) == 0)
    {
        valBool = (prop->ignoreBwViolation == 1);
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_SCH_IGNORE_BW_VIOLATION_NO_WARNING) == 0)
    {
        valBool = (prop->ignoreBwViolation == 2);
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_DFE_ALLOW_EARLY_LINK_UP_MODE) == 0)
    {
        valBool = prop->dfeAllowEarlyLinkUp;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_DFE_ALLOW_KR_PCAL_MODE) == 0)
    {
        valBool = prop->dfeAllowKrPcal;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_DFE_ENABLE_SIGNALOK_DEBOUNCING) == 0)
    {
        valBool = prop->dfeEnableSigOkDebounce;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_PORT_ENABLE_STATUS_POLLING) == 0)
    {
        valBool = prop->enableStatusPolling;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_GSME_TIMESTAMP_MODE) == 0)
    {
        valBool = prop->gsmeTimestampMode;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_MULTICAST_HNI_FLOODING) == 0)
    {
        valBool = prop->hniMcastFlooding;
        expType = FM_API_ATTR_BOOL;
    }
    else if (strcmp(key, FM_AAK_API_HNI_MAC_ENTRIES_PER_PEP) == 0)
    {
        valInt = prop->hniMacEntriesPerPep;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_HNI_MAC_ENTRIES_PER_PORT) == 0)
    {
        valInt = prop->hniMacEntriesPerPort;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_HNI_INN_OUT_ENTRIES_PER_PEP) == 0)
    {
        valInt = prop->hniInnOutEntriesPerPep;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_HNI_INN_OUT_ENTRIES_PER_PORT) == 0)
    {
        valInt = prop->hniInnOutEntriesPerPort;
        expType = FM_API_ATTR_INT;
    }
    else if (strcmp(key, FM_AAK_API_AN_INHBT_TIMER_ALLOW_OUT_OF_SPEC) == 0)
    {
        valBool = prop->anTimerAllowOutSpec;
        expType = FM_API_ATTR_BOOL;
    }


#if defined(FM_SUPPORT_FM10000)

    if (expType == FM_API_ATTR_MAX && 
        strncmp(key, "api.FM10000.", 12) == 0)
    {
        fm10kProp = GET_FM10000_PROPERTY();

        if (strcmp(key, FM_AAK_API_FM10000_WMSELECT) == 0)
        {
            valText = fm10kProp->wmSelect;
            expType = FM_API_ATTR_TEXT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_RX_SMP_PRIVATE_BYTES) == 0)
        {
            valInt = fm10kProp->cmRxSmpPrivBytes;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_TX_TC_HOG_BYTES) == 0)
        {
            valInt = fm10kProp->cmTxTcHogBytes;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_SMP_SD_VS_HOG_PERCENT) == 0)
        {
            valInt = fm10kProp->cmSmpSdVsHogPercent;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_SMP_SD_JITTER_BITS) == 0)
        {
            valBool = fm10kProp->cmSmpSdJitterBits;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_TX_SD_ON_PRIVATE) == 0)
        {
            valBool = fm10kProp->cmTxSdOnPrivate;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_TX_SD_ON_SMP_FREE) == 0)
        {
            valBool = fm10kProp->cmTxSdOnSmpFree;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CM_PAUSE_BUFFER_BYTES) == 0)
        {
            valInt = fm10kProp->cmPauseBufferBytes;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM) == 0)
        {
            valInt = fm10kProp->mcastMaxEntriesPerCam;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_UNICAST_SLICE_1ST) == 0)
        {
            valInt = fm10kProp->ffuUcastSliceRangeFirst;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_UNICAST_SLICE_LAST) == 0)
        {
            valInt = fm10kProp->ffuUcastSliceRangeLast;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_1ST) == 0)
        {
            valInt = fm10kProp->ffuMcastSliceRangeFirst;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_LAST) == 0)
        {
            valInt = fm10kProp->ffuMcastSliceRangeLast;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_ACL_SLICE_1ST) == 0)
        {
            valInt = fm10kProp->ffuAclSliceRangeFirst;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_ACL_SLICE_LAST) == 0)
        {
            valInt = fm10kProp->ffuAclSliceRangeLast;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_MAPMAC_ROUTING) == 0)
        {
            valInt = fm10kProp->ffuMapMacResvdForRoute;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN) == 0)
        {
            valInt = fm10kProp->ffuUcastPrecedenceMin;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_UNICAST_PRECEDENCE_MAX) == 0)
        {
            valInt = fm10kProp->ffuUcastPrecedenceMax;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN) == 0)
        {
            valInt = fm10kProp->ffuMcastPrecedenceMin;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_MULTICAST_PRECEDENCE_MAX) == 0)
        {
            valInt = fm10kProp->ffuMcastPrecedenceMax;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_ACL_PRECEDENCE_MIN) == 0)
        {
            valInt = fm10kProp->ffuAclPrecedenceMin;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_ACL_PRECEDENCE_MAX) == 0)
        {
            valInt = fm10kProp->ffuAclPrecedenceMax;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FFU_ACL_STRICT_COUNT_POLICE) == 0)
        {
            valBool = fm10kProp->ffuAclStrictCountPolice;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INIT_UCAST_FLOODING_TRIGGERS) == 0)
        {
            valBool = fm10kProp->initUcastFloodTriggers;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INIT_MCAST_FLOODING_TRIGGERS) == 0)
        {
            valBool = fm10kProp->initMcastFloodTriggers;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INIT_BCAST_FLOODING_TRIGGERS) == 0)
        {
            valBool = fm10kProp->initBcastFloodTriggers;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INIT_RESERVED_MAC_TRIGGERS) == 0)
        {
            valBool = fm10kProp->initResvdMacTriggers;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_FLOODING_TRAP_PRIORITY) == 0)
        {
            valInt = fm10kProp->floodingTrapPriority;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_AUTONEG_GENERATE_EVENTS) == 0)
        {
            valBool = fm10kProp->autonegGenerateEvents;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_LINK_DEPENDS_ON_DFE) == 0)
        {
            valBool = fm10kProp->linkDependsOfDfe;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_USE_SHARED_ENCAP_FLOWS) == 0)
        {
            valBool = fm10kProp->vnUseSharedEncapFlows;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_MAX_TUNNEL_RULES) == 0)
        {
            valInt = fm10kProp->vnMaxRemoteAddress;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_TUNNEL_GROUP_HASH_SIZE) == 0)
        {
            valInt = fm10kProp->vnTunnelGroupHashSize;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_TE_VID) == 0)
        {
            valInt = fm10kProp->vnTeVid;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_ENCAP_ACL_NUM) == 0)
        {
            valInt = fm10kProp->vnEncapAclNumber;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_DECAP_ACL_NUM) == 0)
        {
            valInt = fm10kProp->vnDecapAclNumber;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_MCAST_NUM_STACK_GROUPS) == 0)
        {
            valInt = fm10kProp->mcastNumStackGroups;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_VN_TUNNEL_ONLY_IN_INGRESS) == 0)
        {
            valBool = fm10kProp->vnTunnelOnlyOnIngress;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_MTABLE_CLEANUP_WATERMARK) == 0)
        {
            valInt = fm10kProp->mtableCleanupWm;
            expType = FM_API_ATTR_INT;
        }
       else if (strcmp(key, FM_AAK_API_FM10000_SCHED_MODE) == 0)
        {
            valText = fm10kProp->schedMode;
            expType = FM_API_ATTR_TEXT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_UPD_SCHED_ON_LNK_CHANGE) == 0)
        {
            valBool = fm10kProp->updateSchedOnLinkChange;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS) == 0)
        {
            valBool = fm10kProp->createRemoteLogicalPorts;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_AUTONEG_CLAUSE_37_TIMEOUT) == 0)
        {
            valInt = fm10kProp->autonegCl37Timeout;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_AUTONEG_SGMII_TIMEOUT) == 0)
        {
            valInt = fm10kProp->autonegSgmiiTimeout;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_HNI_SERVICES_LOOPBACK) == 0)
        {
            valBool = fm10kProp->useHniServicesLoopback;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_ANTI_BUBBLE_WM) == 0)
        {
            valInt = fm10kProp->antiBubbleWm;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_SERDES_OP_MODE) == 0)
        {
            valInt = fm10kProp->serdesOpMode;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_SERDES_DBG_LVL) == 0)
        {
            valInt = fm10kProp->serdesDbgLevel;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_PARITY_INTERRUPTS) == 0)
        {
            valBool = fm10kProp->parityEnableInterrupts;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_START_TCAM_MONITORS) == 0)
        {
            valBool = fm10kProp->parityStartTcamMonitors;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_CRM_TIMEOUT) == 0)
        {
            valBool = fm10kProp->parityCrmTimeout;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_SCHED_OVERSPEED) == 0)
        {
            valInt = fm10kProp->schedOverspeed;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_LINK_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrLinkIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_AUTONEG_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrAutonegIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_SERDES_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrSerdesIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_PCIE_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrPcieIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_MATCN_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrMaTcnIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_FHTAIL_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrFhTailIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_SW_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrSwIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_INTR_TE_IGNORE_MASK) == 0)
        {
            valInt = fm10kProp->intrTeIgnoreMask;
            expType = FM_API_ATTR_INT;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_ENABLE_EEE_SPICO_INTR) == 0)
        {
            valBool = fm10kProp->enableEeeSpicoIntr;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_USE_ALTERNATE_SPICO_FW) == 0)
        {
            valBool = fm10kProp->useAlternateSpicoFw;
            expType = FM_API_ATTR_BOOL;
        }
        else if (strcmp(key, FM_AAK_API_FM10000_ALLOW_KRPCAL_ON_EEE) == 0)
        {
            valBool = fm10kProp->allowKrPcalOnEee;
            expType = FM_API_ATTR_BOOL;
        }
    }
#endif

    err = fmReleaseLock(&fmRootAlos->propertyLock);

    if (expType != FM_API_ATTR_MAX)
    {
        if (attrType != expType)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ATTR,
                         "%s: Got type %d but expected %d\n",
                         key, attrType, expType);
        
            FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ATTR, FM_ERR_INVALID_ARGUMENT);
        }

        switch (attrType)
        {
            case FM_API_ATTR_INT:
                *(fm_int *)value = valInt;
                break;

            case FM_API_ATTR_BOOL:
                *(fm_bool *)value = valBool;
                break;

            case FM_API_ATTR_FLOAT:
                *(fm_float *)value = valFloat;
                break;

            case FM_API_ATTR_TEXT:
                *(fm_text *)value = valText;
                break;

            default:
                FM_LOG_FATAL(FM_LOG_CAT_ATTR,
                             "Unknown type %d for get of property %s\n",
                             attrType, key);

        }   /* end switch (attrEntry->type) */
    }
    else
    {
        FM_LOG_FATAL(FM_LOG_CAT_ATTR,
                     "Property %s not found\n",
                     key);
        err = FM_ERR_INVALID_ARGUMENT;
    }

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_ATTR, err);

}   /* end fmGetApiProperty */




/*****************************************************************************/
/** fmSetApiAttribute
 * \ingroup api
 *
 * \desc            Adds an API property to the database.
 * 
 * \note            Deprecated. Use ''fmSetApiProperty'' instead. This
 *                  function is provided for compatibility with legacy code.
 *
 * \param[in]       key is the dotted string key (see ''API Properties'').
 *
 * \param[in]       attrType is one of the valid type constants.
 *
 * \param[in]       value points to caller supplied storage where the specific
 *                  value is stored. Its type is assumed to match attrType.
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fmSetApiAttribute(fm_text        key,
                            fm_apiAttrType attrType,
                            void *         value)
{

    return fmSetApiProperty(key, attrType, value);

}   /* end fmSetApiAttribute */




/*****************************************************************************/
/** fmGetApiAttribute
 * \ingroup api
 *
 * \desc            Retrieves the value of an API property from the database.
 * 
 * \note            Deprecated. Use ''fmGetApiProperty'' instead. This
 *                  function is provided for compatibility with legacy code.
 *
 * \param[in]       key is the dotted string key (see ''API Properties'').
 *
 * \param[in]       attrType is the expected type of the key. See
 *                  ''fm_apiAttrType''.
 *
 * \param[in]       value points to caller allocated storage where the key's
 *                  value will be stored.
 *
 * \return          FM_OK on success.
 * \return          FM_ERR_KEY_NOT_FOUND if the key is invalid.
 *
 *****************************************************************************/
fm_status fmGetApiAttribute(fm_text        key,
                            fm_apiAttrType attrType,
                            void *         value)
{

    return fmGetApiProperty(key, attrType, value);

}   /* end fmGetApiAttribute */




/*****************************************************************************/
/** fmGetIntApiProperty
 * \ingroup intApi
 *
 * \desc            Retrieves the value of the given integer key from the
 *                  database. Returns an error if the key has a different
 *                  type.
 *
 * \param[in]       key is the dotted string key.
 *
 * \param[in]       defaultValue is the value to return on an error.
 *
 * \return          The integer value (or the default if an error occurred).
 *
 *****************************************************************************/
fm_int fmGetIntApiProperty(fm_text key, fm_int defaultValue)
{
    fm_status err;
    fm_int    value;

    err = fmGetApiProperty(key, FM_API_ATTR_INT, &value);

    if (err != FM_OK)
    {
        return defaultValue;
    }

    return value;

}   /* end fmGetIntApiProperty */




/*****************************************************************************/
/** fmGetBoolApiProperty
 * \ingroup intApi
 *
 * \desc            Retrieves the value of the given Boolean key from the
 *                  database. Returns an error if the key has a different
 *                  type.
 *
 * \param[in]       key is the dotted string key.
 *
 * \param[in]       defaultValue is the value to return on an error.
 *
 * \return          The Boolean value (or the default if an error occurred).
 *
 *****************************************************************************/
fm_bool fmGetBoolApiProperty(fm_text key, fm_bool defaultValue)
{
    fm_status err;
    fm_bool   value;

    err = fmGetApiProperty(key, FM_API_ATTR_BOOL, &value);

    if (err != FM_OK)
    {
        return defaultValue;
    }

    return value;

}   /* end fmGetBoolApiProperty */




/*****************************************************************************/
/** fmGetFloatApiProperty
 * \ingroup intApi
 *
 * \desc            Retrieves the value of the given float key from the
 *                  database. Returns an error if the key has a different
 *                  type.
 *
 * \param[in]       key is the dotted string key.
 *
 * \param[in]       defaultValue is the value to return on an error.
 *
 * \return          The float value (or the default if an error occurred).
 *
 *****************************************************************************/
fm_float fmGetFloatApiProperty(fm_text key, fm_float defaultValue)
{
    fm_status err;
    fm_float  value;

    err = fmGetApiProperty(key, FM_API_ATTR_FLOAT, &value);

    if (err != FM_OK)
    {
        return defaultValue;
    }

    return value;

}   /* end fmGetFloatApiProperty */




/*****************************************************************************/
/** fmGetTextApiProperty
 * \ingroup intApi
 *
 * \desc            Retrieves the value of the given text key from the database.
 *                  Returns an error if the key has a different type.
 *
 * \param[in]       key is the dotted string key.
 *
 * \param[in]       defaultValue is the value to return on an error.
 *
 * \return          The text value (or the default if an error occurred).
 *
 *****************************************************************************/
fm_text fmGetTextApiProperty(fm_text key, fm_text defaultValue)
{
    fm_status err;
    fm_text   value;

    err = fmGetApiProperty(key, FM_API_ATTR_TEXT, &value);

    if (err != FM_OK)
    {
        return defaultValue;
    }

    return value;

}   /* end fmGetTextApiProperty */




/*****************************************************************************/
/** fmDbgDumpApiProperties
 * \ingroup diagMisc
 *
 * \desc            Dump all API properties.
 *
 * \param           None.
 *
 * \return          None.
 *
 *****************************************************************************/
void fmDbgDumpApiProperties(void)
{
    fm_property *prop;
#if defined(FM_SUPPORT_FM10000)
    fm10000_property *fm10kProp;
#endif

    prop = GET_PROPERTY();

    FM_LOG_PRINT("##########################################################\n");
    FM_LOG_PRINT(_FORMAT_I, "isWhiteModel",
        prop->isWhiteModel);

    FM_LOG_PRINT(_FORMAT_I, "defStateVlanMember", prop->defStateVlanMember);
    FM_LOG_PRINT(_FORMAT_I, "directSendToCpu", prop->directSendToCpu);
    FM_LOG_PRINT(_FORMAT_I, "defStateVlanNonMember", prop->defStateVlanNonMember);
    FM_LOG_PRINT(_FORMAT_I, "bootIdentifySw", prop->bootIdentifySw);
    FM_LOG_PRINT(_FORMAT_I, "bootReset", prop->bootReset);
    FM_LOG_PRINT(_FORMAT_I, "autoInsertSwitches", prop->autoInsertSwitches);
    FM_LOG_PRINT(_FORMAT_I, "deviceResetTime", prop->deviceResetTime);
    FM_LOG_PRINT(_FORMAT_I, "swagAutoEnableLinks", prop->swagAutoEnableLinks);
    FM_LOG_PRINT(_FORMAT_I, "eventBlockThreshold", prop->eventBlockThreshold);
    FM_LOG_PRINT(_FORMAT_I, "eventUnblockThreshold", prop->eventUnblockThreshold);
    FM_LOG_PRINT(_FORMAT_I, "eventSemTimeout", prop->eventSemTimeout);
    FM_LOG_PRINT(_FORMAT_I, "rxDirectEnqueueing", prop->rxDirectEnqueueing);
    FM_LOG_PRINT(_FORMAT_I, "rxDriverDestinations", prop->rxDriverDestinations);
    FM_LOG_PRINT(_FORMAT_I, "lagAsyncDeletion", prop->lagAsyncDeletion);
    FM_LOG_PRINT(_FORMAT_I, "maEventOnStaticAddr", prop->maEventOnStaticAddr);
    FM_LOG_PRINT(_FORMAT_I, "maEventOnDynAddr", prop->maEventOnDynAddr);
    FM_LOG_PRINT(_FORMAT_I, "maEventOnAddrChange", prop->maEventOnAddrChange);
    FM_LOG_PRINT(_FORMAT_I, "maFlushOnPortDown", prop->maFlushOnPortDown);
    FM_LOG_PRINT(_FORMAT_I, "maFlushOnVlanChange", prop->maFlushOnVlanChange);
    FM_LOG_PRINT(_FORMAT_I, "maFlushOnLagChange", prop->maFlushOnLagChange);
    FM_LOG_PRINT(_FORMAT_I, "maTcnFifoBurstSize", prop->maTcnFifoBurstSize);
    FM_LOG_PRINT(_FORMAT_I, "swagIntVlanStats", prop->swagIntVlanStats);
    FM_LOG_PRINT(_FORMAT_I, "perLagManagement", prop->perLagManagement);
    FM_LOG_PRINT(_FORMAT_I, "parityRepairEnable", prop->parityRepairEnable);
    FM_LOG_PRINT(_FORMAT_I, "swagMaxAclPortSets", prop->swagMaxAclPortSets);
    FM_LOG_PRINT(_FORMAT_I, "maxPortSets", prop->maxPortSets);
    FM_LOG_PRINT(_FORMAT_I, "packetReceiveEnable", prop->packetReceiveEnable);
    FM_LOG_PRINT(_FORMAT_I, "multicastSingleAddress", prop->multicastSingleAddress);
    FM_LOG_PRINT(_FORMAT_I, "modelPosition", prop->modelPosition);
    FM_LOG_PRINT(_FORMAT_I, "vnNumNextHops", prop->vnNumNextHops);
    FM_LOG_PRINT(_FORMAT_I, "vnEncapProtocol", prop->vnEncapProtocol);
    FM_LOG_PRINT(_FORMAT_I, "vnEncapVersion", prop->vnEncapVersion);
    FM_LOG_PRINT(_FORMAT_I, "supportRouteLookups", prop->supportRouteLookups);
    FM_LOG_PRINT(_FORMAT_I, "routeMaintenanceEnable", prop->routeMaintenanceEnable);
    FM_LOG_PRINT(_FORMAT_I, "autoVlan2Tagging", prop->autoVlan2Tagging);
  
    FM_LOG_PRINT(_FORMAT_I, "interruptHandlerDisable", prop->interruptHandlerDisable);
    FM_LOG_PRINT(_FORMAT_I, "maTableMaintenanceEnable", prop->maTableMaintenanceEnable);
    FM_LOG_PRINT(_FORMAT_I, "fastMaintenanceEnable", prop->fastMaintenanceEnable);
    FM_LOG_PRINT(_FORMAT_I, "fastMaintenancePer", prop->fastMaintenancePer);
    FM_LOG_PRINT(_FORMAT_I, "strictGlotPhysical", prop->strictGlotPhysical);
    FM_LOG_PRINT(_FORMAT_I, "resetWmAtPauseOff", prop->resetWmAtPauseOff);
    FM_LOG_PRINT(_FORMAT_I, "swagAutoSubSwitches", prop->swagAutoSubSwitches);
    FM_LOG_PRINT(_FORMAT_I, "swagAutoIntPorts", prop->swagAutoIntPorts);
    FM_LOG_PRINT(_FORMAT_I, "swagAutoVNVsi", prop->swagAutoVNVsi);
    FM_LOG_PRINT(_FORMAT_I, "byPassEnable", prop->byPassEnable);
    FM_LOG_PRINT(_FORMAT_I, "lagDelSemTimeout", prop->lagDelSemTimeout);
    FM_LOG_PRINT(_FORMAT_I, "stpEnIntPortCtrl", prop->stpEnIntPortCtrl);
    FM_LOG_PRINT(_FORMAT_I, "modelPortMapType", prop->modelPortMapType);
    FM_LOG_PRINT(_FORMAT_T, "modelSwitchType", prop->modelSwitchType);
    FM_LOG_PRINT(_FORMAT_I, "modelSendEOT", prop->modelSendEOT);
    FM_LOG_PRINT(_FORMAT_I, "modelLogEgressInfo", prop->modelLogEgressInfo);
    FM_LOG_PRINT(_FORMAT_I, "enableRefClock", prop->enableRefClock);
    FM_LOG_PRINT(_FORMAT_I, "setRefClock", prop->setRefClock);
    FM_LOG_PRINT(_FORMAT_I, "priorityBufQueues", prop->priorityBufQueues);
    FM_LOG_PRINT(_FORMAT_I, "pktSchedType", prop->pktSchedType);
    FM_LOG_PRINT(_FORMAT_I, "separateBufPoolEnable", prop->separateBufPoolEnable);
    FM_LOG_PRINT(_FORMAT_I, "numBuffersRx", prop->numBuffersRx);
    FM_LOG_PRINT(_FORMAT_I, "numBuffersTx", prop->numBuffersTx);
    FM_LOG_PRINT(_FORMAT_T, "modelPktInterface", prop->modelPktInterface);
    FM_LOG_PRINT(_FORMAT_T, "pktInterface", prop->pktInterface);
    FM_LOG_PRINT(_FORMAT_T, "modelTopologyName", prop->modelTopologyName);
    FM_LOG_PRINT(_FORMAT_I, "modelUseModelPath", prop->modelUseModelPath);
    FM_LOG_PRINT(_FORMAT_T, "modelDevBoardIp", prop->modelDevBoardIp);
    FM_LOG_PRINT(_FORMAT_I, "modelDevBoardPort", prop->modelDevBoardPort);
    FM_LOG_PRINT(_FORMAT_I, "modelDeviceCfg", prop->modelDeviceCfg);
    FM_LOG_PRINT(_FORMAT_I, "modelChipVersion", prop->modelChipVersion);
    FM_LOG_PRINT(_FORMAT_I, "sbusServerPort", prop->sbusServerPort);
    FM_LOG_PRINT(_FORMAT_I, "isWhiteModel", prop->isWhiteModel);
    FM_LOG_PRINT(_FORMAT_T, "initLoggingCat", prop->initLoggingCat);
    FM_LOG_PRINT(_FORMAT_I, "addPepsToFlooding", prop->addPepsToFlooding);
    FM_LOG_PRINT(_FORMAT_I, "allowFtagVlanTagging", prop->allowFtagVlanTagging);
    FM_LOG_PRINT(_FORMAT_I, "ignoreBwViolation", prop->ignoreBwViolation);
    FM_LOG_PRINT(_FORMAT_I, "dfeAllowEarlyLinkUp", prop->dfeAllowEarlyLinkUp);
    FM_LOG_PRINT(_FORMAT_I, "dfeAllowKrPcal", prop->dfeAllowKrPcal);
    FM_LOG_PRINT(_FORMAT_I, "dfeEnableSigOkDebounce", prop->dfeEnableSigOkDebounce);
    FM_LOG_PRINT(_FORMAT_I, "enableStatusPolling", prop->enableStatusPolling);
    FM_LOG_PRINT(_FORMAT_I, "gsmeTimestampMode", prop->gsmeTimestampMode);
    FM_LOG_PRINT(_FORMAT_I, "hniMcastFlooding", prop->hniMcastFlooding);
    FM_LOG_PRINT(_FORMAT_I, "hniMacEntriesPerPep", prop->hniMacEntriesPerPep);
    FM_LOG_PRINT(_FORMAT_I, "hniMacEntriesPerPort", prop->hniMacEntriesPerPort);
    FM_LOG_PRINT(_FORMAT_I, "hniInnOutEntriesPerPep", prop->hniInnOutEntriesPerPep);
    FM_LOG_PRINT(_FORMAT_I, "anTimerAllowOutSpec", prop->hniInnOutEntriesPerPort);
    FM_LOG_PRINT(_FORMAT_I, "anTimerAllowOutSpec", prop->anTimerAllowOutSpec);


#if defined(FM_SUPPORT_FM10000)
    FM_LOG_PRINT("##########################################################\n");

    fm10kProp = GET_FM10000_PROPERTY();
    FM_LOG_PRINT(_FORMAT_T, "wmSelect", fm10kProp->wmSelect);
    FM_LOG_PRINT(_FORMAT_I, "cmRxSmpPrivBytes", fm10kProp->cmRxSmpPrivBytes);
    FM_LOG_PRINT(_FORMAT_I, "cmTxTcHogBytes", fm10kProp->cmTxTcHogBytes);
    FM_LOG_PRINT(_FORMAT_I, "cmSmpSdVsHogPercent", fm10kProp->cmSmpSdVsHogPercent);
    FM_LOG_PRINT(_FORMAT_I, "cmSmpSdJitterBits", fm10kProp->cmSmpSdJitterBits);
    FM_LOG_PRINT(_FORMAT_I, "cmTxSdOnPrivate", fm10kProp->cmTxSdOnPrivate);
    FM_LOG_PRINT(_FORMAT_I, "cmTxSdOnSmpFree", fm10kProp->cmTxSdOnSmpFree);
    FM_LOG_PRINT(_FORMAT_I, "cmPauseBufferBytes", fm10kProp->cmPauseBufferBytes);
    FM_LOG_PRINT(_FORMAT_I, "mcastMaxEntriesPerCam", fm10kProp->mcastMaxEntriesPerCam);
    FM_LOG_PRINT(_FORMAT_I, "ffuUcastSliceRangeFirst", fm10kProp->ffuUcastSliceRangeFirst);
    FM_LOG_PRINT(_FORMAT_I, "ffuUcastSliceRangeLast", fm10kProp->ffuUcastSliceRangeLast);
    FM_LOG_PRINT(_FORMAT_I, "ffuMcastSliceRangeFirst", fm10kProp->ffuMcastSliceRangeFirst);
    FM_LOG_PRINT(_FORMAT_I, "ffuMcastSliceRangeLast", fm10kProp->ffuMcastSliceRangeLast);
    FM_LOG_PRINT(_FORMAT_I, "ffuAclSliceRangeFirst", fm10kProp->ffuAclSliceRangeFirst);
    FM_LOG_PRINT(_FORMAT_I, "ffuAclSliceRangeLast", fm10kProp->ffuAclSliceRangeLast);
    FM_LOG_PRINT(_FORMAT_I, "ffuMapMacResvdForRoute", fm10kProp->ffuMapMacResvdForRoute);
    FM_LOG_PRINT(_FORMAT_I, "ffuUcastPrecedenceMin", fm10kProp->ffuUcastPrecedenceMin);
    FM_LOG_PRINT(_FORMAT_I, "ffuUcastPrecedenceMax", fm10kProp->ffuUcastPrecedenceMax);
    FM_LOG_PRINT(_FORMAT_I, "ffuMcastPrecedenceMin", fm10kProp->ffuMcastPrecedenceMin);
    FM_LOG_PRINT(_FORMAT_I, "ffuMcastPrecedenceMax", fm10kProp->ffuMcastPrecedenceMax);
    FM_LOG_PRINT(_FORMAT_I, "ffuAclPrecedenceMin", fm10kProp->ffuAclPrecedenceMin);
    FM_LOG_PRINT(_FORMAT_I, "ffuAclPrecedenceMax", fm10kProp->ffuAclPrecedenceMax);
    FM_LOG_PRINT(_FORMAT_I, "ffuAclStrictCountPolice", fm10kProp->ffuAclStrictCountPolice);
    FM_LOG_PRINT(_FORMAT_I, "initUcastFloodTriggers", fm10kProp->initUcastFloodTriggers);
    FM_LOG_PRINT(_FORMAT_I, "initMcastFloodTriggers", fm10kProp->initMcastFloodTriggers);
    FM_LOG_PRINT(_FORMAT_I, "initBcastFloodTriggers", fm10kProp->initBcastFloodTriggers);
    FM_LOG_PRINT(_FORMAT_I, "initResvdMacTriggers", fm10kProp->initResvdMacTriggers);
    FM_LOG_PRINT(_FORMAT_I, "floodingTrapPriority", fm10kProp->floodingTrapPriority);
    FM_LOG_PRINT(_FORMAT_I, "autonegGenerateEvents", fm10kProp->autonegGenerateEvents);
    FM_LOG_PRINT(_FORMAT_I, "linkDependsOfDfe", fm10kProp->linkDependsOfDfe);
    FM_LOG_PRINT(_FORMAT_I, "vnUseSharedEncapFlows", fm10kProp->vnUseSharedEncapFlows);
    FM_LOG_PRINT(_FORMAT_I, "vnMaxRemoteAddress", fm10kProp->vnMaxRemoteAddress);
    FM_LOG_PRINT(_FORMAT_I, "vnTunnelGroupHashSize", fm10kProp->vnTunnelGroupHashSize);
    FM_LOG_PRINT(_FORMAT_I, "vnTeVid", fm10kProp->vnTeVid);
    FM_LOG_PRINT(_FORMAT_I, "vnEncapAclNumber", fm10kProp->vnEncapAclNumber);
    FM_LOG_PRINT(_FORMAT_I, "vnDecapAclNumber", fm10kProp->vnDecapAclNumber);
    FM_LOG_PRINT(_FORMAT_I, "mcastNumStackGroups", fm10kProp->mcastNumStackGroups);
    FM_LOG_PRINT(_FORMAT_I, "vnTunnelOnlyOnIngress", fm10kProp->vnTunnelOnlyOnIngress);
    FM_LOG_PRINT(_FORMAT_I, "mtableCleanupWm", fm10kProp->mtableCleanupWm);
    FM_LOG_PRINT(_FORMAT_T, "schedMode", fm10kProp->schedMode);
    FM_LOG_PRINT(_FORMAT_I, "updateSchedOnLinkChange", fm10kProp->updateSchedOnLinkChange);

    FM_LOG_PRINT(_FORMAT_I, "createRemoteLogicalPorts", fm10kProp->createRemoteLogicalPorts);
    FM_LOG_PRINT(_FORMAT_I, "autonegCl37Timeout", fm10kProp->autonegCl37Timeout);
    FM_LOG_PRINT(_FORMAT_I, "autonegSgmiiTimeout", fm10kProp->autonegSgmiiTimeout);
    FM_LOG_PRINT(_FORMAT_I, "useHniServicesLoopback", fm10kProp->useHniServicesLoopback);
    FM_LOG_PRINT(_FORMAT_I, "antiBubbleWm", fm10kProp->antiBubbleWm);
    FM_LOG_PRINT(_FORMAT_I, "serdesOpMode", fm10kProp->serdesOpMode);
    FM_LOG_PRINT(_FORMAT_I, "serdesDbgLevel", fm10kProp->serdesDbgLevel);
    FM_LOG_PRINT(_FORMAT_I, "parityEnableInterrupts", fm10kProp->parityEnableInterrupts);
    FM_LOG_PRINT(_FORMAT_I, "parityStartTcamMonitors", fm10kProp->parityStartTcamMonitors);
    FM_LOG_PRINT(_FORMAT_I, "parityCrmTimeout", fm10kProp->parityCrmTimeout);
    FM_LOG_PRINT(_FORMAT_I, "schedOverspeed", fm10kProp->schedOverspeed);
    FM_LOG_PRINT(_FORMAT_H, "intrLinkIgnoreMask", fm10kProp->intrLinkIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrAutonegIgnoreMask", fm10kProp->intrAutonegIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrSerdesIgnoreMask", fm10kProp->intrSerdesIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrPcieIgnoreMask", fm10kProp->intrPcieIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrMaTcnIgnoreMask", fm10kProp->intrMaTcnIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrFhTailIgnoreMask", fm10kProp->intrFhTailIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrSwIgnoreMask", fm10kProp->intrSwIgnoreMask);
    FM_LOG_PRINT(_FORMAT_H, "intrTeIgnoreMask", fm10kProp->intrTeIgnoreMask);
    FM_LOG_PRINT(_FORMAT_I, "enableEeeSpicoIntr", fm10kProp->enableEeeSpicoIntr);
    FM_LOG_PRINT(_FORMAT_I, "useAlternateSpicoFw", fm10kProp->useAlternateSpicoFw);
    FM_LOG_PRINT(_FORMAT_I, "allowKrPcalOnEee", fm10kProp->allowKrPcalOnEee);

#endif

    FM_LOG_PRINT("##########################################################\n");

}   /* end fmDbgDumpApiProperties */
