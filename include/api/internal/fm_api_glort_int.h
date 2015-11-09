/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces) */
/*****************************************************************************
 * File:           fm_api_glort_int.h
 * Creation Date:  Jul 8, 2015
 * Description:    Global Resource Tag (GloRT) management definitions.
 *
 * Copyright (c) 2005 - 2015, Intel Corporation
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


#ifndef __FM_FM_API_GLORT_INT_H
#define __FM_FM_API_GLORT_INT_H



/***************************************************
 * Additional glort management information.
 **************************************************/
typedef struct
{
    /* Base address for CPU glort range. */
    fm_uint32   cpuBase;

    /* CAM mask for CPU glort range. */
    fm_uint32   cpuMask;

    /* Base address for special glort range. */
    fm_uint32   specialBase;

    /* CAM mask for special glort range. */
    fm_uint32   specialMask;

    /* Number of glorts in special glort range. */
    fm_int      specialSize;

    /* Number of glorts expressed as a power of two. */
    fm_int      specialALength;

} fm_glortInfo;



/****************************************************************************/
/** \ingroup intTypeEnum
 *
 * Identifies the type of GloRTs.
 ****************************************************************************/
typedef enum
{
    FM_GLORT_TYPE_UNSPECIFIED = 0,

    /** A GloRT range available for single physical ports on the local switch.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_PORT,

    /** A GloRT range available for a LAG.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_LAG,

    /** A GloRT range available for multicasting.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_MULTICAST,

    /** A GloRT range available for load balancing.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_LBG,

    /** A GloRT range available for the CPU port.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_CPU,

    /** A GloRT range available for a custom port type.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_SPECIAL,

    /** A GloRT range available for virtual ports. A virtual port is only
     *  identifiable/reachable using a GloRT. Virtual GloRTs are typically
     *  used to identify PF or VF (e.g. VM) on an SR-IOV device.
     *
     *  \chips  FM10000 */
    FM_GLORT_TYPE_PEP,

    /* ----  Add new entries above this line.  ---- */

    /** UNPUBLISHED: For internal use only. */
    FM_GLORT_TYPE_MAX

} fm_glortType;

/***************************************************
 * Constants representing boundaries and sizes for
 * the glort space.
 **************************************************/

/*
 * 0x0000 - 0x00ff  : Physical ports.
 *
 * The glort port size (number of destination table
 * entries) is defined at the switch level.
 *
 * Used to initialize portBaseGlort.
 */
#define FM_GLORT_PORT_BASE(base)        (base)

/* The maximum glort value. */
#define FM_MAX_GLORT                    0xFFFF


/***************************************************
 * Constants representing field values for the
 * GLORT_RAM table.
 **************************************************/

#define FM_GLORT_ENTRY_TYPE_STRICT      3
#define FM_GLORT_ENTRY_TYPE_HASHED      2
#define FM_GLORT_ENTRY_TYPE_ISL         0

#define FM_GLORT_ENTRY_HASH_A           0
#define FM_GLORT_ENTRY_HASH_B           1

/*******************************************************************
 * State bits for the glortState array.
 *******************************************************************/

#define FM_GLORT_STATE_UNUSED       0x00    /* Neither used nor reserved */
#define FM_GLORT_STATE_IN_USE       0x01    /* In use */
#define FM_GLORT_STATE_RESV_MCG     0x02    /* Reserved for MCG */
#define FM_GLORT_STATE_RESV_LAG     0x04    /* Reserved for LAG */
#define FM_GLORT_STATE_RESV_LBG     0x08    /* Reserved for LBG */
#define FM_GLORT_STATE_FREE_PEND    0x10    /* Free pending */

#define FM_GLORT_STATE_USED_BITS    \
        (FM_GLORT_STATE_IN_USE | FM_GLORT_STATE_FREE_PEND)

/***************************************************
 * Utility macros.
 **************************************************/

#define FM_GET_GLORT_NUMBER(swptr, p)       ( (swptr)->portTable[p]->glort )

/***************************************************
 * Function prototypes.
 **************************************************/

fm_status fmVerifyGlortRange(fm_uint32 glort, fm_int size);
fm_status fmCheckGlortRangeStateInt(fm_switch *switchPtr,
                                    fm_uint32  start,
                                    fm_int     numGlorts,
                                    fm_int     state,
                                    fm_int     mask);
fm_status fmCheckGlortRangeState(fm_switch *switchPtr,
                                 fm_uint32  start,
                                 fm_int     numGlorts,
                                 fm_int     state);
fm_status fmCheckGlortRangeType(fm_switch *  switchPtr,
                                fm_uint32    start,
                                fm_int       numGlorts,
                                fm_glortType glortType);
fm_status fmRequestGlortRange(fm_int       sw,
                              fm_uint32    start,
                              fm_int       numGlorts,
                              fm_glortType glortType);
fm_status fmReleaseGlortRangeInt(fm_int       sw,
                                 fm_uint32    start,
                                 fm_int       numGlorts,
                                 fm_glortType glortType,
                                 fm_bool      pending);
fm_status fmReleaseGlortRange(fm_int       sw,
                              fm_uint32    start,
                              fm_int       numGlorts,
                              fm_glortType glortType);
fm_status fmReserveGlortRange(fm_int       sw,
                              fm_uint32    start,
                              fm_int       numGlorts,
                              fm_glortType glortType);
fm_status fmUnreserveGlortRange(fm_int       sw,
                                fm_uint32    start,
                                fm_int       numGlorts,
                                fm_glortType glortType);
fm_status fmFindFreeGlortRangeInt(fm_int       sw,
                                  fm_int       numGlorts,
                                  fm_glortType glortType,
                                  fm_uint32    rangeStart,
                                  fm_int       rangeSize,
                                  fm_bool      reserved,
                                  fm_uint32 *  startGlort);
fm_status fmFindFreeGlortRange(fm_int       sw,
                               fm_int       numGlorts,
                               fm_glortType glortType,
                               fm_uint32 *  startGlort);
fm_status fmDbgDumpGlortRanges(fm_int sw);

#endif /* __FM_FM_API_GLORT_INT_H */
