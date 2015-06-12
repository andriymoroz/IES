/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_event_mac_maint_int.h
 * Creation Date:   May 15, 2007
 * Description:     Functions for dealing with events
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

#ifndef __FM_FM_API_EVENT_MAC_MAINT_INT_H
#define __FM_FM_API_EVENT_MAC_MAINT_INT_H

/* Macros for permission locks. */

#define FM_TAKE_MA_PURGE_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[sw]->maPurgeLock, FM_WAIT_FOREVER)

#define FM_DROP_MA_PURGE_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[sw]->maPurgeLock)


#define TAKE_MAC_MAINT_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[sw]->macTableMaintWorkListLock, \
                  FM_WAIT_FOREVER)

#define DROP_MAC_MAINT_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[sw]->macTableMaintWorkListLock)

#define VID2_ONLY_FLUSH       FM_MAX_VLAN

#define REMID_ONLY_FLUSH      8192     /* 1 bit remoteMac + 12 bits remoteId */

/**************************************************
 * The following macro can be overridden to call
 * a function to perform diagnostic verification
 * of the MA Table purge data structures.
 **************************************************/
#ifndef FM_VERIFY_MA_PURGE
#define FM_VERIFY_MA_PURGE(sw)
#endif


typedef enum
{
    /* No purge in process, ready to purge again. */
    FM_PURGE_STATE_IDLE = 0,
    
    /* Waiting for notification FIFO to quiecesce before starting purge. */
    FM_PURGE_STATE_PENDING,
    
    /* A purge is in progress. */
    FM_PURGE_STATE_ACTIVE,
    
    /* A purge completed, need to restore fast sweeper rules. */
    FM_PURGE_STATE_COMPLETE
    
} fm_purgeState;


typedef fm_status (*fm_addrMaintHandler)(fm_int sw, void *context);


/**************************************************
 * MA Table purge request list entry.
 **************************************************/

typedef struct _fm_maPurgeListEntry
{
    /* The logical port number this entry is associated with. Will be
     * -1 for non-port-specific entries. Note that the logical port may 
     * have been deleted since this entry was created (see glort member). */
    fm_int                      port;
    
    /* The glort associated with the logical port number carried in the
     * port member. We need to carry the glort in case the logical port
     * is deleted after this entry is created, but before the purge is
     * actually initiated. */
    fm_uint32                   glort;

    /* Flags indicating whether or not there are pending MA_PURGE requests 
     * for all learned addresses (via fmFlushPortAddresses or 
     * fmFlushAllDynamicAddresses), or for addresses learned on specific 
     * VLANs (via fmFlushPortVlanAddresses or fmFlushVlanAddresses). */
    fm_bool                     allVlansPending;
    fm_bitArray                 pendingVlans;

    /* Tree used for purging vid2 entries. The tree is indexed by vid1. Purging
     * of vid1/vid2 entries is done by adding vid1 to the pending vlans and
     * creating a fm_bitArray at key index vid1 in the tree. The fm_bitArrray
     * struct in the tree contains the list of vid2 entries that should be
     * purged for the corresponding vid1. The tree index VID2_ONLY_FLUSH is 
     * used to specify a vid2 purge only. */
    fm_tree                     vid2Tree;
    
    /* Tree used for purging remoteID entries. The tree is indexed by vid1.
     * Purging of vid1/remoteId entries is done by adding vid1 to the pending 
     * vlans and creating an fm_bitArray at key index vid1 in the tree. The 
     * fm_bitArrray struct in the tree contains the list of remoteID entries 
     * that should be purged for the corresponding vid1. */
    fm_tree                     remoteIdTree;

    /* False when the port for which the purge request is pending is removed. */
    fm_bool                     portExists;

    /* Doubly linked list link pointers. */
    FM_DLL_DEFINE_NODE(_fm_maPurgeListEntry, next, prev);

} fm_maPurgeListEntry;



/**************************************************
 * Internal diagnostic stats for purge operation.
 **************************************************/

typedef struct _fm_maPurgeStats
{
    /* Number of purges requested (via a call to EnqueueMAPurge) with an 
     * invalid port number. */
    fm_uint32   numRequestedInvalidPort;

    /* Number of purges requested (via a call to EnqueueMAPurge) with a 
     * valid port number. */
    fm_uint32   numRequestedGlobal;
    fm_uint32   numRequestedVlan;
    fm_uint32   numRequestedPort;
    fm_uint32   numRequestedPortVlan;
    fm_uint32   numRequestedPortVid1Vid2;
    fm_uint32   numRequestedPortVid2;
    fm_uint32   numRequestedPortVid1RemoteId;
    fm_uint32   numRequestedVid1RemoteId;
    fm_uint32   numRequestedRemoteId;
    fm_uint32   numRequestedVid1Vid2;
    fm_uint32   numRequestedVid2;
    fm_uint32   numRequestedExpired;

    /* Number of purge requests ignored because the exact same purge was 
     * already pending. */
    fm_uint32   numDuplicateGlobal;
    fm_uint32   numDuplicateVlan;
    fm_uint32   numDuplicatePort;
    fm_uint32   numDuplicatePortVlan;
    fm_uint32   numDuplicatePortVid1Vid2;
    fm_uint32   numDuplicatePortVid2;
    fm_uint32   numDuplicatePortVid1RemoteId;
    fm_uint32   numDuplicateVid1RemoteId;
    fm_uint32   numDuplicateRemoteId;
    fm_uint32   numDuplicateVid1Vid2;
    fm_uint32   numDuplicateVid2;

    /* Number of purge requests ignored because a less-specific purge was 
     * already pending. Note that this can never happen for a global request. */
    fm_uint32   numSuppressedVlan;
    fm_uint32   numSuppressedPort;
    fm_uint32   numSuppressedPortVlan;
    fm_uint32   numSuppressedPortVid1Vid2;
    fm_uint32   numSuppressedPortVid2;
    fm_uint32   numSuppressedPortVid1RemoteId;
    fm_uint32   numSuppressedVid1RemoteId;
    fm_uint32   numSuppressedRemoteId;
    fm_uint32   numSuppressedVid1Vid2;
    fm_uint32   numSuppressedVid2;

    /* Number of purge requests added to the purge list. */
    fm_uint32   numEnqueuedGlobal;
    fm_uint32   numEnqueuedVlan;
    fm_uint32   numEnqueuedPort;
    fm_uint32   numEnqueuedPortVlan;
    fm_uint32   numEnqueuedPortVid1Vid2;
    fm_uint32   numEnqueuedPortVid2;
    fm_uint32   numEnqueuedPortVid1RemoteId;
    fm_uint32   numEnqueuedVid1RemoteId;
    fm_uint32   numEnqueuedRemoteId;
    fm_uint32   numEnqueuedVid1Vid2;
    fm_uint32   numEnqueuedVid2;

    /* Number of purges that were added to the purge list but subsequently 
     * cancelled when a less-specific purge was enqueued. Note that this
     * can never happen for a global request. */
    fm_uint32   numCancelledVlan;
    fm_uint32   numCancelledPort;
    fm_uint32   numCancelledPortVlan;
    fm_uint32   numCancelledPortVid1Vid2;
    fm_uint32   numCancelledPortVid2;
    fm_uint32   numCancelledPortVid1RemoteId;
    fm_uint32   numCancelledVid1RemoteId;
    fm_uint32   numCancelledRemoteId;
    fm_uint32   numCancelledVid1Vid2;
    fm_uint32   numCancelledVid2;

    /* Number of purges that were executed (i.e. written to hardware), either
     * immediately or later on once another request completed. */
    fm_uint32   numExecutedNop;         /* FM4000 only. */
    fm_uint32   numExecutedGlobal;
    fm_uint32   numExecutedVlan;
    fm_uint32   numExecutedPort;
    fm_uint32   numExecutedPortVlan;
    fm_uint32   numExecutedPortVid1Vid2;
    fm_uint32   numExecutedPortVid2;
    fm_uint32   numExecutedPortVid1RemoteId;
    fm_uint32   numExecutedVid1RemoteId;
    fm_uint32   numExecutedRemoteId;
    fm_uint32   numExecutedVid1Vid2;
    fm_uint32   numExecutedVid2;

    /* Number of purges that completed. */
    fm_uint32   numCompletedNop;         /* FM4000 only. */
    fm_uint32   numCompletedOther;

    /* Number of purges that completed when there was another request ready 
     * to be executed. */
    fm_uint32   numCompletedWithMorePending;

    /* Number of port-specific entries on the purge list (i.e. not including 
     * the maPurgeGlobalListEntry) that were allocated. */
    fm_uint32   numEntriesAllocated;

    /* Numbers of port-specific entries on the purge list (i.e. not including 
     * the maPurgeGlobalListEntry) that were freed, either at the time that 
     * the corresponding logical port was freed, or later on after one or 
     * more pending purges were executed or cancelled. */
    fm_uint32   numEntriesFreedWithLogicalPort;
    fm_uint32   numEntriesFreedLater;

} fm_maPurgeStats;


/**************************************************
 * MA Table purge callback structure.
 **************************************************/

typedef struct _fm_maPurgeCallBack
{
    /* Function pointer to call back. */
    fm_addrMaintHandler handler;

    /* Cookie for callback function. */
    void *              context;

    /* Port number for purge request or -1 if not port-specific. */
    fm_int               port;
   
    /* VLAN for purge request or -1 if not VLAN-specific. */
    fm_int               vlan;
   
    /* Sequence number for ensuring ordering with respect to completed
     * purge requests. A completed purge's sequence number must be
     * greater than or equal to this sequence number before the callback
     * function can be called. */
    fm_uint64            seq;

    /* Chain link. */
    struct _fm_maPurgeCallBack *next;
    
} fm_maPurgeCallBack;


/**************************************************
 * Structure used to hold a MAC address table purge
 * request.
 **************************************************/
typedef struct _fm_maPurgeRequest
{
    /* The logical port to match on or -1 to match on all ports  */
    fm_int              port;
    
    /* The glort associated with the logical port. The glort needs to
     * be carried explicitly since the logical port may be deleted
     * between the time a purge is requested and the time it is 
     * actually executed. Ignored if port is -1. */
    fm_uint32           glort;

    /* The VLAN to match on, or -1 to match on all VLANs (if vid2 == -1)
     * or to match on vid2 only. Which MAC Table entries get purged is a 
     * function of whether we are in shared VLAN learning mode or independent 
     * learning mode. If shared, all entries will be purged. */
    fm_int              vid1;

    /* The VLAN2 to match on, or -1 to match on all VLANs (if vid1 == -1)
     * or to match on vid1 only. */
    fm_int              vid2;

    /* The RemoteID to match on. */
    fm_int              remoteId;

    /* Sequence number for determining when a purge request was executed
     * with respect to when a purge callback was registered. */
    fm_uint64           seq;

    /* Indicates whether entries are being deleted (no age events
     * generated for purged entries - TRUE) or flushed (age events
     * generated - FALSE). No longer used, but left in place for
     * possible future use. */
    fm_bool             deleting;
    
    /* Indicates whether static entries should also be purged
     * (currently supported only if deleting is TRUE and only
     * on the FM6000). */
    fm_bool             statics;

    /* Indicates whether we are processing a request to purge entries
     * that have aged out. (FM10000) */
    fm_bool             expired;

} fm_maPurgeRequest;


/**************************************************
 * MA Table purge control structure. Note that this
 * structure is protected by the L2 lock. Some 
 * members are additionally protected by a separate
 * purge lock.
 **************************************************/
typedef struct _fm_maPurge
{
    /* Indicates whether a MAC address table purge is in progress.
     * Qualifies most of the other fields in the structure. */
    fm_purgeState           purgeState;

    /* The purge request being executed. Valid if (active). */
    fm_maPurgeRequest       request;

    /* Specifies whether static MAC address entries should be restored 
     * after the purge has completed. Set when a purge is initiated.
     * The source for this value is the "api.FM4000.purge.restoreLocked"
     * API attribute. */
    fm_bool                 restoreLocked;

    /* Specifies whether static MAC address entries should be reloaded 
     * after the purge has completed. Set at switch boot time. 
     * The source for this value is the 
     * "api.FM4000.MATable.reloadStaticEntriesOnPurge" API attribute. 
     * Partially superseded by the restoreLocked field. */
    fm_bool                 reload;

    /* Indicates the time at which this purge began execution. Protected 
     * by purge lock.*/
    fm_timestamp            startTime;

    /* Specifies the maximum amount of time a purge is allowed to execute
     * before it times out. Set when a purge is initiated.
     * The source for this value is the "api.FM4000.purgeTimeout" API
     * attribute. */
    fm_uint                 purgeTimeout;

    /* A doubly-linked list of objects that record pending MA purges.  An 
     * entry is allocated and added to the list when a purge needs to be 
     * recorded (i.e. a purge is requested but can't be processed immediately 
     * since there is already one in progress), and there is not already an 
     * entry for the logical port (i.e. fm_port.maPurgeListEntry is NULL).  
     * An entry is only removed from the list once the logical port has been 
     * destroyed and the entry does not record any more pending purges, at 
     * which point the entry is also freed. Protected by purge lock. */
    FM_DLL_DEFINE_LIST(_fm_maPurgeListEntry, listHead, listTail);
    
    /* Head of list of callback function records. */
    fm_maPurgeCallBack *    callbackList;

    /* The current scan position in the purge list.  Indicates the last 
     * MA_PURGE request that was written to the chip. Protected by purge 
     * lock. */
    fm_maPurgeListEntry *   scanEntry;
    
    /* Entry in the MA purge list that records pending non-port-specific 
     * purges.  This is created at initialization and is permanently a member 
     * of the purge list. Protected by purge lock. */
    fm_maPurgeListEntry *   globalListEntry;
    
    /* Next activated purge sequence number. */
    fm_uint64               nextSeq;

    /* Statistics about MA purges. Protected by purge lock. */
    fm_maPurgeStats         stats;

    /* Indicates whether an FM_UPD_FLUSH_EXPIRED request is pending. */
    fm_bool                 flushExpired;

} fm_maPurge;



/**************************************************
 * These enumerated values are for the workType
 * argument to fmUpdateMATable.
 **************************************************/
typedef enum _fm_maWorkType
{
    /* The MA Table table update FIFO overflowed. This event is used
     * to initiate a full table rescan. (FM4000 only) */
    FM_UPD_UPDATE_OVERFLOW = 0x0104,

    /* Flush all MA Table entries that match the specified criteria.
     * The fm_maWorkTypeData structure specifies the purge mode and
     * any qualifying parameters. */
    FM_UPD_FLUSH_ADDRESSES,

    /* Used by the fmAddACLPort and fmDeleteACLPort API functions to
     * signal the driver to update all MA Table entries for a change to
     * ACLs. The fm_maWorkTypeData structure specifies the port number
     * with which the ACL is associated. (FM2000 only) */
    FM_UPD_ACL_UPDATE,

    /* Event used to indicate that the MA table update FIFO needs to be
     * drained. */
    FM_UPD_SERVICE_MAC_FIFO,

    /* Scan the hardware MA Table looking for inconsistencies with the table
     * cache. Update any inconsistencies and generate learn or age events
     * as appropriate. Static entries appearing in the cache will be
     * rewritten to the hardware on the assumption they were removed by
     * a hardware-based purge. */
    FM_UPD_SYNC_CACHE,

    /* Used to signal the driver to handle the purge request queue.
     * (FM4000, FM6000) */
    FM_UPD_HANDLE_PURGE,

    /* Handle a hardware purge complete. (FM6000 only) */
    FM_UPD_PURGE_COMPLETE,

    /* Used to signal the driver to scan the hardware MAC Table looking for
     * dynamic entries with remote glorts which need to be reset to the
     * young state. */
    FM_UPD_REFRESH_REMOTE,

    /* Flush all MA Table entries that have aged out. (FM10000 only) */
    FM_UPD_FLUSH_EXPIRED,

} fm_maWorkType;



/**************************************************
 * MAC table maintenance work type data.
 *
 * This structure contains data accompanying a
 * specified workType.
 **************************************************/
typedef struct _fm_maWorkTypeData
{
    /* The type of flush operation (fm_flushMode) to be performed.
     * Used with FM_UPD_FLUSH_ADDRESSES. */
    fm_int  flushMode;

    /* The port number to be updated or flushed.
     * Used with FM_UPD_FLUSH_ADDRESSES and FM_UPD_ACL_UPDATE. */
    fm_int  port;

    /* The VLAN (VLAN1) identifier to be flushed.
     * Used with FM_UPD_FLUSH_ADDRESSES. */
    fm_int  vid1;

    /* The VLAN2 identifier to be flushed.
     * Used with FM_UPD_FLUSH_ADDRESSES. (FM6000 only) */
    fm_int  vid2;

    /* The remoteID to be flushed.
     * Used with FM_UPD_FLUSH_ADDRESSES. (FM6000 only) */
    fm_int  remoteId;

} fm_maWorkTypeData;


/**************************************************
 * These enumerated values are for the 'reason'
 * member of the fm_eventTableUpdate structure.
 **************************************************/
typedef enum
{
    FM_MAC_REASON_NONE = 0,

    FM_MAC_REASON_PORT_DOWN,
    FM_MAC_REASON_LINK_DOWN,
    FM_MAC_REASON_STP_DOWN,
    FM_MAC_REASON_ALL_PORTS_DOWN,

    /* Aged as the result of a Purge request. */
    FM_MAC_REASON_PURGE_AGED,

#if 0
    FM_MAC_REASON_SCAN_AGED,
    FM_MAC_REASON_SCAN_REPLACED,
    FM_MAC_REASON_SCAN_CHANGED,
    FM_MAC_REASON_SCAN_LEARNED,

    FM_MAC_REASON_HARD_AGING,
    FM_MAC_REASON_HARD_LEARNING,
    FM_MAC_REASON_SOFT_AGING,
    FM_MAC_REASON_SOFT_LEARNING,

    FM_MAC_REASON_TCAM_MIGRATED,
    FM_MAC_REASON_TCAM_PURGED,
    FM_MAC_REASON_TCAM_AGED,
    FM_MAC_REASON_TCAM_CHANGED,

    FM_MAC_REASON_ACL_COUNT,
    FM_MAC_REASON_ACL_MONITOR,
    FM_MAC_REASON_ACL_PERMIT,
    FM_MAC_REASON_ACL_DENY,
#endif

    FM_MAC_REASON_FLUSH_DYN_ADDR,
    FM_MAC_REASON_FLUSH_PORT,
    FM_MAC_REASON_FLUSH_VLAN,
    FM_MAC_REASON_FLUSH_VLAN_PORT,

    FM_MAC_REASON_AGE_EVENT,
    FM_MAC_REASON_LEARN_EVENT,
    FM_MAC_REASON_LEARN_CHANGED,
    FM_MAC_REASON_LEARN_REPLACED,

    /* Generated by the API when the application deletes an address
     * from MA table */
    FM_MAC_REASON_API_AGED,

    /* Generated by the API when the application adds an address to the
     * MA table and an invalid (unused) entry is selected */
    FM_MAC_REASON_API_LEARNED,

    /* Generated by the API when the application adds an address to the
     * MA table. Indicates that an existing entry with the same vlanId and
     * and same MAC address is used and changed.  */
    FM_MAC_REASON_API_LEARN_CHANGED,

    /* Generated by the API when the application adds an address to the
     * MA table. Indicates that an existing entry is overwritten (replaced). */
    FM_MAC_REASON_API_LEARN_REPLACED,

    FM_MAC_REASON_VLAN_STATE,

    FM_MAC_REASON_MEM_ERROR,

} fm_macReason;


/**************************************************
 * MAC table maintenance handler work list
 *
 * this structure contains details about the work
 * that the mac table maintenance handler needs to do.
 **************************************************/
typedef struct _fm_addrMaintWorkList
{
    /* State flags. */
    fm_uint32           maintFlags;

    /* One bit per cardinal port.
     * Used with FM_MAC_MAINT_FLUSH_PORT. */
    fm_bitArray         portAddressFlushArray;

    /* One bit per cardinal port. (FM2000 only)
     * Used with FM_MAC_MAINT_UPDATE_ACL. */
    fm_bitArray         portAclUpdateArray;

    /* One bit per vlan.
     * Used with FM_MAC_MAINT_FLUSH_VLAN. */
    fm_bitArray         vlanAddressFlushArray;

    /* One bit per cardinal port per vlan.
     * Used with FM_MAC_MAINT_FLUSH_VLAN_PORT. */
    fm_bitArray         vlanPortAddressFlushArray;

    /* Pointer to callback routine. May be NULL. */
    fm_addrMaintHandler handler;

    /* Opaque parameter to be passed to callback routine. */
    void *              context;

} fm_addrMaintWorkList;


/**************************************************
 * These values are for the maintFlags field
 * of the fm_mac_table_work_list structure.
 **************************************************/

/* These flags may be set externally. */

/* The MA Table table update FIFO overflowed. Set by
 * FM_UPD_UPDATE_OVERFLOW. (FM4000 only) */
#define FM_MAC_MAINT_HANDLE_OVERFLOW    (1 << 0)

/* Update all MA Table entries for a change to ACLs.
 * Set by FM_UPD_ACL_UPDATE. (FM2000 only)
 * Uses portAclUpdateArray. */
#define FM_MAC_MAINT_UPDATE_ACL         (1 << 1)

/* Flush all MA Table entries for a particular port.
 * Set by FM_UPD_FLUSH_PORT_ADDRESSES.
 * Uses portAddressFlushArray. */
#define FM_MAC_MAINT_FLUSH_PORT         (1 << 2)

/* Flush all dynamic MA Table entries. Set by
 * FM_UPD_FLUSH_DYN_ADDRESSES. */
#define FM_MAC_MAINT_FLUSH_DYN_ADDR     (1 << 3)

/* Flush all MA Table entries for a particular vlan.
 * Set by FM_UPD_FLUSH_VLAN_ADDRESSES.
 Uses vlanAddressFlushArray. */
#define FM_MAC_MAINT_FLUSH_VLAN         (1 << 4)

/* Flush all MA Table entries for a given (Port, Vlan) combination.
 * Set by FM_UPD_FLUSH_PORT_VLAN_ADDRESSES.
 * Uses vlanPortAddressFlushArray */
#define FM_MAC_MAINT_FLUSH_VLAN_PORT    (1 << 5)

/* Read and process the MA table update (TCN) FIFO.
 * Set by FM_UPD_SERVICE_MAC_FIFO. */
#define FM_MAC_MAINT_SERVICE_FIFO       (1 << 6)

/* Scan the hardware MA Table looking for inconsistencies
 * with the software cache. Set by FM_UPD_SYNC_CACHE. */
#define FM_MAC_MAINT_SYNC_CACHE         (1 << 7)

/* Service the purge request queue. Set by FM_UPD_HANDLE_PURGE.
 * (FM4000, FM6000) */
#define FM_MAC_MAINT_HANDLE_PURGE       (1 << 8)

/* Handle a hardware purge complete condition. Set by
 * FM_UPD_PURGE_COMPLETE. (FM6000 only) */
#define FM_MAC_MAINT_PURGE_COMPLETE     (1 << 9)

/* Scan the hardware MAC Table looking for dynamic entries with
 * remote glorts that need to be reset to the young state.
 * Set by FM_UPD_REFRESH_REMOTE. */
#define FM_MAC_MAINT_REFRESH_REMOTE     (1 << 10)

/* These flags may be set internally. */

/* Rescan the cache (issue another FM_UPD_SYNC_CACHE) at the
 * end of the current pass. */
#define FM_MAC_MAINT_RESCAN_CACHE       (1 << 16)

/* Rescan the TCN FIFO (issue another FM_UPD_SERVICE_MAC_FIFO)
 * at the end of the current pass. */
#define FM_MAC_MAINT_RESCAN_FIFO        (1 << 17)

/* Rescan the current row in the MA Table to reconcile it with
 * the software cache. Used to unwind from an error. (FM4000 only) */
#define FM_MAC_MAINT_RESCAN_ROW         (1 << 18)

/* Suspend the table scan if an MA Table purge has been initiated.
 * A new scan will be kicked off when the purge completes.
 * (FM4000 only) */
#define FM_MAC_MAINT_SUSPEND_SCAN       (1 << 19)

/* These flags are the flush requests. */
#define FM_MAC_MAINT_FLUSH_REQUEST      \
    (FM_MAC_MAINT_FLUSH_DYN_ADDR    |   \
     FM_MAC_MAINT_FLUSH_PORT        |   \
     FM_MAC_MAINT_FLUSH_VLAN        |   \
     FM_MAC_MAINT_FLUSH_VLAN_PORT)

/* These flags request operations that require a full table scan. */
#define FM_MAC_MAINT_SCAN_NEEDED        \
    (FM_MAC_MAINT_FLUSH_DYN_ADDR    |   \
     FM_MAC_MAINT_FLUSH_PORT        |   \
     FM_MAC_MAINT_FLUSH_VLAN        |   \
     FM_MAC_MAINT_FLUSH_VLAN_PORT   |   \
     FM_MAC_MAINT_HANDLE_OVERFLOW   |   \
     FM_MAC_MAINT_SYNC_CACHE        |   \
     FM_MAC_MAINT_UPDATE_ACL        |   \
     FM_MAC_MAINT_REFRESH_REMOTE)

/* These flags may be set internally to request further processing. */
#define FM_MAC_MAINT_INTERNAL_REQUEST   \
    (FM_MAC_MAINT_RESCAN_CACHE      |   \
     FM_MAC_MAINT_RESCAN_FIFO       |   \
     FM_MAC_MAINT_RESCAN_ROW        |   \
     FM_MAC_MAINT_SUSPEND_SCAN)


/**************************************************
 * fm_api_event_mac_maint.c
 **************************************************/

fm_status fmAllocateMacTableMaintenanceDataStructures(fm_switch *switchPtr);
fm_status fmInitMacTableMaintenance(fm_switch *switchPtr);
fm_status fmFreeMacTableMaintenanceDataStructures(fm_switch *switchPtr);

/* Returns string representation of MA Table work type. */
char* fmMATableWorkTypeToText(fm_int workType);

fm_status fmAddMacTableMaintenanceWork(fm_int              sw,
                                       fm_maWorkType       workType,
                                       fm_maWorkTypeData   data,
                                       fm_addrMaintHandler handler,
                                       void *              context);

fm_status fmIssueMacMaintRequest(fm_int         sw,
                                 fm_maWorkType  workType);

void *fmTableMaintenanceHandler(void *args);

fm_status fmFlushPortAddrInternal(fm_int              sw,
                                  fm_int              port,
                                  fm_addrMaintHandler handler,
                                  void *              context);

/* Requests an MA Table flush. */
fm_status fmFlushMATable(fm_int              sw,
                         fm_flushMode        mode,
                         fm_flushParams      params,
                         fm_addrMaintHandler handler,
                         void *              context);

/* Requests an MA Table scan. */
fm_status fmUpdateMATable(fm_int              sw,
                          fm_maWorkType       workType,
                          fm_maWorkTypeData   data,
                          fm_addrMaintHandler handler,
                          void *              context);

/* Triggers an MA Table purge. */
fm_status fmTriggerMAPurge(fm_int sw);

/* Send a table update up to the API event handler */
void fmSendMacUpdateEvent(fm_int     sw,
                          fm_thread *eventHandler,
                          fm_uint32 *numUpdates,
                          fm_event **event,
                          fm_bool    needNewEvent);

void fmSendPurgeScanCompleteEvent(fm_int     sw,
                                  fm_thread *eventHandler);

fm_status fmAddUpdateToEvent(fm_int                   sw,
                             fm_int                   updateType,
                             fm_int                   reason,
                             fm_int                   tableIndex,
                             fm_internalMacAddrEntry *updatePtr,
                             fm_uint32 *              numUpdates,
                             fm_event *               eventPtr);

void fmGenerateUpdateForEvent(fm_int                   sw,
                              fm_thread *              eventHandler,
                              fm_int                   updateType,
                              fm_int                   reason,
                              fm_int                   tableIndex,
                              fm_internalMacAddrEntry *update,
                              fm_uint32 *              numUpdates,
                              fm_event **              outEvent);

fm_status fmCommonUpdateMATable(fm_int              sw,
                                fm_maWorkType       workType,
                                fm_maWorkTypeData   data,
                                fm_addrMaintHandler handler,
                                void *              context);

/**************************************************
 * fm_api_event_mac_purge_table.c
 **************************************************/

fm_status fmEnqueueMAPurge(fm_int              sw,
                           fm_maWorkType       workType,
                           fm_maWorkTypeData   data,
                           fm_addrMaintHandler handler,
                           void *              context);

fm_bool fmMaybeDestroyMAPurgeListEntry(fm_int sw, fm_maPurgeListEntry *entry);

fm_status fmGetNextPurgeRequest(fm_int sw);

fm_status fmAllocateMacTablePurgeList(fm_switch* switchPtr);

fm_status fmFreeMacTablePurgeList(fm_switch* switchPtr);

fm_bool fmIsAnotherPurgePending(fm_int sw);

fm_status fmCancelMacTableFlushRequests(fm_int sw);

void fmProcessPurgeCallbacks(fm_int sw);

fm_status fmCommonDumpPurgeStats(fm_int sw);

fm_status fmCommonResetPurgeStats(fm_int sw);

#endif /* __FM_FM_API_EVENT_MAC_MAINT_INT_H */
