/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            api/fm10000/fm10000_api_lbg.c
 * Creation Date:   September, 2013
 * Description:     FM10000 services for LBG management.
 *
 * Copyright (c) 2005 - 2016, Intel Corporation
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

#define NO_ARP_BLOCK_INDEX              (0xFFFF)
#define NO_ARP_BLOCK_HANDLE             (0xFFFF)

#define FM10000_MAX_NUM_LBG             FM10000_ARP_TABLE_ENTRIES

#define FM10000_NUM_LBG_BIN_PER_GROUP   16

#define LBG_NOT_USED                    0
#define LBG_IN_USE                      1

/* Above 16, the number of bins supported must be a power of two */
const fm_uint16 nbBinsTable[] = { 32, 
                                  64, 
                                  128, 
                                  256, 
                                  512, 
                                  1024, 
                                  2048, 
                                  4096};

/* Simple helper macro that counts and returns the number of bits set in
 * value. */
#define FM_COUNT_SET_BITS(value, bits)                                         \
    {                                                                          \
        fm_int modifValue = value;                                             \
        for ((bits) = 0; modifValue; (bits)++)                                 \
        {                                                                      \
            modifValue &= modifValue - 1;                                      \
        }                                                                      \
    }

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** FillArpDataFromLBGMember
 * \ingroup intLbg
 *
 * \desc            Fills in the ARP_TABLE entry fields based on the type
 *                  lbgMember.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       lbgMember points to the lbgMember for which arp data has
 *                  to be filled.
 * 
 * \param[out]      arpData is where the filled in ARP_TABLE entry has to
 *                  be stored.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status FillArpDataFromLBGMember(fm_int        sw,
                                          fm_LBGMember *lbgMember,
                                          fm_uint64 *   arpData)
{
    fm_status  err = FM_OK;
    fm_uint32  dglort;
    fm_int     vroff;
    fm_int     routerId;
    fm_macaddr dmac;
    fm_int     egressVlan;
    fm_int     markRouted;
    fm_int     mtuIndex;
    fm_bool    isL3SwitchOnly;
    fm_bool    isL2SwitchOnly;
    fm_int     mcastLogicalPort;
    fm_int     lbgLogicalPort;
    fm_tunnelGlortUser glortUser;
    

    FM_LOG_ENTRY_VERBOSE(FM_LOG_CAT_LBG,
                         "sw=%d lbgMember=%p, arpDataPtr=%p\n",
                         sw, (void *) lbgMember, (void *) arpData );

    switch (lbgMember->lbgMemberType)
    {
        case FM_LBG_MEMBER_TYPE_PORT:
            if ( (fmIsValidPort(sw, 
                                lbgMember->port, 
                                ( ALLOW_REMOTE  | 
                                  ALLOW_CPU     |
                                  ALLOW_VIRTUAL  ) ) ) ||
                 ( lbgMember->port == FM_PORT_DROP ) )
            {
                err = fmGetLogicalPortGlort(sw, lbgMember->port, &dglort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

                FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_GLORT, DGLORT, dglort);
                FM_SET_BIT64(*arpData, FM10000_ARP_ENTRY_GLORT, markRouted, 0);
            }
            else
            {
                err = FM_ERR_INVALID_VALUE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            break;

        case FM_LBG_MEMBER_TYPE_MAC_ADDR:
            dmac = lbgMember->dmac;
            egressVlan = lbgMember->egressVlan;
            if (lbgMember->vrid == FM_ROUTER_ANY)
            {
                routerId = FM_ROUTER_ID_NO_REPLACEMENT;
            }
            else
            {
                vroff = fmGetVirtualRouterOffset(sw, lbgMember->vrid);
                if (vroff < 0)
                {
                    err = FM_ERR_INVALID_VRID;
                }
                routerId = vroff + 1;
            }
            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_DMAC, DMAC, dmac);
            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_DMAC, EVID, egressVlan);
            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_DMAC, RouterId, routerId);
            break;

        case FM_LBG_MEMBER_TYPE_MCAST_GROUP:
            err = fmGetMcastGroupPortInt(sw, lbgMember->mcastGroup, (void *)&mcastLogicalPort); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            err = fmGetMcastGroupAttributeInt(sw, 
                                              lbgMember->mcastGroup, 
                                              FM_MCASTGROUP_L2_SWITCHING_ONLY, 
                                              (void *)&isL2SwitchOnly);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            err = fmGetMcastGroupAttributeInt(sw, 
                                              lbgMember->mcastGroup, 
                                              FM_MCASTGROUP_L3_SWITCHING_ONLY, 
                                              (void *)&isL3SwitchOnly);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            markRouted = (!isL2SwitchOnly && !isL3SwitchOnly) ? 1 : 0;

            err = fmGetMcastGroupAttributeInt(sw, 
                                              lbgMember->mcastGroup, 
                                              FM_MCASTGROUP_MTU_INDEX, 
                                              (void *)&mtuIndex);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            err = fmGetLogicalPortGlort(sw, mcastLogicalPort, &dglort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_GLORT, DGLORT, dglort);
            FM_SET_BIT64(*arpData, FM10000_ARP_ENTRY_GLORT, markRouted, markRouted);
            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_GLORT, MTU_Index, mtuIndex);
            break;

        case FM_LBG_MEMBER_TYPE_L234_LBG:
            err = fm10000GetLBGAttribute(sw,
                                         lbgMember->l234Lbg,
                                         FM_LBG_LOGICAL_PORT,
                                         &lbgLogicalPort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            
            err = fmGetLogicalPortGlort(sw, lbgLogicalPort, &dglort);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_GLORT, DGLORT, dglort);
            break;

        case FM_LBG_MEMBER_TYPE_TUNNEL:
            err = fm10000GetTunnelAttribute(sw,
                                            lbgMember->tunnelGrp,
                                            lbgMember->tunnelRule,
                                            FM_TUNNEL_GLORT_USER,
                                            &glortUser);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            /* Only accept rule redirection that don't make uses of the user
             * field. */
            if ( (glortUser.userMask != 0) ||
                 (glortUser.glortMask != 0xFFFF) )
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_GLORT, DGLORT,
                           glortUser.glort);
            FM_SET_FIELD64(*arpData, FM10000_ARP_ENTRY_GLORT, RouterIdGlort,
                           FM_ROUTER_ID_NO_REPLACEMENT);
            FM_SET_BIT64(*arpData, FM10000_ARP_ENTRY_GLORT, markRouted, 0);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            break;
    }

ABORT:

    FM_LOG_EXIT_VERBOSE(FM_LOG_CAT_LBG, err);

}   /* end FillArpDataFromLBGMember */





/*****************************************************************************/
/** UpdateDistributionInHWGlortTable
 * \ingroup intLbg
 *
 * \desc            This updates the Glort Table in hardware with hwDistribution.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group points to the state structure for the group.
 *
 * \param[in]       firstBin is the bin number of the first bin to update.
 *
 * \param[in]       numberOfBins is the number of bins that should be updated
 *                  starting at firstBin.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateDistributionInHWGlortTable(fm_int       sw,
                                                  fm_LBGGroup *group,
                                                  fm_int       firstBin,
                                                  fm_int       numberOfBins)
{
    fm_status           err = FM_OK;
    fm_port *           groupPortPtr;
    fm_port *           memberPortPtr;
    fm_int              destIndex;
    fm_logicalPortInfo *lportInfo;
    fm_glortDestEntry * destEntry;
    fm_portmask *       mask;
    fm_portmask         destMask;
    fm_int              bin;


    FM_LOG_ENTRY(FM_LOG_CAT_LBG, 
                 "sw = %d\n", 
                 sw);
    
    if (group->lbgMode != FM_LBG_MODE_MAPPED_L234HASH)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_ARGUMENT);
    }

    groupPortPtr  = GET_PORT_PTR(sw, group->lbgLogicalPort);

    if (!groupPortPtr)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_PORT);
    }

    lportInfo = GET_LPORT_INFO(sw);

    for ( bin = firstBin ; bin < firstBin + numberOfBins ; bin++ )
    {
        if ( (group->hwDistributionV2[bin].lbgMemberType == FM_LBG_MEMBER_TYPE_PORT) &&
             (group->hwDistributionV2[bin].port != -1) )
        {
            memberPortPtr = GET_PORT_PTR(sw, group->hwDistributionV2[bin].port);

            if (!memberPortPtr)
            {
                FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_PORT);
            }

            destIndex = groupPortPtr->camEntry->destIndex + bin;
            mask      = &memberPortPtr->destEntry->destMask;

            FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                          (destIndex >= 0) &&
                          (destIndex < lportInfo->numDestEntries),
                          "Dest entry index is out of range\n");

            destEntry = &lportInfo->destEntries[destIndex];

            if (fmIsRemotePort(sw, group->hwDistributionV2[bin].port))
            {
                err = fmGetRemotePortDestMask(sw,
                                              group->hwDistributionV2[bin].port,
                                              NULL,
                                              &destMask);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            else
            {
                destMask = *mask;
            }

            /* Update the dest table entry */
            err = fm10000SetGlortDestMask(sw,
                                          destEntry,
                                          &destMask);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end UpdateDistributionInHWGlortTable */




/*****************************************************************************/
/** UpdateDistributionInHWArpTable
 * \ingroup intLbg
 *
 * \desc            This updates the hardware with hwDistribution.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group points to the state structure for the group.
 * 
 * \param[in]       firstBin is the bin number of the first bin to update.
 * 
 * \param[in]       numberOfBins is the number of bins that should be updated
 *                  starting at firstBin.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateDistributionInHWArpTable(fm_int       sw, 
                                                fm_LBGGroup *group, 
                                                fm_int       firstBin,
                                                fm_int       numberOfBins)
{
    fm_status                 err = FM_OK;
    fm_port *                 memberPortPtr;
    fm_uint32                 dglort;
    fm_int                    bin;
    fm10000_LBGGroup *        groupExt;
    fm_switch *               switchPtr;
    fm_int                    numGroup;
    fm_int                    numGroupBit;
    fm_int                    numGroupLowBit;
    fm_int                    numGroupHighBit;
    fm_int                    numGroupMask;
    fm_int                    selectGroup;
    fm_int                    remapGroup;
    fm_int                    remapBin;
    fm_uint64                 arpData;
    fm_LBGMember *            lbgMember;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d group=%p, firstBin=%d, numberOfBins=%d\n",
                 sw, (void *) group, firstBin, numberOfBins);

    switchPtr = GET_SWITCH_PTR(sw);
    groupExt = group->extension;

    if ( (groupExt->arpBlockIndex == NO_ARP_BLOCK_INDEX) ||
         (groupExt->arpBlockIndex >= (FM10000_ARP_TABLE_ENTRIES - 1) ) )
    {
        /* Unexpected failure */
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_FAIL);
    }

    if ((firstBin + numberOfBins) > group->numBins)
    {
        /* Unexpected failure */
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_FAIL);
    }

    /* Compute some variable used to scramble the hash bin */
    numGroup = group->numBins / FM10000_NUM_LBG_BIN_PER_GROUP;
    numGroupMask = (numGroup - 1);
    FM_COUNT_SET_BITS(numGroupMask, numGroupBit);
    numGroupLowBit = (numGroupBit >> 1) + (numGroupBit & 0x1);
    numGroupHighBit = (numGroupBit >> 1);

    for ( bin = 0 ; bin < numberOfBins ; bin++ )
    {
        /* Remap the bins to improve hashing. FM10000 can hash over up
         * to 4096 bins using a checksum algorithm. The result is scrambled
         * using a 4-bit pTable applied to the lowest 4-bit position. Due to
         * linear CRC and usage of only last 4-bit, similar keys result in
         * similar bin selection. 
         *
         * This mostly impact bins configured in block as opposed to
         * stripped, e.g.: Bin[0..99] = Port 1, Bin[100..199] = Port 2,...
         *
         * The following algorithm is used to scramble the block in software
         * to improve hashing:
         * 
         * 1 - Divide the global size in group of 16 entries.
         * 2 - Select the group based on the low order bits.
         * 3 - Scramble the group selection in a non linear way by inverting
         *     low bit position with high one.
         * 4 - Use the high order bit to index the entry inside the group. */ 
        if (numGroup > 1)
        {
            selectGroup = (firstBin + bin) & numGroupMask;
            remapGroup = ((selectGroup & ((numGroupMask >> numGroupLowBit) << numGroupLowBit)) >> numGroupLowBit) |
                         ((selectGroup & (numGroupMask >> numGroupHighBit)) << numGroupHighBit);

            remapBin = (remapGroup * FM10000_NUM_LBG_BIN_PER_GROUP) +
                       ((firstBin + bin) >> numGroupBit);
        }
        else
        {
            remapBin = firstBin + bin;
        }

        /* Update ARP table entry in the remapBin */
        arpData = 0LL;
        if (group->lbgMode == FM_LBG_MODE_REDIRECT)
        {
            memberPortPtr = GET_PORT_PTR(sw, group->hwDistribution[firstBin + bin]);

            if (!memberPortPtr)
            {
                err = FM_ERR_INVALID_PORT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            
            dglort = memberPortPtr->glort;

            FM_SET_FIELD64(arpData, FM10000_ARP_ENTRY_GLORT, DGLORT, dglort);
            FM_SET_BIT64(arpData, FM10000_ARP_ENTRY_GLORT, markRouted, 0);
            err = switchPtr->WriteUINT64(sw,
                                         FM10000_ARP_TABLE(groupExt->arpBlockIndex + remapBin,
                                                           0),
                                         arpData);
        }
        else
        {
            lbgMember = &group->hwDistributionV2[firstBin + bin];

            err = FillArpDataFromLBGMember(sw, lbgMember, &arpData);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_PORT, err);

            err = switchPtr->WriteUINT64(sw,
                                         FM10000_ARP_TABLE(groupExt->arpBlockIndex + remapBin,
                                                           0),
                                         arpData);
        }

        /* Note that the following fields are set to 0 and are not used by the
         * chip in this scenario:
         * -FM10000_ARP_ENTRY_GLORT.MTU_Index
         * -FM10000_ARP_ENTRY_GLORT.IPv6Entry
         * -FM10000_ARP_ENTRY_GLORT.RouterIdGlort
         * -FM10000_ARP_ENTRY_GLORT.EVID
         * -FM10000_ARP_ENTRY_GLORT.RouterId */
    }

ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end UpdateDistributionInHWArpTable */




/*****************************************************************************/
/** UpdateDistribution
 * \ingroup intLbg
 *
 * \desc            This updates the distribution across forwarding entries.
 *                  This function doesn't need to know the ports being added
 *                  or removed because it simply redistributes the
 *                  ports without regard to maintaing flow hashing.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       group points to the state structure for the group.
 * 
 * \param[in]       firstBin is the bin number of the first bin to update.
 * 
 * \param[in]       numberOfBins is the number of bins that should be updated
 *                  starting at firstBin.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateDistribution(fm_int       sw, 
                                    fm_LBGGroup *group, 
                                    fm_int       firstBin,
                                    fm_int       numberOfBins)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, firstBin=%d, numberOfBins=%d\n",
                 sw, (void *) group, firstBin, numberOfBins);

    FM_NOT_USED(sw);

    switch (group->lbgMode)
    {
        case FM_LBG_MODE_MAPPED:
            err = UpdateDistributionInHWArpTable(sw, group, firstBin, numberOfBins);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        case FM_LBG_MODE_REDIRECT:
            err = fmCommonResetLBGDistributionForRedirect(sw, group);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            /* Write final hardware state */
            err = UpdateDistributionInHWArpTable(sw, group, firstBin, numberOfBins);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        case FM_LBG_MODE_MAPPED_L234HASH:
            err = UpdateDistributionInHWGlortTable(sw, group, firstBin, numberOfBins);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;

    }   /* end switch (group->lbgMode) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end UpdateDistribution */




/*****************************************************************************/
/** FreeLBGGroup
 * \ingroup intLbg
 *
 * \desc            This method frees the linked list structures of the
 *                  member list.
 *
 * \param[in]       ptr points to the object being freed.
 *
 * \return          None
 *
 *****************************************************************************/
static void FreeLBGGroup(void *ptr)
{
    fm_LBGGroup     *  group = (fm_LBGGroup *) ptr;
    fm10000_LBGGroup * groupExt;
    fm_intLBGMember *  m;
    fm_intLBGMember *  p;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, "ptr=%p\n", (void *) ptr);

    if (group != NULL)
    {
        groupExt = group->extension;

        if (groupExt != NULL)
        {
            fmFree(groupExt);
        }

        if (group->userDistribution)
        {
            fmFree(group->userDistribution);
        }

        if (group->hwDistribution)
        {
            fmFree(group->hwDistribution);
        }

        if (group->hwDistributionV2 != NULL)
        {
            fmFree(group->hwDistributionV2);
        }

        for (m = group->firstMember ; m ; )
        {
            p = m;
            m = m->nextMember;
            fmFree(p);
        }

        fmFree(group);
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_LBG);

}   /* end FreeLBGGroup */




/*****************************************************************************/
/** AllocateLbgHandle
 * \ingroup intLbg
 *
 * \desc            Returns a free LBG handle. 
 *
 * \param[in]       sw is the switch number to operate on
 *
 * \param[out]      lbgNumber is caller allocated storage where the LBG handle
 *                  should be stored. 
 *
 * \return          FM_OK if a free handle is returned
 * \return          FM_ERR_NO_MORE if all LBG handles are in use. 
 *
 *****************************************************************************/
static fm_status AllocateLbgHandle(fm_int sw,
                                   fm_int *lbgNumber)
{
    fm_status        err = FM_OK;
    fm10000_switch * switchExt;
    fm_int           newLbg;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, 
                 "sw = %d, lbgNumber = %p\n", 
                 sw, 
                 (void *)lbgNumber);

    switchExt = GET_SWITCH_EXT(sw);
    
    err = fmFindBitInBitArray(&switchExt->lbgUsage, 0, LBG_NOT_USED, &newLbg);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    err = fmSetBitArrayBit(&switchExt->lbgUsage, newLbg, LBG_IN_USE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    *lbgNumber = newLbg;

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end AllocateLbgHandle */




/*****************************************************************************/
/** FreeLbgHandle
 * \ingroup intLbg
 *
 * \desc            Releases a LBG handle that was previously in use. 
 *
 * \param[in]       sw is the switch number to operate on
 *
 * \param[out]      lbgNumber is LBG handle to free.
 *
 * \return          FM_OK if a free handle is returned
 * \return          FM_ERR_INVALID_LBG if the LBG handle is not in use. 
 *
 *****************************************************************************/
static fm_status FreeLbgHandle(fm_int sw,
                               fm_int lbgNumber)
{
    fm_status        err = FM_OK;
    fm10000_switch * switchExt;
    fm_bool          lbgState;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, 
                 "sw = %d, lbgNumber = %d\n", 
                 sw, 
                 lbgNumber);
    
    switchExt = GET_SWITCH_EXT(sw);
    
    err = fmGetBitArrayBit(&switchExt->lbgUsage, lbgNumber, &lbgState);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    if (lbgState != LBG_IN_USE)
    {
        err = FM_ERR_INVALID_LBG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    err = fmSetBitArrayBit(&switchExt->lbgUsage, lbgNumber, LBG_NOT_USED);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end FreeLbgHandle */



/*****************************************************************************/
/** ValidateDistributionMapRangeV2
 * \ingroup intLbg
 *
 * \desc            Validates DistributionMapRangeV2 size and its members based
 *                  on the LBG group's mode.
 *
 * \param[in]       sw is the switch number to operate on
 *
 * \param[in]       group is the LBG group for this rangeV2 has to be verified.
 *
 * \param[in]       rangeV2 points to the DistributionMapRangeV2 to be verified.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ValidateDistributionMapRangeV2(
                            fm_int                         sw,
                            fm_LBGGroup *                  group,
                            fm_LBGDistributionMapRangeV2 * rangeV2)
                                                
{

    fm_status        err = FM_OK;
    fm_int           bin;
    fm_LBGMember   * lbgMember;
    fm_LBGMode       lbgMode; 
    fm_tunnelCondition       tunnelCondition;
    fm_tunnelConditionParam  tunnelConditionParam;
    fm_tunnelAction          tunnelAction = 0;
    fm_tunnelActionParam     tunnelActionParam;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG, 
                 "sw=%d lbgGroup=%p, rangeV2=%p\n",
                 sw, (void *) group, (void *) rangeV2 );
    
    if (group->lbgMode != FM_LBG_MODE_MAPPED)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    if (!rangeV2 || !rangeV2->lbgMembers)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    if ((rangeV2->numberOfBins <= 0) ||
        (rangeV2->numberOfBins > group->numBins))
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    if ( (rangeV2->firstBin < 0) ||
         (rangeV2->firstBin >= group->numBins) ||
         ( (rangeV2->firstBin + rangeV2->numberOfBins) > group->numBins) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    for ( bin = 0 ;
          bin < rangeV2->numberOfBins ;
          bin++ )
    {
        lbgMember = &rangeV2->lbgMembers[bin];

        /* Validate for each member type */
        if (lbgMember->lbgMemberType == FM_LBG_MEMBER_TYPE_PORT)
        {
            if (!fmIsValidPort(sw, 
                               lbgMember->port, 
                               ( ALLOW_REMOTE  | 
                                 ALLOW_CPU     |
                                 ALLOW_VIRTUAL  ) ) )
            {
                /* If the port is not the drop port, consider it invalid */
                if (lbgMember->port != FM_PORT_DROP)
                {
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
                }
            }
        }
        /* Whether the mcast group is valid can be identified while 
         * retrieving logical port of mcast group */
        else if (lbgMember->lbgMemberType == FM_LBG_MEMBER_TYPE_MAC_ADDR ||
                 lbgMember->lbgMemberType == FM_LBG_MEMBER_TYPE_MCAST_GROUP) 
        {
            err = FM_OK;
        }
        else if (lbgMember->lbgMemberType == FM_LBG_MEMBER_TYPE_L234_LBG)
        {
            err = fm10000GetLBGAttribute(sw,
                                         lbgMember->l234Lbg,
                                         FM_LBG_GROUP_MODE,
                                         &lbgMode);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            if (lbgMode != FM_LBG_MODE_MAPPED_L234HASH)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
        }
        else if (lbgMember->lbgMemberType == FM_LBG_MEMBER_TYPE_TUNNEL)
        {
            err = fm10000GetTunnelRule(sw,
                                       lbgMember->tunnelGrp,
                                       lbgMember->tunnelRule,
                                       &tunnelCondition,
                                       &tunnelConditionParam,
                                       &tunnelAction,
                                       &tunnelActionParam);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
        }
        else
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
        }
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end ValidateDistributionMapRangeV2 */




/*****************************************************************************/
/** ValidateLbgParams
 * \ingroup intLbg
 *
 * \desc            Creates a load balancing group.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       lbgNumber points to the location of lbgNumber given by the
 *                  caller.
 *
 * \param[in]       params points to the parameters for the LBG to be
 *                  validated.
 *
 * \return          FM_ERR_INVALID_ARGUMENT if parameters are invalid.
 * \return          FM_ERR_UNSUPPORTED if parameters are not supported in 
 *                  this chip.
 *
 *****************************************************************************/
fm_status ValidateLbgParams(fm_int        sw,
                            fm_int *      lbgNumber,
                            fm_LBGParams *params)
{
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%p, params=%p\n",
                 sw,
                 (void *) lbgNumber,
                 (void *) params);

    switch (params->mode)
    {
        case FM_LBG_MODE_REDIRECT:
        case FM_LBG_MODE_MAPPED:
        case FM_LBG_MODE_MAPPED_L234HASH:
            err = FM_OK;
            break;
        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    if ( (params->numberOfBins < 0) ||
         ( (params->mode == FM_LBG_MODE_MAPPED_L234HASH) && 
           ( (params->numberOfBins > 16) || (params->numberOfBins < 1) ) ) )
    {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end ValidateLbgParams */


/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fm10000LBGInit
 * \ingroup intLbg
 *
 * \desc            Initialize LBGs.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000LBGInit(fm_int sw)
{
    fm_status        err = FM_OK;
    fm10000_switch * switchExt;
    fm_LBGInfo *     info;
    
    info        = GET_LBG_INFO(sw);
    switchExt   = GET_SWITCH_EXT(sw);
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG, "sw=%d\n",sw);

    fmTreeInit(&info->groups);
    err = fmCreateBitArray(&switchExt->lbgUsage, FM10000_MAX_NUM_LBG);

    info->mode = FM_LBG_MODE_REDIRECT;

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000LBGInit */




/*****************************************************************************/
/** fm10000FreeLBGResource
 * \ingroup intLbg
 *
 * \desc            Free the LBG-related resources. This is to undo what is
 *                  done in fm10000LBGInit. 
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000FreeLBGResource(fm_int sw)
{
    fm_status        err = FM_OK;
    fm10000_switch * switchExt;
    fm_LBGInfo *     info;
    
    info        = GET_LBG_INFO(sw);
    switchExt   = GET_SWITCH_EXT(sw);
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG, "sw=%d\n",sw);

    err = fmDeleteBitArray(&switchExt->lbgUsage);
    fmTreeDestroy(&info->groups, FreeLBGGroup);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000FreeLBGResource */




/*****************************************************************************/
/** fm10000AllocateLBGs
 * \ingroup intLbg
 *
 * \desc            Allocate LBGs given a glort range. The function
 *                  returns the base LBG handle and the number of handles
 *                  created. The caller can then emumerate these handles up to
 *                  the number of handles allocated. These handles will have
 *                  the same CAM resources across multiple switches, given the
 *                  input glort information is the same.
 *
 * \note            The return base handle might not be the same on different
 *                  switches. However the cam resources for
 *                  (baseLBGHandle + n) will be consistent on different
 *                  switches.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       startGlort is the starting glort to use to create the
 *                  multicast resources.
 *
 * \param[in]       glortSize is the glort size to use. This value must be a
 *                  power of two.
 *
 * \param[out]      baseLbgHandle points to caller-allocated storage
 *                  where this function should place the base LBG group
 *                  handle of the newly allocated stacking LBGs.
 *
 * \param[out]      numLbgs points to caller-allocated storage
 *                  where this function should place the number of LBGs
 *                  allocated given the specified glort space.
 *
 * \param[out]      step points to caller-allocated storage
 *                  where this function should place the value where
 *                  the caller can offset from the base the get
 *                  subsequent LBG numbers
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT input parameters fail checking.
 * \return          FM_ERR_LOG_PORT_UNAVAILABLE if any glort or port resources
 *                  required in the given glort range is being used.
 * \return          FM_ERR_NO_LBG_RESOURCES if no more resources are available
 *
 *****************************************************************************/
fm_status fm10000AllocateLBGs(fm_int sw,
                              fm_uint startGlort,
                              fm_uint glortSize,
                              fm_int *baseLbgHandle,
                              fm_int *numLbgs,
                              fm_int *step)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, baseLbgHandle=%p, numLbgs=%p, step=%p\n",
                 sw, (void *) baseLbgHandle, (void *) numLbgs, (void *) step);

    err = fmAllocateLbgHandles(sw,
                               startGlort,
                               glortSize,
                               baseLbgHandle,
                               numLbgs,
                               step);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000AllocateLBGs */




/*****************************************************************************/
/** fm10000FreeLBGs
 * \ingroup intLbg
 *
 * \desc            Free allocated LBGs previously created with
 *                  ''fm10000AllocateLBGs''.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       baseLbgHandle is the handle previously created with
 *                  ''fm10000AllocateLBGs''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_PORT_IN_USE if any port resources are still being
 *                  used.
 *
 *****************************************************************************/
fm_status fm10000FreeLBGs(fm_int sw, fm_int baseLbgHandle)
{
    fm_status err = FM_OK;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, baseLbgHandle=%d\n",
                 sw, baseLbgHandle);

    err = fmFreeLbgHandles(sw, baseLbgHandle);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000FreeLBGs */




/*****************************************************************************/
/** fm10000CreateLBG
 * \ingroup intLbg
 *
 * \desc            Creates a load balancing group.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in,out]   lbgNumber points to a caller-supplied variable that will
 *                  receive the logical port number of the load balancing
 *                  group. On entry, lbgNumber contains a hint value, or
 *                  FM_LOGICAL_PORT_ANY if no hint is to be provided.
 *                  On exit, lbgNumber will contain the actual logical port
 *                  number, which may be different from the hint value.
 *
 * \param[in]       params points to the parameters for the LBG to be
 *                  created.
 *
 * \return          FM_OK if successful.
 * \return          params->numberOfBins
 *
 *****************************************************************************/
fm_status fm10000CreateLBG(fm_int sw,
                           fm_int *lbgNumber,
                           fm_LBGParams *params)
{
    fm_status              err = FM_OK;
    fm_status              cleanupErr;
    fm_switch *            switchPtr;
    fm_LBGInfo *           info;
    fm_LBGGroup *          group = NULL;
    fm10000_LBGGroup *     groupExt = NULL;
    fm_bool                handleAllocated = FALSE;
    fm_uint16              arpBlockHandle;
    fm10000_ArpBlkDscrptor arpBlk;
    fm_uint                i;
    fm_int                 j;
    fm_LBGMember *         hwDistributionV2;
    fm_int                 logicalPort = -1;
    fm_int                 useHandle;
    fm_port *              lbgLogicalPort;
    fm_portType            portType;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, group=%p, lbgNumber=%p, params=%p\n",
                 sw,
                 (void *) group,
                 (void *) lbgNumber,
                 (void *) params);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_LBG_INFO(sw);

    err = ValidateLbgParams(sw, lbgNumber, params);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    
    if (params->numberOfBins == 0)
    {
        params->numberOfBins = FM_FM10000_LBG_DEFAULT_BINS_PER_DISTRIBUTION;
    }
    else if (params->numberOfBins > 16)
    {
        /* The number of bins supported are 1..16 and then it must be a
         * power of two with a maximum of 4096. */
        for (i = 0; i < FM_NENTRIES(nbBinsTable); i++)
        {
            if (params->numberOfBins == nbBinsTable[i])
            {
                break;
            }
        }

        /* The size provide is out of range */
        if (i >= FM_NENTRIES(nbBinsTable))
        {
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
        }
    }

    switch(params->mode)
    {
        case FM_LBG_MODE_MAPPED_L234HASH:
            logicalPort = *lbgNumber;
            /* Allocate logicalPort and fall through to 
             * allocate lbgHandles */
            useHandle = (logicalPort == FM_LOGICAL_PORT_ANY) ? FALSE : TRUE;

            err = switchPtr->AllocLogicalPort(sw,
                                              FM_PORT_TYPE_LBG,
                                              params->numberOfBins,
                                              &logicalPort,
                                              useHandle);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
        
        case FM_LBG_MODE_MAPPED:
        case FM_LBG_MODE_REDIRECT:
            err = AllocateLbgHandle(sw, lbgNumber);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    handleAllocated = TRUE;

    /***************************************************
     * Allocate the group.
     **************************************************/
    group = (fm_LBGGroup *) fmAlloc( sizeof(fm_LBGGroup) );
    if (group == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_CLEAR(*group);

    /* The group has a list of all members */
    FM_DLL_INIT_LIST(group, firstMember, lastMember);

    /***************************************************
     * Initialize the group structure
     **************************************************/

    /* lbgNumber is just a handle. It is not related to logical port. */
    group->lbgPort        = *lbgNumber;
    group->state          = FM_LBG_STATE_INACTIVE;
    group->numBins        = params->numberOfBins;
    group->lbgMode        = params->mode;
    group->redirectMode   = FM_LBG_REDIRECT_PORT;
    group->lbgLogicalPort = logicalPort;

    /***************************************************
     * Allocate the group extension.
     **************************************************/

    groupExt = fmAlloc(sizeof(fm10000_LBGGroup));
    if (groupExt == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_CLEAR(*groupExt);
    groupExt->arpBlockIndex  = NO_ARP_BLOCK_INDEX;
    groupExt->arpBlockHandle = NO_ARP_BLOCK_HANDLE;

    group->extension = groupExt;

    /***************************************************
     * Allocate distribution bins.
     **************************************************/
    switch(group->lbgMode)
    {
        case FM_LBG_MODE_REDIRECT:
            /* Only used/needed in redirect mode */
            group->hwDistribution = fmAlloc(sizeof(fm_int) * group->numBins);
            if (group->hwDistribution == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            for (j = 0; j < group->numBins; j++)
            {
                group->hwDistribution[j] = FM_PORT_DROP;
            }
        
            group->userDistribution = fmAlloc(sizeof(fm_int) * group->numBins);
            if (group->userDistribution == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            for (j = 0; j < group->numBins; j++)
            {
                group->userDistribution[j] = FM_PORT_DROP;
            }

            /***************************************************
             * Allocate nexthop entries 
             **************************************************/
            err = fm10000RequestArpBlock(sw,
                                         FM10000_ARP_CLIENT_LBG,
                                         group->numBins,
                                         FM10000_ARP_BLOCK_OPT_DO_NOT_MOVE,
                                         &arpBlockHandle);
            if (err == FM_OK)
            {
                groupExt->arpBlockHandle = arpBlockHandle;
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            /* Retrieve the index of the ARP Block */
            err = fm10000GetArpBlockInfo(sw,
                                         FM10000_ARP_CLIENT_LBG,
                                         groupExt->arpBlockHandle,
                                         &arpBlk);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_LBG,
                                   arpBlk.length == group->numBins,
                                   err = FM_FAIL,
                                   "Requested %d bins but got %d\n",
                                   group->numBins,
                                   arpBlk.length);

            groupExt->arpBlockIndex = arpBlk.offset;
            break;

        case FM_LBG_MODE_MAPPED:
            /***************************************************
             * Allocate nexthop entries 
             **************************************************/
            err = fm10000RequestArpBlock(sw,
                                         FM10000_ARP_CLIENT_LBG,
                                         group->numBins,
                                         FM10000_ARP_BLOCK_OPT_DO_NOT_MOVE,
                                         &arpBlockHandle);
            if (err == FM_OK)
            {
                groupExt->arpBlockHandle = arpBlockHandle;
            }
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            /* Retrieve the index of the ARP Block */
            err = fm10000GetArpBlockInfo(sw, 
                                         FM10000_ARP_CLIENT_LBG, 
                                         groupExt->arpBlockHandle,
                                         &arpBlk);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            FM_LOG_ABORT_ON_ASSERT(FM_LOG_CAT_LBG, 
                                   arpBlk.length == group->numBins, 
                                   err = FM_FAIL, 
                                   "Requested %d bins but got %d\n",
                                   group->numBins,
                                   arpBlk.length);

            groupExt->arpBlockIndex = arpBlk.offset;

            hwDistributionV2 = fmAlloc(sizeof(fm_LBGMember) * group->numBins);
            if (hwDistributionV2 == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            for (j = 0; j < group->numBins; j++)
            {
                err = fmResetLBGMember(&hwDistributionV2[j]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            group->hwDistributionV2 = hwDistributionV2;
            break;

        case FM_LBG_MODE_MAPPED_L234HASH:
            err = fm10000GetLogicalPortAttribute(sw,
                                                 group->lbgLogicalPort,
                                                 FM_LPORT_TYPE,
                                                 &portType);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);;

            if (portType == FM_PORT_TYPE_LBG)
            {
                lbgLogicalPort = GET_PORT_PTR(sw, group->lbgLogicalPort);
                if (lbgLogicalPort != NULL)
                {
                    lbgLogicalPort->lbgHandle = group->lbgPort;
                }
                else
                {
                    err = FM_ERR_INVALID_LBG_STATE;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
                }
            }
            else
            {
                err = FM_ERR_INVALID_LBG_STATE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            hwDistributionV2 = fmAlloc(sizeof(fm_LBGMember) * group->numBins);
            if (hwDistributionV2 == NULL)
            {
                err = FM_ERR_NO_MEM;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            for (j = 0; j < group->numBins; j++)
            {
                err = fmResetLBGMember(&hwDistributionV2[j]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            group->hwDistributionV2 = hwDistributionV2;
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    /***************************************************
     * Insert this group into the global LBG tree
     **************************************************/
    err = fmTreeInsert(&info->groups, *lbgNumber, group);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

ABORT:
    if ( err != FM_OK) 
    {
        if (group != NULL)
        {
            if ( (groupExt != NULL) &&
                 (groupExt->arpBlockHandle != NO_ARP_BLOCK_HANDLE) )
            {
                /* Release ARP block if it was allocated */
                cleanupErr = fm10000FreeArpBlock(sw, 
                                                 FM10000_ARP_CLIENT_LBG, 
                                                 groupExt->arpBlockHandle);

                if (cleanupErr != FM_OK)
                {
                    FM_LOG_FATAL(FM_LOG_CAT_LBG,
                                 "Error cleaning up: %s\n",
                                 fmErrorMsg(cleanupErr));
                }
            }

            /* Free any LBG resource allocated */
            FreeLBGGroup((void *) group);
        }

        if (logicalPort != -1)
        {
            cleanupErr = fm10000FreeLogicalPort(sw, logicalPort);
            if (cleanupErr != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_LBG,
                             "Error cleaning up: %s\n",
                             fmErrorMsg(cleanupErr));
            }
        }

        if (handleAllocated)
        {
            cleanupErr = FreeLbgHandle(sw, *lbgNumber);
            if (cleanupErr != FM_OK)
            {
                FM_LOG_FATAL(FM_LOG_CAT_LBG,
                             "Error cleaning up: %s\n",
                             fmErrorMsg(cleanupErr));
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000CreateLBG */




/*****************************************************************************/
/** fm10000DeleteLBG
 * \ingroup intLbg
 *
 * \desc            Deletes a load-balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port number of the group
 *                  to be deleted.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DeleteLBG(fm_int sw, fm_int lbgNumber)
{
    fm_status            err = FM_OK;
    fm_LBGGroup *        group;
    fm10000_LBGGroup *   groupExt;
    fm_LBGInfo *         info;
    fm_bool              isUsed = FALSE;
    fm_portType          portType;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d\n",
                 sw, lbgNumber);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG);
    }

    groupExt = group->extension;

    switch (group->lbgMode)
    {
        case FM_LBG_MODE_MAPPED:
        case FM_LBG_MODE_REDIRECT:
            /* Before deleting the LBG, we must make sure no one is using it.
             * ACL is the only user of LBG's so ask this module. */
            err = fm10000ValidateAclArpBaseIndexId(sw, groupExt->arpBlockIndex, &isUsed);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            if (isUsed)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_LBG, 
                             "LBG is in use by ACLs and cannot be deleted\n");
                err = FM_ERR_INVALID_LBG_STATE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            err = fm10000FreeArpBlock(sw, 
                                      FM10000_ARP_CLIENT_LBG, 
                                      groupExt->arpBlockHandle);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        case FM_LBG_MODE_MAPPED_L234HASH:
            /* Make sure that logicalPort is valid LBG port and  is not used in 
             * any ACL and MAC Table module. */ 
            err = fm10000ValidateAclLogicalPort(sw, group->lbgLogicalPort, &isUsed);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            if (isUsed)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_LBG,
                             "LBG is in use by ACLs and cannot be deleted\n");
                err = FM_ERR_INVALID_LBG_STATE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            err = fm10000GetLogicalPortAttribute(sw, 
                                                 group->lbgLogicalPort, 
                                                 FM_LPORT_TYPE, 
                                                 &portType);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);;

            if ( portType == FM_PORT_TYPE_LBG )
            {
                err = fm10000FreeLogicalPort(sw, group->lbgLogicalPort);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            else
            {
                err = FM_ERR_INVALID_PORT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            break;

        default:
            err = FM_ERR_INVALID_LBG_MODE;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    err = FreeLbgHandle(sw, lbgNumber);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    /* Remove from tree and free memory */
    err = fmTreeRemove(&info->groups, lbgNumber, FreeLBGGroup);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000DeleteLBG */




/*****************************************************************************/
/** fm10000AddLBGPort
 * \ingroup intLbg
 *
 * \desc            Adds a member port to a load-balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port number of the LBG.
 *
 * \param[in]       port is the logical port number of the port to be added
 *                  to the LBG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if no memory available for data structures.
 * \return          FM_ERR_INVALID_LBG_STATE if the LBG is currently in the
 *                  active state.
 * \return          FM_ERR_ALREADY_EXISTS if the port is already a member
 *                  of the group.
 *
 *****************************************************************************/
fm_status fm10000AddLBGPort(fm_int sw, fm_int lbgNumber, fm_int port)
{
    fm_status         err = FM_OK;
    fm_LBGGroup *     group;
    fm_intLBGMember * member;
    fm_LBGInfo *      info;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, port=%d\n",
                 sw, lbgNumber, port);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG);
    }

    if (group->state != FM_LBG_STATE_INACTIVE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG_STATE);
    }

    /* We don't support LBG_MODE_REDIRECT under stacking
     * Since the failover for this mode is complicated */
    if ( (group->lbgMode == FM_LBG_MODE_REDIRECT) &&
         (!fmIsCardinalPort(sw, port)) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_UNSUPPORTED);
    }

    member = NULL;

    err = fmCommonFindLBGMember(group, port, &member);
    if (err == FM_OK && member)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_ALREADY_EXISTS);
    }
    else if (err != FM_ERR_NOT_FOUND)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, err);
    }
    
    /* Allocate the member port */
    member = (fm_intLBGMember *) fmAlloc( sizeof(fm_intLBGMember) );

    if (!member)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_NO_MEM);
    }

    FM_CLEAR(*member);

    member->lbgMemberPort  = port;
    member->group          = group;
    member->redirectTarget = FM_PORT_DROP;

    /* Port is initially standby */
    member->mode          = FM_LBG_PORT_STANDBY;
    
    FM_DLL_INSERT_LAST(group, firstMember, lastMember,
                       member, nextMember, prevMember);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end fm10000AddLBGPort */




/*****************************************************************************/
/** fm10000DeleteLBGPort
 * \ingroup intLbg
 *
 * \desc            Removes a member port from a load-balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port number of the LBG.
 *
 * \param[in]       port is the logical port number of the port to be
 *                  removed from the LBG.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_PORT_NOT_LBG_MEMBER if port is not a member of the
 *                  LBG.
 * \return          FM_ERR_INVALID_LBG_STATE if the LBG is currently in the
 *                  active state.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 *
 *****************************************************************************/
fm_status fm10000DeleteLBGPort(fm_int sw, fm_int lbgNumber, fm_int port)
{
    fm_status         err = FM_OK;
    fm_LBGGroup *     group;
    fm_intLBGMember * member;
    fm_LBGInfo *      info;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, port=%d\n",
                 sw, lbgNumber, port);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG);
    }

    if (group->state != FM_LBG_STATE_INACTIVE)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG_STATE);
    }

    member = NULL;

    err = fmCommonFindLBGMember(group, port, &member);
    if ((err == FM_OK) && member)
    {
        FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                      (member->lbgMemberPort == port),
                      "Member port did not match port key\n");
    }
    else
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_PORT_NOT_LBG_MEMBER);
    }
    
    FM_DLL_REMOVE_NODE(group, firstMember, lastMember, member,
                       nextMember, prevMember);

    fmFree(member);

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000DeleteLBGPort */




/*****************************************************************************/
/** fm10000SetLBGAttribute
 * \ingroup intLbg
 *
 * \desc            Sets an attribute of a load-balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port number of the LBG.
 *
 * \param[in]       attr is the LBG attribute to be set.
 *
 * \param[out]      value points to the value to be assigned to the
 *                  attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_LBG if lbgNumber is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is NULL or out of valid
 *                  range.
 * \return          FM_ERR_UNSUPPORTED if attr is not recognized.
 * \return          FM_ERR_READONLY_ATTRIB if attr is read-only.
 *
 *****************************************************************************/
fm_status fm10000SetLBGAttribute(fm_int sw,
                                 fm_int lbgNumber,
                                 fm_int attr,
                                 void *value)
{
    fm_status                      err = FM_OK;
    fm_LBGGroup *                  group;
    fm_LBGDistributionMapRange *   range;
    fm_LBGInfo *                   info;
    fm_int                         bin;
    fm_int                         method;
    fm_LBGDistributionMapRangeV2 * rangeV2;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, attr=%d, value=%p\n",
                 sw, lbgNumber, attr, (void *) value);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);
    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG);
    }

    switch (attr)
    {
        case FM_LBG_STATE:

            switch ( *( (fm_int *) value ) )
            {
                case FM_LBG_STATE_INACTIVE:
                case FM_LBG_STATE_ACTIVE:
                    group->state = *( (fm_int *) value );
                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
                    break;
            }

            if (group->state == FM_LBG_STATE_ACTIVE)
            {
                /***************************************************
                 * Update the distribution to the current
                 * set of ports.
                 **************************************************/
                err = UpdateDistribution(sw, group, 0, group->numBins);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            break;

        case FM_LBG_DISTRIBUTION_MAP:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        case FM_LBG_DISTRIBUTION_MAP_RANGE:

            range = (fm_LBGDistributionMapRange *) value;

            if ( !( (group->lbgMode == FM_LBG_MODE_MAPPED_L234HASH) ||
                    (group->lbgMode == FM_LBG_MODE_MAPPED) ) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            if (!range || !range->ports)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            if ((range->numberOfBins <= 0) ||
                (range->numberOfBins > group->numBins))
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            if ( (range->firstBin < 0) ||
                 (range->firstBin >= group->numBins) ||
                 ( (range->firstBin + range->numberOfBins) > group->numBins) )
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            /* First pass, validate all ports are valid */
            for ( bin = 0 ;
                  bin < range->numberOfBins ;
                  bin++ )
            {
                if (!fmIsValidPort(sw, 
                                   range->ports[bin], 
                                   ( ALLOW_REMOTE  | 
                                     ALLOW_CPU     |
                                     ALLOW_VIRTUAL  ) ) )
                {
                    /* If the port is not the drop port, consider it invalid */
                    if (range->ports[bin] != FM_PORT_DROP)
                    {
                        err = FM_ERR_INVALID_ARGUMENT;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
                    }
                }
            }

            /* Second pass, update the hw distribution table.  */
            for ( bin = 0 ;
                  bin < range->numberOfBins ;
                  bin++ )
            {
                group->hwDistributionV2[bin + range->firstBin].lbgMemberType = FM_LBG_MEMBER_TYPE_PORT;
                group->hwDistributionV2[bin + range->firstBin].port          = range->ports[bin];
            }

            /* This applies the range */
            err = UpdateDistribution(sw, group, range->firstBin, range->numberOfBins);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        case FM_LBG_DISTRIBUTION_MAP_RANGE_V2:

            rangeV2 = (fm_LBGDistributionMapRangeV2 *) value;

            err = ValidateDistributionMapRangeV2(sw, group, rangeV2); 
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            /* Update the hw distribution table.  */
            for ( bin = 0 ;
                  bin < rangeV2->numberOfBins ;
                  bin++ )
            {
                err = fmCopyLBGMember(&group->hwDistributionV2[bin + rangeV2->firstBin], 
                                      &rangeV2->lbgMembers[bin]);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            /* This applies the range */
            err = UpdateDistribution(sw, group, rangeV2->firstBin, rangeV2->numberOfBins);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        case FM_LBG_REDIRECT_METHOD:
            method = *( (fm_int *) value);

            if ( (method == FM_LBG_REDIRECT_PORT)           ||
                 (method == FM_LBG_REDIRECT_STANDBY)        ||
                 (method == FM_LBG_REDIRECT_PREFER_STANDBY) ||
                 (method == FM_LBG_REDIRECT_ALL_PORTS) )
            {
                group->redirectMode = method;

                /* This may change the distribution, so reapply */
                err = UpdateDistribution(sw, group, 0, group->numBins);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            else
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            break;

        case FM_LBG_LOGICAL_PORT:
            err = FM_ERR_READONLY_ATTRIB;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

    }   /* end switch (attr) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000SetLBGAttribute */




/*****************************************************************************/
/** fm10000GetLBGAttribute
 * \ingroup intLbg
 *
 * \desc            Gets an attribute of a load-balancing group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port number of the LBG.
 *
 * \param[in]       attr is the attribute to be retrieved.
 *
 * \param[out]      value points to a caller-supplied variable to
 *                  receive the attribute value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetLBGAttribute(fm_int sw, 
                                 fm_int lbgNumber, 
                                 fm_int attr,
                                 void *value)
{
    fm_status                     err = FM_OK;
    fm_LBGGroup *                 group;
    fm_LBGInfo *                  info;
    fm_LBGDistributionMapRange *  mapRange;
    fm_LBGDistributionMapRangeV2 *mapRangeV2;
    fm_int                        i;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, attr=%d, value=%p\n",
                 sw, lbgNumber, attr, (void *) value);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG);
    }

    switch (attr)
    {
        case FM_LBG_GROUP_MODE:
            *( (fm_LBGMode *) value ) = group->lbgMode;
            break;

        case FM_LBG_STATE:
            *( (fm_int *) value ) = group->state;
            break;

        case FM_LBG_DISTRIBUTION_MAP_RANGE:
            if ( !( (group->lbgMode == FM_LBG_MODE_MAPPED_L234HASH) ||
                    (group->lbgMode == FM_LBG_MODE_MAPPED) ) )
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            mapRange = (fm_LBGDistributionMapRange *) value;
            
            if (mapRange->firstBin > group->numBins)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            if ((mapRange->firstBin + mapRange->numberOfBins) > group->numBins)
            {
                mapRange->numberOfBins = group->numBins - mapRange->firstBin;
            }

            for (i = mapRange->firstBin; i < group->numBins; i++)
            {
                mapRange->ports[i] = group->hwDistributionV2[i].port;
            }
            break;

        case FM_LBG_DISTRIBUTION_MAP_RANGE_V2:
            if (group->lbgMode != FM_LBG_MODE_MAPPED)
            {
                err = FM_ERR_UNSUPPORTED;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            mapRangeV2 = (fm_LBGDistributionMapRangeV2 *) value;
            
            if (mapRangeV2->firstBin > group->numBins)
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            if ((mapRangeV2->firstBin + mapRangeV2->numberOfBins) > group->numBins)
            {
                mapRangeV2->numberOfBins = group->numBins - mapRangeV2->firstBin;
            }
            FM_MEMCPY_S( mapRangeV2->lbgMembers,
                         mapRangeV2->numberOfBins * sizeof(fm_LBGMember),
                         &group->hwDistributionV2[mapRangeV2->firstBin],
                         mapRangeV2->numberOfBins * sizeof(fm_LBGMember) );
            break;

        case FM_LBG_DISTRIBUTION_MAP_SIZE:
            *( (fm_int *) value ) = group->numBins;
            break;

        case FM_LBG_REDIRECT_METHOD:
            *( (fm_int *) value ) = group->redirectMode;
            break;

        case FM_LBG_LOGICAL_PORT:
            *( (fm_int *) value ) = group->lbgLogicalPort;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    }   /* end switch (attr) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000GetLBGAttribute */




/*****************************************************************************/
/** fm10000SetLBGPortAttribute
 * \ingroup intLbg
 *
 * \desc            Sets an attribute of a member port of a load-balancing
 *                  group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the logical port number of the LBG.
 *
 * \param[in]       port is the logical port number of the member port.
 *
 * \param[in]       attr is the attribute to be set.
 *
 * \param[out]      value points to the value to be assigned to the
 *                  attribute.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetLBGPortAttribute(fm_int sw,
                                    fm_int lbgNumber,
                                    fm_int port,
                                    fm_int attr,
                                    void *value)
{
    fm_status         err = FM_OK;
    fm_LBGGroup *     group;
    fm_intLBGMember * member;
    fm_intLBGMember * redirectMember;
    fm_LBGInfo *      info;
    fm_int            newMode;
    fm_bool           hwUpdateNeeded;
    fm_int            redirectTarget;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, port=%d, attr=%d, value=%p\n",
                 sw,
                 lbgNumber,
                 port,
                 attr,
                 (void *) value);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);

    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_LBG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    member = NULL;

    err = fmCommonFindLBGMember(group, port, &member);
    if ((err == FM_OK) && member)
    {
        FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                      (member->lbgMemberPort == port),
                      "Member port did not match port key\n");
    }
    else
    {
        err = FM_ERR_PORT_NOT_LBG_MEMBER;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    switch (attr)
    {
        case FM_LBG_PORT_MODE:
            newMode = *((fm_int *) value);

            if (group->lbgMode != FM_LBG_MODE_REDIRECT)
            {
                err = FM_ERR_INVALID_LBG_MODE;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            if ((newMode < 0) || (newMode >= FM_LBG_PORT_MODE_MAX))
            {
                err = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            /* The mode is the same, exit silently */
            if (member->mode == newMode)
            {
                goto ABORT;
            }

            /***************************************************
             * Filter out any disallowed transitions.
             **************************************************/

            err = fmCommonHandleLBGPortModeTransition(sw, 
                                                      group, 
                                                      member, 
                                                      newMode, 
                                                      &hwUpdateNeeded);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

            if (hwUpdateNeeded)
            {
                /* Update the hardware */
                err = UpdateDistributionInHWArpTable(sw, group, 0, group->numBins);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            break;

        case FM_LBG_PORT_REDIRECT_TARGET:
            redirectTarget = *( (fm_int *) value );

            /* Port must be cardinal, there is no support for stacking yet */
            if ( !fmIsCardinalPort(sw, redirectTarget) )
            {
                err = FM_ERR_INVALID_PORT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }
            
            redirectMember = NULL;

            /* The redirect target must be in the LBG's members */
            err = fmCommonFindLBGMember(group, redirectTarget, &redirectMember);
            if ((err == FM_OK) && redirectMember)
            {
                FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                              (redirectMember->lbgMemberPort == redirectTarget),
                              "Member port did not match port key\n");
            }
            else
            {
                err = FM_ERR_PORT_NOT_LBG_MEMBER;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            }

            member->redirectTarget    = redirectTarget;
            member->redirectTargetPtr = redirectMember;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
            break;

    }   /* end switch (attr) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000SetLBGPortAttribute */




/*****************************************************************************/
/** fm10000GetLBGPortAttribute
 * \ingroup intLbg
 *
 * \desc            Returns an attribute of a member port of a load-balancing
 *                  group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the load balancing group number.
 *
 * \param[in]       port is the logical port number of the meeber port.
 *
 * \param[in]       attr is the LBG port attribute to be returned.
 *
 * \param[out]      value points to a caller-supplied variable to receive
 *                  the attribute value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetLBGPortAttribute(fm_int sw,
                                    fm_int lbgNumber,
                                    fm_int port,
                                    fm_int attr,
                                    void *value)
{
    fm_status         err = FM_OK;
    fm_LBGGroup *     group;
    fm_intLBGMember * member;
    fm_LBGInfo *      info;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d, port=%d, attr=%d, value=%p\n",
                 sw, lbgNumber, port, attr, (void *) value);

    info = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_ERR_INVALID_LBG);
    }

    member = NULL;

    err = fmCommonFindLBGMember(group, port, &member);
    if ((err == FM_OK) && member)
    {
        FM_LOG_ASSERT(FM_LOG_CAT_LBG,
                      (member->lbgMemberPort == port),
                      "Member port did not match port key\n");
    }
    else
    {
        err = FM_ERR_PORT_NOT_LBG_MEMBER;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    switch (attr)
    {
        case FM_LBG_PORT_MODE:
            *( (fm_int *) value ) = member->mode;
            break;

        case FM_LBG_PORT_REDIRECT_TARGET:
            *( (fm_int *) value ) = member->redirectTarget;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            break;

    }   /* end switch (attr) */

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000GetLBGPortAttribute */




/*****************************************************************************/
/** fm10000GetLBGInfo
 * \ingroup intLbg
 *
 * \desc            Retrieve the ARP block base and length of a LBG.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       lbgNumber is the load balancing group number.
 *
 * \param[out]      arpBaseIndex points to a location where the
 *                  ARP base index should be stored. 
 *
 * \param[out]      arpBlockLength points to a location where the
 *                  length of the block should be stored. 
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetLBGInfo(fm_int sw, 
                            fm_int lbgNumber,
                            fm_int *arpBaseIndex, 
                            fm_int *arpBlockLength)
{
    fm_status           err = FM_OK;
    fm_LBGGroup *       group;
    fm10000_LBGGroup *  groupExt;
    fm_LBGInfo *        info;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d lbgNumber=%d\n",
                 sw,
                 lbgNumber);

    info      = GET_LBG_INFO(sw);

    err = fmTreeFind(&info->groups, lbgNumber, (void **) &group);
    if (err != FM_OK)
    {
        err = FM_ERR_INVALID_LBG;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    groupExt = group->extension;

    *arpBaseIndex = groupExt->arpBlockIndex;
    *arpBlockLength = group->numBins;
    
ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000GetLBGInfo */




/*****************************************************************************/
/** fm10000DbgDumpLBG
 * \ingroup intDebug
 *
 * \desc            Prints out debugging information related to the LBG group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       lbgNumber is the group number.  Use -1 to dump information
 *                  about all LBGs.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000DbgDumpLBG(fm_int sw, fm_int lbgNumber)
{
    fm_status           err = FM_OK;
    fm_switch *         switchPtr;
    fm_LBGInfo *        info;
    fm_LBGGroup *       group;
    fm10000_LBGGroup *  groupExt;
    fm_intLBGMember *   member;
    fm_uint64           groupNumber;
    fm_text             portModes[] = { "Active", "Failover", "Standby", "Inactive" };
    fm_treeIterator     iter;
    fm_int              i;
    fm_int              userBins;
    fm_int              hwBins;
    fm_uint64           arpEntry;
    fm_uint32           dglort = -1;
    fm_uint32           lbgGlort;
    
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, lbgNumber=%d\n",
                 sw, lbgNumber);

    switchPtr = GET_SWITCH_PTR(sw);
    info      = GET_LBG_INFO(sw);
    
    fmTreeIterInit(&iter, &info->groups);

    while(TRUE)
    {
        err = fmTreeIterNext(&iter, &groupNumber, (void **) &group);

        if (err == FM_ERR_NO_MORE)
        {
            break;
        }
        /* Skip if this is not the one we want */
        else if ((lbgNumber != -1) && (groupNumber != (fm_uint64) lbgNumber))
        {
            continue;
        }

        groupExt = group->extension;
        
        FM_LOG_PRINT("\nLoad Balancing Group #%d (%s)\n", lbgNumber,
                     (group->state == FM_LBG_STATE_ACTIVE) ?
                     "active" :
                     "inactive");
        FM_LOG_PRINT("-------------------------------------------\n");
        FM_LOG_PRINT("\n");
        if (group->lbgMode == FM_LBG_MODE_REDIRECT)
        {

            FM_LOG_PRINT("ArpTableIndex = %d, length=%d\n", 
                         groupExt->arpBlockIndex,
                         group->numBins);

            FM_LOG_PRINT("\nMember List:\n");

            FM_LOG_PRINT("%-4s  %-8s  %-8s  %-8s  %s\n", "Port", "Mode", "UserBins", "HwBins", "Comment");

            for ( member = group->firstMember ;
                  member ;
                  member = member->nextMember )
            {
                userBins = 0;
                hwBins = 0;

                for ( i = 0 ; i < group->numBins ; i++)
                {
                    if (group->userDistribution[i] == member->lbgMemberPort)
                    {
                        userBins++;
                    }

                    if (group->hwDistribution[i] == member->lbgMemberPort)
                    {
                        hwBins++;
                    }
                }

                FM_LOG_PRINT("%-4d  %-8s  %-8d  %-8d  redirect target of %d\n",
                             member->lbgMemberPort,
                             portModes[member->mode],
                             userBins,
                             hwBins,
                             member->redirectTarget);
            }
        }
        else if (group->lbgMode == FM_LBG_MODE_MAPPED)
        {
            FM_LOG_PRINT("ArpTableIndex = %d, length=%d\n", 
                         groupExt->arpBlockIndex,
                         group->numBins);

            FM_LOG_PRINT("\nBin Map (%d bins):\n", group->numBins);
            FM_LOG_PRINT("%-5s  %-10s  %-10s  %-10s  %-10s  %-10s %-10s\n", "Bin", "UserPort", "HwPort", "McastGroup", "L234Lbg", "dglort", "NextHop(DMAC,evlan,vrid)");

            for ( i = 0 ; i < group->numBins ; i++)
            {
                dglort = 0;
                if (group->hwDistributionV2[i].lbgMemberType != FM_LBG_MEMBER_TYPE_MAC_ADDR)
                {
                    switchPtr->ReadUINT64(sw, 
                                          FM10000_ARP_TABLE(groupExt->arpBlockIndex + i, 0),
                                          &arpEntry);

                    dglort = FM_GET_FIELD64(arpEntry, FM10000_ARP_ENTRY_GLORT, DGLORT);
                }

                FM_LOG_PRINT("%-5d  %-10s  %-10d  %-10d  %-10d  0x%04x    0x%012llX,%4d,%2d\n", 
                             i, 
                             "-",
                             (group->hwDistributionV2[i].port == FM_PORT_DROP) ? -1 : 
                             group->hwDistributionV2[i].port,
                             group->hwDistributionV2[i].mcastGroup,
                             group->hwDistributionV2[i].l234Lbg,
                             dglort,
                             group->hwDistributionV2[i].dmac,
                             group->hwDistributionV2[i].egressVlan,
                             group->hwDistributionV2[i].vrid);
            }
        }
        else if (group->lbgMode == FM_LBG_MODE_MAPPED_L234HASH)
        {
            fmGetLogicalPortGlort(sw, group->lbgLogicalPort, &lbgGlort);

            FM_LOG_PRINT("Logical Port = %d, Glort = 0x%04x, length=%d\n", 
                         group->lbgLogicalPort,
                         lbgGlort,
                         group->numBins);

            FM_LOG_PRINT("\nBin Map (%d bins):\n", group->numBins);
            FM_LOG_PRINT("%-5s  %-8s  %-8s \n", "Bin", "UserPort", "HwPort");

            for ( i = 0 ; i < group->numBins ; i++)
            {
                FM_LOG_PRINT("%-5d  %-8s  %-8d \n", 
                             i, 
                             "-",
                             group->hwDistributionV2[i].port);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end fm10000DbgDumpLBG */




/*****************************************************************************/
/** fm10000GetPortParametersForLBG
 * \ingroup intLbg
 *
 * \desc            Computes the number of logical ports and the number of
 *                  dest entries to allocate for an LBG.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      numPorts points to caller allocated storage where the
 *                  number of logical ports will be written.
 *
 * \param[out]      numDestEntries points to caller allocated storage where
 *                  the number of dest entries to use will be written.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetPortParametersForLBG(fm_int sw,
                                         fm_int *numPorts,
                                         fm_int *numDestEntries)
{

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, numPorts=%p, numDestEntries=%p\n",
                 sw, (void *) numPorts, (void *) numDestEntries);

    /***************************************************
     * The assumption is that the caller, has set
     * numPorts based on the number of bins they want.
     **************************************************/
    *numDestEntries = *numPorts;
    *numPorts = 1;

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end fm10000GetPortParametersForLBG */




/*****************************************************************************/
/** fm10000AssignLBGPortResources
 * \ingroup intLbg
 *
 * \desc            Allocates glort CAM and dest entries for a LBG.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      params points to a logical port parameter structure
 *                  for this LBG.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AssignLBGPortResources(fm_int sw, void *params)
{
    fm_switch *         switchPtr;
    fm_logicalPortInfo *lportInfo;
    fm_glortDestEntry **destEntry;
    fm_status           err;
    fm_int              j;
    fm_uint32           firstGlort;
    fm_int              firstDestEntry;
    fm_int              firstLogicalPort;
    fm_portParms *      parmPtr;
    fm_int              destEntrySize;
    fm_glortDestEntry * currDestEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                 "sw=%d, params=%p\n",
                 sw, (void *) params);

    switchPtr = GET_SWITCH_PTR(sw);
    lportInfo = &switchPtr->logicalPortInfo;
    parmPtr   = params;
    destEntry = NULL;

    /* First find unused glorts in the reserved LBG space */
    err = fmFindUnusedLbgGlorts(sw,
                                parmPtr->numPorts,
                                parmPtr->basePort,
                                &firstLogicalPort,
                                &firstGlort);
    if (err == FM_OK)
    {
        /* Save the logical port in the port parameter structure */
        parmPtr->basePort = firstLogicalPort;
    }
    else if (!parmPtr->useHandle)
    {
        /* We might be able to find one in the unreserved space.
         * In this case the firstLogicalPort would have been previously saved
         * in basePort by SelectLogicalPorts(). */
        err = fmFindFreeGlortRange(sw,
                                   1,
                                   FM_GLORT_TYPE_LBG,
                                   &firstGlort);

        if ( err != FM_OK )
        {
            err = FM_ERR_LOG_PORT_UNAVAILABLE;
        }
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    FM_LOG_DEBUG(FM_LOG_CAT_LBG, "Found glort 0x%x\n", firstGlort);

    destEntrySize = parmPtr->numDestEntries * sizeof(fm_glortDestEntry *);
    destEntry     = fmAlloc(destEntrySize);
    if (destEntry == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);
    }

    FM_MEMSET_S( destEntry, destEntrySize, 0, destEntrySize);

    err = fmAllocDestEntries(sw,
                             parmPtr->numDestEntries,
                             NULL,
                             destEntry,
                             parmPtr->portType);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    firstDestEntry = destEntry[0]->destIndex;

    /***************************************************
     * Each LBG will get its own CAM entry since the
     * hashing size can be different for each one.
     **************************************************/

    err = fmCreateGlortCamEntry(sw,
                                0xFFFF,                 /* exact match mask */
                                firstGlort,
                                FM_GLORT_ENTRY_TYPE_HASHED,
                                firstDestEntry,
                                parmPtr->numDestEntries,
                                0,                      /* A length */
                                0,                      /* A offset */
                                0,                      /* B length */
                                0,                      /* B length */
                                FM_GLORT_ENTRY_HASH_A,
                                0,                      /* dglortTag */
                                &parmPtr->camIndex);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_LBG, err);

    for ( j = 0 ; j < parmPtr->numDestEntries ; j++ )
    {
        destEntry[j]->owner = &lportInfo->camEntries[parmPtr->camIndex];
    }
    parmPtr->baseDestIndex = firstDestEntry;
    parmPtr->baseGlort     = firstGlort;

    FM_LOG_DEBUG(FM_LOG_CAT_PORT | FM_LOG_CAT_LBG,
                 "Assigned LBG port resources: "
                 "baseGlort=0x%x baseDest=0x%x num=%d basePort=%d cam=0x%x\n",
                 parmPtr->baseGlort,
                 parmPtr->baseDestIndex,
                 parmPtr->numDestEntries,
                 parmPtr->basePort,
                 parmPtr->camIndex);

ABORT:

    if (err != FM_OK)
    {
        if (destEntry != NULL)
        {
            for (j = 0 ; j < parmPtr->numDestEntries ; j++)
            {
                currDestEntry = destEntry[j];
                if (currDestEntry != NULL)
                {
                    currDestEntry->destIndex = 0;
                    currDestEntry->owner     = NULL;
                    currDestEntry->usedBy    = 0;
                }
            }
        }
    }

    if (destEntry != NULL)
    {
        fmFree(destEntry);
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, err);

}   /* end fm10000AssignLBGPortResources */



/*****************************************************************************/
/** fm10000GetHardwareLbgGlortRange
 * \ingroup intLbg
 *
 * \desc            Get the glort range usable for LBGs.
 *
 * \param[out]      lbgGlortBase points to caller-provided storage into
 *                  which the base glort value will be written.
 *
 * \param[out]      lbgGlortCount points to caller-provided storage into
 *                  which the number of usable glorts will be written.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetHardwareLbgGlortRange(fm_uint32 *lbgGlortBase,
                                          fm_uint32 *lbgGlortCount)
{
    FM_LOG_ENTRY(FM_LOG_CAT_LBG,
                "lbgGlortBase=%p, lbgGlortCount=%p\n",
                 (void *) lbgGlortBase,
                 (void *) lbgGlortCount);

    if (lbgGlortBase != NULL)
    {
        *lbgGlortBase = FM10000_GLORT_LBG_BASE;
    }

    if (lbgGlortCount != NULL)
    {
        *lbgGlortCount = FM10000_GLORT_LBG_SIZE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_LBG, FM_OK);

}   /* end fm10000GetHardwareLbgGlortRange */






