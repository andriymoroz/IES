/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_map_int.h
 * Creation Date:  April 17, 2013
 * Description:    Low-level API for manipulating the Mappers.
 *
 * Copyright (c) 2013 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_FM_API_MAP_INT_H
#define __FM_FM10000_FM_API_MAP_INT_H

/* Maximum number for each mapper types. */
#define FM_FM10000_MAX_MAC_MAPPER           15
#define FM_FM10000_MAX_IP_MAPPER            15
#define FM_FM10000_MAX_ETYPE_MAPPER         15
#define FM_FM10000_MAX_PROT_MAPPER          7
#define FM_FM10000_MAX_L4PORT_MAPPER        63
#define FM_FM10000_MAX_LENGTH_MAPPER        15

/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapSourcePort'' and 
 *  ''fm10000GetMapSourcePort'' and specifies the 
 *  source port mapper configuration for a port.
 **************************************************/
typedef struct _fm_fm10000MapSrcPortCfg
{
    /** MAP_SRC field (4 bits). */
    fm_byte     mapSrc;

    /** Routable flag (1 bit). */
    fm_byte     routable;

} fm_fm10000MapSrcPortCfg;



/**************************************************/
/** Source Port Mapper Field Selectors
 *  \ingroup constFm10kMapSrcSel
 *  \page fm10kMapSrcSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapSrcPortCfg'' structure should be
 *  written to the mapper register.
 **************************************************/
/** \ingroup constFm10kMapSrcSel
 * @{ */

/** Write MAP_SRC field. */
#define FM_FM10000_MAP_SRC_ID           (1 << 0)

/** Write Routable flag. */
#define FM_FM10000_MAP_SRC_ROUTABLE     (1 << 1)

/** Write all fields. */
#define FM_FM10000_MAP_SRC_ALL         (FM_FM10000_MAP_SRC_ID |            \
                                        FM_FM10000_MAP_SRC_ROUTABLE)

/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapMac'' and ''fm10000GetMapMac''
 *  and specifies the mapper setting for a DMAC
 *  or SMAC mapper.
 **************************************************/
typedef struct _fm_fm10000MapMacCfg
{
    /** Ethernet MAC address field (48 bits) */
    fm_macaddr   macAddr;

    /** Ignore this many least significant bits when matching. (6 bits) */
    fm_byte      ignoreLength;

    /** Entry is valid for mapped source MAC addresses. (1 bit) */
    fm_byte      validSMAC;

    /** Entry is valid for mapped destination MAC addresses. (1 bit) */
    fm_byte      validDMAC;

    /** Mapped value to be produced by the mapper for this MAC address. (4 bits) */
    fm_byte      mapMac;

    /** Router indicates this MAC address is the destination MAC address of a router. (1 bit) */
    fm_byte      router;

} fm_fm10000MapMacCfg;



/**************************************************/
/** MAC Mapper Field Selectors
 *  \ingroup constFm10kMapMacSel
 *  \page Fm10kMapMacSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapMacCfg'' structure should be
 *  written to the mapper registers.
 **************************************************/
/** \ingroup constFm10kMapMacSel
 * @{ */

/** Write Ethernet MAC address field. */
#define FM_FM10000_MAP_MAC_ADDR         (1 << 0)

/** Write ignoreLength field. */
#define FM_FM10000_MAP_MAC_IGNORE       (1 << 1)

/** Write validSMAC flag. */
#define FM_FM10000_MAP_MAC_VALID_SMAC   (1 << 2)

/** Write validDMAC flag. */
#define FM_FM10000_MAP_MAC_VALID_DMAC   (1 << 3)

/** Write MAP_MAC field. */
#define FM_FM10000_MAP_MAC_ID           (1 << 4)

/** Write Router flag. */
#define FM_FM10000_MAP_MAC_ROUTER       (1 << 5)

/** Write all fields. */
#define FM_FM10000_MAP_MAC_ALL         (FM_FM10000_MAP_MAC_ADDR |          \
                                        FM_FM10000_MAP_MAC_IGNORE |        \
                                        FM_FM10000_MAP_MAC_VALID_SMAC |    \
                                        FM_FM10000_MAP_MAC_VALID_DMAC |    \
                                        FM_FM10000_MAP_MAC_ID |            \
                                        FM_FM10000_MAP_MAC_ROUTER)

/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapEtherType'' and
 *  ''fm10000GetMapEtherType'' and specifies the
 *  mapper setting for an EtherType mapper.
 **************************************************/
typedef struct _fm_fm10000MapETypeCfg
{
    /** EtherType key field (16 bits) */
    fm_uint16       ethType;

    /** Mapped value to be produced by the mapper for this EtherType (4 bits). */
    fm_byte         mapType;

} fm_fm10000MapETypeCfg;



/**************************************************/
/** EtherType Mapper Field Selectors
 *  \ingroup constFm10kMapETypeSel
 *  \page fm10kMapETypeSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapETypeCfg'' structure should be
 *  written to the mapper registers.
 **************************************************/
/** \ingroup constFm10kMapETypeSel
 * @{ */

/** Write EtherType field. */
#define FM_FM10000_MAP_ETYPE_TYPE        (1 << 0)

/** Write MAP_TYPE field. */
#define FM_FM10000_MAP_ETYPE_ID          (1 << 1)

/** Write all fields. */
#define FM_FM10000_MAP_ETYPE_ALL        (FM_FM10000_MAP_ETYPE_TYPE |       \
                                         FM_FM10000_MAP_ETYPE_ID)

/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapLength'' and ''fm10000GetMapLength''
 *  and specifies the mapper setting for the L3 length
 *  mapper.
 **************************************************/
typedef struct _fm_fm10000MapLenCfg
{
    /** LowerBound field (16 bits). */
    fm_uint16       length;

    /** Mapped bin (4 bits). */
    fm_byte         mapLength;

} fm_fm10000MapLenCfg;



/**************************************************/
/** L3 Length Mapper Field Selectors
 *  \ingroup constFm10kMapLenSel
 *  \page fm10kMapLenSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapLenCfg'' structure should be
 *  written to the mapper register.
 **************************************************/
/** \ingroup constFm10kMapLenSel
 * @{ */

/** Write LowerBound field. */
#define FM_FM10000_MAP_LEN_LBOUND        (1 << 0)

/** Write Bin field. */
#define FM_FM10000_MAP_LEN_BIN           (1 << 1)

/** Write all fields. */
#define FM_FM10000_MAP_LEN_ALL          (FM_FM10000_MAP_LEN_LBOUND |       \
                                         FM_FM10000_MAP_LEN_BIN)

/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapIp'' and ''fm10000GetMapIp'' and 
 *  specifies the mapper setting for a DIP or SIP
 *  mapper.
 **************************************************/
typedef struct _fm_fm10000MapIpCfg
{
    /** IP Address field. */
    fm_ipAddr    ipAddr;

    /** Ignore this many least significant bits when matching. (8 bits) */
    fm_byte      ignoreLength;

    /** Entry is valid for mapped source IP addresses. (1 bit) */
    fm_byte      validSIP;

    /** Entry is valid for mapped destination IP addresses. (1 bit) */
    fm_byte      validDIP;

    /** Mapped value to be produced by the mapper for this IP address. (4 bits) */
    fm_byte      mapIp;

} fm_fm10000MapIpCfg;



/**************************************************/
/** IP Mapper Field Selectors
 *  \ingroup constFm10kMapIpSel
 *  \page Fm10kMapIpSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapIpCfg'' structure should be
 *  written to the mapper registers.
 **************************************************/
/** \ingroup constFm10kMapIpSel
 * @{ */

/** Write IP address field. */
#define FM_FM10000_MAP_IP_ADDR          (1 << 0)

/** Write ignoreLength field. */
#define FM_FM10000_MAP_IP_IGNORE        (1 << 1)

/** Write validSIP flag. */
#define FM_FM10000_MAP_IP_VALID_SIP     (1 << 2)

/** Write validDIP flag. */
#define FM_FM10000_MAP_IP_VALID_DIP     (1 << 3)

/** Write MAP_IP field. */
#define FM_FM10000_MAP_IP_ID            (1 << 4)

/** Write all fields. */
#define FM_FM10000_MAP_IP_ALL          (FM_FM10000_MAP_IP_ADDR |           \
                                        FM_FM10000_MAP_IP_IGNORE |         \
                                        FM_FM10000_MAP_IP_VALID_SIP |      \
                                        FM_FM10000_MAP_IP_VALID_DIP |      \
                                        FM_FM10000_MAP_IP_ID)
    
/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapProt'' and ''fm10000GetMapProt''
 *  and specifies the mapper setting for a protocol
 *  mapper.
 **************************************************/
typedef struct _fm_fm10000MapProtCfg
{
    /** Protocol key field (8 bits) */
    fm_byte         protocol;

    /** Mapped value to be produced by the mapper for this Protocol (3 bits). */
    fm_byte         mapProt;

} fm_fm10000MapProtCfg;



/**************************************************/
/** Protocol Mapper Field Selectors
 *  \ingroup constFm10kMapProtSel
 *  \page fm10kMapProtSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapProtCfg'' structure should be
 *  written to the mapper registers.
 **************************************************/
/** \ingroup constFm10kMapProtSel
 * @{ */

/** Write Protocol field. */
#define FM_FM10000_MAP_PROT_TYPE        (1 << 0)

/** Write MAP_PROT field. */
#define FM_FM10000_MAP_PROT_ID          (1 << 1)

/** Write all fields. */
#define FM_FM10000_MAP_PROT_ALL        (FM_FM10000_MAP_PROT_TYPE |         \
                                        FM_FM10000_MAP_PROT_ID)

/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapL4Port'' and ''fm10000GetMapL4Port''
 *  and specifies the mapper setting for an L4 port
 *  mapper.
 **************************************************/
typedef struct _fm_fm10000MapL4PortCfg
{
    /** LowerBound field (16 bits). */
    fm_uint16       lowerBound;

    /** Mapped Protocol field (3 bits). */
    fm_byte         mapProt;

    /** Valid field (1 bit), which enables the entry. */
    fm_bool         valid;

    /** Mapped result (16 bits). */
    fm_uint16       mapL4Port;

} fm_fm10000MapL4PortCfg;



/**************************************************/
/** L4 Port Mapper Field Selectors
 *  \ingroup constFm10kMapL4PortSel
 *  \page fm10kMapL4PortSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapL4PortCfg'' structure should be
 *  written to the mapper register.
 **************************************************/
/** \ingroup constFm10kMapL4PortSel
 * @{ */

/** Write LowerBound field. */
#define FM_FM10000_MAP_L4PORT_LBOUND       (1 << 0)

/** Write Mapped Protocol field. */
#define FM_FM10000_MAP_L4PORT_MPROT        (1 << 1)

/** Write Valid flag. */
#define FM_FM10000_MAP_L4PORT_VALID        (1 << 2)

/** Write ID field. */
#define FM_FM10000_MAP_L4PORT_ID           (1 << 3)

/** Write all fields. */
#define FM_FM10000_MAP_L4PORT_ALL         (FM_FM10000_MAP_L4PORT_LBOUND |  \
                                           FM_FM10000_MAP_L4PORT_MPROT |   \
                                           FM_FM10000_MAP_L4PORT_VALID |   \
                                           FM_FM10000_MAP_L4PORT_ID)
    
/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm10000SetMapL4Port'' and ''fm10000GetMapL4Port'' 
 * to specify an L4 port mapper.
 **************************************************/
typedef enum
{
    /** Indicates FFU_MAP_L4_SRC L4 port mapper. */
    FM_FM10000_MAP_L4PORT_SRC = 0,

    /** Indicates FFU_MAP_L4_DST L4 port mapper. */
    FM_FM10000_MAP_L4PORT_DST,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_MAP_L4PORT_MAX
    
} fm_fm10000MapL4Port;



/**************************************************/
/** \ingroup typeStruct
 *  This structure is referenced by 
 *  ''fm10000SetMapVID'' and ''fm10000GetMapVID''
 *  and specifies the mapper setting for a VID
 *  mapper.
 **************************************************/
typedef struct _fm_fm10000MapVIDCfg
{
    /** Mapped value to be produced by the mapper for this vlan (12 bits). */
    fm_uint16       mapVid;

    /** Routable flag (1 bit). */
    fm_byte         routable;

} fm_fm10000MapVIDCfg;



/**************************************************/
/** VID Mapper Field Selectors
 *  \ingroup constFm10kMapVIDSel
 *  \page fm10kMapVIDSel
 *  
 *  Bit masks that indicate which field(s) of a
 *  ''fm_fm10000MapVIDCfg'' structure should be
 *  written to the mapper register.
 **************************************************/
/** \ingroup constFm10kMapVIDSel
 * @{ */

/** Write MAP_VLAN field. */
#define FM_FM10000_MAP_VID_MAPPEDVID       (1 << 0)

/** Write Routable flag. */
#define FM_FM10000_MAP_VID_ROUTABLE        (1 << 1)

/** Write all fields to hardware. */
#define FM_FM10000_MAP_VID_ALL            (FM_FM10000_MAP_VID_MAPPEDVID |  \
                                           FM_FM10000_MAP_VID_ROUTABLE)
    
/** @} (end of Doxygen group) */



/**************************************************/
/** \ingroup typeEnum
 * These enumerated values are used by
 * ''fm10000SetMapOwnership'' and 
 * ''fm10000GetMapOwnership'' to specify a mapper resources
 **************************************************/
typedef enum
{
    /** Indicates the Source Port mapper. */
    FM_FM10000_MAP_RESOURCE_SRC_PORT = 0,

    /** Indicates the Length mapper. */
    FM_FM10000_MAP_RESOURCE_LENGTH_COMPARE,

    /** Indicates the EtherType mapper. */ 
    FM_FM10000_MAP_RESOURCE_ETHTYPE,

    /** Indicates the MAC mapper. */
    FM_FM10000_MAP_RESOURCE_MAC,

    /** Indicates the IP mapper. */
    FM_FM10000_MAP_RESOURCE_IP,

    /** Indicates the protocol mapper. */
    FM_FM10000_MAP_RESOURCE_PROT,

    /** Indicates the L4 Source Port mapper. */
    FM_FM10000_MAP_RESOURCE_L4PORT_SRC,

    /** Indicates the L4 Destination Port mapper. */
    FM_FM10000_MAP_RESOURCE_L4PORT_DST,

    /** Indicates the VLAN mapper. */
    FM_FM10000_MAP_RESOURCE_VID,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_MAP_RESOURCE_MAX
    
} fm_fm10000MapResource;


/**************************************************/
/** \ingroup typeEnum
 * These enumerated values indicate the owner of
 * a Mapper resource. 
 ***************************************************/
typedef enum
{
    /** The resource is unowned. */
    FM_FM10000_MAP_OWNER_NONE = 0,

    /** The resource is owned by the application. */
    FM_FM10000_MAP_OWNER_APPLICATION,

    /** The resource is owned by the routing subsystem. */
    FM_FM10000_MAP_OWNER_ROUTING,

    /** The resource is owned by the ACL subsystem. */
    FM_FM10000_MAP_OWNER_ACL,

    /* --- Add other owners above this line --- */

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_MAP_OWNER_MAX

} fm_fm10000MapOwnerType;


/** Defines the ownership for the Mapper resources */
typedef struct _fm_fm10000MapOwnershipInfo
{
    /** For each mapping resource, the owner. */
    fm_fm10000MapOwnerType mapperOwners[FM_FM10000_MAP_RESOURCE_MAX];

} fm_fm10000MapOwnershipInfo;


/*****************************************************************************
 * Public Functions
 *****************************************************************************/
 
fm_status fm10000SetMapSourcePort(fm_int                   sw,
                                  fm_int                   port,
                                  fm_fm10000MapSrcPortCfg *mapSrcCfg,
                                  fm_uint32                fieldSelectMask,
                                  fm_bool                  useCache);

fm_status fm10000GetMapSourcePort(fm_int                   sw,
                                  fm_int                   port,
                                  fm_fm10000MapSrcPortCfg *mapSrcCfg,
                                  fm_bool                  useCache);

fm_status fm10000SetMapMac(fm_int               sw,
                           fm_int               index,
                           fm_fm10000MapMacCfg *mapMacCfg,
                           fm_uint32            fieldSelectMask,
                           fm_bool              useCache);

fm_status fm10000GetMapMac(fm_int               sw,
                           fm_int               index,
                           fm_fm10000MapMacCfg *mapMacCfg,
                           fm_bool              useCache);

fm_status fm10000SetMapEtherType(fm_int                 sw,
                                 fm_int                 index,
                                 fm_fm10000MapETypeCfg *mapETypeCfg,
                                 fm_uint32              fieldSelectMask,
                                 fm_bool                useCache);
                                
fm_status fm10000GetMapEtherType(fm_int                 sw,
                                 fm_int                 index,
                                 fm_fm10000MapETypeCfg *mapETypeCfg,
                                 fm_bool                useCache);
                         
fm_status fm10000SetMapLength(fm_int               sw,
                              fm_int               index,
                              fm_fm10000MapLenCfg *mapLenCfg,
                              fm_uint32            fieldSelectMask,
                              fm_bool              useCache);
                             
fm_status fm10000GetMapLength(fm_int               sw,
                              fm_int               index,
                              fm_fm10000MapLenCfg *mapLenCfg,
                              fm_bool              useCache);

fm_status fm10000SetMapIp(fm_int              sw,
                          fm_int              index,
                          fm_fm10000MapIpCfg *mapIpCfg,
                          fm_uint32           fieldSelectMask,
                          fm_bool             live,
                          fm_bool             useCache);

fm_status fm10000GetMapIp(fm_int              sw,
                          fm_int              index,
                          fm_fm10000MapIpCfg *mapIpCfg,
                          fm_bool             useCache);
                 
fm_status fm10000SetMapProt(fm_int                sw,
                            fm_int                index,
                            fm_fm10000MapProtCfg *mapProtCfg,
                            fm_uint32             fieldSelectMask,
                            fm_bool               useCache);

fm_status fm10000GetMapProt(fm_int                sw,
                            fm_int                index,
                            fm_fm10000MapProtCfg *mapProtCfg,
                            fm_bool               useCache);

fm_status fm10000SetMapL4Port(fm_int                  sw,
                              fm_fm10000MapL4Port     mapper,
                              fm_int                  index,
                              fm_fm10000MapL4PortCfg *mapL4PortCfg,
                              fm_uint32               fieldSelectMask,
                              fm_bool                 useCache);

fm_status fm10000GetMapL4Port(fm_int                  sw,
                              fm_fm10000MapL4Port     mapper,
                              fm_int                  index,
                              fm_fm10000MapL4PortCfg *mapL4PortCfg,
                              fm_bool                 useCache);

fm_status fm10000SetMapVID(fm_int               sw,
                           fm_uint16            vlanId,
                           fm_fm10000MapVIDCfg *mapVIDCfg,
                           fm_uint32            fieldSelectMask,
                           fm_bool              useCache);

fm_status fm10000GetMapVID(fm_int               sw,
                           fm_uint16            vlanId,
                           fm_fm10000MapVIDCfg *mapVIDCfg,
                           fm_bool              useCache);

fm_status fm10000SetMapVIDs(fm_int               sw,
                            fm_uint16            firstVlanId,
                            fm_uint16            numVlanIds,
                            fm_fm10000MapVIDCfg *mapVIDCfg,
                            fm_uint32            fieldSelectMask,
                            fm_bool              useCache);

fm_status fm10000GetMapVIDs(fm_int               sw,
                            fm_uint16            firstVlanId,
                            fm_uint16            numVlanIds,
                            fm_fm10000MapVIDCfg *mapVIDCfg,
                            fm_bool              useCache);


fm_status fm10000GetMapOwnership(fm_int                  sw,
                                 fm_fm10000MapOwnerType *owner,
                                 fm_fm10000MapResource   mapResource);

fm_status fm10000SetMapOwnership(fm_int                 sw,
                                 fm_fm10000MapOwnerType owner,
                                 fm_fm10000MapResource  mapResource);

void fm10000DbgDumpMapper(fm_int sw);
 
#endif  /* __FM_FM10000_FM_API_MAP_INT_H */

