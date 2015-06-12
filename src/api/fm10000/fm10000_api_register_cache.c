/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:            fm10000_api_register_cache.c
 * Creation Date:   April 19, 2013
 * Description:     File containing utilities and data structures to
 *                  manage the Register Cache.                             
 *
 * Copyright (c) 2013 - 2015, Intel Corporation.
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

#if (defined(FV_CODE) || defined(FAST_API_BOOT))
#define FAST_CACHE
#endif

#if defined(FAST_CACHE)
#define CACHE_METHODS(data, valid, defaults) \
        { data, valid, defaults }
#else
#define CACHE_METHODS(data, valid, defaults) \
        { data, valid, NULL }
#endif

/*****************************************************************************
 * Macros, Constants & Types
 *****************************************************************************/

/*****************************************************************************
 * Local function prototypes
 *****************************************************************************/

static fm_uint32   *fm10000GetCacheFfuMapSrc(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapMac(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapVlan(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapType(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapLength(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapIpLo(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapIpHi(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapIpCfg(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapProt(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapL4Src(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMapL4Dst(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuEgressChunkCfg(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuEgressChunkValid(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuEgressChunkActions(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuEgressPortCfg(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuSliceTcam(fm_int sw);
static fm_bitArray *fm10000GetCacheFfuSliceTcamValidKey(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuSliceSram(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuSliceValid(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuSliceCascadeAction(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuSliceCfg(fm_int sw);
static fm_uint32   *fm10000GetCacheFfuMasterValid(fm_int sw);

static fm_uint32   *fm10000GetCachePolicerApplyCfg4k(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerApplyCfg512(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerDscpDownMap(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerSwpriDownMap(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerCfg4k(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerCfg512(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerState4k(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerState512(fm_int sw);
static fm_uint32   *fm10000GetCachePolicerCfg(fm_int sw);

static fm_uint32   *fm10000GetCacheEgressMstTable(fm_int sw);
static fm_uint32   *fm10000GetCacheEgressVidTable(fm_int sw);
static fm_uint32   *fm10000GetCacheGlortCam(fm_int sw);
static fm_uint32   *fm10000GetCacheGlortRam(fm_int sw);
static fm_uint32   *fm10000GetCacheIngressMstTable(fm_int sw);
static fm_uint32   *fm10000GetCacheIngressVidTable(fm_int sw);
static fm_uint32   *fm10000GetCacheModMirrorProfTable(fm_int sw);
static fm_uint32   *fm10000GetCacheModPerPortCfg1(fm_int sw);
static fm_uint32   *fm10000GetCacheModPerPortCfg2(fm_int sw);
static fm_uint32   *fm10000GetCacheModRxVpriMap(fm_int sw);
static fm_uint32   *fm10000GetCacheModVlanTagVid1Map(fm_int sw);
static fm_uint32   *fm10000GetCacheModVpri1Map(fm_int sw);
static fm_uint32   *fm10000GetCacheModVpri2Map(fm_int sw);
static fm_uint32   *fm10000GetCacheParserPortCfg2(fm_int sw);
static fm_uint32   *fm10000GetCacheParserPortCfg3(fm_int sw);
static fm_uint32   *fm10000GetCacheRxVpriMap(fm_int sw);
static fm_uint32   *fm10000GetCacheSafMatrix(fm_int sw);

static fm_uint32   *fm10000GetCacheTeData(fm_int sw);
static fm_uint32   *fm10000GetCacheTeLookup(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDglortMap(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDglortDec(fm_int sw);
static fm_uint32   *fm10000GetCacheTeSglortMap(fm_int sw);
static fm_uint32   *fm10000GetCacheTeSglortDec(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDefaultDglort(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDefaultSglort(fm_int sw);
static fm_uint32   *fm10000GetCacheTeSip(fm_int sw);
static fm_uint32   *fm10000GetCacheTeVni(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDefaultL4Dst(fm_int sw);
static fm_uint32   *fm10000GetCacheTeCfg(fm_int sw);
static fm_uint32   *fm10000GetCacheTePorts(fm_int sw);
static fm_uint32   *fm10000GetCacheTeTunHeaderCfg(fm_int sw);
static fm_uint32   *fm10000GetCacheTeTrapDglort(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDefaultNgeData(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDefaultNgeMask(fm_int sw);
static fm_uint32   *fm10000GetCacheTeExvet(fm_int sw);
static fm_uint32   *fm10000GetCacheTeDmac(fm_int sw);
static fm_uint32   *fm10000GetCacheTeSmac(fm_int sw);
static fm_uint32   *fm10000GetCacheTeTrapCfg(fm_int sw);

#ifdef FAST_CACHE

/* Defaults for simulation purposes 
 * It takes several days to read all of the registers in simulation,
 * so hard code defaults instead.  Actual hardware will read registers. */

/* FFU_SLICE_TCAM does not mem init to a register default */
static fm_uint32    fm10000GetCacheFfuSliceTcamDefault(fm_uint32 addr);

/* Returns 32 bits of 0 for registers that should default to 0. */
static fm_uint32    fm10000GetCacheRegisterDefaultZero(fm_uint32 addr);

/* Returns 32 bits of 1 for registers that should default to 0xffffffff */
static fm_uint32    fm10000GetCacheRegisterDefaultFull(fm_uint32 addr);

/* Default is not 0x00000000 or 0xffffffff */
static fm_uint32    fm10000GetCacheTeLookupDefault(fm_uint32 addr);
static fm_uint32    fm10000GetCacheTeGlortMapDefault(fm_uint32 addr);

#endif


/*****************************************************************************
 * Global Variables
 *****************************************************************************/

/* Description of each individual register  */
/* belonging to the cacheable register list */

/**************************************************
 * FFU registers
 **************************************************/

/* FFU_MAP_SRC register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapSrc =
{
    CACHE_METHODS(fm10000GetCacheFfuMapSrc,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_SRC(0),
    FM10000_FFU_MAP_SRC_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_SRC(1) - FM10000_FFU_MAP_SRC(0) },
    { FM10000_FFU_MAP_SRC_ENTRIES }
};

/* FFU_MAP_MAC register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapMac =
{
    CACHE_METHODS(fm10000GetCacheFfuMapMac,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_MAC(0, 0),
    FM10000_FFU_MAP_MAC_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_MAC(1,0) - FM10000_FFU_MAP_MAC(0, 0)   },
    { FM10000_FFU_MAP_MAC_ENTRIES }
};

/* FFU_MAP_VLAN register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapVlan =
{
    CACHE_METHODS(fm10000GetCacheFfuMapVlan,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_VLAN(0),
    FM10000_FFU_MAP_VLAN_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_VLAN(1) - FM10000_FFU_MAP_VLAN(0) },
    { FM10000_FFU_MAP_VLAN_ENTRIES }
};

/* FFU_MAP_TYPE register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapType =
{
    CACHE_METHODS(fm10000GetCacheFfuMapType,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_TYPE(0),
    FM10000_FFU_MAP_TYPE_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_TYPE(1) - FM10000_FFU_MAP_TYPE(0) },
    { FM10000_FFU_MAP_TYPE_ENTRIES }
};

/* FFU_MAP_LENGTH register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapLength =
{
    CACHE_METHODS(fm10000GetCacheFfuMapLength,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_LENGTH(0),
    FM10000_FFU_MAP_LENGTH_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_LENGTH(1) - FM10000_FFU_MAP_LENGTH(0) },
    { FM10000_FFU_MAP_LENGTH_ENTRIES }
};

/* FFU_MAP_IP_LO register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapIpLo =
{
    CACHE_METHODS(fm10000GetCacheFfuMapIpLo,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_IP_LO(0, 0),
    FM10000_FFU_MAP_IP_LO_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    FALSE,
    { FM10000_FFU_MAP_IP_LO(1,0) - FM10000_FFU_MAP_IP_LO(0, 0)   },
    { FM10000_FFU_MAP_IP_LO_ENTRIES }
};

/* FFU_MAP_IP_HI register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapIpHi =
{
    CACHE_METHODS(fm10000GetCacheFfuMapIpHi,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_IP_HI(0, 0),
    FM10000_FFU_MAP_IP_HI_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    FALSE,
    { FM10000_FFU_MAP_IP_HI(1,0) - FM10000_FFU_MAP_IP_HI(0, 0)   },
    { FM10000_FFU_MAP_IP_HI_ENTRIES }
};

/* FFU_MAP_IP_CFG register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapIpCfg =
{
    CACHE_METHODS(fm10000GetCacheFfuMapIpCfg,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_IP_CFG(0),
    FM10000_FFU_MAP_IP_CFG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    FALSE,
    { FM10000_FFU_MAP_IP_CFG(1) - FM10000_FFU_MAP_IP_CFG(0) },
    { FM10000_FFU_MAP_IP_CFG_ENTRIES }
};

/* FFU_MAP_PROT register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapProt =
{
    CACHE_METHODS(fm10000GetCacheFfuMapProt,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_PROT(0),
    FM10000_FFU_MAP_PROT_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_PROT(1) - FM10000_FFU_MAP_PROT(0) },
    { FM10000_FFU_MAP_PROT_ENTRIES }
};

/* FFU_MAP_L4_SRC register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapL4Src =
{
    CACHE_METHODS(fm10000GetCacheFfuMapL4Src,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_L4_SRC(0, 0),
    FM10000_FFU_MAP_L4_SRC_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_L4_SRC(1,0) - FM10000_FFU_MAP_L4_SRC(0, 0)   },
    { FM10000_FFU_MAP_L4_SRC_ENTRIES }
};

/* FFU_MAP_L4_DST register set descriptor */
const fm_cachedRegs fm10000CacheFfuMapL4Dst =
{
    CACHE_METHODS(fm10000GetCacheFfuMapL4Dst,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_MAP_L4_DST(0, 0),
    FM10000_FFU_MAP_L4_DST_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_MAP_L4_DST(1,0) - FM10000_FFU_MAP_L4_DST(0, 0)   },
    { FM10000_FFU_MAP_L4_DST_ENTRIES }
};

/* FFU_EGRESS_CHUNK_CFG register set descriptor */
const fm_cachedRegs fm10000CacheFfuEgressChunkCfg =
{
    CACHE_METHODS(fm10000GetCacheFfuEgressChunkCfg,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_EGRESS_CHUNK_CFG(0),
    FM10000_FFU_EGRESS_CHUNK_CFG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_EGRESS_CHUNK_CFG(1) - FM10000_FFU_EGRESS_CHUNK_CFG(0) },
    { FM10000_FFU_EGRESS_CHUNK_CFG_ENTRIES }
};

/* FFU_EGRESS_CHUNK_VALID register set descriptor */
const fm_cachedRegs fm10000CacheFfuEgressChunkValid =
{
    CACHE_METHODS(fm10000GetCacheFfuEgressChunkValid,
                  NULL,
                  fm10000GetCacheRegisterDefaultFull),
    FM10000_FFU_EGRESS_CHUNK_VALID(0),
    FM10000_FFU_EGRESS_CHUNK_VALID_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_EGRESS_CHUNK_VALID(1) - FM10000_FFU_EGRESS_CHUNK_VALID(0) },
    { FM10000_FFU_EGRESS_CHUNK_VALID_ENTRIES }
};

/* FFU_EGRESS_CHUNK_ACTIONS register set descriptor */
const fm_cachedRegs fm10000CacheFfuEgressChunkActions =
{
    CACHE_METHODS(fm10000GetCacheFfuEgressChunkActions,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_EGRESS_CHUNK_ACTIONS(0),
    FM10000_FFU_EGRESS_CHUNK_ACTIONS_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_EGRESS_CHUNK_ACTIONS(1) - FM10000_FFU_EGRESS_CHUNK_ACTIONS(0) },
    { FM10000_FFU_EGRESS_CHUNK_ACTIONS_ENTRIES }
};

/* FFU_EGRESS_PORT_CFG register set descriptor */
const fm_cachedRegs fm10000CacheFfuEgressPortCfg =
{
    CACHE_METHODS(fm10000GetCacheFfuEgressPortCfg,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_EGRESS_PORT_CFG(0),
    FM10000_FFU_EGRESS_PORT_CFG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_EGRESS_PORT_CFG(1) - FM10000_FFU_EGRESS_PORT_CFG(0) },
    { FM10000_FFU_EGRESS_PORT_CFG_ENTRIES }
};

/* FFU_SLICE_TCAM register set descriptor */
const fm_cachedRegs fm10000CacheFfuSliceTcam =
{
    /* DV simulations cannot read the FFU TCAM without writing it first or
     * the X values will cause failures.  BIST memory initialization does not
     * initialize the FFU TCAM.  Switch initialization attempts to initialize
     * the register cache to register defaults.  In simulation this is a
     * problem, but in hardware the values may not be valid (random) but
     * it should be able to cache it. */
    CACHE_METHODS(fm10000GetCacheFfuSliceTcam,
                  fm10000GetCacheFfuSliceTcamValidKey,
                  fm10000GetCacheFfuSliceTcamDefault),
    FM10000_FFU_SLICE_TCAM(0, 0, 0),
    FM10000_FFU_SLICE_TCAM_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_FFU_SLICE_TCAM(0, 1, 0) - FM10000_FFU_SLICE_TCAM(0, 0, 0),
      FM10000_FFU_SLICE_TCAM(1, 0, 0) - FM10000_FFU_SLICE_TCAM(0, 0, 0) },
    { FM10000_FFU_SLICE_TCAM_ENTRIES_0, FM10000_FFU_SLICE_TCAM_ENTRIES_1 }
};

/* FFU_SLICE_SRAM register set descriptor */
const fm_cachedRegs fm10000CacheFfuSliceSram =
{
    CACHE_METHODS(fm10000GetCacheFfuSliceSram,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_SLICE_SRAM(0, 0, 0),
    FM10000_FFU_SLICE_SRAM_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_FFU_SLICE_SRAM(0, 1, 0) - FM10000_FFU_SLICE_SRAM(0, 0, 0),
      FM10000_FFU_SLICE_SRAM(1, 0, 0) - FM10000_FFU_SLICE_SRAM(0, 0, 0) },
    { FM10000_FFU_SLICE_SRAM_ENTRIES_0, FM10000_FFU_SLICE_SRAM_ENTRIES_1 }
};

/* FFU_SLICE_VALID register set descriptor */
const fm_cachedRegs fm10000CacheFfuSliceValid =
{
    CACHE_METHODS(fm10000GetCacheFfuSliceValid,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_SLICE_VALID(0, 0),
    FM10000_FFU_SLICE_VALID_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_SLICE_VALID(1, 0) - FM10000_FFU_SLICE_VALID(0, 0) },
    { FM10000_FFU_SLICE_VALID_ENTRIES }
};

/* FFU_SLICE_CASCADE_ACTION register set descriptor */
const fm_cachedRegs fm10000CacheFfuSliceCascadeAction =
{
    CACHE_METHODS(fm10000GetCacheFfuSliceCascadeAction,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_SLICE_CASCADE_ACTION(0, 0),
    FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_FFU_SLICE_CASCADE_ACTION(1, 0) - FM10000_FFU_SLICE_CASCADE_ACTION(0, 0) },
    { FM10000_FFU_SLICE_CASCADE_ACTION_ENTRIES }
};

/* FFU_SLICE_CFG register set descriptor */
const fm_cachedRegs fm10000CacheFfuSliceCfg =
{
    CACHE_METHODS(fm10000GetCacheFfuSliceCfg,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_FFU_SLICE_CFG(0, 0, 0),
    FM10000_FFU_SLICE_CFG_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    FALSE,
    { FM10000_FFU_SLICE_CFG(0, 1, 0) - FM10000_FFU_SLICE_CFG(0, 0, 0),
      FM10000_FFU_SLICE_CFG(1, 0, 0) - FM10000_FFU_SLICE_CFG(0, 0, 0) },
    { FM10000_FFU_SLICE_CFG_ENTRIES_0, FM10000_FFU_SLICE_CFG_ENTRIES_1 }
};

/* FFU_MASTER_VALID register set descriptor */
const fm_cachedRegs fm10000CacheFfuMasterValid =
{
#ifdef FV_CACHE
    { fm10000GetCacheFfuMasterValid, NULL, fm10000GetCacheRegisterDefaultZero},
#else
    { fm10000GetCacheFfuMasterValid, NULL, NULL },
#endif
    FM10000_FFU_MASTER_VALID(0),
    FM10000_FFU_MASTER_VALID_WIDTH,
    FM_REGS_CACHE_NO_INDEX_USED,
    TRUE,
    { FM_REGS_CACHE_INDEX_UNUSED },
    { 1 }
};

/**************************************************
 * POLICER registers
 **************************************************/

/* POLICER_APPLY_CFG_4K register set descriptor */
const fm_cachedRegs fm10000CachePolicerApplyCfg4k =
{
    CACHE_METHODS(fm10000GetCachePolicerApplyCfg4k,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_APPLY_CFG_4K(0, 0),
    FM10000_POLICER_APPLY_CFG_4K_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_POLICER_APPLY_CFG_4K(0, 1) - FM10000_POLICER_APPLY_CFG_4K(0, 0),
      FM10000_POLICER_APPLY_CFG_4K(1, 0) - FM10000_POLICER_APPLY_CFG_4K(0, 0) },
    { FM10000_POLICER_APPLY_CFG_4K_ENTRIES_0, FM10000_POLICER_APPLY_CFG_4K_ENTRIES_1 }
};

/* POLICER_APPLY_CFG_512 register set descriptor */
const fm_cachedRegs fm10000CachePolicerApplyCfg512 =
{
    CACHE_METHODS(fm10000GetCachePolicerApplyCfg512,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_APPLY_CFG_512(0, 0),
    FM10000_POLICER_APPLY_CFG_512_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_POLICER_APPLY_CFG_512(0, 1) - FM10000_POLICER_APPLY_CFG_512(0, 0),
      FM10000_POLICER_APPLY_CFG_512(1, 0) - FM10000_POLICER_APPLY_CFG_512(0, 0) },
    { FM10000_POLICER_APPLY_CFG_512_ENTRIES_0, FM10000_POLICER_APPLY_CFG_512_ENTRIES_1 }
};

/* POLICER_DSCP_DOWN_MAP register set descriptor */
const fm_cachedRegs fm10000CachePolicerDscpDownMap =
{
    CACHE_METHODS(fm10000GetCachePolicerDscpDownMap,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_DSCP_DOWN_MAP(0),
    FM10000_POLICER_DSCP_DOWN_MAP_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_POLICER_DSCP_DOWN_MAP(1) - FM10000_POLICER_DSCP_DOWN_MAP(0) },
    { FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES }
};

/* POLICER_SWPRI_DOWN_MAP register set descriptor */
const fm_cachedRegs fm10000CachePolicerSwpriDownMap =
{
    CACHE_METHODS(fm10000GetCachePolicerSwpriDownMap,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_SWPRI_DOWN_MAP(0),
    FM10000_POLICER_SWPRI_DOWN_MAP_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_POLICER_SWPRI_DOWN_MAP(1) - FM10000_POLICER_SWPRI_DOWN_MAP(0) },
    { FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES }
};

/* POLICER_CFG_4K register set descriptor */
const fm_cachedRegs fm10000CachePolicerCfg4k =
{
    CACHE_METHODS(fm10000GetCachePolicerCfg4k,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_CFG_4K(0, 0, 0),
    FM10000_POLICER_CFG_4K_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_POLICER_CFG_4K(0, 1, 0) - FM10000_POLICER_CFG_4K(0, 0, 0),
      FM10000_POLICER_CFG_4K(1, 0, 0) - FM10000_POLICER_CFG_4K(0, 0, 0) },
    { FM10000_POLICER_CFG_4K_ENTRIES_0, FM10000_POLICER_CFG_4K_ENTRIES_1 }
};

/* POLICER_CFG_512 register set descriptor */
const fm_cachedRegs fm10000CachePolicerCfg512 =
{
    CACHE_METHODS(fm10000GetCachePolicerCfg512,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_CFG_512(0, 0, 0),
    FM10000_POLICER_CFG_512_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_POLICER_CFG_512(0, 1, 0) - FM10000_POLICER_CFG_512(0, 0, 0),
      FM10000_POLICER_CFG_512(1, 0, 0) - FM10000_POLICER_CFG_512(0, 0, 0) },
    { FM10000_POLICER_CFG_512_ENTRIES_0, FM10000_POLICER_CFG_512_ENTRIES_1 }
};

/* POLICER_STATE_4K register set descriptor */
const fm_cachedRegs fm10000CachePolicerState4k =
{
    CACHE_METHODS(fm10000GetCachePolicerState4k,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_STATE_4K(0, 0, 0),
    FM10000_POLICER_STATE_4K_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_POLICER_STATE_4K(0, 1, 0) - FM10000_POLICER_STATE_4K(0, 0, 0),
      FM10000_POLICER_STATE_4K(1, 0, 0) - FM10000_POLICER_STATE_4K(0, 0, 0) },
    { FM10000_POLICER_STATE_4K_ENTRIES_0, FM10000_POLICER_STATE_4K_ENTRIES_1 }
};

/* POLICER_STATE_512 register set descriptor */
const fm_cachedRegs fm10000CachePolicerState512 =
{
    CACHE_METHODS(fm10000GetCachePolicerState512,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_STATE_512(0, 0, 0),
    FM10000_POLICER_STATE_512_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_POLICER_STATE_512(0, 1, 0) - FM10000_POLICER_STATE_512(0, 0, 0),
      FM10000_POLICER_STATE_512(1, 0, 0) - FM10000_POLICER_STATE_512(0, 0, 0) },
    { FM10000_POLICER_STATE_512_ENTRIES_0, FM10000_POLICER_STATE_512_ENTRIES_1 }
};

/* POLICER_CFG register set descriptor */
const fm_cachedRegs fm10000CachePolicerCfg =
{
    CACHE_METHODS(fm10000GetCachePolicerCfg,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_POLICER_CFG(0, 0),
    FM10000_POLICER_CFG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_POLICER_CFG(1, 0) - FM10000_POLICER_CFG(0, 0) },
    { FM10000_POLICER_CFG_ENTRIES }
};

/**************************************************
 * TE registers
 **************************************************/

/* TE_DATA register set descriptor */
const fm_cachedRegs fm10000CacheTeData =
{
    CACHE_METHODS(fm10000GetCacheTeData,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DATA(0, 0, 0),
    FM10000_TE_DATA_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_DATA(0, 1, 0) - FM10000_TE_DATA(0, 0, 0),
      FM10000_TE_DATA(1, 0, 0) - FM10000_TE_DATA(0, 0, 0) },
    { FM10000_TE_DATA_ENTRIES_0, FM10000_TE_DATA_ENTRIES_1 }
};

/* TE_LOOKUP register set descriptor */
const fm_cachedRegs fm10000CacheTeLookup =
{
    CACHE_METHODS(fm10000GetCacheTeLookup,
                  NULL,
                  fm10000GetCacheTeLookupDefault),
    FM10000_TE_LOOKUP(0, 0, 0),
    FM10000_TE_LOOKUP_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_LOOKUP(0, 1, 0) - FM10000_TE_LOOKUP(0, 0, 0),
      FM10000_TE_LOOKUP(1, 0, 0) - FM10000_TE_LOOKUP(0, 0, 0) },
    { FM10000_TE_LOOKUP_ENTRIES_0, FM10000_TE_LOOKUP_ENTRIES_1 }
};

/* TE_DGLORT_MAP register set descriptor */
const fm_cachedRegs fm10000CacheTeDglortMap =
{
    CACHE_METHODS(fm10000GetCacheTeDglortMap,
                  NULL,
                  fm10000GetCacheTeGlortMapDefault),
    FM10000_TE_DGLORT_MAP(0, 0, 0),
    FM10000_TE_DGLORT_MAP_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_DGLORT_MAP(0, 1, 0) - FM10000_TE_DGLORT_MAP(0, 0, 0),
      FM10000_TE_DGLORT_MAP(1, 0, 0) - FM10000_TE_DGLORT_MAP(0, 0, 0) },
    { FM10000_TE_DGLORT_MAP_ENTRIES_0, FM10000_TE_DGLORT_MAP_ENTRIES_1 }
};

/* TE_DGLORT_DEC register set descriptor */
const fm_cachedRegs fm10000CacheTeDglortDec =
{
    CACHE_METHODS(fm10000GetCacheTeDglortDec,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DGLORT_DEC(0, 0, 0),
    FM10000_TE_DGLORT_DEC_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_DGLORT_DEC(0, 1, 0) - FM10000_TE_DGLORT_DEC(0, 0, 0),
      FM10000_TE_DGLORT_DEC(1, 0, 0) - FM10000_TE_DGLORT_DEC(0, 0, 0) },
    { FM10000_TE_DGLORT_DEC_ENTRIES_0, FM10000_TE_DGLORT_DEC_ENTRIES_1 }
};

/* TE_SGLORT_MAP register set descriptor */
const fm_cachedRegs fm10000CacheTeSglortMap =
{
    CACHE_METHODS(fm10000GetCacheTeSglortMap,
                  NULL,
                  fm10000GetCacheTeGlortMapDefault),
    FM10000_TE_SGLORT_MAP(0, 0, 0),
    FM10000_TE_SGLORT_MAP_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_SGLORT_MAP(0, 1, 0) - FM10000_TE_SGLORT_MAP(0, 0, 0),
      FM10000_TE_SGLORT_MAP(1, 0, 0) - FM10000_TE_SGLORT_MAP(0, 0, 0) },
    { FM10000_TE_SGLORT_MAP_ENTRIES_0, FM10000_TE_SGLORT_MAP_ENTRIES_1 }
};

/* TE_SGLORT_DEC register set descriptor */
const fm_cachedRegs fm10000CacheTeSglortDec =
{
    CACHE_METHODS(fm10000GetCacheTeSglortDec,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_SGLORT_DEC(0, 0, 0),
    FM10000_TE_SGLORT_DEC_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_SGLORT_DEC(0, 1, 0) - FM10000_TE_SGLORT_DEC(0, 0, 0),
      FM10000_TE_SGLORT_DEC(1, 0, 0) - FM10000_TE_SGLORT_DEC(0, 0, 0) },
    { FM10000_TE_SGLORT_DEC_ENTRIES_0, FM10000_TE_SGLORT_DEC_ENTRIES_1 }
};

/* TE_DEFAULT_DGLORT register set descriptor */
const fm_cachedRegs fm10000CacheTeDefaultDglort =
{
    CACHE_METHODS(fm10000GetCacheTeDefaultDglort,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DEFAULT_DGLORT(0, 0),
    FM10000_TE_DEFAULT_DGLORT_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_DEFAULT_DGLORT(1,0) - FM10000_TE_DEFAULT_DGLORT(0, 0) },
    { FM10000_TE_DEFAULT_DGLORT_ENTRIES }
};

/* TE_DEFAULT_SGLORT register set descriptor */
const fm_cachedRegs fm10000CacheTeDefaultSglort =
{
    CACHE_METHODS(fm10000GetCacheTeDefaultSglort,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DEFAULT_SGLORT(0, 0),
    FM10000_TE_DEFAULT_SGLORT_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_DEFAULT_SGLORT(1,0) - FM10000_TE_DEFAULT_SGLORT(0, 0) },
    { FM10000_TE_DEFAULT_SGLORT_ENTRIES }
};

/* TE_SIP register set descriptor */
const fm_cachedRegs fm10000CacheTeSip =
{
    CACHE_METHODS(fm10000GetCacheTeSip,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_SIP(0, 0, 0),
    FM10000_TE_SIP_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_SIP(0, 1, 0) - FM10000_TE_SIP(0, 0, 0),
      FM10000_TE_SIP(1, 0, 0) - FM10000_TE_SIP(0, 0, 0) },
    { FM10000_TE_SIP_ENTRIES_0, FM10000_TE_SIP_ENTRIES_1 }
};

/* TE_VNI register set descriptor */
const fm_cachedRegs fm10000CacheTeVni =
{
    CACHE_METHODS(fm10000GetCacheTeVni,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_VNI(0, 0, 0),
    FM10000_TE_VNI_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_VNI(0, 1, 0) - FM10000_TE_VNI(0, 0, 0),
      FM10000_TE_VNI(1, 0, 0) - FM10000_TE_VNI(0, 0, 0) },
    { FM10000_TE_VNI_ENTRIES_0, FM10000_TE_VNI_ENTRIES_1 }
};

/* TE_DEFAULT_L4DST register set descriptor */
const fm_cachedRegs fm10000CacheTeDefaultL4Dst =
{
    CACHE_METHODS(fm10000GetCacheTeDefaultL4Dst,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DEFAULT_L4DST(0, 0),
    FM10000_TE_DEFAULT_L4DST_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_DEFAULT_L4DST(1,0) - FM10000_TE_DEFAULT_L4DST(0, 0) },
    { FM10000_TE_DEFAULT_L4DST_ENTRIES }
};

/* TE_CFG register set descriptor */
const fm_cachedRegs fm10000CacheTeCfg =
{
    { fm10000GetCacheTeCfg, NULL, NULL },
    FM10000_TE_CFG(0, 0),
    FM10000_TE_CFG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_CFG(1,0) - FM10000_TE_CFG(0, 0) },
    { FM10000_TE_CFG_ENTRIES }
};

/* TE_PORT register set descriptor */
const fm_cachedRegs fm10000CacheTePorts =
{
    CACHE_METHODS(fm10000GetCacheTePorts,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_PORTS(0, 0),
    FM10000_TE_PORTS_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_PORTS(1,0) - FM10000_TE_PORTS(0, 0) },
    { FM10000_TE_PORTS_ENTRIES }
};

/* TE_TUN_HEADER_CFG register set descriptor */
const fm_cachedRegs fm10000CacheTeTunHeaderCfg =
{
    { fm10000GetCacheTeTunHeaderCfg, NULL, NULL },
    FM10000_TE_TUN_HEADER_CFG(0, 0),
    FM10000_TE_TUN_HEADER_CFG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_TUN_HEADER_CFG(1,0) - FM10000_TE_TUN_HEADER_CFG(0, 0) },
    { FM10000_TE_TUN_HEADER_CFG_ENTRIES }
};

/* TE_TRAP_DGLORT register set descriptor */
const fm_cachedRegs fm10000CacheTeTrapDglort =
{
    CACHE_METHODS(fm10000GetCacheTeTrapDglort,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_TRAP_DGLORT(0, 0),
    FM10000_TE_TRAP_DGLORT_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_TRAP_DGLORT(1,0) - FM10000_TE_TRAP_DGLORT(0, 0) },
    { FM10000_TE_TRAP_DGLORT_ENTRIES }
};

/* TE_DEFAULT_NGE_DATA register set descriptor */
const fm_cachedRegs fm10000CacheTeDefaultNgeData =
{
    CACHE_METHODS(fm10000GetCacheTeDefaultNgeData,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DEFAULT_NGE_DATA(0, 0, 0),
    FM10000_TE_DEFAULT_NGE_DATA_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_TE_DEFAULT_NGE_DATA(0, 1, 0) - FM10000_TE_DEFAULT_NGE_DATA(0, 0, 0),
      FM10000_TE_DEFAULT_NGE_DATA(1, 0, 0) - FM10000_TE_DEFAULT_NGE_DATA(0, 0, 0) },
    { FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0, FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_1 }
};

/* TE_DEFAULT_NGE_MASK register set descriptor */
const fm_cachedRegs fm10000CacheTeDefaultNgeMask =
{
    CACHE_METHODS(fm10000GetCacheTeDefaultNgeMask,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DEFAULT_NGE_MASK(0, 0),
    FM10000_TE_DEFAULT_NGE_MASK_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_DEFAULT_NGE_MASK(1,0) - FM10000_TE_DEFAULT_NGE_MASK(0, 0) },
    { FM10000_TE_DEFAULT_NGE_MASK_ENTRIES }
};

/* TE_EXVET register set descriptor */
const fm_cachedRegs fm10000CacheTeExvet =
{
    { fm10000GetCacheTeExvet, NULL, NULL },
    FM10000_TE_EXVET(0, 0),
    FM10000_TE_EXVET_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_EXVET(1,0) - FM10000_TE_EXVET(0, 0) },
    { FM10000_TE_EXVET_ENTRIES }
};

/* TE_DMAC register set descriptor */
const fm_cachedRegs fm10000CacheTeDmac =
{
    CACHE_METHODS(fm10000GetCacheTeDmac,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_DMAC(0, 0),
    FM10000_TE_DMAC_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_DMAC(1,0) - FM10000_TE_DMAC(0, 0) },
    { FM10000_TE_DMAC_ENTRIES }
};

/* TE_SMAC register set descriptor */
const fm_cachedRegs fm10000CacheTeSmac =
{
    CACHE_METHODS(fm10000GetCacheTeSmac,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_TE_SMAC(0, 0),
    FM10000_TE_SMAC_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_SMAC(1,0) - FM10000_TE_SMAC(0, 0) },
    { FM10000_TE_SMAC_ENTRIES }
};

/* TE_TRAP_CONFIG register set descriptor */
const fm_cachedRegs fm10000CacheTeTrapCfg =
{
    { fm10000GetCacheTeTrapCfg, NULL, NULL },
    FM10000_TE_TRAP_CONFIG(0, 0),
    FM10000_TE_TRAP_CONFIG_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_TE_TRAP_CONFIG(1,0) - FM10000_TE_TRAP_CONFIG(0, 0) },
    { FM10000_TE_TRAP_CONFIG_ENTRIES }
};

/**************************************************
 * Other registers
 **************************************************/

/* GLORT_CAM register set descriptor */
const fm_cachedRegs fm10000CacheGlortCam =
{
    CACHE_METHODS(fm10000GetCacheGlortCam,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_GLORT_CAM(0),
    FM10000_GLORT_CAM_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_GLORT_CAM(1) - FM10000_GLORT_CAM(0) },
    { FM10000_GLORT_CAM_ENTRIES }
};

/* GLORT_RAM register set descriptor */
const fm_cachedRegs fm10000CacheGlortRam =
{
    CACHE_METHODS(fm10000GetCacheGlortRam,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_GLORT_RAM(0, 0),
    FM10000_GLORT_RAM_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_GLORT_RAM(1, 0) - FM10000_GLORT_RAM(0, 0) },
    { FM10000_GLORT_RAM_ENTRIES }
};

/* INGRESS_MST_TABLE register set descriptor */
const fm_cachedRegs fm10000CacheIngressMstTable =
{
    CACHE_METHODS(fm10000GetCacheIngressMstTable,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_INGRESS_MST_TABLE(0, 0, 0),
    FM10000_INGRESS_MST_TABLE_WIDTH,
    FM_REGS_CACHE_TWO_INDICES_USED,
    TRUE,
    { FM10000_INGRESS_MST_TABLE(0, 1, 0) - FM10000_INGRESS_MST_TABLE(0, 0, 0),
      FM10000_INGRESS_MST_TABLE(1, 0, 0) - FM10000_INGRESS_MST_TABLE(0, 0, 0) },
    { FM10000_INGRESS_MST_TABLE_ENTRIES_0, FM10000_INGRESS_MST_TABLE_ENTRIES_1 }
};

/* EGRESS_MST_TABLE register set descriptor */
const fm_cachedRegs fm10000CacheEgressMstTable =
{
    CACHE_METHODS(fm10000GetCacheEgressMstTable,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_EGRESS_MST_TABLE(0, 0),
    FM10000_EGRESS_MST_TABLE_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_EGRESS_MST_TABLE(1, 0) - FM10000_EGRESS_MST_TABLE(0, 0) },
    { FM10000_EGRESS_MST_TABLE_ENTRIES }
};

/* MOD_PER_PORT_CFG_1 register set descriptor */
const fm_cachedRegs fm10000CacheModPerPortCfg1 =
{
    CACHE_METHODS(fm10000GetCacheModPerPortCfg1,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_MOD_PER_PORT_CFG_1(0, 0),
    FM10000_MOD_PER_PORT_CFG_1_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_MOD_PER_PORT_CFG_1(1, 0) - FM10000_MOD_PER_PORT_CFG_1(0, 0) },
    { FM10000_MOD_PER_PORT_CFG_1_ENTRIES }
};

/* MOD_PER_PORT_CFG_2 register set descriptor */
const fm_cachedRegs fm10000CacheModPerPortCfg2 =
{
    CACHE_METHODS(fm10000GetCacheModPerPortCfg2,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_MOD_PER_PORT_CFG_2(0, 0),
    FM10000_MOD_PER_PORT_CFG_2_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_MOD_PER_PORT_CFG_2(1, 0) - FM10000_MOD_PER_PORT_CFG_2(0, 0) },
    { FM10000_MOD_PER_PORT_CFG_2_ENTRIES }
};

/* PARSER_PORT_CFG_2 register set descriptor */
const fm_cachedRegs fm10000CacheParserPortCfg2 =
{
    CACHE_METHODS(fm10000GetCacheParserPortCfg2,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_PARSER_PORT_CFG_2(0, 0),
    FM10000_PARSER_PORT_CFG_2_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_PARSER_PORT_CFG_2(1, 0) - FM10000_PARSER_PORT_CFG_2(0, 0) },
    { FM10000_PARSER_PORT_CFG_2_ENTRIES }
};

/* PARSER_PORT_CFG_3 register set descriptor */
const fm_cachedRegs fm10000CacheParserPortCfg3 =
{
    CACHE_METHODS(fm10000GetCacheParserPortCfg3,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_PARSER_PORT_CFG_3(0, 0),
    FM10000_PARSER_PORT_CFG_3_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_PARSER_PORT_CFG_3(1, 0) - FM10000_PARSER_PORT_CFG_3(0, 0) },
    { FM10000_PARSER_PORT_CFG_3_ENTRIES }
};

/* INGRESS_VID_TABLE register set descriptor */
const fm_cachedRegs fm10000CacheIngressVidTable =
{
    CACHE_METHODS(fm10000GetCacheIngressVidTable,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_INGRESS_VID_TABLE(0, 0),
    FM10000_INGRESS_VID_TABLE_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_INGRESS_VID_TABLE(1,0) - FM10000_INGRESS_VID_TABLE (0,0) },
    { FM10000_INGRESS_VID_TABLE_ENTRIES }
};

/* EGRESS_VID_TABLE register set descriptor */
const fm_cachedRegs fm10000CacheEgressVidTable =
{
    CACHE_METHODS(fm10000GetCacheEgressVidTable,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_EGRESS_VID_TABLE(0, 0),
    FM10000_EGRESS_VID_TABLE_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_EGRESS_VID_TABLE(1, 0) - FM10000_EGRESS_VID_TABLE (0, 0) },
    { FM10000_EGRESS_VID_TABLE_ENTRIES }
};

/* MOD_MIRROR_PROFILE_TABLE register set descriptor */
const fm_cachedRegs fm10000CacheModMirrorProfTable =
{
    CACHE_METHODS(fm10000GetCacheModMirrorProfTable,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_MOD_MIRROR_PROFILE_TABLE(0, 0),
    FM10000_MOD_MIRROR_PROFILE_TABLE_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_MOD_MIRROR_PROFILE_TABLE(1, 0) - FM10000_MOD_MIRROR_PROFILE_TABLE(0, 0) },
    { FM10000_MOD_MIRROR_PROFILE_TABLE_ENTRIES }
};

/* MOD_VLAN_TAG_VID1_MAP register set descriptor */
const fm_cachedRegs fm10000CacheModVlanTagVid1Map =
{
    CACHE_METHODS(fm10000GetCacheModVlanTagVid1Map,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_MOD_VLAN_TAG_VID1_MAP(0, 0),
    FM10000_MOD_VLAN_TAG_VID1_MAP_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_MOD_VLAN_TAG_VID1_MAP(1,0) - FM10000_MOD_VLAN_TAG_VID1_MAP(0, 0) },
    { FM10000_MOD_VLAN_TAG_VID1_MAP_ENTRIES }
};

/* MOD_VPRI1_MAP register set descriptor */
const fm_cachedRegs fm10000CacheModVpri1Map =
{
    CACHE_METHODS(fm10000GetCacheModVpri1Map,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_MOD_VPRI1_MAP(0, 0),
    FM10000_MOD_VPRI1_MAP_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_MOD_VPRI1_MAP(1, 0) - FM10000_MOD_VPRI1_MAP(0, 0) },
    { FM10000_MOD_VPRI1_MAP_ENTRIES }
};

/* MOD_VPRI2_MAP register set descriptor */
const fm_cachedRegs fm10000CacheModVpri2Map =
{
    CACHE_METHODS(fm10000GetCacheModVpri2Map,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_MOD_VPRI2_MAP(0, 0),
    FM10000_MOD_VPRI2_MAP_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_MOD_VPRI2_MAP(1, 0) - FM10000_MOD_VPRI2_MAP(0, 0) },
    { FM10000_MOD_VPRI2_MAP_ENTRIES }
};

/* RX_VPRI_MAP register set descriptor */
const fm_cachedRegs fm10000CacheRxVpriMap =
{
    CACHE_METHODS(fm10000GetCacheRxVpriMap,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_RX_VPRI_MAP(0, 0),
    FM10000_RX_VPRI_MAP_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_RX_VPRI_MAP(1, 0) - FM10000_RX_VPRI_MAP(0, 0) },
    { FM10000_RX_VPRI_MAP_ENTRIES }
};

/* SAF_MATRIX register set descriptor */
const fm_cachedRegs fm10000CacheSafMatrix =
{
    CACHE_METHODS(fm10000GetCacheSafMatrix,
                  NULL,
                  fm10000GetCacheRegisterDefaultZero),
    FM10000_SAF_MATRIX(0, 0),
    FM10000_SAF_MATRIX_WIDTH,
    FM_REGS_CACHE_ONE_INDEX_USED,
    TRUE,
    { FM10000_SAF_MATRIX(1, 0) - FM10000_SAF_MATRIX(0, 0) },
    { FM10000_SAF_MATRIX_ENTRIES }
};

/**************************************************
 * Definition of the cacheable register list.
 **************************************************/

const fm_cachedRegs *fm10000CachedRegisterList[] =
{
    &fm10000CacheFfuMapSrc,
    &fm10000CacheFfuMapMac,
    &fm10000CacheFfuMapVlan,
    &fm10000CacheFfuMapType,
    &fm10000CacheFfuMapLength,
    &fm10000CacheFfuMapIpLo,
    &fm10000CacheFfuMapIpHi,
    &fm10000CacheFfuMapIpCfg,
    &fm10000CacheFfuMapProt,
    &fm10000CacheFfuMapL4Src,
    &fm10000CacheFfuMapL4Dst,
    &fm10000CacheFfuEgressChunkCfg,
    &fm10000CacheFfuEgressChunkValid,
    &fm10000CacheFfuEgressChunkActions,
    &fm10000CacheFfuEgressPortCfg,
    &fm10000CacheFfuSliceTcam,
    &fm10000CacheFfuSliceSram,
    &fm10000CacheFfuSliceValid,
    &fm10000CacheFfuSliceCascadeAction,
    &fm10000CacheFfuSliceCfg,

    &fm10000CacheGlortRam,
    &fm10000CacheIngressMstTable,
    &fm10000CacheEgressMstTable,
    &fm10000CacheModMirrorProfTable,
    &fm10000CacheModPerPortCfg1,
    &fm10000CacheModPerPortCfg2,
    &fm10000CacheParserPortCfg2,
    &fm10000CacheParserPortCfg3,
    &fm10000CacheModVpri1Map,
    &fm10000CacheModVpri2Map,
    &fm10000CacheRxVpriMap,
    &fm10000CacheSafMatrix,

    &fm10000CachePolicerApplyCfg4k,
    &fm10000CachePolicerApplyCfg512,
    &fm10000CachePolicerDscpDownMap,
    &fm10000CachePolicerSwpriDownMap,
    &fm10000CachePolicerCfg4k,
    &fm10000CachePolicerCfg512,
    &fm10000CachePolicerState4k,
    &fm10000CachePolicerState512,
    &fm10000CachePolicerCfg,

    &fm10000CacheIngressVidTable,
    &fm10000CacheEgressVidTable,
    &fm10000CacheModVlanTagVid1Map,

    &fm10000CacheFfuMasterValid,

    &fm10000CacheTeData,
    &fm10000CacheTeLookup,
    &fm10000CacheTeDglortMap,
    &fm10000CacheTeDglortDec,
    &fm10000CacheTeSglortMap,
    &fm10000CacheTeSglortDec,
    &fm10000CacheTeDefaultDglort,
    &fm10000CacheTeDefaultSglort,
    &fm10000CacheTeSip,
    &fm10000CacheTeVni,
    &fm10000CacheTeDefaultL4Dst,
    &fm10000CacheTeCfg,
    &fm10000CacheTePorts,
    &fm10000CacheTeTunHeaderCfg,
    &fm10000CacheTeTrapDglort,
    &fm10000CacheTeDefaultNgeData,
    &fm10000CacheTeDefaultNgeMask,
    &fm10000CacheTeExvet,
    &fm10000CacheTeDmac,
    &fm10000CacheTeSmac,
    &fm10000CacheTeTrapCfg,
    NULL
};


/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000GetCacheFromSwitch
 * \ingroup intRegCache
 *
 * \desc            Given a switch number, returns a pointer to the
 *                  register cache structure for that switch.
 *
 * \param[in]       sw is the switch on which to operate.
 *
 * \return          pointer to register cache.
 *
 *****************************************************************************/
static fm_fm10000RegCache *fm10000GetCacheFromSwitch(fm_int sw)
{
    return &( (fm10000_switch *)
             fmRootApi->fmSwitchStateTable[sw]->extension )->registerCache;

}   /* end fm10000GetCacheFromSwitch */


static fm_uint32 *fm10000GetCacheFfuMapSrc(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapSrc;

}   /* end fm10000GetCacheFfuMapSrc */


static fm_uint32 *fm10000GetCacheFfuMapMac(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapMac;

}   /* end fm10000GetCacheFfuMapMac */


static fm_uint32 *fm10000GetCacheFfuMapVlan(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapVlan;

}   /* end fm10000GetCacheFfuMapVlan */


static fm_uint32 *fm10000GetCacheFfuMapType(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapType;

}   /* end fm10000GetCacheFfuMapType */


static fm_uint32 *fm10000GetCacheFfuMapLength(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapLength;

}   /* end fm10000GetCacheFfuMapLength */


static fm_uint32 *fm10000GetCacheFfuMapIpLo(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapIpLo;

}   /* end fm10000GetCacheFfuMapIpLo */


static fm_uint32 *fm10000GetCacheFfuMapIpHi(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapIpHi;

}   /* end fm10000GetCacheFfuMapIpHi */


static fm_uint32 *fm10000GetCacheFfuMapIpCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapIpCfg;

}   /* end fm10000GetCacheFfuMapIpCfg */


static fm_uint32 *fm10000GetCacheFfuMapProt(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapProt;

}   /* end fm10000GetCacheFfuMapProt */


static fm_uint32 *fm10000GetCacheFfuMapL4Src(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapL4Src;

}   /* end fm10000GetCacheFfuMapL4Src */


static fm_uint32 *fm10000GetCacheFfuMapL4Dst(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMapL4Dst;

}   /* end fm10000GetCacheFfuMapL4Dst */


static fm_uint32 *fm10000GetCacheFfuEgressChunkCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuEgressChunkCfg;

}   /* end fm10000GetCacheFfuEgressChunkCfg */


static fm_uint32 *fm10000GetCacheFfuEgressChunkValid(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuEgressChunkValid;

}   /* end fm10000GetCacheFfuEgressChunkValid */


static fm_uint32 *fm10000GetCacheFfuEgressChunkActions(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuEgressChunkActions;

}   /* end fm10000GetCacheFfuEgressChunkActions */


static fm_uint32 *fm10000GetCacheFfuEgressPortCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuEgressPortCfg;

}   /* end fm10000GetCacheFfuEgressPortCfg */


static fm_uint32 *fm10000GetCacheFfuSliceTcam(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuSliceTcam;

}   /* end fm10000GetCacheFfuSliceTcam */


static fm_bitArray *fm10000GetCacheFfuSliceTcamValidKey(fm_int sw)
{
    return &(fm10000GetCacheFromSwitch(sw)->ffuSliceTcamValidKey);
     
}  /* end fm10000GetCacheFfuSliceTcamValidKey */


static fm_uint32 *fm10000GetCacheFfuSliceSram(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuSliceSram;

}   /* end fm10000GetCacheFfuSliceSram */


static fm_uint32 *fm10000GetCacheFfuSliceValid(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuSliceValid;

}   /* end fm10000GetCacheFfuSliceValid */


static fm_uint32 *fm10000GetCacheFfuSliceCascadeAction(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuSliceCascadeAction;

}   /* end fm10000GetCacheFfuSliceCascadeAction */


static fm_uint32 *fm10000GetCacheFfuSliceCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuSliceCfg;

}   /* end fm10000GetCacheFfuSliceCfg */


static fm_uint32 *fm10000GetCacheFfuMasterValid(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ffuMasterValid;

}   /* end fm10000GetCacheFfuMasterValid */ 


static fm_uint32 *fm10000GetCacheGlortCam(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->glortCam;

}   /* end fm10000GetCacheGlortCam */


static fm_uint32 *fm10000GetCacheGlortRam(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->glortRam;

}   /* end fm10000GetCacheGlortRam */


static fm_uint32 *fm10000GetCacheIngressMstTable(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ingressMstTable;

}   /* end fm10000GetCacheIngressMstTable */


static fm_uint32 *fm10000GetCacheEgressMstTable(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->egressMstTable;

}   /* end fm10000GetCacheEgressMstTable */


static fm_uint32 *fm10000GetCacheModMirrorProfTable(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->modMirrorProfTable;

}   /* end fm10000GetCacheModMirrorProfTable */


static fm_uint32 *fm10000GetCacheModPerPortCfg1(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->modPerPortCfg1;

}   /* end fm10000GetCacheModPerPortCfg1 */


static fm_uint32 *fm10000GetCacheModPerPortCfg2(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->modPerPortCfg2;

}   /* end fm10000GetCacheModPerPortCfg2 */


static fm_uint32 *fm10000GetCacheParserPortCfg2(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->parserPortCfg2;

}   /* end fm10000GetCacheParserPortCfg2 */


static fm_uint32 *fm10000GetCacheParserPortCfg3(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->parserPortCfg3;

}   /* end fm10000GetCacheParserPortCfg3 */


static fm_uint32 *fm10000GetCacheModVpri1Map(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->modVpri1Map;

}   /* end fm10000GetCacheModVpri1Map */


static fm_uint32 *fm10000GetCacheModVpri2Map(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->modVpri2Map;

}   /* end fm10000GetCacheModVpri2Map */


static fm_uint32 *fm10000GetCacheRxVpriMap(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->rxVpriMap;

}   /* end fm10000GetCacheRxVpriMap */


static fm_uint32 *fm10000GetCacheSafMatrix(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->safMatrix;

}   /* end fm10000GetCacheSafMatrix */


static fm_uint32 *fm10000GetCachePolicerApplyCfg4k(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerApplyCfg4k;

}   /* end fm10000GetCachePolicerApplyCfg4k */


static fm_uint32 *fm10000GetCachePolicerApplyCfg512(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerApplyCfg512;

}   /* end fm10000GetCachePolicerApplyCfg512 */


static fm_uint32 *fm10000GetCachePolicerDscpDownMap(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerDscpDownMap;

}   /* end fm10000GetCachePolicerDscpDownMap */


static fm_uint32 *fm10000GetCachePolicerSwpriDownMap(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerSwpriDownMap;

}   /* end fm10000GetCachePolicerSwpriDownMap */


static fm_uint32 *fm10000GetCachePolicerCfg4k(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerCfg4k;

}   /* end fm10000GetCachePolicerCfg4k */


static fm_uint32 *fm10000GetCachePolicerCfg512(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerCfg512;

}   /* end fm10000GetCachePolicerCfg512 */


static fm_uint32 *fm10000GetCachePolicerState4k(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerState4k;

}   /* end fm10000GetCachePolicerState4k */


static fm_uint32 *fm10000GetCachePolicerState512(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerState512;

}   /* end fm10000GetCachePolicerState512 */


static fm_uint32 *fm10000GetCachePolicerCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->policerCfg;

}   /* end fm10000GetCachePolicerCfg */


static fm_uint32 *fm10000GetCacheIngressVidTable(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->ingressVidTable;

}   /* end fm10000GetCacheIngressVidTable */


static fm_uint32 *fm10000GetCacheEgressVidTable(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->egressVidTable;

}   /* end fm10000GetCacheEgressVidTable */


static fm_uint32 *fm10000GetCacheModVlanTagVid1Map(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->modVlanTagVid1Map;

}   /* end fm10000GetCacheModVlanTagVid1Map */


static fm_uint32 *fm10000GetCacheTeData(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teData;

}   /* end fm10000GetCacheTeData */


static fm_uint32 *fm10000GetCacheTeLookup(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teLookup;

}   /* end fm10000GetCacheTeLookup */


static fm_uint32 *fm10000GetCacheTeDglortMap(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDglortMap;

}   /* end fm10000GetCacheTeDglortMap */


static fm_uint32 *fm10000GetCacheTeDglortDec(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDglortDec;

}   /* end fm10000GetCacheTeDglortDec */


static fm_uint32 *fm10000GetCacheTeSglortMap(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teSglortMap;

}   /* end fm10000GetCacheTeSglortMap */


static fm_uint32 *fm10000GetCacheTeSglortDec(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teSglortDec;

}   /* end fm10000GetCacheTeSglortDec */


static fm_uint32 *fm10000GetCacheTeDefaultDglort(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDefaultDglort;

}   /* end fm10000GetCacheTeDefaultDglort */


static fm_uint32 *fm10000GetCacheTeDefaultSglort(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDefaultSglort;

}   /* end fm10000GetCacheTeDefaultSglort */


static fm_uint32 *fm10000GetCacheTeSip(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teSip;

}   /* end fm10000GetCacheTeSip */


static fm_uint32 *fm10000GetCacheTeVni(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teVni;

}   /* end fm10000GetCacheTeVni */


static fm_uint32 *fm10000GetCacheTeDefaultL4Dst(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDefaultL4Dst;

}   /* end fm10000GetCacheTeDefaultL4Dst */


static fm_uint32 *fm10000GetCacheTeCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teCfg;

}   /* end fm10000GetCacheTeCfg */


static fm_uint32 *fm10000GetCacheTePorts(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->tePorts;

}   /* end fm10000GetCacheTePorts */


static fm_uint32 *fm10000GetCacheTeTunHeaderCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teTunHeaderCfg;

}   /* end fm10000GetCacheTeTunHeaderCfg */


static fm_uint32 *fm10000GetCacheTeTrapDglort(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teTrapDglort;

}   /* end fm10000GetCacheTeTrapDglort */


static fm_uint32 *fm10000GetCacheTeDefaultNgeData(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDefaultNgeData;

}   /* end fm10000GetCacheTeDefaultNgeData */


static fm_uint32 *fm10000GetCacheTeDefaultNgeMask(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDefaultNgeMask;

}   /* end fm10000GetCacheTeDefaultNgeMask */


static fm_uint32 *fm10000GetCacheTeExvet(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teExvet;

}   /* end fm10000GetCacheTeExvet */


static fm_uint32 *fm10000GetCacheTeDmac(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teDmac;

}   /* end fm10000GetCacheTeDmac */


static fm_uint32 *fm10000GetCacheTeSmac(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teSmac;

}   /* end fm10000GetCacheTeSmac */


static fm_uint32 *fm10000GetCacheTeTrapCfg(fm_int sw)
{
    return fm10000GetCacheFromSwitch(sw)->teTrapCfg;

}   /* end fm10000GetCacheTeTrapCfg */


#ifdef FAST_CACHE

/* Defaults for simulation purposes */
static fm_uint32 fm10000GetCacheFfuSliceTcamDefault(fm_uint32 addr)
{
    return 0x00000000;
}

static fm_uint32 fm10000GetCacheRegisterDefaultZero(fm_uint32 addr)
{
    return 0x00000000;
}

static fm_uint32 fm10000GetCacheRegisterDefaultFull(fm_uint32 addr)
{
    return 0xffffffff;
}

static fm_uint32 fm10000GetCacheTeLookupDefault(fm_uint32 addr)
{
    if ((addr % 2) == 0)    /* 31:0 */
    {
        return 0x00810000;
    }
    else                    /* 63:32 */
    {
        return 0x00000000;
    }
}

/* Both TE dglort and sglort maps have the same default values */
static fm_uint32 fm10000GetCacheTeGlortMapDefault(fm_uint32 addr)
{
    if ((addr % 2) == 0)    /* 31:0 */
    {
        return 0x0000FFFF;
    }
    else                    /* 63:31 */
    {
        return 0x00000000;
    }
}

#endif

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/*****************************************************************************/
/** fm10000InitRegisterCache
 * \ingroup intRegCache
 *
 * \chips           FM10000
 *
 * \desc            Function performs FM10000-specific register cache in
 *                  initialization steps, before invoking the generic
 *                  register cache initialization process
 * 
 * \param[in]       sw is the ID of the switch whose cache is being 
 *                  initialized
 * 
 * \return          FM_OK: if successful
 *                  FM_ERR_INVALID_SWITCH: if the switch ID is invalid
 *                  FM_ERR_NO_MEM: if there's no memory for the initialization
 *                  FM_ERR_INVALID_ARGUMENT: if the word count in any register
 *                                           description is wrong
 *                  FM_ERR_BAD_IOCTL: driver error during the first read
 *                  FM_FAIL: any other error
 *
 *****************************************************************************/
fm_status fm10000InitRegisterCache(fm_int sw)
{
    fm_status      err;

    /* sanity check on the switch ID */
    VALIDATE_SWITCH_INDEX(sw);

    FM_LOG_ENTRY(FM_LOG_CAT_SWITCH, "sw=%d\n", sw);

    /* now invoke the generic initialization routine */
    err = fmInitRegisterCache(sw);

    FM_LOG_EXIT(FM_LOG_CAT_SWITCH, err);

}   /* end fm10000InitRegisterCache */

