/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm_api_routing.c
 * Creation Date: February 7, 2007
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
#if 0
#define DEBUG_TRACK_MEMORY_USE
#endif


/*****************************************************************************
 * Global Variables
 *****************************************************************************/


/*****************************************************************************
 * Local Variables
 *****************************************************************************/


/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static fm_customTree *GetRouteTree(fm_int sw, fm_routeEntry *route);
static void FreeRoute(void *key, void *value);
static fm_int CompareRoutes(fm_routeEntry *first,
                            fm_routeEntry *second,
                            fm_bool        ecmpCheck);
static void SetVirtualRouterMac(fm_switch *switchPtr,
                                fm_int     vrMacId,
                                fm_macaddr macAddr);
static void DestroyRecord(void *key, void *data);


/*****************************************************************************
 * Local Functions
 *****************************************************************************/


/*****************************************************************************/
/** GetRouteTree
 * \ingroup intRouter
 *
 * \desc            Returns a pointer to the appropriate route tree to use
 *                  for searching for/editing a route, based upon the route
 *                  type.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route.
 *
 * \return          Pointer to the route tree, NULL if route type was invalid.
 *
 *****************************************************************************/
static fm_customTree *GetRouteTree(fm_int sw, fm_routeEntry *route)
{
    fm_switch *      switchPtr;
    fm_intRouteEntry key;
    fm_customTree *  routeTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, route = %p\n",
                 sw,
                 (void *) route);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Try to find the route */
    key.route = *route;

    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
        case FM_ROUTE_TYPE_UNICAST_ECMP:
            routeTree = &switchPtr->ecmpRouteTree;
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            routeTree = &switchPtr->routeTree;
            break;

        default:
            routeTree = NULL;
            break;
    }

    FM_LOG_EXIT_CUSTOM( FM_LOG_CAT_ROUTING,
                       routeTree,
                       "routeTree = %p\n",
                       (void *) routeTree );

}   /* end GetRouteTree */




/*****************************************************************************/
/** FreeRoute
 * \ingroup intRouter
 *
 * \desc            Releases a Route during switch initialization/shutdown.
 *
 * \param[in]       key points to the key.
 *
 * \param[in]       value points to the route.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static void FreeRoute(void *key, void *value)
{
    fm_intRouteEntry *routePtr = value;

    FM_NOT_USED(key);

    fmFree(routePtr);

}   /* end FreeRoute */




/*****************************************************************************/
/** CompareRoutes
 * \ingroup intRouter
 *
 * \desc            Compare Route entries, either full comparison or ECMP
 *                  check only.
 *
 * \param[in]       first points to the first route.
 *
 * \param[in]       second points to the second route.
 *
 * \param[in]       ecmpCheck is TRUE if the function should simply determine
 *                  if both routes are part of the same ECMP group, FALSE if
 *                  a full route comparison should be performed.
 *
 * \return          -1 if the first route sorts before the second.
 * \return           0 if the routes are identical.
 * \return           1 if the first route sorts after the second.
 *
 *****************************************************************************/
static fm_int CompareRoutes(fm_routeEntry *first,
                            fm_routeEntry *second,
                            fm_bool        ecmpCheck)
{
    fm_int                         i;
    fm_ipAddr *                    destAddr1;
    fm_ipAddr *                    destAddr2;
    fm_int                         destPrefix1;
    fm_int                         destPrefix2;
    fm_ipAddr                      zeroAddr;
    fm_ipAddr *                    nextHopAddr1;
    fm_ipAddr *                    nextHopAddr2;
    fm_int                         nextHopPrefix1;
    fm_int                         nextHopPrefix2;
    fm_uint16                      vlan1;
    fm_uint16                      vlan2;
    fm_uint16                      vlanPrefix1;
    fm_uint16                      vlanPrefix2;
    fm_ipAddr *                    ifAddr1;
    fm_ipAddr *                    ifAddr2;
    fm_int                         vrid1;
    fm_int                         vrid2;
    fm_multicastAddress *          multicast;
    fm_multicastDstIpRoute *       mcastDstIpRoute;
    fm_multicastDstIpVlanRoute *   mcastDstIpVlanRoute;
    fm_multicastDstSrcIpRoute *    mcastDstSrcRoute;
    fm_multicastDstSrcIpVlanRoute *mcastDstSrcVlanRoute;
    fm_int                         ecmpGroup1;
    fm_int                         ecmpGroup2;
    fm_routeType                   route1Type;
    fm_routeType                   route2Type;

    FM_CLEAR(zeroAddr);

    if (first->routeType == FM_ROUTE_TYPE_UNICAST)
    {
        destAddr1    = &first->data.unicast.dstAddr;
        destPrefix1  = first->data.unicast.prefixLength;
        nextHopAddr1 = &first->data.unicast.nextHop;
        ecmpGroup1   = 0;
        route1Type   = FM_ROUTE_TYPE_UNICAST;

        if (nextHopAddr1->isIPv6)
        {
            nextHopPrefix1 = FM_IPV6_MAX_PREFIX_LENGTH;
        }
        else
        {
            nextHopPrefix1 = FM_IPV4_MAX_PREFIX_LENGTH;
        }

        ifAddr1 = &first->data.unicast.interfaceAddr;

        if ( fmIsIPAddressEmpty(ifAddr1) )
        {
            vlan1 = first->data.unicast.vlan;
        }
        else
        {
            vlan1 = 0;
        }

        vlanPrefix1 = FM_VLAN_MAX_PREFIX_LENGTH;
        vrid1       = first->data.unicast.vrid;
    }
    else if (first->routeType == FM_ROUTE_TYPE_UNICAST_ECMP)
    {
        destAddr1      = &first->data.unicastECMP.dstAddr;
        destPrefix1    = first->data.unicastECMP.prefixLength;
        nextHopAddr1   = &zeroAddr;
        nextHopPrefix1 = 0;
        ecmpGroup1     = first->data.unicastECMP.ecmpGroup;
        ifAddr1        = &zeroAddr;
        vlan1          = 0;
        route1Type     = FM_ROUTE_TYPE_UNICAST;
        vlanPrefix1    = 0;
        vrid1          = first->data.unicastECMP.vrid;
    }
    else
    {
        multicast  = &first->data.multicast;
        ecmpGroup1 = 0;
        route1Type = FM_ROUTE_TYPE_MULTICAST;

        switch (multicast->addressType)
        {
            case FM_MCAST_ADDR_TYPE_DSTIP:
                mcastDstIpRoute = &multicast->info.dstIpRoute;
                destAddr1       = &mcastDstIpRoute->dstAddr;
                destPrefix1     = mcastDstIpRoute->dstPrefixLength;
                nextHopPrefix1  = 0;
                nextHopAddr1    = &zeroAddr;
                ifAddr1         = &zeroAddr;
                vlan1           = 0;
                vlanPrefix1     = 0;
                vrid1           = 0;
                break;

            case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
                mcastDstIpVlanRoute = &multicast->info.dstIpVlanRoute;
                destAddr1           = &mcastDstIpVlanRoute->dstAddr;
                destPrefix1         = mcastDstIpVlanRoute->dstPrefixLength;
                nextHopPrefix1      = 0;
                nextHopAddr1        = &zeroAddr;
                ifAddr1             = &zeroAddr;
                vlan1               = mcastDstIpVlanRoute->vlan;
                vlanPrefix1         = mcastDstIpVlanRoute->vlanPrefixLength;
                vrid1               = 0;
                break;

            case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
                mcastDstSrcRoute = &multicast->info.dstSrcIpRoute;
                destAddr1        = &mcastDstSrcRoute->dstAddr;
                destPrefix1      = mcastDstSrcRoute->dstPrefixLength;
                nextHopPrefix1   = mcastDstSrcRoute->srcPrefixLength;
                nextHopAddr1     = &mcastDstSrcRoute->srcAddr;
                ifAddr1          = &zeroAddr;
                vlan1            = 0;
                vlanPrefix1      = 0;
                vrid1            = 0;
                break;

            case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
                mcastDstSrcVlanRoute = &multicast->info.dstSrcIpVlanRoute;
                destAddr1            = &mcastDstSrcVlanRoute->dstAddr;
                destPrefix1          = mcastDstSrcVlanRoute->dstPrefixLength;
                nextHopPrefix1       = mcastDstSrcVlanRoute->srcPrefixLength;
                nextHopAddr1         = &mcastDstSrcVlanRoute->srcAddr;
                ifAddr1              = &zeroAddr;
                vlan1                = mcastDstSrcVlanRoute->vlan;
                vlanPrefix1          = mcastDstSrcVlanRoute->vlanPrefixLength;
                vrid1                = 0;
                break;

            default:
                return 1;

        }   /* end switch (multicast->addressType) */

    }

    if (second->routeType == FM_ROUTE_TYPE_UNICAST)
    {
        destAddr2    = &second->data.unicast.dstAddr;
        destPrefix2  = second->data.unicast.prefixLength;
        nextHopAddr2 = &second->data.unicast.nextHop;
        ecmpGroup2   = 0;
        route2Type   = FM_ROUTE_TYPE_UNICAST;

        if (nextHopAddr2->isIPv6)
        {
            nextHopPrefix2 = FM_IPV6_MAX_PREFIX_LENGTH;
        }
        else
        {
            nextHopPrefix2 = FM_IPV4_MAX_PREFIX_LENGTH;
        }

        ifAddr2 = &second->data.unicast.interfaceAddr;

        if ( fmIsIPAddressEmpty(ifAddr2) )
        {
            vlan2 = second->data.unicast.vlan;
        }
        else
        {
            vlan2 = 0;
        }

        vlanPrefix2 = FM_VLAN_MAX_PREFIX_LENGTH;
        vrid2       = second->data.unicast.vrid;
    }
    else if (second->routeType == FM_ROUTE_TYPE_UNICAST_ECMP)
    {
        destAddr2      = &second->data.unicastECMP.dstAddr;
        destPrefix2    = second->data.unicastECMP.prefixLength;
        nextHopAddr2   = &zeroAddr;
        nextHopPrefix2 = 0;
        ecmpGroup2     = second->data.unicastECMP.ecmpGroup;
        ifAddr2        = &zeroAddr;
        vlan2          = 0;
        route2Type     = FM_ROUTE_TYPE_UNICAST;
        vlanPrefix2    = 0;
        vrid2          = second->data.unicastECMP.vrid;
    }
    else
    {
        multicast  = &second->data.multicast;
        route2Type = FM_ROUTE_TYPE_MULTICAST;
        ecmpGroup2 = 0;

        switch (multicast->addressType)
        {
            case FM_MCAST_ADDR_TYPE_DSTIP:
                mcastDstIpRoute = &second->data.multicast.info.dstIpRoute;
                destAddr2       = &mcastDstIpRoute->dstAddr;
                destPrefix2     = mcastDstIpRoute->dstPrefixLength;
                nextHopPrefix2  = 0;
                nextHopAddr2    = &zeroAddr;
                ifAddr2         = &zeroAddr;
                vlan2           = 0;
                vlanPrefix2     = 0;
                vrid2           = 0;
                break;

            case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
                mcastDstIpVlanRoute = &second->data.multicast.info.dstIpVlanRoute;
                destAddr2           = &mcastDstIpVlanRoute->dstAddr;
                destPrefix2         = mcastDstIpVlanRoute->dstPrefixLength;
                nextHopPrefix2      = 0;
                nextHopAddr2        = &zeroAddr;
                ifAddr2             = &zeroAddr;
                vlan2               = mcastDstIpVlanRoute->vlan;
                vlanPrefix2         = mcastDstIpVlanRoute->vlanPrefixLength;
                vrid2               = 0;
                break;

            case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
                mcastDstSrcRoute = &second->data.multicast.info.dstSrcIpRoute;
                destAddr2        = &mcastDstSrcRoute->dstAddr;
                destPrefix2      = mcastDstSrcRoute->dstPrefixLength;
                nextHopPrefix2   = mcastDstSrcRoute->srcPrefixLength;
                nextHopAddr2     = &mcastDstSrcRoute->srcAddr;
                ifAddr2          = &zeroAddr;
                vlan2            = 0;
                vlanPrefix2      = 0;
                vrid2            = 0;
                break;

            case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
                mcastDstSrcVlanRoute =
                    &second->data.multicast.info.dstSrcIpVlanRoute;
                destAddr2      = &mcastDstSrcVlanRoute->dstAddr;
                destPrefix2    = mcastDstSrcVlanRoute->dstPrefixLength;
                nextHopPrefix2 = mcastDstSrcVlanRoute->srcPrefixLength;
                nextHopAddr2   = &mcastDstSrcVlanRoute->srcAddr;
                ifAddr2        = &zeroAddr;
                vlan2          = mcastDstSrcVlanRoute->vlan;
                vlanPrefix2    = mcastDstSrcVlanRoute->vlanPrefixLength;
                vrid2          = 0;
                break;

            default:
                return -1;

        }   /* end switch (multicast->addressType) */

    }

    /* Full Sort order is:
     *      VRID (low-to-high)
     *      dest address prefix length (high-to-low)
     *      next hop/source address prefix (high-to-low)
     *      vlan prefix length (high-to-low)
     *      route type (unicast and unicastECMP are identical)
     *      dest address (IP address order)
     *      ecmp Group id (low-to-high)
     *      next hop/source address (IP address order)
     *      vlan (low-to-high)
     *      interface IP Address (IP address order)
     * ECMP Sort order is:
     *      VRID (low-to-high)
     *      dest address prefix length (high-to-low)
     *      next hop/source address prefix (high-to-low)
     *      vlan prefix length (high-to-low)
     *      route type (unicast and unicastECMP are identical)
     *      dest address (IP address order)
     *      next hop/source address (IP address order) (multicast only)
     *      vlan (low-to-high) (multicast only)
     */

    if (vrid1 < vrid2)
    {
        return -1;
    }
    else if (vrid1 > vrid2)
    {
        return 1;
    }

    if (destPrefix1 > destPrefix2)
    {
        return -1;
    }
    else if (destPrefix1 < destPrefix2)
    {
        return 1;
    }

    if (nextHopPrefix1 > nextHopPrefix2)
    {
        return -1;
    }
    else if (nextHopPrefix1 < nextHopPrefix2)
    {
        return 1;
    }

    if (vlanPrefix1 > vlanPrefix2)
    {
        return -1;
    }
    else if (vlanPrefix1 < vlanPrefix2)
    {
        return 1;
    }

    if (route1Type < route2Type)
    {
        return -1;
    }
    else if (route1Type > route2Type)
    {
        return 1;
    }

    i = fmCompareIPAddresses(destAddr1, destAddr2);

    if (i < 0)
    {
        return -1;
    }
    else if (i > 0)
    {
        return 1;
    }

    if ( ecmpCheck && (route1Type == FM_ROUTE_TYPE_UNICAST) )
    {
        return 0;
    }

    if (!ecmpCheck)
    {
        if (ecmpGroup1 < ecmpGroup2)
        {
            return -1;
        }
        else if (ecmpGroup1 > ecmpGroup2)
        {
            return 1;
        }
    }

    i = fmCompareIPAddresses(nextHopAddr1, nextHopAddr2);

    if (i < 0)
    {
        return -1;
    }
    else if (i > 0)
    {
        return 1;
    }

    if (vlan1 < vlan2)
    {
        return -1;
    }
    else if (vlan1 > vlan2)
    {
        return 1;
    }

    if (ecmpCheck)
    {
        return 0;
    }

    i = fmCompareIPAddresses(ifAddr1, ifAddr2);

    if (i < 0)
    {
        return -1;
    }
    else if (i > 0)
    {
        return 1;
    }

    return 0;

}   /* end CompareRoutes */




/*****************************************************************************/
/** SetVirtualRouterMac
 * \ingroup intRouter
 *
 * \desc            Store MAC address of the virtual router.
 *
 * \param[in]       switchPtr is a pointer to state structure of the switch
 *                  on which to operate.
 *
 * \param[in]       vrMacId identifies which MAC address of the virtual router
 *                  to store.
 *
 * \param[in]       macAddr is the MAC address value.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void SetVirtualRouterMac(fm_switch *switchPtr,
                                fm_int     vrMacId,
                                fm_macaddr macAddr)
{
    switchPtr->virtualRouterMac[vrMacId] = macAddr;

    /* Setting FM_MAX_VIRTUAL_ROUTERS to 0xFFFF will allow
     * the user to control the bottom 16 bits of the
     * virtual MAC address instead of the bottom 8 bits.
     */ 
#if (FM_MAX_VIRTUAL_ROUTERS < 0xffff)
    /* ignore the bottom 8 bits */
    switchPtr->virtualRouterMac[vrMacId] &= ~FM_LITERAL_U64(0xff);
#else
    /* ignore the bottom 16 bits */
    switchPtr->virtualRouterMac[vrMacId] &= ~FM_LITERAL_U64(0xffff);
#endif

}   /* end SetVirtualRouterMac */




/*****************************************************************************/
/** DestroyRecord
 * \ingroup intRoute
 *
 * \desc            Destroys a record.  Typically called by fmCustomTreeDestroy.
 *
 * \param[in]       key points to the tree key record.
 *
 * \param[in]       data points to the tree data record.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
static void DestroyRecord(void *key, void *data)
{
    if (key != data)
    {
        fmFree(key);
    }

    fmFree(data);

}   /* end DestroyRecord */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/


/*****************************************************************************/
/** fmCompareIntRoutes
 * \ingroup intRouter
 *
 * \desc            Compare internal Route entries.
 *
 * \param[in]       first points to the first route.
 *
 * \param[in]       second points to the second route.
 *
 * \return          -1 if the first route sorts before the second.
 * \return           0 if the routes are identical.
 * \return           1 if the first route sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareIntRoutes(const void *first, const void *second)
{
    fm_intRouteEntry *firstRoute  = (fm_intRouteEntry *) first;
    fm_intRouteEntry *secondRoute = (fm_intRouteEntry *) second;
    fm_int            i;

    i = CompareRoutes(&firstRoute->route, &secondRoute->route, FALSE);

    return i;

}   /* end fmCompareIntRoutes */




/*****************************************************************************/
/** fmCompareIPAddrs
 * \ingroup intRouter
 *
 * \desc            Compare IP Addresses.
 *
 * \param[in]       first points to the first IP Address.
 *
 * \param[in]       second points to the second IP Address.
 *
 * \return          -1 if the first address sorts before the second.
 * \return           0 if the addresses are identical.
 * \return           1 if the first address sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareIPAddrs(const void *first, const void *second)
{
    fm_ipAddr *firstAddr  = (fm_ipAddr *) first;
    fm_ipAddr *secondAddr = (fm_ipAddr *) second;
    fm_int     i;

    i = fmCompareIPAddresses(firstAddr, secondAddr);

    return i;

}   /* end fmCompareIPAddrs */




/*****************************************************************************/
/** fmCompareEcmpIntRoutes
 * \ingroup intRouter
 *
 * \desc            Compare internal Route entries.
 *
 * \param[in]       first points to the first route.
 *
 * \param[in]       second points to the second route.
 *
 * \return          -1 if the first route sorts before the second.
 * \return           0 if the routes are identical.
 * \return           1 if the first route sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareEcmpIntRoutes(const void *first, const void *second)
{
    fm_intRouteEntry *firstRoute  = (fm_intRouteEntry *) first;
    fm_intRouteEntry *secondRoute = (fm_intRouteEntry *) second;
    fm_int            i;

    i = CompareRoutes(&firstRoute->route, &secondRoute->route, TRUE);

    return i;

}   /* end fmCompareEcmpIntRoutes */


/*****************************************************************************/
/** fmIsIPAddressEmpty
 * \ingroup intRouter
 *
 * \desc            See if an IP Address is composed entirely of zeros.
 *
 * \param[in]       addr points to the IP address to check.
 *
 * \return          TRUE if the address is empty.
 * \return          FALSE if the address is not empty.
 *
 *****************************************************************************/
fm_bool fmIsIPAddressEmpty(fm_ipAddr *addr)
{
    fm_bool isEmpty = TRUE;
    fm_int  i;

    if (addr->isIPv6)
    {
        isEmpty = FALSE;
    }
    else
    {
        for (i = 0 ; i < 4 ; i++)
        {
            if (addr->addr[i] != 0)
            {
                isEmpty = FALSE;
                break;
            }
        }
    }

    return isEmpty;

}   /* end fmIsIPAddressEmpty */



/*****************************************************************************/
/** fmGetRouteDestAddress
 * \ingroup intRouter
 *
 * \desc            Retrieves the destination address from a route entry.
 *
 * \param[in]       route points to the route entry.
 *
 * \param[in]       destAddr points to caller-allocated storage into which
 *                  the destination address is placed.
 *
 * \return          none.
 *
 *****************************************************************************/
void fmGetRouteDestAddress(fm_routeEntry *route, fm_ipAddr *destAddr)
{
    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            FM_MEMCPY_S( destAddr,
                         sizeof(*destAddr),
                         &route->data.unicast.dstAddr,
                         sizeof(fm_ipAddr) );
            break;

        case FM_ROUTE_TYPE_UNICAST_ECMP:
            FM_MEMCPY_S( destAddr,
                         sizeof(*destAddr),
                         &route->data.unicastECMP.dstAddr,
                         sizeof(fm_ipAddr) );
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            fmGetMcastDestAddress(&route->data.multicast, destAddr);
            break;

        default:
            FM_CLEAR(*destAddr);
            break;

    }   /* end switch (route->routeType) */

}   /* end fmGetRouteDestAddress */




/*****************************************************************************/
/** fmGetRouteMcastSourceAddress
 * \ingroup intRouter
 *
 * \desc            Retrieves the multicast source address from a route entry.
 *
 * \param[in]       route points to the route entry.
 *
 * \param[in]       srcAddr points to caller-allocated storage into which
 *                  the source address is placed.
 *
 * \return          none.
 *
 *****************************************************************************/
void fmGetRouteMcastSourceAddress(fm_routeEntry *route, fm_ipAddr *srcAddr)
{
    fm_multicastAddress *multicast;

    if (route->routeType != FM_ROUTE_TYPE_MULTICAST)
    {
        FM_CLEAR(*srcAddr);
        return;
    }

    multicast = &route->data.multicast;

    switch (multicast->addressType)
    {
        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
            FM_MEMCPY_S( srcAddr,
                         sizeof(*srcAddr),
                         &multicast->info.dstSrcIpRoute.srcAddr,
                         sizeof(fm_ipAddr) );
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
            FM_MEMCPY_S( srcAddr,
                         sizeof(*srcAddr),
                         &multicast->info.dstSrcIpVlanRoute.srcAddr,
                         sizeof(fm_ipAddr) );
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP:
        case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
        default:
            FM_CLEAR(*srcAddr);
            break;

    }   /* end switch (multicast->addressType) */

}   /* end fmGetRouteMcastSourceAddress */




/*****************************************************************************/
/** fmGetRouteLookupTree
 * \ingroup intRouter
 *
 * \desc            Returns the route lookup tree associated with a vrid
 *                  and prefix.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vrid is the virtual router ID number.
 *
 * \param[in]       prefix is the route mask prefix value.
 *
 * \param[out]      treePtrPtr points to caller-provided storage into which
 *                  the pointer to the custom tree will be written.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if route lookups are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if any of the arguments are invalid.
 *
 *****************************************************************************/
fm_status fmGetRouteLookupTree(fm_int          sw,
                               fm_int          vrid,
                               fm_int          prefix,
                               fm_customTree **treePtrPtr)
{
    fm_switch *switchPtr;
    fm_int     index;
    fm_int     vroff;
    fm_status  err = FM_OK;

    VALIDATE_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    if (switchPtr->routeLookupTrees == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_UNSUPPORTED);
    }

    if(treePtrPtr == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (vrid == FM_ROUTER_ANY)
    {
        /* Use maxVirtualRouters number as an index for vrid = FM_ROUTER_ANY */
        index = (switchPtr->maxVirtualRouters * FM_MAX_NUM_IP_PREFIXES) + prefix;
    }
    else
    {
        index = (vroff * FM_MAX_NUM_IP_PREFIXES) + prefix;
    }

    *treePtrPtr = &switchPtr->routeLookupTrees[index];

    return err;

}   /* end fmGetRouteLookupTree */




/*****************************************************************************/
/** fmGetIntRouteForIP
 * \ingroup intRouter
 *
 * \desc            Given an IP address, attempts to find the route in the
 *                  routing table that would be used to route frames to that
 *                  address.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vrid is the virtual router ID number.
 *
 * \param[in]       ip points to the IP address.
 *
 * \param[out]      routePtrPtr points to caller-provided storage into which
 *                  the pointer to the destination route will be written.
 *                  NULL will be written if a route could not be found.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_UNSUPPORTED if route lookups are not supported.
 * \return          FM_ERR_INVALID_ARGUMENT if ip or routePtrPtr are NULL.
 * \return          FM_ERR_NO_ROUTE_TO_HOST if no route was found for this IP.
 *
 *****************************************************************************/
fm_status fmGetIntRouteForIP(fm_int             sw,
                             fm_int             vrid,
                             fm_ipAddr *        ip,
                             fm_intRouteEntry **routePtrPtr)
{
    fm_switch *       switchPtr;
    fm_int            prefixLength;
    fm_customTree *   routeLookupTree;
    fm_intRouteEntry *route;
    fm_status         status;
    fm_ipAddr         maskedIP;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, vrid = %d, ip = %p, routePtrPtr = %p\n",
                  sw,
                  vrid,
                  (void *) ip,
                  (void *) routePtrPtr );

    VALIDATE_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->routeLookupTrees == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_UNSUPPORTED);
    }

    if ( (ip == NULL) || (routePtrPtr == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    if (ip->isIPv6)
    {
        prefixLength = FM_IPV6_MAX_PREFIX_LENGTH;
    }
    else
    {
        prefixLength = FM_IPV4_MAX_PREFIX_LENGTH;
    }

    while (prefixLength >= 0)
    {
        maskedIP = *ip;
        fmMaskIPAddress(&maskedIP, prefixLength);

        status = fmGetRouteLookupTree(sw, vrid, prefixLength, &routeLookupTree);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

        status = fmCustomTreeFind( routeLookupTree,
                                   &maskedIP,
                                   (void **) &route );
        if (status == FM_OK)
        {
            *routePtrPtr = route;
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
        }

        --prefixLength;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_NO_ROUTE_TO_HOST);

}   /* end fmGetIntRouteForIP */




/*****************************************************************************/
/** fmValidateVirtualRouterId
 * \ingroup intRouter
 *
 * \desc            Validates a virtual router id.  If the virtual router
 *                  id is not in the table and there is room in the
 *                  table for another virtual router, the vrid is stored
 *                  into the table and the needed hardware resources are
 *                  initialized.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vrid is the virtual router id number.
 *
 * \param[out]      vroffPtr contains the offset of the virtual route in
 *                  the virtual router ID table.  If the virtual router ID
 *                  is not found, and a virtual router slot is available,
 *                  the offset of the available slot will be provided and
 *                  the function will return FM_ERR_NOT_FOUND.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmValidateVirtualRouterId(fm_int  sw,
                                    fm_int  vrid,
                                    fm_int *vroffPtr)
{
    fm_switch *switchPtr;
    fm_int     availvroff;
    fm_int     vroff;
    fm_status  err = FM_OK;

    switchPtr = GET_SWITCH_PTR(sw);

    vroff = -1;

    if ( ((switchPtr->virtualRouterIds == NULL)
        || (vrid < 0)
        || (vrid >= FM_MAX_VIRTUAL_ROUTERS))
        && (vrid != FM_ROUTER_ANY ) )
    {
        err = FM_ERR_INVALID_VRID;
    }

    else if ( (vrid == 0) || (vrid == FM_ROUTER_ANY) )
    {
        vroff = 0;
    }

    else
    {
        availvroff = -1;

        for (vroff = 1 ; vroff < switchPtr->maxVirtualRouters ; vroff++)
        {
            if (switchPtr->virtualRouterIds[vroff] == vrid)
            {
                break;
            }
            else if ( (availvroff < 0)
                     && (switchPtr->virtualRouterIds[vroff] < 0) )
            {
                availvroff = vroff;
            }
        }

        if (vroff >= switchPtr->maxVirtualRouters)
        {
            if (availvroff < 0)
            {
                err = FM_ERR_TOO_MANY_VIRTUAL_ROUTERS;
            }
            else
            {
                vroff = availvroff;
                err   = FM_ERR_NOT_FOUND;
            }
        }
    }

    if (vroffPtr != NULL)
    {
        *vroffPtr = vroff;
    }

    return err;

}   /* end fmValidateVirtualRouterId */




/*****************************************************************************/
/** fmRouterAlloc
 * \ingroup intRouter
 *
 * \desc            Allocate resources needed by the routing system for a
 *                  switch.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRouterAlloc(fm_int sw)
{
    fm_switch *switchPtr;
    fm_int     tsize;
    fm_status  err;
    fm_bool    supportRouteLookups;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* Initialize all pointers */
    switchPtr->virtualRouterStates   = NULL;
    switchPtr->virtualRouterMacModes = NULL;
    switchPtr->virtualRouterIds      = NULL;
    switchPtr->routeLookupTrees      = NULL;

    /* If routing is not supported, exit */
    // marka: check for a more uniform / general validation
    // marka: add a general switch variable to indicate if the feature is supported or not.
    if (switchPtr->RouterInit != NULL)
    {
        /**************************************************
         * Allocate Virtual Router Tables
         **************************************************/
        if (switchPtr->maxVirtualRouters > 0)
        {
            tsize = sizeof(fm_routerState) * switchPtr->maxVirtualRouters;
            switchPtr->virtualRouterStates = (fm_routerState *) fmAlloc(tsize);

            if (switchPtr->virtualRouterStates != NULL)
            {
                FM_MEMSET_S(switchPtr->virtualRouterStates, tsize, 0, tsize);
            }
            else
            {
                err = FM_ERR_NO_MEM;
            }

            if (err == FM_OK)
            {
                tsize = sizeof(fm_routerMacMode) * switchPtr->maxVirtualRouters;
                switchPtr->virtualRouterMacModes = (fm_routerMacMode *) fmAlloc(tsize);
                if (switchPtr->virtualRouterMacModes != NULL)
                {
                    FM_MEMSET_S(switchPtr->virtualRouterMacModes, tsize, 0, tsize);
                }
                else
                {
                    err = FM_ERR_NO_MEM;
                }
            }

            if (err == FM_OK)
            {
                tsize = sizeof(fm_int) * switchPtr->maxVirtualRouters;
                switchPtr->virtualRouterIds = (fm_int *) fmAlloc(tsize);
                if (switchPtr->virtualRouterIds != NULL)
                {
                    FM_MEMSET_S(switchPtr->virtualRouterIds, tsize, 0, tsize);
                }
                else
                {
                    err = FM_ERR_NO_MEM;
                }
            }

            if (err == FM_OK)
            {
                /************************************************************************
                 * If Route Lookups must be supported, allocate route lookup trees.
                 * This must be done during switch initialization since the application
                 * may not have an opportunity to configure the API attribute prior to
                 * switch insertion.
                 ************************************************************************/
                supportRouteLookups = fmGetBoolApiProperty(
                        FM_AAK_API_SUPPORT_ROUTE_LOOKUPS,
                        FM_AAD_API_SUPPORT_ROUTE_LOOKUPS);
                if (supportRouteLookups)
                {
                    /* routeLookupTrees points towards a 2 dim array of pointers to trees */
                    /* Additional virtual number is for vrid = FM_ROUTER_ANY */
                    tsize = sizeof(fm_customTree) * (switchPtr->maxVirtualRouters + 1)
                            * FM_MAX_NUM_IP_PREFIXES;
                    switchPtr->routeLookupTrees = fmAlloc(tsize);
                    if (switchPtr->routeLookupTrees != NULL)
                    {
                        FM_MEMSET_S(switchPtr->routeLookupTrees, tsize, 0, tsize);
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
            fmRouterFree(sw);
        }
    }
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmRouterAlloc */



/*****************************************************************************/
/** fmRouterFree
 * \ingroup intRouter
 *
 * \desc            Release all routing resources held by a switch.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRouterFree(fm_int sw)
{
    fm_switch *      switchPtr;
    fm_status        err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    /**************************************************
     * Destroy route tables
     **************************************************/
    fmCustomTreeDestroy(&switchPtr->routeTree, DestroyRecord);
    fmCustomTreeDestroy(&switchPtr->ecmpRouteTree, NULL);

    /**************************************************
     * Deallocate Virtual Router Tables
     **************************************************/
    if (switchPtr->virtualRouterStates != NULL)
    {
        fmFree(switchPtr->virtualRouterStates);
        switchPtr->virtualRouterStates = NULL;
    }

    if (switchPtr->virtualRouterMacModes != NULL)
    {
        fmFree(switchPtr->virtualRouterMacModes);
        switchPtr->virtualRouterMacModes = NULL;
    }

    if (switchPtr->virtualRouterIds != NULL)
    {
        fmFree(switchPtr->virtualRouterIds);
        switchPtr->virtualRouterIds = NULL;
    }

    if (switchPtr->routeLookupTrees != NULL)
    {
        fmFree(switchPtr->routeLookupTrees);
        switchPtr->routeLookupTrees = NULL;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmRouterFree */


/*****************************************************************************/
/** fmRouterInit
 * \ingroup intRouter
 *
 * \desc            Perform initialization for routing subsystem, called at
 *                  switch initialization time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRouterInit(fm_int sw)
{
    fm_switch     *switchPtr;
    fm_status      err;
    fm_int         index1;
    fm_int         index2;
    fm_customTree *routeLookupTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /* If routing is not supported, exit */
    if (switchPtr->RouterInit == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    /**************************************************
     * Init Route Table
     **************************************************/
    if (switchPtr->maxRoutes > 0)
    {
        fmCustomTreeInit(&switchPtr->routeTree, fmCompareIntRoutes);
        fmCustomTreeInit(&switchPtr->ecmpRouteTree, fmCompareEcmpIntRoutes);
    }

    /**************************************************
     * Init Virtual Router Tables
     **************************************************/
    if (switchPtr->maxVirtualRouters > 0)
    {
        switchPtr->virtualRouterStates[0]   = FM_ROUTER_STATE_ADMIN_UP;
        switchPtr->virtualRouterMacModes[0] = FM_ROUTER_MAC_MODE_PHYSICAL_MAC_ADDRESS;
        switchPtr->virtualRouterIds[0]      = 0;

        for (index1 = 1 ; index1 < switchPtr->maxVirtualRouters ; index1++)
        {
            switchPtr->virtualRouterStates[index1]   = FM_ROUTER_STATE_ADMIN_DOWN;
            switchPtr->virtualRouterMacModes[index1] = FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1;
            switchPtr->virtualRouterIds[index1]      = -1;
        }
    }

    /**************************************************
     * Init Physical and Virtual Routers Macs
     **************************************************/
    switchPtr->physicalRouterMac   = 0;
    switchPtr->virtualRouterMac[0] = 0;
    switchPtr->virtualRouterMac[1] = 0;

    /**************************************************
     * Init Route Lookups trees
     **************************************************/

    /* If Route Lookups are supported, initialize route lookup trees  */
    if (switchPtr->routeLookupTrees != NULL)
    {
        routeLookupTree = switchPtr->routeLookupTrees;
        /************************************************************* 
         * switchPtr->routeLookupTrees points towards a 2-array, so
         * it is indexed by vrid and prefix. 
         *  index1 -> vrid;
         *  index2 -> prefix;
         *************************************************************/
        for (index1 = 0 ; index1 < (switchPtr->maxVirtualRouters + 1) ; index1++)
        {
            for (index2 = 0 ; index2 < FM_MAX_NUM_IP_PREFIXES ; index2++)
            {
                fmCustomTreeInit(routeLookupTree++, fmCompareIPAddrs);
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmRouterInit */


/*****************************************************************************/
/** fmRouterCleanup
 * \ingroup intRouter
 *
 * \desc            Releases memory used by the routing subsystem to support
 *                  a specified switch.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmRouterCleanup(fm_int sw)
{
    fm_switch            *switchPtr;
    fm_status             err;
    fm_customTreeIterator iter;
    fm_intRouteEntry     *key;
    fm_intRouteEntry     *route;
    fm_int                vrid;
    fm_int                prefix;
    fm_customTree        *routeLookupTree;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    switchPtr = GET_SWITCH_PTR(sw);
    err = FM_OK;

    /***************************************************
     * Destroy route lookup trees.
     ***************************************************/
    // marka document/verify this: route LookupTrees points to an array of pointers
    // to trees. check for an alternative solution
    if (switchPtr->routeLookupTrees != NULL)
    {
        routeLookupTree = switchPtr->routeLookupTrees;
        for (vrid = 0 ; vrid < (switchPtr->maxVirtualRouters + 1) ; vrid++)
        {
            for (prefix = 0 ; prefix < FM_MAX_NUM_IP_PREFIXES ; prefix++)
            {
                if (fmCustomTreeIsInitialized(routeLookupTree))
                {
                    fmCustomTreeDestroy(routeLookupTree, NULL);
                }
                routeLookupTree++;
            }
        }
    }

    /**************************************************
     * Remove all routes from the route trees
     **************************************************/
    if ( fmCustomTreeIsInitialized(&switchPtr->routeTree) )
    {
        while (err == FM_OK)
        {
            fmCustomTreeIterInit(&iter, 
                                 &switchPtr->routeTree);

            err = fmCustomTreeIterNext(&iter,
                                       (void **) &key,
                                       (void **) &route);
            if (err == FM_OK)
            {
                err = fmCustomTreeRemoveCertain(&switchPtr->routeTree, 
                                                key, 
                                                FreeRoute);
            }
        }

        if (err != FM_OK && err != FM_ERR_NO_MORE)
        {
            /* report the error. This could cause a memory leak */
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "while cleaning up routeTree\n");
        }
        /* clear the error to continue */
        err = FM_OK;
    }

    if ( fmCustomTreeIsInitialized(&switchPtr->ecmpRouteTree) )
    {
        while (err == FM_OK)
        {
            fmCustomTreeIterInit(&iter, &switchPtr->ecmpRouteTree);

            err = fmCustomTreeIterNext(&iter, (void **) &key, (void **) &route);

            if (err == FM_OK)
            {
                err = fmCustomTreeRemoveCertain(&switchPtr->ecmpRouteTree,
                                                key,
                                                NULL);
            }
        }
        if (err != FM_OK && err != FM_ERR_NO_MORE)
        {
            /* report the error. This could cause a memory leak */
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, 
                         "while cleaning up ecmpRouteTree\n");
        }
        /* clear the error to continue */
        err = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fmRouterCleanup */




/*****************************************************************************/
/** fmSetRouterAttribute
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Set a router attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the router attribute to set (see
 *                  ''Router Attributes'').
 *
 * \param[in]       value points to the attribute value to set
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmSetRouterAttribute(fm_int sw,
                               fm_int attr,
                               void * value)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, attr = %d\n",
                     sw,
                     attr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->SetRouterAttribute != NULL)
    {
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            switch (attr)
            {
                case FM_ROUTER_TRAP_TTL1:
                    switchPtr->routerTrapTTL1 = *( (fm_int *) value );
                    break;

                case FM_ROUTER_TRAP_REDIRECT_EVENT:
                    switchPtr->routerTrapRedirectEvent = *( (fm_bool *) value );
                    break;

                case FM_ROUTER_PHYSICAL_MAC_ADDRESS:
                    switchPtr->physicalRouterMac = *( (fm_macaddr *) value );
                    break;

                case FM_ROUTER_VIRTUAL_MAC_ADDRESS:
                    SetVirtualRouterMac(switchPtr, 0, *( (fm_macaddr *) value ));
                    break;

                case FM_ROUTER_VIRTUAL_MAC_ADDRESS_2:
                    SetVirtualRouterMac(switchPtr, 1, *( (fm_macaddr *) value ));
                    break;

                case FM_ROUTER_TRAP_IP_OPTIONS:
                    switchPtr->routerTrapIpOptions = *( (fm_bool *) value );
                    break;

                default:
                    err = FM_ERR_INVALID_ATTRIB;
                    break;

            }   /* end switch (attr) */

            /* Update the attribute in the hardware */
            if (err == FM_OK)
            {
                err = switchPtr->SetRouterAttribute(sw, attr, value);
            }

            fmReleaseWriteLock(&switchPtr->routingLock);
        }

    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetRouterAttribute */




/*****************************************************************************/
/** fmGetRouterAttribute
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Get the current value of a router attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the router attribute to set (see
 *                  ''Router Attributes'').
 *
 * \param[out]      value points to a caller-allocated storage where
 *                  this function will place the value of the attribute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmGetRouterAttribute(fm_int sw,
                               fm_int attr,
                               void * value)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, attr = %d\n",
                     sw,
                     attr);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* only allowed if chip supports routing: check SetRouterAttribute */
    if (switchPtr->SetRouterAttribute != NULL)
    {
        err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        if (err == FM_OK)
        {
            switch (attr)
            {
                case FM_ROUTER_TRAP_TTL1:
                    *( (fm_int *) value ) = switchPtr->routerTrapTTL1;
                    break;

                case FM_ROUTER_TRAP_REDIRECT_EVENT:
                    *( (fm_bool *) value ) = switchPtr->routerTrapRedirectEvent;
                    break;

                case FM_ROUTER_PHYSICAL_MAC_ADDRESS:
                    *( (fm_macaddr *) value ) = switchPtr->physicalRouterMac;
                    break;

                case FM_ROUTER_VIRTUAL_MAC_ADDRESS:
                    *( (fm_macaddr *) value ) = switchPtr->virtualRouterMac[0];
                    break;

                case FM_ROUTER_VIRTUAL_MAC_ADDRESS_2:
                    *( (fm_macaddr *) value ) = switchPtr->virtualRouterMac[1];
                    break;

                case FM_ROUTER_TRAP_IP_OPTIONS:
                    *( (fm_bool *) value ) = switchPtr->routerTrapIpOptions;
                    break;

                default:
                    err = FM_ERR_INVALID_ATTRIB;
                    break;

            }   /* end switch (attr) */

            fmReleaseReadLock(&switchPtr->routingLock);
        }

    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouterAttribute */




/*****************************************************************************/
/** fmCreateVirtualRouter
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Create a virtual router.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the ID of the virtual router to create
 *                  (1 to ''FM_MAX_VIRTUAL_ROUTERS'' - 1).
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VRID if vrid is invalid.
 * \return          FM_ERR_VRID_ALREADY_IN_USE if the vrid is already in use.
 * \return          FM_ERR_TOO_MANY_VIRTUAL_ROUTERS if all available hardware
 *                  virtual router slots have been used.
 *
 *****************************************************************************/
fm_status fmCreateVirtualRouter(fm_int sw,
                                fm_int vrid)
{
    fm_switch  *switchPtr;
    fm_status   err = FM_OK;
    fm_int      vroff;
    fm_bool     lockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, vrid = %d\n",
                     sw,
                     vrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    /* Validate input vrid */
    VALIDATE_VIRTUAL_ROUTER_ID(sw, vrid);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    if (err == FM_OK)
    {
        err = FM_ERR_VRID_ALREADY_IN_USE;
    }
    else if (err == FM_ERR_NOT_FOUND)
    {
        switchPtr->virtualRouterIds[vroff] = vrid;

        if (switchPtr->AddVirtualRouter != NULL)
        {
            err = switchPtr->AddVirtualRouter(sw, vroff);

            if (err != FM_OK)
            {
                switchPtr->virtualRouterIds[vroff] = -1;
            }
        }
        else
        {
            err = FM_OK;
        }
    }

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmCreateVirtualRouter */




/*****************************************************************************/
/** fmDeleteVirtualRouter
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Delete a virtual router.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the ID of the virtual router to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VRID if vrid is invalid.
 *
 *****************************************************************************/
fm_status fmDeleteVirtualRouter(fm_int sw,
                                fm_int vrid)
{
    fm_switch *             switchPtr;
    fm_status               err = FM_OK;
    fm_int                  vroff;
    fm_intRouteEntry *      curRoute;
    fm_intRouteEntry *      nextRoute;
    fm_customTreeIterator   iter;
    fm_intRouteEntry *      key;
    fm_bool                 lockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, vrid = %d\n",
                     sw,
                     vrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    if (vrid == 0)
    {
        err = FM_ERR_PHYS_ROUTER_NOT_DELETABLE;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    fmCustomTreeIterInit(&iter, &switchPtr->routeTree);

    /* Delete all routes for this virtual router */
    while (1)
    {
        err = fmCustomTreeIterNext( &iter,
                                   (void **) &key,
                                   (void **) &curRoute );

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        if (curRoute == NULL)
        {
            break;
        }

        err = fmCustomTreeIterNext( &iter,
                                   (void **) &key,
                                   (void **) &nextRoute );

        if (err == FM_ERR_NO_MORE)
        {
            nextRoute = NULL;
            err       = FM_OK;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        if ( fmIsRouteEntryUnicast(&curRoute->route) )
        {
            if (curRoute->route.data.unicast.vrid == vrid)
            {
                err = fmDeleteRouteInternal(sw, &curRoute->route);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                if (nextRoute != NULL)
                {
                    err = fmCustomTreeIterInitFromKey( &iter,
                                                      &switchPtr->routeTree,
                                                      nextRoute );

                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }
            }
        }

        if (nextRoute == NULL)
        {
            break;
        }
    }

    if (switchPtr->RemoveVirtualRouter != NULL)
    {
        err = switchPtr->RemoveVirtualRouter(sw, vroff);
    }

    if (err == FM_OK)
    {
        switchPtr->virtualRouterIds[vroff]      = -1;
        switchPtr->virtualRouterStates[vroff]   = FM_ROUTER_STATE_ADMIN_DOWN;
        switchPtr->virtualRouterMacModes[vroff] = FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteVirtualRouter */


/*****************************************************************************/
/** fmGetVirtualRouterList
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve a list of all virtual router IDs created on a
 *                  switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numVrids points to caller allocated storage where this
 *                  function should place the number of virtual router IDs
 *                  returned in vridList.
 *
 * \param[out]      vridList is an array that this function will fill with the
 *                  list of virtual router IDs.
 *
 * \param[in]       max is the size of vridList, being the maximum number of
 *                  virtual router IDs that vridList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of virtual router IDs.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support routing.
 *
 *****************************************************************************/
fm_status fmGetVirtualRouterList(fm_int  sw,
                                 fm_int *numVrids,
                                 fm_int *vridList,
                                 fm_int  max)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     offset;
    fm_int     vrid;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, numVrids = %p, vridList = %p, max = %d\n",
                     sw,
                     (void *) numVrids,
                     (void *) vridList,
                     max);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->virtualRouterIds != NULL)
    {
        *numVrids = 0;
        status    = FM_OK;

        for (offset = 1 ; offset < switchPtr->maxVirtualRouters ; offset++)
        {
            vrid = switchPtr->virtualRouterIds[offset];

            if (vrid != -1)
            {
                if (*numVrids >= max)
                {
                    status = FM_ERR_BUFFER_FULL;
                    break;
                }

                vridList[*numVrids] = vrid;
                (*numVrids)++;
            }
        }
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);
}


/*****************************************************************************/
/** fmGetVirtualRouterFirst
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first virtual router ID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      firstVrid points to caller allocated storage where this
 *                  function will store the first virtual router ID. Will be
 *                  set to -1 if no virtual routers have been created.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no virtual routers created.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support routing.
 *
 *****************************************************************************/
fm_status fmGetVirtualRouterFirst(fm_int sw, fm_int *firstVrid)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     offset;
    fm_int     vrid;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, firstVrid = %p\n",
                     sw,
                     (void *) firstVrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->virtualRouterIds != NULL)
    {
        for (offset = 1 ; offset < switchPtr->maxVirtualRouters ; offset++)
        {
            vrid = switchPtr->virtualRouterIds[offset];

            if (vrid != -1)
            {
                *firstVrid = vrid;
                status     = FM_OK;

                break;
            }
        }

        if (offset >= switchPtr->maxVirtualRouters)
        {
            *firstVrid = -1;
            status     = FM_ERR_NO_MORE;
        }
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetVirtualRouterFirst */


/*****************************************************************************/
/** fmGetVirtualRouterNext
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next virtual router ID.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       currentVrid is the last virtual router ID found by a
 *                  previous call to this function or to
 *                  ''fmGetVirtualRouterFirst''.
 *
 * \param[out]      nextVrid points to caller allocated storage where this
 *                  function will store the next virtual router ID. Will be set
 *                  to -1 if no more virtual router have been created.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no more virtual router IDs.
 * \return          FM_ERR_UNSUPPORTED if the switch does not support routing.
 *
 *****************************************************************************/
fm_status fmGetVirtualRouterNext(fm_int  sw,
                                 fm_int  currentVrid,
                                 fm_int *nextVrid)
{
    fm_switch *switchPtr;
    fm_status  status;
    fm_int     offset;
    fm_int     vrid;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, currentVrid = %d, nextVrid = %p\n",
                     sw,
                     currentVrid,
                     (void *) nextVrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->virtualRouterIds != NULL)
    {
        status = fmValidateVirtualRouterId(sw, currentVrid, &offset);

        if (status == FM_OK)
        {
            for (offset++ ; offset < switchPtr->maxVirtualRouters ; offset++)
            {
                vrid = switchPtr->virtualRouterIds[offset];

                if (vrid != -1)
                {
                    *nextVrid = vrid;
                    break;
                }
            }

            if (offset >= switchPtr->maxVirtualRouters)
            {
                *nextVrid = -1;
                status    = FM_ERR_NO_MORE;
            }
        }
    }
    else
    {
        status = FM_ERR_UNSUPPORTED;
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);
}


/*****************************************************************************/
/** fmSetRouterState
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Set the state for a virtual router. If the
 *                  router state is set to DOWN, then all routes
 *                  using that router will immediately cease to be active. If
 *                  the router state is set to UP, then routes using that router
 *                  will become active only if their interfaces, VLANs and the
 *                  routes themselves are active.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the virtual router ID  on which to operate
 *                  (0 for the real router).
 *
 * \param[in]       state is the desired state of the router.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VRID if vrid is invalid.
 *
 *****************************************************************************/
fm_status fmSetRouterState(fm_int         sw,
                           fm_int         vrid,
                           fm_routerState state)
{
    fm_switch *           switchPtr = NULL; /* Avoid compiler warning */
    fm_status             err;
    fm_intRouteEntry *    curRoute;
    fm_bool               setRoute;
    fm_int                vroff;
    fm_bool               lockTaken = FALSE;
    fm_customTreeIterator iter;
    fm_intRouteEntry *    key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, vrid = %d, state = %d\n",
                     sw,
                     vrid,
                     state);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (switchPtr->SetRouteActive == NULL) ||
         (switchPtr->SetRouterState == NULL) )
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    if (switchPtr->virtualRouterStates[vroff] == state)
    {
        goto ABORT;
    }

    err = switchPtr->SetRouterState(sw, vroff, state);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    switchPtr->virtualRouterStates[vroff] = state;

    /*
     *  Update active status for each route
     *  in the virtual router
     */
    fmCustomTreeIterInit(&iter, &switchPtr->routeTree);

    err = fmCustomTreeIterNext( &iter, (void **) &key, (void **) &curRoute );

    if (err == FM_ERR_NO_MORE)
    {
        curRoute = NULL;
        err      = FM_OK;
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    while (curRoute != NULL)
    {
        setRoute = FALSE;

        if ( fmIsRouteEntryUnicast(&curRoute->route) )
        {
            if (curRoute->route.data.unicast.vrid == vrid)
            {
                setRoute = TRUE;
            }
        }
        else if (vrid == 0)
        {
            /* all multicast routes use virtual router 0 */
            setRoute = TRUE;
        }

        if (setRoute)
        {
            err = fmSetRouteActiveFlag(sw, curRoute, TRUE);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }

        err = fmCustomTreeIterNext( &iter,
                                   (void **) &key,
                                   (void **) &curRoute );

        if (err == FM_ERR_NO_MORE)
        {
            curRoute = NULL;
            err      = FM_OK;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetRouterState */




/*****************************************************************************/
/** fmGetRouterState
 * \ingroup routerBase
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the current state for a virtual router.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the virtual router ID  on which to operate
 *                  (0 for the real router).
 *
 * \param[out]      state points to caller-allocated storage where this
 *                  function will place the current router state.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VRID if vrid is invalid.
 *
 *****************************************************************************/
fm_status fmGetRouterState(fm_int          sw,
                           fm_int          vrid,
                           fm_routerState *state)
{
    fm_switch *switchPtr;
    fm_int     vroff;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, vrid = %d\n",
                     sw,
                     vrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    if (err == FM_OK)
    {
        switchPtr = GET_SWITCH_PTR(sw);

        *state = switchPtr->virtualRouterStates[vroff];
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouterState */




/*****************************************************************************/
/** fmSetRouterMacMode
 * \ingroup routerBase
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Set the MAC mode for a virtual router to configure which
 *                  of the two virtual router MAC addresses it is going to use.
 *                  If the router MAC mode is set to VIRTUAL_MAC_ADDRESS_1,
 *                  then the router will use the first (default) virtual router
 *                  MAC address. If the router MAC mode is set to
 *                  VIRTUAL_MAC_ADDRESS_2, then the router will use the second
 *                  virtual router MAC address. For FM6000, if the router MAC
 *                  mode is set to VIRTUAL_MAC_ADDRESS_1_AND_2, then the router
 *                  will use both virtual router MAC addresses.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the virtual router ID  on which to operate
 *                  (0 for the real router).
 *
 * \param[in]       mode is the desired MAC mode of the router.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VRID if vrid is invalid.
 *
 *****************************************************************************/
fm_status fmSetRouterMacMode(fm_int           sw,
                             fm_int           vrid,
                             fm_routerMacMode mode)
{
    fm_switch *switchPtr = NULL; /* Avoid compiler warning */
    fm_status  err;
    fm_int     vroff;
    fm_bool    lockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, vrid = %d, mode = %d\n",
                     sw,
                     vrid,
                     mode);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->SetRouterMacMode == NULL)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    if (switchPtr->virtualRouterMacModes[vroff] == mode)
    {
        goto ABORT;
    }

    err = switchPtr->SetRouterMacMode(sw, vroff, mode);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    switchPtr->virtualRouterMacModes[vroff] = mode;

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetRouterMacMode */




/*****************************************************************************/
/** fmGetRouterMacMode
 * \ingroup routerBase
 *
 * \chips           FM6000, FM10000
 *
 * \desc            Retrieve the current MAC mode for a virtual router.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the virtual router ID  on which to operate
 *                  (0 for the real router).
 *
 * \param[out]      mode points to caller-supplied storage where this
 *                  function will place the current router MAC mode.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_VRID if vrid is invalid.
 *
 *****************************************************************************/
fm_status fmGetRouterMacMode(fm_int            sw,
                             fm_int            vrid,
                             fm_routerMacMode *mode)
{
    fm_switch *switchPtr;
    fm_int     vroff;
    fm_status  err;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, vrid = %d\n",
                     sw,
                     vrid);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    if (err == FM_OK)
    {
        switchPtr = GET_SWITCH_PTR(sw);

        *mode = switchPtr->virtualRouterMacModes[vroff];
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouterMacMode */




/*****************************************************************************/
/** fmAddRouteInternal
 * \ingroup intRoute
 *
 * \desc            Internal function to add a route.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route to add.
 *
 * \param[in]       state is the desired state of this route when the route
 *                  is created.
 *
 * \param[in]       action points to the desired routing action for this route.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if action parameter is NULL.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_ALREADY_EXISTS if the route already exists.
 * \return          FM_ERR_TABLE_FULL if the routing table is full.
 *
 *****************************************************************************/
fm_status fmAddRouteInternal(fm_int          sw,
                             fm_routeEntry * route,
                             fm_routeState   state,
                             fm_routeAction *action)
{
    fm_switch *               switchPtr;
    fm_status                 err;
    fm_intRouteEntry *        routeEntry;
    fm_intRouteEntry *        curRoute;
    fm_intIpInterfaceEntry *  ifEntry;
    fm_bool                   routeAllocated;
    fm_bool                   routeAddedToRouteTree;
    fm_bool                   routeAddedToAlternateTree;
    fm_bool                   routeAddedToLookupTree;
    fm_bool                   routeAddedToHardware;
    fm_bool                   ecmpGroupCreated;
    fm_bool                   routeAddedToEcmpGroup;
    fm_intMulticastGroup *    group;
    fm_int                    vrid;
    fm_int                    vroff;
    fm_int                    prefixLength;
    fm_unicastRouteEntry *    unicast;
    fm_unicastECMPRouteEntry *unicastEcmp;
    fm_multicastAddress *     multicast;
    fm_intRouteEntry          key;
    fm_customTree *           routeTree;
    fm_ecmpNextHop            nextHop;
    fm_intEcmpGroup *         ecmpGroup;
    fm_bool                   needEcmpGroup;
    fm_nextHop *              arpNextHop;
    fm_int                    routePrefixLength;
    fm_customTree *           routeLookupTree;
    fm_routeState             routeState;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, route=%p, state=%d, action=%p(%d)\n",
                 sw,
                 (void *) route,
                 state,
                 (void *) action,
                 (action != NULL) ? action->action : FM_ROUTE_ACTION_MAX);

    if ( (route == NULL) || (action == NULL) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    FM_CLEAR( nextHop );

    nextHop.type              = FM_NEXTHOP_TYPE_ARP;
    arpNextHop                = &nextHop.data.arp;
    switchPtr                 = GET_SWITCH_PTR(sw);
    routeEntry                = NULL;
    routeAllocated            = FALSE;
    routeAddedToRouteTree     = FALSE;
    routeAddedToAlternateTree = FALSE;
    ecmpGroupCreated          = FALSE;
    routeAddedToEcmpGroup     = FALSE;
    routeAddedToLookupTree    = FALSE;
    routeAddedToHardware      = FALSE;
    group                     = NULL;
    unicast                   = NULL;
    routeTree                 = NULL;
    ecmpGroup                 = NULL;
    routeLookupTree           = NULL;

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(route);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* See if the exact route already exists */
    key.route = *route;
    err       = fmCustomTreeFind(&switchPtr->routeTree,
                                 &key,
                                 (void **) &curRoute);

    if (err != FM_ERR_NOT_FOUND)
    {
        if (err == FM_OK)
        {
            err = FM_ERR_ALREADY_EXISTS;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Validate the action and determine whether an ECMP group is needed */
    switch (action->action)
    {
        case FM_ROUTE_ACTION_RPF_FAILURE:
        case FM_ROUTE_ACTION_NOP:
            needEcmpGroup = FALSE;
            break;

        case FM_ROUTE_ACTION_ROUTE:
        case FM_ROUTE_ACTION_DROP:
            needEcmpGroup = TRUE;
            break;

        default:
            err = FM_ERR_UNSUPPORTED;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /********************************************************************
     * Collect vrid and prefixLength information needed for further
     * processing.
     * Note that prefixLength includes the destination IP prefix, source
     * IP prefix, and VLAN prefix using these specific bit positions:
     * 0-3 : Vlan prefix     (FM_VLAN_PREFIX_POSITION)
     * 4-11 : Src IP prefix  (FM_SRC_IP_PREFIX_POSITION)
     * 12-19 : Dst IP prefix (FM_DST_IP_PREFIX_POSITION)
     ********************************************************************/
    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            arpNextHop->addr          = route->data.unicast.nextHop;
            arpNextHop->interfaceAddr = route->data.unicast.interfaceAddr;
            arpNextHop->vlan          = route->data.unicast.vlan;
            unicast                   = &route->data.unicast;
            vrid                      = unicast->vrid;
            routePrefixLength         = unicast->prefixLength;
            prefixLength              = (routePrefixLength << FM_DST_IP_PREFIX_POSITION)
                                        & FM_DST_IP_PREFIX_MASK;
            break;

        case FM_ROUTE_TYPE_UNICAST_ECMP:
            unicastEcmp       = &route->data.unicastECMP;
            vrid              = unicastEcmp->vrid;
            routePrefixLength = unicastEcmp->prefixLength;
            prefixLength      = (routePrefixLength << FM_DST_IP_PREFIX_POSITION)
                                & FM_DST_IP_PREFIX_MASK;
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            vrid      = 0;
            multicast = &route->data.multicast;

            switch (multicast->addressType)
            {
                case FM_MCAST_ADDR_TYPE_DSTIP:
                    routePrefixLength = multicast->info.dstIpRoute.dstPrefixLength;
                    prefixLength      = (routePrefixLength<< FM_DST_IP_PREFIX_POSITION)
                                        & FM_DST_IP_PREFIX_MASK;
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
                    routePrefixLength = multicast->info.dstIpVlanRoute.dstPrefixLength;
                    prefixLength      = ((routePrefixLength << FM_DST_IP_PREFIX_POSITION)
                                          & FM_DST_IP_PREFIX_MASK)
                                        | ((multicast->info.dstIpVlanRoute.vlanPrefixLength)
                                          & FM_VLAN_PREFIX_MASK);
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
                    routePrefixLength = multicast->info.dstSrcIpRoute.dstPrefixLength;
                    prefixLength      = ((routePrefixLength << FM_DST_IP_PREFIX_POSITION)
                                        & FM_DST_IP_PREFIX_MASK) |
                                        ((multicast->info.dstSrcIpRoute.srcPrefixLength
                                          << FM_SRC_IP_PREFIX_POSITION)
                                        & FM_SRC_IP_PREFIX_MASK);
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
                    routePrefixLength = multicast->info.dstSrcIpVlanRoute.dstPrefixLength;
                    prefixLength      = ((routePrefixLength << FM_DST_IP_PREFIX_POSITION)
                                        & FM_DST_IP_PREFIX_MASK) |
                                        ((multicast->info.dstSrcIpVlanRoute.srcPrefixLength
                                          << FM_SRC_IP_PREFIX_POSITION)
                                        & FM_SRC_IP_PREFIX_MASK) |
                                        ((multicast->info.dstSrcIpVlanRoute.vlanPrefixLength)
                                        & FM_VLAN_PREFIX_MASK);
                    break;

                default:
                    err = FM_ERR_INVALID_ARGUMENT;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                    break;

            }   /* end switch (multicast->addressType) */
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    ifEntry = NULL;

    if (route->routeType == FM_ROUTE_TYPE_UNICAST)
    {
        /* If the interface address is non-zero */
        if ( unicast->interfaceAddr.isIPv6
            || (unicast->interfaceAddr.addr[0] != 0) )
        {
            /* try to find an interface using the address.
             * If one is found, store the interface entry pointer and
             * retrieve the vlan for that interface.  Otherwise, set the
             * interface pointer to NULL and invalidate the vlan.
             */
            err = fmFindInterface(sw, &unicast->interfaceAddr, &ifEntry);

            if (err != FM_OK)
            {
                if (err != FM_ERR_INVALID_INTERFACE)
                {
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }
            }
        }
    }
    else if (route->routeType == FM_ROUTE_TYPE_MULTICAST)
    {
        group = fmFindMcastGroup(sw, route->data.multicast.mcastGroup);

        if (group == NULL)
        {
            err = FM_ERR_INVALID_MULTICAST_GROUP;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }
    }

    /* Allocate and initialize a new route record */
    routeEntry = fmAlloc( sizeof(fm_intRouteEntry) );

    if (routeEntry == NULL)
    {
        err = FM_ERR_NO_MEM;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    routeAllocated = TRUE;

    FM_CLEAR(*routeEntry);

    /* Setting state to FM_ROUTE_STATE_UP for new routes with
     * FM_ROUTE_STATE_NEXT_HOP_UP initial state */
    routeState = state;
    if (state == FM_ROUTE_STATE_NEXT_HOP_UP)
    {
        routeState = FM_ROUTE_STATE_UP;
    }

    routeEntry->switchPtr        = switchPtr;
    routeEntry->route            = *route;
    routeEntry->state            = routeState;
    routeEntry->action           = *action;
    routeEntry->prefix           = prefixLength;
    routeEntry->ecmpGroupId      = -1;
    routeEntry->routeEcmpGroupId = -1;
    routeEntry->mcastGroup       = group;

    /* Initialize vn tunnels tree. No tunnels are currently using this route. */
    fmTreeInit(&routeEntry->vnTunnelsTree);

    ecmpGroup = NULL;

    /* Determine which route tree to use for this route type */
    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* If an alternate route tree is used for this route type, search again. */
    if (routeTree != &switchPtr->routeTree)
    {
        err = fmCustomTreeFind(routeTree, &key, (void **) &curRoute);
    }
    else
    {
        err = FM_ERR_NOT_FOUND;
    }

    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            routeEntry->destIPAddress = &routeEntry->route.data.unicast.dstAddr;

            if (err == FM_ERR_NOT_FOUND)
            {
                /* This is the first route in an ECMP group. */
                switch (action->action)
                {
                    case FM_ROUTE_ACTION_ROUTE:
                        /* Create the ECMP group */
                        err = fmCreateECMPGroupInternal(sw,
                                                        &routeEntry->ecmpGroupId,
                                                        NULL,
                                                        NULL);

                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                        routeEntry->routeEcmpGroupId = routeEntry->ecmpGroupId;

                        err = fmAddECMPGroupNextHopsInternal(sw,
                                                             routeEntry->ecmpGroupId,
                                                             1,
                                                             &nextHop);

                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                        ecmpGroupCreated = TRUE;
                        break;

                    case FM_ROUTE_ACTION_RPF_FAILURE:
                    case FM_ROUTE_ACTION_NOP:
                        break;

                    case FM_ROUTE_ACTION_DROP:
                        /* Use the drop ECMP group */
                        if (switchPtr->dropEcmpGroup >= 0)
                        {
                            routeEntry->ecmpGroupId = switchPtr->dropEcmpGroup;
                        }
                        else
                        {
                            err = FM_ERR_UNSUPPORTED;
                            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                        }
                        break;

                    default:
                        err = FM_ERR_UNSUPPORTED;
                        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }
            }
            else if (err == FM_OK)
            {
                /* a matching route was found in the ECMP route table.
                 * If the found route is not the same type, this is an error.
                 * If the requested action is anything other than route,
                 * this is also an error.
                 * Otherwise, add the new route's next hop to the existing
                 * route's ecmp group.
                 */
                if (curRoute->route.routeType != FM_ROUTE_TYPE_UNICAST)
                {
                    err = FM_ERR_ALREADY_EXISTS;
                }
                else if ( (action->action != FM_ROUTE_ACTION_ROUTE)
                    || (curRoute->action.action != FM_ROUTE_ACTION_ROUTE) )
                {
                    err = FM_ERR_UNSUPPORTED;
                }

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                err = fmAddECMPGroupNextHopsInternal(sw,
                                                     curRoute->ecmpGroupId,
                                                     1,
                                                     &nextHop);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                routeEntry->ecmpGroupId      = curRoute->ecmpGroupId;
                routeEntry->routeEcmpGroupId = curRoute->routeEcmpGroupId;

                /* Add the route to the tree */
                err = fmCustomTreeInsert(&switchPtr->routeTree,
                                         routeEntry,
                                         routeEntry);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                /* This is all we need to do for this case.  err is already
                 * equal to FM_OK, so just set the route added flag
                 * and go to ABORT to finish up. */
                routeAddedToRouteTree = TRUE;
                goto ABORT;
            }
            else
            {
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            }
            break;

        case FM_ROUTE_TYPE_UNICAST_ECMP:
            routeEntry->destIPAddress = &routeEntry->route.data.unicastECMP.dstAddr;

            if (err != FM_ERR_NOT_FOUND)
            {
                if (err == FM_OK)
                {
                    err = FM_ERR_ALREADY_EXISTS;
                }

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            }

            if (action->action == FM_ROUTE_ACTION_ROUTE)
            {
                routeEntry->ecmpGroupId = route->data.unicastECMP.ecmpGroup;
            }
            else if (action->action == FM_ROUTE_ACTION_DROP)
            {
                routeEntry->ecmpGroupId = switchPtr->dropEcmpGroup;
            }
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            if (err != FM_ERR_NOT_FOUND)
            {
                if (err == FM_OK)
                {
                    err = FM_ERR_ALREADY_EXISTS;
                }

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            }

            if ( needEcmpGroup && (group != NULL) )
            {
                routeEntry->ecmpGroupId = group->ecmpGroup;
            }
            break;

        default:
            break;
    }

    /* Determine which route lookup tree to use, if any */
    if (switchPtr->routeLookupTrees != NULL)
    {
        if (routeEntry->destIPAddress != NULL)
        {
            err = fmGetRouteLookupTree(sw, vrid, routePrefixLength, &routeLookupTree);
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }
    }

    if (routeEntry->ecmpGroupId >= 0)
    {
        ecmpGroup = switchPtr->ecmpGroups[routeEntry->ecmpGroupId];
    }

    fmSetRouteActiveFlag(sw, routeEntry, FALSE);

    /* Add the route to the tree */
    err = fmCustomTreeInsert(&switchPtr->routeTree, routeEntry, routeEntry);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    routeAddedToRouteTree = TRUE;

#if 0
    /* Find its successor route, if any */
    err = fmCustomTreeSuccessor(&switchPtr->routeTree,
                                routeEntry,
                                (void **) &curKey,
                                (void **) &curRoute);

    if (err != FM_OK)
    {
        if (err == FM_ERR_NOT_FOUND)
        {
            goto ABORT;
        }

        if (err == FM_ERR_NO_MORE)
        {
            curRoute = NULL;
        }
    }

    /* new entry goes before the current entry */
    fmInsertRouteBefore(switchPtr, curRoute, routeEntry);

    routeAddedToLinkedList = TRUE;
#endif

    /* If this route type uses a different route tree in addition to the
     * generic route tree, add the route to the additional tree now. */
    if (routeTree != &switchPtr->routeTree)
    {
        /* Add the route to the ECMP tree */
        err = fmCustomTreeInsert(routeTree, routeEntry, routeEntry);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        routeAddedToAlternateTree = TRUE;
    }

    if (ecmpGroup != NULL)
    {
        err = fmCustomTreeInsert(&ecmpGroup->routeTree,
                                 routeEntry,
                                 routeEntry);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        routeAddedToEcmpGroup = TRUE;
    }

    if (routeLookupTree != NULL)
    {
        err = fmCustomTreeInsert(routeLookupTree,
                                 routeEntry->destIPAddress,
                                 routeEntry);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        routeAddedToLookupTree = TRUE;
    }

    /* Now add the route into the hardware */
    err = switchPtr->AddRoute(sw, routeEntry);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    
    routeAddedToHardware = TRUE;

    if ( (group != NULL) && (group->routePtrPtr != NULL) )
    {
        *group->routePtrPtr = routeEntry;
    }

    err = fmNotifyVNTunnelAboutRouteChange(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);


ABORT:

    if (err != FM_OK)
    {
        if (routeAddedToHardware)
        {
            switchPtr->DeleteRoute(sw, routeEntry);
            if ( (group != NULL) && (group->routePtrPtr != NULL) )
            {
                *group->routePtrPtr = NULL;
            }
        }
        if (routeAddedToEcmpGroup)
        {
            fmCustomTreeRemove(&ecmpGroup->routeTree, routeEntry, NULL);
        }

        if (routeAddedToRouteTree)
        {
            fmCustomTreeRemove(&switchPtr->routeTree, routeEntry, NULL);
        }

        if (routeAddedToAlternateTree)
        {
            fmCustomTreeRemove(routeTree, routeEntry, NULL);
        }

        if (ecmpGroupCreated)
        {
            fmDeleteECMPGroupInternal(sw, routeEntry->ecmpGroupId);
        }

        if (routeAddedToLookupTree)
        {
            fmCustomTreeRemove(routeLookupTree, routeEntry->destIPAddress, NULL);
        }

        if (routeAllocated)
        {
            fmFree(routeEntry);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);


}   /* end fmAddRouteInternal */




/*****************************************************************************/
/** fmAddRoute
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Add a new routing entry to the router table. This
 *                  function will automatically store the route in order to
 *                  ensure that the longest prefix match will be picked first.
 *                  This service allows setting the route state immediately.
 *                  The route is added to the switch regardless whether the
 *                  route is valid and/or active.
 *
 * \note            It is permitted to add a route before the associated
 *                  interface exists. The route will be loaded into the switch
 *                  but will be inactive until the interface is created and
 *                  brought up.
 *                                                                      \lb\lb
 *                  Also, if there is no ARP table entry to resolve the next
 *                  hop's destination MAC address, then this function
 *                  automatically uses the CPU's destination MAC address,
 *                  forcing any frame using that route to be forwarded to the
 *                  CPU for further processing.
 *                                                                      \lb\lb
 *                  Addition of multicast routes is not permitted using this
 *                  function.  To add a multicast route, use the multicast
 *                  group subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route to add.
 *
 * \param[in]       state is the desired state of this route when the route
 *                  is created.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_ALREADY_EXISTS if the route already exists.
 * \return          FM_ERR_TABLE_FULL if the routing table is full.
 * \return          FM_ERR_ARP_TABLE_FULL if there is no more room in the ARP
 *                  table.
 * \return          FM_ERR_NO_FFU_RES_FOUND if no suitable FFU resources are
 *                  available for this route.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if an attempt was made to
 *                  add a multicast route.
 *
 *****************************************************************************/
fm_status fmAddRoute(fm_int         sw,
                     fm_routeEntry *route,
                     fm_routeState  state)
{
    fm_status             err;

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, route=%p, state=%d\n",
                      sw,
                      (void *) route,
                      state );

    err = fmAddRouteExt(sw, route, state, NULL);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmAddRoute */




/*****************************************************************************/
/** fmAddRouteExt
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Add a new routing entry to the router table along with
 *                  a specified route action. This function will automatically
 *                  store the route in order to ensure that the longest prefix
 *                  match will be picked first. This service allows setting 
 *                  the route state immediately. The route is added to the 
 *                  switch regardless whether the route is valid and/or active.
 *
 * \note            It is permitted to add a route before the associated
 *                  interface exists. The route will be loaded into the switch
 *                  but will be inactive until the interface is created and
 *                  brought up.
 *                                                                      \lb\lb
 *                  Also, if there is no ARP table entry to resolve the next
 *                  hop's destination MAC address, then this function
 *                  automatically uses the CPU's destination MAC address,
 *                  forcing any frame using that route to be forwarded to the
 *                  CPU for further processing.
 *                                                                      \lb\lb
 *                  Addition of multicast routes is not permitted using this
 *                  function.  To add a multicast route, use the multicast
 *                  group subsystem.
 *                                                                      \lb\lb
 *                  Routes that are added with a 'drop' action specified will
 *                  not have an ARP entry allocated to them.  This means that
 *                  a later call to ''fmSetRouteAction'' changing the route
 *                  action to 'route' could fail with FM_ERR_ARP_TABLE_FULL.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route to add.
 *
 * \param[in]       state is the desired state of this route when the route
 *                  is created.
 *
 * \param[in]       action points to the desired routing action for this route.
 *                  If NULL, the default action is ''FM_ROUTE_ACTION_ROUTE''.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_ALREADY_EXISTS if the route already exists.
 * \return          FM_ERR_TABLE_FULL if the routing table is full.
 * \return          FM_ERR_ARP_TABLE_FULL if there is no more room in the ARP
 *                  table.
 * \return          FM_ERR_NO_FFU_RES_FOUND if no suitable FFU resources are
 *                  available for this route.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if an attempt was made to
 *                  add a multicast route.
 *
 *****************************************************************************/
fm_status fmAddRouteExt(fm_int          sw,
                        fm_routeEntry * route,
                        fm_routeState   state,
                        fm_routeAction *action)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_ipAddr             destAddr;
    fm_int                maxPrefix;
    fm_int                prefixLength;
    static fm_routeAction defaultAction =
    {
        .action = FM_ROUTE_ACTION_ROUTE
    };
    fm_routeAction *      routeAction;

    if (action == NULL)
    {
        routeAction = &defaultAction;
    }
    else
    {
        routeAction = action;
    }

    FM_LOG_ENTRY_API( FM_LOG_CAT_ROUTING,
                      "sw = %d, route=%p, state=%d, action=%p(%d)\n",
                      sw,
                      (void *) route,
                      state,
                      (void *) action,
                      routeAction->action );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (switchPtr->AddRoute == NULL) || (switchPtr->maxRoutes <= 0) )
    {
        err = FM_ERR_UNSUPPORTED;
        goto ABORT;
    }

    /* error-check incoming route information */
    if (route == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    if ( !fmIsRouteEntryUnicast(route) )
    {
        err = FM_ERR_USE_MCAST_FUNCTIONS;
        goto ABORT;
    }

    fmGetRouteDestAddress(route, &destAddr);

    prefixLength = route->data.unicast.prefixLength;

    maxPrefix = (destAddr.isIPv6)
                ? FM_IPV6_MAX_PREFIX_LENGTH
                : FM_IPV4_MAX_PREFIX_LENGTH;

    if ( (prefixLength < 0) || (prefixLength > maxPrefix) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        goto ABORT;
    }

    /* gain exclusive access to routing tables */
    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    err = fmAddRouteInternal(sw, route, state, routeAction);

    /* release exclusive access to routing tables */
    fmReleaseWriteLock(&switchPtr->routingLock);

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmAddRouteExt */




/*****************************************************************************/
/** fmDeleteRouteInternal
 * \ingroup intRoute
 *
 * \desc            Delete a routing entry from the router table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route to delete.
 *                  If the route is a unicast route and the nextHop IP address
 *                  is set to 0xffffffff, all routes with the specified
 *                  destination address and prefix length will be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if route is NULL.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmDeleteRouteInternal(fm_int         sw,
                                fm_routeEntry *route)
{
    fm_switch *           switchPtr;
    fm_status             err = FM_OK;
    fm_intRouteEntry *    curRoute;
    fm_intRouteEntry *    ecmpRoute;
    fm_intRouteEntry      key;
    fm_customTree *       routeTree;
    fm_intEcmpGroup *     ecmpGroup;
    fm_ecmpNextHop *      nextHopList = NULL;
    fm_int                nextHopCount = 0;
    fm_int                iplen;
    fm_int                i;
    fm_int                size;
    fm_intNextHop *       intNextHop;
    fm_bool               wildCard = FALSE;
    fm_int                routePrefixLength;
    fm_customTree *       routeLookupTree;
    fm_int                vrid;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, route=%p\n",
                 sw,
                 (void *) route);

    switchPtr = GET_SWITCH_PTR(sw);

    if (route == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    routePrefixLength = 0;
    routeLookupTree   = NULL;
    vrid              = 0;

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(route);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Get destination IP address and route prefix length */
    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            routePrefixLength = route->data.unicast.prefixLength;
            vrid              = route->data.unicast.vrid;
            break;

        case FM_ROUTE_TYPE_UNICAST_ECMP:
            routePrefixLength = route->data.unicastECMP.prefixLength;
            vrid              = route->data.unicastECMP.vrid;
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            break;

        default:
            err = FM_ERR_INVALID_ARGUMENT;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* If this is unicast route type, is it also a wildcard deletion? */
    if (route->routeType == FM_ROUTE_TYPE_UNICAST)
    {
        /* Check for wildcard deletion - are all word(s) of nexthop IP
         * address equal to 0xffffffff? */
        if (route->data.unicast.nextHop.isIPv6)
        {
            iplen = 4;
        }
        else
        {
            iplen = 1;
        }

        for (i = 0 ; i < iplen ; i++)
        {
            if (route->data.unicast.nextHop.addr[i] != 0xffffffff)
            {
                break;
            }
        }

        if (i >= iplen)
        {
            wildCard = TRUE;
        }
    }

    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Try to find the route */
    key.route = *route;

    if (wildCard)
    {
        err = fmCustomTreeFind( routeTree, &key, (void **) &curRoute );
    }
    else
    {
        err = fmCustomTreeFind( &switchPtr->routeTree,
                                &key,
                                (void **) &curRoute );
    }

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Determine which route lookup tree to use, if any */
    if ( (switchPtr->routeLookupTrees != NULL) && (curRoute->destIPAddress != NULL) )
    {
        err = fmGetRouteLookupTree(sw, vrid, routePrefixLength, &routeLookupTree);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    if (curRoute->ecmpGroupId >= 0)
    {
        ecmpGroup = switchPtr->ecmpGroups[curRoute->ecmpGroupId];
    }
    else
    {
        ecmpGroup = NULL;
    }

    if (routeTree != &switchPtr->routeTree)
    {
        err = fmCustomTreeFind( routeTree, &key, (void **) &ecmpRoute );

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }
    else
    {
        ecmpRoute = NULL;
    }

    if ( (curRoute->route.routeType == FM_ROUTE_TYPE_UNICAST)
        && (ecmpGroup != NULL) )
    {
        /* Allocate storage for the nexthop(s) that need to be deleted. */
        size        = sizeof(fm_ecmpNextHop) * switchPtr->maxEcmpGroupSize;
        nextHopList = fmAlloc(size);

        if (nextHopList == NULL)
        {
            err = FM_ERR_NO_MEM;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }

        FM_MEMSET_S(nextHopList, size, 0, size);

        if (wildCard)
        {
            /* ECMP wild-card deletion, delete all next-hops */
            err = fmGetECMPGroupNextHopListInternal(sw,
                                                    curRoute->ecmpGroupId,
                                                    &nextHopCount,
                                                    nextHopList,
                                                    switchPtr->maxEcmpGroupSize);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }
        else
        {
            /* Single-route deletion */
            nextHopList->type                   = FM_NEXTHOP_TYPE_ARP;
            nextHopList->data.arp.addr          = route->data.unicast.nextHop;
            nextHopList->data.arp.interfaceAddr = route->data.unicast.interfaceAddr;
            nextHopList->data.arp.vlan          = route->data.unicast.vlan;
            nextHopCount                        = 1;
        }

        /* try to delete the next-hop(s) from the dedicated ECMP group */
        err = fmDeleteECMPGroupNextHopsInternal(sw,
                                                curRoute->ecmpGroupId,
                                                nextHopCount,
                                                nextHopList);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        /* If dedicated ECMP group is not empty, we're done. */
        if (ecmpGroup->nextHopCount > 0)
        {
            /* Remove this route from the route tree. */
            fmCustomTreeRemove(&switchPtr->routeTree, curRoute, NULL);

            if (curRoute == ecmpRoute)
            {
                /* This route is the ECMP route, we have to remove it from the
                 * ECMP route tree and put one of the remaining routes
                 * in that tree in its place. */
                fmCustomTreeRemove(routeTree, ecmpRoute, NULL);
                fmCustomTreeRemove(&ecmpGroup->routeTree, ecmpRoute, NULL);

                /* There is no way that ecmpRoute could be NULL, because we
                 * are only here if curRoute is equal to ecmpRoute, and if
                 * curRoute is NULL, we segfaulted a long ways back in the
                 * code. Nevertheless, Klocwork thinks it is possible, and
                 * it is easier to just make KW happy than to fight it. */
                if ( (routeLookupTree != NULL) && (ecmpRoute != NULL) )
                {
                    fmCustomTreeRemove(routeLookupTree,
                                       ecmpRoute->destIPAddress,
                                       NULL);
                }

                /* Get the first remaining next hop from the ECMP group. */
                intNextHop = ecmpGroup->nextHops[0];

                /* Build a new route key using the next hop information. */
                key.route.data.unicast.nextHop       = intNextHop->nextHop.data.arp.addr;
                key.route.data.unicast.interfaceAddr = intNextHop->nextHop.data.arp.interfaceAddr;
                key.route.data.unicast.vlan          = intNextHop->nextHop.data.arp.vlan;

                /* Find the target route in the main route tree. */
                err = fmCustomTreeFind( &switchPtr->routeTree,
                                       &key,
                                       (void **) &ecmpRoute);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                /* Add the target route to the ECMP group and ECMP route
                 * tree in place of the route that is being deleted. */
                err = fmCustomTreeInsert(routeTree, ecmpRoute, ecmpRoute);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                err = fmCustomTreeInsert(&ecmpGroup->routeTree,
                                         ecmpRoute,
                                         ecmpRoute);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                if (routeLookupTree != NULL)
                {
                    err = fmCustomTreeInsert(routeLookupTree,
                                             ecmpRoute->destIPAddress,
                                             ecmpRoute);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }

                /* Copy the route state into the new base ECMP route */
                ecmpRoute->state  = curRoute->state;
                ecmpRoute->action = curRoute->action;
                ecmpRoute->active = curRoute->active;

                /* Update the switch-specific API */
                if (switchPtr->ReplaceECMPBaseRoute != NULL)
                {
                    err = switchPtr->ReplaceECMPBaseRoute(sw, curRoute, ecmpRoute);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }

                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                             "Replaced ECMP route and ECMP group route, "
                             "old route %p, new route %p\n",
                             (void *) curRoute,
                             (void *) ecmpRoute);
            }

            fmFree(curRoute);

            err = FM_OK;
            goto ABORT;
        }
    }

    /* Remove the entry from the hardware */
    err = switchPtr->DeleteRoute(sw, curRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Remove the route from the ECMP group's route table */
    if (ecmpGroup != NULL)
    {
        err = fmCustomTreeRemove(&ecmpGroup->routeTree, curRoute, NULL);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Now remove the route from the route tree (and optionally from the
     * ECMP route tree as well). */
    fmCustomTreeRemove(&switchPtr->routeTree, curRoute, NULL);

    if (routeTree != &switchPtr->routeTree)
    {
        fmCustomTreeRemove(routeTree, curRoute, NULL);
    }

    if (routeLookupTree != NULL)
    {
        fmCustomTreeRemove(routeLookupTree, curRoute->destIPAddress, NULL);
    }

    switch (curRoute->route.routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            if ( (curRoute->ecmpGroupId >= 0)
                && (curRoute->ecmpGroupId != switchPtr->dropEcmpGroup) )
            {
                err = fmDeleteECMPGroupInternal(sw, curRoute->ecmpGroupId);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            }
            break;

        default:
            break;

    }

    if ( (curRoute->mcastGroup != NULL)
        && (curRoute->mcastGroup->routePtrPtr != NULL) )
    {
        *curRoute->mcastGroup->routePtrPtr = NULL;
    }

    fmFree(curRoute);

    err = fmNotifyVNTunnelAboutRouteChange(sw);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);


ABORT:

    if (nextHopList != NULL)
    {
        fmFree(nextHopList);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteRouteInternal */




/*****************************************************************************/
/** fmDeleteRoute
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Delete a routing entry from the router table.
 * \note            Deletion of multicast routes is not permitted using this
 *                  function.  To delete a multicast route, use the multicast
 *                  group subsystem.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points to the route to delete.
 *                  If the route is a unicast route and the nextHop IP address
 *                  is set to 0xffffffff, all routes with the specified
 *                  destination address and prefix length will be deleted.
 *                  If the route is a multicast route and the source IP
 *                  address is set to all one-bits (i.e., 0xffffffff is
 *                  written to every used word of the address), all routes
 *                  with the specified destination address will be deleted.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if route is NULL.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_USE_MCAST_FUNCTIONS if an attempt was made to
 *                  delete a multicast route.
 *
 *****************************************************************************/
fm_status fmDeleteRoute(fm_int         sw,
                        fm_routeEntry *route)
{
    fm_switch *switchPtr;
    fm_status  err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (route == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    if ( (switchPtr->DeleteRoute != NULL)
        && (switchPtr->maxRoutes > 0) )
    {
        if ( !fmIsRouteEntryUnicast(route) )
        {
            err = FM_ERR_USE_MCAST_FUNCTIONS;
            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }

        /* Get exclusive access to routing tables */
        err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        err = fmDeleteRouteInternal(sw, route);

        fmReleaseWriteLock(&switchPtr->routingLock);
    }
    else
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmDeleteRoute */




/*****************************************************************************/
/** fmReplaceRouteECMP
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Replaces an existing next-hop in an ECMP route with a new
 *                  next-hop. This function may not be used to replace unrelated
 *                  routes. Both old and new routes must be identical except for
 *                  the next-hop information.
 *
 * \note            This function is intended for use when ''fmAddRoute'' 
 *                  has been called (perhaps repeatedly) for a route type
 *                  of ''FM_ROUTE_TYPE_UNICAST''. When ECMP groups are
 *                  created with ''fmCreateECMPGroup'' and 
 *                  ''fmAddECMPGroupNextHops'', use ''fmReplaceECMPGroupNextHop''
 *                  instead of this function.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       oldRoute points to the route to be replaced.
 *
 * \param[in]       newRoute points to the new route that is to replace
 *                  the old route.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_NOT_FOUND if the old route cannot be found in the
 *                  routing table.
 * \return          FM_ERR_ALREADY_EXISTS if the new route already exists in
 *                  the routing table.
 *
 *****************************************************************************/
fm_status fmReplaceRouteECMP(fm_int         sw,
                             fm_routeEntry *oldRoute,
                             fm_routeEntry *newRoute)
{
    fm_switch *       switchPtr = NULL;
    fm_bool           routeLockTaken = FALSE;
    fm_status         err;
    fm_intRouteEntry *curRoute;
    fm_intRouteEntry  key;
    fm_ecmpNextHop    oldNextHop;
    fm_ecmpNextHop    newNextHop;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, oldRoute = %p, newRoute = %p\n",
                  sw,
                  (void *) oldRoute,
                  (void *) newRoute );

    VALIDATE_AND_PROTECT_SWITCH(sw);

    if ( (oldRoute == NULL) || (newRoute == NULL) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (switchPtr->ReplaceECMPGroupNextHop == NULL)
        || (switchPtr->maxRoutes <= 0) )
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    if ( (oldRoute->routeType != newRoute->routeType)
        || (oldRoute->routeType != FM_ROUTE_TYPE_UNICAST) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(oldRoute);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = fmApplyMasksToRoute(newRoute);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    if ( fmCompareEcmpRoutes(oldRoute, newRoute) != 0)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* gain exclusive access to routing tables */
    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    routeLockTaken = TRUE;

    /* Make sure the new route isn't already in the table */
    key.route = *newRoute;

    err = fmCustomTreeFind( &switchPtr->routeTree,
                            &key,
                            (void **) &curRoute );

    if (err == FM_OK)
    {
        err = FM_ERR_ALREADY_EXISTS;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }
    else if (err != FM_ERR_NOT_FOUND)
    {
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Try to find the old route */
    key.route = *oldRoute;

    err = fmCustomTreeFind( &switchPtr->routeTree,
                            &key,
                            (void **) &curRoute );

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    FM_CLEAR( oldNextHop );
    oldNextHop.type                   = FM_NEXTHOP_TYPE_ARP;
    oldNextHop.data.arp.addr          = oldRoute->data.unicast.nextHop;
    oldNextHop.data.arp.interfaceAddr = oldRoute->data.unicast.interfaceAddr;
    oldNextHop.data.arp.vlan          = oldRoute->data.unicast.vlan;
    oldNextHop.data.arp.trapCode      = FM_TRAPCODE_L3_ROUTED_NO_ARP_0;

    FM_CLEAR( newNextHop );
    newNextHop.type                   = FM_NEXTHOP_TYPE_ARP;
    newNextHop.data.arp.addr          = newRoute->data.unicast.nextHop;
    newNextHop.data.arp.interfaceAddr = newRoute->data.unicast.interfaceAddr;
    newNextHop.data.arp.vlan          = newRoute->data.unicast.vlan;
    newNextHop.data.arp.trapCode      = FM_TRAPCODE_L3_ROUTED_NO_ARP_0;

    /* Remove the old route from the tree */
    err = fmCustomTreeRemoveCertain(&switchPtr->routeTree, curRoute, NULL);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Convert the old route's record into the new route */
    curRoute->route.data.unicast.nextHop       = newNextHop.data.arp.addr;
    curRoute->route.data.unicast.interfaceAddr = newNextHop.data.arp.interfaceAddr;
    curRoute->route.data.unicast.vlan          = newNextHop.data.arp.vlan;

    /* Add the new route back into the tree */
    err = fmCustomTreeInsert(&switchPtr->routeTree, curRoute, curRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = fmReplaceECMPGroupNextHopInternal(sw,
                                            curRoute->ecmpGroupId,
                                            &oldNextHop,
                                            &newNextHop);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);


ABORT:

    if (routeLockTaken)
    {
        /* release exclusive access to routing tables */
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmReplaceRouteECMP */


/*****************************************************************************/
/** fmSetRouteState
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Set the state of a route.
 *
 * \note            Deactivated routes are not removed from the switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points the route whose state is to be set.
 *
 * \param[in]       state is the desired state for this route.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if route is NULL.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmSetRouteState(fm_int         sw,
                          fm_routeEntry *route,
                          fm_routeState  state)
{
    fm_switch *       switchPtr;
    fm_status         err;
    fm_intRouteEntry *curRoute;
    fm_bool           lockTaken = FALSE;
    fm_customTree *   routeTree;
    fm_intRouteEntry  key;
    fm_intEcmpGroup * ecmpGroup;
    fm_ecmpNextHop    nextHopRecord;
    fm_nextHop *      arpNextHop;
    fm_intNextHop *   nextHop;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, state = %d\n",
                     sw,
                     state);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if ( (switchPtr->SetRouteActive == NULL) ||
        (switchPtr->maxRoutes <= 0) )
    {
        err = FM_ERR_UNSUPPORTED;
        goto ABORT;
    }

    if (route == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(route);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    lockTaken = TRUE;

    /*
     * Find the route in the route table
     */
    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    key.route = *route;

    err = fmCustomTreeFind(routeTree, &key, (void **) &curRoute);

    if (err != FM_OK)
    {
        goto ABORT;
    }

    else if (state == FM_ROUTE_STATE_NEXT_HOP_UP ||
             state == FM_ROUTE_STATE_NEXT_HOP_DOWN)
    {
        if (route->routeType == FM_ROUTE_TYPE_UNICAST)
        {
            if (curRoute->ecmpGroupId >= 0)
            {
                ecmpGroup = switchPtr->ecmpGroups[curRoute->ecmpGroupId];
            }
            else
            {
                ecmpGroup = NULL;
            }
    
            if (ecmpGroup)
            {
                /* Create the Next Hop Record */
    
                FM_CLEAR( nextHop );
            
                nextHopRecord.type        = FM_NEXTHOP_TYPE_ARP;
                arpNextHop                = &nextHopRecord.data.arp;
                arpNextHop->addr          = route->data.unicast.nextHop;
                arpNextHop->interfaceAddr = route->data.unicast.interfaceAddr;
                arpNextHop->vlan          = route->data.unicast.vlan;
                    
                nextHop = fmFindNextHopInternal(sw,
                                                ecmpGroup,
                                                &nextHopRecord,
                                                NULL);
    
                if (nextHop)
                {
                    if (state == FM_ROUTE_STATE_NEXT_HOP_UP)
                    {
                        nextHop->state = FM_NEXT_HOP_STATE_UP;
                    }
                    else
                    {
                        nextHop->state = FM_NEXT_HOP_STATE_DOWN;
                    }
                    err = fmUpdateEcmpGroupInternal(sw, ecmpGroup);
                }
                else
                {
                    err = FM_ERR_NOT_FOUND;
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }
            }
        }
        else
        {
            err = FM_ERR_UNSUPPORTED;
            goto ABORT;
        }
    }

    /* Found route, update state if needed */
    else if (curRoute->state != state)
    {
        curRoute->state = state;
        err             = fmSetRouteActiveFlag(sw, curRoute, TRUE);
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetRouteState */




/*****************************************************************************/
/** fmGetRouteState
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the state of a route.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points the route whose state is to be retrieved.
 *
 * \param[out]      state points to caller-allocated storage where this
 *                  function will place the current state of the route.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if route is NULL.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmGetRouteState(fm_int         sw,
                          fm_routeEntry *route,
                          fm_routeState *state)
{
    fm_switch *       switchPtr;
    fm_status         err;
    fm_bool           lockTaken = FALSE;
    fm_intRouteEntry *curRoute;
    fm_customTree *   routeTree;
    fm_intRouteEntry  key;
    fm_intEcmpGroup * ecmpGroup;
    fm_intNextHop *   nextHop;
    fm_nextHop *      arpNextHop;
    fm_ecmpNextHop    nextHopRecord;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxRoutes <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    if (route == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(route);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    /*
     * Find the route in the route table
     */
    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    key.route = *route;

    err = fmCustomTreeFind(routeTree, &key, (void **) &curRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    if (curRoute != NULL)
    {
        /* found the route! */
        *state = curRoute->state;

        /*Check next hop state for routes with
         * FM_ROUTE_STATE_UP state.
         */
        if (curRoute->state == FM_ROUTE_STATE_UP)
        {
            if (route->routeType == FM_ROUTE_TYPE_UNICAST)
            {
                if (curRoute->ecmpGroupId >= 0)
                {
                    ecmpGroup = switchPtr->ecmpGroups[curRoute->ecmpGroupId];
                }
                else
                {
                    ecmpGroup = NULL;
                }
                
                if (ecmpGroup)
                {
                    /* Create the Next Hop Record */
    
                    FM_CLEAR( nextHop );
            
                    nextHopRecord.type        = FM_NEXTHOP_TYPE_ARP;
                    arpNextHop                = &nextHopRecord.data.arp;
                    arpNextHop->addr          = route->data.unicast.nextHop;
                    arpNextHop->interfaceAddr = route->data.unicast.interfaceAddr;
                    arpNextHop->vlan          = route->data.unicast.vlan;

                    nextHop = fmFindNextHopInternal(sw,
                                                    ecmpGroup,
                                                    &nextHopRecord,
                                                    NULL);

                    if (nextHop)
                    {
                        if (nextHop->state == FM_NEXT_HOP_STATE_DOWN)
                        {
                            *state = FM_ROUTE_STATE_NEXT_HOP_DOWN;
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* Route wasn't found, return an error */
        err = FM_ERR_NOT_FOUND;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouteState */




/*****************************************************************************/
/** fmSetRouteAction
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Set the routing action for a route.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points the route whose action is to be set.
 *
 * \param[in]       action points to the desired route action for this route.
 *                  If NULL, the default action is FM_ROUTE_ACTION_ROUTE.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if route is NULL.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmSetRouteAction(fm_int         sw,
                          fm_routeEntry * route,
                          fm_routeAction *action)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intRouteEntry *    curRoute;
    fm_bool               lockTaken = FALSE;
    fm_customTree *       routeTree;
    fm_intRouteEntry      key;
    fm_ecmpNextHop        nextHop;
    static fm_routeAction defaultAction =
    {
        .action = FM_ROUTE_ACTION_ROUTE
    };
    fm_routeAction *      routeAction;

    if (action == NULL)
    {
        routeAction = &defaultAction;
    }
    else
    {
        routeAction = action;
    }

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, route = %p, action = %p(%d)\n",
                     sw,
                     (void *) route,
                     (void *) action,
                     routeAction->action);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    FM_CLEAR( nextHop );

    if ( (switchPtr->DeleteRoute == NULL) || (switchPtr->maxRoutes <= 0) )
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    if (route == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(route);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    /*
     * Find the route in the route table
     */
    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    key.route = *route;

    err = fmCustomTreeFind(routeTree, &key, (void **) &curRoute);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* If route wasn't found, return an error */
    if (curRoute == NULL)
    {
        err = FM_ERR_NOT_FOUND;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Found route. */
    curRoute->action = *routeAction;

    if (switchPtr->SetRouteAction == NULL)
    {
        /* Note: This is not an atomic operation. The route will blink out
         * then back in. */

        /* Delete the route from the hardware */
        err = switchPtr->DeleteRoute(sw, curRoute);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Prepare new route action. */
    if (routeAction->action == FM_ROUTE_ACTION_ROUTE)
    {
        switch (curRoute->route.routeType)
        {
            case FM_ROUTE_TYPE_UNICAST:
                curRoute->ecmpGroupId = curRoute->routeEcmpGroupId;

                if (curRoute->ecmpGroupId < 0)
                {
                    /* Create the ECMP group */
                    nextHop.type                   = FM_NEXTHOP_TYPE_ARP;
                    nextHop.data.arp.addr          = curRoute->route.data.unicast.nextHop;
                    nextHop.data.arp.interfaceAddr = curRoute->route.data.unicast.interfaceAddr;
                    nextHop.data.arp.vlan          = curRoute->route.data.unicast.vlan;

                    err = fmCreateECMPGroupInternal(sw,
                                                    &curRoute->ecmpGroupId,
                                                    NULL,
                                                    NULL);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

                    curRoute->routeEcmpGroupId = curRoute->ecmpGroupId;

                    err = fmAddECMPGroupNextHopsInternal(sw,
                                                         curRoute->ecmpGroupId,
                                                         1,
                                                         &nextHop);
                    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                }
                break;

            case FM_ROUTE_TYPE_UNICAST_ECMP:
                curRoute->ecmpGroupId = curRoute->route.data.unicastECMP.ecmpGroup;
                break;

            case FM_ROUTE_TYPE_MULTICAST:
                err = fmCreateECMPGroupInternal(sw,
                                                &curRoute->ecmpGroupId,
                                                NULL,
                                                curRoute->mcastGroup);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                break;

            default:
                break;
        }
    }
    else if (routeAction->action == FM_ROUTE_ACTION_DROP)
    {
        curRoute->ecmpGroupId = switchPtr->dropEcmpGroup;
    }

    if (switchPtr->SetRouteAction != NULL)
    {
        /* This should be an atomic operation which replaces the route
         * action without removing and adding the route. This function
         * must also store the new updated route information from newRoute
         * into curRoute before returning. */
        err = switchPtr->SetRouteAction(sw, curRoute);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }
    else
    {
        /* Add the route back into the hardware */
        err = switchPtr->AddRoute(sw, curRoute);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

ABORT:

    if (lockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetRouteAction */




/*****************************************************************************/
/** fmGetRouteAction
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Set the routing action for a route.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points the route whose action is to be set.
 *
 * \param[out]      action points to caller-allocated memory into which the
 *                  route's action will be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_INVALID_ARGUMENT if route or action are NULL.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the 
 *                  switch.
 *
 *****************************************************************************/
fm_status fmGetRouteAction(fm_int         sw,
                          fm_routeEntry * route,
                          fm_routeAction *action)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intRouteEntry *    curRoute;
    fm_bool               lockTaken = FALSE;
    fm_intRouteEntry      key;
    fm_customTree *       routeTree;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, route = %p, action = %p\n",
                     sw,
                     (void *) route,
                     (void *) action);

    if ( (route == NULL) || (action == NULL) )
    {
        FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Apply Masks to route fields to ensure address consistency */
    err = fmApplyMasksToRoute(route);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    /* Try to find the route */
    key.route = *route;

    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmCustomTreeFind(routeTree, &key, (void **) &curRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* If route wasn't found, return an error */
    if (curRoute == NULL)
    {
        err = FM_ERR_NOT_FOUND;
    }

    /* Found route, update action */
    else
    {
        *action = curRoute->action;
        err     = FM_OK;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouteAction */


/*****************************************************************************/
/** fmGetRouteList
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Return a list of all routes on a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      numRoutes points to caller allocated storage where
 *                  this function should place the number of routes returned
 *                  in routeList.
 *
 * \param[out]      routeList is an array that this function will fill
 *                  with the list of routes.
 *
 * \param[in]       max is the size of routeList, being the maximum
 *                  number of routes that routeList can hold.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_BUFFER_FULL if max was too small to accommodate
 *                  the entire list of routes.
 *
 *****************************************************************************/
fm_status fmGetRouteList(fm_int         sw,
                         fm_int *       numRoutes,
                         fm_routeEntry *routeList,
                         fm_int         max)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intRouteEntry *    route;
    fm_int                curRoute = 0;
    fm_customTreeIterator iter;
    fm_intRouteEntry *    key;
    fm_bool               lockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, numRoutes = %p, routeList = %p, max = %d\n",
                     sw,
                     (void *) numRoutes,
                     (void *) routeList,
                     max);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->RouterInit == NULL ||
        switchPtr->maxRoutes <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    fmCustomTreeIterInit(&iter, &switchPtr->routeTree);

    while (1)
    {
        err = fmCustomTreeIterNext( &iter, (void **) &key, (void **) &route );

        if (err == FM_ERR_NO_MORE)
        {
            err = FM_OK;
            break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        if (curRoute >= max)
        {
            err = FM_ERR_BUFFER_FULL;
            break;
        }

        routeList[curRoute] = route->route;
        curRoute++;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    *numRoutes = curRoute;

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouteList */




/*****************************************************************************/
/** fmGetRouteFirst
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the first route. The routes are sorted by VRID
 *                  and longest prefix match.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      searchToken points to caller-allocated storage of type
 *                  fm_voidptr, where this function will store a token
 *                  to be used in a subsequent call to ''fmGetRouteNext''.
 *
 * \param[out]      firstRoute points to caller-allocated storage where this
 *                  function will store the first route.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NO_MORE if there are no routes in this switch.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmGetRouteFirst(fm_int         sw,
                          fm_voidptr *   searchToken,
                          fm_routeEntry *firstRoute)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intRouteEntry *    firstRouteCandidate;
    fm_bool               lockTaken = FALSE;
    fm_customTreeIterator iter;
    fm_intRouteEntry *    key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxRoutes <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    fmCustomTreeIterInit(&iter, &switchPtr->routeTree);

    err = fmCustomTreeIterNext( &iter,
                               (void **) &key,
                               (void **) &firstRouteCandidate );

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    if (firstRouteCandidate != NULL)
    {
        *firstRoute  = firstRouteCandidate->route;
        *searchToken = (fm_voidptr) firstRouteCandidate;
    }
    else
    {
        err = FM_ERR_NO_MORE;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouteFirst */




/*****************************************************************************/
/** fmGetRouteNext
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Retrieve the next route. The routes are sorted by VRID
 *                  and longest prefix match.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in,out]   searchToken points to caller-allocated storage of type
 *                  fm_voidptr that has been filled in by a prior call to
 *                  this function or to ''fmGetRouteFirst''. It will be updated
 *                  by this function with a new value to be used in a
 *                  subsequent call to this function.
 *
 * \param[out]      nextRoute points to caller-allocated storage where this
 *                  function will store the next route.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_NOT_FOUND if currentRoute is invalid.
 * \return          FM_ERR_NO_MORE if there are no more routes in the switch.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 *
 *****************************************************************************/
fm_status fmGetRouteNext(fm_int         sw,
                         fm_voidptr *   searchToken,
                         fm_routeEntry *nextRoute)
{
    fm_switch *           switchPtr;
    fm_status             err;
    fm_intRouteEntry *    workingRoute;
    fm_bool               lockTaken = FALSE;
    fm_customTreeIterator iter;
    fm_intRouteEntry *    key;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    if (switchPtr->maxRoutes <= 0)
    {
        err = FM_ERR_UNSUPPORTED;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    err = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    lockTaken = TRUE;

    workingRoute = (fm_intRouteEntry *) *searchToken;

    err = fmCustomTreeIterInitFromKey(&iter,
                                      &switchPtr->routeTree,
                                      workingRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* get the previous route */
    err = fmCustomTreeIterNext( &iter,
                               (void **) &key,
                               (void **) &workingRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* get the next route */
    err = fmCustomTreeIterNext( &iter,
                               (void **) &key,
                               (void **) &workingRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    if (workingRoute != NULL)
    {
        *nextRoute   = workingRoute->route;
        *searchToken = (fm_voidptr) workingRoute;
    }
    else
    {
        err = FM_ERR_NO_MORE;
    }


ABORT:

    if (lockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, err);

}   /* end fmGetRouteNext */



/*****************************************************************************/
/** fmIsUnicastIPAddress
 * \ingroup intRouter
 *
 * \desc            Returns TRUE if an IP Address is a unicast address.
 *
 * \param[in]       addr points to the IP Address to inspect.
 *
 * \return          TRUE if the address is a unicast address.
 * \return          FALSE if the address is a multicast address.
 *
 *****************************************************************************/
fm_bool fmIsUnicastIPAddress(fm_ipAddr *addr)
{
    fm_byte ipByte;
    fm_bool isUnicast = TRUE;

    /* High order byte is the byte of interest for both IPv4 and IPv6 */
    if (addr->isIPv6)
    {
        /* IPv6 case */
        ipByte = (ntohl(addr->addr[3]) >> 24) & 0xff;
        if (ipByte == 0xff)
        {
            isUnicast = FALSE;
        }
    }
    else
    {
        /* IPv4 case */
        ipByte = (ntohl(addr->addr[0]) >> 24) & 0xff;
        if (ipByte >= 224 && ipByte <= 239)
        {
            isUnicast = FALSE;
        }
    }

    return isUnicast;

}   /* end fmIsUnicastIPAddress */




/*****************************************************************************/
/** fmIsMulticastIPAddress
 * \ingroup intRouter
 *
 * \desc            Returns TRUE if an IP Address is a multicast address.
 *
 * \param[in]       addr points to the IP Address to inspect.
 *
 * \return          TRUE if the address is a multicast address.
 * \return          FALSE if the address is a unicast address.
 *
 *****************************************************************************/
fm_bool fmIsMulticastIPAddress(fm_ipAddr *addr)
{
    fm_byte ipByte;
    fm_bool isMulticast = FALSE;

    /* High order byte is the byte of interest for both IPv4 and IPv6 */
    /* Must convert to host order since addr is defined as network order */

    if (addr->isIPv6)
    {
        /* IPv6 case */
        ipByte = (ntohl(addr->addr[3]) >> 24) & 0xff;
        if (ipByte == 0xff)
        {
            isMulticast = TRUE;
        }
    }
    else
    {
        /* IPv4 case */
        ipByte = (ntohl(addr->addr[0]) >> 24) & 0xff;
        if (ipByte >= 224 && ipByte <= 239)
        {
            isMulticast = TRUE;
        }
    }

    return isMulticast;

}   /* end fmIsMulticastIPAddress */




/*****************************************************************************/
/** fmIsRouteEntryUnicast
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns TRUE if a route entry is a unicast entry.
 *
 * \param[in]       route points to the route entry to inspect.
 *
 * \return          TRUE if the destination address is a unicast address.
 * \return          FALSE if the destination address is a multicast address.
 *
 *****************************************************************************/
fm_bool fmIsRouteEntryUnicast(fm_routeEntry *route)
{
    fm_bool isUnicast;

    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
        case FM_ROUTE_TYPE_UNICAST_ECMP:
            isUnicast = TRUE;
            break;

        default:
            isUnicast = FALSE;
            break;
    }

    return isUnicast;

}   /* end fmIsRouteEntryUnicast */




/*****************************************************************************/
/** fmIsRouteEntryMulticast
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns TRUE if a route entry is a multicast entry.
 *
 * \param[in]       route points to the route entry to inspect.
 *
 * \return          TRUE if the destination address is a multicast address.
 * \return          FALSE if the destination address is a unicast address.
 *
 *****************************************************************************/
fm_bool fmIsRouteEntryMulticast(fm_routeEntry *route)
{
    fm_bool isMulticast;

    if (route->routeType == FM_ROUTE_TYPE_MULTICAST)
    {
        isMulticast = TRUE;
    }
    else
    {
        isMulticast = FALSE;
    }

    return isMulticast;

}   /* end fmIsRouteEntryMulticast */




/*****************************************************************************/
/** fmIsRouteEntrySGMulticast
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns TRUE if a route entry is an (S,G) multicast entry.
 *
 * \param[in]       route points to the route entry to inspect.
 *
 * \return          TRUE if the route entry is an (S,G) multicast entry.
 * \return          FALSE if the route entry is a unicast or (*,G)
 *                  multicast entry.
 *
 *****************************************************************************/
fm_bool fmIsRouteEntrySGMulticast(fm_routeEntry *route)
{
    fm_bool isSGMulticast;

    if ( !fmIsRouteEntryMulticast(route) )
    {
        return FALSE;
    }

    switch (route->data.multicast.addressType)
    {
        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
            isSGMulticast = TRUE;
            break;

        default:
            isSGMulticast = FALSE;
            break;
    }

    return isSGMulticast;

}   /* end fmIsRouteEntrySGMulticast */




/*****************************************************************************/
/** fmCompareIPAddresses
 * \ingroup intRouter
 *
 * \desc            Compare IP Addresses.
 *
 * \param[in]       first points to the first IP Address.
 *
 * \param[in]       second points to the second IP Address.
 *
 * \return          -1 if the first IP Address sorts before the second.
 * \return           0 if the IP Addresses are identical.
 * \return           1 if the first IP Address sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareIPAddresses(fm_ipAddr *first, fm_ipAddr *second)
{
    fm_int retval = 0;
    fm_int i;

    if (first->isIPv6 == second->isIPv6)
    {
        /* addresses are same type */
        if (first->isIPv6)
        {
            /* IPv6 checks all words */
            for (i = 0 ; i < 4 ; i++)
            {
                if (first->addr[i] < second->addr[i])
                {
                    retval = -1;
                    break;
                }
                else if (first->addr[i] > second->addr[i])
                {
                    retval = 1;
                    break;
                }
            }
        }
        else
        {
            /* IPv4 just checks one word */
            if (first->addr[0] < second->addr[0])
            {
                retval = -1;
            }
            else if (first->addr[0] > second->addr[0])
            {
                retval = 1;
            }
        }
    }
    else
    {
        if (first->isIPv6)
        {
            /* second address must be IPv4, sort it before first address */
            retval = 1;
        }
        else
        {
            /* second address must be IPv6, sort it after first address */
            retval = -1;
        }
    }

    return retval;

}   /* end fmCompareIPAddresses */


/*****************************************************************************/
/** fmCompareRoutes
 * \ingroup intRouter
 *
 * \desc            Compare Route entries.
 *
 * \param[in]       first points to the first route.
 *
 * \param[in]       second points to the second route.
 *
 * \return          -1 if the first route sorts before the second.
 * \return           0 if the routes are identical.
 * \return           1 if the first route sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareRoutes(fm_routeEntry *first, fm_routeEntry *second)
{
    return CompareRoutes(first, second, FALSE);

}   /* end fmCompareRoutes */


/*****************************************************************************/
/** fmCompareEcmpRoutes
 * \ingroup intRouter
 *
 * \desc            Compare Route entries to determine if they are part
 *                  of the same ECMP group.
 *
 * \param[in]       first points to the first route.
 *
 * \param[in]       second points to the second route.
 *
 * \return          -1 if the routes are not part of the same ECMP group
 *                  and the first route sorts before the second.
 * \return          0 if the routes are part of the same ECMP group.
 * \return          1 if the routes are not part of the same ECMP group
 *                  and the first route sorts after the second.
 *
 *****************************************************************************/
fm_int fmCompareEcmpRoutes(fm_routeEntry *first, fm_routeEntry *second)
{
    return CompareRoutes(first, second, TRUE);

}   /* fmCompareRoutes */



/*****************************************************************************/
/** fmSetRouteActiveFlag
 * \ingroup intRouterRoute
 *
 * \desc            Determine whether a route should be active or not.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in,out]   routeEntry points to the route entry.  The "active" field
 *                  is updated by this function.
 *
 * \param[in]       updateHardware is TRUE if the router hardware should
 *                  be updated.  If TRUE, the route MUST have been already
 *                  added to the router hardware, i.e., switchPtr->AddRoute
 *                  must have already been called for this route.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmSetRouteActiveFlag(fm_int            sw,
                               fm_intRouteEntry *routeEntry,
                               fm_bool           updateHardware)
{
    fm_switch *             switchPtr;
    fm_bool                 active;
    fm_int                  vrid;
    fm_int                  vroff;
    fm_status               err;
    fm_bool                 wasActive;
    fm_intMulticastGroup *  group;
    fm_int                  ecmpGroupId;
    fm_intEcmpGroup *       ecmpGroup;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, routeEntry = %p, updateHardware = %d\n",
                 sw,
                 (void *) routeEntry,
                 (fm_int) updateHardware);

    switchPtr = GET_SWITCH_PTR(sw);

    wasActive = routeEntry->active;

    active = TRUE;

    switch (routeEntry->route.routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
        case FM_ROUTE_TYPE_UNICAST_ECMP:
            if (routeEntry->route.routeType == FM_ROUTE_TYPE_UNICAST)
            {
                vrid = routeEntry->route.data.unicast.vrid;
            }
            else
            {
                vrid = routeEntry->route.data.unicastECMP.vrid;
            }

            ecmpGroupId = routeEntry->ecmpGroupId;

            if (ecmpGroupId >= 0)
            {
                ecmpGroup = switchPtr->ecmpGroups[ecmpGroupId];

                if (!ecmpGroup->isUsable)
                {
                    active = FALSE;
                }
            }
            else if ( (routeEntry->action.action != FM_ROUTE_ACTION_NOP)
                && (routeEntry->action.action != FM_ROUTE_ACTION_RPF_FAILURE) )
            {
                active = FALSE;
            }
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            vrid = 0;
            /* If the multicast group is not attached to an address,
             * don't route
             */
            group = fmFindMcastGroup(sw,
                                     routeEntry->route.data.multicast.mcastGroup);

            if (group == NULL)
            {
                active = FALSE;
            }
            break;

        default:
            active = FALSE;
            vrid   = 0;
            break;
    }

    err = fmValidateVirtualRouterId(sw, vrid, &vroff);

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    }

    if (switchPtr->virtualRouterStates[vroff] != FM_ROUTER_STATE_ADMIN_UP)
    {
        active = FALSE;
    }

    if (routeEntry->state != FM_ROUTE_STATE_UP)
    {
        active = FALSE;
    }

    routeEntry->active = active;

    if ( updateHardware
        && (switchPtr->SetRouteActive != NULL)
        && (routeEntry->active != wasActive) )
    {
        err = switchPtr->SetRouteActive(sw, routeEntry);
    }
    else
    {
        err = FM_OK;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmSetRouteActiveFlag */




/*****************************************************************************/
/** fmDbgConvertIPAddressToString
 * \ingroup intDebug
 *
 * \desc            Convert an IP address into a string.
 *
 * \param[in]       ipAddr points to the IP Address to be converted.
 *
 * \param[in]       textOut points to a buffer for the string.  This buffer
 *                  must be large enough for the largest possible IP address
 *                  string.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fmDbgConvertIPAddressToString(const fm_ipAddr *ipAddr, fm_text textOut)
{
    fm_int    i;
    fm_uint32 segment1;
    fm_uint32 segment2;
    fm_char   tempString2[100];
    fm_uint   v4Word;
    fm_uint   v4Byte1;
    fm_uint   v4Byte2;
    fm_uint   v4Byte3;
    fm_uint   v4Byte4;
    fm_ipAddr tempAddr = *ipAddr;

    /**************************************************
     * For IPv6 address: 32 digits + 7 colons + 1 NUL = 40
     **************************************************/
    const fm_uint textOutLength = 40;

    /* First convert address to host byte order */
    if (ipAddr->isIPv6)
    {
        for (i = 3; i >= 0; i--)
        {
            tempAddr.addr[i] = ntohl(ipAddr->addr[i]);
        }
    }
    else
    {
        tempAddr.addr[0] = ntohl(ipAddr->addr[0]);
    }

    if (tempAddr.isIPv6)
    {
        *textOut = 0;

        for (i = 3 ; i >= 0 ; i--)
        {
            segment1 = (tempAddr.addr[i] >> 16) & 0xffff;
            segment2 = tempAddr.addr[i] & 0xffff;
            FM_SNPRINTF_S(tempString2, sizeof(tempString2),
                          "%04X:%04X", segment1, segment2);

            if (i != 0)
            {
                fmStringAppend(tempString2, ":", sizeof(tempString2));
            }

            fmStringAppend(textOut, tempString2, textOutLength);
        }
    }
    else
    {
        v4Word  = *( (fm_uint *) &tempAddr.addr[0] );
        v4Byte1 = (v4Word >> 24) & 0xFF;
        v4Byte2 = (v4Word >> 16) & 0xFF;
        v4Byte3 = (v4Word >> 8) & 0xFF;
        v4Byte4 = v4Word & 0xFF;
        FM_SNPRINTF_S(tempString2, sizeof(tempString2),
                      "%u.%u.%u.%u",
                      v4Byte1,
                      v4Byte2,
                      v4Byte3,
                      v4Byte4);
        fmStringCopy(textOut, tempString2, textOutLength);
    }

}   /* end fmDbgConvertIPAddressToString */




/*****************************************************************************/
/** fmDbgBuildMulticastDescription
 * \ingroup intDebug
 *
 * \desc            Convert a multicast address entry into a text string.
 *
 * \param[in]       mcastPtr points to the multicast address entry.
 *
 * \param[in]       textOut points to a 200-byte buffer for the string.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fmDbgBuildMulticastDescription(fm_multicastAddress *mcastPtr, fm_text textOut)
{
    fm_char       destAddr[40];
    fm_char       srcAddr[40];
    fm_ipAddr *   destIpAddr;
    fm_ipAddr *   srcIpAddr;
    fm_char       ipvFlag;
    fm_char       macAddr[40];
    const fm_uint textOutLength = 200;

    switch (mcastPtr->addressType)
    {
        case FM_MCAST_ADDR_TYPE_UNKNOWN:
            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "%s",
                          "Multicast Address Type == UNKNOWN");
            break;

        case FM_MCAST_ADDR_TYPE_L2MAC_VLAN:
            fmDbgConvertMacAddressToString(
                mcastPtr->info.mac.destMacAddress, macAddr);
            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "MAC/VLAN: macAddr=%s, vlan=%d/vlan2=%d",
                          macAddr,
                          mcastPtr->info.mac.vlan,
                          mcastPtr->info.mac.vlan2);
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP:
            destIpAddr = &mcastPtr->info.dstIpRoute.dstAddr;

            if (destIpAddr->isIPv6)
            {
                ipvFlag = '6';
            }
            else
            {
                ipvFlag = '4';
            }

            fmDbgConvertIPAddressToString(destIpAddr, destAddr);

            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "IPv%cM: dest=%s/%d",
                          ipvFlag,
                          destAddr,
                          mcastPtr->info.dstIpRoute.dstPrefixLength);
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
            destIpAddr = &mcastPtr->info.dstIpVlanRoute.dstAddr;

            if (destIpAddr->isIPv6)
            {
                ipvFlag = '6';
            }
            else
            {
                ipvFlag = '4';
            }

            fmDbgConvertIPAddressToString(destIpAddr, destAddr);

            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "IPv%cM: dest=%s/%d vlan=%d/%d",
                          ipvFlag,
                          destAddr,
                          mcastPtr->info.dstIpVlanRoute.dstPrefixLength,
                          mcastPtr->info.dstIpVlanRoute.vlan,
                          mcastPtr->info.dstIpVlanRoute.vlanPrefixLength);
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
            destIpAddr = &mcastPtr->info.dstSrcIpRoute.dstAddr;
            srcIpAddr  = &mcastPtr->info.dstSrcIpRoute.srcAddr;

            if (destIpAddr->isIPv6)
            {
                ipvFlag = '6';
            }
            else
            {
                ipvFlag = '4';
            }

            fmDbgConvertIPAddressToString(destIpAddr, destAddr);
            fmDbgConvertIPAddressToString(srcIpAddr, srcAddr);

            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "IPv%cM: dest=%s/%d src=%s/%d",
                          ipvFlag,
                          destAddr,
                          mcastPtr->info.dstSrcIpRoute.dstPrefixLength,
                          srcAddr,
                          mcastPtr->info.dstSrcIpRoute.srcPrefixLength);
            break;

        case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
            destIpAddr = &mcastPtr->info.dstSrcIpVlanRoute.dstAddr;
            srcIpAddr  = &mcastPtr->info.dstSrcIpVlanRoute.srcAddr;

            if (destIpAddr->isIPv6)
            {
                ipvFlag = '6';
            }
            else
            {
                ipvFlag = '4';
            }

            fmDbgConvertIPAddressToString(destIpAddr, destAddr);
            fmDbgConvertIPAddressToString(srcIpAddr, srcAddr);

            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "IPv%cM: dest=%s/%d src=%s/%d vlan=%d/%d",
                          ipvFlag,
                          destAddr,
                          mcastPtr->info.dstSrcIpVlanRoute.dstPrefixLength,
                          srcAddr,
                          mcastPtr->info.dstSrcIpVlanRoute.srcPrefixLength,
                          mcastPtr->info.dstSrcIpVlanRoute.vlan,
                          mcastPtr->info.dstSrcIpVlanRoute.vlanPrefixLength);
            break;

        default:
            FM_SNPRINTF_S(textOut,
                          textOutLength,
                          "Unknown Multicast Address Type %d",
                          mcastPtr->addressType);
            break;

    }   /* end switch (mcastPtr->addressType) */

}   /* end fmDbgBuildMulticastDescription */




/*****************************************************************************/
/** fmDbgBuildRouteDescription
 * \ingroup intDebug
 *
 * \desc            Convert a route entry into a text string.
 *
 * \param[in]       route points to the route entry.
 *
 * \param[in]       textOut points to a 200-byte buffer for the string.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fmDbgBuildRouteDescription(fm_routeEntry *route, fm_text textOut)
{
    fm_char                   destAddr[40];
    fm_char                   nextHop[40];
    fm_char                   intfIP[40];
    fm_unicastRouteEntry *    unicast;
    fm_unicastECMPRouteEntry *unicastEcmp;
    fm_ipAddr                 destIpAddr;
    fm_char                   ipvFlag;
    const fm_uint             textOutLength = 200;

    fmGetRouteDestAddress(route, &destIpAddr);

    if (destIpAddr.isIPv6)
    {
        ipvFlag = '6';
    }
    else
    {
        ipvFlag = '4';
    }

    if (route->routeType == FM_ROUTE_TYPE_UNICAST)
    {
        fmDbgConvertIPAddressToString(&destIpAddr, destAddr);

        unicast = &route->data.unicast;
        fmDbgConvertIPAddressToString(&unicast->nextHop, nextHop);
        fmDbgConvertIPAddressToString(&unicast->interfaceAddr, intfIP);

        FM_SNPRINTF_S(textOut, textOutLength,
                      "IPv%cU: dest=%s/%d nh=%s if=%s vlan=%u vrid=%d",
                      ipvFlag,
                      destAddr,
                      unicast->prefixLength,
                      nextHop,
                      intfIP,
                      unicast->vlan,
                      unicast->vrid);
    }

    else if (route->routeType == FM_ROUTE_TYPE_UNICAST_ECMP)
    {
        fmDbgConvertIPAddressToString(&destIpAddr, destAddr);

        unicastEcmp = &route->data.unicastECMP;

        FM_SNPRINTF_S(textOut, textOutLength,
                      "IPv%cU: dest=%s/%d ecmpGroup=%d vrid=%d",
                      ipvFlag,
                      destAddr,
                      unicastEcmp->prefixLength,
                      unicastEcmp->ecmpGroup,
                      unicastEcmp->vrid);
    }

    else
    {
        fmDbgBuildMulticastDescription(&route->data.multicast, textOut);
    }

}   /* end fmDbgBuildRouteDescription */



/*****************************************************************************/
/** fmConvertPrefixLengthToDestMask
 * \ingroup intRouter
 *
 * \desc            Convert a destination route prefix length into a
 *                  destination mask.
 *
 * \param[in]       prefixLength is the desired prefix length.
 *
 * \param[in]       isIPv6 is TRUE if the destination mask needs to be
 *                  set for an IPV6 address, FALSE for an IPV4 address.
 *
 * \param[out]      destMask points to a buffer for the destination mask.
 *                  For IPV6, this must be 128 bits long, for IPv4, 32 bits.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fmConvertPrefixLengthToDestMask(fm_int     prefixLength,
                                     fm_bool    isIPv6,
                                     fm_uint32 *destMask)
{
    fm_int curOffset;
    fm_int curLen;

    if (isIPv6)
    {
        curOffset = 3;

        if (prefixLength > FM_IPV6_MAX_PREFIX_LENGTH)
        {
            prefixLength = FM_IPV6_MAX_PREFIX_LENGTH;
        }
    }
    else
    {
        curOffset = 0;

        if (prefixLength > FM_IPV4_MAX_PREFIX_LENGTH)
        {
            prefixLength = FM_IPV4_MAX_PREFIX_LENGTH;
        }
    }

    while (prefixLength > 0)
    {
        if (prefixLength > 32)
        {
            curLen = 32;
        }
        else
        {
            curLen = prefixLength;
        }

        destMask[curOffset] = ~0 & ~( ( 1 << (32 - curLen) ) - 1 );
        prefixLength       -= curLen;
        curOffset--;
    }

    while (curOffset >= 0)
    {
        destMask[curOffset--] = 0;
    }

}   /* end fmConvertPrefixLengthToDestMask */




/*****************************************************************************/
/** fmMaskIPAddress
 * \ingroup intRouter
 *
 * \desc            Masks an IP Address, given the desired prefix length.
 *
 * \param[in,out]   ipAddr points to the IP address to be masked.
 *
 * \param[in]       prefixLength is the desired prefix length.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fmMaskIPAddress(fm_ipAddr *ipAddr, fm_int prefixLength)
{
    fm_int    i;
    fm_int    curPrefix;
    fm_uint64 mask64;
    fm_uint32 mask;
    fm_ipAddr tempAddr;
    fm_int    j;

    if (ipAddr->isIPv6)
    {

        if (prefixLength > FM_IPV6_MAX_PREFIX_LENGTH)
        {
            prefixLength = FM_IPV6_MAX_PREFIX_LENGTH;
        }

        curPrefix = prefixLength;

        for (i = 3, j = 0 ; i >= 0 ; i--, j++)
        {
            tempAddr.addr[i] = ntohl(ipAddr->addr[j]);
        }

        for (i = 0 ; i < 4 ; i++)
        {
            if (curPrefix >= 32)
            {
                mask = ~0;
                curPrefix -= 32;
            }
            else if (curPrefix > 0)
            {
                mask64 = ~( ( 1LL << (32 - curPrefix) ) - 1 );
                mask = (fm_uint32) mask64;
                curPrefix = 0;
            }
            else
            {
                mask = 0;
            }

            tempAddr.addr[i] &= mask;
        }

        for (i = 3, j = 0 ; i >= 0 ; i--, j++)
        {
            ipAddr->addr[i] = htonl(tempAddr.addr[j]);
        }
    }
    else
    {
        if (prefixLength > FM_IPV4_MAX_PREFIX_LENGTH)
        {
            prefixLength = FM_IPV4_MAX_PREFIX_LENGTH;
        }

        mask64 = ~( ( 1LL << (32 - prefixLength) ) - 1 );
        mask = (fm_uint32) mask64;
        ipAddr->addr[0] &= htonl(mask);
        ipAddr->addr[1] = 0;
        ipAddr->addr[2] = 0;
        ipAddr->addr[3] = 0;
    }

}   /* end fmMaskIPAddress */




/*****************************************************************************/
/** fmApplyIPAddressMask
 * \ingroup intRouter
 *
 * \desc            Masks an IP Address, given the desired IP mask.
 *
 * \param[in,out]   ipAddr points to the IP address to be masked.
 *
 * \param[in]       addrMask points to the mask.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fmApplyIPAddressMask(fm_ipAddr *ipAddr, fm_ipAddr *addrMask)
{
    fm_int    i;
    fm_ipAddr tempAddr;
    fm_ipAddr tempMask;
    fm_int    j;

    if (ipAddr->isIPv6)
    {
        for (i = 3, j = 0 ; i >= 0 ; i--, j++)
        {
            tempAddr.addr[i] = ntohl(ipAddr->addr[j]);
            tempMask.addr[i] = ntohl(addrMask->addr[j]);
        }

        for (i = 0 ; i < 4 ; i++)
        {
            tempAddr.addr[i] &= tempMask.addr[i];
        }

        for (i = 3, j = 0 ; i >= 0 ; i--, j++)
        {
            ipAddr->addr[i] = htonl(tempAddr.addr[j]);
        }
    }
    else
    {
        ipAddr->addr[0] &= addrMask->addr[0];
        ipAddr->addr[1] = 0;
        ipAddr->addr[2] = 0;
        ipAddr->addr[3] = 0;
    }

}   /* end fmApplyIPAddressMask */




/*****************************************************************************/
/** fmIsIPAddressInRangeMask
 * \ingroup intRouter
 *
 * \desc            Determines if an IP address is in a range of IP addresses.
 *
 * \param[in]       baseAddr points to the base address in the range.
 *
 * \param[in]       addrMask points to the range mask.
 *
 * \param[in]       targetAddr points to the address to be checked.
 *
 * \return          TRUE if the target address is within the range, FALSE if not.
 *
 *****************************************************************************/
fm_bool fmIsIPAddressInRangeMask(fm_ipAddr *baseAddr,
                                 fm_ipAddr *addrMask,
                                 fm_ipAddr *targetAddr)
{
    fm_ipAddr maskedBase;
    fm_ipAddr maskedTarget;
    fm_int    i;

    maskedBase   = *baseAddr;
    maskedTarget = *targetAddr;

    fmApplyIPAddressMask(&maskedBase, addrMask);
    fmApplyIPAddressMask(&maskedTarget, addrMask);

    i = fmCompareIPAddresses(&maskedBase, &maskedTarget);

    return (i == 0);

}   /* end fmIsIPAddressInRangeMask */




/*****************************************************************************/
/** fmApplyMasksToRoute
 * \ingroup intRouter
 *
 * \desc            Applies prefix masks to route values so that they
 *                  are all internally consistent. For instance, an IP
 *                  address such as 1.1.1.5/24 would be masked to 1.1.1.0/24.
 *
 * \param[in]       route points to the route entry.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmApplyMasksToRoute(fm_routeEntry *route)
{
    fm_status status;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING, "route = %p\n", (void *) route );

    switch (route->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            fmMaskIPAddress(&route->data.unicast.dstAddr,
                            route->data.unicast.prefixLength);
            break;

        case FM_ROUTE_TYPE_UNICAST_ECMP:
            fmMaskIPAddress(&route->data.unicastECMP.dstAddr,
                            route->data.unicastECMP.prefixLength);
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            status = fmApplyMasksToMulticastAddress(&route->data.multicast);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            break;

        default:
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fmApplyMasksToRoute */


/*****************************************************************************/
/** fmGetVirtualRouterOffset
 * \ingroup intRouter
 *
 * \desc            Finds a virtual router id in the table and returns its
 *                  offset.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vrid is the virtual router id number.
 *
 * \return          offset into the virtual router table, -1 if not found.
 *
 *****************************************************************************/
fm_int fmGetVirtualRouterOffset(fm_int sw, fm_int vrid)
{
    fm_switch *switchPtr;
    fm_int     vroff;

    switchPtr = GET_SWITCH_PTR(sw);

    if (vrid == FM_ROUTER_ANY)
    {
        return 0;
    }

    for (vroff = 0 ; vroff < switchPtr->maxVirtualRouters ; vroff++)
    {
        if (switchPtr->virtualRouterIds[vroff] == vrid)
        {
            break;
        }
    }

    if (vroff >= switchPtr->maxVirtualRouters)
    {
        vroff = -1;
    }

    FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                 "sw=%d, vrid=%d, vroff=%d\n",
                 sw,
                 vrid,
                 vroff);

    return vroff;

}   /* end fmGetVirtualRouterOffset */




/*****************************************************************************/
/** fmDbgDumpRouteStats
 * \ingroup diagMisc
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Dump Router information (short version - see
 *                  ''fmDbgDumpRouteTables'' for the verbose version).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP  if the switch is  not running
 *
 *****************************************************************************/
fm_status fmDbgDumpRouteStats(fm_int sw)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH( sw );

    switchPtr = GET_SWITCH_PTR(sw);

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpRouteStats, sw);

    UNPROTECT_SWITCH( sw );
    return FM_OK;

}   /* end fmDbgDumpRouteStats */




/*****************************************************************************/
/** fmDbgDumpRouteTables
 * \ingroup diagMisc
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Dump Router information (verbose version - see
 *                  ''fmDbgDumpRouteStats'' for the short version).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flags contains a bit-field describing which internal
 *                  tables to dump.  See ''Router Debug Dump Flags'' for
 *                  a list of flag values.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP  if the switch is  not running
 * 
 *
 *****************************************************************************/
fm_status fmDbgDumpRouteTables(fm_int sw, fm_int flags)
{
    fm_switch *switchPtr;

    VALIDATE_AND_PROTECT_SWITCH( sw );

    switchPtr = GET_SWITCH_PTR( sw );

    FM_API_CALL_FAMILY_VOID(switchPtr->DbgDumpRouteTables, sw, flags);

    UNPROTECT_SWITCH( sw );
    return FM_OK;

}   /* end fmDbgDumpRouteTables */


/*****************************************************************************/
/** fmDbgConvertStringToIPAddress
 * \ingroup diagMisc
 *
 * \desc            Function to convert an IP Address string into ''fm_ipAddr''
 *                  format.
 *
 * \param[in]       ipString points to the string.
 *
 * \param[in]       forceIPv6 is TRUE to force an IPv6 address, even if the
 *                  string only contains an IPv4 address.
 *
 * \param[in]       ipAddr points to a structure which will be initialized
 *                  by this function.
 *
 * \return          TRUE if the address was successfully converted.
 * \return          FALSE if the address string could not be parsed.
 *
 *****************************************************************************/
fm_bool fmDbgConvertStringToIPAddress(const fm_char *ipString,
                                      fm_bool        forceIPv6,
                                      fm_ipAddr *    ipAddr)
{
    fm_bool   isIPv6         = FALSE;
    fm_bool   hasIPv4Suffix  = FALSE;
    fm_bool   hasDoubleColon = FALSE;
    fm_char   tempString[200];
    fm_int    i;
    fm_text   ptr;
    fm_uint32 tempIpv4;
    fm_int    segCount;
    fm_int    dblColonSeg;
    fm_int    dblColonSegLen = 0;
    fm_uint16 segments[8];
    fm_bool   haveSegDigit;
    fm_byte   cur;
    fm_text   lastColonPtr;
    fm_int    segOffset;
    fm_int    segVal;
    fm_bool   lastCharWasColon;

    /* Make sure the arguments are valid */
    if( ipAddr == NULL || ipString == NULL  )        
    {
        return FALSE;
    }

    FM_CLEAR(*ipAddr);

    i = strlen(ipString);

    if( i >= (fm_int) sizeof(tempString) )
    {
        return FALSE;
    }

    fmStringCopy(tempString, ipString, sizeof(tempString));
    ptr = tempString;

    if (forceIPv6)
    {
        ipAddr->isIPv6 = TRUE;
    }

    if (strchr(tempString, ':') != NULL)
    {
        isIPv6 = TRUE;
    }

    if (strchr(tempString, '.') != NULL)
    {
        hasIPv4Suffix = TRUE;
    }

    if (isIPv6)
    {
        ipAddr->isIPv6 = TRUE;
        segCount       = 0;
        dblColonSeg    = -1;
        FM_CLEAR(segments);
        haveSegDigit     = FALSE;
        lastColonPtr     = NULL;
        lastCharWasColon = FALSE;

        while (1)
        {
            cur = *ptr;

            if (cur == 0)
            {
                if (haveSegDigit)
                {
                    segCount++;
                }

                break;
            }

            if ( isxdigit(cur) )
            {
                if (segCount >= 8)
                {
                    return FALSE;
                }

                if (cur <= '9')
                {
                    cur -= '0';
                }
                else if (cur <= 'F')
                {
                    cur = cur - 'A' + 10;
                }
                else
                {
                    cur = cur - 'a' + 10;
                }

                segments[segCount] = (segments[segCount] << 4) | cur;
                haveSegDigit       = TRUE;
            }
            else if (cur == ':')
            {
                lastColonPtr = ptr;

                if (haveSegDigit)
                {
                    segCount++;
                    haveSegDigit     = FALSE;
                    lastCharWasColon = TRUE;
                }
                else if (lastCharWasColon)
                {
                    if (hasDoubleColon)
                    {
                        return FALSE;
                    }

                    hasDoubleColon = TRUE;
                    dblColonSeg    = segCount;
                    segCount++;
                }
                else
                {
                    lastCharWasColon = TRUE;
                }
            }
            else if (cur == '.')
            {
                if (haveSegDigit)
                {
                    ptr = lastColonPtr + 1;
                    break;
                }
            }

            ptr++;
        }

        if (hasDoubleColon)
        {
            dblColonSegLen = 8 - segCount;
            segCount       = 8;
        }

        if (segCount != 8)
        {
            if (!hasIPv4Suffix || segCount != 6)
            {
                return FALSE;
            }
        }

        segCount  = 0;
        segOffset = 0;

        while (segCount < 8)
        {
            i = segCount / 2;

            ipAddr->addr[i] = (ipAddr->addr[i] << 16) | segments[segOffset];
 
            if (segCount == dblColonSeg)
            {
                segCount += dblColonSegLen;
            }

            segCount++;
            segOffset++;
        }
    }

    if (hasIPv4Suffix)
    {
        tempIpv4     = 0;
        segCount     = 0;
        haveSegDigit = FALSE;
        segVal       = 0;

        while ( (cur = *ptr++) != 0 )
        {
            if ( isdigit(cur) )
            {
                if (segCount >= 4)
                {
                    return FALSE;
                }

                cur         -= '0';
                segVal       = (segVal * 10) + cur;
                haveSegDigit = TRUE;
            }
            else if (cur == '.')
            {
                if (haveSegDigit)
                {
                    if (segVal > 255)
                    {
                        return FALSE;
                    }

                    tempIpv4 |= segVal << ( 24 - (segCount * 8) );
                }

                haveSegDigit = FALSE;
                segCount++;
                segVal = 0;
            }
            else
            {
                return FALSE;
            }
        }

        if (haveSegDigit)
        {
            if (segVal > 255)
            {
                return FALSE;
            }

            tempIpv4 |= segVal << ( 24 - (segCount * 8) );
        }

        if (!ipAddr->isIPv6)
        {
            ipAddr->addr[0] = tempIpv4;
        }
        else
        {
            ipAddr->addr[3] = tempIpv4;
        }
    }

    /* Now convert address to network byte order */
    if (ipAddr->isIPv6)
    {
        fm_ipAddr tempAddr = *ipAddr;
        fm_int    i;
        fm_int    j;

        for (i = 3, j = 0 ; i >= 0 ; i--, j++)
        {
            ipAddr->addr[i] = htonl(tempAddr.addr[j]);
        }
    }
    else
    {
        ipAddr->addr[0] = htonl(ipAddr->addr[0]);
    }

    return TRUE;

}   /* end fmDbgConvertStringToIPAddress */




/*****************************************************************************/
/** fmDbgDumpRouteLookupTrees
 * \ingroup diagMisc
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Dumps out the contents of the route lookup tables for
 *                  a virtual router.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the virtual router ID number.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP  if the switch is not running
 *
 *
 *****************************************************************************/
fm_status fmDbgDumpRouteLookupTrees(fm_int sw, fm_int vrid)
{
    fm_status             status;
    fm_int                prefix;
    fm_customTree *       lookupTree;
    fm_customTreeIterator iter;
    fm_ipAddr *           routeIP;
    fm_intRouteEntry *    route;
    fm_char               routeDesc[1000];

    VALIDATE_AND_PROTECT_SWITCH( sw );

    for (prefix = FM_MAX_NUM_IP_PREFIXES - 1 ; prefix >= 0 ; prefix--)
    {
        status = fmGetRouteLookupTree(sw, vrid, prefix, &lookupTree);
        if (status != FM_OK)
        {
            FM_LOG_PRINT( "Error getting route lookup tree for prefix %d: %s\n",
                          prefix,
                          fmErrorMsg(status) );
            break;
        }

        if ( fmCustomTreeSize(lookupTree) != 0 )
        {
            fmCustomTreeIterInit(&iter, lookupTree);

            FM_LOG_PRINT("\nPrefix %d\n", prefix);

            while (1)
            {
                status = fmCustomTreeIterNext( &iter,
                                               (void **) &routeIP,
                                               (void **) &route );
                if (status == FM_ERR_NO_MORE)
                {
                    break;
                }
                else if (status != FM_OK)
                {
                    FM_LOG_PRINT( "Unexpected error from route lookup tree "
                                  "iterator for prefix %d: %s\n",
                                  prefix,
                                  fmErrorMsg(status) );
                }
                else
                {
                    fmDbgBuildRouteDescription(&route->route, routeDesc);
                    FM_LOG_PRINT("    %s\n", routeDesc);
                }
            }
        }
    }

    UNPROTECT_SWITCH( sw );

    return FM_OK;

}   /* end fmDbgDumpRouteLookupTrees */




/*****************************************************************************/
/** fmDbgDumpRouteForIP
 * \ingroup diagMisc
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Determines which route services a specified IP
 *                  address and dumps the route information to the
 *                  log.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       vrid is the virtual router ID number.
 *
 * \param[in]       ipAddr contains the IP address as a string.
 *
 * \return          FM_OK if successful
 * \return          FM_ERR_INVALID_SWITCH if the switch ID is invalid
 * \return          FM_ERR_SWITCH_NOT_UP  if the switch is  not running
 *
 *
 *****************************************************************************/
fm_status fmDbgDumpRouteForIP(fm_int sw, fm_int vrid, fm_text ipAddr)
{
    fm_ipAddr         ip;
    fm_status         status;
    fm_intRouteEntry *route;
    fm_char           routeDesc[1000];

    VALIDATE_AND_PROTECT_SWITCH( sw );

    fmDbgConvertStringToIPAddress(ipAddr, FALSE, &ip);

    status = fmGetIntRouteForIP(sw, vrid, &ip, &route);
    if (status != FM_OK)
    {
        FM_LOG_PRINT( "fmGetIntRouteForIP returned error %s\n", fmErrorMsg(status) );
    }
    else
    {
        fmDbgBuildRouteDescription(&route->route, routeDesc);
        FM_LOG_PRINT("%s\n", routeDesc);
    }

    UNPROTECT_SWITCH( sw );

    return FM_OK;

}   /* end fmDbgDumpRouteForIP */




/*****************************************************************************/
/** fmDbgValidateRouteTables
 * \ingroup intDebug
 *
 * \desc            Function to validate internal consistency of the routing
 *                  tables.
 *
 * \param[in]       sw contains the switch number.
 *
 * \return          FM_OK if the route tables are internally consistent.
 * \return          FM_FAIL if the route tables are not internally consistent.
 *
 *****************************************************************************/
fm_status fmDbgValidateRouteTables(fm_int sw)
{
    fm_switch *             switchPtr;
    fm_status               err = FM_FAIL;
    fm_int                  numRoutes;
    fm_intRouteEntry *      prevRoute;
    fm_intRouteEntry *      curRoute;
    fm_intRouteEntry *      routeKey;
    fm_customTreeIterator   iter;
    fm_int                  i;
    fm_char                 curRouteDesc[500];
    fm_char                 prevRouteDesc[500];
    fm_int                  index;
    fm_bool                 bitValue;
// marka    fm_int                  numArps;
// marka    fm_intArpEntry *        prevArp;
// marka    fm_intArpEntry *        curArp;
// marka    fm_intArpEntry *        arpKey;
// marka    fm_intArpEntry *        arpValue;
    fm_char                 tempText[200];
    fm_char                 tempText2[200];
    fm_char                 curArpDesc[500];
    fm_char                 prevArpDesc[500];
    fm_intIpInterfaceEntry *ipIfEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /**********************************************************************
     *  Validate the Route Table
     *********************************************************************/
    if (switchPtr->maxRoutes > 0)
    {
        err = fmCustomTreeValidate(&switchPtr->routeTree);

        if (err != FM_OK)
        {
            FM_LOG_PRINT( "fmDbgValidateRouteTables Validation Failed: "
                          "route custom tree validation returned "
                          "error %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
            goto ABORT;
        }

        i = fmCustomTreeSize(&switchPtr->routeTree);

        if ( (i < 0) || (i > switchPtr->maxRoutes) )
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "custom tree contains %d routes, max = %d\n",
                         i,
                         switchPtr->maxRoutes);
            goto ABORT;
        }

        err = fmCustomTreeValidate(&switchPtr->ecmpRouteTree);

        if (err != FM_OK)
        {
            FM_LOG_PRINT( "fmDbgValidateRouteTables Validation Failed: "
                          "ecmp route custom tree validation returned "
                          "error %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
            goto ABORT;
        }

        i = fmCustomTreeSize(&switchPtr->ecmpRouteTree);

        if ( (i < 0) || (i > switchPtr->maxRoutes) )
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "ecmp custom tree contains %d routes, max = %d\n",
                         i,
                         switchPtr->maxRoutes);
            goto ABORT;
        }

        fmCustomTreeIterInit(&iter, &switchPtr->routeTree);
        numRoutes = 0;
        err       = fmCustomTreeIterNext(&iter,
                                         (void **) &routeKey,
                                         (void **) &curRoute);
        prevRoute = NULL;

        while ( (err == FM_OK) && (curRoute != NULL) )
        {
            if (numRoutes > switchPtr->maxRoutes)
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "too many routes, max = %d\n",
                             switchPtr->maxRoutes);
                goto ABORT;
            }

            if (err != FM_OK)
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "route %d fmCustomTreeIterNext err = %d (%s), "
                             "curRoute = %p\n",
                             numRoutes,
                             err,
                             fmErrorMsg(err),
                             (void *) curRoute);
                goto ABORT;
            }

            if (curRoute != routeKey)
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "route %d, custom tree key = %p value = %p\n",
                             numRoutes,
                             (void *) routeKey,
                             (void *) curRoute);
                goto ABORT;
            }

            if (prevRoute != NULL)
            {
                i = CompareRoutes(&prevRoute->route, &curRoute->route, FALSE);

                if (i == 0)
                {
                    fmDbgBuildRouteDescription(&prevRoute->route,
                                               prevRouteDesc);
                    fmDbgBuildRouteDescription(&curRoute->route,
                                               curRouteDesc);
                    FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                                 "route %d, invalid sort order\n"
                                 "    previous route = %s\n"
                                 "    current route  = %s\n",
                                 numRoutes,
                                 prevRouteDesc,
                                 curRouteDesc);
                    goto ABORT;
                }
                else if (i > 0)
                {
                    fmDbgBuildRouteDescription(&prevRoute->route,
                                               prevRouteDesc);
                    fmDbgBuildRouteDescription(&curRoute->route,
                                               curRouteDesc);
                    FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                                 "route %d, invalid sort order\n"
                                 "    previous route = %s\n"
                                 "    current route  = %s\n",
                                 numRoutes,
                                 prevRouteDesc,
                                 curRouteDesc);
                    goto ABORT;
                }
            }

            /* move to the next route */
            numRoutes++;
            prevRoute = curRoute;
            err       = fmCustomTreeIterNext(&iter,
                                             (void **) &routeKey,
                                             (void **) &curRoute);
        }

        if (err == FM_OK)
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "end of route linked list with %d routes, but "
                         "custom tree contains additional routes\n",
                         numRoutes);
            goto ABORT;
        }

    }

    /* end if (switchPtr->maxRoutes > 0) */


    /**********************************************************************
     *  Validate the ARP Table
     *********************************************************************/
    if (switchPtr->maxArpEntries > 0)
    {
        if (err != FM_OK)
        {
            FM_LOG_PRINT( "fmDbgValidateRouteTables Validation Failed: "
                          "arp custom tree validation returned error "
                          "%d (%s)\n",
                         err,
                         fmErrorMsg(err) );
            goto ABORT;
        }

#if 0
/* marka arpTree was removed, check for an alternative solution */
 
        i = fmCustomTreeSize(&switchPtr->arpTree);

        if ( (i < 0) || (i > switchPtr->maxArpEntries) )
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "custom tree contains %d ARPs, max = %d\n",
                         i,
                         switchPtr->maxArpEntries);
            goto ABORT;
        }
  
        fmCustomTreeIterInit(&iter, &switchPtr->arpTree);
        numArps = 0;
        curArp  = fmGetFirstArp(switchPtr);
        err     = fmCustomTreeIterNext(&iter,
                                       (void **) &arpKey,
                                       (void **) &arpValue);
        prevArp = NULL;
  
        while (curArp != NULL)
        {
            if (numArps > switchPtr->maxArpEntries)
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "too many ARPs, max = %d\n",
                             switchPtr->maxArpEntries);
                goto ABORT;
            }
  
            if (err != FM_OK)
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "arp %d fmCustomTreeIterNext err = %d (%s), "
                             "curArp = %p\n",
                             numArps,
                             err,
                             fmErrorMsg(err),
                             (void *) curArp);
                goto ABORT;
            }
  
            if ( (curArp != arpKey) || (curArp != arpValue) )
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "arp %d, linked list = %p, custom tree "
                             "key = %p value = %p\n",
                             numArps,
                             (void *) curArp,
                             (void *) arpKey,
                             (void *) arpValue);
                goto ABORT;
            }
  
            if (prevArp != NULL)
            {
                i = fmCompareInternalArps(&prevArp->arp, &curArp->arp);
  
                if (i == 0)
                {
                    fmDbgConvertIPAddressToString(&prevArp->arp.ipAddr,
                                                  tempText);
                    fmDbgConvertMacAddressToString(prevArp->arp.macAddr,
                                                   tempText2);
                    FM_SNPRINTF_S(prevArpDesc, sizeof(prevArpDesc),
                                  "addr = %s, interface = %d, vlan = %u, "
                                  "macAddr = %s",
                                  tempText,
                                  prevArp->arp.interface,
                                  (fm_uint) prevArp->arp.vlan,
                                  tempText2);
  
                    fmDbgConvertIPAddressToString(&curArp->arp.ipAddr,
                                                  tempText);
                    fmDbgConvertMacAddressToString(curArp->arp.macAddr,
                                                   tempText2);
                    FM_SNPRINTF_S(curArpDesc, sizeof(curArpDesc),
                                  "addr = %s, interface = %d, vlan = %u, "
                                  "macAddr = %s",
                                  tempText,
                                  curArp->arp.interface,
                                  (fm_uint) curArp->arp.vlan,
                                  tempText2);
  
                    FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                                 "arp %d, invalid sort order\n"
                                 "    previous arp = %s\n"
                                 "    current arp  = %s\n",
                                 numArps,
                                 prevArpDesc,
                                 curArpDesc);
                    goto ABORT;
                }
                else if (i > 0)
                {
                    fmDbgConvertIPAddressToString(&prevArp->arp.ipAddr,
                                                  tempText);
                    fmDbgConvertMacAddressToString(prevArp->arp.macAddr,
                                                   tempText2);
                    FM_SNPRINTF_S(prevArpDesc, sizeof(prevArpDesc),
                                  "addr = %s, interface = %d, vlan = %u, "
                                  "macAddr = %s",
                                  tempText,
                                  prevArp->arp.interface,
                                  (fm_uint) prevArp->arp.vlan,
                                  tempText2);
  
                    fmDbgConvertIPAddressToString(&curArp->arp.ipAddr,
                                                  tempText);
                    fmDbgConvertMacAddressToString(curArp->arp.macAddr,
                                                   tempText2);
                    FM_SNPRINTF_S(curArpDesc, sizeof(curArpDesc),
                                  "addr = %s, interface = %d, vlan = %u, "
                                  "macAddr = %s",
                                  tempText,
                                  curArp->arp.interface,
                                  (fm_uint) curArp->arp.vlan,
                                  tempText2);
  
                    FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                                 "arp %d, invalid sort order\n"
                                 "    previous arp = %s\n"
                                 "    current arp  = %s\n",
                                 numArps,
                                 prevArpDesc,
                                 curArpDesc);
                    goto ABORT;
                }
            }
  
            /* move to the next arp */
            numArps++;
            prevArp = curArp;
            curArp  = fmGetNextArp(curArp);
            err     = fmCustomTreeIterNext(&iter,
                                           (void **) &arpKey,
                                           (void **) &arpValue);
        }
  
        if (err == FM_OK)
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "end of ARP linked list with %d ARPs, but "
                         "custom tree contains additional ARPs\n",
                         numArps);
            goto ABORT;
        }
  
    }
#endif

    }    /* end if (switchPtr->maxArpEntries > 0) */


    /**********************************************************************
     *  Validate the Interface Table
     *********************************************************************/
    if (switchPtr->maxIpInterfaces > 0)
    {
        if (switchPtr->ipInterfaceEntries == NULL)
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "maxIpInterfaces = %d but ipInterfaceEntries == "
                         "NULL\n",
                         switchPtr->maxIpInterfaces);
            goto ABORT;
        }

        for (index = 0 ; index < switchPtr->maxIpInterfaces ; index++)
        {
            ipIfEntry = switchPtr->ipInterfaceEntries + index;

            /* Retrieve the bit from the entries-in-use array */
            err = fmGetBitArrayBit(&switchPtr->ipInterfaceEntriesInUse,
                                   index,
                                   &bitValue);

            if (err != FM_OK)
            {
                FM_LOG_PRINT( "fmDbgValidateRouteTables Validation Failed: "
                              "ip I/F unable to read bit array index "
                              "%d, err=%d (%s)\n",
                             index,
                             err,
                             fmErrorMsg(err) );
                goto ABORT;
            }

            if ( ( !bitValue && (ipIfEntry->interfaceNum != -1) )
                || ( bitValue && (ipIfEntry->interfaceNum == -1) ) )
            {
                FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                             "ip I/F %d (%p) interfaceNum is %d but the "
                             "in-use bit is %s set in the bit array\n",
                             index,
                             (void *) ipIfEntry,
                             ipIfEntry->interfaceNum,
                             (bitValue) ? "" : "not");
                goto ABORT;
            }

        }

        /* for (index = 0 ; ... ) */

    }

    /* end if (switchPtr->maxIpInterfaces > 0) */


    /**********************************************************************
     *  Validate the Virtual Router Table
     *********************************************************************/
    if (switchPtr->maxVirtualRouters > 0)
    {
        if (switchPtr->virtualRouterIds == NULL)
        {
            FM_LOG_PRINT("fmDbgValidateRouteTables Validation Failed: "
                         "maxVirtualRouters = %d but virtualRouterIds == "
                         "NULL\n",
                         switchPtr->maxVirtualRouters);
            goto ABORT;
        }

    }

    /* end if (switchPtr->maxVirtualRouters > 0) */

    /* validate consistency of the switch-specific routing tables */
    if (switchPtr->DbgValidateRouteTables != NULL)
    {
        err = switchPtr->DbgValidateRouteTables(sw);
    }
    else
    {
        err = FM_OK;
    }

ABORT:

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fmDbgValidateRouteTables */




/*****************************************************************************/
/** fmDbgGetRouteCount
 * \ingroup diagMisc
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Returns the number of routes for a switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[out]      countPtr points to caller-allocated storage where this
 *                  function should store the the number of routes.
 *
 * \return          FM_OK unconditionally.
 *
 *****************************************************************************/
fm_status fmDbgGetRouteCount(fm_int sw, fm_int *countPtr)
{
    fm_switch *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    *countPtr = (fm_int) fmCustomTreeSize(&switchPtr->routeTree);

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fmDbgGetRouteCount */


/*****************************************************************************/
/** fmSetRouteAttribute
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Sets an attribute for a specific route.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       route points to the route entry.
 *
 * \param[in]       attr contains the attribute ID (see ''Route Attributes'').
 *
 * \param[in]       value points to the attribute value to be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_NOT_FOUND if the route does not exist.
 * \return          FM_ERR_UNSUPPORTED if routing is not available on the switch.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 *
 *****************************************************************************/
fm_status fmSetRouteAttribute(fm_int         sw,
                              fm_routeEntry *route,
                              fm_int         attr,
                              void *         value)
{
    fm_switch *       switchPtr;
    fm_status         status;
    fm_status         status2;
    fm_intRouteEntry *intRoute;
    fm_customTree *   routeTree;
    fm_intRouteEntry  key;
    fm_bool           routeLockTaken = FALSE;
    fm_int            oldEcmpGroupId = -1;
    fm_int            newEcmpGroupId = -1;
    fm_intEcmpGroup * oldEcmpGroup = NULL;
    fm_intEcmpGroup * newEcmpGroup = NULL;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, route = %p, attr = %d, value = %p\n",
                     sw,
                     (void *) route,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Get exclusive access to routing tables */
    status = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    routeLockTaken = TRUE;

    /* Try to find the route */
    key.route = *route;

    routeTree = GetRouteTree(sw, route);

    if (routeTree == NULL)
    {
        status = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    status = fmCustomTreeFind(&switchPtr->routeTree,
                              &key,
                              (void **) &intRoute);

    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    switch (attr)
    {
        case FM_ROUTE_ECMP_GROUP:
            oldEcmpGroupId = intRoute->ecmpGroupId;

            /* Validate the new ECMP group */
            newEcmpGroupId = *(fm_int *) value;

            if ( (newEcmpGroupId < 0)
                || (newEcmpGroupId >= switchPtr->maxArpEntries) )
            {
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            }

            newEcmpGroup = switchPtr->ecmpGroups[newEcmpGroupId];

            if (newEcmpGroup == NULL)
            {
                status = FM_ERR_INVALID_ARGUMENT;
                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            }

            /* Remove the route from the old ECMP group */
            oldEcmpGroup = switchPtr->ecmpGroups[oldEcmpGroupId];

            status = fmCustomTreeRemove(&oldEcmpGroup->routeTree,
                                        intRoute,
                                        NULL);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

            /* Remove the route from the route tree. */
            status = fmCustomTreeRemove(&switchPtr->routeTree,
                                        intRoute,
                                        NULL);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

            /* Update the route's ECMP Group ID */
            intRoute->ecmpGroupId                      = newEcmpGroupId;
            intRoute->route.data.unicastECMP.ecmpGroup = newEcmpGroupId;

            /* Add the route to the new ECMP group */
            status = fmCustomTreeInsert(&newEcmpGroup->routeTree,
                                        intRoute,
                                        intRoute);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);

            /* Add the route to the route tree. */
            status = fmCustomTreeInsert(&switchPtr->routeTree,
                                        intRoute,
                                        intRoute);

            FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
            break;

        default:
            break;
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->SetRouteAttribute,
                       sw,
                       intRoute,
                       attr,
                       value);

    if (status != FM_OK)
    {
        switch (attr)
        {
            case FM_ROUTE_ECMP_GROUP:
                /* Remove the route from the new ECMP group */
                status2 = fmCustomTreeRemove(&newEcmpGroup->routeTree,
                                             intRoute,
                                             NULL);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status2);

                /* Remove the route from the route tree. */
                status2 = fmCustomTreeRemove(&switchPtr->routeTree,
                                             intRoute,
                                             NULL);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status2);

                /* Update the route's ECMP Group ID */
                intRoute->ecmpGroupId                      = oldEcmpGroupId;
                intRoute->route.data.unicastECMP.ecmpGroup = oldEcmpGroupId;

                /* Add the route to the old ECMP group */
                status2 = fmCustomTreeInsert(&oldEcmpGroup->routeTree,
                                             intRoute,
                                             intRoute);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status2);

                /* Add the route to the route tree. */
                status2 = fmCustomTreeInsert(&switchPtr->routeTree,
                                             intRoute,
                                             intRoute);

                FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status2);
                break;

            default:
                break;
        }

        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }


ABORT:

    if (routeLockTaken)
    {
        fmReleaseWriteLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmSetRouteAttribute */


/*****************************************************************************/
/** fmGetRouteAttribute
 * \ingroup routerRoute
 *
 * \chips           FM4000, FM6000, FM10000
 *
 * \desc            Gets an attribute for a specific route.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       route points to the route entry.
 *
 * \param[in]       attr contains the attribute ID.
 *
 * \param[in]       value points to caller-allocated storage into which the
 *                  function will store the attribute's value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fmGetRouteAttribute(fm_int         sw,
                              fm_routeEntry *route,
                              fm_int         attr,
                              void *         value)
{
    fm_switch *       switchPtr;
    fm_status         status;
    fm_intRouteEntry *intRoute;
    fm_intRouteEntry  key;
    fm_bool           routeLockTaken = FALSE;

    FM_LOG_ENTRY_API(FM_LOG_CAT_ROUTING,
                     "sw = %d, route = %p, attr = %d, value = %p\n",
                     sw,
                     (void *) route,
                     attr,
                     (void *) value);

    VALIDATE_AND_PROTECT_SWITCH(sw);

    switchPtr = GET_SWITCH_PTR(sw);

    /* Get shared access to routing tables */
    status = fmCaptureReadLock(&switchPtr->routingLock, FM_WAIT_FOREVER);

    if (status != FM_OK)
    {
        goto ABORT;
    }

    routeLockTaken = TRUE;

    key.route = *route;
    status    = fmCustomTreeFind(&switchPtr->routeTree,
                                 &key,
                                 (void **) &intRoute);

    if (status != FM_OK)
    {
        goto ABORT;
    }

    FM_API_CALL_FAMILY(status,
                       switchPtr->GetRouteAttribute,
                       sw,
                       intRoute,
                       attr,
                       value);

ABORT:

    if (routeLockTaken)
    {
        fmReleaseReadLock(&switchPtr->routingLock);
    }

    UNPROTECT_SWITCH(sw);

    FM_LOG_EXIT_API(FM_LOG_CAT_ROUTING, status);

}   /* end fmGetRouteAttribute */


/*****************************************************************************/
/** fmDbgTestRouteMask
 * \ingroup intRouter
 *
 * \desc            Tests the fmApplyMasksToRoute function.
 *
 * \param[in]       route points to the route string.
 *
 * \param[in]       prefix is the prefix length.
 *
 * \param[in]       routeType is the route type.
 *
 * \param[in]       mcastType is the  multicast type, if route type is multicast.
 *
 * \return          Nothing.
 *
 *****************************************************************************/
void fmDbgTestRouteMask(fm_char *               route,
                        fm_int                  prefix,
                        fm_routeType            routeType,
                        fm_multicastAddressType mcastType)
{
    fm_status     status;
    fm_routeEntry routeEntry;
    fm_char       buf[100];
    fm_ipAddr *   addrPtr;

    FM_CLEAR(routeEntry);

    routeEntry.routeType = routeType;

    switch (routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
            addrPtr = &routeEntry.data.unicast.dstAddr;
            routeEntry.data.unicast.prefixLength = prefix;
            break;

        case FM_ROUTE_TYPE_UNICAST_ECMP:
            addrPtr = &routeEntry.data.unicastECMP.dstAddr;
            routeEntry.data.unicastECMP.prefixLength = prefix;
            break;

        case FM_ROUTE_TYPE_MULTICAST:
            routeEntry.data.multicast.addressType = mcastType;

            switch (mcastType)
            {
                case FM_MCAST_ADDR_TYPE_L2MAC_VLAN:
                    FM_LOG_PRINT("L2MAC_VLAN mcast invalid\n");
                    return;

                case FM_MCAST_ADDR_TYPE_DSTIP:
                    addrPtr = &routeEntry.data.multicast.info.dstIpRoute.dstAddr;
                    routeEntry.data.multicast.info.dstIpRoute.dstPrefixLength = prefix;
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
                    addrPtr = &routeEntry.data.multicast.info.dstIpVlanRoute.dstAddr;
                    routeEntry.data.multicast.info.dstIpVlanRoute.dstPrefixLength = prefix;
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
                    addrPtr = &routeEntry.data.multicast.info.dstSrcIpRoute.dstAddr;
                    routeEntry.data.multicast.info.dstSrcIpRoute.dstPrefixLength = prefix;
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
                    addrPtr = &routeEntry.data.multicast.info.dstSrcIpVlanRoute.dstAddr;
                    routeEntry.data.multicast.info.dstSrcIpVlanRoute.dstPrefixLength = prefix;
                    break;

                default:
                    FM_LOG_PRINT("Unknown multicast type %d\n", mcastType);
                    return;
            }
            break;

        default:
            FM_LOG_PRINT("Unknown route type %d\n", routeType);
            return;
    }

    fmDbgConvertStringToIPAddress(route, FALSE, addrPtr);

    status = fmApplyMasksToRoute(&routeEntry);

    if (status != FM_OK)
    {
        FM_LOG_PRINT( "fmApplyMasksToRoute failed: %s\n", fmErrorMsg(status) );
    }
    else
    {
        fmDbgConvertIPAddressToString(addrPtr, buf);
        FM_LOG_PRINT("masked address is: %s\n", buf);
    }

}   /* end fmDbgTestRouteMask*/


/*****************************************************************************/
/** fmRoutingMaintenanceTask
 * \ingroup intRouter
 *
 * \desc            Generic thread wrapper for chip specific routing
 *                  maintenance thread.
 *
 * \param[in]       args contains a pointer to the thread information.
 *
 * \return          Should never exit.
 *
 *****************************************************************************/
void *fmRoutingMaintenanceTask(void *args)
{
    fm_thread *  thread;
    fm_switch *  switchPtr;
    fm_thread *  eventHandler;
    fm_int       sw;
    fm_bool      doRoutingTask = FALSE;

#ifdef ENABLE_TIMER
    fm_bool      startTimer = FALSE;
    fm_timestamp t1;
    fm_timestamp t2;
    fm_timestamp t3;
#endif

    thread       = FM_GET_THREAD_HANDLE(args);
    eventHandler = FM_GET_THREAD_PARAM(fm_thread, args);

    /* If logging is disabled, thread and eventHandler won't be used */
    FM_NOT_USED(thread);
    FM_NOT_USED(eventHandler);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH,
                 "thread = %s, eventHandler = %s\n",
                 thread->name,
                 eventHandler->name);

    /* First check to see if we need to call the routing maintenance task */
    do 
    {
        for(sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if (!SWITCH_LOCK_EXISTS(sw))
            {
                continue;
            }

            PROTECT_SWITCH(sw);

            switchPtr = GET_SWITCH_PTR(sw);

            if ( switchPtr &&
                (switchPtr->state == FM_SWITCH_STATE_UP) &&
                 switchPtr->RoutingMaintenanceTask )
            {
                doRoutingTask = TRUE;
            }

            UNPROTECT_SWITCH(sw);
        }
        
        fmDelay(5, 0);

    } 
    while (!doRoutingTask);

    /* Loop forever */

    while (TRUE)
    {
#ifdef ENABLE_TIMER
        fmGetTime(&t1);
#endif
        for (sw = FM_FIRST_FOCALPOINT ; sw <= FM_LAST_FOCALPOINT ; sw++)
        {
            if (!SWITCH_LOCK_EXISTS(sw))
            {
                continue;
            }

            PROTECT_SWITCH(sw);

            switchPtr = GET_SWITCH_PTR(sw);

            if ( switchPtr &&
                 (switchPtr->state == FM_SWITCH_STATE_UP) &&
                 switchPtr->RoutingMaintenanceTask )
            {
                switchPtr->RoutingMaintenanceTask(sw);
#ifdef ENABLE_TIMER
                startTimer = TRUE;
#endif
            }

            UNPROTECT_SWITCH(sw);
        }

#ifdef ENABLE_TIMER
        if (startTimer)
        {
            /* Debugging purpose only */
            fmGetTime(&t2);
            fmSubTimestamps(&t2, &t1, &t3);
            FM_LOG_DEBUG(FM_LOG_CAT_SWITCH, " time: %lld,%lld sec.\n",
                         t3.sec,
                         t3.usec);
        }
#endif

        fmYield();
        
    } /* end while (TRUE) */


    /**************************************************
     * Should never exit.
     **************************************************/

    FM_LOG_ERROR(FM_LOG_CAT_SWITCH,
                 "ERROR: fmRoutingMaintenanceTask: exiting inadvertently!\n");

    fmExitThread(thread);
    return NULL;

}   /* end fmRoutingMaintenanceTask */

