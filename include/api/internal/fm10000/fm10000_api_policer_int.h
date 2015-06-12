/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:           fm10000_api_policer_int.h
 * Creation Date:  May 31, 2013
 * Description:    Low-level API for manipulating the Policers.
 *
 * Copyright (c) 2013 - 2014, Intel Corporation
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

#ifndef __FM_FM10000_FM_API_POLICER_INT_H
#define __FM_FM10000_FM_API_POLICER_INT_H

/* Maximum number for each bank types. */
#define FM_FM10000_MAX_POLICER_4K_INDEX      4095
#define FM_FM10000_MAX_POLICER_512_INDEX      511

/**************************************************/
/** \ingroup typeEnum
 *  Specifies which policer bank to take action on.
 **************************************************/
typedef enum
{
    /** Select the first 4K policer bank. */
    FM_FM10000_POLICER_BANK_4K_1 = 0,

    /** Select the second 4K policer bank. */
    FM_FM10000_POLICER_BANK_4K_2,

    /** Select the first 512 policer bank. */
    FM_FM10000_POLICER_BANK_512_1,
    
    /** Select the second 512 policer bank. */
    FM_FM10000_POLICER_BANK_512_2,

    /** UNPUBLISHED: For internal use only. */
    FM_FM10000_POLICER_BANK_MAX

} fm_fm10000PolicerBank;



/** Defines the ownership for the Policer bank resources */
typedef struct _fm_fm10000PolOwnershipInfo
{
    /** For each counter/policer bank, the owner. */
    fm_ffuOwnerType bankOwner[FM_FM10000_POLICER_BANK_MAX];

} fm_fm10000PolOwnershipInfo;


/*****************************************************************************
 * Public Functions
 *****************************************************************************/

fm_status fm10000PolicerInit(fm_int sw);

fm_status fm10000ConvertPolicerRate(fm_uint32 rate,
                                    fm_uint*  mantissa,
                                    fm_uint*  exponent);

fm_status fm10000ConvertPolicerCapacity(fm_uint32 capacity,
                                        fm_uint * mantissa,
                                        fm_uint * exponent);

fm_status fm10000SetPolicerCounter(fm_int                sw,
                                   fm_fm10000PolicerBank bank,
                                   fm_uint16             index,
                                   fm_uint64             frameCount,
                                   fm_uint64             byteCount);

fm_status fm10000GetPolicerCounter(fm_int                sw,
                                   fm_fm10000PolicerBank bank,
                                   fm_uint16             index,
                                   fm_uint64 *           frameCount,
                                   fm_uint64 *           byteCount);

fm_status fm10000SetPolicerCounters(fm_int                sw,
                                    fm_fm10000PolicerBank bank,
                                    fm_uint16             firstIndex,
                                    fm_uint16             numCounters,
                                    const fm_uint64 *     frameCounts,
                                    const fm_uint64 *     byteCounts);

fm_status fm10000GetPolicerCounters(fm_int                sw,
                                    fm_fm10000PolicerBank bank,
                                    fm_uint16             firstIndex,
                                    fm_uint16             numCounters,
                                    fm_uint64 *           frameCounts,
                                    fm_uint64 *           byteCounts);

fm_status fm10000SetPolicer(fm_int                    sw,
                            fm_fm10000PolicerBank     bank,
                            fm_uint16                 index,
                            const fm_ffuPolicerState *committed,
                            const fm_ffuPolicerState *excess);

fm_status fm10000GetPolicer(fm_int                sw,
                            fm_fm10000PolicerBank bank,
                            fm_uint16             index,
                            fm_ffuPolicerState *  committed,
                            fm_ffuPolicerState *  excess);

fm_status fm10000SetPolicers(fm_int                    sw,
                             fm_fm10000PolicerBank     bank,
                             fm_uint16                 firstIndex,
                             fm_uint16                 numPolicers,
                             const fm_ffuPolicerState *committed,
                             const fm_ffuPolicerState *excess);

fm_status fm10000GetPolicers(fm_int                sw,
                             fm_fm10000PolicerBank bank,
                             fm_uint16             firstIndex,
                             fm_uint16             numPolicers,
                             fm_ffuPolicerState *  committed,
                             fm_ffuPolicerState *  excess);

fm_status fm10000SetPolicerConfig(fm_int                sw,
                                  fm_fm10000PolicerBank bank,
                                  fm_uint16             indexLastPolicer,
                                  fm_ffuColorSource     ingressColorSource,
                                  fm_bool               markDSCP,
                                  fm_bool               markSwitchPri,
                                  fm_bool               useCache);

fm_status fm10000GetPolicerConfig(fm_int                sw,
                                  fm_fm10000PolicerBank bank,
                                  fm_uint16 *           indexLastPolicer,
                                  fm_ffuColorSource *   ingressColorSource,
                                  fm_bool *             markDSCP,
                                  fm_bool *             markSwitchPri,
                                  fm_bool               useCache);

fm_status fm10000SetPolicerDSCPDownMap(fm_int         sw,
                                       const fm_byte *table,
                                       fm_bool        useCache);

fm_status fm10000GetPolicerDSCPDownMap(fm_int   sw,
                                       fm_byte *table,
                                       fm_bool  useCache);

fm_status fm10000SetPolicerSwPriDownMap(fm_int         sw,
                                        const fm_byte *table,
                                        fm_bool        useCache);

fm_status fm10000GetPolicerSwPriDownMap(fm_int   sw,
                                        fm_byte *table,
                                        fm_bool  useCache);

fm_status fm10000SetPolicerOwnership(fm_int                sw,
                                     fm_ffuOwnerType       owner,
                                     fm_fm10000PolicerBank policerBank);

fm_status fm10000GetPolicerOwnership(fm_int                sw,
                                     fm_ffuOwnerType *     owner,
                                     fm_fm10000PolicerBank policerBank);

fm_status fm10000UpdatePolicer(fm_int sw, fm_int policer);

void fm10000DbgDumpPolicers(fm_int sw);
 
#endif  /* __FM_FM10000_FM_API_POLICER_INT_H */

