/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_mapper.h
 * Creation Date:   July 26, 2010
 * Description:     Structures and functions for dealing with ACLs
 *
 * Copyright (c) 2010 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_MAPPER_H
#define __FM_FM_API_MAPPER_H


/**************************************************/
/** \ingroup typeEnum
 *  Used as an argument to ''fmAddMapperEntry'',
 *  ''fmDeleteMapperEntry'' and ''fmClearMapper''. 
 *  These enumerated values identify the mode of
 *  application for the mapper entry. 
 **************************************************/
typedef enum
{
    /** The change to the mapper entry or entries is applied to the
     *  hardware immediately. */
    FM_MAPPER_ENTRY_MODE_APPLY,
    
    /** The change to the mapper entry or entries will not be applied 
     *  to the hardware until the next time ''fmApplyACL'' is called. */
    FM_MAPPER_ENTRY_MODE_CACHE

} fm_mapperEntryMode;


/**************************************************/
/** \ingroup typeEnum
 *  Used to identify which mapper to operate on in
 *  ''fmAddMapperEntry'', ''fmDeleteMapperEntry'', 
 *  ''fmClearMapper'' and ''fmGetMapperSize''.
 **************************************************/
typedef enum
{
    /** Select the source (ingress) port mapper. When adding or 
     *  deleting a mapper entry, the value must be of type 
     *  ''fm_sourceMapperValue''. */
    FM_MAPPER_SOURCE,

    /** Select the layer 4 protocol mapper. When adding or deleting a mapper
     *  entry, the value must be of type ''fm_protocolMapperValue''. */
    FM_MAPPER_PROTOCOL,

    /** Select the layer 4 source port mapper. When adding or deleting a mapper
     *  entry, the value must be of type ''fm_l4PortMapperValue''. */
    FM_MAPPER_L4_SRC,

    /** Select the layer 4 destination port mapper. When adding or 
     *  deleting a mapper entry, the value must be of type 
     *  ''fm_l4PortMapperValue''. */
    FM_MAPPER_L4_DST,

    /** Select the MAC mapper. When adding or deleting a mapper entry, the
     *  value must be of type ''fm_macMapperValue''. */
    FM_MAPPER_MAC,

    /** Select the ethernet type mapper. When adding or deleting a mapper
     *  entry, the value must be of type ''fm_ethTypeValue''. */
    FM_MAPPER_ETH_TYPE,

    /** Select the IP length mapper. When adding or deleting a mapper
     *  entry, the value must be of type ''fm_ipLengthMapperValue''. */
    FM_MAPPER_IP_LENGTH,

    /** Select the IP address mapper. When adding or deleting a mapper
     *  entry, the value must be of type ''fm_ipAddrMapperValue''. */
    FM_MAPPER_IP_ADDR,

    /** Select the vlan ID mapper. When adding or deleting a mapper
     *  entry, the value must be of type ''fm_vlanMapperValue''. */
    FM_MAPPER_VLAN,

    /* ---- Add new entries above this line. ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_MAPPER_MAX

} fm_mapper;


/**************************************************/
/** \ingroup typeStruct
 *  Mapper value when ''fm_mapper'' type is 
 *  ''FM_MAPPER_SOURCE''. Used as an argument to 
 *  ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_sourceMapperValue
{
    /** Ingress source port. */
    fm_byte sourcePort;

    /** Mapped value of source port (4 bit field on FM4000/FM10000
     *  and 8 bit field on FM6000). */
    fm_byte mappedSourcePortValue;

} fm_sourceMapperValue;


/**************************************************/
/** \ingroup typeStruct
 *  Mapper value when ''fm_mapper'' type is 
 *  ''FM_MAPPER_PROTOCOL''. Used as an argument to 
 *  ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_protocolMapperValue
{
    /** L4 Protocol (IPv4) or Next Header (IPv6).
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_byte protocol;

    /** Mask indicating which protocol bit to match on
     *  
     *  \chips  FM6000 */
    fm_byte protocolMask;

    /** Mapped value of L4 Protocol (IPv4) or Next Header (IPv6). This is a
     *  3 bit field on FM4000/FM10000 and a 4 bit field on FM6000. The 0
     *  value is reserved for other non-mapped protocols.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_byte mappedProtocolValue;

} fm_protocolMapperValue;


/**************************************************/
/** \ingroup typeStruct
 *  Mapper value when ''fm_mapper'' type is 
 *  ''FM_MAPPER_L4_SRC'' or ''FM_MAPPER_L4_DST''.
 *  Used as an argument to ''fmAddMapperEntry''
 *  and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_l4PortMapperValue
{
    /** L4 port that starts the range of ports being mapped.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint16 l4PortStart;

    /** L4 port that ends the range of ports being mapped.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint16 l4PortEnd;

    /** Mapped value of the L4 port range (16 bit field). The
     *  value 0 is reserved and should not be used. On FM6000,
     *  if there is no match, then the mapped value is set to
     *  the original input port value, rather than 0. On FM4000
     *  and FM10000, the mapped value is set to 0 if there is
     *  no match.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint16 mappedL4PortValue;

    /** Mapped value of the protocol associated with this L4 port
     *  range. Use 0 to specify all non-mapped protocols.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_byte mappedProtocol;

    /** Protocol associated with this L4 port range.
     *  
     *  \chips  FM6000 */
    fm_byte protocol;

} fm_l4PortMapperValue;


/**************************************************/
/** \ingroup typeStruct
 * Mapper value when ''fm_mapper'' type is 
 * ''FM_MAPPER_MAC''. Used as an argument to 
 * ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_macMapperValue
{
    /** Ethernet MAC address to match.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_macaddr mac;

    /** Ethernet MAC address mask.
     *  
     *  \chips  FM6000 */
    fm_macaddr macMask;

    /** Indicates this entry is valid for source MAC addresses.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_bool validSrcMac;

    /** Indicates this entry is valid for destination MAC addresses.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_bool validDstMac;

    /** Indicates the number of least significant bits to ignore when
     *  matching.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_byte ignoreLength;

    /** Mapped value of this MAC address. This is a 4 bit field. The 0 value
     *  is reserved for other non-mapped MAC address.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_byte mappedMac;

} fm_macMapperValue;


/**************************************************/
/** \ingroup typeStruct
 * Mapper value when ''fm_mapper'' type is 
 * ''FM_MAPPER_ETH_TYPE''. Used as an argument to 
 * ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_ethTypeValue
{
    /** Ethernet type to match.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_uint16 ethType;

    /** Ethernet type mask.
     *  
     *  \chips  FM6000 */
    fm_uint16 ethTypeMask;

    /** Mapped value of this Ethernet type. This is a 4 bit field. The 0 value
     *  is reserved for other non-mapped Ethernet types.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_byte mappedEthType;

} fm_ethTypeValue;


/**************************************************/
/** \ingroup typeStruct
 * Mapper value when ''fm_mapper'' type is 
 * ''FM_MAPPER_IP_LENGTH''. Used as an argument to 
 * ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_ipLengthMapperValue
{
    /** IP length that starts the range of length being mapped. */
    fm_uint16 ipLengthStart;

    /** IP length that ends the range of length being mapped. */
    fm_uint16 ipLengthEnd;

    /** Mapped value of the IP length range (4 bit field). The
     *  value 0 is reserved and should not be used. */
    fm_byte mappedIpLength;

} fm_ipLengthMapperValue;


/**************************************************/
/** \ingroup typeStruct
 * Mapper value when ''fm_mapper'' type is 
 * ''FM_MAPPER_IP_ADDR''. Used as an argument to 
 * ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_ipAddrMapperValue
{
    /** IP address to match.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_ipAddr ipAddr;

    /** IP address mask.
     *  
     *  \chips  FM6000 */
    fm_ipAddr ipAddrMask;

    /** Indicates if this entry is valid for source IP addresses.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_bool validSrcIp;

    /** Indicates if this entry is valid for destination IP addresses.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_bool validDstIp;

    /** Indicates the number of least significant bits to
     *  ignore when matching.
     *  
     *  \chips  FM3000, FM4000, FM10000 */
    fm_byte ignoreLength;

    /** Mapped value of this IP address. This is a 4 bit field. The 0 value
     *  is reserved for other non-mapped IP address.
     *  
     *  \chips  FM3000, FM4000, FM6000, FM10000 */
    fm_byte mappedIp;

} fm_ipAddrMapperValue;


/**************************************************/
/** \ingroup typeStruct
 *  Mapper value when ''fm_mapper'' type is 
 *  ''FM_MAPPER_VLAN''. Used as an argument to 
 *  ''fmAddMapperEntry'' and ''fmDeleteMapperEntry''.
 **************************************************/
typedef struct _fm_vlanMapperValue
{
    /** VLAN ID. */
    fm_uint16 vlanId;

    /** Mapped value of VLAN ID (12 bit field). */
    fm_uint16 mappedVlanId;

} fm_vlanMapperValue;


fm_status fmGetMapperKeyAndSize(fm_int     sw,
                                fm_mapper  mapper,
                                void *     value,
                                fm_uint64 *key,
                                fm_int    *size);

fm_status fmAddMapperEntry(fm_int             sw,
                           fm_mapper          mapper,
                           void *             value,
                           fm_mapperEntryMode mode);
fm_status fmDeleteMapperEntry(fm_int             sw,
                              fm_mapper          mapper,
                              void *             value,
                              fm_mapperEntryMode mode);
fm_status fmClearMapper(fm_int             sw,
                        fm_mapper          mapper,
                        fm_mapperEntryMode mode);
fm_status fmGetMapper(fm_int      sw,
                      fm_mapper   mapper,
                      fm_int *    nEntries,
                      void *      entries,
                      fm_int      maxEntries);
fm_status fmGetMapperSize(fm_int     sw,
                          fm_mapper  mapper,
                          fm_uint32 *mapperSize);

#endif /* __FM_FM_API_MAPPER_H */
