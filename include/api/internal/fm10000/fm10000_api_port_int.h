/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_port_int.h
 * Creation Date:   Branched from fm4000_api_port_int.h on April 18, 2013.
 * Description:     Functions dealing with the state of individual FM10xxx
 *                  ports.
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

#ifndef __FM_FM10000_API_PORT_INT_H
#define __FM_FM10000_API_PORT_INT_H

#define FM10000_MAX_LANES_PER_PORT  8

/* Number of ports that can be from EPL ports */
#define FM10000_NUM_EPL_PORTS       FM10000_MAC_CFG_ENTRIES_1 *                \
                                    FM10000_MAC_CFG_ENTRIES_0

#define FM10000_PORT_EYE_SAMPLES    FM10000_SERDES_EYE_SAMPLES

#define GET_FM10000_PORT_ATTR(sw, port)  \
            &((fm10000_port *) GET_PORT_EXT(sw, port))->attributes;

#define FM10000_PORT_ALL_SAF_ENABLED 0xFFFFFFFFFFFF

#define FM10000_PCIE_INVALID_LOGICAL_PORT -1
#define FM10000_PCIE_MAX_FRAME_SIZE       15360
#define FM10000_PCIE_MIN_FRAME_SIZE       15

/* PCS Modes */
typedef enum
{
    /* disabled */
    FM10000_PCS_SEL_DISABLE    = 0,

    /* Clause 73 Auto-negotiation */
    FM10000_PCS_SEL_AN_73      = 1,

    /* SGMII 10M */
    FM10000_PCS_SEL_SGMII_10   = 2,

    /* SGMII 100M */
    FM10000_PCS_SEL_SGMII_100  = 3,

    /* SGMII 1G */
    FM10000_PCS_SEL_SGMII_1000 = 4,

    /* 1000Base-X */
    FM10000_PCS_SEL_1000BASEX  = 5,
                               
    /* 10GBase-R */            
    FM10000_PCS_SEL_10GBASER   = 6,
                               
    /* 40GBase-R */            
    FM10000_PCS_SEL_40GBASER   = 7,
                               
    /* 100GBase-R */           
    FM10000_PCS_SEL_100GBASER  = 8

} fm10000_pcsTypes;


/* QPL modes */
typedef enum
{
    /* no lanes allocated */
    FM10000_QPL_MODE_XX_XX_XX_XX = 0,

    /* one lane allocated to each port */
    FM10000_QPL_MODE_L1_L1_L1_L1 = 1,

    /* Four lanes, single lane uses PORT 3 and Lane 3 */
    FM10000_QPL_MODE_XX_XX_XX_L4 = 2,    
                                         
    /* Four lanes, single lane uses PORT 2 and Lane 2 */
    FM10000_QPL_MODE_XX_XX_L4_XX = 3,

    /* Four lanes, single lane uses PORT 1 and Lane 1 */
    FM10000_QPL_MODE_XX_L4_XX_XX = 4,

    /* Four lanes, single lane uses PORT 0 and Lane 0 */
    FM10000_QPL_MODE_L4_XX_XX_XX = 5

} fm10000_qplModes;


#define FM10000_PORT_SM_HISTORY_SIZE    16
#define FM10000_PORT_SM_RECORD_SIZE     sizeof(int)
#define FM10000_AN_SM_HISTORY_SIZE    16
#define FM10000_AN_SM_RECORD_SIZE     sizeof(int)


#define WHICH_PORT_OF_EPL_GROUP_MASK            0x3
#define EPL_PORT_GROUP_MASK                     (~WHICH_PORT_OF_EPL_GROUP_MASK)

/* Tx Drain Modes */
#define FM10000_PORT_TX_DRAIN_ALWAYS             0
#define FM10000_PORT_TX_DRAIN_ON_LINK_DOWN       1
#define FM10000_PORT_TX_HOLD_ON_LINK_DOWN        2
#define FM10000_PORT_TX_HOLD_ALWAYS              3

/* Drain monitoring interval */
#define FM10000_DRAIN_DELAY                      1


#define FM10000_LINK_INT_MASK \
    ( (1U << FM10000_LINK_IP_b_LinkFaultDebounced) | \
      (1U << FM10000_LINK_IP_b_LpiWakeError) |     \
      (1U << FM10000_LINK_IP_b_LpIdleIndicate) |   \
      (1U << FM10000_LINK_IP_b_EeePcSilent) )

#define FM10000_EEE_PC_SILENT_INT_MASK  (1U << FM10000_LINK_IP_b_EeePcSilent)
#define FM10000_LPI_INDICATE_INT_MASK   (1U << FM10000_LINK_IP_b_LpIdleIndicate)
#define FM10000_LPI_WAKE_ERROR_INT_MASK (1U << FM10000_LINK_IP_b_LpiWakeError)

#define FM10000_PCIE_INT_MASK  (1U << FM10000_PCIE_IP_b_DeviceStateChange)

#define FM10000_TIMESTAMP_INT_MASK   (1U << FM10000_LINK_IP_b_EgressTimeStamp)

/* link debounce definitions */
/*    link up debounce: time scale = 4 (100us); ticks = 30; debounce time = 3ms */
#define FM10000_LINK_FAULT_TIME_SCALE_UP        4
#define FM10000_LINK_FAULT_TICKS_UP             30 
/*    link down debounce: time scale = 4 (100us); ticks = 5; debounce time = 0.5ms */
#define FM10000_LINK_FAULT_TIME_SCALE_DOWN      4
#define FM10000_LINK_FAULT_TICKS_DOWN           5

/* heartbeat time scales */
/* default time scale */
#define FM10000_HEARTBEAT_TIME_SCALE_DEFAULT    0
/* time scale for 10GBASER */
#define FM10000_HEARTBEAT_TIME_SCALE_10GBASER   4

/* pause during ethernet mode reconfiguration to
 * allow completing serdes powerdown tasks: 100ms */
#define FM10000_ETHERNET_MODE_CONFIG_PAUSE      100000000

/* From EAS MAC_CFG.TxFcsMode */
enum 
{
    FM10000_FCS_PASSTHRU = 0,
    FM10000_FCS_PASSTHRU_CHECK,
    FM10000_FCS_REPLACE_GOOD,
    FM10000_FCS_REPLACE_BAD,
    FM10000_FCS_REPLACE_NORMAL,

} fm10000_txFcsMode;

/* Energy-Efficient Ethernet (EEE) */
/*                                 */ 
/*      TickTimeScale 0 -->   0ns  */
/*      (Default)     1 -->  50ns  */
/*                    2 -->   1us  */
/*                    3 -->  10us  */
/*                    4 --> 100us  */
#define FM10000_PORT_EEE_TX_ACT_TIME_SCALE     3
#define FM10000_PORT_EEE_TX_ACT_TOUT_DIV      10
/*                                 */ 
/*      EeeTimeScale  0 -->  50ns  */
/*                    1 -->   1us  */
/*                    2 -->  10us  */
/*                    3 --> 100us  */
#define FM10000_PORT_EEE_TX_LPI_TIME_SCALE     1
#define FM10000_PORT_EEE_TX_LPI_TOUT_DIV       1

/* EEE Debug Mode */
#define FM10000_EEE_DBG_DISABLE        0x00
#define FM10000_EEE_DBG_ENABLE_RX      0x01
#define FM10000_EEE_DBG_ENABLE_TX      0x02
#define FM10000_EEE_DBG_ENABLE_RXTX    0x03
#define FM10000_EEE_DBG_DISABLE_RX     0x10
#define FM10000_EEE_DBG_DISABLE_TX     0x20
#define FM10000_EEE_DBG_DISABLE_RXTX   0x30

/* PCIe recovery flag base offset */
#define PCIE_RECOVERY_FLAG_BASE               0x1

/* Link optimization mode defines */
#define FM10000_LINK_OPTIM_PARAM_DEFAULT    0x10001
#define FM10000_LINK_OPTIM_PARAM_SPEED_A10G 0x3400
#define FM10000_LINK_OPTIM_PARAM_SPEED_B10G 0x8000
#define FM10000_LINK_OPTIM_PARAM_BALAN_A10G 0x4800
#define FM10000_LINK_OPTIM_PARAM_BALAN_B10G 0xA000
#define FM10000_LINK_OPTIM_PARAM_SPEED_A25G 0x10001
#define FM10000_LINK_OPTIM_PARAM_SPEED_B25G 0x10001
#define FM10000_LINK_OPTIM_PARAM_BALAN_A25G 0x10001
#define FM10000_LINK_OPTIM_PARAM_BALAN_B25G 0x10001


/**************************************************
 * Port Attribute Entry table Structure
 *
 * A variable is present for each of those present
 * in the _fm10000_portAttr structure.
 **************************************************/
typedef struct _fm10000_portAttrEntryTable
{
    fm_portAttrEntry autoNegMode;
    fm_portAttrEntry autoNegBasePage;
    fm_portAttrEntry autoNegPartnerBasePage;
    fm_portAttrEntry autoNegNextPages;
    fm_portAttrEntry autoNegPartnerNextPages;
    fm_portAttrEntry autoNeg25GNxtPgOui;
    fm_portAttrEntry bcastFlooding;
    fm_portAttrEntry bcastPruning;
    fm_portAttrEntry enableTxCutThrough;
    fm_portAttrEntry enableRxCutThrough;
    fm_portAttrEntry defCfi;
    fm_portAttrEntry defDscp;
    fm_portAttrEntry defVlanPri;
    fm_portAttrEntry defVlanPri2;
    fm_portAttrEntry defSwpri;
    fm_portAttrEntry defIslUser;
    fm_portAttrEntry defVlan;
    fm_portAttrEntry defVlan2;
    fm_portAttrEntry linkOptimizationMode;
    fm_portAttrEntry rxTermination;
    fm_portAttrEntry dfeMode;
    fm_portAttrEntry txLaneCursor;
    fm_portAttrEntry txLanePreCursor;
    fm_portAttrEntry txLanePostCursor;
    fm_portAttrEntry txLaneKrInitCursor;
    fm_portAttrEntry txLaneKrInitPreCursor;
    fm_portAttrEntry txLaneKrInitPostCursor;
    fm_portAttrEntry txLaneEnableConfigKrInit;
    fm_portAttrEntry txLaneKrInitialPreDec;
    fm_portAttrEntry txLaneKrInitialPostDec;
    fm_portAttrEntry signalTransitionThreshold;
    fm_portAttrEntry krXconfig1;
    fm_portAttrEntry krXconfig2;
    fm_portAttrEntry dot1xState;
    fm_portAttrEntry dropBv;
    fm_portAttrEntry dropTagged;
    fm_portAttrEntry dropUntagged;
    fm_portAttrEntry internal;
    fm_portAttrEntry islTagFormat;
    fm_portAttrEntry learning;
    fm_portAttrEntry serdesLoopback;
    fm_portAttrEntry loopbackSuppression;
    fm_portAttrEntry maskWide;
    fm_portAttrEntry maxFrameSize;
    fm_portAttrEntry mcastFlooding;
    fm_portAttrEntry mcastPruning;
    fm_portAttrEntry minFrameSize;
    fm_portAttrEntry parser;
    fm_portAttrEntry parserFlagOptions;
    fm_portAttrEntry replaceDscp;
    fm_portAttrEntry routable;
    fm_portAttrEntry rxClassPause;
    fm_portAttrEntry txClassPause;
    fm_portAttrEntry rxLanePolarity;
    fm_portAttrEntry rxPause;
    fm_portAttrEntry swpriDscpPref;
    fm_portAttrEntry swpriSource;
    fm_portAttrEntry taggingMode;
    fm_portAttrEntry txFcsMode;
    fm_portAttrEntry txLanePolarity;
    fm_portAttrEntry txPause;
    fm_portAttrEntry txPauseMode;
    fm_portAttrEntry txPauseResendTime;
    fm_portAttrEntry txCfi;
    fm_portAttrEntry txCfi2;
    fm_portAttrEntry txVpri;
    fm_portAttrEntry txVpri2;
    fm_portAttrEntry ucastFlooding;
    fm_portAttrEntry ucastPruning;
    fm_portAttrEntry updateDscp;
    fm_portAttrEntry updateTtl;
    fm_portAttrEntry autoNegLinkInhbTimer;
    fm_portAttrEntry autoNegLinkInhbTimerKx;
    fm_portAttrEntry autoNegIgnoreNonce;    
    fm_portAttrEntry parserVlan1Tag;
    fm_portAttrEntry parserVlan2Tag;
    fm_portAttrEntry mirrorTruncSize;
    fm_portAttrEntry parserFirstCustomTag;
    fm_portAttrEntry parserSecondCustomTag;
    fm_portAttrEntry parseMpls;
    fm_portAttrEntry securityAction;
    fm_portAttrEntry storeMpls;
    fm_portAttrEntry routedFrameUpdateFields;
    fm_portAttrEntry tcnFifoWm;
    fm_portAttrEntry pcieMode;
    fm_portAttrEntry pcieSpeed;
    fm_portAttrEntry pepMode;
    fm_portAttrEntry ethMode;
    fm_portAttrEntry modifyVlan1Tag;
    fm_portAttrEntry modifyVlan2Tag;
    fm_portAttrEntry eyeScore;
    fm_portAttrEntry generateTimestamps;
    fm_portAttrEntry egressTimestampEvents;
    fm_portAttrEntry bistUserPatterLow40;
    fm_portAttrEntry bistUserPatterUpp40;
    fm_portAttrEntry fabricLoopback;
    fm_portAttrEntry speed;
    fm_portAttrEntry ifg;
    fm_portAttrEntry dicEnable;
    fm_portAttrEntry txPadSize;
    fm_portAttrEntry eeeEnable;
    fm_portAttrEntry eeeState;
    fm_portAttrEntry txPcActTimeout;
    fm_portAttrEntry txLpiTimeout;
    fm_portAttrEntry parserVlan2First;
    fm_portAttrEntry modifyVid2First;
    fm_portAttrEntry linkInterruptEnabled;
    fm_portAttrEntry ignoreIfgErrors;
    fm_portAttrEntry fineTuning;
    fm_portAttrEntry coarseTuning;
    fm_portAttrEntry replaceVlanFields;
    fm_portAttrEntry txClkCompensation;
    fm_portAttrEntry smpLosslessPause;
    fm_portAttrEntry autoDetectModule;

} fm10000_portAttrEntryTable;



/**************************************************
 * FM10000-Specific Port Attributes Structure
 *
 * A variable is present for each FM_PORT_XXX
 * attribute defined in _fm_portAttr that applies
 * to the FM10000 only.
 **************************************************/
typedef struct _fm10000_portAttr
{
    fm_uint32          autoNegLinkInhbTimer;
    fm_uint32          autoNegLinkInhbTimerKx;
    fm_bool            autoNegIgnoreNonce;
    fm_int             parserVlan1Tag;
    fm_int             parserVlan2Tag;
    fm_uint32          mirrorTruncSize;
    fm_uint32          parserFirstCustomTag;
    fm_uint32          parserSecondCustomTag;
    fm_bool            parseMpls;
    fm_int             storeMpls;
    fm_uint32          routedFrameUpdateFields;
    fm_uint32          tcnFifoWm;
    fm_uint32          txPadSize;
    fm_int             fabricLoopback;
    fm_bool            eeeEnable;
    fm_int             eeeState;
    fm_bool            eeeNextPageAdded;
    fm_bool            negotiatedEeeModeEnabled;
    fm_int             dbgEeeMode;
    fm_uint64          lpiRxIndicateCnt;
    fm_uint64          lpiWakeErrorCnt;
    fm_uint64          eeePcSilentCnt;
    fm_uint64          eeePcActiveCnt;
    fm_uint64          eeePcSilentDisabledCnt;
    fm_uint32          txFcsMode;
    fm_uint32          txPcActTimeout;
    fm_uint32          txPcActTimescale;
    fm_uint32          txLpiTimeout;
    fm_uint32          txLpiTimescale;
    fm_uint32          securityAction;
    fm_pcieMode        pcieMode;
    fm_pepMode         pepMode;
    fm_ethMode         ethMode;
    fm_linkOptMode     linkOptimizationMode;
    fm_int             modifyVlan1Tag;
    fm_int             modifyVlan2Tag;
    fm_int             eyeScore;
    fm_bool            generateTimestamps;
    fm_bool            egressTimestampEvents;
    fm_portTaggingMode taggingMode;
    fm_uint64          bistUserPatterLow40;
    fm_uint64          bistUserPatterUpp40;
    fm_bool            parserVlan2First;
    fm_bool            modifyVid2First;
    fm_bool            replaceVlanFields;

} fm10000_portAttr;

/**************************************************
 * Structure used to save port attributes when
 * the port is added to a LAG.
 **************************************************/
typedef struct _fm10000_origAttr
{
    fm_portAttr     genAttr;    /* Generic port attributes structure   */
    fm10000_portAttr extAttr;   /* Port extension attributes structure */
    fm_bool         attrSaved;  /* Indicate whether the attributes are saved */

} fm10000_origAttr;

/**************************************************
 * Endpoint ID union (EPL/PEP)
 **************************************************/
typedef union
{
    fm_int epl;
    fm_int pep;

} fm10000_endpoint;


typedef enum
{
    FM10000_SERDES_RING_NONE = 0,    /* None, must be first */
    FM10000_SERDES_RING_PCIE,        /* PCIE Serdes ring */
    FM10000_SERDES_RING_EPL,         /* EPL Serdes ring*/
} fm_serdesRing;



typedef struct _fm10000_port              fm10000_port;       
typedef struct _fm10000_portSmEventInfo   fm10000_portSmEventInfo;

/* port event info */
struct _fm10000_portSmEventInfo
{
    /**************************************************
     * Generic pieces of info
     **************************************************/
    
    fm_switch        *switchPtr;     /* pointer to the switch structure */
    fm_port          *portPtr;       /* pointer to the port structure */          
    fm10000_port     *portExt;       /* pointer to the extension port structure */
    fm_portAttr      *portAttr;      /* pointer to the attribute structure */          
    fm10000_portAttr *portAttrExt;   /* pointer to the attribute extension structure */

    /**************************************************
     * Event-specific pieces of info
     **************************************************/

    union
    {
        /* used for any lane-oriented _IND event from the serdes */
        fm_int     physLane;

        /* info structure for the CONFIGURE_DFE_REQ event */
        struct
        {
            fm_dfeMode mode;
            fm_int     lane;

        } dfe;

        /* info structure for the CONFIGURE_REQ event */
        struct
        {
            fm_pepMode pepMode;
            fm_ethMode ethMode;
            fm_uint32  speed;

        } config;

        /* info structure for any event that sets an admin mode */
        struct
        {
            fm_int mode;
            fm_int submode;

        } admin;  

        /* info structure for AN_CONFIG_REQ and AN_DISABLE_REQ events */
        struct
        {
            fm_uint32      autoNegMode;
            fm_uint32      autoNegBasePage;
            fm_anNextPages autoNegNextPages;

        } anConfig;

    } info;

    /***************************************************
     * Pieces of info that may be updated by actions
     ***************************************************/

    fm_bool regLockTaken;
    fm_int  hcd;
    
};


/**************************************************
 * FM10000-Specific Port Structure Extension
 **************************************************/

struct _fm10000_port
{
    /**************************************************
     * Generic Port Structure Pointer
     **************************************************/

    /*
     * pointer to the generic port structure,
     * kept here for cross-referencing purposes
     */
    fm_port *        base;

    /* native fabric port associated to this logical port */
    fm_int           fabricPort;

    /* EPL or PEP this port belongs to */
    fm10000_endpoint endpoint;

    /* ring this port belongs to (EPL vs PCIE) */
    fm_serdesRing    ring;

    /**************************************************
     * Current Configuration
     **************************************************/

    /* Port speed */
    fm_uint32        speed;

    /* negotiated ethMode (same as ethMode in fm10000_portAttr unless
       Auto-negotiation is configured and completed successfully */
    fm_ethMode       ethMode;

    /* indicates whether drain mode is enabled */
    fm_bool          isDraining;

    /* Auto-negotiation related attributes */
    fm_uint64       basePage;         /* Clause 37(32-bit) and 73 */
    fm_timestamp    anTimeStamp;
    fm_uint         anRestartCnt;

    /* Caches the basePage and nextPage before updating portAttr. 
       The values are either Pending To Be Updated or 
       Same as in portAttr. */
    fm_uint64       pendingBasePage;
    fm_anNextPages  pendingNextPages; 

    /* Bit array indicating the status of the transceiver module attached
     * to the port, if any. The transceiver status comprends the following
     * signals: module present, receiver Lost of Signal, and transmitter
     * fault. See the ''Transceiver Signals'' for bit definitions. */
    fm_uint         portModStatus[FM10000_MAX_LANES_PER_PORT];

    /* Indicates that port attribute can be set on the port. It is set to TRUE
     * when an attribute is set on a LAG just before iterating through
     * all member ports */
    fm_bool               allowCfg;

    /* Internal cardinal port mask used for masking out LAG member ports */
    fm_portmask           internalPortMask;

    /* link interrupt mask (Ethernet Ports only) */
    fm_uint32             linkInterruptMask;

    /* AN interrupt mask (Ethernet Ports only) */
    fm_uint32             anInterruptMask;

    /* link interrupt mask (PCIE Ports only) */
    fm_uint32             pcieInterruptMask;

    /* Structure used to save the original port attributes when the
     * port is added to a LAG */
    fm10000_origAttr      originalAttr;

    /* chip-specific port attributes defined in _fm_portAttr */
    fm10000_portAttr      attributes;

    /***************************************************
     * For multicast group ports.
     **************************************************/

    /* caches the multicast group type */
    fm_mtableGroupType groupType;

    /* indicates whether or not the mtable group is enabled */
    fm_bool            groupEnabled;

    /* indicates the VLAN ID for the group */
    fm_uint16          groupVlanID;

    /* mask of lanes being used */
    fm_uint32             desiredLaneMask;

    /* mask of lanes having gained PLL lock */
    fm_uint32             lockedLaneMask;

    /* port state machine type */
    fm_int                smType;

    /* Transition History Size for the port state machine */
    fm_int                transitionHistorySize;

    /* port state machine handle */
    fm_smHandle           smHandle;

    /* port state machine handle */
    fm_smHandle           anSmHandle;

    /* port state machine type */
    fm_int                anSmType;

    /* Transition History Size for the AN state machine */
    fm_int                anTransitionHistorySize;

    /* timer associated to this lane serdes */
    fm_timerHandle        timerHandle;

    /* timer associated to the pcie port interrupt stuck recovery */
    fm_timerHandle        pcieIntrTimerHandle;

    /* native lane for Ethernet ports */
    struct _fm10000_lane *nativeLaneExt;

    /* List of lanes currently associated to this port */
    FM_DLL_DEFINE_LIST( _fm10000_lane, firstLane, lastLane );

    /* port state machine event info */
    fm10000_portSmEventInfo eventInfo;

    /* Types of egress timestamps that a logical port belonging to a PEP can 
     * receive. */
    fm_portTxTimestampMode  txTimestampMode;

};

fm_status fm10000SetPortAttribute(fm_int sw,
                                  fm_int port,
                                  fm_int mac,
                                  fm_int lane,
                                  fm_int attr,
                                  void * value);
fm_status fm10000GetPortAttribute(fm_int sw,
                                  fm_int port,
                                  fm_int mac,
                                  fm_int lane,
                                  fm_int attr,
                                  void * value);


fm_status fm10000GetPortLowLevelState(fm_int  sw,
                                      fm_int  port,
                                      fm_int *portState);


fm_status fm10000SetPortState(fm_int sw,
                              fm_int port,
                              fm_int mac,
                              fm_int mode,
                              fm_int subMode);


fm_status fm10000GetPortState(fm_int  sw,
                              fm_int  port,
                              fm_int  mac,
                              fm_int  numBuffers,
                              fm_int *numLanes,
                              fm_int *mode,
                              fm_int *state,
                              fm_int *info);

fm_status fm10000GetCpuPort(fm_int sw, fm_int *cpuPort);
fm_status fm10000SetCpuPort(fm_int sw, fm_int cpuPort);

fm_status fm10000MapPhysicalPortToEplChannel(fm_int  sw,
                                             fm_int  physPort,
                                             fm_int *epl,
                                             fm_int *channel);
fm_status fm10000MapEplChannelToPhysicalPort(fm_int  sw,
                                             fm_int  epl,
                                             fm_int  channel,
                                             fm_int *physPort);

fm_status fm10000GetNumEthLanes(fm_ethMode ethMode, fm_int *numLanes);

fm_status fm10000GetNumPepLanes(fm_pepMode pepMode, fm_int *numLanes);

fm_status fm10000GetNumLanes(fm_int sw, fm_int port, fm_int *numLanes);

fm_status fm10000GetNumPortLanes( fm_int sw, 
                                  fm_int port, 
                                  fm_int mac, 
                                  fm_int *numLanes );

fm_status fm10000UpdatePortMask(fm_int sw, fm_int port);

fm_status fm10000UpdateLoopbackSuppress(fm_int sw, fm_int port);

fm_status fm10000ApplyLagMemberPortAttr(fm_int sw, fm_int port, fm_int lagIndex);

fm_status fm10000RestoreLagMemberPortAttr(fm_int sw, fm_int port);

fm_status fm10000SetPortAttributeOnLAGs(fm_int sw, fm_int attr, void * value);

fm_bool   fm10000IsPerLagPortAttribute(fm_int sw, fm_uint attr);

void fm10000DbgDumpPortAttributes(fm_int sw, fm_int port);

fm_status fm10000IsPortBistActive(fm_int   sw,
                                  fm_int   port,
                                  fm_bool *isBistActive);
fm_status fm10000DrainPhysPort( fm_int  sw,
                                fm_int  physPort,
                                fm_int  numPorts,
                                fm_bool drain );

fm_status fm10000SetPortModuleState( fm_int     sw, 
                                     fm_int     port, 
                                     fm_int     lane, 
                                     fm_uint32  xcvrSignals );
fm_status fm10000DbgDumpPortStateTransitionsV2( fm_int  sw,
                                                fm_int  *portList,
                                                fm_int  portCnt,
                                                fm_int  maxEntries,
                                                fm_text optionStr);
fm_status fm10000DbgDumpPortStateTransitions( fm_int sw, fm_int port );
fm_status fm10000DbgClearPortStateTransitions( fm_int sw, fm_int port );
fm_status fm10000DbgSetPortStateTransitionHistorySize( fm_int sw, 
                                                       fm_int port, 
                                                       fm_int size );

fm_status fm10000DbgDumpPortEeeStatus( fm_int sw, fm_int port, fm_bool clear );
fm_status fm10000DbgEnablePortEee( fm_int sw, fm_int port, fm_int mode );

fm_status fm10000IsPciePort( fm_int sw, fm_int port, fm_bool *isPciePort );
fm_status fm10000IsSpecialPort( fm_int sw, fm_int port, fm_bool *isSpecialPort );
fm_status fm10000IsEplPort( fm_int sw, fm_int port, fm_bool *isEplPort );
fm_status fm10000ConfigurePepMode( fm_int sw, fm_int port );

fm_status fm10000PepRecoveryHandler( fm_int sw, 
                                     fm_int port );

fm_status fm10000PepEventHandler( fm_int sw, 
                                  fm_int pep, 
                                  fm_uint32 pepIp );

fm_status fm10000LinkEventHandler( fm_int    sw, 
                                   fm_int    epl, 
                                   fm_int    lane,
                                   fm_uint32 linkIp );

fm_int fm10000GetPortSpeed(fm_ethMode ethMode);
fm_text fm10000GetEthModeStr(fm_ethMode ethMode);
fm_status fm10000GetPortModeName(fm_int sw, fm_int port, fm_text name, fm_int nameLen );

fm_status fm10000GetPortLaneEyeScore(fm_int      sw,
                                     fm_int      port,
                                     fm_int      lane,
                                     fm_uint32 * pEyeScore );
fm_status fm10000GetBistUserPattern(fm_int      sw,
                                    fm_int      port,
                                    fm_int      lane,
                                    fm_uint64 * pBistUserPatternLow,
                                    fm_uint64 * pBistUserPatternHigh);
fm_status fm10000SetBistUserPattern(fm_int      sw,
                                    fm_int      port,
                                    fm_int      lane,
                                    fm_uint64 * pBistUserPatternLow,
                                    fm_uint64 * pBistUserPatternHigh);
fm_status fm10000MapEthModeToDfeMode( fm_int      sw, 
                                      fm_int      port, 
                                      fm_int      lane,
                                      fm_ethMode  ethMode,
                                      fm_dfeMode *dfeMode );

fm_status fm10000IsPortDisabled( fm_int   sw, 
                                 fm_int   port, 
                                 fm_int   mac, 
                                 fm_bool *isDisabled );
fm_status fm10000GetPortEyeDiagram(fm_int               sw,
                                   fm_int               port,
                                   fm_int               lane,
                                   fm_eyeDiagramSample *pSampleTable);
fm_status fm10000DbgMapLogicalPort(fm_int                 sw,
                                   fm_int                 logPort,
                                   fm_int                 lane,
                                   fm_logPortMappingType  mappingType,
                                   fm_int                *pMapped);
fm_status fm10000UpdateAllSAFValues(fm_int sw);

fm_status fm10000WriteMacIfg( fm_int     sw, 
                              fm_int     epl, 
                              fm_uint    physLane, 
                              fm_uint32 *macCfg, 
                              fm_uint32  ifg );

fm_status fm10000WriteMacDicEnable( fm_int     sw, 
                                    fm_int     epl, 
                                    fm_uint    physLane, 
                                    fm_uint32 *macCfg, 
                                    fm_bool    dicEnable );

fm10000_pcsTypes fm10000GetPcsType( fm_ethMode ethMode, fm_uint32 speed );

fm_status fm10000WriteMacMinColumns( fm_int     sw, 
                                     fm_int     epl, 
                                     fm_uint    physLane, 
                                     fm_uint32 *macCfg, 
                                     fm_uint32  txPadSize );

fm_status fm10000ConfigureEthMode( fm_int      sw, 
                                   fm_int      port, 
                                   fm_ethMode  ethMode,
                                   fm_bool    *restoreMode );

fm_status fm10000WriteMacTxClockCompensation( fm_int     sw,         
                                              fm_int     epl,        
                                              fm_uint    physLane,   
                                              fm_uint32 *macCfg,     
                                              fm_uint32  txClkCompensation,
                                              fm10000_pcsTypes  pcsType,
                                              fm_uint32         speed  );

void fm10000DbgDumpSAFTable(fm_int sw);
fm_status fm10000DbgRead100GBipErrRegs(fm_int      sw,
                                       fm_int      port,
                                       fm_int      regSelector,
                                       fm_uint32  *pResult,
                                       fm_bool     clearReg);
fm_status fm10000DbgReadFecUncorrectedErrReg(fm_int      sw,
                                             fm_int      port,
                                             fm_uint32  *pResult,
                                             fm_bool     clearReg);

fm_status fm10000NotifyEeeModeChange( fm_int sw, fm_int port, fm_bool eeeMode );

fm_status fm10000GetMultiLaneCapabilities(fm_int   sw,
                                          fm_int   port,
                                          fm_bool *is40GCapable,
                                          fm_bool *is100GCapable);
fm_status fm10000GetPortOptModeDfeParam(fm_int   sw,
                                        fm_int   serDes,
                                        fm_int   selector,
                                        fm_int * parameter);
fm_status fm10000ConfigurePortOptimizationMode(fm_int   sw,
                                               fm_int   port);

#endif /* __FM_FM10000_API_PORT_INT_H */

