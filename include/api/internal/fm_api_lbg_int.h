/* vim:ts=4:sw=4:expandtab
 * (No tabs, indent level is 4 spaces)  */
/*****************************************************************************
 * File:            fm_api_lbg_int.h
 * Creation Date:   March 31, 2008
 * Description:     Prototypes for managing load balancing groups.  This
 *                  API is not valid on FM2000 deviecs.  This header
 *                  defines the internal helper functions related to
 *                  load balancing groups.
 *
 * Copyright (c) 2005 - 2011, Intel Corporation
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

#ifndef __FM_FM_API_LBG_INT_H
#define __FM_FM_API_LBG_INT_H

#define FM_TAKE_LBG_LOCK(sw) \
    fmCaptureLock(&fmRootApi->fmSwitchStateTable[(sw)]->lbgInfo.lbgLock, \
                  FM_WAIT_FOREVER);

#define FM_DROP_LBG_LOCK(sw) \
    fmReleaseLock(&fmRootApi->fmSwitchStateTable[(sw)]->lbgInfo.lbgLock);


typedef enum
{
    /** The load balancer group's RouteData is an ARP table index. */
    FM_LBG_ROUTE_ARP = 0,

    /** The load balancer group's RouteData is a GloRT. */
    FM_LBG_ROUTE_GLORT

} fm_LBGRouteType;


/***************************************************
 * This structure contains the state information
 * for a single member of a load balancing group.
 **************************************************/
typedef struct _fm_intLBGMember
{
    /* The logical port number of the member. */
    fm_int  lbgMemberPort;

    /* The cardinal port mask of the member port.
     * Will be zero if the member is a remote port.
     * (FM4000 only) */
    fm_uint32 memberPortMask;

    /* Port mode, specified via the FM_LBG_PORT_MODE port attribute
     * and the _fm_lbgPortMode enumeration. */
    fm_int  mode;

    /* Logical port number to which traffic is to be redirected.
     * Set via the FM_LBG_PORT_REDIRECT_TARGET port attribute. */
    fm_int  redirectTarget;

    /* Whether this port is used in standby mode */
    fm_bool standbyUsed;

    /* Points to the member for the target, if any */
    struct _fm_intLBGMember *redirectTargetPtr;

    /* Points to the LBG group this port belongs to */
    struct _fm_LBGGroup *group;

    /* LBG member lists */
    FM_DLL_DEFINE_NODE(_fm_intLBGMember, nextMember, prevMember);

} fm_intLBGMember;

/***************************************************
 * This structure contains the state information
 * for a single load balancing group.
 **************************************************/
typedef struct _fm_LBGGroup
{
    /* The logical port number of this group */
    fm_int                lbgPort;

    /* The number of bins to be used */
    fm_int                numBins;

    /* The current mode for this LBG */
    fm_LBGMode            lbgMode;

    /* Redirect mode */
    fm_int                redirectMode;

    /* The state for this LBG */
    fm_int                state;

    /* The member list */
    FM_DLL_DEFINE_LIST(_fm_intLBGMember, firstMember, lastMember);

    /* In redirect-mode, used to keep the last member striped on. */
    fm_intLBGMember *        lastStripeMember;
    
    /* Chip specific state structure. This pointer points to a fmXXXX_LBGGroup
     * structure, where XXXX is the chip family. E.g. fm4000_LBGGroup,
     * fm10000_LBGGroup */
    void *                extension;

    /***************************************** 
     * The following structure members are
     * used by FM4000 only
     ****************************************/

    /* Whether redirect mode is set on the LBG. */
    fm_bool               redirectModeSet;

    /* The configured mapped size. */
    fm_int                mapSize;

    /* The member count. */
    fm_int                numMembers;

    /***************************************** 
     * The following structure members are
     * common to FM6000 and FM10000.
     ****************************************/

    /* The distribution table to use when the mode is FM_LBG_MODE_REDIRECT.
     * This takes the place of the generic distribution field, 
     * and allows for a dynamically sized bin map. Memory for this table is
     * allocated in fmXXXXCreateLBG(). Note that the memory is not allocated 
     * for the FM_LBG_MODE_MAPPED mode*/ 
    fm_int *userDistribution;

    /* The distribution table after failover processing, maps directly
     * to the dest entries in the hardware. Memory for this table is
     * allocated in fmXXXXCreateLBG(). */
    fm_int *hwDistribution;

    /* Contains the distribution map configured by
     * FM_LBG_DISTRIBUTION_MAP_RANGE_V2. */
    fm_LBGMember *hwDistributionV2;

    /* Number of active members */
    fm_int numActive;

    /* Number of failover members */
    fm_int numFailover;

    /* Logical port for L234 LBG */
    fm_int lbgLogicalPort;

} fm_LBGGroup;

/***************************************************
 * This structure maintains all of the soft-state of
 * the load balancing group.
 **************************************************/
typedef struct
{
    /* A tree of LBGs keyed by their logical port. Tree values point to
     * a fm_LBGGroup structure. This tree is managed by chip specific code,
     * but can be accessed by generic code. */
    fm_tree    groups;

    /* The current global mode */
    fm_lbgMode mode;

    /* LBG lock to protect LBG structures */
    fm_lock    lbgLock;

} fm_LBGInfo;


fm_status fmGetLBGRouteData(fm_int           sw,
                            fm_int           lbgNumber,
                            fm_LBGRouteType *routeType,
                            fm_int *         routeData,
                            fm_int *         dataCount);

fm_status fmAllocateLBGDataStructures(fm_switch *switchPtr);
fm_status fmFreeLBGDataStructures(fm_switch *switchPtr);
fm_status fmCreateLBGInt(fm_int sw, 
                         fm_int *lbgNumber, 
                         fm_LBGParams *params, 
                         fm_bool stacking);
fm_status fmAllocateLBGsInt(fm_int     sw,
                            fm_uint    startGlort,
                            fm_uint    glortSize,
                            fm_int    *baseLbgNumber,
                            fm_int    *numLbgs,
                            fm_int    *step);
fm_status fmFreeLBGsInt(fm_int sw, fm_int baseLbgNumber);

fm_status fmCommonFindLBGMember(fm_LBGGroup *  group, 
                                fm_int         port, 
                                fm_intLBGMember **member);

fm_status fmCommonResetLBGDistributionForRedirect(fm_int sw, fm_LBGGroup *group);
fm_status fmCommonHandleLBGPortModeTransition(fm_int sw, 
                                              fm_LBGGroup *group, 
                                              fm_intLBGMember *member, 
                                              fm_int newMode,
                                              fm_bool *hwDistChanged);

fm_status fmResetLBGMember(fm_LBGMember *lbgMember);
fm_status fmCopyLBGMember(fm_LBGMember *destLbgMember,
                          fm_LBGMember *srcLbgMember);




#endif /* __FM_FM_API_LBG_INT_H */
