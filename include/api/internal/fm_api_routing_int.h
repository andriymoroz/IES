/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm_api_routing_int.h
 * Creation Date: August 15, 2007
 * Description: header file for routing services.
 *
 * Copyright (c) 2007 - 2013, Intel Corporation
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

#ifndef __FM_FM_API_ROUTING_INT_H
#define __FM_FM_API_ROUTING_INT_H


#define FM_IPV4_MAX_PREFIX_LENGTH  32
#define FM_IPV6_MAX_PREFIX_LENGTH  128
#define FM_VLAN_MAX_PREFIX_LENGTH  12
#define FM_MAX_PREFIX              1024
#define FM_MAX_NUM_IP_PREFIXES     (FM_IPV6_MAX_PREFIX_LENGTH + 1)

#define FM_DST_IP_PREFIX_POSITION  12
#define FM_SRC_IP_PREFIX_POSITION  4
#define FM_VLAN_PREFIX_POSITION    0

#define FM_DST_IP_PREFIX_MASK  0xff000
#define FM_SRC_IP_PREFIX_MASK  0x00ff0
#define FM_VLAN_PREFIX_MASK    0x0000f



/*****************************************************************************
 *
 * Internal Route Entry
 *
 *****************************************************************************/
typedef struct _fm_intRouteEntry
{
    /* Pointer to the switch structure. */
    fm_switch *                   switchPtr;

    /* State of this route. */
    fm_routeState                 state;

    /* Action to be performed when this route hits. */
    fm_routeAction                action;

    /* Route Contents. */
    fm_routeEntry                 route;

    /* Pointer to the destination IP address within the 'route' element above. */
    fm_ipAddr *                   destIPAddress;

    /* Prefix value. */
    fm_int                        prefix;

    /* TRUE if the route is active. */
    fm_bool                       active;

    /* ECMP Group ID supporting this route. */
    fm_int                        ecmpGroupId;

    /* Route ECMP Group ID. Saved in case the route action is changed
     * to drop, which causes ecmpGroupId to be overwritten with the drop group.
     * If the route action is later changed back to route, we recover the
     * real ECMP group id from here. */
    fm_int                        routeEcmpGroupId;

    /* Pointer to the multicast group if this route is a multicast route. NULL
     * if the route is not a multicast route. */
    struct _fm_intMulticastGroup *mcastGroup;

    /* Tree containing all Virtual Network tunnels using this route.
     * Key is virtual network ID, value is pointer to VN tunnel record. */
    fm_tree                       vnTunnelsTree;

} fm_intRouteEntry;


/*****************************************************************************
 *
 * Internal Function Macros
 *
 *****************************************************************************/

fm_status fmRouterAlloc(fm_int sw);
fm_status fmRouterFree(fm_int sw);
fm_status fmRouterInit(fm_int sw);
fm_status fmRouterCleanup(fm_int sw);

fm_status fmAddRouteInternal(fm_int          sw,
                             fm_routeEntry * route,
                             fm_routeState   state,
                             fm_routeAction *action);
fm_status fmDeleteRouteInternal(fm_int         sw,
                                fm_routeEntry *route);
fm_status fmSetRouteActiveFlag(fm_int            sw,
                               fm_intRouteEntry *routeEntry,
                               fm_bool           updateHardware);
void fmConvertPrefixLengthToDestMask(fm_int     prefixLength,
                                     fm_bool    isIPv6,
                                     fm_uint32 *destMask);
void fmApplyIPAddressMask(fm_ipAddr *ipAddr, fm_ipAddr *addrMask);
void fmMaskIPAddress(fm_ipAddr *ipAddr, fm_int prefixLength);
fm_bool fmIsIPAddressInRangeMask(fm_ipAddr *baseAddr,
                                 fm_ipAddr *addrMask,
                                 fm_ipAddr *targetAddr);
fm_status fmApplyMasksToRoute(fm_routeEntry *route);
fm_int fmGetVirtualRouterOffset(fm_int sw, fm_int vrid);
fm_int fmCompareIPAddrs(const void *first, const void *second);
fm_int fmCompareIntRoutes(const void *first, const void *second);
fm_int fmCompareEcmpIntRoutes(const void *first, const void *second);
fm_bool fmIsIPAddressEmpty(fm_ipAddr *addr);
fm_bool fmIsUnicastIPAddress(fm_ipAddr *addr);
fm_bool fmIsMulticastIPAddress(fm_ipAddr *addr);
fm_status fmGetRouteLookupTree(fm_int          sw,
                               fm_int          vrid,
                               fm_int          prefix,
                               fm_customTree **treePtrPtr);
fm_status fmGetIntRouteForIP(fm_int             sw,
                             fm_int             vrid,
                             fm_ipAddr *        ip,
                             fm_intRouteEntry **routePtrPtr);
fm_status fmValidateVirtualRouterId(fm_int  sw,
                                    fm_int  vrid,
                                    fm_int *vroffPtr);
void *fmRoutingMaintenanceTask(void *args);

#endif  /* __FM_FM_API_ROUTING_INT_H */
