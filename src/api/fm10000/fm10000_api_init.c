/* vim:et:sw=4:ts=4:tw=79:
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_init.c
 * Creation Date:   December 3, 2012
 * Description:     FM10xxx-specific API initialization functions
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Default FM_PORT_MAX_FRAME_SIZE. */
#define DEFAULT_MAX_FRAME_SIZE          1536

#define MAX_BUF_SIZE                    256


/* Definition values for FM10000_PLL_FABRIC_LOCK */
#define FM10000_PFL_FEATURE_CODE_FULL         0
#define FM10000_PFL_FEATURE_CODE_LIMITED1     1   /* 600, 500, 400, 300Mhz */
#define FM10000_PFL_FEATURE_CODE_LIMITED2     2   /* 500, 400, 300Mhz */
#define FM10000_PFL_FEATURE_CODE_LIMITED3     3   /* 400, 300Mhz */
#define FM10000_PFL_FEATURE_CODE_LIMITED4     4   /* 300Mhz */
#define FM10000_PFL_FREQ_SEL_USE_CTRL         0
#define FM10000_PFL_FREQ_SEL_F600             1
#define FM10000_PFL_FREQ_SEL_F500             2
#define FM10000_PFL_FREQ_SEL_F400             3
#define FM10000_PFL_FREQ_SEL_F300             4

/* SKU values */
#define FM10840_SKU                           0
#define FM10420_SKU                           1
#define FM10824_SKU                           2
#define FM10064_SKU                           3
#define FM10036_SKU                           4
#define NO_SKU                             0xFF

/* Closest binary value to 980000000 */
#define BINARY_980_MHZ                979369984
/* Closest binary value to 700000000 */
#define BINARY_700_MHZ                699404761

/* INTERRUPT_MASK bits to unmask for parity error detection. */
#define FM10000_INT_PARITY_DETECT   \
    (FM10000_INT_CORE           |   \
     FM10000_INT_CRM            |   \
     FM10000_INT_FH_TAIL        |   \
     FM10000_INT_FH_HEAD        |   \
     FM10000_INT_TUNNEL_0       |   \
     FM10000_INT_TUNNEL_1)

#define MAX_INT_VALUE                         2147483647


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

const fm_switch FM10000SwitchDefaultTable =
{
    /***************************************************
     * Chip-specific Constants
     **************************************************/
    .maxPhysicalPort                    = FM10000_MAX_PORT,
    .vlanTableSize                      = FM10000_MAX_VLAN,
    .macTableSize                       = FM10000_MAX_ADDR,
    .macTableBankCount                  = FM10000_MAC_ADDR_BANK_COUNT,
    .macTableBankSize                   = FM10000_MAC_ADDR_BANK_SIZE,
    .mirrorTableSize                    = FM10000_MAX_MIRRORS_GRP,
    .maxSTPInstances                    = FM10000_MAX_STP_INSTANCE,
    .maxPhysicalLags                    = FM_MAX_NUM_LAGS,
    .maxPhysicalPortsPerLag             = FM10000_MAX_PHYS_PORTS_PER_LAG,
    .maxRoutes                          = FM_MAX_ROUTES,
    .maxArpEntries                      = FM10000_ARP_TABLE_SIZE,
    .maxEcmpGroupSize                   = FM10000_MAX_ECMP_GROUP_SIZE,
    .maxIpInterfaces                    = FM_MAX_IP_INTERFACES,
    .maxVirtualRouters                  = FM10000_MAX_VIRTUAL_ROUTERS,
    .policerBanks                       = FM10000_MAX_POLICER_BANKS,
    .maxSegments                        = FM10000_MAX_MEMORY_SEGMENTS,
    .maxVlanCounter                     = FM10000_MAX_VLAN_COUNTER,
    .vlanLearningMode                   = FM_VLAN_LEARNING_MODE_INDEPENDENT,
    .sharedLearningVlan                 = 1,
#if 0
    .segmentSize                        = FM10000_SEGMENT_SIZE,
#endif
    .maxVNTunnels                       = FM10000_MAX_VN_TUNNELS,

    /***************************************************
     * Chip-specific Function Pointers
     **************************************************/
    .InitSwitch                         = fm10000InitSwitch,
    .InterruptHandler                   = fm10000InterruptHandler,
    .AllocateDataStructures             = fm10000AllocateDataStructures,
    .FreeDataStructures                 = fm10000FreeDataStructures,
    .ComputeFHClockFreq                 = fm10000ComputeFHClockFreq,
    .DbgDumpScheduler                   = fm10000DbgDumpSchedulerConfig,

    /* Switch state manipulation.  */
    .FreeResources                      = fm10000FreeResources,
    .PostBootSwitch                     = fm10000PostBootSwitch,
    .ReleaseSwitch                      = fmPlatformRelease,
    .ResetSwitch                        = fmPlatformReset,
    .SetSwitchState                     = fm10000SetSwitchState,

    /**************************************************
     * Switch Attributes
     **************************************************/
    .GetSwitchInfo                      = fm10000GetSwitchInfo,
    .SetSwitchAttribute                 = fm10000SetSwitchAttribute,
    .GetSwitchAttribute                 = fm10000GetSwitchAttribute,
    .GetCpuPort                         = fm10000GetCpuPort,
    .SetCpuPort                         = fm10000SetCpuPort,

    /**************************************************
     * Logical Port Management
     **************************************************/
    .AllocLogicalPort                   = fm10000AllocLogicalPort,
    .AllocVirtualLogicalPort            = fm10000AllocVirtualLogicalPort,
    .FreeLogicalPort                    = fm10000FreeLogicalPort,
    .FreeVirtualLogicalPort             = fm10000FreeVirtualLogicalPort,
    .CreateLogicalPort                  = fm10000CreateLogicalPort,
    .GetLogicalPortAttribute            = fm10000GetLogicalPortAttribute,
    .SetLogicalPortAttribute            = fm10000SetLogicalPortAttribute,
    .WriteGlortCamEntry                 = fm10000WriteGlortCamEntry,
    .FreeDestEntry                      = fm10000FreeDestEntry,
    .SetGlortDestMask                   = fm10000SetGlortDestMask,
    .FreeLaneResources                  = fm10000FreeLaneResources,
    .GetPcieLogicalPort                 = fm10000GetPcieLogicalPort,
    .GetLogicalPortPcie                 = fm10000GetLogicalPortPcie,
    .DbgMapLogicalPort                  = fm10000DbgMapLogicalPort,

    /***************************************************
     * Port Attributes
     **************************************************/
    .IsPerLagPortAttribute              = fm10000IsPerLagPortAttribute,

    /**************************************************
     * Glort Management
     **************************************************/
    .GetGlortForSpecialPort             = fm10000GetGlortForSpecialPort,
    .GetMaxGlortsPerLag                 = fm10000GetMaxGlortsPerLag,
    .CreateCanonicalCamEntries          = fm10000CreateCanonicalCamEntries,
    .DeleteCanonicalCamEntries          = fm10000DeleteCanonicalCamEntries,

    /**************************************************
     * Vlan Support
     **************************************************/
    .CreateVlan                         = fm10000CreateVlan,
    .DeleteVlan                         = fm10000DeleteVlan,
    .AllocateVlanTableDataStructures    = fm10000AllocateVlanTableDataStructures,
    .FreeVlanTableDataStructures        = fm10000FreeVlanTableDataStructures,
    .InitVlanTable                      = fm10000InitVlanTable,
    .GetVlanAttribute                   = fm10000GetVlanAttribute,
    .SetVlanAttribute                   = fm10000SetVlanAttribute,
    .GetVlanPortAttribute               = fm10000GetVlanPortAttribute,
    .SetVlanPortState                   = fm10000SetVlanPortState,      /* Deprecated */
    .GetVlanPortState                   = fm10000GetVlanPortState,      /* Deprecated */
    .WriteVlanEntry                     = fm10000WriteVlanEntry,
    .WriteTagEntry                      = fm10000WriteTagEntry,
    .AddVlanPortList                    = fm10000AddVlanPortList,
    .DeleteVlanPortList                 = fm10000DeleteVlanPortList,
    .SetVlanCounterID                   = fm10000SetVlanCounterID,

    /**************************************************
     * Spanning Tree Support
     **************************************************/
    .CreateSpanningTree                 = fm10000CreateSpanningTree,
    .AddSpanningTreeVlan                = fm10000AddSpanningTreeVlan,
    .DeleteSpanningTreeVlan             = fm10000DeleteSpanningTreeVlan,
    .RefreshSpanningTree                = fm10000RefreshSpanningTree,
    .ResetVlanSpanningTreeState         = fm10000ResetVlanSpanningTreeState,
    .DbgDumpSpanningTree                = fm10000DbgDumpSpanningTree,


    /**************************************************
     * Storm Controller Support
     **************************************************/
    .CreateStormCtrl                    = fm10000CreateStormCtrl,
    .DeleteStormCtrl                    = fm10000DeleteStormCtrl,
    .SetStormCtrlAttribute              = fm10000SetStormCtrlAttribute,
    .GetStormCtrlAttribute              = fm10000GetStormCtrlAttribute,
    .GetStormCtrlList                   = fm10000GetStormCtrlList,
    .GetStormCtrlFirst                  = fm10000GetStormCtrlFirst,
    .GetStormCtrlNext                   = fm10000GetStormCtrlNext,
    .AddStormCtrlCondition              = fm10000AddStormCtrlCondition,
    .DeleteStormCtrlCondition           = fm10000DeleteStormCtrlCondition,
    .GetStormCtrlConditionList          = fm10000GetStormCtrlConditionList,
    .GetStormCtrlConditionFirst         = fm10000GetStormCtrlConditionFirst,
    .GetStormCtrlConditionNext          = fm10000GetStormCtrlConditionNext,
    .AddStormCtrlAction                 = fm10000AddStormCtrlAction,
    .DeleteStormCtrlAction              = fm10000DeleteStormCtrlAction,
    .GetStormCtrlActionList             = fm10000GetStormCtrlActionList,
    .GetStormCtrlActionFirst            = fm10000GetStormCtrlActionFirst,
    .GetStormCtrlActionNext             = fm10000GetStormCtrlActionNext,
    .DbgDumpStormCtrl                   = fm10000DbgDumpStormCtrl,

    /**************************************************
     * Event Handling Functions
     **************************************************/
    .EventHandlingInitialize            = fm10000EventHandlingInitialize,
    .SendLinkUpDownEvent                = fm10000SendLinkUpDownEvent,
    .DebounceLinkStates                 = fm10000DebounceLinkStates,
    /* QOS support functions.  */
    .GetSwitchQOS                       = fm10000GetSwitchQOS,
    .SetSwitchQOS                       = fm10000SetSwitchQOS,
#if 0
    .StopTraffic                        = fm10000StopTraffic,
    .RestartTraffic                     = fm10000RestartTraffic,
#endif

    /**************************************************
     * Statistics Support
     **************************************************/
    .GetCountersInitMode                = fm10000GetCountersInitMode,
    .GetVLANCounters                    = fm10000GetVLANCounters,
    .ResetVLANCounters                  = fm10000ResetVLANCounters,

    /**************************************************
     * Packet transmission functions.
     **************************************************/
    .GeneratePacketISL                  = fm10000GeneratePacketISL,
    .SendPacket                         = fm10000SendPacket,
    .SendPacketDirected                 = fm10000SendPacketDirected,
    .SendPacketISL                      = fm10000SendPacketISL,
    .SendPacketSwitched                 = fm10000SendPacketSwitched,
    .SetPacketInfo                      = fm10000SetPacketInfo,

    /**************************************************
     * MAC Address Table Functions
     **************************************************/
    .AddAddress                         = fm10000AddAddress,
    .AllocAddrTableData                 = fm10000AllocAddrTableData,
    .AssignTableEntry                   = fm10000AssignTableEntry,
    .CheckFlushRequest                  = fm10000CheckFlushRequest,
    .ComputeAddressIndex                = fm10000ComputeAddressIndex,
    .DeleteAddressPre                   = fm10000DeleteAddressPre,
    .DeleteAllAddresses                 = fm10000DeleteAllAddresses,
    .DumpPurgeStats                     = fm10000DumpPurgeStats,
    .FillInUserEntryFromTable           = fm10000FillInUserEntryFromTable,
    .FindAndInvalidateAddress           = fm10000FindAndInvalidateAddress,
    .FreeAddrTableData                  = fm10000FreeAddrTableData,
    .GetAddressOverride                 = fm10000GetAddress,
    .GetAddressTable                    = fm10000GetAddressTable,
    .GetAddressTableAttribute           = fm10000GetAddressTableAttribute,
    .GetLearningFID                     = fm10000GetLearningFID,
    .GetSecurityStats                   = fm10000GetSecurityStats,
    .InitAddressTable                   = fm10000InitAddressTable,
    .ResetPurgeStats                    = fm10000ResetPurgeStats,
    .ResetSecurityStats                 = fm10000ResetSecurityStats,
    .SetAddressTableAttribute           = fm10000SetAddressTableAttribute,
    .UpdateMATable                      = fm10000UpdateMATable,
    .WriteEntryAtIndex                  = fm10000WriteEntryAtIndex,
    .DbgDumpMACTable                    = fm10000DbgDumpMACTable,
    .DbgDumpMACTableEntry               = fm10000DbgDumpMACTableEntry,

    /**************************************************
     * MAC Maintenance Task functions
     **************************************************/
    .HandleMACTableEvents               = fm10000HandleMACTableEvents,
    .HandlePurgeRequest                 = fm10000HandlePurgeRequest,

    /**************************************************
     * Link Aggregation Group support
     **************************************************/
    .InformLAGPortUp                    = fm10000InformLAGPortUp,
    .InformLAGPortDown                  = fm10000InformLAGPortDown,
    .AllocateLAGs                       = fm10000AllocateLAGs,
    .FreeStackLAGs                      = fm10000FreeStackLAGs,
    .CreateLagOnSwitch                  = fm10000CreateLagOnSwitch,
    .DeleteLagFromSwitch                = fm10000DeleteLagFromSwitch,
    .FreeLAG                            = fm10000FreeLAG,
    .DeletePortFromLag                  = fm10000DeletePortFromLag,
    .AddPortToLag                       = fm10000AddPortToLag,
    .GetLagAttribute                    = fm10000GetLagAttribute,
    .SetLagAttribute                    = fm10000SetLagAttribute,

    /**************************************************
     * Mirror Support
     **************************************************/
    .CreateMirror                       = fm10000CreateMirror,
    .DeleteMirror                       = fm10000DeleteMirror,
    .WritePortMirrorGroup               = fm10000WritePortMirrorGroup,
    .SetMirrorAttribute                 = fm10000SetMirrorAttribute,
    .GetMirrorAttribute                 = fm10000GetMirrorAttribute,
    .AddMirrorVlan                      = fm10000AddMirrorVlan,
    .DeleteMirrorVlan                   = fm10000DeleteMirrorVlan,

    /***************************************************
     * SFlow Functions
     **************************************************/
    .AddSFlowPort                       = fm10000AddSFlowPort,
    .CheckSFlowLogging                  = fm10000CheckSFlowLogging,
    .CreateSFlow                        = fm10000CreateSFlow,
    .DeleteSFlow                        = fm10000DeleteSFlow,
    .DeleteSFlowPort                    = fm10000DeleteSFlowPort,
    .GetSFlowAttribute                  = fm10000GetSFlowAttribute,
    .GetSFlowPortFirst                  = fm10000GetSFlowPortFirst,
    .GetSFlowPortList                   = fm10000GetSFlowPortList,
    .GetSFlowPortNext                   = fm10000GetSFlowPortNext,
    .GetSFlowType                       = fm10000GetSFlowType,
    .SetSFlowAttribute                  = fm10000SetSFlowAttribute,

    /**************************************************
     * Routing Functions
     **************************************************/
    .RouterAlloc                        = fm10000RouterAlloc,
    .RouterFree                         = fm10000RouterFree,
    .RouterInit                         = fm10000RouterInit,
    .SetRouterAttribute                 = fm10000SetRouterAttribute,
    .AddRoute                           = fm10000AddRoute,
    .DeleteRoute                        = fm10000DeleteRoute,
    .SetRouteAction                     = fm10000SetRouteAction,
    .ReplaceECMPBaseRoute               = fm10000ReplaceECMPBaseRoute,
    .SetRouteActive                     = fm10000SetRouteActive,
    .SetInterfaceAttribute              = fm10000SetInterfaceAttribute,
    .AddVirtualRouter                   = fm10000AddVirtualRouter,
    .RemoveVirtualRouter                = fm10000RemoveVirtualRouter,
    .SetRouterState                     = fm10000SetRouterState,
    .SetRouterMacMode                   = fm10000SetRouterMacMode,
    .DbgDumpRouteStats                  = fm10000DbgDumpRouteStats,
    .DbgDumpRouteTables                 = fm10000DbgDumpRouteTables,
    .DbgValidateRouteTables             = fm10000DbgValidateRouteTables,

    /***************************************************
     * NextHop Functions
     **************************************************/
    .NextHopInit                        = fm10000NextHopInit,
    .CreateECMPGroup                    = fm10000CreateECMPGroup,
    .DeleteECMPGroup                    = fm10000DeleteECMPGroup,
    .FreeEcmpGroup                      = fm10000FreeECMPGroup,
    .ValidateECMPGroupDeletion          = fm10000ValidateDeleteEcmpGroup,
    .UpdateEcmpGroup                    = fm10000UpdateEcmpGroup,
    .AddECMPGroupNextHops               = fm10000AddECMPGroupNextHops,
    .DeleteECMPGroupNextHops            = fm10000DeleteECMPGroupNextHops,
    .ReplaceECMPGroupNextHop            = fm10000ReplaceECMPGroupNextHop,
    .SetECMPGroupNextHops               = fm10000SetECMPGroupNextHops,
    .UpdateNextHop                      = fm10000UpdateNextHop,
    .GetNextHopUsed                     = fm10000GetNextHopUsed,
    .GetECMPGroupARPUsed                = fm10000GetECMPGroupARPUsed,
    .GetECMPGroupNextHopIndexRange      = fm10000GetECMPGroupNextHopIndexRange,
    .GetNextHopIndexUsed                = fm10000GetNextHopIndexUsed,
    .SetInterfaceAttribute              = fm10000SetInterfaceAttribute,
    .DbgDumpArpTable                    = fm10000DbgDumpArpTable,
    .DbgDumpArpHandleTable              = fm10000DbgDumpArpHandleTable,
    .DbgPlotUsedArpDiagram              = fm10000DbgPlotUsedArpDiagram,
    .ValidateNextHopTrapCode            = fm10000ValidateNextHopTrapCode,

    /***************************************************
     * Virtual Network Functions
     **************************************************/
    .CreateVirtualNetwork               = fm10000CreateVirtualNetwork,
    .DeleteVirtualNetwork               = fm10000DeleteVirtualNetwork,
    .UpdateVirtualNetwork               = fm10000UpdateVirtualNetwork,
    .FreeVirtualNetwork                 = fm10000FreeVirtualNetwork,
    .CreateVNTunnel                     = fm10000CreateVNTunnel,
    .DeleteVNTunnel                     = fm10000DeleteVNTunnel,
    .SetVNTunnelAttribute               = fm10000SetVNTunnelAttribute,
    .FreeVNTunnel                       = fm10000FreeVNTunnel,
    .AddVNRemoteAddress                 = fm10000AddVNRemoteAddress,
    .DeleteVNRemoteAddress              = fm10000DeleteVNRemoteAddress,
    .AddVNRemoteAddressMask             = fm10000AddVNRemoteAddressMask,
    .DeleteVNRemoteAddressMask          = fm10000DeleteVNRemoteAddressMask,
    .ConfigureVN                        = fm10000ConfigureVN,
    .AddVNLocalPort                     = fm10000AddVNLocalPort,
    .DeleteVNLocalPort                  = fm10000DeleteVNLocalPort,
    .AddVNVsi                           = fm10000AddVNVsi,
    .DeleteVNVsi                        = fm10000DeleteVNVsi,
    .IsVNTunnelInUseByACLs              = fm10000IsVNTunnelInUseByACLs,
    .FreeVNResources                    = fm10000FreeVNResources,
#if 0
    .UpdateVNTunnelECMPGroup            = fm10000UpdateVNTunnelECMPGroup,
#endif

    /**************************************************
     * ACL Operations
     **************************************************/
    .GetACLCountExt                     = fm10000GetACLCountExt,
    .ResetACLCount                      = fm10000ResetACLCount,
    .GetACLEgressCount                  = fm10000GetACLEgressCount,
    .ResetACLEgressCount                = fm10000ResetACLEgressCount,
    .UpdateACLRule                      = fm10000UpdateACLRule,
    .SetACLRuleState                    = fm10000SetACLRuleState,
    .ACLCompile                         = fm10000ACLCompile,
    .ACLApplyExt                        = fm10000ACLApplyExt,
    .AddMapperEntry                     = fm10000AddMapperEntry,
    .DeleteMapperEntry                  = fm10000DeleteMapperEntry,
    .ClearMapper                        = fm10000ClearMapper,
    .GetMapperSize                      = fm10000GetMapperSize,
    .GetMapperL4PortKey                 = fm10000GetMapperL4PortKey,
    .GetACLRuleAttribute                = fm10000GetACLRuleAttribute,
    .SetPolicerAttribute                = fm10000SetPolicerAttribute,
    .UpdatePolicer                      = fm10000UpdatePolicer,
    .ValidateACLAttribute               = fm10000ValidateACLAttribute,
    .GetAclFfuRuleUsage                 = fm10000GetAclFfuRuleUsage,

    /**************************************************
     * Multicast Group Functions
     **************************************************/
    .ActivateMcastGroup                 = fm10000ActivateMcastGroup,
    .AddMulticastListener               = fm10000AddMulticastListener,
    .AllocateMcastGroups                = fm10000AllocateMcastGroups,
    .CreateMcastGroup                   = fm10000CreateMcastGroup,
    .DeactivateMcastGroup               = fm10000DeactivateMcastGroup,
    .DeleteMcastGroup                   = fm10000DeleteMcastGroup,
    .DeleteMulticastListener            = fm10000DeleteMulticastListener,
    .FreeMcastGroups                    = fm10000FreeMcastGroups,
    .GetAvailableMulticastListenerCount = fm10000GetAvailableMulticastListenerCount,
    .GetMcastGroupAttribute             = fm10000GetMcastGroupAttribute,
    .GetMcastGroupHwIndex               = fm10000GetMcastGroupHwIndex,
#if 0
    .GetMcastGroupTrigger               = fm10000GetMcastGroupTrigger,
#endif
    .McastGroupInit                     = fm10000McastGroupInit,
    .MoveReplicationGroupMcastGroup     = fm10000MoveReplicationGroupMcastGroup,
    .SetMcastGroupAttribute             = fm10000SetMcastGroupAttribute,
    .ReserveReplicationGroupMcastIndex  = fm10000ReserveReplicationGroupMcastIndex,
    .ReleaseReplicationGroupMcastIndex  = fm10000ReleaseReplicationGroupMcastIndex,
    .ConfigureMcastGroupAsHNIFlooding   = fm10000ConfigureMcastGroupAsHNIFlooding,

    /***************************************************
     * Load Balancing Group Functions
     **************************************************/
    .CreateLBG                          = fm10000CreateLBG,
    .DeleteLBG                          = fm10000DeleteLBG,
    .AddLBGPort                         = fm10000AddLBGPort,
    .DeleteLBGPort                      = fm10000DeleteLBGPort,
    .SetLBGAttribute                    = fm10000SetLBGAttribute,
    .GetLBGAttribute                    = fm10000GetLBGAttribute,
    .SetLBGPortAttribute                = fm10000SetLBGPortAttribute,
    .GetLBGPortAttribute                = fm10000GetLBGPortAttribute,
    .DbgDumpLBG                         = fm10000DbgDumpLBG,
    .GetPortParametersForLBG            = fm10000GetPortParametersForLBG,
    .AssignLBGPortResources             = fm10000AssignLBGPortResources,
    .AllocateLBGs                       = fm10000AllocateLBGs,
    .FreeLBGs                           = fm10000FreeLBGs,

    /***************************************************
     * Stacking Functions
     **************************************************/
    .CreateForwardingRule               = fm10000CreateForwardingRule,
    .DeleteForwardingRule               = fm10000DeleteForwardingRule,
    .CreateLogicalPortForGlort          = fm10000CreateLogicalPortForGlort,
    .CreateLogicalPortForMailboxGlort   = fm10000CreateLogicalPortForMailboxGlort,

    /**************************************************
     * Flow API Support
     **************************************************/
    .CreateFlowTCAMTable                = fm10000CreateFlowTCAMTable,
    .DeleteFlowTCAMTable                = fm10000DeleteFlowTCAMTable,
    .CreateFlowTETable                  = fm10000CreateFlowTETable,
    .DeleteFlowTETable                  = fm10000DeleteFlowTETable,
    .AddFlow                            = fm10000AddFlow,
    .GetFlow                            = fm10000GetFlow,
    .GetFlowTableType                   = fm10000GetFlowTableType,
    .GetFlowFirst                       = fm10000GetFlowFirst,
    .GetFlowNext                        = fm10000GetFlowNext,
    .GetFlowRuleFirst                   = fm10000GetFlowRuleFirst,
    .GetFlowRuleNext                    = fm10000GetFlowRuleNext,   
    .ModifyFlow                         = fm10000ModifyFlow,
    .DeleteFlow                         = fm10000DeleteFlow,
    .SetFlowState                       = fm10000SetFlowState,
    .GetFlowCount                       = fm10000GetFlowCount,
    .ResetFlowCount                     = fm10000ResetFlowCount,
    .GetFlowUsed                        = fm10000GetFlowUsed,
    .SetFlowAttribute                   = fm10000SetFlowAttribute,
    .GetFlowAttribute                   = fm10000GetFlowAttribute,
    .InitFlowApiForSWAG                 = fm10000InitFlowApiForSWAG,
    .CreateFlowBalanceGrp               = fm10000CreateFlowBalanceGrp,
    .DeleteFlowBalanceGrp               = fm10000DeleteFlowBalanceGrp,
    .AddFlowBalanceGrpEntry             = fm10000AddFlowBalanceGrpEntry,
    .DeleteFlowBalanceGrpEntry          = fm10000DeleteFlowBalanceGrpEntry,

    /**************************************************
     * Tunnel Operations
     **************************************************/
    .CreateTunnel                       = fm10000CreateTunnel,
    .DeleteTunnel                       = fm10000DeleteTunnel,
    .GetTunnel                          = fm10000GetTunnel,
    .GetTunnelFirst                     = fm10000GetTunnelFirst,
    .GetTunnelNext                      = fm10000GetTunnelNext,
    .AddTunnelEncapFlow                 = fm10000AddTunnelEncapFlow,
    .DeleteTunnelEncapFlow              = fm10000DeleteTunnelEncapFlow,
    .UpdateTunnelEncapFlow              = fm10000UpdateTunnelEncapFlow,
    .GetTunnelEncapFlow                 = fm10000GetTunnelEncapFlow,
    .GetTunnelEncapFlowFirst            = fm10000GetTunnelEncapFlowFirst,
    .GetTunnelEncapFlowNext             = fm10000GetTunnelEncapFlowNext,
    .AddTunnelRule                      = fm10000AddTunnelRule,
    .DeleteTunnelRule                   = fm10000DeleteTunnelRule,
    .UpdateTunnelRule                   = fm10000UpdateTunnelRule,
    .GetTunnelRule                      = fm10000GetTunnelRule,
    .GetTunnelRuleFirst                 = fm10000GetTunnelRuleFirst,
    .GetTunnelRuleNext                  = fm10000GetTunnelRuleNext,
    .GetTunnelRuleCount                 = fm10000GetTunnelRuleCount,
    .GetTunnelEncapFlowCount            = fm10000GetTunnelEncapFlowCount,
    .GetTunnelRuleUsed                  = fm10000GetTunnelRuleUsed,
    .ResetTunnelRuleCount               = fm10000ResetTunnelRuleCount,
    .ResetTunnelEncapFlowCount          = fm10000ResetTunnelEncapFlowCount,
    .ResetTunnelRuleUsed                = fm10000ResetTunnelRuleUsed,
    .SetTunnelAttribute                 = fm10000SetTunnelAttribute,
    .GetTunnelAttribute                 = fm10000GetTunnelAttribute,
    .DbgDumpTunnel                      = fm10000DbgDumpTunnel,

    /**************************************************
     * NAT Operations
     **************************************************/
    .CreateNatTable                     = fm10000CreateNatTable,
    .DeleteNatTable                     = fm10000DeleteNatTable,
    .SetNatTunnelDefault                = fm10000SetNatTunnelDefault,
    .CreateNatTunnel                    = fm10000CreateNatTunnel,
    .DeleteNatTunnel                    = fm10000DeleteNatTunnel,
    .AddNatRule                         = fm10000AddNatRule,
    .DeleteNatRule                      = fm10000DeleteNatRule,
    .AddNatPrefilter                    = fm10000AddNatPrefilter,
    .DeleteNatPrefilter                 = fm10000DeleteNatPrefilter,
    .GetNatRuleCount                    = fm10000GetNatRuleCount,
    .ResetNatRuleCount                  = fm10000ResetNatRuleCount,

    /**************************************************
     * Trigger Functions
     **************************************************/
    .CreateTrigger                       = fm10000CreateTrigger,
    .DeleteTrigger                       = fm10000DeleteTrigger,
    .SetTriggerCondition                 = fm10000SetTriggerCondition,
    .SetTriggerAction                    = fm10000SetTriggerAction,
    .GetTrigger                          = fm10000GetTrigger,
    .GetTriggerFirst                     = fm10000GetTriggerFirst,
    .GetTriggerNext                      = fm10000GetTriggerNext,
    .AllocateTriggerResource             = fm10000AllocateTriggerResource,
    .FreeTriggerResource                 = fm10000FreeTriggerResource,
    .GetTriggerResourceFirst             = fm10000GetTriggerResourceFirst,
    .GetTriggerResourceNext              = fm10000GetTriggerResourceNext,
    .SetTriggerRateLimiter               = fm10000SetTriggerRateLimiter,
    .GetTriggerRateLimiter               = fm10000GetTriggerRateLimiter,
    .SetTriggerAttribute                 = fm10000SetTriggerAttribute,
    .GetTriggerAttribute                 = fm10000GetTriggerAttribute,

    /* I2C write read */
    .I2cWriteRead                        = fm10000I2cWriteRead,

    /**************************************************
     * Register debug functions
     **************************************************/
    .DbgDumpRegister                    = fm10000DbgDumpRegister,
    .DbgDumpRegisterV2                  = fm10000DbgDumpRegisterV2,
    .DbgDumpRegisterV3                  = fm10000DbgDumpRegisterV3,
    .DbgGetRegisterName                 = fm10000DbgGetRegisterName,
    .DbgListRegisters                   = fm10000DbgListRegisters,
    .DbgTakeChipSnapshot                = fm10000DbgTakeChipSnapshot,
    .DbgReadRegister                    = fm10000DbgReadRegister,
    .DbgWriteRegister                   = fm10000DbgWriteRegister,
    .DbgWriteRegisterV2                 = fm10000DbgWriteRegisterV2,
    .DbgWriteRegisterV3                 = fm10000DbgWriteRegisterV3,
    .DbgWriteRegisterField              = fm10000DbgWriteRegisterField,

    /**************************************************
     * SerDes debug functions
     **************************************************/
    .DbgReadSBusRegister                = fm10000DbgReadSBusRegister,
    .DbgWriteSBusRegister               = fm10000DbgWriteSBusRegister,
    .DbgInterruptSpico                  = fm10000DbgInterruptSpico,
    .DbgTakeEyeDiagram                  = fm10000DbgTakeEyeDiagram,
    .DbgInitSerDes                      = fm10000DbgSerdesInit,
    .DbgRunSerDesDfeTuning              = fm10000DbgSerdesRunDfeTuning,
    .DbgReadSerDesRegister              = fm10000DbgReadSerDesRegister,
    .DbgWriteSerDesRegister             = fm10000DbgWriteSerDesRegister,
    .DbgDumpSerDes                      = fm10000DbgDumpSerDes,
    .DbgSetSerDesTxPattern              = fm10000DbgSetSerDesTxPattern,
    .DbgSetSerDesRxPattern              = fm10000DbgSetSerDesRxPattern,
    .DbgResetSerDesErrorCounter         = fm10000ResetSerdesErrorCounter,
    .DbgGetSerDesErrorCounter           = fm10000GetSerdesErrorCounter,
    .DbgInjectSerDesErrors              = fm10000DbgSerdesInjectErrors,
    .DbgSetSerDesCursor                 = fm10000SetSerdesCursor,
    .DbgSetSerDesPreCursor              = fm10000SetSerdesPreCursor,
    .DbgSetSerDesPostCursor             = fm10000SetSerdesPostCursor,
    .DbgSetSerDesPolarity               = fm10000DbgSetSerdesPolarity,
    .DbgSetSerDesLoopback               = fm10000DbgSetSerdesLoopback,
#if 0
    .DbgPlotEyeDiagram                  = fm10000DbgPlotEyeDiagram,    
    .DbgDeleteEyeDiagram                = fm10000DbgDeleteEyeDiagram,
#endif

    /**************************************************
     * Miscellaneous debug functions
     **************************************************/
    .DbgDumpFFU                         = fm10000DbgDumpFFU,
    .DbgDumpGlortDestTable              = fm10000DbgDumpGlortDestTable,
    .DbgDumpGlortTable                  = fm10000DbgDumpGlortTable,
    .DbgDumpMapper                      = fm10000DbgDumpMapper,
    .DbgDumpMemoryUsage                 = fm10000DbgDumpMemoryUsage,
    .DbgDumpMirror                      = fm10000DbgDumpMirror,
    .DbgDumpMirrorProfile               = fm10000DbgDumpMirrorProfile,
    .DbgDumpPolicers                    = fm10000DbgDumpPolicers,
    .DbgDumpPortAttributes              = fm10000DbgDumpPortAttributes,
    .DbgDumpTriggers                    = fm10000DbgDumpTriggers,
    .DbgDumpVid                         = fm10000DbgDumpVid,
    .DbgDumpWatermarks                  = fm10000DbgDumpWatermarks,
    .DbgDumpPortMap                     = fm10000DbgDumpPortMap,
    .DbgDumpPortEeeStatus               = fm10000DbgDumpPortEeeStatus, 
    .DbgEnablePortEee                   = fm10000DbgEnablePortEee, 
    .DbgDumpPortStateTransitionsV2      = fm10000DbgDumpPortStateTransitionsV2,
    .DbgDumpPortStateTransitions        = fm10000DbgDumpPortStateTransitions, 
    .DbgClearPortStateTransitions       = fm10000DbgClearPortStateTransitions, 
    .DbgSetPortStateTransitionHistorySize   
                                        = fm10000DbgSetPortStateTransitionHistorySize,
    .DbgDumpSAFTable                    = fm10000DbgDumpSAFTable,
.    DbgDumpPortIdxMap                  = fm10000DbgDumpPortIdxMap,
    .DbgDumpSwpriMap                    = fm10000DbgDumpSwpriMap,
    .DbgDumpWatermarksV3                = fm10000DbgDumpWatermarksV3,
    .DbgDumpMemoryUsageV3               = fm10000DbgDumpMemoryUsageV3,
    .DbgDumpQOS                         = fm10000DbgDumpQOS,
#if 0
    .DbgDumpPortMasks                   = fm10000DbgDumpPortMasks,
    .DbgDumpPortMax                     = fm10000DbgDumpPortMax,
    .DbgPolicerTest                     = fm10000DbgPolicerTest,
    .DbgSwitchSelfTest                  = fm10000DbgSwitchSelfTest,
    .DbgDumpWatermarksV2                = fm10000DbgDumpWatermarksV2,
    .DbgDumpMemoryUsageV2               = fm10000DbgDumpMemoryUsageV2,
#endif

    /***************************************************
     * Parity Error management
     **************************************************/
    .GetParityAttribute                 = fm10000GetParityAttribute,
    .GetParityErrorCounters             = fm10000GetParityErrorCounters,
    .ResetParityErrorCounters           = fm10000ResetParityErrorCounters,
    .SetParityAttribute                 = fm10000SetParityAttribute,

    .ParityRepairTask                   = fm10000ParityRepairTask,
    .parityRepairEnabled                = TRUE,

    /**************************************************
     * Miscellaneous Task functions
     **************************************************/
    .FastMaintenanceTask                = fm10000FastMaintenanceTask,

    /* list of cached registers */
    .CachedRegisterList                 = (void *)fm10000CachedRegisterList,

    /***************************************************
     * Mailbox functions
     **************************************************/
    .WriteMailboxResponseMessage         = fm10000WriteResponseMessage,
    .ReadMailboxControlHdr               = fm10000ReadMailboxControlHdr,
    .UpdateMailboxSmHdr                  = fm10000UpdateSmHeader,
    .ValidateMailboxMessageLength        = fm10000ValidateMessageLength,
    .ReadMailboxRequestArguments         = fm10000ReadRequestArguments,
    .SetMailboxLportGlortRange           = fm10000SetLportGlortRange,
    .SetHostSrvErrResponse               = fm10000SetHostSrvErrResponse,
    .MapPepToLogicalPort                 = fm10000MapPepToLogicalPort,
    .MapLogicalPortToPep                 = fm10000MapLogicalPortToPep,
    .ProcessMailboxLoopbackRequest       = fm10000ProcessLoopbackRequest,
    .MapVsiGlortToPepNumber              = fm10000MapGlortToPepNumber,
    .MapVsiGlortToPepLogicalPort         = fm10000MapVsiGlortToLogicalPort,
    .MailboxAllocateDataStructures       = fm10000MailboxAllocateDataStructures,
    .MailboxInit                         = fm10000MailboxInit,
    .MailboxFreeDataStructures           = fm10000MailboxFreeDataStructures,
    .MailboxFreeResources                = fm10000MailboxFreeResources,
    .ProcessCreateFlowTableRequest       = fm10000ProcessCreateFlowTableRequest,
    .GetSchedPortSpeedForPep             = fm10000GetSchedPortSpeedForPep,
    .FindInternalPortByMailboxGlort      = fm10000FindInternalPortByMailboxGlort,
    .SetXcastFlooding                    = fm10000SetXcastFlooding,
    .AssociateMcastGroupWithFlood        = fm10000AssociateMcastGroupWithFlood,
    .GetMailboxGlortRange                = fm10000GetMailboxGlortRange,

};  /* end FM10000SwitchDefaultTable */


/*****************************************************************************
 * Local Function Prototypes
 *****************************************************************************/

static fm_status InitCardinalPortMap(fm_switch *switchPtr);
static void GetIMProperties(fm_int sw);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** InitCardinalPortMap
 * \ingroup intSwitch
 *
 * \desc            Initializes the cardinal port map.
 *
 * \param[in]       switchPtr is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitCardinalPortMap(fm_switch *switchPtr)
{
    fm_cardinalPortInfo *cardinalPortInfo;
    fm_status            status;
    fm_int               cpi;
    fm_int               logPort;
    fm_int               logSwitch;
    fm_int               physPort;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "switchPtr=%p<%d>\n",
                 (void *) switchPtr,
                 switchPtr->switchNumber);

    cardinalPortInfo = &switchPtr->cardinalPortInfo;

    /***************************************************
     * Allocate the cardinal port map.
     **************************************************/

    status = fmAllocCardinalPortMap(switchPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    /***************************************************
     * Initialize the cardinal port map.
     **************************************************/

    cpi = 0;

    /* Enumerate the physical ports. */
    for (physPort = 0 ; physPort <= switchPtr->maxPhysicalPort ; physPort++)
    {
        status = fmPlatformMapPhysicalPortToLogical(switchPtr->switchNumber,
                                                    physPort,
                                                    &logSwitch,
                                                    &logPort);

        if (status != FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                         "physical port %d not defined, skipped\n",
                         physPort);
            continue;
        }

        if (logPort > cardinalPortInfo->maxLogicalPort)
        {
            cardinalPortInfo->maxLogicalPort = logPort;
        }

        cardinalPortInfo->portMap[cpi].logPort  = logPort;
        cardinalPortInfo->portMap[cpi].physPort = physPort;
        cpi++;
    }

    switchPtr->numCardinalPorts = cpi;

    /***************************************************
     * Order cardinal ports by logical port number.
     **************************************************/

    qsort(cardinalPortInfo->portMap,
          switchPtr->numCardinalPorts,
          sizeof(fm_cardinalPort),
          fmCompareCardinalPorts);

    /***************************************************
     * Create the cardinal port index table.
     **************************************************/

    status = fmCreateCardinalPortIndexTable(switchPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    /***************************************************
     * Set maximum reserved port number.
     **************************************************/

#if defined(FM_PLATFORM_MAX_RESERVED_PORT)
    switchPtr->maxReservedPort = FM_PLATFORM_MAX_RESERVED_PORT;
#else
    switchPtr->maxReservedPort = switchPtr->maxPhysicalPort;
#endif

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end InitCardinalPortMap */




/*****************************************************************************/
/** InitSpecialPort
 * \ingroup intSwitch
 *
 * \desc            Initializes one of the special logical ports.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       port is the port number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitSpecialPort(fm_int sw, fm_int port)
{
    fm_portmask destMask;
    fm_switch * switchPtr;
    fm_port *   portPtr;
    fm_int      cpi;
    fm_int      portNumber;
    fm_status   err;
    fm_bool     isPciePort;
    fm_bool     addPepsToFlooding;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d port=%d\n", sw, port);

    switchPtr = GET_SWITCH_PTR(sw);

    /*
     * Initialize destination mask according to special port type.
     * This is also where we screen for unsupported special ports.
     */
    switch (port)
    {
        case FM_PORT_BCAST:
        case FM_PORT_MCAST:
        case FM_PORT_FLOOD:
            fmPortMaskEnableAll(&destMask, switchPtr->numCardinalPorts);

            addPepsToFlooding =
                fmGetBoolApiProperty(FM_AAK_API_PORT_ADD_PEPS_TO_FLOODING,
                                     FM_AAD_API_PORT_ADD_PEPS_TO_FLOODING);

            for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
            {
                /* Get logical port number and its pointer. */
                portNumber = GET_LOGICAL_PORT(sw, cpi);
                portPtr = GET_PORT_PTR(sw, portNumber);
               
                err = fmIsPciePort(sw, portNumber, &isPciePort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
 
                /* Now exclude the unwanted ports in destMask. */
                if ( (portPtr->portType == FM_PORT_TYPE_CPU) ||
                     (portPtr->portType == FM_PORT_TYPE_TE) ||
                     (portPtr->portType == FM_PORT_TYPE_CPU_MGMT2) ||
                     (portPtr->portType == FM_PORT_TYPE_LOOPBACK) ||
                     ( (isPciePort == TRUE) && (addPepsToFlooding == FALSE) ) )
                {
                    FM_PORTMASK_DISABLE_BIT(&destMask, portNumber);
                }
            }
            break;

        case FM_PORT_RPF_FAILURE:
            fmPortMaskEnableAll(&destMask, switchPtr->numCardinalPorts);

            for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
            {
                /* Get logical port number and its pointer. */
                portNumber = GET_LOGICAL_PORT(sw, cpi);
                portPtr = GET_PORT_PTR(sw, portNumber);

                /* Now exclude the unwanted ports in destMask. */
                if ( (portPtr->portType == FM_PORT_TYPE_TE) ||
                     (portPtr->portType == FM_PORT_TYPE_CPU_MGMT2) ||
                     (portPtr->portType == FM_PORT_TYPE_LOOPBACK) )
                {
                    FM_PORTMASK_DISABLE_BIT(&destMask, portNumber);
                }
            }
            break;

        case FM_PORT_DROP:
        case FM_PORT_NOP_FLOW:
            /* Do not send to any ports. */
            FM_PORTMASK_DISABLE_ALL(&destMask);
            break;

        default:
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                         "Unsupported special port (%d)\n",
                         port);
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_UNSUPPORTED);
    }

    /* Create logical port structure and assign hardware resources. */
    err = fm10000AllocLogicalPort(sw, FM_PORT_TYPE_SPECIAL, 1, &port, 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Set destination mask for port. */
    err = fm10000SetLogicalPortAttribute(sw, port, FM_LPORT_DEST_MASK, &destMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                 "Initialized special %s port %d\n",
                 fmSpecialPortToText(port),
                 port);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end InitSpecialPort */




/*****************************************************************************/
/** InitPortQoS
 * \ingroup intSwitch
 *
 * \desc            Initializes QoS on the cardinal ports.
 *                  Called by fm10000InitializeAfterReset.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
static fm_status InitPortQoS(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_port *       portPtr;
    fm_status       err;
    fm_int          cpi;
    fm_int          port;
    fm_int          index;
    fm_int          vpriCfi;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = FM_OK;

    for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        port    = GET_LOGICAL_PORT(sw, cpi);
        portPtr = switchPtr->portTable[port];

        if (!portPtr)
        {
            FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                         "Port pointer for port %d is NULL\n",
                         port);

            continue;
        }

        for (index = 0 ; index < FM_MAX_VLAN_PRIORITIES ; index++)
        {
            /***************************************************
             * This initializes the priority map so that a
             * default of <priority, cfi> egresses as
             * <priority, cfi>
             *
             * Maps VPRI into SWPRI so that CFI is ignored in VPRI
             **************************************************/
            vpriCfi = index;

            err = portPtr->SetPortQOS(sw,
                                      port,
                                      FM_QOS_RX_PRIORITY_MAP,
                                      index,
                                      &vpriCfi);
            if (err != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Unable to initialize RX priority map: %s\n",
                             fmErrorMsg(err));
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }

            err = portPtr->SetPortQOS(sw,
                                      port,
                                      FM_QOS_TX_PRIORITY_MAP,
                                      index,
                                      &vpriCfi);
            if (err != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Unable to initialize TX priority map: %s\n",
                             fmErrorMsg(err));
            }

            err = portPtr->SetPortQOS(sw,
                                      port,
                                      FM_QOS_TX_PRIORITY2_MAP,
                                      index,
                                      &vpriCfi);
            if (err != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Unable to initialize TX priority2 map: %s\n",
                             fmErrorMsg(err));
            }

            vpriCfi = index >> 1;
            err = switchPtr->SetSwitchQOS(sw,
                                          FM_QOS_VPRI_SWPRI_MAP,
                                          index,
                                          &vpriCfi);
            if (err != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                             "Unable to initialize VPRI SWPRI priority map: %s\n",
                             fmErrorMsg(err));
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
            }
        }

    }   /* end for (cpi = 0 ; cpi < switchPtr->numCardinalPorts ; cpi++) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end InitPortQoS */



/*****************************************************************************/
/** GetIMProperties
 * \ingroup intSwitch
 *
 * \desc            Reads interrupt mask properties to override interrupt
 *                  handler behaviour.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          void
 *
 *****************************************************************************/
static void GetIMProperties(fm_int sw)
{
    fm10000_switch *switchExt;

    switchExt = GET_SWITCH_EXT(sw);

    /* API property interrupt masks to override interrupt handler behaviour */
    switchExt->anImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_AUTONEG_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_AUTONEG_IGNORE_MASK);

    switchExt->linkImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_LINK_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_LINK_IGNORE_MASK);

    switchExt->serdesImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_SERDES_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_SERDES_IGNORE_MASK);

    switchExt->pcieImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_PCIE_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_PCIE_IGNORE_MASK);

    switchExt->maTcnImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_MATCN_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_MATCN_IGNORE_MASK);

    switchExt->fhTailImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_FHTAIL_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_FHTAIL_IGNORE_MASK);

    switchExt->swImProp = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_INTR_SW_IGNORE_MASK,
                            FM_AAD_API_FM10000_INTR_SW_IGNORE_MASK);
}   /* end GetIMProperties */



/*****************************************************************************/
/** fm10000InitializeGlortRanges
 * \ingroup intSwitch
 *
 * \desc            Initializes the glort ranges for the FM10000 switch
 *                  if they have not already been defined. 
 *
 * \param[in]       sw contains the switch number
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
static fm_status fm10000InitializeGlortRanges(fm_int sw)
{
    fm_status      err = FM_OK;
    fm_switch *    switchPtr;
    fm_glortRange *glorts;
    fm_int         numStackMcastGroups;
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    glorts    = &switchPtr->glortRange;

    /***************************************************
     * Initialize the glort ranges if they have not
     * been already initialized.
     **************************************************/

    if (glorts->cpuPortCount == 0)
    {
        glorts->cpuPortCount = FM10000_GLORT_CPU_SIZE;
    }

    if (glorts->glortMask == 0)
    {
        glorts->glortBase = 0;
        glorts->glortMask = 0xffff;
    }

    if (glorts->portBaseGlort == (fm_uint32) ~0)
    {
        glorts->portBaseGlort = FM_GLORT_PORT_BASE(glorts->glortBase);
        glorts->portCount     = FM10000_GLORT_PORT_SIZE;
    }

    glorts->portALength = FM10000_PHYSICAL_PORT_ALENGTH;

    glorts->cpuMgmtGlort = FM10000_GLORT_FIBM(glorts->glortBase);

    if (glorts->lagBaseGlort == (fm_uint32) ~0)
    {
        /* Allocate the LAG glort range from the end. If stacking 
         * is used, the boundary will not have to be calculated, those
         * can start at FM10000_GLORT_LAG_BASE. */
        glorts->lagCount     = FM_NUM_LOCAL_LAGS * FM10000_GLORTS_PER_LAG;
        glorts->lagBaseGlort = (FM10000_GLORT_LAG_BASE + FM10000_GLORT_LAG_SIZE)
                               - glorts->lagCount;

        FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_SWITCH, 
                               glorts->lagCount <= FM10000_GLORT_LAG_SIZE,
                               err = FM_ERR_ASSERTION_FAILED,
                               "Lag glort space (%d) exceeded (%d)\n",
                               FM10000_GLORT_LAG_SIZE,
                               glorts->lagCount);

        /* For FM10000, the glort space allocated for LAG is limited to
         * 0x2000-0x2FFF. Make sure that this will not be exceeded by
         * stacking glort requirements. If this glort space needs to be 
         * exceeded, the default glort range for local lags should be overridden 
         * with fmSetStackGlortRangeExt(). */
        FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_SWITCH, 
                               ((FM_NUM_LOCAL_LAGS + FM_NUM_STACK_LAGS) 
                                * FM10000_GLORTS_PER_LAG) <=
                               (FM10000_GLORT_LAG_SIZE),
                               err = FM_ERR_ASSERTION_FAILED,
                               "Lag glort space (%d) exceeded (%d)\n",
                               FM10000_GLORT_LAG_SIZE,
                               ((FM_NUM_LOCAL_LAGS + FM_NUM_STACK_LAGS) 
                                * FM10000_GLORTS_PER_LAG) );
    }


    if (glorts->lbgBaseGlort == (fm_uint32) ~0)
    {
        glorts->lbgBaseGlort = FM10000_GLORT_LBG_BASE;
        glorts->lbgCount     = FM10000_GLORT_LBG_SIZE;
    }

    if (glorts->mcastBaseGlort == (fm_uint32) ~0)
    {
        numStackMcastGroups =
            fmGetIntApiProperty(FM_AAK_API_FM10000_MCAST_NUM_STACK_GROUPS,
                                FM_AAD_API_FM10000_MCAST_NUM_STACK_GROUPS);

        /* Allocate the local multicast groups glort range from the end.
         * If stacking is used, the boundary will not have to be calculated,
         * those can start at FM10000_GLORT_MCAST_BASE. */
        glorts->mcastBaseGlort =
            FM10000_GLORT_MCAST_BASE + numStackMcastGroups; 
        glorts->mcastCount     =
            FM10000_GLORT_MCAST_SIZE - numStackMcastGroups;

        /* For FM10000, the default glort space allocated for multicasts is
         * 0x3000-0x3FFF. Make sure that this will not be exceeded by
         * stacking glort requirements. If this glort space needs to be 
         * exceeded, the default glort range for local mulitcasts should be
         * overridden with fmSetStackGlortRangeExt(). */
        FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_SWITCH, 
                               numStackMcastGroups <=
                               FM10000_GLORT_MCAST_SIZE,
                               err = FM_ERR_ASSERTION_FAILED,
                               "Mcast glort space (%d) exceeded (%d)\n",
                               FM10000_GLORT_MCAST_SIZE,
                               numStackMcastGroups);
    }

    if (switchPtr->mailboxInfo.glortBase == (fm_uint16) ~0)
    {
        switchPtr->mailboxInfo.glortBase    = FM10000_MAILBOX_GLORT_BASE; 
        switchPtr->mailboxInfo.glortMask    = FM10000_MAILBOX_GLORT_MASK;
        switchPtr->mailboxInfo.glortsPerPep = FM10000_MAILBOX_GLORTS_PER_PEP;
    }

    /***************************************************
     * CPU glort range for the FM10000.
     **************************************************/

    switchPtr->glortInfo.cpuBase        = FM10000_GLORT_CPU_BASE;
    switchPtr->glortInfo.cpuMask        = FM10000_GLORT_CPU_MASK;

    switchPtr->glortInfo.specialBase    = FM10000_GLORT_SPECIAL_BASE;
    switchPtr->glortInfo.specialMask    = FM10000_GLORT_SPECIAL_MASK;
    switchPtr->glortInfo.specialSize    = FM10000_GLORT_SPECIAL_SIZE;
    switchPtr->glortInfo.specialALength = FM10000_GLORT_SPECIAL_A_LENGTH;

    glorts    = &switchPtr->glortRange;

    /*****************************************************************
     * Multicast glort information 
     *  
     * The value must be a a power of 2 greater then 64 but not greater
     * then FM_MCG_MAX_ENTRIES_PER_GLORT
     ****************************************************************/
    switchPtr->mcastMaxEntryPerGlort =
        fmGetIntApiProperty(FM_AAK_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM,
                            FM_AAD_API_FM10000_MCAST_MAX_ENTRIES_PER_CAM);

    switchPtr->mcastMaxEntryPerCam = 0;
    while ((1 << switchPtr->mcastMaxEntryPerCam) < switchPtr->mcastMaxEntryPerGlort)
    {
        switchPtr->mcastMaxEntryPerCam++;
    }

    if ((switchPtr->mcastMaxEntryPerGlort > FM_MCG_MAX_ENTRIES_PER_GLORT) ||
        ((1 << switchPtr->mcastMaxEntryPerCam) != switchPtr->mcastMaxEntryPerGlort) ||
        (switchPtr->mcastMaxEntryPerGlort & 0x3F) != 0)
    {
        switchPtr->mcastMaxEntryPerGlort = FM_MCG_MAX_ENTRIES_PER_GLORT;
        switchPtr->mcastMaxEntryPerCam = FM_MCAST_MAX_ENTRY_PER_CAM;
    }

    switchPtr->mcastMaxDestTableEntries = FM_MAX_NUM_MULTICAST_GROUP;
    if (switchPtr->mcastMaxDestTableEntries > FM10000_GLORT_DEST_TABLE_ENTRIES)
    {
        switchPtr->mcastMaxDestTableEntries = FM10000_GLORT_DEST_TABLE_ENTRIES;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitializeGlortRanges */




/*****************************************************************************/
/** fm10000InitializeAfterReset
 * \ingroup intSwitch
 *
 * \desc            Further initialization steps following a chip reset.
 *                  Called by fm10000BootSwitch.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
static fm_status fm10000InitializeAfterReset(fm_int sw)
{
    fm_switch *     switchPtr;
    fm_status       err;
    fm_int          i;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /***************************************************
     * Initialize glort ranges
     **************************************************/
    err = fm10000InitializeGlortRanges(sw);
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to initialize glort ranges: %s\n",
                     fmErrorMsg(err));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
    /***************************************************
     * Initialize all the cache arrays
     **************************************************/
    err = fm10000InitRegisterCache(sw);
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to initialize register cache: %s\n",
                     fmErrorMsg(err));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize the counter rate monitor subsystem
     **************************************************/
    err = fm10000InitCrm(sw);
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to initialize CRM subsystem: %s\n",
                     fmErrorMsg(err));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize glort cam
     **************************************************/
    err = fm10000InitGlortCam(sw);
    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                      "Unable to initialize glort cam: %s\n",
                      fmErrorMsg(err) );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize logical ports
     **************************************************/
    err = fmInitializeLogicalPorts(sw);
    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                      "Unable to initialize logical port subsystem: %s\n",
                      fmErrorMsg(err) );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize the port tables
     **************************************************/
    err = fm10000InitPortTable(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                      "Unable to initialize port table: %s\n",
                      fmErrorMsg(err) );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize the triggers
     **************************************************/
    err = fm10000InitTriggers(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                      "Unable to initialize triggers: %s\n",
                      fmErrorMsg(err) );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize the storm controllers
     **************************************************/
    err = fm10000InitStormControllers(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                      "Unable to initialize storm controllers: %s\n",
                      fmErrorMsg(err) );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize the MTable structures
     **************************************************/
    err = fm10000MTableInitialize(switchPtr->switchNumber);
    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_SWITCH,
                      "Unable to initialize MTable data structures: %s\n",
                      fmErrorMsg(err) );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize the parity subsystem
     **************************************************/
    err = fm10000InitParity(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH,
                     "Unable to initialize parity subsystem: %s\n",
                     fmErrorMsg(err));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Configure QoS on cardinal ports.
     **************************************************/
    err = InitPortQoS(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Set the MTU size to maximum for all MTU entries.
     **************************************************/
    for (i = 0 ; i < FM10000_MTU_TABLE_ENTRIES ; i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_MTU_TABLE(i), 0xffffffff);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Initialize info struct
     **************************************************/
    err = switchPtr->GetSwitchInfo(sw, &switchPtr->info);
    if (err != FM_OK )
    {
        FM_LOG_FATAL(FM_LOG_CAT_SWITCH, 
                     "Unable to get switch info: %s\n",
                     fmErrorMsg(err));
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
    
}   /* end fm10000InitializeAfterReset */




/*****************************************************************************/
/** InitDefaultAttributes
 * \ingroup intSwitch
 *
 * \desc            Initializes all physical ports and switch to have default 
 *                  attributes.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitDefaultAttributes(fm_int sw)
{
    fm_switch *          switchPtr;
    fm_status            err;
    fm10000_switch *     switchExt;
    fm_int               port;
    fm_int               cpi;
    fm_int               epl;
    fm_int               lane;
    fm_bool              hasEpl;
    fm10000_portAttr *   portAttrExt;
    fm_portAttr *        portAttr;
    fm_bool              isPciePort;
    

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /**********************************************************************
     * The following attributes value differ from hardware defaults. By
     * setting these port attribute, register values are updated as per the
     * API default values.
     **********************************************************************/
    for (cpi = 0; cpi < switchPtr->numCardinalPorts; cpi++)
    {
        port  = GET_LOGICAL_PORT(sw, cpi);

        portAttr    = GET_PORT_ATTR(sw, port);
        portAttrExt = GET_FM10000_PORT_ATTR(sw, port);

        err = fmIsPciePort(sw, port, &isPciePort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
 
        err = fm10000MapLogicalPortToEplLane(sw, port, &epl, &lane);
        hasEpl = (err == FM_OK);

        /* Port attribute value is set in fm10000InitPort based on port type. */
        if (hasEpl)
        {
            err = fm10000SetPortAttribute(sw,
                                          port,
                                          FM_PORT_ACTIVE_MAC,
                                          FM_PORT_LANE_NA,
                                          FM_PORT_MAX_FRAME_SIZE,
                                          &portAttr->maxFrameSize);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        } 

        err = fm10000SetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_PARSER_VLAN1_TAG,
                                      &portAttrExt->parserVlan1Tag);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = fm10000SetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_RX_PAUSE,
                                      &portAttr->rxPause);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = fm10000SetPortAttribute(sw,
                                      port,
                                      FM_PORT_ACTIVE_MAC,
                                      FM_PORT_LANE_NA,
                                      FM_PORT_LOOPBACK_SUPPRESSION,
                                      &portAttr->loopbackSuppression);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        if (isPciePort)
        {
            err = fm10000SetPortAttribute(sw, 
                                          port,
                                          FM_PORT_ACTIVE_MAC, 
                                          FM_PORT_LANE_NA, 
                                          FM_PORT_ISL_TAG_FORMAT,
                                          &portAttr->islTagFormat);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

    err = fm10000UpdateAllSAFValues(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000InitSwitchAttributes(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000ConfigureVlanLearningMode(sw, switchPtr->vlanLearningMode);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);
 
}   /* end InitDefaultAttributes */




/*****************************************************************************/
/** ResetSwitch
 * \ingroup intSwitch
 *
 * \desc            Puts switch fabric/EPL domains into soft reset.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ResetSwitch(fm_int sw)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  rv;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, SwitchReady, 0);

    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* wait 100us */
    fmDelay(0, 100000);

    err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, SwitchReset, 1);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, EPLReset, 1);

    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* wait 1ms */
    fmDelay(0, 1000000);

ABORT:
    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end ResetSwitch */




/*****************************************************************************/
/** ReleaseSwitch
 * \ingroup intSwitch
 *
 * \desc            Takes switch fabric/EPL domains out of reset and initialize
 *                  the associated memories.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReleaseSwitch(fm_int sw)
{
    fm_status err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  rv;
    fm_uint64  rv64;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);

    TAKE_REG_LOCK(sw);

    /* The switch is assumed to be in reset */

    /******************************** 
     * Clear the switch/tunnel/EPL memories
     ********************************/
    err = switchPtr->ReadUINT64(sw, FM10000_BIST_CTRL(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_FABRIC, 1);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_TUNNEL, 1);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_EPL, 1);
    
    err = switchPtr->WriteUINT64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Intentionnaly sequenced after setting BistMode_FABRIC */
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_FABRIC, 1);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_TUNNEL, 1);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_EPL, 1);

    err = switchPtr->WriteUINT64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* wait 0.8ms */
    fmDelay(0, 800000);

    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_FABRIC, 0);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_TUNNEL, 0);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistRun_EPL, 0);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_FABRIC, 0);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_TUNNEL, 0);
    FM_SET_BIT64(rv64, FM10000_BIST_CTRL, BistMode_EPL, 0);
    
    err = switchPtr->WriteUINT64(sw, FM10000_BIST_CTRL(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /******************************** 
     * Take the switch out of reset 
     ********************************/
    err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, SwitchReset, 0);
    FM_SET_BIT(rv, FM10000_SOFT_RESET, EPLReset, 0);

    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* wait 100ns */
    fmDelay(0, 100);

ABORT:
    DROP_REG_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end ReleaseSwitch */




/*****************************************************************************/
/** FindFabricPllParams
 * \ingroup intSwitch
 *
 * \desc            Find the PLL parameters that gives the closest frequency
 *                  for the given frame handler clock. The resulting
 *                  clock frequency is rounded to the nearest upper clock value. 
 *
 * \param[in]       fhClock is the requested clock in Hz
 * 
 * \param[out]      refDiv is a pointer to the caller allocated storage where
 *                  the refDiv value should be stored.
 * 
 * \param[out]      outDiv is a pointer to the caller allocated storage where
 *                  the outDiv value should be stored.
 * 
 * \param[out]      fbDiv4 is a pointer to the caller allocated storage where
 *                  the fbDiv4 value should be stored.
 * 
 * \param[out]      fbDiv255 is a pointer to the caller allocated storage where
 *                  the fbDiv255 value should be stored.
 * 
 * \param[out]      outFhClock is a pointer to the caller allocated storage where
 *                  the resulting clock value should be stored.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FindFabricPllParams(fm_uint32  fhClock,
                                     fm_uint32 *refDiv,
                                     fm_uint32 *outDiv,
                                     fm_uint32 *fbDiv4,
                                     fm_uint32 *fbDiv255,
                                     fm_uint32 *outFhClock)
{
    fm_status err = FM_OK;
    fm_uint32 refDivA;
    fm_uint32 outDivA;
    fm_uint32 fbDiv4A;
    fm_uint32 fbDiv255A;
    fm_uint64 freq;
    fm_uint64 vco;
    fm_uint64 closestVco;
    fm_uint64 closestFreq;
    fm_bool   perfectMatch;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "fhClock=%d\n", fhClock);

    /* Store the best results in the output variables */
    *refDiv = 0;
    *outDiv = 0;
    *fbDiv4 = 0;
    *fbDiv255 = 0;

    closestFreq = 0xFFFFFFFFFFFFFFFF;
    closestVco  = 0;
    perfectMatch = FALSE;

    /* Iterate over all possibilities to find the best match. This looping
     * is not very efficient, it takes roughly 300ms to go through all 
     * possibilities.
     *  
     * We could use a pre-computed table of PLL values for
     * frequencies between 350MHz and 1200Mhz with a 1Mhz increment. */
    for (outDivA = 63; outDivA >= 2; outDivA--)
    {
        for (refDivA = 63; refDivA >= 1; refDivA--)
        {
            for (fbDiv4A = 0; fbDiv4A < 2; fbDiv4A++)
            {
                for (fbDiv255A = 1; fbDiv255A < 256; fbDiv255A++)
                {
                    vco = ((fm_uint64)FM10000_ETH_REF_CLOCK_FREQ * 4 * 
                          fbDiv255A * (1 + fbDiv4A)) / refDivA;

                    /* VCO must be in the (8GHz <= VCO <= 12GHz) range. */
                    if (vco < 8e9 || vco > 12e9)
                    {
                        continue;
                    }

                    freq = vco / (2 * outDivA);

                    if ( (freq < MAX_INT_VALUE) &&
                         (freq >= fhClock) &&
                         (freq < closestFreq) )
                    {
                        closestVco  = vco;
                        closestFreq = freq;
                        *refDiv = refDivA;
                        *outDiv = outDivA;
                        *fbDiv4 = fbDiv4A;
                        *fbDiv255 = fbDiv255A;

                        if (freq == fhClock)
                        {
                            /* We found a perfect match, just exit */
                            perfectMatch = TRUE;
                        }
                    }

                    if (perfectMatch)
                    {
                        break;
                    }
                }

                if (perfectMatch)
                {
                    break;
                }
            }

            if (perfectMatch)
            {
                break;
            }
        }

        if (perfectMatch)
        {
            break;
        }
    }

    if (closestFreq < MAX_INT_VALUE)
    {
        *outFhClock = closestFreq;

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "fhClock=%d Hz, outFhClock=%d Hz, "
                     "diff=%d Hz, VCO=%" FM_FORMAT_64 "d\n",
                     fhClock, 
                     *outFhClock, 
                     *outFhClock - fhClock,
                     closestVco);
    }
    else
    {
        /* Got unexpected clock frequency higher than what we can store */
        err = FM_FAIL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end FindFabricPllParams */




/*****************************************************************************/
/** SetFHClockFreq
 * \ingroup intSwitch
 *
 * \desc            Sets the frame handler PLL multipliers to match the
 *                  requested FH clock. Use -1 for default clock.
 * 
 *                  Will be rounded to the closest matching value.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      fhClock is the frame handler clock frequency in Hz.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetFHClockFreq(fm_int sw, fm_int fhClock)
{
    fm_status   err;
    fm_uint32   rv;
    fm_uint32   featureCode;
    fm_uint32   freqSel;
    fm_int      maxFreq;
    fm_uint32   refDiv;
    fm_uint32   outDiv;
    fm_uint32   fbDiv4;
    fm_uint32   fbDiv255;
    fm_uint32   outFhClock;
    fm_bool     regLockTaken;
    fm_int      sku;
    fm_int      skuClock;

    fm_switch * switchPtr = GET_SWITCH_PTR(sw); 

    regLockTaken = FALSE;

    /* Read SKU value from registry */
    err = switchPtr->ReadUINT32(sw, FM10000_FUSE_DATA_0(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (rv == 0)
    {
        sku = NO_SKU;
    }
    else
    {
        sku = FM_GET_FIELD(rv, FM10000_FUSE_DATA_0, Sku);
    }

    /* Evaluate if there are any restrictions */
    err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_LOCK(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    featureCode = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_LOCK, FeatureCode);
    freqSel     = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_LOCK, FreqSel);

    switch(sku)
    {
        case FM10840_SKU:
        case FM10064_SKU:
            refDiv   = 0x1f;
            outDiv   = 0x5;
            fbDiv4   = 0x1;
            fbDiv255 = 0xf3;
            skuClock = BINARY_980_MHZ;
            break;
        case FM10824_SKU:
        case NO_SKU:
        default:
            refDiv   = 0x3;
            outDiv   = 0x7;
            fbDiv4   = 0x0;
            fbDiv255 = 0x2f;
            skuClock = BINARY_700_MHZ;
            break;
    }

    /* Full control over the PLL */
    if (featureCode == FM10000_PFL_FEATURE_CODE_FULL)
    {
        if (fhClock == -1)
        {
            outFhClock = (fm_uint32)skuClock;
        }
        else
        {
            err = FindFabricPllParams(fhClock, 
                                      &refDiv, 
                                      &outDiv, 
                                      &fbDiv4, 
                                      &fbDiv255, 
                                      &outFhClock);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

            if(outFhClock > (fm_uint32)skuClock)
            {
                FM_LOG_WARNING(FM_LOG_CAT_SWITCH,
                               "fhClock=%d is greater than maximum SKU frequency %d, using %d\n",
                               fhClock,
                               skuClock,
                               skuClock);

                outFhClock = skuClock;
            }
        }

        FM_FLAG_TAKE_REG_LOCK(sw);
        
        err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_CTRL(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_SET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, RefDiv, refDiv);
        FM_SET_BIT  (rv, FM10000_PLL_FABRIC_CTRL, FbDiv4, fbDiv4);
        FM_SET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, FbDiv255, fbDiv255);
        FM_SET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, OutDiv, outDiv);

        err = switchPtr->WriteUINT32(sw, FM10000_PLL_FABRIC_CTRL(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* Toggle Nreset */
        FM_SET_BIT  (rv, FM10000_PLL_FABRIC_CTRL, Nreset, 0);
        err = switchPtr->WriteUINT32(sw, FM10000_PLL_FABRIC_CTRL(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* wait 500ns */
        fmDelay(0, 500);

        FM_SET_BIT  (rv, FM10000_PLL_FABRIC_CTRL, Nreset, 1);
        err = switchPtr->WriteUINT32(sw, FM10000_PLL_FABRIC_CTRL(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* wait 1ms for PLL to lock*/
        fmDelay(0, 1000000);

        err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_LOCK(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_SET_FIELD(rv, 
                     FM10000_PLL_FABRIC_LOCK, 
                     FreqSel, 
                     FM10000_PFL_FREQ_SEL_USE_CTRL);

        err = switchPtr->WriteUINT32(sw, FM10000_PLL_FABRIC_LOCK(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_FLAG_DROP_REG_LOCK(sw);
    }
    else
    {
        switch (featureCode)
        {
            case FM10000_PFL_FEATURE_CODE_LIMITED1:
                maxFreq = 600000000;
                break;

            case FM10000_PFL_FEATURE_CODE_LIMITED2:
                maxFreq = 500000000;
                break;

            case FM10000_PFL_FEATURE_CODE_LIMITED3:
                maxFreq = 400000000;
                break;

            case FM10000_PFL_FEATURE_CODE_LIMITED4:
                maxFreq = 300000000;
                break;

            default:
                /* Should never get here */
                maxFreq = 600000000;
                break;
        }

        if(maxFreq > skuClock)
        {
            maxFreq = skuClock;
        }
        if (fhClock > (maxFreq))
        {
            FM_LOG_WARNING(FM_LOG_CAT_SWITCH, 
                           "fhClock=%d is greater than maximum supported frequency %d, using %d\n",
                           fhClock, 
                           maxFreq,
                           maxFreq);

            fhClock = maxFreq;
        }
        else if (fhClock == -1)
        {
            /* Default to highest supported clock */
            fhClock = maxFreq;
        }

        if (fhClock >= 600000000)
        {
            freqSel = FM10000_PFL_FREQ_SEL_F600;
        }
        else if (fhClock >= 500000000)
        {
            freqSel = FM10000_PFL_FREQ_SEL_F500;
        }
        else if (fhClock >= 400000000)
        {
            freqSel = FM10000_PFL_FREQ_SEL_F400;
        }
        else
        {
            freqSel = FM10000_PFL_FREQ_SEL_F300;
        }

        FM_FLAG_TAKE_REG_LOCK(sw);
        
        err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_LOCK(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_SET_FIELD(rv, FM10000_PLL_FABRIC_LOCK, FreqSel, freqSel);

        err = switchPtr->WriteUINT32(sw, FM10000_PLL_FABRIC_LOCK(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_FLAG_DROP_REG_LOCK(sw);
    }

ABORT:
    if (regLockTaken)
    {
        DROP_REG_LOCK(sw);
    }

    return err;
    
}   /* end SetFHClockFreq */




/*****************************************************************************/
/** ResetSwitchExtension
 * \ingroup intSwitch
 *
 * \desc            Restores the switch extension data structure to its
 *                  initial (power-on) state.
 *
 * \param[in]       switchPtr points to the switch state table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ResetSwitchExtension(fm_switch * switchPtr)
{
    fm10000_switch *switchExt;

    if (switchPtr == NULL)
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    switchExt = switchPtr->extension;

    if (switchExt)
    {
        /* Zero the switch extension structure. */
        FM_CLEAR(*switchExt);

        /* Restore pointer to base structure. */
        switchExt->base = switchPtr;
    }

    return FM_OK;

}   /* end ResetSwitchExtension */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000AllocateDataStructures
 * \ingroup intSwitch
 *
 * \desc            Allocates data structures upon switch insertion.
 *
 * \param[in]       switchPtr points to the switch state table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AllocateDataStructures(fm_switch *switchPtr)
{
    fm_status err;
    fm_int    sw;

    FM_LOG_ENTRY( FM_LOG_CAT_SWITCH,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  switchPtr != NULL ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    sw = switchPtr->switchNumber;

    err = fmAllocateLogicalPortDataStructures(sw,
                                              FM10000_GLORT_CAM_ENTRIES,
                                              FM10000_GLORT_DEST_TABLE_ENTRIES);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmMailboxAllocateDataStructures(sw); 
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fm10000AllocateDataStructures */




/*****************************************************************************/
/** fm10000FreeDataStructures
 * \ingroup intSwitch
 *
 * \desc            Frees data structures upon switch removal.
 *                  Called through the FreeDataStructures function pointer.
 *
 * \param[in]       switchPtr points to the switch state table
 *
 * \return          FM_OK on success.
 *
 *****************************************************************************/
fm_status fm10000FreeDataStructures(fm_switch *switchPtr)
{
    fm_status       err;
    fm10000_switch *switchExt;

    FM_LOG_ENTRY( FM_LOG_CAT_SWITCH,
                 "switchPtr=%p <sw=%d>\n",
                 (void *) switchPtr,
                 (switchPtr != NULL) ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    switchExt = (fm10000_switch *) switchPtr->extension;

    err = fmFreeLogicalPortDataStructures(switchPtr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Free Mailbox Resources */
    err = fmMailboxFreeDataStructures(switchPtr->switchNumber);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing mailbox data structures: %s\n",
                      fmErrorMsg(err) );
        /* Don't return, just continue on */
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fm10000FreeDataStructures */




/*****************************************************************************/
/** fm10000FreeResources
 * \ingroup intSwitch
 *
 * \desc            This is called before the switch is brought down, to free 
 *                  the switch resources pre-allocated when the switch is
 *                  brought up. Called through the FreeResources function
 *                  pointer.
 *
 * \param[in]       sw contains the switch number
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
fm_status fm10000FreeResources(fm_int sw)
{
    fm_status  err;
    fm_status  retErr;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    err    = FM_OK;
    retErr = FM_OK;

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCancelMacTableFlushRequests(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error canceling MAC Table flush requests: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000QOSPriorityMapperFreeResources(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing QOS Priority Mapper resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000AclFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing ACL resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000TunnelFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing Tunnel resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000NatFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing Nat resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free MAC Security before Triggers. */
    err = fm10000FreeMacSecurity(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing MAC security resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free Flooding resources before Triggers. */
    err = fm10000FreeFlooding(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing Flooding resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free SFlow resources before Triggers. */
    err = fm10000FreeSFlows(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing SFlow resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }
   
    /* Free Mailbox resources before routing and multicast */
    err = fm10000MailboxFreeResources(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing mailbox resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

 
    /* Free router resources before Triggers. */
    err = fm10000RouterFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing router resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free NextHop resources before Triggers. */
    err = fm10000NextHopFree(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing NextHop resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000DestroyTriggers(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing trigger resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free multicast resources */
    err = fm10000FreeMcastResource(sw);
    if ( err != FM_OK )
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                     "Fail to cleanup multicast resources: %s\n",
                     fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free MTable resources */
    err = fm10000FreeMTableResources(sw);
    if ( err != FM_OK )
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                     "Fail to cleanup mtable resources: %s\n",
                     fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000FreeLBGResource(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing LBG resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fm10000FreeFlowResource(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing flow resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    err = fmFreeLogicalPortResources(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing logical port resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free register cache */
    err = fmFreeRegisterCache(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing register cache: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free Scheduler Resources */
    err = fm10000FreeSchedulerResources(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing scheduler resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free Lane Resources */
    err = fm10000FreeLaneResources(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing lane resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Free Parity Resources */
    err = fm10000FreeParityResources(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error freeing parity resources: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    /* Reset the switch extension. */
    err = ResetSwitchExtension(switchPtr);
    if (err != FM_OK)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                      "Error resetting switch extension: %s\n",
                      fmErrorMsg(err) );
        retErr = err;
        /* Don't return, just continue on */
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, retErr);

}   /* end fm10000FreeResources */




/*****************************************************************************/
/** fm10000BootSwitch
 * \ingroup intSwitch
 *
 * \desc            Performs the auto-boot process on a chip.
 *                  Called by fm10000SetSwitchState.
 *
 * \param[in]       sw contains the switch number
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
fm_status fm10000BootSwitch(fm_int sw)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm10000_switch *    switchExt;
    fm_uint32           rv;
    fm_uint32           regAddr;
    fm_uint32           pcieEnable;
    fm_int              i;
    fm_uint32           rvMult[4] = {0,0,0,0};
    fm_bool             regLockTaken;
    fm_char             buf[MAX_BUF_SIZE+1];
    fm_int              fhClock;
#if 0
    fm_timestamp t1;
    fm_timestamp t2;
    fm_timestamp tDiff;

    fmGetTime(&t1);
#endif 
    
    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    err       = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    regLockTaken = FALSE;

#if defined(FV_CODE) || defined (FAST_API_BOOT)
    FM_LOG_PRINT("####################################\n");
    FM_LOG_PRINT("# \n");
    FM_LOG_PRINT("# WARNING: USING FAST_API_BOOT (for DEBUG)\n");
    FM_LOG_PRINT("# \n");
    FM_LOG_PRINT("####################################\n");
#endif

    /***************************************************
     * Step 0a: Put the switch domain into reset. 
     *          We may be recovering from a bad state
     *          so we must enforce reset state here
     *          such that we have a clean start.
     **************************************************/
    err = ResetSwitch(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Step 0b: We are in reset, let's setup the 
     *          FABRIC_PLL based on requested
     *          frame handler clock.
     **************************************************/
   
    /* Retrieve the clock from platform config attribute */
    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_FH_CLOCK, 
                  sw);
    fhClock = fmGetIntApiProperty(buf, FM_AAD_API_PLATFORM_FH_CLOCK);

    if ( ( (fhClock < 0) && (fhClock != -1)) || 
         (fhClock > MAX_INT_VALUE) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH, 
                     FM_AAK_API_PLATFORM_FH_CLOCK " is out of range: [-1, %d]\n",
                     sw, 
                     MAX_INT_VALUE);

        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Configure the clock */
    err = SetFHClockFreq(sw, fhClock);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    /***************************************************
     * Step 1a: Determine SerDes OpMode 
     *          This MUST be done before initializing
     *          the SBus and SerDes
     **************************************************/
    err = fm10000SerdesInitOpMode(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    /***************************************************
     * Step 1b: Initialize switch SBus (EPL ring only)
     **************************************************/
    err = fm10000SbusInit(sw, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    /***************************************************
     * Step 1c: Initialize switch SERDES
     **************************************************/
    err = fm10000InitSwSerdes(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
   
    /***************************************************
     * Step 2: Take EPLs out of reset
     **************************************************/

    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, EPLReset, 0);

    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_FLAG_DROP_REG_LOCK(sw);

    /***************************************************
     * Step 3: Take the switch out of reset.
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                 "Taking switch fabric domain %d out of reset... \n", sw);

    err = ReleaseSwitch(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    if (fmGetBoolApiProperty(FM_AAK_API_PLATFORM_IS_WHITE_MODEL,
                             FM_AAD_API_PLATFORM_IS_WHITE_MODEL))
    {
        /* Must be done after ReleaseSwitch because the model starts changing
         * SOFT_RESET during its init */
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, 
                     "Setting PCIeReset to ~PCIeEnable ... \n");

        /* Need to take PCIe devices out of warm reset */
        err = switchPtr->ReadUINT32(sw, FM10000_DEVICE_CFG(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        
        pcieEnable = FM_GET_UNNAMED_FIELD(rv, 
                                          FM10000_DEVICE_CFG_b_PCIeEnable_0,
                                          FM10000_NUM_PEPS);

        err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
      
        /* PCIeReset and PCIeEnable are going to be the same value */
        FM_SET_FIELD(rv, FM10000_SOFT_RESET, PCIeReset, ~pcieEnable);

        err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /* Configure the clock (because the Release Switch cleared
         * register values) */
        err = SetFHClockFreq(sw, fhClock);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Step 4: Disable scan
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Disabling SCAN...\n");

    rv = 0;
    FM_SET_BIT(rv, FM10000_SCAN_DATA_IN, UpdateNodes, 1);
    FM_SET_BIT(rv, FM10000_SCAN_DATA_IN, Passthru, 1);

    err = switchPtr->WriteUINT32(sw, FM10000_SCAN_DATA_IN(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Step 5: Disable switch loopbacks
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Disabling switch loopbacks...\n");

    FM_FLAG_TAKE_REG_LOCK(sw);

    /* Disable loopback on EPLs */ 
    for (i = 0; i < FM10000_NUM_EPLS; i++)
    {
        regAddr = FM10000_EPL_CFG_A(i);

        err = switchPtr->ReadUINT32(sw, regAddr, &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        FM_SET_FIELD(rv, FM10000_EPL_CFG_A, Active, 1);

        err = switchPtr->WriteUINT32(sw, regAddr, rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Do not disable loopback on TEs here - it will be done in the TE
     * init function. */

    FM_FLAG_DROP_REG_LOCK(sw);

    /***************************************************
     * Step 6a: Initialize CM
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing CM...\n");

    err = fm10000InitializeCM(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Step 6b: TCN FIFO Interrupts
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing TCN FIFO Interrupts...\n");
    err = switchPtr->WriteUINT32(sw, FM10000_MA_TCN_IM(), 0);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    rv = 0;
    FM_SET_BIT(rv, FM10000_FH_TAIL_IM, TCN, 1);
    err = switchPtr->MaskUINT32(sw, FM10000_FH_TAIL_IM(), rv, FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Step 7: Initialize the scheduler
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing the scheduler...\n");

    err = fm10000InitScheduler(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Step 8: Initialize LED Controller
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Initializing LED Controller...\n");

    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_LED_CFG(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT(rv, FM10000_LED_CFG, LEDEnable, 1);

    err = switchPtr->WriteUINT32(sw, FM10000_LED_CFG(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_FLAG_DROP_REG_LOCK(sw);

    /***************************************************
     * Step 9: Assert Switch Ready
     **************************************************/
    FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Marking switch as ready...\n");

    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_BIT(rv, FM10000_SOFT_RESET, SwitchReady, 1);

    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_FLAG_DROP_REG_LOCK(sw);
    
    /***************************************************
     * Step 10: Proceed with initialization per block
     **************************************************/
    err = fm10000InitializeAfterReset(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if 0
    fmGetTime(&t2);

    fmSubTimestamps(&t2, &t1, &tDiff);

    FM_LOG_PRINT("fm10000BootSwitch:    %d.%d sec\n", (fm_uint)tDiff.sec, (fm_uint)tDiff.usec/1000);
#endif

ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000BootSwitch */




/*****************************************************************************/
/** fm10000GetSwitchPartNumber
 * \ingroup intSwitch
 *
 * \desc            Retrieve the part number from the switch
 *
 * \param[in]       sw is the switch for which to retrieve information.
 *
 * \param[out]      pn points to the caller-allocated storage where
 *                  the part number should be stored
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetSwitchPartNumber(fm_int sw, fm_switchPartNum *pn)
{
    fm_status status;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d pn=%p\n", sw, (void *) pn);

    status = fmPlatformGetSwitchPartNumber(sw, pn);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fm10000GetSwitchPartNumber */




/*****************************************************************************/
/** fm10000IdentifySwitch
 * \ingroup intSwitch
 *
 * \desc            This function attempts to determine if a chip is an 
 *                  FM10xxx device.
 *
 * \param[in]       sw contains the switch number
 *
 * \param[out]      family contains the switch family
 *                  or FM_SWITCH_FAMILY_UNKNOWN
 *
 * \param[out]      model contains the switch model or FM_SWITCH_MODEL_UNKNOWN
 *
 * \param[out]      version contains the switch version
 *                  or FM_SWITCH_VERSION_UNKNOWN
 *
 * \return          FM_OK
 *
 *****************************************************************************/
fm_status fm10000IdentifySwitch(fm_int            sw,
                                fm_switchFamily * family,
                                fm_switchModel *  model,
                                fm_switchVersion *version)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_uint32  partNumber;
    fm_uint32  vpd;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "sw=%d family=%p model=%p version=%p\n", 
                 sw,
                 (void *) family,
                 (void *) model,
                 (void *) version);

    *family  = FM_SWITCH_FAMILY_UNKNOWN;
    *model   = FM_SWITCH_MODEL_UNKNOWN;
    *version = FM_SWITCH_VERSION_UNKNOWN;

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    status = switchPtr->ReadUINT32(sw, FM10000_VITAL_PRODUCT_DATA(), &vpd);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);
    
    partNumber = FM_GET_FIELD(vpd, 
                              FM10000_VITAL_PRODUCT_DATA,
                              PartNumber);

    if (partNumber == 0xAE21)
    {
        *family  = FM_SWITCH_FAMILY_FM10000;
        *model   = FM_SWITCH_MODEL_FM10440;
        *version = FM_SWITCH_VERSION_FM10440_A0;

        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Identified 10xxx series device\n");
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

}   /* end fm10000IdentifySwitch */




/*****************************************************************************/
/** fm10000InitSwitch
 * \ingroup intSwitch
 *
 * \desc            Performs FM10000-specific switch state initialization.
 *                  Called through the InitSwitch function pointer.
 *
 * \param[in]       switchPtr points to the switch state table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitSwitch(fm_switch *switchPtr)
{
    fm_status       status;
    fm10000_switch *extension;
    fm_int          msiPep;
    fm_uint64       rv64;
    fm_uint32       rv;
    fm_uint32       devCfg;
    fm_uint32       softReset;
    fm_bool         regLockTaken;
    fm_int          pepId;
    fm_int          epl;
    fm_char         buf[MAX_BUF_SIZE+1];
    fm_int          msiEnabled;
    fm_int          sw;

    FM_LOG_ENTRY( FM_LOG_CAT_SWITCH,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  (switchPtr) ? switchPtr->switchNumber : -1 );

    if (switchPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_ARGUMENT);
    }

    sw = switchPtr->switchNumber;

    /* The extension must not already exist */
    if (switchPtr->extension != NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH,
                     "Switch %d extension not NULL on entry: %p\n", 
                     sw, switchPtr->extension);
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_FAIL);
    }

    /* allocate storage for switch-specific state/configuration information */
    extension = (fm10000_switch *) fmAlloc(sizeof(fm10000_switch));
    if (extension == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
    }

    /* initialize the switch-specific area */
    memset(extension, 0, sizeof(fm10000_switch));
    extension->base = switchPtr;

    /* Save the pointer in the base switch state table. */
    switchPtr->extension = extension;

    /* Initialize the number of portSets */
    switchPtr->maxPortSets =
        fmGetIntApiProperty(FM_AAK_API_MAX_PORT_SETS,
                            FM_AAD_API_MAX_PORT_SETS);

    /* Initially there is no owner of ethernet timestamps. */
    extension->ethTimestampsOwnerPort = -1;

    /* Initialize rx frame dropping on unknown port */
    extension->dropPacketUnknownPort = TRUE;

    /*****************************************************
     * register all known state machine types. NOTE: 
     * all registration functions handle gracefully the 
     * case where the same state machine type is 
     * registered multiple times. Therefore the 
     * registrations below are safe when multiple switches 
     * of the same type exist and across reset cycles 
     * of the same switch (the state machines types aren't 
     * unregistered when a switch is reset) 
     *****************************************************/

    /* port level state machines */
    status = fm10000RegisterBasicPortStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterPciePortStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterAnPortStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterClause37AnStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterClause73AnStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );


    /* serdes-level state machine types. The API attribute below
       is used to determine whether or not we want to use the
       stub SerDes state machine for port-level debugging */
    status = fm10000RegisterStubSerDesStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterBasicSerDesStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterBasicSerDesDfeStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    status = fm10000RegisterPcieSerDesStateMachine();
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_SWITCH, status );

    /* Initialize the cardinal port map. */
    status = InitCardinalPortMap(switchPtr);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    switchPtr->ReadUINT32(sw, FM10000_VITAL_PRODUCT_DATA(), &rv);
    if (rv != 0xAE21)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH, 
              "Register access to switch#%d is lost. "
              "VPD = 0x%x\n", sw, rv);
        return FM_FAIL;
    }

    /* Mask out the all the unwanted interrupts, all interrupts execpt BSM */
    FM_FLAG_TAKE_REG_LOCK(sw);

    status = switchPtr->ReadUINT32(sw, FM10000_BSM_SCRATCH(1), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);  

    /* Clear API NVM lock taken by previous API exiting not cleanly */
    if (FM_GET_UNNAMED_FIELD(rv, 0, 2) == 2)
    {
        switchPtr->WriteUINT32(sw, FM10000_BSM_SCRATCH(1), 0);
    }

    status = switchPtr->ReadUINT64(sw, FM10000_INTERRUPT_MASK_FIBM(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);                            
                                                                               
    FM_SET_UNNAMED_FIELD64(rv64,                                               
                           FM10000_INTERRUPT_MASK_FIBM_b_PCIE_0,                                   
                           FM10000_INTERRUPT_MASK_FIBM_b_BSM -                                     
                           FM10000_INTERRUPT_MASK_FIBM_b_PCIE_0 + 1,                               
                           0x1FFFFFFFF);                                       
                                                                               
    status = switchPtr->WriteUINT64(sw, FM10000_INTERRUPT_MASK_FIBM(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);                            

    /* FM10000_INTERRUPT_MASK_INT */
    status = switchPtr->ReadUINT64(sw, FM10000_INTERRUPT_MASK_INT(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);                            
                                                                               
    FM_SET_UNNAMED_FIELD64(rv64,                                               
                           FM10000_INTERRUPT_MASK_INT_b_PCIE_0,                                   
                           FM10000_INTERRUPT_MASK_INT_b_BSM -                                     
                           FM10000_INTERRUPT_MASK_INT_b_PCIE_0 + 1,                               
                           0x1FFFFFFFF);                                       
    status = switchPtr->WriteUINT64(sw, FM10000_INTERRUPT_MASK_INT(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);                            

    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        /* FM10000_INTERRUPT_MASK_PCIE[pepId] */
        status = switchPtr->ReadUINT64(sw,
                                       FM10000_INTERRUPT_MASK_PCIE(pepId, 0),
                                       &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);                            
                                                                                   
        FM_SET_UNNAMED_FIELD64(rv64,                                               
                               FM10000_INTERRUPT_MASK_PCIE_b_PCIE_0,                                   
                               FM10000_INTERRUPT_MASK_PCIE_b_BSM -                                     
                               FM10000_INTERRUPT_MASK_PCIE_b_PCIE_0 + 1,                               
                               0x1FFFFFFFF);                                       
                                                                                   
        status = switchPtr->WriteUINT64(sw,
                                        FM10000_INTERRUPT_MASK_PCIE(pepId, 0),
                                        rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);                            
    }
    
    FM_FLAG_DROP_REG_LOCK(sw);

    /* Clear and mask software interrupts */
    status = switchPtr->WriteUINT32(sw, 
                                    FM10000_SW_IM(), 
                                    0xffffffff);
    status = switchPtr->WriteUINT32(sw,
                                    FM10000_SW_IP(),
                                    0x0);

    /* Select the desired interrupt mask register */
    FM_SNPRINTF_S(buf, 
                  MAX_BUF_SIZE, 
                  FM_AAK_API_PLATFORM_MSI_ENABLED, 
                  sw);
    msiEnabled = fmGetBoolApiProperty(buf, FM_AAD_API_PLATFORM_MSI_ENABLED);

    if (msiEnabled)
    {
        msiPep = switchPtr->msiPep;

        status = switchPtr->ReadUINT64(sw,
                                       FM10000_INTERRUPT_MASK_PCIE(msiPep, 0),
                                       &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

        extension->interruptMaskReg = FM10000_INTERRUPT_MASK_PCIE(msiPep,0);
        extension->interruptMaskValue = rv64;
    }
    else
    {
        status = switchPtr->ReadUINT64(sw, FM10000_INTERRUPT_MASK_INT(0), &rv64);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);   

        extension->interruptMaskReg = FM10000_INTERRUPT_MASK_INT(0);
        extension->interruptMaskValue = rv64;
    }

    /* Set up mask to PEPs */
    status = switchPtr->ReadUINT32(sw, FM10000_DEVICE_CFG(), &devCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    status = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &softReset);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

    for (pepId = 0; pepId < FM10000_NUM_PEPS; pepId++)
    {
        /* Only enable for enabled PEPs */
        if (FM_GET_UNNAMED_FIELD(devCfg, 
                                 FM10000_DEVICE_CFG_b_PCIeEnable_0 + pepId, 
                                 1))
        {
            FM_SET_UNNAMED_FIELD64(extension->interruptMaskValue,
                                   FM10000_INTERRUPT_MASK_INT_b_PCIE_0 + pepId,
                                   1,
                                   0);

            /* Force PEP into active state so we can update its PCIE_IM mask
             * to come out of reset (from API PoV) */
            FM_SET_UNNAMED_FIELD(softReset, 
                                 FM10000_SOFT_RESET_b_PCIeActive_0 + pepId, 
                                 1, 
                                 1);

            status = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), softReset);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

            /* Enable PCIE_IM.DeviceStateChange interrupt */
            rv = 0xFFFFFFFF;

            FM_SET_BIT(rv, FM10000_PCIE_IP, DeviceStateChange, 0);

            status = switchPtr->WriteUINT32(sw, 
                                            FM10000_PCIE_PF_ADDR(FM10000_PCIE_IM(), 
                                                                 pepId), 
                                           rv);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);

            /* Restore PEP state */
            FM_SET_UNNAMED_FIELD(softReset, 
                                 FM10000_SOFT_RESET_b_PCIeActive_0 + pepId, 
                                 1, 
                                 0);

            status = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), softReset);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, status);
        }
    }

    /* This should only enable for EPLs in use */
    for (epl = 0; epl < 9 ; epl++)
    {
        FM_SET_UNNAMED_FIELD64(extension->interruptMaskValue,
                               FM10000_INTERRUPT_MASK_INT_b_EPL_0 + epl,
                               1,
                               0);
    }

    FM_SET_BIT64(extension->interruptMaskValue,
                 FM10000_INTERRUPT_MASK_INT,
                 SOFTWARE,
                 0);

    /* TCN FIFO interrupt detection. */
    FM_SET_BIT64(extension->interruptMaskValue,
                 FM10000_INTERRUPT_MASK_INT,
                 FH_TAIL,
                 0);

    /* Parity error interrupt detection. */
    extension->interruptMaskValue &= ~FM10000_INT_PARITY_DETECT;

    /* Read interrupt masks to override interrupt handler behaviour */
    GetIMProperties(sw);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, status);

}   /* end fm10000InitSwitch */




/*****************************************************************************/
/** fm10000SetSwitchState
 * \ingroup intSwitch
 *
 * \desc            Boots the switch and does any initialization required
 *                  to bring a chip up.  The switch is assumed to be already
 *                  protected, but has not been booted yet.  This function
 *                  should set the switch's up property.
 *                  Called through the SetSwitchState function pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       state indicates whether to set the switch state to up
 *                  (TRUE) or down (FALSE).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_FAIL if general failure accessing switch structures.
 *
 *****************************************************************************/
fm_status fm10000SetSwitchState(fm_int sw, fm_bool state)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d state=%d\n",
                 sw, state);

    switchPtr = GET_SWITCH_PTR(sw);

    if (state)
    {
        err = fm10000BootSwitch(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        /**************************************************
         * Now start configuring switch with defaults.
         **************************************************/
        
        switchPtr->state = FM_SWITCH_STATE_BOOT_DONE;

    }
    else
    {
        /* put the chip back into reset */
        err = switchPtr->ResetSwitch(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_OK);

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000SetSwitchState */




/*****************************************************************************/
/** fm10000InitPortTable
 * \ingroup intSwitch
 *
 * \desc            Gives switch-specific code an opportunity to initialize
 *                  port state tables prior to the individual initialization
 *                  calls for each port.
 *
 * \param[in]       switchPtr points to the switch state table.
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
fm_status fm10000InitPortTable(fm_switch *switchPtr)
{
    fm_status           err = FM_OK;
    fm_int              sw;
    fm_logicalPortInfo *lportInfo;
    fm_lane            *lanePtr;
    fm10000_lane       *laneExtPtr;
    fm10000_laneDfe    *laneDfePtr;
    fm_int              cnt;
    fm_int              size;
    fm_int              portNumber;
    fm_int              portCount;
    fm_portmask         destMask;
    fm_int              cpi;
    fm_uint64           rv64;
    fm_char             serdesTimerName[16];
    fm_char             serdesDfeTimerName[16];

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "switchPtr=%p\n",
                 (void *) switchPtr);

    sw        = switchPtr->switchNumber;
    lportInfo = &switchPtr->logicalPortInfo;

    /***************************************************
     * Allocate and initialize the port state table.
     **************************************************/
    switchPtr->maxPort = FM10000_MAX_PORTS_ALL;
    size = switchPtr->maxPort * sizeof(fm_port *);
    if (!switchPtr->portTable)
    {
        switchPtr->portTable = (fm_port **) fmAlloc(size);

        if (switchPtr->portTable == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }
    }

    memset(switchPtr->portTable, 0, size);

    /***************************************************
     * Initialize (clear) all serDes interrupts.
     **************************************************/
    err = fm10000SerdesClearAllSerDesInterrupts(sw);

    /***************************************************
     * Allocate and initialize the lane state table.
     **************************************************/

    switchPtr->laneTableSize = FM10000_NUM_SERDES;
    size = switchPtr->laneTableSize * sizeof(fm_lane *);
    if (!switchPtr->laneTable)
    {
        switchPtr->laneTable = (fm_lane **) fmAlloc(size);

        if (switchPtr->laneTable == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_NO_MEM);
        }
    }

    memset(switchPtr->laneTable, 0, size);

    for (cnt = 0 ; cnt < switchPtr->laneTableSize ; cnt++)
    {
        lanePtr = fmAlloc(sizeof(fm_lane));
        if (lanePtr == NULL)
        {
            FM_LOG_FATAL( FM_LOG_CAT_PORT,
                          "Unable to allocate memory for lane table entry %d\n", 
                          cnt );
            FM_LOG_EXIT( FM_LOG_CAT_PORT, FM_ERR_NO_MEM );
        }

        memset(lanePtr, 0, sizeof(*lanePtr));
        switchPtr->laneTable[cnt] = lanePtr;

        lanePtr->extension = fmAlloc(sizeof(fm10000_lane));
        if( lanePtr->extension == NULL )
        {
            FM_LOG_FATAL( FM_LOG_CAT_PORT,
                          "Unable to allocate memory for lane "
                          "extension structure %d\n", cnt );
            FM_LOG_EXIT(FM_LOG_CAT_PORT, FM_ERR_NO_MEM);
        }

        /* Initialize the lane extension structure here */
        laneExtPtr       = lanePtr->extension;
        memset(laneExtPtr, 0, sizeof(*laneExtPtr));
        laneExtPtr->base = lanePtr;

        /* Create the state machine associated to this serdes/lane */
        err = fmCreateStateMachine( cnt, 
                                    FM10000_SERDES_SM_HISTORY_SIZE,
                                    FM10000_SERDES_SM_RECORD_SIZE,
                                    &laneExtPtr->smHandle );
        if ( err != FM_OK )
        {
            FM_LOG_ERROR( FM_LOG_CAT_PORT,
                          "Unable to create the state machine for serdes %d\n", 
                          cnt );
            FM_LOG_EXIT( FM_LOG_CAT_PORT, err );
        }

        /* Create the timer(s) associated to this serdes/lane */
        FM_SPRINTF_S( serdesTimerName,  
                      sizeof(serdesTimerName),
                      "SerDes%02dTimer",
                      cnt ); 
        err = fmCreateTimer( serdesTimerName, 
                             fmApiTimerTask, 
                             &laneExtPtr->timerHandle );
        if ( err != FM_OK )
        {
            FM_LOG_ERROR( FM_LOG_CAT_PORT,
                          "Unable to create the timer for serdes %d\n", 
                          cnt );
            FM_LOG_EXIT( FM_LOG_CAT_PORT, err );
        }

        /* initialize the dfe extension structure */
        laneDfePtr = &laneExtPtr->dfeExt;

        laneDfePtr->pLaneExt = laneExtPtr;

        /* create the SerDes-DFE state machine */
        err = fmCreateStateMachine(cnt, 
                                   FM10000_SERDES_DFE_SM_HISTORY_SIZE,
                                   FM10000_SERDES_DFE_SM_RECORD_SIZE,
                                   &laneDfePtr->smHandle);
        if ( err != FM_OK )
        {
            FM_LOG_ERROR( FM_LOG_CAT_PORT,
                          "Unable to create the DFE state machine for serdes %d\n", 
                          cnt );
            FM_LOG_EXIT( FM_LOG_CAT_PORT, err );
        }



        /* Create the timer(s) associated to the dfe state machine */
        FM_SPRINTF_S(serdesDfeTimerName,  
                     sizeof(serdesDfeTimerName),
                     "SerDesDfe%02dTimer",
                     cnt );

        err = fmCreateTimer(serdesDfeTimerName, 
                            fmApiTimerTask, 
                            &laneDfePtr->timerHandle);
        if ( err != FM_OK )
        {
            FM_LOG_ERROR(FM_LOG_CAT_PORT,
                         "Unable to create the timer for serdes %d\n", 
                         cnt );

            FM_LOG_EXIT( FM_LOG_CAT_PORT, err );
        }

        err = fm10000SerdesInitLaneControlStructures(sw, cnt);
        if ( err != FM_OK )
        {
            FM_LOG_ERROR(FM_LOG_CAT_PORT,
                         "Unable to init the lane structure for SerDes %d\n", 
                         cnt );

            FM_LOG_EXIT( FM_LOG_CAT_PORT, err );
        }

    }   /* end for (cnt = 0 ; cnt < switchPtr->laneTableSize ; cnt++ ) */

    /***************************************************
     * Allocate the CPU port.
     **************************************************/

    err = fm10000AllocLogicalPort(sw,
                                  FM_PORT_TYPE_CPU,
                                  1,
                                  &switchPtr->cpuPort,
                                  0);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_PORTMASK_ASSIGN_BIT(&destMask, switchPtr->cpuPort);

    err = fm10000SetLogicalPortAttribute(sw,
                                         switchPtr->cpuPort,
                                         FM_LPORT_DEST_MASK,
                                         &destMask);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_LOG_DEBUG(FM_LOG_CAT_PORT,
                 "Allocated CPU port %d\n",
                 switchPtr->cpuPort);

    /***************************************************
     * Register the CPU trap glort and trap mask
     **************************************************/

    err = switchPtr->WriteUINT32(sw,
                                 FM10000_TRAP_GLORT(),
                                 switchPtr->glortInfo.cpuBase);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000SetCpuPort(sw, switchPtr->cpuPort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Allocate the physical, TE, Loopback and FIBM ports.
     **************************************************/

    if (switchPtr->cpuPort == 0)
    {
        portNumber = 1;
        portCount  = switchPtr->numCardinalPorts - 1;
    }
    else
    {
        portNumber = 0;
        portCount  = switchPtr->numCardinalPorts;
    }

    err = fm10000AllocLogicalPort(sw,
                                  FM_PORT_TYPE_PHYSICAL,
                                  portCount,
                                  &portNumber,
                                  0);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* set the destmask for all cardinal ports */
    for (cpi = 1 ; cpi < switchPtr->numCardinalPorts ; cpi++)
    {
        /* Get logical port number for this cardinal port. */
        portNumber = GET_LOGICAL_PORT(sw, cpi);
        
        /* the set function takes a logical destination mask */
        FM_PORTMASK_ASSIGN_BIT(&destMask, cpi);

        err = fm10000SetLogicalPortAttribute(sw,
                                             portNumber,
                                             FM_LPORT_DEST_MASK,
                                             &destMask);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /***************************************************
     * Allocate a special port for dropping frames.
     **************************************************/

    lportInfo->dropPort = FM_PORT_DROP;

    err = InitSpecialPort(sw, FM_PORT_DROP);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Allocate a special port for handling Reverse Path
     * Forwarding (RPF) failures.
     **************************************************/

    lportInfo->rpfFailurePort = FM_PORT_RPF_FAILURE;

    err = InitSpecialPort(sw, FM_PORT_RPF_FAILURE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Allocate a special port for unicast flooding.
     **************************************************/

    lportInfo->floodPort = FM_PORT_FLOOD;

    err = InitSpecialPort(sw, FM_PORT_FLOOD);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Allocate a special port for multicast flooding.
     **************************************************/

    err = InitSpecialPort(sw, FM_PORT_MCAST);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Allocate a special port for broadcasting.
     **************************************************/

    lportInfo->bcastPort = FM_PORT_BCAST;

    err = InitSpecialPort(sw, FM_PORT_BCAST);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Register the flood and broadcast glorts.
     **************************************************/

    rv64 = 0;

    FM_SET_FIELD64(rv64,
                   FM10000_MA_TABLE_CFG_2,
                   FloodUnicastGlort,
                   FM10000_GLORT_FLOOD);

    FM_SET_FIELD64(rv64,
                   FM10000_MA_TABLE_CFG_2,
                   FloodMulticastGlort,
                   FM10000_GLORT_MCAST);

    FM_SET_FIELD64(rv64,
                   FM10000_MA_TABLE_CFG_2,
                   BroadcastGlort,
                   FM10000_GLORT_BCAST);

    err = switchPtr->WriteUINT64(sw, FM10000_MA_TABLE_CFG_2(0), rv64);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Allocate a special port for Flow API frames.
     **************************************************/

    err = InitSpecialPort(sw, FM_PORT_NOP_FLOW);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitPortTable */




/*****************************************************************************/
/** fm10000InitPort
 * \ingroup intSwitch
 *
 * \desc            Switch-specific and port-specific initialization of a
 *                  single port in a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       portPtr points to the port state structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000InitPort( fm_int sw, fm_port *portPtr )
{
    fm_switch        *switchPtr;
    fm_status         err;
    fm10000_port     *portExt;
    fm_portAttr      *portAttr;
    fm10000_portAttr *portAttrExt;
    fm10000_lane     *laneExt;
    fm_serdesRing     ring;
    fm_int            serDes;
    fm_int            channel;
    fm_int            physLane;
    fm_char           portTimerName[16];
    fm_int            fabricPort;
    fm_bool           isPciePort;
    fm_ethMode        ethMode;
    fm_bool           unused;

    FM_LOG_ENTRY( FM_LOG_CAT_SWITCH,
                  "sw=%d portPtr=%p\n",
                  sw,
                  (void *) portPtr );

    err         = FM_OK;
    switchPtr   = GET_SWITCH_PTR(sw);

    portExt     = portPtr->extension;
    portAttr    = &portPtr->attributes;
    portAttrExt = &portExt->attributes;

    /**************************************************
     * Defaults for generic attributes
     **************************************************/


    portAttr->autoNegMode                       = FM_PORT_AUTONEG_NONE;
    portAttr->autoNegBasePage                   = 0;
    portAttr->autoNegPartnerBasePage            = 0;
    portAttr->autoNegNextPages.numPages         = 0;      
    portAttr->autoNegNextPages.nextPages        = NULL;
    portAttr->autoNegPartnerNextPages.numPages  = 0;      
    portAttr->autoNegPartnerNextPages.nextPages = NULL;

    portAttr->bcastPruning               = FM_DISABLED;
    portAttr->defCfi                     = 0;
    portAttr->defDscp                    = 0;
    portAttr->defVlanPri                 = 0;
    portAttr->defVlanPri2                = 0;
    portAttr->defSwpri                   = 0;
    portAttr->defIslUser                 = 0;
    portAttr->defVlan                    = 1;
    portAttr->defVlan2                   = 1;
    portAttr->dicEnable                  = FM_ENABLED;
    portAttr->txClkCompensation          = 100;
    portAttr->dropBv                     = FM_ENABLED;
    portAttr->dropTagged                 = FM_DISABLED;
    portAttr->dropUntagged               = FM_DISABLED;
    portAttr->ifg                        = 12;
    portAttr->islTagFormat               = FM_ISL_TAG_NONE;
    portAttr->learning                   = FM_ENABLED;
    portAttr->linkInterruptEnabled       = FM_ENABLED;
    portAttr->maxFrameSize               = 1536;
    portAttr->mcastPruning               = FM_DISABLED;
    portAttr->minFrameSize               = 64;
    portAttr->parser                     = FM_PORT_PARSER_STOP_AFTER_L2;
    portAttr->parserFlagOptions          = 0;
    portAttr->replaceDscp                = FM_DISABLED;
    portAttr->rxClassPause               = 0;
    portAttr->rxPause                    = FM_DISABLED;
    portAttr->smpLosslessPause           = 0;
    portAttr->swpriDscpPref              = FM_DISABLED;
    portAttr->swpriSource                = (FM_PORT_SWPRI_VPRI |
                                            FM_PORT_SWPRI_ISL_TAG);
    portAttr->txPause                    = 0xFFFF;
    portAttr->txPauseMode                = FM_PORT_TX_PAUSE_NORMAL;
    portAttr->txCfi                      = FM_PORT_TXCFI_ASIS;
    portAttr->txCfi2                     = FM_PORT_TXCFI_ASIS;
    portAttr->txVpri                     = FM_DISABLED;
    portAttr->txVpri2                    = FM_DISABLED;
    portAttr->ucastPruning               = FM_DISABLED;
    portAttr->updateDscp                 = FM_ENABLED;
    portAttr->updateTtl                  = FM_ENABLED;
    portAttr->ignoreIfgErrors            = FM_DISABLED;

    /**************************************************
     * Defaults for FM10000-specific attributes
     **************************************************/

    portAttrExt->autoNegLinkInhbTimer   =  LINK_INHIBIT_TIMER_USEC;    
    portAttrExt->autoNegLinkInhbTimerKx =  LINK_INHIBIT_TIMER_USEC_KX;
    portAttrExt->autoNegIgnoreNonce     =  FM_DISABLED;              
    
    portAttrExt->taggingMode             = FM_PORT_TAGGING_8021Q;
    portAttrExt->parserVlan1Tag          = 1;
    portAttrExt->parserVlan2Tag          = 0;
    portAttrExt->modifyVlan1Tag          = 1;
    portAttrExt->modifyVlan2Tag          = 1;
    portAttrExt->mirrorTruncSize         = 64;
    portAttrExt->parserFirstCustomTag    = 0; 
    portAttrExt->parserSecondCustomTag   = 0; 
    portAttrExt->parseMpls               = FM_DISABLED;
    portAttrExt->storeMpls               = 0;
    portAttrExt->routedFrameUpdateFields = FM_PORT_ROUTED_FRAME_UPDATE_ALL;
    portAttrExt->tcnFifoWm               = 10;
    portAttrExt->generateTimestamps      = FM_DISABLED;
    portAttrExt->egressTimestampEvents   = FM_DISABLED;
    portAttrExt->txFcsMode               = FM10000_FCS_REPLACE_NORMAL;
    portAttrExt->txPadSize               = 64;

    /* 1 msec */
    portAttrExt->txPcActTimescale        = FM10000_PORT_EEE_TX_ACT_TIME_SCALE;
    portAttrExt->txPcActTimeout          = 1000 / 
                                           FM10000_PORT_EEE_TX_ACT_TOUT_DIV;
    /* 20 usec */
    portAttrExt->txLpiTimescale          = FM10000_PORT_EEE_TX_LPI_TIME_SCALE;
    portAttrExt->txLpiTimeout            = 180 / 
                                           FM10000_PORT_EEE_TX_LPI_TOUT_DIV;

    portAttrExt->parserVlan2First        = FM_DISABLED;
    portAttrExt->modifyVid2First         = FM_DISABLED;
    portAttrExt->replaceVlanFields       = FM_DISABLED;

    FM_PORTMASK_ENABLE_ALL(&portExt->internalPortMask,
                           switchPtr->numCardinalPorts);

    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL) ||
         (portPtr->portType == FM_PORT_TYPE_CPU)  )
    {
        err = fm10000ResetPortSettings(sw, portPtr->portNumber);
        FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );

        /* process physical ports that have serDes associated to them */
        err = fm10000MapPhysicalPortToSerdes( sw,
                                              portPtr->physicalPort,
                                              &serDes,
                                              &ring );
        if ( err == FM_OK )
        {
            /* fill out the event info structure */
            portExt->eventInfo.switchPtr     = switchPtr; 
            portExt->eventInfo.portPtr       = portPtr;   
            portExt->eventInfo.portExt       = portExt;   
            portExt->eventInfo.portAttr      = portAttr;  
            portExt->eventInfo.portAttrExt   = portAttrExt; 
            portExt->transitionHistorySize   = FM10000_PORT_SM_HISTORY_SIZE;

            err = fm10000MapPhysicalPortToFabricPort( sw, 
                                                      portPtr->physicalPort,
                                                      &portExt->fabricPort );
            FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );

            /* Create the port state machine, start it assuming basic type */
            err = fmCreateStateMachine( portPtr->portNumber,
                                        FM10000_PORT_SM_HISTORY_SIZE,
                                        FM10000_PORT_SM_RECORD_SIZE, 
                                        &portExt->smHandle );
            FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );

            /* Create the timer(s) associated to this port */
            FM_SPRINTF_S( portTimerName,  
                          sizeof(portTimerName),
                          "Port%02dTimer",
                          portPtr->portNumber ); 

            err = fmCreateTimer( portTimerName, 
                                 fmApiTimerTask, 
                                 &portExt->timerHandle );
            FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );

            /* dynamic linkage will depend on the ethernet or the PEP mode */
            FM_DLL_INIT_LIST( portExt, firstLane, lastLane );

            /* the rest of the processing is different depending on
               whether this is an Ethernet or PCIE port */
            if ( ring == FM10000_SERDES_RING_EPL )
            {
                portExt->ring     = FM10000_SERDES_RING_EPL;
                portExt->smType   = FM_SMTYPE_UNSPECIFIED;
                portExt->anSmType = FM_SMTYPE_UNSPECIFIED;
                portExt->linkInterruptMask = FM10000_LINK_INT_MASK;
                portExt->anInterruptMask   = 0;
                portExt->anTransitionHistorySize = FM10000_AN_SM_HISTORY_SIZE;

                /* Create the AN state machine */
                err = fmCreateStateMachine( portPtr->portNumber,
                                            FM10000_AN_SM_HISTORY_SIZE,
                                            FM10000_AN_SM_RECORD_SIZE, 
                                            &portExt->anSmHandle );
                FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );


                err = 
                    fm10000MapPhysicalPortToEplChannel( sw, 
                                                        portPtr->physicalPort, 
                                                        &portExt->endpoint.epl,
                                                        &channel );     
                FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );

                err = fm10000MapEplChannelToLane( sw, 
                                                  portExt->endpoint.epl, 
                                                  channel,
                                                  &physLane );
                FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_SWITCH, err );

                /* fill the serdes-level event info structure */
                laneExt  = GET_LANE_EXT( sw, serDes );


                /**************************************************
                 * Create the linkage between port and native lane
                 **************************************************/

                portExt->nativeLaneExt = laneExt;
                laneExt->nativePortExt = portExt;
                laneExt->epl           = portExt->endpoint.epl;
                laneExt->channel       = channel;
                laneExt->physLane      = physLane;

                ethMode = FM_ETH_MODE_DISABLED;
                err = fm10000ConfigureEthMode( sw,
                                               portPtr->portNumber, 
                                               ethMode,
                                               &unused );
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

                portAttrExt->ethMode = ethMode;
                portExt->ethMode     = ethMode;
                portAttr->speed      = 0;
                portExt->speed       = 0;

            }
            else if ( ring == FM10000_SERDES_RING_PCIE )
            {
                portAttr->maxFrameSize = FM10000_PCIE_MAX_FRAME_SIZE;
                portAttr->minFrameSize = FM10000_PCIE_MIN_FRAME_SIZE;

                portPtr->mode   = FM_PORT_MODE_UP;
                portExt->ring   = FM10000_SERDES_RING_PCIE;
                portExt->smType = FM10000_PCIE_PORT_STATE_MACHINE;
                portExt->pcieInterruptMask = FM10000_PCIE_INT_MASK;
                err = fm10000ConfigurePepMode( sw, portPtr->portNumber );
            }
            else
            {
                portAttr->maxFrameSize = 15*1024;
                portAttr->minFrameSize = 15;

                portExt->ring   = FM10000_SERDES_RING_NONE;
            }

            err = fm10000SetPortAttribute(sw,
                                          portPtr->portNumber,
                                          FM_PORT_ACTIVE_MAC,
                                          FM_PORT_LANE_NA,
                                          FM_PORT_MIRROR_TRUNC_SIZE,
                                          &portAttrExt->mirrorTruncSize);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
        else if (err == FM_ERR_INVALID_ARGUMENT)
        {
            /* Set err back to OK, since it is just an indication of non-serdes port. */
            err = FM_OK;
        }
        else
        {
            FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);
        }
    }

    /* Update portType */
    if ( (portPtr->portType == FM_PORT_TYPE_PHYSICAL)
         || ( (portPtr->portType == FM_PORT_TYPE_CPU) && (portPtr->portNumber != 0) ) )
    {
        err = fm10000MapLogicalPortToFabricPort(sw, portPtr->portNumber, &fabricPort);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        if ( (fabricPort == FM10000_TE_TO_FABRIC_PORT(0)) ||
             (fabricPort == FM10000_TE_TO_FABRIC_PORT(1)) )
        {
            portPtr->portType = FM_PORT_TYPE_TE;
        } 
        else if (fabricPort == FM10000_FIBM_TO_FABRIC_PORT)
        {
            portPtr->portType = FM_PORT_TYPE_CPU_MGMT2;
        }
        else if ( (fabricPort == FM10000_LOOPBACK_TO_FABRIC_PORT(0)) ||
                  (fabricPort == FM10000_LOOPBACK_TO_FABRIC_PORT(1)) )
        {
            portPtr->portType = FM_PORT_TYPE_LOOPBACK;
        }
    }

    err = fmIsPciePort(sw, portPtr->portNumber, &isPciePort);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
 
    switch (portPtr->portType)
    {
        case FM_PORT_TYPE_CPU:
        case FM_PORT_TYPE_CPU_MGMT2:
            if (isPciePort)
            {
                portAttr->islTagFormat = FM_ISL_TAG_F56; 
            }
            if (portPtr->portNumber == 0)
            {
                FM_PORTMASK_ENABLE_ALL(&portAttr->portMask,
                                       switchPtr->numCardinalPorts);
                portAttr->enableTxCutThrough  = FALSE;
                portAttr->enableRxCutThrough  = FALSE;
                portAttr->loopbackSuppression = FALSE;
                break;
            }

            /* Fall through into Physical Port code if non-zero CPU port */

        case FM_PORT_TYPE_PHYSICAL:
            if (isPciePort)
            {
                portAttr->islTagFormat = FM_ISL_TAG_F56; 
            }
            /* Fall through to set other different default values. */
            
        case FM_PORT_TYPE_LOOPBACK:
        case FM_PORT_TYPE_TE:
        case FM_PORT_TYPE_LAG:
            FM_PORTMASK_ENABLE_ALL(&portAttr->portMask,
                                   switchPtr->numCardinalPorts);
            portAttr->enableTxCutThrough  = TRUE;
            portAttr->enableRxCutThrough  = TRUE;
            portAttr->loopbackSuppression = TRUE;
            break;

        default:
            FM_PORTMASK_DISABLE_ALL(&portAttr->portMask);
            portAttr->enableTxCutThrough  = FALSE;
            portAttr->enableRxCutThrough  = FALSE;
            portAttr->loopbackSuppression = FALSE;
            break;
    }

    portExt->groupType       = FM_MULTICAST_GROUP_TYPE_L3;
    portExt->groupEnabled    = FALSE;
    portExt->groupVlanID     = 0;

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitPort */




/*****************************************************************************/
/** fm10000PostBootSwitch
 * \ingroup intSwitch
 *
 * \desc            Perform post switch boot processing.  Code called here can
 *                  rely on the switch state being up. Called through the
 *                  PostBootSwitch function pointer.
 *
 * \param[in]       sw contains the switch number
 *
 * \return          Returns API status code
 *
 *****************************************************************************/
fm_status fm10000PostBootSwitch(fm_int sw)
{
    fm_switch *             switchPtr;
    fm_status               err;
    fm_int                  unicastSliceRangeFirst;
    fm_int                  unicastSliceRangeLast;
    fm_int                  multicastSliceRangeFirst;
    fm_int                  multicastSliceRangeLast;
    fm_int                  aclSliceRangeFirst;
    fm_int                  aclSliceRangeLast;
    fm10000_switch *        switchExt;
    fm_uint32               regVal;
    fm_int                  pepId;
#if 0
    fm_int                  L3McastFwdToCPU;
    fm_triggerRequestInfo   trigInfo;
    fm_uint16               vlan;
#endif

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = (fm10000_switch *) switchPtr->extension;

    /**********************************************************
     * Initialize default values for port and switch attributes
     *********************************************************/
    err = InitDefaultAttributes(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);


    /***************************************************
     * Initialize hashing
     **************************************************/
    err = fm10000InitHashing(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if 0
    /***************************************************
     * Flag the CPU port as an internal port.
     **************************************************/
    err = switchPtr->ReadUINT32(sw, FM10000_INTERNAL_PORT_MASK, &regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    regValue &= ~(1 << 0);      /* Clear the bit for port 0 */

    err = switchPtr->WriteUINT32(sw, FM10000_INTERNAL_PORT_MASK, regValue);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fmUpdateSwitchPortMasks(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#endif

    switchPtr->aclInfo.enabled = TRUE;

    err = fm10000FFUInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000PolicerInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000TeInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * Load slice ownership info from parameters.
     **************************************************/

    /* clear ownership, should default to nothing owned */
    FM_CLEAR(switchExt->ffuOwnershipInfo);
    FM_CLEAR(switchExt->mapOwnershipInfo);
    FM_CLEAR(switchExt->polOwnershipInfo);

    unicastSliceRangeFirst =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_UNICAST_SLICE_1ST,
                            FM_AAD_API_FM10000_FFU_UNICAST_SLICE_1ST);
    unicastSliceRangeLast =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_UNICAST_SLICE_LAST,
                            FM_AAD_API_FM10000_FFU_UNICAST_SLICE_LAST);

    if (unicastSliceRangeFirst >= 0)
    {
        err = fm10000SetFFUSliceOwnership(sw,
                                         FM_FFU_OWNER_ROUTING,
                                         unicastSliceRangeFirst,
                                         unicastSliceRangeLast);

        if (err != FM_OK)
        {
            FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                         "fm10000FFUSetSliceOwnership[unicast] returned "
                         "error %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
        }
    }

    multicastSliceRangeFirst =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_1ST,
                            FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_1ST);
    multicastSliceRangeLast =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_LAST,
                            FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_LAST);

    if (multicastSliceRangeFirst >= 0)
    {
        err = fm10000SetFFUSliceOwnership(sw,
                                          FM_FFU_OWNER_ROUTING,
                                          multicastSliceRangeFirst,
                                          multicastSliceRangeLast);

        if (err != FM_OK)
        {
            FM_LOG_ERROR( FM_LOG_CAT_SWITCH,
                         "fm10000FFUSetSliceOwnership[multicast] returned "
                         "error %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
        }
    }

    aclSliceRangeFirst =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_ACL_SLICE_1ST,
                            FM_AAD_API_FM10000_FFU_ACL_SLICE_1ST);
    aclSliceRangeLast =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_ACL_SLICE_LAST,
                            FM_AAD_API_FM10000_FFU_ACL_SLICE_LAST);

    if (aclSliceRangeFirst >= 0)
    {
        err = fm10000SetFFUSliceOwnership(sw,
                                          FM_FFU_OWNER_ACL,
                                          aclSliceRangeFirst,
                                          aclSliceRangeLast);

        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                         "fm10000FFUSetSliceOwnership[ACL] returned "
                         "error %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
        }
    }

    /* Number of mapper entries available for routing. */
    switchPtr->maxVirtualRouters =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_MAPMAC_ROUTING,
                            FM_AAD_API_FM10000_FFU_MAPMAC_ROUTING);

    if (switchPtr->maxVirtualRouters > FM10000_MAX_VIRTUAL_ROUTERS)
    {
        FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                     "Invalid maximim number of virtual routers: %d\n",
                     switchPtr->maxVirtualRouters);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, FM_ERR_TOO_MANY_VIRTUAL_ROUTERS);
    }

    /***************************************************
     * MAC Table Management initialization.
     **************************************************/

    /* Maximum number of TCN FIFO entries to process in a cycle. */
    switchExt->tcnFifoBurstSize =
        fmGetIntApiProperty(FM_AAK_API_MA_TCN_FIFO_BURST_SIZE,
                            FM_AAD_API_MA_TCN_FIFO_BURST_SIZE);

    /* Automatic creation of logical ports for remote glorts/ */
    switchExt->createRemoteLogicalPorts =
        fmGetBoolApiProperty(FM_AAK_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS,
                             FM_AAD_API_FM10000_CREATE_REMOTE_LOGICAL_PORTS);

    /***************************************************
     * Initialize Virtual Networking fields.
     **************************************************/
    switchExt->vnVxlanUdpPort  = FM_VN_VXLAN_UDP_DEST_PORT;
    switchExt->vnGeneveUdpPort = FM_VN_GENEVE_UDP_DEST_PORT;
    switchExt->vnEncapAcl      = -1;
    switchExt->vnDecapAcl      = -1;

    FM_MEMSET_S( switchExt->vnTunnelGroups,
                 sizeof(switchExt->vnTunnelGroups),
                 -1,
                 sizeof(switchExt->vnTunnelGroups) );

    /***************************************************
     * Module-specific initialization.
     **************************************************/

    err = fm10000LagGroupInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000LBGInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000NextHopInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000RouterInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000InitMacSecurity(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000InitCounters(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000MirrorInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000AclInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000TunnelInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000NatInit(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000InitFlooding(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000InitSFlows(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if 0
    err = fm10000CVlanInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#endif

    err = fm10000McastGroupInit(sw, FALSE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000MailboxInit(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

#if 0
    err = fm10000MTableInitialize(sw);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = fm10000InitCrm(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#endif

    /***************************************************
     * Create internal triggers.
     **************************************************/

#if 0
    /****************************************************************************
     *  Pre-allocate a trigger for dropping routed/multicast/special delivery
     *  frames that cannot be handled by configuring PORT_CFG_2.
     *  See Bugzilla 11387. This trigger takes precedence over the mirror
     *  related triggers. This is to ensure the traffic destined to a downed
     *  egress port is not mirrored by an egressing mirror.
     ***************************************************************************/
    switchExt->useRateLimiterTrigger =
              fmGetBoolApiProperty(FM_AAK_API_FM10000_USE_RATE_LIMITER_TRIGGER,
                                   FM_AAD_API_FM10000_USE_RATE_LIMITER_TRIGGER);
    trigInfo.requestRateLimiter = switchExt->useRateLimiterTrigger;

    err = fmAllocateTriggerExt(sw,
                               "removeDownPortsTrigger",
                               &switchExt->removeDownPortsTrigger,
                               &trigInfo);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = SetupTriggerForDownPorts(sw,
                                   switchExt->removeDownPortsTrigger);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    trigInfo.requestRateLimiter = FALSE;
#endif

#if 0
    /* Determine whether L3 Multicast Group with the attribute
     * FM_MCASTGROUP_FWD_TO_CPU should be mapped to a new switch priority.
     */
    L3McastFwdToCPU =
        fmGetIntApiProperty(FM_AAK_API_FM10000_L3_MCAST_FWD_TO_CPU_PRIORITY,
                            FM_AAD_API_FM10000_L3_MCAST_FWD_TO_CPU_PRIORITY);

    if (L3McastFwdToCPU >= 0)
    {
        err = fmAllocateTriggerExt(sw,
                                   "mcastL3FwdToCPUTrigger",
                                   &switchExt->mcastL3FwdToCPUTrigger,
                                   &trigInfo);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = SetupTriggerForL3McastFwdToCPUTrapping(
                    sw,
                    switchExt->mcastL3FwdToCPUTrigger);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
#endif

    /***************************************************
     * Mapper initialization.
     **************************************************/

    err = fm10000QOSPriorityMapperAllocateResources(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /***************************************************
     * QoS initialization.
     **************************************************/

    err = fm10000InitQOS(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Configure the Interrupt Mask 
     **************************************************/

    /* Enable interrupt */
    err = switchPtr->WriteUINT64(sw,
                                 switchExt->interruptMaskReg,
                                 switchExt->interruptMaskValue);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /**************************************************
     * Enable parity interrupts.
     **************************************************/

    if (fmGetBoolApiProperty(FM_AAK_API_FM10000_PARITY_INTERRUPTS,
                             FM_AAD_API_FM10000_PARITY_INTERRUPTS))
    {
        err = fm10000EnableParityInterrupts(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    /* Debug intermittent DMA issue */
    if (!fmGetBoolApiProperty(FM_AAK_API_PLATFORM_IS_WHITE_MODEL,
                              FM_AAD_API_PLATFORM_IS_WHITE_MODEL))
    {
        err = fm10000MapLogicalPortToPep(sw, switchPtr->cpuPort, &pepId);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        err = fm10000ReadPep(sw, FM10000_PCIE_RXQCTL(0), pepId, &regVal);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

        if (!FM_GET_BIT(regVal, FM10000_PCIE_RXQCTL, ENABLE))
        {
            FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                 "RX DMA Disabled. Expect pep %d RXQCTL[0].enable=1 but is 0. RXQCTL=0x%x\n",
                 pepId, regVal);
        }
    }
   
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000PostBootSwitch */




/*****************************************************************************/
/** fm10000GetSwitchInfo
 * \ingroup intSwitch
 *
 * \desc            Retrieves information about the switch.
 *                  Called through the GetSwitchInfo function pointer.
 *
 * \param[in]       sw is the switch for which to retrieve information.
 *
 * \param[out]      info is a pointer to an fm_switchInfo structure
 *                  (see 'fm_switchInfo') to be filled in by this function.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 *
 *****************************************************************************/
fm_status fm10000GetSwitchInfo(fm_int sw, fm_switchInfo *info)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_uint32  vpd;
    fm_uint64  vpdInfo1;
    fm_uint64  vpdInfo2;
    fm_float   fhMhz;

    FM_LOG_ENTRY( FM_LOG_CAT_SWITCH, "sw=%d, info=%p\n", sw, (void *) info );

    /* Why are we validating and protecting the switch inside
     * a switch-specific function? If this really needs to be done here,
     * it needs to be documented here, because it contradicts the
     * switch-specific function contract, which specifies that this will
     * already have been done prior to entering a switch-specific function
     * like this! */
    VALIDATE_SWITCH_LOCK(sw);

    /* Take read access to the switch lock */
    PROTECT_SWITCH(sw);

    /* check if switch exists or not, doesn't need to be up */
    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr == NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, "Switch not allocated\n");
        err = FM_ERR_INVALID_SWITCH;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    FM_MEMSET_S(info, sizeof(fm_switchInfo), 0, sizeof(fm_switchInfo) );

#if 0
    err = switchPtr->ReadUINT32(sw, FM10000_VITAL_PRODUCT_DATA, &vpd);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = switchPtr->ReadUINT64(sw, FM10000_VPD_INFO_1(0), &vpdInfo1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    err = switchPtr->ReadUINT64(sw, FM10000_VPD_INFO_2(0), &vpdInfo2);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
#else
    err      = FM_OK;
    vpd      = 0;
    vpdInfo1 = 0;
    vpdInfo2 = 0;
#endif

    /* Perhaps the FH ref clock should be cached so that we don't have
     * to compute it every time */
    err = fm10000ComputeFHClockFreq(sw, &fhMhz);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    
    info->fhRefClock = (fm_int)fhMhz;

    info->numPorts = switchPtr->maxPhysicalPort + 1;

    info->numCardPorts = switchPtr->numCardinalPorts;

    /* Memory size in bytes */
    info->memorySize          = switchPtr->maxSegments * FM10000_SEGMENT_SIZE;
    info->availableMemorySize = info->memorySize;

    info->maxVLAN         = 4096;
    info->maxPortMirrors  = 4;          /* FM10000_MAX_PORT_MIRRORS */
    info->maxTrigs        = 64;

    /* Note that maxLags is not set to FM10000_MAX_NUM_LAGS because
     * a switch (in a multiswitch system) can contain LAGs with remote
     * ports only. Such LAGs are likely to use the same internal port but
     * with a different DGLORT. Therefore, it is possible for the switch
     * to have more LAGs than FM10000_MAX_NUM_LAGS. */
    info->maxLags         = FM_MAX_NUM_LAGS;
    info->maxPortsPerLag  = FM10000_MAX_NUM_LAG_MEMBERS;
    info->maxVlanCounters = FM10000_MAX_VLAN_COUNTER + 1;

ABORT:
    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000GetSwitchInfo */




/*****************************************************************************/
/** fm10000ComputeFHClockFreq
 * \ingroup intSwitch
 *
 * \desc            Reads the frame handler PLL multipliers from the chip
 *                  and then computes the FH clock.
 *                  Called through the ComputeFHClockFreq function pointer.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      fhMhz points to a caller-supplied location to receive
 *                  the frame handler clock frequency in MHz.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ComputeFHClockFreq(fm_int sw, fm_float *fhMhz)
{
    fm_status   err;
    fm_uint32   rv;
    fm_uint32   featureCode;
    fm_uint32   freqSel;
    fm_float    maxFreq;
    fm_float    refClk;
    fm_uint32   refDiv;
    fm_uint32   fbDiv4;
    fm_uint32   fbDiv255;
    fm_uint32   outDiv;
    fm_switch * switchPtr = GET_SWITCH_PTR(sw); 

    *fhMhz = 0;

    /* Evaluate if there are any restrictions */
    err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_LOCK(), &rv);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    featureCode = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_LOCK, FeatureCode);
    freqSel     = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_LOCK, FreqSel);

    if ( (featureCode == FM10000_PFL_FEATURE_CODE_FULL) &&
         (freqSel     == FM10000_PFL_FREQ_SEL_USE_CTRL) )
    {
        /* RefClk in MHZ */
        refClk = FM10000_ETH_REF_CLOCK_FREQ / (1e6);
        
        err = switchPtr->ReadUINT32(sw, FM10000_PLL_FABRIC_CTRL(), &rv);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        
        refDiv   = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, RefDiv);
        fbDiv4   = FM_GET_BIT  (rv, FM10000_PLL_FABRIC_CTRL, FbDiv4);
        fbDiv255 = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, FbDiv255);
        outDiv   = FM_GET_FIELD(rv, FM10000_PLL_FABRIC_CTRL, OutDiv);

        /* Protect against zero division */
        if ( (refDiv == 0) || (outDiv == 0) )
        {
            err = FM_FAIL;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }

        *fhMhz = ( ( refClk / refDiv) * 4 * fbDiv255 * (1 + fbDiv4) ) / (2 * outDiv);
    }
    else
    {
        switch (featureCode)
        {
            case FM10000_PFL_FEATURE_CODE_FULL:
            case FM10000_PFL_FEATURE_CODE_LIMITED1:
                maxFreq = 600;
                break;

            case FM10000_PFL_FEATURE_CODE_LIMITED2:
                maxFreq = 500;
                break;

            case FM10000_PFL_FEATURE_CODE_LIMITED3:
                maxFreq = 400;
                break;

            case FM10000_PFL_FEATURE_CODE_LIMITED4:
                maxFreq = 300;
                break;

            default:
                /* Should never get here */
                maxFreq = 600;
                break;
        }

        switch (freqSel)
        {
            case FM10000_PFL_FREQ_SEL_F600:
                *fhMhz = 600;
                break;

            case FM10000_PFL_FREQ_SEL_F500:
                *fhMhz = 500;
                break;

            case FM10000_PFL_FREQ_SEL_F400:
                *fhMhz = 400;
                break;

            case FM10000_PFL_FREQ_SEL_F300:
                *fhMhz = 300;
                break;

            default:
                /* Should never get here */
                *fhMhz = 600;
                break;
        }

        if (*fhMhz > maxFreq)
        {
            *fhMhz = maxFreq;
        }
    }

ABORT:
    
    return err;
    
}   /* end fm10000ComputeFHClockFreq */



/*****************************************************************************/
/** fm10000VerifySwitchAliveStatus
 * \ingroup intSwitch
 *
 * \desc            Check whether access to the switch registers is still
 *                  alive by verifying switch VPD register. If the switch VDP
 *                  is not valid, switch state is set to FAILED.
 * 
 * \param[in]       sw is the ID of the switch on which to operate
 *
 * \return          FM_OK if switch VPD is still valid.
 * \return          FM_FAIL if detected invalid switch VPD.
 *
 *****************************************************************************/
fm_status fm10000VerifySwitchAliveStatus(fm_int sw)
{
    fm_switch   *switchPtr;
    fm_uint32    vpd;

    if (fmRootApi->fmSwitchStateTable[sw]->state ==
        FM_SWITCH_STATE_FAILED)
    {
        return FM_FAIL;
    }

    switchPtr = GET_SWITCH_PTR(sw);

    switchPtr->ReadUINT32(sw,
                          FM10000_VITAL_PRODUCT_DATA(),
                          &vpd);
    if (vpd != 0xAE21)
    {
        FM_LOG_ERROR( FM_LOG_CAT_SWITCH, 
              "Register access to switch#%d is lost. "
              "Set switch state to FAILED. VPD = 0x%x\n", sw, vpd);
        if (FM_IS_STATE_ALIVE(
                fmRootApi->fmSwitchStateTable[sw]->state))
        {
            fmRootApi->fmSwitchStateTable[sw]->state =
                FM_SWITCH_STATE_FAILED;
            /* Should send an event here */
        }
        return FM_FAIL;
    }

    return FM_OK;

}   /* end fm10000VerifySwitchAliveStatus */


/*****************************************************************************/
/** fm10000Write1588SystimeCfg
 * \ingroup intSwitch
 *
 * \desc            Writes all SYSTIME0 registers with the given step value and
 *                  offset as 0.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       step is the value by which SYSTIME should be incremented
 *                  for each tick of the 1588 source clock.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000Write1588SystimeCfg(fm_int sw, fm_int step)
{

    fm_switch * switchPtr;
    fm_status   err;
    fm_uint32   rvSoftReset;
    fm_uint32   rvDeviceCfg;
    fm_uint32   origPcieActive;
    fm_uint32   newPcieActive;
    fm_uint64   rv64;
    fm_uint32   regAddr;
    fm_bool     regLockTaken;
    fm_int      i;

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d step=%d\n", sw, step);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_FLAG_TAKE_REG_LOCK(sw);

    err = switchPtr->ReadUINT32(sw, FM10000_SOFT_RESET(), &rvSoftReset);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Cache the value for restoring the value at the end */
    origPcieActive = FM_GET_FIELD(rvSoftReset, FM10000_SOFT_RESET, PCIeActive);

    /* Force all enabled PEPs out of reset */
    err = switchPtr->ReadUINT32(sw, FM10000_DEVICE_CFG(), &rvDeviceCfg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    newPcieActive = FM_GET_FIELD(rvDeviceCfg, FM10000_DEVICE_CFG, PCIeEnable);

    FM_SET_FIELD(rvSoftReset, FM10000_SOFT_RESET, PCIeActive, newPcieActive);
        
    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rvSoftReset);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Update STEP value in global SYSTIME_CFG config */
    err = switchPtr->ReadUINT64(sw, FM10000_SYSTIME_CFG(0), &rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_SET_FIELD64(rv64, FM10000_SYSTIME_CFG, STEP, step);

    err = switchPtr->WriteUINT64(sw, FM10000_SYSTIME_CFG(0), rv64);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* Update STEP value in all end points SYSTIME_CFG config */
    for (i = 0; i < FM10000_NUM_EPLS; i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_EPL_SYSTIME_CFG(i), step);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }

    for (i = 0; i < FM10000_NUM_TUNNEL_ENGINES; i++)
    {
        err = switchPtr->WriteUINT32(sw, FM10000_TE_SYSTIME_CFG(i, 0), step);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
    }
    
    for (i = 0; i < FM10000_NUM_PEPS; i++)
    {
        if ( newPcieActive & (1 << i))
        {
            regAddr = FM10000_PCIE_PF_ADDR(FM10000_PCIE_SYSTIME_CFG(), i);

            err = switchPtr->WriteUINT32(sw, regAddr, step);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);
        }
    }

    /* Restore PCIeActive back to original value. */
    FM_SET_FIELD(rvSoftReset, FM10000_SOFT_RESET, PCIeActive, origPcieActive);
        
    err = switchPtr->WriteUINT32(sw, FM10000_SOFT_RESET(), rvSoftReset);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    FM_FLAG_DROP_REG_LOCK(sw);
    
ABORT:
    if (regLockTaken)
    {
        FM_FLAG_DROP_REG_LOCK(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000Write1588SystimeCfg */




/*****************************************************************************/
/** fm10000GetNvmImageVersion
 * \ingroup intSwitch
 *
 * \desc            Returns NVM image version.
 * 
 * \param[in]       sw is the ID of the switch on which to operate.
 *
 * \param[out]      version points to a caller supplied area where
 *                  this function will return the NVM image version.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetNvmImageVersion(fm_int sw, fm_uint *version)
{
    fm_switch   *switchPtr;
    fm_uint32    val;

    switchPtr = GET_SWITCH_PTR(sw);

    switchPtr->ReadUINT32(sw,
                          FM10000_BSM_SCRATCH(401),
                          &val);
    *version = val;

    return FM_OK;

}   /* end fm10000GetNvmImageVersion  */

