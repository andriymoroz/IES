/* vim:et:sw=4:ts=4:tw=80:
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_switch_int.h
 * Creation Date:   December 3, 2012
 * Description:     FM10xxx-specific API initialization definitions.
 *
 * Copyright (c) 2012 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_SWITCH_INT_H
#define __FM_FM10000_API_SWITCH_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Min NMV version that supports SOFT_RESET locking */
#define NVM_PCIE_RECOVERY_VER               0x122

typedef struct _fm10000_switch
{
    /**************************************************
     * Switch Identification and Capabilities
     **************************************************/

    /* ptr to generic switch struct, kept here for cross-referencing purposes */
    fm_switch *                 base;

    /* local cache of registers */
    fm_fm10000RegCache          registerCache;

    /* holds ownership information for the Mapper */
    fm_fm10000MapOwnershipInfo  mapOwnershipInfo;

    /* holds ownership information for the FFU */
    fm_fm10000FfuOwnershipInfo  ffuOwnershipInfo;

    /* holds ownership information for the Policer */
    fm_fm10000PolOwnershipInfo  polOwnershipInfo;

    fm_fm10000CompiledAcls *    compiledAcls;

    fm_fm10000CompiledAcls *    appliedAcls;

    /* cached API properties */
    fm_int                      aclPrecedenceMin;
    fm_int                      aclPrecedenceMax;
    fm_bool                     aclStrictCount;

    fm_fm10000TunnelCfg *       tunnelCfg;

    fm_fm10000NatCfg *          natCfg;

    /* Canonical glort CAM */
    fm10000CanonCamEntry        canonCamEntry[FM10000_CANONICAL_GLORT_CAM_ENTRIES];

    /***************************************************
     * Cached switch attributes
     **************************************************/

    fm_bool                     trapBpdu;
    fm_bool                     trapLacp;
    fm_bool                     trap8021X;
    fm_bool                     trapGarp;
    fm_bool                     trapMtuViolations;
    fm_bool                     trapPlusLog;

    fm_int                      reservedMacAction[FM_NUM_RESERVED_MACS];
    fm_bool                     reservedMacUsePri[FM_NUM_RESERVED_MACS];

    /* Whether to drop PAUSE frames (FM_DROP_PAUSE). */
    fm_bool                     dropPause;
    
    /* Whether to drop frames with invalid SMACs (FM_DROP_INVALID_SMAC). */
    fm_bool                     dropInvalidSmac;

    /* VLAN Ethernet types (FM_SWITCH_PARSER_VLAN_ETHER_TYPES). */
    fm_uint16                   parserVlanEtherTypes[FM10000_PARSER_VLAN_TAG_ENTRIES];

    /* VLAN Ethernet types (FM_SWITCH_MODIFY_VLAN_ETHER_TYPES). */
    fm_uint16                   modVlanEtherTypes[FM10000_MOD_VLAN_ETYPE_ENTRIES];

    /* MPLS Ethernet types (FM_SWITCH_MPLS_ETHER_TYPES). */
    fm_uint16                   mplsEtherType[FM_NUM_MPLS_ETHER_TYPES];

    /* Global custom tag configuration (FM_SWITCH_PARSER_CUSTOM_TAG). */
    fm_customTagConfig          customTag[FM10000_PARSER_CUSTOM_TAG_ENTRIES];

    /* Deep Inspection filter configurations (FM_SWITCH_PARSER_DI_CFG). */
    fm_parserDiCfgFields        parserDiCfg[FM10000_PARSER_DI_CFG_ENTRIES];

    /* FM_IP_OPTIONS_DISPOSITION */
    fm_uint32                   ipOptionsDisposition;

    /* Frame aging time in milliseconds (FM_FRAME_AGING_TIME_MSEC). */
    fm_uint32                   frameAgingTime;

    /***************************************************
     * Cached PEP to logical port mapping
     **************************************************/

    /* PEP to logical port mapping */
    fm_int              pepPortMapping[FM10000_NUM_PEPS];

    /***************************************************
     * Information regarding the serdes API
     **************************************************/

    /* Indicates the serdes operational mode, that may
     * be: chip, stub, test bench or test board. It 
     * also specifies if SBus or LANE_SAI_xxx registers 
     * should be used to access the SerDes registers.
     * See the enum fm_serDesOpMode for details */
    fm_serDesOpMode             serdesOpMode;

    /* Indicates the type of state machine being used.
     * When using the test board, both state machines 
     * are used: the basic for ports 1..6, and the 
     * stub sm for the other ones */
    fm10000_serDesSmMode        serdesSmMode;

    /* If TRUE, tells to use the LANE_SAI_XXX registers for
     * SerDes Spico interrupts, otherwise use SBus. 
     * ''serdesIntAccssCtrlEna'' must be TRUE to enable this 
     * flag, SBus is used during SerDes interrupts */
    fm_bool                     serdesIntUseLaneSai;

    /* Enable the ''serdesIntUseLaneSai'' flag. This is mostly
     * used to control SerDes interrupts during initialization */
    fm_bool                     serdesIntAccssCtrlEna;

    /* Bypass accesses to the SBus. This is used in very
     * specific situations such us when the API is running 
     * on the test bench */
    fm_bool                     serdesBypassSbus;

    /* Indicates if the SPICO build supports KR or not*/
    fm_bool                     serdesSupportsKR;

    /* pointer to an array of pointers to the arrays that
     * will hold the error samples when plotting the eye
     * diagram. */
    fm_uint32 *                 (*ppEyeDiagramSampleArrays)[];

    /****************************************************
     * Serdes extendes services and general information
     ***************************************************/
    fm10000_serdes              serdesXServices;

    /***************************************************
     * Information regarding the trigger API
     **************************************************/
    fm10000_triggerInfo         triggerInfo;

    /***************************************************
     * Information regarding the scheduler API
     **************************************************/
    fm10000_schedInfo           schedInfo;

    /***************************************************
     * Information regarding the mirror API
     **************************************************/
    fm_fm10000PortMirrorGroup   mirrorGroups[FM10000_MAX_MIRRORS_GRP];

    /* mirror ffu resource mask */
    fm_int                      mirrorFfuResMask;

    /***************************************************
     * Information regarding the Storm Controller API
     **************************************************/
    fm10000_scInfo              scInfo;

    /***************************************************
     * Information regarding the LBG API
     **************************************************/
    fm_bitArray                 lbgUsage;

    /***************************************************
     * MAC Address Table information.
     **************************************************/

    /* MAC Table security violation action. Specified via the
     * ''FM_MAC_TABLE_SECURITY_ACTION'' MAC Table attribute. */
    fm_macSecurityAction        macSecurityAction;

    /* Maximum number of TCN FIFO entries to process in a cycle. */
    fm_int                      tcnFifoBurstSize;

    /* Current state of MA_USED_TABLE sweeper. */
    fm_int                      usedTableSweeperState;

    /* Current position of sweeper in MA_USED_TABLE. */
    fm_int                      usedTableSweeperIndex;

    /* Number of expired entries detected during the current pass. */
    fm_int                      usedTableNumExpired;

    /* Start time of last MA_USED_TABLE sweep. */
    fm_uint64                   usedTableLastSweepTime;

    fm_uint64                   usedTableAgingTime;
    fm_uint64                   usedTableExpiryTime;

    /* Whether the API should automatically create logical ports for
     * remote glorts. */
    fm_bool                     createRemoteLogicalPorts;

    /* MAC Security information. */
    fm10000_securityInfo        securityInfo;

    /***************************************************
     * Flooding control (broadcast, multicast, unicast).
     **************************************************/
    fm10000_floodInfo           floodInfo;
    fm_bcastFlooding            bcastFlooding;
    fm_mcastFlooding            mcastFlooding;
    fm_ucastFlooding            ucastFlooding;

    /***************************************************
     * NextHop subsystem data
     **************************************************/
    fm10000_NextHopSysCtrl     *pNextHopSysCtrl;

    /***************************************************
     * Routing subsystem data
     **************************************************/
    fm10000_RoutingState        routeStateTable;
    fm_int                      unicastMinPrecedence;
    fm_int                      multicastMinPrecedence;
    fm_int                      maxRoutes;
    fm_int                      maxRouteSlices;

    /***************************************************
     * Multicast subsystem 
     **************************************************/
    fm10000_mtableInfo          mtableInfo;

    /**************************************************
     * Information related to the Flow API.
     **************************************************/
    fm10000_flowInfo            flowInfo;

    /**************************************************
     * Information related to the SFlow API.
     **************************************************/
    fm10000_sflowEntry          sflowEntry[FM10000_MAX_SFLOWS];

    /***************************************************
     * Parity error subsystem.
     **************************************************/
    fm10000_parityInfo          parityInfo;
    fm_lock                     parityLock;

    /***************************************************
     * Counter Rate Monitor (CRM) subsystem.
     **************************************************/
    fm10000_crmInfo             crmInfo;
    fm_bool                     isCrmStarted;

    /**************************************************
     * Information related to the Virtual Network API.
     **************************************************/
    /* VXLAN UDP Port Number. */
    fm_uint                     vnVxlanUdpPort;

    /* VXLAN-GPE UDP Port Number. */
    fm_uint                     vnGpeUdpPort;

    /* Geneve (NGE) UDP Port Number. */
    fm_uint                     vnGeneveUdpPort;

    /* TRUE to use shared encap flows with VN encapsulation rules */
    fm_bool                     useSharedEncapFlows;

    /* Maximunm Number of Remote Addresses */
    fm_int                      maxVNRemoteAddresses;

    /* Tunnel group Hash Size. */
    fm_int                      vnTunnelGroupHashSize;

    /* The VID to be used for frames coming back to the switch from the TE after encap. */
    fm_int                      vnTeVid;

    /* NVGRE/Geneve Encapsulation Protocol. */
    fm_uint                     vnEncapProtocol;

    /* NVGRE/Geneve Encapsulation Version. */
    fm_uint                     vnEncapVersion;

    /* The Encapsulation ACL Number. */
    fm_int                      vnEncapAcl;

    /* The decapsulation ACL Number. */
    fm_int                      vnDecapAcl;

    /* Number of virtual networks in use. */
    fm_int                      numVirtualNetworks;

    /* Number of virtual network tunnels in use. */
    fm_int                      numVNTunnels;

    /* Outer TTL Value. */
    fm_int                      vnOuterTTL;

    /* Parser Deep Inspection Configuration Index. */
    fm_int                      vnDeepInspectionCfgIndex;

    /* Group IDs for encapsulation and decapsulation tunnels, as assigned by the raw tunnel API. */
    fm_int                      vnTunnelGroups[FM_VN_NUM_TUNNEL_GROUPS];

    /* Bit Arrays of available rule IDs for tunnel groups. */
    fm_bitArray                 vnTunnelRuleIds[FM_VN_NUM_TUNNEL_GROUPS];

    /* Bit Arrays of internal encapsulation flow IDs in use. */
    fm_bitArray                 vnTunnelActiveEncapFlowIds[FM_VN_NUM_TUNNEL_GROUPS];

    /* Bit Array for encapsulation ACL rule numbers. */
    fm_bitArray                 vnEncapAclRuleNumbers;

    /* Bit Array for encapsulation ACL floodset rule numbers. */
    fm_bitArray                 vnEncapAclFloodsetRuleNumbers;

    /* Bit Array for decapsulation ACL rule numbers. */
    fm_bitArray                 vnDecapAclRuleNumbers;

    /* Decap ACL Rule Tree. */
    fm_customTree               vnDecapAclRuleTree;

    /* VSI table. Index is VSI number, value is pointer to VN. */
    fm_virtualNetwork *         vnVsi[FM10000_TE_VNI_ENTRIES_0];

    /* Register to use for interrupt mask */
    fm_uint32                   interruptMaskReg;
    fm_uint64                   interruptMaskValue;

    /*************************************************************
     * API properties to override IM registers.
     *************************************************************/
    /* mask to override AN_IM register     */
    fm_uint32                   anImProp;

    /* mask to override LINK_IM register   */
    fm_uint32                   linkImProp;

    /* mask to override SERDES_IM register */
    fm_uint32                   serdesImProp;

    /* mask to override PCIE_IM register   */
    fm_uint32                   pcieImProp;

    /* mask to override MA_TCN_IM register*/
    fm_uint32                   maTcnImProp;

    /* mask to override FH_TAIL_IM register*/
    fm_uint32                   fhTailImProp;

    /* mask to override SW_IM register*/
    fm_uint32                   swImProp;

    /* mask to override TE_IM register*/
    fm_uint64                   teImProp;

    /**************************************************
     * Information related to Packet Timestamp.
     **************************************************/
    /* Logical port registered to receive all ethernet egress timestamps */
    fm_int                      ethTimestampsOwnerPort;

    /* Mirror TrapCode ID of the mirror created for PEP to PEP Timestamp retrieval purpose. */
    fm_int                      pepToPepTimestampTrapcodeId;

    /* Indicates whether Egress Timestamping is enabled/disabled. */
    fm_bool                     txTimestampMode;

    /**************************************************
     * Information related to the QoS API.
     **************************************************/
    /* QOS egress scheduler configuration  */
    fm10000_eschedConfigPerPort eschedConfig[FM10000_NUM_PORTS];

    /* Priority mapper mapSet list*/
    fm10000_priorityMapSet *    priorityMapSet;

    /**************************************************
     * Information related to Generic Receive.
     **************************************************/
    /* Drop frames received by user part on unknown port */
    fm_bool                     dropPacketUnknownPort;


} fm10000_switch;



/* Modes that define how the PCIeActive bit is set when changing the xRefClk
 * mode with fm10000SetXrefClkMode */
typedef enum
{
    /* PCIeReset = 1
     * xRefClk.Mode = mode
     * PCIeReset = 0*/
    FM10000_PA_ACTIVE_DO_NOTHING = 0,

    /* PCIeReset = 1
     * xRefClk.Mode = mode
     * PCIeReset = 0
     * PCIeActive = 1 */
    FM10000_PA_ACTIVE_SET_END,

    /* PCIeReset = 1
     * PCIeActive = 0
     * xRefClk.Mode = mode
     * PCIeReset = 0 */
    FM10000_PA_ACTIVE_CLEAR_AFTER_RESET,

} fm10000PaMode;



/*****************************************************************************
 * Public Function Prototypes
 *****************************************************************************/

fm_status fm10000TakeSoftResetLock(fm_int sw);
fm_status fm10000DropSoftResetLock(fm_int sw);
fm_status fm10000SetXrefClkMode(fm_int sw, 
                                 fm_uint32 pep, 
                                 fm_uint32 mode, 
                                 fm10000PaMode paMode);
fm_status fm10000GetXrefClkMode( fm_int sw, 
                                 fm_uint32 pep, 
                                 fm_uint32 *mode,
                                 fm_uint32 *stat);

fm_status fm10000StartPepStatusPollingTimer(fm_int sw, fm_int port);
fm_status fm10000AllocateDataStructures(fm_switch *switchPtr);
fm_status fm10000FreeDataStructures(fm_switch *switchPtr);
fm_status fm10000ComputeFHClockFreq(fm_int sw, fm_float *fhMhz);
fm_status fm10000GetSwitchTrapCode(fm_int sw, fm_trapType type, fm_int *code);
fm_status fm10000GetSwitchTrapType(fm_int sw, fm_int code, fm_trapType *type);
fm_status fm10000EventHandlingInitialize(fm_int sw);
fm_status fm10000FreeResources(fm_int sw);
fm_status fm10000GetSwitchInfo(fm_int sw, fm_switchInfo *info);
fm_status fm10000InitPort(fm_int port, fm_port *portPtr);
fm_status fm10000InitPortTable(fm_switch *switchPtr);
fm_status fm10000InitSwitch(fm_switch *switchPtr);
fm_status fm10000InterruptHandler(fm_switch *switchPtr);
fm_status fm10000PostBootSwitch(fm_int sw);
fm_status fm10000SetSwitchState(fm_int sw, fm_bool state);
fm_status fm10000IdentifySwitch(fm_int            sw,
                                fm_switchFamily * family,
                                fm_switchModel *  model,
                                fm_switchVersion *version);
void fm10000DebounceLinkStates(fm_int     sw,          
                               fm_thread *thread,      
                               fm_thread *handlerThread );
fm_status fm10000VerifySwitchAliveStatus(fm_int sw);
fm_status fm10000GetNvmImageVersion(fm_int sw, fm_uint *version);

void * fm10000FastMaintenanceTask(fm_int sw, void *args);

fm_status fm10000SendLinkUpDownEvent( fm_int           sw,
                                      fm_int           physPort,
                                      fm_int           mac,
                                      fm_bool          linkUp,
                                      fm_eventPriority priority );
fm_status fm10000SendLinkUpDownEventV2(fm_int           sw,
                                       fm_int           physPort,
                                       fm_int           mac,
                                       fm_bool          linkUp,
                                       fm_eventPriority priority,
                                       fm_bool *        pAddedFreeEvent);
fm_status fm10000I2cWriteRead(fm_int     sw,
                              fm_uint    device,
                              fm_byte   *data,
                              fm_uint    wl,
                              fm_uint    rl);
fm_status fm10000Write1588SystimeCfg(fm_int sw, fm_int step);

#endif  /* __FM_FM10000_API_SWITCH_INT_H */

