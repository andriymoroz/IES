/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_addr_int.h
 * Creation Date:   December 3, 2012
 * Description:     Definitions specific to the FM10000 MA Table processing
 *
 * Copyright (c) 2009 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_ADDR_INT_H
#define __FM_FM10000_API_ADDR_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/**************************************************
 * MAC Security information structure.
 **************************************************/
typedef struct _fm10000_SecurityInfo
{
    /* Trigger identifier associated with MAC Trigger T3. Assigned to addresses
     * learned on unsecured ports. Used to detect unsecured-to-secured station
     * moves. */
    fm_uint32   triggerId3;

    /* Rate limiter associated with the FM_STORM_COND_SECURITY_VIOL_NEW_MAC
     * storm controller condition. Applies to the TrPortMissTrap trigger. */
    fm_uint32   newMacLimiter;

    /* Rate limiter associated with the FM_STORM_COND_SECURITY_VIOL_MOVE
     * storm controller condition. Applies to the TrMacMoveTrap and
     * TrPortMoveTrap triggers. */
    fm_uint32   macMoveLimiter;

    /* Dummy rate limiter associated with triggers that don't use one. */
    fm_uint32   noRateLimiter;

    /* Set of all ports that are configured for TRAP or EVENT.
     * Used to configure the TrPortMissTrap and TrPortMoveTrap triggers. */
    fm_int      trapPortSet;

    /* Set of all ports that are configured for DROP.
     * Used to configure the TrPortMissDrop and TrPortMoveDrop triggers. */
    fm_int      dropPortSet;

    /* Port set to use for secure address moves.
     * Set to either FM_PORT_SET_NONE or FM_PORT_SET_ALL_BUT_EXTERNAL.
     * Used to configure the TrMacMoveTrap trigger. */
    fm_int      securePortSet;

} fm10000_securityInfo;


/**************************************************
 * Security violation actions.
 * Returned by fm10000CheckSecurityViolation. 
 **************************************************/
typedef enum
{
    FM10000_SV_NO_ACTION,

    /* Unknown SMAC events. */
    FM10000_SV_UNKNOWN_SMAC_DROP,
    FM10000_SV_UNKNOWN_SMAC_EVENT,
    FM10000_SV_UNKNOWN_SMAC_TRAP,

    /* Non-Secure SMAC events. */
    FM10000_SV_NON_SECURE_SMAC_DROP,
    FM10000_SV_NON_SECURE_SMAC_EVENT,
    FM10000_SV_NON_SECURE_SMAC_TRAP,

    /* Secure SMAC events. */
    FM10000_SV_SECURE_SMAC_DROP,
    FM10000_SV_SECURE_SMAC_EVENT,
    FM10000_SV_SECURE_SMAC_TRAP,

} fm10000_svAction;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

/**************************************************
 * Aliases
 **************************************************/

#define fm10000AllocAddrTableData               fmCommonAllocAddrTableCache
#define fm10000CheckFlushRequest                fmCommonCheckFlushRequest
#define fm10000DeleteAddressPre                 fmCommonDeleteAddressPre
#define fm10000DeleteAllAddresses               fmCommonDeleteAllAddresses
#define fm10000DumpPurgeStats                   fmCommonDumpPurgeStats
#define fm10000FindAndInvalidateAddress         fmCommonFindAndInvalidateAddr
#define fm10000FreeAddrTableData                fmCommonFreeAddrTableCache
#define fm10000ResetPurgeStats                  fmCommonResetPurgeStats
#define fm10000UpdateMATable                    fmCommonUpdateMATable

/**************************************************
 * fm10000_api_addr.c
 **************************************************/

fm_status fm10000AddAddress(fm_int sw, fm_macAddressEntry *entry);

fm_status fm10000AddMacTableEntry(fm_int               sw,
                                  fm_macAddressEntry * entry,
                                  fm_macSource         source,
                                  fm_uint32            trigger,
                                  fm_uint32 *          numUpdates,
                                  fm_event **          outEvent);

fm_status fm10000AssignTableEntry(fm_int              sw,
                                  fm_macAddressEntry *entry,
                                  fm_int              targetBank,
                                  fm_uint32           trigger,
                                  fm_bool             updateHw,
                                  fm_uint32*          numUpdates,
                                  fm_event**          outEvent);

fm_status fm10000CalcAddrHash(fm_macaddr  macAddr,
                              fm_uint16   fid,
                              fm_uint32   hashMode,
                              fm_uint16 * hashes);

fm_status fm10000CheckVlanMembership(fm_int    sw,
                                     fm_uint16 vlanID,
                                     fm_int    port);

fm_status fm10000ComputeAddressIndex(fm_int     sw,
                                     fm_macaddr entry,
                                     fm_uint16  vlanID,
                                     fm_uint16  vlanID2,
                                     fm_uint16 *indexes);

fm_status fm10000ConvertEntryToWords(fm_int                   sw,
                                     fm_internalMacAddrEntry *entry,
                                     fm_int                   port,
                                     fm_uint32 *              words);

#if 0
fm_status fm10000ConvertWordsToEntry(fm_int                      sw,
                                     fm_internalMacAddrEntry *entry,
                                     fm_uint32 *              words);
#endif

fm_status fm10000FillInUserEntryFromTable(fm_int                   sw,
                                          fm_internalMacAddrEntry *tblentry,
                                          fm_macAddressEntry *     entry);

fm_status fm10000GetAddress(fm_int              sw,
                            fm_macaddr          address,
                            fm_uint16           vlanID,
                            fm_uint16           vlanID2,
                            fm_macAddressEntry *entry);

fm_status fm10000GetAddressTable(fm_int              sw,
                                 fm_int *            nEntries,
                                 fm_macAddressEntry *entries,
                                 fm_int              maxEntries);

fm_status fm10000GetAddressTableAttribute(fm_int sw, 
                                          fm_int attr, 
                                          void * value);

fm_status fm10000GetLearningFID(fm_int      sw,
                                fm_uint16   vlanId,
                                fm_uint16 * learningFid);

fm_status fm10000InitAddrHash(void);

fm_status fm10000InitAddressTable(fm_switch * switchPtr);

fm_status fm10000InvalidateEntryAtIndex(fm_int sw, fm_uint32 index);

#if 0
fm_status fm10000ReadEntryAtIndex(fm_int                   sw,
                                  fm_uint32                index,
                                  fm_internalMacAddrEntry *entry);
#endif

fm_status fm10000SetAddressTableAttribute(fm_int sw, 
                                          fm_int attr, 
                                          void * value);

fm_status fm10000WriteEntryAtIndex(fm_int                   sw,
                                   fm_uint32                index,
                                   fm_internalMacAddrEntry *entry);

fm_status fm10000WriteSmacEntry(fm_int                    sw,
                                fm_uint32                 index,
                                fm_internalMacAddrEntry * entry);


/**************************************************
 * fm10000_api_mac_security.c
 **************************************************/

fm_status fm10000AssignMacTrigger(fm_int               sw,
                                  fm_macAddressEntry * entry,
                                  fm_uint32 *          trigger);

fm_status fm10000CheckSecurityViolation(fm_int   sw,
                                        fm_int   port,
                                        fm_int   trapCode,
                                        fm_int * trapAction);

fm_status fm10000DbgDumpMacSecurity(fm_int sw);

fm_status fm10000FreeMacSecurity(fm_int sw);

fm_status fm10000GetSecurityStats(fm_int sw, fm_securityStats * stats);

fm_status fm10000InitMacSecurity(fm_int sw);

fm_status fm10000MoveAddressSecure(fm_int       sw,
                                   fm_macaddr   macAddress,
                                   fm_uint16    vlanID,
                                   fm_int       port,
                                   fm_uint32    index,
                                   fm_uint32 *  numUpdates,
                                   fm_event **  outEvent);

fm_status fm10000NotifyMacSecurityRateLimiterId(fm_int    sw,
                                                fm_int    cond, 
                                                fm_uint32 rateLimiterId);

fm_status fm10000ReportSecurityEvent(fm_int     sw,
                                     fm_macaddr macAddress,
                                     fm_uint16  vlanID,
                                     fm_int     port);

fm_status fm10000ResetSecurityStats(fm_int sw);

fm_status fm10000SetPortSecurityAction(fm_int    sw,
                                       fm_int    port,
                                       fm_uint32 action);

fm_status fm10000UpdateMacSecurity(fm_int sw);

const char * fmSvActionToText(fm_int svAction);


/**************************************************
 * fm10000_debug_mac_table.c
 **************************************************/
 
void fm10000DbgDumpMACTable(fm_int sw, fm_int numEntries);

void fm10000DbgDumpMACTableEntry(fm_int     sw, 
                                 fm_macaddr address, 
                                 fm_uint16  vlan);


#endif /* __FM_FM10000_API_ADDR_INT_H */
