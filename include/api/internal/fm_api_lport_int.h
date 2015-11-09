/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lport_int.h
 * Creation Date:   2005
 * Description:     Contains constants related to logical ports.
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

#ifndef __FM_FM_API_LPORT_INT_H
#define __FM_FM_API_LPORT_INT_H

/****************************************************************************/
/** \ingroup intTypeEnum
 *
 * Identifies the type of port destination that a logical port maps to.
 ****************************************************************************/
typedef enum
{
    /** Allocate an entry for a single physical port on the local switch.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_PHYSICAL = 0,

    /** Allocate a block of entries for a LAG.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_LAG,

    /** Allocate an entry for multicasting.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_MULTICAST,

    /** Allocate an entry for load balancing.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_LBG,

    /** Allocate an entry for the CPU port.
     *  
     *  \chips  FM2000, FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_CPU,

    /** Allocate an entry for the CPU Mgmt (FIBM) port.
     *  This type only has a logical representation.
     *  
     *  \chips  FM4000 */
    FM_PORT_TYPE_CPU_MGMT,

    /** Allocate an entry for the CPU Mgmt (FIBM) port.
     *  This type has both logical and physical port representations.
     *  
     *  \chips  FM10000 */
    FM_PORT_TYPE_CPU_MGMT2,

    /** Allocate an entry for a custom port type.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_SPECIAL,

    /** Allocate an entry for a remote port type.
     *  
     *  \chips  FM4000, FM6000, FM10000 */
    FM_PORT_TYPE_REMOTE,

    /** Allocate an entry for Tunnel Engine ports.
     *  
     *  \chips  FM10000 */
    FM_PORT_TYPE_TE,

    /** Allocate an entry for loopback ports.
     *  
     *  \chips  FM10000 */
    FM_PORT_TYPE_LOOPBACK,

    /** Allocate an entry for a virtual port. A virtual port is only 
     *  identifiable/reachable using a glort. Virtual ports are typically
     *  used to identify PF or VF (e.g. VM) on an SR-IOV device.
     *  
     *  \chips  FM10000 */
    FM_PORT_TYPE_VIRTUAL,

    /** Allocate an entry for a PTI (Packet Test Interface) port.  
     *
     *  \chips  FM10000 */
    FM_PORT_TYPE_PTI,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_PORT_TYPE_MAX

} fm_portType;


/****************************************************************************/
/** \ingroup intTypeEnum
 *
 * Specifies types of egress timestamps that can be sent to a logical port 
 * (belonging to a PCIE port) via Mailbox.
 ****************************************************************************/
typedef enum
{
    /* No timestamps are sent to logical port. */
    FM_PORT_TX_TIMESTAMP_MODE_NONE = 0,

    /* Only egress timestamps of packets sent from a PCIE port to another 
     * PCIE port. */
    FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_PEP,    

    /* Egress timestamps of packets sent from a PCIE port to another PCIE 
     * port and ethernet port. */
    FM_PORT_TX_TIMESTAMP_MODE_PEP_TO_ANY

} fm_portTxTimestampMode;


/**************************************************/
/** \ingroup intPort
 *
 * Specifies whether to update glort CAM, glort RAM,
 * or both CAM and RAM.
 **************************************************/
typedef enum
{
    FM_UPDATE_CAM_ONLY      = 1,
    FM_UPDATE_RAM_ONLY      = 2,
    FM_UPDATE_CAM_AND_RAM   = 3,

} fm_camUpdateMode;




/***************************************************
 * Describes a block of glorts as defined by the
 * RAM/CAM entry that defines it. 
 * (FM4000/FM6000/FM10000)
 **************************************************/
typedef struct _fm_glortCamEntry
{
    /** Stores the index in CAM/RAM that holds this entry. */
    fm_uint32   camIndex;

    /** Indicates the mask and key to be used for searching the CAM */
    fm_uint32   camKey;
    fm_uint32   camMask;

    /** Indicates if the destination mask is hashed across the range */
    fm_uint32   strict;

    /** The first glort index in the block */
    fm_uint32   destIndex;

    /** The number of glorts in the block */
    fm_uint32   destCount;

    /** Defines a bit range in the glort number */
    fm_uint32   rangeAOffset;
    fm_uint32   rangeALength;

    /** (For strict only) defines a bit range in the glort number */
    fm_uint32   rangeBOffset;
    fm_uint32   rangeBLength;

    /** The hash rotation used for the hashing */
    fm_uint32   hashRotation;

    /** (FM6000 only) 1-bit tag associated with DGLORT. */
    fm_uint32   dglortTag;

    /** Indicates whether this entry is used */
    fm_int      useCount;

} fm_glortCamEntry;


/***************************************************
 * Describes a glort destination mask entry. (FM4000/FM6000/FM10000)
 **************************************************/
typedef struct
{
    /** The actual destination index in the dest table in use. */
    fm_uint32           destIndex;

    /** The destination mask associated with this entry. */
    fm_portmask         destMask;

    /** The IP multicast index associated with this entry. */
    fm_uint32           multicastIndex;

    /** The cam entry associated with this dest entry. */
    fm_glortCamEntry *  owner;

    /** Indicates whether this entry is used, and by which port type. */
    fm_byte             usedBy;

} fm_glortDestEntry;


/**
 * Port parameters structure. 
 *  
 * Used during logical port allocation to keep track of the resources that 
 * have been assigned to the port. 
 */
typedef struct
{
    /** Type of logical port being allocated. */
    fm_portType portType;

    /** Number of logical ports to allocate. */
    fm_int      numPorts;

    /** First logical port number. */
    fm_int      basePort;

    /** First glort number. */
    fm_int      baseGlort;

    /** Index of the glort cam entry. */
    fm_uint32   camIndex;

    /** Index of the first entry in the dmask table. */
    fm_int      baseDestIndex;

    /** Number of entries in the dmask table. */
    fm_int      numDestEntries;

    /** Handle specifying a previously allocated resource block.
     *  This is mainly used in stacking configurations. */
    fm_int      useHandle;

    /* Number of logical ports to delete when port is freed. */
    fm_int      freeCount;

} fm_portParms;

/***************************************************
 * Multicast resource definitions.
 **************************************************/
/* max number of cam entries per allocated resource */
#define FM_RESV_MCAST_CAM_MAX           64

/* max entries per cam as a power of 2 */
#define FM_MCAST_MAX_ENTRY_PER_CAM      8
#define FM_MCG_MAX_ENTRIES_PER_GLORT    (1 << FM_MCAST_MAX_ENTRY_PER_CAM)

/* max number of allocated muticast groups */
#define FM_MCG_ALLOC_TABLE_SIZE         8


/***************************************************
 * Keeps track of the resources that have been allocated
 * for multicast groups (MCGs).
 **************************************************/
typedef struct 
{
    fm_uint     baseGlort;
    /* If glortSize == 0, then this entry is invalid */
    fm_uint     glortSize;
    fm_int      baseHandle;
    fm_int      numHandles;

    /* CAM indexes assigned to this block of glorts. */
    fm_uint32   mcgCamIndex[FM_RESV_MCAST_CAM_MAX];

    /* Destination indexes assigned to this block of glorts. */
    fm_uint32   mcgDestIndex[FM_RESV_MCAST_CAM_MAX];

    /* Manages glort dest table entry usage. */
    fm_bitArray      dstInUse[FM_RESV_MCAST_CAM_MAX];

} fm_mcgAllocEntry;


/***************************************************
 * Keeps track of resources that have been allocated
 * for link aggregation groups (LAGs).
 **************************************************/
typedef struct 
{
    fm_uint     baseGlort;
    /* If glortSize == 0, then this entry is invalid */
    fm_uint     glortSize;
    fm_int      baseHandle;
    fm_int      numHandles;
    fm_int      numPorts;

} fm_lagAllocEntry;

/* max number of allocated LAGs */
#define FM_LAG_ALLOC_TABLE_SIZE         FM_ALLOC_LAGS_MAX


/***************************************************
 * Keeps track of resources that have been allocated
 * for load-balancing groups (LBGs).
 **************************************************/
typedef struct 
{
    fm_uint     baseGlort;
    /* If glortSize == 0, then this entry is invalid */
    fm_uint     glortSize;
    fm_int      baseHandle;
    fm_int      numHandles;
    fm_int      numPorts;

} fm_lbgAllocEntry;

/* max number of allocated LBGs */
#define FM_LBG_ALLOC_TABLE_SIZE         16


/***************************************************
 * State structure to keep track of the free space
 * list in the glort RAM/CAM.
 **************************************************/
typedef struct
{
    /***************************************************
     * Manages the glort CAM itself (and the associated
     * RAM).
     **************************************************/
    fm_glortCamEntry *  camEntries;
    fm_int              numCamEntries;

    /***************************************************
     * Manages the glort dest table.
     **************************************************/
    fm_glortDestEntry * destEntries;
    fm_int              numDestEntries;

    /***************************************************
     * Array to indicate whether a glort is free or
     * reserved for a particular use. We need this
     * because the glort space may be bigger than 
     * the physical glort space available.
     **************************************************/
    fm_byte             glortState[FM_MAX_GLORT+1];

    /***************************************************
     * Array to indicate whether a logical port is
     * free or reserved for a particular use.
     **************************************************/
    fm_byte             lportState[FM_MAX_LOGICAL_PORT+1];

    fm_uint32           physicalPortCamIndex;
    fm_uint32           specialPortCamIndex;
    fm_uint32           cpuPortCamIndex;
    fm_uint32           cpuMgmtPortCamIndex;
    fm_uint32           lagPortCamIndex;

    /* special logical ports */
    fm_int              floodPort;
    fm_int              floodCpuPort;
    fm_int              bcastPort;
    fm_int              dropPort;
    fm_int              rpfFailurePort;
    fm_int              cpuMgmtPort; /* For fibm when cpuPort is redirected */

    /* Resources allocated for multicast groups (MCGs). */
    fm_mcgAllocEntry    mcgAllocTable[FM_MCG_ALLOC_TABLE_SIZE];

    /* Resources allocated for link aggregate groups (LAGs). */
    fm_lagAllocEntry    lagAllocTable[FM_LAG_ALLOC_TABLE_SIZE];

    /* Resources allocated for load-balancing groups (LBGs). */
    fm_lbgAllocEntry    lbgAllocTable[FM_LBG_ALLOC_TABLE_SIZE];

} fm_logicalPortInfo;




/*******************************************************************
 * State bits for the lportState array.
 *******************************************************************/

#define FM_LPORT_STATE_IN_USE       0x01    /* In use */
#define FM_LPORT_STATE_RESV_MCG     0x02    /* Reserved for MCG */
#define FM_LPORT_STATE_RESV_LAG     0x04    /* Reserved for LAG */
#define FM_LPORT_STATE_RESV_LBG     0x08    /* Reserved for LBG */
#define FM_LPORT_STATE_FREE_PEND    0x10    /* Free pending */


/*******************************************************************
 * Macros to manipulate the lportState array to check whether a logical
 * port is reserved or in use.
 *******************************************************************/

#define FM_RELEASE_LPORT(info, port)        \
        (info)->lportState[port] = 0

#define FM_SET_LPORT_FREE(info, port)       \
        (info)->lportState[port] &= ~(FM_LPORT_STATE_IN_USE | \
                                      FM_LPORT_STATE_FREE_PEND)

#define FM_SET_LPORT_IN_USE(info, port)     \
        (info)->lportState[port] |= FM_LPORT_STATE_IN_USE

#define FM_RESERVE_LPORT_MCG(info, port)    \
        (info)->lportState[port] |= FM_LPORT_STATE_RESV_MCG

#define FM_RESERVE_LPORT_LAG(info, port)    \
        (info)->lportState[port] |= FM_LPORT_STATE_RESV_LAG

#define FM_RESERVE_LPORT_LBG(info, port)    \
        (info)->lportState[port] |= FM_LPORT_STATE_RESV_LBG

#define FM_IS_LPORT_RSVD_FOR_LBG(info, port) \
        (((info)->lportState[port] & FM_LPORT_STATE_RESV_LBG) != 0)

/* Either used or reserved */
#define FM_IS_LPORT_TAKEN(info, port)       \
        ((info)->lportState[port] != 0)

/* Check for both free and reserved */
#define FM_IS_LPORT_MCG_FREE(info, port)    \
        ((info)->lportState[port] == FM_LPORT_STATE_RESV_MCG)

#define FM_IS_LPORT_LAG_FREE(info, port)    \
        ((info)->lportState[port] == FM_LPORT_STATE_RESV_LAG)

#define FM_IS_LPORT_LBG_FREE(info, port)    \
        ((info)->lportState[port] == FM_LPORT_STATE_RESV_LBG)

/* Due to the problem of another thread is freeing the LAG, thus preventing
 * the code to free the allocated LAGs right after deleting all the LAGs
 * in the allocated groups, so we will need this so we can free the allocated
 * groups
 */
#define FM_SET_LPORT_FREE_PEND(info, port)  \
        (info)->lportState[port] |= FM_LPORT_STATE_FREE_PEND

#define FM_IS_LPORT_FREE_PEND(info, port)   \
        (((info)->lportState[port] & FM_LPORT_STATE_FREE_PEND) != 0)


/***************************************************
 * Utility macros.
 **************************************************/

#define FM_USED_BY_TYPE(type)   (type | 0x80)


/***************************************************
 * Function prototypes.
 **************************************************/

fm_status fmAllocLogicalPort(fm_int      sw,
                             fm_portType type,
                             fm_int      numPorts,
                             fm_int *    firstPortNumber);

fm_status fmFreeLogicalPort(fm_int sw, fm_int port);

fm_status fmSetLogicalPortAttribute(fm_int sw,
                                    fm_int port,
                                    fm_int attr,
                                    void * value);

fm_status fmCreateLogicalPortForGlort(fm_int    sw,
                                      fm_uint32 glort,
                                      fm_int *  logicalPort);

fm_status fmCreateLogicalPortForMailboxGlort(fm_int    sw,
                                             fm_uint32 glort,
                                             fm_int *  logicalPort);

fm_status fmFreeLogicalPortForMailboxGlort(fm_int sw,
                                           fm_int logicalPort);

fm_status fmGetLogicalPortGlort(fm_int sw, fm_int logicalPort, fm_uint32 *glort);
fm_status fmGetGlortLogicalPort(fm_int sw, fm_uint32 glort, fm_int *logicalPort);
fm_status fmGetLogicalPortRange(fm_int sw, fm_int *portRange);

fm_bool fmIsLagPort(fm_int sw, fm_int port);
fm_bool fmIsRemotePort(fm_int sw, fm_int logicalPort);
fm_bool fmIsInternalPort(fm_int sw, fm_int logicalPort);
fm_bool fmIsMgmtPort(fm_int sw, fm_int logicalPort);
fm_bool fmIsVirtualPort(fm_int sw, fm_int logicalPort);
fm_bool fmIsPortLinkUp(fm_int sw, fm_int logicalPort);
fm_bool fmIsValidPort(fm_int sw, fm_int port, fm_int mode);

fm_status fmSortPortByGlort(fm_int sw,
                            fm_int *ports,
                            fm_int nPorts,
                            fm_int *sortedPorts);

fm_status fmAllocDestEntries(fm_int              sw,
                             fm_int              numDestEntries,
                             fm_glortCamEntry *  camEntry,
                             fm_glortDestEntry **destEntry,
                             fm_portType         type);

fm_status fmCreateGlortCamEntry(fm_int     sw,
                                fm_uint32  camMask,
                                fm_uint32  camKey,
                                fm_uint32  strict,
                                fm_uint32  baseIndex,
                                fm_uint32  destCount,
                                fm_uint32  rangeALength,
                                fm_uint32  rangeAOffset,
                                fm_uint32  rangeBLength,
                                fm_uint32  rangeBOffset,
                                fm_uint32  hashRotation,
                                fm_uint32  dglortTag,
                                fm_uint32 *camIndexPtr);

fm_int fmFindFreeLagEntry(fm_int sw);
fm_int fmFindFreeLbgEntry(fm_int sw);
fm_int fmFindFreeMcgEntry(fm_int sw);

fm_lagAllocEntry * fmFindLagEntryByHandle(fm_int sw, fm_int handle);
fm_lbgAllocEntry * fmFindLbgEntryByHandle(fm_int sw, fm_int handle);
fm_mcgAllocEntry * fmFindMcgEntryByHandle(fm_int sw, fm_int handle);

fm_int fmFindUnusedCamEntry(fm_int sw);

fm_int fmFindUnusedDestEntries(fm_int   sw,
                               fm_int   numEntries,
                               fm_int   first);

fm_int fmFindUnusedGlorts(fm_int            sw,
                          fm_int            numGlorts,
                          fm_int            first,
                          fm_glortCamEntry *camEntry);

fm_int fmFindUnusedLogicalPorts(fm_int sw, fm_int numPorts);

fm_status fmFindUnusedLagGlorts(fm_int     sw,
                                fm_int     numPorts,
                                fm_int     useHandle,
                                fm_int     glortsPerLag,
                                fm_int *   logicalPort,
                                fm_uint32 *firstGlort);

fm_status fmFindUnusedLbgGlorts(fm_int     sw,
                                fm_int     numPorts,
                                fm_int     useHandle,
                                fm_int *   logicalPort,
                                fm_uint32 *firstGlort);

fm_status fmFindUnusedMcgGlorts(fm_int             sw,
                                fm_int             numPorts,
                                fm_int             useHandle,
                                fm_mcgAllocEntry **allocEntryPtr,
                                fm_uint32 *        offBasePtr);

const char * fmGetPortTypeAsText(fm_int sw, fm_int port);

const char * fmPortTypeToText(fm_portType type);

const char * fmSpecialPortToText(fm_int port);

fm_status fmRemoveGlortCamEntry(fm_int sw, fm_uint32 camIndex);

void fmResetLogicalPortInfo(fm_logicalPortInfo *lportInfo);

fm_status fmFreeMcastLogicalPort(fm_int sw, fm_int port);

fm_status fmInitializeLogicalPorts(fm_int sw);

fm_status fmAllocateLogicalPortDataStructures(fm_int sw,
                                              fm_int numCamEntries,
                                              fm_int numDestEntries);

fm_status fmFreeLogicalPortDataStructures(fm_switch *switchPtr);

fm_status fmFreeLogicalPortResources(fm_int sw);

fm_status fmFreeLaneResources(fm_int sw);

fm_status fmCommonAllocLogicalPort(fm_int      sw,
                                   fm_portType portType,
                                   fm_int      numPorts,
                                   fm_int *    firstPort,
                                   fm_int      useHandle);

fm_status fmCommonAllocVirtualLogicalPort(fm_int  sw,
                                          fm_int  numPorts,
                                          fm_int *firstPort,
                                          fm_int  useHandle,
                                          fm_int  firstGlort);

fm_status fmCommonFreeLogicalPort(fm_int sw, fm_int logicalPort);

fm_status fmAllocateMcastHandles(fm_int     sw,
                                 fm_uint    startGlort,
                                 fm_int     glortSize,
                                 fm_int    *baseMcastGroupHandle,
                                 fm_int    *numMcastGroups,
                                 fm_int    *step);

fm_status fmFreeMcastHandles(fm_int sw, fm_int handle);

fm_status fmAllocateLagHandles(fm_int   sw,
                               fm_uint  startGlort,
                               fm_int   glortSize,
                               fm_int   glortsPerLag,
                               fm_int   lagMaskSize,
                               fm_int * baseLagHandle,
                               fm_int * numLags,
                               fm_int * step);

fm_status fmFreeLagHandles(fm_int sw, fm_int handle);

fm_status fmAllocateLbgHandles(fm_int   sw,
                               fm_uint  startGlort,
                               fm_int   glortSize,
                               fm_int * baseLbgHandle,
                               fm_int * numLbgs,
                               fm_int * step);

fm_status fmFreeLbgHandles(fm_int sw, fm_int handle);

fm_bool fmIsInLAGGlortRange(fm_int sw, fm_uint32 glort);

fm_status fmDbgDumpGlortConfig(fm_int sw);



#endif /* __FM_FM_API_LPORT_INT_H */
