/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_mtable.c
 * Creation Date:   Mar 4, 2014
 * Description:     Contains internal functions for manipulating the IP
 *                  multicast table.
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

#define GET_MTABLE_INFO(sw) \
    &((fm10000_switch *)(GET_SWITCH_EXT(sw)))->mtableInfo;

#define VLAN_TABLE_MIN_BLOCK_SIZE 1
#define LEN_TABLE_MIN_BLOCK_SIZE 1

/* Copy the dest mask from/to a portmask */
#define COPY_PORTMASK_TO_DESTMASK(portmask,dest)   \
dest = portmask.maskWord[0] | (fm_uint64)portmask.maskWord[1] << 32;

#define COPY_DESTMASK_TO_PORTMASK(dest,portmask)     \
portmask.maskWord[0] = dest & 0xFFFFFFFF;                      \
portmask.maskWord[1] = (dest >> 32) & 0xFFFF;                      \
portmask.maskWord[2] = 0;

#define GET_LISTENER_KEY(port, vlan) \
    ( ( ( (fm_uint64) port ) << 12 ) | ( (fm_uint64) vlan ) )

/* The value is approximately 10th of the time required to 
 * flush all memory segments when all segments are to be egressed on 
 * single port of lowest speed(1.25Gbps). The idea is that within 10 
 * tries Previous EPOCH counter should go to 0 since by that time
 * all the segments in the memory should be scheduled. */
#define EPOCH_USAGE_SCAN_INTERVAL 3000000

/* definition of a structure of listeners count */
typedef struct _MTableListenersCount
{
    /* total number of listeners on a (mcastGroup, port) pair */
    fm_int total;

    /* total number of active listeners on a (mcastGroup, port) pair */
    fm_int active;

} fm10000_MTableListenersCount;


typedef struct _MTableGroupInfo
{
    /* replication group number */
    fm_int repliGroup;

    /* mcast group number (mcast logicalPort) */
    fm_int mcastGroup;

} fm10000_MTableGroupInfo;


typedef enum 
{
    LEN_TABLE = 0,
    VLAN_TABLE = 1,

} fm10000_MTableID;

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/
static fm_status  UpdateUsageCounters( fm10000_mtableInfo *info,
                                       fm_int             destDelta,
                                       fm_int             lenDelta,
                                       fm_int             vlanDelta,
                                       fm_int             clonedDelta );
static fm_status FindUnusedIndex(fm_bitArray *bitArray, fm_uintptr *index);
static fm_status FindUnusedMulticastIndex(fm10000_mtableInfo *info,
                                          fm_uintptr *index);
static fm_status MarkMulticastIndexAvailable(fm10000_mtableInfo *info,
                                             fm_int             index);
static fm_status MarkMulticastIndexUsed(fm10000_mtableInfo *info, fm_int index);
static void FreeEntryUsageList(void *value);
static void FreeListenersCount(void *value);
static fm_status GetListenersCount( fm10000_mtableInfo *info,
                                    fm_int             mcastGroup,
                                    fm_int             physPort,
                                    fm_int            *active,
                                    fm_int            *total );
static fm_status SetListenersCount( fm10000_mtableInfo *info,
                                    fm_int             mcastGroup,
                                    fm_int             physPort,
                                    const fm_int      *active,
                                    const fm_int      *total );
static fm_status MarkVlanIndexAvailable(fm10000_mtableInfo *info, fm_int index);
static fm_status MarkVlanIndexUsed(fm10000_mtableInfo *info, fm_int index);
static fm_status MarkVlanIndexExpired(fm10000_mtableInfo *info, fm_int index);
static fm_bool   VlanIndexInUse(fm10000_mtableInfo *info, fm_int index );
static fm_status FindUnusedVlanTableBlock(fm_int             sw,
                                          fm10000_mtableInfo *info,
                                          fm_int             size,
                                          fm_int *           index);
static fm_status RecoverExpiredVlanIndices(fm_int sw, fm10000_mtableInfo *info);
static fm_status MarkLenTableIndexAvailable(fm10000_mtableInfo *info, fm_int index);
static fm_status MarkLenTableIndexUsed(fm10000_mtableInfo *info, fm_int index);
static fm_status MarkLenTableIndexExpired(fm10000_mtableInfo *info, fm_int index);
static fm_bool   LenTableIndexInUse(fm10000_mtableInfo *info, fm_int index );
static fm_status FindUnusedLenTableBlock(fm10000_mtableInfo *info,
                                         fm_int              size,
                                         fm_int *            index);
static fm_status RecoverExpiredLenIndices(fm_int sw, fm10000_mtableInfo *info);

static fm_status AddListener(fm_int               sw,
                             fm10000_mtableInfo   *info,
                             fm_int               mcastGroup,
                             fm_int               repliGroup,
                             fm_int               lenTableIndex,
                             fm_int               vlanIndex,
                             fm_portmask         *groupMask,
                             fm_int               port,
                             fm10000_mtableEntry listener);

static fm_status InsertListenerEntry(fm10000_mtableInfo *info,
                                     fm_int             mcastGroup,
                                     fm_int             repliGroup,
                                     fm_int             port,
                                     fm_int             vlanIndex,
                                     fm_int             lenTableIndex,
                                     fm_dlist *         entryList,
                                     fm_dlist *         entryListPerLenIndex );

static fm_status AddToEntryListForListener(fm10000_mtableInfo *info,
                                           fm_int              physPort,
                                           fm_uint16           vlan,
                                           fm_uint16           glort,
                                           fm_int              mcastGroup,
                                           fm_int              repliGroup,
                                           fm_int              vlanIndex,
                                           fm_int              lenTableIndex);

static fm_status DeleteFromEntryListForListener(fm10000_mtableInfo *info,
                                                fm_int              physPort,
                                                fm_uint16           vlan,
                                                fm_uint16           glort,
                                                fm_int              mcastGroup,
                                                fm_int              repliGroup,
                                                fm_int              vlanIndex,
                                                fm_int              lenTableIndex,
                                                fm_bool             updateTrees);

static fm_status MTableAddListener( fm_int  sw,
                                    fm_int  mcastGroup,
                                    fm_int  repliGroup,
                                    fm_int  physPort,
                                    fm10000_mtableEntry listener,
                                    fm_int  stpState );
static fm_status MTableCleanup(fm_int sw, fm_bool forceClean);

static fm_status CloneVlanTableBlock( fm_int               sw,
                                      fm10000_mtableInfo  *info,
                                      fm_bool              add,
                                      fm_int              *vlanIndex,
                                      fm_int               lenTableIndex,
                                      fm_int               groupSize,
                                      fm_int               physPort,
                                      fm_int               mcastGroup,
                                      fm_int               repliGroup,
                                      fm_int               delVlanIndex);

static fm_status CloneLenTableBlock( fm_int               sw,
                                           fm10000_mtableInfo  *info,
                                           fm_bool              add,
                                           fm_int              *lenIndex,
                                           fm_int               portPos,
                                           fm_int               groupSize,
                                           fm_portmask         *portMask,
                                           fm_int               mcastGroup,
                                           fm_int               repliGroup);

static fm_status ModifyEntryListForListener(fm10000_mtableInfo *info,
                                            fm_int              physPort,
                                            fm_uint16           vlan,
                                            fm_uint16           glort,
                                            fm_int              mcastGroup,
                                            fm_int              repliGroup,
                                            fm_int              oldVlanIndex,
                                            fm_int              newVlanIndex,
                                            fm_int              oldLenTableIndex,
                                            fm_int              newLenTableIndex);

static fm_status ModifyEntryListLenIndex(fm10000_mtableInfo *info,
                                         fm_int oldLenTableIndex,
                                         fm_int newLenTableIndex);
                        

static fm_status GetVlanIndexEntry(fm10000_mtableInfo *info,
                                   fm_int              physPort,
                                   fm_uint16           vlan,
                                   fm_uint16           glort,
                                   fm_int              mcastGroup,
                                   fm_int              repliGroup,
                                   fm_int              lenTableIndex,
                                   fm_int *            vlanIndex);

 
static fm_status RemoveListenerEntry(fm10000_mtableInfo *info,
                                     fm_int              repliGroup,
                                     fm_int              port,
                                     fm_int              lenTableIndex,
                                     fm_dlist           *entryList,
                                     fm_dlist_node      *entryListNode,
                                     fm_dlist           *entryListPerLenIndex,
                                     fm_dlist_node      *entryListPerLenIndexNode);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

#ifdef FM_DEBUG_CHECK_CONSISTENCY
/*****************************************************************************/
/** ValidateMTableConsistency
 * \ingroup intMulticast
 *
 * \desc            Helper to check group state consistency.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \return          FM_OK.
 *
 *****************************************************************************/
static fm_status ValidateMTableConsistency(fm_int sw)
{
    fm_int                   i, j;
    fm_uint64                destEntry;
    fm_uint64                vlanTableReg;
    fm_uint32                lenTableReg;
    fm_status                err = FM_OK;
    fm_switch               *switchPtr;
    fm10000_mtableInfo      *info;
    fm_bool                  expiring;
    fm_bool                  used;
    fm_portmask              mcastMask;
    fm_treeIterator          treeIter;
    fm_treeIterator          treeIterVlan;
    fm_treeIterator          treeIterLenIndex;
    fm_uint64                key;
    fm_uint64                keyGlort;
    fm_int                   mcastGroup=0;
    fm_dlist *               entryList;
    fm_dlist_node *          entryListNode;
    fm_dlist *               entryListPerLenIndex;
    fm_dlist_node            *entryListPerLenIndexNode;
    fm10000_entryListWrapper *entry = NULL;
    fm10000_entryListWrapper *entryFromList = NULL;
    fm10000_MTableGroupInfo *groupInfo;
    fm_uint64                mcastMaskField;
    fm_int                   destTableCount = 0;
    fm_int                   lenTableCount = 0;
    fm_int                   vlanTableCount = 0;
    fm_int                   lenTableIdx;
    fm_int                   vlanTableIdx;
    fm_int                   portPos = -1;
    fm_int                   physPort;
    fm_int                   lenTableIndex;
    fm_bool                  lenTableIndexUsed;
    fm_bool                  vlanTableIndexUsed;
    fm_int                   vlanIndex;
    fm_int                   len;
    fm_int                   vlan;
    fm_int                   dglort;
    fm_bool                  sameEntryFound = FALSE;
    fm_int                   repliGroup;
    fm_int                   numFound = 0;
    fm_int                   entryListCount;
    fm_int                   entryListPerLenIndexCount;
    fm_tree                 *dglortTree;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);

    for ( i = 1 ; i < FM10000_SCHED_MCAST_DEST_TABLE_ENTRIES - 1 ; i++ )
    {
        err = fmGetBitArrayBit(&info->destTableUsage,
                               i,
                               &used);

        if (i < FM10000_MAX_MCAST_DEST_INDEX)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
        else
        {
            /* used for memory repair */
            used = FALSE;
        }

        if (used)
        {
            FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST, 
                                 "DestTable index: %d\n", 
                                 i);
            destTableCount++;
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_SCHED_MCAST_DEST_TABLE(i, 0),
                                        &destEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            mcastMaskField = FM_GET_FIELD64(destEntry,
                                            FM10000_SCHED_MCAST_DEST_TABLE,
                                            PortMask),
            COPY_DESTMASK_TO_PORTMASK(mcastMaskField, mcastMask);

            lenTableIdx = FM_GET_FIELD64( destEntry,
                                          FM10000_SCHED_MCAST_DEST_TABLE,
                                          LenTableIdx );

            FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST,
                                 "mcastMask: %04x%08x lenTableIdx: %d\n",
                                 mcastMask.maskWord[1], mcastMask.maskWord[0],
                                 lenTableIdx);
                                 
            err = fmTreeFind(&info->groups, i, (void **) &groupInfo);
            if (err == FM_OK)
            {
                mcastGroup = groupInfo->mcastGroup;
                repliGroup = groupInfo->repliGroup;
            }
            else if (err == FM_ERR_NOT_FOUND)
            {
                continue;
            }
            else if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }

            portPos = -1;
            for ( physPort = 0; physPort < FM10000_NUM_PORTS; physPort++)
            {
                if (!FM_PORTMASK_IS_BIT_SET(&mcastMask, physPort))
                {
                    continue;
                }
                FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST, 
                                     "Mcast Mask bit set for physPort: %d\n", 
                                     physPort);
                //else
                portPos++;
                lenTableCount++;
                lenTableIndex = lenTableIdx + portPos;
                /* Check whether it is marked as used */
                err = fmGetBitArrayBit(&info->lenTableUsage,
                                       lenTableIndex,
                                       &lenTableIndexUsed);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                if ( !lenTableIndexUsed )
                {
                    FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                 "LenTableIndex: %d must be marked as used.\n",
                                 lenTableIndex);
                }

                err = fmGetBitArrayBit(&info->clonedLenEntriesBitArray,
                                       lenTableIndex,
                                       &expiring);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                if (expiring)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                 "LenTableIndex: %d must not be marked as expired.\n",
                                 lenTableIndex);
                }

                /* now, get the current MCAST_LEN_TABLE register from the switch  */
                err = switchPtr->ReadUINT32(sw,
                                            FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                            &lenTableReg );
                FM_LOG_ABORT_ON_ERR ( FM_LOG_CAT_MULTICAST, err );

                vlanTableIdx = FM_GET_FIELD( lenTableReg,
                                             FM10000_SCHED_MCAST_LEN_TABLE,
                                             L3_McastIdx );
                len       = FM_GET_FIELD( lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_Repcnt );
                FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST,
                                     "vlanTable block start index: %d len: %d\n",
                                     vlanTableIdx,
                                     len);
                for (j = 0; j <= len; j++)
                {
                    vlanIndex = vlanTableIdx + j;
                    vlanTableCount++;
                    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST,
                                         "VlanIndex:%d\n", 
                                         vlanIndex);

                    /* Check whether it is marked as used */
                    err = fmGetBitArrayBit(&info->vlanTableUsage,
                                           vlanIndex,
                                           &vlanTableIndexUsed);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                    if ( !vlanTableIndexUsed )
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                     "VlanTableIndex: %d must be marked as used.",
                                     vlanIndex);
                    }

                    err = fmGetBitArrayBit(&info->clonedEntriesBitArray,
                                           vlanIndex,
                                           &expiring);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                    if (expiring)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                     "VlanTableIndex: %d must not be marked as expired.\n",
                                     vlanIndex);
                    }

                    err = switchPtr->ReadUINT64(sw,
                                                FM10000_MOD_MCAST_VLAN_TABLE(vlanIndex, 0),
                                                &vlanTableReg);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                    
                    vlan = FM_GET_FIELD64(vlanTableReg,
                                          FM10000_MOD_MCAST_VLAN_TABLE,
                                          VID);

                    dglort =  FM_GET_FIELD64(vlanTableReg,
                                             FM10000_MOD_MCAST_VLAN_TABLE,
                                             DGLORT);

                    FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST,
                                         "key calculated from physPort: %d vlan: %d\n",
                                         physPort,
                                         vlan);

                    key = GET_LISTENER_KEY(physPort, vlan);

                    err = fmTreeFind(&info->entryList,
                                     key,
                                     (void **) &dglortTree);

                    if (err == FM_ERR_NOT_FOUND)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                     "Listener wrapper not found for listener(%d, %d)\n", 
                                     physPort, 
                                     vlan);
                        continue;
                    }
                    else if (err == FM_OK)
                    {
                        err = fmTreeFind(dglortTree,
                                         dglort,
                                         (void **) &entryList);
                         if (err == FM_ERR_NOT_FOUND)
                         {
                             FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                          "Listener wrapper not found for listener(%d, %d, 0x%x)\n", 
                                          physPort, 
                                          vlan,
                                          dglort);
                             continue;
                         }
                         else if (err != FM_OK)
                         {
                             FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
                         }
                    }
                    else if (err != FM_OK)
                    {
                        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
                    }

                    /***************************************************
                     * Search the list to ensure that the index is not
                     * already in the list.
                     **************************************************/

                    numFound = 0;
                    entry = NULL;
                    entryFromList = NULL;
                    for (entryListNode = entryList->head ; entryListNode ; )
                    {
                        entry = (fm10000_entryListWrapper *) entryListNode->data;

                        if ( entry->mcastGroup     == mcastGroup &&
                             entry->repliGroup     == repliGroup &&
                             entry->vlanEntryIndex == vlanIndex  &&
                             entry->lenTableIndex  == lenTableIndex)
                        {
                            entryFromList = entry;
                            numFound++;
                        }
                        entryListNode = entryListNode->next;
                    }

                    if (numFound != 1)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                     "Multiple Listener wrapper or none is found for listener(%d, %d)\n", 
                                     physPort, 
                                     vlan);
                        continue;
                    }

                    err = fmTreeFind(&info->entryListPerLenIndex,
                                     lenTableIndex,
                                     (void **) &entryListPerLenIndex);
                    if (err == FM_ERR_NOT_FOUND)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                     "Listener wrapper not found for lenTableIndex: %d\n",
                                     lenTableIndex);
                        continue;
                    }
                    else if (err != FM_OK)          
                    {
                        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
                    }


                    for (entryListPerLenIndexNode = entryListPerLenIndex->head ; entryListPerLenIndexNode ; )
                    {
                        entry = (fm10000_entryListWrapper *) entryListPerLenIndexNode->data;
                        if (entry != NULL && entry == entryFromList )
                        {
                            sameEntryFound = TRUE;
                        }
                        entryListPerLenIndexNode = entryListPerLenIndexNode->next;
                    }
                
                    if (!sameEntryFound)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                                     "Entry in lenTableIndex keyed tree not found for index: %d\n",
                                     lenTableIndex);
                    }
                }
            }
        }
    }

    entryListCount = 0;
    fmTreeIterInit(&treeIter, &info->entryList);

    while (fmTreeIterNext(&treeIter,
                          &key,
                          (void **) &dglortTree) != FM_ERR_NO_MORE)
    {
        fmTreeIterInit(&treeIterVlan, dglortTree);

        while (fmTreeIterNext(&treeIterVlan,
                              &keyGlort,
                              (void **) &entryList) != FM_ERR_NO_MORE)
        {
        
            if (entryList->head)
            {
                for (entryListNode = entryList->head ; entryListNode ; )
                {
                    entry = (fm10000_entryListWrapper *) entryListNode->data;
                    
                    if (entry->lenTableIndex != 0 && entry->vlanEntryIndex != 0)
                    {
                        entryListCount++;
                    }
                    entryListNode = entryListNode->next;
                }

            }

        }

    }

    entryListPerLenIndexCount = 0;
    fmTreeIterInit(&treeIterLenIndex, &info->entryListPerLenIndex);

    while (fmTreeIterNext(&treeIterLenIndex,
                          &key,
                          (void **) &entryListPerLenIndex) != FM_ERR_NO_MORE)
    {
        if (entryListPerLenIndex->head)
        {

            for (entryListPerLenIndexNode = entryListPerLenIndex->head ; entryListPerLenIndexNode ; )
            {
                entry = (fm10000_entryListWrapper *) entryListPerLenIndexNode->data;

                if (entry->lenTableIndex != 0 && entry->vlanEntryIndex != 0)
                {
                    entryListPerLenIndexCount++;
                }
                entryListPerLenIndexNode = entryListPerLenIndexNode->next;
            }

        }
    }

    /* Check ranges of counts */
    if ( info->vlanTableCount < 1 || info->vlanTableCount > FM10000_MAX_MCAST_VLAN_INDEX + 1 )
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "vlanTableCount %d is out of range.\n",
                      info->vlanTableCount);
    }
 
    if ( info->lenTableCount < 1 || info->lenTableCount > FM10000_MAX_MCAST_LEN_INDEX + 1)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "lenTableCount %d is out of range.\n",
                      info->lenTableCount);
    }

    if ( info->destTableCount < 1 || info->destTableCount > FM10000_MAX_MCAST_DEST_INDEX + 1 )
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "destTableCount %d is out of range.\n",
                      info->destTableCount);
    }
    /* Check usage counts are proper. Index 0 is marked as used during intialization.
     * Hence 1 is subtracted from vlanTableCount, lenTableCount and destTableCount. */
    if ((info->vlanTableCount - 1 - info->clonedEntriesCount) != vlanTableCount)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "vlanTable usage inconsistent. Count in mtable "
                     "struct: %d, actual count: %d\n",
                      info->vlanTableCount,
                      vlanTableCount);
    }

    if ((info->lenTableCount - 1) != lenTableCount)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "lenTable usage inconsistent. Count in mtable "
                     "struct: %d, actual count: %d\n",
                      info->lenTableCount,
                      lenTableCount);
    }
    if ( (info->destTableCount - 1) != destTableCount)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "destTable usage inconsistent. Count in mtable "
                     "struct: %d, actual count: %d\n",
                      info->destTableCount,
                      destTableCount);
    }

    /* Check consistency in api maintained lists */
    if (entryListCount != vlanTableCount || entryListPerLenIndexCount != vlanTableCount)
    {
        FM_LOG_ERROR(FM_LOG_CAT_MULTICAST,
                     "Inconsistent count values. entryListCount: %d"
                     "entryListPerLenIndexCount:%d actual count:  %d\n",
                      entryListCount,
                      entryListPerLenIndexCount,
                      vlanTableCount);
    }

ABORT:

    FM_DROP_MTABLE_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}  /* ValidateMTableConsistency */
#endif



/*****************************************************************************/
/** FreeEntryUsageList
 * \ingroup intMulticast
 *
 * \desc            Helper to free a node in the tree that stores the entry
 *                  usage list per (port, vlan) pair.
 *
 * \param[in]       value points to the value for the entry.
 *
 * \return          None.
 *
 *****************************************************************************/
static void FreeEntryUsageList(void *value)
{
    fm_dlist      *list = (fm_dlist *) value;
    fm_dlist_node *node = list->head;
    fm_dlist_node *oldNode;

    while (node)
    {
        fmFree(node->data);

        oldNode = node;
        node = node->next;

        fmFree(oldNode);
    }

    fmFree(list);

}   /* end FreeEntryUsageList */




/*****************************************************************************/
/** FreeEntryUsageTree
 * \ingroup intMulticast
 *
 * \desc            Helper to free a node in the tree that stores the entry
 *                  usage list per (port, vlan) pair.
 *
 * \param[in]       value points to the value for the entry.
 *
 * \return          None.
 *
 *****************************************************************************/
static void FreeEntryUsageTree(void *value)
{
    fm_tree *dglortTree = (fm_tree *)value;

    /* Destroy the entry list holding listener wrappers  */
    fmTreeDestroy(dglortTree, FreeEntryUsageList);

    fmFree(dglortTree);

}   /* end FreeEntryUsageTree */




/*****************************************************************************/
/** FreeEntryListPerLenIndex
 * \ingroup intMulticast
 *
 * \desc            Helper to free a node in the tree that stores the entry
 *                  usage list per (port, vlan) pair.
 *
 * \param[in]       value points to the value for the entry.
 *
 * \return          None.
 *
 *****************************************************************************/
static void FreeEntryListPerLenIndex(void *value)
{
    fm_dlist      *list = (fm_dlist *) value;
    fm_dlist_node *node = list->head;
    fm_dlist_node *oldNode;

    while (node)
    {
        /* Note: node->data need not be freed here. It will be freed 
           in FreeEntryUsageList since its data object is used here. */

        oldNode = node;
        node = node->next;

        fmFree(oldNode);
    }

    fmFree(list);

}   /* end FreeEntryListPerLenIndex */




/*****************************************************************************/
/** FreeListenersCount
 * \ingroup intMulticast
 *
 * \desc            Helper to free the counter structure in the tree that
 *                  stores the listeners count per (mcastGroup, port)
 *
 * \param[in]       value points to the value for the entry.
 *
 * \return          None.
 *
 *****************************************************************************/
static void FreeListenersCount(void *value)
{
    fmFree(value);

}   /* end FreeListenersCount */




/*****************************************************************************/
/** RemoveListenerEntry
 * \ingroup intMulticast
 *
 * \desc            Removes a listener from the linked list of installed
 *                  listeners and update counters accordingly
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       port is the physical port of the listener.
 * 
 * \param[in]       lenTableIndex is the index of this listener in MCAST_LEN_TABLE.
 *
 * \param[in]       entryList is the pointer to the linked list
 *
 * \param[in]       entryListNode is the pointer to the node to be removed
 * 
 * \param[in]       entryListPerLenIndex is a list of entry instances
 *                  for this lenTableIndex.
 * 
 * \param[in]       entryListPerLenIndexNode is the current node in the
 *                  entryListPerLenIndex list.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status RemoveListenerEntry(fm10000_mtableInfo *info,
                                     fm_int              repliGroup,
                                     fm_int              port,
                                     fm_int              lenTableIndex,
                                     fm_dlist *          entryList,
                                     fm_dlist_node *     entryListNode, 
                                     fm_dlist *          entryListPerLenIndex,
                                     fm_dlist_node *     entryListPerLenIndexNode)
{
    fm_int                   activeCount;
    fm_int                   totalCount;
    fm_status                err;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "Removing listener: group=%d port= %d list=%p node=%p\n",
                 repliGroup,
                 port,
                 (void *)entryList,
                 (void *)entryListNode );

    /* free the user data */
    fmFree(entryListNode->data );

    /* remove the node from the linked list */
    fmDListRemove(entryList, entryListNode);

    if (entryListPerLenIndex != NULL && entryListPerLenIndexNode != NULL)
    {
        /* remove the node from the linked list */
        fmDListRemove(entryListPerLenIndex, entryListPerLenIndexNode);

        /* If list is empty then remove the lenTableIndex key in the tree */
        if (entryListPerLenIndex->head == NULL)
        {
            err = fmTreeRemove(&info->entryListPerLenIndex, 
                               lenTableIndex, 
                               FreeEntryListPerLenIndex);    
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
    }

    /* update the counters */
    err= GetListenersCount(info, repliGroup, port, &activeCount, &totalCount);
    FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    totalCount--;
    err = SetListenersCount( info,
                             repliGroup,
                             port,
                             (const fm_int *)&activeCount,
                             (const fm_int *)&totalCount );

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end RemoveListenerEntry  */




/*****************************************************************************/
/** InserListenerEntry
 * \ingroup intMulticast
 *
 * \desc            Inserts a new listener at the end of the linked list and
 *                  updates the appropriate counters
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       mcastGroup is the multicast group ID
 *
 * \param[in]       repliGroup is the replication group ID
 *
 * \param[in]       port is the physical port of the listener.
 *
 * \param[in]       vlanIndex is the index of the entry in MCAST_VLAN_TABLE.
 * 
 * \param[in]       lenTableIndex is the index of the entry in MCAST_LEN_TABLE.
 *
 * \param[in]       entryList is the pointer to the linked list
 *
 * \param[in]       entryListPerLenIndex is the pointer to the linked list
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InsertListenerEntry( fm10000_mtableInfo *info,
                                      fm_int             mcastGroup,
                                      fm_int             repliGroup,
                                      fm_int             port,
                                      fm_int             vlanIndex,
                                      fm_int             lenTableIndex,
                                      fm_dlist          *entryList, 
                                      fm_dlist          *entryListPerLenIndex )

{
    fm10000_entryListWrapper *entry;
    fm_int                   activeCount;
    fm_int                   totalCount;
    fm_status                err;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "Inserting listener: group=%d port=%d list=%p\n",
                 repliGroup,
                 port,
                 (void *)entryList );


    entry= (fm10000_entryListWrapper *)fmAlloc(sizeof(fm10000_entryListWrapper));
    if (!entry)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_NO_MEM);
    }

    entry->mcastGroup     = mcastGroup;
    entry->repliGroup     = repliGroup;
    entry->vlanEntryIndex = vlanIndex;
    entry->lenTableIndex  = lenTableIndex;

    err = fmDListInsertEnd(entryList, entry);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);

    if (entryListPerLenIndex != NULL)
    {
        err = fmDListInsertEnd(entryListPerLenIndex, entry);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);
    }

    err= GetListenersCount(info, repliGroup, port, &activeCount, &totalCount);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);

    totalCount++;
    err = SetListenersCount( info,
                             repliGroup,
                             port,
                             (const fm_int *)&activeCount,
                             (const fm_int *)&totalCount );
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Inserted node at end of list for index %d\n", vlanIndex);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_OK );

}   /* end InserListenerEntry */




/*****************************************************************************/
/** FindUnusedIndex
 * \ingroup intMulticast
 *
 * \desc            Helper function to look for an unused entry in the
 *                  MCAST_DEST_TABLE or MCAST_VLAN_TABLE
 *
 * \param[in]       bitArray is the pointer to the bit array representing the
 *                  status of either table
 *
 * \param[out]      index points to caller allocated storage where the new
 *                  index is stored.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if either argument is NULL
 *                  FM_ERR_NO_MCAST_RESOURCES if no entries are available
 *
 *****************************************************************************/
static fm_status FindUnusedIndex(fm_bitArray *bitArray,
                                 fm_uintptr *index)
{
    fm_status err;
    fm_int    bit;

    /* sanity check on the arguments */
    if ( bitArray == NULL || index == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* find the first available unused entry */
    err = fmFindBitBlockInBitArray(bitArray, 1, 1, FALSE, &bit);

    /* bail out if there's an error */
    if (err != FM_OK)
    {
        return err;
    }

    /* was there at least one available entry? */
    if (bit == -1)
    {
        /* no */
        return FM_ERR_NO_MCAST_RESOURCES;
    }

    /* everything ok */
    *index = bit;
    return FM_OK;

}   /* end FindUnusedIndex */




/*****************************************************************************/
/** FindUnusedMulticastIndex
 * \ingroup intMulticast
 *
 * \desc            Helper function to look for an unused entry in the
 *                  MCAST_DEST_TABLE
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[out]      index points to caller allocated storage where the new
 *                  index is stored.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if either argument is NULL
 *                  FM_ERR_NO_MCAST_RESOURCES if no entries are available
 *
 *****************************************************************************/
static fm_status FindUnusedMulticastIndex(fm10000_mtableInfo *info,
                                          fm_uintptr *index)
{
    fm_status err;

    /* sanity check on the arguments */
    if ( info == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* find the first available unused entry */
    err = FindUnusedIndex( &info->destTableUsage, index );
    return err;

}   /* end FindUnusedMulticastIndex */




/*****************************************************************************/
/** MarkMcastIndexAvailable
 * \ingroup intMulticast
 *
 * \desc            Helper function to reset the MCAST_DEST_TABLE usage bit.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_DEST_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkMulticastIndexAvailable(fm10000_mtableInfo *info,
                                             fm_int              index)
{
    fm_status err;

    err = fmSetBitArrayBit( &info->destTableUsage, index, FALSE );
    if ( err == FM_OK )
    {
        err = UpdateUsageCounters( info, -1, 0, 0, 0 );
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking index 0x%x as available\n", index);

    return err;

}   /* end MarkMcastIndexAvailable */


/*****************************************************************************/
/** MarkMulticastIndexUsed
 * \ingroup intMulticast
 *
 * \desc            Helper function to set the MCAST_DEST_TABLE usage bit.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_DEST_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkMulticastIndexUsed(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    err = fmSetBitArrayBit( &info->destTableUsage, index, TRUE );
    if ( err == FM_OK )
    {
        err = UpdateUsageCounters( info, 1, 0, 0, 0 );
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking index 0x%x as used\n", index);

    return err;

}   /* end MarkMcastIndexUsed */




/*****************************************************************************/
/** GetListenersCount
 * \ingroup intMulticast
 *
 * \desc            Gets the active listeners count for a given pair
 *                  (mcastGroup, port). Those are the listeners that
 *                  have been added to the MCAST_VLAN_TABLE, as
 *                  their STP state is "forwarding"
 *
 * \param[in]       info is a pointer to the MTable state structure
 *
 * \param[in]       repliGroup is the replication group number for which to get
 *                  the count
 *
 * \param[in]       physPort is the physical port for which to get the count
 *
 * \param[out]      active points to user-allocated storage where the number
 *                  of active listeners is going to be returned (NULL if the
 *                  caller doesn't wish to get this counter )
 *
 * \param[out]      total points to user-allocated storage where the number
 *                  of total listeners is going to be returned (NULL if the
 *                  caller doesn't wish to get this counter )
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_NOT_FOUND if (the mcastGroup, port) pair was not
 *                  found
 *****************************************************************************/
static fm_status GetListenersCount( fm10000_mtableInfo *info,
                                    fm_int             repliGroup,
                                    fm_int             physPort,
                                    fm_int             *active,
                                    fm_int             *total )
{
    fm_status                    err;
    fm_uint64                    key;
    fm10000_MTableListenersCount *counters;

    /* the key is a combination of the port and the group index */
    key = ( repliGroup | (fm_uint64)physPort << 32 );
    err = fmTreeFind( &info->listenersCount, key, (void **)&counters);

    /* was the key found? */
    if ( err == FM_OK )
    {
        /* yes, return the counters */
        if ( active != NULL )
        {
            *active = counters->active;
        }
        if ( total != NULL )
        {
            *total  = counters->total;
        }
    }
    else if ( err == FM_ERR_NOT_FOUND )
    {
        /* no, return zero counters */
        if ( active != NULL )
        {
            *active = 0;
        }
        if ( total != NULL )
        {
            *total  = 0;
        }

        /* let's not consider this an error condition */
        err = FM_OK;
    }

    return err;

}   /* end GetListenersCount */




/*****************************************************************************/
/** SetListenersCount
 * \ingroup intMulticast
 *
 * \desc            Sets the active listeners count for a given pair
 *                  (mcastGroup, port). Those are the listeners that
 *                  have been added to the MCAST_VLAN_TABLE, as
 *                  their STP state is "forwarding"
 *
 * \param[in]       info is a pointer to the MTable state structure
 *
 * \param[in]       repliGroup is the replication group number for which to set
 *                  the count
 *
 * \param[in]       physPort is the physical port for which to get the count
 *
 * \param[out]      active points to user-allocated storage where the number
 *                  of active listeners is going to be passed (NULL if the
 *                  caller doesn't wish to set this counter )
 *
 * \param[out]      total points to user-allocated storage where the number
 *                  of total listeners is going to be returned (NULL if the
 *                  caller doesn't wish to set this counter )
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status SetListenersCount( fm10000_mtableInfo *info,
                                    fm_int             repliGroup,
                                    fm_int             physPort,
                                    const fm_int      *active,
                                    const fm_int      *total )
{
    fm_status          err;
    fm_uint64          key;
    fm10000_MTableListenersCount *counters;

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Setting counts: group=%d physPort=%d active=%d total=%d\n",
                 repliGroup, physPort,
                 active ? *active : -1,
                 total ? *total : -1);

    /* the key is a combination of the port and the group index */
    key = ( repliGroup | (fm_uint64)physPort << 32 );
    err = fmTreeFind( &info->listenersCount, key, (void **)&counters );

    /* was this (repliGroup, port) pair already in the tree? */
    if ( err == FM_OK )
    {
        /* yes */
        if ( total != NULL )
        {
            /* remove the entry from the tree if the total count is zero */
            if ( *total == 0 )
            {
                err = fmTreeRemove( &info->listenersCount,
                                    key,
                                    FreeListenersCount );
                return err;
            }
            counters->total = *total;
        }

        if ( active != NULL )
        {
            counters->active = *active;
        }

    }
    else if ( err == FM_ERR_NOT_FOUND )
    {
        /* no */
        if ( total != NULL && *total == 0 )
        {
            return FM_OK;
        }

        /* no insert the counters for this pair in the tree */
        counters = fmAlloc( sizeof(fm10000_MTableListenersCount) );
        if ( counters != NULL )
        {
            counters->active = active ? *active : 0;
            counters->total  = total  ? *total  : 0;
            err = fmTreeInsert( &info->listenersCount,
                                key,
                                counters );
        }
        else
        {
            err = FM_ERR_NO_MEM;
        }
    }
    return err;

}   /* end SetListenersCount */




/*****************************************************************************/
/** UpdateUsageCounter
 * \ingroup intMulticast
 *
 * \desc            Updates the usage counters in the MTable
 *
 * \note            This function also updated the current watermark level
 *
 * \param[in]       info pointer to the MTable Info structure
 *
 * \param[in]       destDelta number of MCAST_DELTA_TABLE entries added or
 *                  removed
 *
 * \param[in]       lenDelta number of MCAST_LEN_TABLE entries added or
 *                  removed
 *
 * \param[in]       vlanDelta number of MCAST_VLAN_TABLE entries added or
 *                  removed
 *
 * \param[in]       clonedDelta number of MCAST_VLAN_TABLE entry clones added
 *                  or removed
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if the info structure is invalid
 *
 *****************************************************************************/
static fm_status  UpdateUsageCounters( fm10000_mtableInfo *info,
                                        fm_int             destDelta,
                                        fm_int             lenDelta,
                                        fm_int             vlanDelta,
                                        fm_int             clonedDelta )
{
    if ( info == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* straight update of the counters */
    info->destTableCount += destDelta;
    info->vlanTableCount += vlanDelta;
    info->lenTableCount += lenDelta;
    info->clonedEntriesCount += clonedDelta;

    /**************************************************
     * If available MCAST_VLAN_TABLE space has changed:
     * update the garbage collection watermark based
     * on the percentage attribute
     **************************************************/
    if (vlanDelta != 0 || clonedDelta != 0)
    {
        /* Get available space and then calculate waterMark values */
        info->clonedEntriesWatermark =  FM10000_MAX_MCAST_VLAN_INDEX - 1 -
                                        info->vlanTableCount;
        if ( info->clonedEntriesWatermark != 0 )
        {
            info->clonedEntriesWatermark *= info->watermarkPercentage;
            info->clonedEntriesWatermark /= 100;
            info->clonedEntriesWatermark ++;
        }
    }

    return FM_OK;

}   /* end UpdateUsageCounter */




/*****************************************************************************/
/** CloneLenTableBlock
 * \ingroup intMulticast
 *
 * \desc            Copies a current block to a new one with one more free slot
 *                  if possible, updating the expiration time of the current
 *                  block appropriately.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       add indicates if we are cloning for an add or a delete.
 * 
 * \param[in]       lenIndex points to caller-provided storage where the
 *                  new index value will be written.
 * 
 * \param[in]       portPos is the position of this port within the
 *                  port mask.
 *
 * \param[in]       groupSize is the number of entries in the block.
 *
 * \param[in]       portMask is the current destination mask associated with
 *                  the multicast group.  In the adding case,
 *                  this mask will already contain the port to be added. In
 *                  the deleting case, it still contains the port to be
 *                  deleted.
 *
 * \param[in]       mcastGroup is the multicast group ID.
 *
 * \param[in]       repliGroup is the replication group ID.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CloneLenTableBlock( fm_int               sw,
                                     fm10000_mtableInfo  *info,
                                     fm_bool              add,
                                     fm_int              *lenIndex,
                                     fm_int               portPos,
                                     fm_int               groupSize,
                                     fm_portmask         *portMask,
                                     fm_int               mcastGroup,
                                     fm_int               repliGroup)
{
    fm_status               err = FM_OK;
    fm_switch  *            switchPtr;
    fm_int                  newIndex;
    fm_int                  i;
    fm_uint32               value;
    fm_int                  copyIndex;
    fm_int                  readIndex;
    fm_int                  newGroupSize;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d info=%p size=%d Index=%d portPos=%d portMask=%p "
                 "mcastGroup=%d repliGroup=%d\n",
                 sw, (void *) info,  groupSize, *lenIndex, portPos, (void *)portMask,
                 mcastGroup, repliGroup);

    newGroupSize = add ? groupSize + 1 : groupSize - 1;

    /* find an available index in the MCAST_VLAN_TABLE */
    err = FindUnusedLenTableBlock(info, newGroupSize, &newIndex);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* found it? */
    if (newIndex == -1)
    {
        /* no */
        err = FM_ERR_NO_MCAST_RESOURCES;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found new block of size %d at index 0x%x\n",
                 newGroupSize, newIndex);

    for (i = 0 ; i < groupSize ; i++)
    {
        readIndex = *lenIndex + i;
        if (!add)
        {
            if (i == portPos)
            {
                /* skip this entry */
                /* Quarantine this entry */
                MarkLenTableIndexExpired(info, readIndex);
                continue;
            }
        }

        if (add)
        {
            copyIndex = (i < portPos) ? (newIndex + i) : (newIndex + i + 1);
        }
        else
        {
            copyIndex = (i > portPos) ? (newIndex + i - 1) : (newIndex + i);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "readIndex=%d copyIndex=%d\n",
                     readIndex,
                     copyIndex);

        /* read the entry at readIndex into value */
        err = switchPtr->ReadUINT32( sw,
                                     FM10000_SCHED_MCAST_LEN_TABLE(readIndex),
                                     &value );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        /* now copy the value to the next index 'copyIndex' */
        err = switchPtr->WriteUINT32( sw,
                                      FM10000_SCHED_MCAST_LEN_TABLE(copyIndex),
                                      value );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Copied entry from 0x%x to 0x%x\n",
                     readIndex, copyIndex);

        MarkLenTableIndexUsed(info, copyIndex);

        /* Quarantine this entry in the starting block */
        MarkLenTableIndexExpired(info, readIndex);

        err = ModifyEntryListLenIndex(info, readIndex, copyIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    *lenIndex = newIndex;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end CloneLenTableBlock */




/*****************************************************************************/
/** CloneVlanTableBlock
 * \ingroup intMulticast
 *
 * \desc            Copies a current block to a new one with one more free slot
 *                  if possible, updating the expiration time of the current
 *                  block appropriately.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       add indicates if we are cloning for an add or a delete.
 *
 * \param[in]       vlanIndex points to caller allocated storage that contains
 *                  the current starting index to clone.  The new index will
 *                  be stored here.
 *
 * \param[in]       lenTableIndex is the index of this listener in
 *                  MCAST_LEN_TABLE.
 *
 * \param[in]       groupSize is the number of entries in the block.
 *
 * \param[in]       physPort is the physical port ID of the listener being added
 *
 * \param[in]       mcastGroup is the multicast group ID
 *
 * \param[in]       repliGroup is the replication group ID
 *
 * \param[in]       delVlanIndex is the index into the MCAST_VLAN_TABLE
 *                  in which this listener is stored, or -1 if the listener
 *                  is not stored (i.e., this is an add operation).
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CloneVlanTableBlock(fm_int               sw,
                                     fm10000_mtableInfo  *info,
                                     fm_bool              add,
                                     fm_int              *vlanIndex,
                                     fm_int               lenTableIndex,
                                     fm_int               groupSize,
                                     fm_int               physPort,
                                     fm_int               mcastGroup,
                                     fm_int               repliGroup,
                                     fm_int               delVlanIndex)
{
    fm_status               err = FM_OK;
    fm_switch  *            switchPtr;
    fm_int                  newIndex;
    fm_int                  i;
    fm_uint64               value;
    fm_int                  copyIndex;
    fm_int                  readIndex;
    fm_int                  newGroupSize;
    fm_int                  delPosition;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d info=%p size=%d Index=%d \n",
                 sw, (void *) info,  groupSize, *vlanIndex);

    newGroupSize = add ? (groupSize + 1) : (groupSize - 1);

    /* find an available index in the MCAST_VLAN_TABLE */
    err = FindUnusedVlanTableBlock(sw, info, newGroupSize, &newIndex);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* found it? */
    if (newIndex == -1)
    {
        /* no */
        err = FM_ERR_NO_MCAST_RESOURCES;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found new block of size %d at index 0x%x\n",
                 newGroupSize, newIndex);

    for (i = 0 ; i < groupSize ; i++)
    {
        readIndex = *vlanIndex + i;
        if (!add)
        {
            delPosition = delVlanIndex - *vlanIndex;
            if (i == delPosition)
            {
                /* skip this entry */
                /* Quarantine this entry */
                MarkVlanIndexExpired(info, readIndex);
                continue;
            }
        }

        if (add)
        {
            copyIndex =  newIndex + i ;
        }
        else
        {
            copyIndex = (i > delPosition) ? (newIndex + i - 1) : (newIndex + i);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "readIndex=%d copyIndex=%d\n",
                     readIndex,
                     copyIndex);

        /* read the entry at readIndex into value */
        err = switchPtr->ReadUINT64( sw,
                                     FM10000_MOD_MCAST_VLAN_TABLE(readIndex, 0),
                                     &value );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        /* now copy the value to the next index 'copyIndex' */
        err = switchPtr->WriteUINT64( sw,
                                      FM10000_MOD_MCAST_VLAN_TABLE(copyIndex, 0),
                                      value );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Copied entry from 0x%x to 0x%x\n",
                     readIndex, copyIndex);

        MarkVlanIndexUsed(info, copyIndex);

        /* Quarantine this entry in the starting block */
        MarkVlanIndexExpired(info, readIndex);

        err = ModifyEntryListForListener(info,
                                         physPort,
                                         FM_GET_FIELD64(value,
                                                        FM10000_MOD_MCAST_VLAN_TABLE,
                                                        VID),
                                         FM_GET_FIELD64(value,
                                                        FM10000_MOD_MCAST_VLAN_TABLE,
                                                        DGLORT),
                                         mcastGroup,
                                         repliGroup,
                                         readIndex,
                                         copyIndex,
                                         lenTableIndex,
                                         lenTableIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    *vlanIndex = newIndex;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end CloneVlanTableBlock */




/*****************************************************************************/
/** AddListener
 *
 * \desc            Adds a listener to the list for a port in a group.
 *
 * \param[in]       sw is the switch number
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       mcastGroup is the multicast group number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       lenTableIndex is the  index of the entry of the group in the
 *                  MCAST_LEN_TABLE. Entry at this index is updated.
 *
 * \param[in]       vlanIndex is the index of the beginning of group in the
 *                  MCAST_VLAN_TABLE. Listeners port and vlan are added at the end
 *                  of this group.
 *
 * \param[in]       groupMask is the destination mask of the group.
 *
 * \param[in]       port the physical port ID of the listener being added
 *
 * \param[in]       listener is the structure which has all MCAST_VLAN_TABLE fields
 *                  that has to be added.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddListener(fm_int               sw,
                             fm10000_mtableInfo   *info,
                             fm_int               mcastGroup,
                             fm_int               repliGroup,
                             fm_int               lenTableIndex,
                             fm_int               vlanIndex,
                             fm_portmask         *groupMask,
                             fm_int               port,
                             fm10000_mtableEntry  listener)
{
    fm_status  err = FM_OK;
    fm_switch *switchPtr;
    fm_uint32  lenTableReg;
    fm_uint64  vlanTableReg;
    fm_int     activeCount;
    fm_int     listenerIndex;
    fm_int     finalVlanIndex;
    fm_bool    addToList = TRUE;

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d info=%p repliGroup=%d lenTableIndex=%d vlanIndex=%d"
                 " listener=<%d,%d,%d,%d, %d>\n",
                 sw, (void *) info, repliGroup, lenTableIndex, vlanIndex, port, 
                 listener.vlan, listener.dglort, listener.vlanUpdate, 
                 listener.dglortUpdate );

    /* now, get the current MCAST_DEST_TABLE register from the switch  */
    err = switchPtr->ReadUINT32(sw,
                                FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                &lenTableReg);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    err = GetListenersCount( info, repliGroup, port, &activeCount, NULL );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    
    finalVlanIndex = vlanIndex;
    
    /* lenTableReg will be written to the sw after MCAST_VLAN_TABLE is updated */
    FM_SET_FIELD( lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_McastIdx, finalVlanIndex);
    FM_SET_FIELD(lenTableReg,
                 FM10000_SCHED_MCAST_LEN_TABLE,
                 L3_Repcnt,
                 activeCount);

    FM_CLEAR(vlanTableReg);
    FM_SET_FIELD64( vlanTableReg,
                    FM10000_MOD_MCAST_VLAN_TABLE,
                    VID,
                    listener.vlan );
    FM_SET_FIELD64( vlanTableReg,
                    FM10000_MOD_MCAST_VLAN_TABLE,
                    DGLORT,
                    listener.dglort );
    FM_SET_BIT64( vlanTableReg, 
                  FM10000_MOD_MCAST_VLAN_TABLE, 
                  ReplaceVID, 
                  listener.vlanUpdate );
    FM_SET_BIT64( vlanTableReg, 
                  FM10000_MOD_MCAST_VLAN_TABLE, 
                  ReplaceDGLORT, 
                  listener.dglortUpdate );
    
    listenerIndex = finalVlanIndex + activeCount;
    /* listenerIndex should be reserved for this group before calling this function */
 
    err = switchPtr->WriteUINT64( sw,
                                  FM10000_MOD_MCAST_VLAN_TABLE(listenerIndex, 0),
                                  vlanTableReg );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    err = switchPtr->WriteUINT32( sw,
                                  FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                  lenTableReg );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* If the index is already in use and just update is required, 
     * then MarkLenTableIndexUsed should not be called. Else it should be marked. */
    if (!LenTableIndexInUse(info, lenTableIndex))
    {
        err = MarkLenTableIndexUsed(info, lenTableIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    err = MarkVlanIndexUsed(info, listenerIndex);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* Update the active listener count */
    activeCount++;
    err = SetListenersCount( info, repliGroup, port, &activeCount, NULL );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Active count for port %d in group %d is now %d\n",
                 port, repliGroup, activeCount);

    if (addToList)
    {
        /* add this new entry to the list of listeners */
        err = AddToEntryListForListener(info,
                                        port,
                                        listener.vlan,
                                        listener.dglort,
                                        mcastGroup,
                                        repliGroup,
                                        listenerIndex,
                                        lenTableIndex );
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end AddListener */




/*****************************************************************************/
/** MTableCleanup
 * \ingroup intMulticast
 *
 * \desc            Function to cleanup MTable expired resources.
 *
 * \param[in]       sw the switch on which to operate
 *
 * \param[in]       forceClean specifies if this cleanup has to be performed
 *                  irrespective of clonedEntries being higher than the watermark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MTableCleanup(fm_int sw, fm_bool forceClean)
{
    fm_switch          *switchPtr;
    fm10000_mtableInfo *info;
    fm_status           err = FM_OK;
    fm_byte             newEpoch;
    fm_byte             oldEpoch;
    fm_uint32           prevEpochCounter;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_MULTICAST, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);

    /* wait for the MTABLE to be initialized */
    if ( info->isInitialized == FALSE )
    {
        err = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Proceed to cleanup when clonedEntries is greater than watermark% of remaining
     * available entries or forceClean is enabled. */
    if (info->clonedEntriesCount <= info->clonedEntriesWatermark && !forceClean )
    {
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }
    //else
    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "MTable Cleanup: cloned=%d - watermark=%d\n",
                 info->clonedEntriesCount,
                 info->clonedEntriesWatermark);

    oldEpoch = info->epoch;
    newEpoch = (oldEpoch == 0) ? 1 : 0;

    err = switchPtr->WriteUINT32( sw,
                                  FM10000_MCAST_EPOCH(),
                                  newEpoch );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    info->epoch = newEpoch;

    prevEpochCounter = 0;
    do
    {
        err = switchPtr->ReadUINT32( sw,
                                     FM10000_MCAST_EPOCH_USAGE(oldEpoch),
                                     &prevEpochCounter );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        fmDelay(0, EPOCH_USAGE_SCAN_INTERVAL);

    } while (prevEpochCounter != 0);

    err = RecoverExpiredVlanIndices(sw, info);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

#ifdef FM_DEBUG_CHECK_CONSISTENCY
    ValidateMTableConsistency(sw);
#endif

ABORT:

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_MULTICAST,  err);

}   /* end MTableCleanup */


/*****************************************************************************/
/** FindUnusedLenTableBlock
 *
 * \desc            Helper function to look for a block of unused entries
 *                  in the MCAST_LEN_TABLE. The default block size is used
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       size is the number of bits in block.
 *
 * \param[out]      index points to caller allocated storage where the new
 *                  index is stored.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if either argument is NULL
 *
 *****************************************************************************/
static fm_status FindUnusedLenTableBlock(fm10000_mtableInfo *info,
                                         fm_int              size,
                                         fm_int *            index)
{
    fm_status err;
    fm_int    bit;

    /* sanity check on the arguments */
    if ( info == NULL || index == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* find the first available unused entry */
    err = fmFindBitBlockInBitArray(&info->lenTableUsage,
                                   1,
                                   size,
                                   FALSE,
                                   &bit);
    /* bail out if there's an error */
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (bit > 0)
    {
        /* return the initial bit position */
        *index = bit;
    }
    else
    {
        err = FM_ERR_NO_MCAST_RESOURCES;
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);
    }


    return FM_OK;

}   /* end FindUnusedLenTableBlock */




/*****************************************************************************/
/** MarkLenTableIndexAvailable
 *
 * \desc            Helper function to reset the MCAST_LEN_TABLE usage bit.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_LEN_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkLenTableIndexAvailable(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    err = fmSetBitArrayBit(&info->lenTableUsage,
                           index,
                           FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = UpdateUsageCounters( info, 0, -1, 0, 0 );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking LenTable index 0x%x as available\n", index);

ABORT:
    return err;

}   /* end MarkLenTableIndexAvailable */


/*****************************************************************************/
/** MarkLenTableIndexUsed
 *
 * \desc            Helper function to set the usage bit.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_VLAN_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkLenTableIndexUsed(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    err = fmSetBitArrayBit(&info->lenTableUsage,
                           index,
                           TRUE);
    if ( err == FM_OK )
    {
        err = UpdateUsageCounters( info, 0, 1, 0, 0 );
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking index 0x%x as used\n", index);

    return err;

}   /* end MarkLenTableIndexUsed */


/*****************************************************************************/
/** MarkLenTableIndexExpired
 *
 * \desc            Helper function to set the expiration on an entry.  This
 *                  does not mark the index as available.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_LEN_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkLenTableIndexExpired(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    /* We make entries expire by putting them in Quarantine */
    err = fmSetBitArrayBit(&info->clonedLenEntriesBitArray,
                           index,
                           TRUE);

     /* No need to keep count of clone len table entries. 
      * They will be recovered immediately and no watermark is also 
      * necessary for len table. */
    return err;

}   /* end MarkLenTableIndexExpired */


/*****************************************************************************/
/** LenIndexInUse
 *
 * \desc            Helper function to set the expiration on an entry in
 *                  the MCAST_LEN_TABLE. This does not mark the index as
 *                  available.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_LEN_TABLE index to mark.
 *
 * \return          True if the bit is used, false otherwise or on error.
 *
 *****************************************************************************/
static fm_bool LenIndexInUse(fm10000_mtableInfo *info, fm_int index)
{
    fm_status    err;
    fm_bool      bit;

    err = fmGetBitArrayBit(&info->lenTableUsage,
                           index,
                           &bit);

    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_MULTICAST,  "Unable to get bit value: %s\n",
                     fmErrorMsg(err) );

        return FALSE;
    }

    return bit;

}   /* end LenIndexInUse */



/*****************************************************************************/
/** RecoverExpiredLenIndices
 *
 * \desc            Helper function to recover expired entries in the
 *                  MCAST_LEN_TABLE. This marks the expired indices as 
 *                  available.
 * 
 * \param[in]       sw is the switch number.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \return          FM_OK if success.
 *
 *****************************************************************************/
static fm_status RecoverExpiredLenIndices(fm_int sw, fm10000_mtableInfo *info)
{
    fm_status  err;
    fm_int     lenIndex;
    fm_switch *switchPtr;
    
    switchPtr = GET_SWITCH_PTR(sw);
    /* walk the list of cloned entries and mark them as available */
    lenIndex = 0;
    while (lenIndex >= 0)
    {
        err = fmFindBitInBitArray(&info->clonedLenEntriesBitArray,
                                  lenIndex + 1,
                                  TRUE,
                                  &lenIndex);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        if (lenIndex > 0)
        {
            err = MarkLenTableIndexAvailable(info, lenIndex);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            err = switchPtr->WriteUINT32( sw,
                                          FM10000_SCHED_MCAST_LEN_TABLE(lenIndex),
                                          0 );
            FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
        }
    }

    /* Clear the MCAST_LEN_TABLE cloned entries bit array */
    err = fmClearBitArray(&info->clonedLenEntriesBitArray);

    return err;

}   /* end RecoverExpiredLenIndices */

/*****************************************************************************/
/** LenTableIndexInUse
 *
 * \desc            Helper function to set the expiration on an entry in
 *                  the MCAST_LEN_TABLE. This does not mark the index as
 *                  available.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_LEN_TABLE index to mark.
 *
 * \return          True if the bit is used, false otherwise or on error.
 *
 *****************************************************************************/
static fm_bool LenTableIndexInUse(fm10000_mtableInfo *info, fm_int index)
{
    fm_status    err;
    fm_bool      bit;

    err = fmGetBitArrayBit(&info->lenTableUsage,
                           index,
                           &bit);

    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_MULTICAST,  "Unable to get bit value: %s\n",
                     fmErrorMsg(err) );

        return FALSE;
    }

    return bit;

}   /* end LenTableIndexInUse */



/*****************************************************************************/
/** FindUnusedVlanTableBlock
 *
 * \desc            Helper function to look for a block of unused entries
 *                  in the MCAST_VLAN_TABLE. The default block size is used
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       size is the number of bits in block.
 *
 * \param[out]      index points to caller allocated storage where the new
 *                  index is stored.
 *
 * \return          FM_OK if successful.
 *                  FM_ERR_INVALID_ARGUMENT if either argument is NULL
 *
 *****************************************************************************/
static fm_status FindUnusedVlanTableBlock(fm_int             sw,
                                          fm10000_mtableInfo *info,
                                          fm_int             size,
                                          fm_int *           index)
{
    fm_status err;
    fm_int    bit;

    /* sanity check on the arguments */
    if ( info == NULL || index == NULL )
    {
        return FM_ERR_INVALID_ARGUMENT;
    }

    /* find the first available unused entry */
    err = fmFindBitBlockInBitArray(&info->vlanTableUsage,
                                   1,
                                   size,
                                   FALSE,
                                   &bit);

    if (err == FM_OK)
    {
        if (bit < 0)
        {
            err = MTableCleanup(sw, TRUE);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            
            /* find the first available unused entry */
            err = fmFindBitBlockInBitArray(&info->vlanTableUsage,
                                           1,
                                           size,
                                           FALSE,
                                           &bit);
            if (err == FM_OK)
            {
                if (bit < 0)
                {
                    err = FM_ERR_NO_MCAST_RESOURCES;
                }
            }
        }
    }

    if (err != FM_OK)
    {
        /* bail out if there's an error */
        return err;
    }

    /* return the initial bit position */
    *index = bit;

    return FM_OK;

}   /* end FindUnusedMulticastIndex */




/*****************************************************************************/
/** MarkVlanIndexAvailable
 *
 * \desc            Helper function to reset the MCAST_VLAN_TABLE usage bit.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_VLAN_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkVlanIndexAvailable(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    err = fmSetBitArrayBit(&info->clonedEntriesBitArray,
                           index,
                           FALSE );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmSetBitArrayBit(&info->vlanTableUsage,
                           index,
                           FALSE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = UpdateUsageCounters( info, 0, 0, -1, -1 );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking index 0x%x as available\n", index);

ABORT:
    return err;

}   /* end MarkVlanIndexAvailable */


/*****************************************************************************/
/** MarkVlanIndexUsed
 *
 * \desc            Helper function to set the usage bit and expiry for the
 *                  MCAST_VLAN_TABLE
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_VLAN_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkVlanIndexUsed(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    err = fmSetBitArrayBit(&info->vlanTableUsage,
                           index,
                           TRUE);
    if ( err == FM_OK )
    {
        err = UpdateUsageCounters( info, 0, 0, 1, 0 );
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking index 0x%x as used\n", index);

    return err;

}   /* end MarkVlanIndexUsed */


/*****************************************************************************/
/** MarkVlanIndexExpired
 *
 * \desc            Helper function to set the expiration on an entry.  This
 *                  does not mark the index as available.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_VLAN_TABLE index to mark.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status MarkVlanIndexExpired(fm10000_mtableInfo *info, fm_int index)
{
    fm_status err;

    /* We make entries expire by putting them in Quarantine */
    err = fmSetBitArrayBit(&info->clonedEntriesBitArray,
                           index,
                           TRUE);

    if ( err == FM_OK )
    {
        /* Update cloned entries counter */
        err = UpdateUsageCounters( info, 0, 0, 0, 1 );

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "Marking index 0x%x as expired\n", index);
    }
    return err;

}   /* end MarkVlanIndexExpired */


/*****************************************************************************/
/** VlanIndexInUse
 *
 * \desc            Helper function to set the expiration on an entry in
 *                  the MCAST_VLAN_TABLE. This does not mark the index as
 *                  available.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \param[in]       index is the MCAST_VLAN_TABLE index to mark.
 *
 * \return          True if the bit is used, false otherwise or on error.
 *
 *****************************************************************************/
static fm_bool VlanIndexInUse(fm10000_mtableInfo *info, fm_int index)
{
    fm_status    err;
    fm_bool      bit;

    err = fmGetBitArrayBit(&info->vlanTableUsage,
                           index,
                           &bit);

    if (err != FM_OK)
    {
        FM_LOG_FATAL( FM_LOG_CAT_MULTICAST,  "Unable to get bit value: %s\n",
                     fmErrorMsg(err) );

        return FALSE;
    }

    return bit;

}   /* end VlanIndexInUse */


/*****************************************************************************/
/** RecoverExpiredVlanIndices
 *
 * \desc            Helper function to recover expired entries in the
 *                  MCAST_VLAN_TABLE. This marks the expired indices as 
 *                  available.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       info points to the state structure that holds the
 *                  mtable management state.
 *
 * \return          FM_OK if success.
 *
 *****************************************************************************/
static fm_status RecoverExpiredVlanIndices(fm_int sw, fm10000_mtableInfo *info)
{
    fm_status err;
    fm_int    vlanIndex;
    fm_switch *switchPtr;
    
    switchPtr = GET_SWITCH_PTR(sw);
    
    /* walk the list of cloned entries and mark them as available */
    vlanIndex = 0;
    while (vlanIndex >= 0)
    {
        err = fmFindBitInBitArray(&info->clonedEntriesBitArray,
                                  vlanIndex + 1,
                                  TRUE,
                                  &vlanIndex);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        if (vlanIndex > 0)
        {
            err = MarkVlanIndexAvailable(info, vlanIndex);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            err = switchPtr->WriteUINT64( sw,
                                          FM10000_MOD_MCAST_VLAN_TABLE(vlanIndex, 0),
                                          0 );
            FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
        }
    }

    info->clonedEntriesCount = 0;

    /* Clear the MCAST_VLAN_TABLE cloned entries bit array */
    err = fmClearBitArray(&info->clonedEntriesBitArray);
    return err;

}   /* end RecoverExpiredVlanIndices */



/*****************************************************************************/
/** MTableAddListener
 * \ingroup intMulticast
 *
 * \desc            Internal version of the function to add listener
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       mcastGroup is the Multicast Group on which to operate
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       physPort is the physical port the listener belongs to
 * 
 * \param[in]       listener is the structure which has all MCAST_VLAN_TABLE fields
 *                  that has to be added.
 *
 * \param[in]       stpState is the STP state of this listener
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
static fm_status MTableAddListener(fm_int              sw, 
                                   fm_int              mcastGroup, 
                                   fm_int              repliGroup,
                                   fm_int              physPort, 
                                   fm10000_mtableEntry listener,
                                   fm_int              stpState ) 
{
    fm_status      err = FM_OK;
    fm_switch *    switchPtr;
    fm10000_mtableInfo *     info;
    fm_uintptr     mcastIndex;
    fm_int         lenTableIndex;
    fm_int         vlanIndex;
    fm_int         oldVlanIndex;
    fm_int         len;
    fm_portmask    logicalMask;
    fm_portmask    physicalMask;
    fm_portmask    mcastMask;
    fm_int         groupSize;
    fm_bool        addingNewPort = FALSE;
    fm_uint64      mcastDestReg;
    fm_uint32      lenTableReg;
    fm_int         logicalPort;
    fm_int         i;
    fm_int         activeCount;
    fm_intMulticastGroup *  mcastGroupInfo;
    fm_int         portPosition;
    fm_bool        cloneLenTable = FALSE;
    fm_bool        cloneVlanTable = FALSE;
    fm_int         tmpIndex;
    fm_int         lenTableIdx; // Contains beginning of LEN_TABLE_BLOCK

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d mcastGroup=%d repliGroup=%d physPort=%d vlan=%d dglort=%d vlanUpdate=%d dglortUpdate=%d stpState=%d\n",
                 sw,
                 mcastGroup,
                 repliGroup,
                 physPort,
                 listener.vlan,
                 listener.dglort,
                 listener.vlanUpdate,
                 listener.dglortUpdate,
                 stpState);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);

    /* get the multicast index associated with this replication group */
    err = fmTreeFind(&info->mtableDestIndex, repliGroup, (void **) &mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* get the multicast group info */
    err = fmTreeFind(&switchPtr->mcastPortTree,
                     mcastGroup,
                     (void **) &mcastGroupInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "mcastIndex for repliGroup %d is %" FM_FORMAT_64 "u\n",
                 repliGroup,
                 (fm_uint64) mcastIndex);

    if ( (listener.vlanUpdate == TRUE) &
         (stpState != FM_STP_STATE_FORWARDING) &&
         (!mcastGroupInfo->bypassEgressSTPCheck) )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Listener is remembered but not added to hw because "
                     "stpState is not forwarding\n");
        /* Remember the listener and will be added when state comes up */
        err = AddToEntryListForListener(info,
                                        physPort,
                                        listener.vlan,
                                        listener.dglort,
                                        mcastGroup,
                                        repliGroup,
                                        0,
                                        0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* get the destination mask associated with this mcast group */
    err = switchPtr->GetLogicalPortAttribute(sw,
                                             mcastGroup,
                                             FM_LPORT_DEST_MASK,
                                             &logicalMask);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /**************************************************
     * now map the logical port mask to the physical
     * port mask for this mcast group.
     **************************************************/

    err = fmPortMaskLogicalToPhysical( switchPtr,
                                       &logicalMask,
                                       &physicalMask );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* now, get the current MCAST_DEST_TABLE register from the switch  */
    err = switchPtr->ReadUINT64(sw,
                                FM10000_SCHED_MCAST_DEST_TABLE(mcastIndex, 0),
                                &mcastDestReg );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    COPY_DESTMASK_TO_PORTMASK(mcastDestReg, mcastMask);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "mcast mask = 0x%08x%08x\n",
                 mcastMask.maskWord[1],
                 mcastMask.maskWord[0]);

    /***************************************************
     * We compute the number of ports in the mask,
     * using the mcast mask.  The number of ports
     * in the mcast mask is strictly greater than or
     * equal to the number of ports in the external (L2)
     * mask.
     **************************************************/
    err = fmGetPortMaskCount( &mcastMask, &groupSize );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "group size = %d\n", groupSize);

    if ( !FM_PORTMASK_GET_BIT( &mcastMask, physPort ) )
    {
        /* yes */
        addingNewPort = TRUE;
    }

    /* If this is the first listener in the group */
    if (groupSize == 0)
    {
        /* find an available index in the MCAST_VLAN_TABLE */
        err = FindUnusedVlanTableBlock(sw, info, VLAN_TABLE_MIN_BLOCK_SIZE, &vlanIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        /* found it? */
        if (vlanIndex == -1)
        {
            /* no */
            err = FM_ERR_NO_MCAST_RESOURCES;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        /* find an available index in the MCAST_LEN_TABLE */
        err = FindUnusedLenTableBlock(info, LEN_TABLE_MIN_BLOCK_SIZE, &lenTableIdx);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        /* found it? */
        if (lenTableIdx == -1)
        {
            /* no */
            err = FM_ERR_NO_MCAST_RESOURCES;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        lenTableIndex = lenTableIdx;
        portPosition = 0;
    }
    else
    {
        lenTableIdx = FM_GET_FIELD64( mcastDestReg,
                                      FM10000_SCHED_MCAST_DEST_TABLE,
                                      LenTableIdx );

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "lenTableIndex for this repliGroup is %d\n", lenTableIdx );

        /***************************************************
         * Find the insert position of the new listener
         * within the starting block.
         **************************************************/

        for (i = 0, portPosition = 0 ; i < physPort ; i++)
        {
            if ( FM_PORTMASK_GET_BIT(&mcastMask, i) != 0 )
            {
                portPosition++;
            }
        }
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Insert/Update position for port %d in lenTable block is %d\n",
                      physPort,
                      portPosition);  


        if ( addingNewPort )
        {
            /* find an available index in the MCAST_LEN_TABLE */
            err = FindUnusedLenTableBlock(info, groupSize + 1, &tmpIndex);
            FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

            /* found it? */
            if (tmpIndex == -1)
            {
                /* no */
                err = FM_ERR_NO_MCAST_RESOURCES;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }
            cloneLenTable = TRUE;

            /* find an available index in the MCAST_VLAN_TABLE */
            err = FindUnusedVlanTableBlock(sw, info, VLAN_TABLE_MIN_BLOCK_SIZE, &vlanIndex);
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

            /* found it? */
            if (vlanIndex == -1)
            {
                /* no */
                err = FM_ERR_NO_MCAST_RESOURCES;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }
        }
        else
        {
            lenTableIndex = lenTableIdx + portPosition;
            /* now, get the current MCAST_LEN_TABLE register from the switch  */
            err = switchPtr->ReadUINT32(sw,
                                        FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                        &lenTableReg );
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

            /***************************************************
             * vlanIndex is the field in MCAST_DEST_TABLE,
             * indicating the index into MCAST_VLAN_TABLE.  This
             * is not to be confused with mcastIndex, which is
             * the index into MCAST_DEST_TABLE.
             **************************************************/
            oldVlanIndex = FM_GET_FIELD( lenTableReg,
                                         FM10000_SCHED_MCAST_LEN_TABLE,
                                         L3_McastIdx );
            len          = FM_GET_FIELD( lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_Repcnt );

            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "vlanIndex for this repliGroup is %d len is %d\n", oldVlanIndex, len );

            if ( ( (oldVlanIndex + len + 1) < FM10000_MAX_MCAST_VLAN_INDEX ) && 
                 !VlanIndexInUse(info, oldVlanIndex + len + 1))
            {
                vlanIndex = oldVlanIndex;
                /* lenTableIndex remains same */
            }
            else
            {
                /* find an available index in the MCAST_LEN_TABLE */
                err = FindUnusedVlanTableBlock(sw, info, len + 2, &tmpIndex);
                FM_LOG_EXIT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

                /* found it? */
                if (tmpIndex == -1)
                {
                    /* no */
                    err = FM_ERR_NO_MCAST_RESOURCES;
                    FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
                }
                cloneVlanTable = TRUE;
            }
        }

        /* At this point, it is made sure that free slots are available in tables. */
        if (cloneLenTable)
        {
            err = CloneLenTableBlock(sw, info, TRUE, &lenTableIdx, portPosition,
                                     groupSize, &mcastMask, mcastGroup, repliGroup);
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

            lenTableIndex = lenTableIdx + portPosition;
        }

        if (cloneVlanTable)
        {
            err = CloneVlanTableBlock(sw, info, TRUE, &oldVlanIndex, lenTableIndex, len + 1, 
                                            physPort, mcastGroup, repliGroup, -1);
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

            vlanIndex = oldVlanIndex;
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST, 
                 "LenTableIndex: %d VlanTableIndex: %d portPosition in LenTableBlock: %d\n", 
                 lenTableIndex,
                 vlanIndex,
                 portPosition);


    if (addingNewPort)
    {
        FM_PORTMASK_SET_BIT(&mcastMask, physPort, 1);
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "mcast mask = 0x%08x%08x\n",
                     mcastMask.maskWord[1],
                     mcastMask.maskWord[0]);
    }

    /* Yes, because its state is forwarding, or we are in STP bypass mode */
    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "adding listener...\n");
    err = AddListener(sw,
                      info,
                      mcastGroup,
                      repliGroup,
                      lenTableIndex,
                      vlanIndex,
                      &mcastMask,
                      physPort,
                      listener);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /**************************************************
     * To conclude update the MCAST_DEST_TABLE register
     * and update the attributes for the logical port
     * corresponding to this multicast group
     **************************************************/

    COPY_PORTMASK_TO_DESTMASK(mcastMask, mcastDestReg);

    FM_SET_FIELD64(mcastDestReg,
                   FM10000_SCHED_MCAST_DEST_TABLE,
                   LenTableIdx,
                   lenTableIdx);

    err = switchPtr->WriteUINT64( sw,
                                  FM10000_SCHED_MCAST_DEST_TABLE(mcastIndex, 0),
                                  mcastDestReg );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "For dest table index %" FM_FORMAT_64 "u, "
                 "multicast lenTable index %u, "
                 "multicast vlan index %u\n",
                 (fm_uint64) mcastIndex,
                 lenTableIndex,
                 vlanIndex);

    /* Now we can recover the expired len indices, since DEST_TABLE
     * is already modified to point to new LenTable entries. */
    err = RecoverExpiredLenIndices(sw, info);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /*******************************************************
     * get the logical port and update the mask accordingly
     * basically we'll set the bit in the GLORT_DEST_TABLE
     * only if there are active listeners
     ******************************************************/

    err = fmMapPhysicalPortToLogical(switchPtr, physPort, &logicalPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = GetListenersCount(info, repliGroup, physPort, &activeCount, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /***************************************************
     * Here we make sure that the L2 mask's bit for
     * this port is only set if the number of active
     * listeners is non-zero.  The activeCount only
     * includes forwarding (port, vlan) pairs.
     **************************************************/
    fmSetPortInPortMask(sw, &logicalMask, logicalPort, (activeCount != 0));

    err = switchPtr->SetLogicalPortAttribute(sw,
                                             mcastGroup,
                                             FM_LPORT_DEST_MASK,
                                             &logicalMask);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Physical multicast destination mask for group %d, index %d: %08x%08x\n",
                 repliGroup,
                 vlanIndex,
                 mcastMask.maskWord[1],
                 mcastMask.maskWord[0]);

    FM_LOG_DEBUG( FM_LOG_CAT_MULTICAST,
                  "Logical destination mask for port %d, glort 0x%x: %08x%08x\n",
                  mcastGroup,
                  GET_PORT_PTR(sw, mcastGroup)->glort,
                  logicalMask.maskWord[1],
                  logicalMask.maskWord[0] );

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end MTableAddListener */



/*****************************************************************************/
/** AddToEntryListForListener
 *
 * \desc            Adds a given entry to the entry usage list for the given
 *                  (port, vlan).
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       physPort is the physical port of the listener.
 *
 * \param[in]       vlan is the VLAN ID of the listener.
 *
 * \param[in]       dglort is the GDLORT of the listener.
 *
 * \param[in]       mcastGroup is the multicast group number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       vlanIndex is the MCAST_VLAN_TABLE index being added.
 *
 * \param[in]       lenTableIndex is the index of this listener in MCAST_LEN_TABLE
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AddToEntryListForListener(fm10000_mtableInfo *info,
                                           fm_int              physPort,
                                           fm_uint16           vlan,
                                           fm_uint16           dglort,
                                           fm_int              mcastGroup,
                                           fm_int              repliGroup,
                                           fm_int              vlanIndex,
                                           fm_int              lenTableIndex)
{
    fm_status                 err;
    fm_uint64                 key;
    fm_dlist                 *entryList;
    fm_dlist                 *entryListPerLenIndex;
    fm_dlist_node            *entryListNode;
    fm10000_entryListWrapper *entry;
    fm_tree                  *dglortTree; 

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "info=%p physPort=%d vlan=%d dglort=0x%x index=%d\n",
                 (void *) info, physPort, vlan, dglort, vlanIndex);

    if (lenTableIndex != 0)
    {
        err = fmTreeFind(&info->entryListPerLenIndex,
                         lenTableIndex,
                         (void **) &entryListPerLenIndex);
        if (err == FM_ERR_NOT_FOUND)
        {
            /***************************************************
             * Allocate a new list if necessary.
             **************************************************/

            entryListPerLenIndex = (fm_dlist *) fmAlloc( sizeof(fm_dlist) );

            if (entryListPerLenIndex == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_NO_MEM);
            }

            fmDListInit(entryListPerLenIndex);

            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Key not found, allocated new list at "
                         "%p for key %d\n", (void *) entryListPerLenIndex, lenTableIndex);

            err = fmTreeInsert(&info->entryListPerLenIndex, lenTableIndex, entryListPerLenIndex);

            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
            }
        }
        else if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Found list for lenTableIndex %d\n", lenTableIndex);
        }
    }
    else
    {
        entryListPerLenIndex = NULL;
    }

    /***************************************************
     * Search for the node using the computed key.
     **************************************************/

    key = GET_LISTENER_KEY(physPort, vlan);

    err = fmTreeFind(&info->entryList,
                     key,
                     (void **) &dglortTree);


    if (err == FM_ERR_NOT_FOUND)
    {
        /* Allocate and initialize a tree of listeners for a given physical port
         * and vlan. */
        dglortTree = (fm_tree *) fmAlloc( sizeof(fm_tree) );
        if (dglortTree == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_NO_MEM);
        }

        fmTreeInit(dglortTree);

        err = fmTreeInsert(&info->entryList, key, dglortTree);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
        }
    }
    else if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Found list for key %lld\n", key);
    }

    err = fmTreeFind(dglortTree,
                     dglort,
                     (void **) &entryList);
    
    if (err == FM_ERR_NOT_FOUND)
    {
        /***************************************************
         * Allocate a new list if necessary.
         **************************************************/

        entryList = (fm_dlist *) fmAlloc( sizeof(fm_dlist) );

        if (entryList == NULL)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_NO_MEM);
        }

        fmDListInit(entryList);

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Key not found, allocated new list at "
                     "%p for key %lld\n", (void *) entryList, key);

        err = fmTreeInsert(dglortTree, dglort, entryList);

        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
        }
    }
    else if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Found list for dglort 0x%x\n", dglort);
    }

    /***************************************************
     * Search the list to ensure that the index is not
     * already in the list.
     **************************************************/

    for (entryListNode = entryList->head ; entryListNode ; )
    {
        entry = (fm10000_entryListWrapper *) entryListNode->data;

        if ( entry->mcastGroup     == mcastGroup &&
             entry->repliGroup     == repliGroup &&
             entry->vlanEntryIndex == vlanIndex  &&
             entry->lenTableIndex  == lenTableIndex)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_OK );
        }

        entryListNode = entryListNode->next;
    }

    /***************************************************
     * Insert a new entry at the end of the list.
     **************************************************/

    err = InsertListenerEntry( info,
                               mcastGroup,
                               repliGroup,
                               physPort,
                               vlanIndex,
                               lenTableIndex,
                               entryList,
                               entryListPerLenIndex);


    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end AddToEntryListForListener */




/*****************************************************************************/
/** ModifyEntryListForListener
 *
 * \desc            Deletes a given entry from the entry usage list for the
 *                  given (port, vlan) pair.
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       physPort is the physical port of the listener.
 *
 * \param[in]       vlan is the VLAN ID of the listener.
 *
 * \param[in]       dglort is the DGLORT of the listener.
 *
 * \param[in]       mcastGroup is the multicast group number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       oldVlanIndex is the MCAST_VLAN_TABLE index being modified.
 *
 * \param[in]       newVlanIndex is the MCAST_VLAN_TABLE index to update with.
 *
 * \param[in]       oldLenTableIndex is the MCAST_LEN_TABLE index being modified.
 *
 * \param[in]       newLenTableIndex is the MCAST_LEN_TABLE index to update with.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ModifyEntryListForListener(fm10000_mtableInfo *info,
                                            fm_int              physPort,
                                            fm_uint16           vlan,
                                            fm_uint16           dglort,
                                            fm_int              mcastGroup,
                                            fm_int              repliGroup,
                                            fm_int              oldVlanIndex,
                                            fm_int              newVlanIndex,
                                            fm_int              oldLenTableIndex,
                                            fm_int              newLenTableIndex)
{
    fm_status                 err;
    fm_uint64                 key;
    fm_dlist                 *entryList;
    fm_dlist_node            *entryListNode;
    fm10000_entryListWrapper *entry;
    fm_tree                  *dglortTree;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "info=%p physPort=%d vlan=%d dglort=0x%x mcastgroup=%d repligroup=%d "
                 "oldIndex=%d newIndex=%d\n",
                 (void *) info, physPort, vlan, dglort, mcastGroup, repliGroup,
                 oldVlanIndex, newVlanIndex );

    /***************************************************
     * Search for the node using the computed key.
     **************************************************/

    key = GET_LISTENER_KEY(physPort, vlan);


    err = fmTreeFind(&info->entryList,
                     key,
                     (void **) &dglortTree);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found tree for key %lld at %p\n", key, (void *) dglortTree);

    err = fmTreeFind(dglortTree,
                     dglort,
                     (void **) &entryList);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found list for key %lld dglort 0x%xat %p\n",
                 key,
                 dglort,
                 (void *) entryList);

    /***************************************************
     * Search for the index in the list.
     **************************************************/

    for (entryListNode = entryList->head ; entryListNode ; )
    {
        entry = (fm10000_entryListWrapper *) entryListNode->data;

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Found node belonging to mcastgroup %d, repliGroup %d"
                     "vlanIndex %d lenTableIndex %d next=%p\n",
                     entry->mcastGroup,
                     entry->repliGroup,
                     entry->vlanEntryIndex,
                     entry->lenTableIndex,
                     (void *)entryListNode->next);

        if ( entry->repliGroup     == repliGroup &&
             entry->vlanEntryIndex == oldVlanIndex && 
             entry->lenTableIndex  == oldLenTableIndex)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Node for old: %d, %d found, updating to %d %d\n",
                         oldLenTableIndex, oldVlanIndex, 
                         newLenTableIndex, newVlanIndex);

            entry->vlanEntryIndex = newVlanIndex;
            entry->lenTableIndex  = newLenTableIndex;

            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err );
        }

        entryListNode = entryListNode->next;
    }

    /***************************************************
     * It is an error if the index is not found in the
     * list.
     **************************************************/
    FM_LOG_FATAL(FM_LOG_CAT_MULTICAST,
                 "No node found for port %d, location %d,%d\n",
                 physPort, oldVlanIndex, oldLenTableIndex);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_INVALID_ARGUMENT);

}   /* end ModifyEntryListForListener */




/*****************************************************************************/
/** ModifyEntryListLenIndex
 *
 * \desc            Modifies the entry usage list in which lenTableIndex is 
 *                  used as key. Listener wrapper objects mapped to oldLenTableIndex
 *                  is removed and mapped to newLenTableIndex.
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       oldLenTableIndex is the MCAST_LEN_TABLE index being modified.
 *
 * \param[in]       newLenTableIndex is the MCAST_LEN_TABLE index to update with.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status ModifyEntryListLenIndex(fm10000_mtableInfo *info,
                                  fm_int oldLenTableIndex,
                                  fm_int newLenTableIndex)
{
    fm_dlist                 *entryListPerLenIndex;
    fm_dlist_node            *entryListPerLenIndexNode;
    fm_status                 err;
    fm10000_entryListWrapper *entry;
    

    err = fmTreeFind(&info->entryListPerLenIndex,
                     oldLenTableIndex,
                     (void **) &entryListPerLenIndex);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found list for key %d at %p\n", 
                 oldLenTableIndex, 
                 (void *) entryListPerLenIndex);

    err = fmTreeRemoveCertain(&info->entryListPerLenIndex,
                              oldLenTableIndex,
                              NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);

    err = fmTreeInsert(&info->entryListPerLenIndex,
                        newLenTableIndex,
                        entryListPerLenIndex);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);

    for (entryListPerLenIndexNode = entryListPerLenIndex->head ; entryListPerLenIndexNode ; )
    {
        entry = (fm10000_entryListWrapper *) entryListPerLenIndexNode->data;
        if (!entry)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
        }
        entry->lenTableIndex = newLenTableIndex;
        entryListPerLenIndexNode = entryListPerLenIndexNode->next;
    }

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end ModifyEntryListLenIndex */



/*****************************************************************************/
/** GetVlanIndexEntry
 *
 * \desc            Retrieves vlanIndex of the listener in a mcastGroup.
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       physPort is the physical port of the listener.
 *
 * \param[in]       vlan is the VLAN ID of the listener.
 *
 * \param[in]       dglort is the DGLORT of the listener.
 *
 * \param[in]       mcastGroup is the multicast group number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       lenTableIndex is the index of this listener in MCAST_LEN_TABLE.
 *
 * \param[out]      vlanIndex points to externally allocated storage in which
 *                  index of MCAST_VLAN_TABLE in which this listener is stored 
 *                  returned.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status GetVlanIndexEntry(fm10000_mtableInfo *info,
                            fm_int              physPort,
                            fm_uint16           vlan,
                            fm_uint16           dglort,
                            fm_int              mcastGroup,
                            fm_int              repliGroup,
                            fm_int              lenTableIndex,
                            fm_int *            vlanIndex)
{
    fm_status                 err;
    fm_uint64                 key;
    fm_dlist                 *entryList;
    fm_dlist_node            *entryListNode;
    fm10000_entryListWrapper *entry;
    fm_int                    numFound = 0;
    fm_tree                  *dglortTree;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "info=%p physPort=%d vlan=%d dglort=0x%x mcastGroup = %d"
                 "repliGroup=%d "
                 "lentableIndex %d vlanIndex=%p\n",
                 (void *) info, physPort, vlan, dglort, mcastGroup, repliGroup,
                 lenTableIndex, (void *)vlanIndex );

    /***************************************************
     * Search for the node using the computed key.
     **************************************************/

    key = GET_LISTENER_KEY(physPort, vlan);

    err = fmTreeFind(&info->entryList,
                     key,
                     (void **) &dglortTree);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found tree for key %lld at %p\n", key, (void *) dglortTree);

    err = fmTreeFind(dglortTree,
                     dglort,
                     (void **) &entryList);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found list for key %lld dglort 0x%x at %p\n",
                 key, dglort, (void *) entryList);

    /***************************************************
     * Search for the index in the list.
     **************************************************/

    for (entryListNode = entryList->head ; entryListNode ; )
    {
        entry = (fm10000_entryListWrapper *) entryListNode->data;

        if (entry->mcastGroup     == mcastGroup &&
            entry->repliGroup     == repliGroup &&
            entry->lenTableIndex  == lenTableIndex )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Node found in list\n");
            numFound++;
            *vlanIndex = entry->vlanEntryIndex;
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Mismatch: mcastgroup: %d ? %d, repligroup: %d ? %d,"
                         "len index: %d ? %d\n",
                         mcastGroup,
                         entry->mcastGroup,
                         repliGroup,
                         entry->repliGroup,
                         lenTableIndex,
                         entry->lenTableIndex );
        }

        entryListNode = entryListNode->next;
    }

    if (numFound != 1)
    {
        /***************************************************
         * It is an error if the index is not found in the
         * list.
         **************************************************/
        FM_LOG_FATAL(FM_LOG_CAT_MULTICAST,
                     "Number of nodes found %d but expected 1\n", numFound);

        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_INVALID_ARGUMENT);
    }
    else
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_OK);
    }
    
}   /* end GetVlanIndexEntry */



/*****************************************************************************/
/** DeleteFromEntryListForListener
 *
 * \desc            Deletes a given entry from the entry usage list for the
 *                  given (port, vlan) pair.
 *
 * \param[in]       info points to the multicast table state information.
 *
 * \param[in]       physPort is the physical port of the listener.
 *
 * \param[in]       vlan is the VLAN ID of the listener.
 *
 * \param[in]       dglort is the DGLORT of the listener.
 *
 * \param[in]       mcastGroup is the multicast group number.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       vlanIndex is the MCAST_VLAN_TABLE index being deleted.
 *
 * \param[in]       lenTableIndex is the index of this listener in MCAST_LEN_TABLE.
 *
 * \param[in]       updateTrees should be set TRUE if the trees should be updated
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteFromEntryListForListener(fm10000_mtableInfo *info,
                                                fm_int              physPort,
                                                fm_uint16           vlan,
                                                fm_uint16           dglort,
                                                fm_int              mcastGroup,
                                                fm_int              repliGroup,
                                                fm_int              vlanIndex,
                                                fm_int              lenTableIndex,
                                                fm_bool             updateTrees)
{
    fm_status                 err;
    fm_uint64                 key;
    fm_dlist                 *entryList;
    fm_dlist                 *entryListPerLenIndex;
    fm_dlist_node            *entryListNode;
    fm_dlist_node            *entryListPerLenIndexNode;
    fm10000_entryListWrapper *entry;
    fm_tree                  *dglortTree;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "info=%p physPort=%d vlan=%d dglort 0x%x group=%d index=%d\n",
                 (void *) info, physPort, vlan, dglort, repliGroup, vlanIndex );

    /***************************************************
     * Search for the node using the computed key.
     **************************************************/

    key = GET_LISTENER_KEY(physPort, vlan);

    err = fmTreeFind(&info->entryList,
                     key,
                     (void **) &dglortTree);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }
    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found tree for key %lld at %p\n", key, (void *) dglortTree);

    err = fmTreeFind(dglortTree,
                     dglort,
                     (void **) &entryList);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Found list for key %lld dglort 0x%x at %p\n",
                 key, dglort, (void *) entryList);

    if (lenTableIndex != 0)
    {
        err = fmTreeFind(&info->entryListPerLenIndex,
                         lenTableIndex,
                         (void **) &entryListPerLenIndex);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);
        }
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Found list for key %d at %p\n", 
                     lenTableIndex, 
                     (void *) entryListPerLenIndex);
    }
    else
    {
        entryListPerLenIndex = NULL;
    }

    /***************************************************
     * Search for the index in the list.
     **************************************************/

    for (entryListNode = entryList->head ; entryListNode ; )
    {
        entry = (fm10000_entryListWrapper *) entryListNode->data;

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Found node belonging to mcastgroup %d, repliGroup %dindex %d\n",
                     entry->mcastGroup,
                     entry->repliGroup,
                     entry->vlanEntryIndex );

        if (entry->mcastGroup     == mcastGroup &&
            entry->repliGroup     == repliGroup &&
            entry->vlanEntryIndex == vlanIndex )
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Node for index found in entyList\n");
            
            if (lenTableIndex != 0 && entryListPerLenIndex != NULL)
            {
                for (entryListPerLenIndexNode = entryListPerLenIndex->head ; entryListPerLenIndexNode ; )
                {
                    entry = (fm10000_entryListWrapper *) entryListPerLenIndexNode->data;
                    if (entry->mcastGroup     == mcastGroup &&
                        entry->repliGroup     == repliGroup &&
                        entry->vlanEntryIndex == vlanIndex )
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                                     "Node for index found in entryListPerLenIndex." 
                                     "Removing node\n");

                        err = RemoveListenerEntry(info,
                                                  repliGroup,
                                                  physPort,
                                                  lenTableIndex,
                                                  entryList,
                                                  entryListNode,
                                                  entryListPerLenIndex,
                                                  entryListPerLenIndexNode);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                        if ( (entryList->head == NULL) && updateTrees )
                        {
                            err =
                                fmTreeRemoveCertain(dglortTree,
                                                    dglort,
                                                    FreeEntryUsageList);
                            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);
                            
                            if (fmTreeSize(dglortTree) == 0)
                            {
                                err =
                                    fmTreeRemoveCertain(&info->entryList,
                                                        key,
                                                        FreeEntryUsageTree);
                                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);
                            }
                        }
                        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

                    }
                    entryListPerLenIndexNode = entryListPerLenIndexNode->next;
                }
            }
            else
            {
                err = RemoveListenerEntry(info,
                                          repliGroup,
                                          physPort,
                                          lenTableIndex,
                                          entryList,
                                          entryListNode,
                                          NULL,
                                          NULL);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                if ( (entryList->head == NULL) && updateTrees )
                {
                    err =
                        fmTreeRemoveCertain(dglortTree,
                                            dglort,
                                            FreeEntryUsageList);
                    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);
                    
                    if (fmTreeSize(dglortTree) == 0)
                    {
                        err =
                            fmTreeRemoveCertain(&info->entryList,
                                                key,
                                                FreeEntryUsageTree);
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_MULTICAST,  err);
                    }
                }
                FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err );
            }
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                         "Mismatch: mcastgroup: %d ? %d, repligroup: %d ? %d,"
                         " index: %d ? %d\n",
                         mcastGroup,
                         entry->mcastGroup,
                         repliGroup,
                         entry->repliGroup,
                         vlanIndex,
                         entry->vlanEntryIndex );

        }

        entryListNode = entryListNode->next;
    }

    /***************************************************
     * It is an error if the index is not found in the
     * list.
     **************************************************/
    FM_LOG_FATAL(FM_LOG_CAT_MULTICAST,
                 "No node found for index %d\n", vlanIndex);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_INVALID_ARGUMENT);

}   /* end DeleteFromEntryListForListener */




/*****************************************************************************/
/** MTableDeleteListener
 * \ingroup intMulticast
 *
 * \desc            Internal version of the function to delete listeners
 *
 * \param[in]       sw is the switch on which to operate
 *
 * \param[in]       mcastGroup is the Multicast Group on which to operate
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       physPort is the physical port the listener belongs to
 *
 * \param[in]       vlan is the VLAN the listener belongs to
 *
 * \param[in]       dglort is the DGLORT of the listener
 *
 * \param[in]       stpState is the STP state of this listener
 *
 * \param[in]       ignoreStpState is set to TRUE if the operation needs to be
 *                  forced regardless of the STP state. That is needed when
 *                  the STP state is actually transitioning from "forwarding"
 *                  and listeners must be removed from the MCAST_VLAN_TABLE
 *
 * \param[in]       updateDestMask should be set TRUE if the dest mask should
 *                  be updated.
 *
 * \param[in]       updateTrees should be set TRUE if the trees should be updated
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_ARGUMENT if the port is not part of the
 *                  multicast group
 *
 *****************************************************************************/
static fm_status MTableDeleteListener(fm_int    sw,
                                      fm_int    mcastGroup,
                                      fm_int    repliGroup,
                                      fm_int    physPort,
                                      fm_int    vlan,
                                      fm_uint16 dglort,
                                      fm_int    stpState,
                                      fm_bool   ignoreStpState,
                                      fm_bool   updateDestMask,
                                      fm_bool   updateTrees)
{

    fm_status          err = FM_OK;
    fm_switch         *     switchPtr;
    fm10000_mtableInfo *     info;
    fm_uintptr         mcastIndex;
    fm_int             oldVlanIndex;
    fm_portmask        logicalMask;
    fm_portmask        mcastMask;
    fm_int             groupSize;
    fm_int             logicalPort;
    fm_int             i;
    fm_int             activeCount;
    fm_intMulticastGroup *  mcastGroupInfo;
    fm_uint64          mcastDestReg; 
    fm_uint32          lenTableReg;
    fm_int             len;
    fm_int             lenTableIdx;
    fm_int             lenTableIndex;
    fm_int             delVlanIndex;
    fm_int             portPosition;
    fm_int             tmpIndex;
    fm_bool            cloneLenTable = FALSE;
    fm_bool            cloneVlanTable = FALSE;
    fm_int             finalVlanIndex;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d mcastGroup=%d repliGroup=%d physPort=%d vlan=%d stpState=%d ignoreStpState=%d\n",
                 sw,
                 mcastGroup,
                 repliGroup,
                 physPort,
                 vlan,
                 stpState,
                 ignoreStpState);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);
    FM_CLEAR(logicalMask);

    /* get the multicast group info */
    err = fmTreeFind(&switchPtr->mcastPortTree,
                     mcastGroup,
                     (void **) &mcastGroupInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if ((stpState != FM_STP_STATE_FORWARDING) &&
        (!ignoreStpState) &&
        (!mcastGroupInfo->bypassEgressSTPCheck))
    {
        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Listener removed from API maintained list of listeners.\n");
        /* The listener that is to be deleted can only be added when its
         * correspoinding STP state is not forwarding. Hence it will have 
         * vlanIndex and lenTableIndex as 0. 
         * 
         * The function fm10000MTableUpdateListenerState function deletes 
         * the listener completely when STP state becomes non-forwarding. */
        err = DeleteFromEntryListForListener(info,
                                             physPort,
                                             vlan,
                                             dglort,
                                             mcastGroup,
                                             repliGroup,
                                             0,
                                             0,
                                             updateTrees);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* get the multicast index associated with this replication group */
    err = fmTreeFind(&info->mtableDestIndex, repliGroup, (void **) &mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* now, get the current MCAST_DEST_TABLE register from the switch  */
    err = switchPtr->ReadUINT64(sw,
                                FM10000_SCHED_MCAST_DEST_TABLE(mcastIndex, 0),
                                &mcastDestReg );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    COPY_DESTMASK_TO_PORTMASK(mcastDestReg, mcastMask);

    err = fmGetPortMaskCount( &mcastMask, &groupSize );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "group size = %d\n", groupSize);

    lenTableIdx = FM_GET_FIELD64( mcastDestReg,
                                  FM10000_SCHED_MCAST_DEST_TABLE,
                                  LenTableIdx );
    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "lenTableIndex for this repliGroup is %d\n", lenTableIdx );

    /***************************************************
     * Find the insert position of the new listener
     * within the starting block.
     **************************************************/

    for (i = 0, portPosition = 0 ; i < physPort ; i++)
    {
        if ( FM_PORTMASK_GET_BIT(&mcastMask, i) != 0 )
        {
            portPosition++;
        }
    }
    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "Update position for port %d is %d\n",
                  physPort,
                  portPosition);  

    lenTableIndex = lenTableIdx + portPosition;

    /* now, get the current MCAST_LEN_TABLE register from the switch  */
    err = switchPtr->ReadUINT32(sw,
                                FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                &lenTableReg );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /***************************************************
     * vlanIndex is the field in MCAST_DEST_TABLE,
     * indicating the index into MCAST_VLAN_TABLE.  This
     * is not to be confused with mcastIndex, which is
     * the index into MCAST_DEST_TABLE.
     **************************************************/
    oldVlanIndex = FM_GET_FIELD( lenTableReg,
                                 FM10000_SCHED_MCAST_LEN_TABLE,
                                 L3_McastIdx );

    len = FM_GET_FIELD( lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_Repcnt )
        + 1;

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "vlanIndex for this repliGroup is %d len is %d\n", oldVlanIndex, len );

    delVlanIndex = 0; /* Avoid compile warning error*/
    err = GetVlanIndexEntry(info,
                            physPort,
                            vlan,
                            dglort,
                            mcastGroup,
                            repliGroup,
                            lenTableIndex,
                            &delVlanIndex);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "vlanEntryIndex to be deleted: %d\n", delVlanIndex);

    /* If this is the last entry in the vlanTable group */
    if (len == 1)
    {
        /* If this is the last entry in the LenTable group */
        if (groupSize == 1)
        {
            err = MarkLenTableIndexExpired(info, lenTableIdx);
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
            
            lenTableIdx = 0;
        }
        else
        {
            /* Check there is enough room for cloning to be done later */
            err = FindUnusedLenTableBlock(info, groupSize - 1, &tmpIndex);
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

            /* found it? */
            if (tmpIndex == -1)
            {
                /* no */
                err = FM_ERR_NO_MCAST_RESOURCES;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }
            cloneLenTable = TRUE;
        }
        
        FM_PORTMASK_SET_BIT(&mcastMask, physPort, 0);

        /* No vlan table block for this port */    
        finalVlanIndex = 0;
        err = MarkVlanIndexExpired(info, delVlanIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }
    else
    {
        /* Check there is enough room for cloning to be done later */
        err = FindUnusedVlanTableBlock(sw, info, len - 1, &tmpIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        /* found it? */
        if (tmpIndex == -1)
        {
            /* no */
            err = FM_ERR_NO_MCAST_RESOURCES;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }

        cloneVlanTable = TRUE;
    }

    if (cloneVlanTable)
    {
        err = CloneVlanTableBlock(sw, info, FALSE, &oldVlanIndex, lenTableIndex, len, 
                                  physPort, mcastGroup, repliGroup, delVlanIndex);
        FM_LOG_ABORT_ON_ERR ( FM_LOG_CAT_MULTICAST, err );

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Cloned vlan block start index:%d\n", oldVlanIndex);
        finalVlanIndex = oldVlanIndex;
    }

    if (cloneLenTable)
    {
        /* Note: If cloning in len table is needed, then vlan table need not 
         *       be cloned. It means there is no active vlans in the vlan group
         *       and should have been marked as expired already */

        err = CloneLenTableBlock(sw, info, FALSE, &lenTableIdx, portPosition,
                                 groupSize, &mcastMask, mcastGroup, repliGroup);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }
    else
    {
        /*Update contents */
        
        /* lenTableReg will be written to the sw after MCAST_VLAN_TABLE is updated */
        FM_CLEAR(lenTableReg);
        FM_SET_FIELD( lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_McastIdx, finalVlanIndex);

        if (len > 1)
        {
            FM_SET_FIELD(lenTableReg,
                         FM10000_SCHED_MCAST_LEN_TABLE,
                         L3_Repcnt,
                         (len - 2));
        }
        else
        {
            /* Clean field */
            FM_SET_FIELD(lenTableReg,
                         FM10000_SCHED_MCAST_LEN_TABLE,
                         L3_Repcnt,
                         0);
        }

        err = switchPtr->WriteUINT32( sw,
                                      FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                      lenTableReg );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    FM_CLEAR(mcastDestReg);
    COPY_PORTMASK_TO_DESTMASK(mcastMask, mcastDestReg);

    FM_SET_FIELD64(mcastDestReg,
                   FM10000_SCHED_MCAST_DEST_TABLE,
                   LenTableIdx,
                   lenTableIdx);

    err = switchPtr->WriteUINT64( sw,
                                  FM10000_SCHED_MCAST_DEST_TABLE(mcastIndex, 0),
                                  mcastDestReg );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = GetListenersCount( info, repliGroup, physPort, &activeCount, NULL );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "activeCount=0x%x\n", activeCount);

    /* Update the active listener count */
    activeCount--;
    err = SetListenersCount( info, repliGroup, physPort, &activeCount, NULL );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* Update entryList */
    err = DeleteFromEntryListForListener(info,
                                         physPort,
                                         vlan,
                                         dglort,
                                         mcastGroup,
                                         repliGroup,
                                         delVlanIndex,
                                         lenTableIndex,
                                         updateTrees);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Now we can recover the expired len indices, since DEST_TABLE 
     * should have been already modified. */
    err = RecoverExpiredLenIndices(sw, info);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (updateDestMask)
    {
         /*******************************************************
         * Get the logical port and update the mask accordingly.
         * Basically, we'll set the bit in the GLORT_DEST_TABLE
         * only if there are active listeners.
         ******************************************************/

        err = fmMapPhysicalPortToLogical( switchPtr, physPort, &logicalPort );
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = GetListenersCount(info, repliGroup, physPort, &activeCount, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* get the destination mask associated with this mcast group */
        err = switchPtr->GetLogicalPortAttribute(sw,
                                                 mcastGroup,
                                                 FM_LPORT_DEST_MASK,
                                                 &logicalMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        fmSetPortInPortMask(sw, &logicalMask, logicalPort, (activeCount != 0));

        err = switchPtr->SetLogicalPortAttribute(sw,
                                                 mcastGroup,
                                                 FM_LPORT_DEST_MASK,
                                                 &logicalMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "Logical destination mask for port %d, glort 0x%x: 0x%04x%08x%08x\n",
                     mcastGroup,
                     GET_PORT_PTR(sw, mcastGroup)->glort,
                     logicalMask.maskWord[2],
                     logicalMask.maskWord[1],
                     logicalMask.maskWord[0]);
    
    }

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "For dest table index %" FM_FORMAT_64 "u, "
                 "multicast lenTable index %u, "
                 "multicast vlan index %u mcastMask 0x%04x%08x\n ",
                 (fm_uint64) mcastIndex,
                 lenTableIdx,
                 finalVlanIndex,
                 mcastMask.maskWord[1],
                 mcastMask.maskWord[0]);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end MTableDeleteListener */

/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000MTableInitialize
 * \ingroup intMulticast
 *
 * \desc            Initializes structures for managing the multicast table.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableInitialize(fm_int sw)
{
    fm10000_mtableInfo *info;
    fm_status           err;
    fm_switch *         switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,  "sw=%d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = fm10000AllocateMTableDataStructures(switchPtr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

    /* get the MTABLE info for this switch */
    info = GET_MTABLE_INFO(sw);

    /* Initialize the MCAST_DEST_TABLE usage bit array */
    /* make sure mcastIndex = 0 is reserved            */
    info->destTableCount = 1;

    err = fmClearBitArray(&info->destTableUsage);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmSetBitArrayBit(&info->destTableUsage, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Initialize the MCAST_LEN_TABLE usage bit array */
    /* make sure lenIndex = 0 is reserved             */
    info->lenTableCount  = 1;

    err = fmClearBitArray(&info->lenTableUsage);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmSetBitArrayBit(&info->lenTableUsage, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Initialize the MCAST_LEN_TABLE cloned entries bit array */
    err = fmClearBitArray(&info->clonedLenEntriesBitArray);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Initialize the MCAST_VLAN_TABLE usage bit array */
    /* make sure vlanIndex = 0 is reserved             */
    info->vlanTableCount  = 1;

    err = fmClearBitArray(&info->vlanTableUsage);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmSetBitArrayBit(&info->vlanTableUsage, 0, 1);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Initialize the MCAST_VLAN_TABLE cloned entries bit array */
    info->clonedEntriesCount = 0;
    err = fmClearBitArray(&info->clonedEntriesBitArray);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Initialize the garbage collection watermark */
    info->watermarkPercentage = 
        fmGetIntApiProperty(FM_AAK_API_FM10000_MTABLE_CLEANUP_WATERMARK,
                            FM_AAD_API_FM10000_MTABLE_CLEANUP_WATERMARK);
    info->clonedEntriesWatermark = FM10000_MAX_MCAST_VLAN_INDEX - 1;
    info->clonedEntriesWatermark *= info->watermarkPercentage;
    info->clonedEntriesWatermark /= 100;

    /* initialize the group map */
    fmTreeInit(&info->groups);

    /* initialize the destIndex list */
    fmTreeInit(&info->mtableDestIndex);

    /* initialize the listener list */
    fmTreeInit(&info->entryList);

    /* initialize the listener list */
    fmTreeInit(&info->entryListPerLenIndex);

    /* initialize the listeners count per (group, port) pair */
    fmTreeInit(&info->listenersCount);

    /* set MTable cleanup-related variables */
    info->epoch          = 0;

    /* mark it as initialized */
    info->isInitialized = TRUE;

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fm10000MTableInitialize */




/*****************************************************************************/
/** fm10000AllocateMTableDataStructures
 * \ingroup intMulticast
 *
 * \desc            Allocates structures for managing the multicast table.
 *
 * \param[in]       switchPtr points to the switch structure being initialized.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AllocateMTableDataStructures(fm_switch *switchPtr)
{
    fm_status          err = FM_OK;
    fm10000_mtableInfo *info;
    fm_char            semName[100];

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                  "switchPtr=%p<sw=%d>\n",
                  (void *) switchPtr,
                  (switchPtr ? switchPtr->switchNumber : -1) );

    /* validate the switch pointer */
    if ( switchPtr == NULL )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }

    /* get the MTABLE info for this switch */
    info = &( (fm10000_switch*) switchPtr->extension )->mtableInfo;

    /* Create the MCAST_DEST_TABLE usage bit array. The size is set
     * to FM10000_MAX_MCAST_DEST_INDEX to guaranty that the last entry
     * will not be used. This entry is use for memory repair */
    err = fmCreateBitArray(&info->destTableUsage,
                           FM10000_MAX_MCAST_DEST_INDEX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Create the MCAST_LEN_TABLE usage bit array. The size is set
     * to FM10000_MAX_MCAST_LEN_INDEX to guaranty that the last entry
     * will not be used. This entry is use for memory repair */
    err = fmCreateBitArray(&info->lenTableUsage,
                           FM10000_MAX_MCAST_LEN_INDEX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Create the MCAST_VLAN_TABLE usage bit array. The size is set
     * to FM10000_MAX_MCAST_VLAN_INDEX to guaranty that the last entry
     * will not be used. This entry is use for memory repair */
    err = fmCreateBitArray(&info->vlanTableUsage,
                           FM10000_MAX_MCAST_VLAN_INDEX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Create the cloned entries bit array. The size is set
     * to FM10000_MAX_MCAST_VLAN_INDEX to guaranty that the last entry
     * will not be used. This entry is use for memory repair */
    err = fmCreateBitArray(&info->clonedEntriesBitArray,
                           FM10000_MAX_MCAST_VLAN_INDEX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Create the cloned entries bit array. The size is set
     * to FM10000_MAX_MCAST_LEN_INDEX to guaranty that the last entry
     * will not be used. This entry is use for memory repair */
    err = fmCreateBitArray(&info->clonedLenEntriesBitArray,
                           FM10000_MAX_MCAST_LEN_INDEX);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Create the cleanup semaphore */
    FM_SNPRINTF_S( semName,
                   sizeof(semName),
                   "FM10KMtableCleanup%d",
                   switchPtr->switchNumber );

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000AllocateMTableDataStructures */




/*****************************************************************************/
/** fm10000FreeMTableDataStructures
 * \ingroup intMulticast
 *
 * \desc            Frees structures used to manage the multicast table.
 *
 * \param[in]       switchPtr points to the switch structure being removed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeMTableDataStructures(fm_switch *switchPtr)
{
    fm10000_mtableInfo *info;
    fm_status           err;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST,
                 "switchPtr=%p<sw=%d>\n",
                 (void *) switchPtr,
                 (switchPtr ? switchPtr->switchNumber : -1) );

    /* validate the switch pointer */
    if ( switchPtr == NULL )
    {
        FM_LOG_EXIT( FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }
    FM_TAKE_MTABLE_LOCK(switchPtr->switchNumber);

    /* get the MTABLE info for this switch */
    info = &( (fm10000_switch *) (switchPtr->extension) )->mtableInfo;

    /* Delete the MCAST_DEST_TABLE usage bit array */
    err = fmDeleteBitArray( &info->destTableUsage );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Delete the MCAST_LEN_TABLE usage bit array */
    err = fmDeleteBitArray( &info->lenTableUsage );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Delete the MCAST_VLAN_TABLE usage bit array */
    err = fmDeleteBitArray( &info->vlanTableUsage );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Delete the cloned entries bit array */
    err = fmDeleteBitArray( &info->clonedEntriesBitArray );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Delete the cloned entries bit array */
    err = fmDeleteBitArray( &info->clonedLenEntriesBitArray );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

ABORT:
    FM_DROP_MTABLE_LOCK(switchPtr->switchNumber);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err);

}   /* end fm10000FreeMTableDataStructures */




/*****************************************************************************/
/** fm10000FreeMTableResources
 * \ingroup intMulticast
 *
 * \desc            Frees resources utilized by the mtable component
 *
 * \param[in]       sw points to the switch structure from which resources
 *                  should be freed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeMTableResources(fm_int sw)
{
    fm10000_mtableInfo *info;
    fm_status           err;
    fm_switch *         switchPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_MULTICAST, "sw=%d\n", sw);

    /* get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);

    /* validate the switch pointer */
    if ( switchPtr == NULL )
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_ERR_INVALID_ARGUMENT);
    }
    FM_TAKE_MTABLE_LOCK(sw);

    /* get the MTABLE info for this switch */
    info = GET_MTABLE_INFO(sw);
    if ( !info->isInitialized )
    {
        err =  FM_ERR_UNINITIALIZED;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* Destroy the group map */
    fmTreeDestroy(&info->groups, NULL);

    /* Destroy the destIndex list */
    fmTreeDestroy(&info->mtableDestIndex, NULL);

    /* Destroy the entry list holding listener wrappers  */
    fmTreeDestroy(&info->entryList, FreeEntryUsageTree);

    /* Destroy the entry list with lenTableIndex as key */
    fmTreeDestroy(&info->entryListPerLenIndex, FreeEntryListPerLenIndex );

    /* Destroy the active listeners count per (group, port) pair */
    fmTreeDestroy(&info->listenersCount, FreeListenersCount );

    info->isInitialized = FALSE;

    err = fm10000FreeMTableDataStructures(switchPtr);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_SWITCH, err);

ABORT:
    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, err); 

}   /* end fm10000FreeMTableResources */




/*****************************************************************************/
/** fm10000MTableAddListener
 * \ingroup intMulticast
 *
 * \desc            Adds a listener to the given multicast group.  This creates
 *                  entries in the MCAST_DEST_TABLE and MCAST_VLAN_TABLE
 *                  if necessary.  This function will update the multicast
 *                  mask of the group as well.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       mcastGroup is the logical port assigned to the multicast
 *                  group.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       listener is the structure which has all MCAST_VLAN_TABLE fields
 *                  that has to be added.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableAddListener(fm_int               sw,
                                   fm_int               mcastGroup,
                                   fm_int               repliGroup,
                                   fm10000_mtableEntry  listener)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm_port *               portPtr;
    fm10000_mtableInfo *    info;
    fm_uintptr              mcastIndex;
    fm_int                  physPort;
    fm_int                  stpState;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d mcastGroup=%d repliGroup %d listener=<%d,%d>\n",
                 sw, mcastGroup, repliGroup, listener.port, listener.vlan);

    /* get the switch pointer */
    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, mcastGroup);
    info      = GET_MTABLE_INFO(sw);

    FM_TAKE_L2_LOCK(sw);
    FM_TAKE_MTABLE_LOCK(sw);

    /* reject it if it's too early */
    if ( info->isInitialized == FALSE )
    {
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /***************************************************
     * Retrieve all the relevant attributes.
     **************************************************/

    /* get the multicast index associated with this replication group */
    err = fmTreeFind(&info->mtableDestIndex, repliGroup, (void **) &mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

 
    /* is this group already active? */
    if ( ((fm10000_port *)(portPtr->extension))->groupEnabled == FALSE ||
          mcastIndex <= 0 )
    {
        /* no */
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* map the listener logical port to its parent physical port */
    err = fmMapLogicalPortToPhysical(switchPtr, listener.port, &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (listener.vlanUpdate == TRUE)
    {
        /* get the current STP state for this listener */
        err = fmGetVlanPortStateInternal(sw,
                                         listener.vlan,
                                         listener.port,
                                         &stpState);
    }
    else
    {
        stpState = FM_STP_STATE_FORWARDING;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = MTableAddListener(sw, 
                            mcastGroup, 
                            repliGroup, 
                            physPort, 
                            listener, 
                            stpState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

ABORT:
#ifdef FM_DEBUG_CHECK_CONSISTENCY
    ValidateMTableConsistency(sw);
#endif

    FM_DROP_MTABLE_LOCK(sw);
    FM_DROP_L2_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableAddListener */




/*****************************************************************************/
/** fm10000MTableDeleteListener
 * \ingroup intMulticast
 *
 * \desc            Deletes a listener from the given multicast group.  This
 *                  updates any register fields that need updating.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       mcastGroup is the logical assigned to the multicast group.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       listener is the structure which has all MCAST_VLAN_TABLE fields
 *                  that has to be deleted.
 *
 * \param[in]       updateDestMask should be set TRUE if the dest mask should
 *                  be updated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableDeleteListener(fm_int               sw,
                                      fm_int               mcastGroup,
                                      fm_int               repliGroup,
                                      fm10000_mtableEntry  listener,
                                      fm_bool              updateDestMask)
{
    fm_status          err = FM_OK;
    fm_switch         *switchPtr;
    fm10000_mtableInfo *info;
    fm_uintptr         mcastIndex;
    fm_int             physPort;
    fm_int             stpState;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d mcastGroup=%d listener=<%d,%d>\n",
                 sw,
                 mcastGroup,
                 listener.port,
                 listener.vlan);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);

    FM_TAKE_L2_LOCK(sw);
    FM_TAKE_MTABLE_LOCK(sw);

    if ( !info->isInitialized )
    {
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }
    /* get the mtable destination index associated with this replication group */
    err = fmTreeFind(&info->mtableDestIndex, repliGroup, (void **) &mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* index found? */
    if ( mcastIndex <= 0 )
    {
        /* wrong state for this call */
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* map the listener port to its physical port */
    err = fmMapLogicalPortToPhysical(switchPtr,
                                     listener.port,
                                     &physPort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (listener.vlanUpdate == TRUE)
    {
        /* get the current STP state for this listener */
        err = fmGetVlanPortStateInternal(sw,
                                         listener.vlan,
                                         listener.port,
                                         &stpState);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }
    else
    {
        stpState = FM_STP_STATE_FORWARDING;
    }

    err = MTableDeleteListener( sw,
                                mcastGroup,
                                repliGroup,
                                physPort,
                                listener.vlan,
                                listener.dglort,
                                stpState,
                                FALSE,
                                updateDestMask,
                                TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

ABORT:

#ifdef FM_DEBUG_CHECK_CONSISTENCY
    ValidateMTableConsistency(sw);
#endif

    FM_DROP_MTABLE_LOCK(sw);
    FM_DROP_L2_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableDeleteListener */




/*****************************************************************************/
/** fm10000MTableEnableGroup
 * \ingroup intMulticast
 *
 * \desc            Allows the previously defined functions to modify the
 *                  IP multicast index of the group.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       mcastGroup is the logical assigned to the multicast group.
 *
 * \param[in]       mcastGroupType indicates what layer this group is for.
 *
 * \param[in]       vlanID indicates what VLAN this group forwards in for
 *                  L2 mode.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[out]      mcastDestIndex points to caller allocated storage where the
 *                  allocated index into MCAST_DEST_TABLE will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableEnableGroup(fm_int             sw,
                                   fm_int             mcastGroup,
                                   fm_mtableGroupType mcastGroupType,
                                   fm_uint16          vlanID,
                                   fm_int *           repliGroup,
                                   fm_int *           mcastDestIndex)
{
    fm_status                err = FM_OK;
    fm_switch *              switchPtr;
    fm_port *                portPtr;
    fm10000_port *           portExt;
    fm10000_mtableInfo *     info;
    fm_int                   mcastIndex;
    fm_portmask              logicalMask;
    fm_uint32                mcastDestReg[FM10000_SCHED_MCAST_DEST_TABLE_WIDTH];
    fm10000_MTableGroupInfo *groupInfo;
    fm_intMulticastGroup    *mcastGroupInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d mcastLogPort=%d, repliGroup=%d, mcastDestIndex=%d\n",
                 sw, mcastGroup, *repliGroup, *mcastDestIndex);

    switchPtr = GET_SWITCH_PTR(sw );
    portPtr   = GET_PORT_PTR(sw, mcastGroup);
    info      = GET_MTABLE_INFO(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    /* was the MTable initialized? */
    if ( info->isInitialized == FALSE )
    {
        /* no then, we are being called in an invalid state */
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* is this a known Multicast Group? */
    if (!portPtr)
    {
        /* no, give up */
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* WARNING: mcastGroupType and vlanID are set, but not used later on */
    portExt = (fm10000_port *) portPtr->extension;
    portExt->groupType   = mcastGroupType;
    portExt->groupVlanID = vlanID;
    if ( portExt->groupEnabled == TRUE )
    {
        /* nothing to do if the group is already enabled, exit gracefully */
        err = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }
    portExt->groupEnabled = TRUE;

    /* Allocate a new private replication group (mtable destination entry)
     * if it has not been allocated yet */
    if (*repliGroup == -1)
    {
        /* Create a replication group for that mcast group */
        err =
            fmCreateReplicationGroupInt(sw,
                                        repliGroup,
                                        mcastGroup,
                                        &mcastIndex,
                                        TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }
    else
    {
        mcastIndex = *mcastDestIndex;

        /* Update tree */
        err = fmTreeFind(&info->groups, mcastIndex, (void **) &groupInfo);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmTreeRemove(&info->groups, mcastIndex, NULL);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                     "old repliGroup %d, mcast %d, new repliGroup %d mcast %d\n",
                     groupInfo->repliGroup,
                     groupInfo->mcastGroup,
                     *repliGroup,
                     mcastGroup);

        groupInfo->repliGroup = *repliGroup;
        groupInfo->mcastGroup = mcastGroup;
        err = fmTreeInsert(&info->groups, mcastIndex, (void *) groupInfo);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    /* get the multicast group info */
    err = fmTreeFind(&switchPtr->mcastPortTree,
                     mcastGroup,
                     (void **) &mcastGroupInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (mcastGroupInfo->attrLogicalPort == FM_LOGICAL_PORT_NONE)
    {
        /* Save the index as attribute for the multicast group logical port */
        /* set the multicast index attribute for this mcast group */
        /* The following operation is executed under the above if due to bug
         * 26494 in white model. Likely it does not need to be under if on HW.
         */
        err = switchPtr->SetLogicalPortAttribute(sw,
                                                 mcastGroup,
                                                 FM_LPORT_MULTICAST_INDEX,
                                                 &mcastIndex);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        
        /* set the initial destination mask attribute (all cleared) */
        FM_PORTMASK_DISABLE_ALL(&logicalMask);
        err = switchPtr->SetLogicalPortAttribute(sw,
                                                 mcastGroup,
                                                 FM_LPORT_DEST_MASK,
                                                 &logicalMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
    }

    *mcastDestIndex = mcastIndex;

ABORT:

#ifdef FM_DEBUG_CHECK_CONSISTENCY
    ValidateMTableConsistency(sw);
#endif

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableEnableGroup */




/*****************************************************************************/
/** fm10000MTableDisableGroup
 * \ingroup intMulticast
 *
 * \desc            Allows the previously defined functions to not modify the
 *                  IP multicast index of the group.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       mcastGroup is the logical assigned to the multicast group.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \param[in]       privateGroup should be set to TRUE if the replication group
 *                  is not in a shared mode.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableDisableGroup(fm_int  sw,
                                    fm_int  mcastGroup,
                                    fm_int  repliGroup,
                                    fm_bool privateGroup)
{
    fm_status             err = FM_OK;
    fm_switch            *switchPtr;
    fm_port              *portPtr;
    fm10000_port         *portExt;
    fm_uintptr            mcastIndex;
    fm_portmask           logicalMask;
    fm_int                logPortCount;
    fm10000_mtableInfo   *info;
    fm_uint32             mcastDestReg[FM10000_SCHED_MCAST_DEST_TABLE_WIDTH];
    fm_intMulticastGroup *mcastGroupInfo;


    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw=%d mcastLogPort=%d, repliGroup=%d, privateGroup=%d\n",
                 sw, mcastGroup, repliGroup, privateGroup);

    switchPtr = GET_SWITCH_PTR(sw);
    portPtr   = GET_PORT_PTR(sw, mcastGroup);
    info      = GET_MTABLE_INFO(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    /* was the MTable initialized? */
    if (info->isInitialized == FALSE)
    {
        /* no then, we are being called in an invalid state */
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* is this a known Multicast Group? */
    if (!portPtr)
    {
        /* no, give up */
        err = FM_ERR_INVALID_PORT;
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    /* get the mtable destination index associated with this replication group */
    err = fmTreeFind(&info->mtableDestIndex, repliGroup, (void **) &mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* nothing to do if the group is already disabled */
    portExt = (fm10000_port *) portPtr->extension;
    if ( portExt->groupEnabled == FALSE || mcastIndex <= 0 )
    {
        /* it is, exit gracefully */
        err = FM_OK;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    portExt->groupType   = FM_MULTICAST_GROUP_TYPE_L3;
    portExt->groupVlanID = 0;
    portExt->groupEnabled = FALSE;

    /* get the multicast group info */
    err = fmTreeFind(&switchPtr->mcastPortTree,
                     mcastGroup,
                     (void **) &mcastGroupInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    if (mcastGroupInfo->attrLogicalPort == FM_LOGICAL_PORT_NONE)
    {
        /* prevent it if this group has still listeners */
        /* get the destination mask associated with this mcast group */
        err = switchPtr->GetLogicalPortAttribute(sw,
                                                 mcastGroup,
                                                 FM_LPORT_DEST_MASK,
                                                 &logicalMask);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* get the logical port count */
        err = fmGetPortMaskCount(&logicalMask, &logPortCount);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* logPortCount should be 0. Just to spot inconsistencies */
        if ( logPortCount != 0 )
        {
            err = FM_ERR_MCAST_INVALID_STATE;
            FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
        }
    }

    /* release the replication group if not in a shared mode */
    if (privateGroup)
    {
        err = fmDeleteReplicationGroupInt(sw, repliGroup, TRUE);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    /* Clear the multicast index attribute for this mcast group */
    mcastIndex = 0;
    err = switchPtr->SetLogicalPortAttribute(sw,
                                             mcastGroup,
                                             FM_LPORT_MULTICAST_INDEX,
                                             &mcastIndex);

ABORT:

#ifdef FM_DEBUG_CHECK_CONSISTENCY
    fm10000DbgDumpMulticastVlanTable(sw);
    ValidateMTableConsistency(sw);
#endif

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableDisableGroup */




/*****************************************************************************/
/** fm10000MTableUpdateListenerState
 * \ingroup intMulticast
 *
 * \desc            Iterates through the entry list for the port, vlan pair
 *                  and sets those entries to skipped or not based on the
 *                  state.  This function should be called whenever the STP
 *                  state changes for a given port, vlan pair.
 *
 * \param[in]       sw is the switch number to initialize.
 *
 * \param[in]       instance is the STP instance for whose spanning tree instance the
 *                  STP state has changed.
 *
 * \param[in]       port is the logical port on which the STP state has changed.
 *
 * \param[in]       state is the new STP state for that port.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableUpdateListenerState(fm_int sw,
                                           fm_int instance,
                                           fm_int port,
                                           fm_int state)
{
    fm_status                 err = FM_OK;
    fm_switch                *switchPtr;
    fm10000_mtableInfo        *info;
    fm_uint64                 key;
    fm_uint64                 keyDglort;
    fm_dlist                 *entryList;
    fm_dlist_node            *entryListNode;
    fm_dlist_node            *nextNode;
    fm10000_entryListWrapper *entry;
    fm_int                    vlanIndex;
    fm_int                    lenTableIndex;
    fm_int                    mcastGroup;
    fm_int                    repliGroup;
    fm_int                    physPort;
    fm_int                    lastVlan = 0;
    fm_int                    vlan = 0;
    fm_tree *                 stpInfo;
    fm_stpInstanceInfo *      stpInst;
    fm_intMulticastGroup *    mcastGroupInfo;
    fm10000_mtableEntry       listener;
    fm_tree                  *dglortTree;
    fm_treeIterator           treeIter;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_MULTICAST,
                         "sw=%d vlan=%d port=%d state=%d\n",
                         sw,
                         vlan,
                         port,
                         state );

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);
    stpInfo   = GET_STP_INFO(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    /* Retrieve the default stp instance. */
    err = fmTreeFind(stpInfo, instance, (void **) &stpInst);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* note that the key is by listener's physical port */
    fmMapLogicalPortToPhysical(switchPtr, port, &physPort);

    /* search through the VLANs that are a member of this instance */
    while (lastVlan < FM_MAX_VLAN)
    {
        err = fmFindBitInBitArray(&stpInst->vlans, lastVlan, TRUE, &vlan);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        /* No more VLANs? */
        if (vlan == -1)
        {
            break;
        }

        /***************************************************
         * Compute key and find list of entries for this
         * (port, stID) pair.
         **************************************************/

        key = ((physPort << 12) | vlan);

        err = fmTreeFind(&info->entryList,
                         key,
                         (void **) &dglortTree);

        if (err == FM_ERR_NOT_FOUND)
        {
            /* Not a true error. */
            err = FM_OK;

            lastVlan = vlan + 1;
            
            continue;
        }
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        
        fmTreeIterInit(&treeIter, dglortTree);

        while ((err = fmTreeIterNext(&treeIter,
                                     &keyDglort,
                                     (void **) &entryList)) != FM_ERR_NO_MORE)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            if (entryList->head)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "Found entries for port=%d vlan=%d dglort 0x%x\n",
                             physPort,
                             vlan,
                             (unsigned int)keyDglort);
            }

            /***************************************************
             * Iterate through the entry list. There are two cases:
             * 1) Those entries for which vlanIndex is zero are
             * not in the MCAST_VLAN_TABLE yet. If the STP state
             * is now forwarding they have to be added
             * 2) Those entries for which vlanIndex is non-zero
             * are already in the MCAST_VLAN_TABLE. If the STP
             * state is no longer forwarding, then they have to
             * be removed
             **************************************************/
            for (entryListNode = entryList->head ; entryListNode ; )
            {
                entry = (fm10000_entryListWrapper *) entryListNode->data;
                vlanIndex      = entry->vlanEntryIndex;
                lenTableIndex  = entry->lenTableIndex;
                repliGroup     = entry->repliGroup;
                mcastGroup     = entry->mcastGroup;
                
                /* get the multicast group info */
                err = fmTreeFind(&switchPtr->mcastPortTree,
                                 mcastGroup,
                                 (void **) &mcastGroupInfo);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                if ( (vlanIndex == 0) && (lenTableIndex == 0) &&
                     ( (state == FM_STP_STATE_FORWARDING) ||
                       (mcastGroupInfo->bypassEgressSTPCheck) ) )
                {
                    /* save next node to continue from */
                    nextNode = entryListNode->next;

                    err = fmMapPhysicalPortToLogical(switchPtr, physPort, &listener.port);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                    
                    FM_CLEAR(listener);
                    listener.port = physPort;
                    if (vlan != 0)
                    {
                        listener.vlanUpdate = TRUE;
                    }
                    listener.vlan = vlan;
                    listener.dglort = keyDglort;
                    if (keyDglort != 0)
                    {
                        listener.dglortUpdate = TRUE;
                    }
                    
                    /* we take it out of the listener list with the old index */
                    err = RemoveListenerEntry(info,
                                              repliGroup,
                                              physPort,
                                              lenTableIndex,
                                              entryList,
                                              entryListNode,
                                              NULL,
                                              NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                    
                    err = MTableAddListener(sw, 
                                            mcastGroup, 
                                            repliGroup, 
                                            physPort,
                                            listener,
                                            state); 
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

                    /* since we just modified the list, we continue from the next */
                    entryListNode = nextNode;
                    
                    continue;
                }
                else if ( (vlanIndex != 0) && (lenTableIndex != 0) &&
                          (state != FM_STP_STATE_FORWARDING) &&
                          (mcastGroupInfo->bypassEgressSTPCheck == 0) )
                {
                    /* save next node to continue from */
                    nextNode = entryListNode->next;
                    
                    err = MTableDeleteListener(sw,
                                               mcastGroup,
                                               repliGroup,
                                               physPort,
                                               vlan,
                                               keyDglort,
                                               state,
                                               TRUE,
                                               TRUE,
                                               FALSE);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                    
                    /* we add the listener back to the list with index = 0*/
                    err = InsertListenerEntry( info,
                                               mcastGroup,
                                               repliGroup,
                                               physPort,
                                               0,
                                               0,
                                               entryList,
                                               NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
                    
                /* since we just modified the list, we need to reset to the head */
                    entryListNode = nextNode;
                    continue;
                }
                else
                {
                    entryListNode = entryListNode->next;
                }

            }   /* end for (entryListNode = entryList->head ; entryListNode ; ) */

        }   /* end  while ((err = fmTreeIterNext(&treeIter, ... */

        lastVlan = vlan+1;
    }

ABORT:

#ifdef FM_DEBUG_CHECK_CONSISTENCY
    ValidateMTableConsistency(sw);
#endif

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableUpdateListenerState */




/*****************************************************************************/
/** fm10000GetAvailableMulticastListenerCount
 * \ingroup intMulticast
 *
 * \desc            Determines how many unused MCAST_LEN_TABLE and 
 *                  MCAST_VLAN_TABLE entries remain.
 *
 * \note            The number of unused entries is not necessarily equal to
 *                  the number of new listeners that can be added. In other
 *                  words, it's no guarantee that we can accomodate a number
 *                  of new listeners based on the exact counts returned by
 *                  this function
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      count points to caller-allocated storage where this
 *                  function should place the number of unused multicast
 *                  table entries, i.e. the minimum of unused multicast len
 *                  table entries and unused multicast vlan table entries.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetAvailableMulticastListenerCount(fm_int  sw,
                                                    fm_int *count)
{
    fm10000_mtableInfo *info;
    fm_int              lenTableCount;
    fm_int              vlanTableCount;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,  "sw=%d \n", sw);

    FM_TAKE_MTABLE_LOCK(sw);

    info = GET_MTABLE_INFO(sw);

    lenTableCount  = FM10000_MAX_MCAST_LEN_INDEX - info->lenTableCount;
    vlanTableCount = FM10000_MAX_MCAST_VLAN_INDEX - info->vlanTableCount;

    if (vlanTableCount > lenTableCount)
    {
        *count = lenTableCount;
    }
    else
    {
        *count = vlanTableCount;
    }

    FM_DROP_MTABLE_LOCK(sw);
    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_OK);

}   /* end fm10000GetAvailableMulticastListenerCount */




/*****************************************************************************/
/** fm10000MTablePeriodicMaintenance
 * \ingroup intMulticast
 *
 * \desc            Handler for the MTable Maintenance Task.
 *
 * \param[in]       sw the switch on which to operate
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTablePeriodicMaintenance(fm_int sw)
{
    fm_status           err;
    fm10000_mtableInfo *info;

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_MULTICAST, "sw = %d\n", sw);
                 
    info      = GET_MTABLE_INFO(sw);

    /* wait for the MTABLE to be initialized */
    if ( info->isInitialized == FALSE )
    {
        err = FM_OK;
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_MULTICAST,  err);
    }

    /* Proceed to cleanup when clonedEntries is greater than watermark% of 
     * remaining available entries or forceClean is enabled. This check is 
     * done here first to avoid taking lock if it is below watermark.  */
    if (info->clonedEntriesCount <= info->clonedEntriesWatermark)
    {
        err = FM_OK;
        FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_MULTICAST,  err);
    }

    FM_TAKE_MTABLE_LOCK(sw);

    err = MTableCleanup(sw, FALSE);

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTablePeriodicMaintenance */




/*****************************************************************************/
/** fm10000DbgDumpMulticastVlanTable
 * \ingroup intMulticast
 *
 * \desc            Dumps the contents of the MCAST_VLAN_TABLE
 *
 * \param[in]       sw is the switch number to operate on.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpMulticastVlanTable(fm_int sw)
{
    fm_int                   i;
    fm_uint64                destEntry;
    fm_uint64                vlanTableReg;
    fm_uint32                lenTableReg;
    fm_status                err = FM_OK;
    fm_switch *              switchPtr;
    fm10000_mtableInfo *     info;
    fm_bool                  expiring;
    fm_bool                  used;
    fm_portmask              mcastMask;
    fm_portmask              destMask;
    fm_treeIterator          treeIter;
    fm_treeIterator          treeIterLenIndex;
    fm_treeIterator          treeIterDglort;
    fm_uint64                key;
    fm_uint64                keyDglort;
    fm_int                   mcastGroup=0;
    fm_dlist *               entryList;
    fm_dlist_node *          entryListNode;
    fm_dlist *               entryListPerLenIndex;
    fm_dlist_node *          entryListPerLenIndexNode;
    fm10000_entryListWrapper *entry;
    fm_bool                  indexUsedByGroup;
    fm_bool                  invalidPort = FALSE;
    fm_bool                  indexShared;
    fm10000_MTableGroupInfo *groupInfo;
    fm_uint64                mcastMaskField;
    fm_tree                  *treeDglort;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw=%d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_MTABLE_INFO(sw);

    FM_LOG_PRINT("MCAST_DEST_TABLE Contents\n");
    FM_LOG_PRINT("-------------------------\n");

    for ( i = 0 ; i < FM10000_SCHED_MCAST_DEST_TABLE_ENTRIES ; i++ )
    {
        err = fmGetBitArrayBit(&info->destTableUsage,
                               i,
                               &used);

        if (i < FM10000_MAX_MCAST_DEST_INDEX)
        {
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
        }
        else
        {
            used = FALSE;
            FM_LOG_PRINT("#%04d : Entry used for memory repair\n", i);
        }

        if (used)
        {
            err = switchPtr->ReadUINT64(sw,
                                        FM10000_SCHED_MCAST_DEST_TABLE(i, 0),
                                        &destEntry);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            mcastMaskField = FM_GET_FIELD64(destEntry,
                                            FM10000_SCHED_MCAST_DEST_TABLE,
                                            PortMask),
            COPY_DESTMASK_TO_PORTMASK(mcastMaskField, mcastMask);

            err = fmTreeFind(&info->groups, i, (void **) &groupInfo);
            if (err == FM_OK)
            {
                mcastGroup = groupInfo->mcastGroup;
                if (mcastGroup > -1)
                {
                    indexShared = FALSE;
                    indexUsedByGroup = TRUE;

                    if (!GET_PORT_PTR(sw, mcastGroup))
                    {
                        invalidPort = TRUE;
                    }
                }
                else
                {
                    /* No mcast group to that entry yet */
                    indexUsedByGroup = FALSE;
                    indexShared = TRUE;
                }
            }
            else if (err == FM_ERR_NOT_FOUND)
            {
                indexShared = FALSE;
                indexUsedByGroup = FALSE;
            }
            else if (err != FM_OK)
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            }

            if (invalidPort)
            {
                FM_LOG_PRINT("#%04d : Invalid group for Index %lld, RawContent: %llx, Multicast Mask 0x%04x%08x\n",
                             i,
                             FM_GET_FIELD64(destEntry,
                                            FM10000_SCHED_MCAST_DEST_TABLE,
                                            LenTableIdx),
                             destEntry,
                             mcastMask.maskWord[1],
                             mcastMask.maskWord[0]);
            }
            else if (indexUsedByGroup)
            {
                err = switchPtr->GetLogicalPortAttribute(sw,
                                                         mcastGroup,
                                                         FM_LPORT_DEST_MASK,
                                                         &destMask);
                FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

                FM_LOG_PRINT("#%04d : Group %d, Index %lld, RawContent: %llx, "
                             "Multicast Mask 0x%04x%08x, Dest Mask 0x%08x%08x\n",
                             i,
                             mcastGroup,
                             FM_GET_FIELD64(destEntry,
                                            FM10000_SCHED_MCAST_DEST_TABLE,
                                            LenTableIdx),
                             destEntry,
                             mcastMask.maskWord[1],
                             mcastMask.maskWord[0],
                             destMask.maskWord[1],
                             destMask.maskWord[0]);
            }
            else if (indexShared)
            {
                FM_LOG_PRINT("#%04d : Shared, Index %lld, RawContent: %llx, Multicast Mask 0x%04x%08x\n",
                             i,
                             FM_GET_FIELD64(destEntry,
                                            FM10000_SCHED_MCAST_DEST_TABLE,
                                            LenTableIdx),
                             destEntry,
                             mcastMask.maskWord[1],
                             mcastMask.maskWord[0]);
            }
            else
            {
                FM_LOG_PRINT("#%04d : Unused, Index %lld, Multicast Mask 0x%04x%08x\n",
                             i,
                             FM_GET_FIELD64(destEntry,
                                            FM10000_SCHED_MCAST_DEST_TABLE,
                                            LenTableIdx),
                             mcastMask.maskWord[1],
                             mcastMask.maskWord[0]);
            }
        }
    }


    FM_LOG_PRINT("\nMCAST_LEN_TABLE Contents\n");
    FM_LOG_PRINT("-------------------------\n");
    for ( i = 0; i < FM10000_MAX_MCAST_LEN_INDEX ; i++)
    {
        err = fmGetBitArrayBit(&info->clonedLenEntriesBitArray,
                               i,
                               &expiring);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmGetBitArrayBit(&info->lenTableUsage,
                               i,
                               &used);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = switchPtr->ReadUINT32(sw,
                                    FM10000_SCHED_MCAST_LEN_TABLE(i),
                                    &lenTableReg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        if (used)
        {
            printf("#%05d : %c%c : %08x => Entry : L3_Repcnt=%d, L3_McastIdx=%d\n",
                    i,
                    (expiring ? 'E' : '.'),
                    (used ? 'U' : '.'),
                    lenTableReg,
                    FM_GET_FIELD(lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_Repcnt),
                    FM_GET_FIELD(lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_McastIdx));
        }
        else if (lenTableReg)
        {
            printf("#%05d : %c%c : %08x => Available\n",
                    i,
                    (expiring ? 'E' : '.'),
                    (used ? 'U' : '.'),
                    lenTableReg);

        }
    }

    FM_LOG_PRINT("\nMCAST_VLAN_TABLE Contents\n");
    FM_LOG_PRINT("-------------------------\n");

    for ( i = 0 ; i < FM10000_MAX_MCAST_VLAN_INDEX ; i++ )
    {
        err = fmGetBitArrayBit(&info->clonedEntriesBitArray,
                               i,
                               &expiring);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = fmGetBitArrayBit(&info->vlanTableUsage,
                               i,
                               &used);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        err = switchPtr->ReadUINT64(sw,
                                    FM10000_MOD_MCAST_VLAN_TABLE(i, 0),
                                    &vlanTableReg);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

        if (used)
        {
                printf("#%05d : %c%c : %08llx => Entry : VID=%lld, DGLORT=%lld, ReplaceVID=%lld, ReplaceDglort=%lld\n",
                       i,
                       (expiring ? 'E' : '.'),
                       (used ? 'U' : '.'),
                       vlanTableReg,
                       FM_GET_FIELD64(vlanTableReg, FM10000_MOD_MCAST_VLAN_TABLE, VID),
                       FM_GET_FIELD64(vlanTableReg, FM10000_MOD_MCAST_VLAN_TABLE, DGLORT),
                       FM_GET_BIT64(vlanTableReg, FM10000_MOD_MCAST_VLAN_TABLE, ReplaceVID),
                       FM_GET_BIT64(vlanTableReg, FM10000_MOD_MCAST_VLAN_TABLE, ReplaceDGLORT));
        }
        else if (vlanTableReg)
        {
            printf("#%05d : %c%c : %08llx => Available\n",
                   i,
                   (expiring ? 'E' : '.'),
                   (used ? 'U' : '.'),
                   vlanTableReg);
        }
    }

    FM_LOG_PRINT("\nUsage Lists\n");
    FM_LOG_PRINT("-------------------------\n");

    fmTreeIterInit(&treeIter, &info->entryList);

    while (fmTreeIterNext(&treeIter,
                          &key,
                          (void **) &treeDglort) != FM_ERR_NO_MORE)
    {
        fmTreeIterInit(&treeIterDglort, treeDglort);

        while (fmTreeIterNext(&treeIterDglort,
                              &keyDglort,
                              (void **) &entryList) != FM_ERR_NO_MORE)

        {
            if (entryList->head)
            {
                FM_LOG_PRINT("Port %lld, Vlan %lld:\n    ", (key >> 12), (key & 0xfff));
                
                for (entryListNode = entryList->head ; entryListNode ; )
                {
                    entry = (fm10000_entryListWrapper *) entryListNode->data;
                    
                    FM_LOG_PRINT("%d,%d - (%d,%d) ",
                                 entry->lenTableIndex,
                                 entry->vlanEntryIndex,
                                 entry->repliGroup,
                                 entry->mcastGroup);
                    
                    entryListNode = entryListNode->next;
                }
                
                FM_LOG_PRINT("\n");
            }
            else
            {
                FM_LOG_PRINT("Port %lld, Vlan %lld found without any entryList\n", 
                             key >> 12,
                             key & 0xfff);
            }
        }
    }

    FM_LOG_PRINT("\nUsage Lists per LenIndex\n");
    FM_LOG_PRINT("-------------------------\n");

    fmTreeIterInit(&treeIterLenIndex, &info->entryListPerLenIndex);

    while (fmTreeIterNext(&treeIterLenIndex,
                          &key,
                          (void **) &entryListPerLenIndex) != FM_ERR_NO_MORE)
    {
        if (entryListPerLenIndex->head)
        {
            FM_LOG_PRINT("Key LenTableIndex: %lld\n", key );

            for (entryListPerLenIndexNode = entryListPerLenIndex->head ; entryListPerLenIndexNode ; )
            {
                entry = (fm10000_entryListWrapper *) entryListPerLenIndexNode->data;

                FM_LOG_PRINT("    %d,%d - (%d,%d)\n",
                             entry->lenTableIndex,
                             entry->vlanEntryIndex,
                             entry->repliGroup,
                             entry->mcastGroup);

                entryListPerLenIndexNode = entryListPerLenIndexNode->next;
            }

            FM_LOG_PRINT("\n");
        }
        else
        {
            FM_LOG_PRINT("Key LenTableIndex: %lld found without any entryList\n", key );
        }
    }

    FM_LOG_PRINT("\nUsage counts\n");
    FM_LOG_PRINT("-------------------------\n");
    
    FM_LOG_PRINT("vlan table usage count: %d\n", info->vlanTableCount);
    FM_LOG_PRINT("len table usage count: %d\n", info->lenTableCount);
    FM_LOG_PRINT("dest table usage count: %d\n", info->destTableCount);
    FM_LOG_PRINT("number of cloned entries in vlan table: %d\n", info->clonedEntriesCount);

ABORT:

    FM_DROP_MTABLE_LOCK(sw);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST, FM_OK);

}   /* end fm10000DbgDumpMulticastVlanTable */




/*****************************************************************************/
/** fm10000MTableReserveEntry
 * \ingroup intMulticast
 *
 * \desc            Reserve a Replication group's multicast Index.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group is the replication group number.
 *
 * \param[in]       mcastLogicalPort is the logical port of the multicast group.
 *
 * \param[out]      mcastDestIndex points to caller-allocated storage where this
 *                  function should place the mtable destination table index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableReserveEntry(fm_int  sw,
                                    fm_int  group,
                                    fm_int  mcastLogicalPort,
                                    fm_int *mcastDestIndex)
{
    fm_status               err = FM_OK;
    fm_switch *             switchPtr;
    fm10000_mtableInfo *    info;
    fm_uintptr              mcastIndex;
    fm_uint64               mcastDestReg;
    fm10000_MTableGroupInfo *groupInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST,
                 "sw = %d  mcastDestIndex = %p\n",
                 sw,
                 (void *) mcastDestIndex);

    switchPtr = GET_SWITCH_PTR(sw );
    info      = GET_MTABLE_INFO(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    /* was the MTable initialized? */
    if ( info->isInitialized == FALSE )
    {
        /* no then, we are being called in an invalid state */
        err = FM_ERR_MCAST_INVALID_STATE;
        FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
    }

    /* reserve a multicast index */
    err = FindUnusedMulticastIndex(info, &mcastIndex );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    groupInfo = (fm10000_MTableGroupInfo *) fmAlloc(sizeof(fm10000_MTableGroupInfo));

    if (groupInfo == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  FM_ERR_NO_MEM);
    }

    groupInfo->repliGroup = group;
    groupInfo->mcastGroup = mcastLogicalPort;

    err = fmTreeInsert(&info->groups, mcastIndex, (void *) groupInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmTreeInsert(&info->mtableDestIndex, group, (void *) mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "group %d mcastIndex=0x%" FM_FORMAT_64 "x\n",
                 group,
                 (fm_uint64) mcastIndex);

    err = MarkMulticastIndexUsed(info, mcastIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* Initialize the MCAST_DEST_TABLE entry with mask = 0 and index = 0 */
    FM_CLEAR(mcastDestReg);
    err = switchPtr->WriteUINT64( sw,
                                  FM10000_SCHED_MCAST_DEST_TABLE(mcastIndex,0),
                                  mcastDestReg );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    *mcastDestIndex = mcastIndex;

ABORT:

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableReserveEntry */




/*****************************************************************************/
/** fm10000MTableFreeMcastGroupVlanEntries
 * \ingroup intMulticast
 *
 * \desc            Free VLAN entries associated to a mutlicast group.
 *                  Note: This function directly frees the hw indices without
 *                         deleting from entryLists(similar to FM6000). Should
 *                         be revisited and modified when using it. Might be a
 *                         problem in FM6000 depending upon its usage.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       mcastGroup is the mutlicast group number (handle).
 *
 * \param[in]       mtableDestIndex is the mtable destinatin table index.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000MTableFreeMcastGroupVlanEntries(fm_int sw,
                                                 fm_int mcastGroup,
                                                 fm_int mtableDestIndex)
{
    fm_status          err=FM_OK;
    fm_switch *        switchPtr;
    fm10000_mtableInfo *info;
    fm_uint64          mcastDestReg;
    fm_uint32          lenTableReg;
    fm_uint64          mcastMaskField;
    fm_portmask        mcastMask;
    fm_int             groupSize;
    fm_int             lenTableIdx;
    fm_int             vlanTableIdx;
    fm_int             vlanIndex;
    fm_int             lenTableIndex;
    fm_int             i, j;
    fm_int             len;
    fm_bool            expired;
    fm_bool            vlanTableIndexUsed;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw=%d mcastGroup=%d\n",sw, mcastGroup);

    switchPtr = GET_SWITCH_PTR(sw);
    info = GET_MTABLE_INFO(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    /* Get the MCAST_DEST_TABLE register from the switch  */
    err = switchPtr->ReadUINT64(sw,
                                FM10000_SCHED_MCAST_DEST_TABLE(mtableDestIndex, 0),
                                &mcastDestReg );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    /* Get the multicast mask from the register */
    mcastMaskField = FM_GET_FIELD64(mcastDestReg,
                                    FM10000_SCHED_MCAST_DEST_TABLE,
                                    PortMask),
    COPY_DESTMASK_TO_PORTMASK(mcastMaskField, mcastMask);

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "mcast mask = 0x%04x%08x\n",
                 mcastMask.maskWord[1],
                 mcastMask.maskWord[0]);

    /* Compute the number of ports in the mask */
    err = fmGetPortMaskCount( &mcastMask, &groupSize );
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,  "group size = %d\n", groupSize);

    lenTableIdx = FM_GET_FIELD64( mcastDestReg,
                                  FM10000_SCHED_MCAST_DEST_TABLE,
                                  LenTableIdx );

    FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                 "lenTableIdx for this mcastGroup is %d\n", lenTableIdx );

    for (i = 0; i < groupSize; i++)
    {
        lenTableIndex = lenTableIdx + i;

        /* now, get the current MCAST_LEN_TABLE register from the switch  */
        err = switchPtr->ReadUINT32(sw,
                                    FM10000_SCHED_MCAST_LEN_TABLE(lenTableIndex),
                                    &lenTableReg );
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

        vlanTableIdx = FM_GET_FIELD( lenTableReg,
                                     FM10000_SCHED_MCAST_LEN_TABLE,
                                     L3_McastIdx );
        len       = FM_GET_FIELD( lenTableReg, FM10000_SCHED_MCAST_LEN_TABLE, L3_Repcnt );

        FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST,
                             "vlanTable block start index: %d len: %d\n",
                             vlanTableIdx,
                             len);

        for (j = 0; j < len; j++)
        {
            vlanIndex = vlanTableIdx + j;
            FM_LOG_DEBUG_VERBOSE(FM_LOG_CAT_MULTICAST,
                                 "VlanIndex:%d\n",
                                 vlanIndex);

            /* Check whether it is marked as used */
            err = fmGetBitArrayBit(&info->vlanTableUsage,
                                   vlanIndex,
                                   &vlanTableIndexUsed);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

            if ( !vlanTableIndexUsed )
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "VlanTableIndex: %d must be marked as used.",
                             vlanIndex);
                err = FM_FAIL;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }

            err = fmGetBitArrayBit(&info->clonedEntriesBitArray,
                                   vlanIndex,
                                   &expired);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);
            if (expired)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_MULTICAST,
                             "VlanTableIndex: %d must not be marked as expired.\n",
                             vlanIndex);
                err = FM_FAIL;
                FM_LOG_ABORT(FM_LOG_CAT_MULTICAST, err);
            }

            /* Mark as free the entry */
            err = MarkVlanIndexExpired(info, vlanIndex);
            FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
        }

        err = MarkLenTableIndexExpired(info, lenTableIndex);
        FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    }

    err = RecoverExpiredLenIndices(sw, info);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );

ABORT:

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableFreeMcastGroupVlanEntries */




/*****************************************************************************/
/** fm10000MTableFreeDestTableEntry
 * \ingroup intMulticast
 *
 * \desc            Free a mtable destination entry.
 *                  Note: Prior to using this function the individual listeners
 *                        should be deleted.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       repliGroup is the replication group number.
 *
 * \return          FM_OK if successful
 *
 *****************************************************************************/
fm_status fm10000MTableFreeDestTableEntry(fm_int sw, fm_int repliGroup)
{
    fm_status                err=FM_OK;
    fm_switch *              switchPtr;
    fm10000_mtableInfo  *    info;
    fm10000_MTableGroupInfo *groupInfo;
    fm_uintptr               mtableDestIndex;
    fm_uint64                mcastDestReg;

    FM_LOG_ENTRY(FM_LOG_CAT_MULTICAST, "sw=%d repliGroup=%d\n", sw, repliGroup);

    switchPtr = GET_SWITCH_PTR(sw);
    info = GET_MTABLE_INFO(sw);

    FM_TAKE_MTABLE_LOCK(sw);

    /* get the mtable destination index associated with this replication group */
    err = fmTreeFind(&info->mtableDestIndex, repliGroup, (void **) &mtableDestIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    /* read the MCAST_DEST_TABLE register */
    err = switchPtr->ReadUINT64( sw,
                                 FM10000_SCHED_MCAST_DEST_TABLE(mtableDestIndex, 0),
                                 &mcastDestReg );
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = MarkMulticastIndexAvailable(info, mtableDestIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmTreeFind(&info->groups, mtableDestIndex, (void **) &groupInfo);
    FM_LOG_ABORT_ON_ERR( FM_LOG_CAT_MULTICAST, err );
    fmFree(groupInfo);

    err = fmTreeRemove(&info->groups, mtableDestIndex, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

    err = fmTreeRemove(&info->mtableDestIndex, repliGroup, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_MULTICAST, err);

ABORT:

    FM_DROP_MTABLE_LOCK(sw);

    FM_LOG_EXIT(FM_LOG_CAT_MULTICAST,  err);

}   /* end fm10000MTableFreeDestTableEntry */


