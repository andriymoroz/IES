/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_ffu_int.h
 * Creation Date:  April 29, 2013
 * Description:    Low-level API for manipulating the Filtering &
 *                 Forwarding Unit
 *
 * Copyright (c) 2013, Intel Corporation
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

#ifndef __FM_FM10000_FM_API_FFU_INT_H
#define __FM_FM10000_FM_API_FFU_INT_H

#define FM_FM10000_NUM_FFU_SLICES              32

#define FM_FM10000_FFU_RXTAG_FTAG_BIT          0
#define FM_FM10000_FFU_RXTAG_VLAN1_BIT         1
#define FM_FM10000_FFU_RXTAG_VLAN2_BIT         2
#define FM_FM10000_FFU_RXTAG_VLAN2_FIRST_BIT   3
#define FM_FM10000_FFU_RXTAG_IPV4_BIT          4
#define FM_FM10000_FFU_RXTAG_IPV6_BIT          5
#define FM_FM10000_FFU_RXTAG_MPLS_BIT          6
#define FM_FM10000_FFU_RXTAG_CUSTOM_BIT        7

#define FM_FM10000_FFU_IP_MISC_OPTION_BIT      0
#define FM_FM10000_FFU_IP_MISC_DO_NOT_FRAG_BIT 1
#define FM_FM10000_FFU_IP_MISC_HEAD_FRAG_BIT   2
#define FM_FM10000_FFU_IP_MISC_MORE_FRAG_BIT   3

/** Defines the ownership for the FFU resources */
typedef struct _fm_fm10000ffuOwnershipInfo
{
    /** For FFU slice, the owner. */
    fm_ffuOwnerType sliceOwner[FM_FM10000_NUM_FFU_SLICES];

} fm_fm10000FfuOwnershipInfo;


/**************************************************/
/** \ingroup typeStruct
 *  Referenced by ''fm_fm10000FfuSliceKey'', this 
 *  structure represents the case or combination
 *  of cases to which a rule applies. 
 **************************************************/
typedef struct _fm_fm10000FfuCase
{
    /** Case value (4 bits) */
    fm_byte value;

    /** Case mask (4 bits) */
    fm_byte mask;

} fm_fm10000FfuCase;


/**************************************************/
/** \ingroup typeStruct
 *  Used by ''fm10000SetFFURule'' and 
 *  ''fm10000SetFFURules'', this structure specifies 
 *  the key on which an FFU rule should match for a 
 *  single slice.  An array of these structures must 
 *  be used for a chain of slices.
 **************************************************/
typedef struct _fm_fm10000FfuSliceKey
{
    /** Indicates for what case or combination of cases the rule applies to.
     *  This field is only uses on scenario that specified a case location
     *  of ''FM_FFU_CASE_TOP_LOW_NIBBLE'' or ''FM_FFU_CASE_TOP_HIGH_NIBBLE''. */
    fm_fm10000FfuCase   kase;
    
    /** Value that will be compared against the frame data to detect a match,
     *  subject to keyMask. Usable length of this key depends on the case
     *  location for the specified scenario. 40-bit can be defined if the
     *  case location selected is ''FM_FFU_CASE_NOT_MAPPED''. Other
     *  configuration can only make uses of 36-bit key. */
    fm_uint64           key;
    
    /** Mask that determines which bits in key will be compared to frame data
     *  and which are don't-care bits. A 1 indicates compare, a 0 indicates
     *  don't-care. */
    fm_uint64           keyMask;
    
} fm_fm10000FfuSliceKey;


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

fm_status fm10000MoveFFURules(fm_int                 sw,
                              const fm_ffuSliceInfo *slice,
                              fm_uint16              fromIndex,
                              fm_uint16              nRules,
                              fm_uint16              toIndex);

fm_status fm10000SetFFUSliceOwnership(fm_int          sw,
                                      fm_ffuOwnerType owner,
                                      fm_int          firstSlice,
                                      fm_int          lastSlice);

fm_status fm10000GetFFUSliceOwnership(fm_int          sw,
                                      fm_ffuOwnerType owner,
                                      fm_int *        firstSlice,
                                      fm_int *        lastSlice);

fm_status fm10000GetFFUSliceOwner(fm_int           sw,
                                  fm_int           slice,
                                  fm_ffuOwnerType *owner);

fm_status fm10000FFUInit(fm_int sw);


/* slice functions */
fm_status fm10000SetFFUMasterValid(fm_int    sw,
                                   fm_uint32 validIngress,
                                   fm_uint32 validEgress,
                                   fm_bool   useCache);

fm_status fm10000GetFFUMasterValid(fm_int     sw,
                                   fm_uint32 *validIngress,
                                   fm_uint32 *validEgress,
                                   fm_bool    useCache);

fm_status fm10000ConfigureFFUSlice(fm_int                 sw,
                                   const fm_ffuSliceInfo *slice,
                                   fm_bool                useCache);

fm_status fm10000UnconfigureFFUSlice(fm_int                 sw,
                                     const fm_ffuSliceInfo *slice,
                                     fm_bool                useCache);

fm_status fm10000SetFFURule(fm_int                       sw,
                            const fm_ffuSliceInfo *      slice,
                            fm_uint16                    ruleIndex,
                            fm_bool                      valid,
                            const fm_fm10000FfuSliceKey *ruleKey,
                            const fm_ffuAction *         actionList,
                            fm_bool                      live,
                            fm_bool                      useCache);

fm_status fm10000SetFFURules(fm_int                        sw,
                             const fm_ffuSliceInfo *       slice,
                             fm_uint16                     ruleIndex,
                             fm_uint16                     nRules,
                             const fm_bool *               valid,
                             const fm_fm10000FfuSliceKey **ruleKeys,
                             const fm_ffuAction **         actionLists,
                             fm_bool                       live,
                             fm_bool                       useCache);

fm_status fm10000SetFFURuleValid(fm_int                 sw,
                                 const fm_ffuSliceInfo *slice,
                                 fm_uint16              ruleIndex,
                                 fm_bool                valid,
                                 fm_bool                useCache);

fm_status fm10000GetFFURule(fm_int                 sw,
                            const fm_ffuSliceInfo *slice,
                            fm_uint16              ruleIndex,
                            fm_bool *              valid,
                            fm_fm10000FfuSliceKey *ruleKey,
                            fm_ffuAction *         actionList,
                            fm_bool                useCache);

fm_status fm10000GetFFURules(fm_int                  sw,
                             const fm_ffuSliceInfo * slice,
                             fm_uint16               ruleIndex,
                             fm_uint16               nRules,
                             fm_bool *               valid,
                             fm_fm10000FfuSliceKey **ruleKeys,
                             fm_ffuAction **         actionLists,
                             fm_bool                 useCache);

fm_status fm10000CopyFFURules(fm_int                 sw,
                              const fm_ffuSliceInfo *slice,
                              fm_uint16              fromIndex,
                              fm_uint16              nRules,
                              fm_uint16              toIndex,
                              fm_bool                live,
                              fm_bool                useCache);

fm_status fm10000ConfigureFFUEaclChunk(fm_int     sw,
                                       fm_byte    chunk,
                                       fm_uint32  validScenarios,
                                       fm_uint64  dstPhysicalPortMask,
                                       fm_bool    cascade,
                                       fm_bool    useCache);

fm_status fm10000SetFFUEaclAction(fm_int    sw,
                                  fm_uint16 ruleIndex,
                                  fm_bool   drop,
                                  fm_bool   useCache);

fm_status fm10000GetFFUEaclAction(fm_int     sw,
                                  fm_uint16  ruleIndex,
                                  fm_bool   *drop,
                                  fm_bool    useCache);

fm_status fm10000SetFFUEaclCounter(fm_int    sw,
                                   fm_byte   port,
                                   fm_uint64 dropCount);

fm_status fm10000GetFFUEaclCounter(fm_int     sw,
                                   fm_byte    port,
                                   fm_uint64 *dropCount);

void fm10000DbgDumpFFU(fm_int  sw, 
                       fm_bool onlyValidSlices,
                       fm_bool onlyValidRules);


#endif /* __FM_FM10000_FM_API_FFU_INT_H */
