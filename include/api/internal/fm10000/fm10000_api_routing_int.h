/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_routing_int.h
 * Creation Date:   September 17, 2007
 * Description:     Contains constants and functions used to support routing.
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

#ifndef __FM_FM10000_API_ROUTING_INT_H
#define __FM_FM10000_API_ROUTING_INT_H


/* Number of bits to shift left to move value/mask into high-order nibble
 * of TCAM slice */
#define FM10000_FFU_TCAM_36_32         32


#if 0
/* Disabled at present due to difficulties with TCAM slice precedence,
 * see bugzilla #11265. */
#define ENABLE_DIVERSE_MCAST_SUPPORT
#endif

/* Debug flag to maintain route contents in the TCAM route entry structure
 * so that diagnostic software can validate the hardware TCAM against
 * the expected contents. */
#if 1
#define FM10000_DBG_TRACK_ROUTE_CONTENTS
#endif

/* Rule mumbers for routing trigger group  */
#define FM10000_TRIGGER_RULE_ROUTING_ARP_REDIRECT           1
#define FM10000_TRIGGER_RULE_ROUTING_UNRESOLVED_NEXT_HOP    2




/*****************************************************************************
 *
 *  Route Types and Slice Usage Requirements
 *
 *  This defines route types based upon the number of TCAM slices they
 *  use.  IPv4 Unicast and *G Multicast routes use one TCAM slice,
 *  IPv4SG Multicast routes use two TCAM slices, IPv6 Unicast and *G
 *  Multicast use 4 TCAM slices, and IPv6 SG Multicast routes require
 *  8 TCAM slices.
 *
 *****************************************************************************/
typedef enum
{
    FM10000_ROUTE_TYPE_UNUSED,
    FM10000_ROUTE_TYPE_V4U,
    FM10000_ROUTE_TYPE_V6U,
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
    FM10000_ROUTE_TYPE_V4SG,
    FM10000_ROUTE_TYPE_V6SG,
    FM10000_ROUTE_TYPE_V4DV,
    FM10000_ROUTE_TYPE_V6DV,
#endif
    FM10000_ROUTE_TYPE_V4DSV,
    FM10000_ROUTE_TYPE_V6DSV,

    FM10000_NUM_ROUTE_TYPES

} fm10000_RouteTypes;

#define FM10000_ROUTE_NUM_CASES        (FM10000_NUM_ROUTE_TYPES - 1)

#define FM10000_SIZEOF_IPV4_ROUTE      1
#define FM10000_SIZEOF_IPV6_ROUTE      4

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
#define FM10000_SIZEOF_IPV4_SG_ROUTE   2
#define FM10000_SIZEOF_IPV6_SG_ROUTE   8
#define FM10000_SIZEOF_IPV4_DV_ROUTE   2
#define FM10000_SIZEOF_IPV6_DV_ROUTE   4
#endif

#define FM10000_SIZEOF_IPV4_DSV_ROUTE  3
#define FM10000_SIZEOF_IPV6_DSV_ROUTE  9
#define FM10000_MAX_ROUTE_SLICE_WIDTH  FM10000_SIZEOF_IPV6_DSV_ROUTE


/*****************************************************************************
 *
 *  TCAM Slice Per-Case Information
 *
 *  This structure contains information that describes a single case in a
 *  TCAM slice.
 *
 *****************************************************************************/
typedef struct _fm10000_TcamSliceCase
{
    fm_int                          caseNum;
    fm_int                          parentTcamSlice;
    fm10000_RouteTypes              routeType;
    struct _fm10000_RouteSlice *    routeSlice;
    struct _fm10000_RouteTcamSlice *tcamSlice;

} fm10000_TcamSliceCase;


/*****************************************************************************
 *
 *  TCAM Slice Information
 *
 *  This structure contains information that describes a single slice of the
 *  TCAM, also known as a min slice
 *  Each TCAM slice supports 2 separate route cases.  These are described
 *  in this structure
 *
 *****************************************************************************/
typedef struct _fm10000_RouteTcamSlice
{
    int                           minSliceNumber;
    int                           inUse;
    fm10000_TcamSliceCase         caseInfo[FM10000_NUM_ROUTE_TYPES];
    fm_bool                       ipv4UcastOK;
    fm_bool                       ipv4McastOK;
    fm_bool                       ipv6UcastOK;
    fm_bool                       ipv6McastOK;
    fm_byte                       rowStatus[FM10000_FFU_ENTRIES_PER_SLICE];
    struct _fm10000_RoutingState *stateTable;

} fm10000_RouteTcamSlice;


#define FM10000_ROUTE_ROW_FREE             0
#define FM10000_ROUTE_ROW_RESERVED         1
#define FM10000_ROUTE_ROW_INUSE(kase)      (2 + kase)
#define FM10000_GET_ROUTE_ROW_CASE(status) (status - 2)


/*****************************************************************************
 *
 *  TCAM Route Information
 *
 *  This structure describes an individual or ECMP route entry in the TCAM,
 *  including its location within the TCAM.  It provides links to the
 *  actual route information entries.
 *
 *****************************************************************************/
typedef struct _fm10000_TcamRouteEntry
{
    struct _fm10000_RoutingState *stateTable;
    fm_intRouteEntry *           routePtr;
    fm_routeAction               action;
    fm_bool                      dirty;
    struct _fm10000_RoutingTable *routeTable;
    struct _fm10000_RoutePrefix * routePrefix;
    struct _fm10000_RouteSlice *  routeSlice;
    struct _fm10000_EcmpGroup *   ecmpGroup;
    int                          tcamSliceRow;
    FM_DLL_DEFINE_NODE(_fm10000_TcamRouteEntry, nextTcamRoute, prevTcamRoute);
    FM_DLL_DEFINE_NODE(_fm10000_TcamRouteEntry,
                       nextPrefixRoute,
                       prevPrefixRoute);
#ifdef FM10000_DBG_TRACK_ROUTE_CONTENTS
    fm_bool                      valid;
    fm_uint64                    camValue[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_uint64                    camMask[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction                 ffuAction;
#endif

} fm10000_TcamRouteEntry;


/*****************************************************************************
 *
 *  Route Slice Information
 *
 *  This structure provides a consolidated view of the definition of a single
 *  case for one or more TCAM slices
 *
 *****************************************************************************/
typedef struct _fm10000_RouteSlice
{
    fm_bool                       inUse;
    fm10000_RouteTypes            routeType;
    int                           sliceWidth;
    int                           firstTcamSlice;
    int                           lastTcamSlice;
    fm_ffuSliceInfo               sliceInfo;
    fm_int                        highestRow;
    fm_int                        lowestRow;
    struct _fm10000_RoutingState *stateTable;
    fm10000_TcamRouteEntry *      routes[FM10000_FFU_ENTRIES_PER_SLICE];
    FM_DLL_DEFINE_NODE(_fm10000_RouteSlice, nextSlice, prevSlice);
    fm_bool                       usable;
    fm_bool                       movable;

} fm10000_RouteSlice;


/*****************************************************************************
 *
 *  Route Prefix-length-specific information
 *
 *  This structure provides information about all routes with a single
 *  specified prefix-length for a routing table.
 *
 *****************************************************************************/
typedef struct _fm10000_RoutePrefix
{
    fm_int        prefix;
    fm_customTree routeTree;
    FM_DLL_DEFINE_NODE(_fm10000_RoutePrefix, nextPrefix, prevPrefix);
    FM_DLL_DEFINE_LIST(_fm10000_TcamRouteEntry, firstTcamRoute, lastTcamRoute);

} fm10000_RoutePrefix;


/*****************************************************************************
 *
 *  Route Information.
 *
 *  This structure consolidates a bunch of useful information about a route
 *  that is needed when adding a route to the route table.
 *
 *****************************************************************************/
typedef struct _fm10000_RouteInfo
{
    fm10000_RouteTypes           routeType;
    fm_bool                      isIPv6;
    fm_bool                      isUnicast;
    struct _fm10000_RoutingTable *routeTable;
    fm_uint32 *                  dstIpPtr;
    fm_int                       dstPrefix;
    fm_uint32 *                  srcIpPtr;
    fm_int                       srcPrefix;
    fm_int                       ipAddrSize;
    fm_int                       vrid;
    fm_int                       vroff;
    fm_uint16                    vlan;
    fm_uint16                    vlanMask;
    fm_int                       prefixLength;
    fm10000_RoutePrefix *        routePrefix;
    fm10000_RoutePrefix *        prevPrefix;
    fm10000_RoutePrefix *        nextPrefix;

} fm10000_RouteInfo;


/*****************************************************************************
 *
 *  Route Table
 *
 *  This structure describes a routing table.  Each routing table consists
 *  of all routes that require the same number of TCAM slices.
 *  Thus, there are 4 routing tables, one for routes that are 1 slice wide,
 *  1 for 2-slice-wide routes, 1 for 4-slice-wide routes, and 1 for
 *  8-slice-wide routes.
 *
 *****************************************************************************/
typedef struct _fm10000_RoutingTable
{
    fm10000_RouteTypes            routeType;
    struct _fm10000_switch *      ext;
    struct _fm10000_RoutingState *stateTable;
    const fm_ffuSliceInfo *      defaultSliceInfo;
    fm_bool                      ucastOK;
    fm_bool                      mcastOK;
    fm_bool                      locked;
    fm_bool                      useUnauthorizedSlices;
    fm_bool                      unauthorizedSlicesUsed;
    fm_customTree                prefixTree;

    FM_DLL_DEFINE_LIST(_fm10000_RouteSlice, firstSlice, lastSlice);

    /* tree sorted by route */
    fm_customTree                tcamRouteRouteTree;

    /* tree sorted by slice number and row, with associated linked list */
    fm_customTree                tcamSliceRouteTree;
    FM_DLL_DEFINE_LIST(_fm10000_TcamRouteEntry, firstTcamRoute, lastTcamRoute);

} fm10000_RoutingTable;


/*****************************************************************************
 *
 *  Routing Table State
 *
 *  This structure is used to describe the state of the routing table.
 *  There is one instance of this table in the fm10000_switch structure
 *  that defines the current state of the hardware.  Additional instances
 *  of this structure are used during route repacking to track the 'virtual'
 *  state as various route repacking methods are tested.
 *
 *****************************************************************************/
typedef struct _fm10000_RoutingState
{
    /* TRUE if this routing state table represents the actual state of the
     * hardware. FALSE if this routing state table represents a simulation. */
    fm_bool               actualState;

    /* TRUE if previousFirstTcamSlice and previousLastTcamSlice contain values
     * that can be used to create a temporary slice cascade regardless
     * of IPv4/v6 unicast/multicast slice usage restrictions. Normally FALSE,
     * only set to TRUE during TCAM repartitioning. */
    fm_bool               tempSlicesAvailable;

    /* The first TCAM slice number currently authorized for use. */
    fm_int                routeFirstTcamSlice;

    /* The last TCAM slice number currently authorized for use. */
    fm_int                routeLastTcamSlice;

    /* The first TCAM slice number previously usable by routing, whether
     * authorized by the current slice allocation settings or not. Normally,
     * this matches routeFirstTcamSlice, but during a TCAM re-allocation,
     * it is the value that was assigned to routing before the re-allocation.
     * During a re-allocation operation, the routing subsystem can use any
     * slice from the old range and any slice from the new range, provided
     * that it has released all of the old range slices by the time the
     * re-allocation operation concludes. */
    fm_int                previousFirstTcamSlice;

    /* The last TCAM slice number previously usable by routing, whether
     * authorized by the current slice allocation settings or not. During
     * a TCAM re-allocation operation, it is the value that was assigned
     * to routing before the re-allocation. See the definition of
     * previousFirstTcamSlice for further details. */
    fm_int                previousLastTcamSlice;

    /* First TCAM slice authorized for IPv4 Unicast use. */
    fm_int                ipv4UcastFirstTcamSlice;

    /* Last TCAM slice authorized for IPv4 Unicast use. */
    fm_int                ipv4UcastLastTcamSlice;

    /* First TCAM slice authorized for IPv4 Multicast use. */
    fm_int                ipv4McastFirstTcamSlice;

    /* Last TCAM slice authorized for IPv4 Multicast use. */
    fm_int                ipv4McastLastTcamSlice;

    /* First TCAM slice authorized for IPv6 Unicast use. */
    fm_int                ipv6UcastFirstTcamSlice;

    /* Last TCAM slice authorized for IPv6 Unicast use. */
    fm_int                ipv6UcastLastTcamSlice;

    /* First TCAM slice authorized for IPv6 Multicast use. */
    fm_int                ipv6McastFirstTcamSlice;

    /* Last TCAM slice authorized for IPv6 Multicast use. */
    fm_int                ipv6McastLastTcamSlice;

    /* Array of TCAM slice structures. */
    fm10000_RouteTcamSlice routeTcamSliceArray[FM10000_MAX_FFU_SLICES];

    /* IPv4 Unicast route table. */
    fm10000_RoutingTable   ipv4URoutes;

    /* IPv4 DIP/SIP multicast route table. */
    fm10000_RoutingTable   ipv4SGRoutes;

    /* IPv6 Unicast route table. */
    fm10000_RoutingTable   ipv6URoutes;

    /* IPv6 DIP/SIP multicast route table. */
    fm10000_RoutingTable   ipv6SGRoutes;

    /* IPv4 DIP/VLAN multicast route table. */
    fm10000_RoutingTable   ipv4DVRoutes;

    /* IPv4 DIP/SIP/VLAN multicast route table. */
    fm10000_RoutingTable   ipv4DSVRoutes;

    /* IPv6 DIP/VLAN multicast route table. */
    fm10000_RoutingTable   ipv6DVRoutes;

    /* IPv6 DIP/SIP/VLAN multicast route table. */
    fm10000_RoutingTable   ipv6DSVRoutes;

    /* Array of route table pointers, indexed by route type. */
    fm10000_RoutingTable * routeTables[FM10000_NUM_ROUTE_TYPES];

} fm10000_RoutingState;


/*****************************************************************************
 *
 *  Function Prototypes and Function Macros
 *
 *****************************************************************************/


#define fmInitRoutePrefixList(ptr) \
    FM_DLL_INIT_LIST(ptr, firstRoute, lastRoute)

fm_status fm10000RouterAlloc(fm_int sw);
fm_status fm10000RouterFree(fm_int sw);
fm_status fm10000RouterInit(fm_int sw);
fm_status fm10000SetRouterAttribute(fm_int  sw, 
                                    fm_int  attr, 
                                    void *  pValue);
fm_status fm10000AddRoute(fm_int            sw,
                          fm_intRouteEntry *pNewRoute);
fm_status fm10000DeleteRoute(fm_int            sw,
                             fm_intRouteEntry *pRoute);
fm_status fm10000SetRouteAction(fm_int            sw,
                                fm_intRouteEntry *route);
fm_status fm10000ReplaceECMPBaseRoute(fm_int            sw,
                                      fm_intRouteEntry *oldRoute,
                                      fm_intRouteEntry *pNewRoute);
fm_status fm10000SetRouteActive(fm_int            sw,
                                fm_intRouteEntry *pRoute);
fm_status fm10000AddVirtualRouter(fm_int sw,
                                  fm_int vroff);
fm_status fm10000RemoveVirtualRouter(fm_int sw,
                                     fm_int vroff);
fm_status fm10000SetRouterState(fm_int          sw,
                                fm_int          vroff, 
                                fm_routerState  state);
fm_status fm10000SetRouterMacMode(fm_int           sw,
                                  fm_int           vroff, 
                                  fm_routerMacMode mode);
fm_status fm10000UpdateEcmpRoutes(fm_int       sw,
                                  fm_int       groupId,
                                  fm_uint16    newIndex,
                                  fm_int       pathCount,
                                  fm_int       pathCountType);
fm_int fm10000CompareTcamRoutes(const void *first, const void *second);
fm_status fm10000GetTcamRouteEntry(fm_int sw, 
                                  fm_routeEntry *entry, 
                                  fm10000_TcamRouteEntry **tcamEntry);
fm_status fm10000RoutingProcessFFUPartitionChange(fm_int                  sw,
                                                 fm_ffuSliceAllocations *newAllocations,
                                                 fm_bool                 simulated);
fm_status fm10000GetRouteList(fm_int sw,
                              fm_int *pNumRoutes,
                              fm_routeEntry *pRouteList,
                              fm_int max);
fm_status fm10000GetRouteFirst(fm_int sw,
                               fm_voidptr *pSearchToken,
                               fm_routeEntry *pFirstRoute);
fm_status fm10000GetRouteNext(fm_int sw,
                              fm_voidptr *pSearchToken,
                              fm_routeEntry *pNextRoute);
fm_int fm10000GetVirtualRouterOffset(fm_int sw,
                                     fm_int vrid);
fm_status fm10000GetRouteAttribute(fm_int sw,
                                   fm_routeEntry *pRoute,
                                   fm_int attr,
                                   void *pValue);
fm_status fm10000DbgValidateRouteTables(fm_int sw);
void fm10000DbgDumpRouteStats(fm_int sw);
void fm10000DbgDumpStateTable(fm_int sw);
void fm10000DbgDumpPrefixLists(fm_int sw);
void fm10000DbgDumpRouteTables(fm_int sw, fm_int flags);
void fm10000DbgDumpRouteTables(fm_int sw, 
                               fm_int flags);
fm_status fm10000RoutingProcessArpRedirect(fm_int sw, fm_bool *plogArpRedirect);

#endif      /* end #ifndef __FM_FM10000_API_ROUTING_INT_H */
