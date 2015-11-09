/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_addr_int.h
 * Creation Date:   2005
 * Description:     Contains structure definitions and constants related to
 *                  the address table
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

#ifndef __FM_FM_API_ADDR_INT_H
#define __FM_FM_API_ADDR_INT_H


/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* Format for a MAC address. */
#define FM_FORMAT_ADDR      "%012" FM_FORMAT_64 "X"


/* Well-known MAC addresses. */
#define FM_BPDU_DEST_ADDRESS            FM_LITERAL_64(0x0180C2000000)
#define FM_LACP_DEST_ADDRESS            FM_LITERAL_64(0x0180C2000002)
#define FM_DOT1X_DEST_ADDRESS           FM_LITERAL_64(0x0180C2000003)


/* Determine whether we're tracing a specified MAC address. */
#define FM_IS_TEST_TRACE_ADDRESS(macAddr)               \
    ((fmRootApi->testTraceMacAddress != 0) &&           \
     (fmRootApi->testTraceMacAddress == (macAddr)))


/* No trigger identifier specified. */
#define FM_DEFAULT_TRIGGER              ((fm_uint32) -1)


/**************************************************
 * MA Table Address Type masks and predicates.
 **************************************************/

/* Mask for all static address types. */
#define FM_ADDR_TYPE_MASK_STATIC                    \
     ((1 << FM_ADDRESS_STATIC)                  |   \
      (1 << FM_ADDRESS_SECURE_STATIC))

/* Mask for all dynamic address types. */
#define FM_ADDR_TYPE_MASK_DYNAMIC                   \
     ((1 << FM_ADDRESS_DYNAMIC)                 |   \
      (1 << FM_ADDRESS_SECURE_DYNAMIC)          |   \
      (1 << FM_ADDRESS_PROVISIONAL))

/* Mask for all secure address types. */
#define FM_ADDR_TYPE_MASK_SECURE                    \
      ((1 << FM_ADDRESS_SECURE_STATIC)          |   \
       (1 << FM_ADDRESS_SECURE_DYNAMIC))

/* Predicate macros. */
#define FM_IS_ADDR_TYPE(x, mask)        \
       ( ( (1 << (x)) & FM_ADDR_TYPE_MASK_ ## mask ) != 0)

#define FM_IS_ADDR_TYPE_STATIC(x)   FM_IS_ADDR_TYPE(x, STATIC)
#define FM_IS_ADDR_TYPE_DYNAMIC(x)  FM_IS_ADDR_TYPE(x, DYNAMIC)
#define FM_IS_ADDR_TYPE_SECURE(x)   FM_IS_ADDR_TYPE(x, SECURE)


/**************************************************
 * MA Table Entry state masks and predicates.
 **************************************************/

/* Mask for all dynamic address entries. */
#define FM_MAC_ENTRY_STATE_MASK_DYNAMIC             \
        ((1 << FM_MAC_ENTRY_STATE_OLD)          |   \
         (1 << FM_MAC_ENTRY_STATE_YOUNG)        |   \
         (1 << FM_MAC_ENTRY_STATE_EXPIRED))

/* Mask for all young address entries. */
#define FM_MAC_ENTRY_STATE_MASK_YOUNG               \
        ((1 << FM_MAC_ENTRY_STATE_YOUNG)        |   \
         (1 << FM_MAC_ENTRY_STATE_MOVED)        |   \
         (1 << FM_MAC_ENTRY_STATE_PROVISIONAL_YOUNG))

#define FM_IS_MAC_ENTRY_STATE(x, mask)  \
       ( ( (1 << (x)) & FM_MAC_ENTRY_STATE_MASK_ ## mask ) != 0)

#define FM_IS_MAC_ENTRY_STATE_DYNAMIC(x)    FM_IS_MAC_ENTRY_STATE(x, DYNAMIC)
#define FM_IS_MAC_ENTRY_STATE_YOUNG(x)      FM_IS_MAC_ENTRY_STATE(x, YOUNG)


/**************************************************
 * Internal MA Table Entry state.
 * (fm_internalMacAddrEntry::state)
 **************************************************/
typedef enum
{
    FM_MAC_ENTRY_STATE_INVALID,
    FM_MAC_ENTRY_STATE_OLD,
    FM_MAC_ENTRY_STATE_YOUNG,
    FM_MAC_ENTRY_STATE_LOCKED,

    FM_MAC_ENTRY_STATE_MOVED,               /* FM6000 only, read-only */
    FM_MAC_ENTRY_STATE_PROVISIONAL_YOUNG,   /* FM6000 only, read-only */
    FM_MAC_ENTRY_STATE_PROVISIONAL_OLD,     /* FM6000 only, read-only */

    /* Entry has aged out, and needs to be removed from the MA Table.
     * (FM10000 only) */
    FM_MAC_ENTRY_STATE_EXPIRED,

} fm_macEntryState;


/**************************************************
 * Internal MA Table Entry type.
 * (fm_internalMacAddrEntry::entryType)
 * Note that this field is only valid following
 * a call to fmGetAddressInternal.
 **************************************************/
typedef enum
{
    FM_MAC_ENTRY_TYPE_UNKNOWN,

    /* Entry is in software cache. */
    FM_MAC_ENTRY_TYPE_CACHE,

    /* Entry is in TCAM address table. (FM4000 only) */
    FM_MAC_ENTRY_TYPE_OFFLOAD,

    /* Entry is in hardware MA table. */
    FM_MAC_ENTRY_TYPE_HW_MATABLE,

} fm_macEntryType;


/**************************************************
 * Internal MA Table Entry dirty state.
 * (fm_internalMacAddrEntry::dirty)
 **************************************************/
enum
{
    FM_MAC_ENTRY_WAITING,
    FM_MAC_ENTRY_DIRTY,
    FM_MAC_ENTRY_POSTED,
    FM_MAC_ENTRY_RECORDED

};


/**************************************************
 * Internal MA Table Entry source.
 **************************************************/
typedef enum
{
    /* The source of the MA Table entry is unknown. */
    FM_MAC_SOURCE_UNKNOWN = 0,

    /* The MA Table entry is the result of a TCN FIFO NewSource event. */
    FM_MAC_SOURCE_TCN_LEARNED,

    /** The MA Table entry is the result of a TCN FIFO MacMoved event. */
    FM_MAC_SOURCE_TCN_MOVED,

    /** The MA Table entry is the result of an fmAddAddress API call. */
    FM_MAC_SOURCE_API_ADDED,

} fm_macSource;


/* Predicate to determine whether the MA Table Entry source is the TCN FIFO. */
#define FM_IS_MAC_SOURCE_TCN_FIFO(x) \
    ((x) == FM_MAC_SOURCE_TCN_LEARNED || (x) == FM_MAC_SOURCE_TCN_MOVED)


/**************************************************
 * Internal representation of an MA table entry.
 **************************************************/
typedef struct _fm_internalMacAddrEntry
{
    /* Logical port mask. (FM2000 and FM4000 only) */
    fm_uint32  destMask;

    /* Logical port (valid only if destMask is FM_DESTMASK_UNUSED). */
    fm_int     port;

    /* Entry state (INVALID, OLD, YOUNG, LOCKED, etc).
     * See fm_macEntryState for values. */
    fm_int     state;

    /* On FM2000 and FM4000, this is a parity error. On FM6000, this is
     * an unrecoverable ECC error and is read-only. */
    fm_bool    memError;

    /* Type of address entry (FM_MAC_ENTRY_TYPE_CACHE, etc.).
     * See fm_macEntryType for values.
     * At present, this field is only reliable following a call to
     * fmGetAddressInternal. */
    fm_byte    entryType;

    /* Address type specified when the entry was created.
     * See enum _fm_addressTypes for values.
     * (FM10000 and SWAG only) */
    fm_uint16  addrType;

    /* Trigger # to fire for match. (FM2000, FM4000, FM10000) */
    fm_uint32  trigger;

    /* Lower 12 bits of chosen VID. For the FM6000 and FM10000, this
     * is replaced by the FID. */
    fm_uint16  vlanID;

    /* Lower 12 bits of chosen VID2. (FM6000 only) */
    fm_uint16  vlanID2;

    /* MAC Address. */
    fm_macaddr macAddress;

    /* Aging counter, used for soft aging. */
    fm_uint64  agingCounter;

    /* Dirty bits (WAITING, DIRTY, POSTED, RECORDED - FM6000 only) */
    fm_int     dirty;

    /* Remote ID filled with TAG[11:8] | DATA. (FM6000 only) */
    fm_uint16  remoteID;

    /* Flag to let the pipeline know that reaching this MAC will be
     * done using TRILL enabled ports. TAG[7] (FM6000 only) */
    fm_bool    remoteMac;

    /* TAG secure bit. (FM6000, FM10000) */
    fm_bool    secure;

} fm_internalMacAddrEntry;


/* This alias is provided for some legacy regression tests. It should not
 * be used anymore. All references should be to fm_internalMacAddrEntry. */
typedef fm_internalMacAddrEntry fm_internal_mac_addr_entry;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

fm_status fmAllocateAddressTableDataStructures(fm_switch *switchPtr);
fm_status fmInitAddressTable(fm_switch *switchPtr);
fm_status fmFreeAddressTableDataStructures(fm_switch *switchPtr);


/* core helpers */
fm_status fmAddAddressInternal(fm_int              sw,
                               fm_macAddressEntry *entry,
                               fm_uint32           trigger,
                               fm_bool             updateHw,
                               fm_int              bank,
                               fm_uint32*          numUpdates,
                               fm_event**          outEvent);

fm_status fmAddAddressToTable(fm_int              sw,
                              fm_macAddressEntry *entry,
                              fm_uint32           trigger,
                              fm_bool             updateHw,
                              fm_int              bank);

fm_status fmAddAddressToTableInternal(fm_int              sw,
                                      fm_macAddressEntry *entry,
                                      fm_uint32           trigger,
                                      fm_bool             updateHw,
                                      fm_int              bank);

fm_status fmDeleteAddressFromTable(fm_int              sw,
                                   fm_macAddressEntry *entry,
                                   fm_bool             overrideMspt,
                                   fm_bool             updateHw,
                                   fm_int              bank);

fm_int fmCompareInternalMacAddressEntries(const void *key1,
                                          const void *key2);

/* returns the internal entry used by a mac address */
fm_status fmGetAddressInternal(fm_int                    sw,
                               fm_macaddr                address,
                               fm_int                    vlanID,
                               fm_internalMacAddrEntry **entry,
                               fm_uint32 *               addrIndex);

fm_status fmGetAddressIndex(fm_int     sw,
                            fm_macaddr macAddress,
                            fm_int     vlanID,
                            fm_int     vlanID2,
                            fm_int *   index,
                            fm_int *   bank);

fm_text fmAddressTypeToText(fm_int addrType);
fm_text fmEntryStateToText(fm_int entryState);
fm_text fmFlushModeToText(fm_int flushMode);
fm_text fmFlushModeToTextV2(fm_int workType, fm_int flushMode);
fm_text fmMacSourceToText(fm_int srcType);

fm_status fmReadEntryAtIndex(fm_int                   sw,
                             fm_uint32                index,
                             fm_internalMacAddrEntry *entry);

fm_status fmWriteEntryAtIndex(fm_int                   sw,
                              fm_uint32                index,
                              fm_internalMacAddrEntry *entry);

fm_status fmValidateAddressPort(fm_int sw, fm_int logicalPort);

fm_status fmCommonCheckFlushRequest(fm_int          sw,
                                    fm_flushMode    mode,
                                    fm_flushParams *params);

fm_status fmCommonFindAndInvalidateAddr(fm_int     sw,
                                        fm_macaddr macAddress,
                                        fm_uint16  vlanID,
                                        fm_uint16  vlanID2,
                                        fm_int     bank,
                                        fm_uint16 *indexes,
                                        fm_bool    updateHw);

fm_status fmCommonGetAddressTable(fm_int              sw,
                                  fm_int *            nEntries,
                                  fm_macAddressEntry *entries,
                                  fm_int              maxEntries);

fm_status fmCommonAllocAddrTableCache(fm_switch *switchPtr);
fm_status fmCommonFreeAddrTableCache(fm_switch *switchPtr);
fm_status fmCommonDeleteAddressPre(fm_int sw, fm_macAddressEntry *entry);
fm_status fmCommonDeleteAllAddresses(fm_int sw, fm_bool dynamicOnly);


#endif /* __FM_FM_API_ADDR_INT_H */
