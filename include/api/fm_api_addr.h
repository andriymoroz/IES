/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_addr.h
 * Creation Date:   May 2, 2005
 * Description:     Structures and functions for dealing with MA table
 *                  configuration
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_ADDR_H
#define __FM_FM_API_ADDR_H


/****************************************************************************/
/** \ingroup constSystem
 *
 * Used to indicate when a destmask field is not used and a corresponding
 * logical port field is used, for example, in the ''fm_eventTableUpdate''
 * structure.
 ****************************************************************************/
#define FM_DESTMASK_UNUSED  0xffffffffL


/****************************************************************************/
/** \ingroup constSystem
 *
 *  Used to indicate when the port on which a MAC address is learned is not
 *  specified, used for example in the ''fm_eventTableUpdate'' structure.
 *  The port is unspecified in the AGE event for a station move on FM6000
 *  devices.
 ****************************************************************************/
#define FM_MAC_ADDR_PORT_UNSPECIFIED  -1


/****************************************************************************/
/* Other macros
 ****************************************************************************/

#define fmIsMulticastMacAddress(addr)   \
        ( ( (addr) & FM_LITERAL_64(0x010000000000) ) != 0 )

/*
 * Evaluates to TRUE if the argument has a single bit set. 
 * 'x' must be unsigned and non-zero. 
 */
#define FM_IS_ONEHOT_BITMASK(x)     ( ( (x) & ((x) - 1) ) == 0 )


/**************************************************/
/** \ingroup typeEnum
 *  Specifies the type of MA Table flush operation
 *  to be performed. Used as an argument to 
 *  ''fmFlushAddresses''.
 **************************************************/
typedef enum
{
    /** Delete all dynamic addresses from the MA Table.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_FLUSH_MODE_ALL_DYNAMIC = 0,

    /** Delete all addresses from the MA Table associated with the specified 
     *  port.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_FLUSH_MODE_PORT,

    /** Delete all addresses from the MA Table associated with the specified 
     *  port and VLAN (VLAN1).
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_FLUSH_MODE_PORT_VLAN,

    /** Delete all addresses from the MA Table associated with the specified 
     *  VLAN (VLAN1).
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_FLUSH_MODE_VLAN,

    /** Delete all addresses from the MA Table associated with the specified 
     *  VLAN1 and VLAN2.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_VID1_VID2,

    /** Delete all addresses from the MA Table associated with the specified 
     *  port, VLAN1 and VLAN2.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_PORT_VID1_VID2,

    /** Delete all addresses from the MA Table associated with the specified 
     *  port and VLAN2.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_PORT_VID2,

    /** Delete all addresses from the MA Table associated with the specified 
     *  VLAN2.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_VID2,

    /** Delete all addresses from the MA Table associated with the specified 
     *  port, VLAN1 and remoteID.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_PORT_VID1_REMOTEID,

    /** Delete all addresses from the MA Table associated with the specified 
     *  VLAN1 and remoteID.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_VID1_REMOTEID,

    /** Delete all addresses from the MA Table associated with the specified 
     *  remoteID.
     *
     *  \chips  FM6000 */
    FM_FLUSH_MODE_REMOTEID,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_FLUSH_MODE_MAX,

} fm_flushMode;


/****************************************************************************/
/** \ingroup constAddrTypes
 *  MA Table Entry Types, as used in the ''fm_macAddressEntry'' data 
 *  structure.
 ****************************************************************************/
enum _fm_addressTypes
{
    /** MA Table entry is static and will not be subject to aging. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ADDRESS_STATIC = 0,

    /** MA Table entry is dynamic and will be subject to aging. 
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    FM_ADDRESS_DYNAMIC,

    /**
     *  Secure static MA Table entry. Frames received with an SMAC matching
     *  this entry, but on a different port (FM10000) or on an unsecure port
     *  (FM3000, FM4000), will be treated as a security violation. This
     *  entry will not be subject to aging.
     *                                                                  \lb\lb
     *  On FM10000 devices, security violations will be subject to the
     *  action specified by the ''FM_MAC_TABLE_SECURITY_ACTION'' attribute. 
     *
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    FM_ADDRESS_SECURE_STATIC, 

    /**
     *  Secure dynamic MA Table entry. Frames received with an SMAC matching
     *  this entry, but on a different port (FM10000) or on an unsecure port
     *  (FM3000, FM4000), will be treated as a security violation. This
     *  entry will be subject to aging.
     *                                                                  \lb\lb
     *  On FM10000 devices, security violations will be subject to the
     *  action specified by the ''FM_MAC_TABLE_SECURITY_ACTION'' attribute. 
     *
     *  \chips  FM3000, FM4000, FM10000 */
    FM_ADDRESS_SECURE_DYNAMIC,

    /** MA Table entry is provisional and will be subject to aging.
     *
     *  \chips  FM6000 */
    FM_ADDRESS_PROVISIONAL,

};  /* end enum _fm_addressTypes */


/**************************************************/
/** \ingroup typeStruct
 *  Represents an entry in the MAC Address Table.
 *  Used as an argument to ''fmAddAddress'', 
 *  ''fmDeleteAddress'', ''fmGetAddress'' and others.
 **************************************************/
typedef struct _fm_macAddressEntry
{
    /** The MAC address.
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_macaddr macAddress;

    /** VLAN ID (VLAN1).
     *  
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_uint16  vlanID;

    /** VLAN ID2 (VLAN2). Set this field to zero, and the VLAN attribute
     *  ''FM_VLAN_FID2_IVL'' to FM_DISABLED (the default), to prevent FID2
     *  from being considered during the MAC address table lookups.
     *  
     *  \chips  FM6000 */
    fm_uint16  vlanID2;

    /** See 'MA Table Entry Types'.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_uint16  type;

    /** Mask of port numbers to send to when destination is macAddress.
     *  If set to ''FM_DESTMASK_UNUSED'', then port is used, otherwise the 
     *  specified mask is used. This field is deprecated in favor of the
     *  port field, and may not be used for switch aggregates.
     *                                                                  \lb\lb
     *  On FM3000 and FM4000 devices, when adding a dynamic MA Table entry 
     *  for more than one destination port, destMask must be set to 
     *  ''FM_DESTMASK_UNUSED'' and port used to specify a multicast group.
     *                                                                  \lb\lb
     *  This field is not supported on FM6000 devices and should always be
     *  set to ''FM_DESTMASK_UNUSED'' for clarity. If any other value is
     *  used, it will be ignored.
     *                                                                  \lb\lb
     *  On FM10000 devices, this field must be set to FM_DESTMASK_UNUSED.
     *  
     *  \chips  FM2000, FM3000, FM4000 */
    fm_uint32  destMask;

    /** Destination logical port. On FM2000, FM3000, and FM4000 devices,
     *  this field is only used when destMask is ''FM_DESTMASK_UNUSED''.
     *  On FM6000 and FM10000 devices, this field is used unconditionally.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_int     port;

    /** Age of entry. Will be non-zero for a new (young) entry, and zero for
     *  an entry that is ready to age out. Note that this is an output field
     *  only (i.e., it is returned by fmGetAddress and ignored by
     *  fmAddAddress).
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_int     age;

    /** Remote ID. Depending of the application, this field could
     *  represent a VLAN ID stored in the MAC entry that can be used
     *  in an egress VLAN TAG when a DMAC lookup occurs. Used as a
     *  tunnel ID for TRILL, VxLAN, GRE, etc.
     *  
     *  \chips  FM6000 */
    fm_uint16  remoteID;

    /** Flag to let the pipeline know that reaching this MAC will be done
     *  using TRILL enabled ports.
     *  
     *  \chips  FM6000 */
    fm_bool    remoteMac;

} fm_macAddressEntry;


/**************************************************/
/** \ingroup typeStruct
 *  Specifies the parameters of an MA Table flush
 *  operation. Used by ''fmFlushAddresses''.
 **************************************************/
typedef struct _fm_flushParams
{
    /** The port associated with each MA Table entry to be flushed.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_uint    port;

    /** The VLAN (VLAN1) associated with each MA Table entry to be flushed.
     *
     *  \chips  FM2000, FM3000, FM4000, FM6000, FM10000 */
    fm_uint16  vid1;

    /** The VLAN2 associated with each MA Table entry to be flushed.
     *
     *  \chips  FM6000 */
    fm_uint16  vid2;

    /** The remoteID associated with each MA Table entry to be
     *  flushed.
     *
     *  \chips  FM6000 */
    fm_uint16  remoteId;

    /** The remoteMac associated with each remoteId to be flushed.
     *
     *  \chips  FM6000 */
    fm_bool  remoteMac;

} fm_flushParams;


/**************************************************/
/** \ingroup typeStruct
 *  MAC security statistics. Used by
 *  ''fmGetSecurityStats''. 
 **************************************************/
typedef struct _fm_securityStats
{

    /* ----- Unknown SMAC counters ----- */

    /** The number of frames with unknown SMACs received on ports whose
     *  security action is ''FM_PORT_SECURITY_ACTION_DROP''. */
    fm_uint64   cntUnknownSmacDropPkts;

    /** The number of frames with unknown SMACs received on ports whose
     *  security action is ''FM_PORT_SECURITY_ACTION_EVENT''. */
    fm_uint64   cntUnknownSmacEventPkts;

    /** The number of frames with unknown SMACs received on ports whose
     *  security action is ''FM_PORT_SECURITY_ACTION_TRAP''. */
    fm_uint64   cntUnknownSmacTrapPkts;

    /* ----- Non-Secure SMAC counters ----- */

    /** The number of frames with non-secure SMACs moving to a secure
     *  port whose security action is ''FM_PORT_SECURITY_ACTION_DROP''. */
    fm_uint64   cntNonSecureSmacDropPkts;

    /** The number of frames with non-secure SMACs moving to a secure
     *  port whose security action is ''FM_PORT_SECURITY_ACTION_EVENT''. */
    fm_uint64   cntNonSecureSmacEventPkts;

    /** The number of frames with non-secure SMACs moving to a secure
     *  port whose security action is ''FM_PORT_SECURITY_ACTION_TRAP''. */
    fm_uint64   cntNonSecureSmacTrapPkts;

    /* ----- Secure SMAC counters ----- */

    /** The number of frames with secure SMACs moving to any port when
     *  the security action is ''FM_MAC_SECURITY_ACTION_EVENT''. */
    fm_uint64   cntSecureSmacEventPkts;

    /** The number of frames with secure SMACs moving to any port when
     *  the security action is ''FM_MAC_SECURITY_ACTION_TRAP''. */
    fm_uint64   cntSecureSmacTrapPkts;

} fm_securityStats;


/*****************************************************************************
 * Function prototypes.
 *****************************************************************************/

/* adds a new entry to the MA table */
fm_status fmAddAddress(fm_int sw, fm_macAddressEntry *entry);

fm_status fmGetAddress(fm_int              sw,
                       fm_macaddr          address,
                       fm_int              vlanID,
                       fm_macAddressEntry *entry);

fm_status fmGetAddressV2(fm_int              sw,
                         fm_macaddr          address,
                         fm_int              vlanID,
                         fm_int              vlanID2,
                         fm_macAddressEntry *entry);

fm_status fmGetAddressTableExt(fm_int              sw,
                               fm_int *            nEntries,
                               fm_macAddressEntry *entries,
                               fm_int              maxEntries);


/* deletes an entry from the MA table */
fm_status fmDeleteAddress(fm_int sw, fm_macAddressEntry *entry);

fm_status fmGetAddressTable(fm_int              sw,
                            fm_int *            nEntries,
                            fm_macAddressEntry *entries);

fm_status fmDeleteAllAddresses(fm_int sw);
fm_status fmDeleteAllAddressesInternal(fm_int sw);

fm_status fmDeleteAllDynamicAddresses(fm_int sw);
fm_status fmFlushAllDynamicAddresses(fm_int sw);
fm_status fmFlushPortAddresses(fm_int sw, fm_uint port);
fm_status fmFlushVlanAddresses(fm_int sw, fm_uint vlan);
fm_status fmFlushPortVlanAddresses(fm_int sw, fm_uint port, fm_int vlan);
fm_status fmFlushAddresses(fm_int sw, fm_flushMode mode, fm_flushParams params);

fm_status fmGetAddressTableAttribute(fm_int sw, fm_int attr, void *value);
fm_status fmSetAddressTableAttribute(fm_int sw, fm_int attr, void *value);

fm_status fmGetSecurityStats(fm_int sw, fm_securityStats * stats);
fm_status fmResetSecurityStats(fm_int sw);

fm_status fmDbgDumpMACTablePurgeStats(fm_int sw);
fm_status fmDbgResetMACTablePurgeStats(fm_int sw);


#endif /* __FM_FM_API_ADDR_H */
