/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_switch_int.h
 * Creation Date:   Ported from fm_api_common_int.h on Feb 21, 2007
 * Description:     Generic Switch-wide Definitions
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

#ifndef __FM_FM_API_SWITCH_INT_H
#define __FM_FM_API_SWITCH_INT_H


/* Enumerate switch capabilities */
#define FM_SWITCH_CAPABILITY_CAN_ROUTE    (1 << 0)
#define FM_SWITCH_CAPABILITY_LAG_CAPABLE  (1 << 1)

#define FM_MAX_FUSEBOX_ENTRIES  6

#define FM_BOOT_PLL_MULT_P      0
#define FM_BOOT_PLL_MULT_M      1
#define FM_BOOT_PLL_MULT_N      2

/* Mask and positions of ISL tag fields. Note that the masks are not offseted
 * by position and that positions are within the ISL tag word (fm_uint32) */
#define FM_F56_USER_MASK               0xFF
#define FM_F56_USER_POS                24
#define FM_F56_FTYPE_MASK              0x3
#define FM_F56_FTYPE_POS               22
#define FM_F56_VTYPE_MASK              0x3
#define FM_F56_VTYPE_POS               20
#define FM_F56_SWPRI_MASK              0xF
#define FM_F56_SWPRI_POS               16
#define FM_F56_VPRIVLAN_MASK           0xFFFF
#define FM_F56_VPRIVLAN_POS            0
#define FM_F56_SGLORT_MASK             0xFFFF
#define FM_F56_SGLORT_POS              16
#define FM_F56_DGLORT_MASK             0xFFFF
#define FM_F56_DGLORT_POS              0

#define FM_F64_USER_MASK               0xFF
#define FM_F64_USER_POS                16
#define FM_F64_FTYPE_MASK              0x3
#define FM_F64_FTYPE_POS               30
#define FM_F64_VTYPE_MASK              0x3
#define FM_F64_VTYPE_POS               28
#define FM_F64_SWPRI_MASK              0xF
#define FM_F64_SWPRI_POS               24
#define FM_F64_VPRIVLAN_MASK           0xFFFF
#define FM_F64_VPRIVLAN_POS            0
#define FM_F64_SGLORT_MASK             0xFFFF
#define FM_F64_SGLORT_POS              16
#define FM_F64_DGLORT_MASK             0xFFFF
#define FM_F64_DGLORT_POS              0

/* collates miscellaneous boot related info */
/* Note: This structure is used for both FM2xxx and FM4xxxx chip models.
 * Thus, it contains information that may be used by only one or by both
 * of the models. */
typedef struct
{
    fm_bool   bypassFrameHandlerPLL;

    /* in order: P, M, N */
    fm_uint32 frameHandlerPLLDividers[3];

    fm_bool   enableEEPROMMode;

    fm_bool   useShadowFuseboxEntries;
    fm_uint32 shadowFuseboxEntries[FM_MAX_FUSEBOX_ENTRIES];

    fm_bool   invertLED;
    fm_bool   enableLED;

    fm_bool   enableDFT;

    fm_bool   autoBoot;

} fm_bootInfo;


typedef struct
{
    fm_uint32   tag[1];
} fm_f32Tag;    


typedef struct
{
    fm_uint32   tag[2];
} fm_f56Tag;        


typedef struct
{
    fm_uint32   tag[2];
} fm_f64Tag;        


typedef struct
{
    fm_uint32   tag[4];
} fm_f96Tag;


typedef union
{
    fm_f32Tag   f32;
    fm_f56Tag   f56;
    fm_f64Tag   f64;
    fm_f96Tag   f96;
} fm_islTag;




/****************************************************************************/
/** \ingroup platformFuncPtrs
 *
 * A pointer to a function reading a value from a single 32-bit wide register.
 *
 *  \param[in]  sw is the switch on which to operate.
 *
 *  \param[in]  addr is the register address within the chip.
 *
 *  \param[out] value points to caller-allocated storage where this
 *              function is to place the register value.
 *
 *  \return     FM_OK if successful.
 * 
 ****************************************************************************/
typedef fm_status (*fm_registerReadUINT32Func)(fm_int     sw,
                                               fm_uint32  addr,
                                               fm_uint32 *value);




/****************************************************************************/
/** \ingroup platformFuncPtrs
 *
 * A pointer to a function writing a value from a single 32-bit wide register.
 *
 *  \param[in]  sw is the switch on which to operate.
 *
 *  \param[in]  addr is the register address within the chip.
 *
 *  \param[in]  value is the register value to write.
 *
 *  \return     FM_OK if successful.
 * 
 ****************************************************************************/
typedef fm_status (*fm_registerWriteUINT32Func)(fm_int    sw,
                                                fm_uint32 addr,
                                                fm_uint32 value);




/****************************************************************************/
/** \ingroup platformFuncPtrs
 *
 *  Switch state structure
 *
 *  Important Note to Intel developers: 
 *
 *  Comments for most structure members should use standard C style slash-star
 *  comments so they do not appear in the user documentation. Only members 
 *  which are function pointers to be populated by the platform layer should be
 *  decorated with slash-star-star Doxygen style comments.
 ****************************************************************************/
struct _fm_switch
{
    /**************************************************
     * Switch Identification and Capabilities
     **************************************************/

    /* switch number, kept here for cross-referencing purposes */
    fm_int                      switchNumber;

    /* switch family */
    fm_switchFamily             switchFamily;

    /* switch model */
    fm_switchModel              switchModel;

    /* switch version */
    fm_switchVersion            switchVersion;

    /* switch capabilities bit field */
    fm_uint                     capabilities;
    
    /* Number of memory segments available */
    fm_int                      maxSegments;
    fm_uint32                   segmentSize;

    /* Maximum logical port number supported by the API.
     * Used when range-checking logical port numbers, and in those
     * rare cases when we need to operate over the entire logical 
     * port table. */ 
    fm_int                      maxPort;

    /* Maximum physical port number supported by the switch.
     * Used for range-checking and enumerating the physical port domain, 
     * and when allocating structures that are indexed by physical port number. 
     *  
     * Note that this is a maximum value, not a number of values. 
     * The range of the set is 0 <= physPort <= maxPhysicalPort, 
     * and the cardinality of the set is maxPhysicalPort + 1. 
     *  
     * On legacy platforms, this is also the maximum logical port number
     * associated with a physical port; the field has historically been 
     * used for range-checking and enumerating those logical ports. 
     * This usage is now deprecated in favor of the cardinalPortInfo
     * structure and numCardinalPorts. */ 
    fm_int                      maxPhysicalPort;

    /* Maximum number of port sets supported. */
    fm_int                      maxPortSets;

    /* Maximum number of STP instances. */
    fm_int                      maxSTPInstances;

    /* Maximum number of Virtual Network Tunnels supported by the hardware. */
    fm_int                      maxVNTunnels;

    /* Switch-Type-Specific information. */
    void *                      extension;

    /* Port table, contains "maxPorts" fm_port structures.
     * Indexed by logical port number. */
    fm_port **                  portTable;

    /* Size of laneTable */ 
    fm_int                      laneTableSize;

    /* Lane table, contains fm_lane structures.
     * Size and index are chip family specific. */
    fm_lane **                  laneTable;

    /* Parent switch aggregate switch number.  -1 if standalone switch. */
    fm_int                      swag;

    /* Pointer to switch-specific event handler.  If NULL, use global handler */
    fm_switchEventHandler       eventHandler;

    /* switch info structure */
    fm_switchInfo               info;

    /* holds the logical port management state */
    fm_logicalPortInfo          logicalPortInfo;

    /* Highest logical port number reserved by the platform layer.
     * Defaults to maxPhysicalPort on most platforms. 
     * Logical port numbers assigned by the API (for LAGs, etc.)
     * will start at maxReservedPort + 1. */
    fm_int                      maxReservedPort;

    /* Number of management ports such as CPU, loopback, etc */
    fm_int                      numMgmtPorts;

    /***************************************************
     * Cardinal port information.
     **************************************************/

    /* Cardinal port information structure. */
    fm_cardinalPortInfo         cardinalPortInfo;

    /* Number of cardinal ports on this switch.
     *  
     * Used in conjunction with cardinalPortInfo to describe 
     * the set of logical ports that have associated physical 
     * ports (known collectively as cardinal ports). 
     *  
     * Note that this is the cardinality of the set, not the
     * maximum index value (0 <= cpi < numCardinalPorts). */
    fm_int                      numCardinalPorts;

    /***************************************************
     * Stacking information structure.  Note that this
     * information may be updated when the switch is
     * down.
     **************************************************/
    fm_stackingInfo             stackingInfo;

    /***************************************************
     * MAC address table information.
     **************************************************/

    /* Maximum number of entries in the MAC address table. */
    fm_int                      macTableSize;

    /* Number of banks in the MAC address table. */
    fm_int                      macTableBankCount;

    /* Number of entries per bank in the MAC address table. */
    fm_int                      macTableBankSize;

    /* Default MAC address trigger identifier. */
    fm_uint32                   macTableDefaultTrigID;

    /* Current position in the background scan of the MAC address table. */
    fm_int                      macTableScanIndex;

    /* Time at which the fast maintenance task last refreshed the dynamic
     * remote glorts in the hardware MAC address table. */
    fm_timestamp                macTableLastRemoteRefresh;

    /* Cached value of the FM_AAK_API_MA_EVENT_ON_STATIC_ADDR attribute.
     * Specifies whether the API should generate a LEARNED or AGED event
     * when the application adds or deletes a static MA table entry. */
    fm_bool                     generateEventOnStaticAddr;

    /* Cached value of the FM_AAK_API_MA_EVENT_ON_DYNAMIC_ADDR attribute.
     * Specifies whether the API should generate a LEARNED or AGED event
     * when the application adds or deletes a dynamic MA table entry. */
    fm_bool                     generateEventOnDynamicAddr;

    /* Number of supported VLANs */
    fm_int                      vlanTableSize;
    fm_int                      maxVlanCounter;

    /* Mirror Table Size */
    fm_int                      mirrorTableSize;

    /* Link Aggregation limits */
    fm_int                      maxPhysicalLags;
    fm_int                      maxPhysicalPortsPerLag;

    /* Policer limits */
    fm_int                      policerBanks;

    /* Routing Information */
    fm_int                      maxRoutes;
    
    fm_int                      maxVirtualRouters;
    fm_int                      maxEcmpGroupSize;
    fm_customTree               routeTree;
    fm_customTree               ecmpRouteTree;
    fm_bool                     supportRoutingLookups;
    fm_customTree *             routeLookupTrees;


    fm_int *                    virtualRouterIds;
    fm_macaddr                  physicalRouterMac;
    fm_macaddr                  virtualRouterMac[2];
    fm_bitArray                 ecmpGroupsInUse;
    fm_intEcmpGroup **          ecmpGroups;
    fm_int                      dropEcmpGroup;

    /* ipInterfaces information */
    fm_int                      maxIpInterfaces;
    fm_bitArray                 ipInterfaceEntriesInUse;
    fm_intIpInterfaceEntry *    ipInterfaceEntries;
    fm_customTree               noInterfaceNextHops;
    
    /* ARP information */
    fm_int                      maxArpEntries;
    fm_customTree               noArpNextHops;
    FM_DLL_DEFINE_LIST(_fm_intArpEntry, firstArp, lastArp);
    /* Tree contains pointers to internal arps. */
    fm_tree                     arpPointersTree; 


    /* Virtual Network Information */
    fm_tree                     virtualNetworks;
    fm_virtualNetwork **        vnInternalIds;
    fm_bitArray                 vnTunnelsInUse;
    fm_tree                     vnTunnels;
    fm_customTree               vnTunnelRoutes;
    fm_vnTunnel *               vxlanDecapsulationTunnel;
    fm_vnTunnel *               nvgreDecapsulationTunnel;

    /* Multicast Group Information.
     * To avoid deadlocks, access to multicast information is protected by the
     * routing lock. */
    /* tree containing one record for each created multicast group. The key is
     * the multicast group's handle; the value is a pointer to the multicast
     * group record. */
    fm_tree                     mcastTree;

    /* tree containing one record for each created multicast group which has
     * an assigned multicast address. The key is the multicast address; the
     * value is a pointer to the multicast group record. */
    fm_customTree               mcastAddressTree;

    /* tree containing one record for each multicast group which has an
     * associated logical port.  The key is the logical port; the value is
     * a pointer to the multicast group record. */
    fm_tree                     mcastPortTree;

    /* bit array controlling multicast group handle availability.  A bit set
     * to one means that the handle is in use. Handles that have a logical
     * port attached, whether that handle/port pair are being used by a group
     * or not, is considered "in use" from the perspective of this table.
     * This bit array is used by fmAllocateMcastGroupsInt to find an available
     * block of handles for the newly-allocated logical ports.  This array
     * is also used when the API needs to allocate a handle without
     * (immediately) attaching a logical port.  A bit set to one means that
     * the handle is in use. */
    fm_bitArray                 mcastHandles;

    /* tree containing one record for each multicast group handle which has
     * an associated logical port.  The key is the handle; the value is the
     * logical port number encapsulated in a structure. */
    fm_tree                     mcastHandlePortTree;

    fm_int                      mcastMaxEntryPerGlort;
    fm_int                      mcastMaxEntryPerCam;
    fm_int                      mcastMaxDestTableEntries;

    /* Replication Group Information.
     * To avoid deadlocks, access to replication group information is protected 
     * by the routing lock. */
    /* tree containing one record for each created replication group. The key is
     * the replication group's handle; the value is a pointer to the replication
     * group record. */
    fm_tree                     replicationTree;

    /* bit array controlling replication group handle availability.  A bit set
     * to one means that the handle is in use. */
    fm_bitArray                 replicationHandles;

    /* State information for load balancing groups */
    fm_LBGInfo                  lbgInfo;

    /* Table of microcode constants */
    fm_uint64 *                 ucConstants;

    /* CPU Packet Transmission Default Source Port */
    fm_int                      defaultSourcePort;

    /**************************************************
     * Current Status and Configuration
     **************************************************/

    /* switch state */
    fm_switchState              state;

    /* Logical port number of the cpu port */
    fm_int                      cpuPort;

    /* pep number used for MSI interrupt */
    fm_int                      msiPep;

    /* Glort Range */
    fm_glortRange               glortRange;
    fm_glortInfo                glortInfo;

    /* MAC Table cache */
    fm_internalMacAddrEntry *   maTable;

    /* VLAN Table */
    fm_vlanEntry *              vidTable;
    fm_uint16                   reservedVlan;

    /* LAG information structure */
    fm_lagInfo                  lagInfoTable;

    /* ACL Table */
    fm_aclInfo                  aclInfo;
    fm_policerInfo              policerInfo;

    /* PortSet Table */
    fm_portSetInfo              portSetInfo;

    /* Mirror Table */
    fm_portMirrorGroup *        mirrorGroups;

    /* Counter Table */
    fm_counterInfo              counterInfo;

    /* NAT Table */
    fm_natInfo *                natInfo;

    fm_bootInfo                 bootInfo;

    /* Whether periodic MAC table polling is required. */
    fm_bool                     pollMacTable;

    /* Minimum MAC address aging time, in ticks. A value of zero
     * (the default) disables aging. Set / retrieved via the
     * FM_MAC_TABLE_ADDRESS_AGING_TIME address table attribute. */
    fm_uint                     macAgingTicks;
    fm_uint                     macAgingTickDelta;

    /* the watermark management mode */
    fm_bool                     autoPauseMode;

    /* Whether source MAC address lookups should be unconditional
     * (FM_SA_LOOKUP_ALWAYS) or best effort (FM_SA_LOOKUP_BEST_EFFORT). 
     * Set/retrieved via the FM_MAC_TABLE_SA_LOOKUP_MODE address table 
     * attribute. (FM2000, FM4000) */ 
    fm_uint32                   globalMacSALookupMode;

    /* LAG management mode as defined by FM_AAK_API_PER_LAG_MANAGEMENT */
    fm_int                      perLagMgmt;

    /* Global Router Configuration */
    fm_routerState *            virtualRouterStates;
    fm_routerMacMode *          virtualRouterMacModes;
    fm_int                      routerTrapTTL1;
    fm_bool                     routerTrapRedirectEvent;
    fm_bool                     routerTrapIpOptions;

    /* set the EGRESS_VID_TABLE[].membership[] using
     * EGRESS_FID_TABLE[].Forwarding[] & real VLAN membership to make sure that
     * any packet that are being untrapped by the trigger do not bypass the
     * egress STP state checks.
     */ 
    fm_bool                     useEgressVIDasFID;

    /* state information for spanning tree instances */
    fm_tree                     stpInstanceInfo;
    fm_stpInstanceInfo *        defaultSTPInstance;
    fm_stpMode                  stpMode;

    /* Information on transmission state */
    fm_bool                     transmitterLock;

    /* information for managing learning by VLAN */
    fm_vlanLearningMode         vlanLearningMode;
    fm_int                      sharedLearningVlan;

    /**************************************************
     * Generic Feature Access Locks
     **************************************************/

    /* Indicates if the locks have been initialized. */
    fm_bool                     accessLocksInitialized;

    /* Lock taken to protect various state structures. */
    fm_lock                     stateLock;
    
    /* Lock to protect packet send/receive interrupt flags. */
    fm_lock                     pktIntLock;

    /* L2 lock: VLAN, MA Table, STP. */
    fm_lock                     L2Lock;

    /* Lock taken for any access to the queue of pending MA purges.  
     * Note that access to the fields in fm_switch.maPurge that pertain 
     * to the currently active purge (e.g. 'request', 'active', 'startTime') 
     * require the maTableLock instead. */
    fm_lock                     maPurgeLock;

    /* Lock taken for any access to the MAC table maintenance handler's
     * work list. */
    fm_lock                     macTableMaintWorkListLock;

    /* Lock taken for any access to the ACL data structures (aclInfo, above) */
    fm_lock                     aclLock;

    /* Lock taken for any access to the tunnel data structures */
    fm_lock                     tunnelLock;

    /* Lock taken for any access to the Flow API data structures */
    fm_lock                     flowLock;

    /* Lock taken for any access to the NAT data structures */
    fm_lock                     natLock;

    /* Read/Write lock taken for any access to the routing tables */
    fm_rwLock                   routingLock;

    /* Lock taken for any access to the mirror data structures */
    fm_lock                     mirrorLock;

    /* Lock taken for any access to the port set data structures */
    fm_lock                     portSetLock;

    /* Lock taken for any access to the trigger data structures
     * (usedTriggers, usedRateLimiters) */
    fm_lock                     triggerLock;

    /* Lock taken to protect the LAG resources. */
    fm_lock                     lagLock;

    /* Lock taken for any access to the CRM data structures */
    fm_lock                     crmLock;

    /* Lock taken for any access to the MTABLE data structures */
    fm_lock                     mtableLock;

    /* Lock taken for any access to the scheduler data structures */
    fm_lock                     schedulerLock;

    /* Lock taken for any acces to the MAILBOX data structures */
    fm_lock                     mailboxLock;

    /**************************************************
     * Generic MAC Table Maintenance Handler Information
     **************************************************/

    /* MAC Table Maintenance Handler Work Lists */
    fm_addrMaintWorkList        workList1;
    fm_addrMaintWorkList        workList2;
    fm_addrMaintWorkList *      pPendingWorkList;

    /***************************************************
     * MAC address purging data structure.
     * Access to the maPurge member is serialized using
     * the MA Table lock.
     **************************************************/
    fm_maPurge                  maPurge;

    /**************************************************
     * State for interrupt handling
     **************************************************/

    /* flags to indicate what work needs doing in the interrupt task */
    fm_bool                     intrSendPackets;
    fm_bool                     intrReceivePackets;

    /**************************************************
     * State information related to buffer and event 
     * management.
     **************************************************/

    fm_bool                     buffersNeeded;
    fm_eventFreeNotifyHndlr     eventFreeNotifyHndlr[MAX_EVENT_FREE_NOTIFY_HANDLER];

    /**************************************************
     * Parity error management.
     **************************************************/
    fm_paritySweeperConfig      paritySweeperCfg;
    fm_bool                     parityRepairEnabled;

    /**************************************************
     * Mailbox management.
     **************************************************/
    fm_mailboxInfo              mailboxInfo; 

    /**************************************************
     * Is raw socket initialized
     **************************************************/
    fm_bool                     isRawSocketInitialized;

    /**************************************************
     * Generic Switch Support Function Pointers
     * These functions MUST be implemented by all
     * chip-types
     **************************************************/

    /**************************************************
     * Switch Initialization
     **************************************************/
    fm_status                   (*InitSwitch)(fm_switch *switchPtr);
    fm_status                   (*InitBoot)(fm_switch *switchPtr);
    fm_status                   (*AllocateDataStructures)(fm_switch *switchPtr);
    fm_status                   (*FreeDataStructures)(fm_switch *switchPtr);
    fm_status                   (*DbgDumpScheduler)(fm_int sw);

    /**************************************************
     * Switch Attributes
     **************************************************/
    fm_status                   (*GetSwitchInfo)(fm_int sw, fm_switchInfo *info);
    fm_status                   (*GetSwitchAttribute)(fm_int sw, fm_int attr, void *value);
    fm_status                   (*SetSwitchAttribute)(fm_int sw, fm_int attr, void *value);
    fm_status                   (*ComputeFHClockFreq)(fm_int sw, fm_float *fhMhz);
    fm_status                   (*GetCpuPort)(fm_int sw, fm_int *cpuPort);
    fm_status                   (*SetCpuPort)(fm_int sw, fm_int cpuPort);
                                  
    /**************************************************
     * Switch State Manipulation
     **************************************************/
    fm_status                   (*ResetSwitch)(fm_int sw);
    fm_status                   (*ReleaseSwitch)(fm_int sw);
    fm_status                   (*PostBootSwitch)(fm_int sw);
    fm_status                   (*FreeResources)(fm_int sw);
    fm_status                   (*SetSwitchState)(fm_int sw, fm_bool state);
#if 0
    fm_status                   (*GetSwitchState)(fm_int sw, fm_bool *state);
#endif

    /**************************************************
     * Logical Port Management
     **************************************************/
    fm_status                   (*AllocLogicalPort)(fm_int       sw,
                                                    fm_portType  type,
                                                    fm_int       numPorts,
                                                    fm_int       *firstPortNumber,
                                                    fm_int       useHandle);

    fm_status                   (*AllocVirtualLogicalPort)(fm_int  sw,
                                                           fm_int  pepNb,
                                                           fm_int  numPorts,
                                                           fm_int *firstPortNumber,
                                                           fm_int  useHandle,
                                                           fm_int  firstGlort);

    fm_status                   (*FreeLogicalPort)(fm_int sw, fm_int port);

    fm_status                   (*FreeVirtualLogicalPort)(fm_int sw,
                                                          fm_int pepNb,
                                                          fm_int firstGlort,
                                                          fm_int numberOfPorts);

    fm_status                   (*CreateLogicalPort)(fm_int sw, fm_int port);

    fm_status                   (*SetLogicalPortAttribute)(fm_int   sw,
                                                           fm_int   port,
                                                           fm_int   attr,
                                                           void *   value);

    fm_status                   (*GetLogicalPortAttribute)(fm_int   sw,
                                                           fm_int   port,
                                                           fm_int   attr,
                                                           void *   value);

    fm_status                   (*WriteGlortCamEntry)(fm_int            sw,
                                                      fm_glortCamEntry* camEntry,
                                                      fm_camUpdateMode  mode);

    fm_status                   (*FreeDestEntry)(fm_int             sw,
                                                 fm_glortDestEntry *destEntry);

    fm_status                   (*SetGlortDestMask)(fm_int sw,
                                                    fm_glortDestEntry *destEntry,
                                                    fm_portmask * destMask);

    fm_status                   (*FreeLaneResources)( fm_int sw );

    fm_status                   (*GetPcieLogicalPort)(fm_int          sw,
                                                      fm_int          pep,
                                                      fm_pciePortType type,
                                                      fm_int          index,
                                                      fm_int *        logicalPort);

    fm_status                   (*GetLogicalPortPcie)(fm_int           sw,
                                                      fm_int           logicalPort,
                                                      fm_int *         pep,
                                                      fm_pciePortType *type,
                                                      fm_int *         index);

    /***************************************************
     * Port Attributes
     **************************************************/
    fm_bool                     (*IsCpuAttribute)(fm_int  sw,
                                                  fm_uint attr);

    fm_bool                     (*IsPerLagPortAttribute)(fm_int sw,
                                                         fm_uint attr);

    /**************************************************
     * PortSet Operations
     **************************************************/
    fm_status                   (*CreatePortSetInt)(fm_int  sw,
                                                    fm_int  portSet,
                                                    fm_bool isInternal);
    fm_status                   (*DeletePortSetInt)(fm_int  sw,
                                                    fm_int  portSet);
    fm_status                   (*AddPortSetPortInt)(fm_int  sw,
                                                     fm_int  portSet,
                                                     fm_int  port);
    fm_status                   (*DeletePortSetPortInt)(fm_int  sw,
                                                        fm_int  portSet,
                                                        fm_int  port);
    fm_status                   (*ClearPortSetInt)(fm_int  sw,
                                                   fm_int  portSet);
    fm_status                   (*SetPortSetPortInt)(fm_int  sw,
                                                     fm_int  portSet,
                                                     fm_int  port,
                                                     fm_bool state);
    /**************************************************
     * Generic port mapping
     **************************************************/
    fm_status                   (*DbgMapLogicalPort)(fm_int                 sw,
                                                     fm_int                 logPort,
                                                     fm_int                 lane,
                                                     fm_logPortMappingType  mappingType,
                                                     fm_int                *pMapped);

    /**************************************************
     * ACL Operations
     **************************************************/
    fm_status                   (*CreateACL)(fm_int    sw,
                                             fm_int    acl,
                                             fm_uint32 scenarios,
                                             fm_int    precedence);
    fm_status                   (*DeleteACL)(fm_int sw, fm_int acl);
    fm_status                   (*SetACLAttribute)(fm_int sw,
                                                   fm_int acl,
                                                   fm_int attr,
                                                   void * value);

    fm_status                   (*GetACLRuleAttribute)(fm_int sw,
                                                       fm_int acl,
                                                       fm_int rule,
                                                       fm_int attr,
                                                       void*  value);

    fm_status                   (*AddACLRule)(fm_int                sw,
                                              fm_int                acl,
                                              fm_int                rule,
                                              fm_aclCondition       cond,
                                              const fm_aclValue *   value,
                                              fm_aclActionExt       action,
                                              const fm_aclParamExt *param);
    fm_status                   (*DeleteACLRule)(fm_int sw,
                                                 fm_int acl,
                                                 fm_int rule);
    fm_status                   (*AddACLPort)(fm_int     sw,
                                              fm_int     acl,
                                              fm_int     port,
                                              fm_aclType type);
    fm_status                   (*DeleteACLPort)(fm_int                   sw,
                                                 fm_int                   acl,
                                                 const fm_aclPortAndType *portAndType);
    fm_status                   (*ClearACLPort)(fm_int sw, fm_int acl);
    fm_status                   (*CanonicalizeACLPort)(fm_int sw, fm_int *port);
    fm_status                   (*ValidateACLAttribute)(fm_int sw, fm_int attr);
    fm_status                   (*ValidateACLRule)(fm_int sw,
                                                   fm_aclCondition          cond,
                                                   const                    fm_aclValue *value,
                                                   fm_aclActionExt          action,
                                                   const                    fm_aclParamExt *param);
    fm_status                   (*CleanupACLRule)(fm_int sw, fm_int port, fm_aclRule *rule);
    fm_status                   (*PostSetPortACL)(fm_int sw, fm_int port, fm_bool first);
    fm_status                   (*CleanupPortACL)(fm_int sw, fm_int port, fm_bool last);
    fm_status                   (*GetACLCountExt)(fm_int sw,
                                                  fm_int                   acl,
                                                  fm_int                   rule,
                                                  fm_aclCounters *counters);
    fm_status                   (*ResetACLCount)(fm_int sw,
                                                 fm_int                   acl,
                                                 fm_int                   rule);
    fm_status                   (*GetACLEgressCount)(fm_int          sw,
                                                     fm_int          logicalPort,
                                                     fm_aclCounters *counters);
    fm_status                   (*ResetACLEgressCount)(fm_int sw,
                                                       fm_int logicalPort);
    fm_status                   (*SetACLRuleState)(fm_int sw,
                                                   fm_int acl,
                                                   fm_int rule);
    fm_status                   (*ExpandIncrementalACLs)(fm_int    sw,
                                                         fm_uint32 compileFlags);
    fm_status                   (*UpdateACLRule)(fm_int sw,
                                                 fm_int acl,
                                                 fm_int rule);
    fm_status                   (*ACLCompile)(fm_int    sw,
                                              fm_text   statusText,
                                              fm_int    statusTextLength,
                                              fm_uint32 flags,
                                              void *    value);
    fm_status                   (*ACLApplyExt)(fm_int    sw,
                                               fm_uint32 flags,
                                               void *    value);
    fm_status                   (*AddMapperEntry)(fm_int             sw,
                                                  fm_mapper          mapper,
                                                  void *             value,
                                                  fm_mapperEntryMode mode);
    fm_status                   (*DeleteMapperEntry)(fm_int             sw,
                                                     fm_mapper          mapper,
                                                     void *             value,
                                                     fm_mapperEntryMode mode);
    fm_status                   (*ClearMapper)(fm_int             sw,
                                               fm_mapper          mapper,
                                               fm_mapperEntryMode mode);
    fm_status                   (*GetMapperSize)(fm_int     sw,
                                                 fm_mapper  mapper,
                                                 fm_uint32 *mapperSize);
    fm_status                   (*GetMapperL4PortKey)(fm_int     sw,
                                                      fm_mapper  mapper,
                                                      fm_l4PortMapperValue *portMapValue,
                                                      fm_uint64 *key);
    fm_status                   (*IsVNTunnelInUseByACLs)(fm_int   sw,
                                                         fm_int   tunnelId,
                                                         fm_bool *inUse);
    fm_status                   (*GetAclSliceUsage)(fm_int               sw,
                                                    fm_int               acl,
                                                    fm_aclSliceUsage    *aclSliceUsage);
    fm_status                   (*GetAclFfuRuleUsage)(fm_int  sw,
                                                      fm_int *ffuRuleUsed,
                                                      fm_int *ffuRuleAvailable);

    /**************************************************
     * Policer Functions
     **************************************************/
    fm_status                   (*CreatePolicer)(fm_int                  sw,
                                                 fm_int                  bank,
                                                 fm_int                  policer,
                                                 const fm_policerConfig *config);
    fm_status                   (*DeletePolicer)(fm_int sw,
                                                 fm_int policer);
    fm_status                   (*SetPolicerAttribute)(fm_int      sw,
                                                       fm_int      policer,
                                                       fm_int      attr,
                                                       const void *value);
    fm_status                   (*UpdatePolicer)(fm_int sw,
                                                 fm_int policer);

    /**************************************************
     * MAC Address Table functions
     **************************************************/

    /* Adds an entry to the MA table. */
    fm_status   (*AddAddress)(fm_int              sw,
                              fm_macAddressEntry *entry);

    /* Preprocesses an entry to be added to the MA table. May be NULL. */
    fm_status   (*AddAddressToTablePre)(fm_int sw,
                                        fm_macAddressEntry *entry,
                                        fm_uint32           trigger,
                                        fm_bool             updateHw,
                                        fm_int              bank,
                                        fm_bool *           complete);

    /* Allocates MA table data structures. May be NULL. */
    fm_status   (*AllocAddrTableData)(fm_switch *switchPtr);

    /* Assigns an address to an entry in the MA table and writes it
     * to the hardware. */
    fm_status   (*AssignTableEntry)(fm_int              sw,
                                    fm_macAddressEntry *entry,
                                    fm_int              targetBank,
                                    fm_uint32           trigger,
                                    fm_bool             updateHw,
                                    fm_uint32*          numUpdates,
                                    fm_event**          outEvent);

    /* Preprocesses an MA table flush request. May be NULL. */
    fm_status  (*CheckFlushRequest)(fm_int          sw,
                                    fm_flushMode    mode,
                                    fm_flushParams *params);

    /* Computes the index (or indices) of an MA Table entry. */
    fm_status   (*ComputeAddressIndex)(fm_int     sw,
                                       fm_macaddr entry,
                                       fm_uint16  vlanID,
                                       fm_uint16  vlanID2,
                                       fm_uint16 *indexes);

    /* Overrides the body of the fmDeleteAddress[FromTable] function.
     * Usually NULL. (SWAG only) */
    fm_status   (*DeleteAddressOverride)(fm_int              sw,
                                         fm_macAddressEntry *entry,
                                         fm_bool             overrideMspt,
                                         fm_bool             updateHw,
                                         fm_int              bank);

    /* Preprocesses an entry to be deleted from the MA table. */
    fm_status   (*DeleteAddressPre)(fm_int sw,
                                    fm_macAddressEntry *entry);

    /* Deletes all addresses from the MA table. */
    fm_status   (*DeleteAllAddresses)(fm_int sw, fm_bool dynamicOnly);

    /* Dumps the MA Table purge statistics. May be NULL. */
    fm_status   (*DumpPurgeStats)(fm_int sw);

    /* Fills in a user MA Table entry structure from an internal MA Table
     * entry structure. */
    fm_status   (*FillInUserEntryFromTable)(fm_int                   sw,
                                            fm_internalMacAddrEntry *tblentry,
                                            fm_macAddressEntry      *entry);

    /* Scans the MA Table for the specified address/VLAN and invalidates
     * the entry. */
    fm_status   (*FindAndInvalidateAddress)(fm_int     sw, 
                                            fm_macaddr macAddress, 
                                            fm_uint16  vlanID,
                                            fm_uint16  vlanID2,
                                            fm_int     bank,
                                            fm_uint16 *indexes,
                                            fm_bool    updateHw);

    /* Frees MA table data structures. May be NULL. */
    fm_status   (*FreeAddrTableData)(fm_switch *switchPtr);

    /* Provided for the benefit of TestPoint. Returns the index and
     * bank of the MAC Table entry matching the specified MAC address
     * and VLAN ID. (FM6000 only) */
    fm_status   (*GetAddressIndex)(fm_int     sw,
                                   fm_macaddr macAddress,
                                   fm_int     vlanID,
                                   fm_int     vlanID2,
                                   fm_int *   index,
                                   fm_int *   bank);

    /* Overrides the body of the fmGetAddress[V2] function.
     * Usually NULL. (SWAG only) */
    fm_status   (*GetAddressOverride)(fm_int              sw,
                                      fm_macaddr          address,
                                      fm_uint16           vlanID,
                                      fm_uint16           vlanID2,
                                      fm_macAddressEntry *entry);

    /* Retrieves a copy of the MA table. */
    fm_status   (*GetAddressTable)(fm_int              sw,
                                   fm_int             *nEntries,
                                   fm_macAddressEntry *entries,
                                   fm_int              maxEntries);

    /* Returns the value of an MA table related attribute. */
    fm_status   (*GetAddressTableAttribute)(fm_int  sw,
                                            fm_int  attr,
                                            void   *value);

    /* Returns the FID on which MAC addresses are learned for a VLAN. */
    fm_status   (*GetLearningFID)(fm_int     sw, 
                                  fm_uint16  vlanID, 
                                  fm_uint16 *learningFid);

    /* Returns the MAC security statistics. May be NULL. */
    fm_status   (*GetSecurityStats)(fm_int              sw,
                                    fm_securityStats *  stats);

    /* Initializes MA table data structures. May be NULL. */
    fm_status   (*InitAddressTable)(fm_switch *switchPtr);

    /* Determines whether an MA table index is valid. May be NULL.
     * (FM6000 only) */
    fm_bool     (*IsIndexValid)(fm_int    sw,
                                fm_uint32 index);

    /* Reads an entry from the hardware MA table and returns its
     * internal representation. */
    fm_status   (*ReadEntryAtIndex)(fm_int                   sw,
                                    fm_uint32                index,
                                    fm_internalMacAddrEntry *entry);

    /* Resets the MA Table purge statistics. May be NULL. */
    fm_status   (*ResetPurgeStats)(fm_int sw);

    /* Resets the MAC security statistics. May be NULL. */
    fm_status   (*ResetSecurityStats)(fm_int sw);

    /* Sets the value of an MA table related attribute. */
    fm_status   (*SetAddressTableAttribute)(fm_int  sw,
                                            fm_int  attr,
                                            void   *value);

    /* Requests an MA Table Update. */
    fm_status   (*UpdateMATable)(fm_int                 sw,
                                 fm_maWorkType          workType,
                                 fm_maWorkTypeData      data,
                                 fm_addrMaintHandler    handler,
                                 void *                 context);

    /* Writes an entry to the hardware MA table given its
     * internal representation. */
    fm_status   (*WriteEntryAtIndex)(fm_int                   sw,
                                     fm_uint32                index,
                                     fm_internalMacAddrEntry *entry);

    /**************************************************
     * Link Aggregation Group support
     ***********************************************c***/
    fm_status   (*InformLAGPortUp)(fm_int sw, fm_int port);
    fm_status   (*InformLAGPortDown)(fm_int sw, fm_int port);
    fm_status   (*InformLBGLinkChange)(fm_int sw,
                                       fm_int port,
                                       fm_portLinkStatus linkStatus);

    fm_status   (*AllocateLAGs)(fm_int     sw,
                                fm_uint    startGlort,
                                fm_uint    glortSize,
                                fm_int    *baseLagHandle,
                                fm_int    *numLags,
                                fm_int    *step);
    fm_status   (*FreeStackLAGs)(fm_int sw, fm_int baseLagHandle);
    fm_status   (*CreateLagOnSwitch)(fm_int sw, fm_int lagIndex);
    fm_status   (*DeleteLagFromSwitch)(fm_int sw, fm_int lagIndex);
    void        (*FreeLAG)(fm_int sw, fm_int lagIndex);
    fm_status   (*DeletePortFromLag)(fm_int sw, fm_int lagIndex, fm_int port);
    fm_status   (*AddPortToLag)(fm_int sw, fm_int lagIndex, fm_int port);
    fm_status   (*GetLagAttribute)(fm_int   sw,
                                   fm_int   attribute,
                                   fm_int   index,
                                   void *   value);

    fm_status   (*SetLagAttribute)(fm_int   sw,
                                   fm_int   attribute,
                                   fm_int   index,
                                   void *   value);

    fm_status   (*SetPortLAGConfig)(fm_int  sw, 
                                    fm_int  physPort,
                                    fm_int  index,
                                    fm_int  lagSize,
                                    fm_uint hashRotation,
                                    fm_bool inLag);
    
    /**************************************************
     * Mirror Support
     **************************************************/
    fm_status   (*CreateMirror)(fm_int              sw,
                                fm_portMirrorGroup *grp);

    fm_status   (*DeleteMirror)(fm_int              sw,
                                fm_portMirrorGroup *grp);

    fm_status   (*WritePortMirrorGroup)(fm_int              sw,
                                        fm_portMirrorGroup *grp);

    fm_status   (*UpdateMirrorGroups)(fm_int sw, 
                                      fm_int physPort, 
                                      fm_bool up);

    fm_status   (*AddMirrorPort)(fm_int              sw,
                                 fm_portMirrorGroup *grp,
                                 fm_int              port,
                                 fm_mirrorType       mirrorType);

    fm_status   (*DeleteMirrorPort)(fm_int              sw,
                                    fm_portMirrorGroup *grp,
                                    fm_int              port);

    fm_status   (*SetMirrorDestination)(fm_int              sw,
                                        fm_portMirrorGroup *grp,
                                        fm_int              mirrorPort);

    fm_status   (*SetMirrorAttribute)(fm_int              sw,
                                      fm_portMirrorGroup *grp,
                                      fm_int              attr,
                                      void *              value);

    fm_status   (*GetMirrorAttribute)(fm_int              sw,
                                      fm_portMirrorGroup *grp,
                                      fm_int              attr,
                                      void *              value);

    fm_status   (*AddMirrorVlan)(fm_int              sw,
                                 fm_portMirrorGroup *grp,
                                 fm_vlanSelect       vlanSel,
                                 fm_uint16           vlanID,
                                 fm_mirrorVlanType   direction);

    fm_status   (*DeleteMirrorVlan)(fm_int              sw,
                                    fm_portMirrorGroup *grp,
                                    fm_vlanSelect       vlanSel,
                                    fm_uint16           vlanID);

    /**************************************************
     * QOS Support
     **************************************************/
    fm_status   (*SetSwitchQOS)(fm_int sw,
                                fm_int attr,
                                fm_int index,
                                void * value);
    fm_status   (*GetSwitchQOS)(fm_int sw,
                                fm_int attr,
                                fm_int index,
                                void * value);

    /**************************************************
     * Statistics Support
     **************************************************/
    fm_status   (*GetCountersInitMode)(fm_int     sw, 
                                       fm_uint32 *initMode);
    fm_status   (*GetPortCounters)(fm_int           sw,
                                   fm_int           port,
                                   fm_portCounters *counters);
    fm_status   (*AllocateVLANCounters)(fm_int sw, fm_int vlan);
    fm_status   (*FreeVLANCounters)(fm_int sw, fm_int vlan);
    fm_status   (*GetVLANCounters)(fm_int           sw,
                                   fm_int           vcid,
                                   fm_vlanCounters *counters);
    fm_status   (*ResetVLANCounters)(fm_int sw,
                                     fm_int vcid);
    fm_status   (*GetSwitchCounters)(fm_int             sw, 
                                     fm_switchCounters *counters);

    /**************************************************
     * Vlan Support
     **************************************************/
    fm_status   (*WriteVlanEntry)(fm_int sw, fm_uint16 vlanID);
    fm_status   (*WriteTagEntry)(fm_int sw, fm_uint16 vlanID);
    fm_status   (*SetVlanAttribute)(fm_int sw,
                                    fm_uint16                vlanID,
                                    fm_int                   attr,
                                    void *                   value);
    fm_status   (*GetVlanAttribute)(fm_int sw,
                                    fm_uint16                vlanID,
                                    fm_int                   attr,
                                    void *                   value);
    fm_status   (*AllocateVlanTableDataStructures)(fm_switch *switchPtr);
    fm_status   (*FreeVlanTableDataStructures)(fm_switch *switchPtr);
    fm_status   (*InitVlanTable)(fm_switch *switchPtr);
    fm_status   (*SetVlanCounterID)(fm_int sw, fm_uint vlanID, fm_uint vcnt);
    fm_status   (*CreateVlan)(fm_int sw, fm_uint16 vlanID);
    fm_status   (*ResetVlanSpanningTreeState)(fm_int sw, fm_uint16 vlanID);
    fm_status   (*SetVlanPortState)(fm_int sw,
                                    fm_uint16                vlanID,
                                    fm_int                   port,
                                    fm_int                   state);
    fm_status   (*GetVlanPortState)(fm_int sw,
                                    fm_uint16                vlanID,
                                    fm_int                   port,
                                    fm_int *                 state);

    fm_status   (*DeleteVlan)(fm_int    sw,
                              fm_uint16 vlanID);
    fm_status   (*AddVlanPort)(fm_int    sw,
                               fm_uint16 vlanID,
                               fm_int    port,
                               fm_bool   tag);
    fm_status   (*DeleteVlanPort)(fm_int    sw,
                                  fm_uint16 vlanID,
                                  fm_int    port);
    fm_status   (*ChangeVlanPort)(fm_int        sw,
                                  fm_vlanSelect vlanSel,
                                  fm_uint16     vlanID,
                                  fm_int        port,
                                  fm_bool       tag);
    fm_status   (*GetVlanPortAttribute)(fm_int    sw,
                                        fm_uint16 vlanID,
                                        fm_int    port,
                                        fm_int    attr,
                                        void *    value);

    fm_status   (*AddVlanPortList)(fm_int    sw,
                                   fm_uint16 vlanID,
                                   fm_int    numPorts,
                                   fm_int *  portList,
                                   fm_bool   tag);
    fm_status   (*DeleteVlanPortList)(fm_int    sw,
                                      fm_uint16 vlanID,
                                      fm_int    numPorts,
                                      fm_int *  portList);
    fm_status   (*SetVlanPortListState)(fm_int    sw,
                                        fm_uint16 vlanID,
                                        fm_int    numPorts,
                                        fm_int *  portList,
                                        fm_int    state);

    /* functions to manage customer VLANs for provider bridging */
    fm_status   (*AddCVlan)(fm_int      sw,
                            fm_int      port,
                            fm_uint16   cVlan,
                            fm_uint16   sVlan);
    fm_status   (*DeleteCVlan)(fm_int       sw,
                               fm_int       port,
                               fm_uint16    cVlan);
    fm_status   (*GetCVlanFirst)(fm_int     sw,
                                 fm_int *   firstPort,
                                 fm_int *   firstCVlan);
    fm_status   (*GetCVlanNext)(fm_int      sw,
                                fm_int      startPort,
                                fm_uint16   startCVlan,
                                fm_int *    nextPort,
                                fm_int *    nextCVlan);
    fm_status   (*GetSVlanFromPortCVlan)(fm_int     sw,
                                         fm_int     port,
                                         fm_uint16  cVlan,
                                         fm_int *   sVlan); 
    void        (*DbgDumpCVlanCounter)(fm_int sw);

    /**************************************************
     * Spanning Tree Support
     **************************************************/
    fm_status   (*RefreshSpanningTree)(fm_int              sw,
                                       fm_stpInstanceInfo *instance,
                                       fm_int              vlan, 
                                       fm_int              port);
    fm_status   (*CreateSpanningTree)(fm_int sw, 
                                      fm_int stpInstance);
    fm_status   (*DeleteSpanningTree)(fm_int sw, 
                                      fm_int stpInstance);
    fm_status   (*AddSpanningTreeVlan)(fm_int sw,
                                       fm_int stpInstance,
                                       fm_int vlanID);
    fm_status   (*DeleteSpanningTreeVlan)(fm_int sw,
                                          fm_int stpInstance,
                                          fm_int vlanID);
    fm_status   (*SetSpanningTreePortState)(fm_int sw,
                                            fm_int stpInstance,
                                            fm_int port,
                                            fm_int stpState);
    fm_status   (*GetSpanningTreePortState)(fm_int sw,
                                            fm_int stpInstance,
                                            fm_int port,
                                            fm_int *stpState);

    /**************************************************
     * Flow API Support
     **************************************************/
    fm_status   (*CreateFlowTCAMTable)(fm_int           sw, 
                                       fm_int           tableIndex, 
                                       fm_flowCondition condition,
                                       fm_uint32        maxEntries,
                                       fm_uint32        maxAction);
    fm_status   (*DeleteFlowTCAMTable)(fm_int  sw, 
                                       fm_int  tableIndex);
    fm_status   (*CreateFlowBSTTable)(fm_int           sw, 
                                      fm_int           tableIndex, 
                                      fm_flowCondition condition,
                                      fm_uint32        maxEntries,
                                      fm_uint32        maxAction);
    fm_status   (*DeleteFlowBSTTable)(fm_int  sw, 
                                      fm_int  tableIndex);
    fm_status   (*CreateFlowTETable)(fm_int           sw, 
                                     fm_int           tableIndex, 
                                     fm_flowCondition condition,
                                     fm_uint32        maxEntries,
                                     fm_uint32        maxAction);
    fm_status   (*DeleteFlowTETable)(fm_int  sw, 
                                     fm_int  tableIndex);
    fm_status   (*AddFlow)(fm_int           sw, 
                           fm_int           tableIndex,
                           fm_uint16        priority,
                           fm_uint32        precedence, 
                           fm_flowCondition condition,
                           fm_flowValue *   condVal,
                           fm_flowAction    action,
                           fm_flowParam *   param,
                           fm_flowState     flowState,
                           fm_int *         flowId);
    fm_status   (*GetFlow)(fm_int             sw, 
                           fm_int             tableIndex,
                           fm_int             flowId,
                           fm_flowCondition * flowCond,
                           fm_flowValue *     flowValue,
                           fm_flowAction *    flowAction,
                           fm_flowParam *     flowParam,
                           fm_int *           priority,
                           fm_int *           precedence,
                           fm_int *           tunnelGrp);
    fm_status   (*ModifyFlow)(fm_int           sw, 
                              fm_int           tableIndex,
                              fm_int           flowId,
                              fm_uint16        priority,
                              fm_uint32        precedence, 
                              fm_flowCondition condition,
                              fm_flowValue *   condVal,
                              fm_flowAction    action,
                              fm_flowParam *   param);
    fm_status   (*GetFlowTableType)(fm_int             sw,
                                    fm_int             tableIndex,
                                    fm_flowTableType * flowTableType);
    fm_status   (*GetFlowFirst)(fm_int   sw,
                                fm_int * firstTable);
    fm_status   (*GetFlowNext)(fm_int   sw,
                               fm_int   currentTable,
                               fm_int * nextTable);
    fm_status   (*GetFlowRuleFirst)(fm_int   sw,
                                    fm_int   tableIndex,
                                    fm_int * firstRule);
    fm_status   (*GetFlowRuleNext)(fm_int   sw,
                                   fm_int   tableIndex,
                                   fm_int   currentRule,
                                   fm_int * nextRule);
    fm_status   (*DeleteFlow)(fm_int sw,
                              fm_int tableIndex,
                              fm_int flowId);
    fm_status   (*SetFlowState)(fm_int       sw, 
                                fm_int       tableIndex, 
                                fm_int       flowId, 
                                fm_flowState flowState);
    fm_status   (*GetFlowCount)(fm_int           sw, 
                                fm_int           tableIndex, 
                                fm_int           flowId,
                                fm_flowCounters *counters);
    fm_status   (*ResetFlowCount)(fm_int sw, 
                                  fm_int tableIndex, 
                                  fm_int flowId);
    fm_status   (*GetFlowUsed)(fm_int   sw, 
                               fm_int   tableIndex, 
                               fm_int   flowId,
                               fm_bool  clear,
                               fm_bool *used);
    fm_status   (*SetFlowAttribute)(fm_int sw,
                                    fm_int tableIndex,
                                    fm_int attr,
                                    void * value);
    fm_status   (*GetFlowAttribute)(fm_int sw,
                                    fm_int tableIndex,
                                    fm_int attr,
                                    void * value);
    fm_status   (*InitFlowApiForSWAG)(fm_int sw,
                                      fm_int defaultLogicalPort,
                                      fm_int trapLogicalPort,
                                      fm_int fwdToCpuLogicalPort);
    fm_status   (*CreateFlowBalanceGrp)(fm_int  sw,
                                        fm_int *groupId);
    fm_status   (*DeleteFlowBalanceGrp)(fm_int sw,
                                        fm_int groupId);
    fm_status   (*AddFlowBalanceGrpEntry)(fm_int sw,
                                          fm_int groupId,
                                          fm_int tableIndex,
                                          fm_int flowId);
    fm_status   (*DeleteFlowBalanceGrpEntry)(fm_int sw,
                                             fm_int groupId,
                                             fm_int tableIndex,
                                             fm_int flowId);

    /**************************************************
     * Tunnel API Support
     **************************************************/
    fm_status   (*CreateTunnel)(fm_int sw,
                                fm_int *group,
                                fm_tunnelParam *tunnelParam);
    fm_status   (*DeleteTunnel)(fm_int sw, fm_int group);
    fm_status   (*GetTunnel)(fm_int sw,
                             fm_int group,
                             fm_tunnelParam *tunnelParam);
    fm_status   (*GetTunnelFirst)(fm_int sw,
                                  fm_int *firstGroup);
    fm_status   (*GetTunnelNext)(fm_int sw,
                                 fm_int currentGroup,
                                 fm_int *nextGroup);
    fm_status   (*AddTunnelEncapFlow)(fm_int sw,
                                      fm_int group,
                                      fm_int encapFlow,
                                      fm_tunnelEncapFlow field,
                                      fm_tunnelEncapFlowParam *param);
    fm_status   (*DeleteTunnelEncapFlow)(fm_int sw,
                                         fm_int group,
                                         fm_int encapFlow);
    fm_status   (*UpdateTunnelEncapFlow)(fm_int sw,
                                         fm_int group,
                                         fm_int encapFlow,
                                         fm_tunnelEncapFlow field,
                                         fm_tunnelEncapFlowParam *param);
    fm_status   (*GetTunnelEncapFlow)(fm_int sw,
                                      fm_int group,
                                      fm_int encapFlow,
                                      fm_tunnelEncapFlow *field,
                                      fm_tunnelEncapFlowParam *param);
    fm_status   (*GetTunnelEncapFlowFirst)(fm_int sw,
                                           fm_int group,
                                           fm_int *firstEncapFlow,
                                           fm_tunnelEncapFlow *field,
                                           fm_tunnelEncapFlowParam *param);
    fm_status   (*GetTunnelEncapFlowNext)(fm_int sw,
                                          fm_int group,
                                          fm_int currentEncapFlow,
                                          fm_int *nextEncapFlow,
                                          fm_tunnelEncapFlow *field,
                                          fm_tunnelEncapFlowParam *param);
    fm_status   (*AddTunnelRule)(fm_int sw,
                                 fm_int group,
                                 fm_int rule,
                                 fm_tunnelCondition cond,
                                 fm_tunnelConditionParam *condParam,
                                 fm_tunnelAction action,
                                 fm_tunnelActionParam *actParam);
    fm_status   (*DeleteTunnelRule)(fm_int sw,
                                    fm_int group,
                                    fm_int rule);
    fm_status   (*UpdateTunnelRule)(fm_int sw,
                                    fm_int group,
                                    fm_int rule,
                                    fm_tunnelCondition cond,
                                    fm_tunnelConditionParam *condParam,
                                    fm_tunnelAction action,
                                    fm_tunnelActionParam *actParam);
    fm_status   (*GetTunnelRule)(fm_int sw,
                                 fm_int group,
                                 fm_int rule,
                                 fm_tunnelCondition *cond,
                                 fm_tunnelConditionParam *condParam,
                                 fm_tunnelAction *action,
                                 fm_tunnelActionParam *actParam);
    fm_status   (*GetTunnelRuleFirst)(fm_int sw,
                                      fm_int group,
                                      fm_int *firstRule,
                                      fm_tunnelCondition *cond,
                                      fm_tunnelConditionParam *condParam,
                                      fm_tunnelAction *action,
                                      fm_tunnelActionParam *actParam);
    fm_status   (*GetTunnelRuleNext)(fm_int sw,
                                     fm_int group,
                                     fm_int currentRule,
                                     fm_int *nextRule,
                                     fm_tunnelCondition *cond,
                                     fm_tunnelConditionParam *condParam,
                                     fm_tunnelAction *action,
                                     fm_tunnelActionParam *actParam);
    fm_status   (*GetTunnelRuleCount)(fm_int sw,
                                      fm_int group,
                                      fm_int rule,
                                      fm_tunnelCounters *counters);
    fm_status   (*GetTunnelEncapFlowCount)(fm_int sw,
                                           fm_int group,
                                           fm_int encapFlow,
                                           fm_tunnelCounters *counters);
    fm_status   (*GetTunnelRuleUsed)(fm_int sw,
                                     fm_int group,
                                     fm_int rule,
                                     fm_bool *used);
    fm_status   (*ResetTunnelRuleCount)(fm_int sw,
                                        fm_int group,
                                        fm_int rule);
    fm_status   (*ResetTunnelEncapFlowCount)(fm_int sw,
                                             fm_int group,
                                             fm_int encapFlow);
    fm_status   (*ResetTunnelRuleUsed)(fm_int sw,
                                       fm_int group,
                                       fm_int rule);
    fm_status   (*SetTunnelAttribute)(fm_int sw,
                                      fm_int group,
                                      fm_int rule,
                                      fm_int attr,
                                      void *value);
    fm_status   (*GetTunnelAttribute)(fm_int sw,
                                      fm_int group,
                                      fm_int rule,
                                      fm_int attr,
                                      void *value);
    fm_status   (*DbgDumpTunnel)(fm_int sw);

    /**************************************************
     * NAT API Support
     **************************************************/
    fm_status   (*CreateNatTable)(fm_int sw,
                                  fm_int table,
                                  fm_natParam *natParam);
    fm_status   (*DeleteNatTable)(fm_int sw,
                                  fm_int table);
    fm_status   (*SetNatTunnelDefault)(fm_int sw,
                                       fm_natTunnelDefault *tunnelDefault);
    fm_status   (*CreateNatTunnel)(fm_int        sw,
                                   fm_int        table,
                                   fm_int        tunnel,
                                   fm_natTunnel *param);
    fm_status   (*DeleteNatTunnel)(fm_int sw,
                                   fm_int table,
                                   fm_int tunnel);
    fm_status   (*AddNatRule)(fm_int                sw,
                              fm_int                table,
                              fm_int                rule,
                              fm_natCondition       condition,
                              fm_natConditionParam *cndParam,
                              fm_natAction          action,
                              fm_natActionParam *   actParam);
    fm_status   (*DeleteNatRule)(fm_int sw,
                                 fm_int table,
                                 fm_int rule);
    fm_status   (*AddNatPrefilter)(fm_int                sw,
                                   fm_int                table,
                                   fm_int                entry,
                                   fm_natCondition       condition,
                                   fm_natConditionParam *cndParam);
    fm_status   (*DeleteNatPrefilter)(fm_int sw,
                                      fm_int table,
                                      fm_int entry);
    fm_status   (*GetNatRuleCount)(fm_int             sw,
                                   fm_int             table,
                                   fm_int             rule,
                                   fm_tunnelCounters *counters);
    fm_status   (*ResetNatRuleCount)(fm_int sw,
                                     fm_int table,
                                     fm_int rule);

    /**************************************************
     * Storm Controller Support
     **************************************************/
    fm_status   (*CreateStormCtrl)(fm_int sw, fm_int *stormController);
    fm_status   (*DeleteStormCtrl)(fm_int sw, fm_int stormController);
    fm_status   (*SetStormCtrlAttribute)( fm_int sw, 
                                          fm_int stormController, 
                                          fm_int attr, 
                                          void *value );
    fm_status   (*GetStormCtrlAttribute)( fm_int sw, 
                                          fm_int stormController, 
                                          fm_int attr, 
                                          void *value );
    fm_status   (*GetStormCtrlList)( fm_int sw, 
                                     fm_int *numStormControllers, 
                                     fm_int *stormControllers,
                                     fm_int max);
    fm_status   (*GetStormCtrlFirst)(fm_int sw, fm_int *firstStormController);
    fm_status   (*GetStormCtrlNext)( fm_int sw, 
                                     fm_int currentStormController,
                                     fm_int *nextStormController );
    fm_status   (*AddStormCtrlCondition)( fm_int             sw, 
                                          fm_int             stormController,
                                          fm_stormCondition *condition );
    fm_status   (*DeleteStormCtrlCondition)( fm_int            sw, 
                                             fm_int            stormController,
                                             fm_stormCondition *condition);
    fm_status   (*AddStormCtrlAction)( fm_int          sw, 
                                       fm_int          stormController, 
                                       fm_stormAction *action);
    fm_status   (*DeleteStormCtrlAction)(fm_int          sw, 
                                         fm_int          stormController, 
                                         fm_stormAction *action);
    fm_status   (*GetStormCtrlConditionList)( fm_int sw, 
                                              fm_int stormController, 
                                              fm_int *numConditions,
                                              fm_stormCondition *conditionList, 
                                              fm_int max );

    fm_status   (*GetStormCtrlConditionFirst)(fm_int           sw, 
                                              fm_int           stormController,
                                              fm_stormCondition *firstCondition);

    fm_status   (*GetStormCtrlConditionNext)(fm_int            sw, 
                                             fm_int            stormController,
                                             fm_stormCondition *currentCondition,
                                             fm_stormCondition *nextCondition);
    fm_status   (*GetStormCtrlActionList)( fm_int          sw, 
                                           fm_int          stormController, 
                                           fm_int         *numActions,
                                           fm_stormAction *actionList, 
                                           fm_int          max );
    fm_status   (*GetStormCtrlActionFirst)( fm_int          sw, 
                                            fm_int          stormController,
                                            fm_stormAction *firstAction );
    fm_status   (*GetStormCtrlActionNext)( fm_int          sw, 
                                           fm_int          stormController,
                                           fm_stormAction *currentAction, 
                                           fm_stormAction *nextAction );
    void        (*DbgDumpStormCtrl)(fm_int sw, fm_int stormController);

    /**************************************************
     * RBridge API Support
     **************************************************/
    fm_status   (*CreateRBridge)(fm_int            sw, 
                                 fm_remoteRBridge *rbridge, 
                                 fm_int *          tunnelId);
    fm_status   (*DeleteRBridge)(fm_int sw, fm_int tunnelId);
    fm_status   (*UpdateRBridgeEntry)(fm_int            sw, 
                                      fm_int            tunnelId,
                                      fm_remoteRBridge *rbridge);
    fm_status   (*GetRBridgeEntry)(fm_int            sw, 
                                   fm_int            tunnelId,
                                   fm_remoteRBridge *rbridge);
    fm_status   (*GetRBridgeFirst)(fm_int            sw, 
                                   fm_int *          tunnelId,
                                   fm_remoteRBridge *rbridge);
    fm_status   (*GetRBridgeNext)(fm_int            sw, 
                                  fm_int            currentTunnelId,
                                  fm_int *          nextTunnelId,
                                  fm_remoteRBridge *rbridge);

    fm_status   (*CreateRBridgeDistTree)(fm_int       sw, 
                                         fm_distTree *distTree, 
                                         fm_int *     tunnelId);
    fm_status   (*DeleteRBridgeDistTree)(fm_int sw, fm_int tunnelId);
    fm_status   (*UpdateRBridgeDistTree)(fm_int       sw, 
                                         fm_int       tunnelId,
                                         fm_distTree *distTree);
    fm_status   (*GetRBridgeDistTree)(fm_int       sw, 
                                      fm_int       tunnelId,
                                      fm_distTree *distTree);
    fm_status   (*GetRBridgeDistTreeFirst)(fm_int       sw,
                                           fm_int *     tunnelId, 
                                           fm_distTree *distTree);
    fm_status   (*GetRBridgeDistTreeNext)(fm_int       sw,
                                          fm_int       currentTunnelId,
                                          fm_int *     nextTunnelId,
                                          fm_distTree *distTree);
    fm_status   (*SetRBridgePortHopCount)(fm_int    sw,
                                          fm_int    port,
                                          fm_uint32 value);
    fm_status   (*GetRBridgePortHopCount)(fm_int     sw,
                                          fm_int     port,
                                          fm_uint32 *value);

    /**************************************************
     * Interrupt Handling
     **************************************************/
    fm_status   (*InterruptHandler)(fm_switch *switchPtr);
    fm_status   (*CheckFaultStates)(fm_int sw);

    /**************************************************
     * Debugging Support
     **************************************************/
    void        (*DbgReadRegister)(fm_int   sw,
                                   fm_int   firstIndex,
                                   fm_int   secondIndex,
                                   fm_text  registerName,
                                   void *   values);
    void        (*DbgDumpRegister)(fm_int   sw,
                                   fm_int   port,
                                   char *   registerName);
    fm_status   (*DbgDumpRegisterV2)(fm_int  sw,
                                     fm_int  indexA,
                                     fm_int  indexB,
                                     fm_text registerName);
    fm_status   (*DbgDumpRegisterV3)(fm_int  sw,
                                     fm_int  indexA,
                                     fm_int  indexB,
                                     fm_int  indexC,
                                     fm_text registerName);
    void        (*DbgWriteRegister)(fm_int   sw,
                                    fm_int   port,
                                    fm_text  registerName,
                                    fm_int   val);
    fm_status   (*DbgWriteRegisterV2)(fm_int    sw, 
                                      fm_int    wordOffset, 
                                      fm_int    indexA, 
                                      fm_int    indexB, 
                                      fm_text   regName, 
                                      fm_uint32 value);
    fm_status   (*DbgWriteRegisterV3)(fm_int    sw, 
                                      fm_int    wordOffset, 
                                      fm_int    indexA, 
                                      fm_int    indexB, 
                                      fm_int    indexC, 
                                      fm_text   regName, 
                                      fm_uint32 value);
    fm_status   (*DbgWriteRegisterField)(fm_int    sw, 
                                         fm_int    indexA, 
                                         fm_int    indexB, 
                                         fm_int    indexC, 
                                         fm_text   regName, 
                                         fm_text   fieldName, 
                                         fm_uint64 value);
    void        (*DbgDumpMACTable)(fm_int sw, fm_int numEntries);
    void        (*DbgDumpMACCache)(fm_int sw, fm_int numEntries);
    void        (*DbgDumpMACTableEntry)( fm_int sw, 
                                         fm_macaddr address, 
                                         fm_uint16 vlan );
    void        (*DbgListRegisters)( fm_int sw, 
                                     fm_bool showGlobals, 
                                     fm_bool showPorts );
    void        (*DbgGetRegisterName)( fm_int   sw,
                                       fm_int   regId,
                                       fm_uint  regAddress,
                                       fm_text  regName,
                                       fm_uint  regNameLength,
                                       fm_bool *isPort,
                                       fm_int  *index0Ptr,
                                       fm_int  *index1Ptr,
                                       fm_int  *index2Ptr,
                                       fm_bool  logicalPorts,
                                       fm_bool  partialLongRegs );
    void        (*DbgWriteRegisterBits)(fm_int     sw,
                                        fm_uint    reg,
                                        fm_uint32  mask,
                                        fm_uint32  value);
    void        (*DbgTakeChipSnapshot)(fm_int                sw,
                                       fmDbgFulcrumSnapshot *pSnapshot,
                                       fm_regDumpCallback    callback);
    fm_status   (*DbgDumpPortMap)(fm_int sw, fm_int port, fm_int portType);
    fm_status   (*DbgDumpPortMasks)(fm_int sw);
    fm_status   (*DbgDumpLag)(fm_int sw);
    fm_status   (*DbgDumpTriggers)(fm_int sw);
    fm_status   (*DbgDumpTriggerUsage)(fm_int sw);
    fm_status   (*DbgDumpDeviceMemoryStats)(int sw);
    fm_status   (*DbgDumpGlortTable)(fm_int sw);
    fm_status   (*DbgDumpGlortDestTable)(fm_int sw, fm_bool raw);
    void        (*DbgDumpMulticastTables)(fm_int sw);
    fm_status   (*DbgDumpLBG)(fm_int sw, fm_int lbg);
    fm_status   (*DbgDumpMemoryUsage)(fm_int sw);
    fm_status   (*DbgDumpMemoryUsageV2)(fm_int  sw, 
                                        fm_int  rxPort,
                                        fm_int  txPort,
                                        fm_int  rxmp,
                                        fm_int  txmp,
                                        fm_int  bsg,
                                        fm_bool useSegments);
    fm_status   (*DbgDumpMemoryUsageV3)(fm_int  sw, 
                                        fm_int  rxPort,
                                        fm_int  txPort,
                                        fm_int  smp,
                                        fm_int  txTc,
                                        fm_int  bsg,
                                        fm_bool useSegments);
    fm_status   (*DbgDumpWatermarks)(fm_int sw);
    fm_status   (*DbgDumpWatermarksV2)(fm_int sw, 
                                       fm_int rxPort,
                                       fm_int txPort,
                                       fm_int rxmp,
                                       fm_int txmp,
                                       fm_int islPri);
    fm_status   (*DbgDumpWatermarksV3)(fm_int sw, 
                                       fm_int rxPort,
                                       fm_int txPort,
                                       fm_int smp,
                                       fm_int txTc,
                                       fm_int islPri);
    fm_status   (*DbgDumpQOS)(fm_int sw, fm_int port);
    fm_status   (*DbgDumpPortMax)(fm_int sw, fm_int port);
    fm_status   (*DbgDumpSwpriMap)(fm_int sw, fm_int attr);
    fm_status   (*DbgDumpPortIdxMap)(fm_int sw, 
                                     fm_int port, 
                                     fm_int attr);
    fm_status   (*DbgDumpVid)(fm_int sw);
    void        (*DbgDumpSpanningTree)(fm_int sw, fm_int instance);
    fm_status   (*DbgSwitchSelfTest)(fm_int sw);
    fm_status   (*DbgPolicerTest)(fm_int  sw,
                                  fm_int *portList,
                                  fm_int  portCnt,
                                  fm_bool mrlLimiter);
    void        (*DbgSelfTestEventHandler)(fm_event * event);
    void        (*DbgInitSwitchRegisterTable)(fm_int sw);
    void        (*DbgDumpMapper)(fm_int sw);
    void        (*DbgDumpFFU) ( fm_int  sw, 
                                fm_bool validSlicesOnly,
                                fm_bool validRulesOnly );
    void        (*DbgDumpBstTable) ( fm_int  sw );
    void        (*DbgDumpSBusRegister) ( fm_int  sw, 
                                         fm_int  sbusDevID,
                                         fm_int  devRegID,
                                         fm_bool writeReg );
    fm_status   (*DbgReadSBusRegister) ( fm_int     sw, 
                                         fm_int     sbusDevID, 
                                         fm_int     devRegID, 
                                         fm_bool    writeReg,
                                         fm_uint32 *value );
    fm_status   (*DbgWriteSBusRegister) ( fm_int     sw, 
                                          fm_int     sbusDevID, 
                                          fm_int     devRegID, 
                                          fm_uint32  value );
    void        (*DbgDumpEthSerDesRegister) ( fm_int  sw, 
                                            fm_int  port,
                                            fm_int  devRegID,
                                            fm_bool writeReg );
    fm_status   (*DbgReadEthSerDesRegister) ( fm_int     sw, 
                                           fm_int     port,
                                           fm_int     devRegID, 
                                           fm_bool    writeReg,
                                           fm_uint32 *value );
    fm_status   (*DbgWriteEthSerDesRegister) ( fm_int     sw, 
                                           fm_int     port,
                                           fm_int     devRegID, 
                                           fm_uint32  value );
    fm_status   (*DbgInterruptSpico) ( fm_int      sw,
                                       fm_int      cmd,
                                       fm_int      arg,
                                       fm_int      timeout,
                                       fm_uint32  *result );
    fm_status   (*DbgInitSerDes)(fm_int sw,
                                 fm_int serDes,
                                 fm_uint dataWidth,
                                 fm_uint rateSel);
    fm_status   (*DbgRunSerDesDfeTuning)(fm_int        sw,
                                         fm_int        serDes,
                                         fm_dfeMode    dfeMode,
                                         fm_int        dfeHf,
                                         fm_int        dfeLf,
                                         fm_int        dfeDc,
                                         fm_int        dfeBw);
    fm_status   (*DbgReadSerDesRegister)(fm_int     sw, 
                                         fm_int     serDes,
                                         fm_uint     reg, 
                                         fm_uint32 *value);
    fm_status   (*DbgWriteSerDesRegister)(fm_int     sw, 
                                          fm_int     serDes, 
                                          fm_uint    reg, 
                                          fm_uint32  value);
    fm_status   (*DbgDumpSerDes)(fm_int  sw, 
                                 fm_int  serDes,
                                 fm_text cmd);
    fm_status   (*DbgSetSerDesPreCursor)(fm_int     sw,
                                         fm_int     serDes,
                                         fm_int     preCursor);
    fm_status   (*DbgSetSerDesCursor)(fm_int     sw,
                                      fm_int     serDes,
                                      fm_int     cursor);
    fm_status   (*DbgSetSerDesPostCursor)(fm_int     sw,
                                          fm_int     serDes,
                                          fm_int     postCursor);
    fm_status   (*DbgResetSerDesErrorCounter)(fm_int     sw,
                                              fm_int     serDes);
    fm_status   (*DbgGetSerDesErrorCounter)(fm_int     sw,
                                            fm_int     serDes,
                                            fm_uint32 *pCounter);
    fm_status   (*DbgInjectSerDesErrors)(fm_int sw,
                                         fm_int serDes,
                                         fm_int serDesDir, 
                                         fm_uint numErrors);
    fm_status   (*DbgSetSerDesTxPattern)(fm_int  sw,
                                         fm_int  serDes,
                                         fm_text pattern);
    fm_status   (*DbgSetSerDesRxPattern)(fm_int  sw,
                                         fm_int  serDes,
                                         fm_text pattern);
    fm_status   (*DbgSetSerDesPolarity)(fm_int  sw,
                                        fm_int  serDes,
                                        fm_text polStr);
    fm_status   (*DbgSetSerDesLoopback)(fm_int  sw,
                                        fm_int  serDes,
                                        fm_text lbStr);

    void        (*DbgDumpL2LSweepers)(fm_int sw, fm_bool regs);
    void        (*DbgDumpPortAttributes)(fm_int sw, fm_int port);
    void        (*DbgDumpPolicers)(fm_int sw);
    fm_status   (*DbgDumpMirror)(fm_int sw);
    fm_status   (*DbgDumpMirrorProfile)(fm_int sw);
    void        (*DbgDumpSAFTable)(fm_int sw);

    fm_status   (*DbgDumpPortEeeStatus)(fm_int sw, 
                                        fm_int port,
                                        fm_bool clear);

    fm_status   (*DbgEnablePortEee)(fm_int sw, 
                                    fm_int port,
                                    fm_int mode);


    /**************************************************
     * port state machine related function pointers
     **************************************************/
    
    fm_status   (*DbgDumpPortStateTransitionsV2)( fm_int  sw,
                                                  fm_int  *portList,
                                                  fm_int  portCnt,
                                                  fm_int  maxEntries,
                                                  fm_text optionStr);
    fm_status   (*DbgDumpPortStateTransitions)( fm_int sw, 
                                                fm_int port );
    fm_status   (*DbgClearPortStateTransitions)( fm_int sw,
                                                 fm_int port );
    fm_status   (*DbgSetPortStateTransitionHistorySize)( fm_int sw,
                                                         fm_int port,  
                                                         fm_int size ); 

    /**************************************************
     * eye diagram related function pointers
     **************************************************/
    fm_status   (*DbgTakeEyeDiagram)(  fm_int  sw,
                                       fm_int  port,
                                       fm_int  mac,
                                       fm_int  lane,
                                       fm_int *count,
                                       fm_eyeDiagramSample
                                     **eyeDiagramPtr );
    fm_status   (*DbgPlotEyeDiagram)( fm_eyeDiagramSample *sampleTable );
    fm_status   (*DbgDeleteEyeDiagram)( fm_eyeDiagramSample *sampleTable );

    /**************************************************
     * MAC Table Maintenance Task functions
     **************************************************/

    /* The MAC Table Maintenance Task calls this function to service
     * the TCN FIFO. May be NULL. */
    fm_status   (*HandleMACTableEvents)(fm_int sw);

    /* The MAC Table Maintenance Task calls this function to service
     * a Purge Request event. May be NULL. */
    fm_status   (*HandlePurgeRequest)(fm_int sw);

    /* The MAC Table Maintenance Task calls this function to service
     * a Purge Complete event. May be NULL. */
    fm_status   (*HandlePurgeComplete)(fm_int sw);

    /* First-level MAC Table Scan Request handler.
     * The MAC Table Maintenance Task calls this function to service
     * a Scan request. May be NULL. */
    void        (*HandleScanRequest)(fm_int                sw,
                                     fm_addrMaintWorkList* workList,
                                     fm_thread*            thread,
                                     fm_thread*            eventHandler);

    /* The MAC Table Maintenance Task calls this function once per cycle
     * to perform switch-specific processing. May be NULL. */
    void        (*MiscMACMaintenance)(fm_int sw);

    /* The MAC Table Maintenance Task calls this function at the start of
     * processing for a MAC Table Overflow event. May be NULL. */
    fm_status   (*MacTableOverflowStart)(fm_int sw);

    /* The MAC Table Maintenance Task calls this function at the end of
     * processing for a MAC Table Overflow event. */
    fm_status   (*MacTableOverflowDone)(fm_int sw);

    /* The global event handler calls this function to determine
     * whether a learning event is stale, and if so, to discard it.
     * May be NULL. (FM4000) */ 
    fm_bool     (*RemoveStaleLearnEvent)(fm_int                    sw,         
                                         fm_eventTableUpdateBurst *updateEvent,
                                         fm_uint32                 index);                                     

    /**************************************************
     * Fast Maintenance Task functions
     **************************************************/

    /* The Fast Maintenance Task calls this function once per cycle
     * to perform switch-specific processing. May be NULL. */
    void *      (*FastMaintenanceTask)(fm_int sw, void *args);

    /**************************************************
     * Event Handling Functions
     **************************************************/
    fm_status   (*EventHandlingInitialize)(fm_int sw);

    void        (*ThrottleBroadcasts)(fm_int sw);

    fm_status   (*SendLinkUpDownEvent)(fm_int           sw,
                                       fm_int           physPort,
                                       fm_int           mac,
                                       fm_bool          linkUp,
                                       fm_eventPriority priority);

    void        (*DebounceLinkStates)(fm_int     sw,
                                      fm_thread *thread,
                                      fm_thread *handlerThread);

    fm_bool     (*CheckSecurityViolation)(fm_eventPktRecv *event,
                                          fm_thread *      event_handler);

    void        (*SysEventHandler)(fm_int    sw,
                                   fm_event *sysEvent,
                                   fm_thread *thread,
                                   fm_thread *event_handler);

    /**************************************************
     * Packet sending and receiving
     **************************************************/
    fm_status (*SendPackets)(fm_int          sw);

    fm_status (*SendPacket)(fm_int          sw,
                            fm_packetInfo * info,
                            fm_buffer *     pkt);
    fm_status (*SetPacketInfo)(fm_int           sw,
                               fm_packetInfo *  info,
                               fm_uint32        destMask);
    fm_status (*SendPacketDirected)(fm_int           sw,
                                    fm_int *         portList,
                                    fm_int           numPorts,
                                    fm_buffer *      pkt,
                                    fm_packetInfoV2 *info);
                            
    fm_status (*SendPacketSwitched)(fm_int      sw,
                                    fm_buffer * pkt);

    fm_status (*SendPacketISL)(fm_int           sw,
                               fm_uint32        *islTag,
                               fm_islTagFormat  islTagFormat,
                               fm_buffer        *pkt);

    fm_status (*GeneratePacketISL)(fm_int          sw,
                                   fm_buffer      *buffer,
                                   fm_packetInfo  *info,
                                   fm_int          cpuPort,
                                   fm_uint32       switchPriority,
                                   fm_islTagFormat *islTagFormat,
                                   fm_islTag       *islTag,
                                   fm_bool         *suppressVlanTag);

    /** \desc       Required only on systems that receive packets directly 
     *              to the CPU from the switch's LCI. 
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \return     FM_OK if successful.   */
    fm_status (*ReceivePacket)(fm_int sw);

    /**************************************************
     * FIBM Functions
     **************************************************/
     
    /** \desc       Required for FIBM-controlled devices only. Returns the 
     *              remote switch's FIBM configuration. 
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[out] config points to caller allocated storage where this
     *              function will place the remote switch's FIBM configuration.
     *
     *  \return     FM_OK if successful.   */
    fm_status (*GetFibmSwitchConfig)(fm_int sw, fm_fibmSwitchConfig *config);
    
    /** \desc       Required only for switches from which FIBM frames are 
     *              received, even if the frames originate from another 
     *              switch. This will be the remote switch itself if there 
     *              is no locally connected switch between the remote switch 
     *              and the CPU. 
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  pkt points to the buffer containing the management 
     *              message, which may or may not include an ISL tag
     *              (see islTag).
     *
     *  \param[in]  islTag points to an array of 32-bit words, being the ISL
     *              tag when the ISL tag is not included in the packet data
     *              pointed to by pkt. If pkt does include an ISL tag, islTag
     *              is NULL.
     *
     *  \return     FM_OK if successful.   */
    fm_status (*ProcessMgmtPacket)(fm_int sw, fm_buffer *pkt, fm_uint32 *islTag);
    
    /** \desc       Optional and only used on FIBM-controlled switches. 
     *              Turns on/off batching of multiple register writes into a 
     *              single FIBM frame for improved FIBM performance. 
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  enabled will be set to TRUE at the beginning of a batch
     *              and to FALSE when closing a batch. When the batch is
     *              closed, the FIBM frame should be generated with any
     *              outstanding register writes not yet sent in a previous
     *              FIBM frame. 
     *
     *  \return     FM_OK if successful.   */
    fm_status (*EnableFibmBatching)(fm_int sw, fm_bool enabled);


    /**************************************************
     * Routing Functions
     **************************************************/
    fm_status  (*RouterAlloc)(fm_int sw);
    fm_status  (*RouterFree)(fm_int sw);
    fm_status  (*RouterInit)(fm_int sw);
    fm_status  (*SetRouterAttribute)(fm_int sw,
                                     fm_int attr,
                                     void * value);
    fm_status  (*AddRoute)(fm_int sw,
                           fm_intRouteEntry *route);
    fm_status  (*DeleteRoute)(fm_int sw,
                              fm_intRouteEntry *route);
    fm_status  (*ReplaceECMPBaseRoute)(fm_int            sw,
                                       fm_intRouteEntry *oldRoute,
                                       fm_intRouteEntry *newRoute);
    fm_status  (*SetRouteActive)(fm_int sw,
                                 fm_intRouteEntry *route);
    fm_status  (*AddVirtualRouter)(fm_int sw, fm_int vroff);
    fm_status  (*RemoveVirtualRouter)(fm_int sw, fm_int vroff);
    fm_status  (*SetRouterState)(fm_int         sw,
                                 fm_int         vroff,
                                 fm_routerState state);
    fm_status  (*SetRouterMacMode)(fm_int           sw,
                                   fm_int           vroff,
                                   fm_routerMacMode mode);
    void       (*DbgDumpRouteStats)(fm_int sw);
    void       (*DbgDumpRouteTables)(fm_int sw, fm_int flags);
    fm_status  (*DbgValidateRouteTables)(fm_int sw);
    fm_status  (*SetRouteAttribute)(fm_int            sw,
                                    fm_intRouteEntry *route,
                                    fm_int            attr,
                                    void *            value);
    fm_status  (*GetRouteAttribute)(fm_int            sw,
                                    fm_intRouteEntry *route,
                                    fm_int            attr,
                                    void *            value);
    void *     (*RoutingMaintenanceTask)(fm_int sw);
    fm_status  (*SetRouteAction)(fm_int            sw,
                                 fm_intRouteEntry *route);


    /**************************************************
     * Virtual Networking Functions
     **************************************************/
    fm_status  (*CreateVirtualNetwork)(fm_int             sw,
                                       fm_virtualNetwork *vn);
    fm_status  (*DeleteVirtualNetwork)(fm_int             sw,
                                       fm_virtualNetwork *vn);
    fm_status  (*UpdateVirtualNetwork)(fm_int             sw,
                                       fm_virtualNetwork *vn,
                                       fm_vnDescriptor *  oldDescriptor);
    fm_status  (*FreeVirtualNetwork)(fm_int             sw,
                                     fm_virtualNetwork *vn);
    fm_status  (*CreateVNTunnel)(fm_int       sw,
                                 fm_vnTunnel *tunnel);
    fm_status  (*DeleteVNTunnel)(fm_int       sw,
                                 fm_vnTunnel *tunnel);
    fm_status  (*SetVNTunnelAttribute)(fm_int              sw,
                                       fm_vnTunnel *       tunnel,
                                       fm_vnTunnelAttrType attr,
                                       void *              value);
    fm_status  (*UpdateVNTunnelECMPGroup)(fm_int       sw,
                                          fm_vnTunnel *tunnel);
    fm_status  (*FreeVNTunnel)(fm_int       sw,
                               fm_vnTunnel *tunnel);
    fm_status  (*AddVNRemoteAddress)(fm_int             sw,
                                     fm_virtualNetwork *vn,
                                     fm_vnTunnel *      tunnel,
                                     fm_vnAddress *     addr);
    fm_status  (*DeleteVNRemoteAddress)(fm_int             sw,
                                        fm_virtualNetwork *vn,
                                        fm_vnTunnel *      tunnel,
                                        fm_vnAddress *     addr);
    fm_status  (*AddVNRemoteAddressMask)(fm_int             sw,
                                         fm_virtualNetwork *vn,
                                         fm_vnTunnel *      tunnel,
                                         fm_vnAddress *     baseAddr,
                                         fm_vnAddress *     addrMask);
    fm_status  (*DeleteVNRemoteAddressMask)(fm_int             sw,
                                            fm_virtualNetwork *vn,
                                            fm_vnAddress *     baseAddr,
                                            fm_vnAddress *     addrMask);
    fm_status  (*ConfigureVN)(fm_int sw, fm_vnConfiguration *config);
    fm_status  (*FreeVNResources)(fm_int sw);
    fm_status  (*AddVNLocalPort)(fm_int             sw,
                                 fm_virtualNetwork *vn,
                                 fm_int             port);
    fm_status  (*DeleteVNLocalPort)(fm_int             sw,
                                    fm_virtualNetwork *vn,
                                    fm_int             port);
    fm_status  (*AddVNVsi)(fm_int             sw,
                           fm_virtualNetwork *vn,
                           fm_int             vsi);
    fm_status  (*DeleteVNVsi)(fm_int             sw,
                              fm_virtualNetwork *vn,
                              fm_int             vsi);


    /**************************************************
     * NextHop Functions
     **************************************************/
    fm_status  (*NextHopAlloc)(fm_int sw);
    fm_status  (*NextHopFree)(fm_int sw);
    fm_status  (*NextHopInit)(fm_int sw);
    fm_status  (*SetInterfaceAttribute)(fm_int sw,
                                        fm_int interface,
                                        fm_int attr,
                                        void * value);
    void       (*DbgDumpArpTable)(fm_int sw, fm_bool verbose);
    void       (*DbgDumpArpHandleTable)(fm_int sw, fm_bool verbose);
    void       (*DbgPlotUsedArpDiagram)(fm_int sw);
    fm_status  (*CreateInterface)(fm_int  sw, fm_int ifNum);
    fm_status  (*DeleteInterface)(fm_int  sw, fm_int ifNum);
    fm_status  (*AddInterfaceAddr)(fm_int     sw,
                                   fm_int     interface,
                                   fm_ipAddr *addr);
    fm_status  (*DeleteInterfaceAddr)(fm_int     sw,
                                      fm_int     interface,
                                      fm_ipAddr *addr);
    fm_status  (*CreateECMPGroup)(fm_int           sw,
                                  fm_intEcmpGroup *group);
    fm_status  (*DeleteECMPGroup)(fm_int           sw,
                                  fm_intEcmpGroup *group);
    fm_status  (*FreeEcmpGroup)(fm_int           sw,
                                fm_intEcmpGroup *group);
    fm_status  (*ValidateECMPGroupDeletion)(fm_int   sw,
                                            fm_int   ecmpGroupId,
                                            fm_bool *pMayBeDeleted);
    fm_status  (*AddECMPGroupNextHops)(fm_int           sw,
                                       fm_intEcmpGroup *group,
                                       fm_int           numNextHops,
                                       fm_ecmpNextHop * nextHopList);
    fm_status  (*DeleteECMPGroupNextHops)(fm_int           sw,
                                          fm_intEcmpGroup *group,
                                          fm_int           numRemovedHops,
                                          fm_intNextHop ** removedHops,
                                          fm_int           numNextHops,
                                          fm_ecmpNextHop * nextHopList);
    fm_status  (*ReplaceECMPGroupNextHop)(fm_int           sw,
                                          fm_intEcmpGroup *group,
                                          fm_intNextHop *  oldNextHop,
                                          fm_intNextHop *  newNextHop);
    fm_status  (*SetECMPGroupNextHops)(fm_int           sw,
                                       fm_intEcmpGroup *group,
                                       fm_int           firstIndex,
                                       fm_int           numNextHops,
                                       fm_ecmpNextHop * nextHopList);
    fm_status  (*UpdateEcmpGroup)(fm_int           sw,
                                  fm_intEcmpGroup *group);
    fm_status  (*UpdateNextHop)(fm_int         sw,
                                fm_intNextHop *nextHop);
    fm_status  (*GetNextHopUsed)(fm_int         sw,
                                 fm_intNextHop *nextHop,
                                 fm_bool *      used,
                                 fm_bool        resetFlag);
    fm_status  (*RefreshARPUsedCache)(fm_int  sw,
                                      fm_bool invalidateCache,
                                      fm_bool resetFlag);
    fm_status  (*GetECMPGroupARPUsed)(fm_int           sw,
                                      fm_intEcmpGroup *group,
                                      fm_bool *        used,
                                      fm_bool          resetFlag);
    fm_status  (*ValidateNextHopTrapCode)(fm_int      sw,
                                          fm_nextHop *nextHop);
    fm_status  (*GetECMPGroupNextHopIndexRange)(fm_int           sw,
                                                fm_intEcmpGroup *group,
                                                fm_int *         firstIndex,
                                                fm_int *         lastIndex);
    fm_status  (*GetNextHopIndexUsed)(fm_int   sw,
                                      fm_int   index,
                                      fm_bool *used,
                                      fm_bool  reset);
    fm_status  (*AddArpEntry)(fm_int sw, fm_arpEntry *arp);
    fm_status  (*DeleteArpEntry)(fm_int sw, fm_arpEntry *arp);
    fm_status  (*UpdateArpEntryDestMac)(fm_int       sw,
                                        fm_arpEntry *arp);
    fm_status  (*UpdateArpEntryVrid)(fm_int       sw,
                                     fm_arpEntry *arp,
                                     fm_int       vrid);


    /**************************************************
     * Multicast Group Functions
     **************************************************/
    fm_status  (*McastGroupInit)(fm_int sw, fm_bool swagInit);
    fm_status  (*AllocateMcastGroups)(fm_int     sw,
                                      fm_uint    startGlort,
                                      fm_uint    glortSize,
                                      fm_int    *baseMcastGroup,
                                      fm_int    *numMcastGroups,
                                      fm_int    *step);
    fm_status  (*FreeMcastGroups)(fm_int     sw,
                                  fm_int     baseMcastGroupHandle);
    fm_status  (*AssignMcastGroup)(fm_int sw, fm_int *handle);
    fm_status  (*UnassignMcastGroup)(fm_int sw, fm_int handle);
    fm_status  (*CreateMcastGroup)(fm_int sw, fm_intMulticastGroup *group);
    fm_status  (*DeleteMcastGroup)(fm_int sw, fm_intMulticastGroup *group);
    fm_status  (*AddMcastGroupAddress)(fm_int                sw,
                                       fm_intMulticastGroup *group,
                                       fm_multicastAddress * address);
    fm_status  (*DeleteMcastGroupAddress)(fm_int                sw,
                                          fm_intMulticastGroup *group,
                                          fm_multicastAddress * address);
    fm_status  (*AddMulticastListener)(fm_int sw,
                                       fm_intMulticastGroup *group,
                                       fm_intMulticastListener *listener);
    fm_status  (*DeleteMulticastListener)(fm_int sw,
                                          fm_intMulticastGroup *group,
                                          fm_intMulticastListener *listener);
    fm_status  (*ActivateMcastGroup)(fm_int sw, fm_intMulticastGroup *group);
    fm_status  (*DeactivateMcastGroup)(fm_int sw, fm_intMulticastGroup *group);
    fm_status  (*GetAvailableMulticastListenerCount)(fm_int  sw,
                                                     fm_int *count);
    fm_status  (*SetMcastGroupAddress)(fm_int                sw,
                                       fm_intMulticastGroup *group,
                                       fm_multicastAddress * address);
    fm_status  (*SetMcastGroupAttribute)(fm_int                sw,
                                         fm_intMulticastGroup *group,
                                         fm_int                attr,
                                         void *                value);
    fm_status  (*GetMcastGroupAttribute)(fm_int                sw,
                                         fm_intMulticastGroup *group,
                                         fm_int                attr,
                                         void *                value);
    fm_status  (*GetMcastGroupTrigger)(fm_int                sw,
                                       fm_intMulticastGroup *group,
                                       fm_int *              trigger);
    fm_status  (*GetMcastGroupUsed)(fm_int                sw,
                                    fm_intMulticastGroup *mcastGroup,
                                    fm_bool *             used,
                                    fm_bool               resetFlag);
    fm_status  (*ReserveReplicationGroupMcastIndex)(fm_int sw,
                                                    fm_int group,
                                                    fm_int mcastLogPort, 
                                                    fm_int *mcastDestIndex);
    fm_status  (*ReleaseReplicationGroupMcastIndex)(fm_int sw,
                                                    fm_int repliGroup);
    fm_status  (*MoveReplicationGroupMcastGroup)(fm_int sw, 
                                                 fm_int groupHandle,
                                                 fm_int mcastGroup);
    fm_status  (*GetMcastGroupHwIndex)(fm_int                sw,
                                       fm_intMulticastGroup *group,
                                       fm_int *              hwIndex);
    fm_int     (*CompareMulticastAddresses)(const void *key1,
                                            const void *key2);
    fm_status  (*GetMcastGroupForSWAGGroup)(fm_int                sw,
                                            fm_int                realSw,
                                            fm_intMulticastGroup *group,
                                            fm_int *              groupHandle);

    fm_status (*ConfigureMcastGroupAsHNIFlooding)(fm_int  sw,
                                                  fm_int  mcastGroup,
                                                  fm_bool isHNIFlooding);

    /***************************************************
     * Load Balancing Group Functions
     **************************************************/
    fm_status  (*AllocateLBGs)(fm_int     sw,
                               fm_uint    startGlort,
                               fm_uint    glortSize,
                               fm_int    *baseLbgHandle,
                               fm_int    *numLbgs,
                               fm_int    *step);
    fm_status  (*AssignLBGPortResources)(fm_int sw, void *params);
    fm_status  (*GetPortParametersForLBG)(fm_int sw, 
                                          fm_int *numPorts, 
                                          fm_int *numDestEntries);
    fm_status  (*FreeLBGs)(fm_int sw, fm_int baseLbgHandle);
    fm_status  (*CreateLBG)(fm_int       sw, 
                            fm_int *     lbgNumber, 
                            fm_LBGParams *params);
    fm_status  (*DeleteLBG)(fm_int sw, fm_int lbgNumber);
    fm_status  (*AddLBGPort)(fm_int sw, fm_int lbgNumber, fm_int port);
    fm_status  (*DeleteLBGPort)(fm_int sw, fm_int lbgNumber, 
                                fm_int port);
    fm_status  (*SetLBGAttribute)(fm_int sw, fm_int lbgNumber, 
                                  fm_int attr, void *value);
    fm_status  (*GetLBGAttribute)(fm_int sw, fm_int lbgNumber, 
                                  fm_int attr, void *value);
    fm_status  (*SetLBGPortAttribute)(fm_int sw, fm_int lbgNumber, 
                                      fm_int port, fm_int attr, 
                                      void *value);
    fm_status  (*GetLBGPortAttribute)(fm_int sw, fm_int lbgNumber, 
                                      fm_int port, fm_int attr, 
                                      void *value);
    fm_status  (*GetLBGRouteData)(fm_int sw, fm_int lbgNumber, 
                                  fm_LBGRouteType *routeType, fm_int *routeData, 
                                  fm_int *dataCount);

    /***************************************************
     * Glort Management
     **************************************************/
    fm_status  (*CreateLogicalPortForGlort)(fm_int    sw, 
                                            fm_uint32 glort, 
                                            fm_int *  logicalPort);
    fm_status  (*CreateLogicalPortForMailboxGlort)(fm_int    sw, 
                                                   fm_uint32 glort, 
                                                   fm_int *  logicalPort);
    fm_status  (*CreateForwardingRule)(fm_int          sw, 
                                       fm_int *        id, 
                                       fm_forwardRule *rule);
    fm_status  (*UpdateForwardingRule)(fm_int          sw,
                                       fm_int *        id,
                                       fm_forwardRuleInternal *oldRuleInt,
                                       fm_forwardRule *rule);
    fm_status  (*DeleteForwardingRule)(fm_int sw, fm_int id);
    fm_status  (*SetStackGlortRange)(fm_int sw);
    fm_status  (*SetStackLogicalPortState)(fm_int sw,
                                           fm_int port,
                                           fm_int mode);
    fm_status  (*GetGlortForSpecialPort)(fm_int sw,
                                         fm_int port,
                                         fm_int *glort);
    fm_status  (*GetMaxGlortsPerLag)(fm_int sw,
                                     fm_int *maxGlorts);
    fm_status  (*CreateCanonicalCamEntries)(fm_int    sw,
                                            fm_uint32 glort,
                                            fm_uint   glortSize,
                                            fm_int    clearSize);
    fm_status  (*DeleteCanonicalCamEntries)(fm_int    sw,
                                            fm_uint32 glort,
                                            fm_uint   glortSize);


    /**************************************************
     * Platform-Specific device register access functions.
     * These functions may be implemented by a given
     * switch type or not, as needed.
     *
     * Important note to Intel developers: 
     *
     * Each function pointer to be populated by the
     * platform layer must be documented (use 
     * slash-star-star Doxygen style comments). Include 
     * information about whether the pointer is required 
     * or optional.
     **************************************************/

    /** \desc       Required: Write a value to a single 32-bit wide register.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[in]  value is the 32-bit value to be written to the register.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteUINT32)(fm_int    sw,
                              fm_uint   reg,
                              fm_uint32 value);

    /** \desc       Required: Read a value from a single 32-bit wide register.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[out] value points to caller-allocated storage where this
     *              function is to place the register value.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadUINT32)(fm_int     sw,
                             fm_uint    reg,
                             fm_uint32 *value);
    
    /** \desc       Required: Set or clear a set of bits in a single 32-bit 
     *              wide register.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[in]  mask is a bit mask with a '1' in each bit position to
     *              be affected and a '0' in each bit position to be left
     *              alone.
     *
     *  \param[in]  on should be set to TRUE to turn bits on in the register
     *              or FALSE to turn bits off.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*MaskUINT32)(fm_int    sw,
                             fm_uint   reg,
                             fm_uint32 mask,
                             fm_bool   on);
    
    /** \desc       Optional: Write a value to a single 32-bit wide register, 
     *              but without taking the platform lock. This function is a 
     *              high-performance version of ''WriteUINT32'' which is used 
     *              exclusively by ''fmDbgSwitchSelfTest''. If not initialized 
     *              by the platform layer, ''WriteUINT32'' will be used instead.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[in]  value is the 32-bit value to be written to the register.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteRawUINT32)(fm_int    sw,
                                 fm_uint   reg,
                                 fm_uint32 value);
    
    /** \desc       Optional: Read a value from a single 32-bit wide register, 
     *              but without taking the platform lock. This function is a 
     *              high-performance version of ''ReadUINT32'' which is used 
     *              exclusively by ''fmDbgSwitchSelfTest''. If not initialized 
     *              by the platform layer, ''ReadUINT32'' will be used instead.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[out] value points to caller-allocated storage where this
     *              function is to place the register value.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadRawUINT32)(fm_int     sw,
                                fm_uint    reg,
                                fm_uint32 *value);

    /** \desc       Optional: Write a sequence of 32-bit wide registers, but
     *              without taking the platform lock. This function is a 
     *              high-performance version of ''WriteUINT32''. If not
     *              initialized by the platform layer, ''WriteUINT32'' will
     *              be used instead.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg points to an array of register addresses to be
     *              written. The array must be count elements in length.
     *  
     *  \param[in]  value points to an array of values to be written. The
     *              array must be count elements in length.
     *
     *  \param[in]  count contains the number of register addresses to write.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteRawUINT32Seq)(fm_int     sw,
                                    fm_uint32 *reg,
                                    fm_uint32 *value,
                                    fm_int     count);
    
    /** \desc       Required: Write a multiple word value to a multiple 
     *              word register.
     *
     *  \note       For 64-bit registers, use ''WriteUINT64'' or 
     *              ''WriteUINT64Mult''.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the beginning register address within the chip.
     *
     *  \param[in]  count is the number of 32-bit words to be written.
     *
     *  \param[in]  ptr points to an array of 32-bit values to be written
     *              to consecutive register word addresses.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteUINT32Mult)(fm_int     sw,
                                  fm_uint    reg,
                                  fm_int     count,
                                  fm_uint32 *ptr);
    
    /** \desc       Required: Read a multiple word value from a multiple word
     *              register.
     *
     *  \note       For 64-bit registers, use ''ReadUINT64'' or 
     *              ''ReadUINT64Mult''.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the beginning register address within the chip.
     *
     *  \param[in]  count is the number of 32-bit words to be read.
     *
     *  \param[out] value points to caller-allocated storage where this 
     *              function is to place the multiple 32-bit values read
     *              from consecutive register word addresses. The provided
     *              storage must be at least the number of 32-bit words
     *              indicated by count.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadUINT32Mult)(fm_int     sw,
                                 fm_uint    reg,
                                 fm_int     count,
                                 fm_uint32 *value);
    
    /** \desc       Required: Write a value to a single 64-bit wide register.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[in]  value is the 64-bit value to be written to the register.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteUINT64)(fm_int    sw,
                              fm_uint   reg,
                              fm_uint64 value);
    
    /** \desc       Required: Read a value from a single 64-bit wide register.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[out] value points to caller-allocated storage where this
     *              function is to place the 64-bit register value.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadUINT64)(fm_int     sw,
                             fm_uint    reg,
                             fm_uint64 *value);
    
    /** \desc       Required: Write a multiple long-word (64-bit) value to a 
     *              multiple long-word (64-bit) register.
     *
     *  \note       For 32-bit registers, use ''WriteUINT32'' or 
     *              ''WriteUINT32Mult''.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the beginning register address within the chip.
     *
     *  \param[in]  count is the number of 64-bit words to be written.
     *
     *  \param[in]  ptr points to an array of 64-bit values to be written
     *              to consecutive register word addresses.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteUINT64Mult)(fm_int     sw,
                                  fm_uint    reg,
                                  fm_int     count,
                                  fm_uint64 *ptr);
    
    /** \desc       Required: Read a multiple long-word (64-bit) value from 
     *              a multiple long-word (64-bit) register.
     *
     *  \note       For 32-bit registers, use ''ReadUINT32'' or 
     *              ''ReadUINT32Mult''.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the beginning register address within the chip.
     *
     *  \param[in]  count is the number of 64-bit long-words to be read.
     *
     *  \param[out] value points to caller-allocated storage where this 
     *              function is to place the multiple 64-bit values read
     *              from consecutive register addresses. The provided
     *              storage must be at least the number of 64-bit long-words
     *              indicated by count.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadUINT64Mult)(fm_int     sw,
                                 fm_uint    reg,
                                 fm_int     count,
                                 fm_uint64 *value);
    
    /** \desc       Required for FM4000 devices only. Read the Egress FID 
     *              table register for a work-around to Errata #50.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[out] value points to caller-allocated storage where this
     *              function is to place the register value.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadEgressFid)(fm_int     sw,
                                fm_uint    reg,
                                fm_uint32 *value);
    
    /** \desc       Required for FM4000 devices only. Read the Ingress FID 
     *              table register for a work-around to Errata #50.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  reg is the register address within the chip.
     *
     *  \param[out] value points to caller-allocated storage where this
     *              function is to place the register value.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadIngressFid)(fm_int     sw,
                                 fm_uint    reg,
                                 fm_uint64 *value);
    
    /** \desc       Optional: Provides a performance enhancement for devices 
     *              accessed via FIBM. If not initialized by the platform 
     *              layer, ''ReadUINT32Mult'' will be used instead.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  nEntries is the number of entries in the scatter-gather
     *              list.
     *
     *  \param[in]  sgList points to the scatter-gather list to be operated
     *              on.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*ReadScatterGather)(fm_int sw,
                                    fm_int nEntries,
                                    fm_scatterGatherListEntry *sgList);
    
    /** \desc       Optional: Provides a performance enhancement for devices 
     *              accessed via FIBM. If not initialized by the platform 
     *              layer, ''WriteUINT32Mult'' will be used instead.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \param[in]  nEntries is the number of entries in the scatter-gather
     *              list.
     *
     *  \param[in]  sgList points to the scatter-gather list to be operated
     *              on.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*WriteScatterGather)(fm_int sw,
                                     fm_int nEntries,
                                     fm_scatterGatherListEntry *sgList);

    /* Optional: Write a value to a single 32-bit wide register via I2C for
     * diagnostic purposes only. Platforms that provide only I2C access
     * to the switch should initialize WriteUINT32 to point to a function
     * that writes via I2C. */
    fm_status (*I2cWriteUINT32)(fm_int sw, fm_uint reg, fm_uint32 value);

    /* Optional: Read a value from a single 32-bit wide register via I2C for
     * diagnostic purposes only. Platforms that provide only I2C access
     * to the switch should initialize ReadUINT32 to point to a function
     * that reads via I2C. */
    fm_status (*I2cReadUINT32)(fm_int sw, fm_uint reg, fm_uint32 *value);

    /* Optional: Write values to multiple consecutive 32-bit wide registers
     * via I2C for diagnostic purposes only. Platforms that provide only 
     * I2C access to the switch should initialize WriteUINT32Mult to point 
     * to a function that writes via I2C. */
    fm_status (*I2cWriteUINT32Mult)(fm_int     sw,
                                    fm_uint    reg,
                                    fm_int     n,
                                    fm_uint32 *ptr);

    /* Optional: Read values from multiple consecutive 32-bit wide registers
     * via I2C for diagnostic purposes only. Platforms that provide only 
     * I2C access to the switch should initialize ReadUINT32Mult to point to 
     * a function that reads via I2C. */
    fm_status (*I2cReadUINT32Mult)(fm_int     sw,
                                   fm_uint    reg,
                                   fm_int     n,
                                   fm_uint32 *value);

    /** \desc       Required for FIBM-controlled devices only. Initialize 
     *              a register cache for providing more efficient access to 
     *              the remote switch.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*RegCacheInit)(fm_int sw);
    
    /** \desc       Required for FIBM-controlled devices only. Clean up the 
     *              remote switch register cache initialized by ''RegCacheInit''.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*RegCacheCleanup)(fm_int sw);
    
    /** \desc       Optional for FIBM-controlled devices, not used for 
     *              locally accessed devices. Verify that the remote switch 
     *              register cache contents match the hardware. For diagnostic 
     *              purposes only.
     *
     *  \param[in]  sw is the switch on which to operate.
     *
     *  \return     FM_OK if successful.   */
    fm_status  (*RegCacheVerify)(fm_int sw);

    /**************************************************
     * The platform does not need to populate the
     * following pointers. Only used by the caching 
     * module.
     **************************************************/
    fm_status  (*ReadUncacheUINT32)(fm_int sw,
                                    fm_uint reg,
                                    fm_uint32 *value);

    fm_status  (*ReadUncacheUINT32Mult)(fm_int sw,
                                        fm_uint reg,
                                        fm_int count,
                                        fm_uint32 *value);

    fm_status  (*ReadUncacheUINT64)(fm_int sw,
                                    fm_uint reg,
                                    fm_uint64 *value);

    fm_status  (*ReadUncacheUINT64Mult)(fm_int sw,
                                        fm_uint reg,
                                        fm_int count,
                                        fm_uint64 *ptr);
               
    /* pointer to chipset-specific Cached Register List */
    void       **CachedRegisterList;

    /* I2C write read function */
    fm_status                   (*I2cWriteRead)(fm_int   sw,
                                                fm_uint  device,
                                                fm_byte *data,
                                                fm_uint  wl,
                                                fm_uint  rl);

    /***************************************************
     * The following two functions are used by modules
     * that require the data stream to stop while
     * registers are being modified.
     **************************************************/

    fm_status  (*StopTraffic)(fm_int sw);
    fm_status  (*RestartTraffic)(fm_int sw);

    /***************************************************
     * SFlow Functions
     **************************************************/

    fm_status  (*CreateSFlow)(fm_int       sw, 
                              fm_int       sFlowId, 
                              fm_sFlowType sFlowType);
    fm_status  (*DeleteSFlow)(fm_int sw, fm_int sFlowId);
    fm_status  (*AddSFlowPort)(fm_int sw, 
                               fm_int sFlowId, 
                               fm_int port);
    fm_status  (*DeleteSFlowPort)(fm_int sw, 
                                  fm_int sFlowId, 
                                  fm_int port);

    fm_status  (*GetSFlowPortFirst)(fm_int   sw, 
                                    fm_int   sFlow,
                                    fm_int * firstPort);

    fm_status  (*GetSFlowPortNext)(fm_int   sw,
                                   fm_int   sFlowId,
                                   fm_int   startPort,
                                   fm_int * nextPort);

    fm_status  (*GetSFlowPortList)(fm_int   sw, 
                                   fm_int   sFlowId, 
                                   fm_int * numPorts, 
                                   fm_int * portList, 
                                   fm_int   max);

    fm_status  (*SetSFlowAttribute)(fm_int sw, 
                                    fm_int sFlowId, 
                                    fm_int attr, 
                                    void * value);

    fm_status  (*GetSFlowAttribute)(fm_int sw, 
                                    fm_int sFlow, 
                                    fm_int attr, 
                                    void * value);
    
    fm_status  (*GetSFlowType)(fm_int         sw, 
                               fm_int         sFlowId, 
                               fm_sFlowType * sFlowType);

    fm_status  (*CheckSFlowLogging)(fm_int            sw, 
                                    fm_eventPktRecv * pktEvent, 
                                    fm_bool         * isPktSFlowLogged);

    fm_status  (*UpdateRemoveDownPortsTrigger)(fm_int sw,
                                               fm_int physPort,
                                               fm_bool down);

    /***************************************************
     * Trigger Functions
     **************************************************/
    fm_status  (*CreateTrigger)(fm_int  sw, 
                                fm_int  group, 
                                fm_int  rule,
                                fm_bool isInternal,
                                fm_text name); 
    fm_status  (*DeleteTrigger)(fm_int  sw, 
                                fm_int  group, 
                                fm_int  rule,
                                fm_bool isInternal); 
    fm_status  (*SetTriggerCondition)(fm_int sw, 
                                      fm_int group, 
                                      fm_int rule,
                                      const fm_triggerCondition *cond,
                                      fm_bool isInternal); 
    fm_status  (*SetTriggerAction)(fm_int sw, 
                                   fm_int group, 
                                   fm_int rule,
                                   const fm_triggerAction *action,
                                   fm_bool isInternal); 
    fm_status  (*GetTrigger)(fm_int sw, 
                             fm_int group, 
                             fm_int rule, 
                             fm_triggerCondition *cond,
                             fm_triggerAction *action); 
    fm_status  (*GetTriggerFirst)(fm_int sw, 
                                  fm_int *group, 
                                  fm_int *rule);
    fm_status  (*GetTriggerNext)(fm_int sw, 
                                 fm_int curGroup, 
                                 fm_int curRule,
                                 fm_int *nextGroup, 
                                 fm_int *nextRule);
    fm_status  (*AllocateTriggerResource)(fm_int sw, 
                                          fm_triggerResourceType res, 
                                          fm_uint32 *value,
                                          fm_bool isInternal); 
    fm_status  (*FreeTriggerResource)(fm_int sw, 
                                      fm_triggerResourceType res, 
                                      fm_uint32 value,
                                      fm_bool isInternal); 
    fm_status  (*GetTriggerResourceFirst)(fm_int sw, 
                                          fm_triggerResourceType res, 
                                          fm_uint32 *value);
    fm_status  (*GetTriggerResourceNext)(fm_int sw, 
                                         fm_triggerResourceType res, 
                                         fm_uint32 curValue,
                                         fm_uint32 *nextValue);
    fm_status  (*SetTriggerRateLimiter)(fm_int sw,
                                        fm_int rateLimiterId,
                                        fm_rateLimiterCfg *cfg,
                                        fm_bool isInternal);
    fm_status  (*GetTriggerRateLimiter)(fm_int sw,
                                        fm_int rateLimiterId,
                                        fm_rateLimiterCfg *cfg);
    fm_status  (*SetTriggerAttribute)(fm_int sw, 
                                      fm_int group, 
                                      fm_int rule,
                                      fm_int attr, 
                                      void *value,
                                      fm_bool isInternal);
    fm_status  (*GetTriggerAttribute)(fm_int sw, 
                                      fm_int group, 
                                      fm_int rule,
                                      fm_int attr, 
                                      void *value);

    fm_status  (*AllocateTrigger)(fm_int sw, 
                                  fm_text name, 
                                  fm_int *trigger, 
                                  fm_triggerRequestInfo *info);

    fm_status  (*FreeTrigger)(fm_int sw, fm_int trigger); 
    
    /***************************************************
     * Timestamp Functions
     **************************************************/

    fm_status  (*ConvertTimestamp)(fm_int                sw,
                                   fm_uint32             eplTime,
                                   fm_int64              correction,
                                   fm_preciseTimestamp * timestamp);

    /***************************************************
     * Parity Error Functions
     **************************************************/

    void *     (*ParitySweeperTask)(fm_int   sw, 
                                    fm_bool *switchProtected, 
                                    void    *args);

    void *     (*ParityRepairTask)(fm_int    sw, 
                                   fm_bool * switchProtected, 
                                   void *    args);

    fm_status  (*GetParityAttribute)(fm_int sw, fm_int attr, void * value);

    fm_status  (*SetParityAttribute)(fm_int sw, fm_int attr, void * value);

    fm_status  (*GetParityErrorCounters)(fm_int    sw,
                                         void *    counters,
                                         fm_uint   size);

    fm_status  (*ResetParityErrorCounters)(fm_int sw);

    fm_status  (*DbgDumpParity)(fm_int sw);

    fm_status  (*DbgInjectParityError)(fm_int  sw,
                                       fm_text memoryName,
                                       fm_int  index,
                                       fm_int  errMode);

    /***************************************************
     * Mailbox functions
     **************************************************/

    fm_status (*WriteMailboxResponseMessage)(fm_int                        sw,
                                             fm_int                        pepNb,
                                             fm_mailboxControlHeader *     ctrlHdr,
                                             fm_mailboxMessageId           msgTypeId,
                                             fm_mailboxMessageArgumentType argType,
                                             fm_voidptr *                  message);

    fm_status (*ReadMailboxControlHdr)(fm_int                   sw,
                                       fm_int                   pepNb,
                                       fm_mailboxControlHeader *ctrlHdr);

    fm_status (*UpdateMailboxSmHdr)(fm_int                   sw,
                                    fm_int                   pepNb,
                                    fm_mailboxControlHeader *controlHeader,
                                    fm_uint32                updateType);

    fm_status (*ValidateMailboxMessageLength)(fm_int                   sw,
                                              fm_int                   pepNb,
                                              fm_mailboxControlHeader *ctrlHdr,
                                              fm_mailboxMessageHeader *pfTrHdr);

    fm_status (*ReadMailboxRequestArguments)(fm_int                   sw,
                                             fm_int                   pepNb,
                                             fm_mailboxControlHeader *ctrlHdr,
                                             fm_mailboxMessageHeader *pfTrHdr,
                                             fm_uint16                argumentType,
                                             fm_uint16                argumentLength,
                                             fm_voidptr *             message);

    fm_status (*ProcessMailboxLoopbackRequest)(fm_int                   sw,
                                               fm_int                   pepNb,
                                               fm_mailboxControlHeader *controlHeader);

    fm_status (*ProcessCreateFlowTableRequest)(fm_int           sw,
                                               fm_int           pepNb,
                                               fm_int           tableIndex,
                                               fm_uint16        tableType,
                                               fm_flowCondition condition);

    void (*SetMailboxLportGlortRange)(fm_int              sw,
                                      fm_int              pepNb,
                                      fm_hostSrvLportMap *lportMap);

    void (*SetHostSrvErrResponse)(fm_int         sw,
                                  fm_int         pepNb,
                                  fm_hostSrvErr *srvErr);

    fm_status (*MapPepToLogicalPort)(fm_int  sw,
                                     fm_int  pepNb,
                                     fm_int *port);

    fm_status (*MapLogicalPortToPep)(fm_int  sw,
                                     fm_int  port,
                                     fm_int *pepNb);

    fm_status (*MapVsiGlortToPepNumber)(fm_int    sw,
                                        fm_uint32 vsiGlort,
                                        fm_int *  pepNb);

    fm_status (*MapVsiGlortToPepLogicalPort)(fm_int     sw,
                                             fm_uint32  vsiGlort,
                                             fm_int    *port);

    fm_status (*MailboxAllocateDataStructures)(fm_int sw);

    fm_status (*MailboxInit)(fm_int sw);

    fm_status (*MailboxFreeDataStructures)(fm_int sw);

    fm_status (*MailboxFreeResources)(fm_int sw);

    fm_status (*GetSchedPortSpeedForPep)(fm_int  sw,
                                         fm_int  pepId,
                                         fm_int *speed);

    fm_status (*FindInternalPortByMailboxGlort)(fm_int    sw,
                                                fm_uint32 glort,
                                                fm_int *  logicalPort);

    fm_status (*SetXcastFlooding)(fm_int       sw,
                                  fm_int       pepNb,
                                  fm_xcastMode mode,
                                  fm_uint16    xcastFloodMode);

    fm_status (*AssociateMcastGroupWithFlood)(fm_int                sw,
                                              fm_int                pepNb,
                                              fm_int                floodPort,
                                              fm_intMulticastGroup *mcastGroup,
                                              fm_bool               associate);

    fm_status (*GetMailboxGlortRange)(fm_int     sw,
                                      fm_int     pepNb,
                                      fm_uint32 *glortBase,
                                      fm_int *   numberOfGlorts);


};  /* end struct _fm_switch */


/**
 * DEPRECATED: All this macro does is indicate whether the argument is out
 * of bounds. It doesn't ensure that the specified logical port is valid, 
 * or of an acceptable type.
 *  
 * Predicates: 
 *  
 * - fmIsCardinalPort(sw, port) returns FALSE if the argument is not a 
 *   valid cardinal port. It has no side effects.
 *  
 * - fmIsValidPort(sw, port, mode) generates a debug message and returns 
 *   FALSE if the port is invalid (out of bounds, undefined, or not of the
 *   specified type).
 *  
 * Imperatives: 
 *  
 * - VALIDATE_LOGICAL_PORT(sw, port, mode) generates a debug message, 
 *   unprotects the switch, and returns immediately if the port is
 *   invalid. It should follow VALIDATE_AND_PROTECT_SWITCH.
 */
#define NON_CPU_LOGICAL_PORT_NUMBER_OUT_OF_BOUNDS(sw, port) \
    ( (port) <= 0 || (port) >= fmRootApi->fmSwitchStateTable[(sw)]->maxPort )


/**************************************************
 * VALIDATE macros.
 **************************************************/

/* Except for VALIDATE_AND_PROTECT_SWITCH(), all of the VALIDATE_() macros
 * assume the switch index refers to a valid switch.  They also assume
 * that a switch lock was taken prior to calling them, and will release
 * that lock if an error condition must be returned.
 */


/**************************************************
 * Switch VALIDATE macros.
 **************************************************/

#define VALIDATE_SWITCH(sw)                                                        \
    if ( (sw) < 0 || (sw) >= FM_MAX_NUM_SWITCHES )                                 \
    {                                                                              \
        FM_LOG_EXIT(FM_LOG_CAT_SWITCH, FM_ERR_INVALID_SWITCH);                     \
    }                                                                              \

/*  VALIDATE_AND_PROTECT_SWITCH takes a READ lock on the switch lock table.
 *  Code that calls this macro MUST release the lock before returning!
 *  The easiest way to do this is to invoke the UNPROTECT_SWITCH macro */
#define VALIDATE_AND_PROTECT_SWITCH(sw)                                            \
    fm_bool swProtected = FALSE;                                                   \
    if ( (sw) < 0 || (sw) >= FM_MAX_NUM_SWITCHES )                                 \
    {                                                                              \
        return FM_ERR_INVALID_SWITCH;                                              \
    }                                                                              \
    VALIDATE_SWITCH_LOCK(sw);                                                      \
    /* Take read access to the switch lock */                                      \
    PROTECT_SWITCH(sw);                                                            \
    if (!fmRootApi->fmSwitchStateTable[(sw)])                                      \
    {                                                                              \
        UNPROTECT_SWITCH(sw);                                                      \
        return FM_ERR_SWITCH_NOT_UP;                                               \
    }                                                                              \
    if (fmRootApi->fmSwitchStateTable[(sw)]->state < FM_SWITCH_STATE_INIT ||  \
        fmRootApi->fmSwitchStateTable[(sw)]->state > FM_SWITCH_STATE_GOING_DOWN)   \
    {                                                                              \
        UNPROTECT_SWITCH(sw);                                                      \
        return FM_ERR_SWITCH_NOT_UP;                                               \
    }                                                                              \
    swProtected = TRUE


/*  Similar to VALIDATE_AND_PROTECT_SWITCH, but set error code rather than
 *  returning and don't define or set local swProtected variable, so cannot
 *  be blindly followed by other VALIDATE macros. 
 *
 *  Caller must call UNPROTECT_SWITCH unless this macro sets err to something
 *  other than FM_OK. */
#define VALIDATE_AND_PROTECT_SWITCH_NO_RETURN(err, sw)                      \
    (err) = FM_OK;                                                          \
    if ( (sw) < 0 || (sw) >= FM_MAX_NUM_SWITCHES )                          \
    {                                                                       \
        (err) = FM_ERR_INVALID_SWITCH;                                      \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        if ( !SWITCH_LOCK_EXISTS(sw) )                                      \
        {                                                                   \
            (err) = FM_ERR_INVALID_SWITCH;                                  \
        }                                                                   \
        else                                                                \
        {                                                                   \
            /* Take read access to the switch lock */                       \
            PROTECT_SWITCH(sw);                                             \
            if (!fmRootApi->fmSwitchStateTable[(sw)])                       \
            {                                                               \
                UNPROTECT_SWITCH(sw);                                       \
                (err) = FM_ERR_SWITCH_NOT_UP;                               \
            }                                                               \
            else if (fmRootApi->fmSwitchStateTable[(sw)]->state <           \
                                                    FM_SWITCH_STATE_INIT || \
                fmRootApi->fmSwitchStateTable[(sw)]->state >                \
                                                FM_SWITCH_STATE_GOING_DOWN) \
            {                                                               \
                UNPROTECT_SWITCH(sw);                                       \
                (err) = FM_ERR_SWITCH_NOT_UP;                               \
            }                                                               \
        }                                                                   \
    }


/**************************************************
 * Port validation macros.
 **************************************************/

/* Mode values for fmIsValidPort and VALIDATE_LOGICAL_PORT. */
#define ALLOW_ALL       -1
#define DISALLOW_CPU    0x00
#define ALLOW_CPU       0x01
#define ALLOW_LAG       0x02
#define ALLOW_REMOTE    0x04

/*
 * Validates the specified logical port. 
 *  
 * Determines whether the specified port is valid and of the type 
 * specified by the 'mode' parameter. 
 *  
 * Takes no action (does not exit) if the port is valid. 
 *  
 * Unprotects the switch and exits with FM_ERR_INVALID_PORT if the 
 * port is invalid. 
 *  
 * Supersedes VALIDATE_PORT_NUMBER and VALIDATE_LOGICAL_PORT_NUMBER. 
 */
#define VALIDATE_LOGICAL_PORT(sw, port, mode)                               \
    if ( !fmIsValidPort(sw, port, mode) )                                   \
    {                                                                       \
        if (swProtected)                                                    \
        {                                                                   \
            UNPROTECT_SWITCH(sw);                                           \
        }                                                                   \
        return FM_ERR_INVALID_PORT;                                         \
    }

/*
 * DEPRECATED: Use VALIDATE_LOGICAL_PORT with ALLOW_CPU or DISALLOW_CPU 
 * for all new development. (This alias is for backward compatibility 
 * with existing platform layers.) 
 */
#define VALIDATE_PORT_NUMBER(sw, port, allowCpu)        \
    VALIDATE_LOGICAL_PORT(sw, port, (allowCpu) ? ALLOW_CPU : DISALLOW_CPU)


/**************************************************
 * Other validation macros.
 **************************************************/

#define VALIDATE_VLAN_ID(sw, vlanID)                                    \
    if ( VLAN_OUT_OF_BOUNDS(vlanID) ||                                  \
        !fmRootApi->fmSwitchStateTable[sw]->vidTable[(vlanID)].valid || \
        ( fmRootApi->fmSwitchStateTable[sw]->reservedVlan ==            \
         (fm_uint16) (vlanID) ) )                                       \
    {                                                                   \
        if (swProtected)                                                \
        {                                                               \
            UNPROTECT_SWITCH(sw);                                       \
        }                                                               \
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_VLAN);              \
    }

#define VALIDATE_FID_ID(sw, fidID)  \
    if ( FID_OUT_OF_BOUNDS(fidID) ) \
    {                               \
        if (swProtected)            \
        {                           \
            UNPROTECT_SWITCH(sw);   \
        }                           \
        FM_LOG_EXIT(FM_LOG_CAT_VLAN, FM_ERR_INVALID_VLAN); \
    }


#define VALIDATE_STP_INSTANCE(sw, inst)               \
    if ( (inst < 0) || (inst >= GET_SWITCH_PTR(sw)->maxSTPInstances) ) \
    {                                                 \
        FM_LOG_EXIT(FM_LOG_CAT_STP, FM_ERR_INVALID_ARGUMENT); \
    }

#define VALIDATE_VIRTUAL_ROUTER_ID(sw, vrid)                      \
    if ( (vrid < 0) || (vrid >= FM_MAX_VIRTUAL_ROUTERS) )         \
    {                                                             \
        if (swProtected)                                          \
        {                                                         \
            UNPROTECT_SWITCH(sw);                                 \
        }                                                         \
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_VRID); \
    }


/**************************************************
 * Various utility macros.
 **************************************************/

/* Function to return a switch pointer - assumes that switch number has been
 * validated. */
#define GET_SWITCH_PTR(sw)  fmRootApi->fmSwitchStateTable[sw]

/* Function to return a switch extension pointer - assumes that switch
 * number and presence have been validated. */
#define GET_SWITCH_EXT(sw)  fmRootApi->fmSwitchStateTable[sw]->extension

/* Function to return a port pointer - assumes that switch and port have
 * been validated. */
#define GET_PORT_PTR(sw, port)  GET_SWITCH_PTR(sw)->portTable[(port)]

/* Function to return a port extension - assumes that switch and port have
 * been validated. */
#define GET_PORT_EXT(sw, port)  GET_SWITCH_PTR(sw)->portTable[(port)]->extension

/* Function to return a port attribute structure pointer - assumes that switch
 * and port have been validated. */
#define GET_PORT_ATTR(sw, port)  \
                        &(GET_SWITCH_PTR(sw)->portTable[(port)]->attributes)

/* Macro to return a lane pointer - assumes that switch and lane have been
   validated */
#define GET_LANE_PTR(sw, lane) GET_SWITCH_PTR(sw)->laneTable[(lane)]

/* Macro to return a lane extension - assumes that switch and lane have been
   validated */
#define GET_LANE_EXT(sw, lane) GET_SWITCH_PTR(sw)->laneTable[(lane)]->extension

/* Function to return a lane attribute structure pointer - assumes that switch
 * and port have been validated. */
#define GET_LANE_ATTR(sw, index)  \
                        &(GET_SWITCH_PTR(sw)->laneTable[(index)]->attributes)

/* Function to return the port attribute address. */
#define GET_PORT_ATTR_ADDRESS(baseAddr, entry)  \
                        (void *) ( ( (char *) (baseAddr) ) + (entry)->offset );

/* Function to return a vlan entry pointer - assumes that switch and vlan have
 * been validated. */
#define GET_VLAN_PTR(sw, vlan)  &(GET_SWITCH_PTR(sw)->vidTable[(vlan)])

/* Function to return a vlan entry extension pointer - assumes that switch and 
 * vlan have been validated. */
#define GET_VLAN_EXT(sw, vlan)  GET_SWITCH_PTR(sw)->vidTable[(vlan)].vlanExt

/* Function to return the STP instance tree */
#define GET_STP_INFO(sw)        &(GET_SWITCH_PTR(sw)->stpInstanceInfo)

/* Function to return the LBG info structure */
#define GET_LBG_INFO(sw)        &(GET_SWITCH_PTR(sw)->lbgInfo)

/* Function to return the logical port info structure */
#define GET_LPORT_INFO(sw)        &(GET_SWITCH_PTR(sw)->logicalPortInfo)


/* The GET_SWITCH_() macros always assume that the switch index has been
 * validated and a switch lock has been taken.  They validate any other
 * arguments.  Should be used at the top of API functions to validate
 * user-supplied arguments.  Internal API code should be written so that
 * validation is not needed.
 */

#define GET_SWITCH_STATE(sw, ptr)     \
    if (!swProtected)                 \
    {                                 \
        return FM_ERR_INVALID_SWITCH; \
    }                                 \
    (ptr) = fmRootApi->fmSwitchStateTable[(sw)];

#define GET_SWITCH_INFO(sw, ptr)      \
    if (!swProtected)                 \
    {                                 \
        return FM_ERR_INVALID_SWITCH; \
    }                                 \
    (ptr) = &fmRootApi->fmSwitchStateTable[(sw)]->info;

#define GET_SWITCH_LAG_INFO(sw, ptr)  \
    if (!swProtected)                 \
    {                                 \
        return FM_ERR_INVALID_SWITCH; \
    }                                 \
    (ptr) = &fmRootApi->fmSwitchStateTable[(sw)]->lagInfo

#define GET_SWITCH_COUNTER_INFO(sw, ptr) \
    if (!swProtected)                    \
    {                                    \
        return FM_ERR_INVALID_SWITCH;    \
    }                                    \
    (ptr) = &fmRootApi->fmSwitchStateTable[(sw)]->counterInfo

#define GET_PORT_ENTRY(sw, port, ptr) \
    if (!swProtected)                 \
    {                                 \
        return FM_ERR_INVALID_SWITCH; \
    }                                 \
    (ptr) = fmRootApi->fmSwitchStateTable[(sw)]->portTable[(port)]

/**
 * \ingroup intSwitch 
 *  
 * \desc            Validates a port number and retrieves its port table 
 *                  entry and physical port number.
 *  
 * \note            This macro must be preceded by a call to 
 *                  VALIDATE_AND_PROTECT_SWITCH.
 *  
 * \param[in]       sw is the switch on which to operate. 
 *  
 * \param[in]       port is the port on which to operate. 
 *  
 * \param[out]      ptr points to caller-provided storage where this macro 
 *                  should place a pointer to the port table entry.
 *  
 * \param[out]      phyport points to caller-provided storage where this 
 *                  macro should place the physical port number. 
 */
#define GET_PORT_STATE_ENTRY(sw, port, ptr, phyport)                \
    if (!swProtected)                                               \
    {                                                               \
        return FM_ERR_INVALID_SWITCH;                               \
    }                                                               \
    VALIDATE_LOGICAL_PORT( (sw), (port), ALLOW_CPU );               \
    fmMapLogicalPortToPhysical(fmRootApi->fmSwitchStateTable[(sw)], \
                               port, phyport);                      \
    (ptr) = fmRootApi->fmSwitchStateTable[(sw)]->portTable[(port)]

#define GET_ACL_ENTRY(sw, ptr, acl)                                            \
    {                                                                          \
        void *gaePtr_;                                                         \
        if (!fmRootApi->fmSwitchStateTable[(sw)]->aclInfo.enabled)             \
        {                                                                      \
            err = FM_ERR_ACL_DISABLED;                                         \
            goto ABORT;                                                        \
        }                                                                      \
        err = fmTreeFind(&(fmRootApi->fmSwitchStateTable[(sw)]->aclInfo.acls), \
                         (acl), &gaePtr_);                                     \
        if (err == FM_OK)                                                      \
        {                                                                      \
            (ptr) = (fm_acl *) gaePtr_;                                        \
        }                                                                      \
        else if (err == FM_ERR_NOT_FOUND)                                      \
        {                                                                      \
            (ptr) = NULL;                                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            goto ABORT;                                                        \
        }                                                                      \
        err = FM_OK;                                                           \
    }

#define PARITY_SWEEPER_SLEEP_CHECK(k)                                        \
    if ( (k % switchPtr->paritySweeperCfg.readBurstSize) >=                  \
                            switchPtr->paritySweeperCfg.readBurstSize - 1 )  \
    {                                                                        \
        UNPROTECT_SWITCH(sw);                                                \
        *switchProtected = FALSE;                                            \
        fmDelay(0,switchPtr->paritySweeperCfg.sleepPeriod);                  \
        VALIDATE_AND_PROTECT_SWITCH(sw);                                     \
        *switchProtected = TRUE;                                             \
        switchPtr = GET_SWITCH_PTR(sw);                                      \
        switchExt = GET_SWITCH_EXT(sw);                                      \
    }

/**************************************************
 * The state lock is used to protect any soft state
 * (e.g., structures).
 **************************************************/

#define FM_GET_STATE_LOCK(sw ) &fmRootApi->fmSwitchStateTable[(sw)]->stateLock

#define FM_TAKE_STATE_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->stateLock, FM_WAIT_FOREVER);

#define FM_DROP_STATE_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->stateLock);

/**************************************************
 * Macros to take the state lock and manage a local 
 * variable called stateLockTaken, which is kept
 * in sync with the state lock.
 **************************************************/

#define FM_FLAG_TAKE_STATE_LOCK(sw)                         \
                                    FM_TAKE_STATE_LOCK(sw); \
                                    stateLockTaken = TRUE;
                                    
#define FM_FLAG_DROP_STATE_LOCK(sw)                         \
                                    FM_DROP_STATE_LOCK(sw); \
                                    stateLockTaken = FALSE;
                                    
/**************************************************
 * The port attribute lock is used to protect the 
 * structures (i.e. fm_portAttr, fmX000_portAttr) 
 * used to cache port attributes.
 **************************************************/

#define FM_TAKE_PORT_ATTR_LOCK(sw) FM_TAKE_STATE_LOCK(sw)
#define FM_DROP_PORT_ATTR_LOCK(sw) FM_DROP_STATE_LOCK(sw)

/**************************************************
 * Macros to take the port attribute lock and 
 * manage a local variable called portAttrLockTaken, 
 * which is kept in sync with the port attr lock.
 **************************************************/

#define FM_FLAG_TAKE_PORT_ATTR_LOCK(sw)                         \
                                    FM_TAKE_PORT_ATTR_LOCK(sw); \
                                    portAttrLockTaken = TRUE;
                                    
#define FM_FLAG_DROP_PORT_ATTR_LOCK(sw)                         \
                                    FM_DROP_PORT_ATTR_LOCK(sw); \
                                    portAttrLockTaken = FALSE;

/**************************************************
 * The packet interrupt lock is used to protect 
 * The packet send/receive interrupt flag. It is
 * precedentless because no other locks are taken
 * when this lock is held.
 **************************************************/

#define FM_TAKE_PKT_INT_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->pktIntLock, FM_WAIT_FOREVER);

#define FM_DROP_PKT_INT_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->pktIntLock);

/**************************************************
 * The reg lock is used to ensure atomic access
 * to sets of registers, or read-modify-write
 * access to registers. It is a synonym for the
 * platform lock used to protect individual
 * register accesses.
 *
 * This lock is a "low level" lock, meaning
 * that most other locks cannot be taken when this
 * lock is held. Also, because this lock is used
 * frequently and at the register level, it should 
 * not be held for long periods of time.
 *
 * It should not be used to protect soft state
 * (e.g. structures). For that, use stateLock.
 **************************************************/

#define TAKE_REG_LOCK(sw) FM_PLAT_TAKE_REG_LOCK(sw)
#define DROP_REG_LOCK(sw) FM_PLAT_DROP_REG_LOCK(sw)

/**************************************************
 * Macros to take the reg lock and manage a local 
 * variable called regLockTaken, which is kept
 * in sync with the reg lock.
 **************************************************/

#define FM_FLAG_TAKE_REG_LOCK(sw)                         \
                                    TAKE_REG_LOCK(sw);    \
                                    regLockTaken = TRUE;
                                    
#define FM_FLAG_DROP_REG_LOCK(sw)                         \
                                    DROP_REG_LOCK(sw);    \
                                    regLockTaken = FALSE;
                                    
    

/**************************************************
 * The trigger lock is used to protect trigger
 * structures and HW access to trigger resources.
 **************************************************/

#define TAKE_TRIGGER_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->triggerLock, FM_WAIT_FOREVER);

#define DROP_TRIGGER_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->triggerLock);

#define TAKE_LAG_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[sw]->lagLock, FM_WAIT_FOREVER)

#define DROP_LAG_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[sw]->lagLock)

#define TAKE_MIRROR_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->mirrorLock, FM_WAIT_FOREVER);

#define DROP_MIRROR_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->mirrorLock);

#define TAKE_TUNNEL_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->tunnelLock, FM_WAIT_FOREVER);

#define DROP_TUNNEL_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->tunnelLock);

#define TAKE_NAT_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->natLock, FM_WAIT_FOREVER);

#define DROP_NAT_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->natLock);

#define TAKE_FLOW_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->flowLock, FM_WAIT_FOREVER);

#define DROP_FLOW_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->flowLock);

#define TAKE_PORTSET_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->portSetLock, FM_WAIT_FOREVER);

#define DROP_PORTSET_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->portSetLock);

#define TAKE_SCHEDULER_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->schedulerLock, FM_WAIT_FOREVER);

#define DROP_SCHEDULER_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->schedulerLock);
    
/**************************************************
 * Macros to take the LAG lock and manage a local 
 * variable called lagLockTaken, which is kept
 * in sync with the LAG lock.
 **************************************************/

#define FM_FLAG_TAKE_LAG_LOCK(sw)                         \
                                    TAKE_LAG_LOCK(sw);    \
                                    lagLockTaken = TRUE;
                                    
#define FM_FLAG_DROP_LAG_LOCK(sw)                         \
                                    DROP_LAG_LOCK(sw);    \
                                    lagLockTaken = FALSE;
                                    
#define TAKE_CRM_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->crmLock, FM_WAIT_FOREVER);

#define DROP_CRM_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->crmLock);

#define FM_TAKE_MTABLE_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->mtableLock, FM_WAIT_FOREVER)

#define FM_DROP_MTABLE_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->mtableLock)

#define FM_TAKE_MAILBOX_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->mailboxLock, FM_WAIT_FOREVER)

#define FM_DROP_MAILBOX_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->mailboxLock)
    
/* the following functions handle mapping from logical to physical
 * and vice versa
 */

fm_status fmMapLogicalPortToPhysical(fm_switch *sstate,
                                     fm_int     logPort,
                                     fm_int *   physPort);

fm_status fmMapPhysicalPortToLogical(fm_switch *sstate,
                                     fm_int     physPort,
                                     fm_int *   logPort);

fm_status fmMapBitMaskLogicalToPhysical(fm_switch * switchPtr,
                                        fm_uint32   logMask,
                                        fm_uint32 * physMask);

fm_status fmMapBitMaskPhysicalToLogical(fm_switch * switchPtr,
                                        fm_uint32   physMask,
                                        fm_uint32 * logMask);

fm_status fmMapBitMaskLogicalToLinkUpMask(fm_switch * switchPtr,
                                          fm_uint32   logMask,
                                          fm_uint32 * upMask);

fm_int fmFindNextPortInMask(fm_int sw, fm_uint32 mask, fm_int firstBit);

fm_status fmBitArrayToBitMask(fm_bitArray * bitArray,
                              fm_uint32 *   bitMask,
                              fm_int        numBits);

fm_uint fmFindNextPowerOf2(fm_uint value);

fm_status fmLoadPortRemapTable(fm_char *filename, 
                               fm_int *portTable, 
                               fm_int *origTable, 
                               fm_int maxport);

fm_status fmGetPartNumberMaxPort(fm_switchPartNum pn, fm_int *maxPort);

fm_int fmFindNextBitInMask(fm_int sw, fm_uint32 mask, fm_int firstBit);

fm_status fmClearBitInMask(fm_int sw, fm_uint32 *mask, fm_int bitPosition);
    
/**************************************************
 * ARP Entry utility macros.
 **************************************************/

#define fmGetFirstArp(switchPtr) \
    FM_DLL_GET_FIRST(switchPtr, firstArp)

#define fmGetLastArp(switchPtr) \
    FM_DLL_GET_LAST(switchPtr, lastArp)

#define fmGetNextArp(arpPtr) \
    FM_DLL_GET_NEXT(arpPtr, nextArp)

#define fmGetPreviousArp(arpPtr) \
    FM_DLL_GET_PREVIOUS(arpPtr, prevArp)

#define fmInsertArpAfter(switchPtr, curArp, newArp)   \
    FM_DLL_INSERT_AFTER(switchPtr, firstArp, lastArp, \
                        curArp, nextArp, prevArp, newArp)

#define fmInsertArpBefore(switchPtr, curArp, newArp)   \
    FM_DLL_INSERT_BEFORE(switchPtr, firstArp, lastArp, \
                         curArp, nextArp, prevArp, newArp)

#define fmRemoveArp(switchPtr, arpPtr)               \
    FM_DLL_REMOVE_NODE(switchPtr, firstArp, lastArp, \
                       arpPtr, nextArp, prevArp)

#endif /* __FM_FM_API_SWITCH_INT_H */
