/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_nexthop_int.h
 * Creation Date:   August 5th, 2013
 * Description:     Contains constants and functions used to support trigger.
 *
 * Copyright (c) 2007 - 2015, Intel Corporation
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

#ifndef __FM_FM10000_API_NEXTHOP_INT_H
#define __FM_FM10000_API_NEXTHOP_INT_H

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/* ARP related defines */
#define FM10000_ARP_TAB_SIZE                    FM10000_ARP_TABLE_ENTRIES
#define FM10000_ARP_BLK_CTRL_TAB_SIZE           FM10000_ARP_TABLE_ENTRIES
/* Max number of clients per ARP block */
#define FM10000_MAX_ARP_BLOCK_CLIENTS                       4
#define FM10000_ARP_BLOCK_INVALID_HANDLE                    0
#define FM10000_ARP_BLOCK_INVALID_OFFSET                    0xFFFF
#define FM10000_ARP_TAB_VERBOSE_DUMP_ENTRIES                20
#define FM10000_ARP_DUMP_LOCAL_BUFFER_SIZE                  100

/* ARP table maintenance packing parameters */
#define FM10000_ARP_MAINTENANCE_PACKING_THRESHOLD           ((FM10000_ARP_TAB_SIZE * 2) / 3)         
#define FM10000_ARP_MAINTENANCE_MAX_MOVABLE_BLOCK_LENGTH    16
#define FM10000_ARP_MAINTENANCE_NOF_RESERVED_ENTRIES        64

/* ARP table size without maintenance entries and entry 0 */
#define FM10000_ARP_TAB_AVAILABLE_ENTRIES                   (FM10000_ARP_TAB_SIZE - \
                                                             FM10000_ARP_MAINTENANCE_NOF_RESERVED_ENTRIES - 1)

/* ARP table normal packing parameters */
#define FM10000_ARP_DEFRAG_MAX_RECURSION_DEPTH              32
#define FM10000_ARP_PACKING_MAX_ITERATIONS                  1000
#define FM10000_ARP_PACKING_DEFRAG_STAGES_MAX_ITERATIONS    16
#define FM10000_ARP_PACKING_STAGES_MAX_ITERATIONS           128

/* ARP statistics: forced update trigger level */
#define FM10000_ARP_STATS_UPDATE_TRIGGER_LEVEL              4096
#define FM_10000_ARP_HISTOGRAM_MAX_LENGTH                   20

/* Arp Entry Options */
#define FM10000_ARP_BLOCK_OPT_NONE                          0
#define FM10000_ARP_BLOCK_OPT_DO_NOT_MOVE                   (1 << 0)
#define FM10000_ARP_BLOCK_OPT_SWAP_INTERMEDIATE             (1 << 1)
#define FM10000_ARP_BLOCK_OPT_SWAP_FINAL                    (1 << 2)
#define FM10000_ARP_BLOCK_OPT_SWAP      (FM10000_ARP_BLOCK_OPT_SWAP_INTERMEDIATE | \
                                         FM10000_ARP_BLOCK_OPT_SWAP_FINAL)

#define FM10000_MAX_NUM_NEXT_HOPS                           (FM10000_ARP_TABLE_ENTRIES - 2)

/* ECMP groups definitions */
#define FM10000_MAX_ECMP_GROUP_CLIENTS                      2

#define FM10000_ROUTER_ID_NO_REPLACEMENT                    15


/*****************************************************************************
 *
 *  ARP clients
 *
 *  This enum defines all possibles ARP clients.
 *  
 *****************************************************************************/
typedef enum
{
    FM10000_ARP_CLIENT_NONE,
    FM10000_ARP_CLIENT_ECMP,
    FM10000_ARP_CLIENT_LBG,
    FM10000_ARP_CLIENT_VN,
    FM10000_ARP_CLIENT_MAX
} fm10000_ArpClient;

/*****************************************************************************
 *
 *  ARP block size categories
 *
 *  This enum defines size categories groups for the ARP table.
 *  
 *****************************************************************************/
typedef enum
{
    FM10000_ARP_BLOCK_SIZE_1,
    FM10000_ARP_BLOCK_SIZE_2,
    FM10000_ARP_BLOCK_SIZE_3_TO_4,
    FM10000_ARP_BLOCK_SIZE_5_TO_8,
    FM10000_ARP_BLOCK_SIZE_9_TO_16,
    FM10000_ARP_BLOCK_SIZE_17_TO_64,
    FM10000_ARP_BLOCK_SIZE_65_TO_256,
    FM10000_ARP_BLOCK_SIZE_MORE_THAN_257,
    FM10000_ARP_BLOCK_SIZE_MAX
} fm10000_ArpBlockSizeGroups;


/*****************************************************************************
 *
 *  ARP Block Control
 *
 *  This structure contains information that describes a single ARP block
 *
 *****************************************************************************/
typedef struct _fm10000_ArpBlockCtrl
{
    /* offset defines the block location */
    fm_uint16           offset;

    /* length defines the block size */
    fm_uint16           length;

    /* block control options (mostly defag options) */
    fm_uint16           options;

    /* array of clients to be notified when the block is moved
     * first client is also the owner of the block */
    fm_uint16           clients[FM10000_MAX_ARP_BLOCK_CLIENTS];

    /* client specific variable, may be used to store
     * object IDs, etc */
    fm_uint32             opaque;
} fm10000_ArpBlockCtrl;


/*****************************************************************************
 *
 *  ARP Block Descriptor
 *
 *  This structure contains information that describes a single ARP block
 *
 *****************************************************************************/
typedef struct _fm10000_ArpBlkDscrptor
{
    /* offset defines the block position */
    fm_int              offset;

    /* length defines the size of the block */
    fm_int              length;

    /* number of clients */
    fm_uint             numClients;

    /* block flags */
    fm_uint             flags;
} fm10000_ArpBlkDscrptor;

/*****************************************************************************
 *
 *  ARP Table Descriptor
 *
 *  This structure contains information that describes a single ARP block
 *
 *****************************************************************************/
typedef struct _fm10000_ArpTabDscrptor
{
    fm_int              freeEntries;
    fm_int              usageRatio;
    fm_int              largstBlkOffset;
    fm_int              largstBlklngth;
    fm_int              lastAllocatedBlk;
    fm_int              fragIndx;
    fm_int              tabDefragOptions;
} fm10000_ArpTabDscrptor;


/*****************************************************************************
 *
 *  ECMP group type
 *
 *  This enum defines the possible types of ECMP groups.
 *  
 *****************************************************************************/
typedef enum _fm10000_EcmpGroupType
{
    /* undefined type of ECMP group */
    FM10000_ECMP_GROUP_TYPE_UNDEFINED,

    /* "normal" unicast ECMP group */
    FM10000_ECMP_GROUP_TYPE_NORMAL_UNICAST,

    /* "drop" multicast ECMP group */
    FM10000_ECMP_GROUP_TYPE_DROP_MCAST,

    /* "normal" multicast ECMP group */
    FM10000_ECMP_GROUP_TYPE_NORMAL_MCAST,

    /* "normal" multicast ECMP group */
    FM10000_ECMP_GROUP_TYPE_MULTICAST_MCAST

} fm10000_EcmpGroupType;


/*****************************************************************************
 *
 *  ECMP Group clients
 *
 *  This enum defines additional ECMP group clients.
 *  
 *****************************************************************************/
typedef enum
{
    FM10000_ECMP_GROUP_CLIENT_NONE,
    FM10000_ECMP_GROUP_CLIENT_ACL,
    FM10000_ECMP_GROUP_CLIENT_MAX
} fm10000_EcmpGroupClient;


/*****************************************************************************
 *
 *  ECMP Group Next Hop Entry
 *
 *  This structure describes an FM10000 implementation of a next-hop entry
 *  for an ECMP group.
 *
 *****************************************************************************/
typedef struct _fm10000_NextHop
{
    /* pointer to the parent NextHop structure */
    fm_intNextHop * pParent;

    /* associated ARP block */
    fm_uint16       arpBlkHndl;

    /* entry's relative offset to the beginning of the ARP block */
    fm_uint16       arpBlkRelOffset;

    /* arp table data */
    fm_uint64       arpData[FM_RAW_WIDE_NEXTHOP_SIZE];
} fm10000_NextHop;


/*****************************************************************************
 *
 *  ECMP Group
 *
 *  This structure describes an FM10000 implementation of an ECMP group.
 *
 *****************************************************************************/
typedef struct _fm10000_EcmpGroup
{
    /* pointer to the parent (high level) ECMP group structure */
    fm_intEcmpGroup *       pParent;

    /* ECMP group type, see fm10000_EcmpGroupType for more information */
    fm10000_EcmpGroupType   groupType;

    /* handler of the associated ARP block. It is equal to
     * FM10000_ARP_BLOCK_INVALID_HANDLE if no ARP block has
     * been allocated  */
    fm_int                  arpBlockHandle;

    /* table of clients that use this ECMP group. This table is used to send 
       notifications when nextHops are moved during table defragmentations      */
    fm_uint16               ecmpClients[FM10000_MAX_ECMP_GROUP_CLIENTS];

    /* used ARP entries for this ecmp group */
    fm_int                  activeArpCount;

    /* Glort ARP data flag */
    fm_bool                 useGlortArpData;

    /* Glort ARP data */
    fm_uint64               glortArpData;
} fm10000_EcmpGroup;

/*****************************************************************************
 *
 *  NextHopSys Control
 *
 *  This structure contains information that describes and controls the
 *  NextNop subsystem operation.
 *
 *****************************************************************************/
typedef struct _fm10000_NextHopSysCtrl
{
    /************************* 
     *  ARP table section
     *************************/

    /* pArpHndlArray points to an array that keeps the block handlers associated
     * to each ARP entry. It is indexed using the chip ARP table offset.
     * The size of the array is equal to the ARP table (=FM10000_ARP_TAB_SIZE) 
     * Note that this is a pointer to an array and NOT the array itself, which 
     * will be allocated dynamically                                            */ 
    fm_uint16              (*pArpHndlArray)[FM10000_ARP_TAB_SIZE];

    /* pArpBlkHndlTab points to an array of pointers to ARP-block control structs
     * and it is indexed using the block handlers.
     * This table is allocated on demand the first time the ARP table is used.
     * Note that this is a pointer to an array of pointers and NOT the array itself, 
     * which will be allocated dynamically                                       */ 
    fm10000_ArpBlockCtrl * (*ppArpBlkCtrlTab)[FM10000_ARP_BLK_CTRL_TAB_SIZE];

    /* number of free entries */
    fm_int                 arpTabFreeEntryCount;

    /* index of the first free entry in the ARP table */
    fm_int                 arpHndlTabFirstFreeEntry;

    /* index of the upper used entry in the ARP table */
    fm_int                 arpHndlTabLastUsedEntry;

    /* index of the last allocated position in the ARP ctrl table */
    fm_uint                lastArpBlkCtrlTabLookupNdx;

    /* aging counter of the statistical data related with the ARP table.
     * Counters and statistical data related wiht the ARP table are
     *  incrementally updated during alloc/release/move block operations.
     *  the aging counter allow to sync with the absolute values
     *  after a given number of transactions
     */
    fm_int                 arpStatsAgingCounter;

    /* statistical info about the allocated blocks:
     *  allocatedBlkHistogramInfo[0] = number of blocks with 1 entry
     *  allocatedBlkHistogramInfo[1] = number of blocks with 2 entries
     *  allocatedBlkHistogramInfo[2] = number of blocks with 3-4 entries
     *  allocatedBlkHistogramInfo[3] = number of blocks with 5-8 entries
     *  allocatedBlkHistogramInfo[4] = number of blocks with 9-16 entries
     *  allocatedBlkHistogramInfo[5] = number of blocks with 17-64 entries
     *  allocatedBlkHistogramInfo[6] = number of blocks with 65-256 entries
     *  allocatedBlkHistogramInfo[7] = number of blocks with more that 257 entries
     */
    fm_uint16              allocatedBlkStatInfo[8];

    /* statistical info about the free blocks.
     * This information is used to determine the level of fragmentation of the table 
     *  freeBlkHistogramInfo[0] = number of blocks with 1 free entry
     *  freeBlkHistogramInfo[1] = number of blocks with 2 free entries
     *  freeBlkHistogramInfo[2] = number of blocks with 3-4 free entries
     *  freeBlkHistogramInfo[3] = number of blocks with 5-8 free entries
     *  freeBlkHistogramInfo[4] = number of blocks with 9-16 free entries
     *  freeBlkHistogramInfo[5] = number of blocks with 17-64 free entries
     *  freeBlkHistogramInfo[6] = number of blocks with 65-256 free entries
     *  freeBlkHistogramInfo[7] = number of blocks with more that 257 free entries
     */
    fm_uint16              freeBlkStatInfo[8];


    /************************* 
     *  ECMP section
     *************************/

     /* pEcmpGroupsHL points to an array of pointers to the low level
      *  ECMP control structures                                            */
    fm10000_EcmpGroup     (*pEcmpGroupsHL)[FM10000_MAX_NUM_NEXT_HOPS];

} fm10000_NextHopSysCtrl;

/*****************************************************************************
 * Internal Function Prototypes.
 *****************************************************************************/

/* initialization/termination group */
fm_status fm10000NextHopAlloc (fm_int sw);
fm_status fm10000NextHopFree (fm_int sw);
fm_status fm10000NextHopInit (fm_int sw);
fm_status fm10000NextHopCleanTable (fm_int sw);

/* ARP group */
fm_status fm10000InitArpBlockControl (fm_int sw);
fm_status fm10000CleanupArpBlockControl (fm_int sw);
fm_status fm10000RequestArpBlock (fm_int            sw,
                                  fm10000_ArpClient client,
                                  fm_int            arpCount,
                                  fm_uint           arpBlkOptions,
                                  fm_uint16 *       pArpBlkHndl);
fm_status fm10000FreeArpBlock    (fm_int            sw,
                                  fm10000_ArpClient client,
                                  fm_uint16         arpBlkHndl);
fm_status fm10000RegisterArpBlockClient   (fm_int             sw,
                                           fm_uint16          arpBlkHndl,
                                           fm10000_ArpClient  newClient);
fm_status fm10000UnregisterArpBlockClient (fm_int             sw,
                                           fm_uint16          arpBlkHndl,
                                           fm10000_ArpClient  client);
fm_status fm10000CopyArpBlockEntries (fm_int            sw,
                                      fm10000_ArpClient client,
                                      fm_uint16         srcArpBlkHndl,
                                      fm_uint16         dstArpBlkHndl,
                                      fm_int            entryCount,
                                      fm_int            srcOffset,
                                      fm_int            dstOffset);
fm_status fm10000GetArpEntryUsedStatus (fm_int    sw,
                                        fm_int    arpIndex,
                                        fm_bool   resetFlag,
                                        fm_bool * pUsed);
fm_status fm10000ResetAllArpEntryUsedStatusFlags (fm_int  sw);
fm_status fm10000GetArpBlockInfo (fm_int                   sw,
                                  fm10000_ArpClient        client,
                                  fm_uint16                arpBlkHndl,
                                  fm10000_ArpBlkDscrptor * pArpBlkDscrptor);
fm_status fm10000GetArpTabInfo (fm_int                  sw,
                                fm10000_ArpTabDscrptor *pArpTabDscriptor);
fm_status fm10000NotifyArpBlockChange (fm_int    sw,
                                       fm_uint16 arpBlckHndl,
                                       fm_uint16 (*pClientToNotify)[FM10000_MAX_ARP_BLOCK_CLIENTS],
                                       fm_int    newBlockOffset,
                                       fm_int    oldBlockOffset);
fm_status fm10000CheckValidArpBlockSize (fm_int  blockSize);
fm_status fm10000DefragArpTable (fm_int     sw);

/* interface group */

fm_status fm10000SetInterfaceAttribute (fm_int sw,
                                        fm_int interface,
                                        fm_int attr,
                                        void * pValue);

/* ECMP's next-hop group */
fm_status fm10000AddECMPGroupNextHops    (fm_int           sw,
                                          fm_intEcmpGroup *pEcmpGroup,
                                          fm_int           numNextHops,
                                          fm_ecmpNextHop  *pNextHopList);
fm_status fm10000DeleteECMPGroupNextHops (fm_int           sw,
                                          fm_intEcmpGroup *pEcmpGroup,
                                          fm_int           numRemovedHops,
                                          fm_intNextHop ** ppHopsToRemove,
                                          fm_int           numNextHops,
                                          fm_ecmpNextHop * pNextHopList);
fm_status fm10000ReplaceECMPGroupNextHop (fm_int           sw,
                                          fm_intEcmpGroup *pEcmpGroup,
                                          fm_intNextHop *  pOldNextHop,
                                          fm_intNextHop *  pNewNextHop);
fm_status fm10000UpdateEcmpGroup (fm_int           sw,
                                  fm_intEcmpGroup *pEcmpGroup);
fm_status fm10000SetECMPGroupNextHops    (fm_int           sw,
                                          fm_intEcmpGroup *pEcmpGroup,
                                          fm_int           firstIndex,
                                          fm_int           numNextHops,
                                          fm_ecmpNextHop * pNextHopList);
fm_status fm10000UpdateNextHop(fm_int         sw, 
                               fm_intNextHop *pNextHop);
fm_status fm10000UpdateNextHopMulticast(fm_int sw, fm_int ecmpGroup);
fm_status fm10000GetECMPGroupNextHopIndexRange (fm_int           sw,
                                                fm_intEcmpGroup *pEcmpGroup,
                                                fm_int *         pFirstIndex,
                                                fm_int *         pLastIndex);
fm_status fm10000GetNextHopUsed(fm_int         sw,
                                fm_intNextHop *pNextHop,
                                fm_bool *      pUsed,
                                fm_bool        resetFlag);
fm_status fm10000GetNextHopIndexUsed (fm_int   sw,
                                      fm_int   index,
                                      fm_bool *pUsed,
                                      fm_bool  resetFlag);
fm_status fm10000ValidateNextHopTrapCode(fm_int      sw,
                                         fm_nextHop *nextHop);

/* ECMP group */
fm_status fm10000CreateECMPGroup (fm_int           sw,
                                  fm_intEcmpGroup *pEcmpGroup);
fm_status fm10000DeleteECMPGroup (fm_int           sw,
                                  fm_intEcmpGroup *pEcmpGroup);
fm_status fm10000FreeECMPGroup   (fm_int           sw, 
                                  fm_intEcmpGroup *pEcmpGroupHL);
fm_status fm10000GetECMPGroupARPUsed (fm_int           sw,
                                      fm_intEcmpGroup *pEcmpGroup,
                                      fm_bool *        pUsed,
                                      fm_bool          resetFlag);
fm_status fm10000GetECMPGroupArpInfo (fm_int     sw,
                                      fm_int     ecmpGroupId,
                                      fm_uint16 *pHandle,
                                      fm_int *   pIndex,
                                      fm_int *   pPathCount,
                                      fm_int *   pPathCountType);
fm_status fm10000RegisterEcmpClient (fm_int                   sw,
                                     fm_int                   ecmpGroupId,
                                     fm10000_EcmpGroupClient  newClient);
fm_status fm10000UnregisterEcmpClient (fm_int                  sw,
                                       fm_int                  ecmpGroupId,
                                       fm10000_EcmpGroupClient client);
fm_status fm10000NotifyEcmpGroupChange (fm_int   sw,
                                        fm_int   ecmpGroupId,
                                        fm_int   oldIndex);
fm_status fm10000GetArpTablePathCountParameters (fm_int  arpBlkSize,
                                                 fm_int *pPathCount,
                                                 fm_int *pPathCountType);
fm_status fm10000ValidateDeleteEcmpGroup (fm_int   sw,
                                          fm_int   ecmpGroupId,
                                          fm_bool *pMayBeDeleted);
fm_status fm10000ValidateEcmpGroupState (fm_int   sw,
                                         fm_int   ecmpGroupId,
                                         fm_bool *pValid);
fm_status fm10000GetECMPGroupArpInfo (fm_int     sw,
                                      fm_int     ecmpGroupId,
                                      fm_uint16 *pHandle,
                                      fm_int *   pIndex,
                                      fm_int *   pPathCount,
                                      fm_int *   pPathCountType);
fm_status fm10000SetPriorityUnresolvedNextHopTrigger (fm_int   sw,
                                                      fm_int   priority);

/* Debug functions */
void fm10000DbgPrintArpTableInfo (fm_int  sw);
void fm10000DbgDumpArpTable (fm_int  sw,
                             fm_bool verbose);
void fm10000DbgDumpArpTableExtended (fm_int  sw);
void fm10000DbgDumpArpHandleTable (fm_int  sw, 
                                   fm_bool verbose);
void fm10000DbgPlotUsedArpDiagram (fm_int  sw);
void fm10000DbgDumpEcmpTables (fm_int  sw);
void fm10000DbgDumpArpUsedBlockStats (fm_int  sw);
void fm10000DbgDumpArpFreeBlockStats (fm_int  sw);
void fm10000DbgPrintArpFragmentationInfo (fm_int  sw);



#endif      /* end #ifndef __FM_FM10000_API_NEXTHOP_INT_H */

