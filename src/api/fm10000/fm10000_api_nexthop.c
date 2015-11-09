/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm10000_api_nexthop.c
 * Creation Date: August 29, 2013
 * Description: Routing services
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

#include <fm_sdk_fm10000_int.h>

/*****************************************************************************
 * Constants & Types
 *****************************************************************************/

#define FM10000_ARP_PACKING_LOWEST_SCAN_INITIAL_INDEX   16
#define FM10000_ARP_PACKING_LOWEST_SCAN_INDEX           10
#define FM10000_ARP_PACKING_EMPTY_CNT_SATURATION_LEVEL  16
#define FM10000_ARP_PACKING_QUICK_DECISION_SCORE        22

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static const fm_text ArpBlockSizeBinString[FM10000_ARP_BLOCK_SIZE_MAX] =
{
    "     1  ",
    "     2  ",
    " 3 - 4  ",
    " 5 - 8  ",
    " 9 - 16 ",
    "17 - 64 ",
    "65 - 256",
    "   + 257"
};




/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local functions with fmAlloc inside. Notice that "static" prefix
 * can be temporarily removed for the process of detection memory leaks.
 * In this case "static" should be removed for both - declaration
 * and definition. It allows to see the proper function name in memory dump.
 *****************************************************************************/
static fm_status AllocArpBlkCtrlTab(fm_int sw);
static fm_status AllocNextHopExtensions(fm_int           sw,
                                        fm_intEcmpGroup *pParentEcmpGroup,
                                        fm_int           arpBlkHndl,
                                        fm_int           undoTabSize,
                                        fm_int *         pUndoTab);
static fm_status AllocateEcmpGroup(fm_int              sw,
                                   fm10000_EcmpGroup **ppEcmpGroupExt);
/* End of local functions with fmAlloc inside. */

static void UpdateFirstFreeArpEntry(fm_int  sw,
                                    fm_int  initialOffset,
                                    fm_bool blockAllocation);
static void UpdateLastUsedArpEntry(fm_int  sw,
                                   fm_int  initialOffset,
                                   fm_bool blockAllocation);
static fm_int GetBlockSizeBin(fm_int  blockLength);
static void ForceUpdateArpTableStats(fm_int  sw);
static void UpdateArpTableStatsAfterAllocation(fm_int sw,
                                               fm_int baseOffset,
                                               fm_int blockLenght);
static void UpdateArpTableStatsAfterRelease(fm_int sw,
                                            fm_int baseOffset,
                                            fm_int blockLenght);
static fm_bool ValidateArpHandle(fm_int    sw,
                                 fm_uint16 arpBlockHandle);
static fm10000_ArpBlockCtrl *GetArpBlockPtr(fm_int    sw,
                                            fm_uint16 arpBlockHandle);
static fm_uint16 GetArpBlockOffset(fm_int    sw,
                                   fm_uint16 arpBlockHandle);
static fm_uint16 GetArpBlockLength(fm_int       sw,
                                   fm_uint16    arpBlockHandle);
static fm_uint16 GetArpBlockFlags(fm_int       sw,
                                  fm_uint16    arpBlockHandle);
static fm_uint16 GetArpBlockNumOfClients(fm_int       sw,
                                         fm_uint16    arpBlockHandle);
static fm_uint16 GetArpBlockOwner(fm_int       sw,
                                  fm_uint16    arpBlockHandle);
static fm_uint GetArpBlockOpaque(fm_int    sw,
                                 fm_uint16 arpBlockHandle);
static void SetArpBlockOpaque(fm_int    sw,
                              fm_uint16 arpBlockHandle,
                              fm_uint   opaque);
static fm_status GetEcmpGroupArpBlockHandle(fm_int            sw,
                                            fm_intEcmpGroup * pEcmpGroup,
                                            fm_uint16 *       pArpBlockHandle);
static fm_status SetEcmpGroupArpBlockHandle(fm_int            sw,
                                             fm_intEcmpGroup * pEcmpGroup,
                                             fm_uint16         arpBlockHandle);
static fm_status ArpFreeBlockDirectLookup(fm_int     sw,
                                          fm_int     blkLength,
                                          fm_int     upperBound,
                                          fm_uint16 *pBlockOffset);
static fm_status ArpFreeBlockInverseLookup(fm_int     sw,
                                           fm_int     blkLength,
                                           fm_uint16 *pBlockOffset);
static fm_status GetArpBlockToFitIn(fm_int     sw,
                                    fm_int     minScanIndex,
                                    fm_uint16  maxBlockLength,
                                    fm_uint16 *pBlkHandle);
static fm_status CloneArpBlockAndNotify(fm_int  sw,
                                        fm_int  blkHandle,
                                        fm_int  dstArpOffset);
static fm_status ChooseBestBlockToPack(fm_int                  sw,
                                       fm_int                  sIndex,
                                       fm_int                  recursionDepth,
                                       fm_int                  highScore,
                                       fm10000_NextHopSysCtrl *pNextHopCtrlStruct,
                                       fm_uint16 *             pBestBlockHandle);
static fm_status GetNextBlockToPack(fm_int     sw,
                                    fm_int     scanIndex,
                                    fm_uint16 *pBlockHandle,
                                    fm_uint16 *pBlockOffset, 
                                    fm_uint16 *pBlockLength);
static fm_status CreateArpTableImage(fm_int      sw,
                                     fm_int      startIndex,
                                     fm_int      numOfEntries,
                                     fm_uint64  *pDestBuffer);
static fm_status MoveArpBlockIntoFreeEntries(fm_int  sw,
                                             fm_int  startIndex,
                                             fm_bool maintenanceMode,
                                             fm_int *pMovedBlockSize);
static fm_int GetNextGroupOfArpFreeEntries(fm_int  sw,
                                           fm_int  startIndex);
static fm_status PackArpTableDefragStage(fm_int  sw,
                                         fm_int *movedBlocks);
static fm_status PackArpTablePackingStage(fm_int  sw,
                                          fm_int *pMovedBlocks);
static fm_status PackArpTable(fm_int  sw,
                              fm_int  blkLength,
                              fm_int  maxIterations);
static fm_status MaintenanceArpTablePacking(fm_int  sw,
                                            fm_int  startIndex);
static fm_status GetNewArpBlkHndl(fm_int     sw,
                                  fm_uint16 *pArpBlockHndl);
static fm_status SwapArpHandles(fm_int     sw,
                                fm_uint16  arpBlockHndl1,
                                fm_uint16  arpBlockHndl2);
static fm_status FillArpHndlTable(fm_int     sw,
                                  fm_uint16  blkOffset,
                                  fm_uint16  blkLength,
                                  fm_uint16  blkHandle);
static fm_status CleanUpArpTable(fm_int     sw,
                                 fm_uint16  blkOffset,
                                 fm_uint16  blkLength);
static fm_status UpdateEcmpGroupArpBlockInfo(fm_int    sw,
                                             fm_uint16 arpBlkHndl,
                                             fm_int    newBlkOffset,
                                             fm_int    oldBlkOffset);
static fm_status UpdateNextHopData(fm_int           sw,
                                   fm_intEcmpGroup *pEcmpGroup,
                                   fm_intNextHop *  pNextHop);
static fm_status SetEcmpGroupType(fm_int             sw,
                                  fm10000_EcmpGroup *pEcmpGroupExt);
static fm_status AllocateEcmpGroupSpecResources(fm_int             sw,
                                                 fm10000_EcmpGroup *pEcmpGroupExt);
static fm_status CheckRequiredNextHopExtensions(fm_int           sw,
                                                fm_intEcmpGroup *pParentEcmpGroup,
                                                fm_int *         pNextHopToAdd);
static fm_status InitFixedSizeEcmpGroupArpData(fm_int           sw,
                                               fm_intEcmpGroup *pParentEcmpGroup);
static fm_status ReleaseNextHopExtensions(fm_int           sw,
                                          fm_intEcmpGroup *pParentEcmpGroup,
                                          fm_int           nextHopTableSize,
                                          fm_int *         pIndexNextHopToRemoveTab);
static fm_status ReleaseAllNextHopExtensions(fm_int           sw,
                                             fm_intEcmpGroup *pParentEcmpGroup);
static fm_status SetNextHopArpIndexes(fm_int           sw,
                                      fm_intEcmpGroup *pParentEcmpGroup,
                                      fm_uint16        arpBlkHndl);
static fm_status AllocateArpForEcmpGroup(fm_int sw, fm_intEcmpGroup *group);
static 
fm_status AllocateFixedSizeEcmpGroupResources(fm_int             sw,
                                               fm10000_EcmpGroup *pEcmpGroupExt);
static fm_status SetupEcmpGroupArpEntries(fm_int           sw,
                                          fm_intEcmpGroup *pEcmpGroup,
                                          fm_uint16        arpBlkHndl,
                                          fm_bool          updateNextHopData);
static fm_bool CheckArpBlockAvailability(fm_int sw, 
                                         fm_int arpCount);
static fm_status DeleteUnresolvedNextHopRedirectTrigger(fm_int sw);
static fm_status CreateUnresolvedNextHopRedirectTrigger(fm_int sw);
static fm_status BuildGlortArpData(fm_int     sw,
                                   fm_int     logicalPort,
                                   fm_byte    mtuIndex,
                                   fm_bool    markRouted,
                                   fm_uint64 *pArpData);
static fm_status BuildGlortArp(fm_int             sw,
                               fm10000_EcmpGroup *pEcmpGroup,
                               fm_int             logicalPort);
static fm_status BuildNextHopArp(fm_int           sw,
                                 fm10000_NextHop *pNextHop);
static fm_status BuildNextHopTunnel(fm_int           sw,
                                    fm10000_NextHop *pNextHop);
static fm_status BuildNextHopVNTunnel(fm_int           sw,
                                      fm10000_NextHop *pNextHop);
static fm_status SetArpEntryUsedStatus(fm_int   sw,
                                       fm_int   arpBlkIndex,
                                       fm_int   arpBlkLength);


/*****************************************************************************
 * Macros
 *****************************************************************************/

/* Helpers to look at ARP used table entries */
#define FM10000_ARP_USED_INDEX_FOR_ENTRY(entry) (entry >> 5)
#define FM10000_ARP_USED_BIT_FOR_ENTRY(entry)   (1 << (entry & 0x1f))


/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** AllocArpBlkCtrlTab
 * \ingroup intNextHop
 *
 * \desc            Allocates and intializes the ARP-Block control table. This
 *                  table, that is allocated the first time the ARP table is
 *                  used, storages the pointers to the block control
 *                  structures and it is indexed using the handles. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if the ressource cannot be allocated.
 * \return          FM_ERR_UNSUPPORTED if NextHop feature is not supported.
 *                  
 *****************************************************************************/
static fm_status AllocArpBlkCtrlTab(fm_int sw)
{
    fm_status              err;
    fm10000_switch *       pSwitchExt;
    fm_int                 tsize;
    fm10000_ArpBlockCtrl * (*ppArpBlkCtrlTabTmp)[];


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d\n",
                  sw );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_ERR_NO_MEM;

    if (pSwitchExt->pNextHopSysCtrl == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        tsize = FM10000_ARP_BLK_CTRL_TAB_SIZE * sizeof(fm10000_ArpBlockCtrl*);
        ppArpBlkCtrlTabTmp =  fmAlloc (tsize);

        if (ppArpBlkCtrlTabTmp != NULL)
        {
            FM_MEMSET_S(ppArpBlkCtrlTabTmp, tsize, 0, tsize);
            pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab = ppArpBlkCtrlTabTmp;
            err = FM_OK;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocArpBlkCtrlTab */




/*****************************************************************************/
/** AllocNextHopExtensions
 * \ingroup intNextHop
 *
 * \desc            Allocs the required low level nextHop extensions. This
 *                  function may create an optional undo table which permits
 *                  release the added extensions in case of eventual errors.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pParentEcmpGroup pointer to parent (high level) ECMP group
 *                  structure
 * 
 * \param[in]       arpBlkHndl is the handle of the ARP block allocated for
 *                  the specified ECMP group.
 * 
 * \param[in]       undoTabSize is the size of the undo table; it should be 0
 *                  if the undo table is not used.
 *
 * \param[out]      pUndoTab is a pointer to a caller allocated table where the
 *                  undo information will be stored. It may be NULL.
 *                  The undo info are the indexes of the NextHops where
 *                  extensions have been added.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 * \return          FM_ERR_NO_MEM if one of extensions could not be allocated
 *                  (though all previous extensions were allocated and correctly
 *                  stored in the undo table)
 *                  
 *****************************************************************************/
static fm_status AllocNextHopExtensions(fm_int           sw,
                                        fm_intEcmpGroup *pParentEcmpGroup,
                                        fm_int           arpBlkHndl,
                                        fm_int           undoTabSize,
                                        fm_int *         pUndoTab)
{
    fm_status        err;
    fm_int           index;
    fm_int           maxIndex;
    fm_int           allocCnt;
    fm_int           maxAllocations;
    fm_int           undoTabIndex;
    fm10000_NextHop *pArpNextHopExt;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pParentEcmpGroup=%p, arpBlkHndl=%d, undoTabSize=%d, "
                  "pUndoTab=%p\n",
                  sw,
                  (void *) pParentEcmpGroup,
                  arpBlkHndl,
                  undoTabSize,
                  (void *) pUndoTab );

    err = FM_OK;

    /* argument validation */
    if (pParentEcmpGroup == NULL ||
        (undoTabSize != 0 && pUndoTab == NULL))
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* init undo table */
        if (pUndoTab != NULL)
        {
            undoTabIndex = 0;

            while (undoTabIndex < undoTabSize)
            {
                *(pUndoTab + undoTabIndex++) = FM_NEXTHOP_INDEX_UNSPECIFIED;
            }
        }

        /* allocate fm10000 nexthop extensions */
        maxIndex = pParentEcmpGroup->maxNextHops;
        undoTabIndex = 0;
        allocCnt = 0;

        /* limit the number of iterations to minimum */
        maxAllocations = undoTabSize != 0 ? undoTabSize : maxIndex;

        for (index = 0; index < maxIndex ; index++)
        {
            if (pParentEcmpGroup->nextHops[index] != NULL &&
                (pParentEcmpGroup->nextHops[index])->extension == NULL)
            {
                pArpNextHopExt = fmAlloc(sizeof(fm10000_NextHop));

                if (pArpNextHopExt == NULL)
                {
                    err = FM_ERR_NO_MEM;
                    break;
                }
                else
                {
                    /* initialize pArpNextHopExt */
                    pArpNextHopExt->pParent         = pParentEcmpGroup->nextHops[index];
                    pArpNextHopExt->arpBlkHndl     = arpBlkHndl;
                    pArpNextHopExt->arpBlkRelOffset = FM10000_ARP_BLOCK_INVALID_OFFSET;

                    /* attach the extension to the parent structure */
                    (pParentEcmpGroup->nextHops[index])->extension = pArpNextHopExt;                  

                    if (pUndoTab != NULL && undoTabIndex < undoTabSize)
                    {
                        /* save the next hop index */
                        *(pUndoTab + undoTabIndex++) = index;
                    }

                    /* quit once completed the expected number of allocations */
                    if (++allocCnt >= maxAllocations)
                    {
                        break;
                    }
                }
            }
        }   /* end for (index = 0...) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocNextHopExtensions */




/*****************************************************************************/
/** AllocateEcmpGroup
 * \ingroup intNextHop
 *
 * \desc            Allocates the required ressouces for a new, low level
 *                  ECMP group at hardware level.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      ppEcmpGroupExt pointer to a caller allocated storage
 *                   used to return a pointer to the new ECMP group.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT
 * \return          FM_ERR_NO_MEM if the ressource cannot be allocated.
 *                  
 *****************************************************************************/
static fm_status AllocateEcmpGroup(fm_int              sw,
                                   fm10000_EcmpGroup **ppEcmpGroupExt)
{
    fm_status   err;
    fm_int      tsize;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, ppEcmpGroupExt=%p\n",
                  sw,
                  (void *) ppEcmpGroupExt );

    err = FM_OK;

    if (ppEcmpGroupExt == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        tsize = sizeof(fm10000_EcmpGroup);
        *ppEcmpGroupExt = (fm10000_EcmpGroup*) fmAlloc(tsize);

        if (*ppEcmpGroupExt == NULL)
        {
            err = FM_ERR_NO_MEM;
        }
        else
        {
            FM_MEMSET_S(*ppEcmpGroupExt, tsize, 0, tsize);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocateEcmpGroup */




/*****************************************************************************/
/** UpdateFirstFreeArpEntry
 * \ingroup intNextHop
 *
 * \desc            Updates the first free entry to the ARP table. This
 *                  function looks for a free entry starting at the initial
 *                  offset specified by initialOffset. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       initialOffset is the offset of the lower position that
 *                  was modified.
 * 
 * \param[in]       blockAllocation tells if the update is being performed
 *                   after a block allocation (TRUE) or a block release (FALSE)
 *                   This changes the sens of the scanning.
 * 
 *                  
 *****************************************************************************/
static void UpdateFirstFreeArpEntry(fm_int  sw,
                                    fm_int  initialOffset,
                                    fm_bool blockAllocation)
{
    fm10000_switch *pSwitchExt;
    fm_uint16 *     pArpHndlTabScan;
    fm_uint16 *     pArpHndlTabUpperBound;
    fm_uint16 *     pArpHndlTabLowerBound;


    pSwitchExt = GET_SWITCH_EXT(sw);

    if ( initialOffset < 1 ||
         initialOffset >= FM10000_ARP_TABLE_SIZE )
    {
        /* the specified intial offset is invalid, start from the begining of the table */
        initialOffset = 1;

        /* force a scanning */
        blockAllocation = TRUE;
    }


    if (blockAllocation == FALSE)
    {
        if (initialOffset < pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry)
        {
            pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry = initialOffset;
        }

    }
    else
    {
        /* a block was allocated */
        if (initialOffset <= pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry)
        {
            pArpHndlTabScan       = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[initialOffset];
            pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE - 1];
            pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];

            while (pArpHndlTabScan < pArpHndlTabUpperBound)
            {
                if (*pArpHndlTabScan == FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    break;
                }
                pArpHndlTabScan++;
            }

            if (pArpHndlTabScan < pArpHndlTabUpperBound)
            {
                pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry = pArpHndlTabScan - pArpHndlTabLowerBound;
            }
            else
            {
                /* no free entry was found */
                pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry = 0;
            }
        }
    }

}   /* end UpdateFirstFreeArpEntry */




/*****************************************************************************/
/** UpdateLastUsedArpEntry
 * \ingroup intNextHop
 *
 * \desc            Updates the last used entry in the ARP table. This
 *                  function looks for the last used entry. The search
 *                  is performed in reverse order starting at the initial
 *                  offset specified by initialOffset. 
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       initialOffset is the offset of the higher position that
 *                  was modified.
 *
 * \param[in]       blockAllocation tells if the update is being performed
 *                   after a block allocation (TRUE) or a block release (FALSE)
 * 
 *****************************************************************************/
static void UpdateLastUsedArpEntry(fm_int  sw,
                                   fm_int  initialOffset,
                                   fm_bool blockAllocation)
{
    fm10000_switch *pSwitchExt;
    fm_uint16 *     pArpHndlTabScan;
    fm_uint16 *     pArpHndlTabLowerBound;


    pSwitchExt = GET_SWITCH_EXT(sw);

    if ( initialOffset < 1 ||
         initialOffset >= FM10000_ARP_TABLE_SIZE )
    {
        /* the specified intial offset is invalid, start from the end of the table */
        initialOffset = FM10000_ARP_TABLE_SIZE - 1;
        /* force a scan */
        blockAllocation = FALSE;
    }

    if (blockAllocation == TRUE)
    {
        if (initialOffset > pSwitchExt->pNextHopSysCtrl->arpHndlTabLastUsedEntry)
        {
            pSwitchExt->pNextHopSysCtrl->arpHndlTabLastUsedEntry = initialOffset;
        }
    }
    else
    {
        if (initialOffset >= pSwitchExt->pNextHopSysCtrl->arpHndlTabLastUsedEntry)
        {
            /* a block was released, so scan the table in reverse order */
            pArpHndlTabScan       = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[initialOffset];
            pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];

            while (pArpHndlTabScan > pArpHndlTabLowerBound)
            {
                if (*pArpHndlTabScan != FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    break;
                }
                pArpHndlTabScan--;
            }

            if (pArpHndlTabScan > pArpHndlTabLowerBound)
            {
                /* determine the index of the used entry */
                pSwitchExt->pNextHopSysCtrl->arpHndlTabLastUsedEntry = pArpHndlTabScan - pArpHndlTabLowerBound;
                }
            else
            {
                /* no used entry was found */
                pSwitchExt->pNextHopSysCtrl->arpHndlTabLastUsedEntry = 0;
            }
        }

    }

}   /* end UpdateLastUsedArpEntry */




/*****************************************************************************/
/** GetBlockSizeBin
 * \ingroup intNextHop
 *
 * \desc            Returns the block size bin according to the specified
 *                  block length.
 *
 * \param[in]       blockLength is the length of the block.
 * 
 * \return          the block size bin.
 * 
 *                  
 *****************************************************************************/
static fm_int GetBlockSizeBin(fm_int  blockLength)
{
    fm_int  blockSizeBin;


    if (blockLength <= 1)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_1;
    }
    else if (blockLength <= 2)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_2;
    }
    else if (blockLength <= 4)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_3_TO_4;
    }
    else if (blockLength <= 8)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_5_TO_8;
    }
    else if (blockLength <= 16)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_9_TO_16;
    }
    else if (blockLength <= 64)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_17_TO_64;
    }
    else if (blockLength <= 256)
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_65_TO_256;
    }
    else
    {
        blockSizeBin = FM10000_ARP_BLOCK_SIZE_MORE_THAN_257;
    }

    return blockSizeBin;

}   /* end GetBlockSizeBin */




/*****************************************************************************/
/** ForceUpdateArpTableStats
 * \ingroup intNextHop
 *
 * \desc            Updates the ARP table statistics after the allocation of
 *                  a block of entries.
 *
 * \param[in]       sw is the switch number.
 * 
 *                  
 *****************************************************************************/
static void ForceUpdateArpTableStats(fm_int  sw)
{
    fm10000_switch *pSwitchExt;
    fm_uint16 *     pArpHndBlockScan;
    fm_uint16 *     pArpHndlTabUpperBound;
    fm_uint16       currHandle;
    fm_int          blkSize;

    pSwitchExt = GET_SWITCH_EXT(sw);

    pArpHndBlockScan = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[1];
    pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE];


    FM_CLEAR(pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo);
    FM_CLEAR(pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo);

    /* set the initial conditions */
    currHandle = *pArpHndBlockScan++;
    blkSize = 1;

    while (pArpHndBlockScan < pArpHndlTabUpperBound)
    {
        if (*pArpHndBlockScan != currHandle)
        {
            if (currHandle == FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[GetBlockSizeBin(blkSize)]++;
            }
            else
            {
                (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[GetBlockSizeBin(blkSize)]++;
            }
            blkSize = 1;
            currHandle = *pArpHndBlockScan;
        }
        else
        {
            blkSize++;
        }
        
        pArpHndBlockScan++;
    }

    if (currHandle == FM10000_ARP_BLOCK_INVALID_HANDLE)
    {
        (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[GetBlockSizeBin(blkSize)]++;
    }
    else
    {
        (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[GetBlockSizeBin(blkSize)]++;
    }

    pSwitchExt->pNextHopSysCtrl->arpStatsAgingCounter = 0;

}   /* end ForceUpdateArpTableStats */




/*****************************************************************************/
/** UpdateArpTableStatsAfterAllocation
 * \ingroup intNextHop
 *
 * \desc            Updates the ARP table statistics after the allocation of
 *                  a block of entries.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       baseOffset is the base offset of the allocated block.
 * 
 * \param[in]       blockLenght is the length of the allocated block.
 * 
 *                  
 *****************************************************************************/
static void UpdateArpTableStatsAfterAllocation(fm_int  sw,
                                               fm_int  baseOffset,
                                               fm_int  blockLenght)
{
    fm10000_switch *pSwitchExt;
    fm_int          sizeBin;
    fm_int          previousEmptyBlockSize;
    fm_int          followingEmptyBlockSize;
    fm_uint16 *     pArpHndlBlockBase;
    fm_uint16 *     pArpHndlTabScan;
    fm_uint16 *     pArpHndlTabUpperBound;
    fm_uint16 *     pArpHndlTabLowerBound;


    pSwitchExt = GET_SWITCH_EXT(sw);

    if ( (baseOffset <  1)                                          ||
         (blockLenght <= 0)                                         ||
         (baseOffset + blockLenght > FM10000_ARP_TABLE_SIZE)        ||
         ((pSwitchExt->pNextHopSysCtrl->arpStatsAgingCounter)++ >
          FM10000_ARP_STATS_UPDATE_TRIGGER_LEVEL) )
    {
        ForceUpdateArpTableStats(sw);
    }
    else
    {
        /* update the allocated block state information */
        sizeBin = GetBlockSizeBin(blockLenght);
        (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[sizeBin]++;

        
        /* free areas must be managed in an special way because the allocated block may
         * break a large free area in 2 smaller ones. For this reason is necessary to check 
         * if there are free areas before and after the allocated block and adjust the 
         * respective counters */

        /* check if there are free areas before and after the current block */
        pArpHndlBlockBase     = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[baseOffset];
        pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE - 1];
        pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];
        previousEmptyBlockSize = 0;
        followingEmptyBlockSize = 0;

        pArpHndlTabScan = pArpHndlBlockBase - 1;
        while ( pArpHndlTabScan > pArpHndlTabLowerBound &&
               *pArpHndlTabScan-- == FM10000_ARP_BLOCK_INVALID_HANDLE )
        {
            previousEmptyBlockSize++;
        }

        pArpHndlTabScan = pArpHndlBlockBase + blockLenght;
        while ( pArpHndlTabScan <= pArpHndlTabUpperBound &&
               *pArpHndlTabScan++ == FM10000_ARP_BLOCK_INVALID_HANDLE )
        {
            followingEmptyBlockSize++;
        }

        /* update the free block state information */

        sizeBin = GetBlockSizeBin(blockLenght + previousEmptyBlockSize + followingEmptyBlockSize);
        (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[sizeBin]--;

        if (previousEmptyBlockSize > 0)
        {
            sizeBin = GetBlockSizeBin(previousEmptyBlockSize);
            (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[sizeBin]++;
        }

        if (followingEmptyBlockSize > 0)
        {
            sizeBin = GetBlockSizeBin(followingEmptyBlockSize);
            (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[sizeBin]++;
        }
    }

}   /* end UpdateArpTableStatsAfterAllocation */




/*****************************************************************************/
/** UpdateArpTableStatsAfterRelease
 * \ingroup intNextHop
 *
 * \desc            Updates the ARP table statistics after releasing a block
 *                  of entries.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       baseOffset is the base offset of the released block.
 * 
 * \param[in]       blockLenght is the length of the released block.
 * 
 *                  
 *****************************************************************************/
static void UpdateArpTableStatsAfterRelease(fm_int  sw,
                                            fm_int  baseOffset,
                                            fm_int  blockLenght)
{
    fm10000_switch *pSwitchExt;
    fm_int          sizeBin;
    fm_int          previousEmptyBlockSize;
    fm_int          followingEmptyBlockSize;
    fm_uint16 *     pArpHndlBlockBase;
    fm_uint16 *     pArpHndlTabScan;
    fm_uint16 *     pArpHndlTabUpperBound;
    fm_uint16 *     pArpHndlTabLowerBound;


    pSwitchExt = GET_SWITCH_EXT(sw);

    if ( (baseOffset <  1)                                       ||
         (blockLenght   <= 0)                                    ||
         (baseOffset + blockLenght > FM10000_ARP_TABLE_SIZE)     ||
         ((pSwitchExt->pNextHopSysCtrl->arpStatsAgingCounter)++ >
          FM10000_ARP_STATS_UPDATE_TRIGGER_LEVEL) )
    {
        ForceUpdateArpTableStats(sw);
    }
    else
    {
        /* update the allocated block state information */
        sizeBin = GetBlockSizeBin(blockLenght);
            
        (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[sizeBin]--;

        /* free areas must be managed in an special way because contiguous free areas combines into
         * a bigger, single area. For this reason is necessary to check if there are free areas
         * before and after the released block and adjust the respective counters */

        /* check if there are free areas before and after the current block */
        pArpHndlBlockBase     = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[baseOffset];
        pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE - 1];
        pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];
        previousEmptyBlockSize = 0;
        followingEmptyBlockSize = 0;

        pArpHndlTabScan = pArpHndlBlockBase - 1;
        while ( pArpHndlTabScan > pArpHndlTabLowerBound &&
               *pArpHndlTabScan-- == FM10000_ARP_BLOCK_INVALID_HANDLE)
        {
            previousEmptyBlockSize++;
        }

        pArpHndlTabScan = pArpHndlBlockBase + blockLenght;
        while ( (pArpHndlTabScan <= pArpHndlTabUpperBound) &&
                (*pArpHndlTabScan++ == FM10000_ARP_BLOCK_INVALID_HANDLE) )
        {
            followingEmptyBlockSize++;
        }

        /* update the free block state information */

        if (previousEmptyBlockSize > 0)
        {
            sizeBin = GetBlockSizeBin(previousEmptyBlockSize);
            (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[sizeBin]--;
        }

        if (followingEmptyBlockSize > 0)
        {
            sizeBin = GetBlockSizeBin(followingEmptyBlockSize);
            (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[sizeBin]--;
        }

        sizeBin = GetBlockSizeBin(blockLenght + previousEmptyBlockSize + followingEmptyBlockSize);
        (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[sizeBin]++;

    }

}   /* end UpdateArpTableStatsAfterRelease */




/*****************************************************************************/
/** ValidateArpHandle
 * \ingroup intNextHop
 *
 * \desc            Validates the ARP handle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle to be validated.
 * 
 * \return          TRUE if the handle is valid or FALSE otherwise.
 *                  
 *****************************************************************************/
static fm_bool ValidateArpHandle(fm_int    sw,
                                 fm_uint16 arpBlockHandle)
{
    fm_bool               valid;
    fm10000_switch *      pSwitchExt;
    

    pSwitchExt = GET_SWITCH_EXT(sw);
    valid = FALSE;

    if (arpBlockHandle > 0  &&
        arpBlockHandle < FM10000_ARP_BLK_CTRL_TAB_SIZE &&
        pSwitchExt->pNextHopSysCtrl != NULL &&
        pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab != NULL &&
        ((*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlockHandle]) != NULL)
    {
        valid = TRUE;
    }

    return valid;

}   /* end ValidateArpHandle */




/*****************************************************************************/
/** GetArpBlockPtr
 * \ingroup intNextHop
 *
 * \desc            Returns a pointer to the ARP block structure specified by
 *                  arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          A pointer to the structure or NULL on error.
 *                  
 *****************************************************************************/
static fm10000_ArpBlockCtrl *GetArpBlockPtr(fm_int    sw,
                                            fm_uint16 arpBlockHandle)
{
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    

    pSwitchExt = GET_SWITCH_EXT(sw);
    pArpBlkCtrl = NULL;

    if (arpBlockHandle > 0  &&
        arpBlockHandle < FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        if (pSwitchExt->pNextHopSysCtrl != NULL &&
            pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab != NULL)
        {
            pArpBlkCtrl = (*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlockHandle];
        }
    }

    return pArpBlkCtrl;

}   /* end GetArpBlockPtr */




/*****************************************************************************/
/** GetArpBlockOffset
 * \ingroup intNextHop
 *
 * \desc            Returns the base offset for the ARP block specified by
 *                  arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          The offset of the block if successful or
 *                  ''FM10000_ARP_BLOCK_INVALID_OFFSET'' otherwise.
 *                  
 *****************************************************************************/
static fm_uint16 GetArpBlockOffset(fm_int    sw,
                                   fm_uint16 arpBlockHandle)
{
    fm_uint16             offset;
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    

    pSwitchExt = GET_SWITCH_EXT(sw);
    offset = FM10000_ARP_BLOCK_INVALID_OFFSET;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        if ( (offset = pArpBlkCtrl->offset) == 0 ||
              offset >= FM10000_ARP_TAB_SIZE )
        {
            offset = FM10000_ARP_BLOCK_INVALID_OFFSET;
        }
    }

    return offset;

}   /* end GetArpBlockOffset */




/*****************************************************************************/
/** GetArpBlockLength
 * \ingroup intNextHop
 *
 * \desc            Returns the length of ARP block specified by
 *                  arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          The length of the block if successful or 0 (zero)
 *                  otherwise.
 *                  
 *****************************************************************************/
static fm_uint16 GetArpBlockLength(fm_int       sw,
                                   fm_uint16    arpBlockHandle)
{
    fm_uint16             length;
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    

    pSwitchExt = GET_SWITCH_EXT(sw);
    length = 0;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        if ( (length = pArpBlkCtrl->length) >= (FM10000_ARP_TAB_SIZE-1) )
        {
            /* invalid length */
            length = 0;
        }
    }

    return length;

}   /* end GetArpBlockOffset */




/*****************************************************************************/
/** GetArpBlockFlags
 * \ingroup intNextHop
 *
 * \desc            Returns the option flags for the ARP block specified by
 *                  arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          The option flags of the block if successful or 0xffff
 *                  otherwise.
 *                  
 *****************************************************************************/
static fm_uint16 GetArpBlockFlags(fm_int       sw,
                                  fm_uint16    arpBlockHandle)
{
    fm_uint16             optionFlags;
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    
    pSwitchExt = GET_SWITCH_EXT(sw);
    optionFlags = 0xffff;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        optionFlags = pArpBlkCtrl->options;
    }

    return optionFlags;

}   /* end GetArpBlockFlags */




/*****************************************************************************/
/** GetArpBlockNumOfClients
 * \ingroup intNextHop
 *
 * \desc            Returns the number of clients for the ARP block specified
 *                  by arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          The number of clients for the block if successful or 0xffff
 *                  otherwise.
 *                  
 *****************************************************************************/
static fm_uint16 GetArpBlockNumOfClients(fm_int       sw,
                                         fm_uint16    arpBlockHandle)
{
    fm_uint16             numClients;
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    fm_int                index;
    
    pSwitchExt = GET_SWITCH_EXT(sw);
    numClients = 0xffff;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        numClients = 0;

        for (index = 0; index < FM10000_MAX_ARP_BLOCK_CLIENTS; index++)
        {
            if ( (pArpBlkCtrl->clients)[index] != FM10000_ARP_CLIENT_NONE )
            {
                numClients++;
            }
        }
    }

    return numClients;

}   /* end GetArpBlockNumOfClients */




/*****************************************************************************/
/** GetArpBlockOwner
 * \ingroup intNextHop
 *
 * \desc            Returns the owner, which is the first client, of the ARP
 *                  block specified by arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          The owner of the block if successful or 0xffff
 *                  otherwise.
 *                  
 *****************************************************************************/
static fm_uint16 GetArpBlockOwner(fm_int       sw,
                                  fm_uint16    arpBlockHandle)
{
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    fm_uint16             owner;
    
    pSwitchExt = GET_SWITCH_EXT(sw);
    owner = 0xffff;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        owner = (pArpBlkCtrl->clients)[0];
    }

    return owner;

}   /* end GetArpBlockOwner */




/*****************************************************************************/
/** GetArpBlockOpaque
 * \ingroup intNextHop
 *
 * \desc            Returns the opaque variable stored in the ARP block
 *                  specified by arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \return          The value of the opaque variable stored in the ARP block.
 *                  
 *****************************************************************************/
static fm_uint GetArpBlockOpaque(fm_int    sw,
                                 fm_uint16 arpBlockHandle)
{
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    fm_uint               opaque;
    

    pSwitchExt = GET_SWITCH_EXT(sw);
    opaque = 0;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        opaque = pArpBlkCtrl->opaque;
    }
    
    return opaque;

}   /* end GetArpBlockOpaque */




/*****************************************************************************/
/** SetArpBlockOpaque
 * \ingroup intNextHop
 *
 * \desc            Sets the opaque variable to be stored in the ARP control
 *                  block stucture specified by arpBlockHandle.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlockHandle is the ARP block handle.
 * 
 * \param[in]       opaque is the value to be stored in the ARP control block
 *                   structure.
 *****************************************************************************/
static void SetArpBlockOpaque(fm_int    sw,
                              fm_uint16 arpBlockHandle,
                              fm_uint   opaque)
{
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    

    pSwitchExt = GET_SWITCH_EXT(sw);
    opaque = 0;

    pArpBlkCtrl = GetArpBlockPtr(sw, arpBlockHandle);
    if (pArpBlkCtrl != NULL)
    {
        pArpBlkCtrl->opaque = opaque;
    }

}   /* end SetArpBlockOpaque */




/*****************************************************************************/
/** GetEcmpGroupArpHandle
 * \ingroup intNextHop
 *
 * \desc            Returns the ARP block handle associated to the ECMP group
 *                  pointed by pEcmpGroup.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pEcmpGroup pointer to the ECMP group.
 * 
 * \param[out]      pArpBlockHandle pointer to a caller allocated storage
 *                  where this function will return the ARP handle used by
 *                  the ECMP group.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 * \return          FM_ERR_NOT_FOUND if the low level extension of the ECMP
 *                  group is not defined.
 *                  
 *****************************************************************************/
static fm_status GetEcmpGroupArpBlockHandle(fm_int            sw,
                                            fm_intEcmpGroup * pEcmpGroup,
                                            fm_uint16 *       pArpBlockHandle)
{
    fm_status          err;
    fm10000_EcmpGroup *pEcmpGroupExt;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, pArpBlockHandle=%p\n",
                  sw,
                  (void *) pEcmpGroup,
                  (void *) pArpBlockHandle );

    err = FM_OK;

    if (pEcmpGroup == NULL ||
        pArpBlockHandle == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pEcmpGroupExt = (fm10000_EcmpGroup*)(pEcmpGroup->extension);

        if (pEcmpGroupExt != NULL)
        {
            *pArpBlockHandle = pEcmpGroupExt->arpBlockHandle;
        }
        else
        {
            *pArpBlockHandle = FM10000_ARP_BLOCK_INVALID_HANDLE;
            err = FM_ERR_NOT_FOUND;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end GetEcmpGroupArpHandle */




/*****************************************************************************/
/** SetEcmpGroupArpHandle
 * \ingroup intNextHop
 *
 * \desc            Set the ARP block handle associated to the ECMP group
 *                  pointed by pEcmpGroup.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pEcmpGroup pointer to the ECMP group.
 * 
 * \param[in]       arpBlockHandle is the arp block handle.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 * \return          FM_ERR_NOT_FOUND if the low level extension of the ECMP
 *                  group is not defined.
 *                  
 *****************************************************************************/
static fm_status SetEcmpGroupArpBlockHandle(fm_int           sw,
                                            fm_intEcmpGroup *pEcmpGroup,
                                            fm_uint16        arpBlockHandle)
{
    fm_status          err;
    fm10000_EcmpGroup *pEcmpGroupExt;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, arpBlockHandle=%d\n",
                  sw,
                  (void *) pEcmpGroup,
                  arpBlockHandle );

    err = FM_OK;

    if (pEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pEcmpGroupExt = (fm10000_EcmpGroup*)(pEcmpGroup->extension);

        if (pEcmpGroupExt != NULL)
        {
            SetArpBlockOpaque(sw, arpBlockHandle, pEcmpGroup->groupId);
            pEcmpGroupExt->arpBlockHandle = arpBlockHandle;
        }
        else
        {
            err = FM_ERR_NOT_FOUND;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetEcmpGroupArpHandle */




/*****************************************************************************/
/** ArpFreeBlockDirectLookup
 * \ingroup intNextHop
 *
 * \desc            Looks for a block of blkLength consecutives free entries
 *                  in the ARP table. Lookup order is from low  to high table
 *                  indexes (direct order). If it is found, the offset of the
 *                  block is returned in pBlockOffset. If no block is found
 *                  and there was no error during the execution, an invalid
 *                  offset value will be returned using pBlockOffset and the
 *                  status will indicated no error: FM_OK.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       blkLength is the required number of entries.
 * 
 * \param[in]       upperBound is the upper limit of the search area. If
 *                   it is equal to -1 or an invalid value, the size of the
 *                   table is used as upper bound.
 * 
 * \param[out]      pBlockOffset is a caller allocated storage used to return
 *                    the offset of the block. An invalid offset is returned
 *                    if no block is found.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status ArpFreeBlockDirectLookup(fm_int     sw,
                                          fm_int     blkLength,
                                          fm_int     upperBound,
                                          fm_uint16 *pBlockOffset)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_int          startTabScanIndex;
    fm_int          emptyEntryCnt;
    fm_uint16 *     pArpHndlTabEntry;
    fm_uint16 *     pArpHndlTabStart;
    fm_uint16 *     pArpHndlTabUpperBound;
    fm_uint16 *     pArpHndlTabNextFreeEntry;
    fm_uint16 *     pArpAuxHndlTabEntry;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, blkLength=%d, upperBound = %d, pBlockOffset=%p\n",
                  sw,
                  blkLength,
                  upperBound,
                  (void *) pBlockOffset );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    pArpHndlTabEntry = NULL;
    pArpHndlTabNextFreeEntry = NULL;

    if ( pBlockOffset == NULL ||
         blkLength <= 0       ||
         blkLength >= FM10000_ARP_TABLE_SIZE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* initialize the returned value */
        *pBlockOffset = FM10000_ARP_BLOCK_INVALID_OFFSET;

        /* start searching at the last known first free position */
        startTabScanIndex = pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry;

        /* verify the intial index range */
        if (startTabScanIndex <= 0 || startTabScanIndex >= FM10000_ARP_TAB_SIZE)
        {
            /* invalid startTabScanIndex value: start from the begining of the table
             * Note that the first ARP table position is not available */
            startTabScanIndex = 1;
        }
        pArpHndlTabEntry  = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[startTabScanIndex];

        if (upperBound <=0 ||
            upperBound >= FM10000_ARP_TAB_SIZE)
        {
            pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE - 1 - blkLength];
        }
        else if (upperBound - blkLength <= 0)
        {
            /* arguments are fine but no block could be allocated. Set pArpHndlTabUpperBound
             * in such a way that the while loop will not be executed */
            pArpHndlTabUpperBound = pArpHndlTabEntry;
        }
        else
        {
            pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[upperBound - blkLength];
        }

        pArpHndlTabStart      = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];
        
        while (pArpHndlTabEntry < pArpHndlTabUpperBound)
        {
            if (*pArpHndlTabEntry == FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                /* a free entry was found: save the pointer to it entry for future allocations */
                if (pArpHndlTabNextFreeEntry == NULL)
                {
                    pArpHndlTabNextFreeEntry = pArpHndlTabEntry;
                }

                emptyEntryCnt = 1;
                pArpAuxHndlTabEntry = pArpHndlTabEntry + 1;

                /* check if there are enoungh consecutives free entries */
                while (emptyEntryCnt < blkLength)
                {
                    if (*pArpAuxHndlTabEntry++ == FM10000_ARP_BLOCK_INVALID_HANDLE)
                    {
                        emptyEntryCnt++;
                    }
                    else
                    {
                        emptyEntryCnt = -1;
                        pArpHndlTabEntry = pArpAuxHndlTabEntry;
                        break;
                    }
                }
                if (emptyEntryCnt >= blkLength)
                {
                    /* an empty block has been found */
                    /* return the offset via pBlockOffset */
                    *pBlockOffset = (fm_uint16)(pArpHndlTabEntry - pArpHndlTabStart);
                    break;
                }
            }
            /* point to the next entry */
            pArpHndlTabEntry++;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end ArpFreeBlockDirectLookup */




/*****************************************************************************/
/** ArpFreeBlockInverseLookup
 * \ingroup intNextHop
 *
 * \desc            Looks for a block of blkLength consecutives free entries
 *                  in the ARP table. Lookup order is from high to low table
 *                  indexes (inverse order). If a block it is found, the offset
 *                  of the block is returned in pBlockOffset. If no block is
 *                  found and there was no error during the execution, an
 *                  invalid offset value will be returned using pBlockOffset
 *                  and the status will indicated no error: FM_OK.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       blkLength is the required number of entries.
 * 
 * \param[out]      pBlockOffset is a caller allocated storage used to return
 *                    the offset of the block. An invalid offset is returned
 *                    if no block is found.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status ArpFreeBlockInverseLookup(fm_int     sw,
                                           fm_int     blkLength,
                                           fm_uint16 *pBlockOffset)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_int          startTabScanIndex;
    fm_int          emptyEntryCnt;
    fm_int          lowerBound;
    fm_uint16 *     pArpHndlTabEntry;
    fm_uint16 *     pArpHndlTabStart;
    fm_uint16 *     pArpHndlTabLowerBound;
    fm_uint16 *     pArpHndlTabNextFreeEntry;
    fm_uint16 *     pArpAuxHndlTabEntry;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, blkLength=%d, pBlockOffset=%p\n",
                  sw,
                  blkLength,
                  (void *) pBlockOffset );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    pArpHndlTabEntry = NULL;
    pArpHndlTabNextFreeEntry = NULL;

    if ( pBlockOffset == NULL ||
         blkLength <= 0       ||
        (blkLength-1) >= FM10000_ARP_TABLE_SIZE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pSwitchExt->pNextHopSysCtrl == NULL                  ||
             pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
             pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* initialize the returned value */
        *pBlockOffset = FM10000_ARP_BLOCK_INVALID_OFFSET;

        /* start searching at the last known first free position */
        startTabScanIndex = FM10000_ARP_TAB_SIZE - 1;
        pArpHndlTabEntry  = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[startTabScanIndex];

        /* determine the pointer to the lower position to scan, considering the size of
         * the required block */
        lowerBound = pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry;

        if (lowerBound < blkLength)
        {
            pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[blkLength];
        }
        else
        {
            pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[lowerBound];
        }
        
        pArpHndlTabStart      = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];

        while (pArpHndlTabEntry >= pArpHndlTabLowerBound)
        {
            if (*pArpHndlTabEntry == FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                /* check if there are enoungh consecutives free entries */
                emptyEntryCnt = 1;

                /* use an auxilary scan pointer */
                pArpAuxHndlTabEntry = pArpHndlTabEntry;

                /* check if there are enoungh consecutive free entries */
                while (emptyEntryCnt < blkLength)
                {
                    if (*--pArpAuxHndlTabEntry == FM10000_ARP_BLOCK_INVALID_HANDLE)
                    {
                        emptyEntryCnt++;
                    }
                    else
                    {
                        emptyEntryCnt = -1;
                        pArpHndlTabEntry = pArpAuxHndlTabEntry;
                        break;
                    }
                }
                if (emptyEntryCnt >= blkLength)
                {
                    /* an empty block has been found */
                    /* return the offset via pBlockOffset */
                    *pBlockOffset = (fm_uint16) (pArpHndlTabEntry - pArpHndlTabStart);
                    *pBlockOffset -= (blkLength - 1);
                    break;
                }

            }   /* end if (*pArpHndlTabEntry == FM10000_ARP_BLOCK_INVALID_HANDLE) */

            /* point to the next entry */
            pArpHndlTabEntry--;

        }   /* end while (pArpHndlTabEntry >= pArpHndlTabLowerBound) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end ArpFreeBlockInverseLookup */




/*****************************************************************************/
/** GetArpBlockToFitIn
 * \ingroup intNextHop
 *
 * \desc            Looks for an allocated ARP block that fits in
 *                  maxBlockLength. This functions chooses the best block (the
 *                  biggest one) that fits in. If no block is found, an invalid
 *                  handle is returned.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       minScanIndex gives the lower bound of the search.
 * 
 * \param[in]       maxBlockLength is the max allowed length for the block to
 *                  look for
 * 
 * \param[out]      pBlkHandle is a caller allocated storage used to return
 *                  the handle of the the block
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one or more of the arguments are
 *                  out of range.
 *                  
 *****************************************************************************/
static fm_status GetArpBlockToFitIn(fm_int     sw,
                                    fm_int     minScanIndex,
                                    fm_uint16  maxBlockLength,
                                    fm_uint16 *pBlkHandle)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_uint16       blkOptions;
    fm_uint16       blkLength;
    fm_uint16       bestBlockLength;
    fm_uint16 *     pScanArpHndlTab;
    fm_uint16 *     pArpHndlTabLowerBound;
    fm_uint16       (*pArpHndlArrayLoc)[FM10000_ARP_TAB_SIZE];


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, minScanIndex=%d, maxBlockLength=%d, pBlkHandle=%p\n",
                  sw,
                  minScanIndex,
                  maxBlockLength,
                  (void *) pBlkHandle );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    if ( minScanIndex <= 0                              ||
         minScanIndex >= (FM10000_ARP_TAB_SIZE - 1)     ||
         maxBlockLength == 0                            ||
         maxBlockLength >= (FM10000_ARP_TAB_SIZE - 1)   ||
         pBlkHandle == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* create a shortcut to the handle array */
        pArpHndlArrayLoc = pSwitchExt->pNextHopSysCtrl->pArpHndlArray;

        pScanArpHndlTab = &(*pArpHndlArrayLoc)[pSwitchExt->pNextHopSysCtrl->arpHndlTabLastUsedEntry];
        pArpHndlTabLowerBound = &(*pArpHndlArrayLoc)[minScanIndex];

        *pBlkHandle = FM10000_ARP_BLOCK_INVALID_HANDLE;
        bestBlockLength = 0;

        while (pScanArpHndlTab > pArpHndlTabLowerBound)
        {
            if (*pScanArpHndlTab != FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                blkLength = GetArpBlockLength(sw, *pScanArpHndlTab);
                blkOptions = GetArpBlockFlags(sw, *pScanArpHndlTab);

                /* skip unmovable blocks */
                if ( (blkOptions & FM10000_ARP_BLOCK_OPT_DO_NOT_MOVE) == 0 )
                {
                    if (blkLength == maxBlockLength)
                    {
                        /* found a block that exactly fits in, stop searching */
                        *pBlkHandle = *pScanArpHndlTab;
                        break;
                    }
                    else if (blkLength < maxBlockLength)
                    {
                        /* the block fits in but it is smaller than the allowed max size
                         * save the block data but continue searching for a better block */
                        if (blkLength > bestBlockLength)
                        {
                            *pBlkHandle = *pScanArpHndlTab;
                            bestBlockLength = blkLength;
                        }
                    }

                }
                /* adjust scan pointer to point to the next block */
                pScanArpHndlTab -= blkLength;
            }
            else
            {
                pScanArpHndlTab--;
            }

        }   /* end while (pScanArpHndlTab > pArpHndlTabLowerBound) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end GetArpBlockToFitIn */




/*****************************************************************************/
/** CloneArpBlockAndNotify
 * \ingroup intNextHop
 *
 * \desc            Clones an ARP block into the destination position specified
 *                  by dstArpOffset and notify block clients. If the sequence
 *                  is succesfull, the source block is released and the control
 *                  structure is updated. At the destination offset, it must
 *                  exist enough empty entries to alloc the block.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       blkHandle is the handle of the block to be cloned.
 * 
 * \param[in]       dstBlkOffset is the offset of the destination block.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status CloneArpBlockAndNotify(fm_int  sw,
                                        fm_int  blkHandle,
                                        fm_int  dstBlkOffset)
{
    fm_status             err;
    fm_switch *           switchPtr;
    fm10000_switch *      pSwitchExt;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;
    fm_uint16             srcBlkOffset;
    fm_uint16             srcBlkLength;
    fm_int                relOffset;
    fm_uint64             arpData;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, blkHandle=%d, dstBlkOffset=%d\n",
                  sw,
                  blkHandle,
                  dstBlkOffset );

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);

    /* validate the current handle and get the parameters */
    pArpBlkCtrl = (*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[blkHandle];

    if (pArpBlkCtrl == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "ARP control structure not found, handle=%d\n",
                     blkHandle);
        err = FM_FAIL;
    }
    else
    {
        /* get parameters of the current block */
        srcBlkOffset = pArpBlkCtrl->offset;
        srcBlkLength = pArpBlkCtrl->length;

        /* mark the destination block as used */
        err = FillArpHndlTable(sw, dstBlkOffset, srcBlkLength, blkHandle);

        if (err == FM_OK)
        {
            /* clone block data */
            for (relOffset = 0; relOffset < srcBlkLength; relOffset++)
            {
                switchPtr->ReadUINT64(sw,
                                      FM10000_ARP_TABLE((srcBlkOffset+relOffset),0),
                                      &arpData);
                switchPtr->WriteUINT64(sw,
                                       FM10000_ARP_TABLE((dstBlkOffset+relOffset),0),
                                       arpData);
            }

            /* update the offset into in the ARP control block information */
            pArpBlkCtrl->offset = dstBlkOffset;

            /* notify block clients */
            err = fm10000NotifyArpBlockChange(sw,
                                              blkHandle,
                                              NULL,
                                              dstBlkOffset,
                                              srcBlkOffset);
            if (err == FM_OK)
            {
                UpdateArpTableStatsAfterAllocation(sw, dstBlkOffset, srcBlkLength);

                /* release the old block */
                FillArpHndlTable(sw, 
                                  srcBlkOffset, 
                                  srcBlkLength, 
                                  FM10000_ARP_BLOCK_INVALID_HANDLE);

                /* reset "used" flags for the old block */
                fm10000GetArpEntryUsedStatus(sw,
                                             srcBlkOffset,
                                             TRUE,
                                             NULL);
                /* update the indexes of the first free and last used ARP entries
                 *  these functions are called twice because the block is being moved,
                 *  so there is an allocation and a release */
                UpdateFirstFreeArpEntry(sw, dstBlkOffset, TRUE);
                UpdateFirstFreeArpEntry(sw, srcBlkOffset, FALSE);
                UpdateLastUsedArpEntry(sw, dstBlkOffset+srcBlkLength-1, TRUE);
                UpdateLastUsedArpEntry(sw, srcBlkOffset+srcBlkLength-1, FALSE);
                UpdateArpTableStatsAfterRelease(sw, srcBlkOffset, srcBlkLength);

            }
            else
            {
                /* ERROR: restore the block offset to its original value */
                pArpBlkCtrl->offset = srcBlkOffset;

                /* mark the new block as free */
                FillArpHndlTable(sw, 
                                  dstBlkOffset, 
                                  srcBlkLength, 
                                  FM10000_ARP_BLOCK_INVALID_HANDLE);

                /* reset "used" flags for the new block */
                fm10000GetArpEntryUsedStatus(sw,
                                             dstBlkOffset,
                                             TRUE,
                                             NULL);
            }
        }

    }   /* end if (pArpBlkCtrl == NULL) - else */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CloneArpBlockAndNotify */




/*****************************************************************************/
/** ChooseBestBlockToPack
 * \ingroup intNextHop
 *
 * \desc            Chooses a block to pack performing a scan of the ARP
 *                  table starting at the sIndex. The searching algorithm
 *                  is recursive and assigns a score to each block based on its
 *                  size and the number of empty entries before and after it.
 *                  The block whith the higher score is selected. An early
 *                  stop condition allows to stop the searching if a good
 *                  enough score is found. Blocks whose "do not move" flag
 *                  is set are skipped.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       sIndex is the starting index. 
 * 
 * \param[in]       recursionDepth is the current recursion level and it is
 *                  used to control the maximum recursion depth.
 * 
 * \param[in]       highScore is the higher block score.
 * 
 * \param[in]       pNextHopCtrlStruct pointer to the NextHop control structure
 * 
 * \param[out]      pBestBlockHandle pointer to a caller allocated storage
 *                  where this function will return the handle of the block
 *                  with the higher score.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status ChooseBestBlockToPack(fm_int                  sw,
                                       fm_int                  sIndex,
                                       fm_int                  recursionDepth,
                                       fm_int                  highScore,
                                       fm10000_NextHopSysCtrl *pNextHopCtrlStruct,
                                       fm_uint16 *             pBestBlockHandle) 
{
    fm_status             err;
    fm_int                localScore;
    fm_uint16 *           pArpHndlTabScan;
    fm_uint16 *           pArpHndlTabUpperBound;
    fm_uint16 *           pArpHndlTabLowerBound;
    fm_uint16             blkOffset;
    fm_uint16             blkLength;
    fm_uint16             blkHandle;
    fm_int                freeEntriesBefore;
    fm_int                freeEntriesAfter;
    fm10000_ArpBlockCtrl *pArpBlkCtrl;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, index=%d, recursionDepth=%d, score=%d, pNextHopCtrlStruct=%p, pBestBlockHandle=%p\n",
                  sw,
                  sIndex,
                  recursionDepth,
                  highScore,
                  (void *) pNextHopCtrlStruct,
                  (void *) pBestBlockHandle );

    err = FM_OK;
    freeEntriesBefore = 0;
    freeEntriesAfter  = 0;
    localScore = 0;

    /* argument validation is only performed at
     * the lowest recursion level */
    if ( recursionDepth == 1            &&
        (pBestBlockHandle ==  NULL      ||
         pNextHopCtrlStruct == NULL     ||
         sIndex >= FM10000_ARP_TAB_SIZE) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    /* it does not have sense to continue if the index is too small */
    else if (sIndex < FM10000_ARP_TABLE_SIZE - FM10000_ARP_MAINTENANCE_NOF_RESERVED_ENTRIES)
    {
        pArpHndlTabScan       = &(*pNextHopCtrlStruct->pArpHndlArray)[sIndex];
        pArpHndlTabUpperBound = 
            &(*pNextHopCtrlStruct->pArpHndlArray)[FM10000_ARP_TABLE_SIZE - 
                                                  FM10000_ARP_MAINTENANCE_NOF_RESERVED_ENTRIES - 1];
        pArpHndlTabLowerBound = &(*pNextHopCtrlStruct->pArpHndlArray)[0];

        /* check if there are empty entries before the block */
        do
        {
            /* free entries have an invalid handle */
            if ( (*pArpHndlTabScan) == FM10000_ARP_BLOCK_INVALID_HANDLE &&
                   freeEntriesBefore < FM10000_ARP_PACKING_EMPTY_CNT_SATURATION_LEVEL )
            {
                /* saturate freeEntriesBefore at FM10000_ARP_PACKING_EMPTY_CNT_SATURATION_LEVEL */
                freeEntriesBefore++;
            }
            else
            {
                /* the entry is not free */
                break;
            }
        }
        while (++pArpHndlTabScan < pArpHndlTabUpperBound);
        
        if ((blkHandle = *pArpHndlTabScan) != FM10000_ARP_BLOCK_INVALID_HANDLE)
        {
            pArpBlkCtrl = (*pNextHopCtrlStruct->ppArpBlkCtrlTab)[blkHandle];
            blkOffset = pArpBlkCtrl->offset;
            blkLength = pArpBlkCtrl->length;

            /* skip the rest of the block */
            pArpHndlTabScan += blkLength;

            /* check if there are empty entries after the block */
            while ( *pArpHndlTabScan == FM10000_ARP_BLOCK_INVALID_HANDLE &&
                     pArpHndlTabScan < pArpHndlTabUpperBound && 
                     freeEntriesAfter < FM10000_ARP_PACKING_EMPTY_CNT_SATURATION_LEVEL )
            {
                freeEntriesAfter++;
                pArpHndlTabScan++;
            }

            /* compute the score of the block, ignore "do not move"  blocks */
            if ( (pArpBlkCtrl->options & FM10000_ARP_BLOCK_OPT_DO_NOT_MOVE) == 0 )
            {
                localScore = freeEntriesBefore;
            }

            if (localScore > highScore)
            {
                *pBestBlockHandle = blkHandle;
                highScore = localScore;
                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING, 
                             "best block to move: index=%d, length=%d, emptyBefore=%d, emptyAfter=%d, score=%d\n",
                             blkOffset,
                             blkLength,
                             freeEntriesBefore,
                             freeEntriesAfter,
                             localScore);
            }

            if (highScore < FM10000_ARP_PACKING_QUICK_DECISION_SCORE &&
                ++recursionDepth < FM10000_ARP_DEFRAG_MAX_RECURSION_DEPTH)
            {
                /* continue scanning just after the current allocated block*/
                err = ChooseBestBlockToPack(sw,
                                            blkOffset+blkLength,
                                            recursionDepth,
                                            highScore,
                                            pNextHopCtrlStruct,
                                            pBestBlockHandle);
            }
        }
    }   /* end else if (sIndex < FM10000_ARP_TABLE_SIZE ...) */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
}




/*****************************************************************************/
/** GetNextBlockToPack
 * \ingroup intNextHop
 *
 * \desc            Looks for the next best block to be packet.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       scanIndex is initial index, the scanning is performed in
 *                  ascending order.
 * 
 * \param[out]      pBlockHandle pointer to a caller allocated storage
 *                  where this function will return the handle of the block
 *                  to pack.
 * 
 * \param[out]      pBlockOffset pointer to a caller allocated storage
 *                  where this function will return the block offset.
 *
 * \param[out]      pBlockLength pointer to a caller allocated storage
 *                  where this function will return the block length.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_UNSUPPORTED if NextHop is not supported by the
 *                   current implementation.
 *                  
 *****************************************************************************/
static fm_status GetNextBlockToPack(fm_int     sw,
                                    fm_int     scanIndex,
                                    fm_uint16 *pBlockHandle,
                                    fm_uint16 *pBlockOffset, 
                                    fm_uint16 *pBlockLength)
{
    fm_status               err;
    fm10000_switch *        pSwitchExt;
    fm10000_NextHopSysCtrl *pNextHopCtrl;
    fm_uint16               bestBlockHandle;
    fm10000_ArpBlockCtrl *  pArpBlkCtrl;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, scanIndex=%d, pBlockHandle=%p, pBlockOffset=%p, pBlockLength=%p\n",
                  sw,
                  scanIndex,
                  (void *) pBlockHandle,
                  (void *) pBlockOffset,
                  (void *) pBlockLength );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    pNextHopCtrl = pSwitchExt->pNextHopSysCtrl;
    bestBlockHandle = FM10000_ARP_BLOCK_INVALID_HANDLE;

    if (pBlockHandle == NULL ||
        pBlockOffset == NULL ||
        pBlockLength == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pNextHopCtrl == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        *pBlockHandle = FM10000_ARP_BLOCK_INVALID_HANDLE;
        *pBlockOffset = 0;
        *pBlockLength = 0;

        err = ChooseBestBlockToPack(sw,
                                    scanIndex,
                                    1,
                                    0,
                                    pNextHopCtrl,
                                    &bestBlockHandle);

        if (err == FM_OK && bestBlockHandle != FM10000_ARP_BLOCK_INVALID_HANDLE)
        {
            if ( (pArpBlkCtrl = (*pNextHopCtrl->ppArpBlkCtrlTab)[bestBlockHandle]) != NULL )
            {
                *pBlockHandle = bestBlockHandle;
                *pBlockOffset  = pArpBlkCtrl->offset;
                *pBlockLength  = pArpBlkCtrl->length;
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                             "Invalid block handle\n");

                err = FM_FAIL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end GetNextBlockToPack */




/*****************************************************************************/
/** CreateArpTableImage
 * \ingroup intNextHop
 *
 * \desc            Creates an partial image of the ARP table. The initial
 *                  index and the number entries to copy are controlled by
 *                  startIndex and numOfEntries. The content of the table
 *                  may change dynamically, so once the image was created,
 *                  it is verified againts the ARP table; if a difference
 *                  is detected, the full image is created again.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       startIndex is the intial ARP table index.
 * 
 * \param[in]       numOfEntries is the number of entries to copy.
 * 
 * \param[out]      pDestBuffer is a caller allocated storage where this
 *                  this function will create the image of the ARP table.
 *                  It must be big enough to write the number of registers
 *                  specified by numOfEntries.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status CreateArpTableImage(fm_int      sw,
                                     fm_int      startIndex,
                                     fm_int      numOfEntries,
                                     fm_uint64  *pDestBuffer)
{
    fm_status   err;
    fm_switch * switchPtr;
    fm_int      index;
    fm_int      endIndex;
    fm_int      maxIterations;
    fm_uint64   regEntry;
    fm_uint64 * pImageScan;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, startIndex=%d, numOfEntries=%d, pDestBuffer=%p\n",
                  sw,
                  startIndex,
                  numOfEntries,
                  (void *) pDestBuffer );


    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    maxIterations = 5;

    /* argument validation */
    if (startIndex   <  0                                 ||
        startIndex   >= FM10000_ARP_TAB_SIZE              ||
        numOfEntries <= 0                                 ||
        numOfEntries >  FM10000_ARP_TAB_SIZE              ||
        startIndex + numOfEntries > FM10000_ARP_TAB_SIZE  ||
        pDestBuffer == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        while (maxIterations --)
        {
            /* copy the ARP table into the buffer */
            index = startIndex;
            endIndex = startIndex + numOfEntries;
            pImageScan = pDestBuffer;

            while (index < endIndex && err == FM_OK)
            {
                /* Read hardware entry */
                err = switchPtr->ReadUINT64(sw,
                                            FM10000_ARP_TABLE(index++, 0),
                                            pImageScan++);
            }
            

            /* skip verification if there was an error */
            if (err != FM_OK)
            {
                continue;
            }

            /* verify the image */
            index = startIndex;
            endIndex = startIndex + numOfEntries;
            pImageScan = pDestBuffer;

            while (index < endIndex)
            {
                /* Read hardware entry */
                err = switchPtr->ReadUINT64(sw,
                                            FM10000_ARP_TABLE(index++, 0),
                                            &regEntry);

                if (err == FM_OK && regEntry == *pImageScan)
                {
                    pImageScan++;
                }
                else
                {
                    break;
                }
            }

            if (err == FM_OK && regEntry == *pImageScan)
            {
                /* image was validated */
                break;
            }
        }   /* end while (maxIterations --) */

        if (err == FM_OK  && maxIterations == 0)
        {
            err = FM_FAIL;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CreateArpTableImage */




/*****************************************************************************/
/** MoveArpBlockIntoFreeEntries
 * \ingroup intNextHop
 *
 * \desc            Determines the number of free ARP entries at the given
 *                  index and then tries to move an allocated ARP block from
 *                  the high ARP table there. This function looks for the block
 *                  that best fits in the available place.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       startIndex is the intial index of an free ARP block.
 * 
 * \param[in]       maintenanceMode is TRUE is this function is called during a
 *                  maintenance defragmentation, or FALSE otherwise.
 * 
 * \param[out]      pMovedBlockSize points to a caller allocated storage where
 *                  this function will return the size of the moved block or
 *                  zero if no operation was performed. This pointer may be
 *                  NULL.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if startIndex is out of range.
 *                  
 *****************************************************************************/
static fm_status MoveArpBlockIntoFreeEntries(fm_int  sw,
                                             fm_int  startIndex,
                                             fm_bool maintenanceMode,
                                             fm_int *pMovedBlockSize)
{
    fm_status               err;
    fm10000_switch *        pSwitchExt;
    fm10000_NextHopSysCtrl *pNextHopCtrl;
    fm_uint16 *             pScanArpHndlTab;
    fm_uint16               freeEntries;
    fm_uint16               blkHandle;
    fm_int                  minScanIndex;
    fm_int                  index;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, startIndex=%d, pMovedBlockSize=%p\n",
                  sw,
                  startIndex,
                  (void *) pMovedBlockSize );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    pNextHopCtrl = pSwitchExt->pNextHopSysCtrl;

    if ( startIndex <= 0 || startIndex > FM10000_ARP_TAB_AVAILABLE_ENTRIES )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (pMovedBlockSize != NULL)
        {
            *pMovedBlockSize = 0;
        }

        /* maintenance packing conditions */
        if (pNextHopCtrl->arpTabFreeEntryCount < FM10000_ARP_MAINTENANCE_PACKING_THRESHOLD)
        {
            /* determine the number of consecutive of available free entries at startIndex
             * saturates at FM10000_ARP_MAINTENANCE_MAX_MOVABLE_BLOCK_LENGTH */
            freeEntries = 0;
            pScanArpHndlTab = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[startIndex];

            index = startIndex;
            while (*pScanArpHndlTab++ == FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                if (++index > FM10000_ARP_TAB_AVAILABLE_ENTRIES)
                {
                    break;
                } 

                if ( ++freeEntries >= FM10000_ARP_MAINTENANCE_MAX_MOVABLE_BLOCK_LENGTH &&
                     maintenanceMode == TRUE )
                {
                    break;
                }
            }

            if (freeEntries > 0)
            {
                /* set the lower bound for the search of a block to move */
                minScanIndex = startIndex + freeEntries;

                err = GetArpBlockToFitIn(sw, 
                                         minScanIndex,
                                         freeEntries,
                                         &blkHandle);

                if (err == FM_OK && blkHandle != FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    err = CloneArpBlockAndNotify(sw, blkHandle, startIndex);

                    if (pMovedBlockSize != NULL && err == FM_OK)
                    {
                        *pMovedBlockSize = GetArpBlockLength(sw, blkHandle);
                    }
                }
            }

        }   /* end if (pNextHopCtrl->arpTabFreeEntryCount < ...) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end MoveArpBlockIntoFreeEntries */




/*****************************************************************************/
/** GetNextGroupOfArpFreeEntries
 * \ingroup intNextHop
 *
 * \desc            Starting at the startIndex, skip the current group of
 *                  free ARP entries, if there is any, and determines the
 *                  position of the next block of free entries.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       startIndex is the current ARP index
 * 
 * \return          the index of the following group of free entries.
 *                  
 *****************************************************************************/
static fm_int GetNextGroupOfArpFreeEntries(fm_int  sw,
                                           fm_int  startIndex)
{
    fm10000_switch *        pSwitchExt;
    fm_uint16 *             pScanArpHndlTab;
    fm_uint16 *             pArpHndlTabUpperBound;
    fm_uint16 *             pArpHndlTabLowerBound;
    fm_int                  nextFreeEntryIndex;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, startIndex=%d\n",
                  sw,
                  startIndex );

    pSwitchExt = GET_SWITCH_EXT(sw);
    nextFreeEntryIndex = FM10000_ARP_BLOCK_INVALID_OFFSET;

    if (startIndex >= 1 && startIndex < FM10000_ARP_TABLE_SIZE)
    {
        pScanArpHndlTab = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[startIndex];
        pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE - 1];
        pArpHndlTabLowerBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];

        /* skip current group of free entries */
        while (*pScanArpHndlTab == FM10000_ARP_BLOCK_INVALID_HANDLE &&
                pScanArpHndlTab < pArpHndlTabUpperBound)
        {
            pScanArpHndlTab++;
        }

        while (*pScanArpHndlTab != FM10000_ARP_BLOCK_INVALID_HANDLE &&
                pScanArpHndlTab < pArpHndlTabUpperBound)
        {
            pScanArpHndlTab++;
        }
        nextFreeEntryIndex = pScanArpHndlTab - pArpHndlTabLowerBound;
    }

    return nextFreeEntryIndex;

}   /* end GetNextGroupOfArpFreeEntries */




/*****************************************************************************/
/** PackArpTableDefragStage
 * \ingroup intNextHop
 *
 * \desc            Relocates blocks in the ARP table trying to reduce the
 *                  fragmentation.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pMovedBlocks points to a caller allocated storage where
 *                  this function will return the number of blocks that have
 *                  been moved. It may be NULL.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status PackArpTableDefragStage(fm_int  sw,
                                         fm_int *pMovedBlocks)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_int          scanIndex;
    fm_int          nbMovedBlocks;
    fm_int          iterCount;
    fm_uint16       blkHandle;
    fm_uint16       srcBlkLength;
    fm_uint16       srcBlkOffset;
    fm_uint16       dstBlkOffset;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pMovedBlocks=%p\n",
                  sw,
                  (void *) pMovedBlocks );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    iterCount = FM10000_ARP_PACKING_DEFRAG_STAGES_MAX_ITERATIONS;
    nbMovedBlocks = 0;

    while (iterCount-- > 0 && err == FM_OK)
    {
        /* always start searching at the first free entry */
        scanIndex = pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry;

        err = GetNextBlockToPack(sw, 
                                 scanIndex, 
                                 &blkHandle, 
                                 &srcBlkOffset, 
                                 &srcBlkLength);

        if (err == FM_OK && srcBlkLength > 0)
        {
            /* first, try to do a normal allocation */
            err = ArpFreeBlockDirectLookup(sw, 
                                           srcBlkLength,
                                           srcBlkOffset,
                                           &dstBlkOffset);

            if (err == FM_OK && dstBlkOffset == FM10000_ARP_BLOCK_INVALID_OFFSET)
            {
                /* try to use the swap reserved area in the ARP table */
                err = ArpFreeBlockInverseLookup(sw, 
                                                srcBlkLength,
                                                &dstBlkOffset);
            }

            /*  move the block if a place was found */
            if (err == FM_OK && dstBlkOffset != FM10000_ARP_BLOCK_INVALID_OFFSET)
            {
                err = CloneArpBlockAndNotify(sw, blkHandle, dstBlkOffset);

                /* if the swap reserved area was used it was temporary only
                 * and now we should be able to relocate the block back
                 * to the available space */
                if (dstBlkOffset >
                    (FM10000_ARP_TAB_AVAILABLE_ENTRIES + 1 - srcBlkLength))
                {
                    err = ArpFreeBlockDirectLookup(sw, 
                                                   srcBlkLength,
                                                   srcBlkOffset + srcBlkLength,
                                                   &dstBlkOffset);

                    if (err == FM_OK)
                    {
                        if (dstBlkOffset == FM10000_ARP_BLOCK_INVALID_OFFSET)
                        {
                            err = FM_ERR_ARP_TABLE_FULL;
                        }
                        else
                        {
                            err = CloneArpBlockAndNotify(sw, blkHandle, dstBlkOffset);
                        }
                    }

                    if (err != FM_OK)
                    {
                        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                                     "Cannot release the swap reserved area\n");
                        break;
                    }
                }

                if (err == FM_OK)
                {
                    /* inc counter of moved blocks */
                    nbMovedBlocks++;
                }
            }
            else if ((err == FM_OK) && 
                     (dstBlkOffset == FM10000_ARP_BLOCK_INVALID_OFFSET))
            {
                break;
            }
        }
        else
        {
            /* there was an error or no block to move was found */
            break;
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                 "Iterations=%d, Moved blocks=%d\n",
                 FM10000_ARP_PACKING_DEFRAG_STAGES_MAX_ITERATIONS - iterCount,
                 nbMovedBlocks);

    if (pMovedBlocks != NULL)
    {
        *pMovedBlocks = nbMovedBlocks;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end PackArpTableDefragStage */




/*****************************************************************************/
/** PackArpTablePackingStage
 * \ingroup intNextHop
 *
 * \desc            Moves blocks of allocated entries in the ARP table from
 *                  into groups of free entries created by either blocks that
 *                  were released or moved during the defragmentation stage.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pMovedBlocks points to a caller allocated storage where
 *                  this function will return the number of blocks that have
 *                  been moved. It may be NULL.
 * 
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status PackArpTablePackingStage(fm_int  sw,
                                          fm_int *pMovedBlocks)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_int          scanIndex;
    fm_int          nbMovedBlocks;
    fm_int          movedBlockSize;
    fm_int          iterCount;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pMovedBlocks=%p\n",
                  sw,
                  (void *) pMovedBlocks );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    iterCount = FM10000_ARP_PACKING_STAGES_MAX_ITERATIONS;
    nbMovedBlocks = 0;

    /* start at the first first free entry */
    scanIndex = pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry;

    while (iterCount-- > 0 &&
           scanIndex <= FM10000_ARP_TAB_AVAILABLE_ENTRIES &&
           err == FM_OK)
    {
        err = MoveArpBlockIntoFreeEntries(sw,
                                          scanIndex,
                                          FALSE,
                                          &movedBlockSize);

        if (err == FM_OK)
        {
            /* update the scan index */
            if (movedBlockSize > 0)
            {
                /* a block has been moved, use the new value of the first free entry */
                scanIndex = pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry;
                nbMovedBlocks++;
            }
            else
            {
                /* set the scanIndex to point to the next group of free entries */
                scanIndex = GetNextGroupOfArpFreeEntries(sw, scanIndex);
            }
        }
    }

    FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                 "Iterations=%d, Moved blocks=%d\n",
                 FM10000_ARP_PACKING_STAGES_MAX_ITERATIONS - iterCount,
                 nbMovedBlocks);

    if (pMovedBlocks != NULL)
    {
        *pMovedBlocks = nbMovedBlocks;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end PackArpTablePackingStage */




/*****************************************************************************/
/** PackArpTable
 * \ingroup intNextHop
 *
 * \desc            Relocates blocks in the ARP table trying to reduce the
 *                  fragmentation and create a room of at least blkLength
 *                  entries. This function also may be called for maintenance;
 *                  in order to do so, blkLength must be equal to -1 and
 *                  maxIterations must be used to limit the number of
 *                  iterations.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       blkLength is the required block size. If a valid value
 *                  is specified, this parameter takes precedence over the
 *                  maxIterations and the algorithm will continue iterating
 *                  trying to create a place for the required block.
 * 
 * \param[in]       maxIterations is the max number of iterations. This is
 *                  intended to be used during maintenance packing. This
 *                  parameter is ignored if a valid blkLength is specified.
 *                  If maxIterations is equal to -1, the default number
 *                  of iterations is used.
 * 
 * \return          FM_OK if successful.
 *                  FM_ERR_ARP_TABLE_FULL if failed to pack the ARP table
 *                  
 *****************************************************************************/
static fm_status PackArpTable(fm_int  sw,
                              fm_int  blkLength,
                              fm_int  maxIterations)
{
    fm_status               err;
    fm10000_switch *        pSwitchExt;
    fm10000_NextHopSysCtrl *pNextHopCtrl;
    fm_int                  iterCount;
    fm_int                  maxIterationsLocal;
    fm_int                  movedBlocksStage1;
    fm_int                  movedBlocksStage2;
    fm_int                  totalMovedBlocks;
    fm_int                  requiredBlkLength;
    fm_uint16               dstBlkOffset;
    

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, blkLength=%d, maxIterations=%d\n",
                  sw,
                  blkLength,
                  maxIterations );

    /****************************************************************************** 
     * the goal is to group all used entries at the top of the table. 
     * Two parameters allow to control the operation: blkLength and maxIterations.
     * blkLength takes precedence and it is used when the caller tries to allocate
     * a block of entries and it does not fount enough contigouos entries. In this
     * mode, table defragmentation will continue until a free group big enough to
     * allocate the required block is found or when additional defragmentation is
     * not possible. 
     * maxIterations may be used by maintenance calls in order to perform a partial
     * defragmentation with a limited number of iterations.
     * 
     * The steps to pack the table are: 
     * 
     * 1- if a blkLength was specified, check if the number of empty entries is
     *    bigger than the size of the block, and quit inmediatly if allocation is
     *    not possible.
     * 2- identify the fragmented areas of the table and select specific blocks to be
     *    moved to the swap area in order to consolidate groups of free entries. The
     *    "best" blocks to be moved are selected using a score system. 
     * 3- pack the table, moving blocks placed in the swap zone or in the high part
     *    of the table into the groups of free entries created in the previous step.
     * 4- repeat steps 2) and 3) and stop iterating if one of the following conditions
     *    is true:
     *    a) blkLength is valid and there is enough place to alloc the required block.
     *    b) the maximum number of iterations is over
     *    c) there was not any improvement in the last iteration
     *
     *********************************************************************************/

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    iterCount = 0;
    movedBlocksStage1 = 0;
    movedBlocksStage2 = 0;
    totalMovedBlocks = 0;
    dstBlkOffset = FM10000_ARP_BLOCK_INVALID_OFFSET;
    maxIterationsLocal = FM10000_ARP_PACKING_MAX_ITERATIONS;
    requiredBlkLength = 0;

    pNextHopCtrl = pSwitchExt->pNextHopSysCtrl;

    if (blkLength > 0 && blkLength < (FM10000_ARP_TAB_SIZE - 1))
    {
        /* the size of the block to allocate is valid, ignore maxIterations */
        if (pNextHopCtrl->arpTabFreeEntryCount > blkLength)
        {
            requiredBlkLength = blkLength;
        }
        else
        {
            err = FM_ERR_NO_MEM;
        }
    }
    else if ((maxIterations > 0) && (maxIterations < FM10000_ARP_PACKING_MAX_ITERATIONS))
    {
        /* only perform a maintenance packing: stop after the given number of
         * iterations or if no more blocks can be moved */
        maxIterationsLocal = maxIterations;
    }

    while ( err == FM_OK && iterCount++ < maxIterationsLocal )
    {
        /* stage 1: defragmentation */
        err = PackArpTableDefragStage(sw, &movedBlocksStage1);

        if (err == FM_OK)
        {
            /* stage 2: packing */
            err = PackArpTablePackingStage(sw, &movedBlocksStage2);
        }

        if (err == FM_OK)
        {
            /* if a valid block size was specified, check if there is
             * enough place for it and stop iterating if so */
            if (blkLength > 0)
            {
                err = ArpFreeBlockDirectLookup(sw, 
                                               requiredBlkLength,
                                               -1,
                                               &dstBlkOffset);
                if ( (err == FM_OK) &&
                     (dstBlkOffset <=
                      (FM10000_ARP_TAB_AVAILABLE_ENTRIES + 1 - requiredBlkLength)) )
                {
                    /* there is enough place for the required block */
                    break;
                }
            }

            totalMovedBlocks += movedBlocksStage1 + movedBlocksStage2;

            if ( (movedBlocksStage1 + movedBlocksStage2) == 0 )
            {
                /* stop if there was not any improvement in the last iteration */
                break;
            }
        }

    }   /* end while (iterCount++ ...) */

    if (err == FM_OK)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "ARP table defragmentation, iterations=%d, Moved blocks=%d\n",
                     iterCount,
                     totalMovedBlocks);

        if ( (requiredBlkLength > 0) &&
             (dstBlkOffset == FM10000_ARP_BLOCK_INVALID_OFFSET) )
        {
            /* a place for a block was required but it was not found */
            err = FM_ERR_ARP_TABLE_FULL;
            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                         "Cannot allocate block of %d entries\n",
                         requiredBlkLength);
        }
        else
        {
            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                         "ARP block of %d may be allocated at offset=%d\n",
                         requiredBlkLength,
                         dstBlkOffset);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end PackArpTable */




/*****************************************************************************/
/** MaintenanceArpTablePacking
 * \ingroup intNextHop
 *
 * \desc            Performs a maintenance, single step pack operation of the
 *                  ARP table. This function is typically called after a block
 *                  has have been released in the ARP table and it looks for
 *                  a block that fits in the created empty block. Maintenance
 *                  packing is only allowed if the table usage is higher than
 *                  FM10000_ARP_MAINTENANCE_PACKING_THRESHOLD. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       startIndex is the intial index of an empty block.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if startIndex is out of range.
 *                  
 *****************************************************************************/
static fm_status MaintenanceArpTablePacking(fm_int  sw,
                                            fm_int  startIndex)
{
    fm_status               err;
    fm10000_switch *        pSwitchExt;
    fm10000_NextHopSysCtrl *pNextHopCtrl;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, startIndex=%d\n",
                  sw,
                  startIndex );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    pNextHopCtrl = pSwitchExt->pNextHopSysCtrl;

    if (startIndex <= 0 || startIndex >= FM10000_ARP_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* maintenance packing conditions */
        if ( pNextHopCtrl->arpTabFreeEntryCount < FM10000_ARP_MAINTENANCE_PACKING_THRESHOLD &&
             startIndex <= FM10000_ARP_TAB_AVAILABLE_ENTRIES )
        {
            err = MoveArpBlockIntoFreeEntries(sw, startIndex, TRUE, NULL);

        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end MaintenanceArpTablePacking */




/*****************************************************************************/
/** GetNewArpBlkHndl
 * \ingroup intNextHop
 *
 * \desc            Returns the next available ARP block handle, if there is
 *                  any or FM10000_ARP_BLOCK_INVALID_HANDLE otherwise.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pArpBlockHndl is a caller allocated storage used to
 *                   return the next available handle.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 * \return          FM_ERR_UNSUPPORTED if feature is not supported/intialized.
 * \return          FM_ERR_NO_MEM if a ressource cannot be allocated
 *                  
 *****************************************************************************/
static fm_status GetNewArpBlkHndl(fm_int     sw,
                                  fm_uint16 *pArpBlockHndl)
{
    fm10000_switch *pSwitchExt;
    fm_status       err;
    fm_uint16       index;
    fm_int          maxLoops;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pArpBlockHndl=%p\n",
                  sw,
                  (void *) pArpBlockHndl );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_ERR_NO_MEM;

    if (pArpBlockHndl == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pSwitchExt->pNextHopSysCtrl == NULL ||
             pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        *pArpBlockHndl = FM10000_ARP_BLOCK_INVALID_HANDLE;

        /* start the search using the last saved position */
        index = pSwitchExt->pNextHopSysCtrl->lastArpBlkCtrlTabLookupNdx;
        maxLoops = FM10000_ARP_BLK_CTRL_TAB_SIZE - 1;

        /* in the ppArpBlkCtrlTab table, look for an entry pointing to NULL */
        while (--maxLoops > 0)
        {
            /* wrap-around the index */
            if (++index >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
            {
                /* index 0 is not allowed */
                index = 1;
            }
            /* stop searching when a NULL pointer is found */
            if ( (*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[index] == NULL )
            {
                /* save the current position for the next search */
                pSwitchExt->pNextHopSysCtrl->lastArpBlkCtrlTabLookupNdx = index;
            
                /* use the current index as the handle */
                *pArpBlockHndl = index;
                err = FM_OK;
                break;
            }
        }   /* end while (--maxLoops > 0) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end GetNewArpBlkHndl */




/*****************************************************************************/
/** SwapArpHandles
 * \ingroup intNextHop
 *
 * \desc            Swaps ARP block handles in such a way that the first
 *                  handle is associtad to the block pointed by the second
 *                  handle and viceversa. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      arpBlockHndl1 is the first handle.
 * 
 * \param[out]      arpBlockHndl2 is the second handle.
 * 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one handle is invalid.
 * \return          FM_ERR_UNSUPPORTED if feature is not supported/intialized.
 *                  
 *****************************************************************************/
static fm_status SwapArpHandles(fm_int     sw,
                                fm_uint16  arpBlockHndl1,
                                fm_uint16  arpBlockHndl2)
{
    fm_status              err;
    fm10000_switch *       pSwitchExt;
    fm10000_ArpBlockCtrl * pArpBlkCtrlAux;
    fm10000_ArpBlockCtrl **ppArpBlkCtrl1;
    fm10000_ArpBlockCtrl **ppArpBlkCtrl2;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, handle1=%d, handle2=%d\n",
                  sw,
                  arpBlockHndl1,
                  arpBlockHndl2 );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);


    if (arpBlockHndl1 == FM10000_ARP_BLOCK_INVALID_HANDLE ||
        arpBlockHndl1 >= FM10000_ARP_BLK_CTRL_TAB_SIZE    ||
        arpBlockHndl2 == FM10000_ARP_BLOCK_INVALID_HANDLE ||
        arpBlockHndl2 >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        ppArpBlkCtrl1 = &((*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlockHndl1]);
        ppArpBlkCtrl2 = &((*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlockHndl2]);

        if ((*ppArpBlkCtrl1 == NULL) || (*ppArpBlkCtrl1 == NULL))
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Null ARP Block control\n");
        }

        /* update the ARP handle table */
        if (err == FM_OK)
        {
            err = FillArpHndlTable(sw,
                                   (*ppArpBlkCtrl1)->offset,
                                   (*ppArpBlkCtrl1)->length,
                                   FM10000_ARP_BLOCK_INVALID_HANDLE);
        }

        if (err == FM_OK)
        {
            err = FillArpHndlTable(sw,
                                   (*ppArpBlkCtrl1)->offset,
                                   (*ppArpBlkCtrl1)->length,
                                   arpBlockHndl2);
        }

        if (err == FM_OK)
        {
            err = FillArpHndlTable(sw,
                                   (*ppArpBlkCtrl2)->offset,
                                   (*ppArpBlkCtrl2)->length,
                                   FM10000_ARP_BLOCK_INVALID_HANDLE);
        }

        if (err == FM_OK)
        {
            err = FillArpHndlTable(sw,
                                   (*ppArpBlkCtrl2)->offset,
                                   (*ppArpBlkCtrl2)->length,
                                   arpBlockHndl1);
        }

        if (err == FM_OK)
        {
            /* swap pointers in the ARP control tab */
            pArpBlkCtrlAux = *ppArpBlkCtrl1;
            *ppArpBlkCtrl1 = *ppArpBlkCtrl2;
            *ppArpBlkCtrl2 = pArpBlkCtrlAux;
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "cannot swap handles: hndl1=%d, off1=%d, len1=%d, hndl2=%d, off2=%d, len2=%d\n",
                         arpBlockHndl1,
                         *ppArpBlkCtrl1 ? (*ppArpBlkCtrl1)->offset : -1,
                         *ppArpBlkCtrl1 ? (*ppArpBlkCtrl1)->length : -1,
                         arpBlockHndl2,
                         *ppArpBlkCtrl2 ? (*ppArpBlkCtrl2)->offset : -1,
                         *ppArpBlkCtrl2 ? (*ppArpBlkCtrl2)->length : -1);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SwapArpHandles */




/*****************************************************************************/
/** FillArpHndlTable
 * \ingroup intNextHop
 *
 * \desc            Fills a block of entries in the ARP handle table with the
 *                  given handle.If blkHandle is a valid handle, entries
 *                  must be free when this function is called, otherwise this
 *                  function will return an error.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       blkOffset is the base offset of the block.
 * 
 * \param[in]       blkLength is the number of entries of the block
 * 
 * \param[in]       blkHandle is the handle to be stored in every entry of
 *                   the block.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 * \return          FM_ERR_UNSUPPORTED if feature is not supported/intialized.
 * \return          FM_ERR_NO_MEM if a ressource cannot be allocated
 *                  
 *****************************************************************************/
static fm_status FillArpHndlTable(fm_int     sw,
                                  fm_uint16  blkOffset,
                                  fm_uint16  blkLength,
                                  fm_uint16  blkHandle)
{
    fm10000_switch *pSwitchExt;
    fm_status       err;
    fm_int          loopCnt;
    fm_uint16 *     pArpHndlTabEntry;
    fm_uint16 *     pArpHndlTabScan;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, blkOffset=%d, blkLength=%d, blkHandle=%d\n",
                  sw,
                  blkOffset,
                  blkLength,
                  blkHandle );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    /* argument validation */
    if ( blkLength == 0                                 ||
        (blkOffset + blkLength) > FM10000_ARP_TAB_SIZE  ||
         blkHandle >= FM10000_ARP_BLK_CTRL_TAB_SIZE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* use a pointer to access the table */
        pArpHndlTabEntry = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[blkOffset];
        pArpHndlTabScan = pArpHndlTabEntry;
        loopCnt = (fm_uint)blkLength;
        
        /* if handle is valid, verify that all entries are free */
        if (blkHandle != FM10000_ARP_BLOCK_INVALID_HANDLE)
        {
            while (loopCnt--)
            {
                if (*pArpHndlTabScan++ != FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    err = FM_ERR_NO_MEM;
                    break;
                }
            }
        }

        if (err == FM_OK)
        {
            pArpHndlTabScan = pArpHndlTabEntry;
            loopCnt = (fm_uint)blkLength;

            /* fill the table with the given handle */
            while (loopCnt--)
            {
                *pArpHndlTabScan++ = blkHandle;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end FillArpHndlTable */




/*****************************************************************************/
/** CleanUpArpTable
 * \ingroup intNextHop
 *
 * \desc            Cleans up the specified range of the ARP table.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       blkOffset is the base offset of the block.
 * 
 * \param[in]       blkLength is the number of entries of the block
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 * \return          FM_ERR_UNSUPPORTED if feature is not supported/intialized.
 * \return          FM_ERR_NO_MEM if a ressource cannot be allocated
 *                  
 *****************************************************************************/
static fm_status CleanUpArpTable(fm_int     sw,
                                 fm_uint16  blkOffset,
                                 fm_uint16  blkLength)
{
    fm_switch *     switchPtr;
    fm10000_switch *pSwitchExt;
    fm_status       err;
    fm_int          index;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, blkOffset=%d, blkLength=%d\n",
                  sw,
                  blkOffset,
                  blkLength );

    err = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);

    /* argument validation */
    if ( blkLength == 0  || (blkOffset + blkLength) > FM10000_ARP_TAB_SIZE )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        for (index = blkOffset; index < blkOffset + blkLength; index++)
        {
            /* errors are ignored here, this is intentional */
            switchPtr->WriteUINT64(sw,
                                   FM10000_ARP_TABLE(index, 0),
                                   0LLU);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CleanUpArpTable */




/*****************************************************************************/
/** UpdateEcmpGroupArpBlockInfo
 * \ingroup intNextHop
 *
 * \desc            Looks for the specified ECMP group and updates the ARP
 *                  table info.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       arpBlkHndl handle of the block that was modified.
 *
 * \param[in]       newBlkOffset is the offset of the new position of the block.
 * 
 * \param[in]       oldBlkOffset is the offset of the old position of the
 *                  ARP block.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 *
 *****************************************************************************/
static fm_status UpdateEcmpGroupArpBlockInfo(fm_int    sw,
                                             fm_uint16 arpBlkHndl,
                                             fm_int    newBlkOffset,
                                             fm_int    oldBlkOffset)
{
    fm_status   err;
    fm_switch * switchPtr;
    fm_int      ecmpGroupId;
    fm_uint16   emcpArpBlkHndl;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, arpBlkHndl=%d, newBlkOffset=%d oldBlkOffset=%d\n",
                  sw,
                  arpBlkHndl,
                  newBlkOffset,
                  oldBlkOffset );

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    if (newBlkOffset < 0                                    ||
        newBlkOffset >= FM10000_ARP_BLK_CTRL_TAB_SIZE       ||
        oldBlkOffset < 0                                    ||
        oldBlkOffset >= FM10000_ARP_BLK_CTRL_TAB_SIZE       ||
        arpBlkHndl == 0                                     ||
        arpBlkHndl >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = FM_ERR_NOT_FOUND;

        if (ValidateArpHandle(sw, arpBlkHndl))
        {
            /* use the opaque as ecmpGroupId */
            ecmpGroupId = (fm_int)GetArpBlockOpaque(sw, arpBlkHndl);

            if (switchPtr->ecmpGroups[ecmpGroupId] != NULL)
            {
                err = GetEcmpGroupArpBlockHandle(sw,
                                                  switchPtr->ecmpGroups[ecmpGroupId],
                                                  &emcpArpBlkHndl);

                if (err == FM_OK && arpBlkHndl == emcpArpBlkHndl)
                {
                    /* notify ECMP group clients and update nexthop information */
                    err = fm10000NotifyEcmpGroupChange(sw, ecmpGroupId, oldBlkOffset);

                }
            }

        }   /* end if (ValidateArpHandle(sw, arpBlockHandle)) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateEcmpGroupArpBlockInfo */




/*****************************************************************************/
/** UpdateNextHopData
 * \ingroup intNextHop
 *
 * \desc            Fills in the arpData field to point to a specified
 *                  logical port/glort.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pEcmpGroup points to the high level ECMP group structure.
 *
 * \param[in]       pNextHop points to the internal next-hop record to be
 *                  updated. It may be NULL if it is a multicast group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 * \return          FM_ERR_UNSUPPORTED if used for multicasts
 *
 *****************************************************************************/
static fm_status UpdateNextHopData(fm_int           sw,
                                   fm_intEcmpGroup *pEcmpGroup,
                                   fm_intNextHop *  pNextHop)
{
    fm_status        err;
    fm10000_NextHop *pNextHopExt;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, pNextHop=%p\n",
                  sw,
                  (void *) pEcmpGroup,
                  (void *) pNextHop );

    err  = FM_OK;

    /* arg validation */
    if (pEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pNextHop == NULL) 
    {
        if (pEcmpGroup->mcastGroup == NULL)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            /* Multicast not supported. */
            err = FM_ERR_UNSUPPORTED;
        }
    }
    else
    {
        if ( (pNextHopExt = pNextHop->extension) == NULL )
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            /* Clear hardware specific next-hop field */
            FM_CLEAR(pNextHopExt->arpData);

            /* Populate the next-hop field using the provided next-hop data. */
            switch (pNextHop->nextHop.type)
            {
                case FM_NEXTHOP_TYPE_ARP:
                    if (pEcmpGroup->mcastGroup == NULL)
                    {
                        err = BuildNextHopArp(sw, pNextHopExt);
                    }
                    break;

                case FM_NEXTHOP_TYPE_DROP:
                    err =
                        BuildGlortArpData(sw,
                                          FM_PORT_DROP,
                                          0,
                                          0,
                                          pNextHopExt->arpData);
                    break;

                case FM_NEXTHOP_TYPE_DMAC:

                    break;

                case FM_NEXTHOP_TYPE_TUNNEL:
                    err = BuildNextHopTunnel(sw, pNextHopExt);
                    break;

                case FM_NEXTHOP_TYPE_VN_TUNNEL:
                    err = BuildNextHopVNTunnel(sw, pNextHopExt);
                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    break;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateNextHopData */




/*****************************************************************************/
/** SetEcmpGroupType
 * \ingroup intNextHop
 *
 * \desc            Determines and set the type of the ECMP group. The
 *                  possible type are enumerated by ''fm10000_EcmpGroupType''
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pEcmpGroupExt pointer to the chip level ECMP group
 *                  extension
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT
 *                  
 *****************************************************************************/
static fm_status SetEcmpGroupType(fm_int             sw,
                                  fm10000_EcmpGroup *pEcmpGroupExt)
{
    fm_status        err;
    fm_intEcmpGroup *pParentEcmpGroup;
 

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroupExt=%p\n",
                  sw,
                  (void *) pEcmpGroupExt );

    err = FM_OK;

    if (pEcmpGroupExt == NULL || pEcmpGroupExt->pParent == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pParentEcmpGroup = pEcmpGroupExt->pParent;

        if ( pParentEcmpGroup->mcastGroup == (fm_intMulticastGroup *) ~0 )
        {
            pEcmpGroupExt->groupType = FM10000_ECMP_GROUP_TYPE_DROP_MCAST;
        }
        else if (pParentEcmpGroup->mcastGroup != NULL)
        {
            pEcmpGroupExt->groupType = FM10000_ECMP_GROUP_TYPE_NORMAL_MCAST;
        }
        else
        {
            pEcmpGroupExt->groupType = FM10000_ECMP_GROUP_TYPE_NORMAL_UNICAST;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetEcmpGroupType */




/*****************************************************************************/
/** AllocateEcmpGroupSpecResources
 * \ingroup intNextHop
 *
 * \desc            Allocs specific resources required by ECMP groups
 *                  supporting multicast groups.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pEcmpGroupExt pointer to the chip level ECMP group
 *                  extension
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *                  
 *****************************************************************************/
static fm_status AllocateEcmpGroupSpecResources(fm_int             sw,
                                          fm10000_EcmpGroup *pEcmpGroupExt)
{
    fm_intEcmpGroup *group;
    fm_status        status;

    group = pEcmpGroupExt->pParent;
    /* If this ECMP Group is supporting a "drop" multicast group,
     * initialize it appropriately. */
    if ( group->mcastGroup == (fm_intMulticastGroup *) ~0 )
    {
        status = BuildGlortArp(sw, pEcmpGroupExt, FM_PORT_DROP);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        status = AllocateArpForEcmpGroup(sw, group);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    }
    else if (group->mcastGroup != NULL)
    {
        status = BuildGlortArp(sw, pEcmpGroupExt, 0);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        status = AllocateArpForEcmpGroup(sw, group);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }
    /* Otherwise, this must be a normal ECMP group */
    else
    {
        status = FM_OK;
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end AllocateEcmpGroupSpecResources */




/*****************************************************************************/
/** CheckRequiredNextHopExtensions
 * \ingroup intNextHop
 *
 * \desc            Determines the number of low level nextHop extensions to
 *                  be added.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pParentEcmpGroup pointer to parent (high level) ECMP group
 *                  structure
 * 
 * \param[out]      pNextHopToAdd pointer to a caller allocated storage
 *                  where this function will return the number of nextHop
 *                  extensions to be added. 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 *                  
 *****************************************************************************/
static fm_status CheckRequiredNextHopExtensions(fm_int           sw,
                                                fm_intEcmpGroup *pParentEcmpGroup,
                                                fm_int *         pNextHopToAdd)
{
    fm_status   err;
    fm_int      index;
    fm_int      requiredExtensions;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pParentEcmpGroup=%p, pNextHopToAdd=%p\n",
                  sw,
                  (void *) pParentEcmpGroup,
                  (void *) pNextHopToAdd );

    err = FM_OK;
    requiredExtensions = 0;

    /* argument validation */
    if (pParentEcmpGroup == NULL  || pNextHopToAdd == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        index = pParentEcmpGroup->maxNextHops - 1;

        while (index >= 0)
        {
            /* count parent nextHop structures that do not have extensions */
            if (pParentEcmpGroup->nextHops[index] != NULL &&
                (pParentEcmpGroup->nextHops[index])->extension == NULL)
            {
                requiredExtensions++;
            }
            index--;
        }
        *pNextHopToAdd = requiredExtensions;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CheckRequiredNextHopExtensions */




/*****************************************************************************/
/** InitFixedSizeEcmpGroupArpData
 * \ingroup intNextHop
 *
 * \desc            Initilizes ARP data for a fixed size ECMP group.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pParentEcmpGroup pointer to parent (high level) ECMP group
 *                  structure
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 *                  
 *****************************************************************************/
static fm_status InitFixedSizeEcmpGroupArpData(fm_int           sw,
                                               fm_intEcmpGroup *pParentEcmpGroup)
{
    fm_status        err;
    fm_int           index;
    fm10000_NextHop *pArpNextHopExt;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pParentEcmpGroup=%p\n",
                  sw,
                  (void *) pParentEcmpGroup );

    err = FM_OK;

    /* argument validation */
    if (pParentEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* initialize fm10000 nexthop extensions */
        for (index = 0; index < pParentEcmpGroup->maxNextHops ; index++)
        {
            if (pParentEcmpGroup->nextHops[index] != NULL)
            {
                if ( (pArpNextHopExt= (pParentEcmpGroup->nextHops[index])->extension) != NULL )
                {
                    err =
                        BuildGlortArpData(sw,
                                          FM_PORT_DROP,
                                          0,
                                          0,
                                          pArpNextHopExt->arpData);
                }
            }
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot init ARP for fixedSize ECMP, groupId=%d, "
                             "nextHop index=%d\n",
                             pParentEcmpGroup->groupId,
                             index);
                break;
            }

        }   /* end for (index = 0...) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end InitFixedSizeEcmpGroupArpData */




/*****************************************************************************/
/** ReleaseNextHopExtensions
 * \ingroup intNextHop
 *
 * \desc            Releases the low level Next Hop extensions specify by
 *                  the "nextHops to remove" table. The number of table entries
 *                  is given by nextHopTableSize. 
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pParentEcmpGroup pointer to parent (high level) ECMP group
 *                  structure.
 * 
 * \param[in]       nextHopTableSize is the size of the table of nextHop
 *                  extensions to remove.
 *
 * \param[out]      pIndexNextHopToRemoveTab is a pointer to a table containing
 *                  the indexes of the next hops whose extensions must be
 *                  removed.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 * \return          FM_ERR_NO_MEM if a nextHop extension could not be allocated
 *                  
 *****************************************************************************/
static fm_status ReleaseNextHopExtensions(fm_int           sw,
                                          fm_intEcmpGroup *pParentEcmpGroup,
                                          fm_int           nextHopTableSize,
                                          fm_int *         pIndexNextHopToRemoveTab)
{
    fm_status   err;
    fm_int      index;
    fm_int      removeTabIndex;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pParentEcmpGroup=%p, nextHopTableSize=%d, "
                  "pIndexNextHopToRemoveTab=%p\n",
                  sw,
                  (void *) pParentEcmpGroup,
                  nextHopTableSize,
                  (void *) pIndexNextHopToRemoveTab );

    err = FM_OK;

    /* argument validation */
    if (pParentEcmpGroup == NULL ||
        nextHopTableSize == 0    ||
        pIndexNextHopToRemoveTab == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        for (removeTabIndex = 0; removeTabIndex < nextHopTableSize; removeTabIndex++)
        {
            index = pIndexNextHopToRemoveTab[removeTabIndex];
            if (index != FM_NEXTHOP_INDEX_UNSPECIFIED)
            {
                if (pParentEcmpGroup->nextHops[index] != NULL &&
                    (pParentEcmpGroup->nextHops[index])->extension != NULL)
                {
                    fmFree((pParentEcmpGroup->nextHops[index])->extension);
                    (pParentEcmpGroup->nextHops[index])->extension = NULL;
                }
            }
        }   /* end for (removeTabIndex = 0; removeTabIndex < ...) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end ReleaseNextHopExtensions */




/*****************************************************************************/
/** ReleaseAllNextHopExtensions
 * \ingroup intNextHop
 *
 * \desc            Removes all low level nextHop extensions for the given
 *                  ECMP group.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pParentEcmpGroup pointer to parent (high level) ECMP group
 *                  structure.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 * \return          FM_ERR_NO_MEM if a nextHop extension could not be allocated
 *                  
 *****************************************************************************/
static fm_status ReleaseAllNextHopExtensions(fm_int           sw,
                                             fm_intEcmpGroup *pParentEcmpGroup)
{
    fm_status   err;
    fm_int      index;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pParentEcmpGroup=%p\n",
                  sw,
                  (void *) pParentEcmpGroup );

    err = FM_OK;

    /* argument validation */
    if (pParentEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        index = pParentEcmpGroup->maxNextHops;

        while (--index >= 0 && err == FM_OK)
        {

            if (pParentEcmpGroup->nextHops[index] != NULL &&
                (pParentEcmpGroup->nextHops[index])->extension != NULL)
            {
                fmFree((pParentEcmpGroup->nextHops[index])->extension);
                (pParentEcmpGroup->nextHops[index])->extension = NULL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end ReleaseAllNextHopExtensions */




/*****************************************************************************/
/** SetNextHopArpIndexes
 * \ingroup intNextHop
 *
 * \desc            Set the arp indexes for next hops entries.
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pParentEcmpGroup pointer to parent (high level) ECMP group
 *                  structure
 * 
 * \param[in]       arpBlkHndl is the handle of the ARP block allocated for
 *                  the specified ECMP group.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if there was an error in one or
 *                  more arguments
 *                  
 *****************************************************************************/
static fm_status SetNextHopArpIndexes(fm_int           sw,
                                      fm_intEcmpGroup *pParentEcmpGroup,
                                      fm_uint16        arpBlkHndl)
{
    fm_status        err;
    fm_int           index;
    fm_uint16        arpIndex;
    fm_uint16        blkLength;
    fm10000_NextHop *pArpNextHopExt;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pParentEcmpGroup=%p, arpBlkHndl=%d\n",
                  sw,
                  (void *) pParentEcmpGroup,
                  arpBlkHndl );

    err = FM_OK;

    /* argument validation */
    if (pParentEcmpGroup == NULL ||
        arpBlkHndl == 0         ||
        arpBlkHndl >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        blkLength = GetArpBlockLength(sw, arpBlkHndl);
        arpIndex  = 0;

        for (index = 0; index < pParentEcmpGroup->maxNextHops; index++)
        {
            if (pParentEcmpGroup->nextHops[index] != NULL &&
                (pParentEcmpGroup->nextHops[index])->extension != NULL)
            {
                pArpNextHopExt = (fm10000_NextHop*)((pParentEcmpGroup->nextHops[index])->extension);

                pArpNextHopExt->arpBlkRelOffset = arpIndex;

                if (++arpIndex >= blkLength)
                {
                    break;
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetNextHopArpIndexes */




/*****************************************************************************/
/** AllocateArpForEcmpGroup
 * \ingroup intRoute
 *
 * \desc            Allocates a Next Hop entry for a multicast, drop,
 *                  or RPF Failure ECMP group.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group points to the ECMP Group's information.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status AllocateArpForEcmpGroup(fm_int sw, fm_intEcmpGroup *group)
{
    fm_switch         *switchPtr;
    fm10000_switch    *switchExt;
    fm10000_EcmpGroup *ext;
    fm_status          err;
    fm_status          localErr;
    fm_uint16          arpBlkHndl;
    fm_uint16          groupBaseOffset;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, group = %p(%d)\n",
                  sw,
                  (void *) group,
                  group->groupId );

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);
    ext       = group->extension;

    /* alloc an ARP block of 1 entry
     *  this step is performed in first place because the ARP
     *  table is the most limited resource used by the ECMP */

    err = fm10000RequestArpBlock(sw,
                                 FM10000_ARP_CLIENT_ECMP,
                                 1,
                                 FM10000_ARP_BLOCK_OPT_NONE,
                                 &arpBlkHndl);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* get the base offset of the block */
    groupBaseOffset = GetArpBlockOffset(sw, arpBlkHndl);

    if ((groupBaseOffset <= 0) &&
        (groupBaseOffset >= FM10000_ARP_TAB_SIZE))
    {
        err = FM_ERR_INVALID_INDEX;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Write the register */
    err = switchPtr->WriteUINT64(sw,
                                 FM10000_ARP_TABLE(groupBaseOffset, 0),
                                 ext->glortArpData);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    
    /* write the ARP table */
    err = SetEcmpGroupArpBlockHandle(sw, group, arpBlkHndl);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    ext->activeArpCount = 1;
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

ABORT:

    /* release the ARP block */
    localErr = fm10000FreeArpBlock(sw,
                                   FM10000_ARP_CLIENT_ECMP,
                                   arpBlkHndl);
    
    if (localErr != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "Error %d Cannot release ARP block, handle=%d\n",
                     arpBlkHndl,
                     localErr);
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocateArpForEcmpGroup */




/*****************************************************************************/
/** AllocateFixedSizeEcmpGroupResources
 * \ingroup intNextHop
 *
 * \desc            Allocs resources for "fixed size" ECMP groups. The size
 *                  of the block is validated and it must be one of these:
 *                  [1..16, 32, 64, 128, 256, 512, 1024, 2048, 4096]
 * 
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pEcmpGroupExt pointer to the chip level ECMP group
 *                  extension
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if specified ECMP group is
 *                  not valid.
 * \return          FM_ERR_INVALID_VALUE if the number of next-hop entries is
 *                  invalid for a fixed size ECMP group. Valid values for fixed
 *                  size ECMP groups are: (1..16,32,64,128,256,512,1024,2048,
 *                  4096).
 *
 *****************************************************************************/
static 
fm_status AllocateFixedSizeEcmpGroupResources(fm_int             sw,
                                               fm10000_EcmpGroup *pEcmpGroupExt)
{ 
    fm_status        err;
    fm_status        localErr;
    fm_int           blockSize;
    fm_uint16        arpHndlTmp;
    fm_intEcmpGroup *pParentEcmpGroup;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroupExt=%p\n",
                  sw,
                  (void *) pEcmpGroupExt );

    /* argument validation */
    if (pEcmpGroupExt == NULL || pEcmpGroupExt->pParent == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pParentEcmpGroup = pEcmpGroupExt->pParent;
        blockSize = pParentEcmpGroup->numFixedEntries;

        /* check if the number of nexthops is valid: the number of nextHops of 
         * a "fixed size" ECMP group must be equal to ECMP modulos supported by 
         * fm10000:
         * [1...16, 32, 64, 128, 256, 512, 1024, 2048 or 4096]
         * fm10000CheckValidArpBlockSize() return an error if the block size is not 
         * included in the previous list. */
        err = fm10000CheckValidArpBlockSize(pParentEcmpGroup->maxNextHops);

        if ( (err != FM_OK) || (blockSize != pParentEcmpGroup->maxNextHops) )
        {
            err = FM_ERR_INVALID_VALUE;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Invalid ARP block size=%d\n",
                         pParentEcmpGroup->maxNextHops);
        }
        else
        {
            /* alloc an ARP block of entries
             *  this step is performed in first place because the ARP
             *  table is the most limited ressourse used by the ECMP */
            err = fm10000RequestArpBlock(sw,
                                         FM10000_ARP_CLIENT_ECMP,
                                         pParentEcmpGroup->maxNextHops,
                                         FM10000_ARP_BLOCK_OPT_NONE,
                                         &arpHndlTmp);
        }

        if (err == FM_OK)
        {
            /* allocate all fm10000 nexthop extensions. It is not necessary
             *  to create an undo table */
            err = AllocNextHopExtensions(sw,
                                         pParentEcmpGroup,
                                         arpHndlTmp,
                                         0,
                                         NULL);
            if (err == FM_OK)
            {
                err = InitFixedSizeEcmpGroupArpData(sw, pParentEcmpGroup);

                if (err == FM_OK)
                {
                    /* set arp indexes for the modified group */
                    err = SetNextHopArpIndexes(sw, pParentEcmpGroup, arpHndlTmp);
                }

                if (err == FM_OK)
                {
                    /* write the ARP table */
                    err = SetupEcmpGroupArpEntries(sw, pParentEcmpGroup, arpHndlTmp, FALSE);
                }

                if (err == FM_OK)
                {
                    err = SetEcmpGroupArpBlockHandle(sw, pParentEcmpGroup, arpHndlTmp);
                }
            }

            /* if there was an error release the allocated resources */
            if (err != FM_OK)
            {
                /* release the structures */
                localErr = ReleaseAllNextHopExtensions(sw, pParentEcmpGroup);
                if (localErr != FM_OK)
                {
                    /* just log the error, do not abort */
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                 "Cannot release NextHop extensions, ecmpGrouId=%d\n",
                                 pParentEcmpGroup->groupId);
                }

                /* release the ARP block */
                localErr = fm10000FreeArpBlock(sw,
                                               FM10000_ARP_CLIENT_ECMP,
                                               arpHndlTmp);

                if (localErr != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "Cannot release ARP block, handle=%d\n",
                                 arpHndlTmp);
                }
            }   /* end if (err != FM_OK) */
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocateFixedSizeEcmpGroupResources */




/*****************************************************************************/
/** SetupEcmpGroupArpEntries
 * \ingroup intNextHop
 *
 * \desc            Setups a block of ARP entries for an ECMP group. This
 *                  function updates the required information in the nextHop
 *                  structures; then it write the ARP entries with the
 *                  appropiated information and then notifies the ECMP users
 *                  about the changes.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pEcmpGroup is a pointer to the high level (parent) ECMP
 *                  group structure.
 * 
 * \param[in]       arpBlkHndl is the handle of the ARP block to be used.
 * 
 * \param[in]       updateNextHopData TRUE if nextHop data must be updated.
 * 
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT
 *                  
 *****************************************************************************/
static fm_status SetupEcmpGroupArpEntries(fm_int           sw,
                                          fm_intEcmpGroup *pEcmpGroup,
                                          fm_uint16        arpBlkHndl,
                                          fm_bool          updateNextHopData)
{
    fm_status        err;
    fm_switch       *switchPtr;
    fm_int           hopIndex;
    fm_int           arpIndex;
    fm_intNextHop   *pNextHop;
    fm10000_NextHop *pNextHopExt;
    fm_uint16        groupBaseOffset;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, arpBlkHndl=%d updateNextHopData=%s\n",
                  sw,
                  (void *) pEcmpGroup,
                  arpBlkHndl,
                  updateNextHopData? "TRUE" : "FALSE" );

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    if (pEcmpGroup == NULL ||
        arpBlkHndl >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = SetEcmpGroupArpBlockHandle(sw, pEcmpGroup, arpBlkHndl);

        /* get the base offset of the block */
        groupBaseOffset = GetArpBlockOffset(sw, arpBlkHndl);

        if (groupBaseOffset > 0 &&
            groupBaseOffset < FM10000_ARP_TAB_SIZE)
        {
            /* for each active NextHop, determine the absolute offset and
             * write the date into the ARP table */
            for (hopIndex = 0; hopIndex < pEcmpGroup->nextHopCount; hopIndex++)
            {
                pNextHop = pEcmpGroup->nextHops[hopIndex];
                if (pNextHop  != NULL)
                {
                    pNextHopExt = pNextHop->extension;
                    if (pNextHopExt != NULL)
                    {
                        if (updateNextHopData)
                        {
                            err = UpdateNextHopData(sw, pEcmpGroup, pNextHop);
                        }
                
                        if (err == FM_OK)
                        {
                            /* calcule NextHop absolute offset */
                            arpIndex = groupBaseOffset + pNextHopExt->arpBlkRelOffset;
                            pNextHopExt->arpBlkHndl = arpBlkHndl;

                            /* validate that the absolute index is OK */
                            if (arpIndex > 0 &&
                                arpIndex < FM10000_ARP_TAB_SIZE)
                            {
                                err = switchPtr->WriteUINT64(sw,
                                                             FM10000_ARP_TABLE(arpIndex,0),
                                                             pNextHopExt->arpData[0]);
                            }
                            else
                            {
                                /* log the error and continue */
                                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                             "Invalid ARP index: EcmpGroupId=%d, hopIndex=%d, baseIndex=%d, relativeIndex=%d\n",
                                             pEcmpGroup->groupId,
                                             hopIndex,
                                             groupBaseOffset,
                                             pNextHopExt->arpBlkRelOffset);
                            }
                        }
                    }
                }   /* end if (pNextHop != NULL) */

            }   /* end for (hopIndex = 0; ...) */

        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "invalid block offset, handle=%d, offset=%d\n",
                         arpBlkHndl,
                         groupBaseOffset);
            err = FM_FAIL;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetupEcmpGroupArpEntries */




/*****************************************************************************/
/** CheckArpBlockAvailability
 * \ingroup intNextHop
 *
 * \desc            Does a preliminary check if an ARP block can be allocated.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       arpCount is the number of consecutive entries that is
 *                  being requested.
 *
 * \return          FAIL if ARP block should not be allocated
 * 
 * \return          TRUE if ARP block can be allocated
 *
 *****************************************************************************/
static fm_bool CheckArpBlockAvailability(fm_int sw, fm_int arpCount)
{
    fm10000_switch *pSwitchExt;

    pSwitchExt = GET_SWITCH_EXT(sw);
    if ( pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount <
         (arpCount + FM10000_ARP_MAINTENANCE_NOF_RESERVED_ENTRIES) )
    {
        return FALSE;
    }
    return TRUE;

} /* end CheckArpBlockAvailability */




/*****************************************************************************/
/** DeleteUnresolvedNextHopRedirectTrigger
 * \ingroup intRouterBase
 *
 * \desc            Delete unresolved NH trigger
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteUnresolvedNextHopRedirectTrigger(fm_int sw)
{
    fm_status           err;
    fm10000_switch     *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw %d\n", sw);

    /* Initialize return code */
    err = FM_OK;

    /* Get switch ext pointer */
    switchExt = GET_SWITCH_EXT(sw);

    /* Delete unresolved NH trigger */
    err = fm10000DeleteTrigger(sw,
                               FM10000_TRIGGER_GROUP_ROUTING,
                               FM10000_TRIGGER_RULE_ROUTING_UNRESOLVED_NEXT_HOP,
                               TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}/* end DeleteUnresolvedNextHopRedirectTrigger */




/*****************************************************************************/
/** CreateUnresolvedNextHopRedirectTrigger
 * \ingroup intRouterBase
 *
 * \desc            Create redirect trigger for unresolved next hop 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateUnresolvedNextHopRedirectTrigger(fm_int sw)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm10000_switch     *switchExt;
    fm_switch          *switchPtr;
    fm_status           err;
    fm_char             trigName[32];
    fm_int              ruleNr;
    fm_uint32           glort;
    fm_int              trapCode;
    fm_bool             triggerAllocated;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw %d\n", sw);

    /* Initialize return code */
    err = FM_OK;
    ruleNr = FM10000_TRIGGER_RULE_ROUTING_UNRESOLVED_NEXT_HOP;
    triggerAllocated = FALSE;

    /* Get switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Create the trigger's name */
    FM_SPRINTF_S(trigName,
                 sizeof(trigName),
                 "routingUnresolvedNextHopT%d",
                 ruleNr);
    
    /* Create the trigger */
    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_ROUTING,
                               ruleNr,
                               TRUE,
                               trigName);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    triggerAllocated = TRUE;

    /* Initialize trigger's actions */
    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Set trigger's action as forwarding redirect */
    trigAction.cfg.forwardingAction = FM_TRIGGER_FORWARDING_ACTION_REDIRECT;
    /* Set trigger's action params: (note that glort stays unchanged) */
    /*  1. redirect to CPU */
    trigAction.param.newDestPortset = FM_PORT_SET_CPU; 
    /*  2. prevent link aggregation filtering  */
    trigAction.param.filterDestMask = 0;

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_ROUTING,
                                  ruleNr,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Initialize trigger's conditions */
    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Set trigger's conditions: */
    /*  1. match to dst glort unconditionally */
    trigCond.cfg.matchDestGlort = FM_TRIGGER_MATCHCASE_MATCHIFEQUAL;
    /*  2. match to all rx ports  */
    trigCond.cfg.rxPortset = FM_PORT_SET_ALL;    
    /* Set trigger's conditions params:  */

    /* Get glort for CPU port */
    err = fmGetLogicalPortGlort(sw, switchPtr->cpuPort, &glort);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Set dest glort and its mask as concatenation of CPU glort and 
       NO_ARP trap code. Clear the lowest bit of trap code to  match 
       both NO_ARP trap codes (then only one trigger is needed) */

    err = fm10000GetSwitchTrapCode(sw, 
                                   FM_TRAPCODE_L3_ROUTED_NO_ARP_0,
                                   &trapCode);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    trigCond.param.destGlort = glort | (trapCode & 0xFF);
    trigCond.param.destGlortMask = 0xFFFE;

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_ROUTING,
                                     ruleNr,
                                     &trigCond,
                                     TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

ABORT:
    if( (err != FM_OK) && (triggerAllocated == TRUE) )
        DeleteUnresolvedNextHopRedirectTrigger(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end CreateUnresolvedNextHopRedirectTrigger */




/*****************************************************************************/
/** BuildGlortArpData
 * \ingroup intNextHop
 *
 * \desc            Fills in the arpData field to point to a specified
 *                  logical port/glort.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       logicalPort is the logical port number.
 *
 * \param[in]       mtuIndex a 3-bit MTU index.
 *
 * \param[in]       markRouted is set when frames are to be routed to the glort
 *
 * \param[out]      pArpData points to a caller-provided array into which
 *                  the ARP data will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status BuildGlortArpData(fm_int     sw,
                                   fm_int     logicalPort,
                                   fm_byte    mtuIndex,
                                   fm_bool    markRouted,
                                   fm_uint64 *pArpData)
{
    fm_switch *switchPtr;
    fm_uint32  glort;
    fm_status  err;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, logicalPort=%d, pArpData=%p\n",
                  sw,
                  logicalPort,
                  (void *) pArpData );

    switchPtr = GET_SWITCH_PTR(sw);

    /* Pre-initialize the ARP fields */
    pArpData[0] = 0;

    if (logicalPort == -1)
    {
        logicalPort = switchPtr->cpuPort;
    }

    err = fmGetLogicalPortGlort(sw, logicalPort, &glort);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    }

    FM_ARRAY_SET_FIELD( (fm_uint32 *) pArpData,
                        FM10000_ARP_ENTRY_GLORT,
                        DGLORT,
                        glort );

    FM_ARRAY_SET_FIELD( (fm_uint32 *) pArpData,
                        FM10000_ARP_ENTRY_GLORT,
                        MTU_Index,
                        mtuIndex );
    if (markRouted)
    {
        FM_ARRAY_SET_BIT( (fm_uint32 *)pArpData,
                          FM10000_ARP_ENTRY_GLORT,
                          markRouted,
                          1 );
    }

    /* Physical router. */
    FM_ARRAY_SET_FIELD( (fm_uint32 *) pArpData,
                        FM10000_ARP_ENTRY_GLORT,
                        RouterIdGlort,
                        1 );

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end BuildGlortArpData */




/*****************************************************************************/
/** BuildGlortArp
 * \ingroup intNextHop
 *
 * \desc            Fills in the arpData fields for an ECMP group
 *                  to point to a specified logical port/glort.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pEcmpGroup points to the ECMP group.
 *
 * \param[in]       logicalPort is the logical port number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status BuildGlortArp(fm_int             sw,
                               fm10000_EcmpGroup *pEcmpGroup,
                               fm_int             logicalPort)
{
    fm_switch *                   switchPtr;
    fm_status                     err;
    fm_byte                       mtuIndex;
    struct _fm_intMulticastGroup *mcastGroup;
    fm_bool                       markRouted;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=0x%p(%d), logicalPort =%d\n",
                  sw,
                  (void *) pEcmpGroup,
                  pEcmpGroup->pParent ? pEcmpGroup->pParent->groupId : -1,
                  logicalPort );

    switchPtr  = GET_SWITCH_PTR(sw);
    mtuIndex   = 0;
    markRouted = FALSE;
    
    if ( (pEcmpGroup->pParent) && (pEcmpGroup->pParent->mcastGroup) &&
         (pEcmpGroup->pParent->mcastGroup != (fm_intMulticastGroup *) ~0) )
    {
        mcastGroup = pEcmpGroup->pParent->mcastGroup;
        mtuIndex = mcastGroup->mtuIndex;
        markRouted = !mcastGroup->l3SwitchingOnly;

        if (mcastGroup->fwdToCpu)
        {
            logicalPort = switchPtr->cpuPort;
        }
        else
        {
            logicalPort = mcastGroup->logicalPort;
        }
        
    }

    err =
        BuildGlortArpData(sw,
                          logicalPort,
                          mtuIndex,
                          markRouted,
                          &pEcmpGroup->glortArpData);

    if (err == FM_OK)
    {
        pEcmpGroup->useGlortArpData = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end BuildGlortArp */




/*****************************************************************************/
/** BuildNextHopArp
 * \ingroup intNextHop
 *
 * \desc            Fills in the arpData fields for a given next-hop.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pNextHop points to the fm10000 next-hop record.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if argument is invalid
 * \return          FM_ERR_INVALID_VRID if invalid Router ID
 *
 *****************************************************************************/
static fm_status BuildNextHopArp(fm_int           sw,
                                 fm10000_NextHop *pNextHop)
{
    fm_switch *     switchPtr;
    fm_status       err;
    fm_uint32       glort;
    fm_intArpEntry *pArpEntry;
    fm_uint16       vlan;
    fm_nextHop     *pArpNextHop;
    fm_int          vroff;
    fm_int          routerId;
    fm_int          trapCode;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pNextHop=0x%p\n",
                  sw,
                  (void *) pNextHop );

    switchPtr = GET_SWITCH_PTR(sw);
    err       = FM_OK;
    vlan      = FM_INVALID_VLAN;

    if (pNextHop == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pNextHop->pParent->nextHop.type != FM_NEXTHOP_TYPE_ARP)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pArpNextHop = &pNextHop->pParent->nextHop.data.arp;
        pArpEntry   = pNextHop->pParent->arp;

        pNextHop->arpData[0] = 0;
        pNextHop->arpData[1] = 0;

        if ( (vlan = pNextHop->pParent->vlan) == FM_INVALID_VLAN )
        {
            vlan = 0;
        }
        
        if (pArpEntry != NULL)
        {
            /* If we have an ARP entry, build a MAC-based ARP entry */
          
            if (pArpEntry->vrid == FM_ROUTER_ANY)
            {
                routerId = FM10000_ROUTER_ID_NO_REPLACEMENT;
            }
            else
            {
                vroff = fmGetVirtualRouterOffset(sw, pArpEntry->vrid);
                if (vroff < 0)
                {
                    err = FM_ERR_INVALID_VRID;
                }
                routerId = vroff + 1;
            }

            if (err == FM_OK)
            {
                FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                    FM10000_ARP_ENTRY_DMAC,
                                    EVID,
                                    vlan );
            }

            /* Is this ARP an IPv6 Stateless Autoconfiguration entry? */
            if ( (err == FM_OK) &&
                 (pArpEntry->arp.macAddr & FM_LITERAL_64(0x0000FFFFFFFFFFFF)) ==
                 FM_MAC_STATELESS_AUTOCONFIG )
            {
                FM_ARRAY_SET_BIT( (fm_uint32 *)pNextHop->arpData,
                                   FM10000_ARP_ENTRY_GLORT,
                                   IPv6Entry,
                                   1 );

                FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                    FM10000_ARP_ENTRY_GLORT,
                                    RouterIdGlort,
                                    routerId );

            }
            else if (err == FM_OK)
            {
                FM_ARRAY_SET_FIELD64( (fm_uint32 *)pNextHop->arpData,
                                      FM10000_ARP_ENTRY_DMAC,
                                      DMAC,
                                      pArpEntry->arp.macAddr );
            
                FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                    FM10000_ARP_ENTRY_DMAC,
                                    RouterId,
                                    routerId );
            }
        }
        else
        {
            /* this is a GLORT-based ARP entry */
            err = fmGetLogicalPortGlort(sw, switchPtr->cpuPort, &glort);

            /* Set trap code in for unresolved ARP entry*/
            if(err == FM_OK)
            {
                FM_LOG_ASSERT(FM_LOG_CAT_ROUTING,
                              (pArpNextHop->trapCode == FM_TRAPCODE_L3_ROUTED_NO_ARP_0)
                              || (pArpNextHop->trapCode == FM_TRAPCODE_L3_ROUTED_NO_ARP_1),
                              "Trap code has invalid value(%d), "
                              "should be related to unresolved ARP entry\n",
                              pArpNextHop->trapCode);

                err = fm10000GetSwitchTrapCode(sw, 
                                               pArpNextHop->trapCode,
                                               &trapCode);

                if(err == FM_OK)
                    glort |= (trapCode & 0xFF);
           
            }

            if (err == FM_OK)
            {
                FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                    FM10000_ARP_ENTRY_GLORT,
                                    DGLORT,
                                    glort );

                FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                    FM10000_ARP_ENTRY_GLORT,
                                    RouterIdGlort,
                                    FM10000_ROUTER_ID_NO_REPLACEMENT );

                FM_ARRAY_SET_BIT( (fm_uint32 *)pNextHop->arpData,
                                  FM10000_ARP_ENTRY_GLORT,
                                  markRouted,
                                  0 );

            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end BuildNextHopArp */




/*****************************************************************************/
/** BuildNextHopTunnel
 * \ingroup intNextHop
 *
 * \desc            Fills in the arpData fields for a given tunnel next-hop.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pNextHop points to the fm10000 next-hop record.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if argument is invalid
 *
 *****************************************************************************/
static fm_status BuildNextHopTunnel(fm_int           sw,
                                    fm10000_NextHop *pNextHop)
{
    fm_status          err;
    fm_tunnelNextHop * tunnel;
    fm_tunnelGlortUser glortUser;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pNextHop=0x%p\n",
                  sw,
                  (void *) pNextHop );

    err = FM_OK;

    if (pNextHop == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pNextHop->pParent->nextHop.type != FM_NEXTHOP_TYPE_TUNNEL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        tunnel = &pNextHop->pParent->nextHop.data.tunnel;

        pNextHop->arpData[0] = 0;
        pNextHop->arpData[1] = 0;

        err = fm10000GetTunnelAttribute(sw,
                                        tunnel->tunnelGrp,
                                        tunnel->tunnelRule,
                                        FM_TUNNEL_GLORT_USER,
                                        &glortUser);
        if (err == FM_OK)
        {
            /* Only accept rule redirection that don't make uses of the user
             * field. */
            if ( (glortUser.userMask != 0) ||
                 (glortUser.glortMask != 0xFFFF) )
            {
                err = FM_ERR_UNSUPPORTED;
            }
        }

        if (err == FM_OK)
        {
            FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                FM10000_ARP_ENTRY_GLORT,
                                DGLORT,
                                glortUser.glort );

            FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                                FM10000_ARP_ENTRY_GLORT,
                                RouterIdGlort,
                                FM10000_ROUTER_ID_NO_REPLACEMENT );

            FM_ARRAY_SET_BIT( (fm_uint32 *)pNextHop->arpData,
                              FM10000_ARP_ENTRY_GLORT,
                              markRouted,
                              0 );

        }

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end BuildNextHopTunnel */




/*****************************************************************************/
/** BuildNextHopVNTunnel
 * \ingroup intNextHop
 *
 * \desc            Fills in the arpData fields for a given Virtual-Networking
 *                  tunnel next-hop.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pNextHop points to the fm10000 next-hop record.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if argument is invalid
 *
 *****************************************************************************/
static fm_status BuildNextHopVNTunnel(fm_int           sw,
                                      fm10000_NextHop *pNextHop)
{
    fm_status           status;
    fm_vnTunnelNextHop *vnTunnel;
    fm_tunnelGlortUser  glortUser;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pNextHop=0x%p\n",
                  sw,
                  (void *) pNextHop );

    if (pNextHop == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (pNextHop->pParent->nextHop.type != FM_NEXTHOP_TYPE_VN_TUNNEL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    pNextHop->arpData[0] = 0;
    pNextHop->arpData[1] = 0;

    vnTunnel = &pNextHop->pParent->nextHop.data.vnTunnel;

    status = fm10000GetVNTunnelGroupAndRule(sw,
                                            vnTunnel->tunnel,
                                            vnTunnel->encap,
                                            vnTunnel->vni,
                                            NULL,
                                            NULL,
                                            &glortUser);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    /* Only accept next-hop if it doesn't make use of the user field. */
    if ( (glortUser.userMask != 0) || (glortUser.glortMask != 0xFFFF) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_UNSUPPORTED);
    }

    FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                        FM10000_ARP_ENTRY_GLORT,
                        DGLORT,
                        glortUser.glort );

    FM_ARRAY_SET_FIELD( (fm_uint32 *)pNextHop->arpData,
                        FM10000_ARP_ENTRY_GLORT,
                        RouterIdGlort,
                        FM10000_ROUTER_ID_NO_REPLACEMENT );

    FM_ARRAY_SET_BIT( (fm_uint32 *)pNextHop->arpData,
                      FM10000_ARP_ENTRY_GLORT,
                      markRouted,
                      0 );

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end BuildNextHopVNTunnel */




/*****************************************************************************/
/** SetArpEntryUsedStatus
 * \ingroup intNextHop
 *
 * \desc            Cleans the given block of ARP entries and sets the "used"
 *                  flag in the ARP_USED table.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlkIndex is the base index of the block.
 * 
 * \param[in]       arpBlkLength is the number of entries of the block.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERROR_INVALID_PARAMETER upon bad parameters.
 *
 *****************************************************************************/
static fm_status SetArpEntryUsedStatus(fm_int    sw,
                                       fm_int    arpBlkIndex,
                                       fm_int    arpBlkLength)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_int     index;
    fm_int     count;
    fm_uint64  arpData;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, arpBlkIndex=%d, arpBlkLength=%d\n",
                  sw,
                  arpBlkIndex,
                  arpBlkLength );

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    /* argument validation */
    if (arpBlkIndex <= 0                                ||
        arpBlkIndex >= FM10000_ARP_BLK_CTRL_TAB_SIZE    ||
        arpBlkLength <= 0                               ||
        arpBlkLength >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        arpData = 0;
        index = arpBlkIndex;
        count = arpBlkLength;

        while (count--)
        {
            switchPtr->WriteUINT64(sw,
                                   FM10000_ARP_TABLE(index,0),
                                   arpData);
            index++;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetArpEntryUsedStatus */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000CheckValidArpBlockSize
 * \ingroup intNextHop
 *
 * \desc            Verifies if the given blockSize is among the list of ECMP
 *                  modulos supported by fm10000: [1..16, 32, 64, 128, 256,
 *                  512, 1028, 2046, 4092]
 *
 * \param[in]       blockSize is the size of the ARP block to be validated
 * 
 * \return          FM_OK is blockSize is among the allowed values
 * \return          FM_FAIL otherwise
 *
 *****************************************************************************/
fm_status fm10000CheckValidArpBlockSize(fm_int  blockSize)
{
    fm_status   err;


    err = FM_FAIL;

    /* Note that most probable values are checked first. In the second if(..)
     * conditions are evaluated from left to right */
    if (blockSize > 0 && blockSize <= 16)
    {
        err = FM_OK;
    }
    else if (blockSize == 32   ||
             blockSize == 64   ||
             blockSize == 128  ||
             blockSize == 256  ||
             blockSize == 512  ||
             blockSize == 1024 ||
             blockSize == 2048 ||
             blockSize == 4096 )
    {
        err = FM_OK;

    }

    return err;

}   /* end fm10000CheckValidArpBlockSize */




/*****************************************************************************/
/** fm10000GetArpTablePathCountParameters
 * \ingroup intNextHopArp
 *
 * \desc            Determines the pathCount and the path count type used to
 *                  specify a valid ARP block size in the FFU.
 *
 * \param[in]       arpBlkSize is the size of the ARP block.
 *
 * \param[out]      pPathCount points to a caller allocated storage where
 *                  this function will return the number of entries of the
 *                  block, if the path count type is zero, or the exponent
 *                  of 2 if the path count type is 1. In this last case,
 *                  the size of the ARP block is given equal to (2 ^ pathCount)
 *
 * \param[out]      pPathCountType points to a caller allocated storage
 *                  where this function will return the path count type.
 *                  When it is zero, the effective ARP index is:
 *                    ARP index = ARP index + (hash[11:0] * PathCount)/4096
 *                  If path count type is 1, the ARP index is:
 *                    ARP index = ARP index + hash[11:0] & ((1<<PathCount)-1)
 *
 * \return          FM_OK if successful.
 * \return          FM_FAIL if the specified ARP block size is not supported.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 *
 *****************************************************************************/
fm_status fm10000GetArpTablePathCountParameters(fm_int  arpBlkSize,
                                                fm_int *pPathCount,
                                                fm_int *pPathCountType)
{
    fm_status   err;
    fm_int      exponent;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "arpBlkSize=%d, pPathCount=%p, pPathCountType=%p\n",
                  arpBlkSize,
                  (void *) pPathCount,
                  (void *) pPathCountType );
    
    if (pPathCount == NULL  ||
        pPathCountType == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fm10000CheckValidArpBlockSize(arpBlkSize);

        if (err == FM_OK)
        {
            if (arpBlkSize <= 16)
            {
                *pPathCount = arpBlkSize;
                *pPathCountType = 0;
            }
            else
            {
                exponent = 5;
                while (arpBlkSize > (1 << exponent))
                {
                    exponent++;
                }
                *pPathCount = exponent;
                *pPathCountType = 1;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetArpTablePathCountParameters */




/*****************************************************************************/
/** fm10000NextHopAlloc
 * \ingroup intNextHop
 *
 * \desc            Allocates resources for nextHop subsystem, called at
 *                  switch insertion time. No initialization is performed
 *                  by this function
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_NO_MEM if the required resources cannot be
 *                  allocated.
 *
 *****************************************************************************/
fm_status fm10000NextHopAlloc(fm_int sw)
{
    fm10000_switch *         pSwitchExt;
    fm_status                err;
    fm_int                   tsize;
    fm10000_NextHopSysCtrl * pNextHopCtrlTmp;
    fm_uint16              (*pArpHndlArrayTmp)[];
    fm10000_EcmpGroup      (*pEcmpGroupsHLTmp)[];

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d\n",
                  sw );

    pSwitchExt = GET_SWITCH_EXT(sw);
    /* assume an error condition */
    err = FM_ERR_NO_MEM;
    pArpHndlArrayTmp = NULL;
    pEcmpGroupsHLTmp = NULL;
    pNextHopCtrlTmp  = NULL;

    pSwitchExt->pNextHopSysCtrl = NULL;

    /* Alloc resources for NextHop */
    tsize = sizeof(fm10000_NextHopSysCtrl);
    pNextHopCtrlTmp  = (fm10000_NextHopSysCtrl*) fmAlloc(tsize);

    tsize = sizeof(fm_uint16) * FM10000_ARP_TAB_SIZE;
    pArpHndlArrayTmp = (fm_uint16(*)[]) fmAlloc (tsize);

    tsize = sizeof(fm10000_EcmpGroup*) * FM10000_MAX_NUM_NEXT_HOPS;
    pEcmpGroupsHLTmp = (fm10000_EcmpGroup(*)[]) fmAlloc(tsize);

    if (pNextHopCtrlTmp  != NULL &&
        pArpHndlArrayTmp != NULL &&
        pEcmpGroupsHLTmp != NULL )
    {
        FM_CLEAR(**pArpHndlArrayTmp);
        FM_CLEAR(**pEcmpGroupsHLTmp);
        FM_CLEAR(*pNextHopCtrlTmp);

        pNextHopCtrlTmp->pArpHndlArray = pArpHndlArrayTmp;
        pNextHopCtrlTmp->pEcmpGroupsHL  = pEcmpGroupsHLTmp;
        pSwitchExt->pNextHopSysCtrl     = pNextHopCtrlTmp;
        /* indicate that there is no errors */
        err = FM_OK;
    }

    /* in case of error: free all the allocated resources */
    if (err != FM_OK)
    {
        if (pArpHndlArrayTmp != NULL)
        {
            fmFree (pArpHndlArrayTmp);
            pArpHndlArrayTmp = NULL;
        }
        if (pEcmpGroupsHLTmp != NULL)
        {
            fmFree (pEcmpGroupsHLTmp);
            pEcmpGroupsHLTmp = NULL;
        }
        if (pNextHopCtrlTmp != NULL)
        {
            fmFree (pNextHopCtrlTmp);
            pNextHopCtrlTmp = NULL;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}




/*****************************************************************************/
/** fm10000NextHopFree
 * \ingroup intNextHop
 *
 * \desc            Releases resources used by nextHop subsystem, called at
 *                  switch removal time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NextHopFree(fm_int sw)
{
    fm10000_switch *       pSwitchExt;
    fm_status              err;
    fm_int                 index;
    fm10000_ArpBlockCtrl **ppArpBlkCtrlAux;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d\n",
                  sw );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pSwitchExt->pNextHopSysCtrl != NULL)
    {
        if (pSwitchExt->pNextHopSysCtrl->pArpHndlArray != NULL)
        {
            fmFree (pSwitchExt->pNextHopSysCtrl->pArpHndlArray);
        }
        if (pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab != NULL)
        {
            /* check for allocated ARP block control structures */
            for (index=0; index < FM10000_ARP_BLK_CTRL_TAB_SIZE; index++)
            {
                ppArpBlkCtrlAux =  &(*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[index];
                if (ppArpBlkCtrlAux != NULL && *ppArpBlkCtrlAux != NULL)
                {
                    fmFree(*ppArpBlkCtrlAux);
                    *ppArpBlkCtrlAux = NULL;
                }
            }
            fmFree(pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab);
            pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab = NULL;
        }
        if (pSwitchExt->pNextHopSysCtrl->pEcmpGroupsHL != NULL)
        {
            fmFree(pSwitchExt->pNextHopSysCtrl->pEcmpGroupsHL);
        }
        fmFree(pSwitchExt->pNextHopSysCtrl);
        pSwitchExt->pNextHopSysCtrl = NULL;
    }

    err = DeleteUnresolvedNextHopRedirectTrigger(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "Cannot delete unresolved NH trigger \n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}




/*****************************************************************************/
/** fm10000NextHopInit
 * \ingroup intNextHop
 *
 * \desc            Performs initialization for nextHop subsystem, called at
 *                  switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NextHopInit(fm_int sw)
{
    fm_status               err;
    fm10000_switch         *pSwitchExt;
    fm10000_NextHopSysCtrl *pNextHopCtrl;
    fm_int                  index;
    

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d\n",
                  sw );

    pSwitchExt = GET_SWITCH_EXT(sw);

    err = fm10000NextHopAlloc(sw);

    if (err != FM_OK || pSwitchExt->pNextHopSysCtrl == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* create a shortcut */
        pNextHopCtrl = pSwitchExt->pNextHopSysCtrl;

        /* first ARP table entry is not available */
        pNextHopCtrl->arpTabFreeEntryCount = FM10000_ARP_TAB_SIZE - 1 ;
        pNextHopCtrl->arpHndlTabFirstFreeEntry = 1;
        pNextHopCtrl->arpHndlTabLastUsedEntry = 0;
        pNextHopCtrl->arpStatsAgingCounter = 0;
        pNextHopCtrl->lastArpBlkCtrlTabLookupNdx = 1;

        /* intialize ARP handle table */
        err = FillArpHndlTable(sw, 1, FM10000_ARP_TAB_SIZE-1, FM10000_ARP_BLOCK_INVALID_HANDLE);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Cannot initialize ARP handle table\n");
        }
        else
        {

            /* intialize ARP table use arrays */
            for (index = 0; index < 8; index++)
            {
                pNextHopCtrl->allocatedBlkStatInfo[index] = 0;
                pNextHopCtrl->freeBlkStatInfo[index] = 0;
            }
            /* ARP table is all free */
            pNextHopCtrl->freeBlkStatInfo[FM10000_ARP_BLOCK_SIZE_MORE_THAN_257] = 1;

            /* clear ARP-entry used flags */
            err = fm10000ResetAllArpEntryUsedStatusFlags(sw);
        }
    }

    err = CreateUnresolvedNextHopRedirectTrigger(sw);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "Cannot create unresolved NH trigger \n");
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end fm10000NextHopInit */




/*****************************************************************************/
/** fm10000ValidateNextHopTrapCode
 * \ingroup intRoute
 *
 * \desc            Validates that the trap code is valid, if it is unset,
 *                  it will be set to the default value.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       nextHop contains a pointer to the nextHop to be validated
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ValidateNextHopTrapCode(fm_int      sw,
                                        fm_nextHop *nextHop)
{
    fm_status  status = FM_OK;
    
    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, nextHop->trapCode = %d\n",
                      sw,
                      nextHop->trapCode );

    switch (nextHop->trapCode)
    {
        case FM_DEFAULT_NEXTHOP_TRAPCODE:
            /* Trap code has not been set, set the default value */
            nextHop->trapCode = FM_TRAPCODE_L3_ROUTED_NO_ARP_0;
            break;

        case FM_TRAPCODE_L3_ROUTED_NO_ARP_0:
        case FM_TRAPCODE_L3_ROUTED_NO_ARP_1:
            /* Valid cases, do nothing */
            break;

        default:
            status = FM_ERR_INVALID_TRAP_CODE;
            break;
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fm10000ValidateNextHopTrapCode */




/*****************************************************************************/
/** fm10000NextHopCleanUpTab
 * \ingroup intNextHop
 *
 * \desc            Performs the clean up of the data structures associated
 *                  to the ARP table.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NextHopCleanUpTab(fm_int sw)
{
    FM_NOT_USED(sw);

    return FM_OK;
}   /* end fm10000NextHopCleanUpTab */





/*****************************************************************************/
/** fm10000RequestArpBlock
 * \ingroup intNextHop
 *
 * \desc            Allocates a block of arpCount consecutive entries in the
 *                  ARP table. If the specified block cannot be allocated but
 *                  there is enough place in the table, a partial 
 *                  defragmentation will be performed. Only valid clients are
 *                  allowed to allocate ARP blocks. Additional clients may be
 *                  register calling ''fm10000RegisterArpBlockClient''.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       client is the subsystem that requires the block of entries.
 *                  Only clients included in the client enumeration are allowed.
 *                  See ''fm10000_ArpClient''. The client that request the block
 *                  becomes the owner it is the only one that can release it.
 * 
 * \param[in]       arpCount is the number of consecutive entries that is
 *                  being requested.
 * 
 * \param[in]       arpBlkOptions defines block management options. 
 * 
 * \param[out]      pArpBlkHndl points to a caller allocated storage where the
 *                  block handle will be returned. This is written to -1 if a
 *                  no entries are available or an execution error has happened
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_UNSUPPORTED if NextHop is not supported by the
 *                   current implementation.
 * \return          FM_ERR_ARP_TABLE_FULL if the required resources cannot be allocated
 * \return          FM_FAIL if no handles are available
 *
 *****************************************************************************/
fm_status fm10000RequestArpBlock(fm_int            sw,
                                 fm10000_ArpClient client,
                                 fm_int            arpCount,
                                 fm_uint           arpBlkOptions,
                                 fm_uint16 *       pArpBlkHndl)
{
    fm10000_switch        *pSwitchExt;
    fm_status              err;
    fm_uint16              blkOffsetTmp;
    fm_uint16              blkHandleTmp;
    fm_uint16              blkOffsetLimit;
    fm10000_ArpBlockCtrl  *pArpBlkCtrlTmp;
    fm10000_ArpBlockCtrl **ppArpBlkCtrlAux;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, client=%d, arpCount=%d, arpBlkOptions=%x, pArpBlkHndl=%p\n",
                      sw,
                      client,
                      arpCount,
                      arpBlkOptions,
                      (void *) pArpBlkHndl );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    blkOffsetTmp = 0;
    blkHandleTmp = FM10000_ARP_BLOCK_INVALID_HANDLE;

    /* argument validation */
    if ( client <= FM10000_ARP_CLIENT_NONE   ||
         client >= FM10000_ARP_CLIENT_MAX    ||
         arpCount <= 0                       ||
         arpCount > (FM10000_ARP_TAB_SIZE-1) ||
         pArpBlkHndl == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pSwitchExt->pNextHopSysCtrl == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if (pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount < arpCount)
    {
        /* there is no more place in the table */
        err = FM_ERR_ARP_TABLE_FULL;
    }
    else if ( ((arpBlkOptions & FM10000_ARP_BLOCK_OPT_SWAP) == 0) &&
              (CheckArpBlockAvailability(sw, arpCount) == FALSE) )
    {
        err = FM_ERR_ARP_TABLE_FULL;
    }
    else
    {
        /* allocate the ARP-block handle table, this must be done only once,
         * the first time that an ARP block is allocated */
        if (pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL)
        {
            err = AllocArpBlkCtrlTab(sw);
        }

        if (err == FM_OK)
        {
            /* look for a free block, use the table size as upper bound
             * for the search (upperBound = -1) */
            err = ArpFreeBlockDirectLookup(sw, 
                                           arpCount,
                                           -1,
                                           &blkOffsetTmp);

            if ( (err == FM_OK) &&
                 (blkOffsetTmp > (FM10000_ARP_TAB_AVAILABLE_ENTRIES + 1 - arpCount)) )
            {
                /* Try packing the table */
                err = PackArpTable(sw, arpCount, -1);
                
                /* try finding a block once again */
                if (err == FM_OK)
                {
                    err = ArpFreeBlockDirectLookup(sw, 
                                                   arpCount,
                                                   -1,
                                                   &blkOffsetTmp);
                }

                if (err == FM_OK)
                {
                    if (arpBlkOptions & FM10000_ARP_BLOCK_OPT_SWAP_INTERMEDIATE)
                    {
                        blkOffsetLimit = FM10000_ARP_TAB_SIZE - arpCount;
                    }
                    else
                    {
                        blkOffsetLimit = FM10000_ARP_TAB_AVAILABLE_ENTRIES + 1 - arpCount;
                    }

                    if (blkOffsetTmp > blkOffsetLimit)
                    {
                        /* it is not possible to allocate the block */
                        err = FM_ERR_ARP_TABLE_FULL;
                    }
                }
            }
        }
        
        if (err == FM_OK)
        {
            /* get a block handle */
            err = GetNewArpBlkHndl(sw, &blkHandleTmp);
            if (err == FM_OK    &&
                blkHandleTmp >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
            {
                err = FM_FAIL;
            }
        }

        if (err == FM_OK)
        {
            /* allocate & intialize the ARP-block control structure */
            pArpBlkCtrlTmp = (fm10000_ArpBlockCtrl*) fmAlloc(sizeof(fm10000_ArpBlockCtrl));

            if (pArpBlkCtrlTmp != NULL)
            {
                err = FillArpHndlTable(sw,
                                        blkOffsetTmp, 
                                        arpCount, 
                                        blkHandleTmp);

                if (err == FM_OK)
                {
                    /* fill up the block control structure */
                    pArpBlkCtrlTmp->offset     = blkOffsetTmp;
                    pArpBlkCtrlTmp->length     = (fm_uint16) arpCount;
                    pArpBlkCtrlTmp->options    = arpBlkOptions;
                    pArpBlkCtrlTmp->clients[0] = client;

                    /* add the pointer to the block ctrl struct into the handle tab */

                    /* use an aux pointer, check why 
                     *  "(*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[blkHandleTmp] = pArpBlkCtrlTmp;"
                     * does not work */
                    if (pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab != NULL)
                    {
                        ppArpBlkCtrlAux =  &(*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[blkHandleTmp];
                        *ppArpBlkCtrlAux = pArpBlkCtrlTmp;
                    }

                    /* adjust the counter of free entries */
                    pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount -= arpCount;

                    /* update the indexes of the first free and last used entries */
                    UpdateFirstFreeArpEntry(sw, blkOffsetTmp, TRUE);
                    UpdateLastUsedArpEntry(sw, blkOffsetTmp+arpCount-1, TRUE);
                    UpdateArpTableStatsAfterAllocation(sw, blkOffsetTmp, arpCount);

                    /* set ARP "used" flags */
                    err = SetArpEntryUsedStatus(sw, blkOffsetTmp, arpCount);
                }
                else
                {
                    /* there was an error filling the ARP ctrl table:
                     *  free the allocated memory and invalidate the handle */
                    fmFree(pArpBlkCtrlTmp);
                    pArpBlkCtrlTmp = NULL;
                    blkHandleTmp = FM10000_ARP_BLOCK_INVALID_HANDLE;
                }
            }
            else
            {
                err = FM_ERR_NO_MEM;
                blkHandleTmp = FM10000_ARP_BLOCK_INVALID_HANDLE;
            }
        }
    }

    if (pArpBlkHndl != NULL)
    {
        *pArpBlkHndl = blkHandleTmp;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RequestArpBlock */




/*****************************************************************************/
/** fm10000FreeArpBlock
 * \ingroup intNextHop
 *
 * \desc            Releases a block of entries in the ARP table specified by
 *                  arpBlkHndl. The client requesting to free the block must
 *                  be the owner of it. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       client is the subsystem that requires the block release.
 *                  Only clients included in the client enumeration are allowed.
 *                  See ''fm10000_ArpClient''. The client that request the block
 *                  becomes the owner it is the only one that can release it.
 * 
 * \param[in]       arpBlkHndl is the ARP block handle. 
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeArpBlock(fm_int            sw,
                              fm10000_ArpClient client,
                              fm_uint16         arpBlkHndl)
{
    fm_switch *            switchPtr;
    fm10000_switch *       pSwitchExt;
    fm_status              err;
    fm10000_ArpBlockCtrl **ppArpBlkCtrlTmp;
    fm_int                 arpBlkIndex;
    fm_int                 arpBlkLength;
    fm_int                 arpEntryIndex;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, client=%d, arpBlkHndl=%d\n",
                      sw,
                      client,
                      arpBlkHndl );

    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    /* argument validation */
    if (client <= FM10000_ARP_CLIENT_NONE   ||
        client >= FM10000_ARP_CLIENT_MAX    ||
        (arpBlkHndl - 1) >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->state == FM_SWITCH_STATE_GOING_DOWN)
    {
        /* ARP block ctrl table has already been freed in this case */
        err = FM_OK;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* create a shortcut */
        ppArpBlkCtrlTmp = &(*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlkHndl];

        /* validate the handle and the owner of the block */
        if (*ppArpBlkCtrlTmp == NULL)
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Invalid ARP block handle. Cannot release ARP block\n");

        }
        else if ( (*ppArpBlkCtrlTmp)->clients[0] != client )
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Invalid client. Cannot release ARP block\n");

        }
        else
        {
            /* Free the entries in the ARP handle table */
            arpBlkIndex = (*ppArpBlkCtrlTmp)->offset;
            arpBlkLength = (*ppArpBlkCtrlTmp)->length;

            err = FillArpHndlTable(sw,
                                    arpBlkIndex,
                                    arpBlkLength,
                                    FM10000_ARP_BLOCK_INVALID_HANDLE);

            /* adjust the counter of free entries */ 
            pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount += arpBlkLength;

            /* update the offset of the first free entry to the ARP table */
            if (pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry > arpBlkIndex)
            {
                pSwitchExt->pNextHopSysCtrl->arpHndlTabFirstFreeEntry = arpBlkIndex;
            }

            /* update the indexes of the first free and last used entries */
            UpdateFirstFreeArpEntry(sw, arpBlkIndex, FALSE);
            UpdateLastUsedArpEntry(sw, arpBlkIndex + arpBlkLength - 1, FALSE);

            CleanUpArpTable(sw, arpBlkIndex, arpBlkLength);
            UpdateArpTableStatsAfterRelease(sw, arpBlkIndex, arpBlkLength);

            /* reset "used" entries flags in the ARP_USED table.
             * pass a NULL pointer because the current status is not relevant */
            arpEntryIndex = arpBlkIndex;
            while (arpBlkLength--)
            {
                err  = fm10000GetArpEntryUsedStatus (sw,
                                                     arpEntryIndex++,
                                                     TRUE,
                                                     NULL);
            }

            /* release the block control structure */
            fmFree (*ppArpBlkCtrlTmp);
            *ppArpBlkCtrlTmp = NULL;

            if (err == FM_OK)
            {
                /* perform a maintenance defragmentation */
                err = MaintenanceArpTablePacking(sw, arpBlkIndex);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000FreeArpBlock */





/*****************************************************************************/
/** fm10000RegisterArpBlockClient
 * \ingroup intNextHop
 *
 * \desc            Registers an additional client for the block specified by
 *                  arpBlkHndl. The maximum number of client supported by a
 *                  block is defined by ''FM10000_ARP_BLOCK_MAX_CLIENTS''.
 *                  The list of clients is used to send notifications in the
 *                  case that blocks are moved during eventual ARP table
 *                  defragmentation.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlkHndl is the ARP block handle.
 * 
 * \param[in]       newClient is the additional subsystem that requires to be
 *                  notified if the block is moved.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000RegisterArpBlockClient(fm_int             sw,
                                        fm_uint16          arpBlkHndl,
                                        fm10000_ArpClient  newClient)
{
    fm10000_switch *      pSwitchExt;
    fm_status             err;
    fm10000_ArpBlockCtrl *pArpBlkCtrlTmp;
    fm_int                index;
    fm_int                freeClientEntry;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, arpBlkHndl=%d, newClient=%d\n",
                      sw,
                      arpBlkHndl,
                      newClient );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    freeClientEntry = -1;

    /* argument validation */
    if (newClient <= FM10000_ARP_CLIENT_NONE   ||
        newClient >= FM10000_ARP_CLIENT_MAX    ||
        (arpBlkHndl - 1) >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* create a shortcut */
        pArpBlkCtrlTmp = (*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlkHndl];

        if (pArpBlkCtrlTmp == NULL)
        {
            /* the specified handle is not valied */
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Invalid ARP handle\n");
        }
        else
        {
            for (index = 0; index < FM10000_MAX_ARP_BLOCK_CLIENTS; index++)
            {
                if ((pArpBlkCtrlTmp->clients)[index] == newClient)
                {
                    /* error: client is already defined */
                    err = FM_FAIL;
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "Client is already registered\n");
                    break;
                }
                else if ( (freeClientEntry == -1)  &&
                          ((pArpBlkCtrlTmp->clients)[index] == FM10000_ARP_CLIENT_NONE) )
                {
                    /* remember the first free entry */
                    freeClientEntry = index;
                }
            }
            if (err == FM_OK)
            {
                if (freeClientEntry > 0)
                {
                    /* register the new client */
                    (pArpBlkCtrlTmp->clients)[freeClientEntry] = newClient;
                }
                else
                {
                    /* ERROR: there is no more room in the table */
                    err = FM_FAIL;
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "ARP client tab is full\n");
                }
            }
        }
    }
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RegisterArpBlockClient */





/*****************************************************************************/
/** fm10000UnregisterArpBlockClient
 * \ingroup intNextHop
 *
 * \desc            Unregisters a client for the ARP block specified by
 *                  arpBlkHndl.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlkHndl is the ARP block handle.
 * 
 * \param[in]       client is the subsystem that requires to be removed from
 *                  the list of clients of the specified block.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UnregisterArpBlockClient(fm_int             sw,
                                          fm_uint16          arpBlkHndl,
                                          fm10000_ArpClient  client)
{
    fm10000_switch *      pSwitchExt;
    fm_status             err;
    fm10000_ArpBlockCtrl *pArpBlkCtrlTmp;
    fm_int                index;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, arpBlkHndl=%d, client=%d\n",
                      sw,
                      arpBlkHndl,
                      client );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    /* argument validation */
    if (client <= FM10000_ARP_CLIENT_NONE   ||
        client >= FM10000_ARP_CLIENT_MAX    ||
        (arpBlkHndl - 1) >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* create a shorcut */
        pArpBlkCtrlTmp = (*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlkHndl];

        if (pArpBlkCtrlTmp == NULL)
        {
            /* the specified handle is not valid */
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Invalid ARP handle\n");
        }
        else
        {
            for (index = 0; index < FM10000_MAX_ARP_BLOCK_CLIENTS; index++)
            {
                if ((pArpBlkCtrlTmp->clients)[index] == client)
                {
                    (pArpBlkCtrlTmp->clients)[index] = FM10000_ARP_CLIENT_NONE;
                    break;
                }
            }
            if (index ==  FM10000_MAX_ARP_BLOCK_CLIENTS)
            {
                /* ERROR: client not found */
                err = FM_FAIL;
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                             "ARP client not found\n");
            }
        }
    }
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000UnregisterArpBlockClient */




/*****************************************************************************/
/** fm10000CopyArpBlockEntries
 * \ingroup intNextHop
 *
 * \desc            Copies a given number of consecutives ARP entries from the
 *                  specified source block into the destination block. Both
 *                  blocks must belong to the given client. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       client is the subsystem that requires the operation. It
 *                  must be the owner of both, the source and the destination
 *                  block of entries.
 * 
 * \param[in]       srcArpBlkHndl is the handle of the source block of
 *                  entries.
 * 
 * \param[in]       dstArpBlkHndl is the handle of the destination block.
 * 
 * \param[in]       entryCount is the number of consecutive entries to be
 *                  copied.
 * 
 * \param[in]       srcOffset is the offset, relative to the begining of the
 *                  source block, of the first entry to be copied.
 * 
 * \param[in]       dstOffset is the offset, relative to the begining of the
 *                  destination block, where the first entry will be copied.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000CopyArpBlockEntries(fm_int            sw,
                                     fm10000_ArpClient client,
                                     fm_uint16         srcArpBlkHndl,
                                     fm_uint16         dstArpBlkHndl,
                                     fm_int            entryCount,
                                     fm_int            srcOffset,
                                     fm_int            dstOffset)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(client);
    FM_NOT_USED(srcArpBlkHndl);
    FM_NOT_USED(dstArpBlkHndl);
    FM_NOT_USED(entryCount);
    FM_NOT_USED(srcOffset);
    FM_NOT_USED(dstOffset);

    return FM_OK;
}   /* end fm10000CopyArpBlockEntries */




/*****************************************************************************/
/** fm10000GetArpEntryUsedStatus
 * \ingroup intNextHop
 *
 * \desc            Retrieves the "used" flag for the given ARP entry and,
 *                  if resetFlag is set, resets its status to "not used".
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpIndex is ARP table offset of the entry to be checked
 * 
 * \param[in]       resetFlag specifies if the "used" flag status must be
 *                  reset after read it.
 * 
 * \param[out]      pUsed points to a caller allocated storage where the
 *                  status of the "used" flag will be returned. May be
 *                  NULL.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERROR_INVALID_PARAMETER if the arpIndex is out of
 *                   range
 *
 *****************************************************************************/
fm_status fm10000GetArpEntryUsedStatus(fm_int   sw,
                                       fm_int   arpIndex,
                                       fm_bool  resetFlag,
                                       fm_bool *pUsed)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_bool    currentFlagStatus;
    fm_uint32  arpUsedFlgWordValue;
    fm_uint32  arpUsedFlgRegAddr;
    fm_uint32  arpUsedFlgMask;
    fm_uint32  arpUsedFlgWordOffset;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, arpIndex=%d, resetFlag=%s, pUsed=%p\n",
                  sw,
                  arpIndex,
                  resetFlag ? "TRUE" : "FALSE",
                  (void *) pUsed );

    err = FM_OK;
    currentFlagStatus = FALSE;
    switchPtr = GET_SWITCH_PTR(sw);

    /* argument validation */
    if (arpIndex <= 0 || arpIndex >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* compute the position of the flag */
        arpUsedFlgWordOffset = FM10000_ARP_USED_INDEX_FOR_ENTRY(arpIndex);
        arpUsedFlgRegAddr    = FM10000_ARP_USED(arpUsedFlgWordOffset);
        arpUsedFlgMask       = FM10000_ARP_USED_BIT_FOR_ENTRY(arpIndex);

        err = switchPtr->ReadUINT32(sw,
                                    arpUsedFlgRegAddr,
                                    &arpUsedFlgWordValue);

        if (err == FM_OK)
        {
            if (arpUsedFlgWordValue & arpUsedFlgMask)
            {
                currentFlagStatus = TRUE;
                if (resetFlag)
                {
                    err = switchPtr->WriteUINT32( sw,
                                                  arpUsedFlgRegAddr,
                                                  arpUsedFlgMask );
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                     "Cannot reset ARP used flag\n");
                    }
                }
            }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Cannot read ARP used flag register\n");
        }

        if (pUsed != NULL)
        {
            *pUsed = currentFlagStatus;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetArpEntryUsedStatus */




/*****************************************************************************/
/** fm10000ResetAllArpEntryUsedStatusFlags
 * \ingroup intNextHop
 *
 * \desc            Retrieves the "used" flag for the given ARP entry and,
 *                  if resetFlag is set, resets its status to "not used".
 *
 * \param[in]       sw is the switch number.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ResetAllArpEntryUsedStatusFlags(fm_int  sw)
{
    fm_status  err;
    fm_switch *switchPtr;
    fm_int     index;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d\n",
                  sw );

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    for (index = 0; index < FM10000_ARP_USED_ENTRIES; index++)
    {
        err = switchPtr->WriteUINT32( sw,
                                      FM10000_ARP_USED(index),
                                      0xFFFFFFFF);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Cannot clear ARP used flag table\n");
            break;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000ResetAllArpEntryUsedStatusFlags */





/*****************************************************************************/
/** fm10000RepairArpUsed
 * \ingroup intParity
 *
 * \desc            Repairs FM10000_ARP_USED_ENTRIES.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000RepairArpUsed(fm_int sw)
{
    fm_switch *switchPtr;
    fm_status  err;
    fm_int     index;

    FM_LOG_ENTRY(FM_LOG_CAT_PARITY,
                 "sw=%d\n",
                 sw);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    TAKE_REG_LOCK(sw);

    for (index = 0; index < FM10000_ARP_USED_ENTRIES; index++)
    {
        err = switchPtr->WriteUINT32( sw,
                                      FM10000_ARP_USED(index),
                                      0xFFFFFFFF);
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_PARITY,
                         "Cannot clear ARP used flag table\n");
            break;
        }
    }

    DROP_REG_LOCK(sw);
    err = fmReleaseWriteLock(&switchPtr->routingLock);

    FM_LOG_EXIT(FM_LOG_CAT_PARITY, err);

}   /* end fm10000RepairArpUsed */




/*****************************************************************************/
/** fm10000GetArpBlockInfo
 * \ingroup intNextHop
 *
 * \desc            Returns information about the ARP block identified by
 *                  arpBlkHndl.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       client is the subsystem that requires the information. 
 * 
 * \param[in]       arpBlkHndl is the handle of the ARP block.
 * 
 * \param[out]      pArpBlkDscrptor points to a caller allocated storage where
 *                  the block information will be returned.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetArpBlockInfo(fm_int                  sw,
                                 fm10000_ArpClient       client,
                                 fm_uint16               arpBlkHndl,
                                 fm10000_ArpBlkDscrptor *pArpBlkDscrptor)
{
    fm_status err;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, client=%d, arpBlkHndl=%d, pArpBlkDscrptor=%p\n",
                  sw,
                  client,
                  arpBlkHndl,
                  (void *) pArpBlkDscrptor );

    err = FM_OK;

    if (client == FM10000_ARP_CLIENT_NONE ||
        client >= FM10000_ARP_CLIENT_MAX  ||
        arpBlkHndl == FM10000_ARP_BLOCK_INVALID_HANDLE ||
        arpBlkHndl >= FM10000_ARP_BLK_CTRL_TAB_SIZE    ||
        pArpBlkDscrptor == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pArpBlkDscrptor->offset = GetArpBlockOffset(sw, arpBlkHndl);
        pArpBlkDscrptor->length = GetArpBlockLength(sw, arpBlkHndl);
        pArpBlkDscrptor->flags =  GetArpBlockFlags(sw, arpBlkHndl);
        pArpBlkDscrptor->numClients = GetArpBlockNumOfClients(sw, arpBlkHndl);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetArpBlockInfo */




/*****************************************************************************/
/** fm10000GetArpTabInfo
 * \ingroup intNextHop
 *
 * \desc            returns information about the whole ARP table.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[out]      pArpTabDscriptor points to a caller allocated storage where
 *                  the ARP table information will be returned.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetArpTabInfo(fm_int                  sw,
                               fm10000_ArpTabDscrptor *pArpTabDscriptor)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(pArpTabDscriptor);

    return FM_OK;
}   /* end fm10000GetArpTabInfo */




/*****************************************************************************/
/** fm10000NotifyArpBlockChange
 * \ingroup intNextHop
 *
 * \desc            notifies all clients of the given ARP block that it has
 *                  been moved during a table defragmentation sequence. At the
 *                  moment of the notification, both the old and the new copies
 *                  of the block coexist. Only after all notifications have
 *                  been sent wihtout errors, the old copy is released. If one
 *                  of the block clients returns an error, the moving operation
 *                  is cancelled. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       arpBlkHndl is the handle of the ARP block.
 * 
 * \param[in]       pClientToNotify pointer to the table of clients to be
 *                   notified. It could be NULL, in which case, the table
 *                   of clients of the control block will be used. This
 *                   pointer is mostly used to undo notifications when one
 *                   of the clients return an error code. Note that the size
 *                   of the table is fixed.
 * 
 * \param[in]       newBlkOffset is the offset of the new copy of the block.
 * 
 * \param[in]       oldBlkOffset is the offset of the old copy of the block.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000NotifyArpBlockChange(fm_int    sw,
                                      fm_uint16 arpBlkHndl,
                                      fm_uint16 (*pClientToNotify)[FM10000_MAX_ARP_BLOCK_CLIENTS],
                                      fm_int    newBlkOffset,
                                      fm_int    oldBlkOffset)
{
    fm10000_switch *       pSwitchExt;
    fm_status              err;
    fm_status              localErr;
    fm10000_ArpBlockCtrl * pArpBlkCtrlTmp;
    fm_int                 index;
    fm_uint16              client;
    fm_uint16              clientUndoTab[FM10000_MAX_ARP_BLOCK_CLIENTS];
    fm_uint16             (*pClientToNotifyLocal)[FM10000_MAX_ARP_BLOCK_CLIENTS];

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, arpBlkHndl=%d, newBlkOffset=%d oldBlkOffset=%d\n",
                      sw,
                      arpBlkHndl,
                      newBlkOffset,
                      oldBlkOffset );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    localErr = FM_OK;
    pArpBlkCtrlTmp = NULL;

    /* argument validation */
    if (newBlkOffset < 0                                    ||
        newBlkOffset >= FM10000_ARP_BLK_CTRL_TAB_SIZE       ||
        oldBlkOffset < 0                                    ||
        oldBlkOffset >= FM10000_ARP_BLK_CTRL_TAB_SIZE       ||
        arpBlkHndl == 0                                     ||
        arpBlkHndl >= FM10000_ARP_BLK_CTRL_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* intialize undo table */
        for (index = 0; index < FM10000_MAX_ARP_BLOCK_CLIENTS; index++)
        {
            clientUndoTab[index] = FM10000_ARP_CLIENT_NONE;
        }

        /* create a shorcut */
        pArpBlkCtrlTmp = (*pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab)[arpBlkHndl];

        if (pArpBlkCtrlTmp == NULL)
        {
            /* the specified handle is not valid */
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Invalid ARP handle\n");
        }
        else
        {
            if (pClientToNotify == NULL)
            {
                pClientToNotifyLocal = (fm_uint16 (*)[FM10000_MAX_ARP_BLOCK_CLIENTS])(pArpBlkCtrlTmp->clients);
            }
            else
            {
                pClientToNotifyLocal = pClientToNotify;
            }

            /* notify all clients. If a single client returns an error,
             * stop sending notifications, undo the changes and come back
             * to the previous state */
            for (index = 0; index < FM10000_MAX_ARP_BLOCK_CLIENTS; index++)
            {
                client = (*pClientToNotifyLocal)[index];
                switch (client)
                {
                    case FM10000_ARP_CLIENT_ECMP:
                        /* Notify ECMP client here. */
                        err = UpdateEcmpGroupArpBlockInfo(sw,
                                                          arpBlkHndl,
                                                          newBlkOffset,
                                                          oldBlkOffset);
                        break;
                    case FM10000_ARP_CLIENT_LBG:
                        /* add funcion to notify LBG client here */
                        break;
                    case FM10000_ARP_CLIENT_VN:
                        /* add funcion to notify VN client here */
                        break;
                }

                if (err == FM_OK)
                {
                    /* add the client to the undo table */
                    clientUndoTab[index] = client;
                }
                else
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "ARP changes notification failed, client=%d, err=%d\n",
                                 client,
                                 err);

                    if (pClientToNotify == NULL)
                    {
                        /* try to undo the changes and return to the initial state:
                         *  fm10000NotifyArpBlockChange is called using the client undo table,
                         *  Note that oldBlkOffset and newBlkOffset have been exchanged to revert the
                         *  change
                         *  A local error variable is used to keep the original error code */ 
                        localErr = fm10000NotifyArpBlockChange(sw,
                                                               arpBlkHndl,
                                                               &clientUndoTab,
                                                               oldBlkOffset,
                                                               newBlkOffset);
                        if (localErr != FM_OK)
                        {
                            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                         "ARP undo changes notification failed, client=%d, errCode=%d\n",
                                         client,
                                         localErr);
                        }
                    }   /* end else if (error == FM_OK) */
                                       
                    break;
                }
            }       /* end of for () ... */
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000NotifyArpBlockChange */




/*****************************************************************************/
/** fm10000DefragArpTable
 * \ingroup intNextHop
 *
 * \desc            Forces a full defragmentation of the ARP table.
 *
 * \param[in]       sw is the switch number.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DefragArpTable(fm_int sw)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_int          requiredBlockSize;
    
    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d\n",
                      sw );

    pSwitchExt = GET_SWITCH_EXT(sw);

    requiredBlockSize = 
        pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount
        - FM10000_ARP_MAINTENANCE_NOF_RESERVED_ENTRIES;

    err = PackArpTable(sw, requiredBlockSize, -1);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000DefragArpTable */




/*****************************************************************************/
/** fm10000SetInterfaceAttribute
 * \ingroup intNextHopIf
 *
 * \desc            Set interface attributes at low level.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface on which to operate.
 *
 * \param[in]       attr is the attribute to work on.
 *
 * \param[in]       pValue is the new attribute value.  This parameter may
 *                  be ignored, as the value has already been parsed and
 *                  stored into the switch table.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fm10000SetInterfaceAttribute(fm_int sw,
                                       fm_int interface,
                                       fm_int attr,
                                       void * pValue)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(interface);
    FM_NOT_USED(attr);
    FM_NOT_USED(pValue);

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, interface=%d, attr=%d, pValue=%p\n",
                  sw,
                  interface,
                  attr,
                  (void *) pValue );

    /* nothing to do at low level */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fm10000SetInterfaceAttribute */




/*****************************************************************************/
/** fm10000AddECMPGroupNextHops
 * \ingroup intRoute
 *
 * \desc            Adds new next-hops to a dynamic ECMP Group at low level.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the ECMP Group's information.
 *
 * \param[in]       numNextHops contains the number of next hop records
 *                  in pNextHopList.
 *
 * \param[in]       pNextHopList points to an array of next hops to be added.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddECMPGroupNextHops(fm_int           sw,
                                      fm_intEcmpGroup *pEcmpGroup,
                                      fm_int           numNextHops,
                                      fm_ecmpNextHop * pNextHopList)
{
    fm_status  err;
    fm_status  err2;
    fm_switch *switchPtr;
    fm_int     nextHopToAdd;
    fm_int *   pUndoNextHopTab;
    fm_int     addedNextHopExtnsions;
    fm_uint16  arpBlockHndl;
    fm_uint16  arpBlockHndlAux;
    fm_uint16  oldOffset;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, numNextHops=%d, pNextHopList=%p\n",
                  sw,
                  (void *) pEcmpGroup,
                  numNextHops,
                  (void *) pNextHopList );

    switchPtr             = GET_SWITCH_PTR(sw);
    err                   = FM_OK;
    addedNextHopExtnsions = 0;
    pUndoNextHopTab       = NULL;

    if (pEcmpGroup   == NULL ||
        pNextHopList == NULL ||
        numNextHops  <= 0    ||
        numNextHops  >= FM10000_ARP_TAB_SIZE)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pEcmpGroup->wideGroup)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if (pEcmpGroup->nextHopCount > switchPtr->maxEcmpGroupSize)
    {
        err = FM_ERR_ECMP_GROUP_IS_FULL;
    }
    else
    {
        /* in order to simplify undo actions:
         *  1- verify if there are new next hops that have been added at
         *     high level;
         *  2- check if there is enough place to alloc a new block of
         *     entries in the ARP table. 
         *  3- create the "add-nexthop-undo" table
         *  4- add new nextHop low level extensions
         *  5- configure the new block
         *  6- send notifications to the ECMP group clients
         *  7- if something was wrong, come back to the original state
         *  
         *  More restrictive conditions are evaluated first to make easier
         *  undo actions. */

        err = CheckRequiredNextHopExtensions(sw,
                                             pEcmpGroup,
                                             &nextHopToAdd);

        if (err == FM_OK && nextHopToAdd > 0)
        {
            /* try to alloc an ARP block of entries.
             *  
             *  Each time an entry is added to a dynamic ECMP group,
             *  a new block is created, so during the transaction, the ARP
             *  table must have enough place to keep both the old block and
             *  the new one at the same time. After the transaction is completed,
             *  the old block is released. */

            /* get the old ARP block handle */
            err = GetEcmpGroupArpBlockHandle(sw, pEcmpGroup, &arpBlockHndl);

            if (err == FM_OK)
            {
                /* alloc a new block of entries */
                err = fm10000RequestArpBlock(sw,
                                             FM10000_ARP_CLIENT_ECMP,
                                             pEcmpGroup->nextHopCount,
                                             FM10000_ARP_BLOCK_OPT_NONE,
                                             &arpBlockHndlAux);

                if (err == FM_OK)
                {
                    /* if the old handle was valid, swap handles to access the new block
                     * using the old handle */
                    if (arpBlockHndl != FM10000_ARP_BLOCK_INVALID_HANDLE)
                    {
                        err = SwapArpHandles(sw, arpBlockHndl, arpBlockHndlAux);
                    }
                    else
                    {
                        /* this is the first next hop for this group */
                        arpBlockHndl = arpBlockHndlAux;
                        arpBlockHndlAux = FM10000_ARP_BLOCK_INVALID_HANDLE;
                    }
                    
                    if (err == FM_OK)
                    {
                        /* alloc the undo table */
                        pUndoNextHopTab = (fm_int *) fmAlloc( nextHopToAdd * sizeof(fm_int) );
                        if (pUndoNextHopTab == NULL)
                        {
                            err = FM_ERR_NO_MEM;
                        }
                    }
                }
            }
            
            if (err == FM_OK)
            {
                /* alloc next hop extensions structures */
                err = AllocNextHopExtensions(sw,
                                             pEcmpGroup,
                                             arpBlockHndl,
                                             nextHopToAdd,
                                             pUndoNextHopTab);

                if (err == FM_OK)
                {
                    /* set arp indexes for the modified group */
                    err = SetNextHopArpIndexes(sw, pEcmpGroup, arpBlockHndl);
                }

                /* set up the new block of ARP entries of the ECMP group:
                 * 1- Update nextHops structures
                 * 2- Write into the ARP table the next hop info
                 * 3- Notify ECMP users about ARP table modifications  */

                if (err == FM_OK)
                {
                    err = SetupEcmpGroupArpEntries(sw,
                                                   pEcmpGroup,
                                                   arpBlockHndl,
                                                   TRUE);
                }

                if (err == FM_OK)
                {
                    oldOffset = GetArpBlockOffset(sw, arpBlockHndlAux);

                    /* notify ECMP group clients */
                    err = fm10000NotifyEcmpGroupChange(sw,
                                                       pEcmpGroup->groupId,
                                                       oldOffset);
                }

                if (err != FM_OK)
                {
                    /* an error has happend, undo all actions */

                    /* swap handles to come back to the original state */
                    if (arpBlockHndlAux != FM10000_ARP_BLOCK_INVALID_HANDLE)
                    {
                        err2 = SwapArpHandles(sw, arpBlockHndl, arpBlockHndlAux);

                        if (err2 != FM_OK)
                        {
                            /* just log the error, do not abort */
                            FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                                          "cannot swap ARP block handles, "
                                          "handle=%d, aux-handle=%d, err2=%d (%s)\n",
                                          arpBlockHndl,
                                          arpBlockHndlAux,
                                          err2,
                                          fmErrorMsg(err2) );
                        }
                    }
                    else
                    {
                        /* this is the first next hop for this group */
                        arpBlockHndlAux = arpBlockHndl;
                        arpBlockHndl = FM10000_ARP_BLOCK_INVALID_HANDLE;
                    }

                    /* release added next hop extensions using the undo table */
                    err2 = ReleaseNextHopExtensions(sw,
                                                    pEcmpGroup,
                                                    nextHopToAdd,
                                                    pUndoNextHopTab);

                    if (err2 != FM_OK)
                    {
                        /* just log the error, do not abort */
                        FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                                      "cannot remove nextHop extensions, err2=%d (%s)\n",
                                      err2,
                                      fmErrorMsg(err2) );
                    }

                    oldOffset = GetArpBlockOffset(sw, arpBlockHndlAux);

                    err2 = fm10000NotifyEcmpGroupChange(sw,
                                                        pEcmpGroup->groupId,
                                                        oldOffset);
                    if (err2 != FM_OK)
                    {
                        /* just log the error, do not abort */
                        FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                                      "cannot notify undo changes, err2=%d (%s)\n",
                                      err2,
                                      fmErrorMsg(err2) );
                    }
                }

                /* release the auxiliary ARP block */
                if (arpBlockHndlAux != FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    err2 = fm10000FreeArpBlock(sw, FM10000_ARP_CLIENT_ECMP, arpBlockHndlAux);

                    if (err2 != FM_OK)
                    {
                        /* just log the error, do not abort */
                        FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                                      "cannot release ARP block, handle=%d, err2=%d (%s)\n",
                                      arpBlockHndlAux,
                                      err2,
                                      fmErrorMsg(err2) );
                    }
                }
            }

            /* release the undo table */
            if (pUndoNextHopTab != NULL)
            {
                fmFree(pUndoNextHopTab);
                pUndoNextHopTab = NULL;
            }
                     
        }   /* end if (err == FM_OK && nextHopToAdd > 0) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000AddECMPGroupNextHops */




/*****************************************************************************/
/** fm10000DeleteECMPGroupNextHops
 * \ingroup intRoute
 *
 * \desc            Removes the low level extensions of the deleted
 *                  next-hops, create a new ARP block and notify ECMP
 *                  clients.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the high level ECMP Group.
 *
 * \param[in]       numRemovedHops is the number of next-hop records in
 *                  removedHops.
 *
 * \param[in]       ppHopsToRemove is a pointer to an array of next-hop
 *                  record pointers which describe the next-hops to be
 *                  removed.
 *
 * \param[in]       numNextHops contains the number of next hop records
 *                  in nextHopList.
 *
 * \param[in]       pNextHopList points to an array of next hops to be deleted.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteECMPGroupNextHops(fm_int           sw,
                                         fm_intEcmpGroup *pEcmpGroup,
                                         fm_int           numRemovedHops,
                                         fm_intNextHop ** ppHopsToRemove,
                                         fm_int           numNextHops,
                                         fm_ecmpNextHop * pNextHopList)
{
    fm_status   err;
    fm_int      index;
    fm_uint16   arpBlockHndl;
    fm_uint16   arpBlockHndlAux;
    fm_uint16   oldOffset;
    fm_uint16   newOffset;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, numRemovedHops=%d, "
                  "ppHopsToRemove=%p, numRemovedHops=%d, pNextHopList=%p\n",
                  sw,
                  (void *) pEcmpGroup,
                  numRemovedHops,
                  (void *) ppHopsToRemove,
                  numNextHops,
                  (void *) pNextHopList );

    FM_NOT_USED(numNextHops);
    FM_NOT_USED(pNextHopList);

    err = FM_OK;

    /* only used parameters are validates */
    if(pEcmpGroup == NULL     ||
       ppHopsToRemove == NULL ||
       numRemovedHops < 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pEcmpGroup->wideGroup)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* get the current handle */
        err = GetEcmpGroupArpBlockHandle(sw, pEcmpGroup, &arpBlockHndl);

        if (arpBlockHndl == FM10000_ARP_BLOCK_INVALID_HANDLE)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "Trying to delete a NextHop from an empty ECMP group, pGroupId=%d\n",
                         pEcmpGroup->groupId);

            err = FM_ERR_NOT_FOUND;
        }
        else if (err == FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                         "Delete NextHops from ECMP group=%d\n",
                         pEcmpGroup->groupId);

            if (pEcmpGroup->nextHopCount > 0)
            {
                /* alloc a new block of entries for the remaining nexthops */
                err = fm10000RequestArpBlock(sw,
                                             FM10000_ARP_CLIENT_ECMP,
                                             pEcmpGroup->nextHopCount,
                                             FM10000_ARP_BLOCK_OPT_SWAP_INTERMEDIATE,
                                             &arpBlockHndlAux);

            }
            else
            {
                /* after deleting the next hops the group remains empty */
                arpBlockHndlAux =  FM10000_ARP_BLOCK_INVALID_HANDLE;
            }

            if (err == FM_OK)
            {
                /* store original ARP index of the ECMP group */
                oldOffset = GetArpBlockOffset(sw, arpBlockHndl);

                /* remove low level extensions of the next-hops in the array
                 * The array have copies of the next-hops already deleted at
                 * high level */
                for (index = 0; index < numRemovedHops; index++)
                {
                    if ( ppHopsToRemove[index]->extension != NULL )
                    {
                        fmFree(ppHopsToRemove[index]->extension);
                        ppHopsToRemove[index]->extension = NULL;
                    }
                }

                /* if the group is not empty, swap handles to access the new block
                 * using the old handle
                 */
                if (arpBlockHndlAux != FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    err = SwapArpHandles(sw, arpBlockHndl, arpBlockHndlAux);

                    if (err == FM_OK)
                    {
                        /* release the original block */
                        err = fm10000FreeArpBlock(sw,
                                                  FM10000_ARP_CLIENT_ECMP,
                                                  arpBlockHndlAux);
                    }

                    newOffset = GetArpBlockOffset(sw, arpBlockHndl);

                    /* if the swap reserved area was used for the new block
                     * it was temporary only and now we should be able
                     * to relocate the block back to the available space */
                    if ((err == FM_OK) &&
                        (newOffset >
                         (FM10000_ARP_TAB_AVAILABLE_ENTRIES + 1 - pEcmpGroup->nextHopCount)))
                    {

                        /* alloc final block of entries for the remaining nexthops */
                        err = fm10000RequestArpBlock(sw,
                                                     FM10000_ARP_CLIENT_ECMP,
                                                     pEcmpGroup->nextHopCount,
                                                     FM10000_ARP_BLOCK_OPT_SWAP_FINAL,
                                                     &arpBlockHndlAux);
                        if (err == FM_OK)
                        {
                            err = SwapArpHandles(sw, arpBlockHndl, arpBlockHndlAux);
                        }
                        
                        if (err == FM_OK)
                        {
                            /* release the temporary block in the swap reserved area  */
                            err = fm10000FreeArpBlock(sw,
                                                      FM10000_ARP_CLIENT_ECMP,
                                                      arpBlockHndlAux);
                        }

                    }

                }
                else
                {
                    /* this was the last next hop, so the group will be empty  */
                    arpBlockHndlAux = arpBlockHndl;
                    arpBlockHndl = FM10000_ARP_BLOCK_INVALID_HANDLE;

                    /* release the original block */
                    err = fm10000FreeArpBlock(sw, FM10000_ARP_CLIENT_ECMP, arpBlockHndlAux);
                }

            }

            if (err == FM_OK)
            {
                if (arpBlockHndl != FM10000_ARP_BLOCK_INVALID_HANDLE)
                {
                    /* set arp indexes for the modified group */
                    err = SetNextHopArpIndexes(sw, pEcmpGroup, arpBlockHndl);

                    if (err == FM_OK)
                    {
                        /* setup ARP table entries */
                        err = SetupEcmpGroupArpEntries(sw, pEcmpGroup, arpBlockHndl, TRUE);
                    }
                }
                else
                {
                    err = SetEcmpGroupArpBlockHandle(sw, pEcmpGroup, arpBlockHndl);
                }

                if (err == FM_OK)
                {
                    /* notify ecmp group clients */
                    err = fm10000NotifyEcmpGroupChange(sw,
                                                   pEcmpGroup->groupId,
                                                   oldOffset);
                }
            }

        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000DeleteECMPGroupNextHops */




/*****************************************************************************/
/** fm10000ReplaceECMPGroupNextHop
 * \ingroup intRoute
 *
 * \desc            Replaces a next-hop in an ECMP Group.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the ECMP Group's information.
 *
 * \param[in]       pOldNextHop points to the next-hop to be replaced.
 *
 * \param[in]       pNewNextHop points to the next-hop which is to replace
 *                  oldNextHop.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000ReplaceECMPGroupNextHop(fm_int           sw,
                                         fm_intEcmpGroup *pEcmpGroup,
                                         fm_intNextHop *  pOldNextHop,
                                         fm_intNextHop *  pNewNextHop)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm10000_EcmpGroup *pEcmpGroupExt;
    fm10000_NextHop *  pNextHopExt;
    fm_uint64 *        nextHopData;
    fm_uint16          groupBaseOffset;
    fm_int             arpIndex;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pGroup=%p, pOldNextHop=%p, pNewNextHop=%p\n",
                 sw,
                 (void *) pEcmpGroup,
                 (void *) pOldNextHop,
                 (void *) pNewNextHop );

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    nextHopData = 0;


    /* argument validation */
    if (pEcmpGroup  == NULL ||
        pOldNextHop == NULL ||
        pNewNextHop == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pEcmpGroupExt = (fm10000_EcmpGroup*)(pEcmpGroup->extension);
        pNextHopExt = (fm10000_NextHop*)(pOldNextHop->extension);

        if (pEcmpGroupExt == NULL || pNextHopExt == NULL)
        {
            err = FM_ERR_NOT_FOUND;
        }
        else if (pNewNextHop->extension != NULL)
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "NewNextHop already has an extension, groupId=%d\n",
                         pEcmpGroup->groupId);
        }
        else
        {
            /* move the extension from old next-hop to the new one */
            pNewNextHop->extension = pNextHopExt;
            pNextHopExt->pParent = pNewNextHop;
            pOldNextHop->extension = NULL;

            err = UpdateNextHopData(sw, pEcmpGroup, pNewNextHop);

            if (err == FM_OK)
            {
                groupBaseOffset = GetArpBlockOffset(sw, pEcmpGroupExt->arpBlockHandle);

                /* calcule NextHop absolute offset */
                arpIndex = groupBaseOffset + pNextHopExt->arpBlkRelOffset;

                /* validate that the absolute index is OK */
                if (arpIndex > 0 &&
                    arpIndex < FM10000_ARP_TAB_SIZE)
                {
                    err = switchPtr->WriteUINT64(sw,
                                                 FM10000_ARP_TABLE(arpIndex,0),
                                                 pNextHopExt->arpData[0]);
                }
                else
                {
                    /* log the error and continue */
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "Invalid ARP index: EcmpGroupId=%d, baseIndex=%d, arpIndex=%d, relativeIndex=%d\n",
                                 pEcmpGroup->groupId,
                                 groupBaseOffset,
                                 arpIndex,
                                 pNextHopExt->arpBlkRelOffset);
                }
            }
        }
    }
    
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000ReplaceECMPGroupNextHop */




/*****************************************************************************/
/** fm10000SetECMPGroupNextHops
 * \ingroup intNextHopEcmp
 *
 * \desc            Stores one or more next-hops for a fixed-size ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pEcmpGroup points to the ECMP Group's information.
 *
 * \param[in]       firstIndex is the index of the first next-hop in the
 *                  ECMP group which is to be updated.
 *
 * \param[in]       numNextHops contains the number of next-hops to be updated.
 *
 * \param[in]       pNextHopList points to an array of next hops to be updated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if a function parameter is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch,
 *                  if the ECMP group is not a fixed-size ECMP group, or if
 *                  this feature is not supported on the switch.
 *
 *****************************************************************************/
fm_status fm10000SetECMPGroupNextHops(fm_int           sw,
                                      fm_intEcmpGroup *pEcmpGroup,
                                      fm_int           firstIndex,
                                      fm_int           numNextHops,
                                      fm_ecmpNextHop * pNextHopList)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm_int             arpIndex;
    fm_intNextHop *    pNextHop;
    fm10000_NextHop *  pNextHopExt;
    fm10000_EcmpGroup *pEcmpGroupExt;
    fm_int             index;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, firstIndex=%d, numNextHops=%d, pNextHopList=%p\n",
                  sw,
                  (void *) pEcmpGroup,
                  firstIndex,
                  numNextHops,
                  (void *) pNextHopList );

    err = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);

    if ( pEcmpGroup == NULL      ||
         pNextHopList == NULL    ||
         firstIndex < 0          ||
         numNextHops < 0 )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* verify that the range of firtIndex and numNextHop are consistent */
        if (firstIndex + numNextHops > pEcmpGroup->nextHopCount)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                         "Invalid nextHop list: groupId=%d, firstIndex=%d, numNextHops=%d, group NextHop Count=%d\n",
                         pEcmpGroup->groupId,
                         firstIndex,
                         numNextHops,
                         pEcmpGroup->nextHopCount);

            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            pEcmpGroupExt = pEcmpGroup->extension;

            TAKE_REG_LOCK(sw);

            for (index = 0 ; index < numNextHops ; index++)
            {
                pNextHop = pEcmpGroup->nextHops[firstIndex + index];
                pNextHopExt = pNextHop->extension;

                /* remove and reinsert the updated next hop */
                err = fmDeleteArpNextHopFromTrees(sw, pNextHop);

                if (err == FM_OK)
                {
                    /* Now rebuild the next-hop record with the updated information. */
                    err = fmInitializeNextHop(sw,
                                              pEcmpGroup,
                                              pNextHop,
                                              pNextHopList + index);
                }

                if (err == FM_OK)
                {
                    /* Update hardware-specific fields from the next-hop data */
                    err = UpdateNextHopData(sw, pEcmpGroup, pNextHop);

                    if (err == FM_OK)
                    {
                        /* arp index is the addition of the block base offset and the
                         * next hop relative offset */
                        arpIndex = GetArpBlockOffset(sw, pNextHopExt->arpBlkHndl);

                        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                                      "Adding NextHop data to ARP table, nextHop=%p, baseIndex=%d, relIndex=%d\n",
                                      (void *) pNextHopExt->pParent,
                                      arpIndex,
                                      pNextHopExt->arpBlkRelOffset );
                        
                        arpIndex += pNextHopExt->arpBlkRelOffset;

                        err = switchPtr->WriteUINT64(sw,
                                                     FM10000_ARP_TABLE(arpIndex,0),
                                                     pNextHopExt->arpData[0]);
                    }


                }
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                 "Cannot update ECMP group NextHop, groupId=%d, nextHop index=%d\n",
                                 pEcmpGroup->groupId,
                                 firstIndex + index);
                }

            }   /* end for (index = 0 ; index < numNextHops ; index++) */

            DROP_REG_LOCK(sw);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000SetECMPGroupNextHops */




/*****************************************************************************/
/** fm10000UpdateNextHopMulticast
 * \ingroup intRoute
 *
 * \desc            Updates a next-hop when its multicast information has
 *                  been added, changed, or removed.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       ecmpGroupId is the ecmp group to update.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the the ecmp group does not point
 *                  to a multicast group or if the ecmp group has no valid ARP
 *                  entry.
 * \return          FM_ERR_INVALID_INDEX if the ARP entry index is invalid.
 *
 *****************************************************************************/
fm_status fm10000UpdateNextHopMulticast(fm_int sw, fm_int ecmpGroupId)
{
    fm_status          status;
    fm_switch         *switchPtr;
    fm10000_EcmpGroup *ecmpGroup;
    fm_uint16          groupBaseOffset;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, ecmpGroupId=%d\n",
                  sw,
                  ecmpGroupId );

    switchPtr  = GET_SWITCH_PTR(sw);

    ecmpGroup = switchPtr->ecmpGroups[ecmpGroupId]->extension;
    
    if (switchPtr->ecmpGroups[ecmpGroupId]->mcastGroup == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    status = BuildGlortArp(sw, ecmpGroup, 0);

    if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);
    }

    if (ecmpGroup->arpBlockHandle == FM10000_ARP_BLOCK_INVALID_HANDLE)
    {
         FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* get the base offset of the block */
    groupBaseOffset = GetArpBlockOffset(sw, ecmpGroup->arpBlockHandle);

    if ((groupBaseOffset <= 0) &&
        (groupBaseOffset >= FM10000_ARP_TAB_SIZE))
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_INDEX);
    }

    /* Write the register */
    status = switchPtr->WriteUINT64(sw,
                                    FM10000_ARP_TABLE(groupBaseOffset, 0),
                                    ecmpGroup->glortArpData);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    
}   /* end fm10000UpdateNextHopMulticast */




/*****************************************************************************/
/** fm10000UpdateNextHop
 * \ingroup intRoute
 *
 * \desc            Updates a next-hop when its ARP information has
 *                  been added, changed, or removed.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pNextHop points to the next-hop record.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if a function parameter is invalid.
 * \return          FM_ERR_UNSUPPORTED if ecmp group is a wide group.
 
 *****************************************************************************/
fm_status fm10000UpdateNextHop(fm_int         sw,
                               fm_intNextHop *pNextHop)
{
    fm_status        err;
    fm_switch *      switchPtr;
    fm10000_NextHop *pNextHopExt;
    fm_uint16        arpBlockHandle;
    fm_uint16        arpOffset;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, pNextHop=%p\n",
                  sw,
                  (void *) pNextHop );

    err = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);

    if (pNextHop == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pNextHop->ecmpGroup->wideGroup)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        pNextHopExt = pNextHop->extension;

        if (pNextHopExt == NULL)
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "NULL next-hop extension\n");
        }    
        else 
        {
            arpBlockHandle = pNextHopExt->arpBlkHndl;
            arpOffset    = GetArpBlockOffset(sw, arpBlockHandle);
            
            switch (pNextHop->nextHop.type)
            {
                case FM_NEXTHOP_TYPE_ARP:
                    err = BuildNextHopArp(sw, pNextHopExt);
                    break;
                    
                default:
                    err = FM_ERR_UNSUPPORTED;
                    break;
            }
        }

        if (err == FM_OK)
        {
            arpOffset += pNextHopExt->arpBlkRelOffset;

            if (arpOffset > 0)
            {
                FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                              "nexthop %p arpData now %" FM_FORMAT_64 "X, arpIndex=%d\n",
                              (void *) pNextHopExt,
                              pNextHopExt->arpData[0],
                              arpOffset );

                /* write to the register */
                err = switchPtr->WriteUINT64(sw,
                                             FM10000_ARP_TABLE(arpOffset, 0),
                                             pNextHopExt->arpData[0]);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000UpdateNextHop */




/*****************************************************************************/
/** fm10000GetECMPGroupNextHopIndexRange
 * \ingroup intRoute
 *
 * \desc            returns the first and last positions in the next-hop
 *                  table that are used by this ECMP group.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the ECMP group structure.
 *
 * \param[out]      pFirstArpIndex points to caller-provided storage into which
 *                  the first used position will be written.
 *
 * \param[out]      pLastArpIndex points to caller-provided storage into which
 *                  the last used position will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetECMPGroupNextHopIndexRange(fm_int           sw,
                                               fm_intEcmpGroup *pEcmpGroup,
                                               fm_int *         pFirstArpIndex,
                                               fm_int *         pLastArpIndex)
{
    fm_status           err;
    fm10000_switch *    pSwitchExt;
    fm10000_EcmpGroup * pEcmpGroupExtTmp;
    fm_uint16           arpBlockOffset;
    fm_uint16           arpBlockLength;
    

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, pFirstArpIndex=%p, pLastArpIndex=%p\n",
                  sw,
                  (void *) pEcmpGroup,
                  (void *) pFirstArpIndex,
                  (void *) pLastArpIndex );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    arpBlockOffset = FM10000_ARP_BLOCK_INVALID_OFFSET;

    if (pEcmpGroup == NULL      ||
        pFirstArpIndex == NULL  ||
        pLastArpIndex == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
              pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( pEcmpGroup->extension == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (pEcmpGroupExtTmp != NULL)
        {
            arpBlockOffset = GetArpBlockOffset(sw, pEcmpGroupExtTmp->arpBlockHandle);
            arpBlockLength = GetArpBlockLength(sw, pEcmpGroupExtTmp->arpBlockHandle);
        }

        if (arpBlockOffset != FM10000_ARP_BLOCK_INVALID_OFFSET)
        {
            *pFirstArpIndex = arpBlockOffset;
            *pLastArpIndex  = arpBlockOffset + arpBlockLength - 1;
        }
        else
        {
            err = FM_ERR_NOT_FOUND;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetECMPGroupNextHopIndexRange */




/*****************************************************************************/
/** fm10000GetNextHopUsed
 * \ingroup intNextHopArp
 *
 * \desc            Retrieves the "used" flag for the given next hop.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pNextHop points to the nexthop entry to be accessed.
 *
 * \param[out]      pUsed points to the location to receive the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetNextHopUsed(fm_int         sw,
                                fm_intNextHop *pNextHop,
                                fm_bool *      pUsed,
                                fm_bool        resetFlag)
{
    fm_status        err;
    fm10000_switch * pSwitchExt;
    fm_uint16        arpEntryIndex;
    fm10000_NextHop *pArpNextHopExt;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pNextHop=%p, pUsed=%p, resetFlag=%s\n",
                  sw,
                  (void *) pNextHop,
                  (void *) pUsed,
                  resetFlag ? "TRUE" : "FALSE" );

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pNextHop == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( (pArpNextHopExt = (fm10000_NextHop*)pNextHop->extension) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        if ( pArpNextHopExt->arpBlkHndl == FM10000_ARP_BLOCK_INVALID_HANDLE )
        {
            err = FM_ERR_NOT_FOUND;
        }
        else
        {
            /* get the absolute offset of the entries associated to this group */
            arpEntryIndex = GetArpBlockOffset(sw, pArpNextHopExt->arpBlkHndl);

            if (arpEntryIndex != FM10000_ARP_BLOCK_INVALID_OFFSET)
            {
                /* add the relative offset of the specified nextHop */
                arpEntryIndex += pArpNextHopExt->arpBlkRelOffset;

                err = fm10000GetArpEntryUsedStatus(sw,
                                                   arpEntryIndex,
                                                   resetFlag,
                                                   pUsed);
            }
            else
            {
                /* ERROR: ARP block control not defined or not found */
                err = FM_ERR_NOT_FOUND;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetNextHopUsed */




/*****************************************************************************/
/** fm10000GetNextHopIndexUsed
 * \ingroup intRoute
 *
 * \desc            Retrieves the "used" flag for the specified next hop
 *                  entry. If "reset" isis used that the trap code is valid, if it is unset,
 *                  it will be set to the default value.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       index is the index of the next-hop record to be checked.
 *
 * \param[out]      pUsed points to caller-provided storage into which the
 *                  used flag will be returned. May be NULL.
 *
 * \param[in]       resetFlag indicates, when it is TRUE, that the "used" flag
 *                  must be reset.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERROR_INVALID_PARAMETER if the arpIndex is out of
 *                   range
 *
 *****************************************************************************/
fm_status fm10000GetNextHopIndexUsed(fm_int   sw,
                                     fm_int   index,
                                     fm_bool *pUsed,
                                     fm_bool  resetFlag)
{
    fm_status err;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, index=%d, pUsed=%p, resetFlag=%s\n",
                  sw,
                  index,
                  (void *) pUsed,
                  resetFlag ? "TRUE" : "FALSE" );

    err = fm10000GetArpEntryUsedStatus(sw, index, resetFlag, pUsed);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetNextHopIndexUsed */




/*****************************************************************************/
/** fm10000CreateECMPGroup
 * \ingroup intRoute
 *
 * \desc            Creates a low level, chip specific extension of an ECMP
 *                  Group.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the ECMP Group's information.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 * \return          FM_ERR_UNSUPPORTED if specified ECMP group's next-hops
 *                  are wide next-hops.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_VALUE if the number of next-hop entries is
 *                  invalid for a fixed size ECMP group. Valid values for fixed
 *                  size ECMP groups are: (1..16,32,64,128,256,512,1024,2048,
 *                  4096).
 *
 *****************************************************************************/
fm_status fm10000CreateECMPGroup(fm_int           sw,
                                 fm_intEcmpGroup *pEcmpGroup)
{
    fm_status          err;
    fm10000_EcmpGroup *pEcmpGroupExtTmp;
    fm_status          local_err;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p\n",
                  sw,
                  (void *) pEcmpGroup );

    /* argument validation */
    if (pEcmpGroup == NULL ||
        pEcmpGroup->maxNextHops <= 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pEcmpGroup->wideGroup)
    {
        /* wide entries are not supported */
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                     "Wide entries are not supported: %s\n",
                     fmErrorMsg(err));
    }
    else
    {
        /* allocate basic resources for the low level extension of the ECMP group */
        err = AllocateEcmpGroup(sw, &pEcmpGroupExtTmp);

        if (err == FM_OK)
        {
            pEcmpGroupExtTmp->pParent = pEcmpGroup;
            pEcmpGroup->extension = pEcmpGroupExtTmp;

            /* determine the type of ECMP group, see fm10000_EcmpGroupType */
            err = SetEcmpGroupType(sw, pEcmpGroupExtTmp);
            
            if (err == FM_OK)
            {
                /* allocate specific resources according to the ecmp group type */
                err = AllocateEcmpGroupSpecResources(sw, pEcmpGroupExtTmp);
            }

            if (err == FM_OK && pEcmpGroup->fixedSize == TRUE)
            {
                /* in the case of a "fixed size" ECMP group, reserve a block of
                *  ARP entries and create the associated low level nextHop
                *  structures                                                   */
                err = AllocateFixedSizeEcmpGroupResources(sw, pEcmpGroupExtTmp);
            }

            if (err != FM_OK)
            {
                /* error case: release all allocated resources */

                /* Note that fm10000FreeECMPGroup uses the pointer to the parent ECMP group */
                local_err = fm10000FreeECMPGroup(sw, pEcmpGroup);
                if (local_err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                 "Failed to free ECMP group: %s\n",
                                 fmErrorMsg(local_err));
                }
                pEcmpGroupExtTmp = NULL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000CreateECMPGroup */




/*****************************************************************************/
/** fm10000DeleteECMPGroup
 * \ingroup intRoute
 *
 * \desc            Deletes an ECMP Group at the hardware level.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the high level ECMP Group structue.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 *
 *****************************************************************************/
fm_status fm10000DeleteECMPGroup(fm_int           sw,
                                 fm_intEcmpGroup *pEcmpGroup)
{
    fm_status          err;
    fm_bool            groupMayBeDeleted;
    fm_uint16          arpBlkHndl;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p\n",
                  sw,
                  (void *) pEcmpGroup );

    err = FM_OK;
    groupMayBeDeleted = TRUE;

    /* argument validation */
    if (pEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pEcmpGroup->extension == NULL)
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        /* check is the group is being referenced by another subsystem */
        err = fm10000ValidateDeleteEcmpGroup(sw,
                                             pEcmpGroup->groupId,
                                             &groupMayBeDeleted);

        if (err != FM_OK || groupMayBeDeleted == FALSE)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "ECMP group is being referenced and it cannot be deleted, groupId=%d\n",
                         pEcmpGroup->groupId);

            /* return an error code but do not override the one returned by fm10000ValidateDeleteEcmpGroup */
            if (err == FM_OK)
            {
                err = FM_FAIL;
            }
        }
        else
        {
            /* release all hardware resources */
            err = GetEcmpGroupArpBlockHandle(sw, pEcmpGroup, &arpBlkHndl);
            if (err == FM_OK && arpBlkHndl != FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                err = fm10000FreeArpBlock(sw,
                                          FM10000_ARP_CLIENT_ECMP,
                                          arpBlkHndl);
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "Cannot release ARP block, ecmpGrouId=%d, handle=%d\n",
                                 pEcmpGroup->groupId,
                                 arpBlkHndl);
                }
            }

            /* release nextHop extensions */
            err = ReleaseAllNextHopExtensions(sw, pEcmpGroup);

            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                             "Cannot release NextHop extensions, ecmpGrouId=%d\n",
                             pEcmpGroup->groupId);
            }

            /* release ECMP extension */
            fmFree(pEcmpGroup->extension);
            pEcmpGroup->extension = NULL;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fm10000DeleteECMPGroup */




/*****************************************************************************/
/** fm10000FreeECMPGroup
 * \ingroup intRoute
 *
 * \desc            Frees the resources allocated by a chip level extension
 *                  of an ECMP group. No notifications are send when an ECMP
 *                  group is being deleted.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the parent ECMP Group.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
fm_status fm10000FreeECMPGroup(fm_int           sw,
                               fm_intEcmpGroup *pEcmpGroup)
{
    fm_status           err;
    fm_status           localErr;
    fm_int              index;
    fm10000_EcmpGroup * pEcmpGroupExt;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p\n",
                  sw,
                  (void *) pEcmpGroup );

    err = FM_OK;
    localErr = FM_OK;

    /* argument validation */
    if (pEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;  
    }
    else
    {
        if (pEcmpGroup->extension != NULL)
        {
            /* relesase ARP entries if an ARP block been allocated */
            pEcmpGroupExt = (fm10000_EcmpGroup*) pEcmpGroup->extension;
            if (pEcmpGroupExt->arpBlockHandle != FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                localErr = fm10000FreeArpBlock(sw,
                                               FM10000_ARP_CLIENT_ECMP,
                                               pEcmpGroupExt->arpBlockHandle);
                if (localErr != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "Cannot release ARP block, handle=%d\n",
                                 pEcmpGroupExt->arpBlockHandle);
                }
            }
            /* free the ECMP chip level extension */
            fmFree(pEcmpGroup->extension);
            pEcmpGroup->extension = NULL;
        }

        /* release the chip level extensions of the nextHops */
        index = (pEcmpGroup->nextHopCount - 1);

        while (index >= 0)
        {
            if ( (pEcmpGroup->nextHops[index] != NULL) &&
                 ((pEcmpGroup->nextHops[index])->extension != NULL) )
            {
                fmFree((pEcmpGroup->nextHops[index])->extension);
                (pEcmpGroup->nextHops[index])->extension = NULL;
            }
            index--;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000FreeECMPGroup */




/*****************************************************************************/
/** fm10000UpdateEcmpGroup
 * \ingroup intRoute
 *
 * \desc            Updates an ECMP group when one or more next-hops in
 *                  the group have changed active state, either becoming
 *                  active or inactive.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEcmpGroup points to the ECMP Group's information.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateEcmpGroup(fm_int           sw,
                                 fm_intEcmpGroup *pEcmpGroup)
{
    /* marka TBC */
    FM_NOT_USED(sw);
    FM_NOT_USED(pEcmpGroup);


    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fm10000UpdateEcmpGroup */




/*****************************************************************************/
/** fm10000GetECMPGroupARPUsed
 * \ingroup intNextHopArp
 *
 * \desc            Retrieves the ARP "used" flag for an ECMP Group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pEcmpGroup points to the ECMP group.
 *
 * \param[out]      pUsed points to the location to receive the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 * \return          FM_ERR_UNSUPPORTED if feature is not supported.
 * \return          FM_ERR_NOT_FOUND if ECMP group cannot be found.
 *
 *****************************************************************************/
fm_status fm10000GetECMPGroupARPUsed(fm_int           sw,
                                     fm_intEcmpGroup *pEcmpGroup,
                                     fm_bool *        pUsed,
                                     fm_bool          resetFlag)
{
    fm_status              err;
    fm10000_switch *       pSwitchExt;
    fm10000_EcmpGroup *    pEcmpGroupExtTmp;
    fm_bool                usedTmp;
    fm_int                 arpIndex;
    fm_int                 loopCount;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pEcmpGroup=%p, pUsed=%p, resetFlag=%s\n",
                  sw,
                  (void *) pEcmpGroup,
                  (void *) pUsed,
                  resetFlag ? "TRUE" : "FALSE" );

    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    arpIndex = 0;
    loopCount = 0;

    if (pEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (pSwitchExt->pNextHopSysCtrl == NULL                  ||
             pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( pEcmpGroup->extension == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        /* get the associated arp handle */
        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (pEcmpGroupExtTmp != NULL)
        {
            arpIndex  = GetArpBlockOffset(sw, pEcmpGroupExtTmp->arpBlockHandle);
            loopCount = GetArpBlockLength(sw, pEcmpGroupExtTmp->arpBlockHandle);
        }

        if ( arpIndex == FM10000_ARP_BLOCK_INVALID_OFFSET )
        {
            err = FM_ERR_NOT_FOUND;
        }
        else
        {
            while (loopCount-- && err == FM_OK)
            {
                err = fm10000GetArpEntryUsedStatus(sw,
                                                   arpIndex++,
                                                   resetFlag,
                                                   &usedTmp);
                if (err == FM_OK)
                {
                    if (pUsed != NULL)
                    {
                        *pUsed |= usedTmp;

                        if (*pUsed == TRUE && resetFlag == FALSE)
                        {
                            break;
                        }
                    }
                }
            }   /* end while (loopCount--) */

        }   /* end else if ( arpIndex == FM10000_ARP_BLOCK_INVALID_OFFSET ) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetECMPGroupUsed */




/*****************************************************************************/
/** fm10000GetECMPGroupArpInfo
 * \ingroup intNextHopArp
 *
 * \desc            Retrieves ARP table parameters associated to the given
 *                  ECMP Group. This function is provided to be used by
 *                  other subsystems, such as ACLs, to get the ARP information
 *                  required to configure the FFU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ecmpGroupId is the ID of the ECMP group.
 *
 * \param[out]      pHandle points to a caller allocated storage where
 *                  this function will return the ARP block handle. May be
 *                  NULL.
 *
 * \param[out]      pIndex points to a caller allocated storage where
 *                  this function will return the block's base index.
 *
 * \param[out]      pPathCount points to a caller allocated storage where
 *                  this function will return the number of entries of the
 *                  block, if the path count type is zero, or the exponent
 *                  of 2 if the path count type is 1. In this last case,
 *                  the size of the ARP block is given equal to (2 ^ pathCount)
 *
 * \param[out]      pPathCountType points to a caller allocated storage
 *                  where this function will return the path count type.
 *                  When it is zero, the effective ARP index is:
 *                    ARP index = ARP index + (hash[11:0] * PathCount)/4096
 *                  If path count type is 1, the ARP index is:
 *                    ARP index = ARP index + hash[11:0] & ((1<<PathCount)-1)
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments.
 * \return          FM_ERR_UNSUPPORTED if feature is not supported.
 * \return          FM_ERR_NOT_FOUND if ECMP group cannot be found.
 *
 *****************************************************************************/
fm_status fm10000GetECMPGroupArpInfo(fm_int     sw,
                                     fm_int     ecmpGroupId,
                                     fm_uint16 *pHandle,
                                     fm_int *   pIndex,
                                     fm_int *   pPathCount,
                                     fm_int *   pPathCountType)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm10000_switch *   pSwitchExt;
    fm_uint16          arpBlkHndl;
    fm_intEcmpGroup *  pEcmpGroup;
    fm10000_EcmpGroup *pEcmpGroupExtTmp;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, ecmpGroupId=%d, pHandle=%p, pIndex=%p, pPathCount=%p, pPathCountType=%p\n",
                  sw,
                  ecmpGroupId,
                  (void *) pHandle,
                  (void *) pIndex,
                  (void *) pPathCount,
                  (void *) pPathCountType );

    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    /* argument validation, note that pHandle may be NULL */
    if (pIndex == NULL ||
        pPathCount  == NULL ||
        pPathCountType  == NULL ||
        ecmpGroupId < 0 ||
        ecmpGroupId >= switchPtr->maxArpEntries)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( (pEcmpGroup = switchPtr->ecmpGroups[ecmpGroupId]) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (pEcmpGroupExtTmp == NULL)
        {
            err = FM_ERR_NOT_FOUND;
        }
        else
        {
            arpBlkHndl = pEcmpGroupExtTmp->arpBlockHandle;

            if (pHandle != NULL)
            {
                *pHandle = arpBlkHndl;
            }

            *pIndex = GetArpBlockOffset(sw, arpBlkHndl);
            
            err = fm10000GetArpTablePathCountParameters(GetArpBlockLength(sw, arpBlkHndl),
                                                        pPathCount,
                                                        pPathCountType);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000GetECMPGroupArpInfo */




/*****************************************************************************/
/** fm10000RegisterEcmpClient
 * \ingroup intNextHop
 *
 * \desc            Registers a client for the ECMP group specified by
 *                  ecmpGroupId. The list of clients is used to send
 *                  notifications when the position of the associated ARP
 *                  block changes because of a modification in the size of
 *                  the group changes or due to an ARP table defragmentation.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       ecmpGroupId is the ECMP group ID.
 * 
 * \param[in]       newClient is the additional subsystem that requires to be
 *                  notified if the ECMP group changes.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the arguments have errors
 * \return          FM_ERR_UNSUPPORTED if feature is not supported
 * \return          FM_ERR_NOT_FOUND if ECMP group cannot be found.
 *
 *****************************************************************************/
fm_status fm10000RegisterEcmpClient(fm_int                   sw,
                                    fm_int                   ecmpGroupId,
                                    fm10000_EcmpGroupClient  newClient)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm_intEcmpGroup *  pEcmpGroup;
    fm10000_EcmpGroup *pEcmpGroupExtTmp;
    fm_int             index;
    fm_int             freeClientEntry;
    fm_bool            alreadyRegistered;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, ecmpGroupId=%d, newClient=%d\n",
                      sw,
                      ecmpGroupId,
                      newClient );

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    freeClientEntry = -1;
    alreadyRegistered = FALSE;

    /* argument validation */
    if (newClient <= FM10000_ECMP_GROUP_CLIENT_NONE   ||
        newClient >= FM10000_ECMP_GROUP_CLIENT_MAX    ||
        ecmpGroupId < 0                               ||
        ecmpGroupId >= switchPtr->maxArpEntries) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( (pEcmpGroup = switchPtr->ecmpGroups[ecmpGroupId]) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (pEcmpGroupExtTmp != NULL)
        {
            for (index = 0; index < FM10000_MAX_ECMP_GROUP_CLIENTS; index++)
            {
                if ((pEcmpGroupExtTmp->ecmpClients)[index] == newClient)
                {
                    /* client is already registered */
                    alreadyRegistered = TRUE;
                    break;
                }
                else if ( (freeClientEntry == -1)  &&
                          ((pEcmpGroupExtTmp->ecmpClients)[index] == FM10000_ARP_CLIENT_NONE) )
                {
                    /* remember the first free entry */
                    freeClientEntry = index;
                }
            }
            if (err == FM_OK && alreadyRegistered == FALSE)
            {
                if (freeClientEntry >= 0)
                {
                    /* register the new client */
                    (pEcmpGroupExtTmp->ecmpClients)[freeClientEntry] = newClient;
                }
                else
                {
                    /* ERROR: there is no room in the table */
                    err = FM_FAIL;
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "ECMP client tab is full\n");
                }
            }
        }
        else
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "ECMP extension not found\n");
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RegisterEcmpClient */





/*****************************************************************************/
/** fm10000UnregisterEcmpClient
 * \ingroup intNextHop
 *
 * \desc            Unregisters a client for the ECMP group specified by
 *                  ecmpGroupId.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       ecmpGroupId is the ECMP group ID.
 * 
 * \param[in]       client is the subsystem that requires to be removed from
 *                  the list of clients.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UnregisterEcmpClient(fm_int                  sw,
                                      fm_int                  ecmpGroupId,
                                      fm10000_EcmpGroupClient client)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm_intEcmpGroup *  pEcmpGroup;
    fm10000_EcmpGroup *pEcmpGroupExtTmp;
    fm_int             index;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, ecmpGroupId=%d, client=%d\n",
                      sw,
                      ecmpGroupId,
                      client );

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* argument validation */
    if (client <= FM10000_ECMP_GROUP_CLIENT_NONE   ||
        client >= FM10000_ECMP_GROUP_CLIENT_MAX    ||
        ecmpGroupId < 0                            ||
        ecmpGroupId >= switchPtr->maxArpEntries) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( (pEcmpGroup = switchPtr->ecmpGroups[ecmpGroupId]) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (pEcmpGroupExtTmp != NULL)
        {
            for (index = 0; index < FM10000_MAX_ECMP_GROUP_CLIENTS; index++)
            {
                if ((pEcmpGroupExtTmp->ecmpClients)[index] == client)
                {
                    (pEcmpGroupExtTmp->ecmpClients)[index] = FM10000_ECMP_GROUP_CLIENT_NONE;
                    break;
                }
            }

            if (index ==  FM10000_ECMP_GROUP_CLIENT_MAX)
            {
                /* ERROR: client not found */
                err = FM_FAIL;
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                             "ECMP client not found\n");
            }   
        }
        else
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "ECMP extension not found\n");
        }        
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000UnregisterEcmpClient */




/*****************************************************************************/
/** fm10000NotifyEcmpGroupChange
 * \ingroup intNextHop
 *
 * \desc            Notifies ECMP group clients of changes in the associated
 *                  ARP block of entries. This function is typically called
 *                  when a nexthop was added or deleted or when the ARP table
 *                  is being defragmented. A client must be registered in 
 *                  order to receive notifications.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       ecmpGroupId is the ECMP group ID.
 * 
 * \param[in]       oldIndex is the old ARP index used by this ECMP group. It
 *                  is only used for validation.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments
 * \return          FM_ERR_UNSUPPORTED if a required feature is not supported
 *                   or not initialized.
 * \return          FM_ERR_NOT_FOUND if the specified ECMP group was not found
 * \return          FM_FAIL on others execution errors
 *
 *****************************************************************************/
fm_status fm10000NotifyEcmpGroupChange(fm_int      sw,
                                       fm_int      ecmpGroupId,
                                       fm_int      oldIndex)
{
    fm_status             err;
    fm_switch *           switchPtr;
    fm10000_switch *      pSwitchExt;
    fm_intEcmpGroup *     pEcmpGroup;
    fm10000_EcmpGroup *   pEcmpGroupExtTmp;
    fm_int                arpBlkHndl;
    fm_uint16             blkOffset;
    fm_uint16             blkLength;
    fm_int                pathCount;
    fm_int                pathCountType;
    fm_int                index;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, ecmpGroupId=%d, oldIndex=%d\n",
                      sw,
                      ecmpGroupId,
                      oldIndex );

    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    blkOffset = 0;
    blkLength = 0;
    pathCount = 0;
    pathCountType = 0;


    /* argument validation */
    if (ecmpGroupId < 0                         ||
        ecmpGroupId >= switchPtr->maxArpEntries) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL                          ||
             pSwitchExt->pNextHopSysCtrl == NULL                    ||
             pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( (pEcmpGroup = switchPtr->ecmpGroups[ecmpGroupId]) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (oldIndex >= FM10000_ARP_TAB_SIZE)
        {
            oldIndex = 0;
        }

        if (pEcmpGroupExtTmp != NULL)
        {
            /* get parameters from the new ARP block */
            arpBlkHndl = pEcmpGroupExtTmp->arpBlockHandle;
            if (arpBlkHndl != FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                blkOffset = GetArpBlockOffset(sw, arpBlkHndl);
                blkLength = GetArpBlockLength(sw, arpBlkHndl);

                err = fm10000GetArpTablePathCountParameters(blkLength,
                                                            &pathCount,
                                                            &pathCountType);
                if (err != FM_OK)
                {
                      FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                   "Invalid pathCount and/or pathCountType for ECMP group=%d err=%d %s\n",
                                   ecmpGroupId,
                                   err,
                                   fmErrorMsg(err));
                }
            }

            if (err == FM_OK)
            {
                /* update routes that use this ECMP group */
                err = fm10000UpdateEcmpRoutes(sw,
                                              ecmpGroupId,
                                              blkOffset,
                                              pathCount,
                                              pathCountType);
                if (err != FM_OK)
                {
                      FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                   "UpdateEcmpRoutes failed for ECMP group=%d err=%d %s\n",
                                   ecmpGroupId,
                                   err,
                                   fmErrorMsg(err));
                }
            }

            if (err == FM_OK)
            {
                for (index = 0; index < FM10000_MAX_ECMP_GROUP_CLIENTS && err == FM_OK; index++)
                {
                    switch ((pEcmpGroupExtTmp->ecmpClients)[index])
                    {
                        case FM10000_ECMP_GROUP_CLIENT_ACL: 
                          err = fm10000NotifyAclEcmpChange(sw,
                                                           ecmpGroupId,
                                                           oldIndex,
                                                           blkOffset,
                                                           pathCount,
                                                           pathCountType);
                            if (err != FM_OK)
                            {
                                /* errors returned by ACL are reported, but not forwarded
                                 * because ACL is not the owner of the ECMP group and all
                                 * parameters have been previously validated.  */

                                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                             "ACL returned an error upon ECMP change notification\n");
                                err = FM_OK;
                            }
                            break;
                        case FM10000_ECMP_GROUP_CLIENT_NONE:

                            break;
                        default:
                            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                         "Invalid ECMP client\n");
                    }
                }   /* end for (index = 0; ....) */
            }
            else
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                             "Invalid pathCount and/or pathCountType for ECMP group=%d err=%d %s\n",
                             ecmpGroupId,
                             err,
                             fmErrorMsg(err));
            }
        }
        else
        {
            err = FM_FAIL;
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "ECMP extension not found\n");
        }   /* end else if (pEcmpGroupExtTmp != NULL) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000NotifyEcmpGroupChange */




/*****************************************************************************/
/** fm10000ValidateDeleteEcmpGroup
 * \ingroup intNextHop
 *
 * \desc            Checks if an ECMP group client may be deleted or not.If at
 *                  least a client has a reference to the group, it cannot be
 *                  deleted.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       ecmpGroupId is the ECMP group ID.
 * 
 * \param[out]      pMayBeDeleted pointer to a caller allocated storage where
 *                  this function will return TRUE if the group may be deleted
 *                  or FALSE otherwise.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments
 * \return          FM_ERR_UNSUPPORTED if a required feature is not supported
 *                   or not initialized.
 * \return          FM_ERR_NOT_FOUND if the specified ECMP group was not found
 * \return          FM_FAIL on others execution errors
 *
 *****************************************************************************/
fm_status fm10000ValidateDeleteEcmpGroup(fm_int   sw,
                                         fm_int   ecmpGroupId,
                                         fm_bool *pMayBeDeleted)
{
    fm_status          err;
    fm_switch *        switchPtr;
    fm10000_switch *   pSwitchExt;
    fm_intEcmpGroup *  pEcmpGroup;
    fm10000_EcmpGroup *pEcmpGroupExtTmp;
    fm_int             index;
    fm_bool            referenced;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, ecmpGroupId=%d, pMayBeDeleted=%p\n",
                      sw,
                      ecmpGroupId,
                      (void *) pMayBeDeleted );

    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;
    referenced = FALSE;

    /* argument validation */
    if (ecmpGroupId < 0                         ||
        ecmpGroupId >= switchPtr->maxArpEntries ||
        pMayBeDeleted == NULL) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL      ||
             pSwitchExt->pNextHopSysCtrl == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( (pEcmpGroup = switchPtr->ecmpGroups[ecmpGroupId]) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        *pMayBeDeleted = TRUE;

        pEcmpGroupExtTmp = (fm10000_EcmpGroup*)pEcmpGroup->extension;

        if (pEcmpGroupExtTmp != NULL)
        {

            for (index = 0; index < FM10000_MAX_ECMP_GROUP_CLIENTS && err == FM_OK; index++)
            {
                switch ((pEcmpGroupExtTmp->ecmpClients)[index])
                {
                    case FM10000_ECMP_GROUP_CLIENT_ACL: 
                        err = fm10000ValidateAclEcmpId(sw,
                                                       ecmpGroupId,
                                                       &referenced,
                                                       NULL,
                                                       NULL);
                        if (err == FM_OK)
                        {
                            if (referenced == TRUE)
                            {
                                *pMayBeDeleted = FALSE;

                                /* quit the loop */
                                index = FM10000_MAX_ECMP_GROUP_CLIENTS;
                            }
                        }
                        else
                        {
                            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                         "ACL returned an error validating ECMP group\n");
                            /* quit the loop */
                            index = FM10000_MAX_ECMP_GROUP_CLIENTS;
                        }
                        break;
                    case FM10000_ECMP_GROUP_CLIENT_NONE:

                        break;
                    default:
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                     "Invalid ECMP client\n");
                }
            }   /* end for (index = 0; ....) */

        }
        else
        {
            err = FM_FAIL;

            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "ECMP extension not found\n");
        }   /* end else if (pEcmpGroupExtTmp != NULL) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000NotifyEcmpGroupChange */




/*****************************************************************************/
/** fm10000ValidateEcmpGroupState
 * \ingroup intNextHop
 *
 * \desc            Checks if an ECMP group is usable and if it is attached to
 *                  a valid ARP block. 
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       ecmpGroupId is the ECMP group ID.
 * 
 * \param[out]      pValid pointer to a caller allocated storage where this
 *                  function will return TRUE if the group is usable and it
 *                  has attached a valid ARP block or FALSE otherwise.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT on bad arguments
 * \return          FM_ERR_UNSUPPORTED if a required feature is not supported
 *                   or not initialized.
 * \return          FM_ERR_NOT_FOUND if the specified ECMP group was not found
 *
 *****************************************************************************/
fm_status fm10000ValidateEcmpGroupState(fm_int   sw,
                                        fm_int   ecmpGroupId,
                                        fm_bool *pValid)
{
    fm_status        err;
    fm_switch *      switchPtr;
    fm10000_switch * pSwitchExt;
    fm_intEcmpGroup *pEcmpGroup;
    fm_uint16        arpBlkHndl;
    fm_uint16        blkOffset;
    fm_uint16        blkLength;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, ecmpGroupId=%d, pValid=%p\n",
                      sw,
                      ecmpGroupId,
                      (void *) pValid );

    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    /* argument validation */
    if (ecmpGroupId < 0                         ||
        ecmpGroupId >= switchPtr->maxArpEntries ||
        pValid == NULL) 
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL      ||
             pSwitchExt->pNextHopSysCtrl == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if ( (pEcmpGroup = switchPtr->ecmpGroups[ecmpGroupId]) == NULL )
    {
        err = FM_ERR_NOT_FOUND;
    }
    else
    {
        *pValid = FALSE;

        if (pEcmpGroup->isUsable)
        {
            err = GetEcmpGroupArpBlockHandle(sw, pEcmpGroup, &arpBlkHndl);

            if (err == FM_OK)
            {
                blkOffset = GetArpBlockOffset(sw, arpBlkHndl);
                blkLength = GetArpBlockLength(sw, arpBlkHndl);
                if (blkOffset != FM10000_ARP_BLOCK_INVALID_OFFSET && blkLength > 0)
                {
                    *pValid = TRUE;
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000ValidateEcmpGroupState */




/*****************************************************************************/
/** fm10000SetPriorityUnresolvedNextHopTrigger
 * \ingroup intNextHop
 *
 * \desc            Configures switch priority for unresolved next hop trigger.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       priority is the switch priority to be set in the trigger 
 *                  action. Value less than zero disables priority settings.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetPriorityUnresolvedNextHopTrigger(fm_int   sw,
                                                     fm_int   priority)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm_status           err;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw %d, priority=%d \n",
                  sw,
                  priority );

    /* Get the trigger */
    err = fm10000GetTrigger(sw,
                            FM10000_TRIGGER_GROUP_ROUTING,
                            FM10000_TRIGGER_RULE_ROUTING_UNRESOLVED_NEXT_HOP,
                            &trigCond,
                            &trigAction);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Set trigger's action and action params */
    if (priority >= 0)
    {
        trigAction.cfg.switchPriAction = FM10000_TRIGGER_SWPRI_ACTION_REASSIGN;
        trigAction.param.newSwitchPri = priority;
    }
    else
    {
        trigAction.cfg.switchPriAction = FM10000_TRIGGER_SWPRI_ACTION_ASIS;
    }

    /* Apply new trigger's action */
    err = fm10000SetTriggerAction(
                               sw,
                               FM10000_TRIGGER_GROUP_ROUTING,
                               FM10000_TRIGGER_RULE_ROUTING_UNRESOLVED_NEXT_HOP,
                               &trigAction,
                               TRUE);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end fm10000SetPriorityUnresolvedNextHopTrigger*/




/*****************************************************************************/
/** fm10000DbgPrintArpTableInfo
 * \ingroup intNextHop
 *
 * \desc            Prints basic information about the ARP table usage.
 * 
 * \param[in]       sw is the switch on which to operate.
 *
 
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgPrintArpTableInfo(fm_int  sw)
{

    fm10000DbgPrintArpFragmentationInfo(sw);

}   /* end fm10000DbgPrintArpTableInfo */




/*****************************************************************************/
/** fm10000DbgDumpArpTable
 * \ingroup intNextHop
 *
 * \desc            Displays the ARP table. If verbose is FALSE, only
 *                  entries that are not equal to zero will be included.
 *                  If verbose is TRUE, first
 *                  ''FM10000_ARP_TAB_VERBOSE_DUMP_ENTRIES'' entries will be
 *                  unconditionally included in the output.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       verbose indicates whether to include the first
 *                  ''FM10000_ARP_TAB_VERBOSE_DUMP_ENTRIES'' entries,
 *                  even if they are all zeros.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpArpTable(fm_int  sw,
                            fm_bool verbose)
{
    fm_status      err;
    fm_switch *    switchPtr;
    fm_int         index;
    fm_uint64 *    pArpTableImage;
    fm_uint64 *    pArpTabImageScan;
    fm_uint64 *    pArpTabImageUpperBound;
    fm_byte        routerId;
    fm_macaddr     macAddr;
    fm_uint16      dglort;
    fm_byte        mtu_Index;
    fm_bool        markRouted;
    fm_bool        iPv6Entry;
    fm_byte        routerIdGlort;
    fm_uint16      eVlan;


    switchPtr = GET_SWITCH_PTR(sw);

    /* create an image of the table */
    pArpTableImage = (fm_uint64*)fmAlloc(sizeof(fm_uint64) * FM10000_ARP_TAB_SIZE);

    if (pArpTableImage == NULL)
    {
        err = FM_ERR_NO_MEM;
    }
    else
    {
        err = CreateArpTableImage(sw, 0, FM10000_ARP_TAB_SIZE, pArpTableImage);
        if (err == FM_OK)
        {
            FM_LOG_PRINT("ARP Table:\n");
            FM_LOG_PRINT("------------------------------------------------------------\n");
            FM_LOG_PRINT("ARP index    Hardware\n");

            pArpTabImageScan = pArpTableImage;
            pArpTabImageUpperBound = pArpTableImage + FM10000_ARP_TAB_SIZE;
            index = 0;
             
            while (pArpTabImageScan < pArpTabImageUpperBound)
            {
                if ( *pArpTabImageScan || (verbose && (index < FM10000_ARP_TAB_VERBOSE_DUMP_ENTRIES)) )
                {
                    FM_LOG_PRINT(" %5d       0x%016llx   ", index, *pArpTabImageScan);

                    routerId = FM_ARRAY_GET_FIELD((fm_uint32 *)pArpTabImageScan, FM10000_ARP_TABLE, RouterId);
                    eVlan  = FM_ARRAY_GET_FIELD((fm_uint32 *)pArpTabImageScan, FM10000_ARP_TABLE, EVID);

                    if (routerId == 0)
                    {
                        /* this is a GLORT type entry */
                        dglort = FM_ARRAY_GET_FIELD((fm_uint32 *)pArpTabImageScan, FM10000_ARP_ENTRY_GLORT, DGLORT);
                        mtu_Index = FM_ARRAY_GET_FIELD((fm_uint32 *)pArpTabImageScan, FM10000_ARP_ENTRY_GLORT, MTU_Index);
                        markRouted = FM_ARRAY_GET_BIT((fm_uint32 *)pArpTabImageScan, FM10000_ARP_ENTRY_GLORT, markRouted);
                        iPv6Entry = FM_ARRAY_GET_BIT((fm_uint32 *)pArpTabImageScan, FM10000_ARP_ENTRY_GLORT, IPv6Entry);
                        routerIdGlort = FM_ARRAY_GET_FIELD((fm_uint32 *)pArpTabImageScan, FM10000_ARP_ENTRY_GLORT, RouterIdGlort);

                        FM_LOG_PRINT("Glort %04X,            vlan %4d, routerId %2d, MTU_index %d, markRouted %d, IPv6Entry %d, RouterIdGlort %d\n",

                                     dglort,
                                     eVlan,
                                     routerId,
                                     mtu_Index,
                                     markRouted,
                                     iPv6Entry,
                                     routerIdGlort);
                    }
                    else
                    {
                        
                        macAddr = FM_ARRAY_GET_FIELD64((fm_uint32 *)pArpTabImageScan, FM10000_ARP_ENTRY_DMAC, DMAC);

                        FM_LOG_PRINT("MAC Addr %012llX, vlan %4d, routerId %2d\n",
                                     macAddr,
                                     eVlan,
                                     routerId);
                    }
                }
                index++;
                pArpTabImageScan++;
            }
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot create an image of the ARP table\n");
        }
        FM_LOG_PRINT("\n");

        fmFree(pArpTableImage);
        pArpTableImage = NULL;
    }

}   /* end fm10000DbgDumpArpTable */




/*****************************************************************************/
/** fm10000DbgDumpArpTableExtended
 * \ingroup intNextHop
 *
 * \desc            Dumps the ARP table, this is an extended version.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpArpTableExtended(fm_int  sw)
{

    fm10000DbgDumpArpTable(sw, FALSE);

}   /* end fm10000DbgDumpArpTableExtended */




/*****************************************************************************/
/** fm10000DbgDumpArpUsedBlockStats
 * \ingroup intNextHop
 *
 * \desc            Plots a histogram of the used ARP blocks classified by
 *                  size.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpArpUsedBlockStats(fm_int  sw)
{
    fm10000_switch *pSwitchExt;
    fm_int          index;
    fm_int          maxValue;
    fm_int          normFactor;
    fm_int          value;
    fm_int          valueNormalized;


    pSwitchExt = GET_SWITCH_EXT(sw);

    /* determina the normalization factor */

    maxValue = 0;

    for (index = 0; index < FM10000_ARP_BLOCK_SIZE_MAX; index++)
    {
        if (maxValue < (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[index])
        {
            maxValue = (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[index];
        }
    }
    
    normFactor = (maxValue > FM_10000_ARP_HISTOGRAM_MAX_LENGTH) ? maxValue : FM_10000_ARP_HISTOGRAM_MAX_LENGTH;

    /* print the histogram (by sizes) of the allocated blocks */
    FM_LOG_PRINT("\nAllocated ARP block size histogram:\n");

    for (index = 0; index < FM10000_ARP_BLOCK_SIZE_MAX; index++)
    {
        value = (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[index];
        /* normalize the number of blocks */
        valueNormalized = (value * FM_10000_ARP_HISTOGRAM_MAX_LENGTH) / normFactor;

        FM_LOG_PRINT(" Size = %s: (%5d) ", 
                     ArpBlockSizeBinString[index], 
                     valueNormalized);

        FM_LOG_PRINT("%.*s\n", 
                     valueNormalized,
                     "########################################");
    }

}   /* end fm10000DbgDumpArpUsedBlockStats */




/*****************************************************************************/
/** fm10000DbgDumpArpFreeBlockStats
 * \ingroup intNextHop
 *
 * \desc            Plots a histogram of the free ARP blocks classified by
 *                  size.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpArpFreeBlockStats(fm_int  sw)
{
    fm10000_switch *pSwitchExt;
    fm_int          index;
    fm_int          maxValue;
    fm_int          normFactor;
    fm_int          value;
    fm_int          valueNormalized;


    pSwitchExt = GET_SWITCH_EXT(sw);

    /* determina the normalization factor */

    maxValue = 0;

    for (index = 0; index < FM10000_ARP_BLOCK_SIZE_MAX; index++)
    {
        if (maxValue < (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[index])
        {
            maxValue = (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[index];
        }
    }
    
    normFactor = (maxValue > FM_10000_ARP_HISTOGRAM_MAX_LENGTH) ? maxValue : FM_10000_ARP_HISTOGRAM_MAX_LENGTH;

    /* print the histogram (by sizes) of the free blocks */
    FM_LOG_PRINT("\nFree ARP block size histogram:\n");

    for (index = 0; index < FM10000_ARP_BLOCK_SIZE_MAX; index++)
    {
        value = (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[index];
        /* normalize the number of blocks */
        valueNormalized = (value * FM_10000_ARP_HISTOGRAM_MAX_LENGTH) / normFactor;

        FM_LOG_PRINT(" Size = %s: (%5d) ", 
                     ArpBlockSizeBinString[index], 
                     valueNormalized);

        FM_LOG_PRINT("%.*s\n", 
                     valueNormalized,
                     "########################################");
    }

}   /* end fm10000DbgDumpArpFreeBlockStats */





/*****************************************************************************/
/** fm10000DbgPrintArpFragmentationInfo
 * \ingroup intNextHop
 *
 * \desc            Prints ARP table fragmentation information.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgPrintArpFragmentationInfo(fm_int  sw)
{
    fm10000_switch *pSwitchExt;
    fm_int          index;
    fm_int          totalAllocatedBlocks;
    fm_int          totalFreeArpAreas;


    pSwitchExt = GET_SWITCH_EXT(sw);

    /* determina the normalization factor */

    totalAllocatedBlocks = 0;
    totalFreeArpAreas = 0;

    for (index = 0; index < FM10000_ARP_BLOCK_SIZE_MAX; index++)
    {
        totalAllocatedBlocks += (pSwitchExt->pNextHopSysCtrl->allocatedBlkStatInfo)[index];
        totalFreeArpAreas += (pSwitchExt->pNextHopSysCtrl->freeBlkStatInfo)[index];
    }
    
    totalFreeArpAreas = totalFreeArpAreas? totalFreeArpAreas: 1;

    /* print the histogram (by sizes) of the free blocks */
    FM_LOG_PRINT("\n---------------------\n");
    FM_LOG_PRINT("ARP table statistics:\n");
    FM_LOG_PRINT(" Total used entries...............%d\n", 
                 FM10000_ARP_TABLE_ENTRIES - pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount);
    FM_LOG_PRINT(" Total allocated blocks...........%d\n", totalAllocatedBlocks);
    FM_LOG_PRINT(" Total free entries...............%d\n", pSwitchExt->pNextHopSysCtrl->arpTabFreeEntryCount);
    FM_LOG_PRINT(" Total free areas.................%d\n", totalFreeArpAreas);
    FM_LOG_PRINT(" Fragmentation index [0..100].....%d\n\n", ((totalFreeArpAreas-1) * 200)/FM10000_ARP_TABLE_ENTRIES);

}   /* end fm10000DbgPrintArpFragmentationInfo */




/*****************************************************************************/
/** fm10000DbgPlotUsedArpDiagram
 * \ingroup intNextHop
 *
 * \desc            Plots a graphical representation of ARP table usage. The
 *                  table is divided in slots of 16 entries. A "." is plotted
 *                  if none of the slot entries are used, "x" is plotted if
 *                  the slot is partially used and a "X" is plotted if it is
 *                  full used.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgPlotUsedArpDiagram(fm_int  sw)
{
    fm10000_switch *pSwitchExt;
    fm_int          hndlIndex;
    fm_int          prnIndex;
    fm_int          usedEntryCnt;
    fm_uint16 *     pArpHndlTabScan;
    fm_char         prntBuff[FM10000_ARP_DUMP_LOCAL_BUFFER_SIZE];


    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pSwitchExt->pNextHopSysCtrl == NULL ||
        pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL)
    {
        FM_LOG_PRINT("Unsupported feature\n");
    }
    else
    {
        pArpHndlTabScan = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[0];
        usedEntryCnt = 0;
        hndlIndex = 0;
        prnIndex = 0;
        prntBuff[prnIndex] = '\0';

        FM_LOG_PRINT("ARP Table Usage:\n");
        FM_LOG_PRINT("------------------------------------------------------------------------\n");
        FM_LOG_PRINT("- Slots of 16 entries\n");
        FM_LOG_PRINT("-  '.': 0 used entries\n");
        FM_LOG_PRINT("-  '-': 1..7  used entries\n");
        FM_LOG_PRINT("-  '+': 8..15 used entries\n");
        FM_LOG_PRINT("-  'X': 16 used entries (slot Full)\n");

        while (hndlIndex <= FM10000_ARP_TAB_SIZE)
        {
            if ( (hndlIndex & 0x03ff) == 0 && hndlIndex < FM10000_ARP_TAB_SIZE )
            {
                FM_LOG_PRINT(" %5d: ",hndlIndex);
            }

            if (*pArpHndlTabScan != FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                usedEntryCnt++;
            }

            hndlIndex++;
            pArpHndlTabScan++;

            /* every 16 entries */
            if ( (hndlIndex & 0x0f) == 0 )
            {
                if (usedEntryCnt == 0)
                {
                    /* slot not used */
                    prntBuff[prnIndex] = '.';
                }
                else if (usedEntryCnt < 8)
                {
                    /* slot partially used */
                    prntBuff[prnIndex] = '-';
                }
                else if (usedEntryCnt < 16)
                {
                    /* slot partially used */
                    prntBuff[prnIndex] = '+';
                }
                else
                {
                    /* slot full */
                    prntBuff[prnIndex] = 'X';
                }
                if (prnIndex < FM10000_ARP_DUMP_LOCAL_BUFFER_SIZE-1)
                {
                    prnIndex++;
                }
                prntBuff[prnIndex] = '\0';
                usedEntryCnt = 0;
            }

            /* every 16 x 64 = 1024 entries */
            if ( (hndlIndex & 0x03ff) == 0 )
            {
                FM_LOG_PRINT("%s\n",prntBuff);
                prnIndex  = 0;
                prntBuff[prnIndex] = '\0';
            }

        }   /* end while (hndlIndex <= FM10000_ARP_TAB_SIZE) */
        FM_LOG_PRINT("\n\n");

        fm10000DbgDumpArpUsedBlockStats(sw);

        fm10000DbgDumpArpFreeBlockStats(sw);

        fm10000DbgPrintArpFragmentationInfo(sw);
    }

}   /* end  fm10000DbgPlotUsedArpDiagram */



/*****************************************************************************/
/** fm10000DbgDumpArpHandleTable
 * \ingroup intNextHop
 *
 * \desc            Displays the ARP table. If verbose is FALSE, only
 *                  entries that are not equal to zero will be included.
 *                  If verbose is TRUE, first
 *                  ''FM10000_ARP_TAB_VERBOSE_DUMP_ENTRIES'' entries will be
 *                  unconditionally included in the output.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       verbose indicates whether to include the first
 *                  ''FM10000_ARP_TAB_VERBOSE_DUMP_ENTRIES'' entries,
 *                  even if they are all zeros.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpArpHandleTable(fm_int  sw,
                                  fm_bool verbose)
{
    fm10000_switch *pSwitchExt;
    fm_int          index;
    fm_int          emptyEntryCnt;
    fm_int          emptyEntryBlockIndex;
    fm_uint16       offset;
    fm_uint16       length;
    fm_uint16       clients;
    fm_uint16       owner;
    fm_uint         opaque;
    fm_uint16 *     pArpHndlTabScan;
    fm_uint16 *     pArpHndlTabUpperBound;


    pSwitchExt = GET_SWITCH_EXT(sw);

    if ( pSwitchExt->pNextHopSysCtrl == NULL                  ||
         pSwitchExt->pNextHopSysCtrl->pArpHndlArray == NULL   ||
         pSwitchExt->pNextHopSysCtrl->ppArpBlkCtrlTab == NULL )
    {
        FM_LOG_PRINT("Unsupported feature\n");
    }
    else
    {
        FM_LOG_PRINT("ARP Handle Table:\n");
        FM_LOG_PRINT("-----------------------------------------\n");
        FM_LOG_PRINT(" Index length handle clients owner opaque\n");

        index = 1;
        emptyEntryCnt = 0;
        emptyEntryBlockIndex = -1;
        pArpHndlTabScan = &((*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[1]);
        pArpHndlTabUpperBound = &(*pSwitchExt->pNextHopSysCtrl->pArpHndlArray)[FM10000_ARP_TAB_SIZE - 1];

        while (pArpHndlTabScan <= pArpHndlTabUpperBound)
        {
            if (*pArpHndlTabScan != FM10000_ARP_BLOCK_INVALID_HANDLE)
            {
                /* print previous free block information, if any */
                if (emptyEntryCnt > 0 && verbose)
                {
                    FM_LOG_PRINT(" %5d  %5d   FREE       -\n",
                                 emptyEntryBlockIndex,
                                 emptyEntryCnt);
                }
                emptyEntryCnt = 0;
                emptyEntryBlockIndex = -1;

                /* print used block info */
                offset = GetArpBlockOffset(sw, *pArpHndlTabScan);
                if (index != offset)
                {
                    FM_LOG_PRINT("Index sync error: index=%d, offset=%d, handle=%d\n",
                                 index,
                                 offset,
                                 *pArpHndlTabScan);
                }
                else
                {
                    length = GetArpBlockLength(sw, *pArpHndlTabScan);
                    clients = GetArpBlockNumOfClients(sw, *pArpHndlTabScan);
                    owner   = GetArpBlockOwner(sw, *pArpHndlTabScan);
                    opaque  = GetArpBlockOpaque(sw, *pArpHndlTabScan);

                    FM_LOG_PRINT(" %5d  %5d  %5d    %4d    %2d 0x%8.8x\n",
                                 index,
                                 length,
                                 *pArpHndlTabScan,
                                 clients,
                                 owner,
                                 opaque);

                    /* jump to the end of the block */
                    index += length;
                    pArpHndlTabScan += length;
                    continue;
                }
            }
            else
            {
                /* this is a free entry, just count it and
                 * save the first index of an empty block */
                if (emptyEntryBlockIndex == -1)
                {
                    emptyEntryBlockIndex = index;
                }
                emptyEntryCnt++;
            }
            index++;
            pArpHndlTabScan++;
        }

        /* print info of the last free block, if any */
        if (emptyEntryCnt > 0 && verbose)
        {
            FM_LOG_PRINT(" %5d  %5d   FREE   -\n",
                         emptyEntryBlockIndex,
                         emptyEntryCnt);
        }
        FM_LOG_PRINT("\n");

    }   /* end while (pArpHndlTabScan <= pArpHndlTabUpperBound) */

}   /* end fm10000DbgDumpArpHandleTable */




/*****************************************************************************/
/** fm10000DbgDumpEcmpTables
 * \ingroup intNextHop
 *
 * \desc            Dumps the ECMP tables.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          None.
 *
 *****************************************************************************/
void fm10000DbgDumpEcmpTables(fm_int  sw)
{
    fm_status             err;
    fm_switch *           switchPtr;
    fm_int                ecmpIndex;
    fm_int                hopIndex;
    fm_int                routeIndex;
    fm_intEcmpGroup *     pEcmpGroup;
    fm10000_EcmpGroup *   pEcmpGroupExt;
    fm_intNextHop *       pNextHop;
    fm10000_NextHop *     pNextHopExt;
    fm_intRouteEntry *    routeKey;
    fm_intRouteEntry *    routePtr;
    fm_customTreeIterator iter;
    fm_uint16             ecmpArpBlkHandle;
    fm_uint16             ecmpArpBlkBaseIndex;
    fm_uint16             ecmpArpBlkLength;
    char                  chrBuff1[200];
    char                  chrBuff2[100];


    FM_LOG_PRINT("\nECMP Groups\n");

    err = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);

    for (ecmpIndex = 0 ; ecmpIndex < switchPtr->maxArpEntries ; ecmpIndex++)
    {
        pEcmpGroup = switchPtr->ecmpGroups[ecmpIndex];

        if (pEcmpGroup != NULL)
        {
            pEcmpGroupExt = pEcmpGroup->extension;
            ecmpArpBlkHandle = pEcmpGroupExt->arpBlockHandle;
            ecmpArpBlkBaseIndex = GetArpBlockOffset(sw, ecmpArpBlkHandle);
            ecmpArpBlkLength = GetArpBlockLength(sw, ecmpArpBlkHandle);

            FM_LOG_PRINT( "  %05d: groupId %d, nextHopCount %d, isUsable %d, fm10k arpBlockHandle %u,\n"
                          "    baseArpIndex %u, allocatedArpCount %u, activeArpCount %d, glortArpData %" FM_FORMAT_64 "X, mcastGroup %p\n",
                          ecmpIndex,
                          pEcmpGroup->groupId,
                          pEcmpGroup->nextHopCount,
                          pEcmpGroup->isUsable,
                          ecmpArpBlkHandle,
                          ecmpArpBlkBaseIndex,
                          ecmpArpBlkLength,
                          pEcmpGroupExt->activeArpCount,
                          pEcmpGroupExt->glortArpData,
                          (void *) pEcmpGroup->mcastGroup );

            FM_LOG_PRINT("    Next Hops:\n");
            for (hopIndex = 0 ; hopIndex < pEcmpGroup->nextHopCount ; hopIndex++)
            {
                pNextHop = pEcmpGroup->nextHops[hopIndex];

                if (pNextHop == NULL)
                {
                    continue;
                }

                pNextHopExt = pNextHop->extension;

                /* use charBuff1 for the arp Ip address and chrBuff2 for the interface address */
                fmDbgConvertIPAddressToString(&pNextHop->nextHop.data.arp.addr, chrBuff1);
                fmDbgConvertIPAddressToString(&pNextHop->nextHop.data.arp.interfaceAddr, chrBuff2);

                FM_LOG_PRINT("      %05d: (%p), addr %s, interfaceAddr %s, hop vlan %u,\n"
                             "        arp %p, vlan %u, oldVlan %u, interfaceAddressEntry %p, routeState %d, isUsable %d,\n"
                             "        extension %p, arpIndex %d, arpData %" FM_FORMAT_64 "X " "%" FM_FORMAT_64 "X\n",
                             hopIndex,
                             (void *) pNextHop,
                             chrBuff1,
                             chrBuff2,
                             pNextHop->nextHop.data.arp.vlan,
                             (void *) pNextHop->arp,
                             pNextHop->nextHop.data.arp.vlan,
                             pNextHop->oldVlan,
                             (void *) pNextHop->interfaceAddressEntry,
                             pNextHop->routeState,
                             pNextHop->isUsable,
                             (void *) pNextHopExt,
                             pNextHopExt ?
                             ecmpArpBlkBaseIndex + pNextHopExt->arpBlkRelOffset : 0,
                             pNextHopExt ? pNextHopExt->arpData[0] : 0,
                             pNextHopExt ? pNextHopExt->arpData[1] : 0);
            }

            fmCustomTreeIterInit(&iter, &pEcmpGroup->routeTree);
            routeIndex = 0;

            FM_LOG_PRINT("    Routes:\n");
            while (err == FM_OK)
            {
                err = fmCustomTreeIterNext( &iter,
                                           (void **) &routeKey,
                                           (void **) &routePtr);
                if (err == FM_OK)
                {
                    fmDbgBuildRouteDescription(&routePtr->route, chrBuff1);

                    FM_LOG_PRINT("      %05d: route %p: %s\n",
                                 routeIndex++,
                                 (void *) routePtr,
                                 chrBuff1);
                }
            }   /* end while (err == FM_OK) */

        }   /* end if (pEcmpGroup != NULL) */

    }   /* for (ecmpIndex = 0 ...) */

}   /* end fm10000DbgDumpEcmpTables */




