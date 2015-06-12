/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File: fm10000_api_routing.c
 * Creation Date: August 15, 2013
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
 * Macros, Constants & Types
 *****************************************************************************/

#define IS_TCAM_SLICE_CASE_AVAILABLE(tcamSlicePtr, useCase)                     \
    ( (!tcamSlicePtr->inUse) ? TRUE                                             \
     : (tcamSlicePtr->caseInfo[useCase].routeType == FM10000_ROUTE_TYPE_UNUSED)  \
        ? TRUE : FALSE )

#if 0
#define DEBUG_TRACK_MEMORY_USE
#endif

#define SWAP_BLOCK_FIRST_INDEX      (FM10000_ARP_TABLE_SIZE - 16)
#define RESERVED_BLOCK_FIRST_INDEX  (FM10000_ARP_TABLE_SIZE - 32)

#define TCAM_CASE_INVALID           255

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Variables
 *****************************************************************************/
static const fm10000_RouteTypes RouteTypes[] =
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
};

static const fm_int            RouteSliceWidths[FM10000_NUM_ROUTE_TYPES] =
{
    0,
    FM10000_SIZEOF_IPV4_ROUTE,
    FM10000_SIZEOF_IPV6_ROUTE,
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
    FM10000_SIZEOF_IPV4_SG_ROUTE,
    FM10000_SIZEOF_IPV6_SG_ROUTE,
    FM10000_SIZEOF_IPV4_DV_ROUTE,
    FM10000_SIZEOF_IPV6_DV_ROUTE,
#endif
    FM10000_SIZEOF_IPV4_DSV_ROUTE,
    FM10000_SIZEOF_IPV6_DSV_ROUTE
};

static const fm_byte           ipv4Selects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC
};

static const fm_byte           ipv6Selects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_DIP_39_32,
    FM_FFU_S1_DIP_47_40,
    FM_FFU_S2_DIP_55_48,
    FM_FFU_S3_DIP_63_56,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_DIP_71_64,
    FM_FFU_S1_DIP_79_72,
    FM_FFU_S2_DIP_87_80,
    FM_FFU_S3_DIP_95_88,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_DIP_103_96,
    FM_FFU_S1_DIP_111_104,
    FM_FFU_S2_DIP_119_112,
    FM_FFU_S3_DIP_127_120,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC
};

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT

static const fm_byte           ipv4SGSelects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_SIP_7_0,
    FM_FFU_S1_SIP_15_8,
    FM_FFU_S2_SIP_23_16,
    FM_FFU_S3_SIP_31_24,
    FM_FFU_SEL_ZERO
};

static const fm_byte           ipv6SGSelects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_DIP_39_32,
    FM_FFU_S1_DIP_47_40,
    FM_FFU_S2_DIP_55_48,
    FM_FFU_S3_DIP_63_56,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_DIP_71_64,
    FM_FFU_S1_DIP_79_72,
    FM_FFU_S2_DIP_87_80,
    FM_FFU_S3_DIP_95_88,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_DIP_103_96,
    FM_FFU_S1_DIP_111_104,
    FM_FFU_S2_DIP_119_112,
    FM_FFU_S3_DIP_127_120,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_SIP_7_0,
    FM_FFU_S1_SIP_15_8,
    FM_FFU_S2_SIP_23_16,
    FM_FFU_S3_SIP_31_24,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_SIP_39_32,
    FM_FFU_S1_SIP_47_40,
    FM_FFU_S2_SIP_55_48,
    FM_FFU_S3_SIP_63_56,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_SIP_71_64,
    FM_FFU_S1_SIP_79_72,
    FM_FFU_S2_SIP_87_80,
    FM_FFU_S3_SIP_95_88,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_SIP_103_96,
    FM_FFU_S1_SIP_111_104,
    FM_FFU_S2_SIP_119_112,
    FM_FFU_S3_SIP_127_120,
    FM_FFU_SEL_ZERO
};

static const fm_byte           ipv4DVSelects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_SEL_ZERO,
    FM_FFU_S0_MAP_VLAN_7_0,
    FM_FFU_S1_VLAN_VPRI_15_12_MAP_VLAN_11_8,
    FM_FFU_SEL_ZERO,
    FM_FFU_SEL_ZERO,
    FM_FFU_SEL_ZERO
};

static const fm_byte           ipv6DVSelects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_S4_MAP_VLAN_3_0,
    FM_FFU_S0_DIP_39_32,
    FM_FFU_S1_DIP_47_40,
    FM_FFU_S2_DIP_55_48,
    FM_FFU_S3_DIP_63_56,
    FM_FFU_S4_MAP_VLAN_7_4,
    FM_FFU_S0_DIP_71_64,
    FM_FFU_S1_DIP_79_72,
    FM_FFU_S2_DIP_87_80,
    FM_FFU_S3_DIP_95_88,
    FM_FFU_S4_MAP_VLAN_11_8,
    FM_FFU_S0_DIP_103_96,
    FM_FFU_S1_DIP_111_104,
    FM_FFU_S2_DIP_119_112,
    FM_FFU_S3_DIP_127_120,
    FM_FFU_SEL_ZERO
};

#endif

static const fm_byte           ipv4DSVSelects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_SIP_7_0,
    FM_FFU_S1_SIP_15_8,
    FM_FFU_S2_SIP_23_16,
    FM_FFU_S3_SIP_31_24,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_MUX_SELECT_VID_7_0,
    FM_FFU_MUX_SELECT_VPRI_VID_11_8,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC
};

static const fm_byte           ipv6DSVSelects[] =
{
    FM_FFU_S0_DIP_7_0,
    FM_FFU_S1_DIP_15_8,
    FM_FFU_S2_DIP_23_16,
    FM_FFU_S3_DIP_31_24,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_DIP_39_32,
    FM_FFU_S1_DIP_47_40,
    FM_FFU_S2_DIP_55_48,
    FM_FFU_S3_DIP_63_56,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_DIP_71_64,
    FM_FFU_S1_DIP_79_72,
    FM_FFU_S2_DIP_87_80,
    FM_FFU_S3_DIP_95_88,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_DIP_103_96,
    FM_FFU_S1_DIP_111_104,
    FM_FFU_S2_DIP_119_112,
    FM_FFU_S3_DIP_127_120,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_SIP_7_0,
    FM_FFU_S1_SIP_15_8,
    FM_FFU_S2_SIP_23_16,
    FM_FFU_S3_SIP_31_24,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_SIP_39_32,
    FM_FFU_S1_SIP_47_40,
    FM_FFU_S2_SIP_55_48,
    FM_FFU_S3_SIP_63_56,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_SIP_71_64,
    FM_FFU_S1_SIP_79_72,
    FM_FFU_S2_SIP_87_80,
    FM_FFU_S3_SIP_95_88,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_S0_SIP_103_96,
    FM_FFU_S1_SIP_111_104,
    FM_FFU_S2_SIP_119_112,
    FM_FFU_S3_SIP_127_120,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_MUX_SELECT_VID_7_0,
    FM_FFU_MUX_SELECT_VPRI_VID_11_8,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC,
    FM_FFU_MUX_SELECT_MAP_SMAC_MAP_DMAC
};

static const fm_ffuCaseLocation ipv4CaseLocation[] =
{
    FM_FFU_CASE_TOP_LOW_NIBBLE
};

static const fm_ffuCaseLocation ipv6CaseLocation[] =
{
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE
};

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT

static const fm_ffuCaseLocation ipv4SGCaseLocation[] =
{
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_NOT_MAPPED
};

static const fm_ffuCaseLocation ipv6SGCaseLocation[] =
{
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE
};

static const fm_ffuCaseLocation ipv4DVCaseLocation[] =
{
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE
};

static const fm_ffuCaseLocation ipv6DVCaseLocation[] =
{
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE,
    FM_FFU_CASE_TOP_HIGH_NIBBLE   
};

#endif

static const fm_ffuCaseLocation ipv4DSVCaseLocation[] =
{
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE
};

static const fm_ffuCaseLocation ipv6DSVCaseLocation[] =
{
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE,
    FM_FFU_CASE_TOP_LOW_NIBBLE
};

static const fm_ffuSliceInfo    ipv4SliceInfo =
{
    FM_FFU_SCN_IPv4_ROUTABLE
    | FM_FFU_SCN_IPv4_RTDGLORT,
    FM10000_ROUTE_TYPE_V4U - 1,
    0,
    0,
    0,
    ipv4Selects,
    TRUE,
    TRUE,
    ipv4CaseLocation
};

static const fm_ffuSliceInfo     ipv6SliceInfo =
{
    FM_FFU_SCN_IPv6_ROUTABLE
    | FM_FFU_SCN_IPv6_RTDGLORT
    | FM_FFU_SCN_IPv4INIPv6_ROUTABLE
    | FM_FFU_SCN_IPv4INIPv6_RTDGLORT,
    FM10000_ROUTE_TYPE_V6U - 1,
    0,
    3,
    3,
    ipv6Selects,
    TRUE,
    TRUE,
    ipv6CaseLocation
};

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT

static const fm_ffuSliceInfo    ipv4SGSliceInfo =
{
    FM_FFU_SCN_IPv4_ROUTABLE
    | FM_FFU_SCN_IPv4_ROUTABLEMCAST
    | FM_FFU_SCN_IPv4_RTDGLORT
    | FM_FFU_SCN_IPv4_RTDMCASTGLORT,
    FM10000_ROUTE_TYPE_V4SG - 1,
    0,
    1,
    1,
    ipv4SGSelects,
    TRUE,
    TRUE,
    ipv4SGCaseLocation
};

static const fm_ffuSliceInfo    ipv6SGSliceInfo =
{
    FM_FFU_SCN_IPv6_ROUTABLE
    | FM_FFU_SCN_IPv6_ROUTABLEMCAST
    | FM_FFU_SCN_IPv6_RTDGLORT
    | FM_FFU_SCN_IPv4INIPv6_ROUTABLE
    | FM_FFU_SCN_IPv4INIPv6_RTDGLORT
    | FM_FFU_SCN_IPv4INIPv6_ROUTABLEMCAST
    | FM_FFU_SCN_IPv4INIPv6_RTDMCASTGLORT,
    FM10000_ROUTE_TYPE_V6SG - 1,
    0,
    FM10000_SIZEOF_IPV6_DSV_ROUTE - 1,
    FM10000_SIZEOF_IPV6_DSV_ROUTE - 1,
    ipv6SGSelects,
    TRUE,
    TRUE,
    ipv6SGCaseLocation
};

static const fm_ffuSliceInfo    ipv4DVSliceInfo =
{
    FM_FFU_SCN_IPv4_ROUTABLE
    | FM_FFU_SCN_IPv4_ROUTABLEMCAST
    | FM_FFU_SCN_IPv4_RTDGLORT
    | FM_FFU_SCN_IPv4_RTDMCASTGLORT,
    FM10000_ROUTE_TYPE_V4DV - 1,
    0,
    1,
    1,
    ipv4DVSelects,
    TRUE,
    TRUE,
    ipv4DVCaseLocation
};

static const fm_ffuSliceInfo    ipv6DVSliceInfo =
{
    FM_FFU_SCN_IPv6_ROUTABLE
    | FM_FFU_SCN_IPv6_ROUTABLEMCAST
    | FM_FFU_SCN_IPv6_RTDGLORT
    | FM_FFU_SCN_IPv4INIPv6_ROUTABLE
    | FM_FFU_SCN_IPv4INIPv6_RTDGLORT
    | FM_FFU_SCN_IPv4INIPv6_ROUTABLEMCAST
    | FM_FFU_SCN_IPv4INIPv6_RTDMCASTGLORT,
    FM10000_ROUTE_TYPE_V6DV - 1,
    0,
    3,
    3,
    ipv6DVSelects,
    TRUE,
    TRUE,
    ipv6DVCaseLocation
};

#endif

static const fm_ffuSliceInfo    ipv4DSVSliceInfo =
{
    FM_FFU_SCN_IPv4_ROUTABLEMCAST
    | FM_FFU_SCN_IPv4_SWGLORT
    | FM_FFU_SCN_IPv4_RTDMCASTGLORT,
    FM10000_ROUTE_TYPE_V4DSV - 1,
    0,
    2,
    2,
    ipv4DSVSelects,
    TRUE,
    TRUE,
    ipv4DSVCaseLocation
};

static const fm_ffuSliceInfo    ipv6DSVSliceInfo =
{
    FM_FFU_SCN_IPv6_ROUTABLEMCAST
    | FM_FFU_SCN_IPv6_SWGLORT
    | FM_FFU_SCN_IPv6_RTDMCASTGLORT
    | FM_FFU_SCN_IPv4INIPv6_SWGLORT
    | FM_FFU_SCN_IPv4INIPv6_ROUTABLEMCAST
    | FM_FFU_SCN_IPv4INIPv6_RTDMCASTGLORT,
    FM10000_ROUTE_TYPE_V6DSV - 1,
    0,
    7,
    7,
    ipv6DSVSelects,
    TRUE,
    TRUE,
    ipv6DSVCaseLocation
};

static const fm_int triggerRuleArpRedirect = 
                                     FM10000_TRIGGER_RULE_ROUTING_ARP_REDIRECT;

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

/*****************************************************************************
 * Local functions with fmAlloc inside. Notice that "static" prefix
 * can be temporarily removed for the process of detection memory leaks.
 * In this case "static" should be removed for both - declaration
 * and definition. It allows to see the proper function name in memory dump.
 *****************************************************************************/
static fm10000_RoutePrefix *AllocatePrefix(fm10000_switch *pSwitchExt,
                                           fm_int          prefixLength);
static fm_status CopyRoutingTable(fm_int                sw,
                                  fm10000_RouteSlice *  pRouteSliceXref[FM10000_MAX_FFU_SLICES][FM10000_ROUTE_NUM_CASES][2],
                                  fm10000_RoutingTable *pSource,
                                  fm10000_RoutingState *pNewState,
                                  fm10000_RoutingTable *pClone);
static fm_status CloneRoutingState(fm_int                 sw,
                                   fm10000_RoutingState * pSource,
                                   fm10000_RoutingState **ppClone);
static fm_status AllocateRouteSlice(fm_int                sw,
                                    fm10000_RoutingTable *pRouteTable,
                                    fm_int                firstTcamSlice,
                                    fm10000_RouteSlice ** routeSlicePtrPtr);
static fm_status AllocateAndInitTcamRoute(fm_int                   sw,
                                          fm_intRouteEntry *       pRoute,
                                          fm10000_RouteInfo *      pRouteInfo,
                                          fm10000_TcamRouteEntry **ppTcamRoute);
/* End of local functions with fmAlloc inside. */

static fm_status InitRouteTable(fm_int                 sw,
                                fm10000_RouteTypes     routeType,
                                fm_int                 routeTableSize,
                                fm10000_RoutingTable * pRouteTable,
                                const fm_ffuSliceInfo *pSliceInfo);
static fm_status SetInitialSliceBoundaries(fm_int                sw,
                                           fm10000_RoutingState *pStateTable);
static fm_int CompareTcamRoutesBySlice(const void *pFirstRoute, 
                                       const void *pSecondRoute);
static fm_status SetFFUSliceUsageForRoutingState(fm_int                  sw,
                                                 fm_ffuSliceAllocations *pNewAllocations,
                                                 fm10000_RoutingState *  pRouteState);
static fm_bool RemoveSliceFromRoute(fm_int                sw,
                                    fm10000_RoutingTable *pRouteTable,
                                    fm10000_RouteSlice *  pSlice,
                                    fm_bool               updateHardware);
static fm_int ComparePrefixRoutes(const void *pFirstRoute,
                                  const void *pSecondRoute);
static fm_int ComparePrefix(const void *first, 
                            const void *second);
static void InsertTcamRouteCallback(const void *pKey,
                                    void *      pValue,
                                    const void *pPrevKey,
                                    void *      pPrevValue,
                                    const void *pNextKey,
                                    void *      pNextValue);
static void DeleteTcamRouteCallback(const void *pKey,
                                    void *      pValue,
                                    const void *pPrevKey,
                                    void *      pPrevValue,
                                    const void *pNextKey,
                                    void       *pNextValue);
static void InsertPrefixRouteCallback(const void *pKey,
                                      void *      pValue,
                                      const void *pPrevKey,
                                      void *      pPrevValue,
                                      const void *pNextKey,
                                      void *      pNextValue);
static void DeletePrefixRouteCallback(const void *pKey,
                                      void *      pValue,
                                      const void *pPrevKey,
                                      void *      pPrevValue,
                                      const void *pNextKey,
                                      void *      pNextValue);
static void InsertPrefixCallback(const void *pKey,
                                 void *      pValue,
                                 const void *pPrevKey,
                                 void *      pPrevValue,
                                 const void *pNextKey,
                                 void *      pNextValue);
static void DeletePrefixCallback(const void *pKey,
                                 void *      pValue,
                                 const void *pPrevKey,
                                 void *      pPrevValue,
                                 const void *pNextKey,
                                 void *      pNextValue);
static fm_status InsertSliceIntoRouteTable(fm10000_RoutingTable *pRouteTable,
                                           fm10000_RouteSlice *  pSlice);
static fm_bool GetFirstTcamSliceForRouteType(fm_int                sw,
                                             fm10000_RouteTypes    routeType,
                                             fm10000_RoutingState *pStateTable,
                                             fm_int *              firstSlice,
                                             fm_int *              lastSlice);
static fm_status GetPrefixRecord(fm_int                sw,
                                 fm10000_RoutingTable *pRouteTable,
                                 fm_int                prefixLength,
                                 fm10000_RoutePrefix **ppPrefix,
                                 fm10000_RoutePrefix **ppNextPrefix,
                                 fm10000_RoutePrefix **ppPrevPrefix);
static void FreePrefixRecord(void *pKey,
                             void *pValue);
static fm_status NormalizeFFUSliceRanges(fm_int                sw,
                                         fm10000_RoutingState *pRouteState);
static fm10000_TcamRouteEntry *GetFirstPrefixRoute(fm10000_RoutePrefix *pRoutePrefix);
static fm10000_TcamRouteEntry *GetLastPrefixRoute(fm10000_RoutePrefix *pRoutePrefix);
static fm10000_RouteTcamSlice *GetTcamSlicePtr(fm_int                sw,
                                               fm10000_RoutingState *pStateTable,
                                               fm_int                tcamSlice);
static fm_bool CopyRouteWithinSlice(fm_int                  sw,
                                    fm10000_TcamRouteEntry *pRouteEntry,
                                    fm_int                  destRow);
static fm_bool CopyRouteAcrossSlices(fm_int                  sw,
                                     fm10000_TcamRouteEntry *pRouteEntry,
                                     fm10000_RouteSlice *    pDestSlicePtr,
                                     fm_int                  destRow);
static fm_bool SelectCascadeFromSliceRangeFullSearch(fm_int               sw,
                                                     fm10000_RouteSlice **ppSliceSearchList,
                                                     fm_int               maxSlices,
                                                     fm10000_RouteSlice **ppDestSlice,
                                                     fm_int *             pSliceIndex);
static fm_bool SelectCascadeFromSliceRange(fm_int               sw,
                                           fm10000_RouteSlice * pFirstSlice,
                                           fm10000_RouteSlice * pLastSlice,
                                           fm10000_RouteSlice **ppSliceSearchList,
                                           fm_int               maxSlices,
                                           fm_bool              optimize,
                                           fm10000_RouteSlice **ppDestSlice,
                                           fm_int *             pSliceIndex);
static fm_bool FindEmptyRowWithinSliceRange(fm_int               sw,
                                            fm10000_RouteSlice * pFirstSlice,
                                            fm_int               firstRow,
                                            fm10000_RouteSlice * pLastSlice,
                                            fm_int               lastRow,
                                            fm_bool              optimize,
                                            fm10000_RouteSlice **ppDestSlice,
                                            fm_int *             pDestRow,
                                            fm10000_RouteSlice **ppUnauthSlice,
                                            fm_int *             pUnauthRow);
static fm_bool FindEmptyRowWithinPrefixRange(fm_int               sw,
                                             fm10000_RoutePrefix *routePrefix,
                                             fm10000_RouteSlice **slicePtr,
                                             fm_int *             rowPtr,
                                             fm_bool              unauthSliceOK,
                                             fm_bool              optimize);
static fm_bool MoveFirstRowDownWithinPrefix(fm_int                sw,
                                            fm10000_RoutingTable *routeTable,
                                            fm10000_RoutePrefix * routePrefix,
                                            fm_bool               unauthSliceOK,
                                            fm_bool               optimize);
static fm_bool MoveLastRowUpWithinPrefix(fm_int                sw,
                                         fm10000_RoutingTable *routeTable,
                                         fm10000_RoutePrefix * routePrefix,
                                         fm_bool               unauthSliceOK,
                                         fm_bool               optimize);
static fm_bool MoveRoute(fm_int                  sw,
                         fm10000_TcamRouteEntry *pRoute,
                         fm10000_RouteSlice *    slicePtr,
                         fm_int                  row);
static fm_bool MoveRouteUpWithinPrefix(fm_int                  sw,
                                       fm10000_RoutingTable *  pRouteTable,
                                       fm10000_TcamRouteEntry *pRoute,
                                       fm_bool                 unauthSliceOK,
                                       fm_bool                 optimize);
static fm_bool MoveRouteDownWithinPrefix(fm_int                  sw,
                                         fm10000_RoutingTable *  pRouteTable,
                                         fm10000_TcamRouteEntry *pRoute,
                                         fm_bool                 unauthSliceOK,
                                         fm_bool                 optimize);
static fm_bool UpdateTcamRoutePosition(fm_int                  sw,
                                       fm10000_TcamRouteEntry *pRouteEntry,
                                       fm10000_RouteSlice *    pDestSlicePtr,
                                       fm_int                  destRow,
                                       fm_bool                 updateHardware);
static fm_bool UpdateTcamSliceRow(fm_int                  sw,
                                  fm10000_RoutingState *  pStateTable,
                                  fm10000_RouteSlice *    pSlice,
                                  fm_int                  row,
                                  fm10000_TcamRouteEntry *pRouteEntry);
static void UpdateTcamSliceRowInfoAfterRemoveRoute(fm_int                  sw,
                                                   fm10000_RouteSlice *    pOldSlice,
                                                   fm_int                  oldRow,
                                                   fm10000_TcamRouteEntry *pRemovedRouteEntry);
static void UpdateTcamSliceRowInfoAfterInsertRoute(fm_int                  sw,
                                                   fm10000_RouteSlice *    pDestSlice,
                                                   fm_int                  destRow,
                                                   fm10000_TcamRouteEntry *pInsertedRouteEntry);
static void UpdatePrefixTreeAfterRemoveRoute(fm_int                  sw,
                                             fm10000_TcamRouteEntry *pRemovedRouteEntry);
static void FreeTcamRoute(void *pKey,
                          void *pValue);
static fm10000_RoutingTable *GetRouteTable(fm_int             sw,
                                           fm10000_RouteTypes routeType);
static fm10000_RouteTypes GetRouteType(fm_routeEntry *route);
static fm10000_RoutingTable *GetRouteTableForRoute(fm_int         sw,
                                                   fm_routeEntry *pRouteEntry);
static fm_status ReleaseRoutingTableCopy(fm_int                sw,
                                         fm10000_RoutingTable *pRouteTable);
static void ReleaseRoutingState(fm_int                 sw,
                                fm10000_RoutingState * routeState);
static fm_status MoveUnauthorizedRoutes(fm_int               sw,
                                        fm10000_RoutingTable *pRouteTable);
static fm_status MoveAllUnauthorizedRoutes(fm_int                sw,
                                           fm10000_RoutingState *pStateTable);
static fm_bool ClearCascadeRowWithinSliceRange(fm_int               sw,
                                               fm10000_RouteSlice * firstSlice,
                                               fm_int               firstRow,
                                               fm10000_RouteSlice * lastSlice,
                                               fm_int               lastRow,
                                               fm10000_RouteSlice **ppDestSlice,
                                               fm_int *             pDestRow,
                                               fm_bool              unauthSliceOK,
                                               fm_bool              optimize);
static void ClassifyRoute(fm_int             sw,
                          fm_intRouteEntry * pRoute,
                          fm10000_RouteInfo *pRouteInfo);
static fm_bool IsMcastRouteType(fm10000_RouteTypes routeType);
static fm_status ValidateRouteInformation(fm_int             sw,
                                          fm_intRouteEntry * pRoute,
                                          fm10000_RouteInfo *pRouteInfo);
static fm_status InitFfuRouteAction(fm_int             sw,
                                    fm_intRouteEntry * pRoute,
                                    fm10000_RouteInfo *pRouteInfo,
                                    fm_ffuAction *     pFfuAction);
static fm_status SetFfuRouteAction(fm_int             sw,
                                   fm_intRouteEntry * pRoute,
                                   fm10000_RouteInfo *pRouteInfo,
                                   fm_ffuAction *     pFfuAction,
                                   fm_bool *          pRuleValid);
static fm_status PreallocateRouteSlices(fm_int                sw,
                                        fm10000_RoutingState *pStateTable,
                                        fm_bool               reAlloc);
static fm_bool IsValidCascadeForRouteType(fm_int                sw,
                                          fm10000_RouteTypes    routeType,
                                          fm10000_RoutingState *pStateTable,
                                          fm_int                tcamSlice);
static fm_status AllocateTemporaryCascade(fm_int                sw,
                                          fm10000_RoutingState *pStateTable,
                                          fm10000_RouteTypes    routeType);
static fm_status FindFfuEntryForNewRoute(fm_int                  sw,
                                         fm_intRouteEntry *      pRoute,
                                         fm10000_RouteInfo *     pRouteInfo,
                                         fm10000_RouteSlice **   ppDestSlice,
                                         fm_int *                pDestRow);
static fm_status SetFFuRuleKeyForRoute(fm_int                sw,
                                       fm10000_RouteInfo *   pRouteInfo,
                                       fm_fm10000FfuSliceKey ruleKeyArray[FM10000_MAX_ROUTE_SLICE_WIDTH]);
static void BuildCascadeList(fm10000_RouteSlice * pFirstSlice,
                             fm10000_RouteSlice * pLastSlice,
                             fm10000_RouteSlice **ppSliceSearchList,
                             fm_int *             pNumSlices);
static fm_bool FindEmptyRowInSliceWithinSliceRange(fm_int               sw,
                                                   fm10000_RouteSlice * pFirstSlice,
                                                   fm_int               firstRow,
                                                   fm10000_RouteSlice * pLastSlice,
                                                   fm_int               lastRow,
                                                   fm10000_RouteSlice * pCurSlice,
                                                   fm_int *             pDestRow,
                                                   fm_int *             pUnauthRow);
static fm_bool MoveFirstRowDownWithinPrefix(fm_int                sw,
                                            fm10000_RoutingTable *pRouteTable,
                                            fm10000_RoutePrefix * pRoutePrefix,
                                            fm_bool               unauthSliceOK,
                                            fm_bool               optimize);
static fm_bool MoveRouteElsewhereWithinPrefix(fm_int                  sw,
                                              fm10000_RoutingTable *  routeTable,
                                              fm10000_TcamRouteEntry *route,
                                              fm_bool                 unauthSliceOK,
                                              fm_bool                 optimize);
static fm_int ComparePrefix(const void *pFirstPrefix, 
                            const void *pSecondPrefix);
static fm_bool InvalidateRouteSliceRow(fm_int              sw,
                                       fm10000_RouteSlice *pRouteSlice,
                                       fm_int              row);
static void DumpAllTcamSliceUsage(fm_int                sw,
                                  fm10000_RoutingState *pStateTable);
static void DumpTcamSliceUsage(fm_int                sw,
                               fm10000_RoutingState *routeState,
                               fm_int                tcamSlice);
static fm_status UnconfigureSliceCascade(fm_int              sw,
                                         fm10000_RouteSlice *slicePtr);
static fm_status SetVirtualRouterMacRegisters(fm_int         sw,
                                              fm_int         vroff,
                                              fm_int         vrMacId,
                                              fm_routerState state);




/*****************************************************************************
 * Macros
 *****************************************************************************/


#define GetFirstRoute(routeTable) \
    FM_DLL_GET_FIRST(routeTable, firstRoute)

#define GetLastRoute(routeTable) \
    FM_DLL_GET_LAST(routeTable, lastRoute)

#define GetNextRoute(route) \
    FM_DLL_GET_NEXT(route, nextRoute)

#define GetPreviousRoute(route) \
    FM_DLL_GET_PREVIOUS(route, prevRoute)

#define InsertRouteAfter(routeTable, curRoute, pNewRoute)   \
    FM_DLL_INSERT_AFTER(routeTable, firstRoute, lastRoute, \
                        curRoute, nextRoute, prevRoute,    \
                        pNewRoute)

#define InsertRouteBefore(routeTable, curRoute, pNewRoute)   \
    FM_DLL_INSERT_BEFORE(routeTable, firstRoute, lastRoute, \
                         curRoute, nextRoute, prevRoute,    \
                         pNewRoute)

#define RemoveRoute(routeTable, route)                    \
    FM_DLL_REMOVE_NODE(routeTable, firstRoute, lastRoute, \
                       route, nextRoute, prevRoute)

#define UnlinkRoutes(routeTable, first, last)                      \
    FM_DLL_UNLINK_NODES(routeTable, firstTcamRoute, lastTcamRoute, \
                        first, last, nextTcamRoute, prevTcamRoute)

#define InitSlice(slice)                           \
    FM_DLL_INIT_NODE(slice, nextSlice, prevSlice); \
    FM_DLL_INIT_LIST(slice, firstRoute, lastRoute)

#define GetFirstSlice(routeTable) \
    FM_DLL_GET_FIRST(routeTable, firstSlice)

#define GetLastSlice(routeTable) \
    FM_DLL_GET_LAST(routeTable, lastSlice)

#define GetNextSlice(routeSlice) \
    FM_DLL_GET_NEXT(routeSlice, nextSlice)

#define GetPreviousSlice(routeSlice) \
    FM_DLL_GET_PREVIOUS(routeSlice, prevSlice)

#define InsertSliceAfter(routeTable, pCurSlice, newSlice)   \
    FM_DLL_INSERT_AFTER(routeTable, firstSlice, lastSlice, \
                        pCurSlice, nextSlice, prevSlice,    \
                        newSlice)

#define InsertSliceBefore(routeTable, pCurSlice, newSlice)   \
    FM_DLL_INSERT_BEFORE(routeTable, firstSlice, lastSlice, \
                         pCurSlice, nextSlice, prevSlice,    \
                         newSlice)

#define RemoveSlice(routeTable, slice)                    \
    FM_DLL_REMOVE_NODE(routeTable, firstSlice, lastSlice, \
                       slice, nextSlice, prevSlice)

#define GetFirstRouteForSlice(slice) \
    ( (slice->highestRow >= 0) ? slice->routes[slice->highestRow] : NULL )

#define GetLastRouteForSlice(slice) \
    ( (slice->lowestRow >= 0) ? slice->routes[slice->lowestRow] : NULL )

#define AppendRouteToEcmpList(tcamRoute, routeEntry) \
    FM_DLL_INSERT_LAST(tcamRoute,                    \
                       firstEcmpRoute,               \
                       lastEcmpRoute,                \
                       routeEntry,                   \
                       nextEcmpRoute,                \
                       prevEcmpRoute)

#define RemoveRouteFromEcmpList(tcamRoute, routeEntry) \
    FM_DLL_REMOVE_NODE(tcamRoute,                      \
                       firstEcmpRoute,                 \
                       lastEcmpRoute,                  \
                       routeEntry,                     \
                       nextEcmpRoute,                  \
                       prevEcmpRoute)


/*****************************************************************************
 * Local Functions
 *****************************************************************************/




/*****************************************************************************/
/** GetRouteCase
 * \ingroup intRouter
 *
 * \desc            Given a route type, returns the TCAM case assigned to
 *                  that type.

 * \param[in]       routeType is the route type.
 *
 * \return          TCAM case for the specified route type.
 *
 *****************************************************************************/
static fm_int GetRouteCase(fm10000_RouteTypes routeType)
{
    fm_int caseToUse;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "routeType=%d\n", routeType);

    switch (routeType)
    {
        case FM10000_ROUTE_TYPE_V4U:
            caseToUse = FM10000_ROUTE_TYPE_V4U - 1;
            break;
            
        case FM10000_ROUTE_TYPE_V6U:
            caseToUse = FM10000_ROUTE_TYPE_V6U - 1;
            break;

        case FM10000_ROUTE_TYPE_V4DSV:
            caseToUse =  FM10000_ROUTE_TYPE_V4DSV - 1;
            break;
  
        case FM10000_ROUTE_TYPE_V6DSV:
            caseToUse = FM10000_ROUTE_TYPE_V6DSV - 1;
            break;

        default:
            caseToUse = TCAM_CASE_INVALID;
            break;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       caseToUse,
                       "caseToUse = %d\n",
                       caseToUse);

}   /* end GetRouteCase */




/*****************************************************************************/
/** InitRouteTable
 * \ingroup intRouter
 *
 * \desc            Perform initialization of the specified route table.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       routeType is the route type as defined by
 *                  ''fm10000_RouteTypes''.
 * 
 * \param[in]       routeTableSize is the number of entries of the route table.
 * 
 * \param[in]       pRouteTable points  to the table to be initialized
 * 
 * \param[in]       pSliceInfo points to the structure that contains FFU
 *                  user defined information.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InitRouteTable(fm_int                 sw,
                                fm10000_RouteTypes     routeType,
                                fm_int                 routeTableSize,
                                fm10000_RoutingTable * pRouteTable,
                                const fm_ffuSliceInfo *pSliceInfo)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, routeType=%d, routeTableSize=%d, pRouteTable=%p, pSliceInfo=%p\n",
                 sw,
                 routeType,
                 routeTableSize,
                 (void *) pRouteTable,
                 (void *) pSliceInfo);

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (routeType <= FM10000_ROUTE_TYPE_UNUSED ||
        routeType >= FM10000_NUM_ROUTE_TYPES   ||
        routeTableSize <= 0                    ||
        pRouteTable == NULL                    ||
        pSliceInfo  == NULL )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        FM_MEMSET_S(pRouteTable, routeTableSize, 0, routeTableSize);

        pSwitchExt->routeStateTable.routeTables[routeType] = pRouteTable;
        pRouteTable->routeType = routeType;
        pRouteTable->ext = pSwitchExt;
        pRouteTable->defaultSliceInfo = pSliceInfo;
        pRouteTable->stateTable = &pSwitchExt->routeStateTable;

        /* Initialize tcam route tree "sorted by route" */
        fmCustomTreeInit(&pRouteTable->tcamRouteRouteTree,
                         fm10000CompareTcamRoutes);

        /* Initialize tcam route tree "sorted by slice number and row" */
        fmCustomTreeInit(&pRouteTable->tcamSliceRouteTree,
                         CompareTcamRoutesBySlice);

        fmCustomTreeRequestCallbacks(&pRouteTable->tcamSliceRouteTree,
                                     InsertTcamRouteCallback,
                                     DeleteTcamRouteCallback);

        FM_DLL_INIT_LIST(pRouteTable, firstTcamRoute, lastTcamRoute);

        FM_DLL_INIT_LIST(pRouteTable, firstSlice, lastSlice);

        /* Initialize route prefix list */
        fmCustomTreeInit(&pRouteTable->prefixTree, ComparePrefix);

        fmCustomTreeRequestCallbacks(&pRouteTable->prefixTree,
                                     InsertPrefixCallback,
                                     DeletePrefixCallback);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end InitRouteTable */




/*****************************************************************************/
/** GetRouteTable
 * \ingroup intRouter
 *
 * \desc            Given the switch number and route type, return the
 *                  route table pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       routeType is the route type.
 *
 * \return          Pointer to the routing table.
 *
 *****************************************************************************/
static fm10000_RoutingTable *GetRouteTable(fm_int            sw,
                                           fm10000_RouteTypes routeType)
{
    fm10000_switch *pSwitchExt;

    pSwitchExt = GET_SWITCH_EXT(sw);

    return pSwitchExt->routeStateTable.routeTables[routeType];

}   /* end GetRouteTable */




/*****************************************************************************/
/** GetRouteType
 * \ingroup intRouter
 *
 * \desc            Given a route pointer, return the FM10000 route type.
 *
 * \param[in]       pRoute points to the route entry from which to extract
 *                  the type.
 *
 * \return          FM10000 Route Type.
 *
 *****************************************************************************/
static fm10000_RouteTypes GetRouteType(fm_routeEntry *pRoute)
{
    fm10000_RouteTypes   routeType;
    fm_ipAddr            destAddr;
    fm_bool              isIPv6;
    fm_multicastAddress *multicast;
    fm_bool              isRoute;

    fmGetRouteDestAddress(pRoute, &destAddr);
    isIPv6    = destAddr.isIPv6;
    isRoute   = FALSE;
    routeType = FM10000_ROUTE_TYPE_UNUSED;

    /* determine route type */
    switch (pRoute->routeType)
    {
        case FM_ROUTE_TYPE_UNICAST:
        case FM_ROUTE_TYPE_UNICAST_ECMP:
            isRoute = TRUE;

            if (isIPv6)
            {
                routeType = FM10000_ROUTE_TYPE_V6U;
            }
            else
            {
                routeType = FM10000_ROUTE_TYPE_V4U;
            }
            break;
        case FM_ROUTE_TYPE_MULTICAST:
            multicast = &pRoute->data.multicast;

            switch (multicast->addressType)
            {
                case FM_MCAST_ADDR_TYPE_UNKNOWN:
                case FM_MCAST_ADDR_TYPE_L2MAC_VLAN:
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP:
                    isRoute = TRUE;

                    routeType = isIPv6 ? FM10000_ROUTE_TYPE_V6U : FM10000_ROUTE_TYPE_V4U;

                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
                    isRoute = TRUE;
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
                    routeType = isIPv6 ? FM10000_ROUTE_TYPE_V6DV : FM10000_ROUTE_TYPE_V4DV;
#endif
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
                    isRoute = TRUE;
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
                    routeType = isIPv6 ? FM10000_ROUTE_TYPE_V6SG : FM10000_ROUTE_TYPE_V4SG;
#endif
                    break;

                case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
                    isRoute = TRUE;
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
                    routeType = isIPv6 ? FM10000_ROUTE_TYPE_V6DSV : FM10000_ROUTE_TYPE_V4DSV;
#endif
                    break;

                default:
                    break;

            }   /* end switch (multicast->addressType) */
            break;

        default:
            break;
    }

#ifndef ENABLE_DIVERSE_MCAST_SUPPORT
    /* All multicasts must be converted into a single type due to
     * TCAM slice precedence requirements */
    if ( (pRoute->routeType == FM_ROUTE_TYPE_MULTICAST) && isRoute )
    {
        routeType = isIPv6 ? FM10000_ROUTE_TYPE_V6DSV : FM10000_ROUTE_TYPE_V4DSV;
    }
#endif

    return routeType;

}   /* end GetRouteType */




/*****************************************************************************/
/** GetRouteTableForRoute
 * \ingroup intRouter
 *
 * \desc            Given the switch number and route pointer, return the
 *                  route table pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteEntry points to the route structure.
 *
 * \return          Pointer to the routing table.
 *
 *****************************************************************************/
static fm10000_RoutingTable *GetRouteTableForRoute(fm_int         sw,
                                                   fm_routeEntry *pRouteEntry)
{
    fm10000_RouteTypes    routeType;
    fm10000_RoutingTable *pRouteTable;

    routeType = GetRouteType(pRouteEntry);

    pRouteTable = GetRouteTable(sw, routeType);

    return pRouteTable;

}   /* end GetRouteTableForRoute */




/*****************************************************************************/
/** GetFirstPrefixRoute
 * \ingroup intRouter
 *
 * \desc            Returns the pointer to the first route in the route table
 *                  for a specified prefix length.
 *
 * \param[in]       pRoutePrefix points to the prefix table.
 *
 * \return          Pointer to the first route, if this prefix length table
 *                  currently has any routes.
 * \return          NULL if there are no routes with this prefix length.
 *
 *****************************************************************************/
static fm10000_TcamRouteEntry *GetFirstPrefixRoute(fm10000_RoutePrefix *pRoutePrefix)
{
    fm_status               err;
    fm_customTreeIterator   iter;
    fm10000_TcamRouteEntry *pKey;
    fm10000_TcamRouteEntry *pRoute;

    fmCustomTreeIterInit(&iter, &pRoutePrefix->routeTree);
    err = fmCustomTreeIterNext(&iter, (void **) &pKey, (void **) &pRoute);

    if (err != FM_OK)
    {
        pRoute = NULL;
    }

    return pRoute;

}   /* end GetFirstPrefixRoute */




/*****************************************************************************/
/** GetLastPrefixRoute
 * \ingroup intRouter
 *
 * \desc            Returns the pointer to the last route in the route table
 *                  for a specified prefix length.
 *
 * \param[in]       pRoutePrefix points to the prefix table.
 *
 * \return          Pointer to the last route, if this prefix length table
 *                  currently has any routes.
 * \return          NULL if there are no routes with this prefix length.
 *
 *****************************************************************************/
static fm10000_TcamRouteEntry *GetLastPrefixRoute(fm10000_RoutePrefix *pRoutePrefix)
{
    fm_status               err;
    fm_customTreeIterator   iter;
    fm10000_TcamRouteEntry *pKey;
    fm10000_TcamRouteEntry *pRoute;

    fmCustomTreeIterInitBackwards(&iter, &pRoutePrefix->routeTree);
    err = fmCustomTreeIterNext(&iter, (void **) &pKey, (void **) &pRoute);

    if (err != FM_OK)
    {
        pRoute = NULL;
    }

    return pRoute;

}   /* end GetLastPrefixRoute */




/*****************************************************************************/
/** GetTcamSlicePtr
 * \ingroup intRouter
 *
 * \desc            Given a tcam slice number, return the tcam slice pointer.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pStateTable points to the route state table. If NULL
 *                  the route state table will be retrieved automatically.
 *
 * \param[in]       tcamSlice contains the TCAM slice number.
 *
 * \return          Other route type.
 *
 *****************************************************************************/
static fm10000_RouteTcamSlice *GetTcamSlicePtr(fm_int                sw,
                                               fm10000_RoutingState *pStateTable,
                                               fm_int                tcamSlice)
{
    fm10000_switch *        pSwitchExt;
    fm10000_RouteTcamSlice *pTtcamSlice;

    if (pStateTable == NULL)
    {
        pSwitchExt = GET_SWITCH_EXT(sw);

        pStateTable = &pSwitchExt->routeStateTable;
    }

    pTtcamSlice = &pStateTable->routeTcamSliceArray[tcamSlice];

    return pTtcamSlice;

}   /* end GetTcamSlicePtr */




/*****************************************************************************/
/** CopyRouteWithinSlice
 * \ingroup intRouter
 *
 * \desc            moves a route from one row in a slice to another row,
 *                  freeing up the original row.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteEntry points to the route.
 *
 * \param[in]       destRow is the destination row for the route.
 *
 * \return          TRUE if successful.
 * \return          FALSE if route was not moved.
 *
 *****************************************************************************/
static fm_bool CopyRouteWithinSlice(fm_int                  sw,
                                    fm10000_TcamRouteEntry *pRouteEntry,
                                    fm_int                  destRow)
{
    fm_status             err;
    fm10000_switch *      pSwitchExt;
    fm10000_RouteSlice *  pRouteSlice;
    fm10000_RoutingState *pStateTable;
    fm_bool               routeMoved;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pRouteEntry=%p, destRow=%d\n",
                  sw,
                  (void *) pRouteEntry,
                  destRow);

    err = FM_OK;
    routeMoved = FALSE;
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pRouteEntry == NULL ||
        destRow <  0 ||
        destRow >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                     "moving route=%p in slice=%p (%d-%d) from row=%d to row=%d\n",
                     (void *) pRouteEntry,
                     (void *) pRouteEntry->routeSlice,
                     pRouteEntry->routeSlice->firstTcamSlice,
                     pRouteEntry->routeSlice->lastTcamSlice,
                     pRouteEntry->tcamSliceRow,
                     destRow );

        pStateTable = (pRouteEntry->stateTable != NULL) ? pRouteEntry->stateTable : &pSwitchExt->routeStateTable;

        pRouteSlice = pRouteEntry->routeSlice;

        if (pRouteSlice->routes[destRow] != NULL)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ROUTING,
                         "sw=%d: Attempt to move route into already-used position, "
                         "route=%p, slice=%p (%d-%d), source row=%d, destRow=%d\n",
                         sw,
                         (void *) pRouteEntry,
                         (void *) pRouteSlice,
                         pRouteSlice->firstTcamSlice,
                         pRouteSlice->lastTcamSlice,
                         pRouteEntry->tcamSliceRow,
                         destRow);
        }
        else
        {
            if (pStateTable->actualState)
            {
                /* Move the rule from the old row to the new row */
                err = fm10000MoveFFURules(sw,
                                          &pRouteSlice->sliceInfo,
                                          pRouteEntry->tcamSliceRow,
                                          1,
                                          destRow);
            }

            if (err == FM_OK)
            {
                routeMoved = UpdateTcamRoutePosition(sw, 
                                                     pRouteEntry, 
                                                     pRouteSlice, 
                                                     destRow, 
                                                     FALSE);
            }
        }

        if (routeMoved == FALSE)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                         "sw=%d: Cannot move route=%p, slice=%p (%d-%d), row=%d into destRow=%d\n",
                         sw,
                         (void *) pRouteEntry,
                         (void *) pRouteSlice,
                         pRouteSlice->firstTcamSlice,
                         pRouteSlice->lastTcamSlice,
                         pRouteEntry->tcamSliceRow,
                         destRow);
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       routeMoved,
                       "%s\n",
                       routeMoved ? "TRUE" : "FALSE");

}   /* end CopyRouteWithinSlice */




/*****************************************************************************/
/** CopyRouteAcrossSlices
 * \ingroup intRouter
 *
 * \desc            moves a route from one slice to another,
 *                  freeing up the original slice row.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteEntry points to the route.
 *
 * \param[in]       pDestSlice points to the destination slice.
 *
 * \param[in]       destRow is the destination row.
 *
 * \return          TRUE if successful.
 * \return          FALSE if route was not moved.
 *
 *****************************************************************************/
static fm_bool CopyRouteAcrossSlices(fm_int                  sw,
                                     fm10000_TcamRouteEntry *pRouteEntry,
                                     fm10000_RouteSlice *    pDestSlice,
                                     fm_int                  destRow)
{
    fm_status             err;
    fm10000_switch *      pSwitchExt;
    fm10000_RoutingState *pStateTable;
    fm10000_RouteSlice *  pFromSlice;
    fm10000_RouteSlice *  pToSlice;
    fm_bool               valid;
    fm_fm10000FfuSliceKey ruleKey[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction          actionList;
    fm_bool               routeMoved;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteEntry=%p, pDestSlice=%p, destRow=%d\n",
                 sw,
                 (void *) pRouteEntry,
                 (void *) pDestSlice,
                 destRow);

    err = FM_OK;
    routeMoved = FALSE;
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pRouteEntry == NULL ||
        pDestSlice == NULL  ||
        destRow < 0 ||
        destRow >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "moving route=%p from slice=%p (%d-%d) row=%d to slice=%p (%d-%d) row %d\n",
                     (void *) pRouteEntry,
                     (void *) pRouteEntry->routeSlice,
                     pRouteEntry->routeSlice->firstTcamSlice,
                     pRouteEntry->routeSlice->lastTcamSlice,
                     pRouteEntry->tcamSliceRow,
                     (void *) pDestSlice,
                     pDestSlice->firstTcamSlice,
                     pDestSlice->lastTcamSlice,
                     destRow);

        pStateTable = (pRouteEntry->stateTable != NULL) ? pRouteEntry->stateTable : &pSwitchExt->routeStateTable;
        pFromSlice = pRouteEntry->routeSlice;
        pToSlice   = pDestSlice;

        if (pToSlice->routes[destRow] != NULL)
        {
            FM_LOG_FATAL(FM_LOG_CAT_ROUTING,
                         "sw=%d: Attempt to move route into already-used position, "
                         "route=%p from slice=%p (%d-%d) row=%d to slice=%p (%d-%d) row=%d\n",
                         sw,
                         (void *) pRouteEntry,
                         (void *) pRouteEntry->routeSlice,
                         pRouteEntry->routeSlice->firstTcamSlice,
                         pRouteEntry->routeSlice->lastTcamSlice,
                         pRouteEntry->tcamSliceRow,
                         (void *) pDestSlice,
                         pDestSlice->firstTcamSlice,
                         pDestSlice->lastTcamSlice,
                         destRow);
        }
        else
        {
            if (pStateTable->actualState)
            {
                /* Read rule from existing slice/row */
                err = fm10000GetFFURule(sw,
                                        &pFromSlice->sliceInfo,
                                        pRouteEntry->tcamSliceRow,
                                        &valid,
                                        ruleKey,
                                        &actionList,
                                        TRUE);

                if (err == FM_OK)
                {
                    err = fm10000SetFFURule(sw,
                                            &pToSlice->sliceInfo,
                                            destRow,
                                            valid,
                                            ruleKey,
                                            &actionList,
                                            TRUE,
                                            TRUE);
                }
            }

            if (err == FM_OK)
            {
                routeMoved = UpdateTcamRoutePosition(sw, pRouteEntry, pToSlice, destRow, TRUE);
            }

            if (routeMoved == FALSE)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "sw=%d, cannot move route=%p from slice=%p (%d-%d) row=%d to slice=%p (%d-%d) row=%d\n",
                             sw,
                             (void *) pRouteEntry,
                             (void *) pRouteEntry->routeSlice,
                             pRouteEntry->routeSlice->firstTcamSlice,
                             pRouteEntry->routeSlice->lastTcamSlice,
                             pRouteEntry->tcamSliceRow,
                             (void *) pDestSlice,
                             pDestSlice->firstTcamSlice,
                             pDestSlice->lastTcamSlice,
                             destRow);
            }

        }   /* end else (pToSlice->routes[destRow] != NULL) */

    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       routeMoved,
                       "%s\n",
                       routeMoved ? "TRUE" : "FALSE");

}   /* end CopyRouteAcrossSlices */




/*****************************************************************************/
/** BuildCascadeList
 * \ingroup intRouter
 *
 * \desc            Builds a list of slice cascade pointers given a range
 *                  of slices.
 *
 * \param[in]       pFirstSlice points to the first route slice to be searched.
 *
 * \param[in]       pLastSlice points to the last route slice to be searched.
 *
 * \param[out]      ppSliceSearchList points to caller-allocated memory which
 *                  is assumed to be an array of route slice pointers into
 *                  which each slice pointer from the range will be written.
 *
 * \param[out]      pNumSlices points to caller-allocated memory into which
 *                  will be written the number of records which were written
 *                  into sliceSearchList.
 *
 * \return          none.
 *
 *****************************************************************************/
static void BuildCascadeList(fm10000_RouteSlice * pFirstSlice,
                             fm10000_RouteSlice * pLastSlice,
                             fm10000_RouteSlice **ppSliceSearchList,
                             fm_int *             pNumSlices)
{
    fm_int              maxSlices;
    fm10000_RouteSlice *pCurSlice;
    fm_bool             searchDown;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "pFirstSlice=%p, pLastSlice=%p, ppSliceSearchList=%p, pNumSlices=%p\n",
                 (void *) pFirstSlice,
                 (void *) pLastSlice,
                 (void *) ppSliceSearchList,
                 (void *) pNumSlices );

    if (pFirstSlice == NULL ||
        pLastSlice == NULL  ||
        ppSliceSearchList == NULL ||
        pNumSlices == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer\n");
    }
    else
    {
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                     "pFirstSlice = %p(%d-%d), pLastSlice = %p(%d-%d)\n",
                     (void *) pFirstSlice,
                     pFirstSlice->firstTcamSlice,
                     pFirstSlice->lastTcamSlice,
                     (void *) pLastSlice,
                     pLastSlice->firstTcamSlice,
                     pLastSlice->lastTcamSlice);

        /* determine the search direction */
        searchDown = (pFirstSlice->firstTcamSlice > pLastSlice->firstTcamSlice) ? TRUE : FALSE;

        /* Build the list of all searchable slices. */
        maxSlices = 0;
        pCurSlice = pFirstSlice;

        while (pCurSlice != NULL)
        {
            if (maxSlices >= FM10000_MAX_FFU_SLICES)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "too many slices in slice range\n");
                break;
            }

            ppSliceSearchList[maxSlices++] = pCurSlice;

            if (pCurSlice == pLastSlice)
            {
                break;
            }

            pCurSlice = searchDown ? pCurSlice->nextSlice : pCurSlice->prevSlice;
        }

        *pNumSlices = maxSlices;
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_ROUTING);

}   /* end BuildCascadeList */




/*****************************************************************************/
/** SelectCascadeFromSliceRangeFullSearch
 * \ingroup intRouter
 *
 * \desc            Searches a specified route slice range for an appropriate
 *                  cascade to use for storage of a route. The search attempts
 *                  to select a cascade that will optimize slice sharing. It
 *                  does this by trying to find a cascade that is exclusively
 *                  for use by this route type. Failing that, it looks for one
 *                  that is only partially shared with other route types or one
 *                  that is fully shared with other route types, but where all
 *                  of the other route type's cascades are also fully shared.
 *                  In other words, the attempt is to maximize use of cascades
 *                  that are fully or partially exclusive to this route type
 *                  and minimize use of cascades that are partially exclusive
 *                  to other route types. The purpose is to keep those other
 *                  slice cascades available for use by other route types for
 *                  as long as possible.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       ppSliceSearchList points to an array of route slice pointers
 *                  which represent slices from the range which are to be
 *                  searched. This allows for slices which have already been
 *                  completely searched to be skipped in subsequent searches.
 *
 * \param[in]       maxSlices contains the maximum number of records in
 *                  sliceSearchList. Some of those records may contain NULL,
 *                  indicating that the slice has been removed from the search
 *                  list.
 *
 * \param[out]      ppDestSlice points to caller-allocated memory into
 *                  which will be written the pointer to the route slice cascade
 *                  which has been selected for searching, or NULL if no
 *                  suitable cascade was found.
 *
 * \param[out]      pSliceIndex points to caller-allocated memory into
 *                  which the returned cascade's index into sliceSearchList
 *                  will be written.
 *
 * \return          TRUE if successful.
 * \return          FALSE if an empty row was not found.
 *
 *****************************************************************************/
static fm_bool SelectCascadeFromSliceRangeFullSearch(fm_int               sw,
                                                     fm10000_RouteSlice **ppSliceSearchList,
                                                     fm_int               maxSlices,
                                                     fm10000_RouteSlice **ppDestSlice,
                                                     fm_int *             pSliceIndex)
{
    fm10000_RouteSlice *pCurSlice;
    fm_int              sliceIndex;
    fm_bool             rowFound;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, ppsliceSearchList=%p, maxSlices=%d, ppDestSlicePtrPtr=%p, pSliceIndexPtr=%p\n",
                 sw,
                 (void *) ppSliceSearchList,
                 maxSlices,
                 (void *) ppDestSlice,
                 (void *) pSliceIndex );

    /* Select the first non-NULL slice in the list. */
    rowFound = FALSE;
    sliceIndex = 0;
    
    if (ppSliceSearchList != NULL &&
        ppDestSlice != NULL &&
        pSliceIndex != NULL &&
        maxSlices >= 0      &&
        maxSlices < FM10000_MAX_FFU_SLICES )
    {
        *ppDestSlice = NULL;

        while (sliceIndex < maxSlices)
        {
            pCurSlice = ppSliceSearchList[sliceIndex];

            if (pCurSlice != NULL)
            {
                *ppDestSlice = pCurSlice;
                *pSliceIndex = sliceIndex;
                rowFound = TRUE;

                FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                              "Full scan selected slice %p (%d-%d)\n",
                              (void *) pCurSlice,
                              pCurSlice->firstTcamSlice,
                              pCurSlice->lastTcamSlice );
                break;
            }
            sliceIndex++;
        }
    }

    FM_LOG_EXIT_CUSTOM( FM_LOG_CAT_ROUTING,
                       rowFound,
                       "full scan returning %s\n",
                       rowFound ? "TRUE" : "FALSE");

}   /* end SelectCascadeFromSliceRangeFullSearch */




/*****************************************************************************/
/** SelectCascadeFromSliceRange
 * \ingroup intRouter
 *
 * \desc            Searches a specified route slice range for an appropriate
 *                  cascade to use for storage of a route. The search criteria
 *                  used depend upon the setting of the optimize flag. If
 *                  optimize is TRUE, the search attempts to select a cascade
 *                  that will optimize slice sharing. It does this by trying
 *                  to find a cascade that is exclusively for use by this
 *                  route type. Failing that, it looks for one that is only
 *                  partially shared with other route types or one that is
 *                  fully shared with other route types, but where all of the
 *                  other route type's cascades are also fully shared. In other
 *                  words, the attempt is to maximize use of cascades that are
 *                  fully or partially exclusive to this route type and minimize
 *                  use of cascades that are partially exclusive to other route
 *                  types. The purpose is to keep those other slice cascades
 *                  available for use by other route types for as long as
 *                  possible.
 *                  If optimize is FALSE, the first available slice cascade
 *                  is returned.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pFirstSlice points to the first route slice to be searched.
 *
 * \param[in]       pLastSlice points to the last route slice to be searched.
 *
 * \param[in]       ppSliceSearchList points to an array of route slice pointers
 *                  which represent slices from the range which are to be
 *                  searched. This allows for slices which have already been
 *                  completely searched to be skipped in subsequent searches.
 *
 * \param[in]       maxSlices contains the maximum number of records in
 *                  sliceSearchList. Some of those records may contain NULL,
 *                  indicating that the slice has been removed from the search
 *                  list.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \param[out]      ppDestSlice points to caller-allocated memory into
 *                  which will be written the pointer to the route slice cascade
 *                  which has been selected for searching, or NULL if no
 *                  suitable cascade was found.
 *
 * \param[out]      pSliceIndex points to caller-allocated memory into
 *                  which the returned cascade's index into sliceSearchList
 *                  will be written.
 *
 * \return          TRUE if successful.
 * \return          FALSE if an empty row was not found.
 *
 *****************************************************************************/
static fm_bool SelectCascadeFromSliceRange(fm_int               sw,
                                           fm10000_RouteSlice * pFirstSlice,
                                           fm10000_RouteSlice * pLastSlice,
                                           fm10000_RouteSlice **ppSliceSearchList,
                                           fm_int               maxSlices,
                                           fm_bool              optimize,
                                           fm10000_RouteSlice **ppDestSlice,
                                           fm_int *             pSliceIndex)
{
    fm10000_RouteSlice *    pCurSlice;
    fm_int                  curTcamSlice;
    fm10000_RouteTcamSlice *pCurTcamSlice;
    fm_int                  sliceIndex;
    fm10000_RoutingState *  pStateTable;
    fm10000_RouteTypes      routeType;
    fm10000_RouteSlice *    pOtherRouteSlice;
    fm_bool                 isOtherCascadePartiallyUnshared;
    fm_int                  otherTcamSlice;
    fm10000_RouteTcamSlice *pOtherTcamSlice;
    fm_bool                 scanSlice;
    fm_bool                 rowFound;
    fm_int                  caseToUse;
    fm_int                  kase;
    fm10000_RouteTypes      kaseRouteType;
    fm_int                  otherKase;

    FM_NOT_USED(pLastSlice);

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pFirstSlice=%p, pLastSlice=%p, ppsliceSearchList=%p, maxSlices=%d, ppDestSlicePtrPtr=%p, pSliceIndexPtr=%p\n",
                 sw,
                 (void *) pFirstSlice,
                 (void *) pLastSlice,
                 (void *) ppSliceSearchList,
                 maxSlices,
                 (void *) ppDestSlice,
                 (void *) pSliceIndex );


    rowFound = FALSE;

    if (pFirstSlice == NULL       ||
        ppSliceSearchList == NULL ||
        ppDestSlice == NULL       ||
        pSliceIndex == NULL       ||
        maxSlices < 0             ||
        maxSlices >= FM10000_MAX_FFU_SLICES)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                     "sw=%d, pFirstSlice=%p(%d-%d), pLastSlice=%p(%d-%d)\n",
                     sw,
                     (void *) pFirstSlice,
                     pFirstSlice->firstTcamSlice,
                     pFirstSlice->lastTcamSlice,
                     (void *) pLastSlice,
                     pLastSlice->firstTcamSlice,
                     pLastSlice->lastTcamSlice);

        pStateTable = pFirstSlice->stateTable;
        routeType  = pFirstSlice->routeType;

        caseToUse =  GetRouteCase(routeType);

        if (caseToUse == TCAM_CASE_INVALID)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                         "Invalid argument: Invalid route type %d\n",
                         routeType);
        }
        else if (!optimize)
        {
            rowFound = SelectCascadeFromSliceRangeFullSearch(sw,
                                                             ppSliceSearchList,
                                                             maxSlices,
                                                             ppDestSlice,
                                                             pSliceIndex);
        }
        else
        {
            /* First, look for a slice cascade within the specified slice range that
             * is not at all shared with other route types. If any are found, search
             * for an available row. */
            sliceIndex = 0;
            while (sliceIndex < maxSlices)
            {
                pCurSlice = ppSliceSearchList[sliceIndex];

                if (pCurSlice != NULL)
                {
                    for (curTcamSlice  = pCurSlice->firstTcamSlice ;
                         curTcamSlice <= pCurSlice->lastTcamSlice ;
                         curTcamSlice++)
                    {
                        pCurTcamSlice =
                            GetTcamSlicePtr(sw, pStateTable, curTcamSlice);

                        for (kase = 0 ;
                             kase < FM10000_ROUTE_NUM_CASES ;
                             kase++)
                        {
                            kaseRouteType =
                                pCurTcamSlice->caseInfo[kase].routeType;
                            
                            if ( (kaseRouteType != routeType)
                                 && (kaseRouteType != FM10000_ROUTE_TYPE_UNUSED) )
                            {
                                break;
                            }
                        }

                        if (kase < FM10000_ROUTE_NUM_CASES)
                        {
                            break;
                        }
                    }

                    if (curTcamSlice > pCurSlice->lastTcamSlice)
                    {
                        /* We found a completely unshared slice, return it. */
                        *ppDestSlice = pCurSlice;
                        *pSliceIndex = sliceIndex;
                        rowFound = TRUE;

                        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                                      "exclusive scan selected slice %p (%d-%d)\n",
                                      (void *) pCurSlice,
                                      pCurSlice->firstTcamSlice,
                                      pCurSlice->lastTcamSlice );
                        break;
                    }
                }               
                sliceIndex++;
            }

            /* Scan the slice list again. We want to scan any cascade which is
             * only partially shared with other route types. All remaining cascades
             * should be fully shared, that is, every TCAM slice in the cascade is
             * shared with another route type. We want to skip cascades where the other
             * cascade sees its view of its cascade as being partially unshared. For
             * instance, if we are trying to find an empty row for route type IPv4
             * Unicast, which has only a single slice in its cascade, and that slice
             * is shared with IPv4 Multicast, we look at the IPv4 Multicast cascade.
             * If any of its TCAM slices are unshared, we want to avoid using this
             * cascade if possible so as not to prematurely reduce the number of
             * routes available to that cascade.
             * So, if we find any cascades which are only partially shared, or which
             * are fully shared and where all cascades to which every TCAM slice
             * in this cascade are also fully shared, we will scan that cascade for
             * an empty row. */
            sliceIndex = 0;
            while (sliceIndex < maxSlices)
            {
                pCurSlice = ppSliceSearchList[sliceIndex];

                if (pCurSlice == NULL)
                {
                    sliceIndex++;
                    continue;
                }

                scanSlice                       = FALSE;
                isOtherCascadePartiallyUnshared = FALSE;

                for (curTcamSlice = pCurSlice->firstTcamSlice ;
                     curTcamSlice <= pCurSlice->lastTcamSlice ;
                     curTcamSlice++)
                {
                    pCurTcamSlice = GetTcamSlicePtr(sw, pStateTable, curTcamSlice);

                    for (kase = 0 ; kase < FM10000_ROUTE_NUM_CASES ; kase++)
                    {
                        kaseRouteType =
                            pCurTcamSlice->caseInfo[kase].routeType;

                        if (kaseRouteType == FM10000_ROUTE_TYPE_UNUSED)
                        {
                            scanSlice = TRUE;
                            break;
                        }
                        
                        if (kaseRouteType != routeType)
                        {
                            pOtherRouteSlice =
                                pCurTcamSlice->caseInfo[kase].routeSlice;
                            
                            if (pOtherRouteSlice != NULL)
                            {
                                for (otherTcamSlice = pOtherRouteSlice->firstTcamSlice ;
                                     otherTcamSlice <= pOtherRouteSlice->lastTcamSlice ;
                                     otherTcamSlice++)
                                {
                                    pOtherTcamSlice = GetTcamSlicePtr(sw,
                                                                      pStateTable,
                                                                      otherTcamSlice);

                                    for (otherKase = 0 ;
                                         otherKase < FM10000_ROUTE_NUM_CASES ;
                                         otherKase++)
                                    {
                                        if (pOtherTcamSlice->caseInfo[otherKase].routeType
                                            == FM10000_ROUTE_TYPE_UNUSED)
                                        {
                                            isOtherCascadePartiallyUnshared = TRUE;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                scanSlice = TRUE;
                            }
                        }
                        
                    }   /* end for (kase = 0 ; kase < FM10000_NUM_ROUTE_CASES ; kase++) */
                }  /* end for (curTcamSlice = pCurSlice->firstTcamSlice ;... */

                /* If all partner cascades are fully shared, scan this cascade. */
                if (!isOtherCascadePartiallyUnshared)
                {
                    scanSlice = TRUE;
                }

                if (scanSlice)
                {
                    *ppDestSlice = pCurSlice;
                    *pSliceIndex = sliceIndex;
                    rowFound = TRUE;
                    break;
                }

                sliceIndex++;
            }

            if (rowFound == FALSE)
            {
                /* Optimized search failed*/
                *ppDestSlice = NULL;
            }
        }
    }

    FM_LOG_EXIT_CUSTOM( FM_LOG_CAT_ROUTING,
                       rowFound,
                       "scan returning %s\n",
                       rowFound ? "TRUE" : "FALSE");

}   /* end SelectCascadeFromSliceRange */




/*****************************************************************************/
/** FindEmptyRowInSliceWithinSliceRange
 * \ingroup intRouter
 *
 * \desc            Searches a single slice within a range of cascades for an
 *                  empty row. The range information is only used to provide
 *                  top/bottom rows for the first and last cascades in the
 *                  range.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pFirstSlice points to the first route slice in the range.
 *
 * \param[in]       firstRow is the first row number in the range.
 *
 * \param[in]       pLastSlice points to the last route slice in the range.
 *
 * \param[in]       lastRow is the last row number in the range.
 *
 * \param[in]       pCurSlice points to the route slice to be searched.
 *
 * \param[out]      pDestRow points to caller-allocated memory into which the
 *                  available row number will be written, or -1 if an empty
 *                  row was not found.
 *
 * \param[out]      pUnauthRow points to caller-allocated memory into which
 *                  the available but unauthorized row number will be written,
 *                  or -1 if no empty unauthorized row was found. This pointer
 *                  may be NULL.
 *
 * \return          TRUE if successful.
 * \return          FALSE if an empty row was not found.
 *
 *****************************************************************************/
static
fm_bool FindEmptyRowInSliceWithinSliceRange(fm_int              sw,
                                            fm10000_RouteSlice *pFirstSlice,
                                            fm_int              firstRow,
                                            fm10000_RouteSlice *pLastSlice,
                                            fm_int              lastRow,
                                            fm10000_RouteSlice *pCurSlice,
                                            fm_int *            pDestRow,
                                            fm_int *            pUnauthRow)
{
    fm_int                  curRow;
    fm_int                  curTcamSlice;
    fm10000_RouteTcamSlice *pCurTcamSlice;
    fm_bool                 searchDown;
    fm_bool                 foundRow;
    fm_int                  unauthRow;
    fm_int                  topRow;
    fm_int                  bottomRow;
    fm10000_RoutingState *  pStateTable;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pFirstSlice=%p, firstRow=%d, pLastSlice=%p, lastRow=%d pCurSlice=%p, pDestRow=%p, pUunauthRow=%p\n",
                 sw,
                 (void *) pFirstSlice,
                 firstRow,
                 (void *) pLastSlice,
                 lastRow,
                 (void *) pCurSlice,
                 (void *) pDestRow,
                 (void *) pUnauthRow );

    foundRow = FALSE;
    curRow = -1;

    if (pFirstSlice == NULL ||
        pLastSlice == NULL  ||
        pCurSlice == NULL   ||
        pDestRow == NULL    ||
        firstRow < 0        || firstRow >= FM10000_FFU_ENTRIES_PER_SLICE ||
        lastRow < 0         || lastRow >= FM10000_FFU_ENTRIES_PER_SLICE ) 
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {

        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                     "sw=%d, pFirstSlice=%p(%d-%d), pLlastSlice=%p(%d-%d),  pCurSlice=%p(%d-%d)\n",
                     sw,
                     (void *) pFirstSlice,
                     pFirstSlice->firstTcamSlice,
                     pFirstSlice->lastTcamSlice,
                     (void *) pLastSlice,
                     pLastSlice->firstTcamSlice,
                     pLastSlice->lastTcamSlice,
                     (void *) pCurSlice,
                     pCurSlice->firstTcamSlice,
                     pCurSlice->lastTcamSlice);
        
        unauthRow   = -1;
        pStateTable = pFirstSlice->stateTable;

        if (pFirstSlice->firstTcamSlice != pLastSlice->firstTcamSlice)
        {
            searchDown = (pFirstSlice->firstTcamSlice > pLastSlice->firstTcamSlice) ? TRUE : FALSE;
        }
        else
        {
            searchDown = firstRow > lastRow ? TRUE : FALSE;
        }

        /* determine search boundaries */
        if (searchDown)
        {
            topRow    = (pCurSlice == pFirstSlice) ? firstRow : FM10000_FFU_ENTRIES_PER_SLICE - 1;
            bottomRow = (pCurSlice == pLastSlice) ? lastRow  : 0;
        }
        else
        {
            bottomRow = (pCurSlice == pFirstSlice) ? firstRow : 0;
            topRow    = (pCurSlice == pLastSlice) ? lastRow  : FM10000_FFU_ENTRIES_PER_SLICE - 1;
        }

        /* Search for an empty row */
        for (curRow = bottomRow ; curRow <= topRow; curRow++)
        {
            for (curTcamSlice  = pCurSlice->firstTcamSlice ;
                 curTcamSlice <= pCurSlice->lastTcamSlice ;
                 curTcamSlice++)
            {
                pCurTcamSlice = GetTcamSlicePtr(sw, pStateTable, curTcamSlice);

                if (pCurTcamSlice->rowStatus[curRow] != FM10000_ROUTE_ROW_FREE)
                {
                    break;
                }
            }

            if (curTcamSlice > pCurSlice->lastTcamSlice)
            {
                if (pCurSlice->usable)
                {
                    /* Found an empty row */
                    foundRow = TRUE;
                }
                else if (unauthRow == -1)
                {
                    unauthRow = curRow;
                }
                break;
            }
        }

        if (!foundRow)
        {
            curRow = -1;
        }

        *pDestRow = curRow;

        if (pUnauthRow != NULL)
        {
            *pUnauthRow = unauthRow;
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       foundRow,
                       "foundRow=%s, curRow=%d\n",
                       foundRow? "TRUE" : "FALSE",
                       curRow);

}   /* end FindEmptyRowInSliceWithinSliceRange */




/*****************************************************************************/
/** FindEmptyRowWithinSliceRange
 * \ingroup intRouter
 *
 * \desc            Searches a specified route slice range for an empty row.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pFirstSlice points to the first route slice to be searched.
 *
 * \param[in]       firstRow is the first row number to be searched.
 *
 * \param[in]       pLastSlice points to the last route slice to be searched.
 *
 * \param[in]       lastRow is the last row number to be searched.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \param[out]      ppDestSlice points to caller-allocated memory into
 *                  which will be written the pointer to the route slice which
 *                  contains the available row, or NULL if an empty row
 *                  was not found.
 *
 * \param[out]      pDestRow points to caller-allocated memory into which the
 *                  available row number will be written, or -1 if an empty
 *                  row was not found.
 *
 * \param[out]      ppUnauthSlice points to caller-allocated memory into
 *                  which will be written the pointer to the route slice which
 *                  contains an available but unauthorized row, or NULL if no
 *                  empty unauthorized row was found. This pointer may be NULL.
 *
 * \param[out]      pUnauthRow points to caller-allocated memory into which
 *                  the available but unauthorized row number will be written,
 *                  or -1 if no empty unauthorized row was found. This pointer
 *                  may be NULL.
 *
 * \return          TRUE if successful.
 * \return          FALSE if an empty row was not found.
 *
 *****************************************************************************/
static fm_bool FindEmptyRowWithinSliceRange(fm_int               sw,
                                            fm10000_RouteSlice * pFirstSlice,
                                            fm_int               firstRow,
                                            fm10000_RouteSlice * pLastSlice,
                                            fm_int               lastRow,
                                            fm_bool              optimize,
                                            fm10000_RouteSlice **ppDestSlice,
                                            fm_int *             pDestRow,
                                            fm10000_RouteSlice **ppUnauthSlice,
                                            fm_int *             pUnauthRow)
{
    fm10000_RouteSlice * pCurSlice;
    fm_int               curRow;
    fm_bool              foundRow;
    fm10000_RouteSlice * pUnauthSlice;
    fm_int               unauthRow;
    fm_int               tempUnauthRow;
    fm10000_RouteSlice * pSliceSearchList[FM10000_MAX_FFU_SLICES];
    fm_int               maxSlices;
    fm_int               sliceIndex;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pFirstSlice=%p, firstRow=%d, pLastSlice=%p, lastRow=%d, optimize=%d, "
                 "ppDestSlice=%p, pDestRow=%p, ppUnauthSlice=%p, pUnauthRow=%p\n",
                 sw,
                 (void *) pFirstSlice,
                 firstRow,
                 (void *) pLastSlice,
                 lastRow,
                 optimize,
                 (void *) ppDestSlice,
                 (void *) pDestRow,
                 (void *) ppUnauthSlice,
                 (void *) pUnauthRow );

    foundRow    = FALSE;
    curRow      = -1;
    unauthRow   = -1;
    pUnauthSlice = NULL;
    pCurSlice    = NULL;

    if (pFirstSlice == NULL ||
        pLastSlice  == NULL ||
        ppDestSlice == NULL ||
        pDestRow    == NULL ||
        firstRow < 0 || firstRow >= FM10000_FFU_ENTRIES_PER_SLICE ||
        lastRow  < 0 || lastRow  >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "sw=%d, pFirstSlice=%p(%d-%d), pLastSlice=%p(%d-%d)\n",
                      sw,
                      (void *) pFirstSlice,
                      pFirstSlice->firstTcamSlice,
                      pFirstSlice->lastTcamSlice,
                      (void *) pLastSlice,
                      pLastSlice->firstTcamSlice,
                      pLastSlice->lastTcamSlice);

        /* Build a list of all searchable slices. This list will be used to keep
         * slices from being searched multiple times. */
        BuildCascadeList(pFirstSlice,
                         pLastSlice,
                         pSliceSearchList,
                         &maxSlices);

        while (1)
        {
            /* looks for a cascade to storage the route */
            if ( !SelectCascadeFromSliceRange(sw,
                                              pFirstSlice,
                                              pLastSlice,
                                              pSliceSearchList,
                                              maxSlices,
                                              optimize,
                                              &pCurSlice,
                                              &sliceIndex) )
            {
                /* no empty row was found */
                break;
            }

            /* now search for an empty row */
            tempUnauthRow = -1;
            curRow   = -1;

            foundRow = FindEmptyRowInSliceWithinSliceRange(sw,
                                                           pFirstSlice,
                                                           firstRow,
                                                           pLastSlice,
                                                           lastRow,
                                                           pCurSlice,
                                                           &curRow,
                                                           &tempUnauthRow);

            /* saves only the first unauthorized row found */
            if ( (tempUnauthRow != -1) && (pUnauthSlice == NULL) )
            {
                pUnauthSlice = pCurSlice;
                unauthRow  = tempUnauthRow;
            }

            if (foundRow && pCurSlice != NULL && pCurSlice->usable)
            {
                break;
            }

            /* Slice was completely searched and it has no available rows,
             * so remove it frome the list. */
            pSliceSearchList[sliceIndex] = NULL;
        }

        if (!foundRow)
        {
            pCurSlice = NULL;
            curRow   = -1;
        }

        *ppDestSlice = pCurSlice;
        *pDestRow = curRow;

        /* only if pointers to unauthorized slice and row are not NULL */
        if (ppUnauthSlice != NULL && pUnauthRow != NULL)
        {
            *ppUnauthSlice = pUnauthSlice;
            *pUnauthRow = unauthRow;
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       foundRow,
                       "foundRow=%d, pCurSlice=%p, curRow=%d\n",
                       foundRow,
                       (void *) pCurSlice,
                       curRow);

}   /* end FindEmptyRowWithinSliceRange */




/*****************************************************************************/
/** FindEmptyRowWithinPrefixRange
 * \ingroup intRouter
 *
 * \desc            Searches for an available route slice and TCAM row
 *                  within the range of rows currently assigned to a
 *                  specific prefix length.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRoutePrefix points to the route prefix table.
 *
 * \param[out]      slicePtr contains the slice pointer of an empty row.
 *
 * \param[out]      rowPtr contains the row number of an empty row.
 *
 * \param[in]       unauthSliceOK is TRUE if an unauthorized slice/row should
 *                  be returned if an authorized slice/row can't be found.
 *
 * \param[in]       optimize is TRUE if the search should attempt to return a
 *                  row from a slice that will optimize slice sharing by
 *                  seeking slices that are unshared.
 *
 * \return          TRUE if an empty row was found.
 * \return          FALSE if there were no empty rows within the range.
 *
 *****************************************************************************/
static fm_bool FindEmptyRowWithinPrefixRange(fm_int               sw,
                                             fm10000_RoutePrefix *pRoutePrefix,
                                             fm10000_RouteSlice **slicePtr,
                                             fm_int *             rowPtr,
                                             fm_bool              unauthSliceOK,
                                             fm_bool              optimize)
{
    fm_bool                 success;
    fm10000_TcamRouteEntry *pTempRoute;
    fm10000_RouteSlice *    pFirstSlice;
    fm_int                  firstRow;
    fm10000_RouteSlice *    pUnauthSlice;
    fm_int                  unauthRow;
    fm10000_RouteSlice *    pCurSlice;
    fm_int                  curRow;
    fm10000_RouteSlice *    pLastSlice;
    fm_int                  lastRow;


    success = FALSE;
    pUnauthSlice = NULL;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoutePrefix=%p, slicePtr=%p, rowPtr=%p, unauthSliceOK=%d, optimize=%d\n",
                 sw,
                 (void *) pRoutePrefix,
                 (void *) slicePtr,
                 (void *) rowPtr,
                 unauthSliceOK,
                 optimize);

    if (pRoutePrefix == NULL ||
        slicePtr == NULL ||
        rowPtr == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument\n");
    }
    else
    {

        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "routePrefix=(%d)\n",
                     pRoutePrefix->prefix);

        pTempRoute = GetFirstPrefixRoute(pRoutePrefix);

        if (pTempRoute != NULL)
        {

            pFirstSlice = pTempRoute->routeSlice;
            firstRow = pTempRoute->tcamSliceRow;

            pTempRoute = GetLastPrefixRoute(pRoutePrefix);

            if (pTempRoute != NULL)
            {
                pLastSlice = pTempRoute->routeSlice;
                lastRow = pTempRoute->tcamSliceRow;

                if ( FindEmptyRowWithinSliceRange(sw,
                                                  pFirstSlice,
                                                  firstRow,
                                                  pLastSlice,
                                                  lastRow,
                                                  optimize,
                                                  &pCurSlice,
                                                  &curRow,
                                                  &pUnauthSlice,
                                                  &unauthRow)
                     && (pCurSlice != NULL) )
                {
                    *slicePtr = pCurSlice;
                    *rowPtr   = curRow;

                    success = TRUE;
                }
                else if ( unauthSliceOK && (pUnauthSlice != NULL) )
                {
                    *slicePtr = pUnauthSlice;
                    *rowPtr   = unauthRow;

                    success = TRUE;
                }

            }   /* end 2nd if (pTempRoute != NULL) */

        }   /* end 1rs if (pTempRoute != NULL) */

    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       success,
                       "An empty row was%s found\n",
                       (success) ? "" : " NOT");

}   /* end FindEmptyRowWithinPrefixRange */




/*****************************************************************************/
/** MoveFirstRowDownWithinPrefix
 * \ingroup intRouter
 *
 * \desc            Moves the first route in a prefix down to somewhere within
 *                  the range of rows currently used by that prefix, or to
 *                  a row after the current last row used by that prefix,
 *                  if possible.  Will recurse, moving prefixes after this
 *                  prefix, if necessary.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table.
 *
 * \param[in]       pRoutePrefix points to the route prefix table.
 *
 * \param[in]       unauthSliceOK is TRUE if the route may be moved into
 *                  an unauthorized slice.  This may be done during TCAM
 *                  repartitioning in order to move some routes temporarily
 *                  in order to make room for other routes that need to be
 *                  moved before this route can be moved to an authorized
 *                  slice.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \return          TRUE if successful and the first route's original
 *                  slice and row are now free.
 * \return          FALSE if there was no room to move the row.
 *
 *****************************************************************************/
static fm_bool MoveFirstRowDownWithinPrefix(fm_int                sw,
                                            fm10000_RoutingTable *pRouteTable,
                                            fm10000_RoutePrefix * pRoutePrefix,
                                            fm_bool               unauthSliceOK,
                                            fm_bool               optimize)
{
    fm_bool                 success;
    fm10000_TcamRouteEntry *pFirstRoute;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoutePrefix=%p, pRoutePrefix=%p, unauthSliceOK=%d, optimize=%d\n",
                 sw,
                 (void *) pRouteTable,
                 (void *) pRoutePrefix,
                 unauthSliceOK,
                 optimize);

    success = FALSE;

    if (pRouteTable != NULL && pRoutePrefix != NULL)
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,"Route type=%d, prefix=%d",
                     pRouteTable->routeType,
                     pRoutePrefix->prefix);

        pFirstRoute = GetFirstPrefixRoute(pRoutePrefix);

        if (pFirstRoute != NULL)
        {
            success = MoveRouteDownWithinPrefix(sw,
                                                pRouteTable,
                                                pFirstRoute,
                                                unauthSliceOK,
                                                optimize);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "There is no route\n");
        }
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Invalid argument: NULL pointer\n");
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       success,
                       "Route was%s moved\n",
                       (success) ? "" : " NOT");

}   /* end MoveFirstRowDownWithinPrefix */




/*****************************************************************************/
/** MoveLastRowUpWithinPrefix
 * \ingroup intRouter
 *
 * \desc            Moves the last route in a prefix up to somewhere within
 *                  the range of rows currently used by that prefix, or to
 *                  a row before the current first row used by that prefix,
 *                  if possible.  Will recurse, moving prefixes before this
 *                  prefix, if necessary.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table.
 *
 * \param[in]       pRoutePrefix points to the route prefix table.
 *
 * \param[in]       unauthSliceOK is TRUE if the route may be moved into
 *                  an unauthorized slice.  This may be done during TCAM
 *                  repartitioning in order to move some routes temporarily
 *                  in order to make room for other routes that need to be
 *                  moved before this route can be moved to an authorized
 *                  slice.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \return          TRUE if successful and the last route's original
 *                  slice and row are now free.
 * \return          FALSE if there was no room to move the row.
 *
 *****************************************************************************/
static fm_bool MoveLastRowUpWithinPrefix(fm_int                sw,
                                         fm10000_RoutingTable *pRouteTable,
                                         fm10000_RoutePrefix * pRoutePrefix,
                                         fm_bool               unauthSliceOK,
                                         fm_bool               optimize)
{
    fm_bool                 success;
    fm10000_TcamRouteEntry *pLastRoute;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoutePrefix=%p, pRoutePrefix=%p, unauthSliceOK=%d, optimize=%d\n",
                 sw,
                 (void *) pRouteTable,
                 (void *) pRoutePrefix,
                 unauthSliceOK,
                 optimize);

    success = FALSE;

    if (pRouteTable != NULL && pRoutePrefix != NULL)
    {
        pLastRoute = GetLastPrefixRoute(pRoutePrefix);

        if (pLastRoute != NULL)
        {
            success = MoveRouteUpWithinPrefix(sw,
                                              pRouteTable,
                                              pLastRoute,
                                              unauthSliceOK,
                                              optimize);
        }
        else
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "There is no route\n");
        }
    }
    else
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Invalid argument: NULL pointer\n");
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       success,
                       "Route was%s moved\n",
                       (success) ? "" : " NOT");

}   /* end MoveLastRowUpWithinPrefix */




/*****************************************************************************/
/** MoveRoute
 * \ingroup intRouter
 *
 * \desc            Moves the specified into the slice specified by slicePtr
 *                  at the position given by row.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRoute points to the route to be moved.
 *
 * \param[in]       slicePtr points to the target slice.
 *
 * \param[in]       row identifies the target row in the detination slice.
 *
 * \return          TRUE if successful and the route's original
 *                  slice and row are now free.
 * \return          FALSE if there was no room to move the route.
 *
 *****************************************************************************/
static fm_bool MoveRoute(fm_int                  sw,
                         fm10000_TcamRouteEntry *pRoute,
                         fm10000_RouteSlice *    slicePtr,
                         fm_int                  row)
{
    fm_bool     routeMoved = FALSE;


    if (slicePtr == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "slicePtr is NULL\n");
    }
    else if (pRoute == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "pRoute is NULL\n");
    }
    else
    {
        /* move the route to the new location */
        if ( pRoute->routeSlice == slicePtr )
        {
            /* try moving within a slice */
            if ( !CopyRouteWithinSlice(sw, pRoute, row) )
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "cannot copy route whithin slice to row=%d", row);
            }
            else
            {
                routeMoved = TRUE;
            }
        }
        else
        {
            /* move across slices */
            if ( !CopyRouteAcrossSlices(sw, pRoute, slicePtr, row) )
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "cannot copy route across slices to slice %d-%d row=%d",
                             slicePtr->firstTcamSlice,
                             slicePtr->lastTcamSlice,
                             row);
            }
            else
            {
                routeMoved = TRUE;
            }
        }
    }

    return routeMoved;

}   /* end MoveRoute */





/*****************************************************************************/
/** MoveRouteUpWithinPrefix
 * \ingroup intRouter
 *
 * \desc            Moves a specified route up to somewhere else within
 *                  the range of rows currently used by its prefix, or to
 *                  a row before the current first row used by its prefix,
 *                  if possible.  Will recurse, moving prefixes before the
 *                  route's prefix, if necessary.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table.
 *
 * \param[in]       pRoute points to the route to be moved.
 *
 * \param[in]       unauthSliceOK is TRUE if the route may be moved into
 *                  an unauthorized slice.  This may be done during TCAM
 *                  repartitioning in order to move some routes temporarily
 *                  in order to make room for other routes that need to be
 *                  moved before this route can be moved to an authorized
 *                  slice.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \return          TRUE if successful and the route's original
 *                  slice and row are now free.
 * \return          FALSE if there was no room to move the route.
 *
 *****************************************************************************/
static fm_bool MoveRouteUpWithinPrefix(fm_int                  sw,
                                       fm10000_RoutingTable *  pRouteTable,
                                       fm10000_TcamRouteEntry *pRoute,
                                       fm_bool                 unauthSliceOK,
                                       fm_bool                 optimize)
{
    fm10000_RoutePrefix *   pRoutePrefix;
    fm10000_TcamRouteEntry *pTempRoute;
    fm10000_RouteSlice *    slicePtr;
    fm_int                  row;
    fm10000_RouteSlice *    unauthSlicePtr;
    fm_int                  unauthRow;
    fm10000_RoutePrefix *   pPrevPrefix;
    fm_int *                pTmpPrefixLength;
    fm10000_RouteSlice *    pFirstSlice;
    fm10000_RouteSlice *    pLastSlice;
    fm_int                  firstRow;
    fm_int                  lastRow;
    fm_bool                 routeMoved;
    fm_bool                 stopProcessing;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteTable=%p, pRoute=%p, unauthSliceOK=%d, optimize=%d\n",
                 sw,
                 (void *) pRouteTable,
                 (void *) pRoute,
                 unauthSliceOK,
                 optimize );

    pRoutePrefix   = pRoute->routePrefix;
    pFirstSlice    = pRoute->routeSlice;
    firstRow       = pRoute->tcamSliceRow + 1;
    pLastSlice     = NULL;
    lastRow        = -1;
    unauthSlicePtr = NULL;
    unauthRow      = -1;
    routeMoved     = FALSE;
    stopProcessing = FALSE;

    if (firstRow >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        pFirstSlice = pFirstSlice->prevSlice;
        firstRow   = 0;
    }

    if ( fmCustomTreePredecessor(&pRouteTable->prefixTree,
                                 &pRoutePrefix->prefix,
                                 (void **) &pTmpPrefixLength,
                                 (void **) &pPrevPrefix) != FM_OK )
    {
        pPrevPrefix = NULL;
    }

    if (pPrevPrefix != NULL)
    {
        if ((pTempRoute = GetLastPrefixRoute(pPrevPrefix)) != NULL)
        {
            pLastSlice = pTempRoute->routeSlice;
            lastRow = pTempRoute->tcamSliceRow - 1;

            if (lastRow < 0)
            {
                pLastSlice = pLastSlice->nextSlice;
                lastRow = FM10000_FFU_ENTRIES_PER_SLICE - 1;
            }
        }
    }
    else
    {
        pLastSlice = pRouteTable->firstSlice;
        lastRow = FM10000_FFU_ENTRIES_PER_SLICE - 1;
    }

    if ( (pFirstSlice == NULL) || (pLastSlice == NULL) )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING, "Either first or last slice == NULL\n");
        stopProcessing = TRUE;
    }

    if (!stopProcessing)
    {
        /* Try to find an existing empty row within the specified range */
        if ( FindEmptyRowWithinSliceRange(sw,
                                          pFirstSlice,
                                          firstRow,
                                          pLastSlice,
                                          lastRow,
                                          optimize,
                                          &slicePtr,
                                          &row,
                                          &unauthSlicePtr,
                                          &unauthRow) )
        {
            routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
            stopProcessing = TRUE;
        }

        /* If there is another prefix length before the current one, recurse
         * trying to move that prefix up.
         */
        else if (pPrevPrefix != NULL)
        {
            /* Try to recurse */
            pTempRoute = GetLastPrefixRoute(pPrevPrefix);

            if (pTempRoute != NULL)
            {
                slicePtr  = pTempRoute->routeSlice;
                row       = pTempRoute->tcamSliceRow;

                if ( MoveLastRowUpWithinPrefix(sw,
                                               pRouteTable,
                                               pPrevPrefix,
                                               unauthSliceOK,
                                               optimize) )
                {
                    routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
                    stopProcessing = TRUE;
                }
            }
        }
    }

    if (!stopProcessing)
    {
        /* If we found an empty but unauthorized slice/row earlier, and the
         * caller is willing to use an unauthorized slice/row, return it */
        if ( unauthSliceOK && (unauthSlicePtr != NULL) )
        {
            slicePtr = unauthSlicePtr;
            row      = unauthRow;
            routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
        }

        /* Either there isn't a previous prefix, or it couldn't be moved.
         * Try to create an empty row within our prefix range. */
        else if ( ClearCascadeRowWithinSliceRange(sw,
                                                  pFirstSlice,
                                                  firstRow,
                                                  pLastSlice,
                                                  lastRow,
                                                  &slicePtr,
                                                  &row,
                                                  unauthSliceOK,
                                                  optimize) )
        {
            routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING, routeMoved, "%s\n",
                       routeMoved? "Route has been moved" : "Cannot move route");

}   /* end MoveRouteUpWithinPrefix */




/*****************************************************************************/
/** MoveRouteDownWithinPrefix
 * \ingroup intRouter
 *
 * \desc            Moves a specified route down to somewhere else within
 *                  the range of rows currently used by its prefix, or to
 *                  a row after the current last row used by its prefix,
 *                  if possible.  Will recurse, moving prefixes after the
 *                  route's prefix, if necessary.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table.
 *
 * \param[in]       pRoute points to the route to be moved
 *
 * \param[in]       unauthSliceOK is TRUE if the route may be moved into
 *                  an unauthorized slice.  This may be done during TCAM
 *                  repartitioning in order to move some routes temporarily
 *                  in order to make room for other routes that need to be
 *                  moved before this route can be moved to an authorized
 *                  slice.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \return          TRUE if successful and the first route's original
 *                  slice and row are now free.
 * \return          FALSE if there was no room to move the row.
 *
 *****************************************************************************/
static fm_bool MoveRouteDownWithinPrefix(fm_int                  sw,
                                         fm10000_RoutingTable *  pRouteTable,
                                         fm10000_TcamRouteEntry *pRoute,
                                         fm_bool                 unauthSliceOK,
                                         fm_bool                 optimize)
{
    fm10000_RoutePrefix *   pRoutePrefix;
    fm10000_TcamRouteEntry *pTempRoute;
    fm10000_RouteSlice *    slicePtr;
    fm_int                  row;
    fm10000_RouteSlice *    unauthSlicePtr;
    fm_int                  unauthRow;
    fm10000_RoutePrefix *   pNextPrefix;
    fm_int *                pTmpPrefixLength;
    fm10000_RouteSlice *    pFirstSlice;
    fm10000_RouteSlice *    pLastSlice;
    fm_int                  firstRow;
    fm_int                  lastRow;
    fm_bool                 routeMoved;
    fm_bool                 stopProcessing;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteTable=%p, route=%p, unauthSliceOK=%d, optimize=%d\n",
                 sw,
                 (void *) pRouteTable,
                 (void *) pRoute,
                 unauthSliceOK,
                 optimize );

    pRoutePrefix    = pRoute->routePrefix;
    pFirstSlice     = pRoute->routeSlice;
    firstRow        = pRoute->tcamSliceRow - 1;
    pNextPrefix     = NULL;
    pLastSlice      = NULL;
    unauthSlicePtr  = NULL;
    unauthRow       = -1;
    routeMoved      = FALSE;
    stopProcessing  = FALSE;

    if (firstRow < 0)
    {
        pFirstSlice = pFirstSlice->nextSlice;
        firstRow = FM10000_FFU_ENTRIES_PER_SLICE - 1;
    }

    if ( fmCustomTreeSuccessor(&pRouteTable->prefixTree,
                               &pRoutePrefix->prefix,
                               (void **) &pTmpPrefixLength,
                               (void **) &pNextPrefix) != FM_OK )
    {
        pNextPrefix = NULL;
    }

    if (pNextPrefix != NULL)
    {
        pTempRoute = GetFirstPrefixRoute(pNextPrefix);
        if (pTempRoute == NULL)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Prefix without route entry\n");
            stopProcessing = TRUE;
        }
        else
        {
            pLastSlice = pTempRoute->routeSlice;
            lastRow = pTempRoute->tcamSliceRow + 1;

            if (lastRow >= FM10000_FFU_ENTRIES_PER_SLICE)
            {
                pLastSlice = pLastSlice->prevSlice;
                lastRow   = 0;
            }
        }
    }
    else
    {
        pLastSlice = pRouteTable->lastSlice;
        lastRow   = 0;
    }

    if ( (pFirstSlice == NULL) || (pLastSlice == NULL) )
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING, "Either first or last slice == NULL\n");
        stopProcessing = TRUE;
    }

    if (!stopProcessing)
    {
        /* Try to find an existing empty row within the specified range */
        if ( FindEmptyRowWithinSliceRange(sw,
                                          pFirstSlice,
                                          firstRow,
                                          pLastSlice,
                                          lastRow,
                                          optimize,
                                          &slicePtr,
                                          &row,
                                          &unauthSlicePtr,
                                          &unauthRow) )
        {
            routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
            stopProcessing = TRUE;
        }

        /* If there is another prefix length after the current one, recurse
         * trying to move that prefix down.
         */
        else if (pNextPrefix != NULL)
        {
            /* Try to recurse */
            pTempRoute = GetFirstPrefixRoute(pNextPrefix);
            if (pTempRoute == NULL)
            {
                FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                                   FALSE,
                                   "FALSE: Prefix without route entry\n");
            }
            slicePtr  = pTempRoute->routeSlice;
            row       = pTempRoute->tcamSliceRow;

            if ( MoveFirstRowDownWithinPrefix(sw,
                                              pRouteTable,
                                              pNextPrefix,
                                              unauthSliceOK,
                                              optimize) )
            {
                routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
                stopProcessing = TRUE;
            }
        }
    }

    if (!stopProcessing)
    {
        /* If we found an empty but unauthorized slice/row earlier, and the
         * caller is willing to use an unauthorized slice/row, return it */
        if ( unauthSliceOK && (unauthSlicePtr != NULL) )
        {
            slicePtr = unauthSlicePtr;
            row      = unauthRow;
            routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
        }

        /* Either there isn't a next prefix, or it couldn't be moved.
         * Try to create an empty row within our prefix range. */
        else if ( ClearCascadeRowWithinSliceRange(sw,
                                                  pFirstSlice,
                                                  firstRow,
                                                  pLastSlice,
                                                  lastRow,
                                                  &slicePtr,
                                                  &row,
                                                  unauthSliceOK,
                                                  optimize) )
        {
            routeMoved = MoveRoute(sw, pRoute, slicePtr, row);
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING, routeMoved, "%s\n",
                       routeMoved? "Route has been moved" : "Cannot move route");

}   /* end MoveRouteDownWithinPrefix */




/*****************************************************************************/
/** MoveRouteElsewhereWithinPrefix
 * \ingroup intRouter
 *
 * \desc            Moves a specified route to somewhere else within
 *                  the range of rows currently used by its prefix. It first
 *                  tries to move it down in the prefix. If that fails,
 *                  it tries to move it up in the prefix.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table.
 *
 * \param[in]       pRoute points to the route to be moved
 *
 * \param[in]       unauthSliceOK is TRUE if the route may be moved into
 *                  an unauthorized slice.  This may be done during TCAM
 *                  repartitioning in order to move some routes temporarily
 *                  in order to make room for other routes that need to be
 *                  moved before this route can be moved to an authorized
 *                  slice.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \return          TRUE if successful and the first route's original
 *                  slice and row are now free.
 * \return          FALSE if there was no room to move the row.
 *
 *****************************************************************************/
static fm_bool MoveRouteElsewhereWithinPrefix(fm_int                  sw,
                                              fm10000_RoutingTable *  pRouteTable,
                                              fm10000_TcamRouteEntry *pRoute,
                                              fm_bool                 unauthSliceOK,
                                              fm_bool                 optimize)
{
    fm_bool success;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteTable=%p, pRoute=%p, unauthSliceOK=%d, "
                 "optimize=%d\n",
                 sw,
                 (void *) pRouteTable,
                 (void *) pRoute,
                 unauthSliceOK,
                 optimize );

    success = MoveRouteDownWithinPrefix(sw,
                                        pRouteTable,
                                        pRoute,
                                        unauthSliceOK,
                                        optimize);

    if (!success)
    {
        success = MoveRouteUpWithinPrefix(sw,
                                          pRouteTable,
                                          pRoute,
                                          unauthSliceOK,
                                          optimize);
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       success,
                       "%s\n",
                       (success) ? "Success!" : "Failure!");

}   /* end MoveRouteElsewhereWithinPrefix */




/*****************************************************************************/
/* ClassifyRoute
 * \ingroup intRouterRoute
 *
 * \desc            Classifies a route, returning a number of identifying
 *                  characteristics.
 *
 * \note            This function only returns physical route information -
 *                  it does not use any simulated route tables.
 *
 * \param[in]       sw contains the switch number.
 *
 * \param[in]       pRoute points to the route.
 *
 * \param[out]      pRouteInfo points to caller-allocated storage into which
 *                  the route information will be placed.
 *
 *****************************************************************************/
static void ClassifyRoute(fm_int             sw,
                          fm_intRouteEntry * pRoute,
                          fm10000_RouteInfo *pRouteInfo)
{
    fm_status            err;
    fm_int               iter;
    fm_ipAddr            destAddr;
    fm_int               vlanPrefix;
    fm_uint16            vlanMask;
    fm_multicastAddress *multicast;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw=%d, pRoute = %p, pRouteInfo = %p\n",
                  sw,
                  (void *) pRoute,
                  (void *) pRouteInfo );

    if (pRoute == NULL || pRouteInfo == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid arguments, pRouteInfo will not be initialized\n");
    }
    else
    {
        FM_CLEAR(*pRouteInfo);
        vlanPrefix = 0;

        fmGetRouteDestAddress(&pRoute->route, &destAddr);

        pRouteInfo->isIPv6 = destAddr.isIPv6;
        pRouteInfo->ipAddrSize = destAddr.isIPv6 ? 4 : 1; 
        pRouteInfo->routeType    = GetRouteType(&pRoute->route);
        pRouteInfo->prefixLength = pRoute->prefix;

        /* Get pointers to data structures for each route type */
        switch (pRoute->route.routeType)
        {
            case FM_ROUTE_TYPE_UNICAST:
                pRouteInfo->isUnicast = TRUE;
                pRouteInfo->dstIpPtr = pRoute->route.data.unicast.dstAddr.addr;
                pRouteInfo->dstPrefix = pRoute->route.data.unicast.prefixLength;
                pRouteInfo->vrid = pRoute->route.data.unicast.vrid;
                break;

            case FM_ROUTE_TYPE_UNICAST_ECMP:
                pRouteInfo->isUnicast = TRUE;
                pRouteInfo->dstIpPtr = pRoute->route.data.unicastECMP.dstAddr.addr;
                pRouteInfo->dstPrefix = pRoute->route.data.unicastECMP.prefixLength;
                pRouteInfo->vrid = pRoute->route.data.unicastECMP.vrid;
                break;

            case FM_ROUTE_TYPE_MULTICAST:
                pRouteInfo->isUnicast = FALSE;
                multicast = &pRoute->route.data.multicast;

                switch (multicast->addressType)
                {
                    case FM_MCAST_ADDR_TYPE_UNKNOWN:
                    case FM_MCAST_ADDR_TYPE_L2MAC_VLAN:
                        break;

                    case FM_MCAST_ADDR_TYPE_DSTIP:
                        pRouteInfo->dstIpPtr = multicast->info.dstIpRoute.dstAddr.addr;
                        pRouteInfo->dstPrefix = multicast->info.dstIpRoute.dstPrefixLength;
                        break;

                    case FM_MCAST_ADDR_TYPE_DSTIP_VLAN:
                        pRouteInfo->dstIpPtr = multicast->info.dstIpVlanRoute.dstAddr.addr;
                        pRouteInfo->dstPrefix = multicast->info.dstIpVlanRoute.dstPrefixLength;
                        pRouteInfo->vlan = multicast->info.dstIpVlanRoute.vlan;
                        vlanPrefix = multicast->info.dstIpVlanRoute.vlanPrefixLength;
                        break;

                    case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP:
                        pRouteInfo->dstIpPtr = multicast->info.dstSrcIpRoute.dstAddr.addr;
                        pRouteInfo->dstPrefix   = multicast->info.dstSrcIpRoute.dstPrefixLength;
                        pRouteInfo->srcIpPtr = multicast->info.dstSrcIpRoute.srcAddr.addr;
                        pRouteInfo->srcPrefix   = multicast->info.dstSrcIpRoute.srcPrefixLength;
                        break;

                    case FM_MCAST_ADDR_TYPE_DSTIP_SRCIP_VLAN:
                        pRouteInfo->dstIpPtr = multicast->info.dstSrcIpVlanRoute.dstAddr.addr;
                        pRouteInfo->dstPrefix = multicast->info.dstSrcIpVlanRoute.dstPrefixLength;
                        pRouteInfo->srcIpPtr = multicast->info.dstSrcIpVlanRoute.srcAddr.addr;
                        pRouteInfo->srcPrefix = multicast->info.dstSrcIpVlanRoute.srcPrefixLength;
                        pRouteInfo->vlan = multicast->info.dstSrcIpVlanRoute.vlan;
                        vlanPrefix  = multicast->info.dstSrcIpVlanRoute.vlanPrefixLength;
                        break;

                }   /* end switch (multicast->addressType) */
                break;

            default:
                break;
        }

        pRouteInfo->vroff = fmGetVirtualRouterOffset(sw, pRouteInfo->vrid);
        pRouteInfo->routeTable = (pRouteInfo->routeType != FM10000_ROUTE_TYPE_UNUSED) ? 
                                GetRouteTable(sw, pRouteInfo->routeType) : NULL;

        if (pRouteInfo->routeTable != NULL)
        {
            err = GetPrefixRecord(sw,
                                  pRouteInfo->routeTable,
                                  pRouteInfo->prefixLength,
                                  &pRouteInfo->routePrefix,
                                  &pRouteInfo->nextPrefix,
                                  &pRouteInfo->prevPrefix);
            /* only log the error */
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Cannot get the prefix record\n");
            }
        }

        /* build the vlan mask */
        vlanMask = 0;
        for (iter = 0 ; iter < vlanPrefix ; iter++)
        {
            vlanMask |= 1 << (11 - iter);
        }
        pRouteInfo->vlanMask = vlanMask;

        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                     "routeType=%d, isIPv6=%d, isUnicast=%d, routeTable=%p, "
                     "dstValuePtr=%p, dstPrefix=%d,\n"
                     "    srcValuePtr=%p, srcPrefix=%d, ipAddrSize=%d, vrid=%d, "
                     "vroff=%d, vlan=%u, vlanMask=0x%x,\n    prefixLength=%d, "
                     "routePrefix=%p, prevPrefix=%p, nextPrefix=%p\n",
                     pRouteInfo->routeType,
                     pRouteInfo->isIPv6,
                     pRouteInfo->isUnicast,
                     (void *) pRouteInfo->routeTable,
                     (void *) pRouteInfo->dstIpPtr,
                     pRouteInfo->dstPrefix,
                     (void *) pRouteInfo->srcIpPtr,
                     pRouteInfo->srcPrefix,
                     pRouteInfo->ipAddrSize,
                     pRouteInfo->vrid,
                     pRouteInfo->vroff,
                     pRouteInfo->vlan,
                     pRouteInfo->vlanMask,
                     pRouteInfo->prefixLength,
                     (void *) pRouteInfo->routePrefix,
                     (void *) pRouteInfo->nextPrefix,
                     (void *) pRouteInfo->prevPrefix);
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_ROUTING);

}   /* end ClassifyRoute */




/*****************************************************************************/
/** IsMcastRouteType
 * \ingroup intRouter
 *
 * \desc            Returns TRUE if the route type matches one of the
 *                  multicast route types.
 *
 * \param[in]       routeType contains the route type.
 *
 * \return          TRUE if routeType is a multicast route type.
 * \return          FALSE if routeType is not a multicast route type.
 *
 *****************************************************************************/
static fm_bool IsMcastRouteType(fm10000_RouteTypes routeType)
{
    fm_bool isMcast;

    switch (routeType)
    {
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
        case FM10000_ROUTE_TYPE_V4SG:
        case FM10000_ROUTE_TYPE_V6SG:
        case FM10000_ROUTE_TYPE_V4DV:
        case FM10000_ROUTE_TYPE_V6DV:
#endif
        case FM10000_ROUTE_TYPE_V4DSV:
        case FM10000_ROUTE_TYPE_V6DSV:
            isMcast = TRUE;
            break;

        default:
            isMcast = FALSE;
            break;

    }   /* end switch (routeType) */

    return isMcast;

}   /* end IsMcastRouteType */




/*****************************************************************************/
/* ValidateRouteInformation
 * \ingroup intRouterRoute
 *
 * \desc            Validates some specific members of the route infor
 *                  structure.
 *
 * \param[in]       sw contains the switch number.
 *
 * \param[in]       pRoute points to the route to be validated.
 * 
 * \param[in]       pRouteInfo points to route information structure to be
 *                  validated.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if pRouteInfo is a NULL pointer or
 *                  invalid vroff
 * \return          FM_ERR_NO_FFU_RES_FOUND if no suitable FFU resources are
 *                  available for this route.
 * \return          FM_ERR_ALREADY_EXISTS if the route already exists
 * 
 *****************************************************************************/
static fm_status ValidateRouteInformation(fm_int             sw,
                                          fm_intRouteEntry * pRoute,
                                          fm10000_RouteInfo *pRouteInfo)
{
    fm_status   err;
    fm10000_TcamRouteEntry tcamRouteKey;
    fm10000_TcamRouteEntry *tcamRoute;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p, pRouteInfo=%p\n",
                 sw,
                 (void*) pRoute,
                 (void *) pRouteInfo);

    err = FM_OK;

    /* Validate that:
     * -pRouteInfo is not null
     * -virtual router is valid. */
    if ( (pRouteInfo == NULL) || (pRouteInfo->vroff < 0) )
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    /* validate that:
     *  -routeTable exists
     *  -routePrefix exists
     *  -there are slices attached to the route, otherwise the route type
     *   is not supported.  */
    else if (pRouteInfo->routeTable == NULL ||
             pRouteInfo->routePrefix == NULL || 
             pRouteInfo->routeTable->firstSlice == NULL)
    {
        err = FM_ERR_NO_FFU_RES_FOUND;
    }
    else
    {
        FM_CLEAR(tcamRouteKey);
        tcamRouteKey.routePtr = pRoute;

        /* check if the route already exists */
        err = fmCustomTreeFind( &pRouteInfo->routeTable->tcamRouteRouteTree,
                                &tcamRouteKey,
                                (void **) &tcamRoute );
        if (err == FM_ERR_NOT_FOUND)
        {
            /* this is the expected error code */
            err = FM_OK;
        }
        else if (err == FM_OK)
        {
            err = FM_ERR_ALREADY_EXISTS;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end ValidateRouteInformation */




/*****************************************************************************/
/** InitFfuRouteAction
 * \ingroup intRouter
 *
 * \desc            Initializes a FFU action structure.
 * 
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       pRoute is the points to route.
 *
 * \param[in]       pRouteInfo points to the associated route info structure.
 *
 * \param[out]      pFfuAction points to the structure to be initialized.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one or more argument are invalid.
 * \return          FM_FAIL If action is route-arp and the ECMP Group is not
 *                  defined.
 * 
 *****************************************************************************/
static fm_status InitFfuRouteAction(fm_int             sw,
                                    fm_intRouteEntry * pRoute,
                                    fm10000_RouteInfo *pRouteInfo,
                                    fm_ffuAction *     pFfuAction)
{
    fm_status              err;
    fm_switch *            switchPtr;
    fm10000_switch *       pSwitchExt;
    fm_int                 mcastGroupHdnl;
    fm_intMulticastGroup * pMcastGroup;
    fm10000_EcmpGroup *    pEcmpGroupExt;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p, pRouteInfo=%p, pFfuAction=%p\n",
                 sw,
                 (void* ) pRoute,
                 (void *) pRouteInfo,
                 (void *) pFfuAction);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pFfuAction == NULL ||
        pRoute == NULL ||
        pRouteInfo == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        FM_CLEAR(*pFfuAction);
        mcastGroupHdnl = 0;

        pFfuAction->precedence = pRouteInfo->isUnicast ? pSwitchExt->unicastMinPrecedence : pSwitchExt->multicastMinPrecedence;

        /* Set the route action:
         *     route using ARP entry
         *     route/switch using logical port & MTABLE
         */
        switch (pRoute->action.action)
        {
            case FM_ROUTE_ACTION_ROUTE:
                if (!pRouteInfo->isUnicast)
                {
                    /* Find the multicast group */
                    mcastGroupHdnl = pRoute->route.data.multicast.mcastGroup;
                    pMcastGroup = fmFindMcastGroup(sw, mcastGroupHdnl);

                    if (pMcastGroup == NULL)
                    {
                        err = FM_ERR_INVALID_MULTICAST_GROUP;
                        break;
                    }

                    if (pMcastGroup->l2SwitchingOnly)
                    {
                         if (pMcastGroup->l2FloodSet)
                         {
                             pFfuAction->action = FM_FFU_ACTION_ROUTE_FLOOD_DEST;
                         }
                         else
                         {
                             /* Note that the hardware action is actually switch,
                              * not route.
                              */
                             pFfuAction->action = FM_FFU_ACTION_ROUTE_LOGICAL_PORT;
                         }
                    }
                    else if (pMcastGroup->l3SwitchingOnly &&
                             pMcastGroup->l3FloodSet)
                    {
                        pFfuAction->action = FM_FFU_ACTION_ROUTE_FLOOD_DEST;
                    }
                    else
                    {
                        pFfuAction->action = FM_FFU_ACTION_ROUTE_ARP;
                    }
                }
                else
                {
                    pFfuAction->action = FM_FFU_ACTION_ROUTE_ARP;
                }
                break;

            case FM_ROUTE_ACTION_DROP:
                pFfuAction->action = FM_FFU_ACTION_ROUTE_ARP;
                break;

            case FM_ROUTE_ACTION_RPF_FAILURE:
                pFfuAction->action = FM_FFU_ACTION_ROUTE_LOGICAL_PORT;
                break;

            case FM_ROUTE_ACTION_NOP:
                pFfuAction->action = FM_FFU_ACTION_NOP;
                break;

            case FM_ROUTE_ACTION_TRIGGER:
                pFfuAction->action = FM_FFU_ACTION_SET_TRIGGER;
                pFfuAction->data.trigger.value = pRoute->action.data.trigger.triggerValue;
                pFfuAction->data.trigger.mask  = pRoute->action.data.trigger.triggerMask;
                break;

            default:
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_UNSUPPORTED);
        }

        if ( (err == FM_OK) && (pFfuAction->action == FM_FFU_ACTION_ROUTE_ARP) )
        {
            pEcmpGroupExt = switchPtr->ecmpGroups[pRoute->ecmpGroupId]->extension;

            if ( pEcmpGroupExt == NULL )
            {
                /* If action is route-arp, an ECMP Group pointer is required */
                err = FM_FAIL;
            }

        }   /* end if ( (err == FM_OK)  && ... */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end InitFfuRouteAction */




/*****************************************************************************/
/** SetFfuRouteAction
 * \ingroup intRouter
 *
 * \desc            Completes the initialization of a FFU action structure.
 *                  The FFU action structure must be previously intialized
 *                  by InitFfyRouteAction.
 * 
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       pRoute is the points to route.
 *
 * \param[in]       pRouteInfo points to the associated route info structure.
 *
 * \param[out]      pFfuAction points to the FFU action structure.
 * 
 * \param[out]      pRuleValid points to a caller allocated storage where this
 *                  function will return TRUE if the FFU rule is valid or FALSE
 *                  otherwise.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one or more argument are invalid.
 * \return          FM_FAIL If action is route-arp and the ECMP Group is not
 *                  defined.
 * 
 *****************************************************************************/
static fm_status SetFfuRouteAction(fm_int             sw,
                                   fm_intRouteEntry * pRoute,
                                   fm10000_RouteInfo *pRouteInfo,
                                   fm_ffuAction *     pFfuAction,
                                   fm_bool *          pRuleValid)
{
    fm_status              err;
    fm_switch *            switchPtr;
    fm10000_switch *       pSwitchExt;
    fm_int                 mcastGroupHdnl;
    fm_intMulticastGroup * pMcastGroup;
    fm_int                 arpIndex;
    fm_int                 pathCount;
    fm_int                 pathCountType;
    fm_bool                ecmpGroupValid;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p, pRouteInfo=%p, pFfuAction=%p, pRuleValid=%p\n",
                 sw,
                 (void* ) pRoute,
                 (void *) pRouteInfo,
                 (void *) pFfuAction,
                 (void *) pRuleValid);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pFfuAction == NULL ||
        pRoute == NULL ||
        pRouteInfo == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        arpIndex = 0;
        pathCount = 0;
        if (pRoute->ecmpGroupId >= 0) 
        {
            err = fm10000ValidateEcmpGroupState(sw, 
                                                pRoute->ecmpGroupId, 
                                                &ecmpGroupValid);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

            if ( (err == FM_OK) && (ecmpGroupValid == TRUE) )
            {
                err = fm10000GetECMPGroupArpInfo(sw,
                                                 pRoute->ecmpGroupId,
                                                 NULL,
                                                 &arpIndex,
                                                 &pathCount,
                                                 &pathCountType);
                FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            }
        }

        /* assume that the rule is valid */
        *pRuleValid = TRUE;

        /* Complete the Action definition */
        switch (pFfuAction->action)
        {
            case FM_FFU_ACTION_ROUTE_ARP:
                /* Connect the route to the arp entry or entries */
                pFfuAction->data.arp.arpIndex = arpIndex;

                if ( (arpIndex > 0) && (pathCount > 0) && pRoute->active )
                {
                    pFfuAction->data.arp.count = pathCount;
                }
                else
                {
                    pFfuAction->data.arp.count = 1;
                    *pRuleValid = FALSE;
                }
                break;

            case FM_FFU_ACTION_ROUTE_LOGICAL_PORT:
                if (pRoute->action.action == FM_ROUTE_ACTION_RPF_FAILURE)
                {
                    pFfuAction->data.logicalPort = FM_PORT_RPF_FAILURE;
                }
                else
                {
                    /* Connect the route to the multicast group logical port */
                    mcastGroupHdnl = pRoute->route.data.multicast.mcastGroup;
                    pMcastGroup = fmFindMcastGroup(sw, mcastGroupHdnl);

                    if (pMcastGroup == NULL)
                    {
                        err = FM_FAIL;
                        *pRuleValid = FALSE;

                        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Invalid multicast group, mcastGropHandle=%d\n",
                                     mcastGroupHdnl);
                    }
                    else
                    {
                        pFfuAction->data.logicalPort = pMcastGroup->logicalPort;
                    }
                }

                break;

            case FM_FFU_ACTION_ROUTE_FLOOD_DEST:
                /* Connect the route to the multicast group logical port */
                mcastGroupHdnl = pRoute->route.data.multicast.mcastGroup;
                pMcastGroup = fmFindMcastGroup(sw, mcastGroupHdnl);

                if (pMcastGroup == NULL)
                {
                    err = FM_FAIL;
                    *pRuleValid = FALSE;
                    
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Invalid multicast group, mcastGropHandle=%d\n",
                                 mcastGroupHdnl);
                }
                else
                {
                    pFfuAction->data.logicalPort = pMcastGroup->logicalPort;
                }
                break;

            case FM_FFU_ACTION_SET_FLAGS:
                break;

            case FM_FFU_ACTION_SET_TRIGGER:
                break;

            default:
                *pRuleValid = FALSE;
                break;
        }


    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetFfuRouteAction */




/*****************************************************************************/
/** AllocateAndInitTcamRoute
 * \ingroup intRouter
 *
 * \desc            Allocates and initializes a new TCAM route.
 * 
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       pRoute points to the new route.
 *
 * \param[in]       pRouteInfo points to the route info struture for the new
 *                  route.
 * 
 * \param[out]      ppTcamRoute caller allocated storage where this function
 *                  returns the pointer to the created TCAM route if
 *                  successful or NULL otherwhise.
 * 
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if one or more arguments is invalid.
 *
 *****************************************************************************/
static fm_status AllocateAndInitTcamRoute(fm_int                   sw,
                                          fm_intRouteEntry *       pRoute,
                                          fm10000_RouteInfo *      pRouteInfo,
                                          fm10000_TcamRouteEntry **ppTcamRoute)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm10000_switch *        pSwitchExt;
    fm10000_TcamRouteEntry *pTcamRouteLoc;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p, pRouteInfo=%p, ppTcamRoute=%p\n",
                 sw,
                 (void* ) pRoute,
                 (void *) pRouteInfo,
                 (void *) ppTcamRoute);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);

    if (pRoute == NULL ||
        pRouteInfo == NULL ||
        ppTcamRoute == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *ppTcamRoute = NULL;
        pTcamRouteLoc = fmAlloc( sizeof(fm10000_TcamRouteEntry) );

        if (pTcamRouteLoc == NULL)
        {
            err = FM_ERR_NO_MEM;
        }
        else
        {
            FM_CLEAR(*pTcamRouteLoc);

            pTcamRouteLoc->stateTable        = &pSwitchExt->routeStateTable;
            pTcamRouteLoc->routePtr          = pRoute;
            pTcamRouteLoc->action            = pRoute->action;
            pTcamRouteLoc->routeTable        = pRouteInfo->routeTable;
            pTcamRouteLoc->routePrefix       = pRouteInfo->routePrefix;
            pTcamRouteLoc->routeSlice        = NULL;

            if (pRoute->ecmpGroupId == -1)
            {
                pTcamRouteLoc->ecmpGroup = NULL;
            }
            else
            {
                pTcamRouteLoc->ecmpGroup =
                    switchPtr->ecmpGroups[pRoute->ecmpGroupId]->extension;
            }
            
            pTcamRouteLoc->tcamSliceRow      = -1;
            FM_DLL_INIT_NODE(pTcamRouteLoc, nextTcamRoute, prevTcamRoute);
            FM_DLL_INIT_NODE(pTcamRouteLoc, nextPrefixRoute, prevPrefixRoute);

            /* Add new tcam route into sort-by-route tcam route tree */
            err = fmCustomTreeInsert(&pRouteInfo->routeTable->tcamRouteRouteTree,
                                     pTcamRouteLoc,
                                     pTcamRouteLoc);

            if (err == FM_OK)
            {
                *ppTcamRoute = pTcamRouteLoc;
            }
            else
            {
                fmFree(pTcamRouteLoc);
                pTcamRouteLoc = NULL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocateAndInitTcamRoute */




/*****************************************************************************/
/** FindFfuEntryForNewRoute
 * \ingroup intRouter
 *
 * \desc            Tries to find a place for a new route in the FFU.
 * 
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       pRoute points to the route to be added.
 *
 * \param[in]       pRouteInfo points to the route info structure associated
 *                  to the route to be added.
 * 
 * \param[out]      ppDestSlice points to a caller allocater strorage where
 *                  this functions will return a pointer to the route slice
 *                  structure, if a place is found or NULL otherwise.
 * 
 * \param[out]      pDestRow points to a caller allocated storage wher this
 *                  function will return the slice destination row, if a place
 *                  is found or -1 otherwise.
 *
 * \return          FM_OK if succesful.
 * \return          FM_ERR_INVALID_ARGUMENT if argument are out of range or
 *                  NULL pointers
 * \return          FM_ERR_NO_FFU_RES_FOUND if no place was found for the new
 *                  route.
 *
 *****************************************************************************/
static fm_status FindFfuEntryForNewRoute(fm_int                  sw,
                                         fm_intRouteEntry *      pRoute,
                                         fm10000_RouteInfo *     pRouteInfo,
                                         fm10000_RouteSlice **   ppDestSlice,
                                         fm_int *                pDestRow)
{
    fm_status              err;
    fm10000_RoutingTable * pRouteTable;
    fm10000_RoutePrefix *  pRoutePrefix;
    fm10000_RoutePrefix *  pPrevPrefix;
    fm10000_RoutePrefix *  pNextPrefix;
    fm10000_TcamRouteEntry *pTempTcamRoute;
    fm10000_RouteSlice *   pFirstSearchSlicePtr;
    fm_int                 firstSearchRow;
    fm10000_RouteSlice *   pLastSearchSlicePtr;
    fm_int                 lastSearchRow;
    fm_bool                optimize;
    fm_bool                entryFound;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p, pRouteInfo=%p, ppDestSlice=%p, pDestRow=%p\n",
                 sw,
                 (void *) pRoute,
                 (void *) pRouteInfo,
                 (void *) ppDestSlice,
                 (void *) pDestRow);

    err = FM_OK;
    entryFound = FALSE;

    if (pRoute == NULL ||
        pRouteInfo == NULL ||
        ppDestSlice == NULL ||
        pDestRow == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        *ppDestSlice = NULL;
        *pDestRow      = -1;

        pRouteTable = pRouteInfo->routeTable;


        /* Is this the very first route for this route type? */
        if (pRouteTable->firstTcamRoute == NULL)
        {
            optimize = TRUE;

            do
            {
                /* Yes, search for an available row. During the first pass,
                 * the optimize flag will be true. At the end of the pass, it
                 * will be toggled to false, and the attempt will be made again.
                 * When it is set to true again at the bottom of the loop, the
                 * while condition will be false and the loop will end. */
                if ( FindEmptyRowWithinSliceRange(sw,
                                                  pRouteTable->firstSlice,
                                                  FM10000_FFU_ENTRIES_PER_SLICE - 1,
                                                  pRouteTable->lastSlice,
                                                  0,
                                                  optimize,
                                                  ppDestSlice,
                                                  pDestRow,
                                                  NULL,
                                                  NULL) )
                {
                    /* found an available row, write the route */
                    entryFound = TRUE;
                    break;
                }

                /* No free rows found, try to empty one */
                if ( ClearCascadeRowWithinSliceRange(sw,
                                                     pRouteTable->firstSlice,
                                                     FM10000_FFU_ENTRIES_PER_SLICE - 1,
                                                     pRouteTable->lastSlice,
                                                     0,
                                                     ppDestSlice,
                                                     pDestRow,
                                                     FALSE,
                                                     optimize) )
                {
                    /* we were able to free up a row, write the route */
                    entryFound = TRUE;
                    break;
                }

                optimize = !optimize;
            }
            while (!optimize);
        }

        if (entryFound != TRUE)
        {
            /* We will attempt to find a location for the route twice. During the first
             * attempt, we will try to only use slices that are unshared with other
             * routes or where every slice in the cascade and in the cascades from
             * other route types that share slices with this route type are fully
             * shared. In other words, we are attempting to maximize use of slices
             * that are exclusive to this route type and minimize use of slices
             * that are partially exclusive to other route types.
             * If we are unable to find a place for the route using that criteria,
             * we'll try to put it anyplace we can. */
            *ppDestSlice = NULL;
            *pDestRow      = -1;
            optimize     = TRUE;
            pRoutePrefix = pRouteInfo->routePrefix;
            pPrevPrefix = pRouteInfo->prevPrefix;
            pNextPrefix = pRouteInfo->nextPrefix;

            do
            {
                /* are there any empty rows within the range of rows used by this prefix? */
                if ( FindEmptyRowWithinPrefixRange(sw,
                                                   pRoutePrefix,
                                                   ppDestSlice,
                                                   pDestRow,
                                                   FALSE,
                                                   optimize) )
                {
                    /* yes, write route to row */
                    entryFound = TRUE;
                    break;
                }

                /* Try to move the last route of the previous prefix up */
                if (pPrevPrefix != NULL)
                {
                    pTempTcamRoute = GetLastPrefixRoute(pPrevPrefix);

                    if (pTempTcamRoute != NULL)
                    {
                        *ppDestSlice = pTempTcamRoute->routeSlice;
                        *pDestRow = pTempTcamRoute->tcamSliceRow;

                        if ( MoveRouteUpWithinPrefix(sw,
                                                     pRouteTable,
                                                     pTempTcamRoute,
                                                     FALSE,
                                                     optimize) )
                        {
                            entryFound = TRUE;
                            break;
                        }
                    }
                }

                /* Try to move the first route of the next prefix down */
                if (pNextPrefix != NULL)
                {
                    pTempTcamRoute = GetFirstPrefixRoute(pNextPrefix);

                    if (pTempTcamRoute != NULL)
                    {
                        *ppDestSlice = pTempTcamRoute->routeSlice;
                        *pDestRow = pTempTcamRoute->tcamSliceRow;

                        if ( MoveRouteDownWithinPrefix(sw,
                                                       pRouteTable,
                                                       pTempTcamRoute,
                                                       FALSE,
                                                       optimize) )
                        {
                            entryFound = TRUE;
                            break;
                        }
                    }
                }

                /* Determine the slice/row boundaries between the last row of the
                 * previous prefix and the first route of the next prefix.
                 * If there is no previous prefix, use the top of the first slice.
                 * If there is no next prefix, use the bottom of the last slice. */
                pFirstSearchSlicePtr = NULL;
                firstSearchRow      = -1;
                pLastSearchSlicePtr  = NULL;
                lastSearchRow       = -1;

                if (pPrevPrefix != NULL)
                {
                    pTempTcamRoute = GetLastPrefixRoute(pPrevPrefix);

                    if (pTempTcamRoute != NULL)
                    {
                        pFirstSearchSlicePtr = pTempTcamRoute->routeSlice;
                        firstSearchRow      = pTempTcamRoute->tcamSliceRow - 1;

                        if (firstSearchRow < 0)
                        {
                            pFirstSearchSlicePtr = pFirstSearchSlicePtr->nextSlice;
                            firstSearchRow      = FM10000_FFU_ENTRIES_PER_SLICE - 1;
                        }
                    }
                }
                else
                {
                    pFirstSearchSlicePtr = pRouteTable->firstSlice;
                    firstSearchRow      = FM10000_FFU_ENTRIES_PER_SLICE - 1;
                }

                if (pNextPrefix != NULL)
                {
                    pTempTcamRoute = GetFirstPrefixRoute(pNextPrefix);

                    if (pTempTcamRoute != NULL)
                    {
                        pLastSearchSlicePtr = pTempTcamRoute->routeSlice;
                        lastSearchRow      = pTempTcamRoute->tcamSliceRow + 1;

                        if (lastSearchRow >= FM10000_FFU_ENTRIES_PER_SLICE)
                        {
                            pLastSearchSlicePtr = pLastSearchSlicePtr->prevSlice;
                            lastSearchRow      = 0;
                        }
                    }
                }
                else
                {
                    pLastSearchSlicePtr = pRouteTable->lastSlice;
                    lastSearchRow      = 0;
                }

                /* If we have valid boundaries */
                if ( (pFirstSearchSlicePtr != NULL)
                    && (pLastSearchSlicePtr != NULL) )
                {
                    /* Try to find an empty row between the last route of the
                     * previous prefix and the first route of the next prefix. */
                    if ( FindEmptyRowWithinSliceRange(sw,
                                                      pFirstSearchSlicePtr,
                                                      firstSearchRow,
                                                      pLastSearchSlicePtr,
                                                      lastSearchRow,
                                                      optimize,
                                                      ppDestSlice,
                                                      pDestRow,
                                                      NULL,
                                                      NULL) )
                    {
                        /* found an empty row, write route to row */
                        entryFound = TRUE;
                        break;
                    }

                    /* Try to create an empty row between the last route of the
                     * previous prefix and the first route of the next prefix. */
                    if ( ClearCascadeRowWithinSliceRange(sw,
                                                        pFirstSearchSlicePtr,
                                                        firstSearchRow,
                                                        pLastSearchSlicePtr,
                                                        lastSearchRow,
                                                        ppDestSlice,
                                                        pDestRow,
                                                        FALSE,
                                                        optimize) )
                    {
                        /* created an empty row, write route to row */
                        entryFound = TRUE;
                        break;
                    }
                }

                optimize = !optimize;
            }
            while (!optimize);
        }

        if (entryFound != TRUE)
        {
            /* Route could not be added, clear destSlicePtr */
            *ppDestSlice = NULL;
            *pDestRow      = -1;
            err = FM_ERR_NO_FFU_RES_FOUND;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end FindFfuEntryForNewRoute */




/*****************************************************************************/
/** SetFFuRuleKeyForRoute
 * \ingroup intRouter
 *
 * \desc            Configures a FFU rule key for the given route.
 * 
 * \param[in]       sw is the switch on which to operate
 * 
 * \param[in]       pRouteInfo points to the route info structure associated
 *                  to the route to be added.
 * 
 * \param[out]      ruleKeyArray points to caller allocated FFU key/mask array
 *                  to be filled by this function.
 *
 * \return          FM_OK if succesful.
 * \return          FM_ERR_INVALID_ARGUMENT if argument are out of range or
 *                  NULL pointers
 * \return          FM_ERR_NO_FFU_RES_FOUND if no place was found for the new
 *                  route.
 *
 *****************************************************************************/
static fm_status SetFFuRuleKeyForRoute(fm_int                sw,
                                       fm10000_RouteInfo *   pRouteInfo,
                                       fm_fm10000FfuSliceKey ruleKeyArray[FM10000_MAX_ROUTE_SLICE_WIDTH])
{
    fm_status       err;
    fm_int          index;
    fm_uint32       dstMask[4];
    fm_uint32       srcMask[4];
    fm_int          kase;    

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteInfo=%p, ruleKeyArray=%p\n",
                 sw,
                 (void *) pRouteInfo,
                 (void *) ruleKeyArray);

    err = FM_OK;

    if (pRouteInfo == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

    if (err == FM_OK)
    {
        kase = GetRouteCase(pRouteInfo->routeType);
        
        if (kase == TCAM_CASE_INVALID)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
    }

    if (err == FM_OK)
    {
        /* copy route data into key and keyMask arrays */
        FM_CLEAR(dstMask);
        FM_CLEAR(srcMask);

        fmConvertPrefixLengthToDestMask(pRouteInfo->dstPrefix,
                                        pRouteInfo->isIPv6,
                                        dstMask);

        if (pRouteInfo->srcIpPtr != NULL)
        {
            fmConvertPrefixLengthToDestMask(pRouteInfo->srcPrefix,
                                            pRouteInfo->isIPv6,
                                            srcMask);
        }

        for (index = 0 ; index < pRouteInfo->ipAddrSize ; index++)
        {
            ruleKeyArray[index].key      = (fm_uint64) ntohl(pRouteInfo->dstIpPtr[index]);
            ruleKeyArray[index].keyMask  = (fm_uint64) dstMask[index];

            if (pRouteInfo->srcIpPtr != NULL)
            {
                ruleKeyArray[index + pRouteInfo->ipAddrSize].key     = (fm_uint64) ntohl(pRouteInfo->srcIpPtr[index]);
                ruleKeyArray[index + pRouteInfo->ipAddrSize].keyMask = (fm_uint64) srcMask[index];
            }
            
            if (!pRouteInfo->isUnicast)
            {
                ruleKeyArray[index + pRouteInfo->ipAddrSize].kase.value = kase;
                ruleKeyArray[index + pRouteInfo->ipAddrSize].kase.mask  = 0xf;
            }

            ruleKeyArray[index].kase.value = kase;
            ruleKeyArray[index].kase.mask = 0xf;
        }

        /* Add physical/virtual router id */
        if (pRouteInfo->isUnicast)
        {
            ruleKeyArray[0].key |=
                ( ((fm_uint64) pRouteInfo->vroff + 1 ) << 4)
                << FM10000_FFU_TCAM_36_32;
            
            if (pRouteInfo->vrid != FM_ROUTER_ANY)
            {
                ruleKeyArray[0].keyMask |=
                    ( (fm_uint64) 0xf0 ) << FM10000_FFU_TCAM_36_32;
            }
        }

        /* Add VLAN information if needed */
        switch (pRouteInfo->routeType)
        {
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
            case FM10000_ROUTE_TYPE_V4DV:
                ruleKeyArray[1].key     = (fm_uint64) pRouteInfo->vlan;
                ruleKeyArray[1].keyMask = (fm_uint64) pRouteInfo->vlanMask;
                break;
#endif
            case FM10000_ROUTE_TYPE_V4DSV:
                ruleKeyArray[2].key        = (fm_uint64) pRouteInfo->vlan;
                ruleKeyArray[2].keyMask    = (fm_uint64) pRouteInfo->vlanMask;
                ruleKeyArray[2].kase.value = kase;
                ruleKeyArray[2].kase.mask  = 0xf;
                break;
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
            case FM10000_ROUTE_TYPE_V6DV:
#endif
            case FM10000_ROUTE_TYPE_V6DSV:
                ruleKeyArray[FM10000_SIZEOF_IPV6_DSV_ROUTE - 1].key        =
                    (fm_uint64) pRouteInfo->vlan;
                ruleKeyArray[FM10000_SIZEOF_IPV6_DSV_ROUTE - 1].keyMask    =
                    (fm_uint64) pRouteInfo->vlanMask;
                ruleKeyArray[FM10000_SIZEOF_IPV6_DSV_ROUTE - 1].kase.value =
                    kase;
                ruleKeyArray[FM10000_SIZEOF_IPV6_DSV_ROUTE - 1].kase.mask  =
                    0xf;
                break;

            default:
                break;

        }   /* end switch (routeInfo->routeType) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetFFuRuleKeyForRoute */




/*****************************************************************************/
/** CompareTcamRoutesBySlice
 * \ingroup intRouter
 *
 * \desc            Compare fm10000 internal TCAM Route entries using
 *                  slice number and slice row.
 *
 * \param[in]       pFirstRoute points to the first route.
 *
 * \param[in]       pSecondRoute points to the second route.
 *
 * \return          -1 if the first route sorts before the second.
 * \return           0 if the routes are identical.
 * \return           1 if the first route sorts after the second.
 *
 *****************************************************************************/
static fm_int CompareTcamRoutesBySlice(const void *pFirstRoute, 
                                       const void *pSecondRoute)
{
    fm_int  diff;


    diff = 0;

    if (pFirstRoute == NULL || pSecondRoute == NULL)
    {
        FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                      "Invalid argument\n");
    }
    else   
    {
            /* TCAM routes are sorted by slice and row,
             * from highest to lowest */
            diff = ((fm10000_TcamRouteEntry*)pSecondRoute)->routeSlice->firstTcamSlice -
                   ((fm10000_TcamRouteEntry*)pFirstRoute)->routeSlice->firstTcamSlice;

            if (diff == 0)
            {
                diff = ((fm10000_TcamRouteEntry*)pSecondRoute)->tcamSliceRow -
                       ((fm10000_TcamRouteEntry*)pFirstRoute)->tcamSliceRow;
            }

            if (diff > 0)
            {
                diff = 1;
            }
            else if (diff < 0)
            {
                diff = -1;
            }
    }

    return diff;

}   /* end CompareTcamRoutesBySlice */




/*****************************************************************************/
/** SetFFUSliceUsageForRoutingState
 * \ingroup intRoute
 *
 * \desc            Sets the FFU slice usage boundaries for a routing state.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pNewAllocations points to the record containing the new
 *                  FFU slice assignments.  If NULL, the existing first
 *                  and last slice elements aren't changed.  In either case,
 *                  the tcam slice structures are updated to reflect whether
 *                  they can be used for unicast and/or multicast routes.
 *
 * \param[in]       pRouteState points to the routing state structure.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if pRouteState is a NULL pointer.
 * \return          FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE if the unicast
 *                  slice range does not provide room for at least one
 *                  cascade for the unicast route type (unless the first/last
 *                  slice numbers are set to -1).
 * \return          FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE if the multicast
 *                  slice range does not provide room for at least one
 *                  cascade for the multicast route type (unless the first/last
 *                  slice numbers are set to -1).
 *
 *****************************************************************************/
static fm_status SetFFUSliceUsageForRoutingState(fm_int                  sw,
                                                 fm_ffuSliceAllocations *pNewAllocations,
                                                 fm10000_RoutingState *  pRouteState)
{
    fm_status               err;
    fm_int                  index;
    fm10000_RouteTcamSlice *pTcamSlice;
    fm_bool                 isUsable;
    fm_bool                 wasUsable;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pNewAllocations=%p, pRouteState=%p\n",
                 sw,
                 (void *) pNewAllocations,
                 (void *) pRouteState);

    err = FM_OK;

    if (pRouteState == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        if (pNewAllocations != NULL)
        {
            pRouteState->ipv4UcastFirstTcamSlice = pNewAllocations->ipv4UnicastFirstSlice;
            pRouteState->ipv4UcastLastTcamSlice  = pNewAllocations->ipv4UnicastLastSlice;
            pRouteState->ipv4McastFirstTcamSlice = pNewAllocations->ipv4MulticastFirstSlice;
            pRouteState->ipv4McastLastTcamSlice  = pNewAllocations->ipv4MulticastLastSlice;
            pRouteState->ipv6UcastFirstTcamSlice = pNewAllocations->ipv6UnicastFirstSlice;
            pRouteState->ipv6UcastLastTcamSlice  = pNewAllocations->ipv6UnicastLastSlice;
            pRouteState->ipv6McastFirstTcamSlice = pNewAllocations->ipv6MulticastFirstSlice;
            pRouteState->ipv6McastLastTcamSlice  = pNewAllocations->ipv6MulticastLastSlice;

            if ( (pRouteState->ipv4UcastFirstTcamSlice < 0) &&
                 (pRouteState->ipv4McastFirstTcamSlice < 0) && 
                 (pRouteState->ipv6UcastFirstTcamSlice < 0) && 
                 (pRouteState->ipv6McastFirstTcamSlice < 0) )
            {
                pRouteState->routeFirstTcamSlice = -1;
                pRouteState->routeLastTcamSlice  = -1;
                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING, "Routing disabled\n");
            }
            else
            {
                err = NormalizeFFUSliceRanges(sw, pRouteState);
            }
        }

        if (err == FM_OK)
        {
            for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
            {
                pTcamSlice = &pRouteState->routeTcamSliceArray[index];

                if (pTcamSlice->ipv4UcastOK ||
                    pTcamSlice->ipv4McastOK || 
                    pTcamSlice->ipv6UcastOK || 
                    pTcamSlice->ipv6McastOK)
                {
                    wasUsable = TRUE;
                }
                else
                {
                    wasUsable = FALSE;
                }

                isUsable = FALSE;

                if ( (index >= pRouteState->ipv4UcastFirstTcamSlice) && 
                     (index <= pRouteState->ipv4UcastLastTcamSlice) )
                {
                    pTcamSlice->ipv4UcastOK = TRUE;
                    isUsable = TRUE;
                }
                else
                {
                    pTcamSlice->ipv4UcastOK = FALSE;
                }

                if ( (index >= pRouteState->ipv4McastFirstTcamSlice) &&
                     (index <= pRouteState->ipv4McastLastTcamSlice) )
                {
                    pTcamSlice->ipv4McastOK = TRUE;
                    isUsable = TRUE;
                }
                else
                {
                    pTcamSlice->ipv4McastOK = FALSE;
                }

                if ( (index >= pRouteState->ipv6UcastFirstTcamSlice) &&
                     (index <= pRouteState->ipv6UcastLastTcamSlice) )
                {
                    pTcamSlice->ipv6UcastOK = TRUE;
                    isUsable = TRUE;
                }
                else
                {
                    pTcamSlice->ipv6UcastOK = FALSE;
                }

                if ( (index >= pRouteState->ipv6McastFirstTcamSlice) &&
                     (index <= pRouteState->ipv6McastLastTcamSlice) )
                {
                    pTcamSlice->ipv6McastOK = TRUE;
                    isUsable = TRUE;
                }
                else
                {
                    pTcamSlice->ipv6McastOK = FALSE;
                }

                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                             "TCAM slice %d:%s%s%s%s%s%s\n",
                             index,
                             (pTcamSlice->ipv4UcastOK) ? " V4U" : "",
                             (pTcamSlice->ipv4McastOK) ? " V4M" : "",
                             (pTcamSlice->ipv6UcastOK) ? " V6U" : "",
                             (pTcamSlice->ipv6McastOK) ? " V6M" : "",
                             (pTcamSlice->ipv4UcastOK || 
                              pTcamSlice->ipv4McastOK || 
                              pTcamSlice->ipv6UcastOK ||
                              pTcamSlice->ipv6McastOK) ? "" : " N/A",
                             (wasUsable) ? ( (isUsable) ? "" : " Now Unusable" ) : ( (isUsable) ? " Now Usable" : "" ) );
            }   /* end for (index = 0 ;...) */
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetFFUSliceUsageForRoutingState */




/*****************************************************************************/
/** RemoveSliceFromRoute
 * \ingroup intRouter
 *
 * \desc            Removes a route slice from a route, removing any
 *                  remaining routes using that slice and releasing all
 *                  TCAM resources reserved by that route slice.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the routing table.
 *
 * \param[in]       pSlice points to the route slice.
 *
 * \param[in]       updateHardware is TRUE if the hardware registers should
 *                  also be updated, FALSE to simply remove the slice in
 *                  the software tables.
 *
 * \return          TRUE if successful.
 *
 *****************************************************************************/
static fm_bool RemoveSliceFromRoute(fm_int                sw,
                                    fm10000_RoutingTable *pRouteTable,
                                    fm10000_RouteSlice *  pSlice,
                                    fm_bool               updateHardware)
{
    fm10000_TcamRouteEntry *first;
    fm10000_TcamRouteEntry *last;
    fm_status               err;
    fm_int                  i;
    fm_int                  kase;
    fm10000_RouteTcamSlice *tcamSlicePtr;
    fm10000_TcamSliceCase  *curCasePtr;
    fm_int                  caseUsed;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, pRouteTable = %p, slicePtr = %p (%d-%d)\n",
                  sw,
                  (void *) pRouteTable,
                  (void *) pSlice,
                  (pSlice != NULL) ? pSlice->firstTcamSlice : -1,
                  (pSlice != NULL) ? pSlice->lastTcamSlice : -1 );
    
    if ((pSlice == NULL ) || (pRouteTable == NULL))
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                           FALSE,
                           "FALSE\n");
    }

    if (pSlice->stateTable != NULL)
    {
        /* Override hardware updates if virtual state */
        if (!pSlice->stateTable->actualState)
        {
            updateHardware = FALSE;
        }
    }

    /* release all routes that use this slice */
    first = GetFirstRouteForSlice(pSlice);
    last  = GetLastRouteForSlice(pSlice);

    /* first remove all routes on this slice from the route chain */
    if (first != NULL)
    {
        UnlinkRoutes(pRouteTable, first, last);
    }


    if ( updateHardware && (pSlice->highestRow >= 0) )
    {
        /* Disable the slice in the FFU */
        err = UnconfigureSliceCascade(sw, pSlice);
        if (err != FM_OK)
        {
            FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                               FALSE,
                               "FALSE\n");
        }
    }

    caseUsed = GetRouteCase(pSlice->routeType);
    
    if (caseUsed == TCAM_CASE_INVALID)
    {
        FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                           FALSE,
                           "FALSE\n"); 
    }

    /* Clear tcam slice information */
    for (i = pSlice->firstTcamSlice ; i <= pSlice->lastTcamSlice ; i++)
    {
        tcamSlicePtr = GetTcamSlicePtr(sw, pRouteTable->stateTable, i);
        curCasePtr   = &tcamSlicePtr->caseInfo[caseUsed];
        
        curCasePtr->parentTcamSlice = -1;
        curCasePtr->routeType       = FM10000_ROUTE_TYPE_UNUSED;
        curCasePtr->routeSlice      = NULL;
        curCasePtr->tcamSlice       = NULL;
        
        for (kase = 0 ; kase < FM10000_ROUTE_NUM_CASES ; kase++)
        {
            if (tcamSlicePtr->caseInfo[kase].routeType !=
                FM10000_ROUTE_TYPE_UNUSED)
            {
                break;
            }
        }

        if (kase >= FM10000_ROUTE_NUM_CASES)
        {
            tcamSlicePtr->inUse = 0;
        }
    }

    /* remove the slice from the linked list */
    RemoveSlice(pRouteTable, pSlice);

    /* release the slice from use */
    pSlice->inUse = FALSE;

    fmFree(pSlice);
    
    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING, TRUE, "TRUE\n");
}   /* end RemoveSliceFromRoute */




/*****************************************************************************/
/** ComparePrefixRoutes
 * \ingroup intRouter
 *
 * \desc            Compare fm10000 routes for a route prefix list.
 *
 * \param[in]       pFirstRoute points to the first route.
 *
 * \param[in]       pSecondRoute points to the second route.
 *
 * \return          -1 if the first route sorts before the second.
 * \return           0 if the routes are identical.
 * \return           1 if the first route sorts after the second.
 *
 *****************************************************************************/
static fm_int ComparePrefixRoutes(const void *pFirstRoute, 
                                  const void *pSecondRoute)
{
    fm_int  diff;


    diff = 0;

    if (pFirstRoute == NULL || pSecondRoute == NULL)
    {
        FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                      "Invalid argument\n");
    }
    else
    {
        /* sort by prefix, highest to lowest */
        diff = ((fm10000_TcamRouteEntry*)pSecondRoute)->routePrefix->prefix - 
               ((fm10000_TcamRouteEntry*)pFirstRoute)->routePrefix->prefix;

        if (diff == 0)
        {
            diff = ((fm10000_TcamRouteEntry*)pSecondRoute)->routeSlice->firstTcamSlice - 
                   ((fm10000_TcamRouteEntry*)pFirstRoute)->routeSlice->firstTcamSlice;
            if (diff == 0)
            {
                diff = ((fm10000_TcamRouteEntry*)pSecondRoute)->tcamSliceRow - 
                       ((fm10000_TcamRouteEntry*)pFirstRoute)->tcamSliceRow;
            }
        }
        if (diff > 0)
        {
            diff = 1;
        }
        else if (diff < 0)
        {
            diff = -1;
        }
    }

    return diff;

}   /* end ComparePrefixRoutes */




/*****************************************************************************/
/** ComparePrefix
 * \ingroup intRouter
 *
 * \desc            Compare prefix for a sorted prefix tree.
 *
 * \param[in]       pFirstPrefix points to the first prefix.
 *
 * \param[in]       pSecondPrefix points to the second prefix.
 *
 * \return          -1 if the first prefix sorts before the second.
 * \return           0 if the prefix are identical.
 * \return           1 if the first prefix sorts after the second.
 *
 *****************************************************************************/
static fm_int ComparePrefix(const void *pFirstPrefix, 
                            const void *pSecondPrefix)
{
    fm_int      diff;


    diff = 0;

    /* sort by prefix, highest to lowest */
    diff = ((fm10000_RoutePrefix*)pSecondPrefix)->prefix -
           ((fm10000_RoutePrefix*)pFirstPrefix)->prefix;

    if (diff < 0)
    {
        diff = -1;
    }
    else if (diff > 0)
    {
        diff = 1;
    }

    return diff;

}   /* end ComparePrefix */




/*****************************************************************************/
/** InsertTcamRouteCallback
 * \ingroup intRouter
 *
 * \desc            Function called when a TCAM route is being inserted
 *                  into a custom tree.
 *
 * \param[in]       pKey points to the key value.
 *
 * \param[in]       pValue points to the node being inserted.
 *
 * \param[in]       pPrevKey points to the key of the previous node or NULL.
 *
 * \param[in]       pPrevValue points to the node that preceeds the inserted
 *                  node.
 *
 * \param[in]       pNextKey points to the key of the following node or NULL.
 *
 * \param[in]       pNextValue points to the node that follows the inserted
 *                  node.
 *
 *****************************************************************************/
static void InsertTcamRouteCallback(const void *pKey,
                                    void *      pValue,
                                    const void *pPrevKey,
                                    void *      pPrevValue,
                                    const void *pNextKey,
                                    void *      pNextValue)
{
    fm10000_TcamRouteEntry *pNode;
    fm10000_TcamRouteEntry *pPrevNode;

    FM_NOT_USED(pKey);
    FM_NOT_USED(pPrevKey);
    FM_NOT_USED(pNextKey);
    FM_NOT_USED(pNextValue);

    pNode  = (fm10000_TcamRouteEntry *)pValue;
    pPrevNode = (fm10000_TcamRouteEntry *)pPrevValue;

    FM_DLL_INSERT_AFTER(pNode->routeTable,
                        firstTcamRoute,
                        lastTcamRoute,
                        pPrevNode,
                        nextTcamRoute,
                        prevTcamRoute,
                        pNode);

}   /* end InsertTcamRouteCallback */




/*****************************************************************************/
/** DeleteTcamRouteCallback
 * \ingroup intRouter
 *
 * \desc            Function called when an internal route is being removed
 *                  from a custom tree.
 *
 * \param[in]       pKey points to the key value.
 *
 * \param[in]       pValue points to the node being inserted.
 *
 * \param[in]       pPrevKey points to the key of the previous node or NULL.
 *
 * \param[in]       pPrevValue points to the node that preceeds the node
 *                  being removed.
 *
 * \param[in]       pNextKey points to the key of the following node or NULL.
 *
 * \param[in]       pNextValue points to the node that follows the node
 *                  being removed.
 *
 *****************************************************************************/
static void DeleteTcamRouteCallback(const void *pKey,
                                    void *      pValue,
                                    const void *pPrevKey,
                                    void *      pPrevValue,
                                    const void *pNextKey,
                                    void       *pNextValue)
{
    fm10000_TcamRouteEntry *pNode;

    FM_NOT_USED(pKey);
    FM_NOT_USED(pPrevKey);
    FM_NOT_USED(pPrevValue);
    FM_NOT_USED(pNextKey);
    FM_NOT_USED(pNextValue);

    pNode = (fm10000_TcamRouteEntry*)pValue;

    FM_DLL_REMOVE_NODE(pNode->routeTable,
                       firstTcamRoute,
                       lastTcamRoute,
                       pNode,
                       nextTcamRoute,
                       prevTcamRoute);

}   /* end DeleteTcamRouteCallback */




/*****************************************************************************/
/** InsertPrefixRouteCallback
 * \ingroup intRouter
 *
 * \desc            Function called when a TCAM route is being inserted
 *                  into a prefix custom tree.
 *
 * \param[in]       pKey points to the key value.
 *
 * \param[in]       pValue points to the node being inserted.
 *
 * \param[in]       pPrevKey points to the key of the previous node or NULL.
 *
 * \param[in]       pPrevValue points to the node that preceeds the inserted
 *                  node.
 *
 * \param[in]       pNextKey points to the key of the following node or NULL.
 *
 * \param[in]       pNextValue points to the node that follows the inserted
 *                  node.
 *
 *****************************************************************************/
static void InsertPrefixRouteCallback(const void *pKey,
                                      void *      pValue,
                                      const void *pPrevKey,
                                      void *      pPrevValue,
                                      const void *pNextKey,
                                      void *      pNextValue)
{
    fm10000_TcamRouteEntry *pNode;
    fm10000_TcamRouteEntry *pPrevNode;

    FM_NOT_USED(pKey);
    FM_NOT_USED(pPrevKey);
    FM_NOT_USED(pNextKey);
    FM_NOT_USED(pNextValue);

    pNode = (fm10000_TcamRouteEntry*)pValue;
    pPrevNode = (fm10000_TcamRouteEntry*)pPrevValue;

    FM_DLL_INSERT_AFTER(pNode->routePrefix,
                        firstTcamRoute,
                        lastTcamRoute,
                        pPrevNode,
                        nextPrefixRoute,
                        prevPrefixRoute,
                        pNode);

}   /* end InsertPrefixRouteCallback */




/*****************************************************************************/
/** DeletePrefixRouteCallback
 * \ingroup intRouter
 *
 * \desc            Function called when an internal route is being removed
 *                  from a custom tree.
 *
 * \param[in]       pKey points to the key value.
 *
 * \param[in]       pValue points to the node being inserted.
 *
 * \param[in]       pPrevKey points to the key of the previous node or NULL.
 *
 * \param[in]       pPrevValue points to the node that preceeds the node
 *                  being removed.
 *
 * \param[in]       pNextKey points to the key of the following node or NULL.
 *
 * \param[in]       pNextValue points to the node that follows the node
 *                  being removed.
 *
 *****************************************************************************/
static void DeletePrefixRouteCallback(const void *pKey,
                                      void *      pValue,
                                      const void *pPrevKey,
                                      void *      pPrevValue,
                                      const void *pNextKey,
                                      void *      pNextValue)
{
    fm10000_TcamRouteEntry *pNode;

    FM_NOT_USED(pKey);
    FM_NOT_USED(pPrevKey);
    FM_NOT_USED(pPrevValue);
    FM_NOT_USED(pNextKey);
    FM_NOT_USED(pNextValue);

    pNode = (fm10000_TcamRouteEntry*)pValue;

    FM_DLL_REMOVE_NODE(pNode->routePrefix,
                       firstTcamRoute,
                       lastTcamRoute,
                       pNode,
                       nextPrefixRoute,
                       prevPrefixRoute);

}   /* end DeletePrefixRouteCallback */




/*****************************************************************************/
/** InsertPrefixCallback
 * \ingroup intRouter
 *
 * \desc            Function called when a prefix is being inserted into a
 *                  prefix custom tree.
 *
 * \param[in]       pKey points to the key value.
 *
 * \param[in]       pValue points to the node being inserted.
 *
 * \param[in]       pPrevKey points to the key of the previous node or NULL.
 *
 * \param[in]       pPrevValue points to the node that preceeds the inserted
 *                  node.
 *
 * \param[in]       pNextKey points to the key of the following node or NULL.
 *
 * \param[in]       pNextValue points to the node that follows the inserted
 *                  node.
 *
 *****************************************************************************/
static void InsertPrefixCallback(const void *pKey,
                                 void *      pValue,
                                 const void *pPrevKey,
                                 void *      pPrevValue,
                                 const void *pNextKey,
                                 void *      pNextValue)
{
    fm10000_RoutePrefix *pNode;

    FM_NOT_USED(pKey);
    FM_NOT_USED(pPrevKey);
    FM_NOT_USED(pPrevValue);
    FM_NOT_USED(pNextKey);
    FM_NOT_USED(pNextValue);

    pNode = (fm10000_RoutePrefix*)pValue;

    fmCustomTreeInit(&pNode->routeTree, ComparePrefixRoutes);
    fmCustomTreeRequestCallbacks(&pNode->routeTree,
                                 InsertPrefixRouteCallback,
                                 DeletePrefixRouteCallback);

}   /* end InsertPrefixCallback */




/*****************************************************************************/
/** DeletePrefixCallback
 * \ingroup intRouter
 *
 * \desc            Function called when a prefix is being removed from a
 *                  prefix custom tree.
 *
 * \param[in]       pKey points to the key value.
 *
 * \param[in]       pValue points to the node being inserted.
 *
 * \param[in]       pPrevKey points to the key of the previous node or NULL.
 *
 * \param[in]       pPrevValue points to the node that preceeds the node
 *                  being removed.
 *
 * \param[in]       pNextKey points to the key of the following node or NULL.
 *
 * \param[in]       pNextValue points to the node that follows the node
 *                  being removed.
 *
 *****************************************************************************/
static void DeletePrefixCallback(const void *pKey,
                                 void *      pValue,
                                 const void *pPrevKey,
                                 void *      pPrevValue,
                                 const void *pNextKey,
                                 void *      pNextValue)
{
    fm10000_RoutePrefix *pNode;

    FM_NOT_USED(pKey);
    FM_NOT_USED(pPrevKey);
    FM_NOT_USED(pPrevValue);
    FM_NOT_USED(pNextKey);
    FM_NOT_USED(pNextValue);

    pNode = (fm10000_RoutePrefix*)pValue;

    fmCustomTreeDestroy(&pNode->routeTree, NULL);

}   /* end DeletePrefixCallback */




/*****************************************************************************/
/** InsertSliceIntoRouteTable
 * \ingroup intRouter
 *
 * \desc            Inserts a route slice into its route table's linked list.
 *
 * \param[in]       pRouteTable points to the routing table.
 *
 * \param[in]       pSlice points to the route slice.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status InsertSliceIntoRouteTable(fm10000_RoutingTable *pRouteTable,
                                           fm10000_RouteSlice *  pSlice)
{
    fm_status           err;
    fm10000_RouteSlice *pPrevSlice;
    fm10000_RouteSlice *pCurSlice;
    fm_int              tcamSlice;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "routeTable = %p, slicePtr = %p\n",
                 (void*)pRouteTable,
                 (void*)pSlice);
    
    err = FM_OK;

    /* argument validation */
    if (pRouteTable == NULL ||
        pSlice == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                      "Slice info: (%d-%d)\n",             
                      pSlice->firstTcamSlice,
                      pSlice->lastTcamSlice );

        tcamSlice = pSlice->firstTcamSlice;

        /* Insert slice into route table, sorted from highest slice to lowest */
        pPrevSlice = NULL;
        pCurSlice  = GetFirstSlice(pRouteTable);

        while (1)
        {
            if (pCurSlice == NULL)
            {
                InsertSliceAfter(pRouteTable, pPrevSlice, pSlice);
                break;
            }

            if (pCurSlice->firstTcamSlice < tcamSlice)
            {
                /* insert new slice before this slice */
                InsertSliceBefore(pRouteTable, pCurSlice, pSlice);
                break;
            }

            pPrevSlice = pCurSlice;
            pCurSlice  = GetNextSlice(pCurSlice);
        }

        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "Inserted slice %p into route table %p "
                     "after slice %p and before slice %p\n",
                     (void *) pSlice,
                     (void *) pRouteTable,
                     (void *) pSlice->prevSlice,
                     (void *) pSlice->nextSlice);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end InsertSliceIntoRouteTable */




/*****************************************************************************/
/** GetFirstTcamSliceForRouteType
 * \ingroup intRouter
 *
 * \desc            Returns the first and last TCAM slices authorized for use
 *                  by the specified route type.  Returns FALSE if this route
 *                  type is not authorized to use any TCAM slices.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       routeType is the route type to be checked.
 *
 * \param[in]       pStateTable points to the routing state table.
 *
 * \param[out]      firstSlice points to caller-allocated memory into which
 *                  the first authorized TCAM slice number will be stored.
 *
 * \param[out]      lastSlice points to caller-allocated memory into which
 *                  the last authorized TCAM slice number will be stored.
 *
 * \return          TRUE if the first and last TCAM slice number have been
 *                  stored into firstSlice and lastSlice.
 * \return          FALSE if the route type is not authorized to use any TCAM
 *                  slices.
 *
 *****************************************************************************/
static fm_bool GetFirstTcamSliceForRouteType(fm_int                sw,
                                             fm10000_RouteTypes    routeType,
                                             fm10000_RoutingState *pStateTable,
                                             fm_int *              firstSlice,
                                             fm_int *              lastSlice)
{
    fm_int  firstAuthorizedSlice;
    fm_int  lastAuthorizedSlice;
    fm_bool routeAuthorized;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, routeType=%d, pStateTable=%p, firstSlice=%p, lastSlice=%p\n",
                 sw,
                 routeType,
                 (void *) pStateTable,
                 (void *) firstSlice,
                 (void *) lastSlice );

    FM_NOT_USED(sw);
    routeAuthorized = FALSE;


    if (pStateTable == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Invalid Argument\n");
    }
    else
    {
        switch (routeType)
        {
            case FM10000_ROUTE_TYPE_V4U:
                firstAuthorizedSlice = pStateTable->ipv4UcastFirstTcamSlice;
                lastAuthorizedSlice  = pStateTable->ipv4UcastLastTcamSlice;
                break;

    #ifdef ENABLE_DIVERSE_MCAST_SUPPORT
            case FM10000_ROUTE_TYPE_V4DV:
            case FM10000_ROUTE_TYPE_V4SG:
    #endif
            case FM10000_ROUTE_TYPE_V4DSV:
                firstAuthorizedSlice = pStateTable->ipv4McastFirstTcamSlice;
                lastAuthorizedSlice  = pStateTable->ipv4McastLastTcamSlice;
                break;

            case FM10000_ROUTE_TYPE_V6U:
                firstAuthorizedSlice = pStateTable->ipv6UcastFirstTcamSlice;
                lastAuthorizedSlice  = pStateTable->ipv6UcastLastTcamSlice;
                break;

    #ifdef ENABLE_DIVERSE_MCAST_SUPPORT
            case FM10000_ROUTE_TYPE_V6DV:
            case FM10000_ROUTE_TYPE_V6SG:
    #endif
            case FM10000_ROUTE_TYPE_V6DSV:
                firstAuthorizedSlice = pStateTable->ipv6McastFirstTcamSlice;
                lastAuthorizedSlice  = pStateTable->ipv6McastLastTcamSlice;
                break;

            default:
                firstAuthorizedSlice = -1;
                lastAuthorizedSlice  = -1;
                break;
        }

        if (firstAuthorizedSlice >= 0)
        {
            if (firstSlice != NULL)
            {
                *firstSlice = firstAuthorizedSlice;
            }

            if (lastSlice != NULL)
            {
                *lastSlice = lastAuthorizedSlice;
            }
            routeAuthorized = TRUE;

            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                         "firstAuthorizedSlice=%d, lastAuthorizedSlice=%d\n",
                         firstAuthorizedSlice,
                         lastAuthorizedSlice);
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       routeAuthorized,
                       "Route type %d is %s TCAM slices\n",
                        routeType,
                        routeAuthorized ?  "authorized to use" : "NOT authorized to use any" );

}   /* end GetFirstTcamSliceForRouteType */




/*****************************************************************************/
/** AllocatePrefix
 * \ingroup intRouter
 *
 * \desc            Allocates and initializes a route prefix record.
 *
 * \param[in]       pSwitchExt points to the FM10000-specific switch state
 *                  extension.
 *
 * \param[in]       prefixLength contains the prefix length.
 *
 * \return          Pointer to a prefix.
 * \return          NULL if memory could not be allocated.
 *
 *****************************************************************************/
static fm10000_RoutePrefix *AllocatePrefix(fm10000_switch *pSwitchExt,
                                           fm_int          prefixLength)
{
    fm10000_RoutePrefix *pPrefix;

    FM_NOT_USED(pSwitchExt);

    pPrefix = fmAlloc( sizeof(fm10000_RoutePrefix) );

    if (pPrefix != NULL)
    {
        FM_CLEAR(*pPrefix);

        pPrefix->prefix = prefixLength;

        FM_DLL_INIT_NODE(pPrefix, nextPrefix, prevPrefix);
        FM_DLL_INIT_LIST(pPrefix, firstTcamRoute, lastTcamRoute);
    }

    return pPrefix;

}   /* end AllocatePrefix */




/*****************************************************************************/
/** GetPrefixRecord
 * \ingroup intRouter
 *
 * \desc            Returns the prefix record for the specified prefix,
 *                  creating a new one if the desired record doesn't exist.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the routing table.
 *
 * \param[in]       prefixLength is the prefix length for the prefix record.
 *
 * \param[out]      ppPrefix points to caller-allocated storage into which
 *                  the prefix pointer will be placed. It may be NULL.
 *
 * \param[out]      ppNextPrefix points to caller-allocated storage into
 *                  which the next prefix pointer will be placed. It may be
 *                  NULL.
 *
 * \param[out]      ppPrevPrefix points to caller-allocated storage into
 *                  which the previous prefix pointer will be placed. It may
 *                  be NULL.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
static fm_status GetPrefixRecord(fm_int                sw,
                                 fm10000_RoutingTable *pRouteTable,
                                 fm_int                prefixLength,
                                 fm10000_RoutePrefix **ppPrefix,
                                 fm10000_RoutePrefix **ppNextPrefix,
                                 fm10000_RoutePrefix **ppPrevPrefix)
{
    fm_status            err;
    fm10000_switch *     pSwitchExt;
    fm10000_RoutePrefix *pRoutePrefix;
    fm10000_RoutePrefix *pNextPrefix;
    fm10000_RoutePrefix *pPrevPrefix;
    fm_int *             pTmpPrefixLength;
    fm_bool              created;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteTable=%p, prefixLength=%d, ppPrefix=%p, ppNextPrefix=%p, ppPrevPrefix=%p\n",
                 sw,
                 (void *) pRouteTable,
                 prefixLength,
                 (void *) ppPrefix,
                 (void *) ppNextPrefix,
                 (void *) ppPrevPrefix);

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    created = FALSE;

    /* argument validation */
    if (pRouteTable == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pRoutePrefix = NULL;
        pNextPrefix = NULL;
        pPrevPrefix = NULL;
        pTmpPrefixLength = NULL;

        err = fmCustomTreeFind(&pRouteTable->prefixTree,
                               &prefixLength,
                               (void **) &pRoutePrefix);

        if (err == FM_ERR_NOT_FOUND)
        {
            /* allocate a new prefix entry */
            pRoutePrefix = AllocatePrefix(pSwitchExt, prefixLength);

            if (pRoutePrefix == NULL)
            {
                err = FM_ERR_NO_MEM;
            }
            else
            {
                created = TRUE;

                err = fmCustomTreeInsert(&pRouteTable->prefixTree,
                                         &pRoutePrefix->prefix,
                                         pRoutePrefix);
            }
        }

        if (err == FM_OK)
        {
            if (ppPrefix != NULL)
            {
                *ppPrefix = pRoutePrefix;
            }

            if (ppNextPrefix != NULL)
            {
                err = fmCustomTreeSuccessor(&pRouteTable->prefixTree,
                                            &prefixLength,
                                            (void **) &pTmpPrefixLength,
                                            (void **) &pNextPrefix);
                if (err != FM_OK)
                {
                    pNextPrefix = NULL;

                    /* not an error if successor not found */
                    err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
                }
                *ppNextPrefix = pNextPrefix;
            }
        }

        if (err == FM_OK)
        {
            if (ppPrevPrefix != NULL)
            {
                err = fmCustomTreePredecessor(&pRouteTable->prefixTree,
                                              &prefixLength,
                                              (void **) &pTmpPrefixLength,
                                              (void **) &pPrevPrefix);

                if (err != FM_OK)
                {
                    pPrevPrefix = NULL;

                    /* not an error if predecessor not found */
                    err = (err == FM_ERR_NO_MORE) ? FM_OK : err;
                }

                *ppPrevPrefix = pPrevPrefix;
            }
        }

        if (err == FM_OK)
        {
            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                         "created=%d, pRoutePrefix=%p, pNextPrefix=%p, pPrevPrefix=%p\n",
                         created,
                         (void *) pRoutePrefix,
                         (void *) pNextPrefix,
                         (void *) pPrevPrefix);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end GetPrefixRecord */




/*****************************************************************************/
/** FreePrefixRecord
 * \ingroup intRouter
 *
 * \desc            Releases a prefix record during switch
 *                  initialization/shutdown or release of a cloned routing
 *                  state.
 *
 * \param[in]       pKey points to the key.
 *
 * \param[in]       pValue points to the prefix record.
 *
 *****************************************************************************/
static void FreePrefixRecord(void *pKey,
                             void *pValue)
{
    fm10000_RoutePrefix *pPrefixRecord;

    FM_NOT_USED(pKey);

    pPrefixRecord = (fm10000_RoutePrefix*)pValue;

    fmCustomTreeDestroy(&pPrefixRecord->routeTree, NULL);

    fmFree(pPrefixRecord);

}   /* end FreePrefixRecord */




/*****************************************************************************/
/** NormalizeFFUSliceRanges
 * \ingroup intRoute
 *
 * \desc            Normalizes the FFU slice ranges by moving the last
 *                  slice for each route type down to an exact multiple
 *                  of the route type width.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pRouteState points to the routing state structure.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status NormalizeFFUSliceRanges(fm_int                sw,
                                         fm10000_RoutingState *pRouteState)
{
    fm_int    numCascades;
    fm_status err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, pRouteState = %p\n",
                 sw,
                 (void *) pRouteState);

    FM_NOT_USED(sw);

    err = FM_OK;

    if (pRouteState == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pRouteState->routeFirstTcamSlice = 9999;
        pRouteState->routeLastTcamSlice  = -1;

        if (pRouteState->ipv4UcastFirstTcamSlice >= 0)
        {
            /* Adjust last slice to even cascade boundary */
            numCascades = (pRouteState->ipv4UcastLastTcamSlice + 1 - 
                           pRouteState->ipv4UcastFirstTcamSlice) / RouteSliceWidths[FM10000_ROUTE_TYPE_V4U];

            if (numCascades <= 0)
            {
                pRouteState->ipv4UcastFirstTcamSlice = -1;
                pRouteState->ipv4UcastLastTcamSlice  = -1;
            }
            else
            {
                pRouteState->ipv4UcastLastTcamSlice = pRouteState->ipv4UcastFirstTcamSlice - 1 + 
                                                      (numCascades * RouteSliceWidths[FM10000_ROUTE_TYPE_V4U]);

                if (pRouteState->ipv4UcastFirstTcamSlice < pRouteState->routeFirstTcamSlice)
                {
                    pRouteState->routeFirstTcamSlice = pRouteState->ipv4UcastFirstTcamSlice;
                }

                if (pRouteState->ipv4UcastLastTcamSlice > pRouteState->routeLastTcamSlice)
                {
                    pRouteState->routeLastTcamSlice = pRouteState->ipv4UcastLastTcamSlice;
                }
            }
        }

        if (pRouteState->ipv4McastFirstTcamSlice >= 0)
        {
            /* Adjust last slice to even cascade boundary */
            numCascades = (pRouteState->ipv4McastLastTcamSlice + 1 - 
                           pRouteState->ipv4McastFirstTcamSlice) / RouteSliceWidths[FM10000_ROUTE_TYPE_V4DSV];

            if (numCascades <= 0)
            {
                pRouteState->ipv4McastFirstTcamSlice = -1;
                pRouteState->ipv4McastLastTcamSlice  = -1;
            }
            else
            {
                pRouteState->ipv4McastLastTcamSlice = pRouteState->ipv4McastFirstTcamSlice - 1 +
                                                     (numCascades * RouteSliceWidths[FM10000_ROUTE_TYPE_V4DSV]);

                if (pRouteState->ipv4McastFirstTcamSlice < pRouteState->routeFirstTcamSlice)
                {
                    pRouteState->routeFirstTcamSlice = pRouteState->ipv4McastFirstTcamSlice;
                }

                if (pRouteState->ipv4McastLastTcamSlice > pRouteState->routeLastTcamSlice)
                {
                    pRouteState->routeLastTcamSlice = pRouteState->ipv4McastLastTcamSlice;
                }
            }
        }

        if (pRouteState->ipv6UcastFirstTcamSlice >= 0)
        {
            /* Adjust last slice to even cascade boundary */
            numCascades = (pRouteState->ipv6UcastLastTcamSlice + 1 - 
                           pRouteState->ipv6UcastFirstTcamSlice) / RouteSliceWidths[FM10000_ROUTE_TYPE_V6U];

            if (numCascades <= 0)
            {
                pRouteState->ipv6UcastFirstTcamSlice = -1;
                pRouteState->ipv6UcastLastTcamSlice  = -1;
            }
            else
            {
                pRouteState->ipv6UcastLastTcamSlice = pRouteState->ipv6UcastFirstTcamSlice -1 + 
                                                      (numCascades * RouteSliceWidths[FM10000_ROUTE_TYPE_V6U]);

                if (pRouteState->ipv6UcastFirstTcamSlice < pRouteState->routeFirstTcamSlice)
                {
                    pRouteState->routeFirstTcamSlice = pRouteState->ipv6UcastFirstTcamSlice;
                }

                if (pRouteState->ipv6UcastLastTcamSlice > pRouteState->routeLastTcamSlice)
                {
                    pRouteState->routeLastTcamSlice = pRouteState->ipv6UcastLastTcamSlice;
                }
            }
        }

        if (pRouteState->ipv6McastFirstTcamSlice >= 0)
        {
            /* Adjust last slice to even cascade boundary */
            numCascades = (pRouteState->ipv6McastLastTcamSlice + 1 -
                           pRouteState->ipv6McastFirstTcamSlice) / RouteSliceWidths[FM10000_ROUTE_TYPE_V6DSV];

            if (numCascades <= 0)
            {
                pRouteState->ipv6McastFirstTcamSlice = -1;
                pRouteState->ipv6McastLastTcamSlice  = -1;
            }
            else
            {
                pRouteState->ipv6McastLastTcamSlice = pRouteState->ipv6McastFirstTcamSlice - 1 +
                                                      (numCascades * RouteSliceWidths[FM10000_ROUTE_TYPE_V6DSV]);

                if (pRouteState->ipv6McastFirstTcamSlice < pRouteState->routeFirstTcamSlice)
                {
                    pRouteState->routeFirstTcamSlice = pRouteState->ipv6McastFirstTcamSlice;
                }

                if (pRouteState->ipv6McastLastTcamSlice > pRouteState->routeLastTcamSlice)
                {
                    pRouteState->routeLastTcamSlice = pRouteState->ipv6McastLastTcamSlice;
                }
            }
        }

        if (pRouteState->routeFirstTcamSlice >= FM10000_MAX_FFU_SLICES)
        {
            pRouteState->routeFirstTcamSlice = -1;
            pRouteState->routeLastTcamSlice  = -1;
        }

        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "New Routing Slice Usage Rules:\n"
                     "    ipv4UcastFirstTcamSlice = %d, ipv4UcastLastTcamSlice = %d\n"
                     "    ipv4McastFirstTcamSlice = %d, ipv4McastLastTcamSlice = %d\n"
                     "    ipv6UcastFirstTcamSlice = %d, ipv6UcastLastTcamSlice = %d\n"
                     "    ipv6McastFirstTcamSlice = %d, ipv6McastLastTcamSlice = %d\n"
                     "    routeFirstTcamSlice = %d, routeLastTcamSlice = %d\n",
                     pRouteState->ipv4UcastFirstTcamSlice,
                     pRouteState->ipv4UcastLastTcamSlice,
                     pRouteState->ipv4McastFirstTcamSlice,
                     pRouteState->ipv4McastLastTcamSlice,
                     pRouteState->ipv6UcastFirstTcamSlice,
                     pRouteState->ipv6UcastLastTcamSlice,
                     pRouteState->ipv6McastFirstTcamSlice,
                     pRouteState->ipv6McastLastTcamSlice,
                     pRouteState->routeFirstTcamSlice,
                     pRouteState->routeLastTcamSlice);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end NormalizeFFUSliceRanges */




/*****************************************************************************/
/** InvalidateRouteSliceRow
 * \ingroup intRouter
 *
 * \desc            Invalidates a route row by clearing out all information
 *                  about the route in the TCAM.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteSlice points to the route slice.
 *
 * \param[in]       row is the row number.
 *
 * \return          TRUE if successful.
 *
 *****************************************************************************/
static fm_bool InvalidateRouteSliceRow(fm_int              sw,
                                       fm10000_RouteSlice *pRouteSlice,
                                       fm_int              row)
{
    fm_status                 err;
    fm_fm10000FfuSliceKey     ruleKeyArray[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction              action;
    fm_bool                   rowInvalidated;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteSlice=%p, row=%d\n",
                 sw,
                 (void *) pRouteSlice,
                 row);

    rowInvalidated = FALSE;

    if (pRouteSlice == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer\n");
    }
    else
    {
        /* if the stateTable is defined, actualState must be TRUE */
        if (pRouteSlice->stateTable == NULL ||
            ((pRouteSlice->stateTable != NULL) &&
             (pRouteSlice->stateTable->actualState == TRUE)) )
        {
            FM_CLEAR(ruleKeyArray);
            FM_CLEAR(action);
            action.action = FM_FFU_ACTION_NOP;

            err = fm10000SetFFURule(sw,
                                    &pRouteSlice->sliceInfo,
                                    row,
                                    FALSE,
                                    ruleKeyArray,
                                    &action,
                                    TRUE,
                                    TRUE);
            if (err == FM_OK)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                             "slice %p row %d invalidated\n",
                             (void *) pRouteSlice,
                             row);
                rowInvalidated = TRUE;

            }
        }
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       rowInvalidated,
                       "%s\n",
                       rowInvalidated ? "TRUE" : "FALSE");

}   /* end InvalidateRouteSliceRow */




/*****************************************************************************/
/** IsValidCascadeForRouteType
 * \ingroup intRouter
 *
 * \desc            Returns TRUE if the specified TCAM slice number is the
 *                  first slice in a valid cascade of TCAM slices for the
 *                  supplied route type.  A cascade is valid if and only if
 *                  the first slice number is at an even multiple of the
 *                  route type slice width from the start of the authorized
 *                  TCAM slice range for this route type and that slice and
 *                  all subsequent slices in the potential cascade, if any,
 *                  are within the range of slices authorized for that
 *                  route type.  Otherwise, it returns FALSE.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       routeType is the route type to be checked.
 *
 * \param[in]       pStateTable points to the routing state table.
 *
 * \param[in]       tcamSlice is the first TCAM slice number.
 *
 * \return          TRUE if the cascade of TCAM slices are authorized for use
 *                  by this route type.
 * \return          FALSE if the cascade of TCAM slices are not authorized
 *                  for use by this route type.
 *
 *****************************************************************************/
static fm_bool IsValidCascadeForRouteType(fm_int                sw,
                                          fm10000_RouteTypes    routeType,
                                          fm10000_RoutingState *pStateTable,
                                          fm_int                tcamSlice)
{
    fm_int  firstAuthorizedSlice;
    fm_int  lastAuthorizedSlice;
    fm_bool isAuthorized;
    fm_int  tempTcamSlice;
    fm_int  sliceWidth;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw = %d, routeType = %d, pStateTable = %p, tcamSlice = %d\n",
                 sw,
                 routeType,
                 (void *) pStateTable,
                 tcamSlice );

    isAuthorized = TRUE;

    if ( GetFirstTcamSliceForRouteType(sw,
                                       routeType,
                                       pStateTable,
                                       &firstAuthorizedSlice,
                                       &lastAuthorizedSlice) )
    {
        /* Is the starting slice number an even multiple of the route type's
         * slice width from the first authorized slice? */
        sliceWidth = RouteSliceWidths[routeType];

        if ( ( (tcamSlice - firstAuthorizedSlice) % sliceWidth ) != 0 )
        {
            isAuthorized = FALSE;
        }
        else
        {
            /* Determine if this route type is authorized to use this cascade
             * of TCAM slices */
            for (tempTcamSlice = tcamSlice; tempTcamSlice < tcamSlice  + sliceWidth; tempTcamSlice++)
            {
                if (tempTcamSlice >= FM10000_MAX_FFU_SLICES)
                {
                    /* cascade extends past the last slice, so it can't
                     * possibly be authorized. */
                    isAuthorized = FALSE;
                    break;
                }

                if ( (tempTcamSlice < firstAuthorizedSlice) || (tempTcamSlice > lastAuthorizedSlice) )
                {
                    isAuthorized = FALSE;
                    break;
                }
            }
        }
    }
    else
    {
        isAuthorized = FALSE;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       isAuthorized,
                       "isAuthorized = %d\n",
                       isAuthorized);

}   /* end IsValidCascadeForRouteType */




/*****************************************************************************/
/** AllocateRouteSlice
 * \ingroup intRouter
 *
 * \desc            Allocates a route slice cascade given the route table and
 *                  first TCAM slice in the cascade.

 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table.
 *
 * \param[in]       firstTcamSlice contains the slice number for the first
 *                  (or only) slice in the cascade.
 *
 * \param[out]      routeSlicePtrPtr points to caller-allocated storage into
 *                  which the route slice pointer will be returned. NULL can
 *                  be used instead. Since the cascade will be added to the
 *                  linked list of cascades by this function, the caller only
 *                  needs to supply a non-NULL pointer if they have some need
 *                  for the slice pointer.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory could not be allocated.
 * \return          FM_ERR_INVALID_ARGUMENT if route type is invalid
 *
 *****************************************************************************/
static fm_status AllocateRouteSlice(fm_int                sw,
                                    fm10000_RoutingTable *pRouteTable,
                                    fm_int                firstTcamSlice,
                                    fm10000_RouteSlice ** routeSlicePtrPtr)
{
    fm_status               err;
    fm10000_RoutingState   *pStateTable;
    fm10000_switch         *switchExt;
    fm10000_RouteSlice     *slicePtr;
    fm_int                  sliceWidth;
    fm10000_RouteTcamSlice *tcamSlicePtr;
    fm_int                  tcamSlice;
    fm10000_TcamSliceCase  *pCaseInfo;
    fm_int                  caseToUse;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteTable=%p, firstTcamSlice=%d, routeSlicePtrPtr=%p\n",
                 sw,
                 (void *) pRouteTable,
                 firstTcamSlice,
                 (void *) routeSlicePtrPtr );

    err = FM_OK;
    switchExt = GET_SWITCH_EXT(sw);
    pStateTable = pRouteTable->stateTable;
    sliceWidth = RouteSliceWidths[pRouteTable->routeType];

    if (pStateTable == NULL)
    {
        pStateTable = &switchExt->routeStateTable;
    }

    caseToUse = GetRouteCase(pRouteTable->routeType);
    if (caseToUse == TCAM_CASE_INVALID)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }

    if (err == FM_OK)
    {
        /* create a cascade record */
        slicePtr = fmAlloc( sizeof(fm10000_RouteSlice) );

        if (slicePtr == NULL)
        {
            err =  FM_ERR_NO_MEM;
        }
    }
    
    if (err == FM_OK)
    {
        FM_MEMSET_S( slicePtr, sizeof(fm10000_RouteSlice), 0, sizeof(fm10000_RouteSlice) );

        slicePtr->inUse = TRUE;
        FM_DLL_INIT_NODE(slicePtr, nextSlice, prevSlice);
        slicePtr->routeType      = pRouteTable->routeType;
        slicePtr->sliceWidth     = sliceWidth;
        slicePtr->firstTcamSlice = firstTcamSlice;
        slicePtr->lastTcamSlice  = firstTcamSlice + sliceWidth - 1;
        slicePtr->highestRow     = -1;
        slicePtr->lowestRow      = -1;
        slicePtr->stateTable     = pStateTable;
        slicePtr->usable         = TRUE;
        slicePtr->movable        = TRUE;

        FM_MEMSET_S( &slicePtr->routes, sizeof(slicePtr->routes), 0, sizeof(slicePtr->routes) );

        FM_MEMCPY_S( &slicePtr->sliceInfo, 
                     sizeof(slicePtr->sliceInfo),
                     pRouteTable->defaultSliceInfo,
                     sizeof(fm_ffuSliceInfo) );

        slicePtr->sliceInfo.keyStart  = slicePtr->firstTcamSlice;
        slicePtr->sliceInfo.keyEnd    = slicePtr->lastTcamSlice;
        slicePtr->sliceInfo.actionEnd = slicePtr->lastTcamSlice;

        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                     "Allocating new route slice %p using tcam slices %d to %d\n",
                     (void *) slicePtr,
                     slicePtr->firstTcamSlice,
                     slicePtr->lastTcamSlice );

        /* Insert slice into route table, sorted from highest slice to lowest */
        err = InsertSliceIntoRouteTable(pRouteTable, slicePtr);

        if (err != FM_OK)
        {
            fmFree(slicePtr);
        }
        else
        {

            /* Update each TCAM slice record */
            for (tcamSlice = slicePtr->firstTcamSlice ;tcamSlice <= slicePtr->lastTcamSlice ;tcamSlice++)
            {
                tcamSlicePtr = GetTcamSlicePtr(sw, pStateTable, tcamSlice);
                tcamSlicePtr->inUse = 1;

                pCaseInfo = &tcamSlicePtr->caseInfo[caseToUse];

                pCaseInfo->parentTcamSlice = slicePtr->firstTcamSlice;
                pCaseInfo->routeType       = pRouteTable->routeType;
                pCaseInfo->routeSlice      = slicePtr;
                pCaseInfo->tcamSlice       = tcamSlicePtr;
            }

            if (routeSlicePtrPtr != NULL)
            {
                *routeSlicePtrPtr = slicePtr;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end AllocateRouteSlice */




/*****************************************************************************/
/** PreallocateRouteSlicesV2
 * \ingroup intRouter
 *
 * \desc            Pre-allocate all of the TCAM slices available to the route 
 *                  type. This function may be called during a slice 
 *                  re-allocation operation during which some TCAM slices may
 *                  be used by route types that are no longer authorized .
 *                  to do so. Until the re-allocation operation is complete,
 *                  this is not an error.
 *
 * \note            Note that this function was written with the assumption
 *                  that only one IPv4 multicast route type would be in use
 *                  and that only one IPv6 multicast route type would be in
 *                  use.  Specifically, the function only allows IPv4/IPv6
 *                  multicast routes to use one TCAM slice case (i.e., one
 *                  for IPv4 multicast and another for IPv6 multicast).
 *                  If multiple IPv4 or IPv6 multicast route types are enabled,
 *                  the first route type processed will consume all of the
 *                  slices possible.  In that case, it is the user's
 *                  responsibility to ensure that slices will be allocated for
 *                  each enabled route type, and this may not be possible.
 *                  As an example, if all IPv4 multicast route types were
 *                  enabled, and IPv4 multicast was authorized to use slices
 *                  0 through 7, the routePreallocationOrder table below would
 *                  cause this function to allocate slices 0-2 and 3-5 to
 *                  the DIP/SIP/VLAN  route type, then slices 6 and 7 would be
 *                  allocated for use by the DIP/VLAN route type.  This would
 *                  leave no slices for use by the DIP/SIP and DIP route types.
 *                  If the multicast precedence issue is ever resolved so that
 *                  multiple multicast route types can be used, this function
 *                  will have to be modified.  In addition, it is likely that
 *                  the slice allocation structure will have to be enhanced
 *                  to allow the customer to specify which TCAM slices are
 *                  available for use by which multicast route types, since
 *                  the hardware won't allow multiple IPv4 or multiple IPv6
 *                  route types to use different cases of a given cascade (i.e.,
 *                  IPv4 and IPv6 multicast can co-exist in a cascade, but IPv4
 *                  DIP and IPv4 DIP/SIP can't share the same cascade because
 *                  the hardware will only search through a single case in
 *                  a given cascade while processing a frame).  If two cases
 *                  in a cascade both specify that they are configured for
 *                  IPv4 multicast frames, only one of the cases will be
 *                  used by the hardware.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       routeTable points to the routing table.
 *
 * \param[in]       reAlloc is TRUE if a slice reallocation operation is
 *                  in progress.  This causes unauthorized slice usage to
 *                  be ignored.  If reAlloc is FALSE, this function will report
 *                  an error condition for every TCAM slice that is being used
 *                  by an unauthorized route type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory could not be allocated.
 *
 *****************************************************************************/
static fm_status PreallocateRouteSlicesV2(fm_int                sw,
                                          fm10000_RoutingTable *routeTable,
                                          fm_bool               reAlloc)
{
    fm_status               err;
    fm10000_RouteTypes      routeType;
    fm10000_RouteSlice     *slicePtr;
    fm_int                  sliceWidth;
    fm10000_RouteTcamSlice *tcamSlicePtr;
    fm_int                  tcamSlice;
    fm10000_RouteTcamSlice *tempTcamSlicePtr;
    fm_int                  tempTcamSlice;
    fm10000_RouteTypes      tempRouteType;
    fm_int                  caseToUse;
    fm_bool                 caseUsed;
    fm_int                  slicesFound;
    fm10000_TcamSliceCase  *caseInfo;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, routeTable=%p, reAlloc=%d\n",
                 sw,
                 (void *) routeTable,
                 reAlloc );

    routeType = routeTable->routeType;

    /**********************************************************************
     * Search for valid cascades and create new slice cascade records.
     **********************************************************************/

    if (routeType == FM10000_ROUTE_TYPE_UNUSED)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    caseToUse     = GetRouteCase(routeType);
   
    if (caseToUse == TCAM_CASE_INVALID)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);;
    }

    sliceWidth = RouteSliceWidths[routeType];

    tcamSlice = 0;

    /* Scan the TCAM slices */
    while (tcamSlice < FM10000_MAX_FFU_SLICES)
    {
        tcamSlicePtr = GetTcamSlicePtr(sw, routeTable->stateTable, tcamSlice);
        caseInfo     = &tcamSlicePtr->caseInfo[caseToUse];

        /* Is the TCAM slice in use by this route type? */
        if (caseInfo->routeType == routeType)
        {
            /* case is in use by this route type */
            caseUsed = TRUE;
            slicePtr = caseInfo->routeSlice;
        }
        else if (caseInfo->routeType == FM10000_ROUTE_TYPE_UNUSED)
        {
            /* case is available for use */
            caseUsed = FALSE;
            slicePtr = NULL;
        }
        else
        {
            /* case is in use by another route type, skip this slice */
            tcamSlice++;
            continue;
        }

        /* Determine if this route type is authorized to use this
            * potential cascade of TCAM slices */
        if ( !IsValidCascadeForRouteType(sw,
                                         routeType,
                                         routeTable->stateTable,
                                         tcamSlice) )
        {
            /* This route type is not authorized to use this slice.
             * If it is already using it, and we are not in the middle
             * of repartitioning the TCAM, this is an error, because it
             * indicates that we were unable to move routes out of their
             * original slices during a repartition that must have just
             * finished (otherwise, how did unauthorized routes get into
             * the slices in the first place?).  The only remaining
             * solution is to stop traffic, empty all routes from the
             * table, repartition the empty TCAM, then add them back in
             * and restart traffic, and even that is not guaranteed to
             * work if insufficient slices have been allocated. */ 
            if (caseUsed && !reAlloc)
            {
                /* Slice is in use and we aren't in the middle of a
                 * re-partition.  This indicates that we were unable
                 * to move routes out of their original slices during
                 * the repartition that must have just finished and
                 * thus can't repartition as requested without stopping
                 * traffic (and it may not work even then). */
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING,
                            FM_ERR_TRAFFIC_STOP_REQUIRED);
            }

            /* Not authorized, but either the case isn't in use, or
             * we are in the middle of a repartition operation.
             * For now, just move to the next slice. */
            tcamSlice++;
            continue;
        }

        /* Cascade is authorized, if a cascade record already exists,
         * this slice is already in use by this route type, so just
         * skip past this cascade. */
        if (slicePtr != NULL)
        {
            tcamSlice += sliceWidth;
            continue;
        }

        /* We have found 1 available slice so far */
        slicesFound = 1;

        /* check all other TCAM slices (if any) in the potential cascade */
        for (tempTcamSlice = tcamSlice + 1 ;
                tempTcamSlice < tcamSlice + sliceWidth ;
                tempTcamSlice++)
        {
            tempTcamSlicePtr = GetTcamSlicePtr(sw,
                                               routeTable->stateTable,
                                               tempTcamSlice);

            /* Ensure that the TCAM slice isn't already in use for
             * this route type.  This could happen during a move in
             * which slice cascades are being shifted.  For example,
             * a cascade that was aligned to start on slice X might
             * now be starting on slice X-1.  In that case, the
             * cascade starting on slice X can't be deleted and a new
             * cascade created starting on slice X-1 until all routes
             * have been removed from the original cascade. */
            tempRouteType = tempTcamSlicePtr->caseInfo[caseToUse].routeType;

            if (tempRouteType == routeType)
            {
                break;
            }

            /* Is the required case available? */
            if (tempRouteType != FM10000_ROUTE_TYPE_UNUSED)
            {
                break;
            }

            /* This slice is available! */
            slicesFound++;

        }   /* end for (tempTcamSlice... */

        /* If we didn't get enough available slices on one of the cases,
         * this cascade isn't (yet) available for use. */
        if (slicesFound < sliceWidth)
        {
            /* cascade of slices is not available for use. */
            if (!reAlloc)
            {
                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                             "TCAM slice authorized for route %d but "
                             "not available for use, sw %d, TCAM "
                             "slice %d\n",
                             routeType,
                             sw,
                             tcamSlicePtr->minSliceNumber);
            }

            /* Continue with processing for this route type */
            tcamSlice++;
            continue;
        }

        /* cascade of slices is available, create a cascade record */ 
        err = AllocateRouteSlice(sw,
                                 routeTable,
                                 tcamSlice,
                                 NULL);

        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        /* Move to next slice */
        tcamSlice += sliceWidth;

    }   /* end while (1): scan TCAM slices */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end PreallocateRouteSlicesV2 */




/*****************************************************************************/
/** PreallocateRouteSlices
 * \ingroup intRouter
 *
 * \desc            Iterate through all route types and pre-allocate all
 *                  of the TCAM slices available to it.  This function may
 *                  be called during a slice re-allocation operation during
 *                  which some TCAM slices may be used by route types that
 *                  are no longer authorized to do so.  Until the re-allocation
 *                  operation is complete, this is not an error.
 *
 * \note            Note that this function was written with the assumption
 *                  that only one IPv4 multicast route type would be in use
 *                  and that only one IPv6 multicast route type would be in
 *                  use.  Specifically, the function only allows IPv4/IPv6
 *                  multicast routes to use one TCAM slice case (i.e., one
 *                  for IPv4 multicast and another for IPv6 multicast).
 *                  If multiple IPv4 or IPv6 multicast route types are enabled,
 *                  the first route type processed will consume all of the
 *                  slices possible.  In that case, it is the user's
 *                  responsibility to ensure that slices will be allocated for
 *                  each enabled route type, and this may not be possible.
 *                  As an example, if all IPv4 multicast route types were
 *                  enabled, and IPv4 multicast was authorized to use slices
 *                  0 through 7, the routePreallocationOrder table below would
 *                  cause this function to allocate slices 0-2 and 3-5 to
 *                  the DIP/SIP/VLAN  route type, then slices 6 and 7 would be
 *                  allocated for use by the DIP/VLAN route type.  This would
 *                  leave no slices for use by the DIP/SIP and DIP route types.
 *                  If the multicast precedence issue is ever resolved so that
 *                  multiple multicast route types can be used, this function
 *                  will have to be modified.  In addition, it is likely that
 *                  the slice allocation structure will have to be enhanced
 *                  to allow the customer to specify which TCAM slices are
 *                  available for use by which multicast route types, since
 *                  the hardware won't allow multiple IPv4 or multiple IPv6
 *                  route types to use different cases of a given cascade (i.e.,
 *                  IPv4 and IPv6 multicast can co-exist in a cascade, but IPv4
 *                  DIP and IPv4 DIP/SIP can't share the same cascade because
 *                  the hardware will only search through a single case in
 *                  a given cascade while processing a frame).  If two cases
 *                  in a cascade both specify that they are configured for
 *                  IPv4 multicast frames, only one of the cases will be
 *                  used by the hardware.

 * \param[in]       sw is the switch number.
 *
 * \param[in]       pStateTable points to the routing state table.
 *
 * \param[in]       reAlloc is TRUE if a slice reallocation operation is
 *                  in progress.  This causes unauthorized slice usage to
 *                  be ignored.  If reAlloc is FALSE, this function will report
 *                  an error condition for every TCAM slice that is being used
 *                  by an unauthorized route type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if memory could not be allocated.
 *
 *****************************************************************************/
static fm_status PreallocateRouteSlices(fm_int                sw,
                                        fm10000_RoutingState *pStateTable,
                                        fm_bool               reAlloc)
{
    fm_status                 err;
    fm10000_switch *          switchExt;
    fm10000_RouteTypes        routeType;
    fm10000_RouteSlice *      slicePtr;
    fm10000_RouteSlice *      nextSlicePtr;
    fm10000_RoutingTable *    pRouteTable;
    fm_int                    typeIndex;
    fm_int                    caseToUse;
    static fm10000_RouteTypes routePreallocationOrder[] =
    {
        FM10000_ROUTE_TYPE_V6DSV, 
        FM10000_ROUTE_TYPE_V6U,
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
        FM10000_ROUTE_TYPE_V6DV,
        FM10000_ROUTE_TYPE_V6SG,
        FM10000_ROUTE_TYPE_V4DV,
        FM10000_ROUTE_TYPE_V4SG,
#endif
        FM10000_ROUTE_TYPE_V4DSV,
        FM10000_ROUTE_TYPE_V4U,
        FM10000_ROUTE_TYPE_UNUSED
    };

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pStateTable=%p, reAlloc=%d\n",
                 sw,
                 (void *) pStateTable,
                 reAlloc );

    err = FM_OK;
    switchExt = GET_SWITCH_EXT(sw);

    if (pStateTable == NULL)
    {
        pStateTable = &switchExt->routeStateTable;
    }

    /**********************************************************************
     * Start by deleting all unathorized empty cascades.
     **********************************************************************/
    typeIndex = 0;

    while ( (err == FM_OK) &&  
           (routeType = routePreallocationOrder[typeIndex++]) != FM10000_ROUTE_TYPE_UNUSED)
    {
        pRouteTable = pStateTable->routeTables[routeType];
        slicePtr   = pRouteTable->firstSlice;

        while (slicePtr != NULL)
        {
            nextSlicePtr = slicePtr->nextSlice;

            if ( !IsValidCascadeForRouteType(sw,
                                             routeType,
                                             pStateTable,
                                             slicePtr->firstTcamSlice) )
            {
                slicePtr->usable = FALSE;

                if (slicePtr->highestRow < 0)
                {
                    /* Slice is empty, remove it, freeing up the case. */
                    if ( !RemoveSliceFromRoute(sw,
                                               pRouteTable,
                                               slicePtr,
                                               TRUE) )
                    {
                        err = FM_FAIL;
                        break;
                    }
                }
                else if (!reAlloc)
                {
                    /* Unauthorized TCAM slice in use, report it */
                    FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                                 "Switch %d TCAM slices %d-%d used by route "
                                 "type %d without authorization\n",
                                 sw,
                                 slicePtr->firstTcamSlice,
                                 slicePtr->lastTcamSlice,
                                 routeType);
                }
            }

            slicePtr = nextSlicePtr;

        }   /* end while (slicePtr != NULL) */

    }   /* end while (err == FM_OK && ...) */

    /**********************************************************************
     * Search for valid cascades and create new slice cascade records.
     **********************************************************************/
    typeIndex = 0;

    while ( err == FM_OK &&
            (routeType = routePreallocationOrder[typeIndex++]) != FM10000_ROUTE_TYPE_UNUSED)
    {

        caseToUse = GetRouteCase(routeType);

        if (caseToUse == TCAM_CASE_INVALID)
        {
            continue;
        }
        
        pRouteTable = pStateTable->routeTables[routeType];

        err = PreallocateRouteSlicesV2(sw, pRouteTable, reAlloc);

    }   /* end while (err == FM_OK && ...): process each route type */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end PreallocateRouteSlices */




/*****************************************************************************/
/** AllocateTemporaryCascade
 * \ingroup intRouter
 *
 * \desc            Given a state table and a route type, this function
 *                  attempts to allocate a slice cascade for temporary
 *                  use. For this purpose, the function ignores any
 *                  unicast/multicast restrictions on TCAM slice usage. Any
 *                  range of TCAM slices that is currently in use or available
 *                  for use by routing is assumed to be eligible for use.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pStateTable points to the routing state table.
 *
 * \param[in]       routeType is the route type.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_FFU_RES_FOUND if function was unable to
 *                  locate sufficient suitable TCAM slices.
 * \return          FM_ERR_NO_MEM if memory could not be allocated.
 *
 *****************************************************************************/
static fm_status AllocateTemporaryCascade(fm_int                sw,
                                          fm10000_RoutingState *pStateTable,
                                          fm10000_RouteTypes    routeType)
{
    fm_status               err;
    fm10000_switch         *switchExt;
    fm_int                  sliceWidth;
    fm10000_RouteTcamSlice *tcamSlicePtr;
    fm_int                  tcamSlice;
    fm_int                  firstCascadeSlice;
    fm_int                  lastCascadeSlice;
    fm10000_RouteTcamSlice *tempTcamSlicePtr;
    fm10000_RoutingTable   *routeTable;
    fm_bool                 skipSlice;
    fm10000_RouteSlice     *routeSlice;
    fm_int                  caseToUse;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw=%d, pStateTable=%p, routeType=%d\n",
                 sw,
                 (void *) pStateTable,
                 routeType );

    switchExt = GET_SWITCH_EXT(sw);

    if (pStateTable == NULL)
    {
        pStateTable = &switchExt->routeStateTable;
    }

    routeTable = pStateTable->routeTables[routeType];
    sliceWidth = RouteSliceWidths[routeType];

    /**********************************************************************
     * Search for valid cascades and create new slice cascade records.
     **********************************************************************/
    firstCascadeSlice = 0;

    /* Scan the TCAM slices */
    while (firstCascadeSlice < FM10000_MAX_FFU_SLICES)
    {
        /* Is this slice usable by routing? */
        if ( (firstCascadeSlice > pStateTable->routeLastTcamSlice) &&
             (firstCascadeSlice > pStateTable->previousLastTcamSlice) )
        {
            /* Slice beyond last usable slice, quit checking */
            break;
        }

        if ( ((firstCascadeSlice < pStateTable->routeFirstTcamSlice)    ||
              (firstCascadeSlice > pStateTable->routeLastTcamSlice))    &&
             ((firstCascadeSlice < pStateTable->previousFirstTcamSlice) ||
              (firstCascadeSlice > pStateTable->previousLastTcamSlice)) )
        {
            /* Slice not within either new or old slice range, skip it */
            firstCascadeSlice++;
            continue;
        }

        lastCascadeSlice = firstCascadeSlice + sliceWidth - 1;

        tcamSlicePtr = GetTcamSlicePtr(sw, pStateTable, firstCascadeSlice);

        skipSlice = TRUE;

        caseToUse = GetRouteCase(routeType);
        
        if (caseToUse == TCAM_CASE_INVALID)
        {
            /* slice is not usable */
            ++firstCascadeSlice;
            continue;
        }

        /* Is the TCAM slice in use by this route type? */
        if (tcamSlicePtr->caseInfo[caseToUse].routeType == routeType)
        {
            break;
        }
        else if (tcamSlicePtr->caseInfo[caseToUse].routeType ==
            FM10000_ROUTE_TYPE_UNUSED)
        {
            /* check all other TCAM slices in the potential cascade */
            for (tcamSlice = firstCascadeSlice + 1 ; tcamSlice <= lastCascadeSlice ; tcamSlice++)
            {
                if ( ((tcamSlice < pStateTable->routeFirstTcamSlice)    ||
                      (tcamSlice > pStateTable->routeLastTcamSlice))    &&
                     ((tcamSlice < pStateTable->previousFirstTcamSlice) ||
                      (tcamSlice > pStateTable->previousLastTcamSlice)) )
                {
                    /* Cascade not within either new or old slice range, skip it */
                    break;
                }

                tempTcamSlicePtr = GetTcamSlicePtr(sw, pStateTable, tcamSlice);

                if ( (tempTcamSlicePtr->caseInfo[caseToUse].routeType ==
                      routeType) ||
                     (tempTcamSlicePtr->caseInfo[caseToUse].routeType !=
                      FM10000_ROUTE_TYPE_UNUSED) )
                {
                    break;
                }
            }

            if (tcamSlice > lastCascadeSlice)
            {
                skipSlice = FALSE;
            }
        }

        if (skipSlice)
        {
            firstCascadeSlice++;
            continue;
        }

        /* cascade of slices is available, create a cascade record */
        err = AllocateRouteSlice(sw,
                                 routeTable,
                                 firstCascadeSlice,
                                 &routeSlice);

        if (err == FM_OK)
        {
            routeSlice->usable = FALSE;
        }

        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_NO_FFU_RES_FOUND);

}   /* end AllocateTemporaryCascade */




/*****************************************************************************/
/** UpdateTcamSliceRow
 * \ingroup intRouter
 *
 * \desc            Updates TCAM slice row in-use flags for all TCAM
 *                  slices used in a route slice.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pStateTable points to the actual or virtual state table
 *                  in use.  If NULL, the actual state table will be used.
 *
 * \param[in]       pSlice points to the route slice.
 *
 * \param[in]       row is the slice row number.
 *
 * \param[in]       pRouteTcamEntry points to the route using this row.  If
 *                  NULL, the row is to be marked available for use by a new
 *                  route. If not NULL, the row is to be marked in-use.
 *
 * \return          TRUE if successful.
 * \return          FALSE if bad arguments.
 *
 *****************************************************************************/
static fm_bool UpdateTcamSliceRow(fm_int                  sw,
                                  fm10000_RoutingState *  pStateTable,
                                  fm10000_RouteSlice *    pSlice,
                                  fm_int                  row,
                                  fm10000_TcamRouteEntry *pRouteTcamEntry)
{
    fm10000_RouteTcamSlice *pTcamSlice;
    fm_int                  index;
    fm10000_switch *        pSwitchExt;
    fm_bool                 rowUpdated;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pStateTable=%p, pSlice=%p, row=%d, pRouteTcamEntry=%p\n",
                 sw,
                 (void *) pStateTable,
                 (void *) pSlice,
                 row,
                 (void *) pRouteTcamEntry);

    pSwitchExt = GET_SWITCH_EXT(sw);
    rowUpdated = FALSE;

    if (pSlice == NULL ||
        row < 0 || row >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "sw=%d, pStateTable=%p, pSlice=%p (%d-%d), row=%d, pRouteEntry=%p\n",
                     sw,
                     (void *) pStateTable,
                     (void *) pSlice,
                     (pSlice != NULL) ? pSlice->firstTcamSlice : -1,
                     (pSlice != NULL) ? pSlice->lastTcamSlice : -1,
                     row,
                     (void *) pRouteTcamEntry);

        pStateTable = (pStateTable != NULL) ? pStateTable : &pSwitchExt->routeStateTable;

        for (index = pSlice->firstTcamSlice ; index <= pSlice->lastTcamSlice ; index++)
        {
            pTcamSlice = GetTcamSlicePtr(sw, pStateTable, index);

            if (pRouteTcamEntry != NULL)
            {
                pTcamSlice->rowStatus[row] =
                    FM10000_ROUTE_ROW_INUSE(pSlice->sliceInfo.kase);
                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING, "TCAM slice %d row %d in use\n", index, row);
            }
            else
            {
                pTcamSlice->rowStatus[row] = FM10000_ROUTE_ROW_FREE;
                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING, "TCAM slice %d row %d freed\n", index, row);
            }
        }

        pSlice->routes[row] = pRouteTcamEntry;

        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "slice %p row %d now %s\n",
                     (void *) pSlice,
                     row,
                     (pRouteTcamEntry != NULL) ? "in use" : "available");
        rowUpdated = TRUE;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       rowUpdated,
                       "%s\n",
                       rowUpdated ? "TRUE" : "FALSE");

}   /* end UpdateTcamSliceRow */




/*****************************************************************************/
/** UpdateTcamSliceRowInfoAfterRemoveRoute
 * \ingroup intRouter
 *
 * \desc            Updates TCAM slice highestRow and lowestRow information
 *                  after a route entry was removed.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pOldSlice points to the route slice used by the removed
 *                  route entry.
 *
 * \param[in]       oldRow is the slice row number used by the removed route
 *                  entry.
 *
 * \param[in]       pRemovedRouteEntry points to the removed route. Cannot be
 *                  NULL.
 * 
 *****************************************************************************/
static void UpdateTcamSliceRowInfoAfterRemoveRoute(fm_int                  sw,
                                                   fm10000_RouteSlice *    pOldSlice,
                                                   fm_int                  oldRow,
                                                   fm10000_TcamRouteEntry *pRemovedRouteEntry)
{
    fm10000_TcamRouteEntry *pTempRouteEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pOldSlice=%p, oldRow=%d, pRemovedRouteEntry=%p\n",
                 sw,
                 (void *) pOldSlice,
                 oldRow,
                 (void *) pRemovedRouteEntry);

    if (pOldSlice == NULL ||
        pRemovedRouteEntry == NULL ||
        oldRow < 0 || oldRow >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        /* If this route was the first route in the slice,
         * update the first slice information */
        if (oldRow == pOldSlice->highestRow )
        {
            pTempRouteEntry = pRemovedRouteEntry->nextTcamRoute;

            if (pTempRouteEntry != NULL &&
                pTempRouteEntry->routeSlice == pOldSlice)
            {
                pOldSlice->highestRow = pTempRouteEntry->tcamSliceRow;
            }
            else
            {
                pOldSlice->highestRow = -1;
            }
        }   /* end if (entryRow == pSlice->highestRow ) */

        /* If this route was the last route in the slice,
         * update the last slice information */
        if (oldRow == pOldSlice->lowestRow)
        {
            pTempRouteEntry = pRemovedRouteEntry->prevTcamRoute;

            if ( (pTempRouteEntry != NULL) && (pTempRouteEntry->routeSlice == pOldSlice) )
            {
                pOldSlice->lowestRow = pTempRouteEntry->tcamSliceRow;
            }
            else
            {
                pOldSlice->lowestRow = -1;
            }

        }   /* end if (entryRow == pSlice->lowestRow) */
    }

}   /* end UpdateTcamSliceRowInfoAfterRemoveRoute */




/*****************************************************************************/
/** UpdateTcamSliceRowInfoAfterInsertRoute
 * \ingroup intRouter
 *
 * \desc            Updates TCAM slice highestRow and lowestRow information
 *                  after a route entry was inserted.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pDestSlice points to the slice where the route was
 *                  inserted.
 *
 * \param[in]       destRow is the slice row number where the route was
 *                  inserted.
 *
 * \param[in]       pInsertedRouteEntry points to the inserted route. Cannot be
 *                  NULL. It is not required to update the slice and row
 *                  information, since it passed as a separate argument to this
 *                  function.
 * 
 *****************************************************************************/
static void UpdateTcamSliceRowInfoAfterInsertRoute(fm_int                  sw,
                                                   fm10000_RouteSlice *    pDestSlice,
                                                   fm_int                  destRow,
                                                   fm10000_TcamRouteEntry *pInsertedRouteEntry)
{
    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pDestSlice=%p, destRow=%d, pInsertedRouteEntry=%p\n",
                 sw,
                 (void *) pDestSlice,
                 destRow,
                 (void *) pInsertedRouteEntry);


    if (pDestSlice == NULL ||
        pInsertedRouteEntry == NULL ||
        destRow < 0 || destRow >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        /* If the route is now the first route in the destination slice,
         * update the first route pointer */
        if ( (pInsertedRouteEntry->prevTcamRoute == NULL)
             || (pInsertedRouteEntry->prevTcamRoute->routeSlice != pDestSlice) )
        {
            pDestSlice->highestRow = pInsertedRouteEntry->tcamSliceRow;
        }

        /* If this route is now the last route in the destination slice,
         * update the last route pointer */
        if ( (pInsertedRouteEntry->nextTcamRoute == NULL)
             || (pInsertedRouteEntry->nextTcamRoute->routeSlice != pDestSlice) )
        {
            pDestSlice->lowestRow = pInsertedRouteEntry->tcamSliceRow;
        }
    }

}   /* end UpdateTcamSliceRowInfoAfterInsertRoute */




/*****************************************************************************/
/** UpdatePrefixTreeAfterRemoveRoute
 * \ingroup intRouter
 *
 * \desc            Updates TCAM slice highestRow and lowestRow information
 *                  after a route entry was removed.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRemovedRouteEntry points to the removed route. Cannot be
 *                  NULL.
 * 
 *****************************************************************************/
static void UpdatePrefixTreeAfterRemoveRoute(fm_int                  sw,
                                             fm10000_TcamRouteEntry *pRemovedRouteEntry)
{
    fm_status   err;

    FM_NOT_USED(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRemovedRouteEntry=%p\n",
                 sw,
                 (void *) pRemovedRouteEntry);

    err = FM_OK;

    if (pRemovedRouteEntry == NULL)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer\n");
    }
    else
    {
        /* check if there are other routes using the prefix */
        if (fmCustomTreeSize(&pRemovedRouteEntry->routePrefix->routeTree) == 0)
        {
            /* no one else uses the prefix, remove it from the prefix tree */
            err = fmCustomTreeRemove(&pRemovedRouteEntry->routeTable->prefixTree,
                                     &pRemovedRouteEntry->routePrefix->prefix,
                                      NULL);

            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot remove prefix entry for tree, prefix=%d",
                             pRemovedRouteEntry->routePrefix->prefix);
            }
            else
            {
                /* release the prefix's memory */
                fmFree(pRemovedRouteEntry->routePrefix);
                pRemovedRouteEntry->routePrefix = NULL;
            }
        }
    }

}   /* end UpdatePrefixTreeAfterRemoveRoute */




/*****************************************************************************/
/** ConfigureSliceCascade
 * \ingroup intRouter
 *
 * \desc            Configures a cascade of slices for use by a route type.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       slicePtr points to the route slice structure identifying
 *                  the slice cascade.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ConfigureSliceCascade(fm_int              sw,
                                       fm10000_RouteSlice *slicePtr)
{
    fm_status                      status;
    fm_int                         i;
    fm_uint32                      sliceValid;
    fm_uint32                      newSliceValid;
    fm_uint32                      actionValid;
    fm_uint32                      newActionValid;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, slicePtr = %p(%d:%d-%d)\n",
                  sw,
                  (void *) slicePtr,
                  slicePtr->routeType,
                  slicePtr->firstTcamSlice,
                  slicePtr->lastTcamSlice );

    status = fm10000ConfigureFFUSlice(sw, &slicePtr->sliceInfo, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    status = fm10000GetFFUMasterValid(sw, &sliceValid, &actionValid, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    newSliceValid  = sliceValid;
    newActionValid = actionValid;

    for (i = slicePtr->sliceInfo.keyStart; i <= slicePtr->sliceInfo.keyEnd; i++)
    {
        newSliceValid |= 1 << i;
    }

    for (i = slicePtr->sliceInfo.keyEnd; i <= slicePtr->sliceInfo.actionEnd; i++)
    {
        newActionValid |= 1 << i;
    }

    if ( (newSliceValid != sliceValid) || (newActionValid != actionValid) )
    {
        status = fm10000SetFFUMasterValid(sw,
                                          newSliceValid,
                                          newActionValid,
                                          TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);
    
}   /* end ConfigureSliceCascade */



/*****************************************************************************/
/** UnconfigureSliceCascade
 * \ingroup intRouter
 *
 * \desc            Unconfigures a cascade of slices previously used by a
 *                  route type.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       slicePtr points to the route slice structure identifying
 *                  the slice cascade.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status UnconfigureSliceCascade(fm_int              sw,
                                         fm10000_RouteSlice *slicePtr)
{
    fm_status              status;
    fm_int                 i;
    fm_uint32              sliceValid;
    fm_uint32              newSliceValid;
    fm_uint32              actionValid;
    fm_uint32              newActionValid;


    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                  "sw = %d, slicePtr = %p(%d:%d-%d)\n",
                  sw,
                 (void *) slicePtr,
                  slicePtr->routeType,
                  slicePtr->firstTcamSlice,
                  slicePtr->lastTcamSlice );
    
    status = fm10000GetFFUMasterValid(sw, &sliceValid, &actionValid, TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);

    newSliceValid  = sliceValid;
    newActionValid = actionValid;

    /* Start by assuming that the slice will be turned off. */
    for (i = slicePtr->sliceInfo.keyStart; i <= slicePtr->sliceInfo.keyEnd; i++)
    {
        newSliceValid &= ~(1 << i);
    }

    for (i = slicePtr->sliceInfo.keyEnd; i <= slicePtr->sliceInfo.actionEnd; i++)
    {
        newActionValid &= ~(1 << i);
    }

    if ( (newSliceValid != sliceValid) || (newActionValid != actionValid) )
    {
        status = fm10000SetFFUMasterValid(sw,
                                          newSliceValid,
                                          newActionValid,
                                          TRUE);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, status);
    }

    /* Disable the slice in the FFU */
    status = fm10000UnconfigureFFUSlice(sw, &slicePtr->sliceInfo, TRUE);
    
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end UnconfigureSliceCascade */




/*****************************************************************************/
/** UpdateTcamRoutePosition
 * \ingroup intRouter
 *
 * \desc            keeps track of a TCAM route's position in slice/row
 *                  by updating the appropriate custom trees and linked lists.
 *
 *                  NOTE: This function will update the routeEntry, replacing
 *                  the previous slice pointer and slice row with the new
 *                  destination information.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteEntry points to the route.
 *
 * \param[in]       pDestSlice points to the slice into which the route is
 *                  being placed/moved. It may be NULL if the route is being
 *                  removed from the TCAM.
 *
 * \param[in]       destRow is the destination row for the route. It may be
 *                  -1 if the route is being removed from the TCAM.
 *
 * \param[in]       updateHardware is TRUE if the hardware registers should
 *                  also be updated, FALSE to simply remove the slice in
 *                  the software tables.
 *
 * \return          TRUE if successful.
 *
 *****************************************************************************/
static fm_bool UpdateTcamRoutePosition(fm_int                  sw,
                                       fm10000_TcamRouteEntry *pRouteEntry,
                                       fm10000_RouteSlice *    pDestSlice,
                                       fm_int                  destRow,
                                       fm_bool                 updateHardware)
{
    fm_status               err;
    fm10000_RouteSlice *    pOldSlice;
    fm_int                  oldRow;
    fm10000_switch *        pSwitchExt;
    fm10000_RoutingState *  pStateTable;
    fm_bool                 tcamPositionUpdated;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRouteEntry=%p, pDestSlice=%p, destRow=%d, updateHardware=%d\n",
                 sw,
                 (void *) pRouteEntry,
                 (void *) pDestSlice,
                 destRow,
                 updateHardware);

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);
    tcamPositionUpdated = FALSE;

    /* argument validation
     * Note that pDestSlice may be NULL and destRow may be negative if the
     * route is being removed */
    if (pRouteEntry == NULL ||
        destRow >= FM10000_FFU_ENTRIES_PER_SLICE)
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument: NULL pointer or argument out of range\n");
    }
    else
    {
        /* Trace additional info. Use conditionals to consider the cases where:
         *  1- a new route is being added
         *  2- a route is being removed */
        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "sw=%d, route=%p, old slice=%p(%d-%d), row=%d, new slice=%p(%d-%d)\n",
                     sw,
                     (void *) pRouteEntry,
                     (void *) pRouteEntry->routeSlice,
                     (pRouteEntry->routeSlice != NULL) ? pRouteEntry->routeSlice->firstTcamSlice : -1,
                     (pRouteEntry->routeSlice != NULL) ? pRouteEntry->routeSlice->lastTcamSlice : -1,
                     pRouteEntry->tcamSliceRow,
                     (void *) pDestSlice,
                     (pDestSlice != NULL) ? pDestSlice->firstTcamSlice : -1,
                     (pDestSlice != NULL) ? pDestSlice->lastTcamSlice  : -1);

        /* save the old slice and row */
        pOldSlice = pRouteEntry->routeSlice;
        oldRow = pRouteEntry->tcamSliceRow;
        pStateTable = (pRouteEntry->stateTable != NULL)? pRouteEntry->stateTable : &pSwitchExt->routeStateTable;

        /* override updateHardware if actualState is FALSE, which means that the
         * state table does NOT represent the actual state of the hardware */
        updateHardware = (pStateTable->actualState == FALSE)? FALSE : updateHardware;

        /* only if the route is not being removed */
        if ( (pDestSlice != NULL) && (destRow >= 0) )
        {
            /* mark the new row in-use.
             * UpdateTcamSliceRow cannot fail here because pDestSlice
             * and destRow were already verified */
            UpdateTcamSliceRow(sw, pStateTable, pDestSlice, destRow, pRouteEntry);

        }

        /* If the slice cascade was empty before this route was added, and we
         * are working with the actual hardware state instead of simulation,
         * enable the cascade in the hardware */
        if ( (pDestSlice != NULL)
             && (pDestSlice->highestRow == -1)
             && ( (pDestSlice->stateTable == NULL)
                  || pDestSlice->stateTable->actualState ) )
        {
            err = ConfigureSliceCascade(sw, pDestSlice);
            if (err != FM_OK)
            {
                FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                                   FALSE,
                                   "FALSE\n");
            }
        }

        /* If the route already existed and it is being moved,
         * remove it from the existing tables/lists */
         if ( (pOldSlice != NULL) && (oldRow >= 0) )
        {
            if (updateHardware)
            {
                /* erase the old row */
                if ( !InvalidateRouteSliceRow(sw, pOldSlice, oldRow) )
                {
                    /* if there was a problem invalidating the entry, trace
                     * the error and continue, the specified row will not be available */
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot invalidate row: pSlice=%p, row=%d\n",
                                 (void *) pOldSlice,
                                 oldRow);
                }
            }

            /* mark the old row available */
            UpdateTcamSliceRow(sw, pStateTable, pOldSlice, oldRow, NULL);

            /* update hihgestRow and lowestRow in SliceInfo */
            UpdateTcamSliceRowInfoAfterRemoveRoute(sw,
                                                   pOldSlice,
                                                   oldRow,
                                                   pRouteEntry);

            /* Remove the route from the "sort-by-slice" route tree */
            err = fmCustomTreeRemove(&pRouteEntry->routeTable->tcamSliceRouteTree,
                                     pRouteEntry,
                                     NULL);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot remove route from sort-by-slice tree, pRouteEntry=%p\n",
                             (void *) pRouteEntry);
            }

            /* Remove the route from the prefix tree */
            err = fmCustomTreeRemove(&pRouteEntry->routePrefix->routeTree,
                                     pRouteEntry,
                                     NULL);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot remove route from prefix tree, pRouteEntry=%p\n",
                             (void *) pRouteEntry);
            }

        }   /* end if ( (pOldSlice != NULL) && (oldRow >= 0) ) */

        /* update the route entry */
        pRouteEntry->routeSlice   = pDestSlice;
        pRouteEntry->tcamSliceRow = destRow;

        /* If the route is being removed from the TCAM, check the prefix record
         * for remaining routes.  If none exist, release the prefix record.
         * Then exit. */
        if (pDestSlice == NULL)
        {
            UpdatePrefixTreeAfterRemoveRoute (sw, pRouteEntry);
            tcamPositionUpdated = TRUE;
        }
        else
        {   

            /* Insert the route into the "sort-by-slice" route tree */
            err = fmCustomTreeInsert(&pRouteEntry->routeTable->tcamSliceRouteTree,
                                     pRouteEntry,
                                     pRouteEntry);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot insert route into the sort-b-slice tree, pRouteEntry=%p %s\n",
                             (void *) pRouteEntry,
                             fmErrorMsg(err));
                
            }
            else
            {

                /* Insert the route into the route prefix tree */
                err = fmCustomTreeInsert(&pRouteEntry->routePrefix->routeTree,
                                         pRouteEntry,
                                         pRouteEntry);
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Cannot insert route into the prefix tree, pRouteEntry=%p\n",
                                 (void *) pRouteEntry);
                }
                else
                {
                    /* update destination information */
                    UpdateTcamSliceRowInfoAfterInsertRoute(sw, 
                                                           pDestSlice, 
                                                           destRow,
                                                           pRouteEntry);
                    tcamPositionUpdated = TRUE;
                }
            }

        }   /* else if if (pDestSlice == NULL) */

    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       tcamPositionUpdated,
                       "%s\n", tcamPositionUpdated ? "TRUE" : "FALSE");

}   /* end UpdateTcamRoutePosition */




/*****************************************************************************/
/** FreeTcamRoute
 * \ingroup intRouter
 *
 * \desc            Releases a TCAM Route during switch
 *                  initialization/shutdown.
 *
 * \param[in]       pKey points to the key.
 *
 * \param[in]       pValue points to the tcam route.
 *
 *****************************************************************************/
static void FreeTcamRoute(void *pKey,
                          void *pValue)
{
    fm10000_TcamRouteEntry *pTcamRoute;
    fm10000_switch *        pSwitchExt;
    fm_int                  sw;

    FM_NOT_USED(pKey);

    pTcamRoute = pValue;
    pSwitchExt = pTcamRoute->routeTable->ext;
    sw         = pSwitchExt->base->switchNumber;

    /* remove the TCAM route */
    if ( UpdateTcamRoutePosition(sw, pTcamRoute, NULL, -1, FALSE) )
    {
        fmFree(pTcamRoute);
        pTcamRoute = NULL;
    }

}   /* end FreeTcamRoute */




/*****************************************************************************/
/** CopyRoutingTable
 * \ingroup intRouter
 *
 * \desc            Copies a new route table structure from an existing
 *                  structure.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteSliceXref contains an array with route slice
 *                  pointers from the source and clone state tables
 *                  so that this code can quickly determine which slice
 *                  pointer to use for each route as it is cloned into
 *                  the new routing table.
 *
 * \param[in]       pSource points to the source route table structure.
 *
 * \param[in]       pNewState points to the state structure for the new
 *                  routing table.
 *
 * \param[in]       pClone points to caller-allocated memory for the new
 *                  new routing table.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
static fm_status CopyRoutingTable(fm_int                sw,
                                  fm10000_RouteSlice *  pRouteSliceXref[FM10000_MAX_FFU_SLICES][FM10000_ROUTE_NUM_CASES][2],
                                  fm10000_RoutingTable *pSource,
                                  fm10000_RoutingState *pNewState,
                                  fm10000_RoutingTable *pClone)
{
    fm_status               err;
    fm_customTreeIterator   iter;
    fm10000_RoutePrefix *   routePrefix;
    fm10000_TcamRouteEntry *tcamRouteKey;
    fm10000_TcamRouteEntry *tcamOldRoute;
    fm10000_TcamRouteEntry *tcamNewRoute;
    fm_int                  index;
    fm10000_RouteSlice *    slicePtr;
    fm10000_RouteSlice *    pTempSlice;
    fm_int                  kase;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, pSource = %p, pNewState = %p, pClone = %p\n",
                 sw,
                 (void *) pSource,
                 (void *) pNewState,
                 (void *) pClone);

    pClone->routeType        = pSource->routeType;
    pClone->ext              = pSource->ext;
    pClone->stateTable       = pNewState;
    pClone->ucastOK          = pSource->ucastOK;
    pClone->mcastOK          = pSource->mcastOK;
    pClone->locked           = pSource->locked;
    pClone->defaultSliceInfo = pSource->defaultSliceInfo;

    /* Initialize tcam route tree "sorted by route" */
    fmCustomTreeInit(&pClone->tcamRouteRouteTree, fm10000CompareTcamRoutes);

    /* Initialize tcam route tree "sorted by slice number and row" */
    fmCustomTreeInit(&pClone->tcamSliceRouteTree, CompareTcamRoutesBySlice);
    fmCustomTreeRequestCallbacks(&pClone->tcamSliceRouteTree,
                                 InsertTcamRouteCallback,
                                 DeleteTcamRouteCallback);
    FM_DLL_INIT_LIST(pClone, firstTcamRoute, lastTcamRoute);

    FM_DLL_INIT_LIST(pClone, firstSlice, lastSlice);

    /* Initialize prefix tree */
    fmCustomTreeInit(&pClone->prefixTree, ComparePrefix);
    fmCustomTreeRequestCallbacks(&pClone->prefixTree,
                                 InsertPrefixCallback,
                                 DeletePrefixCallback);

    /* Insert all route slice records into the route table */
    for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
    {
        for (kase = 0 ; kase < FM10000_ROUTE_NUM_CASES ; kase++)
        {
            if (pRouteSliceXref[index][kase][0] != NULL)
            {
                slicePtr = pRouteSliceXref[index][kase][1];
                
                if (slicePtr->routeType == pClone->routeType)
                {
                    pTempSlice = pClone->firstSlice;
                    while (pTempSlice != NULL)
                    {
                        if (pTempSlice == slicePtr)
                        {
                            break;
                        }

                        pTempSlice = pTempSlice->nextSlice;
                    }
                    
                    if (pTempSlice == NULL)
                    {
                        err = InsertSliceIntoRouteTable(pClone, slicePtr);
                        
                        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
                    }
                }
            }
        }
    }

    /* Clone all routes from the TCAM source routing table */
    fmCustomTreeIterInit(&iter, &pSource->tcamRouteRouteTree);

    while (1)
    {
        err = fmCustomTreeIterNext(&iter,
                                      (void **) &tcamRouteKey,
                                      (void **) &tcamOldRoute);

        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                break;
            }

            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
        }

         tcamNewRoute = fmAlloc( sizeof(fm10000_TcamRouteEntry) );

         if (tcamNewRoute == NULL)
         {
             FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_NO_MEM);
         }

         FM_CLEAR(*tcamNewRoute);

         tcamNewRoute->stateTable   = pNewState;
         tcamNewRoute->routePtr     = tcamOldRoute->routePtr;
         tcamNewRoute->action       = tcamOldRoute->action;
         tcamNewRoute->dirty        = FALSE;
         tcamNewRoute->routeTable   = pClone;
         tcamNewRoute->ecmpGroup    = tcamOldRoute->ecmpGroup;
         tcamNewRoute->tcamSliceRow = -1;
         FM_DLL_INIT_NODE(tcamNewRoute, nextTcamRoute, prevTcamRoute);
         FM_DLL_INIT_NODE(tcamNewRoute, nextPrefixRoute, prevPrefixRoute);

         /* Get/create a prefix record */
         err = GetPrefixRecord(sw,
                               pClone,
                               tcamNewRoute->routePtr->prefix,
                               &routePrefix,
                               NULL,
                               NULL);

        if (err != FM_OK)
        {
            fmFree(tcamNewRoute);
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
        }

        tcamNewRoute->routePrefix = routePrefix;

        slicePtr = NULL;

        for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
        {
            for (kase = 0 ; kase < FM10000_ROUTE_NUM_CASES ; kase++)
            {
                if (pRouteSliceXref[index][kase][0] == tcamOldRoute->routeSlice)
                {
                    slicePtr = pRouteSliceXref[index][kase][1];
                    break;
                }
            }
        }

        if (slicePtr == NULL)
        {
            fmFree(tcamNewRoute);
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_FAIL);
        }

        err = fmCustomTreeInsert(&pClone->tcamRouteRouteTree,
                                 tcamNewRoute,
                                 tcamNewRoute);

        if (err != FM_OK)
        {
            fmFree(tcamNewRoute);
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
        }

        if ( !UpdateTcamRoutePosition(sw,
                                      tcamNewRoute,
                                      slicePtr,
                                      tcamOldRoute->tcamSliceRow,
                                      FALSE) )
        {
            fmCustomTreeRemove(&pClone->tcamRouteRouteTree, tcamNewRoute, NULL);
            fmFree(tcamNewRoute);
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_FAIL);
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end CopyRoutingTable */




/*****************************************************************************/
/**ReleaseRoutingTableCopy
 * \ingroup intRouter
 *
 * \desc            Releases a copied route table structure.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the copied route table structure.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status ReleaseRoutingTableCopy(fm_int                sw,
                                         fm10000_RoutingTable *pRouteTable)
{
    fm_status          err;
    fm10000_RouteSlice *pSlicePtr;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, pRouteTable=%p\n",
                 sw,
                 (void *) pRouteTable);

    FM_NOT_USED(sw);

    err = FM_OK;

    /* argument validation */
    if (pRouteTable == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* Delete the TCAM route tree, releasing all routes */
        fmCustomTreeDestroy(&pRouteTable->tcamRouteRouteTree, FreeTcamRoute);

        /* Delete the TCAM slice route tree */
        fmCustomTreeDestroy(&pRouteTable->tcamSliceRouteTree, NULL);

        /* Delete the prefix tree, releasing all route prefix records */
        fmCustomTreeDestroy(&pRouteTable->prefixTree, FreePrefixRecord);

        /* Release all route slice records */
        while ( (pSlicePtr = pRouteTable->firstSlice) != NULL )
        {
            RemoveSlice(pRouteTable, pSlicePtr);
            fmFree(pSlicePtr);
        }

        /* Clear the table itself */
        FM_CLEAR(*pRouteTable);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end ReleaseRoutingTableCopy */




/*****************************************************************************/
/** ReleaseRoutingState
 * \ingroup intRouter
 *
 * \desc            Release a routing state structure.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteState points to the routing state to be released.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
static void ReleaseRoutingState(fm_int                 sw,
                                fm10000_RoutingState * pRouteState)
{
    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, routeState=%p\n",
                 sw,
                 (void *)pRouteState);

    if (pRouteState != NULL)
    {
        /* Release the routing tables */
        ReleaseRoutingTableCopy(sw, &pRouteState->ipv4URoutes);
        ReleaseRoutingTableCopy(sw, &pRouteState->ipv6URoutes);

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT

        ReleaseRoutingTableCopy(sw, &pRouteState->ipv4SGRoutes);
        ReleaseRoutingTableCopy(sw, &pRouteState->ipv6SGRoutes);
        ReleaseRoutingTableCopy(sw, &pRouteState->ipv4DVRoutes);
        ReleaseRoutingTableCopy(sw, &pRouteState->ipv6DVRoutes);

#endif

        ReleaseRoutingTableCopy(sw, &pRouteState->ipv4DSVRoutes);
        ReleaseRoutingTableCopy(sw, &pRouteState->ipv6DSVRoutes);

        fmFree(pRouteState);
        pRouteState = NULL;
    }

    FM_LOG_EXIT_VOID(FM_LOG_CAT_ROUTING);

}   /* end ReleaseRoutingState */




/*****************************************************************************/
/** CloneRoutingState
 * \ingroup intRouter
 *
 * \desc            Creates a new routing state structure from an existing
 *                  structure.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pSource points to the source routing state structure.
 *
 * \param[out]      ppClone points to caller-allocated memory into which the
 *                  new routing state's pointer will be placed.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_NO_MEM if unable to allocate memory.
 *
 *****************************************************************************/
static fm_status CloneRoutingState(fm_int                 sw,
                                   fm10000_RoutingState * pSource,
                                   fm10000_RoutingState **ppClone)
{
    fm_status               err;
    fm10000_RoutingState   *newState;
    fm10000_RouteTcamSlice *srcSlice;
    fm10000_RouteTcamSlice *newSlice;
    fm_int                  index;
    fm_int                  kase;
    fm_int                  i;
    fm10000_RouteSlice     *srcRouteSlice;
    fm10000_RouteSlice     *pNewRouteSlice;
    fm10000_RouteSlice     *routeSliceXref[FM10000_MAX_FFU_SLICES][FM10000_ROUTE_NUM_CASES][2];
    

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pSource=%p, ppClone=%p\n",
                 sw,
                 (void *) pSource,
                 (void *) ppClone);

    /* argument validation */
    if (pSource == NULL ||
        ppClone == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        FM_CLEAR(routeSliceXref);

        newState = fmAlloc( sizeof(fm10000_RoutingState) );

        if (newState == NULL)
        {
            err = FM_ERR_NO_MEM;
        }
        else
        {
            FM_CLEAR(*newState);

            /* Assume this is a simulated state */
            newState->actualState = FALSE;

            newState->routeFirstTcamSlice     = pSource->routeFirstTcamSlice;
            newState->routeLastTcamSlice      = pSource->routeLastTcamSlice;
            newState->ipv4UcastFirstTcamSlice = pSource->ipv4UcastFirstTcamSlice;
            newState->ipv4UcastLastTcamSlice  = pSource->ipv4UcastLastTcamSlice;
            newState->ipv4McastFirstTcamSlice = pSource->ipv4McastFirstTcamSlice;
            newState->ipv4McastLastTcamSlice  = pSource->ipv4McastLastTcamSlice;
            newState->ipv6UcastFirstTcamSlice = pSource->ipv6UcastFirstTcamSlice;
            newState->ipv6UcastLastTcamSlice  = pSource->ipv6UcastLastTcamSlice;
            newState->ipv6McastFirstTcamSlice = pSource->ipv6McastFirstTcamSlice;
            newState->ipv6McastLastTcamSlice  = pSource->ipv6McastLastTcamSlice;

            for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
            {
                srcSlice = &pSource->routeTcamSliceArray[index];
                newSlice = &newState->routeTcamSliceArray[index];

                newSlice->minSliceNumber       = srcSlice->minSliceNumber;
                newSlice->inUse                = srcSlice->inUse;
                newSlice->ipv4UcastOK          = srcSlice->ipv4UcastOK;
                newSlice->ipv4McastOK          = srcSlice->ipv4McastOK;
                newSlice->ipv6UcastOK          = srcSlice->ipv6UcastOK;
                newSlice->ipv6McastOK          = srcSlice->ipv6McastOK;

                for (kase = 0 ; kase < FM10000_ROUTE_NUM_CASES ; kase++)
                {
                    newSlice->caseInfo[kase].caseNum         =
                        srcSlice->caseInfo[kase].caseNum;
                    newSlice->caseInfo[kase].parentTcamSlice =
                        srcSlice->caseInfo[kase].parentTcamSlice;
                    newSlice->caseInfo[kase].routeType       =
                        srcSlice->caseInfo[kase].routeType;
                    newSlice->caseInfo[kase].tcamSlice       = newSlice;

                    srcRouteSlice = srcSlice->caseInfo[kase].routeSlice;
                    
                    if ( (srcRouteSlice != NULL)
                         && (routeSliceXref[index][kase][0] == NULL) )
                    {
                        pNewRouteSlice = fmAlloc( sizeof(fm10000_RouteSlice) );
                        
                        if (pNewRouteSlice == NULL)
                        {
                            ReleaseRoutingState(sw, newState);
                            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_NO_MEM);
                        }

                        FM_CLEAR(*pNewRouteSlice);
                        
                        for (i = srcRouteSlice->firstTcamSlice ;
                             i <= srcRouteSlice->lastTcamSlice ;
                             i++)
                        {
                            routeSliceXref[i][kase][0] = srcRouteSlice;
                            routeSliceXref[i][kase][1] = pNewRouteSlice;
                        }

                        pNewRouteSlice->inUse          = srcRouteSlice->inUse;
                        pNewRouteSlice->routeType      = srcRouteSlice->routeType;
                        pNewRouteSlice->sliceWidth     = srcRouteSlice->sliceWidth;
                        pNewRouteSlice->firstTcamSlice = srcRouteSlice->firstTcamSlice;
                        pNewRouteSlice->lastTcamSlice  = srcRouteSlice->lastTcamSlice;
                        pNewRouteSlice->highestRow     = srcRouteSlice->highestRow;
                        pNewRouteSlice->lowestRow      = srcRouteSlice->lowestRow;
                        pNewRouteSlice->stateTable     = newState;
                        pNewRouteSlice->usable         = srcRouteSlice->usable;
                        pNewRouteSlice->movable        = srcRouteSlice->movable;

                        FM_MEMCPY_S( &pNewRouteSlice->sliceInfo,
                                     sizeof(pNewRouteSlice->sliceInfo),
                                     &srcRouteSlice->sliceInfo,
                                     sizeof(srcRouteSlice->sliceInfo) );
                    }
                    else if ( (routeSliceXref[index][kase][0] != NULL) )
                    {
                        pNewRouteSlice = routeSliceXref[index][kase][1];
                    }
                    else
                    {
                        pNewRouteSlice = NULL;
                    }

                    newSlice->caseInfo[kase].routeSlice = pNewRouteSlice;
                }

                FM_MEMCPY_S( newSlice->rowStatus,
                             sizeof(newSlice->rowStatus),
                             srcSlice->rowStatus,
                             sizeof(srcSlice->rowStatus) );

                newSlice->stateTable = newState;
            }

            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv4URoutes,
                                   newState,
                                   &newState->ipv4URoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V4U] = &newState->ipv4URoutes;

            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv6URoutes,
                                   newState,
                                   &newState->ipv6URoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V6U] = &newState->ipv6URoutes;

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv4SGRoutes,
                                   newState,
                                   &newState->ipv4SGRoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V4SG] = &newState->ipv4SGRoutes;
            
            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv6SGRoutes,
                                   newState,
                                   &newState->ipv6SGRoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V6SG] = &newState->ipv6SGRoutes;
            
            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv4DVRoutes,
                                   newState,
                                   &newState->ipv4DVRoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V4DV] = &newState->ipv4DVRoutes;
            
            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv6DVRoutes,
                                   newState,
                                   &newState->ipv6DVRoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V6DV] = &newState->ipv6DVRoutes;
    
#endif

            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv4DSVRoutes,
                                   newState,
                                   &newState->ipv4DSVRoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V4DSV] = &newState->ipv4DSVRoutes;
            
            err = CopyRoutingTable(sw,
                                   routeSliceXref,
                                   &pSource->ipv6DSVRoutes,
                                   newState,
                                   &newState->ipv6DSVRoutes);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
            newState->routeTables[FM10000_ROUTE_TYPE_V6DSV] = &newState->ipv6DSVRoutes;

            *ppClone = newState;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end CloneRoutingState */




/*****************************************************************************/
/** MoveUnauthorizedRoutes
 * \ingroup intRouter
 *
 * \desc            Given a route table, attempts to move any routes
 *                  that reside in unauthorized slices into authorized slices.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteTable points to the route table to be processed..
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE if the
 *                  (presumably new) FFU slice distribution doesn't leave
 *                  enough space for the existing multicast routes.
 * \return          FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE if the
 *                  (presumably new) FFU slice distribution doesn't leave
 *                  enough space for the existing unicast routes.
 *
 *****************************************************************************/
static fm_status MoveUnauthorizedRoutes(fm_int               sw,
                                        fm10000_RoutingTable *pRouteTable)
{
    fm_status               err;
    fm10000_RouteSlice *    routeSlicePtr;
    fm10000_RouteSlice *    nextRouteSlicePtr;
    fm10000_RouteSlice *    prevRouteSlicePtr;
    fm10000_RouteSlice *    curSlicePtr;
    fm10000_TcamRouteEntry *curRoute;
    fm_int                  curRow;
    fm10000_RouteTypes      routeType;

    FM_LOG_ENTRY( FM_LOG_CAT_ROUTING,
                 "sw = %d, pRouteTable = %p (%d)\n",
                 sw,
                 (void *) pRouteTable,
                 (pRouteTable != NULL) ? (fm_int) pRouteTable->routeType : -1 );

    err = FM_OK;

    if (pRouteTable == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* Are there any routes of this type? */
        if (pRouteTable->firstSlice != NULL)
        {
            routeType = pRouteTable->routeType;

            /* Start at the first (highest-numbered) slice and work down. */
            routeSlicePtr     = pRouteTable->firstSlice;
            prevRouteSlicePtr = NULL;

            while (routeSlicePtr != NULL && err == FM_OK)
            {
                nextRouteSlicePtr = routeSlicePtr->nextSlice;

                /* Is this slice authorized for use? */
                if (routeSlicePtr->usable)
                {
                    /* Yes, was the last examined slice unauthorized?  If so,
                     * work backwards through the unauthorized slice(s) trying to
                     * move all routes down into authorized slices */
                    if (prevRouteSlicePtr != NULL)
                    {
                        curSlicePtr = prevRouteSlicePtr;

                        while ( (curSlicePtr != NULL) && !curSlicePtr->usable )
                        {
                            prevRouteSlicePtr = curSlicePtr->prevSlice;

                            for (curRow = curSlicePtr->lowestRow ;
                                 curRow <= curSlicePtr->highestRow ;
                                 curRow++)
                            {
                                curRoute = curSlicePtr->routes[curRow];

                                if (curRoute != NULL)
                                {
                                    if ( !MoveRouteDownWithinPrefix(sw,
                                                                    pRouteTable,
                                                                    curRoute,
                                                                    FALSE,
                                                                    FALSE) )
                                    {
                                        /* Unable to move route, abort out */
                                        prevRouteSlicePtr = NULL;
                                        break;
                                    }
                                }
                            }

                            if (curSlicePtr->highestRow < 0)
                            {
                                /* Slice is empty, remove it. */
                                if ( !RemoveSliceFromRoute(sw,
                                                           pRouteTable,
                                                           curSlicePtr,
                                                           TRUE) )
                                {
                                    err = FM_FAIL;
                                    break;
                                }
                            }

                            curSlicePtr = prevRouteSlicePtr;
                        }
                    }
                }
                else
                {
                    /* This slice is not usable.  If the preceeding slice is usable,
                     * try to move all routes from this slice up into preceeding
                     * slices */
                    if ( (prevRouteSlicePtr != NULL) && prevRouteSlicePtr->usable )
                    {
                        for (curRow = routeSlicePtr->highestRow ;
                             (routeSlicePtr->lowestRow >= 0)
                                && (curRow >= routeSlicePtr->lowestRow) ;
                             curRow--)
                        {
                            curRoute = routeSlicePtr->routes[curRow];

                            if (curRoute != NULL)
                            {
                                if ( !MoveRouteUpWithinPrefix(sw,
                                                              pRouteTable,
                                                              curRoute,
                                                              FALSE,
                                                              FALSE) )
                                {
                                    /* Unable to move route, abort out */
                                    break;
                                }
                            }
                        }

                        if (routeSlicePtr->highestRow < 0)
                        {
                            /* Slice is empty, remove it. */
                            curSlicePtr   = routeSlicePtr;
                            routeSlicePtr = routeSlicePtr->prevSlice;

                            if ( !RemoveSliceFromRoute(sw,
                                                       pRouteTable,
                                                       curSlicePtr,
                                                       TRUE) )
                            {
                                err = FM_FAIL;
                                break;
                            }
                        }
                    }
                }

                prevRouteSlicePtr = routeSlicePtr;
                routeSlicePtr     = nextRouteSlicePtr;

            }   /* end while (routeSlicePtr != NULL) */

            /* Now check for any remaining used slices that aren't usable.  These
             * indicate an error condition. */
            routeSlicePtr = pRouteTable->firstSlice;

            while (routeSlicePtr != NULL && err == FM_OK)
            {
                nextRouteSlicePtr = routeSlicePtr->nextSlice;

                if (!routeSlicePtr->usable)
                {

                    FM_LOG_PRINT("MoveUnauthorizedRoutes FAILED, TCAM Dump Follows:\n");
                    DumpAllTcamSliceUsage(sw, pRouteTable->stateTable);

                    err = IsMcastRouteType(routeType) ? 
                          FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE :
                          FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE;
                    break;
                }

                routeSlicePtr = nextRouteSlicePtr;

            }   /* routeSlicePtr != NULL && err == FM_OK */

        }   /* if (routeTable->firstSlice != NULL) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end MoveUnauthorizedRoutes */




/*****************************************************************************/
/** MoveAllUnauthorizedRoutes
 * \ingroup intRouter
 *
 * \desc            Given a  routing state, attempts to move all routes of
 *                  each route type from unauthorized slices into authorized
 *                  slices.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pStateTable points to the actual or virtual route state
 *                  table.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE if the
 *                  (presumably new) FFU slice distribution doesn't leave
 *                  enough space for the existing multicast routes.
 * \return          FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE if the
 *                  (presumably new) FFU slice distribution doesn't leave
 *                  enough space for the existing unicast routes.
 *
 *****************************************************************************/
static fm_status MoveAllUnauthorizedRoutes(fm_int                sw,
                                           fm10000_RoutingState *pStateTable)
{
    fm10000_switch *          switchExt;
    fm10000_RouteSlice *      routeSlicePtr;
    fm_status                 err;
    fm10000_RouteTypes        routeType;
    fm10000_RoutingTable *    pRouteTable;
    fm_int                    index;
    static fm10000_RouteTypes routeConsolidationOrder[] =
    {
        /* Move widest routes first, working down to narrowest last */
        FM10000_ROUTE_TYPE_V6DSV,
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
        FM10000_ROUTE_TYPE_V6SG,
        FM10000_ROUTE_TYPE_V6U,
        FM10000_ROUTE_TYPE_V6DV,
        FM10000_ROUTE_TYPE_V4DSV,
        FM10000_ROUTE_TYPE_V4SG,
        FM10000_ROUTE_TYPE_V4DV,
#else
        FM10000_ROUTE_TYPE_V6U,
        FM10000_ROUTE_TYPE_V4DSV,
#endif
        FM10000_ROUTE_TYPE_V4U,
        FM10000_ROUTE_TYPE_UNUSED
    };

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, pStateTable = %p\n",
                 sw,
                 (void *) pStateTable);

    switchExt = GET_SWITCH_EXT(sw);
    err = FM_OK;

    if (pStateTable == NULL)
    {
        pStateTable = &switchExt->routeStateTable;
    }

    /* Initialize each route type and each slice, making sure the slice bounds
     * and usability flags are set correctly for each slice in each route table */
    for (index = 0 ; index < FM10000_NUM_ROUTE_TYPES ; index++)
    {
        pRouteTable = pStateTable->routeTables[index];

        if (pRouteTable != NULL)
        {
            pRouteTable->useUnauthorizedSlices  = TRUE;
            pRouteTable->unauthorizedSlicesUsed = FALSE;

            routeSlicePtr = pRouteTable->firstSlice;

            while (routeSlicePtr != NULL)
            {
                routeSlicePtr->movable = TRUE;
                routeSlicePtr = routeSlicePtr->nextSlice;
            }
        }
    }

    /* Process each route type in turn */
    index = 0;

    while ( (routeType = routeConsolidationOrder[index]) != FM10000_ROUTE_TYPE_UNUSED)
    {
        pRouteTable = pStateTable->routeTables[routeType];

        err = MoveUnauthorizedRoutes(sw, pRouteTable);

        if (err != FM_OK)
        {
            break;
        }

        pRouteTable->useUnauthorizedSlices = FALSE;

        index++;
    }

    for (index = 0 ; index < FM10000_NUM_ROUTE_TYPES ; index++)
    {
        pRouteTable = pStateTable->routeTables[index];

        if (pRouteTable != NULL)
        {
            pRouteTable->useUnauthorizedSlices = FALSE;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end MoveAllUnauthorizedRoutes */




/*****************************************************************************/
/** ClearCascadeRowWithinSliceRange
 * \ingroup intRouter
 *
 * \desc            Searches a specified route slice range, attempting to
 *                  create an empty row by cleaning all routes out of it.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pFirstSlice points to the first route slice in the range.
 *
 * \param[in]       firstRow is the first row number in the range.
 *
 * \param[in]       pLastSlice points to the last route slice in the range.
 *
 * \param[in]       lastRow is the last row number in the range.
 *
 * \param[out]      ppDestSlice points to caller-allocated memory into
 *                  which will be written the pointer to the route slice which
 *                  contains the available row, or NULL if an empty row
 *                  could not be created.
 *
 * \param[out]      pDestRow points to caller-allocated memory into which the
 *                  available row number will be written, or -1 if an empty
 *                  row could not be created.
 *
 * \param[in]       unauthSliceOK is TRUE if an unauthorized row may be
 *                  cleared.  This may be done during TCAM repartitioning
 *                  when there is a need to move a route temporarily to an
 *                  unauthorized location.
 *
 * \param[in]       optimize is TRUE if the search should only attempt to
 *                  return a row from a slice that will optimize slice sharing
 *                  by seeking slices that are unshared.
 *
 * \return          TRUE if successful.
 * \return          FALSE if an empty row could not be created.
 *
 *****************************************************************************/
static fm_bool ClearCascadeRowWithinSliceRange(fm_int               sw,
                                               fm10000_RouteSlice * pFirstSlice,
                                               fm_int               firstRow,
                                               fm10000_RouteSlice * pLastSlice,
                                               fm_int               lastRow,
                                               fm10000_RouteSlice **ppDestSlice,
                                               fm_int *             pDestRow,
                                               fm_bool              unauthSliceOK,
                                               fm_bool              optimize)
{
    fm_status               err;
    fm_int                  index;
    fm_bool                 foundRow;
    fm_bool                 skipCascade;
    fm_int                  numMovableRouteTypes;
    fm10000_RoutingState *  pStateTable;
    fm10000_RoutingTable *  pRouteTable;
    fm10000_RouteTypes      movableRouteTypes[FM10000_NUM_ROUTE_TYPES];
    fm_bool                 reservedSlots[FM10000_MAX_FFU_SLICES];
    fm10000_RouteSlice *    pCurSlice;
    fm10000_RouteTcamSlice *pCurTcamSlice;
    fm10000_TcamSliceCase * pCurSliceCase;
    fm_int                  curRow;
    fm_byte                 rowStatus;
    fm_int                  curTcamSlice;
    fm_int                  tempSlice;
    fm10000_RouteTcamSlice *pTempSlice;
    fm_byte                 tempStatus;
    fm10000_RouteSlice *    pTempRouteSlice;
    fm10000_RouteTypes      tempType;
    fm10000_RouteSlice *    pOtherSlice;
    fm10000_TcamRouteEntry *pOtherRoute;
    fm10000_RouteTypes      otherRouteType;
    fm10000_RoutingTable *  pOtherRouteTable;
    fm_int                  firstReserve;
    fm_int                  lastReserve;
    fm10000_RouteSlice *    sliceSearchList[FM10000_MAX_FFU_SLICES];
    fm_int                  maxSlices;
    fm_int                  sliceIndex;
    fm_bool                 searchDown;
    fm_int                  topRow;
    fm_int                  bottomRow;
    fm_bool                 moved;
    fm_int                  curCase;
    fm_int                  otherCase;
    fm_byte                 tempCase;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pFirstSlice=%p, firstRow=%d, pLastSlice=%p, lastRow=%d, "
                 "ppDestSlicePtrPtr=%p, pDestRow=%p, unauthSliceOK=%d, optimize=%d\n",
                 sw,
                 (void *) pFirstSlice,
                 firstRow,
                 (void *) pLastSlice,
                 lastRow,
                 (void *) ppDestSlice,
                 (void *) pDestRow,
                 unauthSliceOK,
                 optimize );


    foundRow = FALSE;
    pCurSlice = NULL;
    curRow = -1;

    if (pFirstSlice == NULL     ||
        pLastSlice == NULL      ||
        ppDestSlice == NULL     ||
        pDestRow == NULL)
    {
        /* invalid argument */
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,"Invalid argument\n");
    }
    else
    {

        FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                     "sw=%d, pFirstSlice=(%d-%d), firstRow=%d, pLastSlice=(%d-%d), lastRow=%d\n",
                     sw,
                     pFirstSlice->firstTcamSlice,
                     pFirstSlice->lastTcamSlice,
                     firstRow,
                     pLastSlice->firstTcamSlice,
                     pLastSlice->lastTcamSlice,
                     lastRow);

        searchDown = FALSE;

        if ( pFirstSlice->firstTcamSlice > pLastSlice->firstTcamSlice ||
            (pFirstSlice->firstTcamSlice == pLastSlice->firstTcamSlice && firstRow > lastRow) )
        {
            searchDown = TRUE;
        }

        pStateTable = pFirstSlice->stateTable;
        pRouteTable = pStateTable->routeTables[pFirstSlice->routeType];

        /* Build a list of all searchable slices. This list will be used to keep
         * slices from being searched multiple times. */
        BuildCascadeList(pFirstSlice, pLastSlice, sliceSearchList, &maxSlices);

        pRouteTable->locked = TRUE;
        numMovableRouteTypes = 0;

        /* Scan all route tables.  If the table isn't locked and it has at least
         * one route, save it as a movable route type.
         * Note that if locked, the route type belongs to a parent route that 
         * recursed down to this point. */
        for ( index = 0 ; index < FM10000_NUM_ROUTE_TYPES ; index++)
        {
            movableRouteTypes[index] = FM10000_ROUTE_TYPE_UNUSED;
            pOtherRouteTable = pStateTable->routeTables[index];

            if (pOtherRouteTable != NULL)
            {
                if (!pOtherRouteTable->locked)
                {
                    if (pOtherRouteTable->firstTcamRoute != NULL)
                    {
                        movableRouteTypes[index] = index;
                        numMovableRouteTypes++;
                    }
                }
            }
        }

        /* Scan each slice and row in the specified range */
        sliceIndex = -1;
        topRow = -1;
        bottomRow = -1;

        while (numMovableRouteTypes > 0)
        {
            if (pCurSlice == NULL)
            {
                if ( !SelectCascadeFromSliceRange(sw,
                                                  pFirstSlice,
                                                  pLastSlice,
                                                  sliceSearchList,
                                                  maxSlices,
                                                  optimize,
                                                  &pCurSlice,
                                                  &sliceIndex) )
                {
                    /* Search completed unsuccessfully */
                    break;
                }

                /* Skip this slice if it is unauthorized and we aren't
                 * allowed to move routes into unauthorized slices */
                if (!pCurSlice->usable && !unauthSliceOK)
                {
                    sliceSearchList[sliceIndex] = NULL;
                    pCurSlice                   = NULL;
                    continue;
                }

                bottomRow = 0;
                topRow = FM10000_FFU_ENTRIES_PER_SLICE - 1;

                if (searchDown == TRUE)
                {
                    if (pCurSlice == pFirstSlice)
                    {
                        topRow = firstRow;
                    }

                    if (pCurSlice == pLastSlice)
                    {
                        bottomRow = lastRow;
                    }

                    curRow = topRow;
                }
                else
                {
                    if (pCurSlice == pFirstSlice)
                    {
                        bottomRow = firstRow;
                    }

                    if (pCurSlice == pLastSlice)
                    {
                        topRow = lastRow;
                    }

                    curRow = bottomRow;
                }
            }

            /* Skip this cascade row if any TCAM slice it uses is already reserved
             * or any route type using it is not movable. */
            skipCascade = FALSE;

            for (tempSlice = pCurSlice->firstTcamSlice ; tempSlice <= pCurSlice->lastTcamSlice ; tempSlice++)
            {
                
                pTempSlice = GetTcamSlicePtr(sw, pStateTable, tempSlice);
                tempStatus = pTempSlice->rowStatus[curRow];

                if (tempStatus == FM10000_ROUTE_ROW_RESERVED)
                {
                    skipCascade = TRUE;
                }
                else if (tempStatus != FM10000_ROUTE_ROW_FREE)
                {
                    tempCase = FM10000_GET_ROUTE_ROW_CASE(tempStatus);
                    pTempRouteSlice = pTempSlice->caseInfo[tempCase].routeSlice;
                    tempType = pTempRouteSlice->routeType;

                    if (movableRouteTypes[tempType] == FM10000_ROUTE_TYPE_UNUSED)
                    {
                        /* this is an immovable route type, try to find another row. */
                        skipCascade = TRUE;
                    }
                }
            }

            if (skipCascade == FALSE)
            {
                curCase      = pCurSlice->sliceInfo.kase;
                curTcamSlice = pCurSlice->firstTcamSlice;

                pCurTcamSlice = GetTcamSlicePtr(sw, pStateTable, curTcamSlice);

                /* Check this row if it isn't already in use by this route type */
                otherCase =
                    FM10000_GET_ROUTE_ROW_CASE(pCurTcamSlice->rowStatus[curRow]);

                if (otherCase != curCase)
                {
                    /* Reserve any slice/row that is already available so that it
                     * won't be mistakenly used when moving routes in other slices */
                    FM_CLEAR(reservedSlots);

                    for (tempSlice = pCurSlice->firstTcamSlice; tempSlice <= pCurSlice->lastTcamSlice; tempSlice++)
                    {
                        pTempSlice = GetTcamSlicePtr(sw, pStateTable, tempSlice);

                        if (pTempSlice->rowStatus[curRow] == FM10000_ROUTE_ROW_FREE)
                        {
                            pTempSlice->rowStatus[curRow] = FM10000_ROUTE_ROW_RESERVED;
                            reservedSlots[tempSlice] = TRUE;
                            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                                         "TCAM slice %d row %d pre-reserved\n",
                                         tempSlice,
                                         curRow);
                        }
                    }

                    /* Try to move any routes to other locations */
                    err = FM_OK;

                    while (curTcamSlice <= pCurSlice->lastTcamSlice)
                    {
                        pCurTcamSlice = GetTcamSlicePtr(sw, pStateTable, curTcamSlice);
                        rowStatus = pCurTcamSlice->rowStatus[curRow];

                        if (rowStatus != FM10000_ROUTE_ROW_RESERVED)
                        {
                            otherCase     = FM10000_GET_ROUTE_ROW_CASE(rowStatus);
                            pCurSliceCase = &pCurTcamSlice->caseInfo[otherCase];
                            pOtherSlice = pCurSliceCase->routeSlice;
                            pOtherRoute = pOtherSlice->routes[curRow];
                            otherRouteType = pOtherSlice->routeType;
                            pOtherRouteTable = pStateTable->routeTables[otherRouteType];

                            if (pOtherRouteTable->locked)
                            {
                                break;
                            }

                            moved = FALSE;

                            if ( !(moved = MoveRouteElsewhereWithinPrefix(sw,
                                                                          pOtherRouteTable,
                                                                          pOtherRoute,
                                                                          TRUE,
                                                                          optimize)) &&
                                   pOtherRouteTable->useUnauthorizedSlices )
                            {
                                err = AllocateTemporaryCascade(sw, pStateTable, otherRouteType);

                                if (err == FM_OK)
                                {
                                    moved = MoveRouteElsewhereWithinPrefix(sw,
                                                                           pOtherRouteTable,
                                                                           pOtherRoute,
                                                                           TRUE,
                                                                           optimize);
                                }
                                else
                                {
                                    if (err == FM_ERR_NO_FFU_RES_FOUND)
                                    {
                                        err = FM_OK;
                                    }
                                    else
                                    {
                                        FM_LOG_DEBUG( FM_LOG_CAT_ROUTING,
                                                     "AllocateTemporaryCascade returned "
                                                     "error %d (%s) for sw %d, "
                                                     "route type %d\n",
                                                     err,
                                                     fmErrorMsg(err),
                                                     sw,
                                                     otherRouteType );
                                        break;
                                    }
                                }
                            }

                            if (!moved)
                            {
                                /* This route can't be moved, quit trying to clear this row */
                                movableRouteTypes[otherRouteType] = FM10000_ROUTE_TYPE_UNUSED;
                                numMovableRouteTypes--;
                                break;
                            }

                            /* The route was moved, reserve the slice/rows that were just
                             * freed and are needed for the target route type */
                            if (pCurSlice->firstTcamSlice <= pOtherSlice->firstTcamSlice)
                            {
                                firstReserve = pOtherSlice->firstTcamSlice;
                            }
                            else
                            {
                                firstReserve = pCurSlice->firstTcamSlice;
                            }

                            if (pCurSlice->lastTcamSlice <= pOtherSlice->lastTcamSlice)
                            {
                                lastReserve = pCurSlice->lastTcamSlice;
                            }
                            else
                            {
                                lastReserve = pOtherSlice->lastTcamSlice;
                            }

                            for (tempSlice = firstReserve; tempSlice <= lastReserve; tempSlice++)
                            {
                                pTempSlice = GetTcamSlicePtr(sw, pStateTable, tempSlice);

                                FM_LOG_ASSERT(FM_LOG_CAT_ROUTING,
                                              pTempSlice->rowStatus[curRow] == FM10000_ROUTE_ROW_FREE,
                                              "TCAM Slice %d row %d should be "
                                              "free after route move, but "
                                              "isn't (%d)!\n",
                                              tempSlice,
                                              curRow,
                                              pTempSlice->rowStatus[curRow]);

                                pTempSlice->rowStatus[curRow] = FM10000_ROUTE_ROW_RESERVED;
                                reservedSlots[tempSlice] = TRUE;
                                FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                                             "TCAM slice %d row %d now reserved\n",
                                             tempSlice,
                                             curRow);
                            }
                        }
                        curTcamSlice++;

                    }   /* end while (curTcamSlice <= pCurSlice->lastTcamSlice) */

                    /* release reservations */
                    for (tempSlice = pCurSlice->firstTcamSlice; tempSlice <= pCurSlice->lastTcamSlice; tempSlice++)
                    {
                        if (reservedSlots[tempSlice])
                        {
                            pTempSlice = GetTcamSlicePtr(sw, pStateTable, tempSlice);

                            pTempSlice->rowStatus[curRow] = FM10000_ROUTE_ROW_FREE;
                            FM_LOG_DEBUG(FM_LOG_CAT_ROUTING,
                                         "TCAM slice %d row %d unreserved\n",
                                         tempSlice,
                                         curRow);
                        }
                    }

                    if (err != FM_OK)
                    {
                        break;
                    }

                    if (curTcamSlice > pCurSlice->lastTcamSlice)
                    {
                        /* Found an empty row */
                        foundRow = TRUE;
                        break;
                    }
                }   /* end if (otherCase != curCase) */
            }

            /* process the next row */
            if (searchDown)
            {
                if (--curRow < bottomRow)
                {
                    pCurSlice = NULL;
                    sliceSearchList[sliceIndex] = NULL;
                }
            }
            else
            {
                if (++curRow >= topRow)
                {
                    pCurSlice = NULL;
                    sliceSearchList[sliceIndex] = NULL;
                }
            }
        }   /* end while (1) */

        pRouteTable->locked = FALSE;

        if (!foundRow)
        {
            pCurSlice = NULL;
            curRow   = -1;
        }

        *ppDestSlice = pCurSlice;
        *pDestRow      = curRow;
    }

    FM_LOG_EXIT_CUSTOM(FM_LOG_CAT_ROUTING,
                       foundRow,
                       "foundRow=%d, pCurSlice=%p, curRow=%d\n",
                       foundRow,
                       (void *) pCurSlice,
                       curRow);

}   /* end ClearCascadeRowWithinSliceRange */




/*****************************************************************************/
/** DumpTcamSliceUsage
 * \ingroup intRouter
 *
 * \desc            Dumps TCAM usage information for a TCAM slice.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRouteState points to the routing state structure.
 *
 * \param[in]       tcamSlice is the TCAM slice number.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void DumpTcamSliceUsage(fm_int                sw,
                               fm10000_RoutingState *pRouteState,
                               fm_int                tcamSlice)
{
    fm10000_RouteTcamSlice *pTcamSlice;
    fm_byte *               rowStatus;
    fm_int                  linesNeeded;
    fm_int                  lastLineRowCount;
    fm_int                  line;
    fm_int                  sliceRow;
    fm_int                  sliceRowsPerLine;
    fm_char                 textBuff[200];
    fm_int                  index;
    fm_text                 rowUsageString;
    fm_byte                 curRowStatus;

    FM_NOT_USED(sw);

    pTcamSlice = &pRouteState->routeTcamSliceArray[tcamSlice];

    FM_LOG_PRINT("  %02d: tcam slice %2d%s%s%s%s%s\n",
                 tcamSlice,
                 pTcamSlice->minSliceNumber,
                 (pTcamSlice->ipv4UcastOK) ? ", V4 Ucast OK" : "",
                 (pTcamSlice->ipv4McastOK) ? ", V4 Mcast OK" : "",
                 (pTcamSlice->ipv6UcastOK) ? ", V6 Ucast OK" : "",
                 (pTcamSlice->ipv6McastOK) ? ", V6 Mcast OK" : "",
                 (pTcamSlice->inUse) ? ", In Use" : "");

    if (pTcamSlice->inUse)
    {
        FM_LOG_PRINT("      Rows In Use: ");

        rowStatus = pTcamSlice->rowStatus;

#define SLICE_ROWS_PER_LINE  32

        linesNeeded = FM10000_FFU_ENTRIES_PER_SLICE / SLICE_ROWS_PER_LINE;

#if ( ( FM10000_FFU_ENTRIES_PER_SLICE % SLICE_ROWS_PER_LINE) != 0)

        lastLineRowCount = FM10000_FFU_ENTRIES_PER_SLICE % SLICE_ROWS_PER_LINE;
        linesNeeded++;

#else
        lastLineRowCount = SLICE_ROWS_PER_LINE;
#endif


        sliceRow = FM10000_FFU_ENTRIES_PER_SLICE - 1;

        for (line = 0 ; line < linesNeeded ; line++)
        {
            if ( (line + 1) < linesNeeded )
            {
                sliceRowsPerLine = SLICE_ROWS_PER_LINE;
            }
            else
            {
                sliceRowsPerLine = lastLineRowCount;
            }

            FM_SNPRINTF_S( textBuff,
                           sizeof(textBuff),
                           "%3d - %3d: ",
                           sliceRow,
                           sliceRow - sliceRowsPerLine + 1);

            for (index = 0 ; index < sliceRowsPerLine ; index++)
            {
                if ( (index != 0) && ( (index % 4) == 0 ) )
                {
                    fmStringAppend( textBuff, "|", sizeof(textBuff) );
                }

                curRowStatus = rowStatus[sliceRow];

                if (curRowStatus == FM10000_ROUTE_ROW_FREE)
                {
                    /* row is not used */
                    rowUsageString = "_";
                }
                else if (curRowStatus == FM10000_ROUTE_ROW_RESERVED)
                {
                    /* the row is reserved */
                    rowUsageString = "R";
                }
                else
                {
                    /* indicate that the row is in use */
                    rowUsageString = "U";
                }

                fmStringAppend(textBuff, rowUsageString, sizeof(textBuff) );

                sliceRow--;
            }

            fmStringAppend( textBuff, "\n", sizeof(textBuff) );

            if (line != 0)
            {
                FM_LOG_PRINT("                   ");
            }

            FM_LOG_PRINT("%s", textBuff);
        }

    }

}   /* end DumpTcamSliceUsage */




/*****************************************************************************/
/** DumpAllTcamSliceUsage
 * \ingroup intRouter
 *
 * \desc            Dumps TCAM usage information for all TCAM slices.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pStateTable points to the routing state structure.
 *
 * \return          nothing.
 *
 *****************************************************************************/
static void DumpAllTcamSliceUsage(fm_int                sw,
                                  fm10000_RoutingState *pStateTable)
{
    fm_int index;

    FM_LOG_PRINT("\nRouting TCAM Slice Array\n"
                 "  _ = Row is not in use.\n"
                 "  R = Row is reserved.\n"
                 "  A = Row is used by Case A,\n  a = Row used by Case A "
                 "but route pointer is NULL,\n"
                 "  B = Row is used by Case B,\n  b = Row used by Case B "
                 "but route pointer is NULL,\n"
                 "  X = Row not used by Case A, but route pointer is "
                 "NOT NULL,\n"
                 "  Y = Row not used by Case B, but route pointer is "
                 "NOT NULL\n");

    for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
    {
        DumpTcamSliceUsage(sw, pStateTable, index);
    }

}   /* end DumpAllTcamSliceUsage */




/*****************************************************************************/
/** CreateRouterArpRedirectTrigger
 * \ingroup intRouterBase
 *
 * \desc            Create ARP redirect trigger
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status CreateRouterArpRedirectTrigger(fm_int sw)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm10000_switch     *switchExt;
    fm_switch          *switchPtr;
    fm_status           err;
    fm_char             trigName[32];

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw %d\n", sw);

    /* Initialize return code */
    err = FM_OK;

    /* Get switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Create the trigger's name */
    FM_SPRINTF_S(trigName,
                 sizeof(trigName),
                 "arpRedirectT%d",
                 triggerRuleArpRedirect);
    
    /* Create the trigger */
    err = fm10000CreateTrigger(sw,
                               FM10000_TRIGGER_GROUP_ROUTING,
                               triggerRuleArpRedirect,
                               TRUE,
                               trigName);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Initialize trigger's actions */
    err = fmInitTriggerAction(sw, &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Set trigger's action */
    trigAction.cfg.trapAction = FM_TRIGGER_TRAP_ACTION_DONOT_TRAP_OR_LOG;

    err = fm10000SetTriggerAction(sw,
                                  FM10000_TRIGGER_GROUP_ROUTING,
                                  triggerRuleArpRedirect,
                                  &trigAction,
                                  TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Initialize trigger's conditions */
    err = fmInitTriggerCondition(sw, &trigCond);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Set trigger's conditions */
    trigCond.cfg.rxPortset = FM_PORT_SET_ALL;
    trigCond.cfg.HAMask = FM_TRIGGER_HA_LOG_ARP_REDIRECT;

    err = fm10000SetTriggerCondition(sw,
                                     FM10000_TRIGGER_GROUP_ROUTING,
                                     triggerRuleArpRedirect,
                                     &trigCond,
                                     TRUE);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end CreateRouterArpRedirectTrigger */




/*****************************************************************************/
/** DeleteRouterArpRedirectTrigger
 * \ingroup intRouterBase
 *
 * \desc            Delete ARP redirect event trigger
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status DeleteRouterArpRedirectTrigger(fm_int sw)
{
    fm_status           err;
    fm10000_switch     *switchExt;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw %d\n", sw);

    /* Initialize return code */
    err = FM_OK;

    /* Get switch ext pointer */
    switchExt = GET_SWITCH_EXT(sw);

    /* Delete arp redirect trigger */
    err = fm10000DeleteTrigger(sw,
                               FM10000_TRIGGER_GROUP_ROUTING,
                               triggerRuleArpRedirect,
                               TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}/* end DeleteRouterArpRedirectTrigger */




/*****************************************************************************/
/** SetRouterArpRedirectTrigger
 * \ingroup intRouterBase
 *
 * \desc            Enable/Disable ARP redirect reporting trigger.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetRouterArpRedirectTrigger(fm_int sw)
{
    fm_triggerCondition trigCond;
    fm_triggerAction    trigAction;
    fm10000_switch     *switchExt;
    fm_switch          *switchPtr;
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw %d\n", sw);

    /* Initialize return code */
    err = FM_OK;

    /* Get the switch pointers */
    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    /* Get the trigger's conditions */
    err = fm10000GetTrigger(sw, 
                            FM10000_TRIGGER_GROUP_ROUTING, 
                            triggerRuleArpRedirect, 
                            &trigCond,
                            &trigAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Check if trap redirect event and state of arp redirect logging  */
    if ( switchPtr->routerTrapRedirectEvent )
    {
        /* Disable the trigger when routerTrapRedirectEvent is set and 
           arp redirect logging is disabled (which means that the 
           trigger is enabled) */
        
        /* Disable the trigger by setting trigger's list of source
           ports to match on none ports (the trigger will never hit) */
        trigCond.cfg.rxPortset = FM_PORT_SET_NONE;

        err = fm10000SetTriggerCondition(sw,
                                         FM10000_TRIGGER_GROUP_ROUTING,
                                         triggerRuleArpRedirect,
                                         &trigCond,
                                         TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    }
    else 
    {
        /* Enable the trigger when routerTrapRedirectEvent is not set and 
           arp redirect logging is enabled (which means that the 
           trigger is disabled)*/

        /* Enable the trigger by setting trigger's list of source
           ports to match on all ports */
        trigCond.cfg.rxPortset = FM_PORT_SET_ALL;

        err = fm10000SetTriggerCondition(sw,
                                         FM10000_TRIGGER_GROUP_ROUTING,
                                         triggerRuleArpRedirect,
                                         &trigCond,
                                         TRUE);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    }

ABORT:
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end SetRouterArpRedirectTrigger */




/*****************************************************************************/
/** SetInitialSliceBoundaries
 * \ingroup intRouter
 *
 * \desc            Perform the initial routing slice boundary configuration.
 *
 * \param[in]       sw is the switch number.
 * 
 * \param[in]       pStateTable points to the routing state table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status SetInitialSliceBoundaries (fm_int                sw,
                                     fm10000_RoutingState *pStateTable)
{
    fm_status   err;
    fm_int      firstUnicastSlice;
    fm_int      lastUnicastSlice;
    fm_int      firstMulticastSlice;
    fm_int      lastMulticastSlice;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pStateTable=%p\n",
                 sw,
                 (void *) pStateTable);
    err = FM_OK;

    if (pStateTable == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        /* Get unicast and multicast slice boundaries */

        firstUnicastSlice =
            fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_UNICAST_SLICE_1ST,
                                FM_AAD_API_FM10000_FFU_UNICAST_SLICE_1ST);
        lastUnicastSlice =
            fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_UNICAST_SLICE_LAST,
                                FM_AAD_API_FM10000_FFU_UNICAST_SLICE_LAST);

        firstMulticastSlice =
            fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_1ST,
                                FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_1ST);
        lastMulticastSlice =
            fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_MULTICAST_SLICE_LAST,
                                FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_LAST);

        if (firstUnicastSlice > lastUnicastSlice    ||
            firstMulticastSlice > lastMulticastSlice)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                         "sw=%d: Invalid API route slice ranges\n"
                         "    firstUnicastSlice=%d, lastUnicastSlice=%d\n"
                         "    firstMulticastSlice=%d, lastMulticastSlice=%d\n",
                         sw,
                         firstUnicastSlice,
                         lastUnicastSlice,
                         firstMulticastSlice,
                         lastMulticastSlice);

            firstUnicastSlice   = FM_AAD_API_FM10000_FFU_UNICAST_SLICE_1ST;
            lastUnicastSlice    = FM_AAD_API_FM10000_FFU_UNICAST_SLICE_LAST;
            firstMulticastSlice = FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_1ST;
            lastMulticastSlice  = FM_AAD_API_FM10000_FFU_MULTICAST_SLICE_LAST;
        }

        /* Save the slice boundaries */
        pStateTable->ipv4UcastFirstTcamSlice = firstUnicastSlice;
        pStateTable->ipv6UcastFirstTcamSlice = firstUnicastSlice;
        pStateTable->ipv4UcastLastTcamSlice  = lastUnicastSlice;
        pStateTable->ipv6UcastLastTcamSlice  = lastUnicastSlice;
        pStateTable->ipv4McastFirstTcamSlice = firstMulticastSlice;
        pStateTable->ipv6McastFirstTcamSlice = firstMulticastSlice;
        pStateTable->ipv4McastLastTcamSlice  = lastMulticastSlice;
        pStateTable->ipv6McastLastTcamSlice  = lastMulticastSlice;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetInitialSliceBoundaries */




/*****************************************************************************/
/** SetVirtualRouterMacRegisters
 * \ingroup intRouter
 *
 * \desc            Configure DMAC mapper and router SMAC for a MAC address
 *                  of a virtual router.
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       vroff is the offset of the virtual router in the table.
 *
 * \param[in]       vrMacId identifies which MAC address of the virtual router
 *                  to configure.
 *
 * \param[in]       state is the state of the virtual router.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
static fm_status SetVirtualRouterMacRegisters(fm_int         sw,
                                              fm_int         vroff,
                                              fm_int         vrMacId,
                                              fm_routerState state)
{
    fm_switch *         switchPtr;
    fm_status           err;
    fm_macaddr          tmpMac;
    fm_bool             routerState;
    fm_fm10000MapMacCfg mapperMacCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw %d, vroff = %d, vrMacId = %d, state = %d\n",
                 sw, vroff, vrMacId, state);

    switchPtr = GET_SWITCH_PTR(sw);

    tmpMac = switchPtr->virtualRouterMac[vrMacId]
             | switchPtr->virtualRouterIds[vroff];

    err = switchPtr->WriteUINT64(sw,
                                 FM10000_MOD_ROUTER_SMAC(vroff + 1, 0),
                                 tmpMac);

    if (err == FM_OK)
    {
        routerState = (state == FM_ROUTER_STATE_ADMIN_UP) ? TRUE : FALSE;

        mapperMacCfg.macAddr = tmpMac;
        mapperMacCfg.ignoreLength = 0;
        mapperMacCfg.validSMAC = FALSE;
        mapperMacCfg.validDMAC = TRUE;
        mapperMacCfg.mapMac = vroff + 1;
        mapperMacCfg.router = routerState;
                        
        err = fm10000SetMapMac(sw, 
                               vroff + 1, 
                               &mapperMacCfg, 
                               FM_FM10000_MAP_MAC_ALL, 
                               TRUE);
                        
        if (err != FM_OK)
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                         "vroff %d SetMapMac failed: %s\n",
                         vroff,
                         fmErrorMsg(err));
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end SetVirtualRouterMacRegisters */




/*****************************************************************************
 * Public Functions
 *****************************************************************************/




/*****************************************************************************/
/** fm10000RouterAlloc
 * \ingroup intRouter
 *
 * \desc            Allocates memory for router subsystem, called at
 *                  switch insertion time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000RouterAlloc(fm_int sw)
{
    FM_NOT_USED(sw);
    
    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d\n",
                 sw);

     /* nothing to do here, this function is kept as a placeholder */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fm10000RouterAlloc */




/*****************************************************************************/
/** fm10000RouterFree
 * \ingroup intRouter
 *
 * \desc            Releases memory and resources used by router subsystem,
 *                  called at switch removal time.
 *
 * \param[in]       sw is the switch number.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000RouterFree(fm_int sw)
{
    fm_status             err;
    fm10000_switch *      pSwitchExt;
    fm10000_RoutingTable *pRouteTable;
    fm_int                index;
    fm10000_RouteSlice *  pRouteSlice;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d\n",
                 sw);

    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);

    /* remove and release all active routes and route slices */
    for (index = 0 ; index < FM10000_NUM_ROUTE_TYPES ; index++)
    {
        pRouteTable = pSwitchExt->routeStateTable.routeTables[index];

        if (pRouteTable != NULL)
        {
            /* remove and release all active routes */
            if ( fmCustomTreeIsInitialized(&pRouteTable->tcamRouteRouteTree) )
            {
                fmCustomTreeDestroy(&pRouteTable->tcamRouteRouteTree, FreeTcamRoute);
            }

            /* remove all slices from the route */
            while ( ( pRouteSlice = GetFirstSlice(pRouteTable) ) != NULL )
            {
                RemoveSliceFromRoute(sw, pRouteTable, pRouteSlice, FALSE);
            }
        }
    }

    /* Delete ARP redirect event trigger */
    err = DeleteRouterArpRedirectTrigger(sw);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RouterFree */




/*****************************************************************************/
/** fm10000RouterInit
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
fm_status fm10000RouterInit(fm_int sw)
{
    fm_status               err;
    fm10000_switch *        pSwitchExt;
    fm10000_RoutingState *  pStateTable;
    fm10000_RouteTcamSlice *pTcamSlice;
    fm_int                  index;
    fm_int                  errCount;
    fm_switch              *swptr;
    fm_int                  kase;
    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d\n",
                 sw);

    err = FM_OK;
    errCount = 0;
    swptr = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    
    /* Get the unicast/multicast min precedence values */
    pSwitchExt->unicastMinPrecedence =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN,
                            FM_AAD_API_FM10000_FFU_UNICAST_PRECEDENCE_MIN);
    pSwitchExt->multicastMinPrecedence =
        fmGetIntApiProperty(FM_AAK_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN,
                            FM_AAD_API_FM10000_FFU_MULTICAST_PRECEDENCE_MIN);

    pStateTable = &pSwitchExt->routeStateTable;
    pStateTable->actualState = TRUE;
    pStateTable->tempSlicesAvailable = FALSE;

    err = fm10000GetFFUSliceOwnership(sw,
                                      FM_FFU_OWNER_ROUTING,
                                      &pStateTable->routeFirstTcamSlice,
                                      &pStateTable->routeLastTcamSlice);
    if (err == FM_ERR_NO_FFU_RES_FOUND)
    {
        pStateTable->routeFirstTcamSlice = -1;
        pStateTable->routeLastTcamSlice  = -1;
        err = FM_OK;
    }
    
    if (err == FM_OK)
    {

        /* Get unicast and multicast slice boundaries */
        err = SetInitialSliceBoundaries (sw, pStateTable);

        if (err == FM_OK)
        {
            err = NormalizeFFUSliceRanges(sw, &pSwitchExt->routeStateTable);
        }
        
        if (err == FM_OK)
        {
            pStateTable->previousFirstTcamSlice = pStateTable->routeFirstTcamSlice;
            pStateTable->previousLastTcamSlice  = pStateTable->routeLastTcamSlice;

            /* Initialize slice array */
            pSwitchExt->maxRoutes = FM10000_MAX_FFU_SLICES * FM10000_FFU_ENTRIES_PER_SLICE;
            pSwitchExt->maxRouteSlices = pSwitchExt->maxRoutes; 

            FM_CLEAR(pStateTable->routeTcamSliceArray);

            for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
            {
                pTcamSlice = &pStateTable->routeTcamSliceArray[index];

                pTcamSlice->minSliceNumber = index;
                pTcamSlice->inUse          = FALSE;
                pTcamSlice->ipv4UcastOK    = FALSE;
                pTcamSlice->ipv4McastOK    = FALSE;
                pTcamSlice->ipv6UcastOK    = FALSE;
                pTcamSlice->ipv6McastOK    = FALSE;
                pTcamSlice->stateTable     = &pSwitchExt->routeStateTable;

                for (kase = 0 ; kase < FM10000_ROUTE_NUM_CASES ; kase++)
                {
                    pTcamSlice->caseInfo[kase].caseNum = kase;
                    pTcamSlice->caseInfo[kase].parentTcamSlice = -1;
                    pTcamSlice->caseInfo[kase].routeType =
                        FM10000_ROUTE_TYPE_UNUSED;
                }
            }
            err = SetFFUSliceUsageForRoutingState(sw, NULL, &pSwitchExt->routeStateTable);
        }

        if (err == FM_OK)
        {
            FM_CLEAR(pStateTable->routeTables);

            /* Initialize IPv4 Unicast / *G Multicast Route Table */
            err = InitRouteTable(sw,
                                 FM10000_ROUTE_TYPE_V4U,
                                 sizeof(pStateTable->ipv4URoutes),
                                 &pStateTable->ipv4URoutes,
                                 &ipv4SliceInfo);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot initialize V4U routes\n");
                errCount++;
            }

            /* Initialize IPv6 Unicast / *G Multicast Route Table */
            err = InitRouteTable(sw,
                                 FM10000_ROUTE_TYPE_V6U,
                                 sizeof(pStateTable->ipv6URoutes),
                                 &pStateTable->ipv6URoutes,
                                 &ipv6SliceInfo);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot initialize V6U routes\n");
                errCount++;
            }

            /* Initialize IPv4 Dest/Src/Vlan IGMP Multicast Route Table */
            err = InitRouteTable(sw,
                                 FM10000_ROUTE_TYPE_V4DSV,
                                 sizeof(pStateTable->ipv4DSVRoutes),
                                 &pStateTable->ipv4DSVRoutes,
                                 &ipv4DSVSliceInfo);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot initialize V6U routes\n");
                errCount++;
            }

            /* Initialize IPv6 Dest/Src/Vlan IGMP Multicast Route Table */
            err = InitRouteTable(sw,
                                 FM10000_ROUTE_TYPE_V6DSV,
                                 sizeof(pStateTable->ipv6DSVRoutes),
                                 &pStateTable->ipv6DSVRoutes,
                                 &ipv6DSVSliceInfo);
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot initialize V6U routes\n");
                errCount++;
            }

            /* Create an ECMP group for drop routes */
            err = fmCreateECMPGroupInternal( sw,
                                             &swptr->dropEcmpGroup,
                                             NULL,
                                             (fm_intMulticastGroup *) ~0 );
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot create an ECMP group for drop routes\n");
                errCount++;
            }

            /* Pre-allocate TCAM slices for each route type */
            err = PreallocateRouteSlices(sw, NULL, FALSE);

            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "Cannot preallocateRouteSlices\n");
                errCount++;
            }

            /* Create ARP redirect event trigger. */
            err = CreateRouterArpRedirectTrigger(sw);
            
            if (err != FM_OK)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "CreateRouterArpRedirectTrigger failed %s\n",
                             fmErrorMsg(err));
                errCount++;
            }
            
            /********************************************************** 
             * the following route types are not currently supported
             *  - FM10000_ROUTE_TYPE_V4SG
             *  - FM10000_ROUTE_TYPE_V6SG
             *  - FM10000_ROUTE_TYPE_V4DV
             *  - FM10000_ROUTE_TYPE_V4DV
             **********************************************************/

            if (errCount > 0)
            {
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                             "One or more route tables could not be initialized\n");
                err = FM_FAIL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RouterInit */



/*****************************************************************************/
/** fm10000SetRouterAttribute
 * \ingroup intRouterBase
 *
 * \desc            Set value of a router global attribute.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       attr is the attribute to work on.
 *
 * \param[in]       pValue points to the attribute value.  It has already been
 *                  parsed and stored into the switch configuration, so this
 *                  field may be ignored if the configuration value is used
 *                  directly.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ATTRIB if attr is not a recognized attribute.
 *
 *****************************************************************************/
fm_status fm10000SetRouterAttribute(fm_int  sw, 
                                    fm_int  attr, 
                                    void *  pValue)
{
    fm_switch          *switchPtr;
    fm_status           err = FM_OK;
    fm_uint32           reg;
    fm_int              vroff;
    fm_macaddr          tmpMac;
    fm_fm10000MapMacCfg mapperMacCfg;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, attr=%d, pValue=%p\n",
                 sw,
                 attr,
                 pValue);

    FM_NOT_USED(pValue);

    switchPtr = GET_SWITCH_PTR(sw);

    switch (attr)
    {
        case FM_ROUTER_TRAP_TTL1:

            err = switchPtr->ReadUINT32(sw, FM10000_SYS_CFG_ROUTER(), &reg);
            
            if (err == FM_OK)
            {
                /* set TTLdisposal field */
                FM_SET_FIELD(reg, FM10000_SYS_CFG_ROUTER, TTLdisposal, (switchPtr->routerTrapTTL1 & 3));
                err  = switchPtr->WriteUINT32(sw, FM10000_SYS_CFG_ROUTER(), reg);
            }

            break;

        case FM_ROUTER_TRAP_REDIRECT_EVENT:
            /* Enable/Disable ARP redirect reporting */
            err = SetRouterArpRedirectTrigger(sw);
            break;

        case FM_ROUTER_PHYSICAL_MAC_ADDRESS:

            tmpMac = switchPtr->physicalRouterMac;
            
            /* set physical router SMAC */
            err = switchPtr->WriteUINT64(sw, FM10000_MOD_ROUTER_SMAC(1, 0), tmpMac);

            if (err == FM_OK)
            {
                /* set default (unspecified) router SMAC */
                err = switchPtr->WriteUINT64(sw, FM10000_MOD_ROUTER_SMAC(0, 0), tmpMac);
            }

            if (err == FM_OK)
            {
                mapperMacCfg.macAddr = tmpMac;
                mapperMacCfg.ignoreLength = 0;
                mapperMacCfg.validSMAC = FALSE;
                mapperMacCfg.validDMAC = TRUE;
                mapperMacCfg.mapMac = 1;
                mapperMacCfg.router = TRUE;

                err = fm10000SetMapMac(sw, 
                                       1, 
                                       &mapperMacCfg, 
                                       FM_FM10000_MAP_MAC_ALL, 
                                       TRUE);

            }
            break;

        case FM_ROUTER_VIRTUAL_MAC_ADDRESS:

            for (vroff = 1; vroff < switchPtr->maxVirtualRouters; vroff++)
            {
                if ( (switchPtr->virtualRouterIds[vroff] >= 0) &&
                     (switchPtr->virtualRouterMacModes[vroff] ==
                      FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1) )
                {
                    err = SetVirtualRouterMacRegisters(sw,
                                                       vroff,
                                                       0,
                                                       switchPtr->virtualRouterStates[vroff]);
                    if (err != FM_OK)
                    {
                        break;
                    }
                }
            }

            break;

        case FM_ROUTER_VIRTUAL_MAC_ADDRESS_2:

            for (vroff = 1; vroff < switchPtr->maxVirtualRouters; vroff++)
            {
                if ( (switchPtr->virtualRouterIds[vroff] >= 0) &&
                     (switchPtr->virtualRouterMacModes[vroff] ==
                      FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_2) )
                {
                    err = SetVirtualRouterMacRegisters(sw,
                                                       vroff,
                                                       1,
                                                       switchPtr->virtualRouterStates[vroff]);
                    if (err != FM_OK)
                    {
                        break;
                    }
                }
            }

            break;

        case FM_ROUTER_TRAP_IP_OPTIONS:

            err = switchPtr->ReadUINT32(sw, FM10000_SYS_CFG_ROUTER(), &reg);

            if (err == FM_OK)
            {
                /* set TrapIPOptions field  */
                FM_SET_BIT(reg, FM10000_SYS_CFG_ROUTER, trapIPOptions, switchPtr->routerTrapIpOptions? 1 : 0);
                
                err = switchPtr->WriteUINT32(sw, FM10000_SYS_CFG_ROUTER(), reg);
            }

            break;

        default:
            err = FM_ERR_INVALID_ATTRIB;
            break;

    }   /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000SetRouterAttribute */




/*****************************************************************************/
/** fm10000AddRoute
 * \ingroup intRouterRoute
 *
 * \desc            Add a new routing entry into the router table. The API will
 *                  store the route in order to ensure that the longest
 *                  prefix match will be picked first. The service allows
 *                  setting the route state immediately. The route is added
 *                  to the switch regardless if the route is valid regardless
 *                  if it is active or not.
 *
 *                  Note that it is permitted to add a route before the
 *                  interface exist. The route will loaded in the switch
 *                  but will be inactive until the interface exist and is up.
 *
 *                  Also, if there is no ARP to resolve the DMAC address, then
 *                  the service automatically use the local CPU as DMAC forcing
 *                  any frame using that route to be forward to the local OS
 *                  for further processing.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pNewRoute points to the route that was added to the
 *                  upper-layer route table.  Note that it is expected that
 *                  the pointer passed is the pointer to the entry in the
 *                  table, not to some local copy that may go away.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_NO_FFU_RES_FOUND if no FFU resources can be found.
 * \return          FM_FAIL if updating a route TCAM position failed.
 *
 *****************************************************************************/
fm_status fm10000AddRoute(fm_int            sw,
                          fm_intRouteEntry *pNewRoute)
{
    fm_status               err;
    fm_status               status;
    fm_switch *             switchPtr;
    fm10000_switch *        pSwitchExt;
    fm10000_RouteInfo       routeInfo;
    fm10000_RoutingTable *  routeTable;
    fm10000_TcamRouteEntry *tcamRoute;
    fm10000_RouteSlice *    destSlicePtr;
    fm_int                  destRow;
    fm_fm10000FfuSliceKey   ruleKey[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction            ffuAction;
    fm_bool                 ruleValid;
    fm_bool                 ecmpGroupValid;
    fm10000_RoutePrefix *   routePrefix;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pNewRoute=%p\n",
                 sw,
                 (void *) pNewRoute);

    err        = FM_OK;
    switchPtr  = GET_SWITCH_PTR(sw);
    pSwitchExt = GET_SWITCH_EXT(sw);
    tcamRoute  = NULL;

    if (pNewRoute == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* if the the route uses an ecmp group, perform an early validation of it */
    if (pNewRoute->ecmpGroupId > 0)
    {
        err = fm10000ValidateEcmpGroupState(sw,
                                            pNewRoute->ecmpGroupId,
                                            &ecmpGroupValid);
        FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
    }

    /* Consolidate route information into the routeInfo structure. */
    ClassifyRoute(sw, pNewRoute, &routeInfo);

    /**************************************************
    * ClassifyRoute function may insert TCAM route into
    * the tree. From now use FM_LOG_ABORT instead of
    * FM_LOG_EXIT to perform cleanup correctly.
    **************************************************/

    routeTable = routeInfo.routeTable;
    routePrefix = routeInfo.routePrefix;

    if (routeInfo.vroff == -1)
    {
        err = FM_ERR_INVALID_ARGUMENT;
        FM_LOG_ABORT(FM_LOG_CAT_ROUTING, err);
    }

    err = ValidateRouteInformation(sw, pNewRoute, &routeInfo);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Initialize the action structure */
    err = InitFfuRouteAction(sw, pNewRoute, &routeInfo, &ffuAction);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* allocate a new tcam route */
    err = AllocateAndInitTcamRoute(sw, pNewRoute, &routeInfo, &tcamRoute);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    err = FindFfuEntryForNewRoute(sw,pNewRoute,&routeInfo,&destSlicePtr,&destRow);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    if ( destSlicePtr == NULL )
    {
        /* No destination slice was found. */
        err = FM_ERR_NO_FFU_RES_FOUND;
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING, "Null destination slice\n");
        FM_LOG_ABORT(FM_LOG_CAT_ROUTING, err);
    }

    /* set the FFU key & mask */
    FM_CLEAR(ruleKey);

    err = SetFFuRuleKeyForRoute(sw, &routeInfo, &ruleKey[0]);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Complete the Action definition */
    err = SetFfuRouteAction(sw,
                            pNewRoute,
                            &routeInfo,
                            &ffuAction,
                            &ruleValid);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Write the rule to the hardware */
    err = fm10000SetFFURule(sw,
                            &destSlicePtr->sliceInfo,
                            destRow,
                            ruleValid,
                            ruleKey,
                            &ffuAction,
                            TRUE,
                            TRUE);
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Store the final FFU action structure into the route */
    FM_MEMCPY_S( &tcamRoute->ffuAction,
                 sizeof(tcamRoute->ffuAction),
                 &ffuAction,
                 sizeof(ffuAction) );

    if ( !UpdateTcamRoutePosition(sw,
                                  tcamRoute,
                                  destSlicePtr,
                                  destRow,
                                  TRUE) )
    {
        FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                     "Cannot update TCAM route position\n");
        err = FM_FAIL;
    }
    else
    {
        tcamRoute->dirty = FALSE;
    }
    FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);


ABORT:

        status = err;

        /* If we didn't find a destination slice/row or if there was an
           execution error, release the allocated resourcess. */
        if (status != FM_OK)
        {
            /* Clean up and return the error. */

            if (tcamRoute != NULL)
            {
                err = fmCustomTreeRemove(&routeTable->tcamRouteRouteTree,
                                         tcamRoute,
                                         NULL);
                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                 "Cannot remove TCAM route record from Tree\n");
                }

                /* release TCAM route */
                fmFree(tcamRoute);
                tcamRoute = NULL;
            }

            /* Check if a specific prefix record was created for this route
             * (the tree must be empty). If so, just delete it */
            if ( routePrefix != NULL &&
                 fmCustomTreeSize(&routePrefix->routeTree) == 0 )
            {
                /* remove prefix from the prefix tree */
                err = fmCustomTreeRemove(&routeTable->prefixTree,
                                         &routePrefix->prefix,
                                         NULL);

                if (err != FM_OK)
                {
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                 "Cannot remove prefix record from Tree, prefix=%d\n",
                                 routePrefix->prefix);
                }
                else
                {
                    /* release the prefix */
                    fmFree(routePrefix);
                    routePrefix = NULL;
                }
            }

        }   /* end if (err != FM_OK) */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, status);

}   /* end fm10000AddRoute */


/*****************************************************************************/
/** fm10000DeleteRoute
 * \ingroup intRouterRoute
 *
 * \desc            Delete a routing entry from the router table.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pRoute points to the route to delete.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_UNKNOWN_ROUTE if the route doesn't exist.
 *
 *****************************************************************************/
fm_status fm10000DeleteRoute(fm_int            sw,
                             fm_intRouteEntry *pRoute)
{
    fm_status               err;
    fm10000_RoutingTable *  pRouteTable;
    fm10000_TcamRouteEntry  tcamRouteKey;
    fm10000_TcamRouteEntry *pTcamRoute;
    

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p\n",
                 sw,
                 (void *) pRoute);

    err = FM_OK;

    if (pRoute == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pRouteTable = GetRouteTableForRoute(sw, &pRoute->route);

        if (pRouteTable == NULL)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            /* Try to find the TCAM route in the table */
            FM_CLEAR(tcamRouteKey);
            tcamRouteKey.routePtr = pRoute;

            err = fmCustomTreeFind( &pRouteTable->tcamRouteRouteTree,
                                    &tcamRouteKey,
                                    (void **) &pTcamRoute );

            if (err == FM_OK)
            {
                /* Remove the TCAM route from the hardware */
                if ( UpdateTcamRoutePosition(sw, pTcamRoute, NULL, -1, TRUE) )
                {
                    /* Remove the TCAM route from the software tables */
                    fmCustomTreeRemove(&pRouteTable->tcamRouteRouteTree,
                                       pTcamRoute,
                                       NULL);
                    fmFree(pTcamRoute);
                    pTcamRoute = NULL;
                }
                else
                {
                    /* cannot remove route from TCAM */
                    FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                                 "Cannot remove route from TCAM route=%p\n",
                                 (void *) pRoute);
                    err = FM_FAIL;
                }
            }

        }   /* end slse if (pRouteTable == NULL) */

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000DeleteRoute */


/*****************************************************************************/
/** fm10000ReplaceECMPBaseRoute
 * \ingroup intRouterRoute
 *
 * \desc            Replaces an existing route entry with a new one.
 *                  The assumption is that a route pointer is being
 *                  replaced by a different route pointer but that the
 *                  actual route in the hardware is not being touched.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       oldRoute points to the route being replaced.
 *
 * \param[in]       pNewRoute points to the new route replacing oldRoute.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_SWITCH if sw is invalid.
 * \return          FM_ERR_INVALID_ROUTE if the route description is invalid.
 * \return          FM_ERR_UNKNOWN_ROUTE if the route doesn't exist.
 *
 *****************************************************************************/
fm_status fm10000ReplaceECMPBaseRoute(fm_int            sw,
                                      fm_intRouteEntry *oldRoute,
                                      fm_intRouteEntry *pNewRoute)
{
    fm_status               err;
    fm10000_RoutingTable *  routeTable;
    fm10000_TcamRouteEntry  tcamRouteKey;
    fm10000_TcamRouteEntry *tcamRoute;
    
    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, oldRoute=%p, pNewRoute=%p\n",
                 sw,
                 (void *) oldRoute,
                 (void *) pNewRoute);


    routeTable = GetRouteTableForRoute(sw, &oldRoute->route);

    if (routeTable == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* Try to find the TCAM route in the table */
    FM_CLEAR(tcamRouteKey);
    tcamRouteKey.routePtr = oldRoute;

    err = fmCustomTreeFind( &routeTable->tcamRouteRouteTree,
                            &tcamRouteKey,
                           (void **) &tcamRoute );

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    }

    /* Replace the old generic route pointer with the new route pointer */
    tcamRoute->routePtr = pNewRoute;

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);


}   /* end fm10000ReplaceECMPBaseRoute */




/*****************************************************************************/
/** fm10000SetRouteActive
 * \ingroup intRouterRoute
 *
 * \desc            Set route active or inactive in the hardware.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       pRoute points the route to change.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetRouteActive(fm_int            sw,
                                fm_intRouteEntry *pRoute)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm10000_RoutingTable *  pRouteTable;
    fm10000_TcamRouteEntry  tcamRouteKey;
    fm10000_TcamRouteEntry *pTcamRoute;
    fm_bool                 valid;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p\n",
                 sw,
                 (void *) pRoute);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    if (pRoute == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pRouteTable = GetRouteTableForRoute(sw, &pRoute->route);

        if (pRouteTable == NULL)
        {
            err = FM_ERR_INVALID_ARGUMENT;
        }
        else
        {
            /* Try to find the TCAM route in the table */
            FM_CLEAR(tcamRouteKey);
            tcamRouteKey.routePtr = pRoute;

            err = fmCustomTreeFind(&pRouteTable->tcamRouteRouteTree,
                                   &tcamRouteKey,
                                   (void **) &pTcamRoute );
            if (err == FM_OK)
            {
                /* Determine the valid flag */
                valid = FALSE;

                if (pTcamRoute->routePtr->active)
                {
                    if (pRoute->ecmpGroupId >= 0)
                    {
                        err = fm10000ValidateEcmpGroupState(sw, 
                                                            pRoute->ecmpGroupId, 
                                                            &valid);
                        if (err != FM_OK)
                        {
                            /* on error, flag the route as NOT-valid */ 
                            valid = FALSE;
                        }
                    }
                    else
                    {
                        valid = TRUE;
                    }
                }

                /* update the route's valid flag in the FFU */
                err = fm10000SetFFURuleValid(sw,
                                             &pTcamRoute->routeSlice->sliceInfo,
                                             pTcamRoute->tcamSliceRow,
                                             valid,
                                             TRUE);
            }
        }   /* end else if (pRouteTable == NULL) */
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000SetRouteActive */




/*****************************************************************************/
/** fm10000AddVirtualRouter
 * \ingroup intRouter
 *
 * \desc            Initializes hardware registers for a new virtual router.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vroff is the offset of the virtual router in the table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000AddVirtualRouter(fm_int sw,
                                  fm_int vroff)
{
    fm_status           err;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, vroff=%d\n",
                 sw,
                 vroff);

    if (vroff == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    /*  New virtual router is always initialized for its default MAC mode:
     *  FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1
     */ 
    err = SetVirtualRouterMacRegisters(sw,
                                       vroff,
                                       0,
                                       FM_ROUTER_STATE_ADMIN_DOWN);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000AddVirtualRouter */


/*****************************************************************************/
/** fm10000RemoveVirtualRouter
 * \ingroup intRouter
 *
 * \desc            Releases hardware resources for a virtual router.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vroff is the offset of the virtual router in the table.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000RemoveVirtualRouter(fm_int sw,
                                     fm_int vroff)
{
    fm_status           err;
    fm_fm10000MapMacCfg mapperMacCfg;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, vroff=%d\n",
                 sw,
                 vroff);

    err = FM_OK;

    if (vroff != 0)
    {
        mapperMacCfg.macAddr = 0;
        mapperMacCfg.ignoreLength = 0;
        mapperMacCfg.validSMAC = FALSE;
        mapperMacCfg.validDMAC = FALSE;
        mapperMacCfg.mapMac = 0;
        mapperMacCfg.router = FALSE;

        err = fm10000SetMapMac(sw, 
                               vroff + 1, 
                               &mapperMacCfg, 
                               FM_FM10000_MAP_MAC_ALL, 
                               TRUE);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RemoveVirtualRouter */


/*****************************************************************************/
/** fm10000SetRouterState
 * \ingroup intRouter
 *
 * \desc            Set hardware "Routable" register for an existing virtual
 *                  router.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vroff is the virtual router offset.
 *
 * \param[in]       state is the state of the virtual router.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetRouterState(fm_int          sw,
                                fm_int          vroff, 
                                fm_routerState  state)
{
    fm_status           err;
    fm_switch *         switchPtr;
    fm_macaddr          tmpMac;
    fm_bool             routerState;
    fm_fm10000MapMacCfg mapperMacCfg;
    fm_int              vrMacId;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw=%d, vroff=%d\n", sw, vroff);

    /*  Only Update the status of the router into the
     *  MAC_MAPPER in case of a virtual router  */

    err = FM_OK;
    if (vroff > 0)
    {
        switchPtr = GET_SWITCH_PTR(sw);

        routerState = (state == FM_ROUTER_STATE_ADMIN_UP) ? TRUE : FALSE;

        switch (switchPtr->virtualRouterMacModes[vroff])
        {
            case FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1:
                vrMacId = 0;
                break;
            case FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_2:
                vrMacId = 1;
                break;
            default:
                FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                            "Unexpected MAC mode of virtual router %d: %d!\n",
                            switchPtr->virtualRouterIds[vroff],
                            switchPtr->virtualRouterMacModes[vroff]);
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_ASSERTION_FAILED);
        }
        tmpMac = switchPtr->virtualRouterMac[vrMacId]
                 | switchPtr->virtualRouterIds[vroff];

        mapperMacCfg.macAddr = tmpMac;
        mapperMacCfg.ignoreLength = 0;
        mapperMacCfg.validSMAC = FALSE;
        mapperMacCfg.validDMAC = TRUE;
        mapperMacCfg.mapMac = vroff + 1;
        mapperMacCfg.router = routerState;

        err = fm10000SetMapMac(sw, 
                               vroff + 1, 
                               &mapperMacCfg, 
                               FM_FM10000_MAP_MAC_ALL, 
                               TRUE);
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000SetRouterState */




/*****************************************************************************/
/** fm10000SetRouterMacMode
 * \ingroup intRouter
 *
 * \desc            Set hardware DMAC mapper and router SMAC for an existing
 *                  virtual router according to its updated MAC mode.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       vroff is the virtual router offset.
 *
 * \param[in]       mode is the MAC mode of the virtual router.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetRouterMacMode(fm_int sw,
                                  fm_int vroff,
                                  fm_routerMacMode mode)
{
    fm_switch *    switchPtr;
    fm_status      err;
    fm_routerState state;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, vroff=%d, mode=%d\n",
                 sw, vroff, mode);

    /* Mode update is supported only for virtual routers */
    if (vroff == 0)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_VIRTUAL_ROUTER_ONLY);
    }

    switchPtr = GET_SWITCH_PTR(sw);
    state = switchPtr->virtualRouterStates[vroff];

    switch (mode)
    {
        case FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_1:
            err = SetVirtualRouterMacRegisters(sw, vroff, 0, state);
            break;

        case FM_ROUTER_MAC_MODE_VIRTUAL_MAC_ADDRESS_2:
            err = SetVirtualRouterMacRegisters(sw, vroff, 1, state);
            break;
        default:
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                        "Invalid MAC mode: %d\n",
                        mode);
            err = FM_ERR_UNSUPPORTED;
            break;

    }   /* end switch (mode) */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000SetRouterMacMode */




/*****************************************************************************/
/** fm10000UpdateEcmpRoutes
 * \ingroup intRouter
 *
 * \desc            Updates the ARP index and count for all routes using
 *                  the specified ECMP group.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       groupId is the ECMP group to update.
 * 
 * \param[in]       newIndex is the new ARP index to use for this group.
 *
 * \param[in]       pathCount is the new ARP count to configure for this group.
 * 
 * \param[in]       pathCountType is the new ARP count type to configure for
 *                  this group.
 * 
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000UpdateEcmpRoutes(fm_int       sw,
                                  fm_int       groupId,
                                  fm_uint16    newIndex,
                                  fm_int       pathCount,
                                  fm_int       pathCountType)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm_intEcmpGroup *       parent;
    fm_customTreeIterator   iter;
    fm_intRouteEntry *      routeKey;
    fm_intRouteEntry *      route;
    fm10000_RoutingTable *  routeTable;
    fm10000_TcamRouteEntry  tcamRouteKey;
    fm10000_TcamRouteEntry *tcamRoute;
    fm_bool                 valid;
    fm_bool                 newValid;
    fm_fm10000FfuSliceKey   ruleKey[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction            actionList;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, groupId=%d, newIndex=%d, pathCount=%d, pathCountType=%d\n",
                 sw,
                 groupId,
                 newIndex,
                 pathCount,
                 pathCountType);

    err = FM_OK;
    switchPtr = GET_SWITCH_PTR(sw);

    parent = switchPtr->ecmpGroups[groupId];

    if (parent == NULL)
    {
        /* invalid groupId */
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        fmCustomTreeIterInit(&iter, &parent->routeTree);

        while (1)
        {
            err = fmCustomTreeIterNext( &iter,
                                       (void **) &routeKey,
                                       (void **) &route);

            if (err != FM_OK)
            {
                if (err == FM_ERR_NO_MORE)
                {
                    /* err has local significance only */
                    err = FM_OK;
                }
                break;
            }

            routeTable = GetRouteTableForRoute(sw, &route->route);

            if (routeTable == NULL)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
            }

            /* Try to find the TCAM route in the table */
            FM_CLEAR(tcamRouteKey);
            tcamRouteKey.routePtr = route;

            err = fmCustomTreeFind( &routeTable->tcamRouteRouteTree,
                                    &tcamRouteKey,
                                   (void **) &tcamRoute );

            if (err != FM_OK)
            {
                FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
            }

            if ( (tcamRoute->routeSlice != NULL) && (tcamRoute->tcamSliceRow >= 0) )
            {
                /* read existing rule from the FFU */
                err = fm10000GetFFURule(sw,
                                        &tcamRoute->routeSlice->sliceInfo,
                                        tcamRoute->tcamSliceRow,
                                        &valid,
                                        ruleKey,
                                        &actionList,
                                        TRUE);


                if (err != FM_OK)
                {
                    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
                }

                newValid = valid;

                if ( (newIndex <= 0) || (pathCount <= 0) || !route->active)
                {
                    newValid = FALSE;
                }
                else if (!valid)
                {
                    newValid = TRUE;
                    actionList.data.arp.arpIndex = 0;
                    actionList.data.arp.count = 0;
                    actionList.data.arp.arpType = 0;
                }

                /* if needed, update arp base address and count */
                if ( (valid != newValid)                         ||
                     (actionList.data.arp.arpIndex != newIndex) ||
                     (actionList.data.arp.count != pathCount) )
                {
                    if (newValid)
                    {
                        actionList.data.arp.arpIndex = newIndex;
                        actionList.data.arp.count = pathCount;
                        actionList.data.arp.arpType = pathCountType;
                    }
                    else
                    {
                        actionList.data.arp.arpIndex = 0;
                        actionList.data.arp.count    = 1;
                        actionList.data.arp.arpType = 0;
                    }

                    /* write rule back to the FFU */
                    err = fm10000SetFFURule(sw,
                                            &tcamRoute->routeSlice->sliceInfo,
                                            tcamRoute->tcamSliceRow,
                                            newValid,
                                            ruleKey,
                                            &actionList,
                                            TRUE,
                                            TRUE);

                    if (err != FM_OK)
                    {
                        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
                    }

                    tcamRoute->dirty = FALSE;
                }
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fm10000UpdateEcmpRoutes */




/*****************************************************************************/
/** fm10000DbgDumpRouteStats
 * \ingroup intDebug
 *
 * \desc            Dump Router information (short version).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fm10000DbgDumpRouteStats(fm_int sw)
{
    fm_int                  route;
    fm10000_RoutingTable *  routeTable;
    fm10000_RouteSlice *    slicePtr;
    fm_int                  slice;
    fm10000_RouteTcamSlice *tcamSlicePtr;
    fm10000_TcamRouteEntry *tcamRoutePtr;
    fm_int                  sliceCount;
    fm_int                  tcamRouteCount;
    fm10000_switch *        pSwitchExt;

    FM_LOG_PRINT("fm10000DebugDumpRouteStats\n");
    route = 0;

    pSwitchExt = GET_SWITCH_EXT(sw);

    while (RouteTypes[route] != FM10000_NUM_ROUTE_TYPES)
    {
        routeTable = (RouteTypes[route] == FM10000_ROUTE_TYPE_UNUSED) ? NULL : GetRouteTable(sw, RouteTypes[route]);

        if (routeTable == NULL)
        {
            route++;
            continue;
        }

        sliceCount     = 0;
        tcamRouteCount = 0;

        slicePtr = GetFirstSlice(routeTable);

        while (slicePtr != NULL)
        {
            sliceCount++;
            slicePtr = GetNextSlice(slicePtr);
        }

        tcamRoutePtr = routeTable->firstTcamRoute;

        while (tcamRoutePtr != NULL)
        {
            tcamRouteCount++;

            tcamRoutePtr = tcamRoutePtr->nextTcamRoute;
        }

        FM_LOG_PRINT("    route type %d: slices = %d, TCAM routes = %d\n",
                     (fm_int) RouteTypes[route],
                     sliceCount,
                     tcamRouteCount);

        route++;
    }

    sliceCount   = 0;

    for (slice = 0 ; slice < FM10000_MAX_FFU_SLICES ; slice++)
    {
        tcamSlicePtr = &pSwitchExt->routeStateTable.routeTcamSliceArray[slice];

        if (tcamSlicePtr->inUse)
        {
            sliceCount++;
        }
    }

    FM_LOG_PRINT("    TCAM slices in use = %d.\n",
                 sliceCount);

}   /* end fm10000DbgDumpRouteStats */




/*****************************************************************************/
/** fm10000DbgDumpStateTable
 * \ingroup intDebug
 *
 * \desc            Dump Router State Table information (verbose version).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fm10000DbgDumpStateTable(fm_int sw)
{
    fm_status       err;
    fm10000_switch *pSwitchExt;
    fm_int          firstSlice;
    fm_int          lastSlice;


    pSwitchExt = GET_SWITCH_EXT(sw);

    err = fm10000GetFFUSliceOwnership(sw,
                                      FM_FFU_OWNER_ROUTING,
                                      &firstSlice,
                                      &lastSlice);

    if (err == FM_OK)
    {
        FM_LOG_PRINT("Routing FFU Slice Ownership: "
                     "first slice %d, last slice %d\n\n",
                     firstSlice,
                     lastSlice);
    }
    else if (err == FM_ERR_NO_FFU_RES_FOUND)
    {
        FM_LOG_PRINT("Routing does not own any FFU slices\n\n");
    }
    else
    {
        FM_LOG_PRINT( "Unexpected error from fm10000FFUGetSliceOwnership: %d (%s)\n\n",
                     err,
                     fmErrorMsg(err) );
    }

        /* Dump Route State Table Information */
    FM_LOG_PRINT("\nRoute State Table:\n"
                 "  routeFirstTcamSlice     = %2d, routeLastTcamSlice     = %2d\n"
                 "  ipv4UcastFirstTcamSlice = %2d, ipv4UcastLastTcamSlice = %2d\n"
                 "  ipv4McastFirstTcamSlice = %2d, ipv4McastLastTcamSlice = %2d\n"
                 "  ipv6UcastFirstTcamSlice = %2d, ipv6UcastLastTcamSlice = %2d\n"
                 "  ipv6McastFirstTcamSlice = %2d, ipv6McastLastTcamSlice = %2d\n",
                 pSwitchExt->routeStateTable.routeFirstTcamSlice,
                 pSwitchExt->routeStateTable.routeLastTcamSlice,
                 pSwitchExt->routeStateTable.ipv4UcastFirstTcamSlice,
                 pSwitchExt->routeStateTable.ipv4UcastLastTcamSlice,
                 pSwitchExt->routeStateTable.ipv4McastFirstTcamSlice,
                 pSwitchExt->routeStateTable.ipv4McastLastTcamSlice,
                 pSwitchExt->routeStateTable.ipv6UcastFirstTcamSlice,
                 pSwitchExt->routeStateTable.ipv6UcastLastTcamSlice,
                 pSwitchExt->routeStateTable.ipv6McastFirstTcamSlice,
                 pSwitchExt->routeStateTable.ipv6McastLastTcamSlice);

}   /* end fm10000DbgDumpStateTable */




/*****************************************************************************/
/** fm10000DbgDumpPrefixLists
 * \ingroup intDebug
 *
 * \desc            Dump Router State Table information (verbose version).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fm10000DbgDumpPrefixLists(fm_int sw)
{
    fm_status              err;
    fm10000_switch *       pSwitchExt;
    fm_int                 routeType;
    fm_int                 treeSize;
    fm10000_RoutingTable * pRouteTable;
    fm_int *               prefixKey;
    fm10000_RoutePrefix *  prefixPtr;
    fm_customTreeIterator  iter;


    err = FM_OK;
    pSwitchExt = GET_SWITCH_EXT(sw);

    for (routeType = 0 ; routeType < FM10000_NUM_ROUTE_TYPES ; routeType++)
    {
        pRouteTable = pSwitchExt->routeStateTable.routeTables[routeType];

        if (pRouteTable != NULL)
        {

            FM_LOG_PRINT("Dump of Prefix Records for Route Type %d\n",
                         pRouteTable->routeType);

            fmCustomTreeIterInit(&iter, &pRouteTable->prefixTree);

            while (err == FM_OK)
            {
                err = fmCustomTreeIterNext(&iter,
                                           (void **) &prefixKey,
                                           (void **) &prefixPtr);
                if (err == FM_OK)
                {
                    treeSize = fmCustomTreeSize(&prefixPtr->routeTree);

                    FM_LOG_PRINT("    Prefix 0x%x (DstIP:%d - SrcIP:%d - Vlan:%d): %d routes\n",
                                 prefixPtr->prefix,
                                 ((prefixPtr->prefix >> FM_DST_IP_PREFIX_POSITION) & 0xff),
                                 ((prefixPtr->prefix >> FM_SRC_IP_PREFIX_POSITION) & 0xff),
                                 ((prefixPtr->prefix >> FM_VLAN_PREFIX_POSITION)   & 0x0f),
                                 treeSize);

                    FM_LOG_PRINT( "        prefixPtr %p, prevPrefix %p, "
                                 "nextPrefix %p, firstTcamRoute %p, "
                                 "lastTcamRoute %p\n",
                                 (void *) prefixPtr,
                                 (void *) prefixPtr->prevPrefix,
                                 (void *) prefixPtr->nextPrefix,
                                 (void *) prefixPtr->firstTcamRoute,
                                 (void *) prefixPtr->lastTcamRoute);
                }

            }   /* end while (err == FM_OK) */

            /* notify errors iterating the prefix tree */
            if (err != FM_OK && err != FM_ERR_NO_MORE)
            {
                FM_LOG_PRINT( "Error %d (%s) reading iterator\n",
                             err,
                             fmErrorMsg(err) );
            }

        }   /* if (pRouteTable != NULL) */

    }   /* for (routeIndex = 0 ...) */

}   /* end fm10000DbgDumpPrefixLists */




/*****************************************************************************/
/** fm10000DbgDumpInternalRouteTables
 * \ingroup intDebug
 *
 * \desc            Dump Internal Router State Table information (verbose
 *                  version).
 *
 * \param[in]       sw is the switch on which to operate.
 * 
 * \param[in]       flags contains a bit-field describing which internal
 *                  tables to dump.  See ''Router Debug Dump Flags'' for
 *                  a list of flag values.
 * 
 * \return          nothing.
 *
 *****************************************************************************/
void fm10000DbgDumpInternalRouteTables(fm_int sw,
                                       fm_int flags)
{
    fm_status               err;
    fm10000_switch *        pSwitchExt;
    fm_int                  routeTypeIndex;
    fm_int                  sliceRow;
    fm_int                  treeSize;
    fm_int                  index;
    fm_bool                 routeIsUnicast;
    fm_bool                 dumpMulticastRoutes;
    fm10000_RoutingTable *  pRouteTable;
    fm10000_RouteSlice *    pRouteSlice;
    fm10000_TcamRouteEntry *pFirstTcamRoute;
    fm10000_TcamRouteEntry *pLastTcamRoute;
    fm10000_TcamRouteEntry *pTcamRouteKey;
    fm10000_TcamRouteEntry *pTcamRoute;
    fm_int *                pPrefixKey;
    fm10000_RoutePrefix *   pPrefix;
    fm_customTreeIterator   iter;
    fm_int                  ecmpGroupId;
    fm_int                  ecmpBaseArpIndex;
    fm_int                  ecmpArpCount;
    fm_int                  ecmpPathCountType;
    fm_char                 textBuff[200];


    pSwitchExt = GET_SWITCH_EXT(sw);
    dumpMulticastRoutes = flags & FM_ROUTER_DUMP_MULTICAST_ROUTES;

    for (routeTypeIndex = 0 ; routeTypeIndex < FM10000_NUM_ROUTE_TYPES ; routeTypeIndex++)
    {
        pRouteTable = pSwitchExt->routeStateTable.routeTables[routeTypeIndex];

        if (pRouteTable == NULL)
        {
            continue;
        }

        switch (pRouteTable->routeType)
        {
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
            case FM10000_ROUTE_TYPE_V4SG:
                if (!dumpMulticastRoutes)
                {
                    continue;
                }
                else
                {
                    FM_LOG_PRINT("\nIPv4 SG Multicast Route Table\n");
                }
                break;

            case FM10000_ROUTE_TYPE_V6SG:
                if (!dumpMulticastRoutes)
                {
                    continue;
                }
                else
                {
                    FM_LOG_PRINT("\nIPv6 SG Multicast Route Table\n");
                }
                break;

            case FM10000_ROUTE_TYPE_V4DV:
                if (!dumpMulticastRoutes)
                {
                    continue;
                }
                else
                {
                    FM_LOG_PRINT("\nIPv4 DV Multicast Route Table\n");
                }
                break;

            case FM10000_ROUTE_TYPE_V6DV:
                if (!dumpMulticastRoutes)
                {
                    continue;
                }
                else
                {
                    FM_LOG_PRINT("\nIPv6 DV Multicast Route Table\n");
                }
                break;
#endif
            case FM10000_ROUTE_TYPE_V4DSV:
                if (!dumpMulticastRoutes)
                {
                    continue;
                }
                else
                {
                    FM_LOG_PRINT("\nIPv4 DSV Multicast Route Table\n");
                }
                break;

            case FM10000_ROUTE_TYPE_V6DSV:
                if (!dumpMulticastRoutes)
                {
                    continue;
                }
                else
                {
                    FM_LOG_PRINT("\nIPv6 DSV Multicast Route Table\n");
                }
                break;

            case FM10000_ROUTE_TYPE_V4U:
                FM_LOG_PRINT("\nIPv4 Unicast / *G Multicast Route Table\n");
                    break;

            case FM10000_ROUTE_TYPE_V6U:
                FM_LOG_PRINT("\nIPv6 Unicast / *G Multicast Route Table\n");
                    break;

            default:

                    break;

        }   /* end switch (pRouteTable->routeType) */


        FM_LOG_PRINT("  numPrefixes = %d\n",
                     fmCustomTreeSize(&pRouteTable->prefixTree));
        
        FM_LOG_PRINT("  Route Slice Table\n");
        pRouteSlice = GetFirstSlice(pRouteTable);
        
        while (pRouteSlice != NULL)
        {
            pFirstTcamRoute = (pRouteSlice->highestRow >= 0) ? pRouteSlice->routes[pRouteSlice->highestRow] : NULL;
            pLastTcamRoute  = (pRouteSlice->highestRow >= 0) ? pRouteSlice->routes[pRouteSlice->lowestRow]  : NULL;
            
            FM_LOG_PRINT("    Slice %p: first TCAM route %p, "
                         "last TCAM route %p\n",
                         (void *) pRouteSlice,
                         (void *) pFirstTcamRoute,
                         (void *) pLastTcamRoute);
            
            FM_LOG_PRINT("      width = %d, first Tcam Slice = %d, "
                         "last Tcam Slice = %d\n      Routes:\n",
                         pRouteSlice->sliceWidth,
                         pRouteSlice->firstTcamSlice,
                         pRouteSlice->lastTcamSlice);
            
            for (sliceRow = FM10000_FFU_ENTRIES_PER_SLICE - 1 ; sliceRow >= 0 ; sliceRow--)
            {
                if (pRouteSlice->routes[sliceRow] != NULL)
                {
                    FM_LOG_PRINT( "        %03d: %p\n",
                                  sliceRow,
                                  (void *) pRouteSlice->routes[sliceRow] );
                }
            }

            pRouteSlice = GetNextSlice(pRouteSlice);
            
        }   /* end while (pRouteSlice != NULL) */
        
        FM_LOG_PRINT("  TCAM Routes\n");
        fmCustomTreeIterInit(&iter, &pRouteTable->tcamRouteRouteTree);

        err = fmCustomTreeIterNext(&iter,
                                   (void **) &pTcamRouteKey,
                                   (void **) &pTcamRoute);

        
        if (err != FM_OK)
        {
            if (err == FM_ERR_NO_MORE)
            {
                /* err has local significance only */
                pTcamRoute = NULL;
            }
            else
            {
                FM_LOG_PRINT("Error %d (%s) reading first iterator for route %d\n",
                             err,
                             fmErrorMsg(err),
                             routeTypeIndex);
                break;
            }
        }

        while (pTcamRoute != NULL)
        {
            routeIsUnicast = fmIsRouteEntryUnicast(&pTcamRoute->routePtr->route);

            if (( routeIsUnicast && (flags & FM_ROUTER_DUMP_UNICAST_ROUTES))  ||
                (!routeIsUnicast && (flags & FM_ROUTER_DUMP_MULTICAST_ROUTES)) )
            {
                fmDbgBuildRouteDescription(&pTcamRoute->routePtr->route, textBuff);

                if (pTcamRoute->ecmpGroup == NULL)
                {
                    ecmpGroupId = -1;
                    ecmpBaseArpIndex = 0;
                    ecmpArpCount = 0;
                }
                else
                {
                    ecmpGroupId = pTcamRoute->ecmpGroup->pParent->groupId;
                    err = fm10000GetECMPGroupArpInfo(sw,
                                                     ecmpGroupId,
                                                     NULL,
                                                     &ecmpBaseArpIndex,
                                                     &ecmpArpCount,
                                                     &ecmpPathCountType);
                }

                FM_LOG_PRINT( "    TCAM Route %p: prev = %p, next = %p, "
                             "slice = %p, row = %d\n      %s, action = %d\n"
                             "      ecmpGroup=%p(%d), baseArp=%d, arpCount=%d\n",
                             (void *) pTcamRoute,
                             (void *) pTcamRoute->prevTcamRoute,
                             (void *) pTcamRoute->nextTcamRoute,
                             (void *) pTcamRoute->routeSlice,
                             pTcamRoute->tcamSliceRow,
                             textBuff,
                             pTcamRoute->action.action,
                             (void *) pTcamRoute->ecmpGroup,
                             ecmpGroupId,
                             ecmpBaseArpIndex,
                             ecmpArpCount);

            }

            err = fmCustomTreeIterNext(&iter,
                                       (void **) &pTcamRouteKey,
                                       (void **) &pTcamRoute);

            if (err == FM_ERR_NO_MORE)
            {
                /* err has local significance only */
                pTcamRoute = NULL;
            }
        }

        if (flags & FM_ROUTER_DUMP_BY_PREFIX)
        {
            FM_LOG_PRINT("  Dump of Routes By Prefix\n");

            fmCustomTreeIterInitBackwards(&iter, &pRouteTable->prefixTree);
            err = fmCustomTreeIterNext(&iter, (void **) &pPrefixKey, (void **) &pPrefix);

           
            if (err != FM_OK)
            {
                if (err == FM_ERR_NO_MORE)
                {
                    /* err has local significance only */
                    pPrefix = NULL;
                }
                else
                {
                    FM_LOG_PRINT("Error %d (%s) reading first iterator\n",
                                 err,
                                 fmErrorMsg(err));
                    break;
                }
            }

            while (pPrefix != NULL)
            {
                treeSize = fmCustomTreeSize(&pPrefix->routeTree);

                FM_LOG_PRINT("    Prefix 0x%x (DstIP:%d - SrcIP:%d - Vlan:%d): %d routes\n",
                             pPrefix->prefix,
                             ((pPrefix->prefix >> FM_DST_IP_PREFIX_POSITION) & 0xff),
                             ((pPrefix->prefix >> FM_SRC_IP_PREFIX_POSITION) & 0xff),
                             ((pPrefix->prefix >> FM_VLAN_PREFIX_POSITION)   & 0x0f),
                             treeSize);
                pTcamRoute = GetFirstPrefixRoute(pPrefix);

                index = 0;
                while (pTcamRoute != NULL)
                {
                    fmDbgBuildRouteDescription(&pTcamRoute->routePtr->route, textBuff);
                    FM_LOG_PRINT("        %5d: %s\n"
                                 "               tcamSlice = %d, "
                                 "row = %d\n",
                                 index,
                                 textBuff,
                                 pTcamRoute->routeSlice->firstTcamSlice,
                                 pTcamRoute->tcamSliceRow);

                    pTcamRoute = pTcamRoute->nextPrefixRoute;
                    index++;
                }

                err = fmCustomTreeIterNext(&iter,
                                           (void **) &pPrefixKey,
                                           (void **) &pPrefix);

                if (err == FM_ERR_NO_MORE)
                {
                    /* err has local significance only */
                    pPrefix = NULL;
                }
            }
        }

        if (flags & FM_ROUTER_DUMP_BY_SLICE_AND_ROW)
        {
            FM_LOG_PRINT("  TCAM Routes Sorted by Slice # & Row\n");

            pTcamRoute = (pRouteTable != NULL) ? pRouteTable->firstTcamRoute : NULL;

            while (pTcamRoute != NULL)
            {
                fmDbgBuildRouteDescription(&pTcamRoute->routePtr->route,
                                           textBuff);
                FM_LOG_PRINT("    TCAM Route %p: prev = %p, next = %p, "
                             "slice = %p, row = %d\n      %s\n",
                             (void *) pTcamRoute,
                             (void *) pTcamRoute->prevTcamRoute,
                             (void *) pTcamRoute->nextTcamRoute,
                             (void *) pTcamRoute->routeSlice,
                             pTcamRoute->tcamSliceRow,
                             textBuff);
                pTcamRoute = pTcamRoute->nextTcamRoute;
            }
        }
    }   /* end for (routeType = 0 ; ...) */

}   /* end fm10000DbgDumpInternalRouteTables */




/*****************************************************************************/
/** fm10000DbgDumpHwFFUContent
 * \ingroup intDebug
 *
 * \desc            Dump Routing HW FFU content.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flags contains a bit-field describing which internal
 *                  tables to dump.  See ''Router Debug Dump Flags'' for
 *                  a list of flag values.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fm10000DbgDumpHwFFUContent(fm_int sw, 
                                fm_int flags)
{
    FM_NOT_USED(sw);
    FM_NOT_USED(flags);

    /* marka TBC */

}   /* end fm10000DbgDumpHwFFUContent */




/*****************************************************************************/
/** fm10000DbgDumpRouteTables
 * \ingroup intDebug
 *
 * \desc            Dump Router information (verbose version).
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       flags contains a bit-field describing which internal
 *                  tables to dump.  See ''Router Debug Dump Flags'' for
 *                  a list of flag values.
 *
 * \return          nothing.
 *
 *****************************************************************************/
void fm10000DbgDumpRouteTables(fm_int sw, 
                               fm_int flags)
{
    fm10000_switch *      pSwitchExt;
    

    pSwitchExt = GET_SWITCH_EXT(sw);

    FM_LOG_PRINT("\n\n*** FM10000-Specific Routing Information ***\n");

    fm10000DbgDumpStateTable(sw);

    /* Dump ECMP tables */
    if (flags & FM_ROUTER_DUMP_ECMP_TABLES)
    {
        fm10000DbgDumpEcmpTables(sw);
    }

    /* Dump arp table */
    if (flags & FM_ROUTER_DUMP_INTERNAL_ARP)
    {
        fm10000DbgPrintArpTableInfo(sw);
    }

    /* Dump hardware arp table */
    if (flags & FM_ROUTER_DUMP_HARDWARE_ARP)
    {
        fm10000DbgDumpArpTableExtended(sw);
    }

    if (flags & FM_ROUTER_DUMP_RT_PREFIX_LISTS)
    {
        fm10000DbgDumpPrefixLists(sw);
    }

    /* Dump internal route tables */
    if ( flags & FM_ROUTER_DUMP_ALL_ROUTES)
    {
        fm10000DbgDumpInternalRouteTables(sw, flags);
    }

    /* Dump TCAM Slice Table */
    if (flags & FM_ROUTER_DUMP_TCAM_TABLES)
    {
        DumpAllTcamSliceUsage(sw, &pSwitchExt->routeStateTable);
    }

    /* Dump Hardware FFU Contents */
    if (flags & (FM_ROUTER_DUMP_FFU_HW_TABLES | FM_ROUTER_DUMP_ACL_FFU_HW_TABLES))
    {
        fm10000DbgDumpHwFFUContent(sw, flags);
    }

}   /* end fm10000DbgDumpRouteTables */


/*****************************************************************************/
/** fm10000CompareTcamRoutes
 * \ingroup intRouter
 *
 * \desc            Compare fm10000 internal Route entries.
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
fm_int fm10000CompareTcamRoutes(const void *first,
                                const void *second)
{
    fm10000_TcamRouteEntry *firstRoute  = (fm10000_TcamRouteEntry *) first;
    fm10000_TcamRouteEntry *secondRoute = (fm10000_TcamRouteEntry *) second;
    fm_int comparison;

    comparison = fmCompareEcmpRoutes(&firstRoute->routePtr->route,
                                     &secondRoute->routePtr->route);

    return comparison;

}   /* end fm10000CompareTcamRoutes */


/*****************************************************************************/
/** fm10000DbgValidateRouteTables
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
fm_status fm10000DbgValidateRouteTables(fm_int sw)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm10000_RoutingState *  stateTable;
    fm_int                  route;
    fm10000_RoutingTable *  routeTable;
    fm10000_switch *        pSwitchExt;
    fm_int                  numTcamRoutes;
    fm_int                  tcamTreeRouteCount;
    fm10000_TcamRouteEntry *prevTcamRoute;
    fm10000_TcamRouteEntry *curTcamRoute;
    fm10000_TcamRouteEntry *tcamRouteKey;
    fm10000_TcamRouteEntry *tcamRouteValue;
    fm_customTreeIterator   iter;
    fm_int                  i;
    char                    curRouteDesc[500];
    char                    prevRouteDesc[500];
    fm_fm10000FfuSliceKey   ruleKey[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction            ffuActionList[FM10000_MAX_FFU_SLICES];
    fm_bool                 ruleValid;


    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw = %d\n", sw);

    err        = FM_FAIL;
    switchPtr  = GET_SWITCH_PTR(sw);
    pSwitchExt        = GET_SWITCH_EXT(sw);
    stateTable = &pSwitchExt->routeStateTable;
    ruleValid  = FALSE;

    for (route = FM10000_ROUTE_TYPE_UNUSED ;
         route < FM10000_NUM_ROUTE_TYPES ;
         route++)
    {
        routeTable = stateTable->routeTables[route];

        if (routeTable == NULL)
        {
            continue;
        }

        if (routeTable->routeType != RouteTypes[route])
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "route table %d has type %d but should be %d\n",
                         route,
                         routeTable->routeType,
                         RouteTypes[route]);
        }

        err = fmCustomTreeValidate(&routeTable->tcamRouteRouteTree);

        if (err != FM_OK)
        {
            FM_LOG_PRINT( "fm10000DbgValidateRouteTables Validation Failed: "
                          "sort-by-route route custom tree validation returned "
                          "error %d (%s)\n",
                         err,
                         fmErrorMsg(err) );
        }

        err = fmCustomTreeValidate(&routeTable->tcamSliceRouteTree);

        if (err != FM_OK)
        {
            FM_LOG_PRINT( "fm10000DbgValidateRouteTables Validation Failed: "
                          "sort-by-slice route custom tree validation returned "
                          "error %d (%s)\n",
                          err,
                          fmErrorMsg(err) );
        }

        tcamTreeRouteCount = fmCustomTreeSize(&routeTable->tcamRouteRouteTree);

        if ( (tcamTreeRouteCount < 0)
            || (tcamTreeRouteCount > switchPtr->maxRoutes) )
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "custom tree contains %d routes, max = %d\n",
                         tcamTreeRouteCount,
                         switchPtr->maxRoutes);
        }

        i = fmCustomTreeSize(&routeTable->tcamSliceRouteTree);

        if (i != tcamTreeRouteCount)
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "sort-by-route and sort-by-slice trees have "
                         "different sizes, sort-by-route=%d, "
                         "sort-by-slice=%d\n",
                         tcamTreeRouteCount,
                         i);
        }

        tcamTreeRouteCount = fmCustomTreeSize(&routeTable->tcamRouteRouteTree);

        if ( (tcamTreeRouteCount < 0)
            || (tcamTreeRouteCount > switchPtr->maxRoutes) )
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "custom tree contains %d routes, max = %d\n",
                         tcamTreeRouteCount,
                         switchPtr->maxRoutes);
        }

        i = fmCustomTreeSize(&routeTable->tcamSliceRouteTree);

        if (i != tcamTreeRouteCount)
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "sort-by-route and sort-by-slice trees have "
                         "different sizes, sort-by-route=%d, "
                         "sort-by-slice=%d\n",
                         tcamTreeRouteCount,
                         i);
        }

        fmCustomTreeIterInit(&iter, &routeTable->tcamRouteRouteTree);
        err = fmCustomTreeIterNext(&iter,
                                   (void **) &tcamRouteKey,
                                   (void **) &curTcamRoute);
        numTcamRoutes = 0;
        prevTcamRoute = NULL;

        if (err == FM_ERR_NO_MORE)
        {
            curTcamRoute = NULL;
        }

        while (curTcamRoute != NULL)
        {
            if (err != FM_OK)
            {
                FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                             "route %d:%d sort-by-route fmCustomTreeIterNext "
                             "err = %d (%s), curRoute = %p\n",
                             route,
                             numTcamRoutes,
                             err,
                             fmErrorMsg(err),
                             (void *) curTcamRoute);
            }

            if (curTcamRoute->routeSlice == NULL)
            {
                FM_LOG_PRINT( "fm10000DbgValidateRouteTables Validation Failed: "
                             "route %p (%d:%d) is not assigned to a TCAM slice\n",
                             (void *) curTcamRoute,
                             route,
                             numTcamRoutes );
            }
            else if ( (curTcamRoute->tcamSliceRow < 0)
                || (curTcamRoute->tcamSliceRow >= FM10000_FFU_ENTRIES_PER_SLICE) )
            {
                FM_LOG_PRINT( "fm10000DbgValidateRouteTables Validation Failed: "
                             "route %p (%d:%d) has invalid TCAM row %d in slice "
                             "%p (A:%d-%d)\n",
                             (void *) curTcamRoute,
                             route,
                             numTcamRoutes,
                             curTcamRoute->tcamSliceRow,
                             (void *) curTcamRoute->routeSlice,
                             curTcamRoute->routeSlice->firstTcamSlice,
                             curTcamRoute->routeSlice->lastTcamSlice );
            }
            else
            {
                err = fm10000GetFFURule(sw,
                                        &curTcamRoute->routeSlice->sliceInfo,
                                        curTcamRoute->tcamSliceRow,
                                        &ruleValid,
                                        ruleKey,
                                        ffuActionList,
                                        FALSE);

                if (err != FM_OK)
                {
                    FM_LOG_ERROR( FM_LOG_CAT_ROUTING,
                                 "Error reading route rule from TCAM, %d (%s)\n",
                                 err,
                                 fmErrorMsg(err) );
                }
                else
                {

#ifdef FM10000_DBG_TRACK_ROUTE_CONTENTS
                    if (ruleValid != curTcamRoute->valid)
                    {
                        FM_LOG_PRINT( "fm10000DbgValidateRouteTables Validation "
                                     "Failed: route %p (%d:%d) valid flag "
                                     "mismatch, read %d, expected %d\n",
                                     (void *) curTcamRoute,
                                     route,
                                     numTcamRoutes,
                                     ruleValid,
                                     curTcamRoute->valid );
                    }

                    for (i = 0 ; i < RouteSliceWidths[routeTable->routeType] ; i++)
                    {
                        if (ruleKey[i].key != curTcamRoute->camValue[i])
                        {
                            FM_LOG_PRINT( "fm10000DbgValidateRouteTables "
                                         "Validation Failed: route %p (%d:%d) "
                                         "hardware slice %d camValue is 0x%llx, "
                                         "expected camValue is 0x%llx\n",
                                         (void *) curTcamRoute,
                                         route,
                                         numTcamRoutes,
                                         curTcamRoute->routeSlice->firstTcamSlice + i,
                                         ruleKey[i].key,
                                         curTcamRoute->camValue[i] );
                        }

                        if (ruleKey[i].keyMask != curTcamRoute->camMask[i])
                        {
                            FM_LOG_PRINT( "fm10000DbgValidateRouteTables "
                                         "Validation Failed: route %p (%d:%d) "
                                         "hardware slice %d camMask is 0x%llx, "
                                         "expected camMask is 0x%llx\n",
                                         (void *) curTcamRoute,
                                         route,
                                         numTcamRoutes,
                                         curTcamRoute->routeSlice->firstTcamSlice + i,
                                         ruleKey[i].keyMask,
                                         curTcamRoute->camMask[i] );
                        }
                    }

                    if ( (ffuActionList[0].precedence != curTcamRoute->ffuAction.precedence)
                        || (ffuActionList[0].action != curTcamRoute->ffuAction.action) )
                    {
                        FM_LOG_PRINT( "fm10000DbgValidateRouteTables "
                                     "Validation Failed: route %p (%d:%d) "
                                     "hardware/expected ffuAction mismatch, "
                                     "precedence = %d/%d, action = %d/%d\n",
                                     (void *) curTcamRoute,
                                     route,
                                     numTcamRoutes,
                                     ffuActionList[0].precedence,
                                     curTcamRoute->ffuAction.precedence,
                                     ffuActionList[0].action,
                                     curTcamRoute->ffuAction.action );
                    }
#endif

                }
            }

            if (prevTcamRoute != NULL)
            {
                i = fmCompareRoutes(&prevTcamRoute->routePtr->route,
                                    &curTcamRoute->routePtr->route);

                if (i == 0)
                {
                    fmDbgBuildRouteDescription(&prevTcamRoute->routePtr->route,
                                               prevRouteDesc);
                    fmDbgBuildRouteDescription(&curTcamRoute->routePtr->route,
                                               curRouteDesc);
                    FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation "
                                 "Failed: route %d:%d, sort-by-route duplicate "
                                 "routes\n     previous route = %s\n"
                                 "    current route  = %s\n",
                                 route,
                                 numTcamRoutes,
                                 prevRouteDesc,
                                 curRouteDesc);
                }
                else if (i > 0)
                {
                    fmDbgBuildRouteDescription(&prevTcamRoute->routePtr->route,
                                               prevRouteDesc);
                    fmDbgBuildRouteDescription(&curTcamRoute->routePtr->route,
                                               curRouteDesc);
                    FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation "
                                 "Failed: route %d:%d, invalid sort-by-route "
                                 "sort order\n    previous route = %s\n"
                                 "    current route  = %s\n",
                                 route,
                                 numTcamRoutes,
                                 prevRouteDesc,
                                 curRouteDesc);
                }

                if (curTcamRoute->routePrefix->prefix
                    > prevTcamRoute->routePrefix->prefix)
                {
                    fmDbgBuildRouteDescription(&prevTcamRoute->routePtr->route,
                                               prevRouteDesc);
                    fmDbgBuildRouteDescription(&curTcamRoute->routePtr->route,
                                               curRouteDesc);
                    FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation "
                                 "Failed: route %d:%d, sort-by-route prefix "
                                 "%d after %d\n    previous route = %s\n"
                                 "    current route  = %s\n",
                                 route,
                                 numTcamRoutes,
                                 curTcamRoute->routePrefix->prefix,
                                 prevTcamRoute->routePrefix->prefix,
                                 prevRouteDesc,
                                 curRouteDesc);
                }
            }

            /* move to the next route */
            numTcamRoutes++;
            prevTcamRoute = curTcamRoute;
            err           = fmCustomTreeIterNext(&iter,
                                                 (void **) &tcamRouteKey,
                                                 (void **) &curTcamRoute);

            if (err == FM_ERR_NO_MORE)
            {
                curTcamRoute = NULL;
            }
        }

        if (err == FM_OK)
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "end of TCAM sort-by-route %d linked list with %d "
                         "routes, but custom tree contains %d routes\n",
                         route,
                         numTcamRoutes,
                         tcamTreeRouteCount);
        }

        fmCustomTreeIterInit(&iter, &routeTable->tcamSliceRouteTree);
        numTcamRoutes = 0;
        curTcamRoute  = routeTable->firstTcamRoute;
        err           = fmCustomTreeIterNext(&iter,
                                             (void **) &tcamRouteKey,
                                             (void **) &tcamRouteValue);
        prevTcamRoute = NULL;

        while (curTcamRoute != NULL)
        {
            if (numTcamRoutes > switchPtr->maxRoutes)
            {
                FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                             "too many routes in sort-by-slice table, "
                             "max = %d\n",
                             switchPtr->maxRoutes);
            }

            if (err != FM_OK)
            {
                FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                             "route %d:%d sort-by-slice fmCustomTreeIterNext "
                             "err = %d (%s), curRoute = %p\n",
                             route,
                             numTcamRoutes,
                             err,
                             fmErrorMsg(err),
                             (void *) curTcamRoute);
            }

            if ( (curTcamRoute != tcamRouteKey)
                || (curTcamRoute != tcamRouteValue) )
            {
                FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                             "route %d:%d, sort-by-slice linked list = %p, "
                             "custom tree key = %p value = %p\n",
                             route,
                             numTcamRoutes,
                             (void *) curTcamRoute,
                             (void *) tcamRouteKey,
                             (void *) tcamRouteValue);
            }

            if (prevTcamRoute != NULL)
            {
                i = prevTcamRoute->routeSlice->firstTcamSlice
                    - curTcamRoute->routeSlice->firstTcamSlice;

                if (i < 0)
                {
                }
                else if (i == 0)
                {
                    i = prevTcamRoute->tcamSliceRow
                        - curTcamRoute->tcamSliceRow;

                    if (i < 0)
                    {
                    }
                    else if (i == 0)
                    {
                        fmDbgBuildRouteDescription(&prevTcamRoute->routePtr->route,
                                                   prevRouteDesc);
                        fmDbgBuildRouteDescription(&curTcamRoute->routePtr->route,
                                                   curRouteDesc);
                        FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation "
                                     "Failed: route %d:%d, duplicate routes\n"
                                     "    previous route = %s\n"
                                     "    current route  = %s\n",
                                     route,
                                     numTcamRoutes,
                                     prevRouteDesc,
                                     curRouteDesc);
                    }
                }

                if (curTcamRoute->routePrefix->prefix
                    > prevTcamRoute->routePrefix->prefix)
                {
                    fmDbgBuildRouteDescription(&prevTcamRoute->routePtr->route,
                                               prevRouteDesc);
                    fmDbgBuildRouteDescription(&curTcamRoute->routePtr->route,
                                               curRouteDesc);
                    FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation "
                                 "Failed: route %d:%d, sort-by-slice prefix "
                                 "%d after %d\n    previous route = %s\n"
                                 "    current route  = %s\n",
                                 route,
                                 numTcamRoutes,
                                 curTcamRoute->routePrefix->prefix,
                                 prevTcamRoute->routePrefix->prefix,
                                 prevRouteDesc,
                                 curRouteDesc);
                }
            }

            /* move to the next route */
            numTcamRoutes++;
            prevTcamRoute = curTcamRoute;
            curTcamRoute  = curTcamRoute->nextTcamRoute;
            err           = fmCustomTreeIterNext(&iter,
                                                 (void **) &tcamRouteKey,
                                                 (void **) &tcamRouteValue);

            if (err == FM_ERR_NO_MORE)
            {
                tcamRouteValue = NULL;
            }
        }

        if (err == FM_OK)
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "end of TCAM route %d linked list with %d routes, but "
                         "custom tree contains %d routes\n",
                         route,
                         numTcamRoutes,
                         tcamTreeRouteCount);
        }

        if (numTcamRoutes != tcamTreeRouteCount)
        {
            FM_LOG_PRINT("fm10000DbgValidateRouteTables Validation Failed: "
                         "num TCAM routes found via linked list = %d,\n"
                         "but tree count = %d\n",
                         numTcamRoutes,
                         tcamTreeRouteCount);
        }

        err = FM_OK;

    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000DbgValidateRouteTables */



/*****************************************************************************/
/** fm10000SetRouteAttribute
 * \ingroup intRouterRoute
 *
 * \desc            Sets an attribute for a specific route.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       route points to the route entry.
 *
 * \param[in]       attr contains the attribute ID.
 *
 * \param[in]       value points to the attribute value to be used.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ATTRIB if attr is invalid.
 * \return          FM_ERR_INVALID_ARGUMENT if value is invalid.
 *
 *****************************************************************************/
fm_status fm10000SetRouteAttribute(fm_int            sw,
                                  fm_intRouteEntry *route,
                                  fm_int            attr,
                                  void *            value)
{
    fm_status               err;
    fm10000_RoutingTable *  routeTable;
    fm10000_TcamRouteEntry  tempRoute;
    fm10000_TcamRouteEntry *tcamRoute;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, route=%p, attr=%d, value=%p\n",
                 sw,
                 (void *) route,
                 attr,
                 value);

    FM_NOT_USED(value);

    routeTable = GetRouteTableForRoute(sw, &route->route);

    if (routeTable == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* Try to find the route in the table */
    FM_CLEAR(tempRoute);
    tempRoute.routePtr = route;
    err = fmCustomTreeFind( &routeTable->tcamRouteRouteTree,
                            &tempRoute,
                            (void **) &tcamRoute );

    if (err != FM_OK)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
    }

    switch (attr)
    {
        case FM_ROUTE_FWD_TO_CPU:
            err = FM_ERR_UNSUPPORTED;
            break;

        default:
            err  = FM_ERR_INVALID_ATTRIB;
            break;
    } /* end switch (attr) */

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000SetRouteAttribute */


/*****************************************************************************/
/** fm10000GetRouteAttribute
 * \ingroup intRouterRoute
 *
 * \desc            Gets an attribute for a specific route.
 *
 * \param[in]       sw is the switch number.
 *
 * \param[in]       pRoute points to the internal route entry.
 *
 * \param[in]       attr contains the attribute ID.
 *
 * \param[in]       pValue points to caller-allocated storage into which the
 *                  function will store the attribute's value.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetRouteAttribute(fm_int         sw,
                                   fm_routeEntry *pRoute,
                                   fm_int         attr,
                                   void *         pValue)
{
    
    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d, pRoute=%p, attr=%d, pValue=%p\n",
                 sw,
                 (void*)pRoute,
                 attr,
                 pValue);

    FM_NOT_USED(sw);
    FM_NOT_USED(pRoute);
    FM_NOT_USED(attr);
    FM_NOT_USED(pValue);

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_UNSUPPORTED);

}   /* end fm10000GetRouteAttribute */


/*****************************************************************************/
/** fm10000GetTcamRouteEntry
 * \ingroup intRoute
 *
 * \desc            Returns the TCAM route structures given a ''fm_routeEntry''
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       pEntry points to the ''fm_routeEntry'' whose TCAM entry
 *                  is to be retrieved.
 *
 * \param[out]      ppTcamEntry points to caller allocated storage where the
 *                  internal TCAM route structure is stored.
 *
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000GetTcamRouteEntry(fm_int                   sw,
                                   fm_routeEntry *          pEntry,
                                   fm10000_TcamRouteEntry **ppTcamEntry)
{
    fm_status               err;
    fm_intRouteEntry        pNewRoute;
    fm10000_RoutingTable *  pRouteTable;
    fm10000_TcamRouteEntry  routeKey;
    fm10000_TcamRouteEntry *pRouteEntry;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw=%d pEntry=%p ppTcamEntry=%p\n",
                 sw,
                 (void*)pEntry,
                 (void*)ppTcamEntry);

    /* argument validation */
    if (pEntry == NULL ||
        ppTcamEntry == NULL)
    {
        err = FM_ERR_INVALID_ARGUMENT;
    }
    else
    {
        pRouteTable = GetRouteTableForRoute(sw, pEntry);

        if (pRouteTable == NULL)
        {
            err = FM_ERR_NOT_FOUND;
        }
        else
        {
            /* See if the route already exists */
            FM_CLEAR(routeKey);
            pNewRoute.route    = *pEntry;
            routeKey.routePtr = &pNewRoute;
            err = fmCustomTreeFind( &pRouteTable->tcamRouteRouteTree,
                                    &routeKey,
                                    (void **) &pRouteEntry );

            if ( (err == FM_OK) && (pRouteEntry != NULL) )
            {
                *ppTcamEntry = pRouteEntry;
            }
            else
            {
                err = FM_ERR_NOT_FOUND;
                *ppTcamEntry = NULL;
            }
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end fm10000GetTcamRouteEntry */


/*****************************************************************************/
/** fm10000RoutingProcessFFUPartitionChange
 * \ingroup intRoute
 *
 * \desc            Attempts to update the routing tables based upon a change
 *                  to the FFU slices available to routing.
 *                  If additional slices are simply being made available,
 *                  without changing any of the slices already utilized by
 *                  routing, or if the slices being removed from routing use
 *                  are not currently in use, the function simply updates the
 *                  TCAM slice structures to support the new partitioning and
 *                  returns successfully.
 *                  If the simulated flag is set, the function clones the
 *                  existing routing state and simulates the route
 *                  manipulations that would be required in order to support
 *                  the new partitions without affecting any traffic.
 *                  If the simulated flag is not set, the function executes
 *                  the partition change against the actual live routing
 *                  system.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[in]       newAllocations points to the record containing the new
 *                  FFU slice assignments.
 *
 * \param[in]       simulated specifies whether to simulate the operation or
 *                  perform it for real.  TRUE means simulate, FALSE means
 *                  real.
 *
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if the newAllocations structure
 *                  describes an invalid allocation scheme.
 * \return          FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE if the new
 *                  allocation would not provide sufficient space for the
 *                  already-existing multicast routes.
 * \return          FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE if the new
 *                  allocation would not provide sufficient space for the
 *                  already-existing unicast routes.
 *
 *****************************************************************************/
fm_status fm10000RoutingProcessFFUPartitionChange(fm_int                  sw,
                                                  fm_ffuSliceAllocations *newAllocations,
                                                  fm_bool                 simulated)
{
    fm_status               err;
    fm10000_switch *        switchExt;
    fm_int                  index;
    fm_int                  tcamSlice;
    fm10000_RouteTcamSlice *tcamSlicePtr;
    fm10000_RouteSlice *    routeSlicePtr;
    fm10000_RoutingState *  routeState;
    fm10000_RoutingState *  cloneState;
    fm_bool                 rebuildNeeded = FALSE;
    fm_bool                 foundIPv4UnicastRoutes = FALSE;
    fm_bool                 foundIPv4MulticastRoutes = FALSE;
    fm_bool                 foundIPv6UnicastRoutes = FALSE;
    fm_bool                 foundIPv6MulticastRoutes = FALSE;
    fm10000_RouteTypes      caseVal;
    fm_int                  sliceRouteTypeCount;
    fm_int                  caseNum;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, newAllocations = %p, simulated = %d\n",
                 sw,
                 (void *) newAllocations,
                 simulated);

    err = FM_OK;

    rebuildNeeded = FALSE;
    foundIPv4MulticastRoutes = FALSE;
    foundIPv6UnicastRoutes = FALSE;
    foundIPv6MulticastRoutes = FALSE;

    switchExt = GET_SWITCH_EXT(sw);

    /* First validate the new slice allocations.  Enforce the restriction
     * that only 2 route types may be used per TCAM slice. */
    for (tcamSlice = 0 ; tcamSlice < FM10000_MAX_FFU_SLICES ; tcamSlice++)
    {
        sliceRouteTypeCount = 0;

        if ( (tcamSlice >= newAllocations->ipv4UnicastFirstSlice)
            && (tcamSlice <= newAllocations->ipv4UnicastLastSlice) )
        {
            sliceRouteTypeCount++;
        }

        if ( (tcamSlice >= newAllocations->ipv4MulticastFirstSlice)
            && (tcamSlice <= newAllocations->ipv4MulticastLastSlice) )
        {
            sliceRouteTypeCount++;
        }

        if ( (tcamSlice >= newAllocations->ipv6UnicastFirstSlice)
            && (tcamSlice <= newAllocations->ipv6UnicastLastSlice) )
        {
            sliceRouteTypeCount++;
        }

        if ( (tcamSlice >= newAllocations->ipv6MulticastFirstSlice)
            && (tcamSlice <= newAllocations->ipv6MulticastLastSlice) )
        {
            sliceRouteTypeCount++;
        }

        if (sliceRouteTypeCount > FM10000_ROUTE_NUM_CASES)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
        }
    }

    routeState = &switchExt->routeStateTable;

    /* First check slice usage to see if this change will have any impact */
    for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++)
    {
        tcamSlicePtr = &routeState->routeTcamSliceArray[index];

        if (tcamSlicePtr->inUse)
        {
            /* If this slice is in use and the new allocation doesn't allow
             * the current usage (unicast/multicast), route rebuilding will
             * be required. */
            for (caseNum = 0 ; caseNum < FM10000_ROUTE_NUM_CASES ; caseNum++)
            {
                caseVal       = tcamSlicePtr->caseInfo[caseNum].routeType;
                routeSlicePtr = tcamSlicePtr->caseInfo[caseNum].routeSlice;

                switch (caseVal)
                {
                    case FM10000_ROUTE_TYPE_UNUSED:
                        break;
                        
                    case FM10000_ROUTE_TYPE_V4U:
                        if (routeSlicePtr->highestRow >= 0)
                        {
                            foundIPv4UnicastRoutes = TRUE;
                        }

                        if ( (index < newAllocations->ipv4UnicastFirstSlice)
                             || (index > newAllocations->ipv4UnicastFirstSlice) )
                        {
                            rebuildNeeded = TRUE;
                        }
                        
                        break;
                        
                    case FM10000_ROUTE_TYPE_V6U:
                        if (routeSlicePtr->highestRow >= 0)
                        {
                            foundIPv6UnicastRoutes = TRUE;
                        }
                        
                        if ( (index < newAllocations->ipv6UnicastFirstSlice)
                             || (index > newAllocations->ipv6UnicastFirstSlice) )
                        {
                            rebuildNeeded = TRUE;
                        }
                        
                        break;

#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
                    case FM10000_ROUTE_TYPE_V4SG:
                    case FM10000_ROUTE_TYPE_V4DV:
#endif
                    case FM10000_ROUTE_TYPE_V4DSV:
                        if (routeSlicePtr->highestRow >= 0)
                        {
                            foundIPv4MulticastRoutes = TRUE;
                        }
                        
                        if ( (index < newAllocations->ipv4MulticastFirstSlice)
                             || (index > newAllocations->ipv4MulticastFirstSlice) )
                        {
                            rebuildNeeded = TRUE;
                        }
                        
                        break;
                        
#ifdef ENABLE_DIVERSE_MCAST_SUPPORT
                    case FM10000_ROUTE_TYPE_V6SG:
                    case FM10000_ROUTE_TYPE_V6DV:
#endif
                    case FM10000_ROUTE_TYPE_V6DSV:
                        if (routeSlicePtr->highestRow >= 0)
                        {
                            foundIPv6MulticastRoutes = TRUE;
                        }
                        
                        if ( (index < newAllocations->ipv6MulticastFirstSlice)
                             || (index > newAllocations->ipv6MulticastFirstSlice) )
                        {
                            rebuildNeeded = TRUE;
                        }
                        
                        break;

                    default:
                        break;
                } /* ( switch (caseVal) */
                         } /* for (caseNum = 0 ; caseNum < FM10000_ROUTE_NUM_CASES ; caseNum++) */

        }   /* if (tcamSlicePtr->inUse) */

    }   /* for (index = 0 ; index < FM10000_MAX_FFU_SLICES ; index++) */

    if ( foundIPv4UnicastRoutes && (newAllocations->ipv4UnicastFirstSlice < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE);
    }

    if ( foundIPv6UnicastRoutes && (newAllocations->ipv6UnicastFirstSlice < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INSUFFICIENT_UNICAST_ROUTE_SPACE);
    }

    if ( foundIPv4MulticastRoutes && (newAllocations->ipv4MulticastFirstSlice < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE);
    }

    if ( foundIPv6MulticastRoutes && (newAllocations->ipv6MulticastFirstSlice < 0) )
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INSUFFICIENT_MULTICAST_ROUTE_SPACE);
    }

    if (!rebuildNeeded)
    {
        if (!simulated)
        {
            /* Slices aren't in use, or new use is same as old, so just
             * update the slice information and return. */
            err = SetFFUSliceUsageForRoutingState(sw, newAllocations, routeState);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

            routeState->previousFirstTcamSlice = routeState->routeFirstTcamSlice;
            routeState->previousLastTcamSlice  = routeState->routeLastTcamSlice;

            /* Pre-allocate TCAM slices for each route type */
            err = PreallocateRouteSlices(sw, NULL, FALSE);
            FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);
        }

        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);
    }

    if (simulated)
    {
        /* Create a clone of the routing state tables */
        err = CloneRoutingState(sw,
                                routeState,
                                &cloneState);
        if (err != FM_OK)
        {
            FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);
        }
    }
    else
    {
        /* Do it for real. */
        cloneState = routeState;
    }

    /* Save previous TCAM boundaries for temporary use. */
    cloneState->previousFirstTcamSlice = routeState->routeFirstTcamSlice;
    cloneState->previousLastTcamSlice  = routeState->routeLastTcamSlice;
    cloneState->tempSlicesAvailable    = TRUE;

    /* Change the slice usage rules */
    err = SetFFUSliceUsageForRoutingState(sw, newAllocations, cloneState);

    /* Pre-allocate TCAM slices for each route type */
    if (err == FM_OK)
    {
        err = PreallocateRouteSlices(sw, cloneState, TRUE);
    }

    /* Execute/Simulate the required operations */
    if (err == FM_OK)
    {
        err = MoveAllUnauthorizedRoutes(sw, cloneState);
    }


    if (err == FM_OK)
    {
        /* After consolidation is complete, try to pre-allocate one last time */
        err = PreallocateRouteSlices(sw, cloneState, FALSE);
    }

    if (simulated)
    {
        /* Delete the clone */
        ReleaseRoutingState(sw, cloneState);
    }
    else
    {
        /* Reset previous TCAM boundaries to current TCAM boundaries */
        routeState->previousFirstTcamSlice = routeState->routeFirstTcamSlice;
        routeState->previousLastTcamSlice  = routeState->routeLastTcamSlice;
        routeState->tempSlicesAvailable    = FALSE;
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

}   /* end fm10000RoutingProcessFFUPartitionChange */




/*****************************************************************************/
/** fm10000RoutingProcessArpRedirect
 * \ingroup intRoute
 *
 * \desc            Analyze if ARP redirect should be passed to application.
 *                  Block ARP redirecting from this moment.
 *
 * \param[in]       sw is the switch number to operate on.
 *
 * \param[out]      plogArpRedirect points to a caller allocated boolean
 *                  variable. It indicates if frame can be send
 *                  to the application.
 *
 * \return          FM_OK if successful.
 * \return          FM_ERR_INVALID_ARGUMENT if plogArpRedirect is NULL.
 *
 *****************************************************************************/
fm_status fm10000RoutingProcessArpRedirect(fm_int sw, fm_bool *plogArpRedirect)
{
    fm_status       err;
    fm10000_switch *switchExt;
    fm_switch      *switchPtr;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING,
                 "sw = %d, plogArpRedirect = %p\n",
                 sw,
                 (void*) plogArpRedirect);

    switchPtr = GET_SWITCH_PTR(sw);
    switchExt = GET_SWITCH_EXT(sw);

    err = FM_OK;

    if (plogArpRedirect == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    *plogArpRedirect = FALSE;

    err = fmCaptureWriteLock(&switchPtr->routingLock, FM_WAIT_FOREVER);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    if (switchPtr->routerTrapRedirectEvent)
    {
        switchPtr->routerTrapRedirectEvent = FALSE;

        err = SetRouterArpRedirectTrigger(sw);
        FM_LOG_ABORT_ON_ERR(FM_LOG_CAT_ROUTING, err);

        *plogArpRedirect = TRUE;
    }

ABORT:
    fmReleaseWriteLock(&switchPtr->routingLock);
    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, err);

} /* end fm10000RoutingProcessArpRedirect */




/*****************************************************************************/
/** fm10000SetRouteAction
 * \ingroup intRouterRoute
 *
 * \desc            Set the routing action for a route.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \param[in]       route points the route whose action is to be set.
 *
 * \return          FM_OK if successful.
 *
 *****************************************************************************/
fm_status fm10000SetRouteAction(fm_int            sw,
                                fm_intRouteEntry *route)
{
    fm_status               err;
    fm_switch *             switchPtr;
    fm10000_RouteInfo       routeInfo;
    fm10000_RoutingTable *  routeTable;
    fm10000_TcamRouteEntry  tcamRouteKey;
    fm10000_TcamRouteEntry *tcamRoute;
    fm10000_RouteSlice *    destSlicePtr;
    fm_int                  destRow;
    fm_fm10000FfuSliceKey   ruleKey[FM10000_MAX_ROUTE_SLICE_WIDTH];
    fm_ffuAction            ffuAction;
    fm_bool                 valid;
    fm_bool                 foundRow;

    FM_LOG_ENTRY(FM_LOG_CAT_ROUTING, "sw=%d, route=%p\n", sw, (void *) route);

    switchPtr = GET_SWITCH_PTR(sw);

    if (route == NULL)
    {
        FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_ERR_INVALID_ARGUMENT);
    }

    /* Consolidate route information into the routeInfo structure. */
    ClassifyRoute(sw, route, &routeInfo);

    routeTable = routeInfo.routeTable;

    /* Try to find the TCAM route in the table */
    FM_CLEAR(tcamRouteKey);
    tcamRouteKey.routePtr = route;

    err = fmCustomTreeFind(&routeTable->tcamRouteRouteTree,
                           &tcamRouteKey,
                           (void **) &tcamRoute);

    /* Initialize the action structure */
    err = InitFfuRouteAction(sw, route, &routeInfo, &ffuAction);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Find an available FFU rule for this route. */
    err = FindFfuEntryForNewRoute(sw,
                                  route,
                                  &routeInfo,
                                  &destSlicePtr,
                                  &destRow);
    if (err == FM_OK)
    {
        foundRow = TRUE;
    }
    else
    {
        foundRow     = FALSE;
        err          = FM_OK;
        destSlicePtr = tcamRoute->routeSlice;
        destRow      = tcamRoute->tcamSliceRow;
    }

    /* set the FFU key & mask */
    FM_CLEAR(ruleKey);

    /* Build the rule key */
    err = SetFFuRuleKeyForRoute(sw, &routeInfo, &ruleKey[0]);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Build the rule action */
    err = SetFfuRouteAction(sw, route, &routeInfo, &ffuAction, &valid);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Write the rule to the hardware */
    err = fm10000SetFFURule(sw,
                            &destSlicePtr->sliceInfo,
                            destRow,
                            valid,
                            ruleKey,
                            &ffuAction,
                            TRUE,
                            TRUE);
    FM_LOG_EXIT_ON_ERR(FM_LOG_CAT_ROUTING, err);

    /* Store the final FFU action structure into the route */
    FM_MEMCPY_S( &tcamRoute->ffuAction,
                 sizeof(tcamRoute->ffuAction),
                 &ffuAction,
                 sizeof(ffuAction) );

    if (foundRow)
    {
        if ( !UpdateTcamRoutePosition(sw,
                                      tcamRoute,
                                      destSlicePtr,
                                      destRow,
                                      TRUE) )
        {
            FM_LOG_ERROR(FM_LOG_CAT_ROUTING,
                         "Cannot update TCAM route position\n");
            err = FM_FAIL;
        }
        else
        {
            tcamRoute->dirty = FALSE;
        }
    }

    FM_LOG_EXIT(FM_LOG_CAT_ROUTING, FM_OK);

}   /* end fm10000SetRouteAction */

