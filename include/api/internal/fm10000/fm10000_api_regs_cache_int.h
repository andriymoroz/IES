/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm10000_api_regs_cache_int.h
 * Creation Date:   April 19, 2013
 * Description:     Definitions specific to the FM10000-family of switch chips.
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

#ifndef __FM_FM10000_API_REGS_CACHE_INT_H
#define __FM_FM10000_API_REGS_CACHE_INT_H

#define FM10000_USE_GLORT_CAM_CACHE             TRUE
#define FM10000_USE_GLORT_RAM_CACHE             TRUE
#define FM10000_USE_PORT_CFG_CACHE              TRUE
#define FM10000_USE_MST_TABLE_CACHE             TRUE


/* Declaration of the chipset-specific register cache init routine */
fm_status fm10000InitRegisterCache(fm_int sw);

/**************************************************
 * Declaration of constant cached register 
 * descriptors and of the array listing them all 
 **************************************************/

/* FFU registers */
extern const fm_cachedRegs fm10000CacheFfuMapSrc;
extern const fm_cachedRegs fm10000CacheFfuMapMac;
extern const fm_cachedRegs fm10000CacheFfuMapVlan;
extern const fm_cachedRegs fm10000CacheFfuMapType;
extern const fm_cachedRegs fm10000CacheFfuMapLength;
extern const fm_cachedRegs fm10000CacheFfuMapIpLo;
extern const fm_cachedRegs fm10000CacheFfuMapIpHi;
extern const fm_cachedRegs fm10000CacheFfuMapIpCfg;
extern const fm_cachedRegs fm10000CacheFfuMapProt;
extern const fm_cachedRegs fm10000CacheFfuMapL4Src;
extern const fm_cachedRegs fm10000CacheFfuMapL4Dst;
extern const fm_cachedRegs fm10000CacheFfuEgressChunkCfg;
extern const fm_cachedRegs fm10000CacheFfuEgressChunkValid;
extern const fm_cachedRegs fm10000CacheFfuEgressChunkActions;
extern const fm_cachedRegs fm10000CacheFfuEgressPortCfg;
extern const fm_cachedRegs fm10000CacheFfuSliceSram;
extern const fm_cachedRegs fm10000CacheFfuSliceTcam;
extern const fm_cachedRegs fm10000CacheFfuSliceValid;
extern const fm_cachedRegs fm10000CacheFfuSliceCascadeAction;
extern const fm_cachedRegs fm10000CacheFfuSliceCfg;
extern const fm_cachedRegs fm10000CacheFfuMasterValid;

/* POLICER registers */
extern const fm_cachedRegs fm10000CachePolicerApplyCfg4k;
extern const fm_cachedRegs fm10000CachePolicerApplyCfg512;
extern const fm_cachedRegs fm10000CachePolicerDscpDownMap;
extern const fm_cachedRegs fm10000CachePolicerSwpriDownMap;
extern const fm_cachedRegs fm10000CachePolicerCfg4k;
extern const fm_cachedRegs fm10000CachePolicerCfg512;
extern const fm_cachedRegs fm10000CachePolicerState4k;
extern const fm_cachedRegs fm10000CachePolicerState512;
extern const fm_cachedRegs fm10000CachePolicerCfg;

/* TE registers */
extern const fm_cachedRegs fm10000CacheTeData;
extern const fm_cachedRegs fm10000CacheTeLookup;
extern const fm_cachedRegs fm10000CacheTeDglortMap;
extern const fm_cachedRegs fm10000CacheTeDglortDec;
extern const fm_cachedRegs fm10000CacheTeSglortMap;
extern const fm_cachedRegs fm10000CacheTeSglortDec;
extern const fm_cachedRegs fm10000CacheTeDefaultDglort;
extern const fm_cachedRegs fm10000CacheTeDefaultSglort;
extern const fm_cachedRegs fm10000CacheTeSip;
extern const fm_cachedRegs fm10000CacheTeVni;
extern const fm_cachedRegs fm10000CacheTeDefaultL4Dst;
extern const fm_cachedRegs fm10000CacheTeCfg;
extern const fm_cachedRegs fm10000CacheTePorts;
extern const fm_cachedRegs fm10000CacheTeTunHeaderCfg;
extern const fm_cachedRegs fm10000CacheTeTrapDglort;
extern const fm_cachedRegs fm10000CacheTeDefaultNgeData;
extern const fm_cachedRegs fm10000CacheTeDefaultNgeMask;
extern const fm_cachedRegs fm10000CacheTeExvet;
extern const fm_cachedRegs fm10000CacheTeDmac;
extern const fm_cachedRegs fm10000CacheTeSmac;
extern const fm_cachedRegs fm10000CacheTeTrapCfg;

/* Other registers */
extern const fm_cachedRegs fm10000CacheEgressMstTable;
extern const fm_cachedRegs fm10000CacheEgressVidTable;
extern const fm_cachedRegs fm10000CacheGlortCam;
extern const fm_cachedRegs fm10000CacheGlortRam;
extern const fm_cachedRegs fm10000CacheIngressMstTable;
extern const fm_cachedRegs fm10000CacheIngressVidTable;
extern const fm_cachedRegs fm10000CacheModMirrorProfTable;
extern const fm_cachedRegs fm10000CacheModPerPortCfg1;
extern const fm_cachedRegs fm10000CacheModPerPortCfg2;
extern const fm_cachedRegs fm10000CacheModVlanTagVid1Map;
extern const fm_cachedRegs fm10000CacheModVpri1Map;
extern const fm_cachedRegs fm10000CacheModVpri2Map;
extern const fm_cachedRegs fm10000CacheParserPortCfg2;
extern const fm_cachedRegs fm10000CacheParserPortCfg3;
extern const fm_cachedRegs fm10000CacheRxVpriMap;
extern const fm_cachedRegs fm10000CacheSafMatrix;

/**************************************************
 * Register cache structure
 **************************************************/
typedef struct _fm_fm10000RegCache
{
    /* Local Cache of FFU_MASTER_VALID register */
    fm_uint32 ffuMasterValid[FM10000_FFU_MASTER_VALID_WIDTH];

    /* Local Cache of FFU_MAP_SRC registers */
    fm_uint32 ffuMapSrc[FM10000_FFU_MAP_SRC_WIDTH * FM10000_FFU_MAP_SRC_ENTRIES];

    /* Local Cache of FFU_MAP_MAC registers */
    fm_uint32 ffuMapMac[FM10000_FFU_MAP_MAC_WIDTH * FM10000_FFU_MAP_MAC_ENTRIES];

    /* Local Cache of FFU_MAP_VLAN registers */
    fm_uint32 ffuMapVlan[FM10000_FFU_MAP_VLAN_WIDTH * FM10000_FFU_MAP_VLAN_ENTRIES];

    /* Local Cache of FFU_MAP_TYPE registers */
    fm_uint32 ffuMapType[FM10000_FFU_MAP_TYPE_WIDTH * FM10000_FFU_MAP_TYPE_ENTRIES];

    /* Local Cache of FFU_MAP_LENGTH registers */
    fm_uint32 ffuMapLength[FM10000_FFU_MAP_LENGTH_WIDTH *
                           FM10000_FFU_MAP_LENGTH_ENTRIES];

    /* Local Cache of FFU_MAP_IP_LO registers */
    fm_uint32 ffuMapIpLo[FM10000_FFU_MAP_IP_LO_WIDTH * FM10000_FFU_MAP_IP_LO_ENTRIES];

    /* Local Cache of FFU_MAP_IP_HI registers */
    fm_uint32 ffuMapIpHi[FM10000_FFU_MAP_IP_HI_WIDTH * FM10000_FFU_MAP_IP_HI_ENTRIES];

    /* Local Cache of FFU_MAP_IP_CFG registers */
    fm_uint32 ffuMapIpCfg[FM10000_FFU_MAP_IP_CFG_WIDTH *
                          FM10000_FFU_MAP_IP_CFG_ENTRIES];

    /* Local Cache of FFU_MAP_PROT registers */
    fm_uint32 ffuMapProt[FM10000_FFU_MAP_PROT_WIDTH * FM10000_FFU_MAP_PROT_ENTRIES];

    /* Local Cache of FFU_MAP_L4_SRC registers */
    fm_uint32 ffuMapL4Src[FM10000_FFU_MAP_L4_SRC_WIDTH *
                          FM10000_FFU_MAP_L4_SRC_ENTRIES];

    /* Local Cache of FFU_MAP_L4_DST registers */
    fm_uint32 ffuMapL4Dst[FM10000_FFU_MAP_L4_DST_WIDTH *
                          FM10000_FFU_MAP_L4_DST_ENTRIES];

    /* Local Cache of FFU_EGRESS_CHUNK_CFG registers */
    fm_uint32 ffuEgressChunkCfg[FM10000_FFU_EGRESS_CHUNK_CFG_WIDTH *
                                FM10000_FFU_EGRESS_CHUNK_CFG_ENTRIES];

    /* Local Cache of FFU_EGRESS_CHUNK_VALID registers */
    fm_uint32 ffuEgressChunkValid[FM10000_FFU_EGRESS_CHUNK_VALID_WIDTH *
                                  FM10000_FFU_EGRESS_CHUNK_VALID_ENTRIES];

    /* Local Cache of FFU_EGRESS_CHUNK_ACTIONS registers */
    fm_uint32 ffuEgressChunkActions[FM10000_FFU_EGRESS_CHUNK_ACTIONS_WIDTH *
                                    FM10000_FFU_EGRESS_CHUNK_ACTIONS_ENTRIES];

    /* Local Cache of FFU_EGRESS_PORT_CFG registers */
    fm_uint32 ffuEgressPortCfg[FM10000_FFU_EGRESS_PORT_CFG_WIDTH *
                               FM10000_FFU_EGRESS_PORT_CFG_ENTRIES];

    /* Local Cache of FFU_SLICE_TCAM registers */
    fm_uint32 ffuSliceTcam[FM10000_FFU_SLICE_TCAM_WIDTH *
                           FM10000_FFU_SLICE_TCAM_ENTRIES_0 *
                           FM10000_FFU_SLICE_TCAM_ENTRIES_1];

    /* local cache for FFU_SLICE_TCAM of Bit0 of key and keyInvert */
    fm_bitArray ffuSliceTcamValidKey;

    /* Local Cache of FFU_SLICE_SRAM registers */
    fm_uint32 ffuSliceSram[FM10000_FFU_SLICE_SRAM_WIDTH *
                           FM10000_FFU_SLICE_SRAM_ENTRIES_0 *
                           FM10000_FFU_SLICE_SRAM_ENTRIES_1];

    /* Local Cache of FFU_SLICE_VALID registers */
    fm_uint32 ffuSliceValid[FM10000_FFU_SLICE_VALID_WIDTH *
                            FM10000_FFU_SLICE_VALID_ENTRIES];

    /* Local Cache of FFU_SLICE_CASCADE_ACTION registers */
    fm_uint32 ffuSliceCascadeAction[FM10000_FFU_SLICE_CASCADE_ACTION_WIDTH *
                                    FM10000_FFU_SLICE_CASCADE_ACTION_ENTRIES];

    /* Local Cache of FFU_SLICE_CFG registers */
    fm_uint32 ffuSliceCfg[FM10000_FFU_SLICE_CFG_WIDTH *
                          FM10000_FFU_SLICE_CFG_ENTRIES_0 *
                          FM10000_FFU_SLICE_CFG_ENTRIES_1];

    /* Local cache for GLORT_CAM registers. */
    fm_uint32 glortCam[FM10000_GLORT_CAM_WIDTH * FM10000_GLORT_CAM_ENTRIES];

    /* Local cache for GLORT_RAM registers. */
    fm_uint32 glortRam[FM10000_GLORT_RAM_WIDTH * FM10000_GLORT_RAM_ENTRIES];

    /* Local cache for INGRESS_MST_TABLE registers. */
    fm_uint32 ingressMstTable[FM10000_INGRESS_MST_TABLE_WIDTH *
                              FM10000_INGRESS_MST_TABLE_ENTRIES_0 *
                              FM10000_INGRESS_MST_TABLE_ENTRIES_1];

    /* Local cache for EGRESS_MST_TABLE registers. */
    fm_uint32 egressMstTable[FM10000_EGRESS_MST_TABLE_WIDTH *
                             FM10000_EGRESS_MST_TABLE_ENTRIES];

    /* Local cache for MOD_MIRROR_PROFILE_TABLE registers. */
    fm_uint32 modMirrorProfTable[FM10000_MOD_MIRROR_PROFILE_TABLE_WIDTH *
                                 FM10000_MOD_MIRROR_PROFILE_TABLE_ENTRIES];

    /* Local cache for MOD_PER_PORT_CFG_1 registers. */
    fm_uint32 modPerPortCfg1[FM10000_MOD_PER_PORT_CFG_1_WIDTH *
                             FM10000_MOD_PER_PORT_CFG_1_ENTRIES];

    /* Local cache for MOD_PER_PORT_CFG_2 registers. */
    fm_uint32 modPerPortCfg2[FM10000_MOD_PER_PORT_CFG_2_WIDTH *
                             FM10000_MOD_PER_PORT_CFG_2_ENTRIES];

    /* Local cache for PARSER_PORT_CFG_2 registers. */
    fm_uint32 parserPortCfg2[FM10000_PARSER_PORT_CFG_2_WIDTH *
                             FM10000_PARSER_PORT_CFG_2_ENTRIES];

    /* Local cache for PARSER_PORT_CFG_3 registers. */
    fm_uint32 parserPortCfg3[FM10000_PARSER_PORT_CFG_3_WIDTH *
                             FM10000_PARSER_PORT_CFG_3_ENTRIES];

    /* Local cache for MOD_VPRI1_MAP registers. */
    fm_uint32 modVpri1Map[FM10000_MOD_VPRI1_MAP_WIDTH *
                          FM10000_MOD_VPRI1_MAP_ENTRIES];

    /* Local cache for MOD_VPRI2_MAP registers. */
    fm_uint32 modVpri2Map[FM10000_MOD_VPRI2_MAP_WIDTH *
                          FM10000_MOD_VPRI2_MAP_ENTRIES];

    /* Local cache for RX_VPRI_MAP registers. */
    fm_uint32 rxVpriMap[FM10000_RX_VPRI_MAP_WIDTH *
                        FM10000_RX_VPRI_MAP_ENTRIES];

    /* Local cache for SAF_MATRIX registers. */
    fm_uint32 safMatrix[FM10000_SAF_MATRIX_WIDTH *
                        FM10000_SAF_MATRIX_ENTRIES];

    /* Local Cache of POLICER_APPLY_CFG_4K registers */
    fm_uint32 policerApplyCfg4k[FM10000_POLICER_APPLY_CFG_4K_WIDTH *
                                FM10000_POLICER_APPLY_CFG_4K_ENTRIES_0 *
                                FM10000_POLICER_APPLY_CFG_4K_ENTRIES_1];

    /* Local Cache of POLICER_APPLY_CFG_512 registers */
    fm_uint32 policerApplyCfg512[FM10000_POLICER_APPLY_CFG_512_WIDTH *
                                 FM10000_POLICER_APPLY_CFG_512_ENTRIES_0 *
                                 FM10000_POLICER_APPLY_CFG_512_ENTRIES_1];

    /* Local Cache of POLICER_DSCP_DOWN_MAP registers */
    fm_uint32 policerDscpDownMap[FM10000_POLICER_DSCP_DOWN_MAP_WIDTH *
                                 FM10000_POLICER_DSCP_DOWN_MAP_ENTRIES];

    /* Local Cache of POLICER_SWPRI_DOWN_MAP registers */
    fm_uint32 policerSwpriDownMap[FM10000_POLICER_SWPRI_DOWN_MAP_WIDTH *
                                  FM10000_POLICER_SWPRI_DOWN_MAP_ENTRIES];

    /* Local Cache of POLICER_CFG_4K registers */
    fm_uint32 policerCfg4k[FM10000_POLICER_CFG_4K_WIDTH *
                           FM10000_POLICER_CFG_4K_ENTRIES_0 *
                           FM10000_POLICER_CFG_4K_ENTRIES_1];

    /* Local Cache of POLICER_CFG_512 registers */
    fm_uint32 policerCfg512[FM10000_POLICER_CFG_512_WIDTH *
                            FM10000_POLICER_CFG_512_ENTRIES_0 *
                            FM10000_POLICER_CFG_512_ENTRIES_1];

    /* Local Cache of POLICER_STATE_4K registers */
    fm_uint32 policerState4k[FM10000_POLICER_STATE_4K_WIDTH *
                             FM10000_POLICER_STATE_4K_ENTRIES_0 *
                             FM10000_POLICER_STATE_4K_ENTRIES_1];

    /* Local Cache of POLICER_STATE_512 registers */
    fm_uint32 policerState512[FM10000_POLICER_STATE_512_WIDTH *
                              FM10000_POLICER_STATE_512_ENTRIES_0 *
                              FM10000_POLICER_STATE_512_ENTRIES_1];

    /* Local Cache of POLICER_CFG registers */
    fm_uint32 policerCfg[FM10000_POLICER_CFG_WIDTH *
                         FM10000_POLICER_CFG_ENTRIES];

    /* local cache of the Ingress VID table */
    fm_uint32 ingressVidTable[FM10000_INGRESS_VID_TABLE_WIDTH *
                              FM10000_INGRESS_VID_TABLE_ENTRIES];

    /* local cache of the Egress VID table */
    fm_uint32 egressVidTable[FM10000_EGRESS_VID_TABLE_WIDTH *
                             FM10000_EGRESS_VID_TABLE_ENTRIES];

    /* local cache of MOD Vlan Tag Vid1 Map */
    fm_uint32 modVlanTagVid1Map[FM10000_MOD_VLAN_TAG_VID1_MAP_WIDTH *
                                FM10000_MOD_VLAN_TAG_VID1_MAP_ENTRIES];

    /* local cache of TE_DATA */
    fm_uint32 teData[FM10000_TE_DATA_WIDTH *
                     FM10000_TE_DATA_ENTRIES_0 *
                     FM10000_TE_DATA_ENTRIES_1];

    /* local cache of TE_LOOKUP */
    fm_uint32 teLookup[FM10000_TE_LOOKUP_WIDTH *
                       FM10000_TE_LOOKUP_ENTRIES_0 *
                       FM10000_TE_LOOKUP_ENTRIES_1];

    /* local cache of TE_DGLORT_MAP */
    fm_uint32 teDglortMap[FM10000_TE_DGLORT_MAP_WIDTH *
                          FM10000_TE_DGLORT_MAP_ENTRIES_0 *
                          FM10000_TE_DGLORT_MAP_ENTRIES_1];

    /* local cache of TE_DGLORT_DEC */
    fm_uint32 teDglortDec[FM10000_TE_DGLORT_DEC_WIDTH *
                          FM10000_TE_DGLORT_DEC_ENTRIES_0 *
                          FM10000_TE_DGLORT_DEC_ENTRIES_1];

    /* local cache of TE_SGLORT_MAP */
    fm_uint32 teSglortMap[FM10000_TE_SGLORT_MAP_WIDTH *
                          FM10000_TE_SGLORT_MAP_ENTRIES_0 *
                          FM10000_TE_SGLORT_MAP_ENTRIES_1];

    /* local cache of TE_SGLORT_DEC */
    fm_uint32 teSglortDec[FM10000_TE_SGLORT_DEC_WIDTH *
                          FM10000_TE_SGLORT_DEC_ENTRIES_0 *
                          FM10000_TE_SGLORT_DEC_ENTRIES_1];

    /* local cache of TE_DEFAULT_DGLORT */
    fm_uint32 teDefaultDglort[FM10000_TE_DEFAULT_DGLORT_WIDTH *
                              FM10000_TE_DEFAULT_DGLORT_ENTRIES];

    /* local cache of TE_DEFAULT_SGLORT */
    fm_uint32 teDefaultSglort[FM10000_TE_DEFAULT_SGLORT_WIDTH *
                              FM10000_TE_DEFAULT_SGLORT_ENTRIES];

    /* local cache of TE_SIP */
    fm_uint32 teSip[FM10000_TE_SIP_WIDTH *
                    FM10000_TE_SIP_ENTRIES_0 *
                    FM10000_TE_SIP_ENTRIES_1];

    /* local cache of TE_VNI */
    fm_uint32 teVni[FM10000_TE_VNI_WIDTH *
                    FM10000_TE_VNI_ENTRIES_0 *
                    FM10000_TE_VNI_ENTRIES_1];

    /* local cache of TE_DEFAULT_L4DST */
    fm_uint32 teDefaultL4Dst[FM10000_TE_DEFAULT_L4DST_WIDTH *
                             FM10000_TE_DEFAULT_L4DST_ENTRIES];

    /* local cache of TE_CFG */
    fm_uint32 teCfg[FM10000_TE_CFG_WIDTH *
                    FM10000_TE_CFG_ENTRIES];

    /* local cache of TE_PORTS */
    fm_uint32 tePorts[FM10000_TE_PORTS_WIDTH *
                      FM10000_TE_PORTS_ENTRIES];

    /* local cache of TE_TUN_HEADER_CFG */
    fm_uint32 teTunHeaderCfg[FM10000_TE_TUN_HEADER_CFG_WIDTH *
                             FM10000_TE_TUN_HEADER_CFG_ENTRIES];

    /* local cache of TE_TRAP_DGLORT */
    fm_uint32 teTrapDglort[FM10000_TE_TRAP_DGLORT_WIDTH *
                           FM10000_TE_TRAP_DGLORT_ENTRIES];

    /* local cache of TE_DEFAULT_NGE_DATA */
    fm_uint32 teDefaultNgeData[FM10000_TE_DEFAULT_NGE_DATA_WIDTH *
                               FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_0 *
                               FM10000_TE_DEFAULT_NGE_DATA_ENTRIES_1];

    /* local cache of TE_DEFAULT_NGE_MASK */
    fm_uint32 teDefaultNgeMask[FM10000_TE_DEFAULT_NGE_MASK_WIDTH *
                               FM10000_TE_DEFAULT_NGE_MASK_ENTRIES];

    /* local cache of TE_EXVET */
    fm_uint32 teExvet[FM10000_TE_EXVET_WIDTH *
                      FM10000_TE_EXVET_ENTRIES];

    /* local cache of TE_DMAC */
    fm_uint32 teDmac[FM10000_TE_DMAC_WIDTH *
                     FM10000_TE_DMAC_ENTRIES];

    /* local cache of TE_SMAC */
    fm_uint32 teSmac[FM10000_TE_SMAC_WIDTH *
                     FM10000_TE_SMAC_ENTRIES];

    /* local cache of TE_TRAP_CONFIG */
    fm_uint32 teTrapCfg[FM10000_TE_TRAP_CONFIG_WIDTH *
                        FM10000_TE_TRAP_CONFIG_ENTRIES];

   /* Other register caches are located in platforms/common/reg-cache/fm10000 */

} fm_fm10000RegCache;

/**************************************************
 * Cached register list
 **************************************************/
extern const fm_cachedRegs *fm10000CachedRegisterList[];

#endif /* __FM_FM10000_API_REGS_CACHE_INT_H */
