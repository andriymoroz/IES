/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm_api_nexthop.c
 * Creation Date: August 7, 2013
 * Description: Rouing services
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

#include <fm_sdk_int.h>

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local functions with fmAlloc inside. Notice that "static" prefix
 * can be temporarily removed for the process of detection memory leaks.
 * In this case "static" should be removed for both - declaration
 * and definition. It allows to see the proper function name in memory dump.
 *****************************************************************************/
static fm_status UpdateNextHopArpEntryAddedInt(fm_int           sw,
                                               fm_ipAddr       *pAddr,
                                               fm_intArpEntry  *pArpEntry);
static fm_status UpdateNextHopInterfaceAddrInt(fm_int                          sw,
                                               fm_ipAddr                      *pAddr,
                                               fm_intIpInterfaceAddressEntry  *pAddrEntry);
static fm_status AllocNextHopInt(fm_int            sw,
                                 fm_int            maxNextHops,
                                 fm_int            numFixedEntries,
                                 fm_intEcmpGroup  *pEcmpGroup);
static fm_status AllocEcmpGroupInt(fm_int                sw,
                                   fm_int                maxNextHops,
                                   fm_uint               lbsVlan,
                                   fm_bool               wideNextHops,
                                   fm_bool               mplsGroup,
                                   fm_int                numFixedEntries,
                                   fm_intMulticastGroup *mcastGroup,
                                   fm_intEcmpGroup     **ppEcmpGroup,
                                   fm_int               *groupId);
/* End of local functions with fmAlloc inside. */

static void CleanupNextHopTreeInt(fm_int sw);
static void CleanupArpPointersTree(fm_int sw);
static fm_status CleanupEcmpGroupsInt(fm_int sw);
static fm_status CleanupSingleEcmpGroupInt(fm_int           sw,
                                           fm_intEcmpGroup *pEcmpGroup);
static void FreeArp(void *key,
                    void *value);
static fm_status CleanupArpEntriesInt(fm_int sw);
static fm_status CleanupInterfacesInt(fm_int sw);
static fm_status FindArpEntryExt(fm_int           sw,
                                 fm_arpEntry     *pArp,
                                 fm_intArpEntry **ppArpEntry);
static fm_status FindInterfaceAddrEntry(fm_int                          sw,
                                        fm_ipAddr                      *interfaceAddr,
                                        fm_intIpInterfaceAddressEntry **ifAddrEntry);
static fm_status CheckIfInterfaceVlanIsAlreadyUsed(fm_int      sw,
                                                   fm_uint16   vlan);
static fm_status DeleteInterfaceAddrInt(fm_int                   sw,
                                        fm_intIpInterfaceEntry  *pIfEntry,
                                        fm_ipAddr               *pAddr);
static fm_status UpdateEcmpGroupsForInterface(fm_int                  sw,
                                              fm_intIpInterfaceEntry *pIfEntry);
static fm_status ValidateEcmpGroup(fm_int           sw,
                                   fm_intEcmpGroup *ecmpGroup,
                                   fm_bool *        updated);
static fm_status ValidateNextHop(fm_int         sw,
                                 fm_intNextHop *nextHop,
                                 fm_bool *      updated);
static fm_status UpdateArpEntriesVlanValue(fm_int                  sw,
                                           fm_intIpInterfaceEntry *pIfEntry,
                                           fm_uint16               vlan);
static fm_status UpdateNextHopArpEntryRemovedInt(fm_int           sw,
                                                 fm_intArpEntry  *pArpEntry);

/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** CleanupNextHopTreeInt
 * \ingroup intNextHop
 *
 * \desc            Removes all entries from the noArpNextHops tree
 *
 * \param[in]       sw is the switch number.
 *
 * \return          None.
 *                  
 *****************************************************************************/
static void CleanupNextHopTreeInt(fm_int sw)
{
    fm_switch      *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d\n",
                     sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr != NULL)
    {
        if ( fmCustomTreeIsInitialized(&switchPtr->noArpNextHops) )
        {
            fmCustomTreeDestroy(&switchPtr->noArpNextHops, NULL);
        }
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_ROUTING);

}   /* end CleanupNextHopTreeInt */




/*****************************************************************************/
/** CleanupArpPointersTree
 * \ingroup intNextHop
 *
 * \desc            Removes all entries from the arpPointersTree
 *
 * \param[in]       sw is the switch number.
 *
 * \return          None.
 *                  
 *****************************************************************************/
static void CleanupArpPointersTree(fm_int sw)
{
    fm_switch      *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d\n",
                     sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr != NULL)
    {
        if ( fmTreeIsInitialized(&switchPtr->arpPointersTree) )
        {
            fmTreeDestroy(&switchPtr->arpPointersTree, NULL);
        }
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_ROUTING);

}   /* end CleanupArpPointersTree */




/*****************************************************************************/
/** CleanupSingleEcmpGroupInt
 * \ingroup intNextHop
 *
 * \desc            Cleans up a single ECMP group and its associated
 *                  Next-Hop tree.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pEcmpGroup points to the ECMP group to be cleaned up.
 *                  It may be NULL.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not supported.
 *                  
 *****************************************************************************/
static fm_status CleanupSingleEcmpGroupInt(fm_int           sw,
                                           fm_intEcmpGroup *pEcmpGroup)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_status               errAux;
    fm_customTreeIterator   iter;
    fm_int                  hops;
    fm_intRouteEntry       *key;
    fm_intRouteEntry       *route;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pEcmpGroup=%p\n",
                     sw,
                     (void *) pEcmpGroup);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pEcmpGroup == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ecmpGroups == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        /* clean up the tree of supported routes */
        if ( fmCustomTreeIsInitialized(&pEcmpGroup->routeTree) )
        {
            while (err == FM_OK)
            {
                fmCustomTreeIterInit(&iter, &pEcmpGroup->routeTree);

                err = fmCustomTreeIterNext( &iter,
                                           (void **) &key,
                                           (void **) &route);
                if (err == FM_OK)
                {
                    fmCustomTreeRemoveCertain(&pEcmpGroup->routeTree,
                                              key,
                                              NULL);
                }
            }
            /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
            err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
        }

        /* errors should be handled at family function level,
         * they are ignored here */

        FM_API_CALL_FAMILY(errAux, switchPtr->FreeEcmpGroup, sw, pEcmpGroup);
        if (errAux != FM_OK)
        {
            /* just to keep Klockwork happy */
            errAux = FM_OK;
        }

        /* free next-hop record tables */
        for (hops = 0 ; hops < pEcmpGroup->nextHopCount ; hops++)
        {
            if (pEcmpGroup->nextHops[hops] != NULL)
            {
                fmFree(pEcmpGroup->nextHops[hops]);
                pEcmpGroup->nextHops[hops] = NULL;
            }
        }

        /* free the on-demand allocated memory */
        if (pEcmpGroup->nextHops != NULL)
        {
            fmFree(pEcmpGroup->nextHops);
        }
        fmFree(pEcmpGroup);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CleanupSingleEcmpGroupInt */




/*****************************************************************************/
/** FreeArp
 * \ingroup intRouter
 *
 * \desc            Releases ARP entry during switch initialization/shutdown.
 *
 * \param[in]       key points to the key.
 *
 * \param[in]       value points to the arp entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static void FreeArp(void *key,
                    void *value)
{
    fm_intArpEntry *arpPtr = value;

    FM_NOT_USED(key);

    fmCustomTreeDestroy(&arpPtr->nextHopTree, NULL);
    fmRemoveArp(arpPtr->switchPtr, arpPtr);
    fmFree(arpPtr);

}   /* end FreeArp */




/*****************************************************************************/
/** CleanupArpEntriesInt
 * \ingroup intNextHop
 *
 * \desc            Removes all ARP entries from the ARP tree
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status CleanupArpEntriesInt(fm_int sw)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intArpEntry *      pFirstArp;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d\n",
                  sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    pFirstArp = fmGetFirstArp(switchPtr);

    while (pFirstArp != NULL)
    {
        err = fmTreeRemove(&switchPtr->arpPointersTree,
                           (fm_uintptr)pFirstArp,
                           NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        fmCustomTreeDestroy(&pFirstArp->nextHopTree, NULL);
        fmRemoveArp(switchPtr, pFirstArp);
        fmFree(pFirstArp);
        pFirstArp = fmGetFirstArp(switchPtr);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    
}   /* end CleanupArpEntriesInt */




/*****************************************************************************/
/** CleanupEcmpGroupsInt
 * \ingroup intNextHop
 *
 * \desc            Removes all ECMP groups and cleanup the Next-Hop tree
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status CleanupEcmpGroupsInt(fm_int sw)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_int                  group;
    fm_intEcmpGroup        *pEcmpGroup;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d\n",
                  sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (switchPtr->ecmpGroups != NULL)
    {
        for ( group = 0 ; group < switchPtr->maxArpEntries ; group++ )
        {
            pEcmpGroup = switchPtr->ecmpGroups[group];
            if (pEcmpGroup != NULL)
            {
                err = CleanupSingleEcmpGroupInt(sw, pEcmpGroup);
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                 "while cleaning up ECMP group\n");
                }
                switchPtr->ecmpGroups[group] = NULL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    
}   /* end CleanupEcmpGroupsInt */




/*****************************************************************************/
/** CleanupInterfacesInt
 * \ingroup intNextHop
 *
 * \desc            Removes all ECMP groups and the Next-Hop tree
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *                  
 *****************************************************************************/
static fm_status CleanupInterfacesInt(fm_int sw)
{
    fm_switch                      *switchPtr;
    fm_status                       err;
    fm_int                          index;
    fm_intIpInterfaceEntry         *pEntry;
    fm_intIpInterfaceAddressEntry  *addrPtr;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d\n",
                     sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if ( (switchPtr->maxIpInterfaces > 0) && (switchPtr->ipInterfaceEntries != NULL) )
    {
        for (index = 0 ; index < switchPtr->maxIpInterfaces ; index++)
        {
            pEntry = &switchPtr->ipInterfaceEntries[index];

            while (pEntry->firstAddr != NULL)
            {
                addrPtr = pEntry->firstAddr;
                if ( fmCustomTreeIsInitialized(&addrPtr->nextHopTree) )
                {
                    fmCustomTreeDestroy(&addrPtr->nextHopTree, NULL);
                }

                fmRemoveInterfaceAddress(pEntry, addrPtr);
                fmFree(addrPtr);
            }
            pEntry->interfaceNum = -1;
        }

        if ( fmCustomTreeIsInitialized(&switchPtr->noInterfaceNextHops) )
        {
            fmCustomTreeDestroy(&switchPtr->noInterfaceNextHops, NULL);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CleanupInterfacesInt */




/*****************************************************************************/
/** UpdateNextHopInterfaceAddrInt
 * \ingroup intNextHop
 *
 * \desc            Updates all next-hops that use the given interface address.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pAddr points to the IP address assigned to the interface.
 * 
 * \param[in]       pAddrEntry points to the interface address entry.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *                  
 *****************************************************************************/
static fm_status UpdateNextHopInterfaceAddrInt(fm_int                          sw,
                                               fm_ipAddr                      *pAddr,
                                               fm_intIpInterfaceAddressEntry  *pAddrEntry)
{
    fm_switch               *switchPtr;
    fm_status                err;
    fm_int                   sizeNextHopTab;
    fm_int                   sizeEcmpGroupIdTab;
    fm_int                   index;
    fm_int                   nextHopCount;
    fm_int                   ecmpGroupCount;
    fm_customTreeIterator    nextHopIter;
    fm_int                  *pEcmpGroupIdTab;
    fm_intNextHop          **pNextHopTab;
    fm_intNextHop           *pNextHop;
    fm_intNextHop           *pNextHopValue;
    fm_intEcmpGroup         *pEcmpGroup;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pAddr=%p, pAddrEntry=%p\n",
                     sw,
                     (void* ) pAddr,
                     (void* ) pAddrEntry);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* argument validation */
    if ( pAddr == NULL || pAddrEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pEcmpGroupIdTab = NULL;
        pNextHopTab = NULL;
        nextHopCount = 0;
        ecmpGroupCount = 0;
        sizeEcmpGroupIdTab = 0;
        sizeNextHopTab = 0;

        /* determine the number of ECMP groups to be updated */
        if (&switchPtr->ecmpGroupsInUse != NULL)
        {
            /* determine the size of the required tables */
            err = fmGetBitArrayNonZeroBitCount( &switchPtr->ecmpGroupsInUse ,&sizeEcmpGroupIdTab);
            sizeNextHopTab = fmCustomTreeSize(&switchPtr->noInterfaceNextHops);
            if ( sizeNextHopTab < sizeEcmpGroupIdTab)
            {
                sizeNextHopTab = sizeEcmpGroupIdTab;
            }
        }

        /* continue only if there are ECMP groups to be notified */
        if ( (err == FM_OK) && (sizeEcmpGroupIdTab > 0) ) 
        {
            /* Alloc table of ECMP groups ID to be updated */
            pEcmpGroupIdTab = (fm_int*) fmAlloc(sizeEcmpGroupIdTab * sizeof(fm_int) );

            /* Alloc table to keep NextHops to be updated */
            pNextHopTab = (fm_intNextHop**) fmAlloc(sizeNextHopTab * sizeof(fm_intNextHop*) );

            if ( pNextHopTab == NULL || pEcmpGroupIdTab == NULL )
            {
                err = FM_ERR_NO_MEM;
            }
            else
            {
                FM_CLEAR(*pNextHopTab);
                FM_CLEAR(*pEcmpGroupIdTab);
        
                /**************************************************************************
                 * Find and update all next-hops that use this interface address. Because
                 * we can't modify the noInterfaceNextHops tree while iterating it, we
                 * find each next-hop and save its address in a local holding table, then
                 * process the next-hops once we have the full list.
                 **************************************************************************/
                fmCustomTreeIterInit(&nextHopIter, &switchPtr->noInterfaceNextHops);
        
                /* Fill a table of next-hops that belong to the interface address */
                while ( (nextHopCount < sizeNextHopTab) && err == FM_OK)
                {
                    err = fmCustomTreeIterNext( &nextHopIter,
                                                (void *) &pNextHop,
                                                (void *) &pNextHopValue );

                    if (err != FM_OK)
                    {
                        continue;
                    }

                    switch (pNextHop->nextHop.type)
                    {
                        case FM_NEXTHOP_TYPE_ARP:
                            if ( fmCompareIPAddresses(&pNextHop->nextHop.data.arp.interfaceAddr, pAddr) == 0 )
                            {
                                /* Add the next-hop to the holding table. */
                                pNextHopTab[nextHopCount++] = pNextHop;
                            }
                            break;

                        case FM_NEXTHOP_TYPE_MPLS_ARP:
                            if ( fmCompareIPAddresses(&pNextHop->nextHop.data.mplsArp.interfaceAddr, pAddr) == 0 )
                            {
                                /* Add the next-hop to the holding table. */
                                pNextHopTab[nextHopCount++] = pNextHop;
                            }
                            break;

                        default:
                            break;
                    }
                }

                /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
                err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
                
                /* Now process each next-hop and build a table of ECMP groups to update */
                for (index = 0; index < nextHopCount; index++)
                {
                    /* Point the next-hop to this interface address entry. */
                    (pNextHopTab[index])->interfaceAddressEntry = pAddrEntry;
                    pNextHop = pNextHopTab[index];

                    /* Add the next-hop to the interface address next-hop tree */
                    err = fmCustomTreeInsert(&pAddrEntry->nextHopTree, pNextHop, pNextHop);
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot add nextHop to interface address tree\n");
                        continue;
                    }
        
                    FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                                  "NextHop %p added to addrEntry %p next-hop-tree\n",
                                  (void *) pNextHop,
                                  (void *) pAddrEntry );
        
                    /* Remove the next-hop from the no-interface-next-hops tree. */
                    err = fmCustomTreeRemoveCertain(&switchPtr->noInterfaceNextHops,
                                                    pNextHop,
                                                    NULL);
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot remove nextHop from noInterfaceNextHops tree\n");
                        continue;
                    }
                    pEcmpGroupIdTab[ecmpGroupCount++] = pNextHop->ecmpGroup->groupId;
                }
        
                /* Finally, update each ECMP group */
                for (index = 0; index < ecmpGroupCount; index++)
                {
                    if ( !(pEcmpGroupIdTab[index] < 0 || pEcmpGroupIdTab[index] >= switchPtr->maxArpEntries) )
                    {
                        pEcmpGroup = switchPtr->ecmpGroups[pEcmpGroupIdTab[index]];
        
                        /* Update the ECMP Group */
                        fmUpdateEcmpGroupInternal(sw, pEcmpGroup);
                    }
                    else
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Invalid Group ID\n");
                    }
                }

            }    /* end  else if ( pNextHopTab == NULL || pEcmpGroupIdTab == NULL ) */

            if ( pNextHopTab != NULL)
            {
                fmFree(pNextHopTab);
                pNextHopTab = NULL;
            }
            if ( pEcmpGroupIdTab != NULL)
            {
                fmFree(pEcmpGroupIdTab);
                pEcmpGroupIdTab = NULL;
            }
        }   /* end if ( (err == FM_OK) && (sizeEcmpGroupIdTab > 0) ) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateNextHopInterfaceAddrInt */




/*****************************************************************************/
/** CheckIfInterfaceVlanIsAlreadyUsed
 * \ingroup intNextHop
 *
 * \desc            Verifies if a given Vlan is already used by an interface.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       vlan is the vlan to be verified
 * 
 * \return          FM_OK if successful: the given vlan is not used by other
 *                   interface
 *                  FM_ERR_VLAN_ALREADY_ASSIGNED if the vlan is already
 *                   assigned.
 *
 *****************************************************************************/
static fm_status CheckIfInterfaceVlanIsAlreadyUsed(fm_int      sw,
                                                   fm_uint16   vlan)
{
    fm_status               err;
    fm_int                  ifId;
    fm_intIpInterfaceEntry *pIfEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, vlan=%d\n",
                     sw,
                     vlan);

    err = fmGetInterfaceFirst(sw, &ifId);

    while (err == FM_OK)
    {
        err = fmGetInterface(sw, ifId, &pIfEntry);
        if ( (err == FM_OK) && (pIfEntry != NULL) )
        {
            if (pIfEntry->vlan == vlan)
            {
                /* the given vlan is already in use */
                err = FM_ERR_VLAN_ALREADY_ASSIGNED;
                break;
            }
        }
        err = fmGetInterfaceNext(sw, ifId, &ifId);
    }
    if (err == FM_ERR_NO_MORE)
    {
        err = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* CheckIfInterfaceVlanIsAlreadyUsed */




/*****************************************************************************/
/** UpdateNextHopArpEntryAddedInt
 * \ingroup intNextHop
 *
 * \desc            Updates all next-hops that use the given interface ARP
 *                  ipAddr after the ARP entry was added
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pAddr points to the IP address assigned to the ARP entry.
 * 
 * \param[in]       pArpEntry points to the interface address entry.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *                  
 *****************************************************************************/
static fm_status UpdateNextHopArpEntryAddedInt(fm_int           sw,
                                               fm_ipAddr       *pAddr,
                                               fm_intArpEntry  *pArpEntry)
{
    fm_switch               *switchPtr;
    fm_status                err;
    fm_int                   vlan;
    fm_int                   sizeNextHopTab;
    fm_int                   index;
    fm_int                   nextHopCount;
    fm_customTreeIterator    nextHopIter;
    fm_intNextHop          **ppNextHopTab;
    fm_intNextHop           *pNextHop;
    fm_intNextHop           *pNextHopValue;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pAddr=%p, pArpEntry=%p\n",
                     sw,
                     (void* ) pAddr,
                     (void* ) pArpEntry);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* argument validation */
    if ( pAddr == NULL || pArpEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        ppNextHopTab = NULL;
        nextHopCount = 0;
        sizeNextHopTab = 0;
        vlan = pArpEntry->arp.vlan;

        /* determine the number of NextHop to be updated */
         sizeNextHopTab = fmCustomTreeSize(&switchPtr->noArpNextHops);

        /* continue only if there are something to do */
        if ( sizeNextHopTab > 0 ) 
        {
            /* Alloc a table to keep NextHops to be updated */
            ppNextHopTab = (fm_intNextHop**) fmAlloc(sizeNextHopTab * sizeof(fm_intNextHop*) );

            if ( ppNextHopTab == NULL)
            {
                err = FM_ERR_NO_MEM;
            }
            else
            {
                FM_CLEAR(*ppNextHopTab);
        
                /**************************************************************************
                 * Find and update all next-hops that use this arp address. It is not 
                 * possible to remove nodes the from noArpNextHops tree whitout breaking 
                 * the iterator, so the nodes are saved in a table for now 
                 **************************************************************************/
                fmCustomTreeIterInit(&nextHopIter, &switchPtr->noArpNextHops);
        
                /* Fill a table with the nexhops to be updated */
                while ( (nextHopCount < sizeNextHopTab) && err == FM_OK)
                {
                    err = fmCustomTreeIterNext( &nextHopIter,
                                                (void**) &pNextHop,
                                                (void**) &pNextHopValue );
        
                    if (err != FM_OK)
                    {
                        continue;
                    }

                    switch (pNextHop->nextHop.type)
                    {
                        case FM_NEXTHOP_TYPE_ARP:
                            if ( fmCompareIPAddresses(&pNextHop->nextHop.data.arp.addr, pAddr) == 0 &&
                                 pNextHop->vlan == vlan)
                            {
                                /* Add the next-hop to the holding table. */
                                ppNextHopTab[nextHopCount++] = pNextHop;
                            }
                            break;

                        case FM_NEXTHOP_TYPE_MPLS_ARP:
                            if ( fmCompareIPAddresses(&pNextHop->nextHop.data.mplsArp.addr, pAddr) == 0 )
                            {
                                /* Add the next-hop to the holding table. */
                                ppNextHopTab[nextHopCount++] = pNextHop;
                            }
                            break;

                        default:
                            break;
                    }
                }        

                /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
                err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
        
                /* Now process each next-hop and build a table of ECMP groups to update */
                for (index = 0; index < nextHopCount; index++)
                {
                    /* Point the next-hop to the given ARP entry. */
                    ppNextHopTab[index]->arp = pArpEntry;
                    pNextHop = ppNextHopTab[index];

                    /* Add the next-hop to the ARP's next-hop tree */
                    err = fmCustomTreeInsert(&pArpEntry->nextHopTree, pNextHop, pNextHop);
                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot add nextHop to nextHop tree\n");
                        continue;
                    }
                    else
                    {

                        FM_API_CALL_FAMILY(err,
                                           switchPtr->UpdateNextHop,
                                           sw,
                                           pNextHop);
                    }

                    if (err == FM_OK)
                    {
                        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                                      "NextHop %p added to arpEntry %p next-hop-tree\n",
                                      (void *) pNextHop,
                                      (void *) pArpEntry );
                    }
                    else
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                                     "Execution error updating NextHop at HW level\n");
                    }
        
                    /* Remove the next-hop from the no-arp-nexthops tree. */
                    err = fmCustomTreeRemoveCertain(&switchPtr->noArpNextHops,
                                                    pNextHop,
                                                    NULL);
                    if (err != FM_OK)
                    {
                        /* this should not happen... */
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot remove nextHop from noInterfaceNextHops tree\n");
                    }
                }       
                fmFree(ppNextHopTab);
                ppNextHopTab = NULL;
            }
        }   /* end if ( sizeNextHopTab > 0 ) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateNextHopArpEntryAddedInt */




/*****************************************************************************/
/** UpdateNextHopArpEntryRemovedInt
 * \ingroup intNextHop
 *
 * \desc            Updates all next-hops that use the given interface ARP
 *                  ipAddr after the ARP entry was deleted
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pArpEntry points to the interface address entry.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *                  
 *****************************************************************************/
static fm_status UpdateNextHopArpEntryRemovedInt(fm_int           sw,
                                                 fm_intArpEntry  *pArpEntry)
{
    fm_switch               *switchPtr;
    fm_status                err;
    fm_customTreeIterator    nextHopIter;
    fm_intNextHop           *pNextHop;
    fm_intNextHop           *pNextHopValue;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pArpEntry=%p\n",
                     sw,
                     (void* ) pArpEntry);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* argument validation */
    if ( pArpEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* Find and update all affected next-hops */
        fmCustomTreeIterInit(&nextHopIter, &pArpEntry->nextHopTree);

        while (err == FM_OK)
        {
            err = fmCustomTreeIterNext( &nextHopIter,
                                        (void **) &pNextHop,
                                        (void **) &pNextHopValue);

            if (err == FM_OK)
            {
                pNextHop->arp = NULL;
                err = fmCustomTreeInsert(&switchPtr->noArpNextHops, pNextHop, pNextHop);
            }
            if (err == FM_OK)
            {
                FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                              "next hop %p added to noArpNextHops tree\n",
                              (void *) pNextHop );

                FM_API_CALL_FAMILY(err,
                                   switchPtr->UpdateNextHop,
                                   sw,
                                   pNextHop);
            }
        }
        /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
        err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateNextHopArpEntryRemovedInt */




/*****************************************************************************/
/** UpdateArpEntriesVlanValue
 * \ingroup intNextHop
 *
 * \desc            Finds and updates the vlan in all ARP entries that use the
 *                  given interface.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pIfEntry is the interface whose vlan that has been updated
 * 
 * \param[in]       vlan is the updated vlan number.
 * 
 * \return          FM_OK if successful: the given vlan is not used by other
 *                   interface
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_VLAN_ALREADY_ASSIGNED if the vlan is already assigned.
 *
 *****************************************************************************/
static fm_status UpdateArpEntriesVlanValue(fm_int                  sw,
                                           fm_intIpInterfaceEntry *pIfEntry,
                                           fm_uint16               vlan)
{
    fm_switch      *switchPtr;
    fm_status       err;
    fm_intArpEntry *pArpEntry;
    fm_intArpEntry *pArpEntryInsertPoint;
    fm_status       localErr; 

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, ifEntry=%p, vlan=%d\n",
                     sw,
                     (void*)pIfEntry,
                     vlan);
    
    err = FM_OK;

    /* argument validation */
    if (pIfEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

    switchPtr = GET_SWITCH_PTR(sw);
    pArpEntry = fmGetFirstArp(switchPtr);

    while (pArpEntry != NULL && err == FM_OK)
    {
        if (pArpEntry->ifEntry == pIfEntry)
        {
            fmRemoveArp(switchPtr, pArpEntry);
            /* update the ARP entry's vlan */
            pArpEntry->arp.vlan = vlan;
            err = FindArpEntryExt(sw,
                                  &pArpEntry->arp,
                                  &pArpEntryInsertPoint);
            if (err == FM_ERR_NOT_FOUND)
            {
                fmInsertArpAfter(switchPtr, pArpEntryInsertPoint, pArpEntry);
                err = FM_OK;
            }
            else
            {
                localErr = fmTreeRemove(&switchPtr->arpPointersTree,
                                        (fm_uintptr)pArpEntry,
                                        NULL);
                FM_ERR_COMBINE(err, localErr);
            }
        }
        pArpEntry = fmGetNextArp(pArpEntry);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateArpEntriesVlanValue*/




/*****************************************************************************/
/** AllocEcmpGroupInt
 * \ingroup intNextHop
 *
 * \desc            Allocates memory and intiailizates an ECMP group
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       maxNextHops is the max number of nexthop entries.
 * 
 * \param[in]       lbsVlan is the loopback suppression VLAN ID to store in
 *                  the next-hop entry of the ECMP group attached to a
 *                  multicast group
 * 
 * \param[in]       wideNextHops indicates whether the ECMP group uses
 *                  narrow (FALSE) or wide next-hops max number of nexthop
 *                  entries (TRUE).
 * 
 * \param[in]       mplsGroup indicates whether the ECMP group is used for MPLS.
 * 
 * \param[in]       numFixedEntries max number of nexthop entries.
 * 
 * \param[in]       mcastGroup points to the multicast group, if this ECMP
 *                  group is for a multicast address.  It is NULL if it is
 *                  for unicast.  ~0 means create the multicast drop group.
 * 
 * \param[out]      ppEcmpGroup points to caller-allocated storage into which the
 *                  function will place the pointer to the new ECMP group
 *                  
 * \param[out]      groupId points to caller-allocated storage into which the
 *                  function will place the group allocation index (offset)
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if resources cannot be allocated
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
static fm_status AllocEcmpGroupInt(fm_int                sw,
                                   fm_int                maxNextHops,
                                   fm_uint               lbsVlan,
                                   fm_bool               wideNextHops,
                                   fm_bool               mplsGroup,
                                   fm_int                numFixedEntries,
                                   fm_intMulticastGroup *mcastGroup,
                                   fm_intEcmpGroup     **ppEcmpGroup,
                                   fm_int               *groupId)
{
    fm_switch  *switchPtr;
    fm_status   err;
    fm_int      ecmpGroupNdx;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, maxNextHops = %d, lbsVlan = %u, wideNextHops = %d, "
                  "mplsGroup = %d, numFixedEntries = %d, mcastGroup = %p, "
                  "ppEcmpGroup = %p, groupId = %p\n",
                  sw,
                  maxNextHops,
                  lbsVlan,
                  wideNextHops,
                  mplsGroup,
                  numFixedEntries,
                  (void *) mcastGroup,
                  (void *) ppEcmpGroup,
                  (void *) groupId );

    switchPtr = GET_SWITCH_PTR(sw);

    /* arg validation */
    if ( ppEcmpGroup == NULL || groupId == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    *ppEcmpGroup = NULL;

    /* find an available ECMP group */
    err = fmFindBitInBitArray(&switchPtr->ecmpGroupsInUse,
                              0,
                              FALSE,
                              &ecmpGroupNdx);

    if ( (err != FM_OK) || (ecmpGroupNdx < 0) || (ecmpGroupNdx >= switchPtr->maxArpEntries) )
    {
        err = FM_ERR_TABLE_FULL;
    }
    else
    {
        *ppEcmpGroup = fmAlloc( sizeof(fm_intEcmpGroup) );

        if (*ppEcmpGroup == NULL)
        {
            err = FM_ERR_NO_MEM;
        }
        else
        {
            FM_CLEAR(**ppEcmpGroup);

            (*ppEcmpGroup)->groupId          = ecmpGroupNdx;
            (*ppEcmpGroup)->nextHopCount     = 0;
            (*ppEcmpGroup)->mcastGroup       = mcastGroup;
            (*ppEcmpGroup)->maxNextHops      = maxNextHops;
            (*ppEcmpGroup)->lbsVlan          = lbsVlan;
            (*ppEcmpGroup)->numFixedEntries  = numFixedEntries;
            (*ppEcmpGroup)->mplsGroup        = mplsGroup;

            if (numFixedEntries > 0)
            {
                (*ppEcmpGroup)->fixedSize = TRUE;

                if (wideNextHops)
                {
                    (*ppEcmpGroup)->wideGroup = TRUE;
                }
            }

            if (mplsGroup)
            {
                /* All MPLS groups are wide groups. */
                wideNextHops                      = TRUE;
                (*ppEcmpGroup)->wideGroup         = TRUE;
                (*ppEcmpGroup)->isGroupWidthKnown = TRUE;
            }

            *groupId = ecmpGroupNdx;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocEcmpGroupInt */




/*****************************************************************************/
/** AllocNextHopInt
 * \ingroup intNextHop
 *
 * \desc            Allocates memory and initializes internal NextHop
 *                  instances for the specified ECMP group
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       maxNextHops is the max number of nexthop entries.
 * 
 * \param[in]       numFixedEntries is the number of next-hop entries in a
 *                  fixed-size ECMP group; 0 indicates that the group's
 *                  size is adjustable.
 * 
 * \param[out]      pEcmpGroup points to internal ECMP group structure where
 *                  information about allocated next hops is stored.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if resources cannot be allocated
 *
 *****************************************************************************/
static fm_status AllocNextHopInt(fm_int            sw,
                                 fm_int            maxNextHops,
                                 fm_int            numFixedEntries,
                                 fm_intEcmpGroup  *pEcmpGroup)
{
    fm_switch      *switchPtr;
    fm_status       err;
    fm_int          tsize;
    fm_int          index;
    fm_intNextHop  *intNextHop;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, maxNextHops=%d, numFixedEntries=%d, pEcmpGroup=%p\n",
                  sw,
                  maxNextHops,
                  numFixedEntries,
                  (void *) pEcmpGroup);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* alloc an array of pointers to fm_intNextHop */
    tsize = sizeof(fm_intNextHop *) * maxNextHops;
    pEcmpGroup->nextHops = fmAlloc(tsize);

    if (pEcmpGroup->nextHops == NULL)
    {
        err = FM_ERR_NO_MEM;
    }
    else
    {
        FM_MEMSET_S(pEcmpGroup->nextHops, tsize, 0, tsize);

        for (index = 0 ; index < numFixedEntries ; index++)
        {
            intNextHop = fmAlloc( sizeof(fm_intNextHop) );

            if (intNextHop == NULL)
            {
                err = FM_ERR_NO_MEM;
                break;
            }
            else
            {
                /* initialize the next hop structure */
                FM_CLEAR(*intNextHop);

                intNextHop->nextHop.type = FM_NEXTHOP_TYPE_DROP;
                intNextHop->sw           = sw;
                intNextHop->ecmpGroup    = pEcmpGroup;
                intNextHop->state        = FM_NEXT_HOP_STATE_UP;

                /* Save the nexthop pointer into the next hop table */
                pEcmpGroup->nextHops[index]   = intNextHop;
                pEcmpGroup->nextHopCount++;
            }
        }
    }

    /* if error: free all allocated resources */
    if (err != FM_OK)
    {
        if ( pEcmpGroup->nextHops != NULL )
        {
            
            for (index = 0 ; index < numFixedEntries ; index++)
            {
                if (pEcmpGroup->nextHops[index] != NULL)
                {
                    fmFree(pEcmpGroup->nextHops[index]);
                }
            }
            fmFree(pEcmpGroup->nextHops);
            pEcmpGroup->nextHops = NULL;
        }

        pEcmpGroup->nextHopCount = 0;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocNextHopInt */




/*****************************************************************************/
/** ValidateNextHop
 * \ingroup intNextHopArp
 *
 * \desc            Determines if a next-hop is usable (i.e., are there any
 *                  conditions that prevent its use) and stores the result.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       nextHop points to the next-hop record to be tested.
 *
 * \param[out]      updated points to caller-provided storage into which TRUE
 *                  is written if the group's usable status was changed, or the
 *                  usable status of any next-hop in the group was changed.
 *                  Otherwise, FALSE is returned.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ValidateNextHop(fm_int         sw,
                                 fm_intNextHop *nextHop,
                                 fm_bool *      updated)
{
    fm_status               status;
    fm_bool                 isUsable;
    fm_bool                 wasUsable;
    fm_intIpInterfaceEntry *pIfEntry;
    fm_ipAddr *             interfaceAddr;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, nextHop = %p, updated = %p\n",
                  sw,
                  (void *) nextHop,
                  (void *) updated );

    if (nextHop == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }
        
    isUsable  = TRUE;
    status    = FM_OK;
    wasUsable = nextHop->isUsable;

    if (updated != NULL)
    {
        *updated = FALSE;
    }

    /* Next-Hops in a fixed-Size ECMP Group are always usable */
    if (nextHop->ecmpGroup->fixedSize)
    {
        nextHop->isUsable = isUsable;
        if ( (isUsable != wasUsable) && (updated != NULL) )
        {
            *updated = TRUE;
        }
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    /* If the ECMP group and next-hop were created via fmAddRoute (i.e., the
     * original method of creating ECMP groups), the route state needs to
     * be honored.  For all new ECMP groups, the routeState element defaults
     * to FM_ROUTE_STATE_UP when the next-hop was added.
     */
    if (nextHop->routeState != FM_ROUTE_STATE_UP ||
        nextHop->state == FM_NEXT_HOP_STATE_DOWN)    
    {
        isUsable = FALSE;
        nextHop->isUsable = isUsable;
        if ( (isUsable != wasUsable) && (updated != NULL) )
        {
            *updated = TRUE;
        }
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    switch (nextHop->nextHop.type)
    {
        case FM_NEXTHOP_TYPE_ARP:
            interfaceAddr = &nextHop->nextHop.data.arp.interfaceAddr;
            break;

        case FM_NEXTHOP_TYPE_MPLS_ARP:
            interfaceAddr = &nextHop->nextHop.data.mplsArp.interfaceAddr;
            break;

        case FM_NEXTHOP_TYPE_DROP:
            /* DROP uses arp next-hop record for the next-hop IP address,
             * but doesn't use any of the other fields, so there is nothing
             * to test here. It is always usable. */
            interfaceAddr = NULL;
            break;

        default:
            interfaceAddr = NULL;
            break;
    }

    if (interfaceAddr != NULL)
    {
        /* If the next-hop uses an interface IP address, has that
         * address been attached to an interface?  If not, the
         * next-hop is not usable.
         */
        if ( !fmIsIPAddressEmpty(interfaceAddr) )
        {
            if (nextHop->interfaceAddressEntry == NULL)
            {
                isUsable = FALSE;
            }
        }

        /* Check the interface state */
        if ( isUsable && (nextHop->interfaceAddressEntry != NULL) )
        {
            pIfEntry = nextHop->interfaceAddressEntry->ifEntry;

            if (pIfEntry->state != FM_INTERFACE_STATE_ADMIN_UP)
            {
                isUsable = FALSE;
            }
            else
            {
                nextHop->vlan = pIfEntry->vlan;
            }
        }

        /* Check the VLAN for validity */
        if (nextHop->vlan == FM_INVALID_VLAN)
        {
            isUsable = FALSE;
        }
    }

    /* Store the computed state in the next-hop */
    nextHop->isUsable = isUsable;

    if ( (isUsable != wasUsable) && (updated != NULL) )
    {
        *updated = TRUE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end ValidateNextHop */




/*****************************************************************************/
/** ValidateEcmpGroup
 * \ingroup intNextHopArp
 *
 * \desc            Determines if an ECMP group is usable, i.e., does it
 *                  have any next-hops that are usable.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       ecmpGroup points to the ECMP group to be tested.
 *
 * \param[out]      updated points to caller-provided storage into which TRUE
 *                  is written if the group's usable status was changed, or the
 *                  usable status of any next-hop in the group was changed.
 *                  Otherwise, FALSE is returned.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ValidateEcmpGroup(fm_int           sw,
                                   fm_intEcmpGroup *ecmpGroup,
                                   fm_bool *        updated)
{
    fm_status             status;
    fm_intNextHop *       nextHop;
    fm_int                index;
    fm_bool               isUsable;
    fm_bool               hopUpdated;
    fm_bool               wasUsable;
    fm_customTreeIterator iter;
    fm_intRouteEntry *    routeKey;
    fm_intRouteEntry *    route;
    fm_int                nbUpdatedHops;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, ecmpGroup = %p (%d)\n",
                 sw,
                 (void *) ecmpGroup,
                 ecmpGroup->groupId);

    isUsable  = FALSE;
    nbUpdatedHops = 0;
    wasUsable = ecmpGroup->isUsable;

    if (updated != NULL)
    {
        *updated = FALSE;
    }

    /* Multicast and Drop ECMP groups are always ready */
    if (ecmpGroup->mcastGroup != NULL)
    {
        ecmpGroup->isUsable = TRUE;

        if ( (ecmpGroup->isUsable != wasUsable) && (updated != NULL) )
        {
            *updated = TRUE;
        }

        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    /* Check each next-hop in the group.  If we find a single next-hop that
     * is usable, the group is usable, but we still have to check all of the
     * next-hops to insure that they are all initialized properly so that
     * other functions can rely upon each next-hop's isUsable flag.
     */
    for (index = 0 ; index < ecmpGroup->nextHopCount ; index++)
    {
        nextHop    = ecmpGroup->nextHops[index];
        hopUpdated = FALSE;

        if (nextHop != NULL) 
        {
            status = ValidateNextHop(sw, nextHop, &hopUpdated);

            if (status == FM_OK)
            {
                if (nextHop->isUsable)
                {
                    isUsable = TRUE;
                }

                if (hopUpdated)
                {
                    nbUpdatedHops++;
                }
            }
        }
    }

    /* If the group's usability is changing, update all routes dependent
     * on the group.
     */
    if (isUsable != wasUsable || nbUpdatedHops > 0)
    {
        /* Store the new usability state in the group */
        ecmpGroup->isUsable = isUsable;

        if (updated != NULL)
        {
            *updated = TRUE;
        }

        fmCustomTreeIterInit(&iter, &ecmpGroup->routeTree);

        while (1)
        {
            status = fmCustomTreeIterNext(&iter,
                                          (void **) &routeKey,
                                          (void **) &route);

            if (status == FM_ERR_NO_MORE)
            {
                break;
            }

            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

            status = fmSetRouteActiveFlag(sw, route, FALSE);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end ValidateEcmpGroup */




/*****************************************************************************/
/** UpdateEcmpGroupsForInterface
 * \ingroup intNextHopArp
 *
 * \desc            Updates ECMP groups and routes affected by an interface
 *                  change.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pIfEntry points to the interface entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UpdateEcmpGroupsForInterface(fm_int                  sw,
                                              fm_intIpInterfaceEntry *pIfEntry)
{
    fm_switch *                    switchPtr;
    fm_status                      err;
    fm_customTreeIterator          hopIter;
    fm_intNextHop *                nextHopKey;
    fm_intNextHop *                nextHop;
    fm_bitArray                    ecmpGroupList;
    fm_int                         ecmpGroupId;
    fm_intIpInterfaceAddressEntry *addrEntry;
    fm_intEcmpGroup *              ecmpGroup;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, pIfEntry = %p\n",
                 sw,
                 (void*) pIfEntry);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pIfEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "sw = %d, interfaceNum = %d\n",
                     sw,
                     pIfEntry->interfaceNum);

        err = fmCreateBitArray(&ecmpGroupList, switchPtr->maxArpEntries);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        /* Look at all next-hops associated with each interface address entry.
         * Create a list of ECMP groups that need to be updated. */
        addrEntry = pIfEntry->firstAddr;

        while (addrEntry != NULL)
        {
            fmCustomTreeIterInit(&hopIter, &addrEntry->nextHopTree);

            while ( ( err = fmCustomTreeIterNext( &hopIter,
                                                     (void **) &nextHopKey,
                                                     (void **) &nextHop ) ) == FM_OK )
            {
                err = fmSetBitArrayBit(&ecmpGroupList,
                                          nextHop->ecmpGroup->groupId,
                                          TRUE);
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            }

            addrEntry = addrEntry->nextAddr;
        }
        
        /* Update all ECMP Groups with next-hops using this interface */
        ecmpGroupId = -1;

        while (1)
        {
            err = fmFindBitInBitArray(&ecmpGroupList,
                                         ecmpGroupId + 1,
                                         TRUE,
                                         &ecmpGroupId);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

            if (ecmpGroupId < 0)
            {
                break;
            }

            ecmpGroup = switchPtr->ecmpGroups[ecmpGroupId];

            err = fmUpdateEcmpGroupInternal(sw, ecmpGroup);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }
    }


ABORT:

    fmDeleteBitArray(&ecmpGroupList);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end UpdateEcmpGroupsForInterface */




/*****************************************************************************/
/** DeleteInterfaceAddrInt
 * \ingroup intRouterIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Delete an IP address from the given interface. Note that
 *                  any route that was using this address will automatically
 *                  be placed in a disabled state.
 *                  This is an internal function that is not lock protected.
 *                  The caller is responsible for taking the appropiated locks.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pIfEntry is the interface on which to operate.
 *
 * \param[in]       pAddr points to the IP address to be deleted from this
 *                  interface.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument pointer is NULL.
 * \return          FM_ERR_INVALID_IPADDR if the IP address is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
static fm_status DeleteInterfaceAddrInt(fm_int                   sw,
                                          fm_intIpInterfaceEntry  *pIfEntry,
                                          fm_ipAddr               *pAddr)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_intIpInterfaceAddressEntry *pAddrEntry;
    fm_customTreeIterator          nextHopIter;
    fm_intNextHop *                nextHop;
    fm_intNextHop *                nextHopValue;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, pIfEntry=%p pAddr=%p\n",
                     sw,
                     (void*)pIfEntry,
                     (void*)pAddr);

    switchPtr = GET_SWITCH_PTR(sw);
    pAddrEntry = NULL;
    err = FM_OK;

    if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if (pAddr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* find the given address in the address list */
        pAddrEntry = fmGetFirstInterfaceAddress(pIfEntry);

        while ( pAddrEntry != NULL)
        {
            if (fmCompareIPAddresses(&pAddrEntry->addr, pAddr) == 0)
            {
                break;
            }
            pAddrEntry = fmGetNextInterfaceAddress(pAddrEntry);
        }

        if (pAddrEntry != NULL)
        {
            /* remove the given address from nextHop entries */
            fmCustomTreeIterInit(&nextHopIter, &pAddrEntry->nextHopTree);
            while (err == FM_OK)
            {
                err = fmCustomTreeIterNext( &nextHopIter,
                                            (void*) &nextHop,
                                            (void*) &nextHopValue );
                if (err == FM_OK)
                {
                    nextHop->interfaceAddressEntry = NULL;
                }
            }
            /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
            err = (err == FM_ERR_NO_MORE) ? FM_OK : err;

            if (err == FM_OK)
            {
                /* Update all ECMP Groups and routes using this interface */
                err = UpdateEcmpGroupsForInterface(sw, pIfEntry);
            }

            if (err == FM_OK)
            {
                /* Take each next-hop from the interface address next-hop tree
                 * and put it into the noInterfaceNextHops tree */
                while (err == FM_OK)
                {
                    fmCustomTreeIterInit(&nextHopIter, &pAddrEntry->nextHopTree);
                    err = fmCustomTreeIterNext( &nextHopIter,
                                                (void*) &nextHop,
                                                (void*) &nextHopValue );

                    if (err == FM_OK)
                    {
                        err = fmCustomTreeRemoveCertain(&pAddrEntry->nextHopTree,
                                                        nextHop,
                                                        NULL);
                    }

                    if (err == FM_OK)
                    {
                        err = fmCustomTreeInsert( &switchPtr->noInterfaceNextHops,
                                                  nextHop,
                                                  nextHop);
                    }
                }
                /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
                err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
            }
            if (err == FM_OK)
            {
                /* Remove the address record from the interface address list */
                fmRemoveInterfaceAddress(pIfEntry, pAddrEntry);
                fmFree(pAddrEntry);
                pAddrEntry = NULL;
            }
        }
        if ( err != FM_OK)
        {
            FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                         "Deleting interface address. errcode %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
        }
    }

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end DeleteInterfaceAddrInt */




/*****************************************************************************/
/** FindArpEntryExt
 * \ingroup intNextHopArp
 *
 * \desc            Find an ARP entry in the ARP table.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pArp points to the ARP entry to look for.
 *
 * \param[out]      ppArpEntry contains the pointer to the ARP entry,
 *                  or NULL if the entry wasn't found. 
 *
 * \return          FM_OK if the ARP entry was found.
 * \return          FM_ERR_NOT_FOUND if the entry wasn't found.
 * \return          FM_ERR_INVALID_ARGUMENT if pArpAddr is NULL.
 *
 *****************************************************************************/
static fm_status FindArpEntryExt(fm_int           sw,
                                 fm_arpEntry     *pArp,
                                 fm_intArpEntry **ppArpEntry)
{
    fm_switch      *switchPtr;
    fm_status       err;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pArp=%p, ppArpEntry=%p\n",
                  sw,
                  (void*) pArp,
                  (void*) ppArpEntry );

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;


    /* arg validation */
    if (pArp == NULL || ppArpEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmFindArpEntry(sw, &pArp->ipAddr, pArp->vlan, ppArpEntry);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end FindArpEntryExt */




/*****************************************************************************/
/** FindInterfaceAddrEntry
 * \ingroup intNextHopIf
 *
 * \desc            Find an interface address entry for a specified interface
 *                  IP.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pInterfaceAddr points to the Interface IP Address.
 *
 * \param[out]      ppIfAddrEntry points to caller-provided storage into which
 *                  the interface address entry pointer is written if the entry
 *                  is found, or NULL if the entry wasn't found.
 *                  May be NULL, in which case no pointer will be returned. 
 *
 * \return          FM_OK if the interface entry was found.
 * \return          FM_ERR_INVALID_INTERFACE if the entry wasn't found.
 * \return          FM_ERR_INVALID_ARGUMENT if pInterfaceAddr is NULL.
 *
 *****************************************************************************/
static fm_status FindInterfaceAddrEntry(fm_int                          sw,
                                        fm_ipAddr                      *pInterfaceAddr,
                                        fm_intIpInterfaceAddressEntry **ppIfAddrEntry)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_int                         interfaceNum;
    fm_intIpInterfaceEntry        *pEntry;
    fm_intIpInterfaceAddressEntry *pAddrEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pInterfaceAddr=%p, ppIfAddrEntry=%p\n",
                  sw,
                  (void *) pInterfaceAddr,
                  (void *) ppIfAddrEntry );

    err = FM_OK;

    /* set NULL as returned pointer by default */
    if (ppIfAddrEntry != NULL)
    {
        *ppIfAddrEntry = NULL;
    }

    /* arg validation */
    if ( pInterfaceAddr == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (!fmIsIPAddressEmpty(pInterfaceAddr) )
    {
        switchPtr    = GET_SWITCH_PTR(sw);
        pAddrEntry   = NULL;
        interfaceNum = -1;

        while (err == FM_OK)
        {
            err = fmFindBitInBitArray(&switchPtr->ipInterfaceEntriesInUse,
                                      interfaceNum + 1,
                                      TRUE,
                                      &interfaceNum);
            if (err == FM_OK)
            {
                if (interfaceNum < 0)
                {
                    err = FM_ERR_INVALID_INTERFACE;
                }
                else
                {

                    pEntry = &switchPtr->ipInterfaceEntries[interfaceNum];

                    pAddrEntry = fmGetFirstInterfaceAddress(pEntry);

                    while (pAddrEntry != NULL)
                    {
                        /* quit the inner loop if the ip addresses match */
                        if ( fmCompareIPAddresses(pInterfaceAddr, &pAddrEntry->addr) == 0)
                        {
                            break;
                        }
                        pAddrEntry = fmGetNextInterfaceAddress(pAddrEntry);
                    }

                    /* pAddrEntry != NULL means the Ip addr matches: exit the loop  */
                    if (pAddrEntry != NULL)
                    {
                        break;
                    }
                }
            }
        }   /* end while (err == FM_OK) */

        if (err == FM_OK && ppIfAddrEntry != NULL)
        {
            if ( pAddrEntry != NULL)
            {
                *ppIfAddrEntry= pAddrEntry;
            }
            else
            {
                /* the given addr wasn't found */
                err = FM_ERR_INVALID_INTERFACE;
            }
        }

    }   /* else if ( !fmIsIPAddressEmpty(pInterfaceAddr) ) end */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end FindInterfaceAddrEntry */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmNextHopAlloc
 * \ingroup intNextHop
 *
 * \desc            Allocate resources needed by the nexthop subsystem.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if resources cannot be allocated
 *
 *****************************************************************************/
fm_status fmNextHopAlloc(fm_int sw)
{
    fm_switch *switchPtr;
    fm_int     tsize;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d\n",
                 sw);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    /* Initialize all pointers */
    switchPtr->ipInterfaceEntries  = NULL;
    switchPtr->ipInterfaceEntries = NULL;
    switchPtr->ecmpGroups = NULL;
    switchPtr->ecmpGroupsInUse.bitArrayData = NULL;
    switchPtr->ipInterfaceEntriesInUse.bitArrayData = NULL;

    /* only if nexthop is supported or switchPtr is of SWAG type. */
    if ( (switchPtr->NextHopInit != NULL) ||
         (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG) )
    {
        /**************************************************
         * Allocate ECMP Groups
         **************************************************/
        if (switchPtr->maxArpEntries > 0)
        {
            tsize = sizeof(fm_intEcmpGroup *) * (switchPtr->maxArpEntries);
            switchPtr->ecmpGroups = (fm_intEcmpGroup **) fmAlloc(tsize);
            if (switchPtr->ecmpGroups != NULL)
            {
                FM_MEMSET_S(switchPtr->ecmpGroups, tsize, 0, tsize);
                err = fmCreateBitArray(&switchPtr->ecmpGroupsInUse,
                                       switchPtr->maxArpEntries);
            }
            else
            {
                err = FM_ERR_NO_MEM;
            }
        }

        if (err == FM_OK)
        {
            /**************************************************
             * Allocate Interface Table
             **************************************************/
            if (switchPtr->maxIpInterfaces > 0)
            {
                tsize = sizeof(fm_intIpInterfaceEntry) * (switchPtr->maxIpInterfaces);
                switchPtr->ipInterfaceEntries = (fm_intIpInterfaceEntry *) fmAlloc(tsize);

                if (switchPtr->ipInterfaceEntries != NULL)
                {
                    FM_MEMSET_S(switchPtr->ipInterfaceEntries, tsize, 0, tsize);
                    err = fmCreateBitArray(&switchPtr->ipInterfaceEntriesInUse,
                                           switchPtr->maxIpInterfaces);
                }
                else
                {
                    err = FM_ERR_NO_MEM;
                }
            }
        }
    }

    /* if error: free all allocated memory */
    if (err != FM_OK)
    {
        fmNextHopFree(sw);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmNextHopAlloc */




/*****************************************************************************/
/** fmNextHopFree
 * \ingroup intNextHop
 *
 * \desc            Release all nexthop resources held by a switch. All the
 *                  data pointers are set to NULL.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmNextHopFree(fm_int sw)
{
    fm_switch *      switchPtr;
    fm_status        err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d\n",
                  sw);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Deallocate ECMP Groups
     **************************************************/
    if (switchPtr->ecmpGroups != NULL)
    {
        fmFree(switchPtr->ecmpGroups);
        switchPtr->ecmpGroups = NULL;
    }
    fmDeleteBitArray(&switchPtr->ecmpGroupsInUse);

    /**************************************************
     * Deallocate Interface Table
     **************************************************/
    if (switchPtr->ipInterfaceEntries != NULL)
    {
        fmFree(switchPtr->ipInterfaceEntries);
        switchPtr->ipInterfaceEntries = NULL;
    }
    fmDeleteBitArray(&switchPtr->ipInterfaceEntriesInUse);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmNextHopFree */




/*****************************************************************************/
/** fmNextHopInit
 * \ingroup intNextHop
 *
 * \desc            Perform initialization for nexthop subsystem, called at
 *                  switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmNextHopInit(fm_int sw)
{
    fm_switch              *switchPtr;
    fm_int                  index;
    fm_status               err;
    fm_intIpInterfaceEntry *pEntry;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, 
                 "sw=%d\n",
                 sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* only if nexthop is supported or switchPtr is of SWAG type. */
    if ( (switchPtr->NextHopInit != NULL) || 
         (switchPtr->switchFamily == FM_SWITCH_FAMILY_SWAG) )
    {
        /**************************************************
         * Initialize ECMP Groups
         **************************************************/
        if (switchPtr->maxArpEntries > 0)
        {
            fmClearBitArray(&switchPtr->ecmpGroupsInUse);
            fmCustomTreeInit(&switchPtr->noArpNextHops, fmBasicTreeCompare);
            fmTreeInit(&switchPtr->arpPointersTree);
        }

        /**************************************************
         * Init ARP Table
         **************************************************/
        if (switchPtr->maxArpEntries > 0)
        {
            FM_DLL_INIT_LIST(switchPtr, firstArp, lastArp);
        }

        /**************************************************
         * Init Interface Table
         **************************************************/
        if (switchPtr->ipInterfaceEntries != NULL)
        {
            fmClearBitArray(&switchPtr->ipInterfaceEntriesInUse);
            fmCustomTreeInit(&switchPtr->noInterfaceNextHops, fmBasicTreeCompare);

            for (index = 0 ; index < switchPtr->maxIpInterfaces ; index++)
            {
                pEntry               = &switchPtr->ipInterfaceEntries[index];
                pEntry->interfaceNum = -1;
                pEntry->vlan         = FM_INVALID_VLAN;
                pEntry->state        = FM_INTERFACE_STATE_ADMIN_DOWN;
                fmInitInterfaceEntryLinkedLists(pEntry);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmNextHopInit */




/*****************************************************************************/
/** fmNextHopCleanup
 * \ingroup intNextHop
 *
 * \desc            Releases memory used by the routing subsystem to support
 *                  a specified switch.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmNextHopCleanup(fm_int sw)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_int                  errCnt;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d\n",
                  sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    errCnt = 0;


    /**************************************************
     * Remove all ARPs entries
     **************************************************/
    err = CleanupArpEntriesInt(sw);

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "while cleaning up ARP entries\n");
        errCnt++;
    }

    /**************************************************
     * Destroy ARP tables
     **************************************************/
    CleanupNextHopTreeInt(sw);

    CleanupArpPointersTree(sw);
    
    /**************************************************
     * Delete all ECMP Groups
     **************************************************/
    err = CleanupEcmpGroupsInt(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "while removing ECMP groups\n");
        errCnt++;
    }

    /**************************************************
     * Clear the ecmpGroupsInUse bitArray
     **************************************************/
    fmClearBitArray(&switchPtr->ecmpGroupsInUse);
    switchPtr->dropEcmpGroup = -1;

    /**************************************************
     * Remove All Interfaces
     **************************************************/
    err = CleanupInterfacesInt(sw);
    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                     "while cleaning up interfaces\n");
        errCnt++;
    }

    if (errCnt > 0)
    {
        /* errors could cause memory leaks */
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"One or more errors during NextHop cleanup\n");
        err = FM_FAIL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmNextHopCleanup */




/*****************************************************************************/
/** fmUpdateEcmpGroupInternal
 * \ingroup intNextHopIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            When an ECMP group is updated, every route that uses that
 *                  group must also be updated to point to the group's new
 *                  location and length in the next-hop table. This function
 *                  performs that update.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       ecmpGroup points to the ECMP group to be updated.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmUpdateEcmpGroupInternal(fm_int           sw,
                                    fm_intEcmpGroup *ecmpGroup)
{
    fm_switch *                    switchPtr;
    fm_status                      err;
    fm_intRouteEntry *             route;
    fm_customTreeIterator          routeIter;
    fm_intRouteEntry *             routeKey;
    fm_bool                        updated;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, ecmpGroup=%p\n",
                  sw,
                  (void *) ecmpGroup );

    switchPtr = GET_SWITCH_PTR(sw);

    /* Validate the ECMP Group */
    ValidateEcmpGroup(sw, ecmpGroup, &updated);

    if (!updated)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    /* Update every route that refers to this ECMP group */
    fmCustomTreeIterInit(&routeIter, &ecmpGroup->routeTree);

    while (1)
    {
        err = fmCustomTreeIterNext(&routeIter,
                                   (void **) &routeKey,
                                   (void **) &route);

        if (err == FM_ERR_NO_MORE)
        {
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        /* Update the route's status - don't update the hardware yet */
        err = fmSetRouteActiveFlag(sw, route, FALSE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        err = fmNotifyVNTunnelAboutEcmpChange(sw, route);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    }   /* while (1) */

    /* Update the hardware ARP table and affected routes */
    FM_API_CALL_FAMILY(err,
                       switchPtr->UpdateEcmpGroup,
                       sw,
                       ecmpGroup);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);


ABORT:

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmUpdateEcmpGroupInternal */




/*****************************************************************************/
/** fmFindNextHopInternal
 * \ingroup intNextHopArp
 *
 * \desc            Finds a next hop record in an ECMP group.
 *
 * \note            This function assumes that the routing lock has already
 *                  been taken.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the ECMP group.
 *
 * \param[in]       nextHop points to a user-interface next-hop record that
 *                  identifies the internal next-hop record to be found.
 *
 * \param[out]      hopIndexPtr points to caller-allocated storage into which
 *                  the index of the next hop in the group's next-hop list
 *                  will be placed, if hopIndexPtr is not NULL.
 *
 * \return          pointer to the internal next hop record or NULL if not found.
 *
 *****************************************************************************/
fm_intNextHop *fmFindNextHopInternal(fm_int           sw,
                                     fm_intEcmpGroup *group,
                                     fm_ecmpNextHop  *nextHop,
                                     fm_int          *hopIndexPtr)
{
    fm_intNextHop  tempNextHop;
    fm_intNextHop *intNextHop;
    fm_int         hopIndex;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, group = %p(%d), nextHop = %p\n",
                  sw,
                  (void *) group,
                  group->groupId,
                  (void *) nextHop );

    FM_CLEAR( tempNextHop );
    tempNextHop.sw        = sw;
    tempNextHop.ecmpGroup = group;
    FM_MEMCPY_S( &tempNextHop.nextHop,
                 sizeof(tempNextHop.nextHop),
                 nextHop,
                 sizeof(*nextHop) );

    for ( hopIndex = 0 ; hopIndex < group->nextHopCount ; hopIndex++)
    {
        intNextHop = group->nextHops[hopIndex];

        if (intNextHop == NULL)
        {
            continue;
        }

        intNextHop->ecmpGroup = group;

        if ( fmCompareInternalNextHops(&tempNextHop, intNextHop) == 0 )
        {
            if (hopIndexPtr != NULL)
            {
                *hopIndexPtr = hopIndex;
            }
            break;
        }
    }

    if (hopIndex >= group->nextHopCount)
    {
        intNextHop = NULL;
    }

    FM_LOG_EXIT_CUSTOM( FM_LOG_CAT_ROUTING,
                        intNextHop,
                        "intNextHop=%p\n",
                        (void *) intNextHop );

}   /* end fmFindNextHopInternal */




/*****************************************************************************/
/** fmCompareInternalNextHops
 * \ingroup intNextHop
 *
 * \desc            Compare Next-Hop structures.
 *
 * \param[in]       first points to the first next-hop structure.
 *
 * \param[in]       second points to the second next-hop structure.
 *
 * \return          -1 if the first next-hop sorts before the second.
 * \return           0 if the next-hop structures are identical.
 * \return           1 if the first next-hop sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareInternalNextHops(const void *first,
                                 const void *second)
{
    fm_int              retval = 0;
    fm_intNextHop *     hop1;
    fm_intNextHop *     hop2;
    fm_uint16           vlan1;
    fm_uint16           vlan2;
    fm_int              i;
    fm_nextHop *        arpHop1;
    fm_nextHop *        arpHop2;
    fm_mplsArpNextHop * mplsHop1;
    fm_mplsArpNextHop * mplsHop2;
    fm_portNextHop    * portHop1;
    fm_portNextHop    * portHop2;
    fm_tunnelNextHop *  tunnelHop1;
    fm_tunnelNextHop *  tunnelHop2;
    fm_vnTunnelNextHop *vnTunnelHop1;
    fm_vnTunnelNextHop *vnTunnelHop2;

    hop1 = (fm_intNextHop *) first;
    hop2 = (fm_intNextHop *) second;

    if (hop1->sw < hop2->sw)
    {
        return -1;
    }
    else if (hop1->sw > hop2->sw)
    {
        return 1;
    }

    if (hop1->ecmpGroup->groupId < hop2->ecmpGroup->groupId)
    {
        return -1;
    }
    else if (hop1->ecmpGroup->groupId > hop2->ecmpGroup->groupId)
    {
        return 1;
    }

    if (hop1->nextHop.type < hop2->nextHop.type)
    {
        return -1;
    }
    else if (hop1->nextHop.type > hop2->nextHop.type)
    {
        return 1;
    }

    switch (hop1->nextHop.type)
    {
        case FM_NEXTHOP_TYPE_ARP:
            arpHop1 = &hop1->nextHop.data.arp;
            arpHop2 = &hop2->nextHop.data.arp;

            vlan1 = fmGetInterfaceVlan(hop1->sw,
                                       &arpHop1->interfaceAddr,
                                       arpHop1->vlan);

            vlan2 = fmGetInterfaceVlan(hop2->sw,
                                       &arpHop2->interfaceAddr,
                                       arpHop2->vlan);

            if (vlan1 < vlan2)
            {
                return -1;
            }
            else if (vlan1 > vlan2)
            {
                return 1;
            }

            retval = fmCompareIPAddresses(&arpHop1->addr, &arpHop2->addr);
            break;

        case FM_NEXTHOP_TYPE_DROP:
            arpHop1 = &hop1->nextHop.data.arp;
            arpHop2 = &hop2->nextHop.data.arp;

            retval = fmCompareIPAddresses(&arpHop1->addr, &arpHop2->addr);
            break;

        case FM_NEXTHOP_TYPE_RAW_NARROW:
            if (hop1->nextHop.data.rawNarrow.value < 
                hop2->nextHop.data.rawNarrow.value)
            {
                return -1;
            }
            else if (hop1->nextHop.data.rawNarrow.value > 
                     hop2->nextHop.data.rawNarrow.value)
            {
                return 1;
            }
            retval = 0;
            break;

        case FM_NEXTHOP_TYPE_RAW_WIDE:
            for (i = 0 ; i < FM_RAW_WIDE_NEXTHOP_SIZE ; i++)
            {
                if (hop1->nextHop.data.rawWide.values[i] < 
                    hop2->nextHop.data.rawWide.values[i])
                {
                    return -1;
                }
                else if (hop1->nextHop.data.rawWide.values[i] > 
                         hop2->nextHop.data.rawWide.values[i])
                {
                    return 1;
                }
            }
            retval = 0;
            break;

        case FM_NEXTHOP_TYPE_MPLS_ARP:
            mplsHop1 = &hop1->nextHop.data.mplsArp;
            mplsHop2 = &hop2->nextHop.data.mplsArp;

            vlan1 = fmGetInterfaceVlan(hop1->sw,
                                       &mplsHop1->interfaceAddr,
                                       mplsHop1->vlan);

            vlan2 = fmGetInterfaceVlan(hop2->sw,
                                       &mplsHop2->interfaceAddr,
                                       mplsHop2->vlan);

            if (vlan1 < vlan2)
            {
                return -1;
            }
            else if (vlan1 > vlan2)
            {
                return 1;
            }

            retval = fmCompareIPAddresses(&mplsHop1->addr, &mplsHop2->addr);
            break;

       case FM_NEXTHOP_TYPE_TUNNEL:
            tunnelHop1 = &hop1->nextHop.data.tunnel;
            tunnelHop2 = &hop2->nextHop.data.tunnel;

            if (tunnelHop1->tunnelGrp < tunnelHop2->tunnelGrp)
            {
                return -1;
            }
            else if (tunnelHop1->tunnelGrp > tunnelHop2->tunnelGrp)
            {
                return 1;
            }

            if (tunnelHop1->tunnelRule < tunnelHop2->tunnelRule)
            {
                return -1;
            }
            else if (tunnelHop1->tunnelRule > tunnelHop2->tunnelRule)
            {
                return 1;
            }
            retval = 0;
            break;

        case FM_NEXTHOP_TYPE_VN_TUNNEL:
            vnTunnelHop1 = &hop1->nextHop.data.vnTunnel;
            vnTunnelHop2 = &hop2->nextHop.data.vnTunnel;

            if (vnTunnelHop1->tunnel < vnTunnelHop2->tunnel)
            {
                retval = -1;
            }
            else if (vnTunnelHop1->tunnel > vnTunnelHop2->tunnel)
            {
                retval = 1;
            }
            else if (vnTunnelHop1->vni < vnTunnelHop2->vni)
            {
                retval = -1;
            }
            else if (vnTunnelHop1->vni > vnTunnelHop2->vni)
            {
                retval = 1;
            }
            else if (vnTunnelHop1->encap && !vnTunnelHop2->encap)
            {
                retval = -1;
            }
            else if (!vnTunnelHop1->encap && vnTunnelHop2->encap)
            {
                retval = 1;
            }
            else
            {
                retval = 0;
            }

            break;

        case FM_NEXTHOP_TYPE_LOGICAL_PORT:
            portHop1 = &hop1->nextHop.data.port;
            portHop2 = &hop2->nextHop.data.port;

            if (portHop1->logicalPort < portHop2->logicalPort)
            {
                retval = -1;
            }
            else if (portHop1->logicalPort > portHop2->logicalPort)
            {
                retval = 1;
            }
            else if (portHop1->routed && !portHop2->routed)
            {
                retval = 1;
            }
            else if (!portHop1->routed && portHop2->routed)
            {
                retval = -1;
            }
            else if (portHop1->routerId < portHop2->routerId)
            {
                retval = -1;
            }
            else if (portHop1->routerId > portHop2->routerId)
            {
                retval = 1;
            }
            else if (portHop1->vlan < portHop2->vlan)
            {
                retval = -1;
            }
            else if (portHop1->vlan > portHop2->vlan)
            {
                retval = 1;
            }
            else
            {
                retval = 0;
            }

            break;

        default:
            retval = 0;
            break;
    }

    return retval;

}   /* end fmCompareInternalNextHops */




/*****************************************************************************/
/** fmBasicTreeCompare
 * \ingroup intRouter
 *
 * \desc            Compare pointers to Next-Hop structures.
 *
 * \param[in]       first points to the first next-hop structure.
 *
 * \param[in]       second points to the second next-hop structure.
 *
 * \return          -1 if the first pointer sorts before the second.
 * \return           0 if the next-hop pointers are identical.
 * \return           1 if the first pointer sorts after the second.
 *
 *****************************************************************************/
fm_int fmBasicTreeCompare(const void *first, const void *second)
{
    if (first == second)
    {
        return 0;
    }
    else if (first < second)
    {
        return -1;
    }
    
    return 1;
    
}   /* end fmBasicTreeCompare */




/*****************************************************************************/
/** fmCreateECMPGroup
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Creates an ARP type, variable size ECMP group. See 
 *                  ''fmCreateECMPGroupV2'' for creating alternative ECMP 
 *                  group types.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      groupId points to caller-allocated storage into which the
 *                  function will place the new ECMP group's ID.
 *
 * \param[in]       numNextHops contains the number of next-hops in nextHopList
 *                  that are to be added to the ECMP group when the group
 *                  is created. Set to 0 if the group is to be created with
 *                  no next-hops. Next-hops may be added later with 
 *                  ''fmAddECMPGroupNextHops''.
 *
 * \param[in]       nextHopList points to an array, numNextHops elements in
 *                  length, of next-hop definitions. May be NULL if
 *                  numNextHops is 0.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 *
 *****************************************************************************/
fm_status fmCreateECMPGroup(fm_int      sw,
                            fm_int *    groupId,
                            fm_int      numNextHops,
                            fm_nextHop *nextHopList)
{
    fm_status       err;
    fm_switch *     switchPtr;
    fm_ecmpNextHop *ecmpNextHopList;
    fm_int          tsize;
    fm_int          index;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %p, numNextHops = %d, "
                     "nextHopList = %p\n",
                     sw,
                     (void *) groupId,
                     numNextHops,
                     (void *) nextHopList);

    VALIDATE_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* args validation */
    if ( (numNextHops < 0) || 
         (numNextHops > switchPtr->maxArpEntries) ||
         ( (numNextHops > 0) && (nextHopList == NULL) ) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        PROTECT_SWITCH(sw);
        err = fmCreateECMPGroupInternal(sw, groupId, NULL, NULL);

        if ( (err == FM_OK) && (numNextHops > 0) )
        {
            /* Allocate and initialize an array of fm_ecmpNextHop structures,
             * copying from the caller's fm_nextHop structures. */
            tsize = numNextHops * sizeof(fm_ecmpNextHop);

            ecmpNextHopList = fmAlloc(tsize);

            if (ecmpNextHopList == NULL)
            {
                err = FM_ERR_NO_MEM;
            }
            else
            {
                FM_MEMSET_S(ecmpNextHopList, tsize, 0, tsize);

                for (index = 0 ; index < numNextHops ; index++)
                {
                    ecmpNextHopList[index].type = FM_NEXTHOP_TYPE_ARP;
                    FM_MEMCPY_S( &ecmpNextHopList[index].data.arp,
                                 sizeof(ecmpNextHopList[index].data.arp),
                                 &nextHopList[index],
                                 sizeof(nextHopList[index]) );
                }

                err = fmAddECMPGroupNextHopsInternal(sw, 
                                                     *groupId,
                                                     numNextHops,
                                                     ecmpNextHopList);
                /* allocated memory is not necessary anymore */
                fmFree(ecmpNextHopList);
                ecmpNextHopList = NULL;
            }
        }
        UNPROTECT_SWITCH(sw);
    }
    
    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmCreateECMPGroup */




/*****************************************************************************/
/** fmCreateECMPGroupV2
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Creates an ECMP group. Provided as a more flexible
 *                  alternative to ''fmCreateECMPGroup''. In addition to being
 *                  able to create variable sized ARP-type ECMP groups, this 
 *                  function also permits creating fixed sized ECMP groups
 *                  for load balancing applications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      groupId points to caller-allocated storage into which the
 *                  function will place the new ECMP group's ID.
 *
 * \param[in]       info points to a structure describing the characteristics
 *                  to be applied to this ECMP group. NULL means to create
 *                  a normal ECMP group (narrow, adjustable-size).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_VALUE if the number of next-hop entries is
 *                  invalid for a fixed size ECMP group. Valid values for fixed
 *                  size ECMP groups are: (1..16,32,64,128,256,512,1024,2048,
 *                  4096).
 *
 *****************************************************************************/
fm_status fmCreateECMPGroupV2(fm_int            sw,
                              fm_int *          groupId,
                              fm_ecmpGroupInfo *info)
{
    fm_status        err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, groupId = %p, info = %p\n",
                      sw,
                      (void *) groupId,
                      (void *) info );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmCreateECMPGroupInternal(sw,
                                    groupId,
                                    info,
                                    NULL);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmCreateECMPGroupV2 */




/*****************************************************************************/
/** fmCreateECMPGroupInternal
 * \ingroup intNextHopArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Creates an ECMP group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[out]      groupId points to caller-allocated storage into which the
 *                  function will place the new ECMP group's ID.
 *
 * \param[in]       info points to the structure describing characteristics
 *                  to be applied to this ECMP group. NULL means to create
 *                  a normal ECMP group (narrow, adjustable-size).
 *
 * \param[in]       mcastGroup points to the multicast group, if this ECMP
 *                  group is for a multicast address.  It is NULL if it is
 *                  for unicast.  ~0 means create the multicast drop group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_TABLE_FULL if all available ECMP groups are in use.
 * \return          FM_ERR_NO_MEM if memory cannot be allocated.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_VALUE if the number of next-hop entries is
 *                  invalid for a fixed size ECMP group. Valid values for fixed
 *                  size ECMP groups are: (1..16,32,64,128,256,512,1024,2048,
 *                  4096).
 *
 *****************************************************************************/
fm_status fmCreateECMPGroupInternal(fm_int                sw,
                                    fm_int               *groupId,
                                    fm_ecmpGroupInfo     *info,
                                    fm_intMulticastGroup *mcastGroup)
{
    fm_status          err;
    fm_status          errValid;
    fm_switch         *switchPtr;
    fm_int             tmpGroupId;
    fm_intEcmpGroup   *pEcmpGroup;
    fm_bool            dropGroup;
    fm_bool            multicast;
    fm_bool            wideNextHops;
    fm_bool            mplsGroup;
    fm_int             numFixedEntries;
    fm_int             maxNextHops;
    fm_uint16          lbsVlan;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, groupId = %p, info = %p, mcastGroup=%p\n",
                  sw,
                  (void *) groupId,
                  (void *) info,
                  (void *) mcastGroup );

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    errValid = FM_OK;
    tmpGroupId = -1;
    pEcmpGroup = NULL;

    /* argument validation */
    if (groupId == NULL ||
        ( (info != NULL) && (info->numFixedEntries < 0) ) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( switchPtr->maxArpEntries <= 0)
    {
        err =  FM_ERR_UNSUPPORTED;
    }
    else
    {
        *groupId = tmpGroupId;

        if (info != NULL)
        {
            /* Retrieve caller-requested characteristics */
            wideNextHops    = info->wideNextHops;
            numFixedEntries = info->numFixedEntries;
            lbsVlan         = info->lbsVlan;
            mplsGroup       = info->isMpls;
            maxNextHops     = numFixedEntries > 0 ? numFixedEntries : switchPtr->maxEcmpGroupSize;
        }
        else
        {
            /* Apply default characteristics. */
            wideNextHops    = FALSE;
            maxNextHops     = switchPtr->maxEcmpGroupSize;
            numFixedEntries = 0;
            lbsVlan         = 0;
            mplsGroup       = FALSE;
        }

        /* assume mcastGroup is NULL */
        dropGroup = FALSE;
        multicast = FALSE;
        if (mcastGroup != NULL)
        {
            if ( mcastGroup == (fm_intMulticastGroup *) ~0 )
            {
                dropGroup = TRUE;
                multicast = FALSE;
            }
            else
            {
                dropGroup = FALSE;
                multicast = TRUE;
            }
        }

        /* gain exclusive access to routing tables */
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            err = AllocEcmpGroupInt(sw,
                                    maxNextHops,
                                    lbsVlan,
                                    wideNextHops,
                                    mplsGroup,
                                    numFixedEntries,
                                    mcastGroup,
                                    &pEcmpGroup,
                                    &tmpGroupId);
            if (err == FM_OK)
            {
                err = AllocNextHopInt(sw,
                                      maxNextHops,
                                      numFixedEntries,
                                      pEcmpGroup);

            }
            if (err == FM_OK)
            {
                fmCustomTreeInit(&pEcmpGroup->routeTree, fmCompareIntRoutes);

                /* call chip level function */
                if (switchPtr->CreateECMPGroup != NULL)
                {
                    err = switchPtr->CreateECMPGroup(sw, pEcmpGroup);
                }
            }
            if (err == FM_OK)
            {
                switchPtr->ecmpGroups[tmpGroupId] = pEcmpGroup;
                err = fmSetBitArrayBit(&switchPtr->ecmpGroupsInUse, tmpGroupId, TRUE);
            }

            /* finally, validate the ECMP group */
            if ( (err == FM_OK) && (multicast || dropGroup || pEcmpGroup->fixedSize) )
            {
                errValid = ValidateEcmpGroup(sw, pEcmpGroup, NULL);
            }

            /* if error, free allocated resources */
            if ( (err != FM_OK)  || (errValid != FM_OK) )
            {
                if (pEcmpGroup != NULL)
                {
                    CleanupSingleEcmpGroupInt(sw, pEcmpGroup);
                    if (tmpGroupId >= 0)
                    {
                        fmSetBitArrayBit(&switchPtr->ecmpGroupsInUse, tmpGroupId, FALSE);
                        switchPtr->ecmpGroups[tmpGroupId] = NULL;
                    }
                }
            }
            else
            {
                /* there is no error: return the group Id */

                *groupId = tmpGroupId;
            }
            fmReleaseWriteLock(&switchPtr->routingLock);
        }
    }
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmCreateECMPGroupInternal */




/*****************************************************************************/
/** fmDeleteECMPGroup
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup'' or ''fmCreateECMPGroupV2''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the ECMP group is in use, i.e.,
 *                  it is associated with one or more routes.
 *
 *****************************************************************************/
fm_status fmDeleteECMPGroup(fm_int sw, fm_int groupId)
{
    fm_status        err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d, groupId = %d\n", sw, groupId);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmDeleteECMPGroupInternal(sw, groupId);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteECMPGroup */




/*****************************************************************************/
/** fmDeleteECMPGroupInternal
 * \ingroup intNextHopArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes an ECMP group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       groupId is the group ID from a prior call to
 *                  fmCreateECMPGroup.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the ECMP group is in use, i.e.,
 *                  it is associated with one or more routes.
 *
 *****************************************************************************/
fm_status fmDeleteECMPGroupInternal(fm_int sw, fm_int groupId)
{
    fm_status        err;
    fm_switch *      switchPtr;
    fm_intEcmpGroup *pEcmpGroup;
    fm_intNextHop *  intNextHop;
    fm_ecmpNextHop * removedHops;
    fm_bool          mayBeDeleted;
    fm_int           i;
    fm_uint          treeSize;

    /* Should remove nexthops all together and not one by one */

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d, groupId = %d\n", sw, groupId);

    switchPtr = GET_SWITCH_PTR(sw);
    pEcmpGroup = NULL;

    /* arg validation */
    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* gain exclusive access to routing tables */
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            if ( (pEcmpGroup = switchPtr->ecmpGroups[groupId]) == NULL )
            {
                /* the group doesn't exist */
                err = FM_ERR_INVALID_ARGUMENT;
            }

            if (err == FM_OK)
            {
                if ( (treeSize = fmCustomTreeSize(&pEcmpGroup->routeTree) ) != 0)
                {
                    err = FM_ERR_ECMP_GROUP_IN_USE;
                }
                else
                {
                    /* check if the ECMP group is being referenced by another subsystem */
                    if (switchPtr->ValidateECMPGroupDeletion != NULL)
                    {
                        err = switchPtr->ValidateECMPGroupDeletion(sw, groupId, &mayBeDeleted);

                        if (err == FM_OK && mayBeDeleted == FALSE)
                        {
                            err = FM_ERR_ECMP_GROUP_IN_USE;

                            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "ECMP group #%d is being referenced and cannot be deleted\n",
                                         groupId);
                        }
                    }
                }
            }

            if (err == FM_OK && pEcmpGroup->nextHopCount > 0)
            {
                /* Remove all existing next hops */
                i = sizeof(fm_ecmpNextHop) * pEcmpGroup->nextHopCount;

                removedHops = fmAlloc(i);

                if (removedHops != NULL)
                {
                    FM_MEMSET_S(removedHops, i, 0, i);

                    for (i = 0 ; i < pEcmpGroup->nextHopCount ; i++)
                    {
                        intNextHop = pEcmpGroup->nextHops[i];

                        FM_MEMCPY_S( &removedHops[i],
                                     sizeof(removedHops[i]),
                                     &intNextHop->nextHop,
                                     sizeof(intNextHop->nextHop) );
                    }

                    err = fmDeleteECMPGroupNextHopsInternal(sw,
                                                            groupId,
                                                            pEcmpGroup->nextHopCount,
                                                            removedHops);

                    fmFree(removedHops);
                }
                else
                {
                    err = FM_ERR_NO_MEM;
                }
            }

            if (err == FM_OK)
            {
                /* Delete the ECMP group from the hardware */
                if (switchPtr->DeleteECMPGroup != NULL)
                {
                    err = switchPtr->DeleteECMPGroup(sw, pEcmpGroup);

                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot delete ECMP group #%d at hw level\n",
                                     groupId);
                    }
                }

                /* Delete the remaining structures and memory allocated by the group */
                fmCustomTreeDestroy(&pEcmpGroup->routeTree, NULL);
                if (pEcmpGroup != NULL)
                {
                    if (pEcmpGroup->nextHops != NULL)
                    {
                        fmFree(pEcmpGroup->nextHops);
                    }
                    fmFree(pEcmpGroup);
                }

                switchPtr->ecmpGroups[groupId] = NULL;

                err = fmSetBitArrayBit(&switchPtr->ecmpGroupsInUse, groupId, FALSE);
            }

            fmReleaseWriteLock(&switchPtr->routingLock);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteECMPGroupInternal */




/*****************************************************************************/
/** fmAddECMPGroupNextHops
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds one or more next-hops to a variable sized ECMP group
 *                  using an ''fm_nextHop'' ARP-type next-hop specification.
 *
 * \note            See ''fmAddECMPGroupNextHopsV2'' for ''fm_ecmpNextHop'' 
 *                  type next-hop specifications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       numNextHops is the number of next-hops in nextHopList.
 *
 * \param[in]       nextHopList points to an array of next hops to be added
 *                  to the group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IS_FULL if the group has reached its
 *                  maximum capacity of ARP entries.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_NO_MEM if the function is unable to allocate
 *                  needed memory.
 * \return          FM_ERR_ALREADY_EXISTS if one of the next-hops being
 *                  added already exists in the ECMP group.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is a fixed sized
 *                  group.
 *
 *****************************************************************************/
fm_status fmAddECMPGroupNextHops(fm_int      sw,
                                 fm_int      groupId,
                                 fm_int      numNextHops,
                                 fm_nextHop *nextHopList)
{
    fm_status       status;
    fm_switch *     switchPtr;
    fm_ecmpNextHop *ecmpNextHopList;
    fm_int          i;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, numNextHops = %d, "
                     "nextHopList = %p\n",
                     sw,
                     groupId,
                     numNextHops,
                     (void *) nextHopList);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr       = GET_SWITCH_PTR(sw);
    ecmpNextHopList = NULL;

    if ( (numNextHops <= 0) || (numNextHops > switchPtr->maxArpEntries) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    /* Allocate and initialize an array of fm_ecmpNextHop structures,
     * copying from the caller's fm_nextHop structures. */
    i = numNextHops * sizeof(fm_ecmpNextHop);

    ecmpNextHopList = fmAlloc(i);
    if (ecmpNextHopList == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    FM_MEMSET_S(ecmpNextHopList, i, 0, i);

    for (i = 0 ; i < numNextHops ; i++)
    {
        ecmpNextHopList[i].type = FM_NEXTHOP_TYPE_ARP;
        FM_MEMCPY_S( &ecmpNextHopList[i].data.arp,
                     sizeof(ecmpNextHopList[i].data.arp),
                     &nextHopList[i],
                     sizeof(nextHopList[i]) );
    }

    /* Add the next-hops to the ECMP Group */
    status = fmAddECMPGroupNextHopsInternal(sw,
                                            groupId,
                                            numNextHops,
                                            ecmpNextHopList);

ABORT:

    if (ecmpNextHopList != NULL)
    {
        fmFree(ecmpNextHopList);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmAddECMPGroupNextHops */




/*****************************************************************************/
/** fmAddECMPGroupNextHopsV2
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds one or more next-hops to a variable sized ECMP group
 *                  using an ''fm_ecmpNextHop'' type next-hop specification.
 *
 * \note            See ''fmAddECMPGroupNextHops'' for ''fm_nextHop'' 
 *                  type next-hop specifications.
 *
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       numNextHops is the number of next-hops in nextHopList.
 *
 * \param[in]       nextHopList points to an array of next hops to be added
 *                  to the group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IS_FULL if the group has reached its
 *                  maximum capacity of ARP entries.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_NO_MEM if the function is unable to allocate
 *                  needed memory.
 * \return          FM_ERR_ALREADY_EXISTS if one of the next-hops being
 *                  added already exists in the ECMP group.
 *
 *****************************************************************************/
fm_status fmAddECMPGroupNextHopsV2(fm_int          sw,
                                   fm_int          groupId,
                                   fm_int          numNextHops,
                                   fm_ecmpNextHop *nextHopList)
{
    fm_status status;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, groupId = %d, numNextHops = %d, "
                      "nextHopList = %p\n",
                      sw,
                      groupId,
                      numNextHops,
                      (void *) nextHopList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    status = fmAddECMPGroupNextHopsInternal(sw,
                                            groupId,
                                            numNextHops,
                                            nextHopList);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmAddECMPGroupNextHopsV2 */




/*****************************************************************************/
/** fmAddECMPGroupNextHopsInternal
 * \ingroup intNextHopArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Adds one or more next hops to an ECMP group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       groupId is the group ID from a prior call to
 *                  fmCreateECMPGroup.
 *
 * \param[in]       numNextHops is the number of next-hops in nextHopList.
 *
 * \param[in]       nextHopList points to an array of next hops to be added
 *                  to the group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_ECMP_GROUP_IS_FULL if the group's ARP table is full.
 * \return          FM_ERR_TABLE_FULL if the hardware ARP table is full.
 * \return          FM_ERR_NO_MEM if the function is unable to allocate
 *                  needed memory.
 * \return          FM_ERR_ALREADY_EXISTS if one of the next-hops being
 *                  added already exists in the ECMP group.
 *
 *****************************************************************************/
fm_status fmAddECMPGroupNextHopsInternal(fm_int          sw,
                                         fm_int          groupId,
                                         fm_int          numNextHops,
                                         fm_ecmpNextHop *nextHopList)
{
    fm_status               status;
    fm_switch *             switchPtr;
    fm_intEcmpGroup *       group;
    fm_int                  index;
    fm_ecmpNextHop *        nextHop;
    fm_intNextHop *         intNextHop;
    fm_int                  addedNextHops;
    fm_int                  newNextHopCount;
    fm_bool                 wideGroup;
    fm_customTreeIterator   iter;
    fm_intRouteEntry *      routeKey;
    fm_intRouteEntry *      route;
    fm_bool                 added;
    fm_int                  hopIndex;
    fm_status               localErr;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, groupId = %d, numNextHops = %d, nextHopList = %p\n",
                  sw,
                  groupId,
                  numNextHops,
                  (void *) nextHopList );

    switchPtr = GET_SWITCH_PTR(sw);

    FM_LOG_ASSERT(FM_LOG_CAT_ROUTING,
                  numNextHops > 0,
                  "numNextHops is <= 0, numNextHops = %d\n",
                  numNextHops);

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }
    
    added = FALSE;

    /* gain exclusive access to routing tables */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);
    }

    addedNextHops = 0;

    group = switchPtr->ecmpGroups[groupId];

    if (group == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    /* This function may not be used with fixed-size ECMP Groups */
    if (group->fixedSize)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    if (group->mcastGroup != NULL)
    {
        status = FM_ERR_USE_MCAST_FUNCTIONS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    newNextHopCount = group->nextHopCount + numNextHops;

    if (newNextHopCount > switchPtr->maxEcmpGroupSize)
    {
        status = FM_ERR_ECMP_GROUP_IS_FULL;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    if (group->isGroupWidthKnown)
    {
        wideGroup = group->wideGroup;
    }
    else
    {
        switch (nextHopList[0].type)
        {
            case FM_NEXTHOP_TYPE_ARP:
            case FM_NEXTHOP_TYPE_RAW_NARROW:
            case FM_NEXTHOP_TYPE_TUNNEL:
            case FM_NEXTHOP_TYPE_VN_TUNNEL:
            case FM_NEXTHOP_TYPE_LOGICAL_PORT:
                wideGroup = FALSE;
                break;
    
            case FM_NEXTHOP_TYPE_RAW_WIDE:
            case FM_NEXTHOP_TYPE_MPLS_ARP:
                wideGroup = TRUE;
                break;
    
            default:
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        }

        /* Set the group width, now that it is known. */
        group->isGroupWidthKnown = TRUE;
        group->wideGroup         = wideGroup;
    }

    /* Verify that all next-hops are the same width and that the next-hop
     * does not already exist in the ECMP group. */
    for (index = 0 ; index < numNextHops ; index++)
    {
        nextHop = &nextHopList[index];

        /* Check the width */
        switch (nextHop->type)
        {
            case FM_NEXTHOP_TYPE_ARP:
            case FM_NEXTHOP_TYPE_RAW_NARROW:
            case FM_NEXTHOP_TYPE_DROP:
            case FM_NEXTHOP_TYPE_TUNNEL:
            case FM_NEXTHOP_TYPE_VN_TUNNEL:
            case FM_NEXTHOP_TYPE_LOGICAL_PORT:
                if (wideGroup)
                {
                    status = FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                }
                break;

            case FM_NEXTHOP_TYPE_RAW_WIDE:
            case FM_NEXTHOP_TYPE_MPLS_ARP:
                if (!wideGroup)
                {
                    status = FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                }
                break;

            default:
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                break;
        }

        if (group->mplsGroup)
        {
            switch (nextHop->type)
            {
                case FM_NEXTHOP_TYPE_MPLS_ARP:
                    break;

                default:
                    status = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                    break;
            }
        }

        /* Make sure the next-hop is not already in the ECMP group */
        if ( fmFindNextHopInternal(sw, group, nextHop, NULL) != NULL )
        {
            status = FM_ERR_ALREADY_EXISTS;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        }
    }

    for (index = 0 ; index < numNextHops ; index++)
    {
        /* Point at the new record */
        nextHop = &nextHopList[index];

        /* Allocate a new internal next-hop record */
        intNextHop = fmAlloc( sizeof(fm_intNextHop) );

        if (intNextHop == NULL)
        {
            status = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        }

        /* Clear the next hop structure */
        FM_CLEAR(*intNextHop);

        /* Save the nexthop pointer into the next hop table */
        group->nextHops[group->nextHopCount] = intNextHop;
        group->nextHopCount++;
        addedNextHops++;

        /* Initialize the structure */
        status = fmInitializeNextHop(sw,
                                     group,
                                     intNextHop,
                                     nextHop);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    status = ValidateEcmpGroup(sw, group, NULL);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->AddECMPGroupNextHops,
                       sw,
                       group,
                       numNextHops,
                       nextHopList);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    added = TRUE;

    fmCustomTreeIterInit(&iter, &group->routeTree);

    while (1)
    {
        status = fmCustomTreeIterNext( &iter,
                                       (void **) &routeKey,
                                       (void **) &route );
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        status = fmNotifyVNTunnelAboutEcmpChange(sw, route);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }


    ABORT:

    if (status != FM_OK)
    {
        if (addedNextHops > 0)
        {
            if (added)
            {
                /* attempt to remove the next-hops from the group */
                fmDeleteECMPGroupNextHopsInternal(sw,
                                                  groupId,
                                                  addedNextHops,
                                                  nextHopList);
            }
            else
            {
                for (index = 0 ; index < numNextHops ; index++)
                {
                    /* Point at the new record */
                    nextHop = &nextHopList[index];
                    
                    intNextHop = fmFindNextHopInternal(sw, group, nextHop, &hopIndex);
                    if (intNextHop == NULL)
                    {
                        /* Ignore next-hops that can't be found.  This allows the
                         * application to easily recover from earlier errors that left the
                         * system in an unknown state.  The application can simply attempt
                         * to remove all next-hops that MIGHT be in the group.
                         */
                        continue;
                    }
                    
                    group->nextHops[hopIndex] = NULL;
                    
                    for (hopIndex = hopIndex + 1 ;
                         hopIndex < group->nextHopCount ;
                         hopIndex++)
                    {
                        group->nextHops[hopIndex - 1] = group->nextHops[hopIndex];
                        group->nextHops[hopIndex]     = NULL;
                    }
                    
                    group->nextHopCount--;
                    
                    localErr = fmDeleteArpNextHopFromTrees(sw, intNextHop);
                    if (localErr != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                     "Delete arp error %d %s\n",
                                     localErr,
                                     fmErrorMsg(localErr) );
                    }
                    
                    if (intNextHop->extension)
                    {
                        fmFree(intNextHop->extension);
                    }
                    
                    fmFree(intNextHop);
                    
                } /* end  (index = 0 ; index < numNextHops ; index++) */
            }
        }
        else if ( (group != NULL) && (group->nextHopCount == 0) )
        {
            group->isGroupWidthKnown = FALSE;
            group->wideGroup         = FALSE;
        }
    }

    fmReleaseWriteLock(&switchPtr->routingLock);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end fmAddECMPGroupNextHopsInternal */




/*****************************************************************************/
/** fmAddECMPGroupRawNextHop
 * \ingroup intNextHopEcmp
 *
 * \chips           FM6000
 *
 * \desc            Adds a raw next-hop from an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 * 
 * \param[in]       nextHopType is the type of raw next hop entry, see
 *                  ''fm_rawNextHopType'' for more details
 *                  
 * \param[in]       value0 is the first 64-bit value in the raw next-hop.
 *
 * \param[in]       value1 is the second 64-bit value in the raw next-hop (for
 *                  raw wide next hop entry only).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 *
 *****************************************************************************/
fm_status fmAddECMPGroupRawNextHop(fm_int               sw,
                                   fm_int               groupId,
                                   fm_ecmpNextHopType   nextHopType,
                                   fm_uint64            value0,
                                   fm_uint64            value1)
{
    fm_status      status;
    fm_ecmpNextHop nextHop;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, nextHopType = %d, "
                     "value0 = %" FM_FORMAT_64 "X, "
                     "value1 = %" FM_FORMAT_64 "X\n",
                     sw,
                     groupId,
                     nextHopType,
                     value0,
                     value1);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_CLEAR( nextHop );

    nextHop.type = nextHopType;
    
    switch (nextHopType)
    {
        case FM_NEXTHOP_TYPE_RAW_NARROW:
            nextHop.data.rawNarrow.value = value0;
            break;

        case FM_NEXTHOP_TYPE_RAW_WIDE:
            nextHop.data.rawWide.values[0] = value0;
            nextHop.data.rawWide.values[1] = value1;
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            break;
    }
    
    status = fmAddECMPGroupNextHopsInternal(sw, groupId, 1, &nextHop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmAddECMPGroupRawNextHop */




/*****************************************************************************/
/** fmDeleteECMPGroupNextHops
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes one or more next-hops from a variable sized ECMP 
 *                  group using an ''fm_nextHop'' ARP-type next-hop 
 *                  specification. 
 *
 * \note            See ''fmDeleteECMPGroupNextHopsV2'' for ''fm_ecmpNextHop'' 
 *                  type next-hop specifications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       numNextHops is the number of next-hops in nextHopList.
 *
 * \param[in]       nextHopList points to an array of next-hops to be deleted
 *                  from the group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid or
 *                  nextHopList is NULL. If an address described in nextHopList
 *                  cannot be found in the ECMP group's next-hop table, that
 *                  next-hop will simply be ignored.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the group is being used by
 *                  one or more routes and the last next-hop in the group
 *                  is being deleted.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is a fixed sized
 *                  group.
 *
 *****************************************************************************/
fm_status fmDeleteECMPGroupNextHops(fm_int      sw,
                                    fm_int      groupId,
                                    fm_int      numNextHops,
                                    fm_nextHop *nextHopList)
{
    fm_status       status;
    fm_switch *     switchPtr;
    fm_ecmpNextHop *ecmpNextHopList;
    fm_int          i;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, numNextHops = %d, "
                     "nextHopList = %p\n",
                     sw,
                     groupId,
                     numNextHops,
                     (void *) nextHopList);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr       = GET_SWITCH_PTR(sw);
    ecmpNextHopList = NULL;

    if ( (numNextHops <= 0) || (numNextHops > switchPtr->maxArpEntries) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    /* Allocate and initialize an array of fm_ecmpNextHop structures,
     * copying from the caller's fm_nextHop structures. */
    i = numNextHops * sizeof(fm_ecmpNextHop);

    ecmpNextHopList = fmAlloc(i);
    if (ecmpNextHopList == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    FM_MEMSET_S( ecmpNextHopList, i, 0, i);

    for (i = 0 ; i < numNextHops ; i++)
    {
        ecmpNextHopList[i].type = FM_NEXTHOP_TYPE_ARP;
        FM_MEMCPY_S( &ecmpNextHopList[i].data.arp,
                     sizeof(ecmpNextHopList[i].data.arp),
                     &nextHopList[i],
                     sizeof(nextHopList[i]) );
    }

    status = fmDeleteECMPGroupNextHopsInternal(sw,
                                               groupId,
                                               numNextHops,
                                               ecmpNextHopList);

ABORT:

    if (ecmpNextHopList != NULL)
    {
        fmFree(ecmpNextHopList);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmDeleteECMPGroupNextHops */




/*****************************************************************************/
/** fmDeleteECMPGroupNextHopsV2
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes one or more next-hops from a variable sized ECMP 
 *                  group using an ''fm_ecmpNextHop'' type next-hop 
 *                  specification. 
 *
 * \note            See ''fmDeleteECMPGroupNextHops'' for ''fm_nextHop'' 
 *                  ARP-type next-hop specifications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       numNextHops is the number of next-hops in nextHopList.
 *
 * \param[in]       nextHopList points to an array of next-hops to be deleted
 *                  from the group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid or
 *                  nextHopList is NULL. If an address described in nextHopList
 *                  cannot be found in the ECMP group's next-hop table, that
 *                  next-hop will simply be ignored.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the group is being used by
 *                  one or more routes and the last next-hop in the group
 *                  is being deleted.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is a fixed sized
 *                  group.
 *
 *****************************************************************************/
fm_status fmDeleteECMPGroupNextHopsV2(fm_int          sw,
                                      fm_int          groupId,
                                      fm_int          numNextHops,
                                      fm_ecmpNextHop *nextHopList)
{
    fm_status status;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, numNextHops = %d, "
                     "nextHopList = %p\n",
                     sw,
                     groupId,
                     numNextHops,
                     (void *) nextHopList);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    status = fmDeleteECMPGroupNextHopsInternal(sw,
                                               groupId,
                                               numNextHops,
                                               nextHopList);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmDeleteECMPGroupNextHopsV2 */




/*****************************************************************************/
/** fmDeleteECMPGroupNextHopsInternal
 * \ingroup intNextHopArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Deletes one or more next hops from an ECMP group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       groupId is the group ID from a prior call to
 *                  fmCreateECMPGroup.
 *
 * \param[in]       numNextHops is the number of next-hops in nextHopList.
 *
 * \param[in]       nextHopList points to an array of next hops to be deleted
 *                  from the group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid or
 *                  nextHopList is NULL.  If an address described in nextHopList
 *                  cannot be found in the ECMP group's nexthop table that
 *                  next-hop will simply be ignored.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the group is being used by
 *                  one or more routes and the last nexthop in the group
 *                  is being deleted.
 *
 *****************************************************************************/
fm_status fmDeleteECMPGroupNextHopsInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_int          numNextHops,
                                            fm_ecmpNextHop *nextHopList)
{
    fm_status             status;
    fm_status             status2;
    fm_switch *           switchPtr;
    fm_intEcmpGroup *     group;
    fm_ecmpNextHop *      nextHop;
    fm_intNextHop *       intNextHop;
    fm_int                hopIndex;
    fm_int                index;
    fm_int                i;
    fm_intNextHop **      removedHops = NULL;
    fm_int                removedIndex = 0;
    fm_customTreeIterator iter;
    fm_intRouteEntry *    routeKey;
    fm_intRouteEntry *    route;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, groupId = %d, numNextHops = %d, "
                  "nextHopList = %p\n",
                  sw,
                  groupId,
                  numNextHops,
                  (void *) nextHopList );

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* gain exclusive access to routing tables */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);
    }

    group = switchPtr->ecmpGroups[groupId];

    if (group == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    if (group->fixedSize)
        numNextHops = group->nextHopCount;

    i = sizeof(fm_intNextHop *) * numNextHops;

    removedHops = fmAlloc(i);

    if (removedHops == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    FM_MEMSET_S(removedHops, i, 0, i);

    for (index = 0 ; index < numNextHops ; index++)
    {
        if (group->fixedSize)
        {
            intNextHop = group->nextHops[index];
            group->nextHops[index] = NULL;
        }
        else
        {
            nextHop = &nextHopList[index];

            intNextHop = fmFindNextHopInternal(sw, group, nextHop, &hopIndex);
            if (intNextHop == NULL)
            {
                /* Ignore next-hops that can't be found.  This allows the
                 * application to easily recover from earlier errors that left the
                 * system in an unknown state.  The application can simply attempt
                 * to remove all next-hops that MIGHT be in the group.
                 */
                continue;
            }

            group->nextHops[hopIndex] = NULL;

            for (hopIndex = hopIndex + 1 ;
                 hopIndex < group->nextHopCount ;
                 hopIndex++)
            {
                 group->nextHops[hopIndex - 1] = group->nextHops[hopIndex];
                 group->nextHops[hopIndex]     = NULL;
            }

        }
        removedHops[removedIndex] = intNextHop;
        removedIndex++;
        group->nextHopCount--;
    
        status = fmDeleteArpNextHopFromTrees(sw, intNextHop);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
       
    }   /* end for (index = 0 ; index < numNextHops ; index++) */

    if (group->nextHopCount == 0)
    {
        group->isGroupWidthKnown = FALSE;
        group->wideGroup         = FALSE;
    }

    fmCustomTreeIterInit(&iter, &group->routeTree);

    while (1)
    {
        status = fmCustomTreeIterNext( &iter,
                                       (void **) &routeKey,
                                       (void **) &route );
        if (status == FM_ERR_NO_MORE)
        {
            status = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        status = fmNotifyVNTunnelAboutEcmpChange(sw, route);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }


ABORT:

    if (removedHops != NULL)
    {
        ValidateEcmpGroup(sw, group, NULL);

        FM_API_CALL_FAMILY(status2,
                           switchPtr->DeleteECMPGroupNextHops,
                           sw,
                           group,
                           removedIndex,
                           removedHops,
                           numNextHops,
                           nextHopList);

        for (i = 0 ; i < removedIndex ; i++)
        {
            if (removedHops[i] != NULL)
            {
                fmFree(removedHops[i]);
            }
        }

        fmFree(removedHops);

        if (status == FM_OK)
        {
            status = status2;
        }
    }

    fmReleaseWriteLock(&switchPtr->routingLock);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end fmDeleteECMPGroupNextHopInternal */




/*****************************************************************************/
/** fmDeleteECMPGroupRawNextHop
 * \ingroup intNextHopEcmp
 *
 * \chips           FM6000
 *
 * \desc            Deletes a raw next-hop from an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 * 
 * \param[in]       nextHopType is the type of raw next hop entry, see
 *                  ''fm_rawNextHopType'' for more details  
 *
 * \param[in]       value0 is the first 64-bit value in the raw next-hop.
 *
 * \param[in]       value1 is the second 64-bit value in the raw next-hop (for
 *                  raw wide next hop only). 
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid or
 *                  nextHopList is NULL. If an address described in nextHopList
 *                  cannot be found in the ECMP group's wide next-hop table,
 *                  that next-hop will simply be ignored.
 * \return          FM_ERR_ECMP_GROUP_IN_USE if the group is being used by
 *                  one or more routes and the last next-hop in the group
 *                  is being deleted.
 *
 *****************************************************************************/
fm_status fmDeleteECMPGroupRawNextHop(fm_int                sw,
                                      fm_int                groupId,
                                      fm_ecmpNextHopType    nextHopType,
                                      fm_uint64             value0,
                                      fm_uint64             value1)
{
    fm_status      status;
    fm_ecmpNextHop nextHop;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, nextHopType = %d, "
                     "value0 = %" FM_FORMAT_64 "X, "
                     "value1 = %" FM_FORMAT_64 "X\n",
                     sw,
                     groupId,
                     nextHopType,
                     value0,
                     value1);

    FM_CLEAR( nextHop );

    nextHop.type = nextHopType;
    
    switch (nextHopType)
    {
        case FM_NEXTHOP_TYPE_RAW_NARROW:
            nextHop.data.rawNarrow.value = value0;
            break;

        case FM_NEXTHOP_TYPE_RAW_WIDE:
            nextHop.data.rawWide.values[0] = value0;
            nextHop.data.rawWide.values[1] = value1;
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            break;
    }

    status = fmDeleteECMPGroupNextHopsInternal(sw, groupId, 1, &nextHop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

ABORT:

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmDeleteECMPGroupRawNextHop */




/*****************************************************************************/
/** fmReplaceECMPGroupNextHopInternal
 * \ingroup intNextHopEcmp
 *
 * \desc            Replaces a next-hop in an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       oldNextHop points to the next-hop that is to be replaced.
 *
 * \param[in]       newNextHop points to the next-hop which is to replace
 *                  oldNextHop.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_NO_MEM if memory for the new next-hop cannot be
 *                  allocated.
 * \return          FM_ERR_NOT_FOUND if the old next-hop was not found in the
 *                  ECMP group.
 * \return          FM_ERR_ALREADY_EXISTS if the new next-hop already exists
 *                  in the ECMP group.
 *
 *****************************************************************************/
fm_status fmReplaceECMPGroupNextHopInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_ecmpNextHop *oldNextHop,
                                            fm_ecmpNextHop *newNextHop)
{
    fm_status               status;
    fm_switch *             switchPtr;
    fm_intEcmpGroup *       group;
    fm_int                  hopIndex;
    fm_intNextHop *         oldIntNextHop;
    fm_intNextHop *         newIntNextHop;
    fm_bool                 wideHop;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, groupId = %d, oldNextHop = %p, newNextHop = %p\n",
                 sw,
                 groupId,
                 (void *) oldNextHop,
                 (void *) newNextHop);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    group = switchPtr->ecmpGroups[groupId];

    if (group == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    oldIntNextHop = fmFindNextHopInternal(sw, group, oldNextHop, &hopIndex);

    if (oldIntNextHop == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_NOT_FOUND);
    }

    newIntNextHop = fmFindNextHopInternal(sw, group, newNextHop, NULL);

    if (newIntNextHop != NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_ALREADY_EXISTS);
    }

    switch (oldNextHop->type)
    {
        case FM_NEXTHOP_TYPE_ARP:
        case FM_NEXTHOP_TYPE_DROP:
        case FM_NEXTHOP_TYPE_RAW_NARROW:
        case FM_NEXTHOP_TYPE_TUNNEL:
        case FM_NEXTHOP_TYPE_VN_TUNNEL:
        case FM_NEXTHOP_TYPE_LOGICAL_PORT:
            if (group->wideGroup)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS);
            }
            wideHop = FALSE;
            break;

        case FM_NEXTHOP_TYPE_RAW_WIDE:
        case FM_NEXTHOP_TYPE_MPLS_ARP:
            if (!group->wideGroup)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS);
            }
            wideHop = TRUE;
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
            break;
    }

    switch (newNextHop->type)
    {
        case FM_NEXTHOP_TYPE_ARP:
        case FM_NEXTHOP_TYPE_DROP:
        case FM_NEXTHOP_TYPE_RAW_NARROW:
        case FM_NEXTHOP_TYPE_TUNNEL:
        case FM_NEXTHOP_TYPE_VN_TUNNEL:
        case FM_NEXTHOP_TYPE_LOGICAL_PORT:
            if (wideHop)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS);
            }
            break;

        case FM_NEXTHOP_TYPE_RAW_WIDE:
        case FM_NEXTHOP_TYPE_MPLS_ARP:
            if (!wideHop)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS);
            }
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
            break;
    }

    if ( (newNextHop->type == FM_NEXTHOP_TYPE_MPLS_ARP) && !group->mplsGroup )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* Delete the old next hop from the ARP and interface trees */
    status = fmDeleteArpNextHopFromTrees(sw, oldIntNextHop);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    newIntNextHop = fmAlloc( sizeof(fm_intNextHop) );
    if (newIntNextHop == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_NO_MEM);
    }

    /* initialize the next hop structure */
    FM_CLEAR(*newIntNextHop);

    status = fmInitializeNextHop(sw, group, newIntNextHop, newNextHop);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    /* Store the new nexthop pointer in the next hop table */
    group->nextHops[hopIndex] = newIntNextHop;

    status = ValidateEcmpGroup(sw, group, NULL);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    FM_API_CALL_FAMILY(status,
                       switchPtr->ReplaceECMPGroupNextHop,
                       sw,
                       group,
                       oldIntNextHop,
                       newIntNextHop);

    if (status == FM_OK)
    {
        fmFree(oldIntNextHop);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end fmReplaceECMPGroupNextHopInternal */




/*****************************************************************************/
/** fmReplaceECMPGroupNextHop
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Replaces a next-hop in a variable sized ECMP group with
 *                  a new next-hop in a manner that does not affect the
 *                  hashing of the ECMP group using an ''fm_nextHop'' ARP-type 
 *                  next-hop specification. 
 *
 * \note            See ''fmReplaceECMPGroupNextHopV2'' for ''fm_ecmpNextHop'' 
 *                  type next-hop specifications. See ''fmSetECMPGroupNextHops'' 
 *                  for fixed-sized ECMP groups.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       oldNextHop points to the next-hop that is to be replaced.
 *
 * \param[in]       newNextHop points to the next-hop that is to replace
 *                  oldNextHop.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_NO_MEM if memory for the new next-hop cannot be
 *                  allocated.
 * \return          FM_ERR_NOT_FOUND if the old next-hop was not found in the
 *                  ECMP group.
 * \return          FM_ERR_ALREADY_EXISTS if the new next-hop already exists
 *                  in the ECMP group.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is not a fixed sized
 *                  group.
 *
 *****************************************************************************/
fm_status fmReplaceECMPGroupNextHop(fm_int      sw,
                                    fm_int      groupId,
                                    fm_nextHop *oldNextHop,
                                    fm_nextHop *newNextHop)
{
    fm_status      status;
    fm_switch *    switchPtr;
    fm_bool        routingLockTaken = FALSE;
    fm_ecmpNextHop oldEcmpNextHop;
    fm_ecmpNextHop newEcmpNextHop;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, groupId = %d, oldNextHop = %p, "
                      "newNextHop = %p\n",
                      sw,
                      groupId,
                      (void *) oldNextHop,
                      (void *) newNextHop );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_CLEAR( oldEcmpNextHop );
    oldEcmpNextHop.type = FM_NEXTHOP_TYPE_ARP;
    FM_MEMCPY_S( &oldEcmpNextHop.data.arp,
                 sizeof(oldEcmpNextHop.data.arp),
                 oldNextHop,
                 sizeof(*oldNextHop) );

    FM_CLEAR( newEcmpNextHop );
    newEcmpNextHop.type = FM_NEXTHOP_TYPE_ARP;
    FM_MEMCPY_S( &newEcmpNextHop.data.arp,
                 sizeof(newEcmpNextHop.data.arp),
                 newNextHop,
                 sizeof(*newNextHop) );

    /* gain exclusive access to routing tables */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    routingLockTaken = TRUE;

    status = fmReplaceECMPGroupNextHopInternal(sw,
                                               groupId,
                                               &oldEcmpNextHop,
                                               &newEcmpNextHop);


ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmReplaceECMPGroupNextHop */




/*****************************************************************************/
/** fmReplaceECMPGroupNextHopV2
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Replaces a next-hop in a variable sized ECMP group with
 *                  a new next-hop in a manner that does not affect the
 *                  hashing of the ECMP group using an ''fm_ecmpNextHop'' type 
 *                  next-hop specification. 
 *
 * \note            See ''fmReplaceECMPGroupNextHop'' for ''fm_nextHop'' 
 *                  type next-hop specifications. See ''fmSetECMPGroupNextHops'' 
 *                  for fixed-sized ECMP groups.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       oldNextHop points to the next-hop that is to be replaced.
 *
 * \param[in]       newNextHop points to the next-hop that is to replace
 *                  oldNextHop.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 * \return          FM_ERR_NO_MEM if memory for the new next-hop cannot be
 *                  allocated.
 * \return          FM_ERR_NOT_FOUND if the old next-hop was not found in the
 *                  ECMP group.
 * \return          FM_ERR_ALREADY_EXISTS if the new next-hop already exists
 *                  in the ECMP group.
 * \return          FM_ERR_UNSUPPORTED if the ECMP group is not a fixed sized
 *                  group.
 *
 *****************************************************************************/
fm_status fmReplaceECMPGroupNextHopV2(fm_int          sw,
                                      fm_int          groupId,
                                      fm_ecmpNextHop *oldNextHop,
                                      fm_ecmpNextHop *newNextHop)
{
    fm_status  status;
    fm_switch *switchPtr;
    fm_bool    routingLockTaken = FALSE;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, groupId = %d, oldNextHop = %p, "
                      "newNextHop = %p\n",
                      sw,
                      groupId,
                      (void *) oldNextHop,
                      (void *) newNextHop );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* gain exclusive access to routing tables */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    routingLockTaken = TRUE;

    status = fmReplaceECMPGroupNextHopInternal(sw,
                                               groupId,
                                               oldNextHop,
                                               newNextHop);


ABORT:

    if (routingLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmReplaceECMPGroupNextHopV2 */




/*****************************************************************************/
/** fmSetECMPGroupNextHopsInternal
 * \ingroup intNextHopEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Stores one or more next-hops for a fixed-size ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       firstIndex is the index of the first next-hop in the
 *                  ECMP group which is to be updated.
 *
 * \param[in]       numNextHops contains the number of next-hops to be updated.
 *
 * \param[in]       nextHopList points to an array of next hops to be updated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if a function parameter is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch,
 *                  if the ECMP group is not a fixed-size ECMP group, or if
 *                  this feature is not supported on the switch.
 *
 *****************************************************************************/
fm_status fmSetECMPGroupNextHopsInternal(fm_int          sw,
                                         fm_int          groupId,
                                         fm_int          firstIndex,
                                         fm_int          numNextHops,
                                         fm_ecmpNextHop *nextHopList)
{
    fm_status          status;
    fm_switch *        switchPtr;
    fm_intEcmpGroup *  group;
    fm_bool            lockTaken;
    fm_bool            wideGroup;
    fm_int             i;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, groupId = %d, firstIndex = %d, "
                  "numNextHops = %d, nextHopList = %p\n",
                  sw,
                  groupId,
                  firstIndex,
                  numNextHops,
                  (void *) nextHopList );

    switchPtr = GET_SWITCH_PTR(sw);
    lockTaken = FALSE;

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* gain exclusive access to routing tables */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    lockTaken = TRUE;
    group     = switchPtr->ecmpGroups[groupId];

    if (!group->fixedSize)
    {
        status = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    if (nextHopList == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    if ( (firstIndex < 0) || (firstIndex >= group->maxNextHops) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    if ( group->maxNextHops < (firstIndex + numNextHops) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    wideGroup = group->wideGroup;

    /* Verify that all next-hops are the same width. */
    for (i = 0 ; i < numNextHops ; i++)
    {
        switch (nextHopList[i].type)
        {
            case FM_NEXTHOP_TYPE_ARP:
            case FM_NEXTHOP_TYPE_RAW_NARROW:
            case FM_NEXTHOP_TYPE_DROP:
            case FM_NEXTHOP_TYPE_DMAC:
            case FM_NEXTHOP_TYPE_TUNNEL:
            case FM_NEXTHOP_TYPE_VN_TUNNEL:
            case FM_NEXTHOP_TYPE_LOGICAL_PORT:
                if (wideGroup)
                {
                    status = FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                }
                break;

            case FM_NEXTHOP_TYPE_RAW_WIDE:
                if (!wideGroup)
                {
                    status = FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                }
                break;

            case FM_NEXTHOP_TYPE_MPLS_ARP:
                if (!wideGroup)
                {
                    status = FM_ERR_MIXING_NARROW_AND_WIDE_NEXTHOPS;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                }

                if (!group->mplsGroup)
                {
                    status = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                }
                break;

            default:
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
                break;
        }
    }

    /* Change the next-hop information in the hardware. */
    FM_API_CALL_FAMILY(status,
                       switchPtr->SetECMPGroupNextHops,
                       sw,
                       group,
                       firstIndex,
                       numNextHops,
                       nextHopList);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end fmSetECMPGroupNextHopsInternal */




/*****************************************************************************/
/** fmSetECMPGroupNextHops
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Stores one or more next-hops for a fixed-size ECMP group.
 * \desc            Replaces a next-hop in a fixed-sized ECMP group with
 *                  a new next-hop in a manner that does not affect the
 *                  hashing of the ECMP group using an ''fm_ecmpNextHop'' type 
 *                  next-hop specification. 
 *
 * \note            For variable sized ECMP groups, see 
 *                  ''fmReplaceECMPGroupNextHop'' for ''fm_nextHop'' 
 *                  type next-hop specifications and 
 *                  ''fmReplaceECMPGroupNextHopV2'' ''fm_ecmpNextHop'' type 
 *                  next-hop specifications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       firstIndex is the index of the first next-hop in the
 *                  ECMP group which is to be updated.
 *
 * \param[in]       numNextHops contains the number of next-hops to be updated.
 *
 * \param[in]       nextHopList points to an array of next hops to be updated.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if a function parameter is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch,
 *                  if the ECMP group is not a fixed-size ECMP group, or if
 *                  this feature is not supported on the switch.
 *
 *****************************************************************************/
fm_status fmSetECMPGroupNextHops(fm_int          sw,
                                 fm_int          groupId,
                                 fm_int          firstIndex,
                                 fm_int          numNextHops,
                                 fm_ecmpNextHop *nextHopList)
{
    fm_status status;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, groupId = %d, firstIndex = %d, "
                      "numNextHops = %d, nextHopList = %p\n",
                      sw,
                      groupId,
                      firstIndex,
                      numNextHops,
                      (void *) nextHopList );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    status = fmSetECMPGroupNextHopsInternal(sw,
                                            groupId,
                                            firstIndex,
                                            numNextHops,
                                            nextHopList);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmSetECMPGroupNextHops */




/*****************************************************************************/
/** fmSetECMPGroupRawNextHop
 * \ingroup intNextHopEcmp
 *
 * \chips           FM6000
 *
 * \desc            Stores a raw next-hop into an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       index is the index of the next-hop in the ECMP group which
 *                  is to be updated.
 * 
 * \param[in]       nextHopType is the type of raw next hop entry, see
 *                  ''fm_rawNextHopType'' for more details
 *                  
 * \param[in]       value0 is the first 64-bit value in the raw next-hop.
 *
 * \param[in]       value1 is the second 64-bit value in the raw next-hop (for
 *                  raw wide next hop entry only).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is invalid.
 *
 *****************************************************************************/
fm_status fmSetECMPGroupRawNextHop(fm_int               sw,
                                   fm_int               groupId,
                                   fm_int               index,
                                   fm_ecmpNextHopType   nextHopType,
                                   fm_uint64            value0,
                                   fm_uint64            value1)
{
    fm_status      status;
    fm_ecmpNextHop nextHop;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, index = %d, nextHopType = %d, "
                     "value0 = %" FM_FORMAT_64 "X, "
                     "value1 = %" FM_FORMAT_64 "X\n",
                     sw,
                     groupId,
                     index,
                     nextHopType,
                     value0,
                     value1);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_CLEAR( nextHop );

    nextHop.type = nextHopType;
    
    switch (nextHopType)
    {
        case FM_NEXTHOP_TYPE_RAW_NARROW:
            nextHop.data.rawNarrow.value = value0;
            break;

        case FM_NEXTHOP_TYPE_RAW_WIDE:
            nextHop.data.rawWide.values[0] = value0;
            nextHop.data.rawWide.values[1] = value1;
            break;

        default:
            status = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            break;
    }
    
    status = fmSetECMPGroupNextHopsInternal(sw, groupId, index, 1, &nextHop);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmSetECMPGroupRawNextHop */




/*****************************************************************************/
/** fmGetECMPGroupFirst
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Gets the first ECMP group ID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstGroupId points to caller-allocated storage into which
 *                  the first ECMP Group ID will be placed, or -1 if there
 *                  are no ECMP groups.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NO_MORE if there are no ECMP Groups.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupFirst(fm_int sw, fm_int *firstGroupId)
{
    fm_status        status;
    fm_switch *      switchPtr;
    fm_int           groupId;
    fm_intEcmpGroup *group;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    *firstGroupId = -1;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxArpEntries > 0)
    {
        status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (status == FM_OK)
        {
            for (groupId = 0 ; groupId < switchPtr->maxArpEntries ; groupId++)
            {
                group = switchPtr->ecmpGroups[groupId];

                if (group != NULL)
                {
                    break;
                }
            }

            if (groupId < switchPtr->maxArpEntries)
            {
                *firstGroupId = groupId;
                status        = FM_OK;
            }
            else
            {
                status = FM_ERR_NO_MORE;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupFirst */




/*****************************************************************************/
/** fmGetECMPGroupNext
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Gets the next ECMP group ID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       prevGroupId contains the group ID returned by a prior
 *                  call to ''fmGetECMPGroupFirst'' or ''fmGetECMPGroupNext''.
 *
 * \param[out]      nextGroupId points to caller-allocated storage into which
 *                  the next ECMP Group ID will be placed, or -1 if there are
 *                  no more ECMP groups.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no more ECMP Groups.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNext(fm_int  sw,
                             fm_int  prevGroupId,
                             fm_int *nextGroupId)
{
    fm_status        status;
    fm_switch *      switchPtr;
    fm_int           groupId;
    fm_intEcmpGroup *group;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    *nextGroupId = -1;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status == FM_OK)
    {
        for (groupId = prevGroupId + 1 ;
             groupId < switchPtr->maxArpEntries ;
             groupId++)
        {
            group = switchPtr->ecmpGroups[groupId];

            if (group != NULL)
            {
                break;
            }
        }

        if (groupId < switchPtr->maxArpEntries)
        {
            *nextGroupId = groupId;
            status       = FM_OK;
        }
        else
        {
            status = FM_ERR_NO_MORE;
        }

        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupNext */




/*****************************************************************************/
/** fmGetECMPGroupList
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns a list of ECMP Groups.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numGroups points to caller-allocated storage into which
 *                  will be stored the number of ECMP Groups put into groupList.
 *
 * \param[out]      groupList is an array, max elements in length, that this 
 *                  function will fill with the list of ECMP group IDs.
 *
 * \param[in]       max is the size of groupList, being the maximum number of 
 *                  ECMP group IDs that groupList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if max is <= 0 or either of the
 *                  pointer arguments is NULL.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NO_MORE if there are no ECMP Groups.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of ECMP Groups.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupList(fm_int  sw,
                             fm_int *numGroups,
                             fm_int *groupList,
                             fm_int  max)
{
    fm_status        status;
    fm_switch *      switchPtr;
    fm_int           groupId;
    fm_intEcmpGroup *group;
    fm_int           index;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    if (max <= 0)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (numGroups == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (groupList == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxArpEntries > 0)
    {
        status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (status == FM_OK)
        {
            index = 0;

            for (groupId = 0 ; groupId < switchPtr->maxArpEntries ; groupId++)
            {
                group = switchPtr->ecmpGroups[groupId];

                if (group != NULL)
                {
                    if (index >= max)
                    {
                        status = FM_ERR_BUFFER_FULL;
                        break;
                    }

                    groupList[index] = groupId;
                    index++;
                }
            }

            fmReleaseReadLock(&switchPtr->routingLock);

            if (index == 0)
            {
                status = FM_ERR_NO_MORE;
            }

            *numGroups = index;
        }
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupList */




/*****************************************************************************/
/** fmGetECMPGroupNextHopFirst
 * \ingroup routerEcmp
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Gets the first next-hop for an ECMP group using an
 *                  ''fm_nextHop'' ARP-type next-hop specification. 
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[out]      searchToken points to caller-allocated storage into which
 *                  the function will place a search token used for future
 *                  calls to fmGetECMPGroupNextHopNext.
 *
 * \param[out]      firstNextHop points to caller-allocated storage into which
 *                  the first next-hop address will be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is not a valid group
 *                  or either firstNextHop or searchToken are NULL.
 * \return          FM_ERR_NO_MORE if there are no next-hops in the ECMP group.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopFirst(fm_int      sw,
                                     fm_int      groupId,
                                     fm_int *    searchToken,
                                     fm_nextHop *firstNextHop)
{
    fm_status              status;
    fm_switch *            switchPtr;
    fm_intEcmpGroup *      group;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, searchToken = %p, "
                     "firstNextHop = %p\n",
                     sw,
                     groupId,
                     (void *) searchToken,
                     (void *) firstNextHop);

    if (searchToken == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (firstNextHop == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (status == FM_OK)
        {
            group = switchPtr->ecmpGroups[groupId];

            if (group == NULL)
            {
                status = FM_ERR_INVALID_ARGUMENT;
            }
            else if (group->nextHopCount <= 0)
            {
                status = FM_ERR_NO_MORE;
            }
            else
            {
                FM_MEMCPY_S( firstNextHop,
                             sizeof(*firstNextHop),
                             &group->nextHops[0]->nextHop.data.arp,
                             sizeof(fm_nextHop) );
                *searchToken = 0;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupNextHopFirst */




/*****************************************************************************/
/** fmGetECMPGroupNextHopNext
 * \ingroup routerEcmp
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Gets the next next-hop address for an ECMP group using an
 *                  ''fm_nextHop'' ARP-type next-hop specification.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[out]      searchToken points to caller-allocated storage containing
 *                  a search token provided by an earlier call to
 *                  ''fmGetECMPGroupNextHopFirst'' or 
 *                  ''fmGetECMPGroupNextHopNext''.
 *
 * \param[out]      nextHop points to caller-allocated storage into which
 *                  the next next-hop address will be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is not a valid group.
 * \return          FM_ERR_NO_MORE if there are no more next-hops in the ECMP 
 *                  group.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopNext(fm_int      sw,
                                    fm_int      groupId,
                                    fm_int *    searchToken,
                                    fm_nextHop *nextHop)
{
    fm_status              status;
    fm_switch *            switchPtr;
    fm_intEcmpGroup *      group;
    fm_int                 index;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, searchToken = %p, nextHop = %p\n",
                     sw,
                     groupId,
                     (void *) searchToken,
                     (void *) nextHop);

    if (searchToken == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (nextHop == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (status == FM_OK)
        {
            index = *searchToken + 1;
            group = switchPtr->ecmpGroups[groupId];

            if (group == NULL)
            {
                status = FM_ERR_INVALID_ARGUMENT;
            }
            else if (index < group->nextHopCount)
            {
                FM_MEMCPY_S( nextHop,
                             sizeof(*nextHop),
                             &group->nextHops[index]->nextHop.data.arp,
                             sizeof(fm_nextHop) );
                *searchToken = index;
            }
            else
            {
                status = FM_ERR_NO_MORE;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupNextHopNext */




/*****************************************************************************/
/** fmGetECMPGroupNextHopListInternal
 * \ingroup intNextHopArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns a list of nexthop addresses attached to an ECMP group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       groupId is the ECMP group id provided by an earlier call
 *                  to fmCreateECMPGroup.
 *
 * \param[out]      pNumNextHops points to caller-allocated storage into which
 *                  will be stored the number of nexthop addresses put into
 *                  nextHopList.
 *
 * \param[out]      pNextHopList points to caller-allocated storage into which
 *                  the list of nexthop addresses will be placed.  The allocated
 *                  storage must have sufficient room for 'maxNextHops' nexthop
 *                  addresses, or memory corruption may result.
 *
 * \param[in]       maxNextHops is the maximum number of nexthop addresses that
 *                  may be placed into 'pNextHopList'.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is not a valid group,
 *                  either numNextHops or nextHopList are NULL,
 *                  or max is not >= 1.
 * \return          FM_ERR_NO_MORE if there are no nexthops in the ECMP group.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of nexthop addresses attached to the
 *                  ECMP group.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopListInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_int *        pNumNextHops,
                                            fm_ecmpNextHop *pNextHopList,
                                            fm_int          maxNextHops)
{
    fm_status        err;
    fm_switch *      switchPtr;
    fm_intEcmpGroup *pGroup;
    fm_ecmpNextHop * pNnextHop;
    fm_int           nextHopIndex;
    fm_int           maxIterations;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, groupId=%d, pNumNextHops=%p, pNextHopList=%p, maxNextHops=%d\n",
                  sw,
                  groupId,
                  (void *) pNumNextHops,
                  (void *) pNextHopList,
                  maxNextHops );


    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);


    if (pNumNextHops == NULL                ||
        pNextHopList == NULL                ||
        groupId < 0                         ||
        groupId >= switchPtr->maxArpEntries ||
        maxNextHops <= 0 )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            pGroup       = switchPtr->ecmpGroups[groupId];
            nextHopIndex = 0;


            if (pGroup == NULL)
            {
                err = FM_ERR_INVALID_ARGUMENT;
            }
            else
            {
                pNnextHop     = pNextHopList;
                maxIterations = (pGroup->nextHopCount < maxNextHops) ? pGroup->nextHopCount : maxNextHops;

                for ( ; nextHopIndex < maxIterations ; nextHopIndex++)
                {
                    if (pGroup->nextHops[nextHopIndex] == NULL)
                    {
                        err = FM_ERR_INVALID_ARGUMENT;
                        break;
                    }
                    else
                    {
                            FM_MEMCPY_S( pNnextHop,
                                         sizeof(*pNnextHop),
                                         &pGroup->nextHops[nextHopIndex]->nextHop,
                                         sizeof(fm_ecmpNextHop) );
                            pNnextHop++;
                    }
                }

                if (nextHopIndex <= 0)
                {
                    err = FM_ERR_NO_MORE;
                }
                else
                {
                    /* indicate the buffer is not big enough if there are additional nexthops */
                    for (; nextHopIndex < pGroup->nextHopCount ; nextHopIndex++)
                    {
                        if (pGroup->nextHops[nextHopIndex] != NULL)
                        {
                            err = FM_ERR_BUFFER_FULL;
                            break;
                        }
                    }
                }
            }

            fmReleaseReadLock(&switchPtr->routingLock);

            *pNumNextHops = nextHopIndex;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetECMPGroupNextHopListInternal */




/*****************************************************************************/
/** fmGetECMPGroupNextHopList
 * \ingroup routerEcmp
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns a list of next-hop addresses in an ECMP group 
 *                  using an ''fm_nextHop'' ARP-type next-hop specification.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[out]      numNextHops points to caller-allocated storage into which
 *                  will be stored the number of next-hop addresses put into
 *                  nextHopList.
 *
 * \param[out]      nextHopList is an array, max elements in length, that this 
 *                  function will fill with the list of next-hops in the ECMP 
 *                  group.
 *
 * \param[in]       max is the size of nextHopList, being the maximum number of 
 *                  next-hops that nextHopList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is not a valid group,
 *                  either numNextHops or nextHopList are NULL, or max is 
 *                  not >= 1.
 * \return          FM_ERR_NO_MORE if there are no next-hops in the ECMP group.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of next-hop addresses in the ECMP group.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopList(fm_int      sw,
                                    fm_int      groupId,
                                    fm_int *    numNextHops,
                                    fm_nextHop *nextHopList,
                                    fm_int      max)
{
    fm_status       status;
    fm_ecmpNextHop *ecmpNextHopList;
    fm_int          i;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, numNexthops = %p, "
                     "nextHopList = %p, max = %d\n",
                     sw,
                     groupId,
                     (void *) numNextHops,
                     (void *) nextHopList,
                     max);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    ecmpNextHopList = fmAlloc( sizeof(fm_ecmpNextHop) * max );

    if (ecmpNextHopList == NULL)
    {
        status = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    status = fmGetECMPGroupNextHopListInternal(sw,
                                               groupId,
                                               numNextHops,
                                               ecmpNextHopList,
                                               max);

    if ( (status != FM_OK) && (status != FM_ERR_BUFFER_FULL) )
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    for (i = 0 ; i < *numNextHops ; i++)
    {
        FM_MEMCPY_S( &nextHopList[i],
                     sizeof(nextHopList[i]),
                     &ecmpNextHopList[i].data.arp,
                     sizeof(fm_nextHop) );
    }


ABORT:

    if (ecmpNextHopList != NULL)
    {
        fmFree(ecmpNextHopList);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupNextHopList */




/*****************************************************************************/
/** fmGetECMPGroupNextHopUsed
 * \ingroup routerEcmp
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the "used" flag for a next-hop using an
 *                  ''fm_nextHop'' ARP-type next-hop specification.
 * 
 * \note            See ''fmGetECMPGroupNextHopUsedV2'' for ''fm_ecmpNextHop'' 
 *                  type next-hop specifications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       nextHop points to the next-hop entry to be accessed.
 *
 * \param[out]      used points to caller-allocated storage where this function
 *                  should place the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NOT_FOUND if the specified ARP entry is not
 *                  recognized.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopUsed(fm_int      sw,
                                    fm_int      groupId,
                                    fm_nextHop *nextHop,
                                    fm_bool *   used,
                                    fm_bool     resetFlag)
{
    fm_status      err;
    fm_ecmpNextHop ecmpNextHop;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, groupId=%d, nextHop=%p, used=%p, resetFlag=%d\n",
                      sw,
                      groupId,
                      (void *) nextHop,
                      (void *) used,
                      resetFlag );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    FM_CLEAR( ecmpNextHop );

    ecmpNextHop.type = FM_NEXTHOP_TYPE_ARP;
    FM_MEMCPY_S( &ecmpNextHop.data.arp,
                 sizeof(ecmpNextHop.data.arp),
                 nextHop,
                 sizeof(*nextHop) );

    err = fmGetECMPGroupNextHopUsedInternal(sw,
                                            groupId,
                                            &ecmpNextHop,
                                            used,
                                            resetFlag);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetECMPGroupNextHopUsed */




/*****************************************************************************/
/** fmGetECMPGroupNextHopUsedV2
 * \ingroup routerEcmp
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the "used" flag for a next-hop using an
 *                  ''fm_ecmpNextHop'' type next-hop specification.
 * 
 * \note            See ''fmGetECMPGroupNextHopUsed'' for ''fm_nextHop'' 
 *                  ARP-type next-hop specifications.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[in]       nextHop points to the next-hop entry to be accessed.
 *
 * \param[out]      used points to caller-allocated storage where this function
 *                  should place the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NOT_FOUND if the specified ARP entry is not
 *                  recognized.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopUsedV2(fm_int          sw,
                                      fm_int          groupId,
                                      fm_ecmpNextHop *nextHop,
                                      fm_bool *       used,
                                      fm_bool         resetFlag)
{
    fm_status err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, groupId=%d, nextHop=%p, used=%p, resetFlag=%d\n",
                      sw,
                      groupId,
                      (void *) nextHop,
                      (void *) used,
                      resetFlag );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmGetECMPGroupNextHopUsedInternal(sw,
                                            groupId,
                                            nextHop,
                                            used,
                                            resetFlag);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetECMPGroupNextHopUsedV2 */




/*****************************************************************************/
/** fmGetECMPGroupNextHopUsedInternal
 * \ingroup intNextHopArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the "used" flag for a next hop.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group id.
 *
 * \param[in]       nextHop points to the next-hop entry to be accessed.
 *
 * \param[out]      used points to caller-allocated storage where this function
 *                  should place the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NOT_FOUND if the specified ARP entry is not
 *                  recognized.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopUsedInternal(fm_int          sw,
                                            fm_int          groupId,
                                            fm_ecmpNextHop *nextHop,
                                            fm_bool *       used,
                                            fm_bool         resetFlag)
{
    fm_switch *      switchPtr;
    fm_status        err;
    fm_intEcmpGroup *group;
    fm_intNextHop *  intNextHop;
    fm_bool          nextHopUsed;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, groupId=%d, nextHop=%p, used=%p, resetFlag=%d\n",
                  sw,
                  groupId,
                  (void *) nextHop,
                  (void *) used,
                  resetFlag );

    switchPtr = GET_SWITCH_PTR(sw);

    *used = FALSE;

    if (switchPtr->maxArpEntries <= 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_UNSUPPORTED);
    }

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    }

    group = switchPtr->ecmpGroups[groupId];

    if (group == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    intNextHop = fmFindNextHopInternal(sw, group, nextHop, NULL);

    if (intNextHop == NULL)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    FM_API_CALL_FAMILY(err,
                       switchPtr->GetNextHopUsed,
                       sw,
                       intNextHop,
                       &nextHopUsed,
                       resetFlag);

    if (err == FM_OK)
    {
        if (nextHopUsed)
        {
            *used = TRUE;
        }
    }

ABORT:

    fmReleaseReadLock(&switchPtr->routingLock);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetECMPGroupNextHopUsedInternal */




/*****************************************************************************/
/** fmGetECMPGroupRouteCount
 * \ingroup routerEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Gets the number of routes attached to an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID from a prior call to
 *                  ''fmCreateECMPGroup''.
 *
 * \param[out]      routeCountPtr points to caller-allocated storage into which
 *                  the function will place the number of routes attached to
 *                  the ECMP group.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if groupId is not a valid group
 *                  or routeCountPtr is NULL.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupRouteCount(fm_int  sw,
                                   fm_int  groupId,
                                   fm_int *routeCountPtr)
{
    fm_status        status;
    fm_switch *      switchPtr;
    fm_intEcmpGroup *group;
    fm_uint          count;
    
    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, groupId = %d, routeCountPtr = %p\n",
                     sw,
                     groupId,
                     (void *) routeCountPtr);

    if (routeCountPtr == NULL)
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (groupId < 0) || (groupId >= switchPtr->maxArpEntries) )
    {
        status = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (status == FM_OK)
        {
            group = switchPtr->ecmpGroups[groupId];

            if (group == NULL)
            {
                status = FM_ERR_INVALID_ARGUMENT;
            }
            else
            {
                count = fmCustomTreeSize(&group->routeTree);

                *routeCountPtr = (fm_int) count;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupRouteCount */




/*****************************************************************************/
/** fmDeleteArpNextHopFromTrees
 * \ingroup intNextHopArp
 *
 * \desc            Removes an ARP next-hop from the interface and ARP
 *                  custom trees.
 *
 * \note            This function assumes that the routing lock has already
 *                  been taken.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       intNextHop points to the internal next-hop record.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmDeleteArpNextHopFromTrees(fm_int sw, fm_intNextHop *intNextHop)
{
    fm_status  status;
    fm_switch *switchPtr;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, intNextHop = %p\n",
                  sw,
                  (void *) intNextHop );

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (intNextHop->nextHop.type != FM_NEXTHOP_TYPE_ARP) &&
        (intNextHop->nextHop.type != FM_NEXTHOP_TYPE_DMAC) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    if (intNextHop->interfaceAddressEntry != NULL)
    {
        status = fmCustomTreeRemove(&intNextHop->interfaceAddressEntry->nextHopTree,
                                    intNextHop,
                                    NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "NextHop %p removed from ifEntry %d next-hop-tree\n",
                      (void *) intNextHop,
                      intNextHop->interfaceAddressEntry->ifEntry->interfaceNum );
    }
    else if ( (intNextHop->nextHop.type == FM_NEXTHOP_TYPE_ARP) &&
              (!fmIsIPAddressEmpty(&intNextHop->nextHop.data.arp.interfaceAddr) ) )
    {
        status = fmCustomTreeRemove(&switchPtr->noInterfaceNextHops,
                                    intNextHop,
                                    NULL);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "NextHop %p removed from no-interface-next-hops tree\n",
                      (void *) intNextHop );
    }

    /* If an ARP entry doesn't exist for this next-hop, try to remove the
     * next-hop record from the noArpNextHops tree.  If it isn't there,
     * this is unexpected, but no reason to abort, so simply ignore the
     * returned status.
     */
    if (intNextHop->arp == NULL)
    {
        fmCustomTreeRemove(&switchPtr->noArpNextHops, intNextHop, NULL);
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "next hop %p removed from noArpNextHops tree\n",
                      (void *) intNextHop );
    }
    else
    {
        /* Remove the next-hop from the ARP record's nexthop list */
        status = fmCustomTreeRemove(&intNextHop->arp->nextHopTree,
                                    intNextHop,
                                    NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "next hop %p removed from arp %p next-hop tree\n",
                      (void *) intNextHop,
                      (void *) &intNextHop->arp );
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fmDeleteArpNextHopFromTrees */




/*****************************************************************************/
/** fmInitializeNextHop
 * \ingroup intNextHopArp
 *
 * \desc            Initializes a next-hop record for use in an ECMP group.
 *
 * \note            This function assumes that the routing lock has already
 *                  been taken.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       group points to the ECMP group.
 *
 * \param[in]       intNextHop points to the user-interface next-hop record to
 *                  be initialized.
 *
 * \param[in]       nextHop optionally points to the application-provided
 *                  fm_ecmpNextHop record. If present, the contents will
 *                  be copied into the intNextHop record. Use NULL if no
 *                  copy is needed.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmInitializeNextHop(fm_int           sw,
                              fm_intEcmpGroup *group,
                              fm_intNextHop *  intNextHop,
                              fm_ecmpNextHop * nextHop)
{
    fm_status                      status;
    fm_switch *                    switchPtr;
    fm_nextHop *                   arpNextHop;
    fm_uint16                      vlan;
    fm_intArpEntry *               arpEntry;
    fm_intIpInterfaceAddressEntry *addrEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, group = %p (%d), intNextHop = %p, nextHop = %p\n",
                  sw,
                  (void *) group,
                  group->groupId,
                  (void *) intNextHop,
                  (void *) nextHop );

    switchPtr = GET_SWITCH_PTR(sw);

    /* If an ECMP next-hop record was provided, copy it into the internal
     * record. */
    if (nextHop != NULL)
    {
        FM_MEMCPY_S( &intNextHop->nextHop,
                     sizeof(intNextHop->nextHop),
                     nextHop,
                     sizeof(*nextHop) );
    }

    /* Now use the internal copy of the next-hop information */
    nextHop = &intNextHop->nextHop;

    if (nextHop->type == FM_NEXTHOP_TYPE_ARP)
    {
        arpNextHop = &nextHop->data.arp;

        FM_API_CALL_FAMILY(status,
                           switchPtr->ValidateNextHopTrapCode,
                           sw,
                           arpNextHop);

        if (status == FM_ERR_UNSUPPORTED)
        {
            status = FM_OK;
        }
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        /* Validate the interface */
        addrEntry = NULL;
        if (!fmIsIPAddressEmpty(&arpNextHop->interfaceAddr) )
        {
            status = FindInterfaceAddrEntry(sw,
                                            &arpNextHop->interfaceAddr,
                                            &addrEntry);

            if ( status == FM_ERR_INVALID_INTERFACE)
            {
                /* next-hop specifies an interface IP address that doesn't
                 * exist.  This is not an error, but will prevent the next-hop
                 * from being used by the hardware until the interface IP address
                 * is added to an interface.
                 */
                addrEntry = NULL;
                status = FM_OK;
            }
        }

        /* Try to get the vlan.  If the interface was specified but the
         * address doesn't exist, the vlan will be set to invalid.  This is
         * not an error, but will prevent the next-hop address from being
         * used by the hardware until the interface IP address is added
         * to an interface.
         */
        vlan = fmGetInterfaceVlan(sw,
                                  &arpNextHop->interfaceAddr,
                                  arpNextHop->vlan);

        /* try to find a matching ARP entry */
        status = fmFindArpEntry(sw,
                                &arpNextHop->addr,
                                vlan,
                                &arpEntry);

        if (status == FM_ERR_NOT_FOUND)
        {
            arpEntry = NULL;
        }
        else if (status != FM_OK)
        {
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        }
    }
    else
    {
        vlan      = 0;
        addrEntry = NULL;
        arpEntry  = NULL;
    }

    intNextHop->sw                    = sw;
    intNextHop->ecmpGroup             = group;
    intNextHop->arp                   = arpEntry;
    intNextHop->vlan                  = vlan;
    intNextHop->oldVlan               = vlan;
    intNextHop->interfaceAddressEntry = addrEntry;
    intNextHop->state                 = FM_NEXT_HOP_STATE_UP;


    if (addrEntry != NULL)
    {
        status = fmCustomTreeInsert(&addrEntry->nextHopTree,
                                    intNextHop,
                                    intNextHop);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "NextHop %p added to ifEntry %d next-hop-tree\n",
                      (void *) intNextHop,
                      addrEntry->ifEntry->interfaceNum );
    }
    else if ( (intNextHop->nextHop.type == FM_NEXTHOP_TYPE_ARP) &&
              (!fmIsIPAddressEmpty(&intNextHop->nextHop.data.arp.interfaceAddr) ) )
    {
        status = fmCustomTreeInsert(&switchPtr->noInterfaceNextHops,
                                    intNextHop,
                                    intNextHop);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "NextHop %p added to no-interface-next-hop-tree\n",
                      (void *) intNextHop );
    }

    if (nextHop->type == FM_NEXTHOP_TYPE_ARP)
    {
        /* If an ARP entry doesn't exist for this next-hop, add the next-hop
         * to the noArpNextHops tree so that it can be quickly found when
         * the ARP is finally added.
         */
        if (arpEntry == NULL)
        {
            status = fmCustomTreeInsert(&switchPtr->noArpNextHops,
                                        intNextHop,
                                        intNextHop);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                          "next hop %p added to noArpNextHops tree\n",
                          (void *) intNextHop );
        }
        else
        {
            /* Add the next-hop to the ARP record's nexthop list */
            status = fmCustomTreeInsert(&arpEntry->nextHopTree,
                                        intNextHop,
                                        intNextHop);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                          "next hop %p added to arp %p next-hop tree\n",
                          (void *) intNextHop,
                          (void *) arpEntry );
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fmInitializeNextHop */




/*****************************************************************************/
/** fmGetECMPGroupNextHopIndexRange
 * \ingroup intNextHopEcmp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            This function returns the hardware next-hop index numbers
 *                  for the first and last next-hop in an ECMP group.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       groupId is the ECMP group ID number.
 *
 * \param[out]      firstIndex points to caller-provided storage into which
 *                  the first hardware record index will be written.
 *                  May be NULL, in which case the first index is not
 *                  returned.
 *
 * \param[out]      lastIndex points to caller-provided storage into which
 *                  the last hardware record index will be written.
 *                  May be NULL, in which case the last index will not be
 *                  returned.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if the groupId is not valid.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support this
 *                  request.
 *
 *****************************************************************************/
fm_status fmGetECMPGroupNextHopIndexRange(fm_int  sw,
                                          fm_int  groupId,
                                          fm_int *firstIndex,
                                          fm_int *lastIndex)
{
    fm_status        status;
    fm_switch *      switchPtr;
    fm_bool          lockTaken;
    fm_intEcmpGroup *group;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, groupId = %d, firstIndex = %p, lastIndex = %p\n",
                      sw,
                      groupId,
                      (void *) firstIndex,
                      (void *) lastIndex );

    lockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* gain read access to routing tables */
    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    lockTaken = TRUE;

    group = switchPtr->ecmpGroups[groupId];

    if (group == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetECMPGroupNextHopIndexRange,
                       sw,
                       group,
                       firstIndex,
                       lastIndex);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetECMPGroupNextHopIndexRange */




/*****************************************************************************/
/** fmGetNextHopIndexUsed
 * \ingroup intNextHopEcmp
 * 
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the "used" flag for a next-hop given the
 *                  index of the next-hop within the hardware next-hop table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       index is the index number in the hardware next-hop table.
 *
 * \param[out]      used points to caller-allocated storage where this function
 *                  should place the result.
 *
 * \param[in]       resetFlag specifies whether the used flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_ARGUMENT if the index value is invalid.
 *
 *****************************************************************************/
fm_status fmGetNextHopIndexUsed(fm_int   sw,
                                fm_int   index,
                                fm_bool *used,
                                fm_bool  resetFlag)
{
    fm_status        status;
    fm_switch *      switchPtr;
    fm_bool          lockTaken;


    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw=%d, index=%d, used=%p, resetFlag=%d\n",
                      sw,
                      index,
                      (void *) used,
                      resetFlag );

    lockTaken = FALSE;

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* gain read access to routing tables */
    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    lockTaken = TRUE;

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetNextHopIndexUsed,
                       sw,
                       index,
                       used,
                       resetFlag);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetNextHopIndexUsed */




/*****************************************************************************/
/** fmAddARPEntry
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Add an ARP table entry in the switch. This function does
 *                  not allow a new entry to be added if an identical entry
 *                  (not including destination MAC address) already exists.
 *                  Use ''fmUpdateARPEntryDMAC'' to change the destination
 *                  MAC address of an existing entry. It is not permitted
 *                  to add an ARP entry to an interface that does not exist.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArp points the ARP entry to add.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_DUPLICATE_ARP_ENTRY if the specified ARP entry
 *                  already exists.
 * \return          FM_ERR_INVALID_INTERFACE if the interface specified in the
 *                  ARP entry does not exist.
 * \return          FM_ERR_ARP_TABLE_FULL if routing is not available on the switch.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmAddARPEntry(fm_int       sw,
                        fm_arpEntry *pArp)
{
    fm_switch *             switchPtr;
    fm_status               err;
    fm_char                 ipText[100];
    fm_intIpInterfaceEntry *pIfEntry;
    fm_int16                newEntryVlan;
    fm_intArpEntry         *pArpEntry;
    fm_intArpEntry         *pNewArpEntry;
    fm_bool                 inserted;
    fm_status               localErr;



    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pArp=%p\n",
                     sw,
                     (void*)pArp);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    pArpEntry = NULL;
    pNewArpEntry = NULL;
    pIfEntry = NULL;
    newEntryVlan = FM_INVALID_VLAN;
    inserted     = FALSE;

    if (pArp == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        fmDbgConvertIPAddressToString(&pArp->ipAddr, ipText);
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "sw=%d, ip=%s, vlan=%d, interface=%d, mac=%012llX\n",
                     sw,
                     ipText,
                     pArp->vlan,
                     pArp->interface,
                     pArp->macAddr);

        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
        if (err == FM_OK)
        {
            /**********************************************************
             * perform some additional validations:
             *  - verify that if the ARP entry refers to an interface, 
             *    this last one MUST exist
             *  - verify that the ARP entry was not already defined
             * *******************************************************/

            if (pArp->interface >= 0)
            {
                err = fmGetInterface(sw, pArp->interface, &pIfEntry);
                if (err == FM_OK && pIfEntry != NULL)
                {
                    newEntryVlan = pIfEntry->vlan;
                    pArp->vlan = pIfEntry->vlan;
                }
            }
            else
            {
                newEntryVlan = pArp->vlan;
            }

            if (err == FM_OK)
            {
                err = FindArpEntryExt(sw, pArp, &pArpEntry);
                
                if (err == FM_OK && pArpEntry != NULL)
                {
                    err = FM_ERR_DUPLICATE_ARP_ENTRY;
                }
                err = (err == FM_ERR_NOT_FOUND) ? FM_OK : err;
            }

            if (err == FM_OK)
            {
                /* alloc a new ARP entry */
                pNewArpEntry = (fm_intArpEntry *) fmAlloc(sizeof(fm_intArpEntry) );
                if (pNewArpEntry == NULL)
                {
                    err = FM_ERR_NO_MEM;
                }
                else
                {
                    /* initialize and insert the new ARP entry */
                    FM_CLEAR(*pNewArpEntry);
                    pNewArpEntry->arp       = *pArp;
                    pNewArpEntry->ifEntry   = pIfEntry;
                    pNewArpEntry->switchPtr = switchPtr;
                    pNewArpEntry->vrid      = -1;
                    pNewArpEntry->arp.vlan  = newEntryVlan;

                    fmCustomTreeInit(&pNewArpEntry->nextHopTree, fmCompareInternalNextHops);
                    FM_DLL_INIT_NODE(pNewArpEntry, nextArp, prevArp);

                    /* new entry goes before the current entry */
                    fmInsertArpBefore(switchPtr, pArpEntry, pNewArpEntry);

                    err = fmTreeInsert(&switchPtr->arpPointersTree,
                                       (fm_uintptr)pNewArpEntry,
                                       pNewArpEntry);

                    if (err == FM_OK)
                    {
                        inserted = TRUE;
                        /* Update all next-hops that have been waiting for this
                         * ARP entry */
                        err = UpdateNextHopArpEntryAddedInt(sw,
                                                            &pArp->ipAddr,
                                                            pNewArpEntry);
                    }

                    if (err != FM_OK)
                    {
                        if (inserted)
                        {
                            localErr =
                                fmTreeRemoveCertain(&switchPtr->arpPointersTree,
                                                    (fm_uintptr)pNewArpEntry,
                                                    NULL);
                            if (localErr != FM_OK)
                            {
                                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                             "sw %d Error %d %s\n",
                                             sw,
                                             localErr,
                                             fmErrorMsg(localErr));
                            }
                        }
                        fmRemoveArp(switchPtr,pNewArpEntry);
                        fmFree(pNewArpEntry);
                        pNewArpEntry = NULL;
                    }
                    else if (switchPtr->AddArpEntry != NULL)
                    {
                        /* If the switch requires extra processing, call the switch-specific
                         * function now. */
                        err = switchPtr->AddArpEntry(sw, pArp);
                    }
                }
            }
            fmReleaseWriteLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmAddARPEntry */




/*****************************************************************************/
/** fmDeleteARPEntry
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Delete an ARP table entry from the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArp points the ARP entry to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if the ARP entry does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmDeleteARPEntry(fm_int       sw,
                           fm_arpEntry *pArp)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_intIpInterfaceEntry *pIfEntry;
    fm_intArpEntry         *pArpEntry;
    fm_char                 ipText[100];

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pArp=%p\n",
                     sw,
                     (void*) pArp);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    pArpEntry = NULL;
    pIfEntry = NULL;

    if (pArp == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        fmDbgConvertIPAddressToString(&pArp->ipAddr, ipText);
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "sw=%d, ip=%s, vlan=%d, interface=%d\n",
                     sw,
                     ipText,
                     pArp->vlan,
                     pArp->interface);

        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
        if (err == FM_OK)
        {
            if (pArp->interface >= 0)
            {
                err = fmGetInterface(sw, pArp->interface, &pIfEntry);
                if (err == FM_OK && pIfEntry != NULL)
                {
                    pArp->vlan = pIfEntry->vlan;
                }
            }
            if (err == FM_OK)
            {
                err = FindArpEntryExt(sw, pArp, &pArpEntry);
            }
            if (err == FM_OK)
            {
                err = fmTreeRemoveCertain(&switchPtr->arpPointersTree,
                                          (fm_uintptr)pArpEntry,
                                          NULL);
            }
            if (err == FM_OK)
            {
                /* remove the entry from the table */
                fmRemoveArp(switchPtr, pArpEntry);
            }
            if (err == FM_OK)
            {
                err = UpdateNextHopArpEntryRemovedInt(sw, pArpEntry);
            }

            if (err == FM_OK)
            {
                fmCustomTreeDestroy(&pArpEntry->nextHopTree, NULL);
                fmFree(pArpEntry);
                pArpEntry = NULL;

                /* If the switch requires extra processing, call the switch-specific
                 * function now. */
                if (switchPtr->DeleteArpEntry != NULL)
                {
                    err = switchPtr->DeleteArpEntry(sw, pArp);
                }
            }

            fmReleaseWriteLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteARPEntry */




/*****************************************************************************/
/** fmUpdateARPEntryDMAC
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Write a destination MAC address to an existing ARP entry.
 *                  this function is typically called by the application
 *                  when the actual MAC address has been learned for an IP
 *                  address that had been previously routed to the CPU.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArp points the ARP entry to modify. All fields of the
 *                  ''fm_arpEntry'' structure except the macAddr field will
 *                  be used to identify the existing ARP entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if the ARP entry does not exist.
 *
 *****************************************************************************/
fm_status fmUpdateARPEntryDMAC(fm_int       sw,
                               fm_arpEntry *pArp)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_intIpInterfaceEntry *pIfEntry;
    fm_intArpEntry         *pArpEntry;
    fm_customTreeIterator   iter;
    fm_intNextHop          *pHopKey;
    fm_intNextHop          *pNextHop;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pArp=%p\n",
                     sw,
                     (void*)pArp);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (pArp == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            /* get the vlan */
            if (pArp->interface >= 0)
            {
                err = fmGetInterface(sw, pArp->interface, &pIfEntry);

                if (err == FM_OK && pIfEntry != NULL)
                {
                    pArp->vlan = pIfEntry->vlan;
                }
            }
            if (err == FM_OK)
            {
                /* try to find the entry in the table */
                err = FindArpEntryExt(sw, pArp, &pArpEntry);
            }

            if (err == FM_OK)
            {
                /* update the destination mac address in the table entry */
                FM_MEMCPY_S( &pArpEntry->arp.macAddr,
                             sizeof(pArpEntry->arp.macAddr),
                             &pArp->macAddr,
                             sizeof(pArp->macAddr) );

                /* Find and update all affected next-hops */
                fmCustomTreeIterInit(&iter, &pArpEntry->nextHopTree);

                while (err == FM_OK)
                {
                    err = fmCustomTreeIterNext( &iter,
                                               (void **) &pHopKey,
                                               (void **) &pNextHop);

                    if (err == FM_OK)
                    {
                        FM_API_CALL_FAMILY(err,
                                           switchPtr->UpdateNextHop,
                                           sw,
                                           pNextHop);
                    }
                }
                /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
                err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
            }
            if (err == FM_OK)
            {
                /* If the switch requires extra processing, call the switch-specific
                 * function now. */
                if (switchPtr->UpdateArpEntryDestMac != NULL)
                {
                    err = switchPtr->UpdateArpEntryDestMac(sw, pArp);
                }
            }
        }
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmUpdateARPEntryDMAC */




/*****************************************************************************/
/** fmUpdateARPEntryVrid
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Update the egress virtual router ID associated with an 
 *                  existing ARP entry. This function is typically called by 
 *                  the application after calling ''fmAddARPEntry'' to modify 
 *                  the egress VRID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArp points the ARP entry to modify. All fields of the
 *                  ''fm_arpEntry'' structure will be used to identify the
 *                  existing ARP entry.
 * 
 * \param[in]       vrid is the new value to assign for the egress VRID.
 *                  0 indicates the real router.
 *                  For FM10000, -1 indicates no modification of VRID,
 *                  i.e. ingress VRID is assigned to the egress VRID.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if the ARP entry does not exist.
 *
 *****************************************************************************/
fm_status fmUpdateARPEntryVrid(fm_int       sw,
                               fm_arpEntry *pArp,
                               fm_int       vrid)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_intIpInterfaceEntry *pIfEntry;
    fm_intArpEntry         *pArpEntry;
    fm_customTreeIterator   iter;
    fm_intNextHop          *pHopKey;
    fm_intNextHop          *pNextHop;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pArp=%p, vrid=%d\n",
                     sw,
                     (void*)pArp,
                     vrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    VALIDATE_VIRTUAL_ROUTER_ID(sw, vrid);

    if ( (pArp == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            /* get the vlan */
            if (pArp->interface >= 0)
            {
                err = fmGetInterface(sw, pArp->interface, &pIfEntry);

                if (err == FM_OK && pIfEntry != NULL)
                {
                    pArp->vlan = pIfEntry->vlan;
                }
            }

            if (err == FM_OK)
            {
                /* try to find the entry in the table */
                err = FindArpEntryExt(sw, pArp, &pArpEntry);
            }

            if (err == FM_OK)
            {
                /* update the egress vrid in the table entry */
                pArpEntry->vrid = vrid;

                /* Find and update all affected next-hops */
                fmCustomTreeIterInit(&iter, &pArpEntry->nextHopTree);

                while (err == FM_OK)
                {
                    err = fmCustomTreeIterNext( &iter,
                                               (void **) &pHopKey,
                                               (void **) &pNextHop);
                    if (err == FM_OK)
                    {
                        FM_API_CALL_FAMILY(err,
                                           switchPtr->UpdateNextHop,
                                           sw,
                                           pNextHop);
                    }
                }
                /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
                err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
            }
            if (err == FM_OK)
            {
                /* If the switch requires extra processing, call the switch-specific
                 * function now. */
                if (switchPtr->UpdateArpEntryVrid != NULL)
                {
                    err = switchPtr->UpdateArpEntryVrid(sw, pArp, vrid);
                }
            }
        }
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmUpdateARPEntryVrid */




/*****************************************************************************/
/** fmCompareArps
 * \ingroup intNextHop
 *
 * \desc            Compare ARP entries.
 *
 * \param[in]       first points to the first arp entry.
 *
 * \param[in]       second points to the second arp entry.
 *
 * \return          -1 if the first arp entry sorts before the second.
 * \return           0 if the arp entries are identical.
 * \return           1 if the first arp entry sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareArps(const void *first, const void *second)
{
    fm_arpEntry *firstArp  = (fm_arpEntry *) first;
    fm_arpEntry *secondArp = (fm_arpEntry *) second;
    fm_int          i;

    i = (fm_int) firstArp->vlan - (fm_int) secondArp->vlan;

    if (i < 0)
    {
        return -1;
    }
    else if (i > 0)
    {
        return 1;
    }

    i = fmCompareIPAddresses(&firstArp->ipAddr, &secondArp->ipAddr);

    return i;

}   /* end fmCompareArps */




/*****************************************************************************/
/** fmCompareInternalArps
 * \ingroup intNextHop
 *
 * \desc            Compare internal ARP entries.
 *
 * \param[in]       first points to the first arp entry.
 *
 * \param[in]       second points to the second arp entry.
 *
 * \return          -1 if the first arp entry sorts before the second.
 * \return           0 if the arp entries are identical.
 * \return           1 if the first arp entry sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareInternalArps(const void *first, const void *second)
{
    fm_int          i;

    i = ( (fm_intArpEntry *)first)->arp.vlan - ( (fm_intArpEntry *)second)->arp.vlan;

    if (i < 0)
    {
        return -1;
    }
    else if (i > 0)
    {
        return 1;
    }

    i = fmCompareIPAddresses(&( (fm_intArpEntry *)first)->arp.ipAddr, &( (fm_intArpEntry *)second)->arp.ipAddr);

    return i;
}   /* end fmCompareInternalArps */




/*****************************************************************************/
/** fmGetARPEntryList
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Return a list of all ARP entries on a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      pNumArps points to caller allocated storage where
 *                  this function should place the number of ARP entries
 *                  returned in arpList.
 *
 * \param[out]      pArpList is an array that this function will fill
 *                  with the list of ARP entries.
 *
 * \param[in]       maxItems is the size of arpList, being the maximum
 *                  number of ARP entries that arpList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of ARP entries.
 *
 *****************************************************************************/
fm_status fmGetARPEntryList(fm_int       sw,
                            fm_int      *pNumArps,
                            fm_arpEntry *pArpList,
                            fm_int       maxItems)
{
    fm_switch      *switchPtr;
    fm_status       err;
    fm_int          arpCount;
    fm_intArpEntry *pArp;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, numArps=%p, arpList=%p, max=%d\n",
                     sw,
                     (void *) pNumArps,
                     (void *) pArpList,
                     maxItems);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    arpCount = 0;

    if (switchPtr->maxArpEntries > 0)
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            pArp = fmGetFirstArp(switchPtr);

            while (pArp != NULL)
            {
                if (arpCount >= maxItems)
                {
                    err = FM_ERR_BUFFER_FULL;
                    break;
                }

                pArpList[arpCount] = pArp->arp;
                arpCount++;
                pArp = fmGetNextArp(pArp);
            }

            if (err == FM_ERR_NO_MORE)
            {
                err = FM_OK;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    *pNumArps = arpCount;

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetARPEntryList */




/*****************************************************************************/
/** fmGetARPEntryFirst
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first ARP entry. The ARP entries are returned
 *                  in order of VRID, then VLAN and then IP address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to ''fmGetARPEntryNext''.
 *
 * \param[out]      firstArp points to caller-allocated storage where this
 *                  function will store the first ARP entry.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NO_MORE if there are no arp entries.
 *
 *****************************************************************************/
fm_status fmGetARPEntryFirst(fm_int       sw,
                             fm_voidptr * searchToken,
                             fm_arpEntry *firstArp)
{
    fm_switch *     switchPtr;
    fm_status       err;
    fm_intArpEntry *curArp;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, searchToken=%p, firstArp=%p\n",
                     sw,
                     (void*) searchToken,
                     (void*) firstArp);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (searchToken == NULL ||
        firstArp == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else 
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            curArp = fmGetFirstArp(switchPtr);

            if (curArp == NULL)
            {
                *searchToken = NULL;
                err = FM_ERR_NO_MORE;
            }
            else
            {
                *firstArp = curArp->arp;
                *searchToken = (fm_voidptr) curArp;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetARPEntryFirst */




/*****************************************************************************/
/** fmGetARPEntryNext
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next ARP entry. The ARP entries are returned
 *                  in order of VRID, then VLAN and then IP address.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   pSearchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fmGetARPEntryFirst''. It will be
 *                  updated by this function with a new value to be used in a
 *                  subsequent call to this function.
 *
 * \param[out]      pNextArp points to caller-allocated storage where this
 *                  function will store the next ARP entry.
 *
 * \return          FM_OK if successful.
 * 
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NO_MORE if there are no more ARP entries.
 * \return          FM_ERR_NOT_FOUND if the value stored in pSearchToken was not
 *                  found.
 * \return        
 *
 *****************************************************************************/
fm_status fmGetARPEntryNext(fm_int       sw,
                            fm_voidptr  *pSearchToken,
                            fm_arpEntry *pNextArp)
{
    fm_switch      *switchPtr;
    fm_status       err;
    fm_intArpEntry *pWorkingArp;
    void           *value;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pSearchToken=%p, pNextArp=%p\n",
                     sw,
                     (void*) pSearchToken,
                     (void*) pNextArp);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (pSearchToken == NULL || pNextArp == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            err = fmTreeFind(&switchPtr->arpPointersTree,
                             (fm_uintptr)*pSearchToken,
                             &value);
            if (err != FM_OK)
            {
                fmReleaseReadLock(&switchPtr->routingLock);
                FM_LOG_ABORT(FM_LOG_CAT_ROUTING, err);
            }
            pWorkingArp = fmGetNextArp( (fm_intArpEntry *) *pSearchToken);

            if (pWorkingArp == NULL)
            {
                err = FM_ERR_NO_MORE;
            }
            else
            {
                *pNextArp     = pWorkingArp->arp;
                *pSearchToken = (void *) pWorkingArp;
            }

            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetARPEntryNext */




/*****************************************************************************/
/** fmFindArpEntry
 * \ingroup intNextHopArp
 *
 * \desc            Find an ARP entry in the ARP table.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pArpAddr points to the ARP IP Address.
 *
 * \param[in]       vlan is the vlan number on which to filter the search.
 *
 * \param[out]      ppArpEntry contains the pointer to the ARP entry,
 *                  or NULL if the entry wasn't found. May be NULL.
 *
 * \return          FM_OK if the ARP entry was found.
 * \return          FM_ERR_NOT_FOUND if the entry wasn't found.
 * \return          FM_ERR_INVALID_ARGUMENT if pArpAddr is NULL.
 *
 *****************************************************************************/
fm_status fmFindArpEntry(fm_int           sw,
                         fm_ipAddr       *pArpAddr,
                         fm_uint16        vlan,
                         fm_intArpEntry **ppArpEntry)
{
    fm_switch      *switchPtr;
    fm_status       err;
    fm_intArpEntry *pSearchArp;
    fm_int16        tmpVlan;
    fm_bool         found;
    fm_int          ipAddComparition;
    fm_char         arpIpAddrTxt[100];

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pArpAddr=%p, vlan=%u, ppArpEntry=%p\n",
                  sw,
                  (void*) pArpAddr,
                  vlan,
                  (void*) ppArpEntry );

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* arg validation */
    if (pArpAddr == NULL || ppArpEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        fmDbgConvertIPAddressToString(pArpAddr, arpIpAddrTxt);
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "sw=%d, IpAddr= %s, vlan=%u\n",
                      sw,
                      arpIpAddrTxt,
                      vlan);

        *ppArpEntry = NULL;
        found = FALSE;

        pSearchArp = fmGetFirstArp(switchPtr);

        while (pSearchArp != NULL)
        {
            tmpVlan = pSearchArp->arp.vlan;

            if (tmpVlan < vlan)
            {
                /* avoid additional comparitions */
                pSearchArp = fmGetNextArp(pSearchArp);
                continue;
            }
            else if (tmpVlan > vlan)
            {
                /* Not found */
                break;
            }
            else
            {
                ipAddComparition = fmCompareIPAddresses(&pSearchArp->arp.ipAddr,pArpAddr);
                if (ipAddComparition > 0)
                {
                    /* Not found */
                    break;
                }
                else if (ipAddComparition == 0)
                {
                    found = TRUE;
                    break;
                }
            }
            pSearchArp = fmGetNextArp(pSearchArp);
        }

        *ppArpEntry = pSearchArp;

        if (found == TRUE)
        {
            fmDbgConvertIPAddressToString(pArpAddr, arpIpAddrTxt);
            FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                          "Found ARP, entry=%p\n",
                          (void *) pSearchArp );
        }
        else
        {
            err = FM_ERR_NOT_FOUND;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmFindArpEntry */




/*****************************************************************************/
/** fmGetARPEntryInfo
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns information about an ARP table entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArpEntry points to the arp entry to be accessed.
 *
 * \param[out]      pArpInfo points to a ''fm_arpEntryInfo'' structure into
 *                  which this function will place the results.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmGetARPEntryInfo(fm_int           sw,
                            fm_arpEntry     *pArpEntry,
                            fm_arpEntryInfo *pArpInfo)
{
    fm_switch      *switchPtr;
    fm_status       err;
    fm_intArpEntry *pFoundArp;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, 
                     "sw=%d, pArpEntry=%p, pArpInfo=%p\n",
                     sw,
                     (void*) pArpEntry,
                     (void*) pArpInfo);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pArpEntry == NULL || pArpInfo == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            err = FindArpEntryExt(sw, pArpEntry, &pFoundArp);

            if (err == FM_OK)
            {
                /* Fill in the ARP table entry structure. */
                pArpInfo->arp = pFoundArp->arp;

                /* Get the hardware "used" flag. */
                err = fmGetARPEntryUsedInternal(sw, pArpEntry, &pArpInfo->used, FALSE);
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetARPEntryInfo */




/*****************************************************************************/
/** fmGetARPEntryUsed
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieves the "used" flag for an ARP entry.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArp points to the arp entry to be accessed.
 *
 * \param[out]      pUsed points to caller-allocated storage where this
 *                  function should place the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NOT_FOUND if the specified ARP entry is not
 *                  recognized.
 *
 *****************************************************************************/
fm_status fmGetARPEntryUsed(fm_int       sw,
                            fm_arpEntry *pArp,
                            fm_bool     *pUsed,
                            fm_bool      resetFlag)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pArp=%p, pUsed=%p, resetFlag=%d\n",
                     sw,
                     (void*) pArp,
                     (void*) pUsed,
                     resetFlag);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pArp == NULL || pUsed == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            err = fmGetARPEntryUsedInternal(sw, pArp, pUsed, resetFlag);
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetARPEntryUsed */




/*****************************************************************************/
/** fmGetARPEntryUsedInternal
 * \ingroup intNextHopArp
 *
 * \desc            Retrieves the "used" flag for an ARP entry.
 *
 * \note            This function assumes the routing lock has been taken.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pArp points to the arp entry to be accessed.
 *
 * \param[out]      pUsed points to caller-allocated storage where this function
 *                  should place the result.
 *
 * \param[in]       resetFlag specifies whether the flag should be reset
 *                  after it is read.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NOT_FOUND if the specified ARP entry is not
 *                  recognized.
 *
 *****************************************************************************/
fm_status fmGetARPEntryUsedInternal(fm_int       sw,
                                    fm_arpEntry *pArp,
                                    fm_bool     *pUsed,
                                    fm_bool      resetFlag)
{
    fm_switch            *switchPtr;
    fm_status             err;
    fm_intArpEntry       *pFoundArp;
    fm_customTreeIterator iter;
    fm_intNextHop        *pNextHopKey;
    fm_intNextHop        *pNextHop;
    fm_bool               nextHopUsed;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pArp=%p, pUsed=%p, resetFlag=%d\n",
                 sw,
                 (void*) pArp,
                 (void*) pUsed,
                 resetFlag);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pArp == NULL || pUsed == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->maxArpEntries <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = FindArpEntryExt(sw, pArp, &pFoundArp);
        *pUsed = FALSE;

        if (err == FM_OK)
        {
            /* Check every next-hop that uses this ARP to see if it was used.  If
             * resetFlag is FALSE, we can stop as soon as we find one that is used,
             * because we only return TRUE/FALSE. If resetFlag is TRUE, we have
             * to check every next-hop. */
            fmCustomTreeIterInit(&iter, &pFoundArp->nextHopTree);

            while (err == FM_OK)
            {
                err = fmCustomTreeIterNext( &iter,
                                           (void **) &pNextHopKey,
                                           (void **) &pNextHop );

                if (err == FM_OK)
                {
                    FM_API_CALL_FAMILY(err,
                                       switchPtr->GetNextHopUsed,
                                       sw,
                                       pNextHop,
                                       &nextHopUsed,
                                       resetFlag);
                }
                if (err == FM_OK && nextHopUsed == TRUE)
                {
                    *pUsed = TRUE;

                    if (!resetFlag)
                    {
                        break;
                    }
                }
            }       /* end while (err == FM_OK) */

            /* clear the error if == FM_ERR_NO_MORE, it is a normal loop ending condition */
            err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetARPEntryUsedInternal */




/*****************************************************************************/
/** fmRefreshARPUsedCache
 * \ingroup routerArp
 *
 * \chips           FM4000, FM6000
 *
 * \desc            Takes a snapshot of the ARP used bits in hardware and
 *                  caches them in memory for subsequent examination with
 *                  ''fmGetARPEntryUsed'', ''fmGetMcastGroupUsed'' or
 *                  ''fmGetECMPGroupNextHopUsed''. This function may also be
 *                  used to invalidate the ARP used cache.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       invalidateCache should be set to TRUE if the cache is to 
 *                  be deleted.
 *
 * \param[in]       resetFlag specifies whether the hardware ARP used bits
 *                  should be reset after they are read into the cache or
 *                  the cache is deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_NO_MEM if no memory available for creating the
 *                  cached snapshot.
 *
 *****************************************************************************/
fm_status fmRefreshARPUsedCache(fm_int  sw,
                                fm_bool invalidateCache,
                                fm_bool resetFlag)
{
    fm_status  err;
    fm_switch *switchPtr;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, invalidateCache=%d, resetFlag=%d\n",
                     sw,
                     invalidateCache,
                     resetFlag);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err == FM_OK)
    {
        FM_API_CALL_FAMILY(err,
                           switchPtr->RefreshARPUsedCache,
                           sw,
                           invalidateCache,
                           resetFlag);

        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmRefreshARPUsedCache */




/*****************************************************************************/
/** fmDbgDumpArpTable
 * \ingroup diagMisc
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Displays the ARP table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       verbose indicates whether to include the first 20 entries,
 *                  even if they are all zeros.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP  if the switch is  not running
 * 
 *
 *****************************************************************************/
fm_status fmDbgDumpArpTable(fm_int sw, fm_bool verbose)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH( sw );

    switchPtr = GET_SWITCH_PTR( sw );

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpArpTable, sw, verbose);

    UNPROTECT_SWITCH( sw );
    return FM_OK;

}   /* end fmDbgDumpArpTable */




/*****************************************************************************/
/** fmDbgDumpArpHandleTable
 * \ingroup diagMisc
 *
 * \chips           FM10000
 *
 * \desc            Dumps the ARP handle table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       verbose indicates whether to include empty entries or not.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP  if the switch is  not running
 * 
 *
 *****************************************************************************/
fm_status fmDbgDumpArpHandleTable(fm_int sw, fm_bool verbose)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH( sw );

    switchPtr = GET_SWITCH_PTR( sw );

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpArpHandleTable, sw, verbose);

    UNPROTECT_SWITCH( sw );
    return FM_OK;

}   /* end fmDbgDumpArpHandleTable */




/*****************************************************************************/
/** fmDbgPlotArpUsedDiagram
 * \ingroup diagMisc
 *
 * \chips           FM10000
 *
 * \desc            Plots a diagram that shows ARP table usage.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 *
 *****************************************************************************/
fm_status fmDbgPlotArpUsedDiagram(fm_int sw)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH( sw );

    switchPtr = GET_SWITCH_PTR( sw );

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgPlotUsedArpDiagram, sw);

    UNPROTECT_SWITCH( sw );
    return FM_OK;

}   /* end fmDbgPlotArpUsedDiagram */




/*****************************************************************************/
/** fmCreateInterface
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Create an interface between the layer 3 router and the
 *                  the layer 2 switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      pInterface points to caller-allocated storage where this
 *                  function should place the interface number (handle) of the
 *                  newly created interface.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_TABLE_FULL if the maximum number of interfaces
 *                  (''FM_MAX_IP_INTERFACES'') have already been created.
 *
 *****************************************************************************/
fm_status fmCreateInterface(fm_int  sw,
                            fm_int *pInterface)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_int                  ifNum;
    fm_intIpInterfaceEntry *pIfEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pInterface=%p\n",
                     sw,
                     (void*) pInterface);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    pIfEntry = NULL;
    ifNum = -1;

    if (pInterface == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* set the value returned by default */
        *pInterface = -1;

        if ( switchPtr->ipInterfaceEntries == NULL )
        {
            err = FM_ERR_UNSUPPORTED;
        }
        else 
        {
            err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

            if (err == FM_OK)
            {
                /* find an available interface */
                err = fmFindBitInBitArray(&switchPtr->ipInterfaceEntriesInUse,
                                          0,
                                          FALSE,
                                          &ifNum);

                if ( err == FM_OK &&
                     ( (ifNum < 0) || (ifNum >= switchPtr->maxIpInterfaces) ) )
                {
                    /* no more entries are available */
                    err = FM_ERR_TABLE_FULL;
                }

                if (err == FM_OK)
                {
                    /* set the entry as "inUse" */
                    err = fmSetBitArrayBit(&switchPtr->ipInterfaceEntriesInUse, 
                                           ifNum,
                                           TRUE);
                }

                if (err == FM_OK)
                {
                    /* initialize the new interface */
                    pIfEntry               = &switchPtr->ipInterfaceEntries[ifNum];
                    pIfEntry->interfaceNum = ifNum;
                    pIfEntry->vlan         = FM_INVALID_VLAN;
                    pIfEntry->state        = FM_INTERFACE_STATE_ADMIN_UP;
                    pIfEntry->extension    = NULL;
                    fmInitInterfaceEntryLinkedLists(pIfEntry);
                }

                /* call the low level function */
                if (err == FM_OK && switchPtr->CreateInterface != NULL)
                {
                    err = switchPtr->CreateInterface(sw, ifNum);
                }

                if (err == FM_OK)
                {
                    *pInterface = ifNum;
                }
                fmReleaseWriteLock(&switchPtr->routingLock);
            }

        }   /* end else if ( switchPtr->ipInterfaceEntries == NULL ) */

        /* on error: tag the interface slot as "not-inUse"  */
        if (err != FM_OK && ifNum >= 0)
        {
            err = fmSetBitArrayBit(&switchPtr->ipInterfaceEntriesInUse, 
                               ifNum,
                               FALSE);
            if (pIfEntry != NULL)
            {
                pIfEntry->interfaceNum = -1;
            }
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmCreateInterface */




/*****************************************************************************/
/** fmDeleteInterface
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Delete an interface between the layer 3 router and the
 *                  the layer 2 switch, releasing any allocated resources for
 *                  this interface.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface number returned by
 *                  ''fmCreateInterface''.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if interface is not recognized.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmDeleteInterface(fm_int sw,
                            fm_int interface)
{
    fm_switch *                     switchPtr;
    fm_status                       err;
    fm_intIpInterfaceEntry *        pIfEntry;
    fm_bool                         ifIsInUse;
    fm_intIpInterfaceAddressEntry * pAddrEntry;
    fm_intArpEntry *                pArpEntry;
    fm_intArpEntry *                pNextArpEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, interface=%d\n",
                     sw,
                     interface);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    pIfEntry = NULL;

    if (interface < 0 )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if ( switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            /* make sure the interface is in use */
            err = fmGetBitArrayBit(&switchPtr->ipInterfaceEntriesInUse,
                                   interface,
                                   &ifIsInUse);

            if (err == FM_OK && !ifIsInUse)
            {
                err = FM_ERR_NOT_FOUND;
            }

            if (err == FM_OK)
            {
                /* Delete all arp entries associated with this interface */
                pArpEntry = fmGetFirstArp(switchPtr);
                while (pArpEntry != NULL)
                {
                    if (pArpEntry->ifEntry != NULL)
                    {
                        if (pArpEntry->ifEntry->interfaceNum == interface)
                        {
                            pNextArpEntry = fmGetNextArp(pArpEntry);
                            fmDeleteARPEntry(sw,&pArpEntry->arp);
                            pArpEntry = pNextArpEntry;
                            continue;
                        }
                    }
                    pArpEntry = fmGetNextArp(pArpEntry);
                }
            }
            
            /* prepare to delete all addresses associated with this interface */
            if (err == FM_OK)
            {
                pIfEntry = &switchPtr->ipInterfaceEntries[interface];
                /* Change the interface state to be able to update the ECMP group */
                pIfEntry->state = FM_INTERFACE_STATE_ADMIN_DOWN;

                /* Disable all next-hops using this interface */
                err = UpdateEcmpGroupsForInterface(sw, pIfEntry);
            }

            /* delete all addresses associated with this interface */
            while ( (err == FM_OK) &&
                    ( ( pAddrEntry = fmGetFirstInterfaceAddress(pIfEntry) ) != NULL) )
            {
                err = DeleteInterfaceAddrInt(sw,
                                             pIfEntry,
                                             &pAddrEntry->addr);
            }

            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Routing DELETE I/F: Updating ECMP Groups "
                             "for interface failed with error %d (%s)\n",
                             err,
                             fmErrorMsg(err) );
            }
            else
            {

                if (switchPtr->DeleteInterface != NULL)
                {
                    err = switchPtr->DeleteInterface(sw, interface);

                    if (err != FM_OK)
                    {
                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                     "DeleteInterface(%d,%d) failed: %s\n",
                                     sw,
                                     interface,
                                     fmErrorMsg(err) );
                    }
                }
            }

            if (err == FM_OK)
            {
                pIfEntry->interfaceNum = -1;
                pIfEntry->vlan         = FM_INVALID_VLAN;
                pIfEntry->state        = FM_INTERFACE_STATE_ADMIN_DOWN;
                fmInitInterfaceEntryLinkedLists(pIfEntry);

                /* Release the interface for future re-use */
                err = fmSetBitArrayBit(&switchPtr->ipInterfaceEntriesInUse,
                                       interface,
                                       FALSE);
            }
            fmReleaseWriteLock(&switchPtr->routingLock);
        }


    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteInterface */




/*****************************************************************************/
/** fmGetInterfaceList
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Return a list of all interface numbers on a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      pNumInterfaces points to caller allocated storage where
 *                  this function should place the number of interfaces
 *                  returned in interfaceList.
 *
 * \param[out]      pInterfaceList is an array that this function will fill
 *                  with the list of interfaces numbers.
 *
 * \param[in]       maxIfNum is the size of interfaceList, being the maximum
 *                  number of interface numbers that interfaceList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of interface numbers.
 * \return          FM_ERR_UNSUPPORTED if the switch doesn't support routing.
 *
 *****************************************************************************/
fm_status fmGetInterfaceList(fm_int  sw,
                             fm_int *pNumInterfaces,
                             fm_int *pInterfaceList,
                             fm_int  maxIfNum)
{
    fm_switch *switchPtr;
    fm_int     interfaceId;
    fm_status  err;
    fm_int     ifCount = 0;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, numInterfaces=%p, interfaceList=%p, max=%d\n",
                     sw,
                     (void*) pNumInterfaces,
                     (void*) pInterfaceList,
                     maxIfNum);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    ifCount = 0;
    interfaceId = -1;

    if (pNumInterfaces == NULL || pInterfaceList == NULL || maxIfNum <= 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        *pNumInterfaces = 0;
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            while (err == FM_OK && ifCount < maxIfNum)
            {
                err = fmFindBitInBitArray(&switchPtr->ipInterfaceEntriesInUse,
                                          interfaceId + 1,
                                          TRUE,
                                          &interfaceId);

                if (err == FM_OK)
                {
                    if (interfaceId < 0)
                    {
                        /***************************************** 
                         * fmFindBitInBitArray returns FM_OK and
                         *  interfaceId = -1 if there are no more
                         *  interfaces
                         *****************************************/ 
                        break;
                    }
                    else
                    {
                        pInterfaceList[ifCount++] = interfaceId;
                    }
                }
            }

            if (err == FM_OK)
            {
                *pNumInterfaces = ifCount;
                if (ifCount >= maxIfNum)
                {
                    err = FM_ERR_BUFFER_FULL;
                }
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceList */




/*****************************************************************************/
/** fmGetInterfaceFirst
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first interface number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      pFirstInterface points to caller-allocated storage where
 *                  this function will store the first interface number. Will
 *                  be set to -1 if no interfaces found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no interfaces.
 * \return          FM_ERR_UNSUPPORTED if the switch doesn't support routing.
 *
 *****************************************************************************/
fm_status fmGetInterfaceFirst(fm_int sw,
                              fm_int *pFirstInterface)
{
    fm_switch *switchPtr;
    fm_int     interfaceNum;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, pFirstIf=%p\n",
                     sw,
                     (void*) pFirstInterface);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    interfaceNum = -1;

    if (pFirstInterface == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            err = fmFindBitInBitArray(&switchPtr->ipInterfaceEntriesInUse,
                                      interfaceNum + 1,
                                      TRUE,
                                      &interfaceNum);
            if (err == FM_OK)
            {
                *pFirstInterface = interfaceNum;

                if (interfaceNum < 0)
                {
                    err = FM_ERR_NO_MORE;
                }
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceFirst */




/*****************************************************************************/
/** fmGetInterfaceNext
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next interface number.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentInterface is the last interface found by a previous
 *                  call to this function or to ''fmGetInterfaceFirst''.
 *
 * \param[out]      pNextInterface points to caller-allocated storage where this
 *                  function will store the next interface number. Will be set
 *                  to -1 if no more interfaces found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no more interfaces.
 * \return          FM_ERR_UNSUPPORTED if the switch doesn't support routing.
 *
 *****************************************************************************/
fm_status fmGetInterfaceNext(fm_int  sw,
                             fm_int  currentInterface,
                             fm_int *pNextInterface)
{
    fm_switch *switchPtr;
    fm_int     interfaceNum;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, currentInterface=%d, pNextIf=%p\n",
                     sw,
                     currentInterface,
                     (void*) pNextInterface);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    interfaceNum = -1;

    if (pNextInterface == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            err = fmFindBitInBitArray(&switchPtr->ipInterfaceEntriesInUse,
                                      currentInterface + 1,
                                      TRUE,
                                      &interfaceNum);

            if (err == FM_OK)
            {
                *pNextInterface = interfaceNum;

                if (interfaceNum < 0)
                {
                    err = FM_ERR_NO_MORE;
                }
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceNext */




/*****************************************************************************/
/** fmGetInterfaceVlan
 * \ingroup intNextHop
 *
 * \desc            Retrieves a vlan from an interface.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pIfAddr points to the interface address.
 *
 * \param[in]       alternateVlan is the vlan to be returned if ifAddr
 *                  contains an empty IP address.
 *
 * \return          interface vlan or alternateVlan.
 *
 *****************************************************************************/
fm_uint16 fmGetInterfaceVlan(fm_int     sw,
                             fm_ipAddr *pIfAddr,
                             fm_uint16  alternateVlan)
{
    fm_status               err;
    fm_uint16               vlan;
    fm_intIpInterfaceEntry *pIfEntry;

    vlan = FM_INVALID_VLAN;
    err = FM_OK;
    pIfEntry = NULL;

    if (pIfAddr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fmFindInterface(sw, pIfAddr, &pIfEntry);

        if (err != FM_OK)
        {
            pIfEntry = NULL;
        }
        vlan = (pIfEntry != NULL)? pIfEntry->vlan : alternateVlan;
    }

    return vlan;

}   /* end fmGetInterfaceVlan */




/*****************************************************************************/
/** fmSetInterfaceAttribute
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Set an interface attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface on which to operate.
 *
 * \param[in]       attr is the interface attribute to set (see
 *                  ''Router Interface Attributes'').
 *
 * \param[in]       pValue points to the attribute value to set
 *                  Note: To set the Interface VLAN attribute to "no vlan",
 *                  use the constant FM_INVALID_VLAN for the vlan value.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmSetInterfaceAttribute(fm_int    sw,
                                  fm_int    interface,
                                  fm_int    attr,
                                  void     *pValue)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_intIpInterfaceEntry *pIfEntry;
    fm_interfaceState       newState;
    fm_uint16               newVlan;
    fm_bool                 updateHw;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d, if=%d, attr=%d, pValue=%p\n",
                     sw,
                     interface,
                     attr,
                     pValue);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    updateHw = FALSE;

    /* validate arguments */
    if (pValue == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if ( (switchPtr->SetInterfaceAttribute == NULL) ||
             (switchPtr->ipInterfaceEntries    == NULL) )
        {
            err = FM_ERR_UNSUPPORTED;
        }
        else
        {
            err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

            if (err == FM_OK)
            {
                /* make sure the interface exists, otherwise fmGetInterface returns an error */
                err = fmGetInterface(sw, interface, &pIfEntry);

                if (err == FM_OK)
                {
                    switch (attr)
                    {
                        case FM_INTERFACE_STATE:
                            newState = *( (fm_interfaceState *) pValue );
                            if (newState != pIfEntry->state)
                            {
                                /* Update all ECMP Groups with next-hops using this interface */
                                pIfEntry->state = newState;
                                updateHw = TRUE;
                                err = UpdateEcmpGroupsForInterface(sw, pIfEntry);
                            }
                            break;

                        case FM_INTERFACE_VLAN:

                            newVlan = *( (fm_uint16 *) pValue );
                            if (newVlan != pIfEntry->vlan)
                            {
                                /* scan all other interfaces to make sure this vlan
                                 * isn't already in use
                                 */
                                err = CheckIfInterfaceVlanIsAlreadyUsed(sw, newVlan);

                                if (err == FM_OK)
                                {
                                    pIfEntry->vlan = newVlan;
                                    updateHw = TRUE;
                                    /* Find and update all ARP entries that use this interface */
                                    err = UpdateArpEntriesVlanValue(sw, pIfEntry, newVlan);
                                }
                                if (err == FM_OK)
                                {
                                    /* Update all ECMP Groups with next-hops using this interface */
                                    err = UpdateEcmpGroupsForInterface(sw, pIfEntry);
                                }
                            }
                            break;

                        default:
                            err = FM_ERR_INVALID_ATTRIB;
                            break;

                    }   /* end switch (attr) */
                }

                /* Update the hardware */
                if ( (err == FM_OK) && (updateHw == TRUE) )
                {
                    err = switchPtr->SetInterfaceAttribute(sw, interface, attr, pValue);
                }
                fmReleaseReadLock(&switchPtr->routingLock);
            }
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetInterfaceAttribute */




/*****************************************************************************/
/** fmGetInterfaceAttribute
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Get the current value of an interface attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface on which to operate.
 *
 * \param[in]       attr is the interface attribute to set (see
 *                  ''Router Interface Attributes'').
 *
 * \param[out]      pValue points to a caller-allocated storage where
 *                  this function will place the value of the attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmGetInterfaceAttribute(fm_int sw,
                                  fm_int interface,
                                  fm_int attr,
                                  void  *pValue)
{
    fm_switch              *switchPtr;
    fm_status               err;
    fm_intIpInterfaceEntry *pIfEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, 
                     "sw=%d, if=%d, attr=%d, pValue=%p\n",
                     sw,
                     interface,
                     attr,
                     pValue);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    if (pValue == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED; 
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
        if (err == FM_OK)
        {
            /* make sure the interface already exists */
            err = fmGetInterface(sw, interface, &pIfEntry);
            if (err == FM_OK)
            {
                switch (attr)
                {
                    case FM_INTERFACE_STATE:
                        *( (fm_interfaceState *) pValue ) = pIfEntry->state;
                        break;
                    case FM_INTERFACE_VLAN:
                        *( (fm_uint16 *) pValue ) = pIfEntry->vlan;
                        break;
                    default:
                        err = FM_ERR_INVALID_ATTRIB;
                        break;
                }   /* end switch (attr) */
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceAttribute */




/*****************************************************************************/
/** fmAddInterfaceAddr
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Assign an IP address to an interface. Any route that was
 *                  using this address will automatically be activated if
 *                  possible. Multiple IP addresses may be added per interface.
 *
 * \note            The same IP address cannot be added twice on the same
 *                  switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface on which to operate.
 *
 * \param[in]       pAddr points to an IP address to assign to this interface.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_NO_MEM if not enough memory for address storage.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmAddInterfaceAddr(fm_int     sw,
                             fm_int     interface,
                             fm_ipAddr *pAddr)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_intIpInterfaceEntry        *pIfEntry;
    fm_intIpInterfaceAddressEntry *pAddrEntry;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, interface=%d\n",
                     sw,
                     interface);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;
    pAddrEntry = NULL;

    if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if (pAddr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
        if (err == FM_OK)
        {
            /* make sure the interface exists, otherwise fmGetInterface returns an error */
            err = fmGetInterface(sw, interface, &pIfEntry);

            if (err == FM_OK)
            {
                /* Allocate and initialize a new address entry */
                pAddrEntry = (fm_intIpInterfaceAddressEntry *) fmAlloc( sizeof(fm_intIpInterfaceAddressEntry) );

                if (pAddrEntry == NULL)
                {
                    err = FM_ERR_NO_MEM;
                }
            }

            if (err == FM_OK)
            {
                /* Initialize the new address entry */
                FM_CLEAR(*pAddrEntry);

                pAddrEntry->ifEntry = pIfEntry;
                pAddrEntry->addr    = *pAddr;
                fmCustomTreeInit(&pAddrEntry->nextHopTree, fmCompareInternalNextHops);

                /* Add the address entry to the interface address list */
                fmAppendInterfaceAddress(pIfEntry, pAddrEntry);

                FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                              "Interface Address Entry %p added to Interface %p (%d)\n",
                              (void *) pAddrEntry,
                              (void *) pIfEntry,
                              pIfEntry->interfaceNum );

                /* Deliver the add command to the lower-level, if needed */
                if (switchPtr->AddInterfaceAddr != NULL)
                {
                    err = switchPtr->AddInterfaceAddr(sw, interface, pAddr);
                    /* if error, destroy the interface */

                    /* TBC: destroy the interface */
                }
            }

            if (err == FM_OK)
            {
                err = UpdateNextHopInterfaceAddrInt(sw, pAddr, pAddrEntry);
            }

            /* release the lock */
            fmReleaseWriteLock(&switchPtr->routingLock);
        }
    }

    if (err != FM_OK)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "updating interface IP address\n");
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmAddInterfaceAddr */




/*****************************************************************************/
/** fmDeleteInterfaceAddr
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Delete an IP address from an interface. Note that any
 *                  route that was using this address will automatically be
 *                  placed in a disabled state.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface on which to operate.
 *
 * \param[in]       pAddr points to the IP address to be deleted from this
 *                  interface.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument pointer is NULL.
 * \return          FM_ERR_INVALID_IPADDR if the IP address is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmDeleteInterfaceAddr(fm_int     sw,
                                fm_int     interface,
                                fm_ipAddr *pAddr)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_intIpInterfaceEntry        *pIfEntry;
    fm_intIpInterfaceAddressEntry *pAddrEntry;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d interface=%d pAddr=%p\n",
                     sw,
                     interface,
                     (void*)pAddr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    pAddrEntry = NULL;

    if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else if (pAddr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            /* make sure the interface exists, otherwise fmGetInterface returns an error */
            err = fmGetInterface(sw, interface, &pIfEntry);

            if (err == FM_OK)
            {
                err = DeleteInterfaceAddrInt(sw,
                                             pIfEntry,
                                             pAddr);
            }
            fmReleaseWriteLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteInterfaceAddr */




/*****************************************************************************/
/** fmGetInterfaceAddrList
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Return a list of addresses assigned to an interface.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface number (returned by
 *                  ''fmCreateInterface'') from which the addresses should be
 *                  retrieved.
 *
 * \param[out]      pNumAddresses points to a location where
 *                  this function should store the number of addresses
 *                  returned in pAddressList.
 *
 * \param[out]      pAddressList is an array that this function will fill with
 *                  the list of addresses assigned to the interface.
 *
 * \param[in]       maxEntries is the size of addressList, being the maximum
 *                  number of addresses that addressList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument pointer is NULL.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of addresses.
 *
 *****************************************************************************/
fm_status fmGetInterfaceAddrList(fm_int     sw,
                                 fm_int     interface,
                                 fm_int    *pNumAddresses,
                                 fm_ipAddr *pAddressList,
                                 fm_int     maxEntries)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_intIpInterfaceEntry        *pIfEntry;
    fm_intIpInterfaceAddressEntry *pAddrEntry;
    fm_int                         addrCount;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, interface = %d, numAddresses = %p, "
                     "addressList = %p, maxEntries = %d\n",
                     sw,
                     interface,
                     (void *) pNumAddresses,
                     (void *) pAddressList,
                     maxEntries);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    addrCount = 0;
    pAddrEntry = NULL;
    err = FM_OK;

    if (pNumAddresses == NULL || pAddressList == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        *pNumAddresses = 0;
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            /* make sure the interface exists */
            err = fmGetInterface(sw, interface, &pIfEntry);

            if (err == FM_OK)
            {
                pAddrEntry = fmGetFirstInterfaceAddress(pIfEntry);
            }

            while (pAddrEntry != NULL)
            {
                if (addrCount >= maxEntries)
                {
                    err = FM_ERR_BUFFER_FULL;
                    break;
                }
                pAddressList[addrCount++] = pAddrEntry->addr;
                pAddrEntry = fmGetNextInterfaceAddress(pAddrEntry);
            }

            if (err == FM_OK || err == FM_ERR_BUFFER_FULL)
            {
                *pNumAddresses = addrCount;
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }

    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceAddrList */




/*****************************************************************************/
/** fmGetInterfaceAddrFirst
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first address assigned to this interface.
 *                  Addresses are retrieved in numerical order.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       interface is the interface on which to operate.
 *
 * \param[out]      pFirstAddr points to caller-allocated storage where this
 *                  function will store the first address.  The structure
 *                  will be filled with zeroes if there are no IP addresses
 *                  assigned to this interface.
 *
 * \param[out]      pSearchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to ''fmGetInterfaceAddrNext''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument pointer is NULL.
 * \return          FM_ERR_NO_MORE if there are no addresses on this interface.
 *
 *****************************************************************************/
fm_status fmGetInterfaceAddrFirst(fm_int      sw,
                                  fm_int      interface,
                                  fm_voidptr *pSearchToken,
                                  fm_ipAddr  *pFirstAddr)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_intIpInterfaceEntry        *pIfEntry;
    fm_intIpInterfaceAddressEntry *pAddrEntry;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d\n, if=%d, pSearchToken=%p, pFirstAdd=%p\n",
                     sw,
                     interface,
                     (void*)pSearchToken,
                     (void*)pFirstAddr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    pAddrEntry = NULL;
    err = FM_OK;

    if (pSearchToken == NULL || pFirstAddr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            /* make sure the interface exists */
            err = fmGetInterface(sw, interface, &pIfEntry);

            if (err == FM_OK)
            {
                pAddrEntry = fmGetFirstInterfaceAddress(pIfEntry);

                if (pAddrEntry == NULL)
                {
                    err = FM_ERR_NO_MORE;
                }
                else
                {
                    *pFirstAddr   = pAddrEntry->addr;
                    *pSearchToken = (fm_voidptr) pAddrEntry;
                }
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceAddrFirst */




/*****************************************************************************/
/** fmGetInterfaceAddrNext
 * \ingroup routerIf
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next address assigned to this interface.
 *                  Addresses are retrieved in numerical order.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   pSearchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fmGetInterfaceAddrFirst''. It will be
 *                  updated by this function with a new value to be used in a
 *                  subsequent call to this function.
 *
 * \param[out]      pNextAddr points to caller-allocated storage where this
 *                  function will store the next address.  The structure
 *                  will be filled with zeroes if there are no more IP
 *                  addresses assigned to this interface.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_INTERFACE if interface is unknown.
 * \return          FM_ERR_INVALID_ARGUMENT if an argument pointer is NULL.
 * \return          FM_ERR_NO_MORE if there are no more addresses on this
 *                  interface.
 *
 *****************************************************************************/
fm_status fmGetInterfaceAddrNext(fm_int      sw,
                                 fm_voidptr *pSearchToken,
                                 fm_ipAddr  *pNextAddr)
{
    fm_switch                     *switchPtr;
    fm_status                      err;
    fm_intIpInterfaceAddressEntry *pAddrEntry;


    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw=%d\n, pSearchToken=%p, pNextAddr=%p\n",
                     sw,
                     (void*)pSearchToken,
                     (void*)pNextAddr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);
    pAddrEntry = NULL;
    err = FM_OK;

    if (pSearchToken == NULL || pNextAddr == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else if (switchPtr->ipInterfaceEntries == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
    }
    else
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            pAddrEntry = fmGetNextInterfaceAddress( (fm_intIpInterfaceAddressEntry*) *pSearchToken);

            if (pAddrEntry == NULL)
            {
                err = FM_ERR_NO_MORE;
            }
            else
            {
                *pNextAddr    = pAddrEntry->addr;
                *pSearchToken = (fm_voidptr) pAddrEntry;
            }
            fmReleaseReadLock(&switchPtr->routingLock);
        }
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterfaceAddrNext */




/*****************************************************************************/
/** fmFindInterface
 * \ingroup intNextHopIf
 *
 * \desc            Find an interface entry for a specified interface IP.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pInterfaceAddr points to the Interface IP Address.
 *
 * \param[out]      ppIfEntry point to a caller allocated storage where this
 *                  will copy a pointer to the ifEntry. May be NULL, in
 *                  which case no pointer will be returned by this function.
 *
 * \return          FM_OK if the interface entry was found.
 * \return          FM_ERR_INVALID_INTERFACE if the entry wasn't found.
 * \return          FM_ERR_INVALID_ARGUMENT if pInterfaceAddr is NULL.
 *
 *****************************************************************************/
fm_status fmFindInterface(fm_int                   sw,
                          fm_ipAddr               *pInterfaceAddr,
                          fm_intIpInterfaceEntry **ppIfEntry)
{
    fm_status                      err;
    fm_intIpInterfaceAddressEntry *pAddrEntry;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pInterfaceAddr=%p, ppIfEntry=%p\n",
                  sw,
                  (void *) pInterfaceAddr,
                  (void *) ppIfEntry );

    err = FM_OK;

    /* set NULL as returned pointer by default */
    if (ppIfEntry != NULL)
    {
        *ppIfEntry = NULL;
    }

    if (pInterfaceAddr == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        err = FindInterfaceAddrEntry(sw, pInterfaceAddr, &pAddrEntry);

        if (err == FM_OK && ppIfEntry != NULL && pAddrEntry != NULL)
        {
            *ppIfEntry = pAddrEntry->ifEntry;
        }
    }
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmFindInterface */




/*****************************************************************************/
/** fmGetInterface
 * \ingroup intNextHopIf
 *
 * \desc            Given an interface number, validate that it is active and
 *                  return the pointer to the interface's entry.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       interface is the interface number
 *
 * \param[out]      ppIfEntry contains the pointer to the interface entry,
 *                  or NULL if the entry wasn't found. May be NULL.
 *
 * \return          FM_OK if the interface entry was found.
 * \return          FM_ERR_INVALID_INTERFACE if the entry wasn't found.
 *
 *****************************************************************************/
fm_status fmGetInterface(fm_int                   sw,
                         fm_int                   interface,
                         fm_intIpInterfaceEntry **ppIfEntry)
{
    fm_switch *switchPtr;
    fm_bool    ifIsInUse;
    fm_status  err;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, if=%d, ppIfEntry=%p\n",
                  sw,
                  interface,
                  (void*)ppIfEntry );

    switchPtr = GET_SWITCH_PTR(sw);

    /* set NULL as returned pointer by default */
    if (ppIfEntry != NULL)
    {
        *ppIfEntry = NULL;
    }

    /* make sure the interface is in use */
    err = fmGetBitArrayBit(&switchPtr->ipInterfaceEntriesInUse,
                           interface,
                           &ifIsInUse);
    if (err == FM_OK)
    {
        if (!ifIsInUse)
        {
            err = FM_ERR_INVALID_INTERFACE;
        }
        else if (ppIfEntry != NULL)
        {
            *ppIfEntry = &switchPtr->ipInterfaceEntries[interface];
        }
    }
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetInterface */


