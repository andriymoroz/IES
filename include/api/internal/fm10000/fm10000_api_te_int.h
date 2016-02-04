/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_te_int.h
 * Creation Date:  November 28, 2013
 * Description:    Low-level API for manipulating the Tunneling Engine.
 *
 * Copyright (c) 2013 - 2016, Intel Corporation
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

#ifndef __FM_FM10000_FM_API_TE_INT_H
#define __FM_FM10000_FM_API_TE_INT_H


/* Size of the NGE Data array. */
#define FM10000_TE_NGE_DATA_SIZE                16

/* Size of GPE header in 4-byte words */
#define FM10000_TE_GPE_HDR_SIZE                 2

/* Size of NSH header in 4-byte words */
#define FM10000_TE_NSH_HDR_SIZE                 2

/* Size available for NSH data in 4-byte words */
#define FM10000_TE_NSH_DATA_SIZE                (FM10000_TE_NGE_DATA_SIZE -\
                                                 FM10000_TE_GPE_HDR_SIZE  -\
                                                 FM10000_TE_NSH_HDR_SIZE)
#define FM10000_TE_NSH_DATA_MASK                 ((1 << FM10000_TE_NSH_DATA_SIZE) - 1)

/* Limit the number of teData entry per bin on hash lookup */
#define FM10000_TE_MAX_DATA_BIN_SIZE            512

/* The number of tunnel engines */
#define FM10000_NUM_TE                          FM10000_TE_DGLORT_MAP_ENTRIES_1



/* The bit mask and position where GPE and NSH words are placed in
 * the NGE data array. */
#define FM10000_NGE_MASK_GPE_FLAGS_NEXT_PROT    (1 << 0)
#define FM10000_NGE_MASK_GPE_VNI                (1 << 1)
#define FM10000_NGE_MASK_NSH_BASE_HDR           (1 << 2)
#define FM10000_NGE_MASK_NSH_SERVICE_HDR        (1 << 3)

#define FM10000_NGE_POS_GPE_FLAGS_NEXT_PROT     0
#define FM10000_NGE_POS_GPE_VNI                 1
#define FM10000_NGE_POS_NSH_BASE_HDR            2
#define FM10000_NGE_POS_NSH_SERVICE_HDR         3
#define FM10000_NGE_POS_NSH_DATA                4


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Key Selectors.
 *  
 *  Bit masks that specify which field(s) are included
 *  in the hash or search.
 **************************************************/
typedef enum
{
    /** Whether VSI (encap) or TEP-ID (decap) is used for hash or search. */
    FM10000_TE_KEY_VSI_TEP = (1 << 0),

    /** Whether VNI (decap) is used for hash or search. */
    FM10000_TE_KEY_VNI = (1 << 1),

    /** Whether Destination MAC is used for hash or search. */
    FM10000_TE_KEY_DMAC = (1 << 2),

    /** Whether Source MAC is used for hash or search. */
    FM10000_TE_KEY_SMAC = (1 << 3),

    /** Whether Vlan ID is used for hash or search. */
    FM10000_TE_KEY_VLAN = (1 << 4),

    /** Whether the hash or search key is IPv6. */
    FM10000_TE_KEY_IPV6 = (1 << 5),

    /** Whether Destination IP is used for hash or search. */
    FM10000_TE_KEY_DIP = (1 << 6),

    /** Whether Source IP is used for hash or search. */
    FM10000_TE_KEY_SIP = (1 << 7),

    /** Whether L4 Source Port is used for hash or search. */
    FM10000_TE_KEY_L4SRC = (1 << 8),

    /** Whether L4 Destination Port is used for hash or search. */
    FM10000_TE_KEY_L4DST = (1 << 9),

    /** Whether Protocol is used for hash or search. */
    FM10000_TE_KEY_PROT = (1 << 10),

    /** Whether Protocol == UDP is a match. Only used on search key. */
    FM10000_TE_KEY_UDP = (1 << 14),

    /** Whether Protocol == TCP is a match. Only used on search key. */
    FM10000_TE_KEY_TCP = (1 << 15),

} fm_fm10000TeKeySel;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_fm10000TeDGlort'' to specify the lookup type.
 **************************************************/
typedef enum
{
    /** Index is extracted from {DGLORT,USER}. */
    FM_FM10000_TE_LOOKUP_DIRECT = 0,

    /** Index is computed based on an hash key from frame header fields. */
    FM_FM10000_TE_LOOKUP_HASH,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TE_LOOKUP_MAX

} fm_fm10000TeLookupType;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeLookupData'', 
 *  specifies the lookup value to be written.
 **************************************************/
typedef struct _fm_fm10000TeHashLookup
{
    /** Defines the keys included in the search:
     *  ''FM10000_TE_KEY_VSI_TEP''             \lb
     *  ''FM10000_TE_KEY_VNI''                 \lb
     *  ''FM10000_TE_KEY_DMAC''                \lb
     *  ''FM10000_TE_KEY_SMAC''                \lb
     *  ''FM10000_TE_KEY_VLAN''                \lb
     *  ''FM10000_TE_KEY_IPV6''                \lb
     *  ''FM10000_TE_KEY_DIP''                 \lb
     *  ''FM10000_TE_KEY_SIP''                 \lb
     *  ''FM10000_TE_KEY_L4SRC''               \lb
     *  ''FM10000_TE_KEY_L4DST''               \lb
     *  ''FM10000_TE_KEY_PROT''                 */
    fm_uint16 hashKeyConfig;

    /** Defines the hash table size. Valid sizes are any integer between
     *  2 and 32768 bins and it must be a power of 2. */
    fm_uint16 hashSize;

    /** Location of the TEP-ID field in the {DGLORT[15:0],USER[7:0]} set,
     *  only used for Decap. */
    fm_byte   tepStart;

    /** Width of the TEP-ID field in the {DGLORT[15:0],USER[7:0]} set,
     *  only used for Decap. */
    fm_byte   tepWidth;

} fm_fm10000TeHashLookup;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeLookupData'', 
 *  specifies the lookup value to be written.
 **************************************************/
typedef struct _fm_fm10000TeDirectLookup
{
    /** Location of the embedded index in {DGLORT[15:0],USER[7:0]} set.
     *  This index will be used to recover the flow-data. */
    fm_byte indexStart;

    /** Width of the embedded index in {DGLORT[15:0],USER[7:0]} set.
     *  This index will be used to recover the flow-data. */
    fm_byte indexWidth;

} fm_fm10000TeDirectLookup;


/**************************************************/
/** \ingroup typeStruct
 *  Specifies the data for a TE lookup,  
 *  specific to the lookup type 
 *  (''fm_fm10000TeLookupType'').
 *  Referenced by ''fm_fm10000TeDGlort''.
 **************************************************/
typedef union _fm_fm10000TeLookupData
{
    /** Hash action data for the ''FM_FM10000_TE_LOOKUP_HASH'' TE action type. */
    fm_fm10000TeHashLookup   hashLookup;

    /** Indexing data for the ''FM_FM10000_TE_LOOKUP_DIRECT'' TE action type. */
    fm_fm10000TeDirectLookup directLookup;

} fm_fm10000TeLookupData;


/**************************************************/
/** \ingroup typeStruct
 *  Specifies the destination GLORT configuration
 *  for a given range. Referenced by
 *  fm10000SetTeDGlort and fm10000GetTeDGlort.
 **************************************************/
typedef struct _fm_fm10000TeDGlort
{
    /** The destination GLORT value to match on. */
    fm_uint16 glortValue;

    /** The destination GLORT mask to match on. Setting mask to 0x0 and
     *  value to other than 0x0 will never match. */
    fm_uint16 glortMask;

    /** The USER field value to match on. */
    fm_byte   userValue;

    /** The USER field mask to match on. */
    fm_byte   userMask;

    /** The base of the lookup table for this DGLORT decoder in the
     *  TE_LOOKUP table. */
    fm_uint16 baseLookup;

    /** Whether the change is Encap/NAT (TRUE) or Decap (FALSE). */
    fm_bool   encap;

    /** Whether SGLORT is set to TE_DEFAULT_SGLORT (TRUE) or passthrough
     *  (FALSE). Only applicable if flowData doesn't define an SGLORT to
     *  use. */
    fm_bool   setSGlort;

    /** Whether DGLORT is set to TE_DEFAULT_DGLORT (TRUE) or passthrough
     *  (FALSE). Only applicable if flowData doesn't define a DGLORT to
     *  use. */
    fm_bool   setDGlort;

    /** Whether the index is extracted from {DGLORT,USER} or a hash key from
     *  frame header fields. */
    fm_fm10000TeLookupType lookupType;

    /** Exact lookup position information. */
    fm_fm10000TeLookupData lookupData;

} fm_fm10000TeDGlort;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeSGlort'' and 
 *  ''fm10000GetTeSGlort'' and specifies the 
 *  source GLORT configuration for a specific
 *  range.
 **************************************************/
typedef struct _fm_fm10000TeSGlort
{
    /** The source GLORT value to match on. */
    fm_uint16 glortValue;

    /** The source GLORT mask to match on. Setting mask to 0x0 and
     *  value to other than 0x0 will never match. */
    fm_uint16 glortMask;

    /** Location of first bit of VSI in SGLORT. */
    fm_byte   vsiStart;

    /** Length of VSI in SGLORT. Value of 0 means VSI=0. */
    fm_byte   vsiLength;

    /** The offset to add to the VSI extracted from SGLORT. */
    fm_byte   vsiOffset;

} fm_fm10000TeSGlort;


/**************************************************/
/** \ingroup typeEnum
 *  Default GLORT Tunnel Engine Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeGlortCfg'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write Encap default DGLORT field. */
    FM10000_TE_DEFAULT_GLORT_ENCAP_DGLORT = (1 << 0),

    /** Write Decap default DGLORT field. */
    FM10000_TE_DEFAULT_GLORT_DECAP_DGLORT = (1 << 1),

    /** Write Encap default SGLORT field. */
    FM10000_TE_DEFAULT_GLORT_ENCAP_SGLORT = (1 << 2),

    /** Write Decap default SGLORT field. */
    FM10000_TE_DEFAULT_GLORT_DECAP_SGLORT = (1 << 3),

    /** Write all fields. */
    FM10000_TE_DEFAULT_GLORT_ALL = (FM10000_TE_DEFAULT_GLORT_ENCAP_DGLORT |
                                    FM10000_TE_DEFAULT_GLORT_DECAP_DGLORT |
                                    FM10000_TE_DEFAULT_GLORT_ENCAP_SGLORT |
                                    FM10000_TE_DEFAULT_GLORT_DECAP_SGLORT)

} fm_fm10000TeDefGlortSel;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeDefaultGlort'' and 
 *  ''fm10000GetTeDefaultGlort'' and specifies the 
 *  default GLORT configuration if not passthrough.
 **************************************************/
typedef struct _fm_fm10000TeGlortCfg
{
    /** DGLORT to use during Encap if not passthrough. */
    fm_uint16 encapDglort;

    /** DGLORT to use during Decap if not passthrough. */
    fm_uint16 decapDglort;

    /** SGLORT to use during Encap if not passthrough. */
    fm_uint16 encapSglort;

    /** SGLORT to use during Decap if not passthrough. */
    fm_uint16 decapSglort;

} fm_fm10000TeGlortCfg;


/**************************************************/
/** \ingroup typeEnum
 *  Default TEP Tunnel Engine Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeTepCfg'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write default Source IP field. */
    FM10000_TE_DEFAULT_TEP_SIP = (1 << 0),

    /** Write default VNI/TNI field. */
    FM10000_TE_DEFAULT_TEP_VNI = (1 << 1),

    /** Write all fields. */
    FM10000_TE_DEFAULT_TEP_ALL = (FM10000_TE_DEFAULT_TEP_SIP |
                                  FM10000_TE_DEFAULT_TEP_VNI)

} fm_fm10000TeDefTepSel;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeDefaultTep'' and 
 *  ''fm10000GetTeDefaultTep'' and specifies the 
 *  default Tunnel End Point (TEP) configuration.
 **************************************************/
typedef struct _fm_fm10000TeTepCfg
{
    /** Default Source IP address for each TEP. */
    fm_ipAddr srcIpAddr;

    /** Default VNI/TNI for each TEP. */
    fm_uint32 vni;

} fm_fm10000TeTepCfg;


/**************************************************/
/** \ingroup typeEnum
 *  Default Tunnel Engine Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeTunnelCfg'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write default destination port for VXLAN. */
    FM10000_TE_DEFAULT_TUNNEL_L4DST_VXLAN = (1 << 0),

    /** Write default destination port for NGE. */
    FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE = (1 << 1),

    /** Write default TTL value for tunnels during encapsulation. */
    FM10000_TE_DEFAULT_TUNNEL_TTL = (1 << 2),

    /** Write default TOS value for tunnels during encapsulation. */
    FM10000_TE_DEFAULT_TUNNEL_TOS = (1 << 3),

    /** Write default NGE Data words or GPE/NSH data words if
     *  TE mode is FM10000_TE_MODE_VXLAN_NVGRE_NGE. */
    FM10000_TE_DEFAULT_TUNNEL_NGE_DATA = (1 << 4),

    /** Write default NGE valid words or the GPE/NSH valid words
     *  if the TE mode is FM10000_TE_MODE_VXLAN_NVGRE_NGE. */
    FM10000_TE_DEFAULT_TUNNEL_NGE_MASK = (1 << 5),

    /** Write default NGE Time Tag behavior. */
    FM10000_TE_DEFAULT_TUNNEL_NGE_TIME = (1 << 6),

    /** Write default destination MAC to use when adding a tunnel. */
    FM10000_TE_DEFAULT_TUNNEL_DMAC = (1 << 7),

    /** Write default source MAC to use when adding a tunnel. */
    FM10000_TE_DEFAULT_TUNNEL_SMAC = (1 << 8),

    /** If TE mode is FM10000_TE_MODE_VXLAN_NVGRE_NGE, write default
     *  NVGRE/NGE protocol to use when adding a tunnel. If TE mode
     *  is FM10000_TE_MODE_VXLAN_GPE_NSH, define which NextProtocol
     *  that should be used on decap of GPE frames. */ 
    FM10000_TE_DEFAULT_TUNNEL_PROTOCOL = (1 << 9),

    /** If TE mode is FM10000_TE_MODE_VXLAN_NVGRE_NGE, write default
     *  NVGRE/NGE version to use when adding a tunnel. If TE mode
     *  is FM10000_TE_MODE_VXLAN_GPE_NSH, write the version that
     *  should be checked for in the GPE and NSH headers. */
    FM10000_TE_DEFAULT_TUNNEL_VERSION = (1 << 10),

    /** Write default tunnel operation mode */
    FM10000_TE_DEFAULT_TUNNEL_MODE = (1 << 11),

    /** Clear all the GPE/NSH options that may have been set
     *  previously. This should generally be done when changing
     *  the tunnel mode to FM10000_TE_MODE_VXLAN_GPE_NSH. */
    FM10000_TE_DEFAULT_GPE_NSH_CLEAR = (1 << 12),

    /** Defines the GPE header's next protocol field for this
     *  tunnel. */
    FM10000_TE_DEFAULT_GPE_NEXT_PROT = (1 << 13),

    /** Defines the GPE header's VNI field for this tunnel.  */
    FM10000_TE_DEFAULT_GPE_VNI = (1 << 14),

    /** Defines the NSH Base Header fields (Critical Flag, Length
     *  and MD Type) for this tunnel. */
    FM10000_TE_DEFAULT_NSH_BASE_HDR = (1 << 15),

    /** Defines the NSH Service Header fields for this tunnel. */
    FM10000_TE_DEFAULT_NSH_SERVICE_HDR = (1 << 16),

    /** Defines the NSH header's data bytes field for this
     *  tunnel. */
    FM10000_TE_DEFAULT_NSH_DATA = (1 << 17),

    /** Write all fields. (Except for
     *  FM10000_TE_DEFAULT_TUNNEL_MODE and GPE/NSH fields) */
    FM10000_TE_DEFAULT_TUNNEL_ALL = (FM10000_TE_DEFAULT_TUNNEL_L4DST_VXLAN |
                                     FM10000_TE_DEFAULT_TUNNEL_L4DST_NGE   |
                                     FM10000_TE_DEFAULT_TUNNEL_TTL         |
                                     FM10000_TE_DEFAULT_TUNNEL_TOS         |
                                     FM10000_TE_DEFAULT_TUNNEL_DMAC        |
                                     FM10000_TE_DEFAULT_TUNNEL_SMAC        |
                                     FM10000_TE_DEFAULT_TUNNEL_PROTOCOL    |
                                     FM10000_TE_DEFAULT_TUNNEL_VERSION),

    /** Write all NGE fields. */
    FM10000_TE_DEFAULT_TUNNEL_NGE_ALL    = (FM10000_TE_DEFAULT_TUNNEL_NGE_DATA | 
                                            FM10000_TE_DEFAULT_TUNNEL_NGE_MASK | 
                                            FM10000_TE_DEFAULT_TUNNEL_NGE_TIME),

    /** Write all GPE/NSH fields. */
    FM10000_TE_DEFAULT_TUNNEL_GPE_ALL = (FM10000_TE_DEFAULT_GPE_NSH_CLEAR |
                                         FM10000_TE_DEFAULT_GPE_NEXT_PROT |
                                         FM10000_TE_DEFAULT_GPE_VNI),

    /** Write all NSH fields. */
    FM10000_TE_DEFAULT_TUNNEL_NSH_ALL = (FM10000_TE_DEFAULT_NSH_BASE_HDR    |
                                         FM10000_TE_DEFAULT_NSH_SERVICE_HDR |
                                         FM10000_TE_DEFAULT_NSH_DATA),

    /** Write all GPE/NSH fields. */
    FM10000_TE_DEFAULT_TUNNEL_GPE_NSH_ALL = (FM10000_TE_DEFAULT_TUNNEL_GPE_ALL |
                                             FM10000_TE_DEFAULT_TUNNEL_NSH_ALL),

} fm_fm10000TeDefTunnelSel;

/**************************************************/
/** \ingroup typeEnum
 *  
 *  Tunnel Engine Mode
 *  
 *  Bit masks that specify which field(s) are included
 *  in the hash or search.
 **************************************************/
typedef enum
{
    /** Support the VXLAN, NVGRE and NGE protocols. */
    FM10000_TE_MODE_VXLAN_NVGRE_NGE = 0,

    /** Support the VXLAN, GPE and NSH protocols. */
    FM10000_TE_MODE_VXLAN_GPE_NSH,

} fm_fm10000TeMode;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeDefaultTunnel'' and 
 *  ''fm10000GetTeDefaultTunnel'' and specifies the 
 *  default Tunnel configuration.
 **************************************************/
typedef struct _fm_fm10000TeTunnelCfg
{
    /** Default destination port for VXLAN. */
    fm_uint16  l4DstVxLan;

    /** Default destination port for NGE. */
    fm_uint16  l4DstNge;

    /** Default TTL for tunnels during encapsulation if none specified
     *  with tunnel parameters. If this field is set to 0, then the outer IP
     *  header will use the inner TTL received. */
    fm_byte    ttl;

    /** Default TOS for tunnels during encapsulation, if none specified
     *  with tunnel parameters and deriveOuterTOS is off. */
    fm_byte    tos;

    /** How the TOS is set during encapsulation if none specified with
     *  tunnel parameters. Choices are:                                 \lb\lb
     *  FALSE: use the default tos field.                               \lb
     *  TRUE:  copy over the InnerTOS from packet. */
    fm_bool    deriveOuterTOS;

    /** NGE Data words. */
    fm_uint32  ngeData[FM10000_TE_NGE_DATA_SIZE];

    /** Which words are valid in ngeData[] */
    fm_uint16  ngeMask;

    /** Whether the timetag should be loaded into words[14..15] (TRUE). */
    fm_bool    ngeTime;

    /** Default destination MAC to use when adding a tunnel. */
    fm_macaddr dmac;

    /** Default source MAC to use when adding a tunnel. */
    fm_macaddr smac;

    /** The protocol to use in the NVGRE and NGE headers when 
     *  encapsulating. Can also be used to check protocol on decap.
     *  
     *  When the TE mode is FM10000_TE_MODE_VXLAN_GPE_NSH, this
     *  field defines the decap "NextProtocol" supported in the GPE
     *  header. The L2 Ethernet Protocol should be stored in
     *  encapProtocol[7:0] while the NSH Protocol should be stored
     *  in encapProtocol[15:8]. The typical value is 0x0403. */
    fm_uint16  encapProtocol;

    /** The version to use in the NVGRE and NGE headers when
     *  encapsulating. Can also be used to check version on decap.
     *  
     *  When the TE mode is FM10000_TE_MODE_VXLAN_GPE_NSH, this
     *  field is used to check the version of GPE and NSH headers
     *  against this value if fm_fm10000TeParserCfg.checkVersion is
     *  enabled */
    fm_byte    encapVersion;

    /** The mode controls which set of protocols the TE should
     *  operate with.
     *  
     *  Note: that FM10000_TE_MODE_VXLAN_GPE_NSH is only supported
     *  in the B0 silicon revision.  */
    fm_fm10000TeMode mode;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_GPE_NEXT_PROT'' is set. This is the VNI
     *  that will be stored in the VXLAN-GPE header. */
    fm_uint32  gpeNextProt;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_GPE_VNI'' is set. This is the VNI
     *  that will be stored in the VXLAN-GPE header. */
    fm_uint32  gpeVni;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_BASE_HDR'' is set. This is length
     *  of the NSH header in 4-byte words including the Base Header,
     *  Service Path Header and Context Data.  */
    fm_byte    nshLength;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_BASE_HDR'' is set. This bit
     *  should be set if there are critical TLV that are included in
     *  the NSH data. */
    fm_bool    nshCritical;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_BASE_HDR'' is set. This field
     *  should contain the MD Type. */
    fm_byte    nshMdType;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_SERVICE_HDR'' is set. This field
     *  should contain the Service Path ID. */
    fm_uint32  nshSvcPathId;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_SERVICE_HDR'' is set. This field
     *  should contain the Service Index. */
    fm_byte    nshSvcIndex;

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_DATA'' is set. This field should
     *  contain the NSH context data that follows the service
     *  header. */
    fm_uint32  nshData[FM_TUNNEL_NSH_DATA_SIZE];

    /** Must be set to the proper value if
     *  ''FM10000_TE_DEFAULT_NSH_DATA'' is set. This field is a
     *  bitmask of the valid 32-bit words in nshData. */
    fm_uint16  nshDataMask;

} fm_fm10000TeTunnelCfg;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Checksum Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeChecksumCfg'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write the Encap checksum options for non IP packet. */
    FM10000_TE_CHECKSUM_NOT_IP = (1 << 0),

    /** Write the Encap checksum options for IP packet that are not TCP or UDP. */
    FM10000_TE_CHECKSUM_NOT_TCP_OR_UDP = (1 << 1),

    /** Write the Encap checksum options for IP packet that are TCP or UDP. */
    FM10000_TE_CHECKSUM_TCP_OR_UDP = (1 << 2),

    /** Write the Decap checksum validation options. */
    FM10000_TE_CHECKSUM_DECAP_VALID = (1 << 3),

    /** Write the Decap Update checksum options. */
    FM10000_TE_CHECKSUM_DECAP_UPDATE = (1 << 4),

    /** Write all fields. */
    FM10000_TE_CHECKSUM_ALL = (FM10000_TE_CHECKSUM_NOT_IP           |
                               FM10000_TE_CHECKSUM_NOT_TCP_OR_UDP   |
                               FM10000_TE_CHECKSUM_TCP_OR_UDP       |
                               FM10000_TE_CHECKSUM_DECAP_VALID      |
                               FM10000_TE_CHECKSUM_DECAP_UPDATE)


} fm_fm10000TeChecksumSel;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_fm10000TeChecksumCfg'' to specify the 
 * checksum option.
 **************************************************/
typedef enum
{
    /** Trap those frames. */
    FM_FM10000_TE_CHECKSUM_TRAP = 0,

    /** No need to compute checksum. VXLAN/NGE tunnel header will be set to
     *  a zero checksum at exit. */
    FM_FM10000_TE_CHECKSUM_ZERO,

    /** Compute checksum from entire packet starting after FTAG and stopping
     *  at end of packet. */
    FM_FM10000_TE_CHECKSUM_COMPUTE,

    /** Recover checksum from UDP/TCP header. This checksum will be used for
     *  computing VXLAN/NGE checksum. If the checksum is zero, then TE will
     *  compute the outer checksum walking over the entire IP packet. */
    FM_FM10000_TE_CHECKSUM_HEADER,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TE_CHECKSUM_MAX

} fm_fm10000TeChecksumAction;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeChecksum'' and 
 *  ''fm10000GetTeChecksum'' and specifies the 
 *  checksum configuration.
 **************************************************/
typedef struct _fm_fm10000TeChecksumCfg
{
    /** Encap checksum options for non-IP packets. */
    fm_fm10000TeChecksumAction notIp;

    /** Encap checksum options for IP packets that are not TCP or UDP. */
    fm_fm10000TeChecksumAction notTcpOrUdp;

    /** Encap checksum options for IP packets that are TCP or UDP. */
    fm_fm10000TeChecksumAction tcpOrUdp;

    /** Validation of outer checksum for VXLAN and NGE.
     *  Choices are:                                                    \lb\lb
     *  FALSE: Does not check the value.                                \lb
     *  TRUE:  The checksum is validated for VXLAN and NGE if and only if the
     *         checksum is not 0. If found invalid, then the packet is trapped. */
    fm_bool   verifDecapChecksum;

    /** Value to insert in outer checksum for VXLAN and NGE in decap
     *  mode ''FM_FM10000_TE_OUTER_HEADER_LEAVE_AS_IS''.
     *  Choices are:                                                    \lb\lb
     *  FALSE: Overwrite outer VXLAN/NGE UDP checksum with 0.           \lb
     *  TRUE:  Update outer VXLAN/NGE checksum based on frame modification. */
    fm_bool   updateDecapChecksum;

} fm_fm10000TeChecksumCfg;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Parser Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeParserCfg'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write the L4 port used for VXLAN. */
    FM10000_TE_PARSER_VXLAN_PORT = (1 << 0),

    /** Write the L4 port used for NGE. */
    FM10000_TE_PARSER_NGE_PORT = (1 << 1),

    /** Write the extra ethernet type. Parser jumps over if detected. */
    FM10000_TE_PARSER_ETH_TYPE = (1 << 2),

    /** Write if the extra ethernet type is expected before or after vlan. */
    FM10000_TE_PARSER_ETH_TYPE_VLAN = (1 << 3),

    /** Write the size of the tag referenced by the extra ethernet type. */
    FM10000_TE_PARSER_TAG_SIZE = (1 << 4),

    /** Write if the NVGRE or GRE header protocol field needs to be
     *  validated on decap. */
    FM10000_TE_PARSER_CHECK_PROTOCOL = (1 << 5),

    /** Write if the NVGRE or GRE header version field needs to be
     *  validated on decap. */
    FM10000_TE_PARSER_CHECK_VERSION = (1 << 6),

    /** Write if the NGE OAM bit needs to be compared with zero on decap. */
    FM10000_TE_PARSER_CHECK_NGE_OAM = (1 << 7),

    /** Write if the NGE C bit needs to be compared with zero on decap. */
    FM10000_TE_PARSER_CHECK_NGE_C = (1 << 8),

    /** Write all fields. */
    FM10000_TE_PARSER_ALL = (FM10000_TE_PARSER_VXLAN_PORT       |
                             FM10000_TE_PARSER_NGE_PORT         |
                             FM10000_TE_PARSER_ETH_TYPE         |
                             FM10000_TE_PARSER_ETH_TYPE_VLAN    |
                             FM10000_TE_PARSER_TAG_SIZE         |
                             FM10000_TE_PARSER_CHECK_PROTOCOL   |
                             FM10000_TE_PARSER_CHECK_VERSION    |
                             FM10000_TE_PARSER_CHECK_NGE_OAM    |
                             FM10000_TE_PARSER_CHECK_NGE_C)

} fm_fm10000TeParserSel;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeParser'' and 
 *  ''fm10000GetTeParser'' and specifies the 
 *  parser configuration.
 **************************************************/
typedef struct _fm_fm10000TeParserCfg
{
    /** The L4 port used for VXLAN. Setting to 0 disables VXLAN
     *  detection. */
    fm_uint16 vxLanPort;

    /** The L4 port used for NGE. Setting to 0 disables NGE
     *  detection. When the TE Mode is
     *  ''FM10000_TE_MODE_VXLAN_GPE_NSH'', this field must contain the 
     *  GPE L4 port to use to recognize the GPE header. In that
     *  mode, both vxLanPort and ngePort numbers are equally
     *  accepted for both types of packets (VXLAN and GPE). */
    fm_uint16 ngePort;

    /** The extra ethernet type. Parser jumps over if detected. */
    fm_uint16 etherType;

    /** The size in 16-bit quanta excluding the 16-bit size of the
     *  extra ethernet type. Supported sizes range from 0 to 3. Setting to 0
     *  disables detection of this type. Setting to 3 only works if no VLAN
     *  tag is present. */
    fm_byte   tagSize;

    /** Whether the extra ethernet type is expected before or after vlan tag.
     *  Choices are:                                                    \lb\lb
     *  FALSE: Before VLAN Tag (0x8100).                                \lb
     *  TRUE:  After VLAN Tag (0x8100). */
    fm_bool   afterVlan;

    /** Whether the NVGRE or GRE header protocol field needs to be
     *  validated on decap (TRUE) or not (FALSE). */
    fm_bool   checkProtocol;

    /** Whether the NVGRE or GRE header version field needs to be
     *  validated on decap (TRUE) or not (FALSE).
     *  
     *  When the TE mode is FM10000_TE_MODE_VXLAN_GPE_NSH, defines
     *  whether the GPE and NSH header version fields need to be
     *  validated on decap (TRUE) or not (FALSE) */
    fm_bool   checkVersion;

    /** Whether the NGE OAM bit needs to be compared with zero on
     *  decap (TRUE) or not (FALSE). */
    fm_bool   checkNgeOam;

    /** Whether the NGE C bit needs to be compared with zero on
     *  decap (TRUE) or not (FALSE). */
    fm_bool   checkNgeC;

} fm_fm10000TeParserCfg;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Trap Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeTrapCfg'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write the trap DGLORT base. */
    FM10000_TE_TRAP_BASE_DGLORT = (1 << 0),

    /** Write the trapping behavior applied on normal matching flow. */
    FM10000_TE_TRAP_NORMAL = (1 << 1),

    /** Write the trapping behavior applied on incoming frames where DGLORT did
     *  not match any entry as specified in ''fm_fm10000TeDGlort''. */
    FM10000_TE_TRAP_NO_DGLORT_MATCH = (1 << 2),

    /** Write the trapping behavior applied on incoming frames where SGLORT did
     *  not match any entry as specified in ''fm_fm10000TeSGlort''. */
    FM10000_TE_TRAP_NO_SGLORT_MATCH = (1 << 3),

    /** Write the trapping behavior applied on decapped frames where no outer
     *  L3 header is found. */
    FM10000_TE_TRAP_DECAP_NO_OUTER_L3 = (1 << 4),

    /** Write the trapping behavior applied on decapped frames where no outer
     *  L4 header is found. */
    FM10000_TE_TRAP_DECAP_NO_OUTER_L4 = (1 << 5),

    /** Write the trapping behavior applied on decapped frames where no outer
     *  tunnel header is found. */
    FM10000_TE_TRAP_DECAP_NO_OUTER_TUN = (1 << 6),

    /** Write the trapping behavior applied on decapped frames where bad L4
     *  checksum is found and fm_fm10000TeChecksumCfg.verifDecapChecksum is
     *  set to TRUE. */
    FM10000_TE_TRAP_DECAP_BAD_CHECKSUM = (1 << 7),

    /** Write the trapping behavior applied on decapped frames where bad tunneling
     *  version, nonzero OAM/C bits, or protocol are found in tunnel header
     *  and fm_fm10000TeParserCfg.check**** is set to TRUE. */
    FM10000_TE_TRAP_DECAP_BAD_TUNNEL = (1 << 8),

    /** Write the trapping behavior applied on encapped frames where no L3
     *  header is found and fm_fm10000TeChecksumCfg.notIp is set to trap. */
    FM10000_TE_TRAP_ENCAP_NO_L3 = (1 << 9),

    /** Write the trapping behavior applied on encapped frames where no L4
     *  header is found and fm_fm10000TeChecksumCfg.notTcpOrUdp is set to
     *  trap. */
    FM10000_TE_TRAP_ENCAP_NO_L4 = (1 << 10),

    /** Write the trapping behavior applied on encapped frames where L4
     *  header is found and fm_fm10000TeChecksumCfg.tcpOrUdp is set to
     *  trap. */
    FM10000_TE_TRAP_ENCAP_ANY_L4 = (1 << 11),

    /** Write the trapping behavior applied on received frames for which an
     *  imcompletely formed header is found. */
    FM10000_TE_TRAP_TRUNCATED_HEADER = (1 << 12),

    /** Write the trapping behavior applied on received frames for which
     *  incompletely formed IP payload is found. */
    FM10000_TE_TRAP_TRUNC_IP_PAYLOAD = (1 << 13),

    /** Write the trapping behavior applied on flows that do not match any
     *  entry. */
    FM10000_TE_TRAP_NO_FLOW_MATCH = (1 << 14),

    /** Write the trapping behavior applied on detection of flow table
     *  corruption. */
    FM10000_TE_TRAP_MISSING_RECORD = (1 << 15),

    /** Write the trapping behavior applied when an uncorrectable ECC error
     *  is encountered in flow table. */
    FM10000_TE_TRAP_UC_ERR = (1 << 16),

    /** Write the trapping behavior applied on hash or index lookup out of
     *  bounds. */
    FM10000_TE_TRAP_LOOKUP_BOUNDS = (1 << 17),

    /** Write the trapping behavior applied on flow table index out of bounds. */
    FM10000_TE_TRAP_DATA_BOUNDS = (1 << 18),

    /** Write the trapping behavior applied on received frames with a total
     *  header length (outer plus inner) that exceeds 512 bytes. */
    FM10000_TE_TRAP_HEADER_LIMIT = (1 << 19),

    /** Write all fields. */
    FM10000_TE_TRAP_ALL = (FM10000_TE_TRAP_BASE_DGLORT          |
                           FM10000_TE_TRAP_NORMAL               |
                           FM10000_TE_TRAP_NO_DGLORT_MATCH      |
                           FM10000_TE_TRAP_NO_SGLORT_MATCH      |
                           FM10000_TE_TRAP_DECAP_NO_OUTER_L3    |
                           FM10000_TE_TRAP_DECAP_NO_OUTER_L4    |
                           FM10000_TE_TRAP_DECAP_NO_OUTER_TUN   |
                           FM10000_TE_TRAP_DECAP_BAD_CHECKSUM   |
                           FM10000_TE_TRAP_DECAP_BAD_TUNNEL     |
                           FM10000_TE_TRAP_ENCAP_NO_L3          |
                           FM10000_TE_TRAP_ENCAP_NO_L4          |
                           FM10000_TE_TRAP_ENCAP_ANY_L4         |
                           FM10000_TE_TRAP_TRUNCATED_HEADER     |
                           FM10000_TE_TRAP_TRUNC_IP_PAYLOAD     |
                           FM10000_TE_TRAP_NO_FLOW_MATCH        |
                           FM10000_TE_TRAP_MISSING_RECORD       |
                           FM10000_TE_TRAP_UC_ERR               |
                           FM10000_TE_TRAP_LOOKUP_BOUNDS        |
                           FM10000_TE_TRAP_DATA_BOUNDS          |
                           FM10000_TE_TRAP_HEADER_LIMIT)

} fm_fm10000TeTrapSel;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_fm10000TeTrapCfg'' to specify the 
 * trap option.
 **************************************************/
typedef enum
{
    /** Send frames to the specified trap DGLORT. */
    FM_FM10000_TE_TRAP_DGLORT0 = 0,

    /** Send frames to the specified trap DGLORT + 1. */
    FM_FM10000_TE_TRAP_DGLORT1,

    /** Send frames to the specified trap DGLORT + 2. */
    FM_FM10000_TE_TRAP_DGLORT2,

    /** Send frames to the specified trap DGLORT + 3. */
    FM_FM10000_TE_TRAP_DGLORT3,

    /** Forward frames to the incoming frame DGLORT. */
    FM_FM10000_TE_TRAP_PASS,

    /** Forward frames to the incoming frame DGLORT, but invalidate
     *  them at the end. */
    FM_FM10000_TE_TRAP_PASS_WITH_ERROR,

    /** Drop frames. */
    FM_FM10000_TE_TRAP_DROP,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TE_TRAP_MAX

} fm_fm10000TeTrapAction;


/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetTeTrap'' and 
 *  ''fm10000GetTeTrap'' and specifies the 
 *  trap configuration.
 **************************************************/
typedef struct _fm_fm10000TeTrapCfg
{
    /** The trap DGLORT base. */
    fm_uint16              trapGlort;

    /** The trapping behavior applied on normal matching flow. */
    fm_fm10000TeTrapAction normal;

    /** The trapping behavior applied on incoming frames where DGLORT
     *  did not match any entry as specified in ''fm_fm10000TeDGlort''. */
    fm_fm10000TeTrapAction noDglortMatch;

    /** The trapping behavior applied on incoming frames where SGLORT
     *  did not match any entry as specified in ''fm_fm10000TeSGlort''. */
    fm_fm10000TeTrapAction noSglortMatch;

    /** The trapping behavior applied on decapped frames where no outer
     *  L3 header is found. */
    fm_fm10000TeTrapAction decapNoOuterL3;

    /** The trapping behavior applied on decapped frames where no outer
     *  L4 header is found. */
    fm_fm10000TeTrapAction decapNoOuterL4;

    /** The trapping behavior applied on decapped frames where no outer
     *  tunnel header is found. */
    fm_fm10000TeTrapAction decapNoOuterTun;

    /** The trapping behavior applied on decapped frames where bad L4
     *  checksum is found and fm_fm10000TeChecksumCfg.verifDecapChecksum is
     *  set to TRUE. */
    fm_fm10000TeTrapAction decapBadChecksum;

    /** The trapping behavior applied on decapped frames where bad
     *  tunneling version, nonzero OAM/C bits, or protocol is found in
     *  tunnel header and fm_fm10000TeParserCfg.check**** is set to TRUE. */
    fm_fm10000TeTrapAction decapBadTunnel;

    /** The trapping behavior applied on encapped frames where no L3
     *  header is found and fm_fm10000TeChecksumCfg.notIp is set to trap. */
    fm_fm10000TeTrapAction encapNoL3;

    /** The trapping behavior applied on encapped frames where no L4
     *  header is found and fm_fm10000TeChecksumCfg.notTcpOrUdp is set to
     *  trap. */
    fm_fm10000TeTrapAction encapNoL4;

    /** The trapping behavior applied on encapped frames where L4
     *  header is found and fm_fm10000TeChecksumCfg.tcpOrUdp is set to
     *  trap. */
    fm_fm10000TeTrapAction encapAnyL4;

    /** The trapping behavior applied on received frames for which an
     *  incompletely formed header is found. */
    fm_fm10000TeTrapAction truncatedHeader;

    /** The trapping behavior applied on received frames for which
     *  incompletely formed IP payload is found. */
    fm_fm10000TeTrapAction truncIpPayload;

    /** The trapping behavior applied on flows that do not match any
     *  entry. */
    fm_fm10000TeTrapAction noFlowMatch;

    /** The trapping behavior applied on flow table corruption
     *  detection. */
    fm_fm10000TeTrapAction missingRecord;

    /** The trapping behavior applied on uncorrectable ECC error
     *  encountered in flow table. */
    fm_fm10000TeTrapAction ucErr;

    /** The trapping behavior applied on hash or index lookup out of
     *  bounds. */
    fm_fm10000TeTrapAction lookupBounds;

    /** The trapping behavior applied on flow table index out of
     *  bounds. */
    fm_fm10000TeTrapAction dataBounds;

    /** The trapping behavior applied on received frames with a total
     *  header length (outer plus inner) that exceeds 512 bytes. */
    fm_fm10000TeTrapAction headerLimit;

} fm_fm10000TeTrapCfg;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Lookup Field Selectors.
 *  
 *  Bit masks that specify which field(s) of a
 *  ''fm_fm10000TeLookup'' structure should be
 *  written to the tunnel engine register.
 **************************************************/
typedef enum
{
    /** Write the data location. */
    FM10000_TE_LOOKUP_DATA_PTR = (1 << 0),

    /** Write the data length. */
    FM10000_TE_LOOKUP_DATA_LENGTH = (1 << 1),

    /** Write the last bit. */
    FM10000_TE_LOOKUP_LAST = (1 << 2),

    /** Write all fields. */
    FM10000_TE_LOOKUP_ALL = (FM10000_TE_LOOKUP_DATA_PTR     |
                             FM10000_TE_LOOKUP_DATA_LENGTH  |
                             FM10000_TE_LOOKUP_LAST)

} fm_fm10000TeLookupSel;


/**************************************************/
/** \ingroup typeStruct
 *  This structure specifies a lookup entry
 *  configuration. It is referenced by 
 *  fm10000SetTeLookup and fm10000GetTeLookup.
 **************************************************/
typedef struct _fm_fm10000TeLookup
{
    /** Data pointer. */
    fm_uint16 dataPtr;

    /** Data length in 128b words of data structure. */
    fm_byte   dataLength;

    /** Whether the data pointer refers to the last block of a data
     *  structure. Choices are:                                         \lb\lb
     *  FALSE: Data structures are chained.                             \lb
     *  TRUE:  Last block of the data structure. */
    fm_bool   last;

} fm_fm10000TeLookup;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_fm10000TeData'' to specify the block type.
 **************************************************/
typedef enum
{
    /** Whether this block is a pointer. This can be set to chain multiple
     *  blocks together if the lookup originator sets
     *  fm_fm10000TeLookup.last == FALSE. It can also be used to share
     *  multiple tunnel data blocks among multiple flows. */
    FM_FM10000_TE_DATA_BLOCK_POINTER = 0,

    /** Flow Key block type. */
    FM_FM10000_TE_DATA_BLOCK_FLOW_KEY,

    /** Encap Flow Data block type. */
    FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP,

    /** Decap Flow Data block type. */
    FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP,

    /** Tunnel Data block type. */
    FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TE_DATA_BLOCK_MAX

} fm_fm10000TeDataBlockType;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeDataBlockVal'', 
 *  specifies the flow key value to be written.
 **************************************************/
typedef struct _fm_fm10000TeDataFlowKeyVal
{
    /** Defines the keys included in the search:
     *  ''FM10000_TE_KEY_VSI_TEP''             \lb
     *  ''FM10000_TE_KEY_VNI''                 \lb
     *  ''FM10000_TE_KEY_DMAC''                \lb
     *  ''FM10000_TE_KEY_SMAC''                \lb
     *  ''FM10000_TE_KEY_VLAN''                \lb
     *  ''FM10000_TE_KEY_IPV6''                \lb
     *  ''FM10000_TE_KEY_DIP''                 \lb
     *  ''FM10000_TE_KEY_SIP''                 \lb
     *  ''FM10000_TE_KEY_L4SRC''               \lb
     *  ''FM10000_TE_KEY_L4DST''               \lb
     *  ''FM10000_TE_KEY_PROT''                \lb
     *  ''FM10000_TE_KEY_UDP''                 \lb
     *  ''FM10000_TE_KEY_TCP''                 */
    fm_uint16  searchKeyConfig;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_VSI_TEP'' is set. */
    fm_uint16  vsiTep;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_VNI'' is set. */
    fm_uint32  vni;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_DMAC'' is set. */
    fm_macaddr dmac;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_SMAC'' is set. */
    fm_macaddr smac;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_VLAN'' is set. */
    fm_uint16  vlan;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_DIP'' is set. */
    fm_ipAddr  dip;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_SIP'' is set. */
    fm_ipAddr  sip;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_L4SRC'' is set. */
    fm_uint16  l4Src;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_L4DST'' is set. */
    fm_uint16  l4Dst;

    /** Must be set to the proper matching value if the search key
     *  ''FM10000_TE_KEY_PROT'' is set. */
    fm_byte    protocol;

} fm_fm10000TeDataFlowKeyVal;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Flow Encap Selectors.
 *  
 *  Bit masks that specify which field(s) are defined
 *  in the flow encapsulation. Undefined field would
 *  uses proper default value.
 **************************************************/
typedef enum
{
    /** Defines the VNI for this flow. */
    FM10000_TE_FLOW_ENCAP_VNI = (1 << 0),

    /** Whether a counter structure is specified. */
    FM10000_TE_FLOW_ENCAP_COUNTER = (1 << 1),

    /** Defines the Destination MAC for this flow. */
    FM10000_TE_FLOW_ENCAP_DMAC = (1 << 2),

    /** Defines the Source MAC for this flow. */
    FM10000_TE_FLOW_ENCAP_SMAC = (1 << 3),

    /** Defines the Vlan ID for this flow. */
    FM10000_TE_FLOW_ENCAP_VLAN = (1 << 4),

    /** Whether the supplied L3 address are IPv6. */
    FM10000_TE_FLOW_ENCAP_IPV6 = (1 << 5),

    /** Defines the Destination IP for this flow. */
    FM10000_TE_FLOW_ENCAP_DIP = (1 << 6),

    /** Defines the Source IP for this flow. */
    FM10000_TE_FLOW_ENCAP_SIP = (1 << 7),

    /** Defines the L4 Source Port for this flow. */
    FM10000_TE_FLOW_ENCAP_L4SRC = (1 << 8),

    /** Defines the L4 Destination Port for this flow. */
    FM10000_TE_FLOW_ENCAP_L4DST = (1 << 9),

    /** Defines the TTL for this flow. */
    FM10000_TE_FLOW_ENCAP_TTL = (1 << 10),

    /** Defines the NGE Data for this flow. */
    FM10000_TE_FLOW_ENCAP_NGE = (1 << 11),

    /** Whether tunneling information is supplied. */
    FM10000_TE_FLOW_ENCAP_TUNNEL = (1 << 12),

    /** Whether the tunneling information following the flow data is a pointer
     *  to where the tunnel data is or if the tunnel data is immediately following
     *  the flow data. */
    FM10000_TE_FLOW_ENCAP_TUNNEL_PTR = (1 << 13),

    /** Whether time tag should be loaded into NGE in words[14:15] */
    FM10000_TE_FLOW_ENCAP_NGE_TIME = (1 << 14),

} fm_fm10000TeFlowEncapSel;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeDataBlockVal'', 
 *  specifies the flow encap value to be written.
 **************************************************/
typedef struct _fm_fm10000TeDataFlowEncapVal
{
    /** Bit mask specifying the block fields included:  \lb
     *  ''FM10000_TE_FLOW_ENCAP_VNI''              \lb
     *  ''FM10000_TE_FLOW_ENCAP_COUNTER''          \lb
     *  ''FM10000_TE_FLOW_ENCAP_DMAC''             \lb
     *  ''FM10000_TE_FLOW_ENCAP_SMAC''             \lb
     *  ''FM10000_TE_FLOW_ENCAP_VLAN''             \lb
     *  ''FM10000_TE_FLOW_ENCAP_IPV6''             \lb
     *  ''FM10000_TE_FLOW_ENCAP_DIP''              \lb
     *  ''FM10000_TE_FLOW_ENCAP_SIP''              \lb
     *  ''FM10000_TE_FLOW_ENCAP_L4SRC''            \lb
     *  ''FM10000_TE_FLOW_ENCAP_L4DST''            \lb
     *  ''FM10000_TE_FLOW_ENCAP_TTL''              \lb
     *  ''FM10000_TE_FLOW_ENCAP_NGE''              \lb
     *  ''FM10000_TE_FLOW_ENCAP_TUNNEL''           \lb
     *  ''FM10000_TE_FLOW_ENCAP_TUNNEL_PTR''       \lb
     *  ''FM10000_TE_FLOW_ENCAP_NGE_TIME''              */
    fm_uint16  encapConfig;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_ENCAP_VNI''
     *  is set. */
    fm_uint32  vni;

    /** Must be set to the counter index if ''FM10000_TE_FLOW_ENCAP_COUNTER''
     *  is set. */
    fm_uint16  counterIdx;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_ENCAP_DMAC''
     *  is set. */
    fm_macaddr dmac;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_ENCAP_SMAC''
     *  is set. */
    fm_macaddr smac;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_ENCAP_VLAN''
     *  is set. */
    fm_uint16  vlan;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_DIP'' is
     *  set. */
    fm_ipAddr  dip;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_SIP'' is
     *  set. */
    fm_ipAddr  sip;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_L4SRC'' is
     *  set. */
    fm_uint16  l4Src;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_L4DST'' is
     *  set. */
    fm_uint16  l4Dst;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_TTL'' is
     *  set. */
    fm_byte    ttl;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_NGE'' is
     *  set. This is a mask indicating which of 16 words is present.  */
    fm_uint16  ngeMask;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_ENCAP_NGE'' is
     *  set. This is the value to set based on each word set in the mask.  */
    fm_uint32  ngeData[FM10000_TE_NGE_DATA_SIZE];

    /** Must be set to the proper tunnel index if
     *  ''FM10000_TE_FLOW_ENCAP_TUNNEL_PTR'' is set. */
    fm_fm10000TeLookup tunnelIdx;

} fm_fm10000TeDataFlowEncapVal;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Flow Decap Selectors.
 *  
 *  Bit masks that specify which field(s) are defined
 *  in the flow decapsulation. Undefined field would
 *  use proper default value.
 **************************************************/
typedef enum
{
    /** Defines the Destination GLORT for this flow. */
    FM10000_TE_FLOW_DECAP_DGLORT = (1 << 3),

    /** Defines the Destination MAC for this flow. */
    FM10000_TE_FLOW_DECAP_DMAC = (1 << 4),

    /** Defines the Source MAC for this flow. */
    FM10000_TE_FLOW_DECAP_SMAC = (1 << 5),

    /** Defines the Vlan ID for this flow. */
    FM10000_TE_FLOW_DECAP_VLAN = (1 << 6),

    /** Whether the supplied L3 address is IPv6. */
    FM10000_TE_FLOW_DECAP_IPV6 = (1 << 7),

    /** Defines the Destination IP for this flow. */
    FM10000_TE_FLOW_DECAP_DIP = (1 << 8),

    /** Defines the Source IP for this flow. */
    FM10000_TE_FLOW_DECAP_SIP = (1 << 9),

    /** Defines the TTL for this flow. */
    FM10000_TE_FLOW_DECAP_TTL = (1 << 10),

    /** Defines the L4 Source Port for this flow. */
    FM10000_TE_FLOW_DECAP_L4SRC = (1 << 11),

    /** Defines the L4 Destination Port for this flow. */
    FM10000_TE_FLOW_DECAP_L4DST = (1 << 12),

    /** Whether a counter structure is specified. */
    FM10000_TE_FLOW_DECAP_COUNTER = (1 << 13),

} fm_fm10000TeFlowDecapSel;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_fm10000TeDataFlowDecapVal'' to specify the
 * outer header disposition.
 **************************************************/
typedef enum
{
    /** Delete the Outer Header. */
    FM_FM10000_TE_OUTER_HEADER_DELETE = 0,

    /** Leave the Outer Header in place but update the L4 checksum if the
     *  inner header is updated. */
    FM_FM10000_TE_OUTER_HEADER_LEAVE_AS_IS,

    /** Move the Outer Header to the end of the packet and append the Outer
     *  Header length and Flow Pointer. */
    FM_FM10000_TE_OUTER_HEADER_MOVE_TO_END,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TE_OUTER_HEADER_MAX

} fm_fm10000TeOuterHeader;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeDataBlockVal'', 
 *  specifies the flow decap value to be written.
 **************************************************/
typedef struct _fm_fm10000TeDataFlowDecapVal
{
    /** Specifies the disposition of the outer header. */
    fm_fm10000TeOuterHeader outerHeader;

    /** Bit mask specifying the block fields included:  \lb
     *  ''FM10000_TE_FLOW_DECAP_DGLORT''           \lb
     *  ''FM10000_TE_FLOW_DECAP_DMAC''             \lb
     *  ''FM10000_TE_FLOW_DECAP_SMAC''             \lb
     *  ''FM10000_TE_FLOW_DECAP_VLAN''             \lb
     *  ''FM10000_TE_FLOW_DECAP_IPV6''             \lb
     *  ''FM10000_TE_FLOW_DECAP_DIP''              \lb
     *  ''FM10000_TE_FLOW_DECAP_SIP''              \lb
     *  ''FM10000_TE_FLOW_DECAP_TTL''              \lb
     *  ''FM10000_TE_FLOW_DECAP_L4SRC''            \lb
     *  ''FM10000_TE_FLOW_DECAP_L4DST''            \lb
     *  ''FM10000_TE_FLOW_DECAP_COUNTER''             */
    fm_uint16  decapConfig;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_DECAP_DGLORT''
     *  is set. */
    fm_uint16  dglort;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_DECAP_DMAC''
     *  is set. */
    fm_macaddr dmac;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_DECAP_SMAC''
     *  is set. */
    fm_macaddr smac;

    /** Must be set to the proper field value if ''FM10000_TE_FLOW_DECAP_VLAN''
     *  is set. */
    fm_uint16  vlan;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_DECAP_DIP'' is
     *  set. */
    fm_ipAddr  dip;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_DECAP_SIP'' is
     *  set. */
    fm_ipAddr  sip;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_DECAP_TTL'' is
     *  set. */
    fm_byte    ttl;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_DECAP_L4SRC'' is
     *  set. */
    fm_uint16  l4Src;

    /** Must be set to the proper value if ''FM10000_TE_FLOW_DECAP_L4DST'' is
     *  set. */
    fm_uint16  l4Dst;

    /** Must be set to the counter index if ''FM10000_TE_FLOW_DECAP_COUNTER''
     *  is set. */
    fm_uint16  counterIdx;

} fm_fm10000TeDataFlowDecapVal;


/**************************************************/
/** \ingroup typeEnum
 *  Tunnel Engine Encap Tunnel Selectors.
 *  
 *  Bit masks that specify which field(s) are defined
 *  in the tunnel encapsulation. Undefined fields would
 *  use proper default value.
 **************************************************/
typedef enum
{
    /** Whether the supplied L3 address is IPv6. */
    FM10000_TE_TUNNEL_ENCAP_IPV6 = (1 << 2),

    /** Defines the Source IP for this tunnel. */
    FM10000_TE_TUNNEL_ENCAP_SIP = (1 << 3),

    /** Defines the TOS for this tunnel. */
    FM10000_TE_TUNNEL_ENCAP_TOS = (1 << 4),

    /** Defines the TTL for this tunnel. */
    FM10000_TE_TUNNEL_ENCAP_TTL = (1 << 5),

    /** Defines the L4 Destination Port for this tunnel. */
    FM10000_TE_TUNNEL_ENCAP_L4DST = (1 << 6),

    /** Defines the L4 Source Port for this tunnel. */
    FM10000_TE_TUNNEL_ENCAP_L4SRC = (1 << 7),

    /** Whether a counter structure is specified. */
    FM10000_TE_TUNNEL_ENCAP_COUNTER = (1 << 8),

    /** Defines the NGE Data for this tunnel. */
    FM10000_TE_TUNNEL_ENCAP_NGE = (1 << 9),

    /** Whether time tag should be loaded into NGE in words[14:15]. */
    FM10000_TE_TUNNEL_ENCAP_NGE_TIME = (1 << 10),

} fm_fm10000TeTunnelEncapSel;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm_fm10000TeDataTunnelVal'' to specify the
 * tunnel type.
 **************************************************/
typedef enum
{
    /** Delete the Outer Header. */
    FM_FM10000_TE_TUNNEL_TYPE_GENERIC = 0,

    /** Encapsulate using VXLAN type of tunnel. */
    FM_FM10000_TE_TUNNEL_TYPE_VXLAN,

    /** Encapsulate using NGE type of tunnel. */
    FM_FM10000_TE_TUNNEL_TYPE_NGE,

    /** Encapsulate using NVGRE type of tunnel. */
    FM_FM10000_TE_TUNNEL_TYPE_NVGRE,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_TE_TUNNEL_TYPE_MAX

} fm_fm10000TeTunnelType;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeDataBlockVal'', 
 *  specifies the tunnel encap value to be written.
 **************************************************/
typedef struct _fm_fm10000TeDataTunnelVal
{
    /** Specifies the disposition of the outer header. */
    fm_fm10000TeTunnelType tunnelType;

    /** Bit mask specifying the block fields included:  \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_IPV6''           \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_SIP''            \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_TOS''            \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_TTL''            \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_L4DST''          \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_L4SRC''          \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_COUNTER''        \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_NGE''            \lb
     *  ''FM10000_TE_TUNNEL_ENCAP_NGE_TIME''        */
    fm_uint16  tunnelConfig;

    /** Must always be set (REQUIRED). */
    fm_ipAddr  dip;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_IPV6'' is
     *  set. */
    fm_ipAddr  sip;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_TOS'' is
     *  set. */
    fm_byte    tos;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_TTL'' is
     *  set. */
    fm_byte    ttl;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_L4DST'' is
     *  set. */
    fm_uint16  l4Dst;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_L4SRC'' is
     *  set. */
    fm_uint16  l4Src;

    /** Must be set to the counter index if ''FM10000_TE_TUNNEL_ENCAP_COUNTER''
     *  is set. */
    fm_uint16  counterIdx;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_NGE'' is
     *  set. This is a mask indicating which of 16 words is present.  */
    fm_uint16  ngeMask;

    /** Must be set to the proper value if ''FM10000_TE_TUNNEL_ENCAP_NGE'' is
     *  set. This is the value to set based on each word set in the mask.  */
    fm_uint32  ngeData[FM10000_TE_NGE_DATA_SIZE];

} fm_fm10000TeDataTunnelVal;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000TeData'', this union 
 *  specifies the data for a TE Data Block,  
 *  specific to the block type 
 *  (''fm_fm10000TeDataBlockType'').
 **************************************************/
typedef union _fm_fm10000TeDataBlockVal
{
    /** Next Pointer information for the ''FM_FM10000_TE_DATA_BLOCK_POINTER''
     *  block type. */
    fm_fm10000TeLookup           nextLookup;

    /** Search key information for the ''FM_FM10000_TE_DATA_BLOCK_FLOW_KEY''
     *  block type. */
    fm_fm10000TeDataFlowKeyVal   flowKeyVal;

    /** Encap flow data information for the
     *  ''FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_ENCAP'' block type. */
    fm_fm10000TeDataFlowEncapVal flowEncapVal;

    /** Encap flow data information for the
     *  ''FM_FM10000_TE_DATA_BLOCK_FLOW_DATA_DECAP'' block type. */
    fm_fm10000TeDataFlowDecapVal flowDecapVal;

    /** Tunnel data information for the
     *  ''FM_FM10000_TE_DATA_BLOCK_TUNNEL_DATA'' block type. */
    fm_fm10000TeDataTunnelVal    tunnelVal;

} fm_fm10000TeDataBlockVal;


/**************************************************/
/** \ingroup typeStruct
 *  This structure specifies a data block.
 *  It is referenced by fm10000SetTeData and 
 *  fm10000GetTeData.
 **************************************************/
typedef struct _fm_fm10000TeData
{
    /** Block type. */
    fm_fm10000TeDataBlockType blockType;

    /** Data assigned to this block type. */
    fm_fm10000TeDataBlockVal  blockVal;

} fm_fm10000TeData;


/*****************************************************************************
 * Public Functions
 *****************************************************************************/
 
fm_status fm10000TeInit(fm_int sw);

fm_status fm10000SyncTeDataLookup(fm_int sw,
                                  fm_int te);

fm_status fm10000SetTeDGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeDGlort *teDGlort,
                             fm_bool             useCache);

fm_status fm10000GetTeDGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeDGlort *teDGlort,
                             fm_bool             useCache);

fm_status fm10000SetTeSGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeSGlort *teSGlort,
                             fm_bool             useCache);

fm_status fm10000GetTeSGlort(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeSGlort *teSGlort,
                             fm_bool             useCache);

fm_status fm10000SetTeDefaultGlort(fm_int                sw,
                                   fm_int                te,
                                   fm_fm10000TeGlortCfg *teGlortCfg,
                                   fm_uint32             fieldSelectMask,
                                   fm_bool               useCache);

fm_status fm10000GetTeDefaultGlort(fm_int                sw,
                                   fm_int                te,
                                   fm_fm10000TeGlortCfg *teGlortCfg,
                                   fm_bool               useCache);

fm_status fm10000SetTeDefaultTep(fm_int              sw,
                                 fm_int              te,
                                 fm_int              tep,
                                 fm_fm10000TeTepCfg *teTepCfg,
                                 fm_uint32           fieldSelectMask,
                                 fm_bool             useCache);

fm_status fm10000GetTeDefaultTep(fm_int              sw,
                                 fm_int              te,
                                 fm_int              tep,
                                 fm_fm10000TeTepCfg *teTepCfg,
                                 fm_bool             useCache);

fm_status fm10000SetTeDefaultTunnel(fm_int                 sw,
                                    fm_int                 te,
                                    fm_fm10000TeTunnelCfg *teTunnelCfg,
                                    fm_uint32              fieldSelectMask,
                                    fm_bool                useCache);

fm_status fm10000GetTeDefaultTunnel(fm_int                 sw,
                                    fm_int                 te,
                                    fm_fm10000TeTunnelCfg *teTunnelCfg,
                                    fm_bool                useCache);

fm_status fm10000SetTeChecksum(fm_int                   sw,
                               fm_int                   te,
                               fm_fm10000TeChecksumCfg *teChecksumCfg,
                               fm_uint32                fieldSelectMask,
                               fm_bool                  useCache);

fm_status fm10000GetTeChecksum(fm_int                   sw,
                               fm_int                   te,
                               fm_fm10000TeChecksumCfg *teChecksumCfg,
                               fm_bool                  useCache);

fm_status fm10000SetTeParser(fm_int                 sw,
                             fm_int                 te,
                             fm_fm10000TeParserCfg *teParserCfg,
                             fm_uint32              fieldSelectMask,
                             fm_bool                useCache);

fm_status fm10000GetTeParser(fm_int                 sw,
                             fm_int                 te,
                             fm_fm10000TeParserCfg *teParserCfg,
                             fm_bool                useCache);

fm_status fm10000SetTeTrap(fm_int               sw,
                           fm_int               te,
                           fm_fm10000TeTrapCfg *teTrapCfg,
                           fm_uint32            fieldSelectMask,
                           fm_bool              useCache);

fm_status fm10000GetTeTrap(fm_int               sw,
                           fm_int               te,
                           fm_fm10000TeTrapCfg *teTrapCfg,
                           fm_bool              useCache);

fm_status fm10000SetTeCnt(fm_int    sw,
                          fm_int    te,
                          fm_uint16 dropCnt,
                          fm_uint32 frameInCnt,
                          fm_uint32 frameDoneCnt);

fm_status fm10000GetTeCnt(fm_int     sw,
                          fm_int     te,
                          fm_uint16 *dropCnt,
                          fm_uint32 *frameInCnt,
                          fm_uint32 *frameDoneCnt);

fm_status fm10000SetTeFlowCnt(fm_int    sw,
                              fm_int    te,
                              fm_int    index,
                              fm_uint64 frameCnt,
                              fm_uint64 byteCnt);

fm_status fm10000GetTeFlowCnt(fm_int     sw,
                              fm_int     te,
                              fm_int     index,
                              fm_uint64 *frameCnt,
                              fm_uint64 *byteCnt);

fm_status fm10000SetTeFlowUsed(fm_int  sw,
                               fm_int  te,
                               fm_int  index,
                               fm_bool used);

fm_status fm10000GetTeFlowUsed(fm_int   sw,
                               fm_int   te,
                               fm_int   index,
                               fm_bool *used);

fm_status fm10000ResetTeFlowUsed(fm_int  sw,
                                 fm_int  te);

fm_status fm10000SetTeLookup(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeLookup *teLookup,
                             fm_uint32           fieldSelectMask,
                             fm_bool             useCache);

fm_status fm10000GetTeLookup(fm_int              sw,
                             fm_int              te,
                             fm_int              index,
                             fm_fm10000TeLookup *teLookup,
                             fm_bool             useCache);

fm_status fm10000SetTeData(fm_int            sw,
                           fm_int            te,
                           fm_int            baseIndex,
                           fm_fm10000TeData *teData,
                           fm_int            teDataLength,
                           fm_bool           useCache);

fm_status fm10000GetTeData(fm_int            sw,
                           fm_int            te,
                           fm_int            baseIndex,
                           fm_int            blockLength,
                           fm_bool           encap,
                           fm_fm10000TeData *teData,
                           fm_int            teDataLength,
                           fm_int           *teDataReturnLength,
                           fm_bool           useCache);

fm_status fm10000GetTeDataBlockLength(fm_fm10000TeData *teData,
                                      fm_int            teDataLength,
                                      fm_int           *blockLength);

void fm10000DbgDumpTe(fm_int sw, fm_int te);
 
#endif  /* __FM_FM10000_FM_API_TE_INT_H */

