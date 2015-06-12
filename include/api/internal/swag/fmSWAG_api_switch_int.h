/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fmSWAG_api_switch_int.h
 * Creation Date:   March 12, 2008
 * Description:     Definitions specific to switch-arrays.
 *
 * Copyright (c) 2005 - 2013, Intel Corporation
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

#ifndef __FM_FMSWAG_API_SWITCH_INT_H
#define __FM_FMSWAG_API_SWITCH_INT_H

/**************************************************
 * Aliases
 **************************************************/

#define fmSWAGDeleteAddressPre              fmCommonDeleteAddressPre

typedef struct _fmSWAG_vlanEntry
{
    /* trigger to fire when this VLAN is accessed */
    fm_int    trigger;
 
    /* mtu index */
    fm_int    mtuIndex;

    /* index into counters for this VLAN */
    fm_int    statIndex;

} fmSWAG_vlanEntry;


fm_status fmSWAGIdentifySwitch(fm_int            sw,
                               fm_switchFamily * family,
                               fm_switchModel *  model,
                               fm_switchVersion *version);

fm_status fmSWAGInitSwitch(fm_switch *swptr);
fm_status fmSWAGAllocateDataStructures(fm_switch *switchPtr);
fm_status fmSWAGFreeDataStructures(fm_switch *switchPtr);
fm_status fmSWAGGetSwitchAttribute(fm_int sw, fm_int attr, void *value);
fm_status fmSWAGSetSwitchAttribute(fm_int sw, fm_int attr, void *value);
fm_status fmSWAGComputeFHClockFreq(fm_int  sw,
                                   fm_int *pllP,
                                   fm_int *pllM,
                                   fm_int *pllN);

fm_status fmSWAGInitPortTable(fm_switch *swptr);
fm_status fmSWAGInitPort(fm_int port, fm_port *portPtr);
fm_status fmSWAGInitVirtualPort(fm_int sw, fm_port *portPtr);

fm_status fmSWAGInitBoot(fm_switch *swptr);
fm_status fmSWAGPostBootSwitch(fm_int sw);
fm_status fmSWAGBootSwitch(fm_int sw);
fm_status fmSWAGFreeResources(fm_int sw);


fm_status fmSWAGSetSwitchState(fm_int sw, fm_bool state);

fm_status fmSWAGInitializeLogicalPorts(fm_int sw);
fm_status fmSWAGEventHandlingInitialize(fm_int sw);
fm_status fmSWAGAllocLogicalPortsExt(fm_int      sw,
                                     fm_portType type,
                                     fm_int      numPorts,
                                     fm_int *    firstPortNumber,
                                     fm_int      useHandle);
fm_status fmSWAGAllocVirtualLogicalPortsExt(fm_int  sw,
                                            fm_int  memberSw,
                                            fm_int  numPorts,
                                            fm_int *firstPortNumber,
                                            fm_int  memberPortNumber,
                                            fm_int  useHandle,
                                            fm_int  firstGlort);
fm_status fmSWAGFreeLogicalPort(fm_int sw, fm_int port);
fm_status fmSWAGGetLogicalPort(fm_int    sw,
                               fm_uint32 glort,
                               fm_int *  logicalPort);
fm_status fmSWAGGetGlort(fm_int     sw,
                         fm_int     logicalPort,
                         fm_uint32 *glort);
fm_status fmSWAGSetLogicalPortAttribute(fm_int sw,
                                        fm_int port,
                                        fm_int attr,
                                        void * value);
fm_status fmSWAGGetLogicalPortAttribute(fm_int sw,
                                        fm_int port,
                                        fm_int attr,
                                        void * value);


#if 0
fm_status fmSWAGAllocDestEntries(fm_int              sw,
                                 fm_int              numDestEntries,
                                 fm_glortCamEntry *  camEntry,
                                 fm_glortDestEntry **destEntry);
fm_status fmSWAGFreeDestEntry(fm_int             sw,
                              fm_glortDestEntry *destEntry);
fm_status fmSWAGSetGlortDestMask(fm_int             sw,
                                 fm_glortDestEntry *destEntry,
                                 fm_uint32          destMask);
fm_status fmSWAGWriteGlortCamEntry(fm_int            sw,
                                   fm_glortCamEntry *camEntry);


#endif
fm_status fmSWAGGetSwitchInfo(fm_int sw, fm_switchInfo *info);

fm_status fmSWAGAllocAddrTableData(fm_switch *switchPtr);
fm_status fmSWAGFreeAddrTableData(fm_switch *switchPtr);
fm_status fmSWAGInitAddressTable(fm_switch *switchPtr);
fm_status fmSWAGAddAddressToTablePre(fm_int sw,
                                     fm_macAddressEntry *entry,
                                     fm_uint32           trigger,
                                     fm_bool             updateHw,
                                     fm_int              bank,
                                     fm_bool *           complete);
fm_status fmSWAGDeleteAddressFromTable(fm_int              sw,
                                       fm_macAddressEntry *entry,
                                       fm_bool             overrideMspt,
                                       fm_bool             updateHw,
                                       fm_int              bank);
fm_status fmSWAGRemoveAddressFromTree(fm_int              sw,
                                      fm_macAddressEntry *entry,
                                      fm_bool             overrideMspt,
                                      fm_bool             updateHw);
fm_status fmSWAGUpdateMATable(fm_int              sw,
                              fm_maWorkType       workType,
                              fm_maWorkTypeData   data,
                              fm_addrMaintHandler handler,
                              void *              context);
fm_status fmSWAGGetAddress(fm_int              sw,
                           fm_macaddr          address,
                           fm_uint16           vlanID,
                           fm_uint16           vlanID2,
                           fm_macAddressEntry *entry);
fm_status fmSWAGAddAddress(fm_int sw, fm_macAddressEntry *entry);
fm_status fmSWAGDeleteAllAddresses(fm_int sw, fm_bool dynamicOnly);
fm_status fmSWAGConvertEntryToWords(fm_int                      sw,
                                    fm_internal_mac_addr_entry *entry,
                                    fm_uint32 *                 words);
fm_status fmSWAGConvertWordsToEntry(fm_int                      sw,
                                    fm_internal_mac_addr_entry *entry,
                                    fm_uint32 *                 words);
fm_status fmSWAGFillInUserEntryFromTable(fm_int                      sw,
                                         fm_internal_mac_addr_entry *tblentry,
                                         fm_macAddressEntry *        entry);
fm_status fmSWAGComputeAddressIndex(fm_int     sw,
                                    fm_macaddr entry,
                                    fm_uint16  vlanID,
                                    fm_uint16  vlanID2,
                                    fm_uint16 *indexes);
fm_status fmSWAGReadEntryAtIndex(fm_int                      sw,
                                 fm_uint32                   index,
                                 fm_internal_mac_addr_entry *entry);
fm_status fmSWAGWriteEntryAtIndex(fm_int                      sw,
                                  fm_uint32                   index,
                                  fm_internal_mac_addr_entry *entry);
fm_status fmSWAGGetAddressTable(fm_int              sw,
                                fm_int *            nEntries,
                                fm_macAddressEntry *entries,
                                fm_int              maxEntries);
fm_status fmSWAGGetAddressTableAttribute(fm_int sw,
                                         fm_int attr,
                                         void * value);
fm_status fmSWAGSetAddressTableAttribute(fm_int sw,
                                         fm_int attr,
                                         void * value);
fm_status fmSWAGGetDefaultSVLFID(fm_int sw, fm_uint16 *defaultSvlFid);
fm_status fmSWAGGetLearningFID(fm_int sw, fm_uint16 vlanId, fm_uint16 *learningFid);
fm_status fmSWAGProcessMACLearnedEvent(fm_int sw,
                                       fm_int sourceSwitch,
                                       fm_eventTableUpdate *macEvent);
fm_status fmSWAGProcessMACAgedEvent(fm_int sw,
                                    fm_int sourceSwitch,
                                    fm_eventTableUpdate *macEvent);

fm_status fmSWAGSetSwitchQOS(fm_int sw,
                             fm_int attr,
                             fm_int index,
                             void * value);
fm_status fmSWAGGetSwitchQOS(fm_int sw,
                             fm_int attr,
                             fm_int index,
                             void * value);

fm_status fmSWAGGetWatermarkParameters(fm_int sw, fm_wmParam *wpm);
fm_status fmSWAGSetWatermarkParameters(fm_int sw);
fm_status fmSWAGGetPauseState(fm_int sw, fm_bool *pause_en);

fm_status fmSWAGAllocateLAGs(fm_int    sw,
                             fm_uint    startGlort,
                             fm_uint    glortSize,
                             fm_int    *baseLagHandle,
                             fm_int    *numLags,
                             fm_int    *step);
fm_status fmSWAGFreeStackLAGs(fm_int sw, fm_int baseLagHandle);
fm_status fmSWAGCreateLagOnSwitch(fm_int sw, fm_int lagIndex);
fm_status fmSWAGDeleteLagFromSwitch(fm_int sw, fm_int lagIndex);
fm_status fmSWAGAddPortToLag(fm_int sw, fm_int lagIndex, fm_int port);
fm_status fmSWAGDeletePortFromLag(fm_int sw,
                                  fm_int lagIndex,
                                  fm_int port);
void fmSWAGFreeLAG(fm_int sw, fm_int lagIndex);
fm_status fmSWAGInformLAGPortUp(fm_int sw, fm_int port);
fm_status fmSWAGInformLAGPortDown(fm_int sw, fm_int port);
fm_status fmSWAGGetLagAttribute(fm_int sw,
                                fm_int attribute,
                                fm_int index,
                                void * value);
fm_status fmSWAGSetLagAttribute(fm_int sw,
                                fm_int attribute,
                                fm_int index,
                                void * value);
fm_status fmSWAGGetGlortForLagPort(fm_int     sw,
                                   fm_int     port,
                                   fm_uint32 *glort);

fm_status fmSWAGCreateMirror(fm_int              sw,
                             fm_portMirrorGroup *grp);
fm_status fmSWAGDeleteMirror(fm_int              sw,
                             fm_portMirrorGroup *grp);
fm_status fmSWAGAddMirrorPort(fm_int              sw,
                              fm_portMirrorGroup *grp,
                              fm_int              port,
                              fm_mirrorType       mirrorType);
fm_status fmSWAGDeleteMirrorPort(fm_int              sw,
                                 fm_portMirrorGroup *grp,
                                 fm_int              port);
fm_status fmSWAGSetMirrorDestination(fm_int              sw,
                                     fm_portMirrorGroup *grp,
                                     fm_int              mirrorPort);
fm_status fmSWAGSetMirrorAttribute(fm_int              sw,
                                   fm_portMirrorGroup *grp,
                                   fm_int              attr,
                                   void *              value);
fm_status fmSWAGGetMirrorAttribute(fm_int              sw,
                                   fm_portMirrorGroup *grp,
                                   fm_int              attr,
                                   void *              value);
fm_status fmSWAGAddMirrorVlan(fm_int              sw,
                              fm_portMirrorGroup *grp,
                              fm_vlanSelect       vlanSel,
                              fm_uint16           vlanID,
                              fm_mirrorVlanType   direction);
fm_status fmSWAGDeleteMirrorVlan(fm_int              sw,
                                 fm_portMirrorGroup *grp,
                                 fm_vlanSelect       vlanSel,
                                 fm_uint16           vlanID);

fm_status fmSWAGResetFIDPortState(fm_int sw, fm_int fid, fm_int port);
fm_status fmSWAGGetVlanMembership(fm_int        sw,
                                  fm_vlanEntry *entry,
                                  fm_int        port,
                                  fm_bool *     state);

fm_status fmSWAGSetVlanMembership(fm_int        sw,
                                  fm_vlanEntry *entry,
                                  fm_int        port,
                                  fm_bool       state);
fm_status fmSWAGSetVlanPortState(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    port,
                                 fm_int    state);
fm_status fmSWAGGetVlanPortState(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    port,
                                 fm_int *  state);
fm_status fmSWAGCreateVlan(fm_int sw, fm_uint16 vlanID);
fm_status fmSWAGDeleteVlan(fm_int sw, fm_uint16 vlanID);
fm_status fmSWAGAddVlanPort(fm_int    sw,
                            fm_uint16 vlanID,
                            fm_int    port,
                            fm_bool   tag);
fm_status fmSWAGDeleteVlanPort(fm_int    sw,
                               fm_uint16 vlanID,
                               fm_int    port);
fm_status fmSWAGChangeVlanPort(fm_int        sw,
                               fm_vlanSelect vlanSel,
                               fm_uint16     vlanID,
                               fm_int        port,
                               fm_bool       tag);
fm_status fmSWAGGetPortAttribute(fm_int sw,
                                 fm_int port,
                                 fm_int mac,
                                 fm_int lane,
                                 fm_int attr,
                                 void * value);
fm_status fmSWAGSetPortAttribute(fm_int sw,
                                 fm_int port,
                                 fm_int mac,
                                 fm_int lane,
                                 fm_int attr,
                                 void * value);
fm_status fmSWAGSetVlanTag(fm_int        sw,
                           fm_vlanSelect vlanSel,
                           fm_vlanEntry *entry,
                           fm_int        port,
                           fm_bool       tag);

fm_status fmSWAGGetVlanTag(fm_int        sw,
                           fm_vlanSelect vlanSel,
                           fm_vlanEntry *entry,
                           fm_int        port,
                           fm_bool *     tag);
fm_status fmSWAGSetVlanAttribute(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    attr,
                                 void *    value);
fm_status fmSWAGGetVlanAttribute(fm_int    sw,
                                 fm_uint16 vlanID,
                                 fm_int    attr,
                                 void *    value);

fm_status fmSWAGInterruptHandler(fm_switch *switchPtr);

fm_status fmSWAGRefreshSpanningTree(fm_int sw,
                                    fm_stpInstanceInfo *instance,
                                    fm_int vlanID,
                                    fm_int port);

fm_status fmSWAGCreateSpanningTree(fm_int sw,
                                   fm_int stpInstance);

fm_status fmSWAGDeleteSpanningTree(fm_int sw,
                                   fm_int stpInstance);

fm_status fmSWAGAddSpanningTreeVlan(fm_int sw,
                                    fm_int stpInstance,
                                    fm_int vlanID);

fm_status fmSWAGDeleteSpanningTreeVlan(fm_int sw,
                                       fm_int stpInstance,
                                       fm_int vlanID);

fm_status fmSWAGSetSpanningTreePortState(fm_int sw,
                                         fm_int stpInstance,
                                         fm_int port,
                                         fm_int stpState);

fm_status fmSWAGGetSpanningTreePortState(fm_int sw,
                                         fm_int stpInstance,
                                         fm_int port,
                                         fm_int *stpState);

/* Debug functions. */
fm_status fmSWAGDbgDumpGlortTable(fm_int sw);
void fmSWAGDbgDumpVid(fm_int sw);
void fmSWAGDbgDumpMulticastTables(fm_int sw);
void fmSWAGDbgDumpMulticastUsage(fm_int sw);
void fmSWAGDbgDumpMulticastPortVlanEntryList(fm_int sw);
void fmSWAGDbgDumpMACTable(fm_int sw, fm_int numEntries);
void fmSWAGDbgReadRegister(fm_int  sw,
                           fm_int  firstIndex,
                           fm_int  secondIndex,
                           fm_text registerName,
                           void *  value);
void fmSWAGDbgDumpRegister(fm_int sw, fm_int port, fm_text regname);
fm_status fmSWAGDbgDumpRegisterV2(fm_int  sw,
                                  fm_int  indexA,
                                  fm_int  indexB,
                                  fm_text regname);
void fmSWAGDbgListRegisters(fm_int sw, fm_bool showGlobals, fm_bool showPorts);
void fmSWAGDbgGetRegisterName(fm_int   sw,
                              fm_int   regId,
                              fm_uint  regAddress,
                              fm_text  regName,
                              fm_uint  regNameLength,
                              fm_bool *isPort,
                              fm_int * index0Ptr,
                              fm_int * index1Ptr,
                              fm_int * index2Ptr,
                              fm_bool  logicalPorts,
                              fm_bool  partialLongRegs);
void fmSWAGDbgTakeChipSnapshot(fm_int                sw,
                               fmDbgFulcrumSnapshot *pSnapshot,
                               fm_regDumpCallback    callback);
void fmSWAGDbgDumpFFU(fm_int sw, fm_bool onlyPrintValidMinslices);


/* Statistics.
 */
fm_status fmSWAGGetSwitchCounters(fm_int sw, fm_switchCounters *counters);
fm_status fmSWAGGetVLANCounters(fm_int           sw,
                                fm_int           vcid,
                                fm_vlanCounters *counters);

void fmSWAGDebounceLinkStates(fm_int     sw,
                              fm_thread *thread,
                              fm_thread *handlerThread);

fm_status fmSWAGSendLinkUpDownEvent(fm_int           sw,
                                    fm_int           physPort,
                                    fm_bool          linkUp,
                                    fm_eventPriority priority);

fm_status fmSWAGSendPacketDirected(fm_int           sw,
                                   fm_int *         portList,
                                   fm_int           numPorts,
                                   fm_buffer *      pkt,
                                   fm_packetInfoV2 *info);

fm_status fmSWAGSendPacket(fm_int         sw,
                           fm_packetInfo *info,
                           fm_buffer *    pkt);

fm_status fmSWAGSetPacketInfo(fm_int         sw,
                              fm_packetInfo *info,
                              fm_uint32      destMask);

fm_status fmSWAGSendPacketSwitched(fm_int     sw,
                                   fm_buffer *pkt);

fm_status fmSWAGSendPacketISL(fm_int          sw,
                              fm_uint32 *     islTag,
                              fm_islTagFormat islTagFormat,
                              fm_buffer *     pkt);


/* Routing functions */
fm_status fmSWAGRouterAlloc(fm_int sw);
fm_status fmSWAGRouterFree(fm_int sw);
fm_status fmSWAGRouterInit(fm_int sw);
fm_status fmSWAGSetRouterAttribute(fm_int sw,
                                   fm_int attr,
                                   void * value);
fm_status fmSWAGAddRoute(fm_int            sw,
                         fm_intRouteEntry *route);
fm_status fmSWAGDeleteRoute(fm_int            sw,
                            fm_intRouteEntry *route);
fm_status fmSWAGSetRouteActive(fm_int            sw,
                               fm_intRouteEntry *route);
fm_status fmSWAGAddARPEntry(fm_int          sw,
                            fm_intArpEntry *arp);
fm_status fmSWAGDeleteARPEntry(fm_int          sw,
                               fm_intArpEntry *arp);
fm_status fmSWAGUpdateARPEntryDMAC(fm_int          sw,
                                   fm_intArpEntry *arp);
fm_status fmSWAGUpdateARPEntryVrid(fm_int          sw,
                                   fm_intArpEntry *arp);
fm_status fmSWAGGetARPEntryUsed(fm_int          sw,
                                fm_intArpEntry *arp,
                                fm_bool *       used,
                                fm_bool         resetFlag);
fm_status fmSWAGSetInterfaceAttribute(fm_int sw,
                                      fm_int interface,
                                      fm_int attr,
                                      void * value);
fm_status fmSWAGAddVirtualRouter(fm_int sw, fm_int vroff);
fm_status fmSWAGRemoveVirtualRouter(fm_int sw, fm_int vroff);
fm_status fmSWAGSetRouterState(fm_int         sw,
                               fm_int         vroff,
                               fm_routerState state);
void fmSWAGDbgDumpRouteStats(fm_int sw);
void fmSWAGDbgDumpRouteTables(fm_int sw, fm_int flags);
fm_status fmSWAGDbgValidateRouteTables(fm_int sw);
fm_status fmSWAGCreateInterface(fm_int  sw,
                                fm_int ifNum);
fm_status fmSWAGDeleteInterface(fm_int  sw,
                                fm_int ifNum);
fm_status fmSWAGAddInterfaceAddr(fm_int     sw,
                                 fm_int     interface,
                                 fm_ipAddr *addr);
fm_status fmSWAGDeleteInterfaceAddr(fm_int     sw,
                                    fm_int     interface,
                                    fm_ipAddr *addr);
fm_status fmSWAGCreateECMPGroup(fm_int sw, fm_intEcmpGroup *group);
fm_status fmSWAGDeleteECMPGroup(fm_int sw, fm_intEcmpGroup *group);
fm_status fmSWAGAddECMPGroupNextHops(fm_int           sw,
                                     fm_intEcmpGroup *group,
                                     fm_int           numNextHops,
                                     fm_ecmpNextHop * nextHopList);
fm_status fmSWAGDeleteECMPGroupNextHops(fm_int           sw,
                                        fm_intEcmpGroup *group,
                                        fm_int           numRemovedHops,
                                        fm_intNextHop ** removedHops,
                                        fm_int           numNextHops,
                                        fm_ecmpNextHop * nextHopList);
fm_status fmSWAGReplaceECMPGroupNextHop(fm_int           sw,
                                        fm_intEcmpGroup *group,
                                        fm_intNextHop *  oldNextHop,
                                        fm_intNextHop *  newNextHop);
fm_status fmSWAGSetECMPGroupNextHops(fm_int           sw,
                                     fm_intEcmpGroup *group,
                                     fm_int           firstIndex,
                                     fm_int           numNextHops,
                                     fm_ecmpNextHop * nextHopList);
fm_status fmSWAGUpdateEcmpGroup(fm_int sw, fm_intEcmpGroup *group);
fm_status fmSWAGUpdateNextHop(fm_int sw, fm_intNextHop *nextHop);
fm_status fmSWAGGetNextHopUsed(fm_int         sw,
                               fm_intNextHop *nextHop,
                               fm_bool*       used,
                               fm_bool        resetFlag);
fm_status fmSWAGAddArpEntry(fm_int sw, fm_arpEntry *arp);
fm_status fmSWAGDeleteArpEntry(fm_int sw, fm_arpEntry *arp);
fm_status fmSWAGUpdateArpEntryDestMac(fm_int sw, fm_arpEntry *arp);
fm_status fmSWAGUpdateArpEntryVrid(fm_int sw, fm_arpEntry *arp, fm_int vrid);

fm_status fmSWAGCreateForwardingRule(fm_int          sw,
                                     fm_int *        id,
                                     fm_forwardRule *rule);
fm_status fmSWAGDeleteForwardingRule(fm_int sw, fm_int id);
fm_status fmSWAGCreateLogicalPortForGlort(fm_int    sw, 
                                          fm_uint32 glort, 
                                          fm_int *  logicalPort);
fm_status fmSWAGSetStackGlortRange(fm_int    sw);
fm_status fmSWAGSetStackLogicalPortState(fm_int sw,
                                         fm_int port,
                                         fm_int mode);
fm_status fmSWAGCreatePolicer(fm_int                  sw,
                              fm_int                  bank,
                              fm_int                  policer,
                              const fm_policerConfig *config);
fm_status fmSWAGDeletePolicer(fm_int sw,
                              fm_int policer);
fm_status fmSWAGSetPolicerAttribute(fm_int      sw,
                                    fm_int      policer,
                                    fm_int      attr,
                                    const void *value);
fm_status fmSWAGUpdatePolicer(fm_int sw, fm_int policer);
fm_status fmSWAGInitVlanTable(fm_switch *switchPtr);
fm_status fmSWAGFreeVlanTableDataStructures(fm_switch *switchPtr);
fm_status fmSWAGAllocateVlanTableDataStructures(fm_switch *switchPtr);
fm_status fmSWAGAddVlanPortList(fm_int    sw,
                                fm_uint16 vlanID,
                                fm_int    numPorts,
                                fm_int *  portList,
                                fm_bool   tag);
fm_status fmSWAGDeleteVlanPortList(fm_int    sw,
                                   fm_uint16 vlanID,
                                   fm_int    numPorts,
                                   fm_int *  portList);

#endif /* __FM_FMSWAG_API_SWITCH_INT_H */
