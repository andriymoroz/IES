/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_stacking.h
 * Creation Date:   June 11, 2008
 * Description:     Prototypes for managing stacked intra and extra switch
 *                  aggregate systems.
 *
 * Copyright (c) 2005 - 2014, Intel Corporation
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

#ifndef __FM_FM_API_STACKING_H
#define __FM_FM_API_STACKING_H

#define FM_MAX_STACKING_FORWARDING_RULES    256

/** \ingroup macroSynonym
 * @{ */

/** A legacy synonym for ''fmGetStackForwardingRuleList''. */
#define fmGetForwardingRuleList(sw, numForwardingRules, forwardingRuleIDs, max) \
        fmGetStackForwardingRuleList( (sw), (numForwardingRules), (forwardingRuleIDs), (max) )

/** @} (end of Doxygen group) */


/**************************************************/
/** \ingroup typeStruct
 *  Used by ''fmGetStackGlortRangeExt'' and
 *  ''fmSetStackGlortRangeExt'', this structure
 *  specifies how the glort space is allocated.
 **************************************************/
typedef struct _fm_glortRange
{
    /** The first glort to use in the range. It should generally be
     *  a power of 2 to facilitate masked glort ranges for more efficient 
     *  CAM usage. Note that while this member is 32 bits wide, the hardware
     *  supports only 16-bit glorts. (The member was made oversized to 
     *  accommodate future devices with wider glorts.) */
    fm_uint32 glortBase;

    /** Specifies the range of glorts starting with glortBase. The mask 
     *  is an "inverse mask" where the 1 bits identify the wildcarded bits 
     *  in the glort range. The 1 bits in the mask must be contiguous. 
     *  Note that while this member is 32 bits wide, the hardware supports 
     *  only 16-bit glorts. (The member was made oversized to accommodate 
     *  future devices with wider glorts.) */
    fm_uint32 glortMask;

    /** Starting glort number for switch ports. */
    fm_uint32 portBaseGlort;

    /** Number of glorts reserved for switch ports. */
    fm_int    portCount;

    /** Range A length to use when creating CAM entries for switch ports. */
    fm_int    portALength;

    /** Number of glorts reserved for CPU port. */
    fm_int    cpuPortCount;

    /** CPU management glort number (i.e. FIBM glort). */
    fm_uint32 cpuMgmtGlort;

    /** Starting glort number for LAGs. */
    fm_uint32 lagBaseGlort;

    /** Number of glorts reserved for LAG members. */
    fm_int    lagCount;

    /** Starting glort number for multicast groups. */
    fm_uint32 mcastBaseGlort;

    /** Number of glorts reserved for multicast groups. */
    fm_int    mcastCount;

    /** Number of glorts reserved for load balancing groups. */
    fm_uint32 lbgBaseGlort;

    /** Number of glorts reserved for load balancing groups members. */
    fm_int    lbgCount;

} fm_glortRange;


/****************************************************************************/
/** \ingroup typeStruct
 *
 * Defines a forwarding rule for a glort range.
 ****************************************************************************/
typedef struct _fm_forwardRule
{
    /** The base glort value in the range to be forwarded by this rule. */
    fm_uint32  glort;
    
    /** A bit mask identifying the range of glorts to be forwarded by this
     *  rule. The mask is an "inverse mask" where the 1 bits specify the 
     *  wildcarded bits in the glort range. */
    fm_uint32  mask;

    /** The destination logical port to which the range of glorts should
     *  be forwarded. */
    fm_int     logicalPort;

} fm_forwardRule;


/*****************************************************************************
 * Glort space and glort management functions
 *****************************************************************************/

fm_status fmSetStackGlortRange(fm_int sw,
                               fm_uint32 glortBase, 
                               fm_uint32 mask);

fm_status fmGetStackGlortRange(fm_int sw, 
                               fm_uint32 *glortBase, 
                               fm_uint32 *mask);

fm_status fmSetStackGlortRangeExt(fm_int         sw,
                                  fm_glortRange *glortRange);

fm_status fmGetStackGlortRangeExt(fm_int         sw,
                                  fm_glortRange *glortRange);

fm_status fmGetStackGlort(fm_int sw, 
                          fm_int port, 
                          fm_uint32 *glort);

fm_status fmCreateStackLogicalPort(fm_int sw, 
                                   fm_uint32 glort, 
                                   fm_int *logicalPort);

fm_status fmDeleteStackLogicalPort(fm_int sw, 
                                   fm_int port);

fm_status fmSetStackLogicalPortState(fm_int sw, fm_int port, fm_int mode);

fm_status fmGetStackLogicalPortState(fm_int sw, fm_int port, fm_int *mode);


/*****************************************************************************
 * Glort forwarding rule management functions
 *****************************************************************************/

fm_status fmCreateStackForwardingRule(fm_int sw, 
                                      fm_int *forwardingRuleID, 
                                      fm_forwardRule *rule);

fm_status fmGetStackForwardingRule(fm_int sw, 
                                   fm_int forwardingRuleID, 
                                   fm_forwardRule *rule);

fm_status fmUpdateStackForwardingRule(fm_int          sw,
                                      fm_int *        forwardingRuleID,
                                      fm_forwardRule *newRule);

fm_status fmDeleteStackForwardingRule(fm_int sw, fm_int forwardingRuleID);

fm_status fmGetStackForwardingRuleFirst(fm_int sw, 
                                        fm_int *firstRuleId, 
                                        fm_forwardRule *firstRule);

fm_status fmGetStackForwardingRuleNext(fm_int sw, 
                                       fm_int currentRuleId,
                                       fm_int *nextRuleId, 
                                       fm_forwardRule *nextRule);

fm_status fmGetStackForwardingRuleList(fm_int sw,
                                       fm_int *numForwardingRules,
                                       fm_int *forwardingRuleIDs,
                                       fm_int max); 

/*****************************************************************************
 * Stacking APIs
 *****************************************************************************/

fm_status fmAllocateStackMcastGroups(fm_int   sw,
                                     fm_uint  startGlort,
                                     fm_uint  glortCount,
                                     fm_int * baseMcastGroup,
                                     fm_int * numMcastGroups,
                                     fm_int * step);

fm_status fmFreeStackMcastGroups(fm_int sw, fm_int baseMcastGroup);

fm_status fmCreateStackMcastGroup(fm_int sw, fm_int mcastGroup);

fm_status fmCreateStackMcastGroupExt(fm_int  sw,
                                     fm_int  logicalPort,
                                     fm_int *mcastGroup);

fm_status fmAllocateStackLAGs(fm_int     sw,
                              fm_uint    startGlort,
                              fm_uint    glortCount,
                              fm_int    *baseLagNumber,
                              fm_int    *numLags,
                              fm_int    *step);

fm_status fmFreeStackLAGs(fm_int sw, fm_int baseLagNumber);

fm_status fmCreateStackLAG(fm_int sw, fm_int lagNumber);

fm_status fmAllocateStackLBGs(fm_int     sw,
                              fm_uint    startGlort,
                              fm_uint    glortCount,
                              fm_int    *baseLbgNumber,
                              fm_int    *numLbgs,
                              fm_int    *step);

fm_status fmFreeStackLBGs(fm_int sw, fm_int baseLbgNumber);

fm_status fmCreateStackLBG(fm_int sw, fm_int lbgNumber);

fm_status fmCreateStackLBGExt(fm_int sw, 
                              fm_int lbgNumber, 
                              fm_LBGParams *params);

fm_status fmGetStackLBGHandle(fm_int  sw,
                              fm_int  lbgNumber,
                              fm_int *lbgHandle);

#endif /* __FM_FM_API_STACKING_H */
